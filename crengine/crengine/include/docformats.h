#ifndef BOOKFORMATS_H
#define BOOKFORMATS_H

typedef enum {
    doc_format_none,
    doc_format_fb2,
    doc_format_txt,
    doc_format_rtf,
    doc_format_epub,
    doc_format_html,
    doc_format_chm,
    doc_format_doc,
    doc_format_mobi,
    doc_format_max = doc_format_mobi
} doc_format_t;

extern const char* CRCSS_DEF;
extern const char* CRCSS_CHM;
extern const char* CRCSS_DOC;
extern const char* CRCSS_EPUB;
extern const char* CRCSS_FB2;
extern const char* CRCSS_HTM;
extern const char* CRCSS_RTF;
extern const char* CRCSS_TXT;

/*
int LVDocFormatFromExtension(lString16 &pathName) {
    if (pathName.endsWith(".fb2"))
        return doc_format_fb2;
    if (pathName.endsWith(".txt")
			|| pathName.endsWith(".pml"))
        return doc_format_txt;
    if (pathName.endsWith(".rtf"))
        return doc_format_rtf;
    if (pathName.endsWith(".epub"))
        return doc_format_epub;
    if (pathName.endsWith(".htm")
    		|| pathName.endsWith(".html")
			|| pathName.endsWith(".shtml")
			|| pathName.endsWith(".xhtml"))
        return doc_format_html;
    if (pathName.endsWith(".chm"))
        return doc_format_chm;
    if (pathName.endsWith(".doc"))
        return doc_format_doc;
    if (pathName.endsWith(".pdb")
    		|| pathName.endsWith(".prc")
			|| pathName.endsWith(".mobi")
			|| pathName.endsWith(".azw"))
        return doc_format_mobi;
    return doc_format_none;
}
*/

#endif // BOOKFORMATS_H
