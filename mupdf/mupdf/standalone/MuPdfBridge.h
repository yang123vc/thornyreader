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

#ifndef __MUPDF_BRIDGE_H__
#define __MUPDF_BRIDGE_H__

extern "C" {
#include <mupdf/pdf.h>
#include <mupdf/xps.h>
};

#define RES_MUPDF_PWD_WRONG 251
#define RES_MUPDF_PWD_NEED  252
#define RES_MUPDF_OOM       253
#define RES_MUPDF_FAIL      254

#include "StBridge.h"

class MuPdfBridge : public StBridge
{
private:
    char* fileName;
    char* password;

    fz_context *ctx;
    fz_document *document;
    fz_outline *outline;

    uint32_t pageCount;
    fz_page **pages;
    fz_display_list **pageLists;

    int storememory;
    int format;

public:
    MuPdfBridge();
    ~MuPdfBridge();

    void process(CmdRequest& request, CmdResponse& response);

protected:
    void processOpen(CmdRequest& request, CmdResponse& response);
    void processQuit(CmdRequest& request, CmdResponse& response);
    void processPageInfo(CmdRequest& request, CmdResponse& response);
    void processPage(CmdRequest& request, CmdResponse& response);
    void processPageRender(CmdRequest& request, CmdResponse& response);
    void processPageFree(CmdRequest& request, CmdResponse& response);
    void processOutline(CmdRequest& request, CmdResponse& response);
    void processPageText(CmdRequest& request, CmdResponse& response);
    void processFonts(CmdRequest& request, CmdResponse& response);
    void processStorage(CmdRequest& request, CmdResponse& response);
    void processSystemFont(CmdRequest& request, CmdResponse& response);

    fz_page* getPage(uint32_t pageNo, bool decode);

    bool restart();
    void release();

    void resetFonts();
    void setFontFileName(char* ext_Font, uint8_t* fontFileName);

    void processLinks(int pageNo, CmdResponse& response);
    void processOutline(fz_outline *outline, int level, int index, CmdResponse& response);
    void processText(int pageNo, const char* pattern, CmdResponse& response);
};

#endif
