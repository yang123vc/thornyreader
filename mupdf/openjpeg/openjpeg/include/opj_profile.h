/*
 * The copyright in this software is being made available under the 2-clauses 
 * BSD License, included below. This software may be subject to other third 
 * party and contributor rights, including patent rights, and no such rights
 * are granted under this license.
 *
 * Copyright (c) 2014, Andrei Komarovskikh, Alex Kasatkin
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
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
#ifndef __OPJ_PROFILE_H
#define __OPJ_PROFILE_H

#ifdef OPJ_PROFILE

    #include "opj_clock.h"
    #include "StLog.h"

    #define OPJ_PROFILE_COUNTER(x) OPJ_FLOAT64 x = 0
    #define OPJ_PROFILE_COUNTER_START(x) OPJ_FLOAT64 x = opj_clock()
    #define OPJ_PROFILE_COUNTER_RESTART(x) x = opj_clock()
    #define OPJ_PROFILE_COUNTER_FINISH(x) x = opj_clock() - x
    #define OPJ_PROFILE_COUNTER_FINISH_WITH_SUM(counter, x) x = opj_clock() - x, counter += x

    #define OPJ_PROFILE_MSG(LCTX, args...) ERROR_L(LCTX, args)

#else

    #define OPJ_PROFILE_COUNTER(...)
    #define OPJ_PROFILE_COUNTER_START(...)
    #define OPJ_PROFILE_COUNTER_RESTART(...)
    #define OPJ_PROFILE_COUNTER_FINISH(...)
    #define OPJ_PROFILE_COUNTER_FINISH_WITH_SUM(...)

    #define OPJ_PROFILE_MSG(...)

#endif


#endif /* __OPJ_PROFILE_H */

