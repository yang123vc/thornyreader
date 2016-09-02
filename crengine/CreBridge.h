/*
 * Copyright (C) 2013 The CRE CLI viewer interface Project
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

#ifndef READERA_CREBRIDGE_H
#define READERA_CREBRIDGE_H

#include "StBridge.h"
#include "crengine.h"

class CreBridge : public StBridge
{
private:
    LVDocView* doc_view_;

public:
    CreBridge();
    ~CreBridge();

    void process(CmdRequest& request, CmdResponse& response);

protected:
    void processFonts(CmdRequest& request, CmdResponse& response);
    void processConfig(CmdRequest& request, CmdResponse& response);
    void processOpen(CmdRequest& request, CmdResponse& response);
    void processQuit(CmdRequest& request, CmdResponse& response);
    void processOutline(CmdRequest& request, CmdResponse& response);
    void processPage(CmdRequest& request, CmdResponse& response);
    void processPageRender(CmdRequest& request, CmdResponse& response);
    void processPageByXPath(CmdRequest& request, CmdResponse& response);
    void processPageXPath(CmdRequest& request, CmdResponse& response);
    void processMetadata(CmdRequest& request, CmdResponse& response);
};

#endif //READERA_CREBRIDGE_H
