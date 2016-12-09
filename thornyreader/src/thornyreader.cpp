/*
 * Copyright (C) 2016 ThornyReader
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android/log.h>
#include "thornyreader.h"

const char* ThornyReader_NDEBUG()
{
    const char* ndebug;
#ifdef NDEBUG
    ndebug = "NDEBUG=def";
#else
    ndebug = "NDEBUG=ndef";
#endif
    return ndebug;
}

const char* ThornyReader_AXYDEBUG()
{
    const char* axy_debug;
#ifdef AXYDEBUG
    axy_debug = "AXYDEBUG=def";
#else
    axy_debug = "AXYDEBUG=ndef";
#endif
    return axy_debug;
}

void StartThornyReader(const char* name)
{
    __android_log_print(ANDROID_LOG_INFO, name, "Start %s v%s, %s, %s",
                        name,
                        THORNYREADER_VERSION,
                        ThornyReader_NDEBUG(),
                        ThornyReader_AXYDEBUG());
}