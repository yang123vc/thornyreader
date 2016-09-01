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

#define FORMAT_PDF 1
#define FORMAT_XPS 2

#define LCTX "EBookDroid.MuPDF.Decoder"
#define L_DEBUG false

MuPdfBridge::MuPdfBridge() : StBridge("EBookDroid.MuPDF")
{
    fileName = NULL;
    password = NULL;
    ctx = NULL;
    document = NULL;
    outline = NULL;
    pageCount = 0;
    pages = NULL;
    pageLists = NULL;
    storememory = 64 * 1024 * 1024;
    format = 0;
    resetFonts();
}

MuPdfBridge::~MuPdfBridge()
{
    if (outline != NULL)
    {
        fz_free_outline(ctx, outline);
        outline = NULL;
    }

    release();

    if (fileName)
    {
        free(fileName);
    }
    if (password)
    {
        free(password);
    }
}

void MuPdfBridge::process(CmdRequest& request, CmdResponse& response)
{
    response.reset();

    request.print(LCTX);

    switch (request.cmd)
    {
    case CMD_REQ_OPEN:
        processOpen(request, response);
        break;
    case CMD_REQ_PAGE_INFO:
        processPageInfo(request, response);
        break;
    case CMD_REQ_PAGE:
        processPage(request, response);
        break;
    case CMD_REQ_PAGE_RENDER:
        processPageRender(request, response);
        break;
    case CMD_REQ_PAGE_FREE:
        processPageFree(request, response);
        break;
    case CMD_REQ_PAGE_TEXT:
        processPageText(request, response);
        break;
    case CMD_REQ_OUTLINE:
        processOutline(request, response);
        break;
    case CMD_REQ_QUIT:
        processQuit(request, response);
        break;
    case CMD_REQ_PDF_FONTS:
        processFonts(request, response);
        break;
    case CMD_REQ_PDF_SYSTEM_FONT:
        processSystemFont(request, response);
        break;
    case CMD_REQ_PDF_STORAGE:
        processStorage(request, response);
        break;
    default:
        ERROR_L(LCTX, "Unknown request: %d", request.cmd);
        response.result = RES_UNKNOWN_CMD;
        break;
    }

    response.print(LCTX);
}

void MuPdfBridge::processQuit(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_QUIT;
}

void MuPdfBridge::processStorage(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PDF_STORAGE;
    if (request.dataCount == 0)
    {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    uint32_t storageSize = 0;
    if (!CmdDataIterator(request.first).integer(&storageSize).isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    this->storememory = storageSize * 1024 * 1024;

    INFO_L(LCTX, "Storage size : %d MB", storageSize);
}

void MuPdfBridge::processOpen(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_OPEN;
    if (request.dataCount == 0)
    {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    uint32_t fileType = 0;
    uint8_t* fileName = NULL;
    uint32_t pwdlen = 0;
    uint8_t* password = NULL;
    uint32_t pageNo = 0;

    CmdDataIterator iter(request.first);
    iter.integer(&fileType).required(&fileName).integer(&pageNo).optional(&password, &pwdlen);

    if (!iter.isValid())
    {
        ERROR_L(LCTX, "Bad request data: %u %u %u %08X %u %p %u %p", request.cmd, request.dataCount, iter.getCount(),
            iter.getErrors(), fileType, fileName, pageNo, password);
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    if (this->fileName)
    {
        free(this->fileName);
    }
    this->fileName = strdup((const char*) fileName);
    if (this->password)
    {
        free(this->password);
    }
    this->password = password != NULL ? strdup((const char*) password) : NULL;

    if (fileType < FORMAT_PDF || fileType > FORMAT_XPS)
    {
        ERROR_L(LCTX, "Bad file type: %u", fileType);
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    this->format = fileType;

    if (ctx == NULL)
    {
        DEBUG_L(L_DEBUG, LCTX, "Creating context: storememory = %d", storememory);
        ctx = fz_new_context(NULL, NULL, storememory);
        if (!ctx)
        {
            ERROR_L(LCTX, "Out of Memory");
            response.result = RES_MUPDF_OOM;
            return;
        }
    }

    if (document == NULL)
    {
        INFO_L(LCTX, "Opening document: %d %s", format, fileName);

        fz_try(ctx)
                {
                    if (format == FORMAT_XPS)
                    {
                        document = (fz_document*) xps_open_document(ctx, (char*) fileName);
                    }
                    else
                    {
                        document = (fz_document*) pdf_open_document(ctx, (char*) fileName);
                    }
                }fz_catch(ctx)
        {
            ERROR_L(LCTX, "Opening document failed: %s", (char* ) fz_caught(ctx));
            response.result = RES_MUPDF_FAIL;
            response.add(fz_caught(ctx), true);
            return;
        }

        if (fz_needs_password(document))
        {
            DEBUG_L(L_DEBUG, LCTX, "Document required a password: %s", this->password);
            if (this->password && strlen(this->password))
            {
                int ok = fz_authenticate_password(document, this->password);
                if (!ok)
                {
                    ERROR_L(LCTX, "Wrong password given");
                    response.result = RES_MUPDF_PWD_WRONG;
                    return;
                }
            }
            else
            {
                ERROR_L(LCTX, "Document needs a password!");
                response.result = RES_MUPDF_PWD_NEED;
                return;
            }
        }

        fz_try(ctx)
        {

                    pageCount = fz_count_pages(document);

                    DEBUG_L(L_DEBUG, LCTX, "Document pages: %d", pageCount);

                    pages = (fz_page**) calloc(pageCount, sizeof(fz_page*));
                    pageLists = (fz_display_list**) calloc(pageCount, sizeof(fz_display_list*));

                }fz_catch(ctx)
        {
            ERROR_L(LCTX, "Counting pages failed: %s", (char* ) fz_caught(ctx));
            response.result = RES_MUPDF_FAIL;
            response.add(fz_caught(ctx), true);
            return;
        }
    }

    response.add(pageCount);

    if (pageCount > 0)
    {
        if (pageNo < pageCount)
        {
            getPage(pageNo, true);
        }
    }
}

void MuPdfBridge::processPageInfo(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PAGE_INFO;
    if (request.dataCount == 0)
    {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    uint32_t pageNo = 0;
    if (!CmdDataIterator(request.first).integer(&pageNo).isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (document == NULL)
    {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    fz_page *page = getPage(pageNo, false);
    if (!page)
    {
        ERROR_L(LCTX, "No page %d found", pageNo);
        response.result = RES_MUPDF_FAIL;
        return;
    }

    fz_try(ctx)
            {
                fz_rect bounds = fz_empty_rect;
                fz_bound_page(document, page, &bounds);

                float data[2];
                data[0] = fabs(bounds.x1 - bounds.x0);
                data[1] = fabs(bounds.y1 - bounds.y0);

                response.add(2, data, true);

            }fz_catch(ctx)
    {
        ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
        response.result = RES_MUPDF_FAIL;
        return;
    }
}

void MuPdfBridge::processPage(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PAGE;
    if (request.dataCount == 0)
    {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    uint32_t pageNumber = 0;
    CmdDataIterator iter(request.first);
    if (!iter.integer(&pageNumber).isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (document == NULL)
    {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    if (pageNumber >= pageCount)
    {
        ERROR_L(LCTX, "Bad page index: %d", pageNumber);
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    fz_page* p = getPage(pageNumber, true);

    if (p == NULL)
    {
        response.result = RES_MUPDF_FAIL;
        return;
    }

    processLinks(pageNumber, response);
}

void MuPdfBridge::processPageFree(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PAGE_FREE;
    if (request.dataCount == 0)
    {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    uint32_t pageNumber = 0;
    CmdDataIterator iter(request.first);
    if (!iter.integer(&pageNumber).isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (document == NULL)
    {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    if (pageNumber >= pageCount)
    {
        ERROR_L(LCTX, "Bad page index: %d", pageNumber);
        response.result = RES_BAD_REQ_DATA;
        return;
    }


    if (pageLists[pageNumber]) {
        fz_try(ctx)
        {
        	fz_drop_display_list(ctx, pageLists[pageNumber]);
        }fz_catch(ctx)
        {
            ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
        }
    	pageLists[pageNumber] = NULL;
    }

    if (pages[pageNumber]) {
        fz_try(ctx)
        {
        	fz_free_page(document, pages[pageNumber]);
        }fz_catch(ctx)
        {
            ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
        }
    	pages[pageNumber] = NULL;
    }
}

void MuPdfBridge::processPageRender(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PAGE_RENDER;
    if (request.dataCount == 0)
    {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (document == NULL || pages == NULL)
    {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    uint32_t pageNumber;
    uint32_t*viewboxarr;
    float *matrix;
    uint16_t nightmode;
    uint16_t slowcmyk;

    CmdDataIterator iter(request.first);
    iter.integer(&pageNumber).required(&viewboxarr, 4).required(&matrix, 6).words(&nightmode, &slowcmyk);

    if (!iter.isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    fz_page* page = getPage(pageNumber, true);
    if (!page)
    {
        response.result = RES_MUPDF_FAIL;
        return;
    }

    fz_matrix ctm = fz_identity;
    ctm.a = matrix[0];
    ctm.b = matrix[1];
    ctm.c = matrix[2];
    ctm.d = matrix[3];
    ctm.e = matrix[4];
    ctm.f = matrix[5];

    fz_rect viewbox;
    viewbox.x0 = viewboxarr[0];
    viewbox.y0 = viewboxarr[1];
    viewbox.x1 = viewboxarr[2];
    viewbox.y1 = viewboxarr[3];

    int targetWidth = viewbox.x1 - viewbox.x0;
    int targetHeight = viewbox.y1 - viewbox.y0;
    int size = (targetWidth) * (targetHeight) * 4;

    CmdData* resp = new CmdData(size);
    unsigned char* localpixels = resp->external;

    //add check for night mode and set global variable accordingly
    ctx->ebookdroid_nightmode = nightmode;

    //add check for slowcmyk mode and set global variable accordingly
    ctx->ebookdroid_slowcmyk = slowcmyk;

    fz_device *dev = NULL;
    fz_pixmap *pixmap = NULL;

    fz_try(ctx)
            {
                pixmap = fz_new_pixmap_with_data(ctx, fz_device_rgb(ctx), targetWidth, targetHeight, localpixels);

                fz_clear_pixmap_with_value(ctx, pixmap, 0xff);

                dev = fz_new_draw_device(ctx, pixmap);

                fz_run_display_list(pageLists[pageNumber], dev, &ctm, &viewbox, NULL);

                response.add(resp);
            }fz_always(ctx)
            {
                fz_free_device(dev);
                fz_drop_pixmap(ctx, pixmap);
            }fz_catch(ctx)
    {
        ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
        response.result = RES_MUPDF_FAIL;
        delete resp;
    }
}

void MuPdfBridge::processOutline(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_OUTLINE;

    if (document == NULL)
    {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    if (outline == NULL)
    {
        outline = fz_load_outline(document);
    }

    response.add((uint32_t) 0);
    processOutline(outline, 0, 0, response);
}

void MuPdfBridge::processPageText(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_PAGE_TEXT;

    if (document == NULL)
    {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    uint32_t pageNo;
    uint8_t* pattern = NULL;

    CmdDataIterator iter(request.first);
    if (!iter.integer(&pageNo).isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    DEBUG_L(L_DEBUG, LCTX, "Retrieve page text: %d", pageNo);

    processText((int) pageNo, (const char*) pattern, response);
}

fz_page* MuPdfBridge::getPage(uint32_t pageNo, bool decode)
{
    if (pages[pageNo] == NULL)
    {
        fz_try(ctx)
                {
                    pages[pageNo] = fz_load_page(document, pageNo);
                }fz_catch(ctx)
        {
            ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
            return NULL;
        }
    }

    if (decode && pageLists[pageNo] == NULL)
    {
        fz_device *dev = NULL;
        fz_try(ctx)
                {
                    pageLists[pageNo] = fz_new_display_list(ctx);
                    dev = fz_new_list_device(ctx, pageLists[pageNo]);
                    fz_matrix m = fz_identity;
                    fz_run_page(document, pages[pageNo], dev, &m, NULL);
                }fz_always(ctx)
                {
                    fz_free_device(dev);
                }fz_catch(ctx)
        {
            ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
            fz_drop_display_list(ctx, pageLists[pageNo]);
            pageLists[pageNo] = NULL;
        }
    }

    return pages[pageNo];
}

bool MuPdfBridge::restart()
{
    release();

    DEBUG_L(L_DEBUG, LCTX, "Creating context: storememory = %d", storememory);
    ctx = fz_new_context(NULL, NULL, storememory);
    if (!ctx)
    {
        return false;
    }

    DEBUG_L(L_DEBUG, LCTX, "Opening document: %s", fileName);

    fz_try(ctx)
            {
                if (format == FORMAT_XPS)
                {
                    document = (fz_document*) xps_open_document(ctx, fileName);
                }
                else
                {
                    document = (fz_document*) pdf_open_document(ctx, fileName);
                }

                DEBUG_L(L_DEBUG, LCTX, "Document pages: %d", pageCount);

                pages = (fz_page**) calloc(pageCount, sizeof(fz_page*));
                pageLists = (fz_display_list**) calloc(pageCount, sizeof(fz_display_list*));

            }fz_catch(ctx)
    {
        ERROR_L(LCTX, "%s", (char* ) fz_caught(ctx));
        return false;
    }

    if (fz_needs_password(document))
    {
        DEBUG_L(L_DEBUG, LCTX, "Document required a password: %s", password);
        if (password && strlen((char*) password))
        {
            int ok = fz_authenticate_password(document, (char*) password);
            if (!ok)
            {
                ERROR_L(LCTX, "Wrong password given");
                return false;
            }
        }
        else
        {
            ERROR_L(LCTX, "Document needs a password!");
            return false;
        }
    }

    return true;
}

void MuPdfBridge::release()
{
    if (pageLists != NULL)
    {
        int i;
        for (i = 0; i < pageCount; i++)
        {
            if (pageLists[i] != NULL)
            {
            	fz_drop_display_list(ctx, pageLists[i]);
            }
        }
        free(pageLists);
        pageLists = NULL;
    }
    if (pages != NULL)
    {
        int i;
        for (i = 0; i < pageCount; i++)
        {
            if (pages[i] != NULL)
            {
                fz_free_page(document, pages[i]);
            }
        }
        free(pages);
        pages = NULL;
    }
    if (document)
    {
        fz_close_document(document);
        document = NULL;
    }
    if (ctx)
    {
        fz_flush_warnings(ctx);
        fz_free_context(ctx);
        ctx = NULL;
    }
}

