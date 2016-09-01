/*
 * Copyright (C) 2013 The Crengine CLI viewer interface Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>

#include <android/log.h>

#include "StProtocol.h"
#include "StBridge.h"

#include "crengine/include/crengine.h"
#include "crengine/include/epubfmt.h"
#include "crengine/include/pdbfmt.h"
#include "crengine/include/lvstream.h"
#include "crengine/include/lvdocview.h"
#include "crengine/include/props.h"

// Да, именно так, дважды инклюдим один и тот же хедер, но с разными дефайнами
#include "crengine/include/fb2def.h"
#define XS_IMPLEMENT_SCHEME 1
#include "crengine/include/fb2def.h"
#undef XS_IMPLEMENT_SCHEME

class AndroidLogger : public CRLog
{
public:
	AndroidLogger()
	{
		set_log_level(CRLog::TRACE);
	}

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
    void ToArgb8888()
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
	AndroidDrawBuf(int dx, int dy, lUInt8* pixels, int bpp) : LVColorDrawBuf(dx, dy, pixels, bpp) {}
};

static void AddString(CmdResponse& response, lString16 str16)
{
	lString8 str8 = UnicodeToUtf8(str16);
	uint32_t size = (uint32_t) str8.size();
	//We will place null-terminator at the string end
	size++;
	CmdData* cmd_data = new CmdData();
	unsigned char* str_buffer = (unsigned char*) cmd_data->createBuffer(size);
	memcpy(str_buffer, str8.c_str(), (size - 1));
	str_buffer[size - 1] = 0;
	response.addData(cmd_data);
}

static void OutlineRecursion(LvTocItem* row, CmdResponse& response)
{
	response.addWords((uint16_t) OUTLINE_TARGET_XPATH, row->getPage());
	response.addValue((uint32_t) row->getLevel());

	AddString(response, row->getName());
	AddString(response, row->getPath());

	for (int i=0; i < row->getChildCount(); i++) {
		OutlineRecursion(row->getChild(i), response);
	}
}

class CreBridge : public StBridge
{
private:

	LVDocView* doc_view_;

	void processMetadata(CmdRequest& request, CmdResponse& response)
	{
		response.cmd = CMD_RES_CRE_METADATA;
		CmdDataIterator iter(request.first);
		uint8_t* cre_uri_chars;
		iter.required(&cre_uri_chars);
		if (!iter.isValid()) {
			CRLog::error("processMetadata: iterator invalid data");
			response.result = RES_BAD_REQ_DATA;
			return;
		}
		lString16 cre_uri(reinterpret_cast<const char*>(cre_uri_chars));
		lString16 to_archive_path;
		lString16 in_archive_path;
		bool is_archived = LvSplitArcName(cre_uri, to_archive_path, in_archive_path);
		LvStreamRef doc_stream = LVOpenFileStream((is_archived ? to_archive_path : cre_uri).c_str(), LVOM_READ);
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

		LvStreamRef thumb_stream;
		lString16 	doc_title;
		lString16 	doc_author;
		lString16 	doc_series;
		int 		doc_series_number;
		lString16 	doc_lang;

		if (DetectEpubFormat(doc_stream)) {
			//EPUB
			LVContainerRef doc_stream_zip = LVOpenArchieve(doc_stream);
			//Check is this a ZIP archive
			if (doc_stream_zip.isNull()) {
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
		    LvStreamRef content_stream = doc_stream_encrypted->OpenStream(root_file_path.c_str(), LVOM_READ);
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
			lString16 thumbnail_id;
			for (int i=1; i<20; i++) {
				ldomNode * item = dom->nodeFromXPath(lString16("package/metadata/meta[") << fmt::decimal(i) << "]");
				if (!item) {
					break;
				}
				lString16 name = item->getAttributeValue("name");
				lString16 content = item->getAttributeValue("content");
				if (name == "cover") {
					thumbnail_id = content;
				}
			}
			for (int i=1; i<50000; i++) {
				ldomNode* item = dom->nodeFromXPath(lString16("package/manifest/item[") << fmt::decimal(i) << "]");
				if (!item) {
					break;
				}
				lString16 href = item->getAttributeValue("href");
				lString16 id = item->getAttributeValue("id");
				if (!href.empty() && !id.empty()) {
					if (id == thumbnail_id) {
						lString16 thumbnail_file_name = code_base + href;
						thumb_stream = doc_stream_encrypted->OpenStream(thumbnail_file_name.c_str(), LVOM_READ);
					}
				}
			}
			doc_author = dom->textFromXPath(lString16("package/metadata/creator")).trim();
			doc_title = dom->textFromXPath(lString16("package/metadata/title")).trim();
			doc_lang = dom->textFromXPath(lString16("package/metadata/language")).trim();
			for (int i=1; i<20; i++) {
				ldomNode* item = dom->nodeFromXPath(lString16("package/metadata/meta[") << fmt::decimal(i) << "]");
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
				doc_author = extractDocAuthors(&dom, lString16("|"), false);
				doc_title = extractDocTitle(&dom);
				doc_lang = extractDocLanguage(&dom);
				doc_series = extractDocSeries(&dom, &doc_series_number);
			}
		}

		CmdData* doc_thumb = new CmdData();
		int thumb_width = 0;
		int thumb_height = 0;
		doc_thumb->type = TYPE_VAR;
		if (!thumb_stream.isNull()) {
			LVImageSourceRef thumb_image = LVCreateStreamCopyImageSource(thumb_stream);
			if (!thumb_image.isNull() && thumb_image->GetWidth() > 0 && thumb_image->GetHeight() > 0) {
				thumb_width = thumb_image->GetWidth();
				thumb_height = thumb_image->GetHeight();
				int size = thumb_width * thumb_height * 4;
				unsigned char* thumb_pixels = (unsigned char*) doc_thumb->createBuffer(size);
				AndroidDrawBuf* draw_buf = new AndroidDrawBuf(thumb_width, thumb_height, thumb_pixels, 32);
				draw_buf->Draw(thumb_image, 0, 0, thumb_width, thumb_height, false);
				thumb_image.Clear();
		        draw_buf->ToArgb8888();
		        delete draw_buf;
			}
		}
		response.addData(doc_thumb);
		response.addValue(thumb_width);
		response.addValue(thumb_height);
		AddString(response, doc_title);
		AddString(response, doc_author);
		AddString(response, doc_series);
		response.addValue(doc_series_number);
		AddString(response, doc_lang);
	}

	void processFonts(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_CRE_FONTS;
	    CmdDataIterator iter(request.first);
	    while (iter.hasNext()) {
	    	uint8_t* font_fullpath;
	    	iter.required(&font_fullpath);
			if (!iter.isValid()) {
				CRLog::error("Wrong font data");
				response.result = RES_BAD_REQ_DATA;
				return;
			}
			fontMan->RegisterFont(UnicodeToUtf8(lString16(reinterpret_cast<const char*>(font_fullpath))));
	    }
		lString16Collection registeredFontsNames;
		fontMan->getFaceList(registeredFontsNames);
		int len = registeredFontsNames.length();
		for (int i = 0; i < len; i++) {
			response.addString(LCSTR(registeredFontsNames[i]), true);
		}
		doc_view_ = new LVDocView();
		HyphMan::initDictionaries();
	}

	void processPageBgTexture(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_CRE_PAGE_BG_TEXTURE;
	    CmdDataIterator iter(request.first);

	    LVImageSourceRef img;
	    uint32_t texture_flags = 1;
	    if (request.dataCount == 2) {
	    	unsigned char* texture_pixels;
	    	iter.integer(&texture_flags).required(&texture_pixels);
	    	if (iter.isValid()) {
				LvStreamRef stream = LVCreateMemoryStream(texture_pixels, response.first->value.value32, true, LVOM_READ);
				if (!stream.isNull()) {
					img = LVCreateStreamImageSource(stream);
				}
			} else {
				CRLog::error("processPageBgTexture invalid request data");
			}
	    }
	    doc_view_->setBackgroundImage(img, texture_flags != 0);
	}

	void processProps(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_CRE_PROPS;
	    CmdDataIterator iter(request.first);

	    CRPropRef props = LVCreatePropsContainer();
	    while (iter.hasNext()) {
	    	uint8_t* prop_key;
	    	uint8_t* prop_val;
	    	iter.required(&prop_key).required(&prop_val);
			if (!iter.isValid()) {
				CRLog::error("Wrong request data");
				response.result = RES_BAD_REQ_DATA;
				return;
			}
			const char* const_prop_key = reinterpret_cast<const char*>(prop_key);
			const char* const_prop_val = reinterpret_cast<const char*>(prop_val);
			props->setString(const_prop_key, const_prop_val);
	    }
	    doc_view_->PropsApply(props);
	    doc_view_->CheckRender();
	}

	void processResize(CmdRequest& request, CmdResponse& response)
	{
		response.cmd = CMD_RES_CRE_RESIZE;
		CmdDataIterator iter(request.first);

		uint32_t dx = 0;
		uint32_t dy = 0;
		iter.integer(&dx).integer(&dy);
		if (!iter.isValid()) {
			response.result = RES_BAD_REQ_DATA;
			return;
		}
		doc_view_->Resize(dx, dy);
		doc_view_->CheckRender();
	}

	void processPagesCount(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_CRE_PAGES_COUNT;
	    response.addValue(doc_view_->getPagesCount());
	}

	void processOpen(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_OPEN;
	    CmdDataIterator iter(request.first);

	    uint8_t* file_name = NULL;
	    uint32_t page_number = 0;
	    iter.required(&file_name).integer(&page_number);
	    if (!iter.isValid() || !file_name) {
	        response.result = RES_BAD_REQ_DATA;
	        return;
	    }
	    if (doc_view_->LoadDoc(reinterpret_cast<const char*>(file_name))) {
	    	doc_view_->goToPage(page_number);
	    	response.addValue(doc_view_->getPagesCount());
	    }
	    //INFO_L(LCTX, "DOC (%s) getPageCount %d, IsDocOpened %d, IsRendered %d",
	    //		LCSTR(cre16_file_name), doc_view_->getPagesCount(), doc_view_->IsDocOpened(), doc_view_->IsRendered());
	}

	void processPageRender(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_PAGE_RENDER;
	    CmdDataIterator iter(request.first);

	    uint32_t page_number;
	    uint32_t width;
	    uint32_t height;
	    iter.integer(&page_number).integer(&width).integer(&height);
	    if (!iter.isValid()) {
	    	CRLog::error("Bad request data");
	        response.result = RES_BAD_REQ_DATA;
	        return;
	    }
	    if (doc_view_ == NULL || !doc_view_->IsDocOpened()) {
	    	CRLog::error("Document not yet opened");
	        response.result = RES_DUP_OPEN;
	        return;
	    }
	    doc_view_->goToPage(page_number);

	    CmdData* resp = new CmdData();
	    unsigned char* pixels = (unsigned char*) resp->createBuffer(width * height * 4);

	    AndroidDrawBuf* drawbuf = new AndroidDrawBuf(width, height, pixels, 32);
	    doc_view_->Draw(*drawbuf);
	    drawbuf->ToArgb8888();
	    delete drawbuf;

	    response.addData(resp);
	}

	void processPage(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_PAGE;
	    //processLinks(pageNumber, response);
	}

	void processPageByXPath(CmdRequest& request, CmdResponse& response)
	{
		response.cmd = CMD_RES_CRE_PAGE_BY_XPATH;
		CmdDataIterator iter(request.first);

		uint8_t* xpath_string;
		iter.required(&xpath_string);
		if (!iter.isValid()) {
			CRLog::error("processPageByXPath invalid iterator");
			response.result = RES_BAD_REQ_DATA;
			return;
		}
		int pageIndex = -1;
		lString16 xpath(reinterpret_cast<const char*>(xpath_string));
		ldomXPointer bm = doc_view_->getDocument()->createXPointer(xpath);
		if (!bm.isNull()) {
			doc_view_->goToBookmark(bm);
			pageIndex = doc_view_->getCurPage();
		}
		response.addValue(pageIndex);
	}

	void processPageXPath(CmdRequest& request, CmdResponse& response)
	{
		response.cmd = CMD_RES_CRE_PAGE_XPATH;
		CmdDataIterator iter(request.first);

		uint32_t page;
		iter.integer(&page);
		if (!iter.isValid()) {
			CRLog::error("processPageXPath invalid iterator");
			response.result = RES_BAD_REQ_DATA;
			return;
		}
		ldomXPointer xptr = doc_view_->getPageBookmark(page);
		if (xptr.isNull()) {
			CRLog::error("processPageXPath null ldomXPointer");
			response.result = RES_BAD_REQ_DATA;
			return;
		}
		AddString(response, xptr.toString());
	}

	void processOutline(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_OUTLINE;

        response.addValue((uint32_t) 0);

        LvTocItem* outline = doc_view_->getToc();
        for (int i=0; i < outline->getChildCount(); i++) {
			OutlineRecursion(outline->getChild(i), response);
		}
	}

	void processQuit(CmdRequest& request, CmdResponse& response)
	{
	    response.cmd = CMD_RES_QUIT;
	}

public:

	CreBridge() : StBridge("axy.cre.CreBridge")
	{
		doc_view_ = NULL;
		CRLog::setLogger(new AndroidLogger());
		InitFontManager(lString8::empty_str);
	}

	~CreBridge()
	{
		if (doc_view_) {
			if (doc_view_->IsDocOpened()) {
				doc_view_->Close();
			}
			delete doc_view_;
		}
		HyphMan::uninit();
		ShutdownFontManager();
		CRLog::setLogger(NULL);
	}

	void process(CmdRequest& request, CmdResponse& response)
	{
	    response.reset();
	    switch (request.cmd)
	    {
	    case CMD_REQ_CRE_FONTS:
			processFonts(request, response);
			break;
	    case CMD_REQ_CRE_PAGE_BG_TEXTURE:
			processPageBgTexture(request, response);
			break;
	    case CMD_REQ_CRE_PROPS:
			processProps(request, response);
			break;
	    case CMD_REQ_CRE_RESIZE:
			processResize(request, response);
			break;
	    case CMD_REQ_OPEN:
	        processOpen(request, response);
	        break;
	    case CMD_REQ_CRE_PAGES_COUNT:
			processPagesCount(request, response);
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

/*
	jboolean Javainterface_checkImageInternal(JNIEnv* env, jobject view, jint x, jint y, jobject imageInfo)
	{
	    int dx, dy;
	    bool needRotate = false;
	    int buf_width = GetIntField(env, imageInfo, "bufWidth");
	    int buf_height = GetIntField(env, imageInfo,"bufHeight");

	    // checks whether point belongs to image: if found, returns true, and _currentImage is set to image
	    const int MAX_IMAGE_BUF_SIZE = 1200000;
		current_image_ = doc_view_->getImageByPoint(lvPoint(x, y));
		if (current_image_.isNull())
			return JNI_FALSE;
		dx = current_image_->GetWidth();
		dy = current_image_->GetHeight();
		if (dx < 8 && dy < 8) {
			current_image_.Clear();
			return JNI_FALSE;
		}
		needRotate = false;
		if (buf_width <= buf_height) {
			// screen == portrait
			needRotate = 8 * dx > 10 * dy;
		} else {
			// screen == landscape
			needRotate = 10 * dx < 8 * dy;
		}
		int scale = 1;
		if (dx * dy > MAX_IMAGE_BUF_SIZE) {
			scale = dx * dy /  MAX_IMAGE_BUF_SIZE;
			dx /= scale;
			dy /= scale;
		}
		LVColorDrawBuf * buf = new LVColorDrawBuf(dx, dy);
		buf->Clear(0xFF000000); // transparent
		buf->Draw(current_image_, 0, 0, dx, dy, false);
		if (needRotate) {
			int c = dx;
			dx = dy;
			dy = c;
			buf->Rotate(CR_ROTATE_ANGLE_90);
		}
		current_image_ = LVCreateDrawBufImageSource(buf, true);

		SetIntField(env, imageInfo, "rotation", needRotate ? 1 : 0);
		SetIntField(env, imageInfo, "buf_width", dx);
		SetIntField(env, imageInfo, "height", dy);
		SetIntField(env, imageInfo, "scaledWidth", dx);
		SetIntField(env, imageInfo, "scaledHeight", dy);
		SetIntField(env, imageInfo, "x", 0);
		SetIntField(env, imageInfo, "y", 0);
		return JNI_TRUE;
	}

	jboolean Javainterface_drawImageInternal(JNIEnv* env, jobject view, jobject bitmap, jint bpp, jobject imageInfo)
	{
	    int dx = GetIntField(env, imageInfo,"scaledWidth");
	    int dy = GetIntField(env, imageInfo,"scaledHeight");
	    int x = GetIntField(env, imageInfo,"x");
	    int y = GetIntField(env, imageInfo,"y");
	    int rotation = GetIntField(env, imageInfo,"rotation");
	    int dpi = GetIntField(env, imageInfo,"bufDpi");
		LVDrawBuf * drawbuf = BitmapAccessorInterface::getInstance()->lock(env, bitmap);
		bool res = false;
		if ( drawbuf!=NULL ) {

		    lvRect full(0, 0, drawbuf->GetWidth(), drawbuf->GetHeight());
		    lvRect zoomInRect(full);
		    lvRect zoomOutRect(full);
		    int zoomSize = dpi * 4 / 10;
		    if (rotation == 0) {
		    	zoomInRect.right = zoomInRect.left + zoomSize;
		    	zoomInRect.top = zoomInRect.bottom - zoomSize;
		    	zoomOutRect.left = zoomOutRect.right - zoomSize;
		    	zoomOutRect.top = zoomOutRect.bottom - zoomSize;
		    } else {
		    	zoomInRect.right = zoomInRect.left + zoomSize;
		    	zoomInRect.bottom = zoomInRect.top + zoomSize;
		    	zoomOutRect.right = zoomOutRect.left + zoomSize;
		    	zoomOutRect.top = zoomOutRect.bottom - zoomSize;
		    }
			if (bpp >= 16) {
				// native resolution
				if (current_image_.isNull())
					res = false;
				res = doc_view_->drawImage(drawbuf, current_image_, x, y, dx, dy);
			} else {
				LVGrayDrawBuf grayBuf(drawbuf->GetWidth(), drawbuf->GetHeight(), bpp);
				if (current_image_.isNull())
					res = false;
				res = doc_view_->drawImage(&grayBuf, current_image_, x, y, dx, dy);
				grayBuf.DrawTo(drawbuf, 0, 0, 0, NULL);
			}
		    //CRLog::trace("getPageImageInternal calling bitmap->unlock");
			BitmapAccessorInterface::getInstance()->unlock(env, bitmap, drawbuf);
		} else {
			CRLog::error("bitmap accessor is invalid");
		}
		return res ? JNI_TRUE : JNI_FALSE;
	}

	jboolean Javainterface_doCmdNative(JNIEnv * _env, jobject _this, jint cmd, jint param)
	{
	    return doc_view_->doCommand((LVDocCmd) cmd, param) ? JNI_TRUE : JNI_FALSE;
	}

	jobject Javainterface_getCrePositionNative(JNIEnv* env, jobject _this, jstring jXPathStr)
	{
	    jclass cls = env->FindClass("org/crengine/CrePosition");
	    jmethodID mid = env->GetMethodID(cls, "<init>", "()V");
	    jobject jobj_cre_position = env->NewObject(cls, mid);
	    if (!doc_view_->IsDocOpened()) {
	    	return jobj_cre_position;
	    }
	    lString16 x_path_str = JavaToCreString(env, jXPathStr);
	    ldomXPointer bm;
	    // use current Y position for scroll view mode
	    bool useCurPos = false;
	    doc_view_->CheckPos();
	    if (!x_path_str.empty()) {
	        bm = doc_view_->getDocument()->createXPointer(x_path_str);
	    } else {
	        useCurPos = doc_view_->getViewMode() == DVM_SCROLL;
	        if (!useCurPos) {
	            bm = doc_view_->getBookmark();
	            if (bm.isNull()) {
	                CRLog::error("getCrePositionNative: Cannot get current position bookmark");
	            }
	        }
	    }
	    lvPoint pt = !bm.isNull() ? bm.toPoint() : lvPoint(0, doc_view_->GetPos());
	    SetIntField(env, jobj_cre_position, "x", pt.x);
	    SetIntField(env, jobj_cre_position, "y", pt.y);
	    SetIntField(env, jobj_cre_position, "fullHeight", doc_view_->GetFullHeight());
	    SetIntField(env, jobj_cre_position, "pageHeight", doc_view_->GetHeight());
	    SetIntField(env, jobj_cre_position, "pageWidth", doc_view_->GetWidth());
	    SetIntField(env, jobj_cre_position, "pageIndex", doc_view_->getCurPage());
	    SetIntField(env, jobj_cre_position, "pagesCount", doc_view_->getPageCount());
	    SetIntField(env, jobj_cre_position, "pageMode", doc_view_->getViewMode() == DVM_PAGES
	    		? doc_view_->getVisiblePageCount() : 0);
	    SetIntField(env, jobj_cre_position, "charsCount", doc_view_->getCurrentPageCharCount());
	    SetIntField(env, jobj_cre_position, "imageCount", doc_view_->getCurrentPageImageCount());
	    SetStringField(env, jobj_cre_position, "pageText", doc_view_->getPageText(false, -1));
		return jobj_cre_position;
	}

	jboolean Javainterface_findTextInternal(JNIEnv* env, jobject _this, jstring jpattern, jint origin, jint reverse, jint caseInsensitive)
	{
		//bool findText(lString16 pattern, int origin, bool reverse, bool caseInsensitive)
		// Было class member
		lString16 last_pattern_;
		lString16 pattern = JavaToCreString(env, jpattern);
		if (pattern.empty()) {
			return false;
		}
		if (pattern != last_pattern_ && origin == 1)
			origin = 0;
		last_pattern_ = pattern;
		LVArray<ldomWord> words;
		lvRect rc;
		doc_view_->GetPos( rc );
		int pageHeight = rc.height();
		int start = -1;
		int end = -1;
		if ( reverse ) {
			// reverse
			if ( origin == 0 ) {
				// from end current page to first page
				end = rc.bottom;
			} else if ( origin == -1 ) {
				// from last page to end of current page
				start = rc.bottom;
			} else { // origin == 1
				// from prev page to first page
				end = rc.top;
			}
		} else {
			// forward
			if ( origin == 0 ) {
				// from current page to last page
				start = rc.top;
			} else if ( origin == -1 ) {
				// from first page to current page
				end = rc.top;
			} else { // origin == 1
				// from next page to last
				start = rc.bottom;
			}
		}
		CRLog::debug("findText: Current page: %d .. %d", rc.top, rc.bottom);
		CRLog::debug("findText: searching for text '%s' from %d to %d origin %d", LCSTR(pattern), start, end, origin );
		if ( doc_view_->getDocument()->findText( pattern, caseInsensitive, reverse, start, end, words, 200, pageHeight ) ) {
			CRLog::debug("findText: pattern found");
			doc_view_->ClearSelection();
			doc_view_->selectWords( words );
			ldomMarkedRangeList * ranges = doc_view_->getMarkedRanges();
			if ( ranges ) {
				if ( ranges->length()>0 ) {
					int pos = ranges->get(0)->start.y;
					doc_view_->SetPos(pos);
				}
			}
			return true;
		}
		CRLog::debug("findText: pattern not found");
		return false;
	}

	jint Javainterface_goLinkInternal(JNIEnv* env, jobject _this, jstring _link)
	{
	    lString16 link = JavaToCreString(env, _link);
	    bool res = doc_view_->goLink(link, true);
	    return res ? 1 : 0;
	}

    jstring Javainterface_checkLinkInternal(JNIEnv* env, jobject _this, jint x, jint y, jint delta)
	{
	    lString16 link;
	    for (int r=0; r<=delta; r+=5) {
			int step = 5;
			int n = r / step;
			r = n * step;
			if (r == 0) {
				link = getLink(x, y);
			} else {
				bool found = false;
				for (int xx = -r; xx<=r; xx+=step) {
					link = getLink(x+xx, y-r);
					if (!link.empty()) {
						found = true;
						break;
					}
					link = getLink(x+xx, y+r);
					if (!link.empty()) {
						found = true;
						break;
					}
				}
				if (!found) {
					for (int yy = -r + step; yy<=r - step; yy+=step) {
						link = getLink(x+r, y+yy);
						if (!link.empty()) {
							found = true;
							break;
						}
						link = getLink(x-r, y+yy);
						if (!link.empty()) {
							found = true;
							break;
						}
					}
					if (!found) {
						link = lString16::empty_str;
					}
				}
			}
	    	if (!link.empty()) {
	    		return CreToJavaString(env, link);
	    	}
	    }
	    return NULL;
	}


    lString16 getLink(int x, int y)
    {
    	ldomXPointer p = doc_view_->getNodeByPoint(lvPoint(x, y));
    	if (p.isNull())
    		return lString16::empty_str;
    	lString16 href = p.getHRef();
    	return href;
    }

    // sets current image to null
	jboolean Javainterface_moveSelectionInternal(JNIEnv* env, jobject _this, jobject jobj_selection, jint _cmd, jint _param)
	{
	    if (doc_view_->doCommand((LVDocCmd)_cmd, (int) _param)) {
	        ldomXRangeList & sel = doc_view_->getDocument()->getSelections();
	        if (sel.length() > 0) {
	            ldomXRange currSel;
	            currSel = *sel[0];
	            if (!currSel.isNull()) {
	            	SetStringField(env, jobj_selection, "startPos", currSel.getStart().toString());
	            	SetStringField(env, jobj_selection, "endPos", currSel.getEnd().toString());
	                lvPoint startpt ( currSel.getStart().toPoint() );
	                lvPoint endpt ( currSel.getEnd().toPoint() );
	                SetIntField(env, jobj_selection, "startX", startpt.x);
	                SetIntField(env, jobj_selection, "startY", startpt.y);
	                SetIntField(env, jobj_selection, "endX", endpt.x);
	                SetIntField(env, jobj_selection, "endY", endpt.y);
	                int page = doc_view_->getBookmarkPage(currSel.getStart());
	                int pages = doc_view_->getPageCount();
	                lString16 titleText;
	                lString16 posText;
	                doc_view_->getBookmarkPosText(currSel.getStart(), titleText, posText);
	                int percent = 0;
	                if ( pages>1 )
	                	percent = 10000 * page/(pages-1);
	                lString16 selText = currSel.getRangeText( '\n', 8192 );
	                SetIntField(env, jobj_selection, "percent", percent);
	                SetStringField(env, jobj_selection, "text", selText);
	                SetStringField(env, jobj_selection, "chapter", titleText);
	            	return JNI_TRUE;
	            }
	        }
	    }
	    return JNI_FALSE;
	}

	void Javainterface_updateSelectionInternal(JNIEnv * env, jobject _this, jobject jobj_selection)
	{
	    lvPoint startpt(GetIntField(env, jobj_selection, "startX"), GetIntField(env, jobj_selection, "startY"));
	    lvPoint endpt(GetIntField(env, jobj_selection, "endX"), GetIntField(env, jobj_selection, "endY"));
		ldomXPointer startp = doc_view_->getNodeByPoint(startpt);
		ldomXPointer endp = doc_view_->getNodeByPoint(endpt);
	    if (!startp.isNull() && !endp.isNull()) {
	        ldomXRange r(startp, endp);
	        if (r.getStart().isNull() || r.getEnd().isNull()) {
	        	return;
	        }
	        r.sort();
			if (!r.getStart().isVisibleWordStart()) {
				r.getStart().prevVisibleWordStart();
			}
			if (!r.getEnd().isVisibleWordEnd()) {
				r.getEnd().nextVisibleWordEnd();
			}
	        if (r.isNull()) {
	        	return;
	        }
	        r.setFlags(1);
	        doc_view_->selectRange(r);
	        int page = doc_view_->getBookmarkPage(startp);
	        int pages = doc_view_->getPageCount();
	        lString16 titleText;
	        lString16 posText;
	        doc_view_->getBookmarkPosText(startp, titleText, posText);
	        int percent = 0;
	        if (pages > 1) {
	        	percent = 10000 * page/(pages-1);
	        }
	        lString16 selText = r.getRangeText('\n', 8192);
	        SetIntField(env, jobj_selection, "percent", percent);
	        SetStringField(env, jobj_selection, "startPos", r.getStart().toString());
	        SetStringField(env, jobj_selection, "endPos", r.getEnd().toString());
	        SetStringField(env, jobj_selection, "text", selText);
	        SetStringField(env, jobj_selection, "chapter", titleText);
	    }
	}

	void Javainterface_hilightBookmarksInternal(JNIEnv* env, jobject _this, jobjectArray list)
	{
	    LVPtrVector<CRBookmark> bookmarks;
	    if (list) {
	    	int len = env->GetArrayLength(list);
	    	for (int i=0; i < len; i++) {
	    		jobject jobj_bmk = env->GetObjectArrayElement(list, i);
	    	    CRBookmark* bookmark = new CRBookmark(
	    	    		GetStringField(env, jobj_bmk, "startPos"),
	    	    		GetStringField(env, jobj_bmk, "endPos"));
	    	    bookmark->setType(GetIntField(env, jobj_bmk, "type"));
	    	    bookmarks.add(bookmark);
	    	    env->DeleteLocalRef(jobj_bmk);
	    	}
	    }
	    doc_view_->setBookmarkList(bookmarks);
	}

	jboolean Javainterface_checkBookmarkInternal(JNIEnv* env, jobject view, jint x, jint y, jobject bmk)
	{
	    //CRLog::trace("checkBookmarkInternal(%d, %d)", x, y);
	    CRBookmark* found = doc_view_->findBookmarkByPoint(lvPoint(x, y));
	    if (!found) {
			return JNI_FALSE;
	    }
	    //CRLog::trace("checkBookmarkInternal - found bookmark of type %d", found->getType());
	    SetIntField(env, bmk,"type", found->getType());
	    SetStringField(env, bmk, "startPos", found->getStartPos());
	    SetStringField(env, bmk, "endPos", found->getEndPos());
		return JNI_TRUE;
	}

	jboolean Javainterface_closeImageInternal(JNIEnv * env, jobject view)
	{
		if (current_image_.isNull()) {
			return JNI_FALSE;
		}
		current_image_.Clear();
		return JNI_TRUE;
	}

static LvStreamRef JbyteArrayToCreStream(JNIEnv* env, jbyteArray array)
{
	if (!array)
		return LvStreamRef();
	int len = env->GetArrayLength(array);
	if ( !len )
		return LvStreamRef();
    lUInt8 * data = (lUInt8 *)env->GetByteArrayElements(array, NULL);
    LvStreamRef res = LVCreateMemoryStream(data, len, true, LVOM_READ);
    env->ReleaseByteArrayElements(array, (jbyte*)data, 0);
    return res;
}

static jbyteArray CreStreamToJbyteArray(JNIEnv* env, LvStreamRef stream)
{
	if ( stream.isNull() )
		return NULL;
	unsigned sz = stream->GetSize();
	if ( sz<10 || sz>2000000 )
		return NULL;
    jbyteArray array = env->NewByteArray(sz);
    lUInt8 * array_data = (lUInt8 *)env->GetByteArrayElements(array, 0);
    lvsize_t bytesRead = 0;
    stream->Read(array_data, sz, &bytesRead);
   	env->ReleaseByteArrayElements(array, (jbyte*)array_data, 0);
    if (bytesRead != sz)
    	return NULL;
    return array;
}

static int GetIntField(JNIEnv* env, jobject jobj, const char* field_name)
{
	jclass jcls = env->GetObjectClass(jobj);
	jfieldID jfid = env->GetFieldID(jcls, field_name, "I");
	return env->GetIntField(jobj, jfid);
}

static void SetIntField(JNIEnv* env, jobject jobj, const char* field_name, int val)
{
	jclass jcls = env->GetObjectClass(jobj);
	jfieldID jfid = env->GetFieldID(jcls, field_name, "I");
	env->SetIntField(jobj, jfid, val);
}

static lString16 GetStringField(JNIEnv* env, jobject jobj, const char* field_name)
{
	jclass jcls = env->GetObjectClass(jobj);
	jfieldID jfid = env->GetFieldID(jcls, field_name, "Ljava/lang/String;");
	jstring str = (jstring) env->GetObjectField(jobj, jfid);
	lString16 res = JavaToCreString(env, str);
	env->DeleteLocalRef(str);
	return res;
}

static void SetStringField(JNIEnv* env, jobject jobj, const char* field_name, const lString16& val)
{
	jclass jcls = env->GetObjectClass(jobj);
	jfieldID jfid = env->GetFieldID(jcls, field_name, "Ljava/lang/String;");
	env->SetObjectField(jobj, jfid, CreToJavaString(env, val));
}
*/

};

int main(int argc, char *argv[])
{
    CreBridge cre;
    return cre.main(argc, argv);
}
