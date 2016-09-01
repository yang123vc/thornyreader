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

#define LCTX "EBookDroid.MuPDF.Decoder.Links"
#define L_DEBUG_LINKS true

#define EBD_PAGE_LINK 1
#define EBD_URI_LINK 2
#define EBD_OUTER_PAGE_LINK 3

const char EMPTY_URL[1] = { 0x0 };

void MuPdfBridge::processLinks(int pageNo, CmdResponse& response)
{
    fz_page* page = getPage(pageNo, false);
    if (!page)
    {
        return;
    }

    fz_link* link = fz_load_links(document, page);
    if (!link)
    {
        DEBUG_L(L_DEBUG_LINKS, LCTX, "No links found on page %d", pageNo);
        return;
    }

    do
    {
        if (link->dest.kind == FZ_LINK_URI)
        {
            fz_rect sourceBounds = fz_empty_rect;
            fz_bound_page(document, page, &sourceBounds);

            float sourceWidth = sourceBounds.x1 - sourceBounds.x0;
            float sourceHeight = sourceBounds.y1 - sourceBounds.y0;
            float left = (link->rect.x0 - sourceBounds.x0) / sourceWidth;
            float top = (link->rect.y0 - sourceBounds.y0) / sourceHeight;
            float right = (link->rect.x1 - sourceBounds.x0) / sourceWidth;
            float bottom = (link->rect.y1 - sourceBounds.y0) / sourceHeight;

            DEBUG_L(L_DEBUG_LINKS, LCTX,
                "load: Source rect on page %d: %f %f %f %f / %f %f %f %f", pageNo, link->rect.x0, link->rect.y0, link->rect.x1, link->rect.y1, sourceBounds.x0, sourceBounds.y0, sourceBounds.x1, sourceBounds.y1);

            DEBUG_L(L_DEBUG_LINKS, LCTX, "Uri link found on page %d: %s", pageNo, link->dest.ld.uri.uri);

            char* uri = link->dest.ld.uri.uri;
            response.add((uint16_t) EBD_URI_LINK, 0);
            response.add(uri != NULL ? uri : EMPTY_URL, false);
            response.add(left);
            response.add(top);
            response.add(right);
            response.add(bottom);
        }
        else if (link->dest.kind == FZ_LINK_GOTO)
        {
            int targetNo = link->dest.ld.gotor.page;
            fz_page* target = getPage(targetNo, false);

            if (target)
            {
                fz_rect sourceBounds = fz_empty_rect;
                fz_rect targetBounds = fz_empty_rect;
                fz_bound_page(document, page, &sourceBounds);
                fz_bound_page(document, target, &targetBounds);

                float sourceWidth = sourceBounds.x1 - sourceBounds.x0;
                float sourceHeight = sourceBounds.y1 - sourceBounds.y0;
                float left = (link->rect.x0 - sourceBounds.x0) / sourceWidth;
                float top = (link->rect.y0 - sourceBounds.y0) / sourceHeight;
                float right = (link->rect.x1 - sourceBounds.x0) / sourceWidth;
                float bottom = (link->rect.y1 - sourceBounds.y0) / sourceHeight;

                DEBUG_L(L_DEBUG_LINKS, LCTX,
                    "load: Source rect on page %d: %f %f %f %f / %f %f %f %f", pageNo, link->rect.x0, link->rect.y0, link->rect.x1, link->rect.y1, sourceBounds.x0, sourceBounds.y0, sourceBounds.x1, sourceBounds.y1);

                float targetWidth = targetBounds.x1 - targetBounds.x0;
                float targetHeight = targetBounds.y1 - targetBounds.y0;
                float x = 0;
                float y = 0;
                if ((link->dest.ld.gotor.flags & 0x03) != 0)
                {
                    x = link->dest.ld.gotor.lt.x / targetWidth;
                    y = 1.0f - link->dest.ld.gotor.lt.y / targetHeight;
                }

                DEBUG_L(L_DEBUG_LINKS, LCTX,
                    "load: Target rect on page %d: %x %f %f %f %f / %f %f %f %f", targetNo, link->dest.ld.gotor.flags, link->dest.ld.gotor.lt.x, link->dest.ld.gotor.lt.y, link->dest.ld.gotor.rb.x, link->dest.ld.gotor.rb.y, targetBounds.x0, targetBounds.y0, targetBounds.x1, targetBounds.y1);

                response.add((uint16_t) EBD_PAGE_LINK, (uint16_t) link->dest.ld.gotor.page);
                response.add(left);
                response.add(top);
                response.add(right);
                response.add(bottom);
                response.add(x);
                response.add(y);

                DEBUG_L(L_DEBUG_LINKS, LCTX,
                    "load: Page link found on page %d: %f %f %f %f -> %d %f %f", pageNo, left, top, right, bottom, targetNo, x, y);
            }
            else
            {
                DEBUG_L(L_DEBUG_LINKS, LCTX, "No target link page %d found", targetNo);
            }
        }
        else if (link->dest.kind == FZ_LINK_GOTOR)
        {
            int targetNo = link->dest.ld.gotor.page;
            fz_page* target = getPage(targetNo, false);

            if (target)
            {
                const char* targetFile = link->dest.ld.gotor.file_spec;

                fz_rect sourceBounds = fz_empty_rect;
                fz_bound_page(document, page, &sourceBounds);

                float sourceWidth = sourceBounds.x1 - sourceBounds.x0;
                float sourceHeight = sourceBounds.y1 - sourceBounds.y0;
                float left = (link->rect.x0 - sourceBounds.x0) / sourceWidth;
                float top = (link->rect.y0 - sourceBounds.y0) / sourceHeight;
                float right = (link->rect.x1 - sourceBounds.x0) / sourceWidth;
                float bottom = (link->rect.y1 - sourceBounds.y0) / sourceHeight;

                DEBUG_L(L_DEBUG_LINKS, LCTX,
                    "load: Source rect on page %d: %f %f %f %f / %f %f %f %f", pageNo, link->rect.x0, link->rect.y0, link->rect.x1, link->rect.y1, sourceBounds.x0, sourceBounds.y0, sourceBounds.x1, sourceBounds.y1);

                float x = 0;
                float y = 0;
                if ((link->dest.ld.gotor.flags & 0x03) != 0)
                {
                    x = link->dest.ld.gotor.lt.x;
                    y = -link->dest.ld.gotor.lt.y;
                }

                DEBUG_L(L_DEBUG_LINKS, LCTX,
                    "load: Target rect on page %s:%d: %x %f %f %f %f", targetFile, targetNo, link->dest.ld.gotor.flags, link->dest.ld.gotor.lt.x, link->dest.ld.gotor.lt.y, link->dest.ld.gotor.rb.x, link->dest.ld.gotor.rb.y);

                response.add((uint16_t) EBD_OUTER_PAGE_LINK, (uint16_t) link->dest.ld.gotor.page);
                response.add(left);
                response.add(top);
                response.add(right);
                response.add(bottom);
                response.add(x);
                response.add(y);
                response.add(targetFile != NULL ? targetFile : EMPTY_URL, false);

                DEBUG_L(L_DEBUG_LINKS, LCTX,
                    "load: Remote page link found on page %d: %f %f %f %f -> %s:%d %f %f", pageNo, left, top, right, bottom, targetFile, targetNo, x, y);
            }
            else
            {
                DEBUG_L(L_DEBUG_LINKS, LCTX, "No target link page %d found", targetNo);
            }
        }
        else
        {
            DEBUG_L(L_DEBUG_LINKS, LCTX, "Unknown link kind %d found on page %d", link->dest.kind, pageNo);
        }
        link = link->next;
    } while (link);
}

