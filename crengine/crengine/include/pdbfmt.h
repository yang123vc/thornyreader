#ifndef PDBFMT_H
#define PDBFMT_H

#include "../include/crsetup.h"
#include "../include/lvtinydom.h"

// creates PDB decoder stream for stream
//LvStreamRef LVOpenPDBStream( LvStreamRef srcstream, int &format );

bool DetectPDBFormat( LvStreamRef stream, doc_format_t & contentFormat );
bool ImportPDBDocument( LvStreamRef & stream, CrDom * doc, doc_format_t & contentFormat );
LvStreamRef GetPDBCoverpage(LvStreamRef stream);


#endif // PDBFMT_H
