/** \file lvdocview.h
    \brief XML/CSS document view

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2009

    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.
*/
#ifndef __LV_TEXT_VIEW_H_INCLUDED__
#define __LV_TEXT_VIEW_H_INCLUDED__

#include "crsetup.h"
//#include "crskin.h"
#include "lvtinydom.h"
#include "lvpagesplitter.h"
#include "lvdrawbuf.h"
#include "bookmark.h"

// Supported: 0.65 to 1.35, see gammatbl.h
#define PROP_FONT_GAMMA              			"font.gamma"
#define PROP_FONT_ANTIALIASING       			"font.antialiasing.mode"
#define PROP_FONT_HINTING            			"font.hinting.mode"
#define PROP_FONT_COLOR             			"font.color.default"
#define PROP_FONT_FACE               			"font.face.default"
#define PROP_FONT_KERNING_ENABLED    			"font.kerning.enabled"
#define PROP_BACKGROUND_COLOR        			"background.color.default"
#define PROP_TXT_OPTION_PREFORMATTED 			"crengine.file.txt.preformatted"
#define PROP_FONT_SIZE               			"crengine.font.size"
#define PROP_FALLBACK_FONT_FACE      			"crengine.font.fallback.face"
#define PROP_PAGE_MARGIN_TOP         			"crengine.page.margin.top"
#define PROP_PAGE_MARGIN_BOTTOM      			"crengine.page.margin.bottom"
#define PROP_PAGE_MARGIN_LEFT        			"crengine.page.margin.left"
#define PROP_PAGE_MARGIN_RIGHT       			"crengine.page.margin.right"
/// Pages or scroll
#define PROP_PAGE_VIEW_MODE          			"crengine.page.view.mode"
#define PROP_INTERLINE_SPACE         			"crengine.interline.space"
#define PROP_EMBEDDED_STYLES         			"crengine.doc.embedded.styles.enabled"
#define PROP_EMBEDDED_FONTS          			"crengine.doc.embedded.fonts.enabled"
#define PROP_FOOTNOTES               			"crengine.footnotes"
/// Single column or two column mode
#define PROP_LANDSCAPE_PAGES         			"window.landscape.pages"
#define PROP_FONT_WEIGHT_EMBOLDEN    			"font.face.weight.embolden"

#define PROP_FLOATING_PUNCTUATION    			"crengine.style.floating.punctuation.enabled"
#define PROP_FORMAT_MIN_SPACE_CONDENSING_PERCENT "crengine.style.space.condensing.percent"

#define PROP_HIGHLIGHT_COMMENT_BOOKMARKS 		"crengine.highlight.bookmarks"
#define PROP_HIGHLIGHT_SELECTION_COLOR 			"crengine.highlight.selection.color"
#define PROP_HIGHLIGHT_BOOKMARK_COLOR_COMMENT 	"crengine.highlight.bookmarks.color.comment"
#define PROP_HIGHLIGHT_BOOKMARK_COLOR_CORRECTION "crengine.highlight.bookmarks.color.correction"
// image scaling settings
// mode: 0=disabled, 1=integer scaling factors, 2=free scaling
// scale: 0=auto based on font size, 1=no zoom, 2=scale up to *2, 3=scale up to *3
#define PROP_IMG_SCALING_ZOOMIN_INLINE_MODE  	"crengine.image.scaling.zoomin.inline.mode"
#define PROP_IMG_SCALING_ZOOMIN_INLINE_SCALE  	"crengine.image.scaling.zoomin.inline.scale"
#define PROP_IMG_SCALING_ZOOMOUT_INLINE_MODE 	"crengine.image.scaling.zoomout.inline.mode"
#define PROP_IMG_SCALING_ZOOMOUT_INLINE_SCALE 	"crengine.image.scaling.zoomout.inline.scale"
#define PROP_IMG_SCALING_ZOOMIN_BLOCK_MODE  	"crengine.image.scaling.zoomin.block.mode"
#define PROP_IMG_SCALING_ZOOMIN_BLOCK_SCALE  	"crengine.image.scaling.zoomin.block.scale"
#define PROP_IMG_SCALING_ZOOMOUT_BLOCK_MODE 	"crengine.image.scaling.zoomout.block.mode"
#define PROP_IMG_SCALING_ZOOMOUT_BLOCK_SCALE 	"crengine.image.scaling.zoomout.block.scale"

/// text format import options
typedef enum {
    txt_format_pre,  // no formatting, leave lines as is
    txt_format_auto  // autodetect format
} txt_format_t;

class LVPageWordSelector {
    LVDocView * _docview;
    ldomWordExList _words;
    void updateSelection();
public:
    // selects middle word of current page
    LVPageWordSelector( LVDocView * docview);
    // clears selection
    ~LVPageWordSelector();
    // move current word selection in specified direction, (distance) times
    void moveBy( MoveDirection dir, int distance = 1);
    // returns currently selected word
    ldomWordEx * getSelectedWord() { return _words.getSelWord(); }
    // access to words
    ldomWordExList & getWords() { return _words; }
    // append chars to search pattern
    ldomWordEx * appendPattern( lString16 chars);
    // remove last item from pattern
    ldomWordEx * reducePattern();
    // selects word of current page with specified coords;
    void selectWord(int x, int y);
};

enum LVDocCmd
{
    DCMD_GO_BEGIN 					= 100,
    DCMD_GO_END 					= 110,
    DCMD_LINEUP 					= 101,
    DCMD_LINEDOWN 					= 104,
    DCMD_PAGEUP 					= 102,
    DCMD_PAGEDOWN 					= 103,
    DCMD_LINK_NEXT 					= 107,
    DCMD_LINK_PREV 					= 108,
    DCMD_GO_LINK 					= 109,
    DCMD_GO_POS 					= 111,
    DCMD_GO_PAGE 					= 112,
    DCMD_ZOOM_IN 					= 113,
    DCMD_ZOOM_OUT 					= 114,
    DCMD_TOGGLE_TEXT_FORMAT 		= 115,
    // Param: position of scroll bar slider
    DCMD_GO_SCROLL_POS 				= 117,
    // Toggle paged/scroll view mode
    DCMD_TOGGLE_PAGE_SCROLL_VIEW 	= 118,
    // Select first link on page
    DCMD_LINK_FIRST 				= 119,
    // Save document to cache for fast opening
    DCMD_SAVE_TO_CACHE 				= 122,
    // Togle font bolder attribute
    DCMD_TOGGLE_BOLD 				= 123,
    // Scroll by N pixels, for Scroll view mode only
    DCMD_SCROLL_BY					= 124,
    // Invalidate rendered data
    DCMD_REQUEST_RENDER				= 125,
    // Set internal styles option
    DCMD_SET_INTERNAL_STYLES 		= 127,
    // Set text format, param=1 to autoformat, 0 for preformatted
	DCMD_SET_TEXT_FORMAT 			= 128,
	// Set embedded fonts option (1=enabled, 0=disabled)
	DCMD_SET_DOC_FONTS 				= 129,
    // Select first sentence on page
    DCMD_SELECT_FIRST_SENTENCE 		= 130,
    // Move selection to next sentence
    DCMD_SELECT_NEXT_SENTENCE 		= 131,
    // Move selection to prev sentence
    DCMD_SELECT_PREV_SENTENCE 		= 132,
    // Move selection start by words
    DCMD_SELECT_MOVE_LEFT_BOUND_BY_WORDS = 133,
    // Move selection end by words
    DCMD_SELECT_MOVE_RIGHT_BOUND_BY_WORDS = 134,
};

/// document view mode: pages/scroll
enum LVDocViewMode
{
    DVM_SCROLL,
    DVM_PAGES
};

/// document scroll position info
class LVScrollInfo
{
public:
    int pos;
    int maxpos;
    int pagesize;
    int scale;
    lString16 posText;
    LVScrollInfo() : pos(0), maxpos(0), pagesize(0), scale(1) { }
};

typedef LVArray<int> LVBookMarkPercentInfo;

#define DEF_COLOR_BUFFER_BPP 32

class LVDocView
{
private:
    CrDom* m_doc;
    doc_format_t m_doc_format;
	LVDocViewMode m_view_mode;
	int m_dx;
    int m_dy;
    // current position
    int _pos;  // >=0 is correct vertical offset inside document, <0 - get based on m_page
    int _page; // >=0 is correct page number, <0 - get based on _pos
    bool _posIsSet;
    LVArray<int> font_sizes_;
	int font_size_;
    font_ref_t m_font;
    int def_interline_space_;
    bool is_rendered_;
    lString8 m_stylesheet;
    int m_highlightBookmarks;
    lvRect m_pageMargins;
    int m_pagesVisible;
    bool show_cover;
    bool m_backgroundTiled;
    lUInt32 m_backgroundColor;
    lUInt32 m_textColor;
    LvStreamRef stream_;
    LVContainerRef container_;
	lString16 crengine_uri_;
	lvsize_t  m_filesize;
	LVPtrVector<CRBookmark> _bookmarks;
    ldomXPointer _posBookmark; // bookmark for current position
    LVContainerRef m_arc;
    LVRendPageList m_pages;
    LVScrollInfo m_scrollinfo;
    LVImageSourceRef m_backgroundImage;
    LVRef<LVColorDrawBuf> m_backgroundImageScaled;
    LVPtrVector<LVBookMarkPercentInfo> m_bookmarksPercents;
    lvRect m_pageRects[2];
    lString8 def_font_name_;
    // options
    CRPropRef props_;
    // document properties
    CRPropRef doc_props_;
    /// edit cursor position
    ldomXPointer m_cursorPos;

	inline bool isPageMode() { return m_view_mode==DVM_PAGES; }
	inline bool isScrollMode() { return m_view_mode==DVM_SCROLL; }
    /// sets current document format
    void SetDocFormat(doc_format_t fmt);
    void updateScroll();
    /// makes table of contents for current document
    void makeToc();
    /// updates page layout
    void updateLayout();
    /// load document from stream
    bool LoadDoc(LvStreamRef stream);
    /// parse document from stream_
    bool ParseDoc();
    void insertBookmarkPercentInfo(int start_page, int end_y, int percent);
    void updateDocStyleSheet();
    /// create empty document with specified message (to show errors)
    virtual void CreateDefaultDoc(lString16 title, lString16 message);

protected:
    ldomMarkedRangeList m_markRanges;
    ldomMarkedRangeList m_bmkRanges;
    /// draw to specified buffer by either Y pos or page number (unused param should be -1)
    void Draw(LVDrawBuf & drawbuf, int pageTopPosition, int pageNumber);

    virtual void getPageRectangle( int pageIndex, lvRect & pageRect);
    /// returns document offset for next page
    int getNextPageOffset();
    /// returns document offset for previous page
    int getPrevPageOffset();
    /// selects link on page, if any (delta==0 - current, 1-next, -1-previous). returns selected link range, null if no links.
    virtual ldomXRange * selectPageLink( int delta, bool wrapAround);
    /// create document and set flags
    void CreateEmptyDoc();
    /// get document rectangle for specified cursor position, returns false if not visible
    bool getCursorDocRect( ldomXPointer ptr, lvRect & rc);
    /// get screen rectangle for specified cursor position, returns false if not visible
    bool getCursorRect( ldomXPointer ptr, lvRect & rc, bool scrollToCursor = false);

public:
    /// ensure current position is set to current bookmark value
    void CheckPos();

    /// draw current page to specified buffer
    void Draw(LVDrawBuf & drawbuf);
    
    /// Close document
    void Close();
    /// get screen rectangle for current cursor position, returns false if not visible
    bool getCursorRect(lvRect & rc, bool scrollToCursor = false)
    {
        return getCursorRect(m_cursorPos, rc, scrollToCursor);
    }
    /// returns cursor position
    ldomXPointer getCursorPos() { return m_cursorPos; }
    /// set cursor position
    void setCursorPos( ldomXPointer ptr ) { m_cursorPos = ptr; }

    /// returns selected (marked) ranges
    ldomMarkedRangeList* getMarkedRanges() { return &m_markRanges; }

    /// returns XPointer to middle paragraph of current page
    ldomXPointer getCurrentPageMiddleParagraph();
    /// render document, if not rendered
    void CheckRender();
    /// packs current file path and name
    lString16 getNavigationPath();
	/// -1 moveto previous page, 1 to next page
	bool moveByPage( int delta);
    /// sets new list of bookmarks, removes old values
    void setBookmarkList(LVPtrVector<CRBookmark> & bookmarks);
    /// find bookmark by window point, return NULL if point doesn't belong to any bookmark
    CRBookmark * findBookmarkByPoint(lvPoint pt);
    /// returns true if coverpage display is on
    bool getShowCover() { return  show_cover; }
    /// sets coverpage display flag
    void setShowCover( bool show ) { show_cover = show; }
    /// returns true if page image is available (0=current, -1=prev, 1=next)
    bool isPageImageReady( int delta);

    /// applies properties, returns list of not recognized properties
    void PropsApply(CRPropRef props);

    /// get background image
    LVImageSourceRef getBackgroundImage() const { return m_backgroundImage; }
    /// set background image
    void setBackgroundImage(LVImageSourceRef bgImage, bool tiled=true)
    {
    	m_backgroundImage = bgImage;
    	m_backgroundTiled = tiled;
    	m_backgroundImageScaled.Clear();
    }
    /// clears page background
    void drawPageBackground( LVDrawBuf & drawbuf, int offsetX, int offsetY);

    // doc format functions
    /// set text format options
    void setTextFormatOptions( txt_format_t fmt);
    /// get text format options
    txt_format_t getTextFormatOptions();
    /// get current document format
    doc_format_t getDocFormat() { return m_doc_format; }

    // Links and selections functions
    /// sets selection for whole element, clears previous selection
    virtual void selectElement( ldomNode * elem);
    /// sets selection for range, clears previous selection
    virtual void selectRange( const ldomXRange & range);
    /// sets selection for list of words, clears previous selection
    virtual void selectWords( const LVArray<ldomWord> & words);
    /// sets selections for ranges, clears previous selections
    virtual void selectRanges(ldomXRangeList & ranges);
    /// clears selection
    virtual void ClearSelection();
    /// update selection -- command handler
    int onSelectionCommand(int cmd, int param);

    /// get list of links
    virtual void getCurrentPageLinks( ldomXRangeList & list);
    /// selects first link on page, if any. returns selected link range, null if no links.
    virtual ldomXRange * selectFirstPageLink();
    /// selects next link on page, if any. returns selected link range, null if no links.
    virtual ldomXRange * selectNextPageLink( bool wrapAround);
    /// selects previous link on page, if any. returns selected link range, null if no links.
    virtual ldomXRange * selectPrevPageLink( bool wrapAround);
    /// returns selected link on page, if any. null if no links.
    virtual ldomXRange * getCurrentPageSelectedLink();
    /// follow link, returns true if navigation was successful
    virtual bool goLink(lString16 href);
    /// follow selected link, returns true if navigation was successful
    virtual bool goSelectedLink();

    /// returns default font face
    lString8 getDefaultFontFace() { return def_font_name_; }
    /// set default font face
    void setDefaultFontFace( const lString8 & newFace);
    /// invalidate formatted data, request render
    void RequestRender();
    /// update selection ranges
    void updateSelections();
    void updateBookMarksRanges();
    /// get page document range, -1 for current page
    LVRef<ldomXRange> getPageDocumentRange( int pageIndex=-1);
    /// get page text, -1 for current page
    lString16 getPageText( bool wrapWords, int pageIndex=-1);
    /// returns number of non-space characters on current page
    int getCurrentPageCharCount();
    /// returns number of images on current page
    int getCurrentPageImageCount();
    /// sets page margins
    void setPageMargins( const lvRect & rc);
    /// returns page margins
    lvRect getPageMargins() const { return m_pageMargins; }
    /// returns true if document is opened
    bool IsDocOpened();
    /// returns if Render has been called
    bool IsRendered() { return is_rendered_; }
    /// returns formatted page list
    LVRendPageList* getPageList() { return &m_pages; }
    /// returns pointer to TOC root node
    LvTocItem* getToc();
    /// returns pointer to TOC root node
    bool getFlatToc( LVPtrVector<LvTocItem, false> & items);
    /// update page numbers for items
    void updatePageNumbers( LvTocItem * item);
    /// set view mode (pages/scroll)
    void setViewMode( LVDocViewMode view_mode, int visiblePageCount=-1);
    /// get view mode (pages/scroll)
    LVDocViewMode getViewMode();
    /// toggle pages/scroll view mode
    void toggleViewMode();
    /// get window visible page count (1 or 2)
    int getVisiblePageCount();
    /// set window visible page count (1 or 2)
    void setVisiblePageCount( int n);
    /// returns xpointer for specified window point
    ldomXPointer getNodeByPoint( lvPoint pt);
    /// returns image source for specified window point, if point is inside image
    LVImageSourceRef getImageByPoint(lvPoint pt);
    /// draws scaled image into buffer, clear background according to current settings
    bool drawImage(LVDrawBuf * buf, LVImageSourceRef img, int x, int y, int dx, int dy);
    /// converts point from window to document coordinates, returns true if success
    bool windowToDocPoint( lvPoint & pt);
    /// converts point from documsnt to window coordinates, returns true if success
    bool docToWindowPoint( lvPoint & pt);

    /// returns document
    CrDom* getDocument() { return m_doc; }
    /// return document properties
    CRPropRef getDocProps() { return doc_props_; }
    /// returns book title
    lString16 getTitle() { return doc_props_->getStringDef(DOC_PROP_TITLE); }
    /// returns book language
    lString16 getLanguage() { return doc_props_->getStringDef(DOC_PROP_LANGUAGE); }
    /// returns book author(s)
    lString16 getAuthors() { return doc_props_->getStringDef(DOC_PROP_AUTHORS); }
    lString16 getSeriesName()
    {
        lString16 name = doc_props_->getStringDef(DOC_PROP_SERIES_NAME);
        return name;
    }
    int getSeriesNumber()
    {
        lString16 name = doc_props_->getStringDef(DOC_PROP_SERIES_NAME);
        lString16 number = doc_props_->getStringDef(DOC_PROP_SERIES_NUMBER);
        if (!name.empty() && !number.empty())
            return number.atoi();
        return 0;
    }
    /// draws page to image buffer
    void drawPageTo(LVDrawBuf* drawBuf, LVRendPageInfo& page, lvRect* pageRect, int pageCount, int basePage);
    /// draws coverpage to image buffer
    void drawCoverTo(LVDrawBuf* drawBuf, lvRect& rc);
    /// returns cover page image source, if any
    LVImageSourceRef getCoverPageImage();

    /// returns bookmark
    ldomXPointer getBookmark();
    /// returns bookmark for specified page
    ldomXPointer getPageBookmark( int page);
    /// sets current bookmark
    void setBookmark( ldomXPointer bm);
    /// moves position to bookmark
    void goToBookmark( ldomXPointer bm);
    /// get page number by bookmark
    int getBookmarkPage(ldomXPointer bm);
    /// get bookmark position text
    bool getBookmarkPosText( ldomXPointer bm, lString16 & titleText, lString16 & posText);

    /// returns scrollbar control info
    const LVScrollInfo * getScrollInfo() { updateScroll(); return &m_scrollinfo; }
    /// move to position specified by scrollbar
    bool goToScrollPos( int pos);
    /// converts scrollbar pos to doc pos
    int scrollPosToDocPos( int scrollpos);
    /// returns position in 1/100 of percents
    int getPosPercent();

    /// execute command
    int doCommand( LVDocCmd cmd, int param=0);

    /// set document stylesheet text
    void setStyleSheet( lString8 css_text);

    /// set default interline space, percent (100..200)
    void setDefaultInterlineSpace( int percent);

    /// change font size, if rollCyclic is true, largest font is followed by smallest and vice versa
    void ZoomFont( int delta);
    /// retrieves current base font size
    int  getFontSize() { return font_size_; }
    /// sets new base font size
    void setFontSize(int newSize);
    /// sets posible base font sizes (for ZoomFont)
    void setFontSizes(LVArray<int> & sizes);

    /// get drawing buffer
    //LVDrawBuf * GetDrawBuf() { return &m_drawbuf; }
    /// draw document into buffer
    //void Draw();

    /// resize view
    void Resize( int dx, int dy);
    /// get view height
    int GetHeight();
    /// get view width
    int GetWidth();

    /// get full document height
    int GetFullHeight();

    /// get vertical position of view inside document
    int GetPos();
    /// get position of view inside document
    void GetPos( lvRect & rc);
    /// set vertical position of view inside document
    int SetPos( int pos, bool savePos=true);

	int getPageHeight(int pageIndex);

    /// get number of current page
    int getCurPage();
    /// move to specified page
    bool goToPage(int page, bool updatePosBookmark = true);
    /// returns page count
    int getPagesCount();

    /// clear view
    void Clear();
    void WriteDomToXml();
    /// load document from file
    bool LoadDoc(const char* crengine_uri);
    /// render (format) document
    void Render(int dx = 0, int dy = 0, LVRendPageList *pages = NULL);
    /// set properties before rendering
    void setRenderProps( int dx, int dy);

    bool LoadDocSection(lString16Collection *meta_info, const lChar16 *uri);

    /// Constructor
    LVDocView();
    /// Destructor
    virtual ~LVDocView();
};

#endif
