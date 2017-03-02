#ifndef __CHMFMT_H__
#define __CHMFMT_H__

#include "crsetup.h"
#include "lvtinydom.h"

bool DetectCHMFormat( LVStreamRef stream );
bool ImportCHMDocument( LVStreamRef stream, CrDom * doc);

/// opens CHM container
LVContainerRef LVOpenCHMContainer( LVStreamRef stream );

#endif // __CHMFMT_H__
