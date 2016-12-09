#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>

#include <android/log.h>

#include "thornyreader.h"
#include "StProtocol.h"
#include "StSocket.h"

#include "crengine.h"
#include "epubfmt.h"
#include "pdbfmt.h"
#include "lvstream.h"

// Yep, twice include single header with different define
#include "fb2def.h"
#define XS_IMPLEMENT_SCHEME 1
#include "fb2def.h"
#undef XS_IMPLEMENT_SCHEME

#include "CreBridge.h"

class AndroidLogger : public CRLog
{
public:
	AndroidLogger()	{ }

protected:
	virtual void log(const char* lvl, const char* msg, va_list args)
	{
		int level = ANDROID_LOG_DEBUG;
		if (!strcmp(lvl, "FATAL"))
			level = ANDROID_LOG_FATAL;
		else if (!strcmp(lvl, "ERROR"))
			level = ANDROID_LOG_ERROR;
		else if (!strcmp(lvl, "WARN"))
			level = ANDROID_LOG_WARN;
		else if (!strcmp(lvl, "INFO"))
			level = ANDROID_LOG_INFO;
		else if (!strcmp(lvl, "DEBUG"))
			level = ANDROID_LOG_DEBUG;
		else if (!strcmp(lvl, "TRACE"))
			level = ANDROID_LOG_VERBOSE;
		__android_log_vprint(level, "axy.cre", msg, args);
	}
};

class AndroidDrawBuf : public LVColorDrawBuf
{
public:
    void ToAndroidBitmap()
    {
        if (GetBitsPerPixel() == 32) {
            //Convert Cre colors to Android
            int size = GetWidth() * GetHeight();
            for (lUInt8* p = _data; --size >= 0; p+=4) {
                //Invert A
                p[3] ^= 0xFF;
                //Swap R and B
                lUInt8 temp = p[0];
                p[0] = p[2];
                p[2] = temp;
            }
        }
    }
	AndroidDrawBuf(int dx, int dy, lUInt8* pixels, int bpp)
            : LVColorDrawBuf(dx, dy, pixels, bpp) {}
};

static void AddString(CmdResponse& response, lString16 str16)
{
	lString8 str8 = UnicodeToUtf8(str16);
	uint32_t size = (uint32_t) str8.size();
	//We will place null-terminator at the string end
	size++;
	CmdData* cmd_data = new CmdData();
	unsigned char* str_buffer = (unsigned char*) cmd_data->newByteArray(size);
	memcpy(str_buffer, str8.c_str(), (size - 1));
	str_buffer[size - 1] = 0;
	response.addData(cmd_data);
}

static inline int CeilToEvenInt(int n)
{
	return n += (n & 1);
}

static inline int FloorToEvenInt(int n)
{
	return n &= ~1;
}

static inline int ExportPagesCount(int columns, int pages)
{
	if (columns == 2) {
		return CeilToEvenInt(pages) / columns;
	}
	return pages;
}

static inline int ExportPage(int columns, int page)
{
	if (columns == 2) {
		return FloorToEvenInt(page) / columns;
	}
	return page;
}

static inline int ImportPage(int columns, int page)
{
	return page* columns;
}

static const int ALLOWED_INTERLINE_SPACES[] =
{
        70, 75, 80, 85, 90, 95, 100, 105, 110, 115,
        120, 125, 130, 135, 140, 145, 150, 160, 180, 200
};

static const int ALLOWED_FONT_SIZES[] =
{
		12, 13, 14, 15, 16, 17, 18, 19, 20,
		21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		42, 44, 46, 48, 50,
		52, 54, 56, 58, 60,
		62, 64, 66, 68, 70
};

static int GetClosestValueInArray(const int values[], int values_size, int value)
{
	int closest_value = -1;
	int smallest_delta = -1;
	for (int i = 0; i < values_size; i++) {
		int delta = values[i] - value;
		if (delta < 0) {
			delta = -delta;
		}
		if (smallest_delta == -1 || smallest_delta > delta) {
			smallest_delta = delta;
			closest_value = values[i];
		}
	}
	return closest_value;
}

void CreBridge::processFonts(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PDF_FONTS;

    CmdDataIterator iter(request.first);
    while (iter.hasNext()) {
        uint32_t font_family;
        if (!iter.getInt(&font_family).isValid()) {
            CRLog::error("No font family found");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        uint8_t* fonts[4];
        iter.getByteArray(&fonts[0])
                .getByteArray(&fonts[1])
                .getByteArray(&fonts[2])
                .getByteArray(&fonts[3]);
        if (!iter.isValid()) {
            CRLog::error("processFonts no fonts");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        for (int i = 0; i < 4; i++) {
            const char* const_font = reinterpret_cast<const char*>(fonts[i]);
            if (strlen(const_font) == 0) {
                continue;
            }
            lString16 font = lString16(const_font);
            fontMan->RegisterFont(UnicodeToUtf8(font));
        }
    }
}

void CreBridge::processConfig(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_SET_CONFIG;
    CmdDataIterator iter(request.first);
    if (!doc_view_) {
        doc_view_ = new LVDocView();
    }
    while (iter.hasNext()) {
        uint32_t key;
        uint8_t* temp_val;
        iter.getInt(&key).getByteArray(&temp_val);
        if (!iter.isValid()) {
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        const char* val = reinterpret_cast<const char*>(temp_val);

        if (key == CONFIG_CRENGINE_VIEWPORT_WIDTH) {
            int int_val = atoi(val);
            doc_view_->Resize(int_val, doc_view_->height_);
        } else if (key == CONFIG_CRENGINE_VIEWPORT_HEIGHT) {
            int int_val = atoi(val);
            doc_view_->Resize(doc_view_->width_, int_val);
        } else if (key == CONFIG_CRENGINE_FONT_ANTIALIASING) {
            int int_val = atoi(val);
            if (int_val < 0 || int_val > 2) {
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            fontMan->SetAntialiasMode(int_val);
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_FONT_GAMMA) {
            double gamma = 1.0;
            if (sscanf(val, "%lf", &gamma) == 1) {
                fontMan->SetGamma(gamma);
            }
        } else if (key == CONFIG_CRENGINE_PAGES_COLUMNS) {
            int int_val = atoi(val);
            if (int_val < 1 || int_val > 2) {
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            if (doc_view_->page_columns_ != int_val) {
                doc_view_->page_columns_ = int_val;
                doc_view_->UpdateLayout();
                doc_view_->RequestRender();
                doc_view_->position_is_set_ = false;
            }
        } else if (key == CONFIG_CRENGINE_TXT_SMART_FORMAT) {
            int int_val = atoi(val);
            if (int_val < 0 || int_val > 1) {
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            if (doc_view_->config_txt_smart_format_ != int_val) {
                doc_view_->config_txt_smart_format_ = int_val;
                doc_view_->GetCrDom()->setDocFlag(
                        DOC_FLAG_TXT_NO_SMART_FORMAT,
                        !int_val);
                if (doc_view_->doc_format_ == doc_format_txt) {
                    doc_view_->RequestRender();
                }
            }
        } else if (key == CONFIG_CRENGINE_FONT_COLOR) {
            doc_view_->text_color_ = atoi(val) & 0xFFFFFF;
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_BACKGROUND_COLOR) {
            doc_view_->background_color_ = atoi(val) & 0xFFFFFF;
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_MARGIN_TOP
                   || key == CONFIG_CRENGINE_MARGIN_LEFT
                   || key == CONFIG_CRENGINE_MARGIN_RIGHT
                   || key == CONFIG_CRENGINE_MARGIN_BOTTOM) {
            int margin = atoi(val);
            if (key == CONFIG_CRENGINE_MARGIN_TOP) {
                doc_view_->config_margins_.top = margin;
            } else if (key == CONFIG_CRENGINE_MARGIN_BOTTOM) {
                doc_view_->config_margins_.bottom = margin;
            } else if (key == CONFIG_CRENGINE_MARGIN_LEFT) {
                doc_view_->config_margins_.left = margin;
            } else if (key == CONFIG_CRENGINE_MARGIN_RIGHT) {
                doc_view_->config_margins_.right = margin;
            }
            doc_view_->UpdatePageMargins();
        } else if (key == CONFIG_CRENGINE_FONT_FACE_MAIN) {
            doc_view_->config_font_face_ = UnicodeToUtf8(lString16(val));
            doc_view_->UpdatePageMargins();
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_FONT_FACE_FALLBACK) {
            fontMan->SetFallbackFontFace(UnicodeToUtf8(lString16(val)));
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_FONT_SIZE) {
            int int_val = atoi(val);
            int_val = GetClosestValueInArray(
                    ALLOWED_FONT_SIZES,
                    sizeof(ALLOWED_FONT_SIZES) / sizeof(int),
                    int_val);
            if (doc_view_->config_font_size_ != int_val) {
                doc_view_->config_font_size_ = int_val;
                doc_view_->UpdatePageMargins();
                doc_view_->RequestRender();
            }
        } else if (key == CONFIG_CRENGINE_INTERLINE_SPACE) {
            int int_val = atoi(val);
            int_val = GetClosestValueInArray(
                    ALLOWED_INTERLINE_SPACES,
                    sizeof(ALLOWED_INTERLINE_SPACES) / sizeof(int),
                    int_val);
            if (doc_view_->config_interline_space_ != int_val) {
                    doc_view_->config_interline_space_ = int_val;
                    doc_view_->RequestRender();
                    doc_view_->position_is_set_ = false;
            }
        } else if (key == CONFIG_CRENGINE_EMBEDDED_STYLES) {
            int int_val = atoi(val);
            if (int_val < 0 || int_val > 1) {
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            doc_view_->config_embeded_styles_ = int_val;
            doc_view_->GetCrDom()->setDocFlag(DOC_FLAG_EMBEDDED_STYLES, int_val);
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_EMBEDDED_FONTS) {
            int int_val = atoi(val);
            if (int_val < 0 || int_val > 1) {
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            doc_view_->config_embeded_fonts_ = int_val;
            doc_view_->GetCrDom()->setDocFlag(DOC_FLAG_EMBEDDED_FONTS, int_val);
            doc_view_->RequestRender();
        } else if (key == CONFIG_CRENGINE_FOOTNOTES) {
            int int_val = atoi(val);
            if (int_val < 0 || int_val > 1) {
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            doc_view_->config_enable_footnotes_ = int_val;
            doc_view_->GetCrDom()->setDocFlag(DOC_FLAG_ENABLE_FOOTNOTES, int_val);
            doc_view_->RequestRender();
        } else {
            CRLog::warn("processConfig unknown key: key=%d, val=%s", key, val);
        }
        /*
        TODO TXT format options
        public void toggleTextFormat()
        {
        	DocFormat fmt = mDoc.getFormat();
        	if ((fmt == DocFormat.TXT) || (fmt == DocFormat.HTML) || (fmt == DocFormat.PDB)) {
        		boolean disableTextReflow = mDoc.getFlag(Doc.DONT_REFLOW_TXT_FILES_FLAG);
        		disableTextReflow = !disableTextReflow;
        		mDoc.setFlag(Doc.DONT_REFLOW_TXT_FILES_FLAG, disableTextReflow);
        		// Save and reload
        		saveDoc();
        		openDoc(mDoc, currentDocSectionTemp);
        	}
        }
        */
    }
    doc_view_->RenderIfDirty();
    response.addInt(ExportPagesCount(doc_view_->GetColumns(), doc_view_->GetPagesCount()));
}

void CreBridge::processOpen(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_OPEN;
    CmdDataIterator iter(request.first);

    uint8_t* socket_name = NULL;
    uint8_t* file_name = NULL;
    iter.getByteArray(&socket_name).getByteArray(&file_name);
    if (!iter.isValid() || !socket_name || !file_name) {
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    StSocketConnection connection((const char*) socket_name);
    if (!connection.isValid()) {
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    int fd;
    bool received = connection.receiveFileDescriptor(fd);
    if (!received) {
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    if (doc_view_->LoadDoc(reinterpret_cast<const char*>(file_name))) {
        doc_view_->RenderIfDirty();
        response.addInt(ExportPagesCount(
                doc_view_->GetColumns(),
                doc_view_->GetPagesCount()));
    }
}

void CreBridge::processPageRender(CmdRequest& request, CmdResponse& response)
{
    //CRLog::trace("processPageRender START");
    response.cmd = CMD_RES_PAGE_RENDER;
    CmdDataIterator iter(request.first);

    uint32_t page;
    uint32_t width;
    uint32_t height;
    iter.getInt(&page).getInt(&width).getInt(&height);
    if (!iter.isValid()) {
        CRLog::error("processPageRender bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (doc_view_ == NULL) {
        CRLog::error("processPageRender doc not opened");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (doc_view_->height_ != height || doc_view_->width_ != width) {
        CRLog::error("processPageRender wanted page size mismatch");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    doc_view_->GoToPage(page * doc_view_->GetColumns());

    CmdData* resp = new CmdData();
    unsigned char* pixels = (unsigned char*) resp->newByteArray(width * height * 4);
    AndroidDrawBuf* buf = new AndroidDrawBuf(width, height, pixels, 32);
    doc_view_->Draw(*buf);
    buf->ToAndroidBitmap();
    delete buf;

    response.addData(resp);
    //CRLog::trace("processPageRender END");
}

void CreBridge::processPage(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PAGE;
    //processLinks(pageNumber, response);
}

void CreBridge::processPageByXPath(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_CRE_PAGE_BY_XPATH;
    CmdDataIterator iter(request.first);

    uint8_t* xpath_string;
    iter.getByteArray(&xpath_string);
    if (!iter.isValid()) {
        CRLog::error("processPageByXPath invalid iterator");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    int page = -1;
    lString16 xpath(reinterpret_cast<const char*>(xpath_string));
    ldomXPointer bm = doc_view_->GetCrDom()->createXPointer(xpath);
    if (!bm.isNull()) {
        doc_view_->GoToBookmark(bm);
        page = (ExportPage(doc_view_->GetColumns(), doc_view_->GetCurrPage()));
    }
    response.addInt(page);
}

void CreBridge::processPageXPath(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_CRE_PAGE_XPATH;
    CmdDataIterator iter(request.first);

    uint32_t page;
    iter.getInt(&page);
    if (!iter.isValid()) {
        CRLog::error("processPageXPath invalid iterator");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    ldomXPointer xptr = doc_view_->getPageBookmark(page * doc_view_->GetColumns());
    if (xptr.isNull()) {
        CRLog::error("processPageXPath null ldomXPointer");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    AddString(response, xptr.toString());
}

void CreBridge::processOutline(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_OUTLINE;

    response.addInt((uint32_t) 0);

    int columns = doc_view_->GetColumns();
    LVPtrVector<LvTocItem, false> outline;
    doc_view_->GetOutline(outline);
    for (int i = 0; i < outline.length(); i++) {
        LvTocItem* row = outline[i];
        response.addWords(
                (uint16_t) OUTLINE_TARGET_XPATH,
                (uint16_t) ExportPage(columns, row->getPage()));
        response.addInt((uint32_t) row->getLevel());
        AddString(response, row->getName());
        AddString(response, row->getPath());
    }
}

void CreBridge::processQuit(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_QUIT;
}

void CreBridge::processMetadata(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_CRE_METADATA;
    CmdDataIterator iter(request.first);
    uint8_t* cre_uri_chars;
    iter.getByteArray(&cre_uri_chars);
    if (!iter.isValid()) {
        CRLog::error("processMetadata: iterator invalid data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    lString16 cre_uri(reinterpret_cast<const char*>(cre_uri_chars));
    lString16 to_archive_path;
    lString16 in_archive_path;
    bool is_archived = LVSplitArcName(cre_uri, to_archive_path, in_archive_path);
    LVStreamRef doc_stream = LVOpenFileStream(
            (is_archived ? to_archive_path : cre_uri).c_str(), LVOM_READ);
    if (!doc_stream) {
        CRLog::error("processMetadata: cannot open file");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (is_archived) {
        LVContainerRef container = LVOpenArchieve(doc_stream);
        if (container.isNull()) {
            CRLog::error("processMetadata: cannot open archive container");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        doc_stream = container->OpenStream(in_archive_path.c_str(), LVOM_READ);
        if (doc_stream.isNull()) {
            CRLog::error("processMetadata: cannot open file from archive container");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
    }

    CRLog::trace("Parsing metadata for: %s", LCSTR(cre_uri));

    LVStreamRef thumb_stream;
    lString16 	doc_title;
    lString16 	doc_author;
    lString16 	doc_series;
    int 		doc_series_number = 0;
    lString16 	doc_lang;

    if (DetectEpubFormat(doc_stream)) {
        //EPUB
        LVContainerRef doc_stream_zip = LVOpenArchieve(doc_stream);
        //Check is this a ZIP archive
        if (doc_stream_zip.isNull()) {
            CRLog::error("processMetadata: EPUB doc_stream_zip.isNull()");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        //Check root media type
        lString16 root_file_path = EpubGetRootFilePath(doc_stream_zip);
        if (root_file_path.empty()) {
            CRLog::error("processMetadata: malformed EPUB");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        EncryptedDataContainer* decryptor = new EncryptedDataContainer(doc_stream_zip);
        if (decryptor->open()) {
            CRLog::debug("processMetadata: EPUB encrypted items detected");
        }
        LVContainerRef doc_stream_encrypted = LVContainerRef(decryptor);
        lString16 code_base = LVExtractPath(root_file_path, false);
        LVStreamRef content_stream = doc_stream_encrypted->
                OpenStream(root_file_path.c_str(), LVOM_READ);
        if (content_stream.isNull()) {
            CRLog::error("processMetadata: malformed EPUB");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        CrDom* dom = LVParseXMLStream(content_stream);
        if (!dom) {
            CRLog::error("processMetadata: malformed EPUB");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        lString16 thumb_id;
        for (int i=1; i<20; i++) {
            ldomNode * item = dom->nodeFromXPath(
                    lString16("package/metadata/meta[") << fmt::decimal(i) << "]");
            if (!item) {
                break;
            }
            lString16 name = item->getAttributeValue("name");
            lString16 content = item->getAttributeValue("content");
            if (name == "cover") {
                thumb_id = content;
            }
        }
        for (int i=1; i<50000; i++) {
            ldomNode* item = dom->nodeFromXPath(
                    lString16("package/manifest/item[") << fmt::decimal(i) << "]");
            if (!item) {
                break;
            }
            lString16 href = item->getAttributeValue("href");
            lString16 id = item->getAttributeValue("id");
            if (!href.empty() && !id.empty()) {
                if (id == thumb_id) {
                    lString16 thumbnail_file_name = code_base + href;
                    thumb_stream = doc_stream_encrypted->
                            OpenStream(thumbnail_file_name.c_str(), LVOM_READ);
                }
            }
        }
        doc_author = dom->textFromXPath(lString16("package/metadata/creator")).trim();
        doc_title = dom->textFromXPath(lString16("package/metadata/title")).trim();
        doc_lang = dom->textFromXPath(lString16("package/metadata/language")).trim();
        for (int i=1; i<20; i++) {
            ldomNode* item = dom->nodeFromXPath(
                    lString16("package/metadata/meta[") << fmt::decimal(i) << "]");
            if (!item) {
                break;
            }
            lString16 name = item->getAttributeValue("name");
            lString16 content = item->getAttributeValue("content");
            if (name == "calibre:series") {
                doc_series = content.trim();
            } else if (name == "calibre:series_index") {
                doc_series_number = content.trim().atoi();
            }
        }
        delete dom;
    } else {
        //FB2 or PDB
        thumb_stream = GetFB2Coverpage(doc_stream);
        if (thumb_stream.isNull()) {
            doc_format_t fmt;
            if (DetectPDBFormat(doc_stream, fmt)) {
                thumb_stream = GetPDBCoverpage(doc_stream);
            }
        }
        CrDom dom;
        LvDomWriter writer(&dom, true);
        dom.setNodeTypes(fb2_elem_table);
        dom.setAttributeTypes(fb2_attr_table);
        dom.setNameSpaceTypes(fb2_ns_table);
        LvXmlParser parser(doc_stream, &writer);
        if (parser.CheckFormat() && parser.Parse()) {
            doc_author = ExtractDocAuthors(&dom, lString16("|"));
            doc_title = ExtractDocTitle(&dom);
            doc_lang = ExtractDocLanguage(&dom);
            doc_series = ExtractDocSeries(&dom, &doc_series_number);
        }
    }

    CmdData* doc_thumb = new CmdData();
    int thumb_width = 0;
    int thumb_height = 0;
    doc_thumb->type = TYPE_ARRAY_POINTER;
    if (!thumb_stream.isNull()) {
        LVImageSourceRef thumb_image = LVCreateStreamCopyImageSource(thumb_stream);
        if (!thumb_image.isNull()
                && thumb_image->GetWidth() > 0 && thumb_image->GetHeight() > 0) {
            thumb_width = thumb_image->GetWidth();
            thumb_height = thumb_image->GetHeight();
            int size = thumb_width * thumb_height * 4;
            unsigned char* thumb_pixels = (unsigned char*) doc_thumb->newByteArray(size);
            AndroidDrawBuf* draw_buf = new AndroidDrawBuf(
                    thumb_width,
                    thumb_height,
                    thumb_pixels,
                    32);
            draw_buf->Draw(thumb_image, 0, 0, thumb_width, thumb_height, false);
            thumb_image.Clear();
            draw_buf->ToAndroidBitmap();
            delete draw_buf;
        }
    }

    response.addData(doc_thumb);
    response.addInt(thumb_width);
    response.addInt(thumb_height);
    AddString(response, doc_title);
    AddString(response, doc_author);
    AddString(response, doc_series);
    response.addInt(doc_series_number);
    AddString(response, doc_lang);

    /*
    Empire V EPUB - не извлекается обложка, хотя она есть

    LvStreamRef GetEpubCoverpage(LVContainerRef doc_stream_zip)
    {
    	// check root media type
    	lString16 rootfilePath = EpubGetRootFilePath(doc_stream_zip);
    	if (rootfilePath.empty()) {
    		return LvStreamRef();
    	}
    	EncryptedDataContainer* decryptor = new EncryptedDataContainer(doc_stream_zip);
    	if (decryptor->open()) {
    		CRLog::debug("EPUB: encrypted items detected");
    	}
    	LVContainerRef doc_stream_encrypted = LVContainerRef(decryptor);
    	lString16 codeBase = LVExtractPath(rootfilePath, false);
    	LvStreamRef content_stream = doc_stream_encrypted->OpenStream(
    	        rootfilePath.c_str(),
    	        LVOM_READ);
    	if (content_stream.isNull()) {
    		return LvStreamRef();
    	}
    	LvStreamRef coverPageImageStream;
    	// reading content stream
    	{
    		CrDom* doc_dom = LVParseXMLStream(content_stream);
    		if (!doc_dom)
    			return LvStreamRef();
    		lString16 coverId;
    		for ( int i=1; i<20; i++ ) {
    			ldomNode * item = doc_dom->nodeFromXPath(
    			        lString16("package/metadata/meta[") << fmt::decimal(i) << "]");
    			if ( !item )
    				break;
    			lString16 name = item->getAttributeValue("name");
    			lString16 content = item->getAttributeValue("content");
    			if (name == "cover")
    				coverId = content;
    		}
    		// items
    		for (int i=1; i<50000; i++) {
    			ldomNode * item = doc_dom->nodeFromXPath(
    			        lString16("package/manifest/item[") << fmt::decimal(i) << "]");
    			if (!item) {
    				break;
    			}
    			lString16 href = item->getAttributeValue("href");
    			lString16 id = item->getAttributeValue("id");
    			if ( !href.empty() && !id.empty() ) {
    				if (id == coverId) {
    					// coverpage file
    					lString16 coverFileName = codeBase + href;
    					CRLog::info("EPUB coverpage file: %s", LCSTR(coverFileName));
    					coverPageImageStream = doc_stream_encrypted->OpenStream(
    					        coverFileName.c_str(),
    					        LVOM_READ);
    				}
    			}
    		}
    		delete doc_dom;
    	}
    	return coverPageImageStream;
    }
    */
}

CreBridge::CreBridge() : StBridge("CreBridge")
{
    doc_view_ = NULL;
    CRLog::setLogger(new AndroidLogger());
#ifdef AXYDEBUG
    CRLog::set_log_level(CRLog::INFO);
#else
    CRLog::set_log_level(CRLog::ERROR);
#endif
    InitFontManager(lString8::empty_str);
    // 0 - disabled, 1 - bytecode, 2 - auto
    fontMan->SetHintingMode(HINTING_MODE_BYTECODE_INTERPRETOR);
    fontMan->setKerning(true);
    HyphMan::init();
}

CreBridge::~CreBridge()
{
    if (doc_view_) {
        delete doc_view_;
    }
    HyphMan::uninit();
    ShutdownFontManager();
    CRLog::setLogger(NULL);
}

void CreBridge::process(CmdRequest& request, CmdResponse& response)
{
    response.reset();
    switch (request.cmd)
    {
    case CMD_REQ_PDF_FONTS:
        processFonts(request, response);
        break;
    case CMD_REQ_SET_CONFIG:
        processConfig(request, response);
        break;
    case CMD_REQ_OPEN:
        processOpen(request, response);
        break;
    case CMD_REQ_PAGE:
        processPage(request, response);
        break;
    case CMD_REQ_PAGE_RENDER:
        processPageRender(request, response);
        break;
    case CMD_REQ_OUTLINE:
        processOutline(request, response);
        break;
    case CMD_REQ_CRE_PAGE_BY_XPATH:
        processPageByXPath(request, response);
        break;
    case CMD_REQ_CRE_PAGE_XPATH:
        processPageXPath(request, response);
        break;
    case CMD_REQ_CRE_METADATA:
        processMetadata(request, response);
        break;
    case CMD_REQ_QUIT:
        processQuit(request, response);
        break;
    default:
        CRLog::error("Unknown request: %d", request.cmd);
        response.result = RES_UNKNOWN_CMD;
        break;
    }
    //response.print(LCTX);
}