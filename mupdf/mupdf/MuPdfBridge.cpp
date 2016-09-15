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
#include <signal.h>

#include "StLog.h"
#include "StProtocol.h"
#include "StSocket.h"
#include "StConfig.h"
#include "MuPdfBridge.h"
#include "abitmap-utils.h"

#define LCTX "MuPdfBridge"
#define L_DEBUG false

static int config_format = 0;
static int config_invert_images = 0;

CmdRequest* current = NULL;
sighandler_t defaultHandler = SIG_ERR;

void sig_handler(int signo)
{
    ERROR_L(LCTX, "received signal %d", signo);
    if (current)
    {
        ERROR_L(LCTX, "Request: %u", current->cmd);
        CmdData* data;
        for (data = current->first; data != NULL; data = data->nextData)
        {
            ERROR_L(LCTX, "Data: %p %u %08x", data, data->type, data->value.value32);
        }
    }
    exit(-1);
}

MuPdfBridge::MuPdfBridge() : StBridge("MuPdfBridge")
{
    fd = -1;
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

    // if ((defaultHandler = signal(SIGSEGV, sig_handler)) == SIG_ERR)
    // {
    //     ERROR_L(LCTX, "!!! can't catch SIGSEGV");
    // }
}

MuPdfBridge::~MuPdfBridge()
{
    if (outline != NULL)
    {
        fz_drop_outline(ctx, outline);
        outline = NULL;
    }

    release();

    if (password)
    {
        free(password);
    }
}

void MuPdfBridge::process(CmdRequest& request, CmdResponse& response)
{
    response.reset();

    // current = &request;

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
    case CMD_REQ_PDF_GET_MISSED_FONTS:
        processGetMissedFonts(request, response);
        break;
    case CMD_REQ_PDF_STORAGE:
        processStorage(request, response);
        break;
    case CMD_REQ_SET_CONFIG:
        processConfig(request, response);
        break;
    case CMD_REQ_SMART_CROP:
        processSmartCrop(request, response);
        break;
    default:
        ERROR_L(LCTX, "Unknown request: %d", request.cmd);
        response.result = RES_UNKNOWN_CMD;
        break;
    }

    // current = NULL;

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
    if (!CmdDataIterator(request.first).getInt(&storageSize).isValid())
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
  
    uint8_t* socketName = NULL;
    uint8_t* file_name = NULL;
    uint8_t* password = NULL;

    CmdDataIterator iter(request.first);
    iter.getByteArray(&socketName).getByteArray(&file_name)
            .optionalByteArray(&password, NULL);

    if (!iter.isValid())
    {
        ERROR_L(LCTX, "Bad request data: %u %u %u %p %p",
                request.cmd, request.dataCount, iter.getCount(),
                socketName, password);
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    DEBUG_L(L_DEBUG, LCTX, "Socket name: %s", socketName);
    StSocketConnection connection((const char*)socketName);
    if (!connection.isValid())
    {
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    DEBUG_L(L_DEBUG, LCTX, "client: connected!");
    bool received = connection.receiveFileDescriptor(fd);

    DEBUG_L(L_DEBUG, LCTX, "File descriptor:  %d", fd);
    if (!received)
    {
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    if (this->password)
    {
        free(this->password);
    }
    this->password = password != NULL ? strdup((const char*) password) : NULL;

    if (config_format < FORMAT_PDF || config_format > FORMAT_XPS)
    {
        ERROR_L(LCTX, "Bad file type: %u", config_format);
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    this->format = config_format;

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
        ctx->ebookdroid_linearized_load = 0;
        ctx->ebookdroid_ignore_most_errors = 1;
        ctx->ebookdroid_hasPassword = (this->password && strlen(this->password));
    }

    if (document == NULL)
    {
        INFO_L(LCTX, "Opening document: %d %d", format, fd);

        INFO_L(LCTX, ctx->ebookdroid_hasPassword ? "Password present" : "No password");
        fz_try(ctx)
                {
                    if (format == FORMAT_XPS)
                    {
                        document = (fz_document*) xps_open_document_with_stream(ctx, fz_open_fd(ctx, dup(fd)));
                    }
                    else
                    {
                        document = (fz_document*) pdf_open_document_with_stream(ctx, fz_open_fd(ctx, dup(fd)));
                    }
                }fz_catch(ctx)
        {
            const char* msg = fz_caught_message(ctx);
            ERROR_L(LCTX, "Opening document failed: %s", msg);
            response.result = RES_MUPDF_FAIL;
            response.addIpcString(msg, true);
            return;
        }

        if (fz_needs_password(ctx, document))
        {
            DEBUG_L(L_DEBUG, LCTX, "Document required a password: %s", this->password);
            if (this->password && strlen(this->password))
            {
                int ok = fz_authenticate_password(ctx, document, this->password);
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

                    pageCount = fz_count_pages(ctx, document);

                    DEBUG_L(L_DEBUG, LCTX, "Document pages: %d", pageCount);

                    pages = (fz_page**) calloc(pageCount, sizeof(fz_page*));
                    pageLists = (fz_display_list**) calloc(pageCount, sizeof(fz_display_list*));

                }fz_catch(ctx)
        {
            const char* msg = fz_caught_message(ctx);
            ERROR_L(LCTX, "Counting pages failed: %s", msg );
            response.result = RES_MUPDF_FAIL;
            response.addIpcString(msg, true);
            return;
        }
    }

    response.addInt(pageCount);
	/*
    if (document) {
    	char doc_author[256];
    	fz_lookup_metadata(ctx, document, FZ_META_INFO_AUTHOR, doc_author, 256);

    	char doc_title[256];
    	fz_lookup_metadata(ctx, document, FZ_META_INFO_TITLE, doc_title, 256);

        INFO_L(LCTX, "Document author: %s", doc_author );
        INFO_L(LCTX, "Document title: %s", doc_title );
    }
	*/
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
    if (!CmdDataIterator(request.first).getInt(&pageNo).isValid())
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
                fz_bound_page(ctx, page, &bounds);

                float data[2];
                data[0] = fabs(bounds.x1 - bounds.x0);
                data[1] = fabs(bounds.y1 - bounds.y0);

                response.addFloatArray(2, data, true);

            }fz_catch(ctx)
    {
        const char* msg = fz_caught_message(ctx);
        ERROR_L(LCTX, "%s", msg);
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
    if (!iter.getInt(&pageNumber).isValid())
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
    if (!iter.getInt(&pageNumber).isValid())
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
            const char* msg = fz_caught_message(ctx);
            ERROR_L(LCTX, "%s", msg);
        }
    	pageLists[pageNumber] = NULL;
    }

    if (pages[pageNumber]) {
        fz_try(ctx)
        {
        	fz_drop_page(ctx, pages[pageNumber]);
        }fz_catch(ctx)
        {
            const char* msg = fz_caught_message(ctx);
            ERROR_L(LCTX, "%s", msg);
        }
    	pages[pageNumber] = NULL;
    }
}

/*
	fz_matrix is a a row-major 3x3 matrix used for representing
	transformations of coordinates throughout MuPDF.

	Since all points reside in a two-dimensional space, one vector
	is always a constant unit vector; hence only some elements may
	vary in a matrix. Below is how the elements map between
	different representations.

	a - scale width
	b - ?
	c - ?
	d - scale height
	e - translate left
	f - translate top

	/ a b 0 \
	| c d 0 | normally represented as [ a b c d e f ].
	\ e f 1 /

    typedef struct fz_matrix_s fz_matrix;
    struct fz_matrix_s
    {
        float a, b, c, d, e, f;
    };
*/
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

    uint32_t page_index;
    uint32_t width;
    uint32_t height;
    float *matrix;

    CmdDataIterator iter(request.first);
    iter.getInt(&page_index).getInt(&width).getInt(&height).getFloatArray(&matrix, 6);

    if (!iter.isValid())
    {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    fz_page* page = getPage(page_index, true);

    if (!page || !pageLists[page_index])
    {
        response.result = RES_MUPDF_PAGE_CAN_NOT_BE_RENDERED;
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
    viewbox.x0 = 0;
    viewbox.y0 = 0;
    viewbox.x1 = width;
    viewbox.y1 = height;

    int size = (width) * (height) * 4;

    CmdData* resp = new CmdData();
    unsigned char* pixels = resp->newByteArray(size);

    //add check for night mode and set global variable accordingly
    ctx->ebookdroid_nightmode = config_invert_images;

    //add check for slowcmyk mode and set global variable accordingly
    ctx->ebookdroid_slowcmyk = HARDCONFIG_MUPDF_SLOW_CMYK;

    // clear counter
    ctx->ebookdroid_setcolor_per_page = 0;

    fz_device *dev = NULL;
    fz_pixmap *pixmap = NULL;

    fz_try(ctx) {
		pixmap = fz_new_pixmap_with_data(ctx, fz_device_rgb(ctx), width, height, pixels);

                fz_clear_pixmap_with_value(ctx, pixmap, 0xff);

                dev = fz_new_draw_device(ctx, pixmap);

                fz_run_display_list(ctx, pageLists[page_index], dev, &ctm, &viewbox, NULL);

                response.addData(resp);
            }fz_always(ctx)
            {
                fz_drop_device(ctx, dev);
                fz_drop_pixmap(ctx, pixmap);
            }fz_catch(ctx)
    {
        const char* msg = fz_caught_message(ctx);
        ERROR_L(LCTX, "%s", msg);
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
        outline = fz_load_outline(ctx, document);
    }

    response.addInt((uint32_t) 0);
	
    if (outline)
    {
    	processOutline(outline, 0, 0, response);
    }
    else
    {
        //ERROR_L(LCTX, "No outline");
    }
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
    if (!iter.getInt(&pageNo).isValid())
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
	if (pageNo >= pageCount || pageNo < 0)
	{
		ERROR_L(LCTX, "Invalid page number: %d from %d", pageNo, pageCount);
		return NULL;
	}
    if (pages[pageNo] == NULL)
    {
        fz_try(ctx)
                {
                    pages[pageNo] = fz_load_page(ctx, document, pageNo);
                }fz_catch(ctx)
        {
            const char* msg = fz_caught_message(ctx);
            ERROR_L(LCTX, "%s", msg);
            if (ctx->ebookdroid_linearized_load) {
            	ERROR_L(LCTX, "Try to reopen in non-linearized mode");
            	ctx->ebookdroid_linearized_load = 0;
            	restart();
            	return getPage(pageNo, decode);
            }
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
                    fz_run_page(ctx, pages[pageNo], dev, &m, NULL);
                }fz_always(ctx)
                {
                    fz_drop_device(ctx, dev);
                }fz_catch(ctx)
        {
            const char* msg = fz_caught_message(ctx);
            ERROR_L(LCTX, "%s", msg);
            if (ctx->ebookdroid_linearized_load) {
            	ERROR_L(LCTX, "Try to reopen in non-linearized mode");
            	ctx->ebookdroid_linearized_load = 0;
            	restart();
            	return getPage(pageNo, decode);
            } else
            {
            	fz_drop_display_list(ctx, pageLists[pageNo]);
            	pageLists[pageNo] = NULL;
            }
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

    ctx->ebookdroid_ignore_most_errors = 1;

    fz_try(ctx)
            {
                if (format == FORMAT_XPS)
                {
                    document = (fz_document*) xps_open_document_with_stream(ctx, fz_open_fd(ctx, dup(fd)));
                }
                else
                {
                    document = (fz_document*) pdf_open_document_with_stream(ctx, fz_open_fd(ctx, dup(fd)));
                }

                DEBUG_L(L_DEBUG, LCTX, "Document pages: %d", pageCount);

                pages = (fz_page**) calloc(pageCount, sizeof(fz_page*));
                pageLists = (fz_display_list**) calloc(pageCount, sizeof(fz_display_list*));

            }fz_catch(ctx)
    {
        const char* msg = fz_caught_message(ctx);
        ERROR_L(LCTX, "%s", msg);
        return false;
    }

    if (fz_needs_password(ctx, document))
    {
        DEBUG_L(L_DEBUG, LCTX, "Document required a password: %s", password);
        if (password && strlen((char*) password))
        {
            int ok = fz_authenticate_password(ctx, document, (char*) password);
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
                fz_drop_page(ctx, pages[i]);
            }
        }
        free(pages);
        pages = NULL;
    }
    if (document)
    {
        fz_drop_document(ctx, document);
        document = NULL;
    }
    if (ctx)
    {
        fz_flush_warnings(ctx);
        fz_drop_context(ctx);
        ctx = NULL;
    }
}

void MuPdfBridge::processConfig(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_SET_CONFIG;
    CmdDataIterator iter(request.first);

    while (iter.hasNext()) {
        uint32_t key;
        uint8_t* temp_val;
        iter.getInt(&key).getByteArray(&temp_val);
        if (!iter.isValid()) {
            response.result = RES_BAD_REQ_DATA;
            return;
        }
        const char* val = reinterpret_cast<const char*>(temp_val);

        if (key == CONFIG_MUPDF_FORMAT) {
            int int_val = atoi(val);
            config_format = int_val;
        } else if (key == CONFIG_MUPDF_INVERT_IMAGES) {
            int int_val = atoi(val);
            config_invert_images = int_val;
        } else {
            ERROR_L(LCTX, "processConfig unknown key: key=%d, val=%s", key, val);
        }
    }
    response.addInt(pageCount);
}

void MuPdfBridge::processSmartCrop(CmdRequest& request, CmdResponse& response)
{
    response.cmd = CMD_RES_SMART_CROP;
    if (request.dataCount == 0) {
        ERROR_L(LCTX, "No request data found");
        response.result = RES_BAD_REQ_DATA;
        return;
    }
    if (document == NULL || pages == NULL) {
        ERROR_L(LCTX, "Document not yet opened");
        response.result = RES_DUP_OPEN;
        return;
    }

    uint32_t page_index;
    float origin_w;
    float origin_h;
    float slice_l;
    float slice_t;
    float slice_r;
    float slice_b;
    CmdDataIterator iter(request.first);
    iter.getInt(&page_index).getFloat(&origin_w).getFloat(&origin_h)
            .getFloat(&slice_l).getFloat(&slice_t).getFloat(&slice_r).getFloat(&slice_b);
    if (!iter.isValid()) {
        ERROR_L(LCTX, "Bad request data");
        response.result = RES_BAD_REQ_DATA;
        return;
    }

    float slice_w = slice_r - slice_l;
    float slice_h = slice_b - slice_t;

    fz_page* page = getPage(page_index, true);
    if (!page) {
        response.result = RES_MUPDF_FAIL;
        return;
    }

    //INFO_L(LCTX, "origin_w=%f origin_h=%f slices[%f %f]",	origin_w, origin_h, slice_l, slice_r);
    fz_matrix mat = fz_identity;
    fz_pre_scale(&mat, (float) SMART_CROP_W / origin_w, (float) SMART_CROP_H / origin_h);
    // Manual translating (fz_pre_translate and fz_translate works not as expected)
    mat.e = -slice_l * (float) SMART_CROP_W; // Translate left
    mat.f = -slice_t * (float) SMART_CROP_H; // Translate top
    fz_pre_scale(&mat, 1 / (slice_w), 1 / (slice_h));
    // Manual applying scale to previous translate (fz_pre_scale and fz_scale ignores translation)
    mat.e *= 1 / (slice_w); // Scale translate left
    mat.f *= 1 / (slice_h); // Scale translate top
    //INFO_L(LCTX, "fz_matrix[a=%f b=%f c=%f d=%f e=%f f=%f]", mat.a, mat.b, mat.c, mat.d, mat.e, mat.f);

    fz_rect viewbox;
    viewbox.x0 = 0;
    viewbox.y0 = 0;
    viewbox.x1 = SMART_CROP_W;
    viewbox.y1 = SMART_CROP_H;

    int size = SMART_CROP_W * SMART_CROP_H * 4;

    uint8_t* pixels = (uint8_t*) malloc(size);

    ctx->ebookdroid_nightmode = 0;
    ctx->ebookdroid_slowcmyk = 0;
    ctx->ebookdroid_setcolor_per_page = 0;

    fz_device* device = NULL;
    fz_pixmap* pixmap = NULL;

    fz_try(ctx) {
        pixmap = fz_new_pixmap_with_data(ctx, fz_device_rgb(ctx), viewbox.x1, viewbox.y1, pixels);

        fz_clear_pixmap_with_value(ctx, pixmap, 0xff);

        device = fz_new_draw_device(ctx, pixmap);

        fz_run_display_list(ctx, pageLists[page_index], device, &mat, &viewbox, NULL);

        float smart_crop[4];
        CalcBitmapSmartCrop(smart_crop, pixels, SMART_CROP_W, SMART_CROP_H,
                            slice_l, slice_t, slice_r, slice_b);
        response.addFloatArray(4, smart_crop, true);
    } fz_always(ctx) {
        fz_drop_device(ctx, device);
        fz_drop_pixmap(ctx, pixmap);
    } fz_catch(ctx) {
        const char* msg = fz_caught_message(ctx);
        ERROR_L(LCTX, "%s", msg);
        response.result = RES_MUPDF_FAIL;
        if (pixels) {
            free(pixels);
            pixels = NULL;
        }
    }
    if (pixels) {
        free(pixels);
    }
}