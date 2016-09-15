/*
 * The copyright in this software is being made available under the 2-clauses
 * BSD License, included below. This software may be subject to other third
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2014, Andrei Komarovskikh, Alex Kasatkin
 * Copyright (c) 2002-2014, Universite catholique de Louvain (UCL), Belgium
 * Copyright (c) 2002-2014, Professor Benoit Macq
 * Copyright (c) 2001-2003, David Janssens
 * Copyright (c) 2002-2003, Yannick Verschueren
 * Copyright (c) 2003-2007, Francois-Olivier Devaux
 * Copyright (c) 2003-2014, Antonin Descampe
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2007, Jonathan Ballard <dzonatas@dzonux.net>
 * Copyright (c) 2007, Callum Lerwick <seg@haxxed.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef OPJ_COMPAT_H
#define OPJ_COMPAT_H

/*
==========================================================
   Compiler directives
==========================================================
*/

/*
The inline keyword is supported by C99 but not by C90.
Most compilers implement their own version of this keyword ...
*/
#ifndef INLINE
    #if defined(_MSC_VER)
        #define INLINE __forceinline
    #elif defined(__GNUC__)
        #define INLINE __inline__
    #elif defined(__MWERKS__)
        #define INLINE inline
    #else
        /* add other compilers here ... */
        #define INLINE
    #endif /* defined(<Compiler>) */
#endif /* INLINE */

/* deprecated attribute */
#ifdef __GNUC__
    #define OPJ_DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
    #define OPJ_DEPRECATED(func) __declspec(deprecated) func
#else
    #pragma message("WARNING: You need to implement DEPRECATED for this compiler")
    #define OPJ_DEPRECATED(func) func
#endif

/* Ignore GCC attributes if this is not GCC */
#ifndef __GNUC__
    #define __attribute__(x) /* __attribute__(x) */
#endif


/* Are restricted pointers available? (C99) */
#if (__STDC_VERSION__ != 199901L)
    /* Not a C99 compiler */
    #ifdef __GNUC__
        #define restrict __restrict__
    #else
        #define restrict /* restrict */
    #endif
#endif

/* MSVC before 2013 and Borland C do not have lrintf */
#if defined(_MSC_VER) && (_MSC_VER < 1800) || defined(__BORLANDC__)
    static INLINE long lrintf(float f){
        #ifdef _M_X64
            return (long)((f>0.0f) ? (f + 0.5f):(f -0.5f));
        #else
            int i;

            _asm{
                fld f
                fistp i
            };

            return i;
        #endif
    }
#endif

#if defined(OPJ_STATIC) || !defined(_WIN32)
/* http://gcc.gnu.org/wiki/Visibility */
    #if __GNUC__ >= 4
        #define OPJ_API    __attribute__ ((visibility ("default")))
        #define OPJ_LOCAL  __attribute__ ((visibility ("hidden")))
    #else
        #define OPJ_API
        #define OPJ_LOCAL
    #endif

    #define OPJ_CALLCONV
#else
    #define OPJ_CALLCONV __stdcall

/*
 The following ifdef block is the standard way of creating macros which make exporting
 from a DLL simpler. All files within this DLL are compiled with the OPJ_EXPORTS
 symbol defined on the command line. this symbol should not be defined on any project
 that uses this DLL. This way any other project whose source files include this file see
 OPJ_API functions as being imported from a DLL, wheras this DLL sees symbols
 defined with this macro as being exported.
 */
    #if defined(OPJ_EXPORTS) || defined(DLL_EXPORT)
        #define OPJ_API __declspec(dllexport)
    #else
        #define OPJ_API __declspec(dllimport)
    #endif /* OPJ_EXPORTS */
#endif /* !OPJ_STATIC || !_WIN32 */

#endif

