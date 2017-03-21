/*******************************************************

   CoolReader Engine

   lvmemman.cpp:  memory manager implementation

   (c) Vadim Lopatin, 2000-2006
   This source code is distributed under the terms of
   GNU General Public License
   See LICENSE file for details

*******************************************************/

#include "include/lvmemman.h"
#include "include/lvref.h"
#include "include/lvtinydom.h"

void crFatalError(int code, const char* errorText)
{
   CRLog::error("FATAL ERROR! CODE:%d: MSG:%s", code, errorText);
   //exit(errorCode);
}

ref_count_rec_t ref_count_rec_t::null_ref(NULL);