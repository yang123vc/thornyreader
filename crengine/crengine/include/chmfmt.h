#ifndef CHMFMT_H
#define CHMFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"

#if CHM_SUPPORT_ENABLED==1

bool DetectCHMFormat( LvStreamRef stream );
bool ImportCHMDocument( LvStreamRef stream, CrDom * doc);

/// opens CHM container
LVContainerRef LVOpenCHMContainer( LvStreamRef stream );

#endif

#endif // CHMFMT_H
