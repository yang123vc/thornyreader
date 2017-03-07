#ifndef PDBFMT_H
#define PDBFMT_H

#include "crengine.h"
#include "lvtinydom.h"

bool DetectPDBFormat(LVStreamRef stream, doc_format_t& contentFormat);
bool ImportPDBDocument(LVStreamRef& stream, CrDom* doc, doc_format_t& doc_format);
LVStreamRef GetPDBCoverpage(LVStreamRef stream);

#endif // PDBFMT_H
