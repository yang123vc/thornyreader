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

#include "lvtinydom.h"
#include "lvpagesplitter.h"
#include "lvdrawbuf.h"
#include "lvptrvec.h"
#include "bookmark.h"

/// document view mode: pages/scroll
enum LVDocViewMode
{
    MODE_SCROLL,
    MODE_PAGES
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

class LVDocView;

class LVPageWordSelector
{
    LVDocView* doc_view_;
    ldomWordExList words_;
    void UpdateSelection();
public:
    // selects middle word of current page
    LVPageWordSelector(LVDocView* docview);
    // clears selection
    ~LVPageWordSelector();
    // move current word selection in specified direction, (distance) times
    void MoveBy(MoveDirection dir, int distance = 1);
    // returns currently selected word
    ldomWordEx* GetSelectedWord() { return words_.getSelWord(); }
    // access to words
    ldomWordExList & GetWords() { return words_; }
    // append chars to search pattern
    ldomWordEx* AppendPattern(lString16 chars);
    // remove last item from pattern
    ldomWordEx* ReducePattern();
    // selects word of current page with specified coords;
    void SelectWord(int x, int y);
};

class LVDocView
{
private:
    LVContainerRef container_;
    LVContainerRef archive_container_;
    LVPtrVector<CRBookmark> bookmarks_;
    ldomXPointer bookmark_;
    LVRendPageList pages_list_;
    LVScrollInfo scroll_info_;
    LVImageSourceRef background_image;
    LVRef<LVColorDrawBuf> background_image_scaled_;
    lvRect page_rects_[2];
    CRPropRef doc_props_;
    /// edit cursor position
    ldomXPointer cursor_pos_;
    ldomMarkedRangeList marked_ranges_;
    ldomMarkedRangeList bookmark_ranges_;
    font_ref_t base_font_;
    LVStreamRef stream_;
    CrDom* cr_dom_;
    LVDocViewMode viewport_mode_;
    // current position
    int page_; // >=0 is correct page number, < 0 - get based on offset_
    int offset_;  // >=0 is correct vertical offset inside document, < 0 - get based on page_
    bool is_rendered_;
    int highlight_bookmarks_;
    lvRect margins_;
    bool show_cover_;
    bool background_tiled_;
	inline bool IsPagesMode() { return viewport_mode_ == MODE_PAGES; }
	inline bool IsScrollMode() { return viewport_mode_ == MODE_SCROLL; }
    void UpdateScroll();
    /// load document from stream
    bool LoadDoc(int doc_format, LVStreamRef stream);
    /// create empty document with specified message (to show errors)
    void CreateEmptyDom();
    /// ensure current position is set to current bookmark value
    void CheckPos();
    /// set properties before rendering
    void CheckRenderProps(int dx, int dy);
protected:
    /// selects link on page, if any (delta==0 - current, 1-next, -1-previous).
    // Returns selected link range, null if no links.
    virtual ldomXRange* selectPageLink( int delta, bool wrapAround);
    /// get document rectangle for specified cursor position, returns false if not visible
    bool getCursorDocRect( ldomXPointer ptr, lvRect & rc);
    /// get screen rectangle for specified cursor position, returns false if not visible
    bool getCursorRect( ldomXPointer ptr, lvRect & rc, bool scrollToCursor = false);
public:
    bool position_is_set_;
    int doc_format_;
    lString8 config_font_face_;
    int width_;
    int height_;
    int page_columns_;
    lUInt32 background_color_;
    lUInt32 text_color_;
    lvRect config_margins_;
    int config_font_size_;
    int config_interline_space_;
    bool config_embeded_styles_;
    bool config_embeded_fonts_;
    bool config_enable_footnotes_;
    bool config_txt_smart_format_;

    /// draw current page to specified buffer
    void Draw(LVDrawBuf & drawbuf, bool autoResize = true);
    void UpdateLayout();
    /// get screen rectangle for current cursor position, returns false if not visible
    bool getCursorRect(lvRect & rc, bool scrollToCursor = false)
    {
        return getCursorRect(cursor_pos_, rc, scrollToCursor);
    }
    /// returns cursor position
    ldomXPointer getCursorPos() { return cursor_pos_; }
    /// set cursor position
    void setCursorPos( ldomXPointer ptr ) { cursor_pos_ = ptr; }
    /// returns selected (marked) ranges
    ldomMarkedRangeList* getMarkedRanges() { return &marked_ranges_; }
    /// returns XPointer to middle paragraph of current page
    ldomXPointer getCurrentPageMiddleParagraph();
    /// render document, if not rendered
    void RenderIfDirty();
    /// packs current file path and name
    lString16 getNavigationPath();
    /// sets new list of bookmarks, removes old values
    void SetBookmarks(LVPtrVector<CRBookmark> & bookmarks);
    /// find bookmark by window point, return NULL if point doesn't belong to any bookmark
    CRBookmark* FindBookmarkByPoint(lvPoint pt);
    /// sets coverpage display flag
    void setShowCover(bool show) { show_cover_ = show; }
    /// get background image
    LVImageSourceRef getBackgroundImage() const { return background_image; }
    /// set background image
    void setBackgroundImage(LVImageSourceRef bgImage, bool tiled=true)
    {
    	background_image = bgImage;
    	background_tiled_ = tiled;
    	background_image_scaled_.Clear();
    }
    /// clears page background
    void DrawBackgroundTo(LVDrawBuf & drawbuf, int offsetX, int offsetY, int alpha = 0);
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
    /// invalidate formatted data, request render
    void RequestRender();
    /// update selection ranges
    void UpdateSelections();
    void UpdateBookmarksRanges();
    /// get page document range, -1 for current page
    LVRef<ldomXRange> getPageDocumentRange( int pageIndex=-1);
    /// get page text, -1 for current page
    lString16 getPageText( bool wrapWords, int pageIndex=-1);
    int GetColumns();
    void UpdatePageMargins();
    /// returns pointer to TOC root node
    void GetOutline(LVPtrVector<LvTocItem, false>& outline);
    /// returns xpointer for specified window point
    ldomXPointer getNodeByPoint( lvPoint pt);
    /// returns image source for specified window point, if point is inside image
    LVImageSourceRef getImageByPoint(lvPoint pt);

    /// converts point from window to document coordinates, returns true if success
    bool WindowToDocPoint( lvPoint & pt);
    /// converts point from documsnt to window coordinates, returns true if success
    bool DocToWindowPoint( lvPoint & pt);
    /// returns document
    CrDom* GetCrDom() { return cr_dom_; }
    /// draws scaled image into buffer, clear background according to current settings
    bool DrawImageTo(LVDrawBuf* buf, LVImageSourceRef img, int x, int y, int dx, int dy);
    /// draws page to image buffer
    void DrawPageTo(LVDrawBuf* buf, LVRendPageInfo& page, lvRect* pageRect);
    /// draws coverpage to image buffer
    void DrawCoverTo(LVDrawBuf* buf, lvRect& rc);
    /// returns cover page image source, if any
    LVImageSourceRef getCoverPageImage();
    /// returns bookmark
    ldomXPointer GetBookmark();
    /// returns bookmark for specified page
    ldomXPointer getPageBookmark( int page);
    /// sets current bookmark
    void SetBookmark( ldomXPointer bm);
    /// moves position to bookmark
    void GoToBookmark( ldomXPointer bm);
    /// get page number by bookmark
    int GetPageForBookmark(ldomXPointer bm);
    /// get bookmark position text
    bool getBookmarkPosText( ldomXPointer bm, lString16 & titleText, lString16 & posText);
    /// returns scrollbar control info
    const LVScrollInfo * getScrollInfo() { UpdateScroll(); return &scroll_info_; }
    /// move to position specified by scrollbar
    bool goToScrollPos( int pos);
    /// converts scrollbar pos to doc pos
    int scrollPosToDocPos( int scrollpos );
    void Resize(int dx, int dy);
    /// get view height
    int GetHeight();
    /// get view width
    int GetWidth();
    /// get full document height
    int GetFullHeight();
    /// get vertical position of view inside document
    int GetOffset();
    /// set vertical position of view inside document
    int GoToOffset(int pos, bool savePos=true, bool allowScrollAfterEnd = false);
    /// get number of current page
    int GetCurrPage();
    /// move to specified page
    bool GoToPage(int page, bool updatePosBookmark = true);
    /// returns page count
    int GetPagesCount();
    /// clear view
    void Clear();
    /// load document from file
    bool LoadDoc(int doc_format, const char* crengine_uri);
    LVDocView();
    virtual ~LVDocView();
};
#endif