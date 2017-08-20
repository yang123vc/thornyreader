/*
 * Copyright (C) 2013 The MuPDF CLI viewer interface Project
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
#include <set>

#include "StLog.h"
#include "StProtocol.h"

#include "MuPdfBridge.h"

#define LCTX "EBookDroid.MuPDF.Decoder"
#define L_DEBUG false

#define SANS        0
#define SERIF       1
#define MONO        2
#define SYMBOL      3
#define DINGBAT     4

#define REGULAR     0
#define ITALIC      1
#define BOLD        2
#define BOLD_ITALIC 3

#define MAX_SYS_FONT_TABLE_SIZE 1024

extern "C"
{
extern char ext_font_Courier[1024];
extern char ext_font_CourierBold[1024];
extern char ext_font_CourierOblique[1024];
extern char ext_font_CourierBoldOblique[1024];
extern char ext_font_Helvetica[1024];
extern char ext_font_HelveticaBold[1024];
extern char ext_font_HelveticaOblique[1024];
extern char ext_font_HelveticaBoldOblique[1024];
extern char ext_font_TimesRoman[1024];
extern char ext_font_TimesBold[1024];
extern char ext_font_TimesItalic[1024];
extern char ext_font_TimesBoldItalic[1024];
extern char ext_font_Symbol[1024];
extern char ext_font_ZapfDingbats[1024];

char ext_system_fonts[MAX_SYS_FONT_TABLE_SIZE][2][512];
unsigned int ext_system_fonts_idx[MAX_SYS_FONT_TABLE_SIZE];
int ext_system_fonts_count;
}
;

void MuPdfBridge::resetFonts()
{
    ext_font_Courier[0] = 0;
    ext_font_CourierBold[0] = 0;
    ext_font_CourierOblique[0] = 0;
    ext_font_CourierBoldOblique[0] = 0;
    ext_font_Helvetica[0] = 0;
    ext_font_HelveticaBold[0] = 0;
    ext_font_HelveticaOblique[0] = 0;
    ext_font_HelveticaBoldOblique[0] = 0;
    ext_font_TimesRoman[0] = 0;
    ext_font_TimesBold[0] = 0;
    ext_font_TimesItalic[0] = 0;
    ext_font_TimesBoldItalic[0] = 0;
    ext_font_Symbol[0] = 0;
    ext_font_ZapfDingbats[0] = 0;

    for (int i = 0; i < MAX_SYS_FONT_TABLE_SIZE; i++)
    {
        ext_system_fonts[i][0][0] = 0;
        ext_system_fonts[i][1][0] = 0;
        ext_system_fonts_idx[i] = 0;
    }

    ext_system_fonts_count = 0;
}

void MuPdfBridge::processFonts(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PDF_FONTS;

    CmdDataIterator iter(request.first);
    while (iter.hasNext())
    {
        uint32_t fontFamily;
        if (!iter.getInt(&fontFamily).isValid())
        {
            ERROR_L(LCTX, "No font family found");
            response.result = RES_BAD_REQ_DATA;
            return;
        }

        uint8_t* fonts[4];

        INFO_L(LCTX, "Font family: %d", fontFamily);
        switch (fontFamily)
        {
        case SANS:
            iter.getByteArray(&fonts[0]).getByteArray(&fonts[1]).getByteArray(&fonts[2]).getByteArray(&fonts[3]);
            if (!iter.isValid())
            {
                ERROR_L(LCTX, "No SANS fonts found");
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            INFO_L(LCTX, "Regular     : %s", fonts[REGULAR]);
            INFO_L(LCTX, "Italic      : %s", fonts[ITALIC]);
            INFO_L(LCTX, "Bold        : %s", fonts[BOLD]);
            INFO_L(LCTX, "Bold Italic : %s", fonts[BOLD_ITALIC]);
            setFontFileName(ext_font_Helvetica, fonts[REGULAR]);
            setFontFileName(ext_font_HelveticaOblique, fonts[ITALIC]);
            setFontFileName(ext_font_HelveticaBold, fonts[BOLD]);
            setFontFileName(ext_font_HelveticaBoldOblique, fonts[BOLD_ITALIC]);
            break;
        case SERIF:
            iter.getByteArray(&fonts[0]).getByteArray(&fonts[1]).getByteArray(&fonts[2]).getByteArray(&fonts[3]);
            if (!iter.isValid())
            {
                ERROR_L(LCTX, "No SERIF fonts found");
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            INFO_L(LCTX, "Regular     : %s", fonts[REGULAR]);
            INFO_L(LCTX, "Italic      : %s", fonts[ITALIC]);
            INFO_L(LCTX, "Bold        : %s", fonts[BOLD]);
            INFO_L(LCTX, "Bold Italic : %s", fonts[BOLD_ITALIC]);
            setFontFileName(ext_font_TimesRoman, fonts[REGULAR]);
            setFontFileName(ext_font_TimesItalic, fonts[ITALIC]);
            setFontFileName(ext_font_TimesBold, fonts[BOLD]);
            setFontFileName(ext_font_TimesBoldItalic, fonts[BOLD_ITALIC]);
            break;
        case MONO:
            iter.getByteArray(&fonts[0]).getByteArray(&fonts[1]).getByteArray(&fonts[2]).getByteArray(&fonts[3]);
            if (!iter.isValid())
            {
                ERROR_L(LCTX, "No MONO fonts found");
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            INFO_L(LCTX, "Regular     : %s", fonts[REGULAR]);
            INFO_L(LCTX, "Italic      : %s", fonts[ITALIC]);
            INFO_L(LCTX, "Bold        : %s", fonts[BOLD]);
            INFO_L(LCTX, "Bold Italic : %s", fonts[BOLD_ITALIC]);
            setFontFileName(ext_font_Courier, fonts[REGULAR]);
            setFontFileName(ext_font_CourierOblique, fonts[ITALIC]);
            setFontFileName(ext_font_CourierBold, fonts[BOLD]);
            setFontFileName(ext_font_CourierBoldOblique, fonts[BOLD_ITALIC]);
            break;
        case SYMBOL:
            iter.getByteArray(&fonts[0]).getByteArray(&fonts[1]).getByteArray(&fonts[2]).getByteArray(&fonts[3]);
            if (!iter.isValid())
            {
                ERROR_L(LCTX, "No SYMBOL fonts found");
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            INFO_L(LCTX, "Regular     : %s", fonts[REGULAR]);
            setFontFileName(ext_font_Symbol, fonts[REGULAR]);
            break;
        case DINGBAT:
            iter.getByteArray(&fonts[0]).getByteArray(&fonts[1]).getByteArray(&fonts[2]).getByteArray(&fonts[3]);
            if (!iter.isValid())
            {
                ERROR_L(LCTX, "No DINGBAT fonts found");
                response.result = RES_BAD_REQ_DATA;
                return;
            }
            INFO_L(LCTX, "Regular     : %s", fonts[REGULAR]);
            setFontFileName(ext_font_ZapfDingbats, fonts[REGULAR]);
            break;
        default:
            ERROR_L(LCTX, "Unknown font family: %d", fontFamily);
            response.result = RES_BAD_REQ_DATA;
            break;
        }
    }

    if (document != NULL)
    {
        restart();
    }
}

void MuPdfBridge::setFontFileName(char* ext_Font, uint8_t* fontFileName)
{
    if (fontFileName && fontFileName[0])
    {
        strcpy(ext_Font, (const char*) fontFileName);
    }
    else
    {
        ext_Font[0] = 0;
    }
}

void MuPdfBridge::processSystemFont(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PDF_SYSTEM_FONT;

    CmdDataIterator iter(request.first);
    uint8_t* fonts[2];
    uint32_t index;

    iter.getByteArray(&fonts[0]).getByteArray(&fonts[1]).getInt(&index);
    if (!iter.isValid())
    {
        ERROR_L(LCTX, "No mapping found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    INFO_L(LCTX, "%s: %s[%d]", fonts[0], fonts[1], index);
    if (ext_system_fonts_count < MAX_SYS_FONT_TABLE_SIZE)
    {
        setFontFileName(ext_system_fonts[ext_system_fonts_count][0], fonts[0]);
        setFontFileName(ext_system_fonts[ext_system_fonts_count][1], fonts[1]);
        ext_system_fonts_idx[ext_system_fonts_count] = index;
        ext_system_fonts_count++;
    }
    else
    {
        ERROR_L(LCTX, "No more fonts allowed");
        return;
    }
}

static void gatherfonts(fz_context *ctx, pdf_document* doc, pdf_obj* rsrc, pdf_obj *dict, std::set<std::string>& external, std::set<std::string>& all)
{
    int n = pdf_dict_len(ctx, dict);
    for (int i = 0; i < n; i++)
    {
        pdf_obj *fontdict = NULL;
        pdf_obj *subtype = NULL;
        pdf_obj *basefont = NULL;
        pdf_obj *name = NULL;
        int k;

        fontdict = pdf_dict_get_val(ctx, dict, i);
        if (!pdf_is_dict(ctx, fontdict))
        {
            continue;
        }

        basefont = pdf_dict_gets(ctx, fontdict, "BaseFont");
        if (!basefont || pdf_is_null(ctx, basefont))
        {
            continue;
        }

        std::string basefontname = std::string(pdf_to_name(ctx, basefont));
        if (all.find(basefontname) != all.end())
        {
            continue;
        }

        all.insert(basefontname);

        pdf_font_desc* font = pdf_load_font(ctx, doc, rsrc, fontdict, 0);

        if (font)
        {
            if (font->is_embedded)
            {
                INFO_L(LCTX, "Embedded Document font: basefont=%s", basefontname.c_str());
            }
            else if (font->external_origin == PDF_FD0_BUILTIN)
            {
                INFO_L(LCTX, "Built-in Document font: basefont=%s file=%s", basefontname.c_str(), font->font->ft_filepath);
            }
            else if (font->external_origin == PDF_FD0_SYSTEM)
            {
                INFO_L(LCTX, "System   Document font: basefont=%s file=%s", basefontname.c_str(), font->font->ft_filepath);
            }
            else
            {
                INFO_L(LCTX, "External Document font: basefont=%s file=%s", basefontname.c_str(), font->font->ft_filepath);
                external.insert(basefontname);
            }
        }
        else
        {
               ERROR_L(LCTX, "Unknown  Document font: basefont=%s", basefontname.c_str());
        }
    }
}

static void gatherresourceinfo(fz_context *ctx, pdf_document* doc, pdf_obj *rsrc, std::set<std::string>& external, std::set<std::string>& all)
{
    pdf_obj *font;
    pdf_obj *xobj;
    pdf_obj *shade;
    pdf_obj *pattern;
    pdf_obj *subrsrc;

    font = pdf_dict_gets(ctx, rsrc, "Font");

    gatherfonts(ctx, doc, rsrc, font, external, all);

    int n = pdf_dict_len(ctx, font);
    for (int i = 0; i < n; i++)
    {
        pdf_obj *obj = pdf_dict_get_val(ctx, font, i);

        subrsrc = pdf_dict_gets(ctx, obj, "Resources");
        if (subrsrc && pdf_objcmp(ctx, rsrc, subrsrc))
        {
            gatherresourceinfo(ctx, doc, subrsrc, external, all);
        }
    }
}

void MuPdfBridge::processGetMissedFonts(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PDF_GET_MISSED_FONTS;

    if (format != FORMAT_PDF)
    {
        response.result = RES_NOT_OPENED;
        return;
    }

    restart();

    pdf_document* doc = (pdf_document*) document;

    std::set<std::string> all;
    fonts.clear();
    for (int i = 0; i < pageCount; i++)
    {
        pdf_obj *pageobj;
        pdf_obj *pageref;
        pdf_obj *rsrc;

        pageref = pdf_lookup_page_obj(ctx, doc, i);
        pageobj = pdf_resolve_indirect(ctx, pageref);

        if (pageobj)
        {
            rsrc = pdf_dict_gets(ctx, pageobj, "Resources");
            gatherresourceinfo(ctx, doc, rsrc, fonts, all);
        }
    }

    response.addInt(fonts.size());
    for(std::set<std::string>::iterator it = fonts.begin(); it != fonts.end(); ++it)
    {
    	response.addIpcString((*it).c_str(), false);
    }
}

