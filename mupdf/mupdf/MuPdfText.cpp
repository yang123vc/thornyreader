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

#include "StLog.h"
#include "StProtocol.h"

#include "MuPdfBridge.h"

#define LCTX "EBookDroid.MuPDF.Decoder.Search"
#define L_DEBUG_TEXT false
#define L_DEBUG_CHARS false

char utf8[32 * 1024];

void toResponse(CmdResponse& response, fz_rect& bounds, fz_irect* rr, const char* str, int len)
{
    float width = bounds.x1 - bounds.x0;
    float height = bounds.y1 - bounds.y0;
    float left = (rr->x0 - bounds.x0) / width;
    float top = (rr->y0 - bounds.y0) / height;
    float right = (rr->x1 - bounds.x0) / width;
    float bottom = (rr->y1 - bounds.y0) / height;

    utf8[len] = 0;

    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: add word: %d %f %f %f %f %s", len, left, top, right, bottom, utf8);

    response.addFloat(left);
    response.addFloat(top);
    response.addFloat(right);
    response.addFloat(bottom);
    response.addIpcString(utf8, true);
}

void processLine(CmdResponse& response, fz_context *ctx, fz_rect& bounds, fz_text_line& line)
{
    int index = 0;
    fz_rect rr = fz_empty_rect;
    fz_irect box = fz_empty_irect;

    int spanIndex;
    fz_text_span* span = line.first_span;
    for (span = line.first_span; span != NULL; span = span->next)
    {
        bool prevspace = false;
        DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: span processing: %d", spanIndex);
        if (span->text && span->len > 0)
        {
            int textIndex;
            for (textIndex = 0; textIndex < span->len;)
            {
                fz_text_char& text = span->text[textIndex];
                bool space = (text.c == 0x0D) || (text.c == 0x0A) || (text.c == 0x09) || (text.c == 0x20);
                if (prevspace && space)
                {
                    // Do nothing with spaces
                    textIndex++;
                }
                else if (!prevspace && !space)
                {
                    fz_rect bbox;
                    fz_text_char_bbox(ctx, &bbox, span, textIndex);
                    fz_union_rect(&rr, &bbox);
                    index += fz_runetochar(utf8 + index, text.c);

                    DEBUG_L(L_DEBUG_CHARS, LCTX,
                        "processText: char processing: %d %04x %f %f %f %f", index, text.c, rr.x0, rr.y0, rr.x1, rr.y1);
                    textIndex++;
                }
                else
                {
                    if (index > 0)
                    {
                        toResponse(response, bounds, fz_round_rect(&box, &rr), utf8, index);
                        index = 0;
                    }
                    rr = fz_empty_rect;
                    box = fz_empty_irect;
                }
                prevspace = space;
            }
        }
    }

    if (index > 0)
    {
        DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: tail processing: %d", index);
        toResponse(response, bounds, fz_round_rect(&box, &rr), utf8, index);
    }
}

void MuPdfBridge::processText(int pageNo, const char* pattern, CmdResponse& response)
{
    fz_page *page = getPage(pageNo, false);
    if (page == NULL)
    {
        DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: no page %d", pageNo);
        return;
    }

    fz_text_sheet *sheet = NULL;
    fz_text_page *pagetext = NULL;
    fz_device *dev = NULL;

    fz_try(ctx)
            {
                fz_rect bounds = fz_empty_rect;
                fz_bound_page(ctx, page, &bounds);

                DEBUG_L(L_DEBUG_TEXT, LCTX,
                    "processText: page bounds: %f %f %f %f", bounds.x0, bounds.y0, bounds.x1, bounds.y1);

                sheet = fz_new_text_sheet(ctx);
                pagetext = fz_new_text_page(ctx);
                dev = fz_new_text_device(ctx, sheet, pagetext);

                fz_run_page(ctx, page, dev, &fz_identity, NULL);

                // !!! Last line added to page only on device release
                fz_drop_device(ctx, dev);
                dev = NULL;

                if (pagetext->blocks && pagetext->len > 0)
                {
                    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: text found on page %d: %d/%d blocks", pageNo, pagetext->len, pagetext->cap);

                    int blockIndex;
                    for (blockIndex = 0; blockIndex < pagetext->len; blockIndex++)
                    {
                        fz_page_block& block = pagetext->blocks[blockIndex];
                        if (block.type != FZ_PAGE_BLOCK_TEXT)
                        {
                            continue;
                        }
                        if (block.u.text->lines && block.u.text->len > 0)
                        {
                            DEBUG_L(L_DEBUG_TEXT, LCTX,
                                "processText: block processing: %d, %d/%d lines", blockIndex, block.u.text->len, block.u.text->cap);
                            int lineIndex;
                            for (lineIndex = 0; lineIndex < block.u.text->len; lineIndex++)
                            {
                                fz_text_line& line = block.u.text->lines[lineIndex];
                                if (line.first_span)
                                {
                                    DEBUG_L(L_DEBUG_TEXT, LCTX,
                                        "processText: line processing: %d", lineIndex);
                                    processLine(response, ctx, bounds, line);
                                }
                            }
                        }
                    }
                    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: page processed");
                }
                else
                {
                    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: no text found on page %d", pageNo);
                }
            }
            fz_always(ctx)
            {
                if (dev)
                {
                    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: cleanup dev");
                    fz_drop_device(ctx, dev);
                }
                if (pagetext)
                {
                    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: cleanup pagetext");
                    fz_drop_text_page(ctx, pagetext);
                }
                if (sheet)
                {
                    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: cleanup sheet");
                    fz_drop_text_sheet(ctx, sheet);
                }
            }fz_catch(ctx)
    {
        const char* msg = fz_caught_message(ctx);
        ERROR_L(LCTX, "%s", msg);
        response.result = RES_MUPDF_FAIL;
    }

    DEBUG_L(L_DEBUG_TEXT, LCTX, "processText: end");
}

