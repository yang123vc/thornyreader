/*******************************************************

 Crengine

 lvdocview.cpp:  XML DOM tree rendering tools

 (c) Vadim Lopatin, 2000-2009
 This source code is distributed under the terms of
 GNU General Public License
 See LICENSE file for details

 *******************************************************/

#include "include/thornyreader.h"
#include "include/lvdocview.h"
#include "include/CreBridge.h"
#include "include/rtfimp.h"
#include "include/lvrend.h"
#include "include/epubfmt.h"
#include "include/chmfmt.h"
#include "include/wordfmt.h"
#include "include/pdbfmt.h"
#include "include/crcss.h"

// Yep, twice include single header with different define.
// Probably should be last in include list, to don't mess up with other includes.
#include "include/fb2def.h"
#define XS_IMPLEMENT_SCHEME 1
#include "include/fb2def.h"
//#undef XS_IMPLEMENT_SCHEME

#if 0
#define REQUEST_RENDER(caller) { CRLog::trace("RequestRender " caller); RequestRender(); }
#define CHECK_RENDER(caller) { CRLog::trace("CheckRender " caller); CheckRender(); }
#else
#define REQUEST_RENDER(caller) RequestRender();
#define CHECK_RENDER(caller) RenderIfDirty();
#endif

static const css_font_family_t DEF_FONT_FAMILY = css_ff_sans_serif;

LVDocView::LVDocView()
		: stream_(NULL),
          cr_dom_(NULL),
          viewport_mode_(MODE_PAGES),
		  page_(0),
		  offset_(0),
		  is_rendered_(false),
		  highlight_bookmarks_(1),
		  margins_(),
		  show_cover_(false),
          background_tiled_(true),
		  position_is_set_(false),
		  doc_format_(DOC_FORMAT_NULL),
		  width_(200),
          height_(400),
          page_columns_(1),
		  background_color_(0xFFFFFFE0),
		  text_color_(0x000060),
          config_margins_(),
          config_font_size_(24),
          config_interline_space_(100),
          config_embeded_styles_(false),
          config_embeded_fonts_(false),
          config_enable_footnotes_(true),
          config_txt_smart_format_(false)
{
	config_font_face_ = lString8("Arial, Roboto");
	base_font_ = fontMan->GetFont(
	        config_font_size_,
	        400,
	        false,
	        DEF_FONT_FAMILY,
	        config_font_face_);
	doc_props_ = LVCreatePropsContainer();
	CreateEmptyDom();
}

LVDocView::~LVDocView()
{
	Clear();
}

void LVDocView::Clear()
{
	if (cr_dom_) {
		delete cr_dom_;
		cr_dom_ = NULL;
	}
	page_ = 0;
	offset_ = 0;
	position_is_set_ = false;
	show_cover_ = false;
	is_rendered_ = false;
	bookmark_ = ldomXPointer();
	bookmark_.clear();
	doc_props_->clear();
	if (!stream_.isNull()) {
		stream_.Clear();
	}
	if (!container_.isNull()) {
		container_.Clear();
	}
	if (!archive_container_.isNull()) {
		archive_container_.Clear();
	}
}

void LVDocView::CreateEmptyDom()
{
	Clear();
	cr_dom_ = new CrDom();
	doc_format_ = DOC_FORMAT_NULL;
	cr_dom_->setProps(doc_props_);
	cr_dom_->setDocFlags(0);
	cr_dom_->setDocFlag(DOC_FLAG_ENABLE_FOOTNOTES, config_enable_footnotes_);
	cr_dom_->setDocFlag(DOC_FLAG_EMBEDDED_STYLES, config_embeded_styles_);
	cr_dom_->setDocFlag(DOC_FLAG_EMBEDDED_FONTS, config_embeded_fonts_);
	cr_dom_->setDocParentContainer(container_);
	cr_dom_->setNodeTypes(fb2_elem_table);
	cr_dom_->setAttributeTypes(fb2_attr_table);
	cr_dom_->setNameSpaceTypes(fb2_ns_table);
	marked_ranges_.clear();
    bookmark_ranges_.clear();
    // SHOULD BE CALLED ONLY AFTER setNodeTypes
    cr_dom_->setStylesheet(CR_CSS, true);
}

void LVDocView::RenderIfDirty()
{
	if (is_rendered_) {
		return;
	}
    is_rendered_ = true;
    position_is_set_ = false;
	if (cr_dom_ && cr_dom_->getRootNode() != NULL) {
        int	dx = page_rects_[0].width() - margins_.left - margins_.right;
        int	dy = page_rects_[0].height() - margins_.top - margins_.bottom;
        CheckRenderProps(dx, dy);
        if (base_font_.isNull()) {
            CRLog::error("RenderIfDirty base_font_.isNull()");
            return;
        }
        int y0 = show_cover_ ? dy + margins_.bottom * 4 : 0;
        cr_dom_->render(&pages_list_, dx, dy, show_cover_, y0, base_font_, config_interline_space_);
        fontMan->gc();
        is_rendered_ = true;
        UpdateSelections();
        UpdateBookmarksRanges();
    }
}

/// Invalidate formatted data, request render
void LVDocView::RequestRender()
{
	is_rendered_ = false;
	cr_dom_->clearRendBlockCache();
}

/// Ensure current position is set to current bookmark value
void LVDocView::CheckPos() {
    CHECK_RENDER("CheckPos()");
	if (position_is_set_) {
		return;
	}
	position_is_set_ = true;
	if (bookmark_.isNull()) {
		if (IsPagesMode()) {
			GoToPage(0);
		} else {
			GoToOffset(0, false);
		}
	} else {
		if (IsPagesMode()) {
            GoToPage(GetPageForBookmark(bookmark_), false);
		} else {
			lvPoint pt = bookmark_.toPoint();
			GoToOffset(pt.y, false);
		}
	}
}

void LVDocView::UpdatePageMargins()
{
    int new_margin_left = config_margins_.left;
    int new_margin_right = config_margins_.right;
    if (gFlgFloatingPunctuationEnabled) {
        int align = 0;
        base_font_ = fontMan->GetFont(
                config_font_size_,
                400,
                false,
                DEF_FONT_FAMILY,
                config_font_face_);
        align = base_font_->getVisualAligmentWidth() / 2;
        if (align > new_margin_right) {
            align = new_margin_right;
        }
        new_margin_left += align;
        new_margin_right -= align;
    }
    if (margins_.left != new_margin_left
        || margins_.right != new_margin_right
        || margins_.top != config_margins_.top
        || margins_.bottom != config_margins_.bottom) {
        margins_.left = new_margin_left;
        margins_.right = new_margin_right;
        margins_.top = config_margins_.top;
        margins_.bottom = config_margins_.bottom;
        UpdateLayout();
        REQUEST_RENDER("UpdatePageMargins")
    }
}

void LVDocView::Resize(int width, int height)
{
    if (width < 80 || width > 5000) {
        width = 80;
    }
    if (height < 80 || height > 5000) {
        height = 80;
    }
    if (width == width_ && height == height_) {
        CRLog::trace("Size is not changed: %dx%d", width, height);
        return;
    }
    width_ = width;
    height_ = height;
    if (cr_dom_) {
        UpdateLayout();
        REQUEST_RENDER("resize")
        position_is_set_ = false;
        //goToBookmark(_posBookmark);
        //updateBookMarksRanges();
    }
}

bool LVDocView::LoadDoc(int doc_format, const char* cr_uri_chars)
{
    LVStreamRef stream;
    lString16 cre_uri(cr_uri_chars);
    lString16 to_archive_path;
    lString16 in_archive_path;
    if (LVSplitArcName(cre_uri, to_archive_path, in_archive_path)) {
        // Doc is inside archive
        stream = LVOpenFileStream(to_archive_path.c_str(), LVOM_READ);
        if (stream.isNull()) {
            CRLog::error("Cannot open archive file %s", LCSTR(to_archive_path));
            return false;
        }
        container_ = LVOpenArchieve(stream);
        if (container_.isNull()) {
            CRLog::error("Cannot read archive contents from %s", LCSTR(to_archive_path));
            return false;
        }
        stream = container_->OpenStream(in_archive_path.c_str(), LVOM_READ);
        if (stream.isNull()) {
            CRLog::error("Cannot open stream to file in archive %s", LCSTR(cre_uri));
            return false;
        }
        doc_props_->setString(DOC_PROP_ARC_NAME, LVExtractFilename(to_archive_path));
        doc_props_->setString(DOC_PROP_ARC_PATH, LVExtractPath(to_archive_path));
        doc_props_->setString(DOC_PROP_FILE_NAME, in_archive_path);
    } else {
        lString16 doc_file_name = LVExtractFilename(cre_uri);
        lString16 path_to_doc = LVExtractPath(cre_uri);
        container_ = LVOpenDirectory(path_to_doc.c_str());
        if (container_.isNull()) {
            CRLog::error("Cannot open dir %s", LCSTR(path_to_doc));
            return false;
        }
        stream = container_->OpenStream(doc_file_name.c_str(), LVOM_READ);
        if (!stream) {
            CRLog::error("Cannot open file %s", LCSTR(doc_file_name));
            return false;
        }
        doc_props_->setString(DOC_PROP_FILE_PATH, path_to_doc);
        doc_props_->setString(DOC_PROP_FILE_NAME, doc_file_name);
    }
    if (LoadDoc(doc_format, stream)) {
        stream_.Clear();
        return true;
    } else {
        CreateEmptyDom();
        CRLog::error("Doc stream parsing fail");
        return false;
    }
}

bool LVDocView::LoadDoc(int doc_format, LVStreamRef stream)
{
    stream_ = stream;
    doc_format_ = doc_format;
    CheckRenderProps(0, 0);
    LVFileFormatParser* parser = nullptr;
    if (doc_format == DOC_FORMAT_FB2) {
        LvDomWriter writer(cr_dom_);
        parser = new LvXmlParser(stream_, &writer, false, true);
    } else if (doc_format == DOC_FORMAT_EPUB) {
        if (!DetectEpubFormat(stream_)) {
            return false;
        }
        cr_dom_->setProps(doc_props_);
        if (!ImportEpubDocument(stream_, cr_dom_)) {
            return false;
        }
        container_ = cr_dom_->getDocParentContainer();
        doc_props_ = cr_dom_->getProps();
        archive_container_ = cr_dom_->getDocParentContainer();
    } else if (doc_format == DOC_FORMAT_MOBI) {
        doc_format_t pdb_format = doc_format_none;
        if (!DetectPDBFormat(stream_, pdb_format)) {
            return false;
        }
        cr_dom_->setProps(doc_props_);
        if (pdb_format != doc_format_mobi) {
            CRLog::error("pdb_format != doc_format_mobi");
        }
        if (!ImportPDBDocument(stream_, cr_dom_, pdb_format)) {
            if (pdb_format != doc_format_mobi) {
                CRLog::error("pdb_format != doc_format_mobi");
            }
            return false;
        }
    } else if (doc_format == DOC_FORMAT_DOC) {
#if ENABLE_ANTIWORD == 1
        if (!DetectWordFormat(stream_)) {
            return false;
        }
        cr_dom_->setProps(doc_props_);
        if (!ImportWordDocument(stream_, cr_dom_)) {
            return false;
        }
        archive_container_ = cr_dom_->getDocParentContainer();
#endif //ENABLE_ANTIWORD == 1
    } else if (doc_format == DOC_FORMAT_RTF) {
        LvDomWriter writer(cr_dom_);
        parser = new LVRtfParser(stream_, &writer);
    } else if (doc_format == DOC_FORMAT_CHM) {
        if (!DetectCHMFormat(stream_)) {
            return false;
        }
        cr_dom_->setProps(doc_props_);
        if (!ImportCHMDocument(stream_, cr_dom_)) {
            return false;
        }
        archive_container_ = cr_dom_->getDocParentContainer();
    } else if (doc_format == DOC_FORMAT_TXT) {
        LvDomWriter writer(cr_dom_);
        parser = new LVTextParser(stream_, &writer, config_txt_smart_format_);
    } else if (doc_format == DOC_FORMAT_HTML) {
        LvDomAutocloseWriter writer(cr_dom_, false, HTML_AUTOCLOSE_TABLE);
        parser = new LvHtmlParser(stream_, &writer);
    }
    if (parser) {
        if (!parser->CheckFormat()) {
            delete parser;
            return false;
        }
        if (!parser->Parse()) {
            delete parser;
            return false;
        }
        delete parser;
    }
    offset_ = 0;
    page_ = 0;
#if 0
    lString16 stylesheet = cr_dom_->createXPointer(L"/FictionBook/stylesheet").getText();
    if (!stylesheet.empty() && config_embeded_styles_) {
        cr_dom_->getStylesheet()->parse(UnicodeToUtf8(stylesheet).c_str());
        cr_dom_->setStylesheet(UnicodeToUtf8(stylesheet).c_str(), false);
    }
#endif
    //show_cover_ = !getCoverPageImage().isNull();
    CheckRenderProps(0, 0);
    REQUEST_RENDER("LoadDoc")
    return true;
}

/// returns cover page image source, if any
LVImageSourceRef LVDocView::getCoverPageImage()
{
    lUInt16 path[] = {
            el_FictionBook,
            el_description,
            el_title_info,
            el_coverpage,
            //el_image,
            0};
    ldomNode* cover_el = cr_dom_->getRootNode()->findChildElement(path);
    if (cover_el) {
        ldomNode* cover_img_el = cover_el->findChildElement(LXML_NS_ANY, el_image, 0);
        if (cover_img_el) {
            LVImageSourceRef imgsrc = cover_img_el->getObjectImageSource();
            return imgsrc;
        }
    }
    // Not found: return NULL ref
    return LVImageSourceRef();
}

/// Draws coverpage to image buffer
void LVDocView::DrawCoverTo(LVDrawBuf* drawBuf, lvRect& rc)
{
    if (rc.width() < 130 || rc.height() < 130) {
        return;
    }
    lvRect imgrc = rc;
    LVImageSourceRef imgsrc = getCoverPageImage();
    if (!imgsrc.isNull() && imgrc.height() > 30) {
        int src_dx = imgsrc->GetWidth();
        int src_dy = imgsrc->GetHeight();
        int scale_x = imgrc.width() * 0x10000 / src_dx;
        int scale_y = imgrc.height() * 0x10000 / src_dy;
        if (scale_x < scale_y) {
            scale_y = scale_x;
        } else {
            scale_x = scale_y;
        }
        int dst_dx = (src_dx * scale_x) >> 16;
        int dst_dy = (src_dy * scale_y) >> 16;
        if (dst_dx > rc.width() * 6 / 8) {
            dst_dx = imgrc.width();
        }
        if (dst_dy > rc.height() * 6 / 8) {
            dst_dy = imgrc.height();
        }
        LVColorDrawBuf buf2(src_dx, src_dy, 32);
        buf2.Draw(imgsrc, 0, 0, src_dx, src_dy, true);
        drawBuf->DrawRescaled(
                &buf2,
                imgrc.left + (imgrc.width() - dst_dx) / 2,
                imgrc.top + (imgrc.height() - dst_dy) / 2,
                dst_dx,
                dst_dy,
                0);
    } else {
        imgrc.bottom = imgrc.top;
    }
    rc.top = imgrc.bottom;
}

int LVDocView::GetFullHeight()
{
    CHECK_RENDER("getFullHeight()");
    RenderRectAccessor rd(cr_dom_->getRootNode());
    return (rd.getHeight() + rd.getY());
}

#define FOOTNOTE_MARGIN 8
void LVDocView::DrawPageTo(LVDrawBuf* drawbuf, LVRendPageInfo& page, lvRect* pageRect)
{
    int start = page.start;
    int height = page.height;
    lvRect fullRect(0, 0, drawbuf->GetWidth(), drawbuf->GetHeight());
    if (!pageRect) {
        pageRect = &fullRect;
    }
    drawbuf->setHidePartialGlyphs(IsPagesMode());
    //int offset = (pageRect->height() - m_pageMargins.top - m_pageMargins.bottom - height) / 3;
    //if (offset>16)
    //    offset = 16;
    //if (offset<0)
    //    offset = 0;
    int offset = 0;
    lvRect clip;
    clip.left = pageRect->left + margins_.left;
    clip.top = pageRect->top + margins_.top + offset;
    clip.bottom = pageRect->top + margins_.top + height + offset;
    clip.right = pageRect->left + pageRect->width() - margins_.right;
    if (page.type == PAGE_TYPE_COVER) {
        clip.top = pageRect->top + margins_.top;
    }
    drawbuf->SetClipRect(&clip);
    if (!cr_dom_) {
        drawbuf->SetClipRect(NULL);
        return;
    }
    if (page.type == PAGE_TYPE_COVER) {
        lvRect rc = *pageRect;
        drawbuf->SetClipRect(&rc);
        /*
         if ( m_pageMargins.bottom > m_pageMargins.top )
            rc.bottom -= m_pageMargins.bottom - m_pageMargins.top;
         rc.left += m_pageMargins.left / 2;
         rc.top += m_pageMargins.bottom / 2;
         rc.right -= m_pageMargins.right / 2;
         rc.bottom -= m_pageMargins.bottom / 2;
         */
        DrawCoverTo(drawbuf, rc);
        drawbuf->SetClipRect(NULL);
        return;
    }
    // draw main page text
    if (marked_ranges_.length()) {
        CRLog::trace("Entering DrawDocument(): %d ranges", marked_ranges_.length());
    }
    if (page.height) {
        DrawDocument(*drawbuf,
                cr_dom_->getRootNode(),
                pageRect->left + margins_.left,
                clip.top,
                pageRect->width() - margins_.left - margins_.right,
                height,
                0,
                -start + offset,
                height_,
                &marked_ranges_,
                &bookmark_ranges_);
    }
    // draw footnotes
    int fny = clip.top + (page.height ? page.height + FOOTNOTE_MARGIN : FOOTNOTE_MARGIN);
    int fy = fny;
    bool footnoteDrawed = false;
    for (int fn = 0; fn < page.footnotes.length(); fn++) {
        int fstart = page.footnotes[fn].start;
        int fheight = page.footnotes[fn].height;
        clip.top = fy + offset;
        clip.left = pageRect->left + margins_.left;
        clip.right = pageRect->right - margins_.right;
        clip.bottom = fy + offset + fheight;
        drawbuf->SetClipRect(&clip);
        DrawDocument(
                *drawbuf,
                cr_dom_->getRootNode(),
                pageRect->left + margins_.left,
                fy + offset,
                pageRect->width() - margins_.left - margins_.right,
                fheight,
                0,
                -fstart + offset,
                height_,
                &marked_ranges_);
        footnoteDrawed = true;
        fy += fheight;
    }
    if (footnoteDrawed) { // && page.height
        fny -= FOOTNOTE_MARGIN / 2;
        drawbuf->SetClipRect(NULL);
        lUInt32 cl = drawbuf->GetTextColor();
        cl = (cl & 0xFFFFFF) | (0x55000000);
        drawbuf->FillRect(
                pageRect->left + margins_.left,
                fny,
                pageRect->right - margins_.right,
                fny + 1,
                cl);
    }
    drawbuf->SetClipRect(NULL);
}

/// returns page count
int LVDocView::GetPagesCount()
{
	return (pages_list_.length());
}

/// get vertical position of view inside document
int LVDocView::GetOffset() {
    CheckPos();
    if (IsPagesMode() && page_ >= 0 && page_ < pages_list_.length()) {
    	return pages_list_[page_]->start;
    }
	return offset_;
}

int LVDocView::GetCurrPage()
{
	CheckPos();
	if (IsPagesMode() && page_ >= 0) {
		return page_;
	}
	return pages_list_.FindNearestPage(offset_, 0);
}

int LVDocView::GoToOffset(int offset, bool update_bookmark, bool allowScrollAfterEnd)
{
	position_is_set_ = true;
    CHECK_RENDER("setPos()")
	if (IsScrollMode()) {
		if (offset > GetFullHeight() - height_ && !allowScrollAfterEnd) {
			offset = GetFullHeight() - height_;
		}
		if (offset < 0) {
			offset = 0;
		}
		offset_ = offset;
		int page = pages_list_.FindNearestPage(offset, 0);
		if (page >= 0 && page < pages_list_.length()) {
			page_ = page;
		} else {
			page_ = -1;
		}
	} else {
		int page = pages_list_.FindNearestPage(offset, 0);
		//if (GetColumns() == 2) {
		//	page &= ~1;
		//}
		if (page < pages_list_.length()) {
			offset_ = pages_list_[page]->start;
			page_ = page;
		} else {
			offset_ = 0;
			page_ = 0;
		}
	}
	if (update_bookmark) {
		bookmark_ = GetBookmark();
	}
	position_is_set_ = true;
    UpdateScrollInfo();
	return 1;
}

bool LVDocView::GoToPage(int page, bool update_bookmark)
{
	//CRLog::trace("LVDocView::GoToPage START page=%d, page_=%d", page, page_);
    CHECK_RENDER("GoToPage()")
	if (!pages_list_.length()) {
		return false;
	}
	bool res = true;
	if (IsPagesMode()) {
		if (page >= pages_list_.length()) {
			page = pages_list_.length() - 1;
			res = false;
		}
		if (page < 0) {
			page = 0;
			res = false;
		}
		//if (GetColumns() == 2) {
		//	page &= ~1;
		//}
		if (page >= 0 && page < pages_list_.length()) {
			offset_ = pages_list_[page]->start;
			page_ = page;
		} else {
			offset_ = 0;
			page_ = 0;
			res = false;
		}
	} else {
		if (page >= 0 && page < pages_list_.length()) {
			offset_ = pages_list_[page]->start;
			page_ = page;
		} else {
			res = false;
			offset_ = 0;
			page_ = 0;
		}
	}
	// Нужно сначала выполнить position_is_set_ = true и только
	// затем вызывать GetBookmark(), иначе при первом вызове GoToPage,
	// CheckPos(), вызванный в GetBookmark() сбросит в 0
	// только что установленную нами page_
	position_is_set_ = true;
    if (update_bookmark) {
        bookmark_ = GetBookmark();
    }
    UpdateScrollInfo();
    if (res) {
        UpdateBookmarksRanges();
    }
    //CRLog::trace("LVDocView::GoToPage END page=%d, page_=%d", page, page_);
	return res;
}

/// Check whether resize or creation of buffer is necessary, ensure buffer is ok
static bool CheckBufferSize(LVRef<LVColorDrawBuf>& buf, int dx, int dy) {
    if (buf.isNull() || buf->GetWidth() != dx || buf->GetHeight() != dy) {
        buf.Clear();
        buf = LVRef<LVColorDrawBuf>(new LVColorDrawBuf(dx, dy, 16));
        return false; // need redraw
    } else {
    	return true;
    }
}

void LVDocView::SetBackgroundImage(LVImageSourceRef image, bool tiled)
{
    background_image = image;
    background_tiled_ = tiled;
    background_image_scaled_.Clear();
}

/// clears page background
void LVDocView::DrawBackgroundTo(LVDrawBuf& buf, int offsetX, int offsetY, int alpha)
{
    buf.SetBackgroundColor(background_color_);
    if (background_image.isNull()) {
    	// Solid color
		lUInt32 cl = background_color_;
		if (alpha > 0) {
			cl = (cl & 0xFFFFFF) | (alpha << 24);
			buf.FillRect(0, 0, buf.GetWidth(), buf.GetHeight(), cl);
		} else {
			buf.Clear(cl);
		}
    } else {
        // Texture
        int dx = buf.GetWidth();
        int dy = buf.GetHeight();
        if (background_tiled_) {
            if (!CheckBufferSize(background_image_scaled_,
            		background_image->GetWidth(), background_image->GetHeight())) {
                // unpack
                background_image_scaled_->Draw(
                        LVCreateAlphaTransformImageSource(background_image, alpha),
                		0,
                		0,
                		background_image->GetWidth(),
                		background_image->GetHeight(),
                		false);
            }
            LVImageSourceRef src = LVCreateDrawBufImageSource(
                    background_image_scaled_.get(),
                    false);
            LVImageSourceRef tile = LVCreateTileTransform( src, dx, dy, offsetX, offsetY );
            buf.Draw(LVCreateAlphaTransformImageSource(tile, alpha), 0, 0, dx, dy);
            //CRLog::trace("draw completed");
        } else {
            if (IsScrollMode()) {
                // scroll
                if (!CheckBufferSize(background_image_scaled_, dx, background_image->GetHeight())) {
                    // unpack
                    LVImageSourceRef resized = LVCreateStretchFilledTransform(
                            background_image,
                            dx,
                    		background_image->GetHeight(),
                    		IMG_TRANSFORM_STRETCH,
                    		IMG_TRANSFORM_TILE,
                    		0,
                    		0);
                    background_image_scaled_->Draw(
                            LVCreateAlphaTransformImageSource(resized, alpha),
                    		0,
                    		0,
                    		dx,
                    		background_image->GetHeight(),
                    		false);
                }
                LVImageSourceRef src = LVCreateDrawBufImageSource(
                        background_image_scaled_.get(),
                        false);
                LVImageSourceRef resized = LVCreateStretchFilledTransform(
                        src,
                        dx,
                        dy,
                		IMG_TRANSFORM_TILE,
                		IMG_TRANSFORM_TILE,
                		offsetX,
                		offsetY);
                buf.Draw(LVCreateAlphaTransformImageSource(resized, alpha), 0, 0, dx, dy);
            } else if (GetColumns() != 2) {
                // single page
                if (!CheckBufferSize( background_image_scaled_, dx, dy)) {
                    // unpack
                    LVImageSourceRef resized = LVCreateStretchFilledTransform(
                            background_image,
                            dx,
                            dy,
                    		IMG_TRANSFORM_STRETCH,
                    		IMG_TRANSFORM_STRETCH,
                    		offsetX,
                    		offsetY);
                    background_image_scaled_->Draw(
                            LVCreateAlphaTransformImageSource(resized, alpha),
                    		0,
                    		0,
                    		dx,
                    		dy,
                    		false);
                }
                LVImageSourceRef src = LVCreateDrawBufImageSource(
                        background_image_scaled_.get(),
                        false);
                buf.Draw(LVCreateAlphaTransformImageSource(src, alpha), 0, 0, dx, dy);
            } else {
                // two pages
                int halfdx = (dx + 1) / 2;
                if (!CheckBufferSize( background_image_scaled_, halfdx, dy)) {
                    // unpack
                    LVImageSourceRef resized = LVCreateStretchFilledTransform(
                            background_image,
                            halfdx,
                            dy,
                    		IMG_TRANSFORM_STRETCH,
                    		IMG_TRANSFORM_STRETCH,
                    		offsetX,
                    		offsetY);
                    background_image_scaled_->Draw(
                            LVCreateAlphaTransformImageSource(resized, alpha),
                    		0,
                    		0,
                    		halfdx,
                    		dy,
                    		false);
                }
                LVImageSourceRef src = LVCreateDrawBufImageSource(
                        background_image_scaled_.get(),
                        false);
                buf.Draw(LVCreateAlphaTransformImageSource(src, alpha), 0, 0, halfdx, dy);
                buf.Draw(LVCreateAlphaTransformImageSource(src, alpha), dx/2, 0, dx - halfdx, dy);
            }
        }
    }
    /*
    Рисуем разделительную полосу между страницами в двухколоночном режиме
    if (buf.GetBitsPerPixel() == 32 && GetPagesColumns() == 2) {
        int x = buf.GetWidth() / 2;
        lUInt32 cl = background_color_;
        cl = ((cl & 0xFCFCFC) + 0x404040) >> 1;
        buf.FillRect(x, 0, x + 1, buf.GetHeight(), cl);
    }
    */
}

bool LVDocView::DrawImageTo(LVDrawBuf* buf, LVImageSourceRef img, int x, int y, int dx, int dy)
{
    if (img.isNull() || !buf)
        return false;
    // clear background
    DrawBackgroundTo(*buf, 0, 0);
    // draw image
    buf->Draw(img, x, y, dx, dy, true);
    return true;
}

/// draw current page to specified buffer
void LVDocView::Draw(LVDrawBuf& buf, bool auto_resize)
{
	int offset = -1;
	int page = -1;
	if (IsPagesMode()) {
		page = page_;
		if (page < 0 || page >= pages_list_.length()) {
			return;
		}
	} else {
		offset = offset_;
	}
	if (auto_resize) {
		buf.Resize(width_, height_);
	}
	buf.SetBackgroundColor(background_color_);
	buf.SetTextColor(text_color_);
	if (!is_rendered_ || !cr_dom_ || base_font_.isNull()) {
		return;
	}
	if (IsScrollMode()) {
		buf.SetClipRect(NULL);
        DrawBackgroundTo(buf, 0, offset);
        int cover_height = 0;
		if (pages_list_.length() > 0 && pages_list_[0]->type == PAGE_TYPE_COVER)
			cover_height = pages_list_[0]->height;
		if (offset < cover_height) {
			lvRect rc;
			buf.GetClipRect(&rc);
			rc.top -= offset;
			rc.bottom -= offset;
			rc.top += margins_.top;
			rc.bottom -= margins_.bottom;
			rc.left += margins_.left;
			rc.right -= margins_.right;
			DrawCoverTo(&buf, rc);
		}
		DrawDocument(buf,
                     cr_dom_->getRootNode(),
                     margins_.left,
                     0,
                     buf.GetWidth()	- margins_.left - margins_.right,
                     buf.GetHeight(),
                     0,
                     -offset,
                     buf.GetHeight(),
                     &marked_ranges_,
                     &bookmark_ranges_);
	} else {
		if (page == -1) {
			page = pages_list_.FindNearestPage(offset, 0);
		}
        DrawBackgroundTo(buf, 0, 0);
        if (page >= 0 && page < pages_list_.length()) {
        	DrawPageTo(&buf, *pages_list_[page], &page_rects_[0]);
        }
		if (GetColumns() == 2 && page >= 0 && page + 1 < pages_list_.length()) {
			DrawPageTo(&buf, *pages_list_[page + 1], &page_rects_[1]);
		}
	}
}

/// converts point from window to document coordinates, returns true if success
bool LVDocView::WindowToDocPoint(lvPoint & pt) {
    CHECK_RENDER("windowToDocPoint()")
	if (IsScrollMode()) {
		pt.y += offset_;
		pt.x -= margins_.left;
		return true;
	} else {
		int page = GetCurrPage();
		lvRect* rc = NULL;
		lvRect page1(page_rects_[0]);
		page1.left += margins_.left;
		page1.top += margins_.top;
		page1.right -= margins_.right;
		page1.bottom -= margins_.bottom;
		lvRect page2;
		if (page1.isPointInside(pt)) {
			rc = &page1;
		} else if (GetColumns() == 2) {
			page2 = page_rects_[1];
			page2.left += margins_.left;
			page2.top += margins_.top;
			page2.right -= margins_.right;
			page2.bottom -= margins_.bottom;
			if (page2.isPointInside(pt)) {
				rc = &page2;
				page++;
			}
		}
		if (rc && page >= 0 && page < pages_list_.length()) {
			int page_y = pages_list_[page]->start;
			pt.x -= rc->left;
			pt.y -= rc->top;
			if (pt.y < pages_list_[page]->height) {
				//CRLog::debug(" point page offset( %d, %d )", pt.x, pt.y );
				pt.y += page_y;
				return true;
			}
		}
	}
	return false;
}

/// converts point from document to window coordinates, returns true if success
bool LVDocView::DocToWindowRect(lvRect& rect) {
    CHECK_RENDER("DocToWindowRect()")
	if (IsScrollMode()) {
		rect.top -= offset_;
        rect.bottom -= offset_;
		rect.left += margins_.left;
        rect.right += margins_.left;
		return true;
	} else {
        int page = GetCurrPage();
        if (page >= 0 && page < pages_list_.length() && rect.top >= pages_list_[page]->start) {
            int index = -1;
            if (rect.bottom <= (pages_list_[page]->start + pages_list_[page]->height)) {
                index = 0;
            } else if (GetColumns() == 2 && page + 1 < pages_list_.length() &&
                rect.bottom <= (pages_list_[page + 1]->start + pages_list_[page + 1]->height)) {
                index = 1;
            }
            if (index >= 0) {
                int right = rect.right + page_rects_[index].left + margins_.left;
                if (right < page_rects_[index].right - margins_.right) {
                    rect.left = rect.left + page_rects_[index].left + margins_.left;
                    rect.right = right;
                    rect.top = rect.top + margins_.top - pages_list_[page + index]->start;
                    rect.bottom = rect.bottom + margins_.top - pages_list_[page + index]->start;
                    return true;
                }
            }
        }
        return false;
	}
	return false;
}

void LVDocView::UpdateLayout() {
	lvRect rc(0, 0, width_, height_);
	page_rects_[0] = rc;
	page_rects_[1] = rc;
	if (GetColumns() == 2) {
		int middle = (rc.left + rc.right) >> 1;
        page_rects_[0].right = middle; // - m_pageMargins.right;
        page_rects_[1].left = middle; // + m_pageMargins.left;
	}
}

/// Call setRenderProps(0, 0) to allow apply styles and rend method while loading.
void LVDocView::CheckRenderProps(int width, int height)
{
	if (!cr_dom_ || cr_dom_->getRootNode() == NULL) {
		return;
	}
	UpdateLayout();
	base_font_ = fontMan->GetFont(
            config_font_size_,
            400,
            false,
            DEF_FONT_FAMILY,
            config_font_face_);
	if (!base_font_) {
		return;
	}
    cr_dom_->setRenderProps(width, height, base_font_, config_interline_space_);
    text_highlight_options_t h;
    h.bookmarkHighlightMode = highlight_mode_underline;
    h.selectionColor = 0xC0C0C0 & 0xFFFFFF;
    h.commentColor = 0xA08000 & 0xFFFFFF;
    h.correctionColor = 0xA00000 & 0xFFFFFF;
    cr_dom_->setHightlightOptions(h);
}

/// returns xpointer for specified window point
ldomXPointer LVDocView::getNodeByPoint(lvPoint pt) {
    CHECK_RENDER("getNodeByPoint()")
    if (WindowToDocPoint(pt) && cr_dom_) {
        ldomXPointer ptr = cr_dom_->createXPointer(pt);
        //CRLog::debug("ptr (%d, %d) node=%08X offset=%d",
        //      pt.x, pt.y, (lUInt32)ptr.getNode(), ptr.getOffset() );
        return ptr;
    }
    return ldomXPointer();
}

/// returns image source for specified window point, if point is inside image
LVImageSourceRef LVDocView::getImageByPoint(lvPoint pt) {
    LVImageSourceRef res = LVImageSourceRef();
    ldomXPointer ptr = getNodeByPoint(pt);
    if (ptr.isNull()) {
        return res;
    }
    res = ptr.getNode()->getObjectImageSource();
    if (!res.isNull()) {
        CRLog::debug("getImageByPoint(%d, %d): found image %d x %d",
                     pt.x, pt.y, res->GetWidth(), res->GetHeight());
    }
    return res;
}

/// sets selection for whole element, clears previous selection
void LVDocView::selectElement(ldomNode * elem) {
	ldomXRangeList & sel = GetCrDom()->getSelections();
	sel.clear();
	sel.add(new ldomXRange(elem));
	UpdateSelections();
}

/// sets selection for list of words, clears previous selection
void LVDocView::selectWords(const LVArray<ldomWord> & words) {
	ldomXRangeList & sel = GetCrDom()->getSelections();
	sel.clear();
	sel.addWords(words);
	UpdateSelections();
}

/// sets selections for ranges, clears previous selections
void LVDocView::selectRanges(ldomXRangeList & ranges) {
    ldomXRangeList & sel = GetCrDom()->getSelections();
    if (sel.empty() && ranges.empty())
        return;
    sel.clear();
    for (int i=0; i<ranges.length(); i++) {
        ldomXRange * item = ranges[i];
        sel.add(new ldomXRange(*item));
    }
    UpdateSelections();
}

/// sets selection for range, clears previous selection
void LVDocView::selectRange(const ldomXRange & range) {
    // LVE:DEBUG
//    ldomXRange range2(range);
//    CRLog::trace("selectRange( %s, %s )",
//                LCSTR(range2.getStart().toString()), LCSTR(range2.getEnd().toString()) );
	ldomXRangeList & sel = GetCrDom()->getSelections();
	if (sel.length() == 1) {
		if (range == *sel[0])
			return; // the same range is set
	}
	sel.clear();
	sel.add(new ldomXRange(range));
	UpdateSelections();
}

void LVDocView::ClearSelection() {
	ldomXRangeList& sel = GetCrDom()->getSelections();
	sel.clear();
	UpdateSelections();
}

/// selects first link on page, if any. returns selected link range, null if no links.
ldomXRange* LVDocView::SelectFirstPageLink() {
	ldomXRangeList list;
	GetCurrentPageLinks(list);
	if (!list.length())
		return NULL;
	selectRange(*list[0]);
	ldomXRangeList& sel = GetCrDom()->getSelections();
	UpdateSelections();
	return sel[0];
}

/// update selection ranges
void LVDocView::UpdateSelections() {
    CHECK_RENDER("updateSelections()")
	ldomXRangeList ranges(cr_dom_->getSelections(), true);
    CRLog::trace("updateSelections() : selection count = %d", cr_dom_->getSelections().length());
	ranges.getRanges(marked_ranges_);
}

void LVDocView::UpdateBookmarksRanges()
{
	CHECK_RENDER("UpdateBookmarksRanges()")
    ldomXRangeList ranges;
    //TODO AXY
    if (highlight_bookmarks_) {
		for (int i = 0; i < bookmarks_.length(); i++) {
			CRBookmark * bmk = bookmarks_[i];
			int t = bmk->getType();
			if (t != bmkt_lastpos) {
				ldomXPointer p = cr_dom_->createXPointer(bmk->getStartPos());
				if (p.isNull())
					continue;
				lvPoint pt = p.toPoint();
				if (pt.y < 0)
					continue;
				ldomXPointer ep = (t == bmkt_pos)
				        ? p
				        : cr_dom_->createXPointer(bmk->getEndPos());
				if (ep.isNull())
					continue;
				lvPoint ept = ep.toPoint();
				if (ept.y < 0)
					continue;
				ldomXRange *n_range = new ldomXRange(p, ep);
				if (!n_range->isNull()) {
					int flags = 1;
					if (t == bmkt_pos)
						flags = 2;
					if (t == bmkt_comment)
						flags = 4;
					if (t == bmkt_correction)
						flags = 8;
					n_range->setFlags(flags);
					ranges.add(n_range);
				} else
					delete n_range;
			}
		}
    }
    ranges.getRanges(bookmark_ranges_);
}

/// Minimum EM width of page (prevents show two pages for windows that not enougn wide)
#define MIN_EM_PER_PAGE 20
int LVDocView::GetColumns()
{
	if (viewport_mode_ == MODE_SCROLL
			|| width_ < config_font_size_ * MIN_EM_PER_PAGE
			|| width_ * 5 < height_ * 6) {
		return 1;
	}
	return page_columns_;
}
#undef MIN_EM_PER_PAGE

/// sets current bookmark
void LVDocView::SetBookmark(ldomXPointer bm) {
	bookmark_ = bm;
}

/// get view height
int LVDocView::GetHeight()
{
	return height_;
}

/// get view width
int LVDocView::GetWidth()
{
	return width_;
}

/// returns XPointer to middle paragraph of current page
ldomXPointer LVDocView::getCurrentPageMiddleParagraph() {
	CheckPos();
	ldomXPointer ptr;
	if (!cr_dom_)
		return ptr;

	if (IsScrollMode()) {
		// SCROLL mode
		int starty = offset_;
		int endy = offset_ + height_;
		int fh = GetFullHeight();
		if (endy >= fh)
			endy = fh - 1;
		ptr = cr_dom_->createXPointer(lvPoint(0, (starty + endy) / 2));
	} else {
		// PAGES mode
		int pageIndex = GetCurrPage();
		if (pageIndex < 0 || pageIndex >= pages_list_.length())
			pageIndex = GetCurrPage();
		LVRendPageInfo * page = pages_list_[pageIndex];
		if (page->type == PAGE_TYPE_NORMAL)
			ptr = cr_dom_->createXPointer(lvPoint(0, page->start + page->height	/ 2));
	}
	if (ptr.isNull())
		return ptr;
	ldomXPointerEx p(ptr);
	if (!p.isVisibleFinal())
		if (!p.ensureFinal())
			if (!p.prevVisibleFinal())
				if (!p.nextVisibleFinal())
					return ptr;
	return ldomXPointer(p);
}

/// returns bookmark
ldomXPointer LVDocView::GetBookmark() {
	CheckPos();
	ldomXPointer ptr;
	if (cr_dom_) {
		if (IsPagesMode()) {
			if (page_ >= 0 && page_ < pages_list_.length()) {
				ptr = cr_dom_->createXPointer(lvPoint(0, pages_list_[page_]->start));
			}
		} else {
			ptr = cr_dom_->createXPointer(lvPoint(0, offset_));
		}
	}
	return ptr;
}

/// returns bookmark for specified page
ldomXPointer LVDocView::getPageBookmark(int page) {
    CHECK_RENDER("getPageBookmark()")
	if (page < 0 || page >= pages_list_.length()) {
		return ldomXPointer();
	}
	ldomXPointer ptr = cr_dom_->createXPointer(lvPoint(0, pages_list_[page]->start));
	return ptr;
}

static lString16 GetSectionHeader(ldomNode* section)
{
    lString16 header;
    if (!section || section->getChildCount() == 0) {
        return header;
    }
    ldomNode* child = section->getChildElementNode(0, L"title");
    if (!child) {
        return header;
    }
    header = child->getText(L' ', 1024);
    return header;
}

/// get bookmark position text
bool LVDocView::getBookmarkPosText(ldomXPointer bm, lString16 & titleText, lString16 & posText)
{
	CHECK_RENDER("getBookmarkPosText")
    titleText = posText = lString16::empty_str;
	if (bm.isNull())
		return false;
	ldomNode * el = bm.getNode();
	if (el->isText()) {
        lString16 txt = bm.getNode()->getText();
		int startPos = bm.getOffset();
		int len = txt.length() - startPos;
		if (len > 0)
			txt = txt.substr(startPos, len);
		if (startPos > 0)
            posText = "...";
        posText += txt;
		el = el->getParentNode();
	} else {
        posText = el->getText(L' ', 1024);
	}
    bool inTitle = false;
	do {
		while (el && el->getNodeId() != el_section && el->getNodeId()
				!= el_body) {
			if (el->getNodeId() == el_title || el->getNodeId() == el_subtitle)
				inTitle = true;
			el = el->getParentNode();
		}
		if (el) {
			if (inTitle) {
				posText.clear();
				if (el->getChildCount() > 1) {
					ldomNode * node = el->getChildNode(1);
                    posText = node->getText(' ', 8192);
				}
				inTitle = false;
			}
			if (el->getNodeId() == el_body && !titleText.empty())
				break;
			lString16 txt = GetSectionHeader(el);
			lChar16 lastch = !txt.empty() ? txt[txt.length() - 1] : 0;
			if (!titleText.empty()) {
				if (lastch != '.' && lastch != '?' && lastch != '!')
                    txt += ".";
                txt += " ";
			}
			titleText = txt + titleText;
			el = el->getParentNode();
		}
		if (titleText.length() > 50)
			break;
	} while (el);
    limitStringSize(titleText, 70);
	limitStringSize(posText, 120);
	return true;
}

/// moves position to bookmark
void LVDocView::GoToBookmark(ldomXPointer bm) {
    CHECK_RENDER("goToBookmark()")
	position_is_set_ = false;
	bookmark_ = bm;
}

/// get page number by bookmark
int LVDocView::GetPageForBookmark(ldomXPointer bm) {
    CHECK_RENDER("getBookmarkPage()")
	if (bm.isNull()) {
		return 0;
	} else {
		lvPoint pt = bm.toPoint();
		if (pt.y < 0) {
			return 0;
		}
		return pages_list_.FindNearestPage(pt.y, 0);
	}
}

void LVDocView::UpdateScrollInfo()
{
	CheckPos();
	if (viewport_mode_ == MODE_SCROLL) {
		int npos = offset_;
		int fh = GetFullHeight();
		int shift = 0;
		int npage = height_;
		while (fh > 16384) {
			fh >>= 1;
			npos >>= 1;
			npage >>= 1;
			shift++;
		}
		if (npage < 1)
			npage = 1;
		scroll_info_.pos = npos;
		scroll_info_.maxpos = fh - npage;
		scroll_info_.pagesize = npage;
		scroll_info_.scale = shift;
		char str[32];
		sprintf(str, "%d%%", fh > 0 ? (100 * npos / fh) : 0);
		scroll_info_.posText = lString16(str);
	} else {
		int page = GetCurrPage();
		int vpc = GetColumns();
		scroll_info_.pos = page / vpc;
		scroll_info_.maxpos = (pages_list_.length() + vpc - 1) / vpc - 1;
		scroll_info_.pagesize = 1;
		scroll_info_.scale = 0;
		char str[32] = "";
		if (pages_list_.length() > 1) {
			if (page <= 0) {
				sprintf(str, "cover");
			} else {
                sprintf(str, "%d / %d", page, pages_list_.length() - 1);
            }
		}
		scroll_info_.posText = lString16(str);
	}
}

/// move to position specified by scrollbar
bool LVDocView::goToScrollPos(int pos) {
	if (viewport_mode_ == MODE_SCROLL) {
		GoToOffset(scrollPosToDocPos(pos));
		return true;
	} else {
		int vpc = this->GetColumns();
		int curPage = GetCurrPage();
		pos = pos * vpc;
		if (pos >= GetPagesCount())
			pos = GetPagesCount() - 1;
		if (pos < 0)
			pos = 0;
		if (curPage == pos)
			return false;
		GoToPage(pos);
		return true;
	}
}

/// converts scrollbar pos to doc pos
int LVDocView::scrollPosToDocPos(int scrollpos)
{
	if (viewport_mode_ == MODE_SCROLL) {
		int n = scrollpos << scroll_info_.scale;
		if (n < 0)
			n = 0;
		int fh = GetFullHeight();
		if (n > fh)
			n = fh;
		return n;
	} else {
		int vpc = GetColumns();
		int n = scrollpos * vpc;
		if (!pages_list_.length())
			return 0;
		if (n >= pages_list_.length())
			n = pages_list_.length() - 1;
		if (n < 0)
			n = 0;
		return pages_list_[n]->start;
	}
}

/// get page document range, -1 for current page
LVRef<ldomXRange> LVDocView::GetPageDocRange(int page_index) {
    CHECK_RENDER("getPageDocRange()")
    LVRef<ldomXRange> res(NULL);
    if (IsScrollMode()) {
        // SCROLL mode
        int starty = offset_;
        int endy = offset_ + height_;
        int fh = GetFullHeight();
        if (endy >= fh)
            endy = fh - 1;
        ldomXPointer start = cr_dom_->createXPointer(lvPoint(0, starty));
        ldomXPointer end = cr_dom_->createXPointer(lvPoint(0, endy));
        if (start.isNull() || end.isNull())
            return res;
        res = LVRef<ldomXRange>(new ldomXRange(start, end));
    } else {
        // PAGES mode
        if (page_index < 0 || page_index >= pages_list_.length()) {
            page_index = GetCurrPage();
        }
        LVRendPageInfo* page = pages_list_[page_index];
        if (page->type != PAGE_TYPE_NORMAL) {
            return res;
        }
        ldomXPointer start = cr_dom_->createXPointer(lvPoint(0, page->start));
        //ldomXPointer end = cr_dom_->createXPointer(lvPoint(m_dx + m_dy, page->start + page->height - 1));
        ldomXPointer end = cr_dom_->createXPointer(lvPoint(0, page->start + page->height), 1);
        if (start.isNull() || end.isNull()) {
            return res;
        }
        res = LVRef<ldomXRange>(new ldomXRange(start, end));
    }
    return res;
}

void LVDocView::GetCurrentPageLinks(ldomXRangeList& links_list)
{
	links_list.clear();
	LVRef<ldomXRange> page_range = GetPageDocRange();
	if (page_range.isNull()) {
		return;
	}
    class LinkKeeper: public ldomNodeCallback {
        ldomXRangeList& list_;
        void ProcessFinalNode(ldomNode* node) {
            int end_index = node->isText() ? node->getText().length() : node->getChildCount();
            ldomXPointerEx start = ldomXPointerEx(node, 0);
            ldomXPointerEx end = ldomXPointerEx(node, end_index);
            lvRect start_rect;
            lvRect end_rect;
            if (!start.getRect(start_rect) || !end.getRect(end_rect)) {
                CRLog::error("GetCurrentPageLinks getRect fail");
                return;
            }
            if (start_rect.top == end_rect.top && start_rect.bottom == end_rect.bottom) {
                // Singleline link
                list_.add(new ldomXRange(start, end));
                return;
            }
            lvRect curr_rect;
            for (int i = end_index - 1; i >= 0; i--) {
                ldomXPointerEx curr = ldomXPointerEx(node, i);
                if (!curr.getRect(curr_rect)) {
                    CRLog::error("GetCurrentPageLinks getRect fail");
                    return;
                }
                if (curr_rect.top == start_rect.top) {
                    if (curr_rect.bottom == start_rect.bottom) {
                        // Запрыгнули на верхнюю строчку
                        list_.add(new ldomXRange(start, curr));
                        ldomXPointerEx prev = ldomXPointerEx(node, i + 1);
                        list_.add(new ldomXRange(prev, end));
                        break;
                    } else {
                        CRLog::error("GetCurrentPageLinks tops equals, bottoms doesnt");
                    }
                } else if (curr_rect.top != end_rect.top) {
                    if (curr_rect.bottom != end_rect.bottom) {
                        ldomXPointerEx prev = ldomXPointerEx(node, i + 1);
                        list_.add(new ldomXRange(prev, end));
                        end = ldomXPointerEx(curr);
                        if (!end.getRect(end_rect)) {
                            CRLog::error("GetCurrentPageLinks getRect fail");
                            return;
                        }
                    } else {
                        CRLog::error("GetCurrentPageLinks tops not equals, bottoms does");
                    }
                }
            }
        }
        void ProcessLinkNode(ldomNode* node) {
            for (lUInt32 i = 0; i < node->getChildCount(); i++) {
                ldomNode* child = node->getChildNode(i);
                if (child->isText()) {
                    ProcessFinalNode(child);
                } else {
                    ProcessLinkNode(child);
                }
            }
        }
    public:
        bool text_is_first_ = true;
        LinkKeeper(ldomXRangeList& list) : list_(list) { }
        // Called for each text fragment in range
        virtual void onText(ldomXRange* node_range) {
            if (!text_is_first_) {
                return;
            }
            text_is_first_ = false;
            ldomNode* node = node_range->getStart().getNode();
            ldomNode* element_node = node->getParentNode();
            if (element_node->isNull() || element_node->getNodeId() != el_a) {
                return;
            }
            ProcessLinkNode(element_node);
#ifdef AXYDEBUG
            lString16 text = element_node->getText();
            int start = node_range->getStart().getOffset();
            int end = node_range->getEnd().getOffset();
            if (start < end) {
                text = text.substr(start, end - start);
            }
            ldomNode* end_node = node_range->getEnd().getNode();
            CRLog::debug("GetCurrentPageLinks first text on page: %d-%d %s",
                         node->getDataIndex(), end_node->getDataIndex(), LCSTR(text));
#endif
        }
        // Called for each node in range
        virtual bool onElement(ldomXPointerEx* ptr) {
            ldomNode* element_node = ptr->getNode();
            if (element_node->getNodeId() != el_a) {
                return true;
            }
            for (int i = 0; i < list_.length(); i++) {
                if (list_[i]->getStart().getNode() == element_node) {
                    // Don't add, duplicate found!
                    CRLog::error("GetCurrentPageLinks duplicate");
                    return true;
                }
            }
            ProcessLinkNode(element_node);
#ifdef AXYDEBUG
            if (element_node->getChildCount() == 0) {
                // Empty link in malformed doc, example: <a name="sync_on_demand"></a>
                CRLog::trace("GetCurrentPageLinks empty link in malformed doc");
            }
#endif
            return true;
        }
    };
    LinkKeeper callback(links_list);
    page_range->forEach(&callback);
    if (viewport_mode_ == MODE_PAGES && GetColumns() > 1) {
        // Process second page
        int page_index = GetCurrPage();
        page_range = GetPageDocRange(page_index + 1);
        if (!page_range.isNull()) {
            callback.text_is_first_ = true;
            page_range->forEach(&callback);
        }
    }
}

/// get page text, -1 for current page
lString16 LVDocView::GetPageText(int page_index)
{
    CHECK_RENDER("GetPageText()")
    if (page_index < 0 || page_index >= pages_list_.length()) {
        page_index = GetCurrPage();
    }
    lString16 text;
    LVRef<ldomXRange> range = GetPageDocRange(page_index);
    text = range->GetRangeText();
    if (viewport_mode_ == MODE_PAGES && GetColumns() > 1) {
        // Process second page
        range = GetPageDocRange(page_index + 1);
        text = text + range->GetRangeText();
    }
    return text;
}

/// sets new list of bookmarks, removes old values
void LVDocView::SetBookmarks(LVPtrVector<CRBookmark>& bookmarks)
{
	bookmarks_.clear();
    for (int i = 0; i < bookmarks.length(); i++) {
    	bookmarks_.add(new CRBookmark(*bookmarks[i]));
    }
    UpdateBookmarksRanges();
}

static void UpdateOutline(LVDocView* doc_view,
        LVPtrVector<LvTocItem, false>& list,
        LvTocItem* item)
{
	list.add(item);
	if (!item->getXPointer().isNull()) {
		int page = doc_view->GetPageForBookmark(item->getXPointer());
		if (page >= 0 && page < doc_view->GetPagesCount()) {
			item->setPage(page);
		} else {
			item->setPage(-1);
		}
	} else {
		item->setPage(-1);
	}
	for (int i = 0; i < item->getChildCount(); i++) {
		UpdateOutline(doc_view, list, item->getChild(i));
	}
}

void LVDocView::GetOutline(LVPtrVector<LvTocItem, false>& outline)
{
	outline.clear();
	if (cr_dom_) {
		LvTocItem* outline_root = cr_dom_->getToc();
		// First item its just dummy container, so we skip it
		for (int i=0; i < outline_root->getChildCount(); i++) {
			UpdateOutline(this, outline, outline_root->getChild(i));
		}
	}
}

static int CalcBookmarkMatch(lvPoint pt, lvRect& rc1, lvRect& rc2, int type)
{
    if (pt.y < rc1.top || pt.y >= rc2.bottom)
        return -1;
    if (type == bmkt_pos) {
        return abs(pt.x - 0);
    }
    if (rc1.top == rc2.top) {
        // single line
        if (pt.y >= rc1.top && pt.y < rc2.bottom && pt.x >= rc1.left && pt.x < rc2.right) {
            return abs(pt.x - (rc1.left + rc2.right) / 2);
        }
        return -1;
    } else {
        // first line
        if (pt.y >= rc1.top && pt.y < rc1.bottom && pt.x >= rc1.left) {
            return abs(pt.x - (rc1.left + rc1.right) / 2);
        }
        // last line
        if (pt.y >= rc2.top && pt.y < rc2.bottom && pt.x < rc2.right) {
            return abs(pt.x - (rc2.left + rc2.right) / 2);
        }
        // middle line
        return abs(pt.y - (rc1.top + rc2.bottom) / 2);
    }
}

/// Find bookmark by window point, return NULL if point doesn't belong to any bookmark
CRBookmark* LVDocView::FindBookmarkByPoint(lvPoint pt) {
    if (!WindowToDocPoint(pt))
        return NULL;
    CRBookmark * best = NULL;
    int bestMatch = -1;
    for (int i=0; i<bookmarks_.length(); i++) {
        CRBookmark * bmk = bookmarks_[i];
        int t = bmk->getType();
        if (t == bmkt_lastpos)
            continue;
        ldomXPointer p = cr_dom_->createXPointer(bmk->getStartPos());
        if (p.isNull())
            continue;
        lvRect rc;
        if (!p.getRect(rc))
            continue;
        ldomXPointer ep = (t == bmkt_pos) ? p : cr_dom_->createXPointer(bmk->getEndPos());
        if (ep.isNull())
            continue;
        lvRect erc;
        if (!ep.getRect(erc))
            continue;
        int match = CalcBookmarkMatch(pt, rc, erc, t);
        if (match < 0)
            continue;
        if (match < bestMatch || bestMatch == -1) {
            bestMatch = match;
            best = bmk;
        }
    }
    return best;
}

void LVPageWordSelector::UpdateSelection()
{
    LVArray<ldomWord> list;
    if (words_.getSelWord()) {
    	list.add(words_.getSelWord()->getWord());
    }
    if (list.length()) {
    	doc_view_->selectWords(list);
    } else {
    	doc_view_->ClearSelection();
    }
}

LVPageWordSelector::~LVPageWordSelector()
{
    doc_view_->ClearSelection();
}

LVPageWordSelector::LVPageWordSelector(LVDocView* docview) : doc_view_(docview)
{
    LVRef<ldomXRange> range = doc_view_->GetPageDocRange();
    if (!range.isNull()) {
		words_.addRangeWords(*range, true);
		if (doc_view_->GetColumns() > 1) { // _docview->isPageMode() &&
				// process second page
				int page_index = doc_view_->GetCurrPage();
				range = doc_view_->GetPageDocRange(page_index + 1);
				if (!range.isNull())
					words_.addRangeWords(*range, true);
		}
		words_.selectMiddleWord();
		UpdateSelection();
	}
}

void LVPageWordSelector::MoveBy(MoveDirection dir, int distance)
{
    words_.selectNextWord(dir, distance);
    UpdateSelection();
}

void LVPageWordSelector::SelectWord(int x, int y)
{
	ldomWordEx* word = words_.findNearestWord(x, y, DIR_ANY);
	words_.selectWord(word, DIR_ANY);
	UpdateSelection();
}

// append chars to search pattern
ldomWordEx* LVPageWordSelector::AppendPattern( lString16 chars )
{
    ldomWordEx * res = words_.appendPattern(chars);
    if (res) {
    	UpdateSelection();
    }
    return res;
}

// remove last item from pattern
ldomWordEx* LVPageWordSelector::ReducePattern()
{
    ldomWordEx* res = words_.reducePattern();
    if (res) {
    	UpdateSelection();
    }
    return res;
}

void CreBridge::processMetadata(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_CRE_METADATA;
    CmdDataIterator iter(request.first);
    uint32_t doc_format = 0;
    uint8_t* cre_uri_chars;
    iter.getInt(&doc_format).getByteArray(&cre_uri_chars);
    if (!iter.isValid()) {
        CRLog::error("processMetadata: iterator invalid data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    lString16 cre_uri(reinterpret_cast<const char*>(cre_uri_chars));
    lString16 to_archive_path;
    lString16 in_archive_path;
    bool in_zip = LVSplitArcName(cre_uri, to_archive_path, in_archive_path);
    LVStreamRef stream = LVOpenFileStream((in_zip ? to_archive_path : cre_uri).c_str(), LVOM_READ);
    if (!stream) {
        CRLog::error("processMetadata: cannot open file");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (in_zip) {
        LVContainerRef container = LVOpenArchieve(stream);
        if (container.isNull()) {
            CRLog::error("processMetadata: cannot open archive container");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        stream = container->OpenStream(in_archive_path.c_str(), LVOM_READ);
        if (stream.isNull()) {
            CRLog::error("processMetadata: cannot open file from archive container");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
    }

    LVStreamRef thumb_stream;
    lString16 title;
    lString16 authors;
    lString16 series;
    int series_number = 0;
    lString16 lang;

    if (doc_format == DOC_FORMAT_EPUB) {
        LVContainerRef container = LVOpenArchieve(stream);
        // Check is this a ZIP archive
        if (container.isNull()) {
            CRLog::error("processMetadata: EPUB container.isNull()");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        // Check root media type
        lString16 root_file_path = EpubGetRootFilePath(container);
        if (root_file_path.empty()) {
            CRLog::error("processMetadata: malformed EPUB");
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        EncryptedDataContainer* decryptor = new EncryptedDataContainer(container);
        if (decryptor->open()) {
            CRLog::debug("processMetadata: EPUB encrypted items detected");
        }
        container = LVContainerRef(decryptor);
        lString16 code_base = LVExtractPath(root_file_path, false);
        LVStreamRef content_stream = container->OpenStream(root_file_path.c_str(), LVOM_READ);
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
        for (int i = 1; i < 20; i++) {
            lString16 xpath = lString16("package/metadata/meta[") << fmt::decimal(i) << "]";
            ldomNode* item = dom->nodeFromXPath(xpath);
            if (!item) {
                break;
            }
            lString16 name = item->getAttributeValue("name");
            lString16 content = item->getAttributeValue("content");
            if (name == "cover") {
                thumb_id = content;
            }
        }
        for (int i = 1; i < 50000; i++) {
            lString16 xpath = lString16("package/manifest/item[") << fmt::decimal(i) << "]";
            ldomNode* item = dom->nodeFromXPath(xpath);
            if (!item) {
                break;
            }
            lString16 href = item->getAttributeValue("href");
            lString16 id = item->getAttributeValue("id");
            if (!href.empty() && !id.empty()) {
                if (id == thumb_id) {
                    lString16 thumbnail_file_name = code_base + href;
                    thumb_stream = container->OpenStream(thumbnail_file_name.c_str(), LVOM_READ);
                }
            }
        }
        authors = dom->textFromXPath(lString16("package/metadata/creator")).trim();
        title = dom->textFromXPath(lString16("package/metadata/title")).trim();
        lang = dom->textFromXPath(lString16("package/metadata/language")).trim();
        for (int i = 1; i < 20; i++) {
            lString16 xpath = lString16("package/metadata/meta[") << fmt::decimal(i) << "]";
            ldomNode* item = dom->nodeFromXPath(xpath);
            if (!item) {
                break;
            }
            lString16 name = item->getAttributeValue("name");
            lString16 content = item->getAttributeValue("content");
            if (name == "calibre:series") {
                series = content.trim();
            } else if (name == "calibre:series_index") {
                series_number = content.trim().atoi();
            }
        }
        delete dom;
    } else if (doc_format == DOC_FORMAT_FB2 || doc_format == DOC_FORMAT_MOBI) {
        if (doc_format == DOC_FORMAT_FB2) {
            thumb_stream = GetFB2Coverpage(stream);
        } else {
            thumb_stream = GetPDBCoverpage(stream);
        }
        CrDom dom;
        LvDomWriter writer(&dom, true);
        dom.setNodeTypes(fb2_elem_table);
        dom.setAttributeTypes(fb2_attr_table);
        dom.setNameSpaceTypes(fb2_ns_table);
        LvXmlParser parser(stream, &writer);
        if (parser.CheckFormat() && parser.Parse()) {
            authors = ExtractDocAuthors(&dom, lString16("|"));
            title = ExtractDocTitle(&dom);
            lang = ExtractDocLanguage(&dom);
            series = ExtractDocSeries(&dom, &series_number);
        }
    }

    CmdData* doc_thumb = new CmdData();
    int thumb_width = 0;
    int thumb_height = 0;
    doc_thumb->type = TYPE_ARRAY_POINTER;
    if (!thumb_stream.isNull()) {
        LVImageSourceRef thumb_image = LVCreateStreamCopyImageSource(thumb_stream);
        if (!thumb_image.isNull() && thumb_image->GetWidth() > 0 && thumb_image->GetHeight() > 0) {
            thumb_width = thumb_image->GetWidth();
            thumb_height = thumb_image->GetHeight();
            unsigned char* pixels = doc_thumb->newByteArray(thumb_width * thumb_height * 4);
            LVColorDrawBuf* buf = new LVColorDrawBuf(thumb_width, thumb_height, pixels, 32);
            buf->Draw(thumb_image, 0, 0, thumb_width, thumb_height, false);
            convertBitmap(buf);
            delete buf;
            thumb_image.Clear();
        }
    }
    response.addData(doc_thumb);
    response.addInt((uint32_t) thumb_width);
    response.addInt((uint32_t) thumb_height);
    responseAddString(response, title);
    responseAddString(response, authors);
    responseAddString(response, series);
    response.addInt((uint32_t) series_number);
    responseAddString(response, lang);
}