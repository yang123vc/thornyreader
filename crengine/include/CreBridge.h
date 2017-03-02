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
