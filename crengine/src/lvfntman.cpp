/** \file lvfntman.cpp
    \brief font manager implementation

    CoolReader Engine


    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.
*/
#include <stdlib.h>
#include <stdio.h>

#include "crsetup.h"
#include "lvfntman.h"
#include "lvstream.h"
#include "lvdrawbuf.h"
#include "lvstyles.h"

#define GAMMA_TABLES_IMPL
#include "gammatbl.h"

#ifdef ANDROID
#include "freetype/config/ftheader.h"
#include "freetype/freetype.h"
#else //#ifdef ANDROID

#include <freetype/config/ftheader.h>
#include FT_FREETYPE_H

#endif //#ifdef ANDROID

#if (USE_FONTCONFIG==1)
#include <fontconfig/fontconfig.h>
#endif

#define MAX_LINE_CHARS 2048

inline int myabs(int n) { return n < 0 ? -n : n; }

LVFontManager* fontMan = NULL;

static double gammaLevel = 1.0;
static int gammaIndex = GAMMA_LEVELS/2;

/// returns first found face from passed list, or return face for font found by family only
lString8 LVFontManager::findFontFace(lString8 commaSeparatedFaceList, css_font_family_t fallbackByFamily) {
	// faces we want
	lString8Collection list;
	splitPropertyValueList(commaSeparatedFaceList.c_str(), list);
	// faces we have
	lString16Collection faces;
	getFaceList(faces);
	// find first matched
	for (int i = 0; i < list.length(); i++) {
		lString8 wantFace = list[i];
		for (int j = 0; j < faces.length(); j++) {
			lString16 haveFace = faces[j];
			if (wantFace == haveFace)
				return wantFace;
		}
	}
	// not matched - get by family name
    LVFontRef fnt = GetFont(10, 400, false, fallbackByFamily, lString8("Arial"));
    if (fnt.isNull())
    	return lString8::empty_str; // not found
    // get face from found font
    return fnt->getTypeFace();
}

/// fills array with list of available gamma levels
void LVFontManager::GetGammaLevels(LVArray<double> dst) {
    dst.clear();
    for ( int i=0; i<GAMMA_LEVELS; i++ )
        dst.add(cr_gamma_levels[i]);
}

/// returns current gamma level index
int  LVFontManager::GetGammaIndex() {
    return gammaIndex;
}

/// sets current gamma level index
void LVFontManager::SetGammaIndex( int index ) {
    if ( index<0 )
        index = 0;
    if ( index>=GAMMA_LEVELS )
        index = GAMMA_LEVELS-1;
    if ( gammaIndex!=index ) {
        CRLog::trace("FontManager gamma index changed from %d to %d", gammaIndex, index);
        gammaIndex = index;
        gammaLevel = cr_gamma_levels[index];
        clearGlyphCache();
    }
}

/// returns current gamma level
double LVFontManager::GetGamma() {
    return gammaLevel;
}

/// sets current gamma level
void LVFontManager::SetGamma( double gamma ) {
//    gammaLevel = cr_ft_gamma_levels[GAMMA_LEVELS/2];
//    gammaIndex = GAMMA_LEVELS/2;
    int oldGammaIndex = gammaIndex;
    for ( int i=0; i<GAMMA_LEVELS; i++ ) {
        double diff1 = cr_gamma_levels[i] - gamma;
        if ( diff1<0 ) diff1 = -diff1;
        double diff2 = gammaLevel - gamma;
        if ( diff2<0 ) diff2 = -diff2;
        if ( diff1 < diff2 ) {
            gammaLevel = cr_gamma_levels[i];
            gammaIndex = i;
        }
    }
    if ( gammaIndex!=oldGammaIndex ) {
        CRLog::trace("FontManager gamma index changed from %d to %d", oldGammaIndex, gammaIndex);
        clearGlyphCache();
    }
}


////////////////////////////////////////////////////////////////////

static const char * EMBEDDED_FONT_LIST_MAGIC = "FNTL";
static const char * EMBEDDED_FONT_DEF_MAGIC = "FNTD";

////////////////////////////////////////////////////////////////////
// LVEmbeddedFontDef
////////////////////////////////////////////////////////////////////
bool LVEmbeddedFontDef::serialize(SerialBuf & buf) {
    buf.putMagic(EMBEDDED_FONT_DEF_MAGIC);
    buf << _url << _face << _bold << _italic;
    return !buf.error();
}

bool LVEmbeddedFontDef::deserialize(SerialBuf & buf) {
    if (!buf.checkMagic(EMBEDDED_FONT_DEF_MAGIC))
        return false;
    buf >> _url >> _face >> _bold >> _italic;
    return !buf.error();
}

////////////////////////////////////////////////////////////////////
// LVEmbeddedFontList
////////////////////////////////////////////////////////////////////
LVEmbeddedFontDef * LVEmbeddedFontList::findByUrl(lString16 url) {
    for (int i=0; i<length(); i++) {
        if (get(i)->getUrl() == url)
            return get(i);
    }
    return NULL;
}

bool LVEmbeddedFontList::addAll(LVEmbeddedFontList & list) {
    bool changed = false;
    for (int i=0; i<list.length(); i++) {
        LVEmbeddedFontDef * def = list.get(i);
        changed = add(def->getUrl(), def->getFace(), def->getBold(), def->getItalic()) || changed;
    }
    return changed;
}

bool LVEmbeddedFontList::add(lString16 url, lString8 face, bool bold, bool italic) {
    LVEmbeddedFontDef * def = findByUrl(url);
    if (def) {
        bool changed = false;
        if (def->getFace() != face) {
            def->setFace(face);
            changed = true;
        }
        if (def->getBold() != bold) {
            def->setBold(bold);
            changed = true;
        }
        if (def->getItalic() != italic) {
            def->setItalic(italic);
            changed = true;
        }
        return changed;
    }
    def = new LVEmbeddedFontDef(url, face, bold, italic);
    add(def);
	return false;
}

bool LVEmbeddedFontList::serialize(SerialBuf & buf) {
    buf.putMagic(EMBEDDED_FONT_LIST_MAGIC);
    lUInt32 count = length();
    buf << count;
    for (lUInt32 i = 0; i < count; i++) {
        get(i)->serialize(buf);
        if (buf.error())
            return false;
    }
    return !buf.error();
}

bool LVEmbeddedFontList::deserialize(SerialBuf & buf) {
    if (!buf.checkMagic(EMBEDDED_FONT_LIST_MAGIC))
        return false;
    lUInt32 count = 0;
    buf >> count;
    if (buf.error())
        return false;
    for (lUInt32 i = 0; i < count; i++) {
        LVEmbeddedFontDef * item = new LVEmbeddedFontDef();
        if (!item->deserialize(buf)) {
            delete item;
            return false;
        }
        add(item);
    }
    return !buf.error();
}

////////////////////////////////////////////////////////////////////


/**
 * Max width of -/./,/!/? to use for visial alignment by width
 */
int LVFont::getVisualAligmentWidth()
{
    if ( _visual_alignment_width==-1 ) {
        lChar16 chars[] = { getHyphChar(), ',', '.', '!', ':', ';',
                            (lChar16)L'\xff0c', (lChar16)L'\x3302', (lChar16)L'\xff01', 0 };
        //                  (lChar16)L'，', (lChar16)L'。', (lChar16)L'！', 0 };
        //                  65292 12290 65281
        //                  ff0c 3002 ff01
        int maxw = 0;
        for ( int i=0; chars[i]; i++ ) {
            int w = getCharWidth( chars[i] );
            if ( w > maxw )
                maxw = w;
        }
        _visual_alignment_width = maxw;
    }
    return _visual_alignment_width;
}

/**
    \brief Font properties definition
*/
class LVFontDef
{
private:
    int               _size;
    int               _weight;
    int               _italic;
    css_font_family_t _family;
    lString8          _typeface;
    lString8          _name;
    int               _index;
    // for document font: _documentId, _buf, _name
    int               _documentId;
    LVByteArrayRef    _buf;
public:
    LVFontDef(const lString8 & name, int size, int weight, int italic, css_font_family_t family, const lString8 & typeface, int index=-1, int documentId=-1, LVByteArrayRef buf = LVByteArrayRef())
    : _size(size)
    , _weight(weight)
    , _italic(italic)
    , _family(family)
    , _typeface(typeface)
    , _name(name)
    , _index(index)
    , _documentId(documentId)
    , _buf(buf)
    {
    }
    LVFontDef(const LVFontDef & def)
    : _size(def._size)
    , _weight(def._weight)
    , _italic(def._italic)
    , _family(def._family)
    , _typeface(def._typeface)
    , _name(def._name)
    , _index(def._index)
    , _documentId(def._documentId)
    , _buf(def._buf)
    {
    }

    /// returns true if definitions are equal
    bool operator == ( const LVFontDef & def ) const
    {
        return ( _size == def._size || _size == -1 || def._size == -1 )
            && ( _weight == def._weight || _weight==-1 || def._weight==-1 )
            && ( _italic == def._italic || _italic==-1 || def._italic==-1 )
            && _family == def._family
            && _typeface == def._typeface
            && _name == def._name
            && ( _index == def._index || def._index == -1 )
            && (_documentId == def._documentId || _documentId == -1)
            ;
    }

    lUInt32 getHash() {
        return ((((_size * 31) + _weight)*31  + _italic)*31 + _family)*31 + _name.getHash();
    }

    /// returns font file name
    lString8 getName() const { return _name; }
    void setName( lString8 name) {  _name = name; }
    int getIndex() const { return _index; }
    void setIndex( int index ) { _index = index; }
    int getSize() const { return _size; }
    void setSize( int size ) { _size = size; }
    int getWeight() const { return _weight; }
    void setWeight( int weight ) { _weight = weight; }
    bool getItalic() const { return _italic!=0; }
    bool isRealItalic() const { return _italic==1; }
    void setItalic( int italic ) { _italic=italic; }
    css_font_family_t getFamily() const { return _family; }
    void getFamily( css_font_family_t family ) { _family = family; }
    lString8 getTypeFace() const { return _typeface; }
    void setTypeFace(lString8 tf) { _typeface = tf; }
    int getDocumentId() { return _documentId; }
    void setDocumentId(int id) { _documentId = id; }
    LVByteArrayRef getBuf() { return _buf; }
    void setBuf(LVByteArrayRef buf) { _buf = buf; }
    ~LVFontDef() {}
    /// calculates difference between two fonts
    int CalcMatch( const LVFontDef & def ) const;
    /// difference between fonts for duplicates search
    int CalcDuplicateMatch( const LVFontDef & def ) const;
    /// calc match for fallback font search
    int CalcFallbackMatch( lString8 face, int size ) const;
};

/// font cache item
class LVFontCacheItem
{
    friend class LVFontCache;
    LVFontDef _def;
    LVFontRef _fnt;
public:
    LVFontDef * getDef() { return &_def; }
    LVFontRef & getFont() { return _fnt; }
    void setFont(LVFontRef & fnt) { _fnt = fnt; }
    LVFontCacheItem( const LVFontDef & def )
    : _def( def )
    { }
};

/// font cache
class LVFontCache
{
    LVPtrVector< LVFontCacheItem > _registered_list;
    LVPtrVector< LVFontCacheItem > _instance_list;
public:
    void clear() { _registered_list.clear(); _instance_list.clear(); }
    void gc(); // garbage collector
    void update( const LVFontDef * def, LVFontRef ref );
    void removefont(const LVFontDef * def);
    void removeDocumentFonts(int documentId);
    int  length() { return _registered_list.length(); }
    void addInstance( const LVFontDef * def, LVFontRef ref );
    LVPtrVector< LVFontCacheItem > * getInstances() { return &_instance_list; }
    LVFontCacheItem * find( const LVFontDef * def );
    LVFontCacheItem * findFallback( lString8 face, int size );
    LVFontCacheItem * findDuplicate( const LVFontDef * def );
    LVFontCacheItem * findDocumentFontDuplicate(int documentId, lString8 name);
    /// get hash of installed fonts and fallback font
    virtual lUInt32 GetFontListHash(int documentId) {
        lUInt32 hash = 0;
        for ( int i=0; i<_registered_list.length(); i++ ) {
            int doc = _registered_list[i]->getDef()->getDocumentId();
            if (doc == -1 || doc == documentId) // skip document fonts
                hash = hash + _registered_list[i]->getDef()->getHash();
        }
        return 0;
    }
    virtual void getFaceList( lString16Collection & list )
    {
        list.clear();
        for ( int i=0; i<_registered_list.length(); i++ ) {
            if (_registered_list[i]->getDef()->getDocumentId() != -1)
                continue;
            lString16 name = Utf8ToUnicode( _registered_list[i]->getDef()->getTypeFace() );
            if ( !list.contains(name) )
                list.add( name );
        }
        list.sort();
    }
    virtual void clearFallbackFonts()
    {
        for ( int i=0; i<_registered_list.length(); i++ ) {
            _registered_list[i]->getFont()->setFallbackFont(LVFontRef());
        }
    }
    LVFontCache( )
    { }
    virtual ~LVFontCache() { }
};


#if (USE_FREETYPE==1)

static lChar16 getReplacementChar(lUInt16 code)
{
    switch (code) {
    case UNICODE_SOFT_HYPHEN_CODE:
        return '-';
    case 0x0401: // CYRILLIC CAPITAL LETTER IO
        return 0x0415; //CYRILLIC CAPITAL LETTER IE
    case 0x0451: // CYRILLIC SMALL LETTER IO
        return 0x0435; // CYRILLIC SMALL LETTER IE
    case UNICODE_NO_BREAK_SPACE:
        return ' ';
    case 0x2010:
    case 0x2011:
    case 0x2012:
    case 0x2013:
    case 0x2014:
    case 0x2015:
        return '-';
    case 0x2018:
    case 0x2019:
    case 0x201a:
    case 0x201b:
        return '\'';
    case 0x201c:
    case 0x201d:
    case 0x201e:
    case 0x201f:
    case 0x00ab:
    case 0x00bb:
        return '\"';
    case 0x2039:
        return '<';
    case 0x203A:
        return '>';
    case 0x2044:
        return '/';
    case 0x2022: // css_lst_disc:
        return '*';
    case 0x26AA: // css_lst_disc:
    case 0x25E6: // css_lst_disc:
    case 0x25CF: // css_lst_disc:
        return 'o';
    case 0x25CB: // css_lst_circle:
        return '*';
    case 0x25A0: // css_lst_square:
        return '-';
    }
    return 0;
}


class LVFontGlyphWidthCache
{
private:
    lUInt8 * ptrs[128];
public:
    lUInt8 get( lChar16 ch )
    {
        int inx = (ch>>9) & 0x7f;
        lUInt8 * ptr = ptrs[inx];
        if ( !ptr )
            return 0xFF;
        return ptr[ch & 0x1FF ];
    }
    void put( lChar16 ch, lUInt8 w )
    {
        int inx = (ch>>9) & 0x7f;
        lUInt8 * ptr = ptrs[inx];
        if ( !ptr ) {
            ptr = new lUInt8[512];
            ptrs[inx] = ptr;
            memset( ptr, 0xFF, sizeof(lUInt8) * 512 );
        }
        ptr[ ch & 0x1FF ] = w;
    }
    void clear()
    {
        for ( int i=0; i<128; i++ ) {
            if ( ptrs[i] )
                delete [] ptrs[i];
            ptrs[i] = NULL;
        }
    }
    LVFontGlyphWidthCache()
    {
        memset( ptrs, 0, 128*sizeof(lUInt8*) );
    }
    ~LVFontGlyphWidthCache()
    {
        clear();
    }
};

class LVFreeTypeFace;
static LVFontGlyphCacheItem * newItem( LVFontLocalGlyphCache * local_cache, lChar16 ch, FT_GlyphSlot slot ) // , bool drawMonochrome
{
    FT_Bitmap*  bitmap = &slot->bitmap;
    lUInt8 w = (lUInt8)(bitmap->width);
    lUInt8 h = (lUInt8)(bitmap->rows);
    LVFontGlyphCacheItem * item = LVFontGlyphCacheItem::newItem(local_cache, ch, w, h );
    if ( bitmap->pixel_mode==FT_PIXEL_MODE_MONO ) { //drawMonochrome
        lUInt8 mask = 0x80;
        const lUInt8 * ptr = (const lUInt8 *)bitmap->buffer;
        lUInt8 * dst = item->bmp;
        //int rowsize = ((w + 15) / 16) * 2;
        for ( int y=0; y<h; y++ ) {
            const lUInt8 * row = ptr;
            mask = 0x80;
            for ( int x=0; x<w; x++ ) {
                *dst++ = (*row & mask) ? 0xFF : 00;
                mask >>= 1;
                if ( !mask && x!=w-1) {
                    mask = 0x80;
                    row++;
                }
            }
            ptr += bitmap->pitch;//rowsize;
        }
    } else {
#if 0
        if ( bitmap->pixel_mode==FT_PIXEL_MODE_MONO ) {
            memset( item->bmp, 0, w*h );
            lUInt8 * srcrow = bitmap->buffer;
            lUInt8 * dstrow = item->bmp;
            for ( int y=0; y<h; y++ ) {
                lUInt8 * src = srcrow;
                for ( int x=0; x<w; x++ ) {
                    dstrow[x] =  ( (*src)&(0x80>>(x&7)) ) ? 255 : 0;
                    if ((x&7)==7)
                        src++;
                }
                srcrow += bitmap->pitch;
                dstrow += w;
            }
        } else {
#endif
            memcpy( item->bmp, bitmap->buffer, w*h );
            // correct gamma
            if ( gammaIndex!=GAMMA_LEVELS/2 )
                cr_correct_gamma_buf(item->bmp, w*h, gammaIndex);
//            }
    }
    item->origin_x =   (lInt8)slot->bitmap_left;
    item->origin_y =   (lInt8)slot->bitmap_top;
    item->advance =    (lUInt8)(myabs(slot->metrics.horiAdvance) >> 6);
    return item;
}

void LVFontLocalGlyphCache::clear()
{
    while ( head ) {
        LVFontGlyphCacheItem * ptr = head;
        remove( ptr );
        global_cache->remove( ptr );
        LVFontGlyphCacheItem::freeItem( ptr );
    }
}

LVFontGlyphCacheItem * LVFontLocalGlyphCache::get( lUInt16 ch )
{
    LVFontGlyphCacheItem * ptr = head;
    for ( ; ptr; ptr = ptr->next_local ) {
        if ( ptr->ch == ch ) {
            global_cache->refresh( ptr );
            return ptr;
        }
    }
    return NULL;
}

void LVFontLocalGlyphCache::put( LVFontGlyphCacheItem * item )
{
    global_cache->put( item );
    item->next_local = head;
    if ( head )
        head->prev_local = item;
    if ( !tail )
        tail = item;
    head = item;
}

/// remove from list, but don't delete
void LVFontLocalGlyphCache::remove( LVFontGlyphCacheItem * item )
{
    if ( item==head )
        head = item->next_local;
    if ( item==tail )
        tail = item->prev_local;
    if ( !head || !tail )
        return;
    if ( item->prev_local )
        item->prev_local->next_local = item->next_local;
    if ( item->next_local )
        item->next_local->prev_local = item->prev_local;
    item->next_local = NULL;
    item->prev_local = NULL;
}

void LVFontGlobalGlyphCache::refresh( LVFontGlyphCacheItem * item )
{
    if ( tail!=item ) {
        //move to head
        removeNoLock( item );
        putNoLock( item );
    }
}

void LVFontGlobalGlyphCache::put( LVFontGlyphCacheItem * item )
{
    putNoLock(item);
}

void LVFontGlobalGlyphCache::putNoLock( LVFontGlyphCacheItem * item )
{
    int sz = item->getSize();
    // remove extra items from tail
    while ( sz + size > max_size ) {
        LVFontGlyphCacheItem * removed_item = tail;
        if ( !removed_item )
            break;
        removeNoLock( removed_item );
        removed_item->local_cache->remove( removed_item );
        LVFontGlyphCacheItem::freeItem( removed_item );
    }
    // add new item to head
    item->next_global = head;
    if ( head )
        head->prev_global = item;
    head = item;
    if ( !tail )
        tail = item;
    size += sz;
}

void LVFontGlobalGlyphCache::remove( LVFontGlyphCacheItem * item )
{
    removeNoLock(item);
}

void LVFontGlobalGlyphCache::removeNoLock( LVFontGlyphCacheItem * item )
{
    if ( item==head )
        head = item->next_global;
    if ( item==tail )
        tail = item->prev_global;
    if ( !head || !tail )
        return;
    if ( item->prev_global )
        item->prev_global->next_global = item->next_global;
    if ( item->next_global )
        item->next_global->prev_global = item->prev_global;
    item->next_global = NULL;
    item->prev_global = NULL;
    size -= item->getSize();
}

void LVFontGlobalGlyphCache::clear()
{
    while ( head ) {
        LVFontGlyphCacheItem * ptr = head;
        remove( ptr );
        ptr->local_cache->remove( ptr );
        LVFontGlyphCacheItem::freeItem( ptr );
    }
}

lString8 familyName( FT_Face face )
{
    lString8 faceName( face->family_name );
    if ( faceName == "Arial" && face->style_name && !strcmp(face->style_name, "Narrow") )
        faceName << " " << face->style_name;
    else if ( /*faceName == "Arial" &&*/ face->style_name && strstr(face->style_name, "Condensed") )
        faceName << " " << "Condensed";
    return faceName;
}

static lUInt16 char_flags[] = {
    0, 0, 0, 0, 0, 0, 0, 0, // 0    00
    0, 0, LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER, 0, 0, LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER, 0, 0, // 8    08
    0, 0, 0, 0, 0, 0, 0, 0, // 16   10
    0, 0, 0, 0, 0, 0, 0, 0, // 24   18
    LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER, 0, 0, 0, 0, 0, 0, 0, // 32   20
    0, 0, 0, 0, 0, LCHAR_DEPRECATED_WRAP_AFTER, 0, 0, // 40   28
    0, 0, 0, 0, 0, 0, 0, 0, // 48   30
};

#define GET_CHAR_FLAGS(ch) \
     (ch<48?char_flags[ch]: \
        (ch==UNICODE_SOFT_HYPHEN_CODE?LCHAR_ALLOW_WRAP_AFTER: \
        (ch==UNICODE_NO_BREAK_SPACE?LCHAR_DEPRECATED_WRAP_AFTER|LCHAR_IS_SPACE: \
        (ch==UNICODE_HYPHEN?LCHAR_DEPRECATED_WRAP_AFTER:0))))

class LVFreeTypeFace : public LVFont {
protected:
    lString8      _fileName;
    lString8      _faceName;
    css_font_family_t _fontFamily;
    FT_Library    _library;
    FT_Face       _face;
    FT_GlyphSlot  _slot;
    FT_Matrix     _matrix;                 /* transformation matrix */
    int           _size; // caracter height in pixels
    int           _height; // full line height in pixels
    int           _hyphen_width;
    int           _baseline;
    int            _weight;
    int            _italic;
    LVFontGlyphWidthCache _wcache;
    LVFontLocalGlyphCache _glyph_cache;
    bool          _drawMonochrome;
    bool          _allowKerning;
    hinting_mode_t _hintingMode;
    bool          _fallbackFontIsSet;
    LVFontRef     _fallbackFont;

public:

    // fallback font support
    /// set fallback font for this font
    void setFallbackFont( LVFontRef font ) {
        _fallbackFont = font;
        _fallbackFontIsSet = !font.isNull();
    }

    /// get fallback font for this font
    LVFont * getFallbackFont() {
        if ( _fallbackFontIsSet )
            return _fallbackFont.get();
        if ( fontMan->GetFallbackFontFace()!=_faceName ) // to avoid circular link, disable fallback for fallback font
            _fallbackFont = fontMan->GetFallbackFont(_size);
        _fallbackFontIsSet = true;
        return _fallbackFont.get();
    }

    /// returns font weight
    virtual int getWeight() const { return _weight; }
    /// returns italic flag
    virtual int getItalic() const { return _italic; }
    /// sets face name
    virtual void setFaceName( lString8 face ) { _faceName = face; }

    FT_Library getLibrary() { return _library; }

    LVFreeTypeFace(FT_Library  library, LVFontGlobalGlyphCache * globalCache)
			: _fontFamily(css_ff_sans_serif),
			  _library(library),
			  _face(NULL),
			  _size(0),
			  _hyphen_width(0),
			  _baseline(0),
			  _weight(400),
			  _italic(0),
			  _glyph_cache(globalCache),
			  _drawMonochrome(false),
			  _allowKerning(false),
			  _hintingMode(HINTING_MODE_AUTOHINT),
			  _fallbackFontIsSet(false) {
        _matrix.xx = 0x10000;
        _matrix.yy = 0x10000;
        _matrix.xy = 0;
        _matrix.yx = 0;
        _hintingMode = fontMan->GetHintingMode();
    }

    virtual ~LVFreeTypeFace()
    {
        Clear();
    }

    virtual int getHyphenWidth()
    {
        if ( !_hyphen_width ) {
            _hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );
        }
        return _hyphen_width;
    }

    /// get kerning mode: true==ON, false=OFF
    virtual bool getKerning() const { return _allowKerning; }
    /// get kerning mode: true==ON, false=OFF
    virtual void setKerning( bool kerningEnabled ) { _allowKerning = kerningEnabled; }

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t mode) {
        if (_hintingMode == mode)
            return;
        _hintingMode = mode;
        _glyph_cache.clear();
        _wcache.clear();
    }
    /// returns current hinting mode
    virtual hinting_mode_t  getHintingMode() const { return _hintingMode; }

    /// get bitmap mode (true=bitmap, false=antialiased)
    virtual bool getBitmapMode() { return _drawMonochrome; }
    /// set bitmap mode (true=bitmap, false=antialiased)
    virtual void setBitmapMode( bool drawBitmap )
    {
        if ( _drawMonochrome == drawBitmap )
            return;
        _drawMonochrome = drawBitmap;
        _glyph_cache.clear();
        _wcache.clear();
    }

    bool loadFromBuffer(LVByteArrayRef buf, int index, int size, css_font_family_t fontFamily, bool monochrome, bool italicize )
    {
        _hintingMode = fontMan->GetHintingMode();
        _drawMonochrome = monochrome;
        _fontFamily = fontFamily;
        int error = FT_New_Memory_Face( _library, buf->get(), buf->length(), index, &_face ); /* create face object */
        if (error)
            return false;
        if ( _fileName.endsWith(".pfb") || _fileName.endsWith(".pfa") ) {
            lString8 kernFile = _fileName.substr(0, _fileName.length()-4);
            if ( LVFileExists(Utf8ToUnicode(kernFile) + ".afm" ) ) {
                kernFile += ".afm";
            } else if ( LVFileExists(Utf8ToUnicode(kernFile) + ".pfm" ) ) {
                kernFile += ".pfm";
            } else {
                kernFile.clear();
            }
            if ( !kernFile.empty() )
                error = FT_Attach_File( _face, kernFile.c_str() );
        }
        //FT_Face_SetUnpatentedHinting( _face, 1 );
        _slot = _face->glyph;
        _faceName = familyName(_face);
        CRLog::trace("Loaded font %s [%d]: faceName=%s, ",
                _fileName.c_str(), index, _faceName.c_str());
        //if ( !FT_IS_SCALABLE( _face ) ) {
        //    Clear();
        //    return false;
       // }
        error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            size );  /* pixel_height          */
        if (error) {
            Clear();
            return false;
        }
#if 0
        int nheight = _face->size->metrics.height;
        int targetheight = size << 6;
        error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            (size * targetheight + nheight/2)/ nheight );  /* pixel_height          */
#endif
        _height = _face->size->metrics.height >> 6;
        _size = size; //(_face->size->metrics.height >> 6);
        _baseline = _height + (_face->size->metrics.descender >> 6);
        _weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
        _italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;

        if ( !error && italicize && !_italic ) {
            _matrix.xy = 0x10000*3/10;
            FT_Set_Transform(_face, &_matrix, NULL);
            _italic = true;
        }

        if ( error ) {
            // error
            return false;
        }
        return true;
    }

    bool loadFromFile(
            const char* fname,
            int index,
            int size,
            css_font_family_t fontFamily,
            bool monochrome,
            bool italicize)
    {
        _hintingMode = fontMan->GetHintingMode();
        _drawMonochrome = monochrome;
        _fontFamily = fontFamily;
        if ( fname )
            _fileName = fname;
        if ( _fileName.empty() )
            return false;
        int error = FT_New_Face( _library, _fileName.c_str(), index, &_face ); /* create face object */
        if (error)
            return false;
        if ( _fileName.endsWith(".pfb") || _fileName.endsWith(".pfa") ) {
        	lString8 kernFile = _fileName.substr(0, _fileName.length()-4);
            if ( LVFileExists(Utf8ToUnicode(kernFile) + ".afm") ) {
        		kernFile += ".afm";
            } else if ( LVFileExists(Utf8ToUnicode(kernFile) + ".pfm" ) ) {
        		kernFile += ".pfm";
        	} else {
        		kernFile.clear();
        	}
        	if ( !kernFile.empty() )
        		error = FT_Attach_File( _face, kernFile.c_str() );
        }
        //FT_Face_SetUnpatentedHinting( _face, 1 );
        _slot = _face->glyph;
        _faceName = familyName(_face);
        CRLog::trace("Loaded font %s [%d]: faceName=%s, ",
                _fileName.c_str(), index, _faceName.c_str());
        //if ( !FT_IS_SCALABLE( _face ) ) {
        //    Clear();
        //    return false;
       // }
        error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            size );  /* pixel_height          */
        if (error) {
            Clear();
            return false;
        }
#if 0
        int nheight = _face->size->metrics.height;
        int targetheight = size << 6;
        error = FT_Set_Pixel_Sizes(
            _face,    /* handle to face object */
            0,        /* pixel_width           */
            (size * targetheight + nheight/2)/ nheight );  /* pixel_height          */
#endif
        _height = _face->size->metrics.height >> 6;
        _size = size; //(_face->size->metrics.height >> 6);
        _baseline = _height + (_face->size->metrics.descender >> 6);
        _weight = _face->style_flags & FT_STYLE_FLAG_BOLD ? 700 : 400;
        _italic = _face->style_flags & FT_STYLE_FLAG_ITALIC ? 1 : 0;

        if ( !error && italicize && !_italic ) {
            _matrix.xy = 0x10000*3/10;
            FT_Set_Transform(_face, &_matrix, NULL);
            _italic = true;
        }

        if ( error ) {
            // error
            return false;
        }
        return true;
    }


    FT_UInt getCharIndex( lChar16 code, lChar16 def_char ) {
        if ( code=='\t' )
            code = ' ';
        FT_UInt ch_glyph_index = FT_Get_Char_Index( _face, code );
        if ( ch_glyph_index==0 ) {
            lUInt16 replacement = getReplacementChar( code );
            if ( replacement )
                ch_glyph_index = FT_Get_Char_Index( _face, replacement );
            if ( ch_glyph_index==0 && def_char )
                ch_glyph_index = FT_Get_Char_Index( _face, def_char );
        }
        return ch_glyph_index;
    }

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo(lUInt16 code, glyph_info_t * glyph, lChar16 def_char=0)
    {
        //FONT_GUARD
        int glyph_index = getCharIndex( code, 0 );
        if ( glyph_index==0 ) {
            LVFont * fallback = getFallbackFont();
            if ( !fallback ) {
                // No fallback
                glyph_index = getCharIndex( code, def_char );
                if ( glyph_index==0 )
                    return false;
            } else {
                // Fallback
                return fallback->getGlyphInfo(code, glyph, def_char);
            }
        }
        int flags = FT_LOAD_DEFAULT;
        flags |= (!_drawMonochrome ? FT_LOAD_TARGET_NORMAL : FT_LOAD_TARGET_MONO);
        if (_hintingMode == HINTING_MODE_AUTOHINT)
            flags |= FT_LOAD_FORCE_AUTOHINT;
        else if (_hintingMode == HINTING_MODE_DISABLED)
            flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
        updateTransform();
        int error = FT_Load_Glyph(
            _face,          /* handle to face object */
            glyph_index,   /* glyph index           */
            flags );  /* load flags, see below */
        if ( error )
            return false;
        glyph->blackBoxX = (lUInt8)(_slot->metrics.width >> 6);
        glyph->blackBoxY = (lUInt8)(_slot->metrics.height >> 6);
        glyph->originX =   (lInt8)(_slot->metrics.horiBearingX >> 6);
        glyph->originY =   (lInt8)(_slot->metrics.horiBearingY >> 6);
        glyph->width =     (lUInt8)(myabs(_slot->metrics.horiAdvance) >> 6);
        return true;
    }
/*
  // USE GET_CHAR_FLAGS instead
    inline int calcCharFlags( lChar16 ch )
    {
        switch ( ch ) {
        case 0x0020:
            return LCHAR_IS_SPACE | LCHAR_ALLOW_WRAP_AFTER;
        case UNICODE_SOFT_HYPHEN_CODE:
            return LCHAR_ALLOW_WRAP_AFTER;
        case '-':
            return LCHAR_DEPRECATED_WRAP_AFTER;
        case '\r':
        case '\n':
            return LCHAR_IS_SPACE | LCHAR_IS_EOL | LCHAR_ALLOW_WRAP_AFTER;
        default:
            return 0;
        }
    }
  */
    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return number of characters before max_width reached
    */
    virtual lUInt16 measureText(
                        const lChar16 * text, int len,
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar16 def_char,
                        int letter_spacing = 0,
                        bool allow_hyphenation = true
                     )
    {
        if ( len <= 0 || _face==NULL )
            return 0;
        int error;

#if (ALLOW_KERNING==1)
        int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
        if ( letter_spacing<0 || letter_spacing>50 )
            letter_spacing = 0;

        //int i;

        FT_UInt previous = 0;
        lUInt16 prev_width = 0;
        int nchars = 0;
        int lastFitChar = 0;
        updateTransform();
        // measure character widths
        for ( nchars=0; nchars<len; nchars++) {
            lChar16 ch = text[nchars];
            bool isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE);
            FT_UInt ch_glyph_index = (FT_UInt)-1;
            int kerning = 0;
#if (ALLOW_KERNING==1)
            if ( use_kerning && previous>0  ) {
                if ( ch_glyph_index==(FT_UInt)-1 )
                    ch_glyph_index = getCharIndex( ch, def_char );
                if ( ch_glyph_index != 0 ) {
                    FT_Vector delta;
                    error = FT_Get_Kerning( _face,          /* handle to face object */
                                  previous,          /* left glyph index      */
                                  ch_glyph_index,         /* right glyph index     */
                                  FT_KERNING_DEFAULT,  /* kerning mode          */
                                  &delta );    /* target vector         */
                    if ( !error )
                        kerning = delta.x;
                }
            }
#endif

            flags[nchars] = GET_CHAR_FLAGS(ch); //calcCharFlags( ch );

            /* load glyph image into the slot (erase previous one) */
            int w = _wcache.get(ch);
            if ( w==0xFF ) {
                glyph_info_t glyph;
                if ( getGlyphInfo( ch, &glyph, def_char ) ) {
                    w = glyph.width;
                    _wcache.put(ch, w);
                } else {
                    widths[nchars] = prev_width;
                    continue;  /* ignore errors */
                }
                if ( ch_glyph_index==(FT_UInt)-1 )
                    ch_glyph_index = getCharIndex( ch, 0 );
//                error = FT_Load_Glyph( _face,          /* handle to face object */
//                        ch_glyph_index,                /* glyph index           */
//                        FT_LOAD_DEFAULT );             /* load flags, see below */
//                if ( error ) {
//                    widths[nchars] = prev_width;
//                    continue;  /* ignore errors */
//                }
            }
            widths[nchars] = prev_width + w + (kerning >> 6) + letter_spacing;
            previous = ch_glyph_index;
            if ( !isHyphen ) // avoid soft hyphens inside text string
                prev_width = widths[nchars];
            if ( prev_width > max_width ) {
                if ( lastFitChar < nchars + 7)
                    break;
            } else {
                lastFitChar = nchars + 1;
            }
        }

        // fill props for rest of chars
        for ( int ii=nchars; ii<len; ii++ ) {
            flags[nchars] = GET_CHAR_FLAGS( text[ii] );
        }

        //maxFit = nchars;


        // find last word
        if ( allow_hyphenation ) {
            if ( !_hyphen_width )
                _hyphen_width = getCharWidth( UNICODE_SOFT_HYPHEN_CODE );
            if ( lastFitChar > 3 ) {
                int hwStart, hwEnd;
                lStr_findWordBounds( text, len, lastFitChar-1, hwStart, hwEnd );
                if ( hwStart < lastFitChar-1 && hwEnd > hwStart+3 ) {
                    //int maxw = max_width - (hwStart>0 ? widths[hwStart-1] : 0);
                    HyphMan::hyphenate(text+hwStart, hwEnd-hwStart, widths+hwStart, flags+hwStart, _hyphen_width, max_width);
                }
            }
        }
        return lastFitChar; //nchars;
    }

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
                        const lChar16 * text, int len
        )
    {
        static lUInt16 widths[MAX_LINE_CHARS+1];
        static lUInt8 flags[MAX_LINE_CHARS+1];
        if ( len>MAX_LINE_CHARS )
            len = MAX_LINE_CHARS;
        if ( len<=0 )
            return 0;
        lUInt16 res = measureText(
                        text, len,
                        widths,
                        flags,
                        2048, // max_width,
                        L' ',  // def_char
                        0
                     );
        if ( res>0 && res<MAX_LINE_CHARS )
            return widths[res-1];
        return 0;
    }

    void updateTransform() {
//        static void * transformOwner = NULL;
//        if ( transformOwner!=this ) {
//            FT_Set_Transform(_face, &_matrix, NULL);
//            transformOwner = this;
//        }
    }

    /** \brief get glyph item
        \param code is unicode character
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem * getGlyph(lUInt16 ch, lChar16 def_char=0) {
        //FONT_GUARD
        FT_UInt ch_glyph_index = getCharIndex( ch, 0 );
        if ( ch_glyph_index==0 ) {
            LVFont * fallback = getFallbackFont();
            if ( !fallback ) {
                // No fallback
                ch_glyph_index = getCharIndex( ch, def_char );
                if ( ch_glyph_index==0 )
                    return NULL;
            } else {
                // Fallback
                return fallback->getGlyph(ch, def_char);
            }
        }
        LVFontGlyphCacheItem * item = _glyph_cache.get( ch );
        if ( !item ) {

            int rend_flags = FT_LOAD_RENDER | ( !_drawMonochrome ? FT_LOAD_TARGET_NORMAL : (FT_LOAD_TARGET_MONO) ); //|FT_LOAD_MONOCHROME|FT_LOAD_FORCE_AUTOHINT
            if (_hintingMode == HINTING_MODE_AUTOHINT)
                rend_flags |= FT_LOAD_FORCE_AUTOHINT;
            else if (_hintingMode == HINTING_MODE_DISABLED)
                rend_flags |= FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING;
            /* load glyph image into the slot (erase previous one) */

            updateTransform();
            int error = FT_Load_Glyph( _face,          /* handle to face object */
                    ch_glyph_index,                /* glyph index           */
                    rend_flags );             /* load flags, see below */
            if ( error ) {
                return NULL;  /* ignore errors */
            }
            item = newItem( &_glyph_cache, ch, _slot ); //, _drawMonochrome
            _glyph_cache.put( item );
        }
        return item;
    }

//    /** \brief get glyph image in 1 byte per pixel format
//        \param code is unicode character
//        \param buf is buffer [width*height] to place glyph data
//        \return true if glyph was found
//    */
//    virtual bool getGlyphImage(lUInt16 ch, lUInt8 * bmp, lChar16 def_char=0)
//    {
//        LVFontGlyphCacheItem * item = getGlyph(ch);
//        if ( item )
//            memcpy( bmp, item->bmp, item->bmp_width * item->bmp_height );
//        return item;
//    }

    /// returns font baseline offset
    virtual int getBaseline()
    {
        return _baseline;
    }

    /// returns font height
    virtual int getHeight() const
    {
        return _height;
    }

    /// returns font character size
    virtual int getSize() const
    {
        return _size;
    }

    /// returns char width
    virtual int getCharWidth( lChar16 ch, lChar16 def_char='?' )
    {
        int w = _wcache.get(ch);
        if ( w==0xFF ) {
            glyph_info_t glyph;
            if ( getGlyphInfo( ch, &glyph, def_char ) ) {
                w = glyph.width;
            } else {
                w = 0;
            }
            _wcache.put(ch, w);
        }
        return w;
    }

    /// retrieves font handle
    virtual void * GetHandle()
    {
        return NULL;
    }

    /// returns font typeface name
    virtual lString8 getTypeFace() const
    {
        return _faceName;
    }

    /// returns font family id
    virtual css_font_family_t getFontFamily() const
    {
        return _fontFamily;
    }

    virtual bool kerningEnabled() {
		#if (ALLOW_KERNING==1)
        	return _allowKerning && FT_HAS_KERNING( _face );
		#else
        	return false;
		#endif
    }

    virtual int getKerningOffset(lChar16 ch1, lChar16 ch2, lChar16 def_char) {
		#if (ALLOW_KERNING==1)
    		FT_UInt ch_glyph_index1 = getCharIndex( ch1, def_char );
			FT_UInt ch_glyph_index2 = getCharIndex( ch2, def_char );
            if (ch_glyph_index1 > 0 && ch_glyph_index2 > 0) {
                FT_Vector delta;
                int error = FT_Get_Kerning(
                		_face,          /* handle to face object */
                		ch_glyph_index1,          /* left glyph index      */
                		ch_glyph_index2,         /* right glyph index     */
                        FT_KERNING_DEFAULT,  /* kerning mode          */
                        &delta );    /* target vector         */
                if ( !error )
                    return delta.x;
            }
		#endif
        return 0;
    }

    /// draws text string
    virtual void DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar16 * text, int len,
                       lChar16 def_char, lUInt32 * palette, bool addHyphen, lUInt32 flags, int letter_spacing )
    {
        if ( len <= 0 || _face==NULL )
            return;
        if ( letter_spacing<0 || letter_spacing>50 )
            letter_spacing = 0;
        lvRect clip;
        buf->GetClipRect( &clip );
        updateTransform();
        if ( y + _height < clip.top || y >= clip.bottom )
            return;

        int error;

#if (ALLOW_KERNING==1)
        int use_kerning = _allowKerning && FT_HAS_KERNING( _face );
#endif
        int i;

        FT_UInt previous = 0;
        //lUInt16 prev_width = 0;
        lChar16 ch;
        // measure character widths
        bool isHyphen = false;
        int x0 = x;
        for ( i=0; i<=len; i++) {
            if ( i==len && (!addHyphen || isHyphen) )
                break;
            if ( i<len ) {
                ch = text[i];
                if ( ch=='\t' )
                    ch = ' ';
                isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE) && (i<len-1);
            } else {
                ch = UNICODE_SOFT_HYPHEN_CODE;
                isHyphen = 0;
            }
            FT_UInt ch_glyph_index = getCharIndex( ch, def_char );
            int kerning = 0;
#if (ALLOW_KERNING==1)
            if ( use_kerning && previous>0 && ch_glyph_index>0 ) {
                FT_Vector delta;
                error = FT_Get_Kerning( _face,          /* handle to face object */
                              previous,          /* left glyph index      */
                              ch_glyph_index,         /* right glyph index     */
                              FT_KERNING_DEFAULT,  /* kerning mode          */
                              &delta );    /* target vector         */
                if ( !error )
                    kerning = delta.x;
            }
#endif


            LVFontGlyphCacheItem * item = getGlyph(ch, def_char);
                    _glyph_cache.get( ch );
            if ( !item )
                continue;
            if ( (item && !isHyphen) || i>=len-1 ) { // avoid soft hyphens inside text string
                int w = item->advance + (kerning >> 6);
                buf->Draw( x + (kerning>>6) + item->origin_x,
                    y + _baseline - item->origin_y,
                    item->bmp,
                    item->bmp_width,
                    item->bmp_height,
                    palette);

                x  += w + letter_spacing;
                previous = ch_glyph_index;
            }
        }
        if ( flags & LTEXT_TD_MASK ) {
            // text decoration: underline, etc.
            int h = _size > 30 ? 2 : 1;
            lUInt32 cl = buf->GetTextColor();
            if ( (flags & LTEXT_TD_UNDERLINE) || (flags & LTEXT_TD_BLINK) ) {
                int liney = y + _baseline + h;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
            if ( flags & LTEXT_TD_OVERLINE ) {
                int liney = y + h;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
            if ( flags & LTEXT_TD_LINE_THROUGH ) {
//                int liney = y + _baseline - _size/4 - h/2;
                int liney = y + _baseline - _size*2/7;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
        }
    }

    /// returns true if font is empty
    virtual bool IsNull() const
    {
        return _face == NULL;
    }

    virtual bool operator ! () const
    {
        return _face == NULL;
    }

    virtual void Clear()
    {
        if ( _face )
            FT_Done_Face( _face );
        _face = NULL;
    }

};

class LVFontBoldTransform : public LVFont
{
    LVFontRef _baseFontRef;
    LVFont * _baseFont;
    int _hyphWidth;
    int _hShift;
    int _vShift;
    int           _size;   // glyph height in pixels
    int           _height; // line height in pixels
    //int           _hyphen_width;
    int           _baseline;
    LVFontLocalGlyphCache _glyph_cache;
public:
    /// returns font weight
    virtual int getWeight() const
    {
        int w = _baseFont->getWeight() + 200;
        if ( w>900 )
            w = 900;
        return w;
    }
    /// returns italic flag
    virtual int getItalic() const
    {
        return _baseFont->getItalic();
    }
    LVFontBoldTransform( LVFontRef baseFont, LVFontGlobalGlyphCache * globalCache )
        : _baseFontRef( baseFont ), _baseFont( baseFont.get() ), _hyphWidth(-1), _glyph_cache(globalCache)
    {
        _size = _baseFont->getSize();
        _height = _baseFont->getHeight();
        _hShift = _size <= 36 ? 1 : 2;
        _vShift = _size <= 36 ? 0 : 1;
        _baseline = _baseFont->getBaseline();
    }

    /// hyphenation character
    virtual lChar16 getHyphChar() { return UNICODE_SOFT_HYPHEN_CODE; }

    /// hyphen width
    virtual int getHyphenWidth() {
        if ( _hyphWidth<0 )
            _hyphWidth = getCharWidth( getHyphChar() );
        return _hyphWidth;
    }

    /** \brief get glyph info
        \param glyph is pointer to glyph_info_t struct to place retrieved info
        \return true if glyh was found
    */
    virtual bool getGlyphInfo( lUInt16 code, glyph_info_t * glyph, lChar16 def_char=0  )
    {
        bool res = _baseFont->getGlyphInfo( code, glyph, def_char );
        if ( !res )
            return res;
        glyph->blackBoxX += glyph->blackBoxX>0 ? _hShift : 0;
        glyph->blackBoxY += _vShift;
        glyph->width += _hShift;

        return true;
    }

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \param max_width is maximum width to measure line
        \param def_char is character to replace absent glyphs in font
        \param letter_spacing is number of pixels to add between letters
        \return number of characters before max_width reached
    */
    virtual lUInt16 measureText(
                        const lChar16 * text, int len,
                        lUInt16 * widths,
                        lUInt8 * flags,
                        int max_width,
                        lChar16 def_char,
                        int letter_spacing=0,
                        bool allow_hyphenation=true
                     )
    {
        CR_UNUSED(allow_hyphenation);
        lUInt16 res = _baseFont->measureText(
                        text, len,
                        widths,
                        flags,
                        max_width,
                        def_char,
                        letter_spacing
                     );
        int w = 0;
        for ( int i=0; i<res; i++ ) {
            w += _hShift;
            widths[i] += w;
        }
        return res;
    }

    /** \brief measure text
        \param text is text string pointer
        \param len is number of characters to measure
        \return width of specified string
    */
    virtual lUInt32 getTextWidth(
                        const lChar16 * text, int len
        )
    {
        static lUInt16 widths[MAX_LINE_CHARS+1];
        static lUInt8 flags[MAX_LINE_CHARS+1];
        if ( len>MAX_LINE_CHARS )
            len = MAX_LINE_CHARS;
        if ( len<=0 )
            return 0;
        lUInt16 res = measureText(
                        text, len,
                        widths,
                        flags,
                        2048, // max_width,
                        L' ',  // def_char
                        0
                     );
        if ( res>0 && res<MAX_LINE_CHARS )
            return widths[res-1];
        return 0;
    }

    /** \brief get glyph item
        \param code is unicode character
        \return glyph pointer if glyph was found, NULL otherwise
    */
    virtual LVFontGlyphCacheItem * getGlyph(lUInt16 ch, lChar16 def_char=0) {

        LVFontGlyphCacheItem * item = _glyph_cache.get( ch );
        if ( item )
            return item;

        LVFontGlyphCacheItem * olditem = _baseFont->getGlyph( ch, def_char );
        if ( !olditem )
            return NULL;

        int oldx = olditem->bmp_width;
        int oldy = olditem->bmp_height;
        int dx = oldx ? oldx + _hShift : 0;
        int dy = oldy ? oldy + _vShift : 0;

        item = LVFontGlyphCacheItem::newItem( &_glyph_cache, ch, dx, dy ); //, _drawMonochrome
        item->advance = olditem->advance + _hShift;
        item->origin_x = olditem->origin_x;
        item->origin_y = olditem->origin_y;

        if ( dx && dy ) {
            for ( int y=0; y<dy; y++ ) {
                lUInt8 * dst = item->bmp + y*dx;
                for ( int x=0; x<dx; x++ ) {
                    int s = 0;
                    for ( int yy=-_vShift; yy<=0; yy++ ) {
                        int srcy = y+yy;
                        if ( srcy<0 || srcy>=oldy )
                            continue;
                        lUInt8 * src = olditem->bmp + srcy*oldx;
                        for ( int xx=-_hShift; xx<=0; xx++ ) {
                            int srcx = x+xx;
                            if ( srcx>=0 && srcx<oldx && src[srcx] > s )
                                s = src[srcx];
                        }
                    }
                    dst[x] = s;
                }
            }
        }
        _glyph_cache.put( item );
        return item;
    }

    /** \brief get glyph image in 1 byte per pixel format
        \param code is unicode character
        \param buf is buffer [width*height] to place glyph data
        \return true if glyph was found
    */
//    virtual bool getGlyphImage(lUInt16 code, lUInt8 * buf, lChar16 def_char=0 )
//    {
//        LVFontGlyphCacheItem * item = getGlyph( code, def_char );
//        if ( !item )
//            return false;
//        glyph_info_t glyph;
//        if ( !_baseFont->getGlyphInfo( code, &glyph, def_char ) )
//            return 0;
//        int oldx = glyph.blackBoxX;
//        int oldy = glyph.blackBoxY;
//        int dx = oldx + _hShift;
//        int dy = oldy + _vShift;
//        if ( !oldx || !oldy )
//            return true;
//        LVAutoPtr<lUInt8> tmp( new lUInt8[oldx*oldy+2000] );
//        memset(buf, 0, dx*dy);
//        tmp[oldx*oldy]=123;
//        bool res = _baseFont->getGlyphImage( code, tmp.get(), def_char );
//        if ( tmp[oldx*oldy]!=123 ) {
//            //CRLog::error("Glyph buffer corrupted!");
//            // clear cache
//            for ( int i=32; i<4000; i++ ) {
//                _baseFont->getGlyphInfo( i, &glyph, def_char );
//                _baseFont->getGlyphImage( i, tmp.get(), def_char );
//            }
//            _baseFont->getGlyphInfo( code, &glyph, def_char );
//            _baseFont->getGlyphImage( code, tmp.get(), def_char );
//        }
//        for ( int y=0; y<dy; y++ ) {
//            lUInt8 * dst = buf + y*dx;
//            for ( int x=0; x<dx; x++ ) {
//                int s = 0;
//                for ( int yy=-_vShift; yy<=0; yy++ ) {
//                    int srcy = y+yy;
//                    if ( srcy<0 || srcy>=oldy )
//                        continue;
//                    lUInt8 * src = tmp.get() + srcy*oldx;
//                    for ( int xx=-_hShift; xx<=0; xx++ ) {
//                        int srcx = x+xx;
//                        if ( srcx>=0 && srcx<oldx && src[srcx] > s )
//                            s = src[srcx];
//                    }
//                }
//                dst[x] = s;
//            }
//        }
//        return res;
//        return false;
//    }

    /// returns font baseline offset
    virtual int getBaseline()
    {
        return _baseline;
    }

    /// returns font height
    virtual int getHeight() const
    {
        return _height;
    }

    /// returns font character size
    virtual int getSize() const
    {
        return _size;
    }

    /// returns char width
    virtual int getCharWidth( lChar16 ch, lChar16 def_char=0 )
    {
        int w = _baseFont->getCharWidth( ch, def_char ) + _hShift;
        return w;
    }

    /// retrieves font handle
    virtual void * GetHandle()
    {
        return NULL;
    }

    /// returns font typeface name
    virtual lString8 getTypeFace() const
    {
        return _baseFont->getTypeFace();
    }

    /// returns font family id
    virtual css_font_family_t getFontFamily() const
    {
        return _baseFont->getFontFamily();
    }

    /// draws text string
    virtual void DrawTextString( LVDrawBuf * buf, int x, int y,
                       const lChar16 * text, int len,
                       lChar16 def_char, lUInt32 * palette, bool addHyphen,
                       lUInt32 flags=0, int letter_spacing=0 )
    {
        if ( len <= 0 )
            return;
        if ( letter_spacing<0 || letter_spacing>50 )
            letter_spacing = 0;
        lvRect clip;
        buf->GetClipRect( &clip );
        if ( y + _height < clip.top || y >= clip.bottom )
            return;

        //int error;

        int i;

        //lUInt16 prev_width = 0;
        lChar16 ch;
        // measure character widths
        bool isHyphen = false;
        int x0 = x;
        for ( i=0; i<=len; i++) {
            if ( i==len && (!addHyphen || isHyphen) )
                break;
            if ( i<len ) {
                ch = text[i];
                isHyphen = (ch==UNICODE_SOFT_HYPHEN_CODE) && (i<len-1);
            } else {
                ch = UNICODE_SOFT_HYPHEN_CODE;
                isHyphen = 0;
            }

            LVFontGlyphCacheItem * item = getGlyph(ch, def_char);
            int w  = 0;
            if ( item ) {
                // avoid soft hyphens inside text string
                w = item->advance;
                if ( item->bmp_width && item->bmp_height && (!isHyphen || i>=len-1) ) {
                    buf->Draw( x + item->origin_x,
                        y + _baseline - item->origin_y,
                        item->bmp,
                        item->bmp_width,
                        item->bmp_height,
                        palette);
                }
            }
            x  += w + letter_spacing;
        }
        if ( flags & LTEXT_TD_MASK ) {
            // text decoration: underline, etc.
            int h = _size > 30 ? 2 : 1;
            lUInt32 cl = buf->GetTextColor();
            if ( (flags & LTEXT_TD_UNDERLINE) || (flags & LTEXT_TD_BLINK) ) {
                int liney = y + _baseline + h;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
            if ( flags & LTEXT_TD_OVERLINE ) {
                int liney = y + h;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
            if ( flags & LTEXT_TD_LINE_THROUGH ) {
                int liney = y + _height/2 - h/2;
                buf->FillRect( x0, liney, x, liney+h, cl );
            }
        }
    }

    /// get bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual bool getBitmapMode()
    {
        return _baseFont->getBitmapMode();
    }

    /// set bitmap mode (true=monochrome bitmap, false=antialiased)
    virtual void setBitmapMode( bool m )
    {
        _baseFont->setBitmapMode( m );
    }

    /// sets current hinting mode
    virtual void setHintingMode(hinting_mode_t mode) { _baseFont->setHintingMode(mode); }
    /// returns current hinting mode
    virtual hinting_mode_t  getHintingMode() const { return _baseFont->getHintingMode(); }

    /// get kerning mode: true==ON, false=OFF
    virtual bool getKerning() const { return _baseFont->getKerning(); }

    /// get kerning mode: true==ON, false=OFF
    virtual void setKerning( bool b ) { _baseFont->setKerning( b ); }

    /// returns true if font is empty
    virtual bool IsNull() const
    {
        return _baseFont->IsNull();
    }

    virtual bool operator ! () const
    {
        return !(*_baseFont);
    }
    virtual void Clear()
    {
        _baseFont->Clear();
    }
    virtual ~LVFontBoldTransform()
    {
    }
};

/// create transform for font
//LVFontRef LVCreateFontTransform( LVFontRef baseFont, int transformFlags )
//{
//    if ( transformFlags & LVFONT_TRANSFORM_EMBOLDEN ) {
//        // BOLD transform
//        return LVFontRef( new LVFontBoldTransform( baseFont ) );
//    } else {
//        return baseFont; // no transform
//    }
//}

#if (DEBUG_FONT_SYNTHESIS==1)
static LVFontRef dumpFontRef( LVFontRef fnt ) {
    CRLog::trace("%s %d (%d) w=%d %s", fnt->getTypeFace().c_str(), fnt->getSize(), fnt->getHeight(), fnt->getWeight(), fnt->getItalic()?"italic":"" );
    return fnt;
}
#endif

class LVFreeTypeFontManager : public LVFontManager
{
private:
    lString8    _path;
    lString8    _fallbackFontFace;
    LVFontCache _cache;
    FT_Library  _library;
    LVFontGlobalGlyphCache _globalCache;
    lString16 _requiredChars;
    #if (DEBUG_FONT_MAN==1)
    FILE * _log;
    #endif
public:

    /// get hash of installed fonts and fallback font
    virtual lUInt32 GetFontListHash(int documentId) {
        return _cache.GetFontListHash(documentId) * 75 + _fallbackFontFace.getHash();
    }

    /// set fallback font
    virtual bool SetFallbackFontFace( lString8 face ) {
        if ( face!=_fallbackFontFace ) {
            _cache.clearFallbackFonts();
            CRLog::trace("Looking for fallback font %s", face.c_str());
            LVFontCacheItem * item = _cache.findFallback( face, -1 );
            if (!item)
                face.clear();
            _fallbackFontFace = face;
        }
        return !_fallbackFontFace.empty();
    }

    /// get fallback font face (returns empty string if no fallback font is set)
    virtual lString8 GetFallbackFontFace() { return _fallbackFontFace; }

    /// returns fallback font for specified size
    virtual LVFontRef GetFallbackFont(int size) {
        if ( _fallbackFontFace.empty() )
            return LVFontRef();
        // reduce number of possible distinct sizes for fallback font
        if ( size>40 )
            size &= 0xFFF8;
        else if ( size>28 )
            size &= 0xFFFC;
        else if ( size>16 )
            size &= 0xFFFE;
        LVFontCacheItem * item = _cache.findFallback( _fallbackFontFace, size );
        if ( !item->getFont().isNull() )
            return item->getFont();
        return GetFont(size, 400, false, css_ff_sans_serif, _fallbackFontFace, -1);
    }

    bool isBitmapModeForSize( int size )
    {
        bool bitmap = false;
        switch ( _antialiasMode ) {
        case font_aa_none:
            bitmap = true;
            break;
        case font_aa_big:
            bitmap = size<20 ? true : false;
            break;
        case font_aa_all:
        default:
            bitmap = false;
            break;
        }
        return bitmap;
    }

    /// set antialiasing mode
    virtual void SetAntialiasMode( int mode )
    {
        _antialiasMode = mode;
        gc();
        clearGlyphCache();
        LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
        for ( int i=0; i<fonts->length(); i++ ) {
            fonts->get(i)->getFont()->setBitmapMode( isBitmapModeForSize( fonts->get(i)->getFont()->getHeight() ) );
        }
    }

    /// sets current gamma level
    virtual void SetHintingMode(hinting_mode_t mode) {
        if (_hintingMode == mode)
            return;
        CRLog::trace("Hinting mode is changed: %d", (int)mode);
        _hintingMode = mode;
        gc();
        clearGlyphCache();
        LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
        for ( int i=0; i<fonts->length(); i++ ) {
            fonts->get(i)->getFont()->setHintingMode(mode);
        }
    }

    /// sets current gamma level
    virtual hinting_mode_t  GetHintingMode() {
        return _hintingMode;
    }

    /// set antialiasing mode
    virtual void setKerning( bool kerning )
    {
        _allowKerning = kerning;
        gc();
        clearGlyphCache();
        LVPtrVector< LVFontCacheItem > * fonts = _cache.getInstances();
        for ( int i=0; i<fonts->length(); i++ ) {
            fonts->get(i)->getFont()->setKerning( kerning );
        }
    }
    /// clear glyph cache
    virtual void clearGlyphCache()
    {
        _globalCache.clear();
    }

    virtual int GetFontCount()
    {
        return _cache.length();
    }

    bool initSystemFonts()
    {
        #if (DEBUG_FONT_SYNTHESIS==1)
            fontMan->RegisterFont(lString8("/usr/share/fonts/liberation/LiberationSans-Regular.ttf"));
            CRLog::trace("fonts:");
            LVFontRef fnt4 = dumpFontRef( fontMan->GetFont(24, 200, true, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
            LVFontRef fnt1 = dumpFontRef( fontMan->GetFont(18, 200, false, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
            LVFontRef fnt2 = dumpFontRef( fontMan->GetFont(20, 400, false, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
            LVFontRef fnt3 = dumpFontRef( fontMan->GetFont(22, 600, false, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
            LVFontRef fnt5 = dumpFontRef( fontMan->GetFont(26, 400, true, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
            LVFontRef fnt6 = dumpFontRef( fontMan->GetFont(28, 600, true, css_ff_sans_serif, cs8("Arial, Helvetica") ) );
            CRLog::trace("end of font testing");
        #elif (USE_FONTCONFIG==1)
        {
            CRLog::trace("Reading list of system fonts using FONTCONFIG");
            lString16Collection fonts;

            int facesFound = 0;

            FcFontSet *fontset;

            FcObjectSet *os = FcObjectSetBuild(FC_FILE, FC_WEIGHT, FC_FAMILY,
                                               FC_SLANT, FC_SPACING, FC_INDEX,
                                               FC_STYLE, NULL);
            FcPattern *pat = FcPatternCreate();
            //FcBool b = 1;
            FcPatternAddBool(pat, FC_SCALABLE, 1);

            fontset = FcFontList(NULL, pat, os);

            FcPatternDestroy(pat);
            FcObjectSetDestroy(os);

            // load fonts from file
            CRLog::trace("FONTCONFIG: %d font files found", fontset->nfont);
            for(int i = 0; i < fontset->nfont; i++) {
                FcChar8 *s=(FcChar8*)"";
                FcChar8 *family=(FcChar8*)"";
                FcChar8 *style=(FcChar8*)"";
                //FcBool b;
                FcResult res;
                //FC_SCALABLE
                //res = FcPatternGetBool( fontset->fonts[i], FC_OUTLINE, 0, (FcBool*)&b);
                //if(res != FcResultMatch)
                //    continue;
                //if ( !b )
                //    continue; // skip non-scalable fonts
                res = FcPatternGetString(fontset->fonts[i], FC_FILE, 0, (FcChar8 **)&s);
                if(res != FcResultMatch) {
                    continue;
                }
                lString8 fn( (const char *)s );
                lString16 fn16( fn.c_str() );
                fn16.lowercase();
                if (!fn16.endsWith(".ttf") && !fn16.endsWith(".odf") && !fn16.endsWith(".otf") && !fn16.endsWith(".pfb") && !fn16.endsWith(".pfa")  ) {
                    continue;
                }
                int weight = FC_WEIGHT_MEDIUM;
                res = FcPatternGetInteger(fontset->fonts[i], FC_WEIGHT, 0, &weight);
                if(res != FcResultMatch) {
                    CRLog::trace("no FC_WEIGHT for %s", s);
                    //continue;
                }
                switch ( weight ) {
                case FC_WEIGHT_THIN:          //    0
                    weight = 100;
                    break;
                case FC_WEIGHT_EXTRALIGHT:    //    40
                //case FC_WEIGHT_ULTRALIGHT        FC_WEIGHT_EXTRALIGHT
                    weight = 200;
                    break;
                case FC_WEIGHT_LIGHT:         //    50
                case FC_WEIGHT_BOOK:          //    75
                case FC_WEIGHT_REGULAR:       //    80
                //case FC_WEIGHT_NORMAL:            FC_WEIGHT_REGULAR
                    weight = 400;
                    break;
                case FC_WEIGHT_MEDIUM:        //    100
                    weight = 500;
                    break;
                case FC_WEIGHT_DEMIBOLD:      //    180
                //case FC_WEIGHT_SEMIBOLD:          FC_WEIGHT_DEMIBOLD
                    weight = 600;
                    break;
                case FC_WEIGHT_BOLD:          //    200
                    weight = 700;
                    break;
                case FC_WEIGHT_EXTRABOLD:     //    205
                //case FC_WEIGHT_ULTRABOLD:         FC_WEIGHT_EXTRABOLD
                    weight = 800;
                    break;
                case FC_WEIGHT_BLACK:         //    210
                //case FC_WEIGHT_HEAVY:             FC_WEIGHT_BLACK
                    weight = 900;
                    break;
#ifdef FC_WEIGHT_EXTRABLACK
                case FC_WEIGHT_EXTRABLACK:    //    215
                //case FC_WEIGHT_ULTRABLACK:        FC_WEIGHT_EXTRABLACK
                    weight = 900;
                    break;
#endif
                default:
                    weight = 400;
                    break;
                }
                FcBool scalable = 0;
                res = FcPatternGetBool(fontset->fonts[i], FC_SCALABLE, 0, &scalable);
                int index = 0;
                res = FcPatternGetInteger(fontset->fonts[i], FC_INDEX, 0, &index);
                if(res != FcResultMatch) {
                    CRLog::trace("no FC_INDEX for %s", s);
                    //continue;
                }
                res = FcPatternGetString(fontset->fonts[i], FC_FAMILY, 0, (FcChar8 **)&family);
                if(res != FcResultMatch) {
                    CRLog::trace("no FC_FAMILY for %s", s);
                    continue;
                }
                res = FcPatternGetString(fontset->fonts[i], FC_STYLE, 0, (FcChar8 **)&style);
                if(res != FcResultMatch) {
                    CRLog::trace("no FC_STYLE for %s", s);
                    style = (FcChar8*)"";
                    //continue;
                }
                int slant = FC_SLANT_ROMAN;
                res = FcPatternGetInteger(fontset->fonts[i], FC_SLANT, 0, &slant);
                if(res != FcResultMatch) {
                    CRLog::trace("no FC_SLANT for %s", s);
                    //continue;
                }
                int spacing = 0;
                res = FcPatternGetInteger(fontset->fonts[i], FC_SPACING, 0, &spacing);
                if(res != FcResultMatch) {
                    //CRLog::trace("no FC_SPACING for %s", s);
                    //continue;
                }
//                int cr_weight;
//                switch(weight) {
//                    case FC_WEIGHT_LIGHT: cr_weight = 200; break;
//                    case FC_WEIGHT_MEDIUM: cr_weight = 300; break;
//                    case FC_WEIGHT_DEMIBOLD: cr_weight = 500; break;
//                    case FC_WEIGHT_BOLD: cr_weight = 700; break;
//                    case FC_WEIGHT_BLACK: cr_weight = 800; break;
//                    default: cr_weight=300; break;
//                }
                css_font_family_t fontFamily = css_ff_sans_serif;
                lString16 face16((const char *)family);
                face16.lowercase();
                if ( spacing==FC_MONO )
                    fontFamily = css_ff_monospace;
                else if (face16.pos("sans") >= 0)
                    fontFamily = css_ff_sans_serif;
                else if (face16.pos("serif") >= 0)
                    fontFamily = css_ff_serif;

                //css_ff_inherit,
                //css_ff_serif,
                //css_ff_sans_serif,
                //css_ff_cursive,
                //css_ff_fantasy,
                //css_ff_monospace,
                bool italic = (slant!=FC_SLANT_ROMAN);

                lString8 face((const char*)family);
                lString16 style16((const char*)style);
                style16.lowercase();
                if (style16.pos("condensed") >= 0)
                    face << " Condensed";
                else if (style16.pos("extralight") >= 0)
                    face << " Extra Light";

                LVFontDef def(
                    lString8((const char*)s),
                    -1, // height==-1 for scalable fonts
                    weight,
                    italic,
                    fontFamily,
                    face,
                    index
                );

                CRLog::trace("FONTCONFIG: Font family:%s style:%s weight:%d slant:%d spacing:%d file:%s", family, style, weight, slant, spacing, s);
                if ( _cache.findDuplicate( &def ) ) {
                    CRLog::trace("is duplicate, skipping");
                    continue;
                }
                _cache.update( &def, LVFontRef(NULL) );

                if ( scalable && !def.getItalic() ) {
                    LVFontDef newDef( def );
                    newDef.setItalic(2); // can italicize
                    if ( !_cache.findDuplicate( &newDef ) )
                        _cache.update( &newDef, LVFontRef(NULL) );
                }

                facesFound++;


            }

            FcFontSetDestroy(fontset);
            CRLog::trace("FONTCONFIG: %d fonts registered", facesFound);

            const char * fallback_faces [] = {
                "Arial Unicode MS",
                "AR PL ShanHeiSun Uni",
                "Liberation Sans",
                NULL
            };

            for ( int i=0; fallback_faces[i]; i++ )
                if ( SetFallbackFontFace(lString8(fallback_faces[i])) ) {
                    CRLog::trace("Fallback font %s is found", fallback_faces[i]);
                    break;
                } else {
                    CRLog::trace("Fallback font %s is not found", fallback_faces[i]);
                }

            return facesFound > 0;
        }
        #else
        return false;
        #endif
    }

    virtual ~LVFreeTypeFontManager()
    {
        _globalCache.clear();
        _cache.clear();
        if ( _library )
            FT_Done_FreeType( _library );
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            fclose(_log);
        }
    #endif
    }

    LVFreeTypeFontManager()
    : _library(NULL), _globalCache(GLYPH_CACHE_SIZE)
    {
        int error = FT_Init_FreeType( &_library );
        if ( error ) {
            // error
        	CRLog::error("Error while initializing freetype library");
        }
    #if (DEBUG_FONT_MAN==1)
        _log = fopen(DEBUG_FONT_MAN_LOG_FILE, "at");
        if ( _log ) {
            fprintf(_log, "=========================== LOGGING STARTED ===================\n");
        }
    #endif
        _requiredChars = L"azAZ09";//\x0410\x042F\x0430\x044F";
    }

    virtual void gc() // garbage collector
    {
        _cache.gc();
    }

    lString8 makeFontFileName( lString8 name )
    {
        lString8 filename = _path;
        if (!filename.empty() && _path[_path.length()-1]!=PATH_SEPARATOR_CHAR)
            filename << PATH_SEPARATOR_CHAR;
        filename << name;
        return filename;
    }

    /// returns available typefaces
    virtual void getFaceList( lString16Collection & list )
    {
        _cache.getFaceList( list );
    }

    bool setalias(lString8 alias,lString8 facename,int id,bool italic, bool bold)
    {
        lString8 fontname=lString8("\0");
        LVFontDef def(
                fontname,
                10,
                400,
                true,
                css_ff_inherit,
                facename,
                -1,
                id
        );
        LVFontCacheItem * item = _cache.find( &def);
        LVFontDef def1(
                fontname,
                10,
                400,
                false,
                css_ff_inherit,
                alias,
                -1,
                id
        );
        if (!item->getDef()->getName().empty()) {
            _cache.removefont(&def1);
            /*def.setTypeFace(alias);
            def.setName(item->getDef()->getName());
            def.setItalic(1);
            LVFontDef newDef(*item->getDef());
            newDef.setTypeFace(alias);
            LVFontRef ref = item->getFont();
            _cache.update(&newDef, ref);*/
            int index = 0;

            FT_Face face = NULL;

            // for all faces in file
            for ( ;; index++ ) {
                int error = FT_New_Face( _library, item->getDef()->getName().c_str(), index, &face ); /* create face object */
                if ( error ) {
                    if (index == 0) {
                        CRLog::error("FT_New_Face returned error %d", error);
                    }
                    break;
                }
                //bool scal = FT_IS_SCALABLE( face );
                //bool charset = checkCharSet( face );

                int num_faces = face->num_faces;

                css_font_family_t fontFamily = css_ff_sans_serif;
                if ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
                    fontFamily = css_ff_monospace;
                lString8 familyName(!facename.empty() ? facename : ::familyName(face));
                if ( familyName=="Times" || familyName=="Times New Roman" )
                    fontFamily = css_ff_serif;

                bool boldFlag = !facename.empty() ? bold : (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
                bool italicFlag = !facename.empty() ? italic : (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

                LVFontDef def2(
                        item->getDef()->getName(),
                        -1, // height==-1 for scalable fonts
                        boldFlag ? 700 : 400,
                        italicFlag,
                        fontFamily,
                        alias,
                        index,
                        id
                );

                if ( face ) {
                    FT_Done_Face( face );
                    face = NULL;
                }

                if ( _cache.findDuplicate( &def2 ) ) {
                    CRLog::trace("font definition is duplicate");
                    return false;
                }
                _cache.update( &def2, LVFontRef(NULL) );
                if (!def.getItalic()) {
                    LVFontDef newDef( def2 );
                    newDef.setItalic(2); // can italicize
                    if ( !_cache.findDuplicate( &newDef ) )
                        _cache.update( &newDef, LVFontRef(NULL) );
                }
                if ( index>=num_faces-1 )
                    break;
            }
            return true;
        } else {
            return false;
        }
    }

    virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface, int documentId)
    {
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
             fprintf(_log, "GetFont(size=%d, weight=%d, italic=%d, family=%d, typeface='%s')\n",
                size, weight, italic?1:0, (int)family, typeface.c_str() );
        }
    #endif
        lString8 fontname;
        LVFontDef def(
            fontname,
            size,
            weight,
            italic,
            family,
            typeface,
            -1,
            documentId
        );
    #if (DEBUG_FONT_MAN==1)
        if ( _log )
            fprintf( _log, "GetFont: %s %d %s %s\n",
                typeface.c_str(),
                size,
                weight>400?"bold":"",
                italic?"italic":"" );
    #endif
        LVFontCacheItem * item = _cache.find( &def );
    #if (DEBUG_FONT_MAN==1)
        if ( item && _log ) { //_log &&
            fprintf(_log, "   found item: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s, weightDelta=%d) FontRef=%d\n",
                item->getDef()->getName().c_str(), item->getDef()->getIndex(), item->getDef()->getSize(), item->getDef()->getWeight(), item->getDef()->getItalic()?1:0,
                (int)item->getDef()->getFamily(), item->getDef()->getTypeFace().c_str(),
                weight - item->getDef()->getWeight(), item->getFont().isNull()?0:item->getFont()->getHeight()
            );
        }
    #endif
        bool italicize = false;

        LVFontDef newDef(*item->getDef());

        if (!item->getFont().isNull())
        {
            int deltaWeight = weight - item->getDef()->getWeight();
            if ( deltaWeight >= 200 ) {
                // embolden
                CRLog::trace("font: apply Embolding to increase weight from %d to %d", newDef.getWeight(), newDef.getWeight() + 200 );
                newDef.setWeight( newDef.getWeight() + 200 );
                LVFontRef ref = LVFontRef( new LVFontBoldTransform( item->getFont(), &_globalCache ) );
                _cache.update( &newDef, ref );
                return ref;
            } else {
                //fprintf(_log, "    : fount existing\n");
                return item->getFont();
            }
        }
        lString8 fname = item->getDef()->getName();
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            int index = item->getDef()->getIndex();
            fprintf(_log, "   no instance: adding new one for filename=%s, index = %d\n", fname.c_str(), index );
        }
    #endif
        LVFreeTypeFace* font = new LVFreeTypeFace(_library, &_globalCache);
        lString8 pathname = makeFontFileName( fname );
        //def.setName( fname );
        //def.setIndex( index );

        //if ( fname.empty() || pathname.empty() ) {
        //    pathname = lString8("arial.ttf");
        //}

        if ( !item->getDef()->isRealItalic() && italic ) {
            //CRLog::trace("font: fake italic");
            newDef.setItalic(true);
            italicize = true;
        }

        //printf("going to load font file %s\n", fname.c_str());
        bool loaded = false;
        if (item->getDef()->getBuf().isNull())
            loaded = font->loadFromFile( pathname.c_str(), item->getDef()->getIndex(), size, family, isBitmapModeForSize(size), italicize );
        else
            loaded = font->loadFromBuffer(item->getDef()->getBuf(), item->getDef()->getIndex(), size, family, isBitmapModeForSize(size), italicize );
        if (loaded) {
            //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
            //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
            LVFontRef ref(font);
            font->setKerning( getKerning() );
            font->setFaceName( item->getDef()->getTypeFace() );
            newDef.setSize( size );
            //item->setFont( ref );
            //_cache.update( def, ref );
            _cache.update( &newDef, ref );
            int deltaWeight = weight - newDef.getWeight();
            if ( 1 && deltaWeight >= 200 ) {
                // embolden
                CRLog::trace("font: apply Embolding to increase weight from %d to %d", newDef.getWeight(), newDef.getWeight() + 200 );
                newDef.setWeight( newDef.getWeight() + 200 );
                ref = LVFontRef( new LVFontBoldTransform( ref, &_globalCache ) );
                _cache.update( &newDef, ref );
            }
//            int rsz = ref->getSize();
//            if ( rsz!=size ) {
//                size++;
//            }
            //delete def;
            return ref;
        }
        else
        {
            //printf("not found!\n");
        }
        //delete def;
        delete font;
        return LVFontRef(NULL);
    }

    bool checkCharSet(FT_Face face)
    {
        //TODO: Check existance of required characters (e.g. cyrillic)
        if (face == NULL)
            return false; // invalid face
        for (int i = 0; i < _requiredChars.length(); i++) {
            lChar16 ch = _requiredChars[i];
            FT_UInt ch_glyph_index = FT_Get_Char_Index(face, ch);
            if (ch_glyph_index == 0) {
                //CRLog::trace("Required char (%04x) not found in font %s", ch, face->family_name);
                // No required char
                return false;
            }
        }
        return true;
    }

    /*
    bool isMonoSpaced( FT_Face face )
    {
        //TODO: Check existance of required characters (e.g. cyrillic)
        if (face==NULL)
            return false; // invalid face
        lChar16 ch1 = 'i';
        FT_UInt ch_glyph_index1 = FT_Get_Char_Index( face, ch1 );
        if ( ch_glyph_index1==0 )
            return false; // no required char!!!
        int w1, w2;
        int error1 = FT_Load_Glyph( face,  //    handle to face object
                ch_glyph_index1,           //    glyph index
                FT_LOAD_DEFAULT );         //   load flags, see below
        if ( error1 )
            w1 = 0;
        else
            w1 = (face->glyph->metrics.horiAdvance >> 6);
        int error2 = FT_Load_Glyph( face,  //     handle to face object
                ch_glyph_index2,           //     glyph index
                FT_LOAD_DEFAULT );         //     load flags, see below
        if ( error2 )
            w2 = 0;
        else
            w2 = (face->glyph->metrics.horiAdvance >> 6);

        lChar16 ch2 = 'W';
        FT_UInt ch_glyph_index2 = FT_Get_Char_Index( face, ch2 );
        if ( ch_glyph_index2==0 )
            return false; // no required char!!!
        return w1==w2;
    }
    */

    /// registers document font
    virtual bool RegisterDocumentFont(int documentId, LVContainerRef container, lString16 name, lString8 faceName, bool bold, bool italic) {
        lString8 name8 = UnicodeToUtf8(name);
        CRLog::trace("RegisterDocumentFont(documentId=%d, path=%s)", documentId, name8.c_str());
        if (_cache.findDocumentFontDuplicate(documentId, name8)) {
            return false;
        }
        LVStreamRef stream = container->OpenStream(name.c_str(), LVOM_READ);
        if (stream.isNull())
            return false;
        lUInt32 size = (lUInt32)stream->GetSize();
        if (size < 100 || size > 5000000)
            return false;
        LVByteArrayRef buf(new LVByteArray(size, 0));
        lvsize_t bytesRead = 0;
        if (stream->Read(buf->get(), size, &bytesRead) != LVERR_OK || bytesRead != size)
            return false;
        bool res = false;

        int index = 0;

        FT_Face face = NULL;

        // for all faces in file
        for ( ;; index++ ) {
            int error = FT_New_Memory_Face( _library, buf->get(), buf->length(), index, &face ); /* create face object */
            if ( error ) {
                if (index == 0) {
                    CRLog::error("FT_New_Memory_Face returned error %d", error);
                }
                break;
            }
//            bool scal = FT_IS_SCALABLE( face );
//            bool charset = checkCharSet( face );
//            //bool monospaced = isMonoSpaced( face );
//            if ( !scal || !charset ) {
//    //#if (DEBUG_FONT_MAN==1)
//     //           if ( _log ) {
//                CRLog::trace("    won't register font %s: %s",
//                    name.c_str(), !charset?"no mandatory characters in charset" : "font is not scalable"
//                    );
//    //            }
//    //#endif
//                if ( face ) {
//                    FT_Done_Face( face );
//                    face = NULL;
//                }
//                break;
//            }
            int num_faces = face->num_faces;

            css_font_family_t fontFamily = css_ff_sans_serif;
            if ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
                fontFamily = css_ff_monospace;
            lString8 familyName(!faceName.empty() ? faceName : ::familyName(face));
            if ( familyName=="Times" || familyName=="Times New Roman" )
                fontFamily = css_ff_serif;

            bool boldFlag = !faceName.empty() ? bold : (face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
            bool italicFlag = !faceName.empty() ? italic : (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;

            LVFontDef def(
                name8,
                -1, // height==-1 for scalable fonts
                boldFlag ? 700 : 400,
                italicFlag,
                fontFamily,
                familyName,
                index,
                documentId,
                buf
            );
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            fprintf(_log, "registering font: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                def.getName().c_str(), def.getIndex(), def.getSize(), def.getWeight(), def.getItalic()?1:0, (int)def.getFamily(), def.getTypeFace().c_str()
            );
        }
    #endif
			if ( face ) {
				FT_Done_Face( face );
				face = NULL;
			}

            if ( _cache.findDuplicate( &def ) ) {
                CRLog::trace("font definition is duplicate");
                return false;
            }
            _cache.update( &def, LVFontRef(NULL) );
            if (!def.getItalic()) {
                LVFontDef newDef( def );
                newDef.setItalic(2); // can italicize
                if ( !_cache.findDuplicate( &newDef ) )
                    _cache.update( &newDef, LVFontRef(NULL) );
            }
            res = true;

            if ( index>=num_faces-1 )
                break;
        }

        return res;
    }
    /// unregisters all document fonts
    virtual void UnregisterDocumentFonts(int documentId) {
        _cache.removeDocumentFonts(documentId);
    }

    virtual bool RegisterExternalFont( lString16 name, lString8 family_name, bool bold, bool italic) {
		if (name.startsWithNoCase(lString16("res://")))
			name = name.substr(6);
		else if (name.startsWithNoCase(lString16("file://")))
			name = name.substr(7);
		lString8 fname = UnicodeToUtf8(name);

        bool res = false;

        int index = 0;

        FT_Face face = NULL;

        // for all faces in file
        for ( ;; index++ ) {
            int error = FT_New_Face( _library, fname.c_str(), index, &face ); /* create face object */
            if ( error ) {
                if (index == 0) {
                    CRLog::error("FT_New_Face returned error %d", error);
                }
                break;
            }
            bool scal = FT_IS_SCALABLE( face );
            bool charset = checkCharSet( face );
            //bool monospaced = isMonoSpaced( face );
            if ( !scal || !charset ) {
    //#if (DEBUG_FONT_MAN==1)
     //           if ( _log ) {
                CRLog::debug("    won't register font %s: %s",
                    name.c_str(), !charset?"no mandatory characters in charset" : "font is not scalable"
                    );
    //            }
    //#endif
                if ( face ) {
                    FT_Done_Face( face );
                    face = NULL;
                }
                break;
            }
            int num_faces = face->num_faces;

            css_font_family_t fontFamily = css_ff_sans_serif;
            if ( face->face_flags & FT_FACE_FLAG_FIXED_WIDTH )
                fontFamily = css_ff_monospace;
            lString8 familyName( ::familyName(face) );
            if ( familyName=="Times" || familyName=="Times New Roman" )
                fontFamily = css_ff_serif;

            LVFontDef def(
                fname,
                -1, // height==-1 for scalable fonts
                bold?700:400,
                italic?true:false,
                fontFamily,
                family_name,
                index
            );
    #if (DEBUG_FONT_MAN==1)
        if ( _log ) {
            fprintf(_log, "registering font: (file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                def.getName().c_str(), def.getIndex(), def.getSize(), def.getWeight(), def.getItalic()?1:0, (int)def.getFamily(), def.getTypeFace().c_str()
            );
        }
    #endif
            if ( _cache.findDuplicate( &def ) ) {
                CRLog::trace("font definition is duplicate");
                return false;
            }
            _cache.update( &def, LVFontRef(NULL) );
            if ( scal && !def.getItalic() ) {
                LVFontDef newDef( def );
                newDef.setItalic(2); // can italicize
                if ( !_cache.findDuplicate( &newDef ) )
                    _cache.update( &newDef, LVFontRef(NULL) );
            }
            res = true;

            if ( face ) {
                FT_Done_Face( face );
                face = NULL;
            }

            if ( index>=num_faces-1 )
                break;
        }

        return res;
	}

    virtual bool RegisterFont( lString8 name )
    {
        lString8 fname = makeFontFileName( name );
        //CRLog::trace("font file name : %s", fname.c_str());
#if (DEBUG_FONT_MAN == 1)
        if (_log) {
            fprintf(_log, "RegisterFont( %s ) path=%s\n",
                name.c_str(), fname.c_str()
            );
        }
#endif
        bool res = false;

        int index = 0;

        FT_Face face = NULL;

        // for all faces in file
        for ( ;; index++ ) {
            /* create face object */
            int error = FT_New_Face(_library, fname.c_str(), index, &face);
            if ( error ) {
                if (index == 0) {
                    CRLog::error("FT_New_Face returned error %d for %s",
                            error, name.c_str());
                }
                break;
            }
            bool scal = FT_IS_SCALABLE(face);
            bool charset = checkCharSet(face);
            //bool monospaced = isMonoSpaced( face );
            if (!scal || !charset) {
#if (DEBUG_FONT_MAN == 1)
                if (_log) {
					CRLog::trace("Won't register font %s: %s",
						    name.c_str(),
						    !charset
						            ? "no mandatory characters in charset"
						            : "font is not scalable");
                }
#endif
                if (face) {
                    FT_Done_Face(face);
                    face = NULL;
                }
                break;
            }
            int num_faces = face->num_faces;

            css_font_family_t fontFamily = css_ff_sans_serif;
            if (face->face_flags & FT_FACE_FLAG_FIXED_WIDTH)
                fontFamily = css_ff_monospace;
            lString8 familyName(::familyName(face));
            if (familyName=="Times" || familyName=="Times New Roman")
                fontFamily = css_ff_serif;

            LVFontDef def(
                name,
                -1, // height==-1 for scalable fonts
                ( face->style_flags & FT_STYLE_FLAG_BOLD ) ? 700 : 400,
                ( face->style_flags & FT_STYLE_FLAG_ITALIC ) ? true : false,
                fontFamily,
                familyName,
                index
            );
#if (DEBUG_FONT_MAN==1)
            if ( _log ) {
                fprintf(_log,
                        "registering font: "
                        "(file=%s[%d], size=%d, weight=%d, italic=%d, family=%d, typeface=%s)\n",
                        def.getName().c_str(),
                        def.getIndex(),
                        def.getSize(),
                        def.getWeight(),
                        def.getItalic() ? 1 : 0,
                        (int)def.getFamily(),
                        def.getTypeFace().c_str()
                );
            }
#endif
			if ( face ) {
				FT_Done_Face( face );
				face = NULL;
			}

			if ( _cache.findDuplicate( &def ) ) {
                CRLog::info("Font definition is duplicate %s", def.getName().c_str());
                return false;
            }
            _cache.update( &def, LVFontRef(NULL) );
            if (scal && !def.getItalic()) {
                LVFontDef newDef( def );
                newDef.setItalic(2); // can italicize
                if (!_cache.findDuplicate(&newDef) )
                    _cache.update(&newDef, LVFontRef(NULL));
            }
            res = true;

            if ( index>=num_faces-1 )
                break;
        }

        return res;
    }

    virtual bool Init(lString8 path)
    {
        _path = path;
        initSystemFonts();
        return (_library != NULL);
    }
};
#endif

#if (USE_BITMAP_FONTS==1)
class LVBitmapFontManager : public LVFontManager
{
private:
    lString8    _path;
    LVFontCache _cache;
    //FILE * _log;
public:
    virtual int GetFontCount()
    {
        return _cache.length();
    }
    virtual ~LVBitmapFontManager()
    {
        //if (_log)
        //    fclose(_log);
    }
    LVBitmapFontManager()
    {
        //_log = fopen( "fonts.log", "wt" );
    }
    virtual void gc() // garbage collector
    {
        _cache.gc();
    }
    lString8 makeFontFileName( lString8 name )
    {
        lString8 filename = _path;
        if (!filename.empty() && _path[filename.length()-1]!=PATH_SEPARATOR_CHAR)
            filename << PATH_SEPARATOR_CHAR;
        filename << name;
        return filename;
    }
    virtual LVFontRef GetFont(int size, int weight, bool italic, css_font_family_t family, lString8 typeface, int documentId)
    {
        LVFontDef * def = new LVFontDef(
            lString8::empty_str,
            size,
            weight,
            italic,
            family,
            typeface,
            documentId
        );
        //fprintf( _log, "GetFont: %s %d %s %s\n",
        //    typeface.c_str(),
        //    size,
        //    weight>400?"bold":"",
        //    italic?"italic":"" );
        LVFontCacheItem * item = _cache.find( def );
	delete def;
        if (!item->getFont().isNull())
        {
            //fprintf(_log, "    : fount existing\n");
            return item->getFont();
        }
        LBitmapFont * font = new LBitmapFont;
        lString8 fname = makeFontFileName( item->getDef()->getName() );
        //printf("going to load font file %s\n", fname.c_str());
        if (font->LoadFromFile( fname.c_str() ) )
        {
            //fprintf(_log, "    : loading from file %s : %s %d\n", item->getDef()->getName().c_str(),
            //    item->getDef()->getTypeFace().c_str(), item->getDef()->getSize() );
            LVFontRef ref(font);
            item->setFont( ref );
            return ref;
        }
        else
        {
            //printf("    not found!\n");
        }
        delete font;
        return LVFontRef(NULL);
    }
    virtual bool RegisterFont( lString8 name )
    {
        lString8 fname = makeFontFileName( name );
        //printf("going to load font file %s\n", fname.c_str());
        LVStreamRef stream = LVOpenFileStream( fname.c_str(), LVOM_READ );
        if (!stream)
        {
            //printf("    not found!\n");
            return false;
        }
        tag_lvfont_header hdr;
        bool res = false;
        lvsize_t bytes_read = 0;
        if ( stream->Read( &hdr, sizeof(hdr), &bytes_read ) == LVERR_OK && bytes_read == sizeof(hdr) )
        {
            LVFontDef def(
                name,
                hdr.fontHeight,
                hdr.flgBold?700:400,
                hdr.flgItalic?true:false,
                (css_font_family_t)hdr.fontFamily,
                lString8(hdr.fontName)
            );
            //fprintf( _log, "Register: %s %s %d %s %s\n",
            //    name.c_str(), hdr.fontName,
            //    hdr.fontHeight,
            //    hdr.flgBold?"bold":"",
            //    hdr.flgItalic?"italic":"" );
            _cache.update( &def, LVFontRef(NULL) );
            res = true;
        }
        return res;
    }
    virtual bool Init( lString8 path )
    {
        _path = path;
        return true;
    }
};
#endif



#if (USE_BITMAP_FONTS==1)

LVFontRef LoadFontFromFile( const char * fname )
{
    LVFontRef ref;
    LBitmapFont * font = new LBitmapFont;
    if (font->LoadFromFile(fname))
    {
        ref = font;
    }
    else
    {
        delete font;
    }
    return ref;
}

#endif

bool InitFontManager(lString8 path)
{
    if ( fontMan ) {
    	return true;
        //delete fontMan;
    }
#if (USE_FREETYPE==1)
    fontMan = new LVFreeTypeFontManager;
#else
    fontMan = new LVBitmapFontManager;
#endif
    return fontMan->Init(path);
}

void ShutdownFontManager()
{
    if (fontMan) {
        delete fontMan;
        fontMan = NULL;
    }
}

int LVFontDef::CalcDuplicateMatch( const LVFontDef & def ) const
{
    if (def._documentId != -1 && _documentId != def._documentId)
        return false;
	bool size_match = (_size==-1 || def._size==-1) ? true
        : (def._size == _size);
    bool weight_match = (_weight==-1 || def._weight==-1) ? true
        : (def._weight == _weight);
    bool italic_match = (_italic == def._italic || _italic==-1 || def._italic==-1);
    bool family_match = (_family==css_ff_inherit || def._family==css_ff_inherit || def._family == _family);
    bool typeface_match = (_typeface == def._typeface);
    return size_match && weight_match && italic_match && family_match && typeface_match;
}

int LVFontDef::CalcMatch( const LVFontDef & def ) const
{
    if (_documentId != -1 && _documentId != def._documentId)
        return 0;
    int size_match = (_size==-1 || def._size==-1) ? 256
        : (def._size>_size ? _size*256/def._size : def._size*256/_size );
    int weight_diff = def._weight - _weight;
    if ( weight_diff<0 )
        weight_diff = -weight_diff;
    if ( weight_diff > 800 )
        weight_diff = 800;
    int weight_match = (_weight==-1 || def._weight==-1) ? 256
        : ( 256 - weight_diff * 256 / 800 );
    int italic_match = (_italic == def._italic || _italic==-1 || def._italic==-1) ? 256 : 0;
    if ( (_italic==2 || def._italic==2) && _italic>0 && def._italic>0 )
        italic_match = 128;
    int family_match = (_family==css_ff_inherit || def._family==css_ff_inherit || def._family == _family)
        ? 256
        : ( (_family==css_ff_monospace)==(def._family==css_ff_monospace) ? 64 : 0 );
    int typeface_match = (_typeface == def._typeface) ? 256 : 0;
    return
        + (size_match     * 100)
        + (weight_match   * 5)
        + (italic_match   * 5)
        + (family_match   * 100)
        + (typeface_match * 1000);
}

int LVFontDef::CalcFallbackMatch( lString8 face, int size ) const
{
    if (_typeface != face) {
        //CRLog::trace("'%s'' != '%s'", face.c_str(), _typeface.c_str());
        return 0;
    }
    int size_match = (_size==-1 || size==-1 || _size==size) ? 256 : 0;
    int weight_match = (_weight==-1) ? 256 : ( 256 - _weight * 256 / 800 );
    int italic_match = _italic == 0 ? 256 : 0;
    return
        + (size_match     * 100)
        + (weight_match   * 5)
        + (italic_match   * 5);
}

#if (USE_BITMAP_FONTS==1)

/// returns font baseline offset
int LBitmapFont::getBaseline()
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->fontBaseline;
}

void LBitmapFont::DrawTextString(LVDrawBuf* buf, int x, int y, const lChar16* text, int len,
                   lChar16 def_char, lUInt32* palette, bool addHyphen,
                   lUInt32, int)
{
    //static lUInt8 glyph_buf[16384];
    //LVFont::glyph_info_t info;
    int baseline = getBaseline();
    while (len >= (addHyphen ? 0 : 1)) {
      if (len <= 1 || *text != UNICODE_SOFT_HYPHEN_CODE) {
          lChar16 ch = ((len == 0) ? UNICODE_SOFT_HYPHEN_CODE : *text);
          LVFontGlyphCacheItem * item = getGlyph(ch, def_char);
          int w  = 0;
          if ( item ) {
              // avoid soft hyphens inside text string
              w = item->advance;
              if ( item->bmp_width && item->bmp_height ) {
                  buf->Draw( x + item->origin_x,
                      y + baseline - item->origin_y,
                      item->bmp,
                      item->bmp_width,
                      item->bmp_height,
                      palette);
              }
          }
          x  += w; // + letter_spacing;
//          if ( !getGlyphInfo( ch, &info, def_char ) )
//          {
//              ch = def_char;
//              if ( !getGlyphInfo( ch, &info, def_char ) )
//                  ch = 0;
//          }
//          if (ch && getGlyphImage( ch, glyph_buf, def_char ))
//          {
//              if (info.blackBoxX && info.blackBoxY)
//              {
//                  buf->Draw( x + info.originX,
//                      y + baseline - info.originY,
//                      glyph_buf,
//                      info.blackBoxX,
//                      info.blackBoxY,
//                      palette);
//              }
//              x += info.width;
//          }
      } else if (*text != UNICODE_SOFT_HYPHEN_CODE) {
          //len = len;
      }
      len--;
      text++;
    }
}

bool LBitmapFont::getGlyphInfo( lUInt16 code, LVFont::glyph_info_t * glyph, lChar16 def_char=0 )
{
    const lvfont_glyph_t * ptr = lvfontGetGlyph( m_font, code );
    if (!ptr)
        return false;
    glyph->blackBoxX = ptr->blackBoxX;
    glyph->blackBoxY = ptr->blackBoxY;
    glyph->originX = ptr->originX;
    glyph->originY = ptr->originY;
    glyph->width = ptr->width;
    return true;
}

virtual lUInt16 LBitmapFont::measureText(const lChar16* text, int len, lUInt16* widths,
                    lUInt8* flags, int max_width, lChar16 def_char,
                    int letter_spacing, bool allow_hyphenation)
{
    return lvfontMeasureText( m_font, text, len, widths, flags, max_width, def_char );
}

lUInt32 LBitmapFont::getTextWidth( const lChar16 * text, int len )
{
    //
    static lUInt16 widths[MAX_LINE_CHARS+1];
    static lUInt8 flags[MAX_LINE_CHARS+1];
    if ( len>MAX_LINE_CHARS )
        len = MAX_LINE_CHARS;
    if ( len<=0 )
        return 0;
    // 2048  - max_width, L' ' - def_char
    lUInt16 res = measureText(text, len, widths, flags, 2048, L' ', 0, true);
    if (res > 0 && res < MAX_LINE_CHARS)
        return widths[res-1];
    return 0;
}

/// returns font height
int LBitmapFont::getHeight() const
{
    const lvfont_header_t * hdr = lvfontGetHeader( m_font );
    return hdr->fontHeight;
}



bool LBitmapFont::getGlyphImage(lUInt16 code, lUInt8 * buf, lChar16 def_char=0)
{
    const lvfont_glyph_t * ptr = lvfontGetGlyph( m_font, code );
    if (!ptr)
        return false;
    const hrle_decode_info_t * pDecodeTable = lvfontGetDecodeTable( m_font );
    int sz = ptr->blackBoxX*ptr->blackBoxY;
    if (sz)
        lvfontUnpackGlyph(ptr->glyph, pDecodeTable, buf, sz);
    return true;
}
int LBitmapFont::LoadFromFile( const char * fname )
{
    Clear();
    int res = (void*)lvfontOpen( fname, &m_font )!=NULL;
    if (!res)
        return 0;
    lvfont_header_t * hdr = (lvfont_header_t*) m_font;
    _typeface = lString8( hdr->fontName );
    _family = (css_font_family_t) hdr->fontFamily;
    return 1;
}
#endif

LVFontCacheItem * LVFontCache::findDuplicate( const LVFontDef * def )
{
    for (int i=0; i<_registered_list.length(); i++)
    {
        if ( _registered_list[i]->_def.CalcDuplicateMatch( *def ) )
            return _registered_list[i];
    }
    return NULL;
}

LVFontCacheItem * LVFontCache::findDocumentFontDuplicate(int documentId, lString8 name)
{
    for (int i=0; i<_registered_list.length(); i++) {
        if (_registered_list[i]->_def.getDocumentId() == documentId && _registered_list[i]->_def.getName() == name)
            return _registered_list[i];
    }
    return NULL;
}

LVFontCacheItem * LVFontCache::findFallback( lString8 face, int size )
{
    int best_index = -1;
    int best_match = -1;
    int best_instance_index = -1;
    int best_instance_match = -1;
    int i;
    for (i=0; i<_instance_list.length(); i++)
    {
        int match = _instance_list[i]->_def.CalcFallbackMatch( face, size );
        if (match > best_instance_match)
        {
            best_instance_match = match;
            best_instance_index = i;
        }
    }
    for (i=0; i<_registered_list.length(); i++)
    {
        int match = _registered_list[i]->_def.CalcFallbackMatch( face, size );
        if (match > best_match)
        {
            best_match = match;
            best_index = i;
        }
    }
    if (best_index<=0)
        return NULL;
    if (best_instance_match >= best_match)
        return _instance_list[best_instance_index];
    return _registered_list[best_index];
}

LVFontCacheItem * LVFontCache::find( const LVFontDef * fntdef )
{
    int best_index = -1;
    int best_match = -1;
    int best_instance_index = -1;
    int best_instance_match = -1;
    int i;
    LVFontDef def(*fntdef);
    lString8Collection list;
    splitPropertyValueList( fntdef->getTypeFace().c_str(), list );
    for (int nindex=0; nindex==0 || nindex<list.length(); nindex++)
    {
        if ( nindex<list.length() )
            def.setTypeFace( list[nindex] );
        else
            def.setTypeFace(lString8::empty_str);
        for (i=0; i<_instance_list.length(); i++)
        {
            int match = _instance_list[i]->_def.CalcMatch( def );
            if (match > best_instance_match)
            {
                best_instance_match = match;
                best_instance_index = i;
            }
        }
        for (i=0; i<_registered_list.length(); i++)
        {
            int match = _registered_list[i]->_def.CalcMatch( def );
            if (match > best_match)
            {
                best_match = match;
                best_index = i;
            }
        }
    }
    if (best_index<0)
        return NULL;
    if (best_instance_match >= best_match)
        return _instance_list[best_instance_index];
    return _registered_list[best_index];
}

void LVFontCache::addInstance( const LVFontDef * def, LVFontRef ref )
{
    if ( ref.isNull() )
        printf("Adding null font instance!");
    LVFontCacheItem * item = new LVFontCacheItem(*def);
    item->_fnt = ref;
    _instance_list.add( item );
}

void LVFontCache::removefont(const LVFontDef * def)
{
    int i;
    for (i=0; i<_instance_list.length(); i++)
    {
        if ( _instance_list[i]->_def.getTypeFace() == def->getTypeFace() )
        {
            _instance_list.remove(i);
        }

    }
    for (i=0; i<_registered_list.length(); i++)
    {
        if ( _registered_list[i]->_def.getTypeFace() == def->getTypeFace() )
        {
            _registered_list.remove(i);
        }
    }

}

void LVFontCache::update( const LVFontDef * def, LVFontRef ref )
{
    int i;
    if ( !ref.isNull() ) {
        for (i=0; i<_instance_list.length(); i++)
        {
            if ( _instance_list[i]->_def == *def )
            {
                if (ref.isNull())
                {
                    _instance_list.erase(i, 1);
                }
                else
                {
                    _instance_list[i]->_fnt = ref;
                }
                return;
            }
        }
        // add new
        //LVFontCacheItem * item;
        //item = new LVFontCacheItem(*def);
        addInstance( def, ref );
    } else {
        for (i=0; i<_registered_list.length(); i++)
        {
            if ( _registered_list[i]->_def == *def )
            {
                return;
            }
        }
        // add new
        LVFontCacheItem * item;
        item = new LVFontCacheItem(*def);
        _registered_list.add( item );
    }
}

void LVFontCache::removeDocumentFonts(int documentId)
{
    int i;
    for (i=_instance_list.length()-1; i>=0; i--) {
        if (_instance_list[i]->_def.getDocumentId() == documentId)
            delete _instance_list.remove(i);
    }
    for (i=_registered_list.length()-1; i>=0; i--) {
        if (_registered_list[i]->_def.getDocumentId() == documentId)
            delete _registered_list.remove(i);
    }
}

// garbage collector
void LVFontCache::gc()
{
    int droppedCount = 0;
    int usedCount = 0;
    for (int i=_instance_list.length()-1; i>=0; i--) {
        if (_instance_list[i]->_fnt.getRefCount() <= 1) {
			CRLog::trace("dropping font instance %s[%d] by gc()",
					_instance_list[i]->getDef()->getTypeFace().c_str(), _instance_list[i]->getDef()->getSize());
            _instance_list.erase(i,1);
            droppedCount++;
        } else {
            usedCount++;
        }
    }
}

/// to compare two fonts
bool operator == (const LVFont & r1, const LVFont & r2)
{
    if ( &r1 == &r2 )
        return true;
    return r1.getSize()==r2.getSize()
            && r1.getWeight()==r2.getWeight()
            && r1.getItalic()==r2.getItalic()
            && r1.getFontFamily()==r2.getFontFamily()
            && r1.getTypeFace()==r2.getTypeFace()
            && r1.getKerning()==r2.getKerning()
            && r1.getHintingMode()==r2.getHintingMode()
            ;
}

