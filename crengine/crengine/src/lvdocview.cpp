/*******************************************************

 Crengine

 lvdocview.cpp:  XML DOM tree rendering tools

 (c) Vadim Lopatin, 2000-2009
 This source code is distributed under the terms of
 GNU General Public License
 See LICENSE file for details

 *******************************************************/

#include "../include/crsetup.h"
#include "../include/fb2def.h"
#include "../include/lvdocview.h"
#include "../include/rtfimp.h"

#include "../include/lvstyles.h"
#include "../include/lvrend.h"
#include "../include/lvstsheet.h"

#include "../include/wolutil.h"
#include "../include/crtxtenc.h"
#include "../include/crtrace.h"
#include "../include/epubfmt.h"
#include "../include/chmfmt.h"
#include "../include/wordfmt.h"
#include "../include/pdbfmt.h"
#if defined(__SYMBIAN32__)
#include <e32std.h>
#endif

#if 0 // Log render requests caller
#define REQUEST_RENDER(caller) { CRLog::trace("Request render from " caller); RequestRender(); }
#define CHECK_RENDER(caller) { CRLog::trace("Check render from " caller); CheckRender(); }
#else
#define REQUEST_RENDER(caller) RequestRender();
#define CHECK_RENDER(caller) CheckRender();
#endif

static const char* DEFAULT_FONT_NAME = "Arial, DejaVu Sans";
//    css_ff_serif,
//    css_ff_sans_serif,
//    css_ff_cursive,
//    css_ff_fantasy,
//    css_ff_monospace
static const css_font_family_t DEF_FONT_FAMILY = css_ff_sans_serif;
static const int def_font_sizes[] =
{
		12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
		31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 42, 44, 48, 52, 56, 60, 64, 68, 72
};
/// Minimum EM width of page (prevents show two pages for windows that not enougn wide)
#define MIN_EM_PER_PAGE 20
#define DEF_PAGE_MARGIN 12


static int interline_spaces[] =
{
		100, 70, 75, 80, 85, 90, 95, 100, 105, 110, 115, 120, 125, 130, 135, 140, 145, 150, 160, 180, 200
};

static const char* DEF_STYLESHEET =
		"image { text-align: center; text-indent: 0px } \n"
		"empty-line { height: 1em; } \n"
		"sub { vertical-align: sub; font-size: 70% }\n"
		"sup { vertical-align: super; font-size: 70% }\n"
		"body > image, section > image { text-align: center; margin-before: 1em; margin-after: 1em }\n"
		"p > image { display: inline }\n"
		"a { vertical-align: super; font-size: 80% }\n"
		"p { margin-top:0em; margin-bottom: 0em }\n"
		"text-author { font-weight: bold; font-style: italic; margin-left: 5%}\n"
		"empty-line { height: 1em }\n"
		"epigraph { margin-left: 30%; margin-right: 4%; text-align: left; text-indent: 1px; font-style: italic; margin-top: 15px; margin-bottom: 25px; font-family: Times New Roman, serif }\n"
		"strong, b { font-weight: bold }\n"
		"emphasis, i { font-style: italic }\n"
		"title { text-align: center; text-indent: 0px; font-size: 130%; font-weight: bold; margin-top: 10px; margin-bottom: 10px; font-family: Times New Roman, serif }\n"
		"subtitle { text-align: center; text-indent: 0px; font-size: 150%; margin-top: 10px; margin-bottom: 10px }\n"
		"title { page-break-before: always; page-break-inside: avoid; page-break-after: avoid; }\n"
		"body { text-align: justify; text-indent: 2em }\n"
		"cite { margin-left: 30%; margin-right: 4%; text-align: justyfy; text-indent: 0px;  margin-top: 20px; margin-bottom: 20px; font-family: Times New Roman, serif }\n"
		"td, th { text-indent: 0px; font-size: 80%; margin-left: 2px; margin-right: 2px; margin-top: 2px; margin-bottom: 2px; text-align: left; padding: 5px }\n"
		"th { font-weight: bold }\n"
		"table > caption { padding: 5px; text-indent: 0px; font-size: 80%; font-weight: bold; text-align: left; background-color: #AAAAAA }\n"
		"body[name=\"notes\"] { font-size: 70%; }\n"
		"body[name=\"notes\"]  section[id] { text-align: left; }\n"
		"body[name=\"notes\"]  section[id] title { display: block; text-align: left; font-size: 110%; font-weight: bold; page-break-before: auto; page-break-inside: auto; page-break-after: auto; }\n"
		"body[name=\"notes\"]  section[id] title p { text-align: left; display: inline }\n"
		"body[name=\"notes\"]  section[id] empty-line { display: inline }\n"
		"code, pre { display: block; white-space: pre; font-family: \"Courier New\", monospace }\n";

static const char* def_style_macros[] =
{
    "styles.def.align", "text-align: justify",
    "styles.def.text-indent", "text-indent: 1.2em",
    "styles.def.margin-top", "margin-top: 0em",
    "styles.def.margin-bottom", "margin-bottom: 0em",
    "styles.def.margin-left", "margin-left: 0em",
    "styles.def.margin-right", "margin-right: 0em",
    "styles.title.align", "text-align: center",
    "styles.title.text-indent", "text-indent: 0em",
    "styles.title.margin-top", "margin-top: 0.3em",
    "styles.title.margin-bottom", "margin-bottom: 0.3em",
    "styles.title.margin-left", "margin-left: 0em",
    "styles.title.margin-right", "margin-right: 0em",
    "styles.title.font-size", "font-size: 110%",
    "styles.title.font-weight", "font-weight: bolder",
    "styles.subtitle.align", "text-align: center",
    "styles.subtitle.text-indent", "text-indent: 0em",
    "styles.subtitle.margin-top", "margin-top: 0.2em",
    "styles.subtitle.margin-bottom", "margin-bottom: 0.2em",
    "styles.subtitle.font-style", "font-style: italic",
    "styles.cite.align", "text-align: justify",
    "styles.cite.text-indent", "text-indent: 1.2em",
    "styles.cite.margin-top", "margin-top: 0.3em",
    "styles.cite.margin-bottom", "margin-bottom: 0.3em",
    "styles.cite.margin-left", "margin-left: 1em",
    "styles.cite.margin-right", "margin-right: 1em",
    "styles.cite.font-style", "font-style: italic",
    "styles.epigraph.align", "text-align: left",
    "styles.epigraph.text-indent", "text-indent: 1.2em",
    "styles.epigraph.margin-top", "margin-top: 0.3em",
    "styles.epigraph.margin-bottom", "margin-bottom: 0.3em",
    "styles.epigraph.margin-left", "margin-left: 15%",
    "styles.epigraph.margin-right", "margin-right: 1em",
    "styles.epigraph.font-style", "font-style: italic",
    "styles.pre.align", "text-align: left",
    "styles.pre.text-indent", "text-indent: 0em",
    "styles.pre.margin-top", "margin-top: 0em",
    "styles.pre.margin-bottom", "margin-bottom: 0em",
    "styles.pre.margin-left", "margin-left: 0em",
    "styles.pre.margin-right", "margin-right: 0em",
    //TODO droidmono
    "styles.pre.font-face", "font-family: \"Courier New\", \"Courier\", monospace",
    "styles.poem.align", "text-align: left",
    "styles.poem.text-indent", "text-indent: 0em",
    "styles.poem.margin-top", "margin-top: 0.3em",
    "styles.poem.margin-bottom", "margin-bottom: 0.3em",
    "styles.poem.margin-left", "margin-left: 15%",
    "styles.poem.margin-right", "margin-right: 1em",
    "styles.poem.font-style", "font-style: italic",
    "styles.text-author.font-style", "font-style: italic",
    "styles.text-author.font-weight", "font-weight: bolder",
    "styles.text-author.margin-left", "margin-left: 1em",
    "styles.text-author.font-style", "font-style: italic",
    "styles.text-author.font-weight", "font-weight: bolder",
    "styles.text-author.margin-left", "margin-left: 1em",
    "styles.link.text-decoration", "text-decoration: underline",
    "styles.footnote-link.font-size", "font-size: 70%",
    "styles.footnote-link.vertical-align", "vertical-align: super",
    "styles.footnote", "font-size: 70%",
    "styles.footnote-title", "font-size: 110%",
    "styles.annotation.font-size", "font-size: 90%",
    "styles.annotation.margin-left", "margin-left: 1em",
    "styles.annotation.margin-right", "margin-right: 1em",
    "styles.annotation.font-style", "font-style: italic",
    "styles.annotation.align", "text-align: justify",
    "styles.annotation.text-indent", "text-indent: 1.2em",
    NULL,
    NULL,
};

LVDocView::LVDocView()
		: m_doc(NULL),
		  m_doc_format(doc_format_none),
		  m_view_mode(DVM_PAGES),
		  m_dx(400),
		  m_dy(200),
		  _pos(0),
		  _page(0),
		  _posIsSet(false),
		  font_sizes_(def_font_sizes, sizeof(def_font_sizes) / sizeof(int)),
		  font_size_(24),
		  def_interline_space_(100),
		  is_rendered_(false),
		  m_stylesheet(DEF_STYLESHEET),
          m_highlightBookmarks(1),
          m_pageMargins(DEF_PAGE_MARGIN, DEF_PAGE_MARGIN / 2, DEF_PAGE_MARGIN, DEF_PAGE_MARGIN / 2),
		  m_pagesVisible(2),
		  show_cover(true),
		  m_backgroundTiled(true),
		  m_backgroundColor(0xFFFFE0),
		  m_textColor(0x000060),
		  stream_(NULL)
{
	def_font_name_ = lString8(DEFAULT_FONT_NAME);
	m_font = fontMan->GetFont(font_size_, 400, false, DEF_FONT_FAMILY, def_font_name_);
	props_ = LVCreatePropsContainer();
	doc_props_ = LVCreatePropsContainer();

	static int def_aa_props[] = { 2, 1, 0 };
	props_->limitValueList(PROP_FONT_ANTIALIASING, def_aa_props, sizeof(def_aa_props) / sizeof(int));
	props_->setHexDef(PROP_FONT_COLOR, 0x000000);
	props_->setHexDef(PROP_BACKGROUND_COLOR, 0xFFFFFF);
	//props_->setIntDef(PROP_TXT_OPTION_PREFORMATTED, 0);

	//PROP_FONT_FACE
	lString16Collection list;
	fontMan->getFaceList(list);
	lString8 defFontFace;
	static const char * goodFonts[] = { "DejaVu Sans", "FreeSans", "Liberation Sans", "Arial", "Verdana", NULL };
	for (int i = 0; goodFonts[i]; i++) {
		if (list.contains(lString16(goodFonts[i]))) {
			defFontFace = lString8(goodFonts[i]);
			break;
		}
	}
	if (defFontFace.empty()) {
		defFontFace = UnicodeToUtf8(list[0]);
	}
	props_->setStringDef(PROP_FONT_FACE, defFontFace.c_str());
	if (list.length() > 0 && !list.contains(props_->getStringDef(PROP_FONT_FACE, defFontFace.c_str()))) {
		props_->setString(PROP_FONT_FACE, list[0]);
	}

	props_->setStringDef(PROP_FALLBACK_FONT_FACE, props_->getStringDef(PROP_FONT_FACE, defFontFace.c_str()));
	props_->setIntDef(PROP_FONT_SIZE, font_sizes_[font_sizes_.length() * 2 / 3]);
	props_->limitValueList(PROP_FONT_SIZE, font_sizes_.ptr(), font_sizes_.length());
	props_->limitValueList(PROP_INTERLINE_SPACE, interline_spaces, sizeof(interline_spaces) / sizeof(int));
	static int bool_options_def_true[] = { 1, 0 };
	static int bool_options_def_false[] = { 0, 1 };

	props_->limitValueList(PROP_FONT_WEIGHT_EMBOLDEN, bool_options_def_false, 2);
#ifndef ANDROID
	props_->limitValueList(PROP_EMBEDDED_STYLES, bool_options_def_true, 2);
	props_->limitValueList(PROP_EMBEDDED_FONTS, bool_options_def_true, 2);
#endif
	static int int_option_hinting[] = { 0, 1, 2 };
	props_->limitValueList(PROP_FONT_HINTING, int_option_hinting, 3);
    static int int_options_1_2[] = { 2, 1 };
	props_->limitValueList(PROP_LANDSCAPE_PAGES, int_options_1_2, 2);
	props_->limitValueList(PROP_PAGE_VIEW_MODE, bool_options_def_true, 2);
	props_->limitValueList(PROP_FOOTNOTES, bool_options_def_true, 2);
	props_->limitValueList(PROP_FONT_KERNING_ENABLED, bool_options_def_false, 2);
    //props_->limitValueList(PROP_FLOATING_PUNCTUATION, bool_options_def_true, 2);
    static int def_bookmark_highlight_modes[] = { 0, 1, 2 };
    props_->setIntDef(PROP_HIGHLIGHT_COMMENT_BOOKMARKS, highlight_mode_underline);
    props_->limitValueList(PROP_HIGHLIGHT_COMMENT_BOOKMARKS, def_bookmark_highlight_modes, sizeof(def_bookmark_highlight_modes)/sizeof(int));
    props_->setColorDef(PROP_HIGHLIGHT_SELECTION_COLOR, 0xC0C0C0); // silver
    props_->setColorDef(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xA08020); // yellow
    props_->setColorDef(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xA04040); // red

    static int def_margin[] = {8, 0, 1, 2, 3, 4, 5, 8, 10, 12, 14, 15, 16, 20, 25, 30, 40, 50, 60, 80, 100, 130, 150, 200, 300};
	props_->limitValueList(PROP_PAGE_MARGIN_TOP, def_margin, sizeof(def_margin)/sizeof(int));
	props_->limitValueList(PROP_PAGE_MARGIN_BOTTOM, def_margin, sizeof(def_margin)/sizeof(int));
	props_->limitValueList(PROP_PAGE_MARGIN_LEFT, def_margin, sizeof(def_margin)/sizeof(int));
	props_->limitValueList(PROP_PAGE_MARGIN_RIGHT, def_margin, sizeof(def_margin)/sizeof(int));

    props_->setIntDef(PROP_FLOATING_PUNCTUATION, 1);
#ifndef ANDROID
    props_->setIntDef(PROP_EMBEDDED_STYLES, 1);
    props_->setIntDef(PROP_EMBEDDED_FONTS, 1);
    props_->setIntDef(PROP_TXT_OPTION_PREFORMATTED, 0);
    props_->limitValueList(PROP_TXT_OPTION_PREFORMATTED, bool_options_def_false, 2);
#endif
    props_->setStringDef(PROP_FONT_GAMMA, "1.00");
    img_scaling_option_t defImgScaling;
    props_->setIntDef(PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE, defImgScaling.max_scale);
    props_->setIntDef(PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE, 0); //auto
    props_->setIntDef(PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE, defImgScaling.max_scale);
    props_->setIntDef(PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE, 0); // auto
    props_->setIntDef(PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE, defImgScaling.mode);
    props_->setIntDef(PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE, defImgScaling.mode);
    props_->setIntDef(PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE, defImgScaling.mode);
    props_->setIntDef(PROP_IMG_SCALING_ZOOMIN_INLINE_MODE, defImgScaling.mode);

    int p = props_->getIntDef(PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT, DEF_MIN_SPACE_CONDENSING_PERCENT);
    if (p<25)
        p = 25;
    if (p>100)
        p = 100;
    props_->setInt(PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT, p);

    for (int i=0; def_style_macros[i*2]; i++) {
        props_->setStringDef(def_style_macros[i * 2], def_style_macros[i * 2 + 1]);
    }

	CreateDefaultDoc(cs16("Dummy title"), lString16(L"Dummy text"));
}

LVDocView::~LVDocView()
{
	Clear();
}

/// Get text format options
txt_format_t LVDocView::getTextFormatOptions()
{
    return m_doc && m_doc->getDocFlag(DOC_FLAG_PREFORMATTED_TEXT) ? txt_format_pre : txt_format_auto;
}

/// Set text format options
void LVDocView::setTextFormatOptions(txt_format_t fmt)
{
	txt_format_t m_text_format = getTextFormatOptions();
	if (m_text_format == fmt)
		return; // no change
	props_->setBool(PROP_TXT_OPTION_PREFORMATTED, (fmt == txt_format_pre));
	m_doc->setDocFlag(DOC_FLAG_PREFORMATTED_TEXT, (fmt == txt_format_pre));
	if (getDocFormat() == doc_format_txt) {
		CRLog::trace("setTextFormatOptions(): Text format options changed, document reload required");
	}
}

/// Returns true if document is opened
bool LVDocView::IsDocOpened()
{
	return m_doc && m_doc->getRootNode() && !doc_props_->getStringDef(DOC_PROP_FILE_NAME, "").empty();
}

/// sets page margins
void LVDocView::setPageMargins(const lvRect & rc)
{
	if (m_pageMargins.left + m_pageMargins.right != rc.left + rc.right
            || m_pageMargins.top + m_pageMargins.bottom != rc.top + rc.bottom) {

        m_pageMargins = rc;
        updateLayout();
        REQUEST_RENDER("setPageMargins")
    } else {
        m_pageMargins = rc;
    }
}

lString16 mergeCssMacros(CRPropRef props) {
    lString8 res = lString8::empty_str;
    for (int i=0; i<props->getCount(); i++) {
    	lString8 n(props->getName(i));
    	if (n.endsWith(".day") || n.endsWith(".night"))
    		continue;
        lString16 v = props->getValue(i);
        if (!v.empty()) {
            if (v.lastChar() != ';')
                v.append(1, ';');
            if (v.lastChar() != ' ')
                v.append(1, ' ');
            res.append(UnicodeToUtf8(v));
        }
    }
    return Utf8ToUnicode(res);
}

/// set document stylesheet text
void LVDocView::setStyleSheet(lString8 css_text)
{
    REQUEST_RENDER("setStyleSheet")
    m_stylesheet = css_text;
}

void LVDocView::updateDocStyleSheet()
{
    CRPropRef sub_props = props_->getSubProps("styles.");

    //substituteCssMacros
    lString8 res = lString8(m_stylesheet.length());
	const char * s = m_stylesheet.c_str();
	for (; *s; s++) {
		if (*s == '$') {
			const char * s2 = s + 1;
			bool err = false;
			for (; *s2 && *s2 != ';' && *s2 != '}' &&  *s2 != ' ' &&  *s2 != '\r' &&  *s2 != '\n' &&  *s2 != '\t'; s2++) {
				char ch = *s2;
				if (ch != '.' && ch != '-' && (ch < 'a' || ch > 'z')) {
					err = true;
				}
			}
			if (!err) {
				// substitute variable
				lString8 prop(s + 1, s2 - s - 1);
				lString16 v;
				// $styles.stylename.all will merge all properties like styles.stylename.*
				if (prop.endsWith(".all")) {
					// merge whole branch
					v = mergeCssMacros(sub_props->getSubProps(prop.substr(0, prop.length() - 3).c_str()));
				} else {
					// single property
					sub_props->getString(prop.c_str(), v);
					if (!v.empty()) {
						if (v.lastChar() != ';')
							v.append(1, ';');
						if (v.lastChar() != ' ')
							v.append(1, ' ');
					}
				}
				if (!v.empty()) {
					res.append(UnicodeToUtf8(v));
				}
			}
			s = s2;
		} else {
			res.append(1, *s);
		}
	}
	m_doc->setStyleSheet(res.c_str(), true);
}

void LVDocView::Clear()
{
	if (m_doc) {
		delete m_doc;
	}
	m_doc = NULL;
	doc_props_->clear();
	if (!stream_.isNull()) {
		stream_.Clear();
	}
	if (!container_.isNull()) {
		container_.Clear();
	}
	if (!m_arc.isNull()) {
		m_arc.Clear();
	}
	_posBookmark = ldomXPointer();
	is_rendered_ = false;
	_pos = 0;
	_page = 0;
	_posIsSet = false;
	m_cursorPos.clear();
	crengine_uri_.clear();
}

void LVDocView::Close()
{
    CreateDefaultDoc(lString16::empty_str, lString16::empty_str);
}


/// Invalidate formatted data, request render
void LVDocView::RequestRender() {
	is_rendered_ = false;
	m_doc->clearRendBlockCache();
}

/// Render document, if not rendered
void LVDocView::CheckRender() {
	if (!is_rendered_) {
		Render();
		is_rendered_ = true;
		_posIsSet = false;
	}
}

/// Ensure current position is set to current bookmark value
void LVDocView::CheckPos() {
    CHECK_RENDER("CheckPos()");
	if (_posIsSet)
		return;
	_posIsSet = true;
	if (_posBookmark.isNull()) {
		if (isPageMode()) {
			goToPage(0);
		} else {
			SetPos(0, false);
		}
	} else {
		if (isPageMode()) {
			int p = getBookmarkPage(_posBookmark);
            goToPage(p, false);
		} else {
			lvPoint pt = _posBookmark.toPoint();
			SetPos(pt.y, false);
		}
	}
}

/// Draw current page to specified buffer
void LVDocView::Draw(LVDrawBuf& drawbuf) {
	int offset = -1;
	int p = -1;
	if (isPageMode()) {
		p = _page;
		if (p < 0 || p >= m_pages.length())
			return;
	} else {
		offset = _pos;
	}
	//CRLog::trace("Calling Draw(buf(%d x %d), %d, %d, false)", drawbuf.GetWidth(), drawbuf.GetHeight(), offset, p);
	Draw(drawbuf, offset, p);
}

LvTocItem* LVDocView::getToc() {
	if (!m_doc)
		return NULL;
	updatePageNumbers(m_doc->getToc());
	return m_doc->getToc();
}

static lString16 getSectionHeader(ldomNode* section) {
	lString16 header;
	if (!section || section->getChildCount() == 0)
		return header;
    ldomNode * child = section->getChildElementNode(0, L"title");
    if (!child)
		return header;
	header = child->getText(L' ', 1024);
	return header;
}

int getSectionPage(ldomNode* section, LVRendPageList& pages) {
	if (!section)
		return -1;
#if 1
	int y = ldomXPointer(section, 0).toPoint().y;
#else
	lvRect rc;
	section->getAbsRect(rc);
	int y = rc.top;
#endif
	int page = -1;
	if (y >= 0) {
		page = pages.FindNearestPage(y, -1);
	}
	return page;
}

/*
 static void addTocItems( ldomNode * basesection, LvTocItem * parent )
 {
 if ( !basesection || !parent )
 return;
 lString16 name = getSectionHeader( basesection );
 if ( name.empty() )
 return; // section without header
 ldomXPointer ptr( basesection, 0 );
 LvTocItem * item = parent->addChild( name, ptr );
 int cnt = basesection->getChildCount();
 for ( int i=0; i<cnt; i++ ) {
 ldomNode * section = basesection->getChildNode(i);
 if ( section->getNodeId() != el_section  )
 continue;
 addTocItems( section, item );
 }
 }

 void LVDocView::makeToc()
 {
 LvTocItem * toc = m_doc->getToc();
 if ( toc->getChildCount() ) {
 return;
 }
 CRLog::trace("LVDocView::makeToc()");
 toc->clear();
 ldomNode * body = m_doc->getRootNode();
 if ( !body )
 return;
 body = body->findChildElement( LXML_NS_ANY, el_FictionBook, -1 );
 if ( body )
 body = body->findChildElement( LXML_NS_ANY, el_body, 0 );
 if ( !body )
 return;
 int cnt = body->getChildCount();
 for ( int i=0; i<cnt; i++ ) {
 ldomNode * section = body->getChildNode(i);
 if ( section->getNodeId()!=el_section )
 continue;
 addTocItems( section, toc );
 }
 }
 */

/// update page numbers for items
void LVDocView::updatePageNumbers(LvTocItem * item) {
	if (!item->getXPointer().isNull()) {
		lvPoint p = item->getXPointer().toPoint();
		int h = GetFullHeight();
		int page = getBookmarkPage(item->_position);
		if (page >= 0 && page < getPagesCount()) {
			item->_page = page;
		} else {
			item->_page = -1;
		}
	} else {
		item->_page = -1;
	}
	for (int i = 0; i < item->getChildCount(); i++) {
		updatePageNumbers(item->getChild(i));
	}
}

/// returns cover page image source, if any
LVImageSourceRef LVDocView::getCoverPageImage()
{
	lUInt16 path[] = { el_FictionBook, el_description, el_title_info, el_coverpage, 0 };
	//lUInt16 path[] = { el_FictionBook, el_description, el_title_info, el_coverpage, el_image, 0 };
	ldomNode * cover_el = m_doc->getRootNode()->findChildElement(path);
	//ldomNode * cover_img_el = m_doc->getRootNode()->findChildElement( path );
	if (cover_el) {
		ldomNode * cover_img_el = cover_el->findChildElement(LXML_NS_ANY, el_image, 0);
		if (cover_img_el) {
			LVImageSourceRef imgsrc = cover_img_el->getObjectImageSource();
			return imgsrc;
		}
	}
	return LVImageSourceRef(); // not found: return NULL ref
}

/// Draws coverpage to image buffer
void LVDocView::drawCoverTo(LVDrawBuf* drawBuf, lvRect& rc)
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
        drawBuf->DrawRescaled(&buf2, imgrc.left + (imgrc.width() - dst_dx) / 2,
                imgrc.top + (imgrc.height() - dst_dy) / 2, dst_dx, dst_dy, 0);
	} else {
		imgrc.bottom = imgrc.top;
	}
	rc.top = imgrc.bottom;
}

int LVDocView::GetFullHeight()
{
    CHECK_RENDER("getFullHeight()");
	RenderRectAccessor rd(m_doc->getRootNode());
	return (rd.getHeight() + rd.getY());
}

int LVDocView::getPosPercent() {
	CheckPos();
	if (getViewMode() == DVM_SCROLL) {
		int fh = GetFullHeight();
		int p = GetPos();
		if (fh > 0)
			return (int) (((lInt64) p * 10000) / fh);
		else
			return 0;
	} else {
        int fh = m_pages.length();
        if ( (getVisiblePageCount()==2 && (fh&1)) )
            fh++;
        int p = getCurPage();// + 1;
//        if ( getVisiblePageCount()>1 )
//            p++;
		if (fh > 0)
			return (int) (((lInt64) p * 10000) / fh);
		else
			return 0;
	}
}

void LVDocView::getPageRectangle(int pageIndex, lvRect & pageRect) {
	if ((pageIndex & 1) == 0 || (getVisiblePageCount() < 2))
		pageRect = m_pageRects[0];
	else
		pageRect = m_pageRects[1];
}

void LVDocView::drawPageTo(LVDrawBuf* drawbuf, LVRendPageInfo& page, lvRect* pageRect, int pageCount, int basePage)
{
	int start = page.start;
	int height = page.height;
	lvRect fullRect(0, 0, drawbuf->GetWidth(), drawbuf->GetHeight());
	if (!pageRect) {
		pageRect = &fullRect;
	}
    drawbuf->setHidePartialGlyphs(getViewMode()==DVM_PAGES);
	//int offset = (pageRect->height() - m_pageMargins.top - m_pageMargins.bottom - height) / 3;
	//if (offset>16)
	//    offset = 16;
	//if (offset<0)
	//    offset = 0;
	int offset = 0;
	lvRect clip;
	clip.left = pageRect->left + m_pageMargins.left;
	clip.top = pageRect->top + m_pageMargins.top + offset;
	clip.bottom = pageRect->top + m_pageMargins.top + height + offset;
	clip.right = pageRect->left + pageRect->width() - m_pageMargins.right;
	if (page.type == PAGE_TYPE_COVER) {
		clip.top = pageRect->top + m_pageMargins.top;
	}
	drawbuf->SetClipRect(&clip);
	if (m_doc) {
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
			drawCoverTo(drawbuf, rc);
		} else {
			// draw main page text
            if (m_markRanges.length())
                CRLog::trace("Entering DrawDocument(): %d ranges", m_markRanges.length());
			if (page.height)
				DrawDocument(*drawbuf, m_doc->getRootNode(), pageRect->left
						+ m_pageMargins.left, clip.top, pageRect->width()
						- m_pageMargins.left - m_pageMargins.right, height, 0,
                                                -start + offset, m_dy, &m_markRanges, &m_bmkRanges);
			// draw footnotes
#define FOOTNOTE_MARGIN 8
			int fny = clip.top + (page.height ? page.height + FOOTNOTE_MARGIN : FOOTNOTE_MARGIN);
			int fy = fny;
			bool footnoteDrawed = false;
			for (int fn = 0; fn < page.footnotes.length(); fn++) {
				int fstart = page.footnotes[fn].start;
				int fheight = page.footnotes[fn].height;
				clip.top = fy + offset;
				clip.left = pageRect->left + m_pageMargins.left;
				clip.right = pageRect->right - m_pageMargins.right;
				clip.bottom = fy + offset + fheight;
				drawbuf->SetClipRect(&clip);
				DrawDocument(*drawbuf, m_doc->getRootNode(), pageRect->left
						+ m_pageMargins.left, fy + offset, pageRect->width()
						- m_pageMargins.left - m_pageMargins.right, fheight, 0,
						-fstart + offset, m_dy, &m_markRanges);
				footnoteDrawed = true;
				fy += fheight;
			}
			if (footnoteDrawed) { // && page.height
				fny -= FOOTNOTE_MARGIN / 2;
				drawbuf->SetClipRect(NULL);
                lUInt32 cl = drawbuf->GetTextColor();
                cl = (cl & 0xFFFFFF) | (0x55000000);
				drawbuf->FillRect(pageRect->left + m_pageMargins.left, fny,
						pageRect->right - m_pageMargins.right, fny + 1, cl);
			}
		}
	}
	drawbuf->SetClipRect(NULL);
}

/// returns page count
int LVDocView::getPagesCount() {
	return m_pages.length();
}

//============================================================================
// Navigation code
//============================================================================

/// get position of view inside document
void LVDocView::GetPos(lvRect & rc) {
    CheckPos();
	rc.left = 0;
	rc.right = GetWidth();
	if (isPageMode() && _page >= 0 && _page < m_pages.length()) {
		rc.top = m_pages[_page]->start;
		rc.bottom = rc.top + m_pages[_page]->height;
	} else {
		rc.top = _pos;
		rc.bottom = _pos + GetHeight();
	}
}

int LVDocView::getPageHeight(int pageIndex)
{
	if (isPageMode() && _page >= 0 && _page < m_pages.length()) 
		return m_pages[_page]->height;
	return 0;
}

/// get vertical position of view inside document
int LVDocView::GetPos() {
    CheckPos();
    if (isPageMode() && _page >= 0 && _page < m_pages.length())
		return m_pages[_page]->start;
	return _pos;
}

int LVDocView::SetPos(int pos, bool savePos) {
	_posIsSet = true;
    CHECK_RENDER("setPos()")
	//if ( m_posIsSet && m_pos==pos )
	//    return;
	if (isScrollMode()) {
		if (pos > GetFullHeight() - m_dy)
			pos = GetFullHeight() - m_dy;
		if (pos < 0)
			pos = 0;
		_pos = pos;
		int page = m_pages.FindNearestPage(pos, 0);
		if (page >= 0 && page < m_pages.length())
			_page = page;
		else
			_page = -1;
	} else {
		int pc = getVisiblePageCount();
		int page = m_pages.FindNearestPage(pos, 0);
		if (pc == 2)
			page &= ~1;
		if (page < m_pages.length()) {
			_pos = m_pages[page]->start;
			_page = page;
		} else {
			_pos = 0;
			_page = 0;
		}
	}
	if (savePos)
		_posBookmark = getBookmark();
	_posIsSet = true;
	updateScroll();
	return 1;
	//Draw();
}

int LVDocView::getCurPage()
{
	CheckPos();
	if (isPageMode() && _page >= 0) {
		return _page;
	}
	return m_pages.FindNearestPage(_pos, 0);
}

bool LVDocView::goToPage(int page, bool updatePosBookmark)
{
    CHECK_RENDER("goToPage()")
	if (!m_pages.length()) {
		return false;
	}

	bool res = true;
	if (isPageMode()) {
		int pc = getVisiblePageCount();
		if (page >= m_pages.length()) {
			page = m_pages.length() - 1;
			res = false;
		}
		if (page < 0) {
			page = 0;
			res = false;
		}
		if (pc == 2) {
			page &= ~1;
		}
		if (page >= 0 && page < m_pages.length()) {
			_pos = m_pages[page]->start;
			_page = page;
		} else {
			_pos = 0;
			_page = 0;
			res = false;
		}
	} else {
		if (page >= 0 && page < m_pages.length()) {
			_pos = m_pages[page]->start;
			_page = page;
		} else {
			res = false;
			_pos = 0;
			_page = 0;
		}
	}
    if (updatePosBookmark) {
        _posBookmark = getBookmark();
    }
	_posIsSet = true;
	updateScroll();
    if (res) {
        updateBookMarksRanges();
    }
	return res;
}

/// check whether resize or creation of buffer is necessary, ensure buffer is ok
static bool checkBufferSize( LVRef<LVColorDrawBuf> & buf, int dx, int dy ) {
    if ( buf.isNull() || buf->GetWidth()!=dx || buf->GetHeight()!=dy ) {
        buf.Clear();
        buf = LVRef<LVColorDrawBuf>( new LVColorDrawBuf(dx, dy, 16) );
        return false; // need redraw
    } else
        return true;
}

/// clears page background
void LVDocView::drawPageBackground(LVDrawBuf & drawbuf, int offsetX, int offsetY)
{
    drawbuf.SetBackgroundColor(m_backgroundColor);
    if (!m_backgroundImage.isNull()) {
        // texture
        int dx = drawbuf.GetWidth();
        int dy = drawbuf.GetHeight();
        if ( m_backgroundTiled ) {
            if (!checkBufferSize(m_backgroundImageScaled,
            		m_backgroundImage->GetWidth(), m_backgroundImage->GetHeight())) {
                // unpack
                m_backgroundImageScaled->Draw(m_backgroundImage, 0, 0,
                		m_backgroundImage->GetWidth(), m_backgroundImage->GetHeight(), false);
            }
            LVImageSourceRef src = LVCreateDrawBufImageSource(m_backgroundImageScaled.get(), false);
            LVImageSourceRef tile = LVCreateTileTransform( src, dx, dy, offsetX, offsetY );
            //CRLog::trace("created tile image, drawing");
            drawbuf.Draw(tile, 0, 0, dx, dy);
            //CRLog::trace("draw completed");
        } else {
            if ( getViewMode()==DVM_SCROLL ) {
                // scroll
                if ( !checkBufferSize( m_backgroundImageScaled, dx, m_backgroundImage->GetHeight() ) ) {
                    // unpack
                    LVImageSourceRef resized = LVCreateStretchFilledTransform(m_backgroundImage,
                    		dx, m_backgroundImage->GetHeight(),
                    		IMG_TRANSFORM_STRETCH, IMG_TRANSFORM_TILE, 0, 0);
                    m_backgroundImageScaled->Draw(resized, 0, 0, dx, m_backgroundImage->GetHeight(), false);
                }
                LVImageSourceRef src = LVCreateDrawBufImageSource(m_backgroundImageScaled.get(), false);
                LVImageSourceRef resized = LVCreateStretchFilledTransform(src, dx, dy,
                                                                          IMG_TRANSFORM_TILE,
                                                                          IMG_TRANSFORM_TILE,
                                                                          offsetX, offsetY);
                drawbuf.Draw(resized, 0, 0, dx, dy);
            } else if ( getVisiblePageCount() != 2 ) {
                // single page
                if ( !checkBufferSize( m_backgroundImageScaled, dx, dy ) ) {
                    // unpack
                    LVImageSourceRef resized = LVCreateStretchFilledTransform(m_backgroundImage, dx, dy,
                                                                              IMG_TRANSFORM_STRETCH,
                                                                              IMG_TRANSFORM_STRETCH,
                                                                              offsetX, offsetY);
                    m_backgroundImageScaled->Draw(resized, 0, 0, dx, dy, false);
                }
                LVImageSourceRef src = LVCreateDrawBufImageSource(m_backgroundImageScaled.get(), false);
                drawbuf.Draw(src, 0, 0, dx, dy);
            } else {
                // two pages
                int halfdx = (dx + 1) / 2;
                if ( !checkBufferSize( m_backgroundImageScaled, halfdx, dy ) ) {
                    // unpack
                    LVImageSourceRef resized = LVCreateStretchFilledTransform(m_backgroundImage, halfdx, dy,
                                                                              IMG_TRANSFORM_STRETCH,
                                                                              IMG_TRANSFORM_STRETCH,
                                                                              offsetX, offsetY);
                    m_backgroundImageScaled->Draw(resized, 0, 0, halfdx, dy, false);
                }
                LVImageSourceRef src = LVCreateDrawBufImageSource(m_backgroundImageScaled.get(), false);
                drawbuf.Draw(src, 0, 0, halfdx, dy);
                drawbuf.Draw(src, dx/2, 0, dx - halfdx, dy);
            }
        }
    } else {
        // solid color
        drawbuf.Clear(m_backgroundColor);
    }
    if (drawbuf.GetBitsPerPixel() == 32 && getVisiblePageCount() == 2) {
        int x = drawbuf.GetWidth() / 2;
        lUInt32 cl = m_backgroundColor;
        cl = ((cl & 0xFCFCFC) + 0x404040) >> 1;
        drawbuf.FillRect(x, 0, x + 1, drawbuf.GetHeight(), cl);
    }
}

/// draw to specified buffer
void LVDocView::Draw(LVDrawBuf & drawbuf, int position, int page) {
	//CRLog::trace("Draw() : calling CheckPos()");
	CheckPos();
	//CRLog::trace("Draw() : calling drawbuf.resize(%d, %d)", m_dx, m_dy);
	drawbuf.Resize(m_dx, m_dy);
	drawbuf.SetBackgroundColor(m_backgroundColor);
	drawbuf.SetTextColor(m_textColor);
	//CRLog::trace("Draw() : calling clear()", m_dx, m_dy);

	if (!is_rendered_)
		return;
	if (!m_doc)
		return;
	if (m_font.isNull())
		return;
	if (isScrollMode()) {
		drawbuf.SetClipRect(NULL);
        drawPageBackground(drawbuf, 0, position);
        int cover_height = 0;
		if (m_pages.length() > 0 && m_pages[0]->type == PAGE_TYPE_COVER)
			cover_height = m_pages[0]->height;
		if (position < cover_height) {
			lvRect rc;
			drawbuf.GetClipRect(&rc);
			rc.top -= position;
			rc.bottom -= position;
			rc.top += m_pageMargins.top;
			rc.bottom -= m_pageMargins.bottom;
			rc.left += m_pageMargins.left;
			rc.right -= m_pageMargins.right;
			drawCoverTo(&drawbuf, rc);
		}
		DrawDocument(drawbuf, m_doc->getRootNode(), m_pageMargins.left, 0, m_dx
				- m_pageMargins.left - m_pageMargins.right, m_dy, 0, -position,
                m_dy, &m_markRanges, &m_bmkRanges);
	} else {
		int pc = getVisiblePageCount();
		//CRLog::trace("searching for page with offset=%d", position);
		if (page == -1)
			page = m_pages.FindNearestPage(position, 0);
		//CRLog::trace("found page #%d", page);
        //drawPageBackground(drawbuf, (page * 1356) & 0xFFF, 0x1000 - (page * 1356) & 0xFFF);
        drawPageBackground(drawbuf, 0, 0);

        if (page >= 0 && page < m_pages.length())
			drawPageTo(&drawbuf, *m_pages[page], &m_pageRects[0], m_pages.length(), 1);
		if (pc == 2 && page >= 0 && page + 1 < m_pages.length())
			drawPageTo(&drawbuf, *m_pages[page + 1], &m_pageRects[1], m_pages.length(), 1);
	}
}

/// converts point from window to document coordinates, returns true if success
bool LVDocView::windowToDocPoint(lvPoint & pt) {
    CHECK_RENDER("windowToDocPoint()")
	if (getViewMode() == DVM_SCROLL) {
		// SCROLL mode
		pt.y += _pos;
		pt.x -= m_pageMargins.left;
		return true;
	} else {
		// PAGES mode
		int page = getCurPage();
		lvRect * rc = NULL;
		lvRect page1(m_pageRects[0]);
		page1.left += m_pageMargins.left;
		page1.top += m_pageMargins.top;
		page1.right -= m_pageMargins.right;
		page1.bottom -= m_pageMargins.bottom;
		lvRect page2;
		if (page1.isPointInside(pt)) {
			rc = &page1;
		} else if (getVisiblePageCount() == 2) {
			page2 = m_pageRects[1];
			page2.left += m_pageMargins.left;
			page2.top += m_pageMargins.top;
			page2.right -= m_pageMargins.right;
			page2.bottom -= m_pageMargins.bottom;
			if (page2.isPointInside(pt)) {
				rc = &page2;
				page++;
			}
		}
		if (rc && page >= 0 && page < m_pages.length()) {
			int page_y = m_pages[page]->start;
			pt.x -= rc->left;
			pt.y -= rc->top;
			if (pt.y < m_pages[page]->height) {
				//CRLog::debug(" point page offset( %d, %d )", pt.x, pt.y );
				pt.y += page_y;
				return true;
			}
		}
	}
	return false;
}

/// converts point from documsnt to window coordinates, returns true if success
bool LVDocView::docToWindowPoint(lvPoint & pt) {
    CHECK_RENDER("docToWindowPoint()")
	// TODO: implement coordinate conversion here
	if (getViewMode() == DVM_SCROLL) {
		// SCROLL mode
		pt.y -= _pos;
		pt.x += m_pageMargins.left;
		return true;
	} else {
            // PAGES mode
            int page = getCurPage();
            if (page >= 0 && page < m_pages.length() && pt.y >= m_pages[page]->start) {
                int index = -1;
                if (pt.y <= (m_pages[page]->start + m_pages[page]->height)) {
                    index = 0;
                } else if (getVisiblePageCount() == 2 && page + 1 < m_pages.length() &&
                    pt.y <= (m_pages[page + 1]->start + m_pages[page + 1]->height)) {
                    index = 1;
                }
                if (index >= 0) {
                    int x = pt.x + m_pageRects[index].left + m_pageMargins.left;
                    if (x < m_pageRects[index].right - m_pageMargins.right) {
                        pt.x = x;
                        pt.y = pt.y + m_pageMargins.top - m_pages[page + index]->start;
                        return true;
                    }
                }
            }
            return false;
	}
	return false;
}

/// returns xpointer for specified window point
ldomXPointer LVDocView::getNodeByPoint(lvPoint pt) {
    CHECK_RENDER("getNodeByPoint()")
	if (windowToDocPoint(pt) && m_doc) {
		ldomXPointer ptr = m_doc->createXPointer(pt);
		//CRLog::debug("  ptr (%d, %d) node=%08X offset=%d", pt.x, pt.y, (lUInt32)ptr.getNode(), ptr.getOffset() );
		return ptr;
	}
	return ldomXPointer();
}

/// returns image source for specified window point, if point is inside image
LVImageSourceRef LVDocView::getImageByPoint(lvPoint pt) {
    LVImageSourceRef res = LVImageSourceRef();
    ldomXPointer ptr = getNodeByPoint(pt);
    if (ptr.isNull())
        return res;
    //CRLog::debug("node: %s", LCSTR(ptr.toString()));
    res = ptr.getNode()->getObjectImageSource();
    if (!res.isNull())
        CRLog::debug("getImageByPoint(%d, %d) : found image %d x %d", pt.x, pt.y, res->GetWidth(), res->GetHeight());
    return res;
}

bool LVDocView::drawImage(LVDrawBuf * buf, LVImageSourceRef img, int x, int y, int dx, int dy)
{
    if (img.isNull() || !buf)
        return false;
    // clear background
    drawPageBackground(*buf, 0, 0);
    // draw image
    buf->Draw(img, x, y, dx, dy, true);
    return true;
}

void LVDocView::updateLayout() {
	lvRect rc(0, 0, m_dx, m_dy);
	m_pageRects[0] = rc;
	m_pageRects[1] = rc;
	if (getVisiblePageCount() == 2) {
		int middle = (rc.left + rc.right) >> 1;
		m_pageRects[0].right = middle - m_pageMargins.right / 2;
		m_pageRects[1].left = middle + m_pageMargins.left / 2;
	}
}

/// get page document range, -1 for current page
LVRef<ldomXRange> LVDocView::getPageDocumentRange(int pageIndex) {
    CHECK_RENDER("getPageDocRange()")
	LVRef < ldomXRange > res(NULL);
	if (isScrollMode()) {
		// SCROLL mode
		int starty = _pos;
		int endy = _pos + m_dy;
		int fh = GetFullHeight();
		if (endy >= fh)
			endy = fh - 1;
		ldomXPointer start = m_doc->createXPointer(lvPoint(0, starty));
		ldomXPointer end = m_doc->createXPointer(lvPoint(0, endy));
		if (start.isNull() || end.isNull())
			return res;
		res = LVRef<ldomXRange> (new ldomXRange(start, end));
	} else {
		// PAGES mode
		if (pageIndex < 0 || pageIndex >= m_pages.length())
			pageIndex = getCurPage();
		LVRendPageInfo * page = m_pages[pageIndex];
		if (page->type != PAGE_TYPE_NORMAL)
			return res;
		ldomXPointer start = m_doc->createXPointer(lvPoint(0, page->start));
		//ldomXPointer end = m_doc->createXPointer( lvPoint( m_dx+m_dy, page->start + page->height - 1 ) );
		ldomXPointer end = m_doc->createXPointer(lvPoint(0, page->start
                                + page->height), 1);
		if (start.isNull() || end.isNull())
			return res;
		res = LVRef<ldomXRange> (new ldomXRange(start, end));
	}
	return res;
}

/// returns number of non-space characters on current page
int LVDocView::getCurrentPageCharCount()
{
    lString16 text = getPageText(true);
    int count = 0;
    for (int i=0; i<text.length(); i++) {
        lChar16 ch = text[i];
        if (ch>='0')
            count++;
    }
    return count;
}

/// returns number of images on current page
int LVDocView::getCurrentPageImageCount()
{
    CHECK_RENDER("getCurPageImgCount()")
    LVRef<ldomXRange> range = getPageDocumentRange(-1);
    class ImageCounter : public ldomNodeCallback {
        int count;
    public:
        int get() { return count; }
        ImageCounter() : count(0) { }
        /// called for each found text fragment in range
        virtual void onText(ldomXRange *) { }
        /// called for each found node in range
        virtual bool onElement(ldomXPointerEx * ptr) {
            lString16 nodeName = ptr->getNode()->getNodeName();
            if (nodeName == "img" || nodeName == "image")
                count++;
			return true;
        }

    };
    ImageCounter cnt;
    range->forEach(&cnt);
    return cnt.get();
}

/// get page text, -1 for current page
lString16 LVDocView::getPageText(bool, int pageIndex)
{
    CHECK_RENDER("getPageText()")
	lString16 txt;
	LVRef < ldomXRange > range = getPageDocumentRange(pageIndex);
	txt = range->getRangeText();
	return txt;
}

/// Call setRenderProps(0, 0int dy) to allow apply styles and rend method while loading.
void LVDocView::setRenderProps(int dx, int dy)
{
	if (!m_doc || m_doc->getRootNode() == NULL)
		return;
	updateLayout();
	show_cover = !getCoverPageImage().isNull();

	if (dx == 0)
		dx = m_pageRects[0].width() - m_pageMargins.left - m_pageMargins.right;
	if (dy == 0)
		dy = m_pageRects[0].height() - m_pageMargins.top - m_pageMargins.bottom;

	lString8 fontName = lString8(DEFAULT_FONT_NAME);
	m_font = fontMan->GetFont(font_size_, 400 + LVRendGetFontEmbolden(), false, DEF_FONT_FAMILY, def_font_name_);
	if (!m_font) {
		return;
	}
    updateDocStyleSheet();

    m_doc->setRenderProps(dx, dy, show_cover,
    		show_cover ? dy + m_pageMargins.bottom * 4 : 0,
    		m_font, def_interline_space_, props_);
    text_highlight_options_t h;
    h.bookmarkHighlightMode = props_->getIntDef(PROP_HIGHLIGHT_COMMENT_BOOKMARKS, highlight_mode_underline);
    h.selectionColor = (props_->getColorDef(PROP_HIGHLIGHT_SELECTION_COLOR, 0xC0C0C0) & 0xFFFFFF);
    h.commentColor = (props_->getColorDef(PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT, 0xA08000) & 0xFFFFFF);
    h.correctionColor = (props_->getColorDef(PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION, 0xA00000) & 0xFFFFFF);
    m_doc->setHightlightOptions(h);
}

void LVDocView::Render(int dx, int dy, LVRendPageList * pages)
{
	if (!m_doc || m_doc->getRootNode() == NULL) {
		return;
	}
	if (dx == 0) {
		dx = m_pageRects[0].width() - m_pageMargins.left - m_pageMargins.right;
	}
	if (dy == 0) {
		dy = m_pageRects[0].height() - m_pageMargins.top - m_pageMargins.bottom;
	}
	setRenderProps(dx, dy);
	if (pages == NULL) {
		pages = &m_pages;
	}
	if (!m_font) {
		return;
	}
	CRLog::trace("Render(width: %d, height: %d)", dx, dy);
	m_doc->render(pages, dx, dy,
			show_cover, show_cover ? dy + m_pageMargins.bottom * 4 : 0,
			m_font, def_interline_space_, props_);
	fontMan->gc();
	is_rendered_ = true;
	//makeToc();
	updateSelections();
	CRLog::trace("Render() DONE");
	updateBookMarksRanges();
}

/// sets selection for whole element, clears previous selection
void LVDocView::selectElement(ldomNode * elem) {
	ldomXRangeList & sel = getDocument()->getSelections();
	sel.clear();
	sel.add(new ldomXRange(elem));
	updateSelections();
}

/// sets selection for list of words, clears previous selection
void LVDocView::selectWords(const LVArray<ldomWord> & words) {
	ldomXRangeList & sel = getDocument()->getSelections();
	sel.clear();
	sel.addWords(words);
	updateSelections();
}

/// sets selections for ranges, clears previous selections
void LVDocView::selectRanges(ldomXRangeList & ranges) {
    ldomXRangeList & sel = getDocument()->getSelections();
    if (sel.empty() && ranges.empty())
        return;
    sel.clear();
    for (int i=0; i<ranges.length(); i++) {
        ldomXRange * item = ranges[i];
        sel.add(new ldomXRange(*item));
    }
    updateSelections();
}

/// sets selection for range, clears previous selection
void LVDocView::selectRange(const ldomXRange & range) {
    // LVE:DEBUG
//    ldomXRange range2(range);
//    CRLog::trace("selectRange( %s, %s )", LCSTR(range2.getStart().toString()), LCSTR(range2.getEnd().toString()) );
	ldomXRangeList & sel = getDocument()->getSelections();
	if (sel.length() == 1) {
		if (range == *sel[0])
			return; // the same range is set
	}
	sel.clear();
	sel.add(new ldomXRange(range));
	updateSelections();
}

void LVDocView::ClearSelection() {
	ldomXRangeList& sel = getDocument()->getSelections();
	sel.clear();
	updateSelections();
}

/// selects first link on page, if any. returns selected link range, null if no links.
ldomXRange* LVDocView::selectFirstPageLink() {
	ldomXRangeList list;
	getCurrentPageLinks(list);
	if (!list.length())
		return NULL;
	//
	selectRange(*list[0]);
	//
	ldomXRangeList & sel = getDocument()->getSelections();
	updateSelections();
	return sel[0];
}

/// selects link on page, if any (delta==0 - current, 1-next, -1-previous). returns selected link range, null if no links.
ldomXRange * LVDocView::selectPageLink(int delta, bool wrapAround) {
	ldomXRangeList & sel = getDocument()->getSelections();
	ldomXRangeList list;
	getCurrentPageLinks(list);
	if (!list.length())
		return NULL;
	int currentLinkIndex = -1;
	if (sel.length() > 0) {
		ldomNode * currSel = sel[0]->getStart().getNode();
		for (int i = 0; i < list.length(); i++) {
			if (list[i]->getStart().getNode() == currSel) {
				currentLinkIndex = i;
				break;
			}
		}
	}
	bool error = false;
	if (delta == 1) {
		// next
		currentLinkIndex++;
		if (currentLinkIndex >= list.length()) {
			if (wrapAround)
				currentLinkIndex = 0;
			else
				error = true;
		}

	} else if (delta == -1) {
		// previous
		if (currentLinkIndex == -1)
			currentLinkIndex = list.length() - 1;
		else
			currentLinkIndex--;
		if (currentLinkIndex < 0) {
			if (wrapAround)
				currentLinkIndex = list.length() - 1;
			else
				error = true;
		}
	} else {
		// current
		if (currentLinkIndex < 0 || currentLinkIndex >= list.length())
			error = true;
	}
	if (error) {
		ClearSelection();
		return NULL;
	}
	//
	selectRange(*list[currentLinkIndex]);
	//
	updateSelections();
	return sel[0];
}

/// selects next link on page, if any. returns selected link range, null if no links.
ldomXRange * LVDocView::selectNextPageLink(bool wrapAround) {
	return selectPageLink(+1, wrapAround);
}

/// selects previous link on page, if any. returns selected link range, null if no links.
ldomXRange * LVDocView::selectPrevPageLink(bool wrapAround) {
	return selectPageLink(-1, wrapAround);
}

/// returns selected link on page, if any. null if no links.
ldomXRange * LVDocView::getCurrentPageSelectedLink() {
	return selectPageLink(0, false);
}

/// get document rectangle for specified cursor position, returns false if not visible
bool LVDocView::getCursorDocRect(ldomXPointer ptr, lvRect & rc) {
	rc.clear();
	if (ptr.isNull())
		return false;
	if (!ptr.getRect(rc)) {
		rc.clear();
		return false;
	}
	return true;
}

/// get screen rectangle for specified cursor position, returns false if not visible
bool LVDocView::getCursorRect(ldomXPointer ptr, lvRect & rc,
		bool scrollToCursor) {
	if (!getCursorDocRect(ptr, rc))
		return false;
	for (;;) {

		lvPoint topLeft = rc.topLeft();
		lvPoint bottomRight = rc.bottomRight();
		if (docToWindowPoint(topLeft) && docToWindowPoint(bottomRight)) {
			rc.setTopLeft(topLeft);
			rc.setBottomRight(bottomRight);
			return true;
		}
		// try to scroll and convert doc->window again
		if (!scrollToCursor)
			break;
		// scroll
		goToBookmark(ptr);
		scrollToCursor = false;
	};
	rc.clear();
	return false;
}

/// follow link, returns true if navigation was successful
bool LVDocView::goLink(lString16 link) {
	CRLog::debug("goLink(%s)", LCSTR(link));
	if (link.empty()) {
		ldomXRange* node = LVDocView::getCurrentPageSelectedLink();
		if (node) {
			link = node->getHRef();
			ldomNode * p = node->getStart().getNode();
			if (p->isText())
				p = p->getParentNode();
		}
		if (link.empty())
			return false;
	}
	if (link[0] != '#' || link.length() <= 1) {
		lString16 filename = link;
		lString16 id;
        int p = filename.pos("#");
		if (p >= 0) {
			// split filename and anchor
			// part1.html#chapter3 =>   part1.html & chapter3
			id = filename.substr(p + 1);
			filename = filename.substr(0, p);
		}
        if (filename.pos(":") >= 0) {
			// URL with protocol like http://
        	return true; //TODO return false?
		} else {
			// otherwise assume link to another file
			CRLog::debug("Link to another file: %s   anchor=%s", UnicodeToUtf8(
					filename).c_str(), UnicodeToUtf8(id).c_str());
			lString16 baseDir = doc_props_->getStringDef(DOC_PROP_FILE_PATH, ".");
			LVAppendPathDelimiter(baseDir);
			lString16 fn = doc_props_->getStringDef(DOC_PROP_FILE_NAME, "");
			CRLog::debug("Current path: %s   filename:%s", UnicodeToUtf8(
					baseDir).c_str(), UnicodeToUtf8(fn).c_str());
			baseDir = LVExtractPath(baseDir + fn);
			//lString16 newPathName = LVMakeRelativeFilename( baseDir, filename );
			lString16 newPathName = LVCombinePaths(baseDir, filename);
			lString16 dir = LVExtractPath(newPathName);
			lString16 filename = LVExtractFilename(newPathName);
			LVContainerRef container = container_;
			lString16 arcname =	doc_props_->getStringDef(DOC_PROP_ARC_NAME, "");
			if (arcname.empty()) {
				container = LVOpenDirectory(dir.c_str());
				if (container.isNull())
					return false;
			} else {
				filename = newPathName;
				dir.clear();
			}
			CRLog::trace("Base dir: %s newPathName=%s", LCSTR(baseDir), LCSTR(newPathName));
			LvStreamRef stream = container->OpenStream(filename.c_str(), LVOM_READ);
			if (stream.isNull()) {
				CRLog::error("Go to link: cannot find file %s", LCSTR(filename));
				return false;
			}
			CRLog::trace("Go to link: file %s is found", LCSTR(filename));
			// Close old document
			ClearSelection();
			_posBookmark = ldomXPointer();
			is_rendered_ = false;
			_pos = 0;
			_page = 0;
			doc_props_->setString(DOC_PROP_FILE_PATH, dir);
			doc_props_->setString(DOC_PROP_FILE_NAME, filename);
			doc_props_->setString(DOC_PROP_CODE_BASE, LVExtractPath(filename));
			doc_props_->setString(DOC_PROP_FILE_SIZE, lString16::itoa((int) stream->GetSize()));
			// TODO: load document from stream properly
			if (!LoadDoc(stream)) {
                CreateDefaultDoc(cs16("Load error"), lString16("Cannot open file ") + filename);
				return false;
			}
			//crengine_uri_ = newPathName;
			stream_ = stream;
			container_ = container;
			// TODO: setup properties
			// go to anchor
			if (!id.empty())
                goLink(cs16("#") + id);
			RequestRender();
			return true;
		}
		return false; // only internal links supported (started with #)
	}
	link = link.substr(1, link.length() - 1);
	lUInt16 id = m_doc->getAttrValueIndex(link.c_str());
	ldomNode * dest = m_doc->getNodeById(id);
	if (!dest)
		return false;
	ldomXPointer newPos(dest, 0);
	goToBookmark(newPos);
        updateBookMarksRanges();
	return true;
}

/// follow selected link, returns true if navigation was successful
bool LVDocView::goSelectedLink() {
	ldomXRange * link = getCurrentPageSelectedLink();
	if (!link)
		return false;
	lString16 href = link->getHRef();
	if (href.empty())
		return false;
	return goLink(href);
}

/// packs current file path and name
lString16 LVDocView::getNavigationPath() {
	lString16 fname = doc_props_->getStringDef(DOC_PROP_FILE_NAME, "");
	lString16 fpath = doc_props_->getStringDef(DOC_PROP_FILE_PATH, "");
	LVAppendPathDelimiter(fpath);
	lString16 s = fpath + fname;
	if (!m_arc.isNull())
        s = cs16("/") + s;
	return s;
}

/// update selection ranges
void LVDocView::updateSelections() {
    CHECK_RENDER("updateSelections()")
	ldomXRangeList ranges(m_doc->getSelections(), true);
    CRLog::trace("updateSelections() : selection count = %d", m_doc->getSelections().length());
	ranges.getRanges(m_markRanges);
	if (m_markRanges.length() > 0) {
//		crtrace trace;
//		trace << "LVDocView::updateSelections() - " << "selections: "
//				<< m_doc->getSelections().length() << ", ranges: "
//				<< ranges.length() << ", marked ranges: "
//				<< m_markRanges.length() << " ";
//		for (int i = 0; i < m_markRanges.length(); i++) {
//			ldomMarkedRange * item = m_markRanges[i];
//			trace << "(" << item->start.x << "," << item->start.y << "--"
//					<< item->end.x << "," << item->end.y << " #" << item->flags
//					<< ") ";
//		}
	}
}

void LVDocView::updateBookMarksRanges()
{
    CheckRender();
    ldomXRangeList ranges;
    //AXYTODO
    if (m_highlightBookmarks) {
		for (int i = 0; i < _bookmarks.length(); i++) {
			CRBookmark * bmk = _bookmarks[i];
			int t = bmk->getType();
			if (t != bmkt_lastpos) {
				ldomXPointer p = m_doc->createXPointer(bmk->getStartPos());
				if (p.isNull())
					continue;
				lvPoint pt = p.toPoint();
				if (pt.y < 0)
					continue;
				ldomXPointer ep = (t == bmkt_pos) ? p : m_doc->createXPointer(bmk->getEndPos());
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
    ranges.getRanges(m_bmkRanges);
#if 0
    m_bookmarksPercents.clear();
    if (m_highlightBookmarks) {
        CRFileHistRecord * rec = getCurrentFileHistRecord();
        if (rec) {
            LVPtrVector < CRBookmark > &bookmarks = rec->getBookmarks();

            m_bookmarksPercents.reserve(m_pages.length());
            for (int i = 0; i < bookmarks.length(); i++) {
                CRBookmark * bmk = bookmarks[i];
                if (bmk->getType() != bmkt_comment && bmk->getType() != bmkt_correction)
                    continue;
                lString16 pos = bmk->getStartPos();
                ldomXPointer p = m_doc->createXPointer(pos);
                if (p.isNull())
                    continue;
                lvPoint pt = p.toPoint();
                if (pt.y < 0)
                    continue;
                ldomXPointer ep = m_doc->createXPointer(bmk->getEndPos());
                if (ep.isNull())
                    continue;
                lvPoint ept = ep.toPoint();
                if (ept.y < 0)
                    continue;
                insertBookmarkPercentInfo(m_pages.FindNearestPage(pt.y, 0), ept.y, bmk->getPercent());
            }
        }
    }

    ldomXRangeList ranges;
    CRFileHistRecord * rec = m_bookmarksPercents.length() ? getCurrentFileHistRecord() : NULL;
    if (!rec) {
        m_bmkRanges.clear();
        return;
    }
    int page_index = getCurPage();
    if (page_index >= 0 && page_index < m_bookmarksPercents.length()) {
        LVPtrVector < CRBookmark > &bookmarks = rec->getBookmarks();
        LVRef < ldomXRange > page = getPageDocumentRange();
        LVBookMarkPercentInfo *bmi = m_bookmarksPercents[page_index];
        for (int i = 0; bmi != NULL && i < bmi->length(); i++) {
            for (int j = 0; j < bookmarks.length(); j++) {
                CRBookmark * bmk = bookmarks[j];
                if ((bmk->getType() != bmkt_comment && bmk->getType() != bmkt_correction) ||
                    bmk->getPercent() != bmi->get(i))
                    continue;
                lString16 epos = bmk->getEndPos();
                ldomXPointer ep = m_doc->createXPointer(epos);
                if (!ep.isNull()) {
                    lString16 spos = bmk->getStartPos();
                    ldomXPointer sp = m_doc->createXPointer(spos);
                    if (!sp.isNull()) {
                        ldomXRange bmk_range(sp, ep);

                        ldomXRange *n_range = new ldomXRange(*page, bmk_range);
                        if (!n_range->isNull())
                            ranges.add(n_range);
                        else
                            delete n_range;
                    }
                }
            }
        }
    }
    ranges.getRanges(m_bmkRanges);
#endif
}

/// set view mode (pages/scroll)
void LVDocView::setViewMode(LVDocViewMode view_mode, int visiblePageCount)
{
	if (m_view_mode == view_mode && (visiblePageCount == m_pagesVisible
			|| visiblePageCount < 1))
		return;
	m_view_mode = view_mode;
	props_->setInt(PROP_PAGE_VIEW_MODE, m_view_mode == DVM_PAGES ? 1 : 0);
    if (visiblePageCount == 1 || visiblePageCount == 2) {
		m_pagesVisible = visiblePageCount;
        props_->setInt(PROP_LANDSCAPE_PAGES, m_pagesVisible);
    }
    REQUEST_RENDER("setViewMode")
    _posIsSet = false;

//	goToBookmark( _posBookmark);
//        updateBookMarksRanges();
}

/// get view mode (pages/scroll)
LVDocViewMode LVDocView::getViewMode() {
	return m_view_mode;
}

/// toggle pages/scroll view mode
void LVDocView::toggleViewMode() {
	if (m_view_mode == DVM_SCROLL)
		setViewMode( DVM_PAGES);
	else
		setViewMode( DVM_SCROLL);

}

int LVDocView::getVisiblePageCount() {
	return (m_view_mode == DVM_SCROLL || m_dx < font_size_ * MIN_EM_PER_PAGE
			|| m_dx * 5 < m_dy * 6) ? 1 : m_pagesVisible;
}

/// set window visible page count (1 or 2)
void LVDocView::setVisiblePageCount(int n)
{
    int newCount = (n == 2) ? 2 : 1;
    if (m_pagesVisible == newCount)
        return;
    m_pagesVisible = newCount;
	updateLayout();
    REQUEST_RENDER("setVisiblePageCount")
    _posIsSet = false;
}

static int findBestFit(LVArray<int> & v, int n, bool rollCyclic = false) {
	int bestsz = -1;
	int bestfit = -1;
	if (rollCyclic) {
		if (n < v[0])
			return v[v.length() - 1];
		if (n > v[v.length() - 1])
			return v[0];
	}
	for (int i = 0; i < v.length(); i++) {
		int delta = v[i] - n;
		if (delta < 0)
			delta = -delta;
		if (bestfit == -1 || bestfit > delta) {
			bestfit = delta;
			bestsz = v[i];
		}
	}
	if (bestsz < 0)
		bestsz = n;
	return bestsz;
}

void LVDocView::setDefaultInterlineSpace(int percent)
{
    REQUEST_RENDER("setDefaultInterlineSpace")
	def_interline_space_ = percent;
    _posIsSet = false;
}

void LVDocView::setFontSize(int new_size)
{
	int old_size = font_size_;
	font_size_ = findBestFit(font_sizes_, new_size);
	if (old_size != new_size) {
		props_->setInt(PROP_FONT_SIZE, font_size_);
        REQUEST_RENDER("setFontSize")
	}
}

void LVDocView::setDefaultFontFace(const lString8 & newFace)
{
	def_font_name_ = newFace;
    REQUEST_RENDER("setDefaulFontFace")
}

/// sets posible base font sizes (for ZoomFont feature)
void LVDocView::setFontSizes(LVArray<int> & sizes)
{
	font_sizes_ = sizes;
}

void LVDocView::ZoomFont(int delta) {
	if (m_font.isNull())
		return;
#if 1
	int sz = font_size_;
	for (int i = 0; i < 15; i++) {
		sz += delta;
		int nsz = findBestFit(font_sizes_, sz, false);
		if (nsz != font_size_) {
			setFontSize(nsz);
			return;
		}
		if (sz < 12)
			break;
	}
#else
	LVFontRef nfnt;
	int sz = m_font->getHeight();
	for (int i=0; i<15; i++)
	{
		sz += delta;
		nfnt = fontMan->GetFont( sz, 400, false, DEFAULT_FONT_FAMILY, lString8(DEFAULT_FONT_NAME) );
		if ( !nfnt.isNull() && nfnt->getHeight() != m_font->getHeight() )
		{
			// found!
			//ldomXPointer bm = getBookmark();
			font_size_ = nfnt->getHeight();
			Render();
			goToBookmark(_posBookmark);
			return;
		}
		if (sz<12)
		break;
	}
#endif
}

/// sets current bookmark
void LVDocView::setBookmark(ldomXPointer bm) {
	_posBookmark = bm;
}

/// get view height
int LVDocView::GetHeight()
{
	return m_dy;
}

/// get view width
int LVDocView::GetWidth()
{
	return m_dx;
}

void LVDocView::Resize(int dx, int dy)
{
	if (dx == m_dx && dy == m_dy) {
		CRLog::trace("Size is not changed: %dx%d", dx, dy);
		return;
	}
	CRLog::trace("LVDocView:Resize(%dx%d)", dx, dy);
	if (dx < 80 || dx > 3000)
		dx = 80;
	if (dy < 80 || dy > 3000)
		dy = 80;
	//m_drawbuf.Resize(dx, dy);
	if (m_doc) {
		//ldomXPointer bm = getBookmark();
		if (dx != m_dx || dy != m_dy || m_view_mode != DVM_SCROLL || !is_rendered_) {
			m_dx = dx;
			m_dy = dy;
			updateLayout();
            REQUEST_RENDER("resize")
		}
        _posIsSet = false;
        //goToBookmark(_posBookmark);
        //updateBookMarksRanges();
	}
	m_dx = dx;
	m_dy = dy;
}

#define XS_IMPLEMENT_SCHEME 1
#include "../include/fb2def.h"

bool LVDocView::LoadDoc(const char* crengine_uri_chars)
{
	Clear();
	LvStreamRef stream;
	lString16 crengine_uri(crengine_uri_chars);
	lString16 to_archive_path;
	lString16 in_archive_path;
	if (LvSplitArcName(crengine_uri, to_archive_path, in_archive_path)) {
		// Doc is inside archive
		stream = LVOpenFileStream(to_archive_path.c_str(), LVOM_READ);
		if (stream.isNull()) {
			CRLog::error("Cannot open archive file %s", LCSTR(to_archive_path));
			return false;
		}
		int archive_size = (int) stream->GetSize();
		container_ = LVOpenArchieve(stream);
		if (container_.isNull()) {
			CRLog::error("Cannot read archive contents from %s", LCSTR(to_archive_path));
			return false;
		}
		stream = container_->OpenStream(in_archive_path.c_str(), LVOM_READ);
		if (stream.isNull()) {
			CRLog::error("Cannot open stream to file in archive %s", LCSTR(crengine_uri));
			return false;
		}
		doc_props_->setString(DOC_PROP_ARC_NAME, LVExtractFilename(to_archive_path));
		doc_props_->setString(DOC_PROP_ARC_PATH, LVExtractPath(to_archive_path));
		doc_props_->setString(DOC_PROP_ARC_SIZE, lString16::itoa(archive_size));
		doc_props_->setString(DOC_PROP_FILE_NAME, in_archive_path);
	} else {
		lString16 doc_file_name = LVExtractFilename(crengine_uri);
		lString16 path_to_doc = LVExtractPath(crengine_uri);
		container_ = LVOpenDirectory(path_to_doc.c_str());
		if (container_.isNull()) {
			return false;
		}
		stream = container_->OpenStream(doc_file_name.c_str(), LVOM_READ);
		if (!stream) {
			return false;
		}
		doc_props_->setString(DOC_PROP_FILE_PATH, path_to_doc);
		doc_props_->setString(DOC_PROP_FILE_NAME, doc_file_name);
	}
	doc_props_->setString(DOC_PROP_FILE_SIZE, lString16::itoa((int) stream->GetSize()));
	if (LoadDoc(stream)) {
		stream_.Clear();
		crengine_uri_ = lString16(crengine_uri_chars);
#if 0 // Debug XPointer navigation, dump opened doc sentences
        LvStreamRef out = LVOpenFileStream("/sdcard/crengine_dbg/sentences.txt", LVOM_WRITE);
        if (!out.isNull()) {
            CheckRender();
            {
                ldomXPointerEx ptr(m_doc->getRootNode(), m_doc->getRootNode()->getChildCount());
                *out << "FORWARD ORDER:\n\n";
                //ptr.nextVisibleText();
                ptr.prevVisibleWordEnd();
                if ( ptr.thisSentenceStart() ) {
                    while ( 1 ) {
                        ldomXPointerEx ptr2(ptr);
                        ptr2.thisSentenceEnd();
                        ldomXRange range(ptr, ptr2);
                        lString16 str = range.getRangeText();
                        *out << ">sentence: " << UnicodeToUtf8(str) << "\n";
                        if ( !ptr.nextSentenceStart() )
                            break;
                    }
                }
            }
            {
                ldomXPointerEx ptr(m_doc->getRootNode(), 1);
                *out << "\n\nBACKWARD ORDER:\n\n";
                while (ptr.lastChild())
                    ;// do nothing
                if (ptr.thisSentenceStart()) {
                    while (1) {
                        ldomXPointerEx ptr2(ptr);
                        ptr2.thisSentenceEnd();
                        ldomXRange range(ptr, ptr2);
                        lString16 str = range.getRangeText();
                        *out << "<sentence: " << UnicodeToUtf8(str) << "\n";
                        if (!ptr.prevSentenceStart())
                            break;
                    }
                }
            }
        } else {
        	CRLog::warn("UNABLE TO OPEN STREAM TO MAKE SENTENCES DUMP");
        }
#endif
        return true;
	}
    return false;
}

bool LVDocView::LoadDocSection(lString16Collection* meta_info, const lChar16* uri)
{
	Clear();
	lString16 fn = LVExtractFilename(uri);
	lString16 dir = LVExtractPath(uri);
	doc_props_->setString(DOC_PROP_FILE_PATH, dir);
	doc_props_->setString(DOC_PROP_FILE_NAME, fn);
	crengine_uri_ = lString16(uri);

	container_ = LVOpenDirectory(dir.c_str());
	if (container_.isNull()) return false;

	LvStreamRef stream = container_->OpenStream(fn.c_str(), LVOM_READ);
	if (!stream) {
		return false;
	}
	setRenderProps(0, 0);

	m_filesize = stream->GetSize();
	stream_ = stream;

	CreateEmptyDoc();

	//LvDomWriter writer(m_doc);
	LvDomAutocloseWriter writerFilter(m_doc, false, HTML_AUTOCLOSE_TABLE);
	SetDocFormat(doc_format_html);
	LVFileFormatParser* parser = new LvHtmlParser(stream_, &writerFilter);
	//SetDocFormat(doc_format_fb2);
    //LVFileFormatParser* parser = new LvXmlParser(stream_, &writer, false, true);
	if (!parser->CheckFormat()) {
		//delete parser;
		//return false;
	}
	updateDocStyleSheet();
	setRenderProps(0, 0);
	if (!parser->Parse()) {
		delete parser;
		return false;
	}
	delete parser;
	_pos = 0;
	_page = 0;

	if (doc_props_->getStringDef(DOC_PROP_TITLE, "").empty()) {
		doc_props_->setString(DOC_PROP_AUTHORS, extractDocAuthors(m_doc));
		doc_props_->setString(DOC_PROP_TITLE, extractDocTitle(m_doc));
	}
	//m_doc->persist();
	show_cover = !getCoverPageImage().isNull();
    REQUEST_RENDER("loadDocument")
    stream_.Clear();
#if 0
	{
		LvDomWriter LvscWriter(m_doc);
		_pos = 0;
		_page = 0;

		LvscWriter.OnTagOpen(NULL, L"?xml");
		LvscWriter.OnAttribute(NULL, L"version", L"1.0");
		LvscWriter.OnAttribute(NULL, L"encoding", L"utf-8");
		LvscWriter.OnEncoding(L"utf-8", NULL);
		LvscWriter.OnTagBody();
		LvscWriter.OnTagClose(NULL, L"?xml");
		LvscWriter.OnTagOpenNoAttr(NULL, L"FictionBook");
		// DESCRIPTION
		LvscWriter.OnTagOpenNoAttr(NULL, L"description");
			LvscWriter.OnTagOpenNoAttr(NULL, L"title-info");
				LvscWriter.OnTagOpenNoAttr(NULL, L"book-title");
					//LvscWriter.OnTagOpenNoAttr(NULL, L"lang");
					LvscWriter.OnText((*acho_meta_info_)[0].c_str(), (*acho_meta_info_)[0].length(), 0);
				LvscWriter.OnTagClose(NULL, L"book-title");
				LvscWriter.OnTagOpenNoAttr( NULL, L"author" );
					LvscWriter.OnTagOpenNoAttr( NULL, L"first-name" );
					LvscWriter.OnText((*acho_meta_info_)[1].c_str(), (*acho_meta_info_)[1].length(), 0);
					LvscWriter.OnTagClose(NULL, L"first-name");
					LvscWriter.OnTagOpenNoAttr(NULL, L"middle-name");
					//	LvscWriter.OnText( middleName.c_str(), middleName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
					LvscWriter.OnTagClose(NULL, L"middle-name");
					LvscWriter.OnTagOpenNoAttr(NULL, L"last-name");
					//	LvscWriter.OnText( lastName.c_str(), lastName.length(), TXTFLG_TRIM|TXTFLG_TRIM_REMOVE_EOL_HYPHENS );
					LvscWriter.OnTagClose(NULL, L"last-name");
				LvscWriter.OnTagClose(NULL, L"author");
				/*
				if ( !seriesName.empty() || !seriesNumber.empty() ) {
					callback->OnTagOpenNoAttr( NULL, L"sequence" );
					if ( !seriesName.empty() )
						callback->OnAttribute( NULL, L"name", seriesName.c_str() );
					if ( !seriesNumber.empty() )
						callback->OnAttribute( NULL, L"number", seriesNumber.c_str() );
					callback->OnTagClose( NULL, L"sequence" );
				}
				*/
			LvscWriter.OnTagOpenNoAttr(NULL, L"title-info");
		LvscWriter.OnTagClose(NULL, L"description");
		// BODY
		LvscWriter.OnTagOpenNoAttr(NULL, L"body");
			LvscWriter.OnTagOpenNoAttr(NULL, L"title");
				LvscWriter.OnTagOpenNoAttr(NULL, L"p");
					LvscWriter.OnText( (*acho_meta_info_)[0].c_str(), (*acho_meta_info_)[0].length(), 0 );
				LvscWriter.OnTagClose(NULL, L"p");
				LvscWriter.OnTagOpenAndClose( NULL, L"empty-line" );
				LvscWriter.OnTagOpenNoAttr(NULL, L"p");
					LvscWriter.OnText( (*acho_meta_info_)[1].c_str(), (*acho_meta_info_)[1].length(), 0 );
				LvscWriter.OnTagClose(NULL, L"p");
				LvscWriter.OnTagOpenAndClose( NULL, L"empty-line" );
				LvscWriter.OnTagOpenAndClose( NULL, L"empty-line" );
			LvscWriter.OnTagClose(NULL, L"title");
		LvscWriter.OnTagClose(NULL, L"body");
		LvscWriter.OnTagClose(NULL, L"FictionBook");

		updateDocStyleSheet();
		doc_props_->clear();
		m_doc->setProps(doc_props_);

		doc_props_->setString(DOC_PROP_FILE_NAME, "Virtual file name");
		doc_props_->setString(DOC_PROP_TITLE, (*meta_info)[0].c_str());
		doc_props_->setString(DOC_PROP_AUTHORS, (*meta_info)[1].c_str());
	}
#endif
	return true;
}

bool LVDocView::LoadDoc(LvStreamRef stream)
{
	stream_ = stream;
	// To allow apply styles and rend method while loading
	setRenderProps(0, 0);
	m_filesize = stream_->GetSize();
#if (USE_ZLIB==1)
	doc_format_t pdb_format = doc_format_none;
	if (DetectPDBFormat(stream_, pdb_format)) {
		CRLog::trace("PDB format detected");
		CreateEmptyDoc();
		m_doc->setProps(doc_props_);
		setRenderProps(0, 0);
		SetDocFormat(pdb_format);
		updateDocStyleSheet();
		doc_format_t contentFormat = doc_format_none;
		bool res = ImportPDBDocument(stream_, m_doc, contentFormat);
		if (!res) {
			SetDocFormat(doc_format_none);
			CreateDefaultDoc(cs16("Error reading PDB format"), cs16("Cannot open document"));
			return false;
		} else {
			setRenderProps(0, 0);
			REQUEST_RENDER("loadDocument")
			return true;
		}
	}
	if (DetectEpubFormat(stream_)) {
		CRLog::trace("EPUB format detected");
		CreateEmptyDoc();
		m_doc->setProps(doc_props_);
		setRenderProps(0, 0);
		SetDocFormat(doc_format_epub);
		updateDocStyleSheet();
		bool res = ImportEpubDocument(stream_, m_doc);
		if (!res) {
			SetDocFormat(doc_format_none);
			CreateDefaultDoc(cs16("Error reading EPUB format"), cs16("Cannot open document"));
			return false;
		} else {
			container_ = m_doc->getContainer();
			doc_props_ = m_doc->getProps();
			setRenderProps(0, 0);
			REQUEST_RENDER("loadDocument")
			m_arc = m_doc->getContainer();
			return true;
		}
	}
#if CHM_SUPPORT_ENABLED==1
	if (DetectCHMFormat(stream_)) {
		// CHM
		CRLog::trace("CHM format detected");
		CreateEmptyDoc();
		m_doc->setProps( doc_props_ );
		// to allow apply styles and rend method while loading
		setRenderProps(0, 0);
		SetDocFormat(doc_format_chm);
		updateDocStyleSheet();
		bool res = ImportCHMDocument(stream_, m_doc);
		if ( !res ) {
			SetDocFormat(doc_format_none);
			CreateDefaultDoc( cs16("ERROR: Error reading CHM format"), cs16("Cannot open document") );
			return false;
		} else {
			setRenderProps( 0, 0 );
			RequestRender();
			m_arc = m_doc->getContainer();
			return true;
		}
	}
#endif
#if ENABLE_ANTIWORD == 1
	if (DetectWordFormat(stream_)) {
		CRLog::trace("Word format detected");
		CreateEmptyDoc();
		m_doc->setProps(doc_props_);
		setRenderProps(0, 0);
		SetDocFormat(doc_format_doc);
		updateDocStyleSheet();
		bool res = ImportWordDocument(stream_, m_doc);
		if ( !res ) {
			SetDocFormat(doc_format_none);
			CreateDefaultDoc(cs16("Error reading DOC format"), cs16("Cannot open document"));
			return false;
		} else {
			setRenderProps( 0, 0 );
			REQUEST_RENDER("loadDocument")
			m_arc = m_doc->getContainer();
			return true;
		}
	}
#endif
#endif //USE_ZLIB
#if 0
	LvStreamRef stream = LVCreateBufferedStream( stream_, FILE_STREAM_BUFFER_SIZE );
	lvsize_t sz = stream->GetSize();
	const lvsize_t bufsz = 0x1000;
	lUInt8 buf[bufsz];
	lUInt8 * fullbuf = new lUInt8 [sz];
	stream->SetPos(0);
	stream->Read(fullbuf, sz, NULL);
	lvpos_t maxpos = sz - bufsz;
	for (int i=0; i<1000; i++)
	{
		lvpos_t pos = (lvpos_t)((((lUInt64)i) * 1873456178) % maxpos);
		stream->SetPos( pos );
		lvsize_t readsz = 0;
		stream->Read( buf, bufsz, &readsz );
		if (readsz != bufsz)
		{
			//
			fprintf(stderr, "data read error!\n");
		}
		for (int j=0; j<bufsz; j++)
		{
			if (fullbuf[pos+j] != buf[j])
			{
				fprintf(stderr, "invalid data!\n");
			}
		}
	}
	delete[] fullbuf;
#endif
	LvStreamRef tcr_decoder = LVCreateTCRDecoderStream(stream_);
	if (!tcr_decoder.isNull()) {
		stream_ = tcr_decoder;
	}
	return ParseDoc();
}

bool LVDocView::ParseDoc()
{
	CreateEmptyDoc();
	LvDomWriter writer(m_doc);
	if (stream_->GetSize() < 5) {
		CreateDefaultDoc(cs16("ERROR: Wrong document size"), cs16("Cannot open document"));
		return false;
	}
	// FB2 format
	SetDocFormat(doc_format_fb2);
	LVFileFormatParser* parser = new LvXmlParser(stream_, &writer, false, true);
	if (!parser->CheckFormat()) {
		delete parser;
		parser = NULL;
	}
	// RTF format
	if (parser == NULL) {
		SetDocFormat(doc_format_rtf);
		parser = new LVRtfParser(stream_, &writer);
		if (!parser->CheckFormat()) {
			delete parser;
			parser = NULL;
		}
	}
	// HTML format
	LvDomAutocloseWriter autoclose_writer(m_doc, false, HTML_AUTOCLOSE_TABLE);
	if (parser == NULL) {

		SetDocFormat(doc_format_html);
		parser = new LvHtmlParser(stream_, &autoclose_writer);
		if (!parser->CheckFormat()) {
			delete parser;
			parser = NULL;
		}
	}
	// Plain text format
	if (parser == NULL) {
		//m_text_format = txt_format_pre; // DEBUG!!!
		SetDocFormat(doc_format_txt);
		parser = new LVTextParser(stream_, &writer, getTextFormatOptions()
				== txt_format_pre);
		if (!parser->CheckFormat()) {
			delete parser;
			parser = NULL;
		}
	}
	// unknown format
	if (!parser) {
		SetDocFormat(doc_format_none);
		CreateDefaultDoc(cs16("Unknown document format"), cs16("Cannot open document"));
		return false;
	}
	updateDocStyleSheet();
	setRenderProps(0, 0);
	// set stylesheet
	//m_doc->getStyleSheet()->clear();
	//m_doc->getStyleSheet()->parse(m_stylesheet.c_str());
	//m_doc->setStyleSheet( m_stylesheet.c_str(), true );
	if (!parser->Parse()) {
		delete parser;
		CreateDefaultDoc(cs16("ERROR: Bad document format"), cs16("Cannot open document"));
		return false;
	}
	delete parser;
	_pos = 0;
	_page = 0;

	if (m_doc_format == doc_format_html) {
		static lUInt16 path[] = { el_html, el_head, el_title, 0 };
		ldomNode* el = m_doc->getRootNode()->findChildElement(path);
		if (el != NULL) {
			lString16 s = el->getText(L' ', 1024);
			if (!s.empty()) {
				doc_props_->setString(DOC_PROP_TITLE, s);
			}
		}
	}
	//lString16 docstyle = m_doc->createXPointer(L"/FictionBook/stylesheet").getText();
	//if (!docstyle.empty() && m_doc->getDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES)) {
	//      //m_doc->getStyleSheet()->parse(UnicodeToUtf8(docstyle).c_str());
	//      m_doc->setStyleSheet(UnicodeToUtf8(docstyle).c_str(), false);
	//}
	//m_doc->getProps()->clear();
	if (doc_props_->getStringDef(DOC_PROP_TITLE, "").empty()) {
		doc_props_->setString(DOC_PROP_AUTHORS, extractDocAuthors(m_doc));
		doc_props_->setString(DOC_PROP_TITLE, extractDocTitle(m_doc));
		doc_props_->setString(DOC_PROP_LANGUAGE, extractDocLanguage(m_doc));
		int seriesNumber = -1;
		lString16 seriesName = extractDocSeries(m_doc, &seriesNumber);
		doc_props_->setString(DOC_PROP_SERIES_NAME, seriesName);
		doc_props_->setString(DOC_PROP_SERIES_NUMBER,
				seriesNumber > 0 ? lString16::itoa(seriesNumber) : lString16::empty_str);
	}
	//m_doc->persist();
	show_cover = !getCoverPageImage().isNull();
    REQUEST_RENDER("loadDocument")
	return true;
}

/// create document and set flags
void LVDocView::CreateEmptyDoc()
{
	if (m_doc) {
		delete m_doc;
	}
	_posBookmark = ldomXPointer();
	is_rendered_ = false;
	m_doc = new CrDom();
	m_cursorPos.clear();
	m_markRanges.clear();
    m_bmkRanges.clear();
	_posBookmark.clear();
	_posIsSet = false;
	m_doc->setProps(doc_props_);
	m_doc->setDocFlags(0);
	m_doc->setDocFlag(DOC_FLAG_PREFORMATTED_TEXT, props_->getBoolDef(PROP_TXT_OPTION_PREFORMATTED, false));
	m_doc->setDocFlag(DOC_FLAG_ENABLE_FOOTNOTES, props_->getBoolDef(PROP_FOOTNOTES, true));
	m_doc->setDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES, props_->getBoolDef(PROP_EMBEDDED_STYLES, true));
    m_doc->setDocFlag(DOC_FLAG_ENABLE_DOC_FONTS, props_->getBoolDef(PROP_EMBEDDED_FONTS, true));
    m_doc->setMinSpaceCondensingPercent(props_->getIntDef(PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT, 50));
    m_doc->setContainer(container_);
	m_doc->setNodeTypes(fb2_elem_table);
	m_doc->setAttributeTypes(fb2_attr_table);
	m_doc->setNameSpaceTypes(fb2_ns_table);
}

void LVDocView::CreateDefaultDoc(lString16 title, lString16 message)
{
	Clear();
	show_cover = false;
	CreateEmptyDoc();
	LvDomWriter writer(m_doc);

	_pos = 0;
	_page = 0;

	// make fb2 document structure
	writer.OnTagOpen(NULL, L"?xml");
	writer.OnAttribute(NULL, L"version", L"1.0");
	writer.OnAttribute(NULL, L"encoding", L"utf-8");
	writer.OnEncoding(L"utf-8", NULL);
	writer.OnTagBody();
	writer.OnTagClose(NULL, L"?xml");
	writer.OnTagOpenNoAttr(NULL, L"FictionBook");
	// DESCRIPTION
	writer.OnTagOpenNoAttr(NULL, L"description");
	writer.OnTagOpenNoAttr(NULL, L"title-info");
	writer.OnTagOpenNoAttr(NULL, L"book-title");
	writer.OnTagOpenNoAttr(NULL, L"lang");
	writer.OnText(title.c_str(), title.length(), 0);
	writer.OnTagClose(NULL, L"book-title");
	writer.OnTagOpenNoAttr(NULL, L"title-info");
	writer.OnTagClose(NULL, L"description");
	// BODY
	writer.OnTagOpenNoAttr(NULL, L"body");
	//m_callback->OnTagOpen( NULL, L"section" );
	// process text
	if (title.length()) {
		writer.OnTagOpenNoAttr(NULL, L"title");
		writer.OnTagOpenNoAttr(NULL, L"p");
		writer.OnText(title.c_str(), title.length(), 0);
		writer.OnTagClose(NULL, L"p");
		writer.OnTagClose(NULL, L"title");
	}
	writer.OnTagOpenNoAttr(NULL, L"p");
	writer.OnText(message.c_str(), message.length(), 0);
	writer.OnTagClose(NULL, L"p");
	//m_callback->OnTagClose( NULL, L"section" );
	writer.OnTagClose(NULL, L"body");
	writer.OnTagClose(NULL, L"FictionBook");
	// set stylesheet
    updateDocStyleSheet();
	//m_doc->getStyleSheet()->clear();
	//m_doc->getStyleSheet()->parse(m_stylesheet.c_str());
	doc_props_->clear();
	m_doc->setProps(doc_props_);
	doc_props_->setString(DOC_PROP_FILE_NAME, "Untitled");
	doc_props_->setString(DOC_PROP_TITLE, title);
    REQUEST_RENDER("resize")
}

/// sets current document format
void LVDocView::SetDocFormat(doc_format_t fmt) {
	m_doc_format = fmt;
	doc_props_->setInt(DOC_PROP_FILE_FORMAT_ID, (int) fmt);
}

/// returns XPointer to middle paragraph of current page
ldomXPointer LVDocView::getCurrentPageMiddleParagraph() {
	CheckPos();
	ldomXPointer ptr;
	if (!m_doc)
		return ptr;

	if (getViewMode() == DVM_SCROLL) {
		// SCROLL mode
		int starty = _pos;
		int endy = _pos + m_dy;
		int fh = GetFullHeight();
		if (endy >= fh)
			endy = fh - 1;
		ptr = m_doc->createXPointer(lvPoint(0, (starty + endy) / 2));
	} else {
		// PAGES mode
		int pageIndex = getCurPage();
		if (pageIndex < 0 || pageIndex >= m_pages.length())
			pageIndex = getCurPage();
		LVRendPageInfo * page = m_pages[pageIndex];
		if (page->type == PAGE_TYPE_NORMAL)
			ptr = m_doc->createXPointer(lvPoint(0, page->start + page->height
					/ 2));
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
ldomXPointer LVDocView::getBookmark() {
	CheckPos();
	ldomXPointer ptr;
	if (m_doc) {
		if (isPageMode()) {
			if (_page >= 0 && _page < m_pages.length()) {
				ptr = m_doc->createXPointer(lvPoint(0, m_pages[_page]->start));
			}
		} else {
			ptr = m_doc->createXPointer(lvPoint(0, _pos));
		}
	}
	return ptr;
}

/// returns bookmark for specified page
ldomXPointer LVDocView::getPageBookmark(int page) {
    CHECK_RENDER("getPageBookmark()")
	if (page < 0 || page >= m_pages.length()) {
		return ldomXPointer();
	}
	ldomXPointer ptr = m_doc->createXPointer(lvPoint(0, m_pages[page]->start));
	return ptr;
}

/// get bookmark position text
bool LVDocView::getBookmarkPosText(ldomXPointer bm, lString16 & titleText, lString16 & posText)
{
	CheckRender();
    titleText = posText = lString16::empty_str;
	if (bm.isNull())
		return false;
	ldomNode * el = bm.getNode();
	CRLog::trace("getBookmarkPosText() : getting position text");
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
			lString16 txt = getSectionHeader(el);
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
void LVDocView::goToBookmark(ldomXPointer bm) {
    CHECK_RENDER("goToBookmark()")
	_posIsSet = false;
	_posBookmark = bm;
}

/// get page number by bookmark
int LVDocView::getBookmarkPage(ldomXPointer bm) {
    CHECK_RENDER("getBookmarkPage()")
	if (bm.isNull()) {
		return 0;
	} else {
		lvPoint pt = bm.toPoint();
		if (pt.y < 0)
			return 0;
		return m_pages.FindNearestPage(pt.y, 0);
	}
}

void LVDocView::updateScroll()
{
	CheckPos();
	if (m_view_mode == DVM_SCROLL) {
		int npos = _pos;
		int fh = GetFullHeight();
		int shift = 0;
		int npage = m_dy;
		while (fh > 16384) {
			fh >>= 1;
			npos >>= 1;
			npage >>= 1;
			shift++;
		}
		if (npage < 1)
			npage = 1;
		m_scrollinfo.pos = npos;
		m_scrollinfo.maxpos = fh - npage;
		m_scrollinfo.pagesize = npage;
		m_scrollinfo.scale = shift;
		char str[32];
		sprintf(str, "%d%%", (int) (fh > 0 ? (100 * npos / fh) : 0));
		m_scrollinfo.posText = lString16(str);
	} else {
		int page = getCurPage();
		int vpc = getVisiblePageCount();
		m_scrollinfo.pos = page / vpc;
		m_scrollinfo.maxpos = (m_pages.length() + vpc - 1) / vpc - 1;
		m_scrollinfo.pagesize = 1;
		m_scrollinfo.scale = 0;
		char str[32] = { 0 };
		if (m_pages.length() > 1) {
			if (page <= 0) {
				sprintf(str, "cover");
			} else
				sprintf(str, "%d / %d", page, m_pages.length() - 1);
		}
		m_scrollinfo.posText = lString16(str);
	}
}

/// move to position specified by scrollbar
bool LVDocView::goToScrollPos(int pos) {
	if (m_view_mode == DVM_SCROLL) {
		SetPos(scrollPosToDocPos(pos));
		return true;
	} else {
		int vpc = this->getVisiblePageCount();
		int curPage = getCurPage();
		pos = pos * vpc;
		if (pos >= getPagesCount())
			pos = getPagesCount() - 1;
		if (pos < 0)
			pos = 0;
		if (curPage == pos)
			return false;
		goToPage(pos);
		return true;
	}
}

/// converts scrollbar pos to doc pos
int LVDocView::scrollPosToDocPos(int scrollpos) {
	if (m_view_mode == DVM_SCROLL) {
		int n = scrollpos << m_scrollinfo.scale;
		if (n < 0)
			n = 0;
		int fh = GetFullHeight();
		if (n > fh)
			n = fh;
		return n;
	} else {
		int vpc = getVisiblePageCount();
		int n = scrollpos * vpc;
		if (!m_pages.length())
			return 0;
		if (n >= m_pages.length())
			n = m_pages.length() - 1;
		if (n < 0)
			n = 0;
		return m_pages[n]->start;
	}
}

/// get list of links
void LVDocView::getCurrentPageLinks(ldomXRangeList & list) {
	list.clear();
	/// get page document range, -1 for current page
	LVRef < ldomXRange > page = getPageDocumentRange();
	if (!page.isNull()) {
		// search for links
		class LinkKeeper: public ldomNodeCallback {
			ldomXRangeList &_list;
		public:
			LinkKeeper(ldomXRangeList & list) :
				_list(list) {
			}
			/// called for each found text fragment in range
			virtual void onText(ldomXRange *) {
			}
			/// called for each found node in range
			virtual bool onElement(ldomXPointerEx * ptr) {
				//
				ldomNode * elem = ptr->getNode();
				if (elem->getNodeId() == el_a) {
					for (int i = 0; i < _list.length(); i++) {
						if (_list[i]->getStart().getNode() == elem)
							return true; // don't add, duplicate found!
					}
                                        _list.add(new ldomXRange(elem->getChildNode(0)));
				}
				return true;
			}
		};
		LinkKeeper callback(list);
		page->forEach(&callback);
		if (m_view_mode == DVM_PAGES && getVisiblePageCount() > 1) {
			// process second page
			int pageNumber = getCurPage();
			page = getPageDocumentRange(pageNumber + 1);
			if (!page.isNull())
				page->forEach(&callback);
		}
	}
}

/// returns document offset for next page
int LVDocView::getNextPageOffset() {
	CheckPos();
	if (isScrollMode()) {
		return GetPos() + m_dy;
	} else {
		int p = getCurPage() + getVisiblePageCount();
		if (p < m_pages.length())
			return m_pages[p]->start;
		if (!p || m_pages.length() == 0)
			return 0;
		return m_pages[m_pages.length() - 1]->start;
	}
}

/// returns document offset for previous page
int LVDocView::getPrevPageOffset() {
	CheckPos();
	if (m_view_mode == DVM_SCROLL) {
		return GetPos() - m_dy;
	} else {
		int p = getCurPage();
		p -= getVisiblePageCount();
		if (p < 0)
			p = 0;
		if (p >= m_pages.length())
			return 0;
		return m_pages[p]->start;
	}
}

static void addItem(LVPtrVector<LvTocItem, false> & items, LvTocItem * item) {
	if (item->getLevel() > 0)
		items.add(item);
	for (int i = 0; i < item->getChildCount(); i++) {
		addItem(items, item->getChild(i));
	}
}

/// returns pointer to TOC root node
bool LVDocView::getFlatToc(LVPtrVector<LvTocItem, false> & items) {
	items.clear();
	addItem(items, getToc());
	return items.length() > 0;
}

/// -1 moveto previous page, 1 to next page
bool LVDocView::moveByPage(int delta) {
	if (m_view_mode == DVM_SCROLL) {
		int p = GetPos();
		SetPos(p + m_dy * delta);
		return GetPos() != p;
	} else {
		int cp = getCurPage();
		int p = cp + delta * getVisiblePageCount();
		goToPage(p);
		return getCurPage() != cp;
	}
}

/// sets new list of bookmarks, removes old values
void LVDocView::setBookmarkList(LVPtrVector<CRBookmark> & bookmarks)
{
	_bookmarks.clear();
    for (int i = 0; i < bookmarks.length(); i++) {
    	_bookmarks.add(new CRBookmark(*bookmarks[i]));
    }
    updateBookMarksRanges();
}

inline int myabs(int n) { return n < 0 ? -n : n; }

static int calcBookmarkMatch(lvPoint pt, lvRect & rc1, lvRect & rc2, int type) {
    if (pt.y < rc1.top || pt.y >= rc2.bottom)
        return -1;
    if (type == bmkt_pos) {
        return myabs(pt.x - 0);
    }
    if (rc1.top == rc2.top) {
        // single line
        if (pt.y >= rc1.top && pt.y < rc2.bottom && pt.x >= rc1.left && pt.x < rc2.right) {
            return myabs(pt.x - (rc1.left + rc2.right) / 2);
        }
        return -1;
    } else {
        // first line
        if (pt.y >= rc1.top && pt.y < rc1.bottom && pt.x >= rc1.left) {
            return myabs(pt.x - (rc1.left + rc1.right) / 2);
        }
        // last line
        if (pt.y >= rc2.top && pt.y < rc2.bottom && pt.x < rc2.right) {
            return myabs(pt.x - (rc2.left + rc2.right) / 2);
        }
        // middle line
        return myabs(pt.y - (rc1.top + rc2.bottom) / 2);
    }
}

/// find bookmark by window point, return NULL if point doesn't belong to any bookmark
CRBookmark * LVDocView::findBookmarkByPoint(lvPoint pt) {
    if (!windowToDocPoint(pt))
        return NULL;
    CRBookmark * best = NULL;
    int bestMatch = -1;
    for (int i=0; i<_bookmarks.length(); i++) {
        CRBookmark * bmk = _bookmarks[i];
        int t = bmk->getType();
        if (t == bmkt_lastpos)
            continue;
        ldomXPointer p = m_doc->createXPointer(bmk->getStartPos());
        if (p.isNull())
            continue;
        lvRect rc;
        if (!p.getRect(rc))
            continue;
        ldomXPointer ep = (t == bmkt_pos) ? p : m_doc->createXPointer(bmk->getEndPos());
        if (ep.isNull())
            continue;
        lvRect erc;
        if (!ep.getRect(erc))
            continue;
        int match = calcBookmarkMatch(pt, rc, erc, t);
        if (match < 0)
            continue;
        if (match < bestMatch || bestMatch == -1) {
            bestMatch = match;
            best = bmk;
        }
    }
    return best;
}

int LVDocView::doCommand(LVDocCmd cmd, int param) {
	CRLog::trace("doCommand(%d, %d)", (int) cmd, param);
	switch (cmd) {
    case DCMD_SET_DOC_FONTS:
        CRLog::trace("DCMD_SET_DOC_FONTS(%d)", param);
        props_->setBool(PROP_EMBEDDED_FONTS, (param&1)!=0);
        getDocument()->setDocFlag(DOC_FLAG_ENABLE_DOC_FONTS, param!=0);
        REQUEST_RENDER("doCommand-set embedded doc fonts")
        break;
    case DCMD_SET_INTERNAL_STYLES:
        CRLog::trace("DCMD_SET_INTERNAL_STYLES(%d)", param);
        props_->setBool(PROP_EMBEDDED_STYLES, (param&1)!=0);
        getDocument()->setDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES, param!=0);
        REQUEST_RENDER("doCommand-set internal styles")
        break;
    case DCMD_REQUEST_RENDER:
        REQUEST_RENDER("doCommand-request render")
		break;
	case DCMD_TOGGLE_BOLD: {
		int b = props_->getIntDef(PROP_FONT_WEIGHT_EMBOLDEN, 0) ? 0 : 1;
		props_->setInt(PROP_FONT_WEIGHT_EMBOLDEN, b);
		LVRendSetFontEmbolden(b ? STYLE_FONT_EMBOLD_MODE_EMBOLD
				: STYLE_FONT_EMBOLD_MODE_NORMAL);
        REQUEST_RENDER("doCommand-toggle bold")
	}
		break;
	case DCMD_TOGGLE_PAGE_SCROLL_VIEW: {
		toggleViewMode();
	}
		break;
	case DCMD_GO_SCROLL_POS: {
		return goToScrollPos(param);
	}
		break;
	case DCMD_GO_BEGIN: {
		if (getCurPage() > 0) {
			return SetPos(0);
		}
	}
		break;
	case DCMD_LINEUP: {
		if (m_view_mode == DVM_SCROLL) {
			return SetPos(GetPos() - param * (font_size_ * 3 / 2));
		} else {
			int p = getCurPage();
			return goToPage(p - getVisiblePageCount());
			//goToPage( m_pages.FindNearestPage(m_pos, -1));
		}
	}
		break;
	case DCMD_PAGEUP: {
		if (param < 1)
			param = 1;
		return moveByPage(-param);
	}
		break;
	case DCMD_PAGEDOWN: {
		if (param < 1)
			param = 1;
		return moveByPage(param);
	}
		break;
	case DCMD_LINK_NEXT: {
		selectNextPageLink(true);
	}
		break;
	case DCMD_LINK_PREV: {
		selectPrevPageLink(true);
	}
		break;
	case DCMD_LINK_FIRST: {
		selectFirstPageLink();
	}
		break;
	case DCMD_GO_LINK: {
		goSelectedLink();
	}
		break;
	case DCMD_LINEDOWN: {
		if (m_view_mode == DVM_SCROLL) {
			return SetPos(GetPos() + param * (font_size_ * 3 / 2));
		} else {
			int p = getCurPage();
			return goToPage(p + getVisiblePageCount());
			//goToPage( m_pages.FindNearestPage(m_pos, +1));
		}
	}
		break;
	case DCMD_GO_END: {
		if (getCurPage() < getPagesCount() - getVisiblePageCount()) {
			return SetPos(GetFullHeight());
		}
	}
		break;
	case DCMD_GO_POS: {
		if (m_view_mode == DVM_SCROLL) {
			return SetPos(param);
		} else {
			return goToPage(m_pages.FindNearestPage(param, 0));
		}
	}
		break;
	case DCMD_SCROLL_BY: {
		if (m_view_mode == DVM_SCROLL) {
			CRLog::trace("DCMD_SCROLL_BY %d", param);
			return SetPos(GetPos() + param);
		} else {
			CRLog::trace("DCMD_SCROLL_BY ignored: not in SCROLL mode");
		}
	}
		break;
	case DCMD_GO_PAGE: {
		if (getCurPage() != param) {
			return goToPage(param);
		}
	}
		break;
	case DCMD_ZOOM_IN: {
		ZoomFont(+1);
	}
		break;
	case DCMD_ZOOM_OUT: {
		ZoomFont(-1);
	}
		break;
	case DCMD_TOGGLE_TEXT_FORMAT: {
		if (getTextFormatOptions() == txt_format_auto)
			setTextFormatOptions(txt_format_pre);
		else
			setTextFormatOptions(txt_format_auto);
	}
		break;
    case DCMD_SET_TEXT_FORMAT: {
        CRLog::trace("DCMD_SET_TEXT_FORMAT(%d)", param);
        setTextFormatOptions(param ? txt_format_auto : txt_format_pre);
        REQUEST_RENDER("doCommand-set text format")
    }
        break;
    case DCMD_SELECT_FIRST_SENTENCE:
    case DCMD_SELECT_NEXT_SENTENCE:
    case DCMD_SELECT_PREV_SENTENCE:
    case DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS:
    case DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS:
        return onSelectionCommand(cmd, param);
	default:
		break;
	}
	return 1;
}

int LVDocView::onSelectionCommand(int cmd, int param)
{
    CHECK_RENDER("onSelectionCommand()")
    LVRef<ldomXRange> pageRange = getPageDocumentRange();
    ldomXPointerEx pos( getBookmark() );
    ldomXRangeList & sel = getDocument()->getSelections();
    ldomXRange currSel;
    if ( sel.length()>0 )
        currSel = *sel[0];
    bool moved = false;
    bool makeSelStartVisible = true; // true: start, false: end
    if ( !currSel.isNull() && !pageRange->isInside(currSel.getStart()) && !pageRange->isInside(currSel.getEnd()) )
        currSel.clear();
    if ( currSel.isNull() || currSel.getStart().isNull() ) {
        // select first sentence on page
        if ( pos.isNull() ) {
            ClearSelection();
            return 0;
        }
        if ( pos.thisSentenceStart() )
            currSel.setStart(pos);
        moved = true;
    }
    if ( currSel.getStart().isNull() ) {
        ClearSelection();
        return 0;
    }
    if (cmd==DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS || cmd==DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS) {
        if (cmd==DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS)
            makeSelStartVisible = false;
        int dir = param>0 ? 1 : -1;
        int distance = param>0 ? param : -param;
        bool res;
        if (cmd==DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS) {
            // DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS
            for (int i=0; i<distance; i++) {
                if (dir>0) {
                    res = currSel.getStart().nextVisibleWordStart();
                    CRLog::debug("nextVisibleWordStart returned %s", res?"true":"false");
                } else {
                    res = currSel.getStart().prevVisibleWordStart();
                    CRLog::debug("prevVisibleWordStart returned %s", res?"true":"false");
                }
            }
            if (currSel.isNull()) {
                currSel.setEnd(currSel.getStart());
                currSel.getEnd().nextVisibleWordEnd();
            }
        } else {
            // DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS
            for (int i=0; i<distance; i++) {
                if (dir>0) {
                    res = currSel.getEnd().nextVisibleWordEnd();
                    CRLog::debug("nextVisibleWordEnd returned %s", res?"true":"false");
                } else {
                    res = currSel.getEnd().prevVisibleWordEnd();
                    CRLog::debug("prevVisibleWordEnd returned %s", res?"true":"false");
                }
            }
            if (currSel.isNull()) {
                currSel.setStart(currSel.getEnd());
                currSel.getStart().prevVisibleWordStart();
            }
        }
        moved = true;
    } else {
        // selection start doesn't match sentence bounds
        if ( !currSel.getStart().isSentenceStart() ) {
            currSel.getStart().thisSentenceStart();
            moved = true;
        }
        // update sentence end
        if ( !moved )
            switch ( cmd ) {
            case DCMD_SELECT_NEXT_SENTENCE:
                if ( !currSel.getStart().nextSentenceStart() )
                    return 0;
                break;
            case DCMD_SELECT_PREV_SENTENCE:
                if ( !currSel.getStart().prevSentenceStart() )
                    return 0;
                break;
            case DCMD_SELECT_FIRST_SENTENCE:
            default: // unknown action
                break;
        }
        currSel.setEnd(currSel.getStart());
        currSel.getEnd().thisSentenceEnd();
    }
    currSel.setFlags(1);
    selectRange(currSel);
    lvPoint startPoint = currSel.getStart().toPoint();
    lvPoint endPoint = currSel.getEnd().toPoint();
    int y0 = GetPos();
    int h = m_pageRects[0].height() - m_pageMargins.top - m_pageMargins.bottom;
    //int y1 = y0 + h;
    if (makeSelStartVisible) {
        // make start of selection visible
        if (startPoint.y < y0 + font_size_ * 2 || startPoint.y > y0 + h * 3/4)
            SetPos(startPoint.y - font_size_ * 2);
        //goToBookmark(currSel.getStart());
    } else {
        // make end of selection visible
        if (endPoint.y > y0 + h * 3/4 - font_size_ * 2)
            SetPos(endPoint.y - h * 3/4 + font_size_ * 2, false);
    }
    CRLog::debug("Sel: %s", LCSTR(currSel.getRangeText()));
    return 1;
}

/// Applies properties, returns list of not recognized properties
void LVDocView::PropsApply(CRPropRef props)
{
	for (int i = 0; i < props->getCount(); i++) {
		lString8 name(props->getName(i));
		lString16 value = props->getValue(i);
		if (name == PROP_FONT_ANTIALIASING) {
			int antialiasingMode = props->getIntDef(PROP_FONT_ANTIALIASING, 2);
			fontMan->SetAntialiasMode(antialiasingMode);
            REQUEST_RENDER("propsApply - font antialiasing")
        } else if (name.startsWith(cs8("styles."))) {
            REQUEST_RENDER("propsApply - styles.*")
        } else if (name == PROP_FONT_GAMMA) {
            double gamma = 1.0;
            lString16 s = props->getStringDef(PROP_FONT_GAMMA, "1.0");
            lString8 s8 = UnicodeToUtf8(s);
            if (sscanf(s8.c_str(), "%lf", &gamma) == 1) {
                fontMan->SetGamma(gamma);
            }
        } else if (name == PROP_FONT_HINTING) {
            int mode = props->getIntDef(PROP_FONT_HINTING, (int) HINTING_MODE_AUTOHINT);
            if ((int) fontMan->GetHintingMode() != mode && mode >= 0 && mode <= 2) {
                fontMan->SetHintingMode((hinting_mode_t) mode);
                RequestRender();
            }
        } else if (name == PROP_HIGHLIGHT_SELECTION_COLOR
        		|| name == PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT
        		|| name == PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT) {
            REQUEST_RENDER("propsApply - highlight")
        } else if (name == PROP_LANDSCAPE_PAGES) {
            int pages = props->getIntDef(PROP_LANDSCAPE_PAGES, 2);
			setVisiblePageCount(pages);
		} else if (name == PROP_FONT_KERNING_ENABLED) {
			bool kerning = props->getBoolDef(PROP_FONT_KERNING_ENABLED, false);
			fontMan->setKerning(kerning);
            REQUEST_RENDER("propsApply - kerning")
		} else if (name == PROP_FONT_WEIGHT_EMBOLDEN) {
			bool embolden = props->getBoolDef(PROP_FONT_WEIGHT_EMBOLDEN, false);
			int v = embolden ? STYLE_FONT_EMBOLD_MODE_EMBOLD : STYLE_FONT_EMBOLD_MODE_NORMAL;
			if (v != LVRendGetFontEmbolden()) {
				LVRendSetFontEmbolden(v);
                REQUEST_RENDER("propsApply - embolden")
			}
		} else if (name == PROP_TXT_OPTION_PREFORMATTED) {
			bool preformatted = props->getBoolDef(PROP_TXT_OPTION_PREFORMATTED,	false);
			setTextFormatOptions(preformatted ? txt_format_pre : txt_format_auto);
        } else if (name == PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE || name == PROP_IMG_SCALING_ZOOMIN_INLINE_MODE
                   || name == PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE || name == PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE
                   || name == PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE || name == PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE
                   || name == PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE || name == PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE) {
            props_->setString(name.c_str(), value);
            REQUEST_RENDER("propsApply -img scale")
        } else if (name == PROP_FONT_COLOR || name == PROP_BACKGROUND_COLOR) {
			// update current value in properties
			props_->setString(name.c_str(), value);
			m_textColor = props_->getIntDef(PROP_FONT_COLOR, 0x000000);
			m_backgroundColor = props_->getIntDef(PROP_BACKGROUND_COLOR, 0xFFFFFF);
			REQUEST_RENDER("propsApply  color") // TODO: only colors to be changed
		} else if (name == PROP_PAGE_MARGIN_TOP || name	== PROP_PAGE_MARGIN_LEFT
				|| name == PROP_PAGE_MARGIN_RIGHT || name == PROP_PAGE_MARGIN_BOTTOM) {
            int margin = props->getIntDef(name.c_str(), 8);
            int maxmargin = (name == PROP_PAGE_MARGIN_LEFT || name == PROP_PAGE_MARGIN_RIGHT) ? m_dx / 3 : m_dy / 3;
            if (margin > maxmargin)
                margin = maxmargin;
			lvRect rc = getPageMargins();
			if (name == PROP_PAGE_MARGIN_TOP)
				rc.top = margin;
			else if (name == PROP_PAGE_MARGIN_BOTTOM)
				rc.bottom = margin;
			else if (name == PROP_PAGE_MARGIN_LEFT)
				rc.left = margin;
			else if (name == PROP_PAGE_MARGIN_RIGHT)
				rc.right = margin;
			setPageMargins(rc);
		} else if (name == PROP_FONT_FACE) {
			setDefaultFontFace(UnicodeToUtf8(value));
		} else if (name == PROP_FALLBACK_FONT_FACE) {
			lString8 oldFace = fontMan->GetFallbackFontFace();
			if ( UnicodeToUtf8(value)!=oldFace )
				fontMan->SetFallbackFontFace(UnicodeToUtf8(value));
			value = Utf8ToUnicode(fontMan->GetFallbackFontFace());
			if ( UnicodeToUtf8(value) != oldFace ) {
				REQUEST_RENDER("propsApply  fallback font face")
			}
		} else if (name == PROP_FONT_SIZE) {
			int fontSize = props->getIntDef(PROP_FONT_SIZE, font_sizes_[0]);
			setFontSize(fontSize);//cr_font_sizes
			value = lString16::itoa(font_size_);
		} else if (name == PROP_INTERLINE_SPACE) {
			int interlineSpace = props->getIntDef(PROP_INTERLINE_SPACE,	interline_spaces[0]);
			setDefaultInterlineSpace(interlineSpace);//cr_font_sizes
			value = lString16::itoa(def_interline_space_);
		} else if (name == PROP_EMBEDDED_STYLES) {
			bool value = props->getBoolDef(PROP_EMBEDDED_STYLES, true);
			getDocument()->setDocFlag(DOC_FLAG_ENABLE_INTERNAL_STYLES, value);
            REQUEST_RENDER("propsApply embedded styles")
        } else if (name == PROP_EMBEDDED_FONTS) {
            bool value = props->getBoolDef(PROP_EMBEDDED_FONTS, true);
            getDocument()->setDocFlag(DOC_FLAG_ENABLE_DOC_FONTS, value);
            REQUEST_RENDER("propsApply doc fonts")
        } else if (name == PROP_FOOTNOTES) {
			bool value = props->getBoolDef(PROP_FOOTNOTES, true);
			getDocument()->setDocFlag(DOC_FLAG_ENABLE_FOOTNOTES, value);
            REQUEST_RENDER("propsApply footnotes")
        } else if (name == PROP_FLOATING_PUNCTUATION) {
            bool value = props->getBoolDef(PROP_FLOATING_PUNCTUATION, true);
            if (gFlgFloatingPunctuationEnabled != value) {
                gFlgFloatingPunctuationEnabled = value;
                REQUEST_RENDER("propsApply floating punct")
            }
        } else if (name == PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT) {
            int value = props->getIntDef(PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT, DEF_MIN_SPACE_CONDENSING_PERCENT);
            if (getDocument()->setMinSpaceCondensingPercent(value))
                REQUEST_RENDER("propsApply condensing percent")
        } else if (name == PROP_HIGHLIGHT_COMMENT_BOOKMARKS) {
        	int value = props->getIntDef(PROP_HIGHLIGHT_COMMENT_BOOKMARKS, highlight_mode_underline);
            if (m_highlightBookmarks != value) {
                m_highlightBookmarks = value;
                updateBookMarksRanges();
            }
            REQUEST_RENDER("PropsApply PROP_HIGHLIGHT_COMMENT_BOOKMARKS")
        } else if (name == PROP_PAGE_VIEW_MODE) {
			LVDocViewMode m = props->getIntDef(PROP_PAGE_VIEW_MODE, 1) ? DVM_PAGES : DVM_SCROLL;
			setViewMode(m);
		} else {
			CRLog::warn("PropsApply unknown property: name(%s), value(%s)", name.c_str(), value.c_str());
		}
		// Update current value in properties
		props_->setString(name.c_str(), value);
	}
}

void LVPageWordSelector::updateSelection()
{
    LVArray<ldomWord> list;
    if (_words.getSelWord())
        list.add(_words.getSelWord()->getWord() );
    if ( list.length() )
        _docview->selectWords(list);
    else
        _docview->ClearSelection();
}

LVPageWordSelector::~LVPageWordSelector()
{
    _docview->ClearSelection();
}

LVPageWordSelector::LVPageWordSelector(LVDocView * docview) : _docview(docview)
{
    LVRef<ldomXRange> range = _docview->getPageDocumentRange();
    if (!range.isNull()) {
		_words.addRangeWords(*range, true);
		if (_docview->getVisiblePageCount() > 1) { // _docview->isPageMode() &&
				// process second page
				int pageNumber = _docview->getCurPage();
				range = _docview->getPageDocumentRange(pageNumber + 1);
				if (!range.isNull())
					_words.addRangeWords(*range, true);
		}
		_words.selectMiddleWord();
		updateSelection();
	}
}

void LVPageWordSelector::moveBy(MoveDirection dir, int distance)
{
    _words.selectNextWord(dir, distance);
    updateSelection();
}

void LVPageWordSelector::selectWord(int x, int y)
{
	ldomWordEx* word = _words.findNearestWord(x, y, DIR_ANY);
	_words.selectWord(word, DIR_ANY);
	updateSelection();
}

// append chars to search pattern
ldomWordEx* LVPageWordSelector::appendPattern( lString16 chars )
{
    ldomWordEx * res = _words.appendPattern(chars);
    if ( res )
        updateSelection();
    return res;
}

// remove last item from pattern
ldomWordEx * LVPageWordSelector::reducePattern()
{
    ldomWordEx * res = _words.reducePattern();
    if ( res )
        updateSelection();
    return res;
}

void LVDocView::insertBookmarkPercentInfo(int start_page, int end_y, int percent)
{
    for (int j = start_page; j < m_pages.length(); j++) {
        if (m_pages[j]->start > end_y)
            break;
        LVBookMarkPercentInfo *bmi = m_bookmarksPercents[j];
        if (bmi == NULL) {
            bmi = new LVBookMarkPercentInfo(1, percent);
            m_bookmarksPercents.set(j, bmi);
        } else
            bmi->add(percent);
    }
}
