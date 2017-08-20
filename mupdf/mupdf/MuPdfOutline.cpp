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

#define LCTX "EBookDroid.MuPDF.Decoder.Outline"
#define L_DEBUG_OUTLINE false

const char EMPTY_STR[1] = { 0x0 };

void toResponse(CmdResponse& response, int level, char* title, int kind, int pageNo, char* url, float x, float y)
{
    response.addWords((uint16_t) kind, pageNo);
	response.addInt((uint32_t) level);
	response.addIpcString(title != NULL ? title : EMPTY_STR, false);

    if (kind == FZ_LINK_URI)
    {
        response.addIpcString(url != NULL ? url : EMPTY_STR, false);
    }
    else if (kind == FZ_LINK_GOTO)
    {
        response.addFloat(x).addFloat(y);
    }
}

void MuPdfBridge::processOutline(fz_outline *outline, int level, int index, CmdResponse& response)
{
    int kind = outline->dest.kind;
    int pageNo = 0;
    char* title = NULL;
    char* url = NULL;
    float x = 0;
    float y = 0;
    bool recognized = false;

    DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] Outline item found: %d", level, index, kind);

    if (outline->title)
    {
        title = outline->title;
        DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] Title: %s", level, index, title);
    }
    else
    {
        DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] No title", level, index);
    }

    if (kind == FZ_LINK_NONE)
    {
    	recognized = true;
    	pageNo = -1;
    } else if (kind == FZ_LINK_URI)
    {
        recognized = true;
        if (outline->dest.ld.uri.uri)
        {
            url = outline->dest.ld.uri.uri;
            DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] Target url: %s", level, index, url);
        }
        else
        {
            DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] No target url", level, index);
        }
    }
    else if (outline->dest.kind == FZ_LINK_GOTO)
    {
        recognized = true;
        pageNo = outline->dest.ld.gotor.page;
        fz_page* target = getPage(pageNo, false);
        if (target)
        {
            fz_rect targetBounds = fz_empty_rect;
            fz_bound_page(ctx, target, &targetBounds);

            float targetWidth = targetBounds.x1 - targetBounds.x0;
            float targetHeight = targetBounds.y1 - targetBounds.y0;
            int flags = outline->dest.ld.gotor.flags;

            if ((flags & 0x03) != 0)
            {
                x = outline->dest.ld.gotor.lt.x / targetWidth;
                y = 1.0f - outline->dest.ld.gotor.lt.y / targetHeight;
                DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] Target page: %d %f %f", level, index, pageNo, x, y);
            }
            else
            {
                DEBUG_L(L_DEBUG_OUTLINE, LCTX,
                    "[%d:%d] Unknown page rect: %d %x %f %f", level, index, pageNo, flags, outline->dest.ld.gotor.lt.x, outline->dest.ld.gotor.lt.y);
            }

        }
        else
        {
            DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] Unknown page: %d", level, index, pageNo);
        }
    }
    else
    {
        DEBUG_L(L_DEBUG_OUTLINE, LCTX, "[%d:%d] Unknown kind: %d", level, index, kind);
    }

    if (recognized)
    {
        toResponse(response, level, title, kind, pageNo, url, x, y);
    }
    if (outline->down)
    {
        processOutline(outline->down, level + 1, 0, response);
    }

    if (outline->next)
    {
        processOutline(outline->next, level, index + 1, response);
    }
}
