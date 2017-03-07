#ifndef __CHMFMT_H__
#define __CHMFMT_H__

#include "crengine.h"
#include "lvtinydom.h"

bool DetectCHMFormat(LVStreamRef stream);
bool ImportCHMDocument(LVStreamRef stream, CrDom* doc);

/// Opens CHM container
LVContainerRef LVOpenCHMContainer(LVStreamRef stream);

#endif // __CHMFMT_H__
