#ifndef WORDFMT_H
#define WORDFMT_H

#if ENABLE_ANTIWORD==1

#include "lvtinydom.h"

// MS WORD format support using AntiWord library
bool DetectWordFormat(LVStreamRef stream);

bool ImportWordDocument(LVStreamRef stream, CrDom* dom);

#endif // ENABLE_ANTIWORD==1

#endif // WORDFMT_H
