#ifndef WORDFMT_H
#define WORDFMT_H
#if ENABLE_ANTIWORD==1

#include "lvtinydom.h"

// MS WORD format support using AntiWord library
bool DetectWordFormat( LvStreamRef stream );

bool ImportWordDocument( LvStreamRef stream, CrDom * m_doc);


#endif // ENABLE_ANTIWORD==1
#endif // WORDFMT_H
