/** \file lvmemman.h
    \brief Fast memory manager implementation

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    This source code is distributed under the terms of
    GNU General Public License.

    See LICENSE file for details.

*/

#ifndef __LV_MEM_MAN_H_INCLUDED__
#define __LV_MEM_MAN_H_INCLUDED__

#include "crengine.h"
#include "lvtypes.h"

/// fatal error function calls fatal error handler
void crFatalError( int code, const char * errorText );
inline void crFatalError() { crFatalError( -1, "Unknown fatal error" ); }

/// typed realloc with result check (size is counted in T), fatal error if failed
template <typename T> T * cr_realloc( T * ptr, size_t newSize ) {
    T * newptr = reinterpret_cast<T*>(realloc(ptr, sizeof(T)*newSize));
    if ( newptr )
        return newptr;
    free(ptr); // to bypass cppcheck warning
    crFatalError(-2, "realloc failed");
    return NULL;
}

#endif //__LV_MEM_MAN_H_INCLUDED__
