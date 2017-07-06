/*
 * Copyright (C) 2013 The Common CLI viewer interface Project
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

#include <stdlib.h>
#include <string.h>

#include "StProtocol.h"
#include "StLog.h"

#define L_PRINT_CMD false

CmdData::CmdData()
{
    type = TYPE_NONE;
    value.value32 = 0;
    owned_external = true;
    external_array = NULL;
    nextData = NULL;
}

CmdData::~CmdData()
{
    freeArray();
    type = TYPE_NONE;
    value.value32 = 0;
    external_array = NULL;
    if (nextData != NULL)
    {
        delete nextData;
        nextData = NULL;
    }
}

uint8_t* CmdData::newByteArray(int n)
{
    freeArray();
    type = TYPE_ARRAY_POINTER;
    value.value32 = n;
    owned_external = true;
    external_array = (uint8_t*) malloc(value.value32);
    return external_array;
}

void CmdData::freeArray()
{
    if (owned_external && external_array != NULL)
    {
        free(external_array);
    }
    type = TYPE_NONE;
    value.value32 = 0;
    external_array = NULL;
    owned_external = true;
}

CmdData* CmdData::setInt(uint32_t val)
{
    freeArray();
    type = TYPE_FIX_INT;
    value.value32 = val;
    return this;
}

CmdData* CmdData::setWords(uint16_t val0, uint16_t val1)
{
    freeArray();
    type = TYPE_FIX_WORDS;
    value.value16[0] = val0;
    value.value16[1] = val1;
    return this;
}

CmdData* CmdData::setFloat(float val)
{
    freeArray();
    type = TYPE_FIX_FLOAT;
    value.valuef = val;
    return this;
}

CmdData* CmdData::setByteArray(int n, uint8_t* ptr, bool owned)
{
    freeArray();
    type = TYPE_ARRAY_POINTER;
    value.value32 = n;
    owned_external = owned;
    if (owned)
    {
        external_array = (uint8_t*) calloc(1, value.value32);
        memcpy(external_array, ptr, value.value32);
    }
    else
    {
        external_array = ptr;
    }
    return this;
}

CmdData* CmdData::setIntArray(int n, int* ptr, bool owned)
{
    freeArray();
    type = TYPE_ARRAY_POINTER;
    value.value32 = n * sizeof(int);
    owned_external = owned;
    if (owned)
    {
        external_array = (uint8_t*) calloc(1, value.value32);
        memcpy(external_array, ptr, value.value32);
    }
    else
    {
        external_array = (uint8_t*) ptr;
    }
    return this;
}

CmdData* CmdData::setFloatArray(int n, float* ptr, bool owned)
{
    freeArray();
    type = TYPE_ARRAY_POINTER;
    value.value32 = n * sizeof(float);
    owned_external = owned;
    if (owned)
    {
        external_array = (uint8_t*) calloc(1, value.value32);
        memcpy(external_array, ptr, value.value32);
    }
    else
    {
        external_array = (uint8_t*) ptr;
    }
    return this;
}

CmdData* CmdData::setIpcString(const char* data, bool owned)
{
    freeArray();
    type = TYPE_ARRAY_POINTER;
    owned_external = owned;
    if (data != NULL)
    {
        value.value32 = strlen(data) + 1;
        external_array = owned ? (uint8_t*) strdup(data) : (uint8_t*) data;
    }
    else
    {
        value.value32 = 0;
        external_array = NULL;
    }
    return this;
}

void CmdData::print(const char* lctx)
{
    DEBUG_L(L_PRINT_CMD, lctx, "Data: %p %u %08x %p",
            this, this->type, this->value.value32, this->external_array);
}

CmdDataList::CmdDataList()
{
    dataCount = 0;
    first = last = NULL;
}

CmdDataList& CmdDataList::addData(CmdData* data)
{
    if (data != NULL)
    {
        if (last == NULL)
        {
            first = last = data;
        }
        else
        {
            last->nextData = data;
            last = data;
        }
        dataCount++;
    }
    return *this;
}

CmdDataList& CmdDataList::addInt(uint32_t val)
{
    return addData((new CmdData())->setInt(val));
}

CmdDataList& CmdDataList::addWords(uint16_t val0, uint16_t val1)
{
    return addData((new CmdData())->setWords(val0, val1));
}

CmdDataList& CmdDataList::addFloat(float val)
{
    return addData((new CmdData())->setFloat(val));
}

CmdDataList& CmdDataList::addByteArray(int n, uint8_t* ptr, bool owned)
{
    return addData((new CmdData())->setByteArray(n, ptr, owned));
}

CmdDataList& CmdDataList::addIntArray(int n, int* ptr, bool owned)
{
    return addData((new CmdData())->setIntArray(n, ptr, owned));
}
CmdDataList& CmdDataList::addFloatArray(int n, float* ptr, bool owned)
{
    return addData((new CmdData())->setFloatArray(n, ptr, owned));
}

CmdDataList& CmdDataList::addIpcString(const char* data, bool owned)
{
    return addData((new CmdData())->setIpcString(data, owned));
}

CmdRequest::CmdRequest()
{
    cmd = CMD_UNKNOWN;
}
CmdRequest::CmdRequest(uint8_t c)
{
    cmd = c;
}
CmdRequest::~CmdRequest()
{
    reset();
}

void CmdRequest::reset()
{
    if (first != NULL)
    {
        delete first;
    }
    cmd = CMD_UNKNOWN;
    dataCount = 0;
    first = last = NULL;
}

void CmdRequest::print(const char* lctx)
{
    DEBUG_L(L_PRINT_CMD, lctx, "Request: %u", this->cmd);
    CmdData* data;
    for (data = this->first; data != NULL; data = data->nextData)
    {
        data->print(lctx);
    }
}

CmdResponse::CmdResponse()
{
    cmd = CMD_UNKNOWN;
    result = RES_OK;
}

CmdResponse::CmdResponse(uint8_t c)
{
    cmd = c;
    result = RES_OK;
}

CmdResponse::~CmdResponse()
{
    reset();
}

void CmdResponse::reset()
{
    if (first != NULL)
    {
        delete first;
    }
    cmd = CMD_UNKNOWN;
    result = RES_OK;
    first = last = NULL;

}

void CmdResponse::print(const char* lctx)
{
    DEBUG_L(L_PRINT_CMD, lctx, "Response: %u %u", this->cmd, this->result);
    CmdData* data;
    for (data = this->first; data != NULL; data = data->nextData)
    {
        data->print(lctx);
    }
}

CmdDataIterator::CmdDataIterator(CmdData* d)
{
    this->count = 0;
    this->data = d;
    this->errors = 0;
}
CmdDataIterator::~CmdDataIterator()
{
    this->count = 0;
    this->data = NULL;
    this->errors = 0;
}

bool CmdDataIterator::hasNext()
{
    return this->data != NULL;
}

bool CmdDataIterator::isValid()
{
    return this->errors == 0;
}

bool CmdDataIterator::isValid(int index)
{
    return (this->errors & (1 << index)) != 0;
}

int CmdDataIterator::getCount()
{
    return count;
}

CmdDataIterator& CmdDataIterator::getWords(uint16_t* v0, uint16_t* v1)
{
    *v0 = *v1 = 0;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_FIX_WORDS)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *v0 = this->data->value.value16[0];
        *v1 = this->data->value.value16[1];
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;

}

CmdDataIterator& CmdDataIterator::getInt(uint32_t* v0)
{
    *v0 = 0;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_FIX_INT)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *v0 = this->data->value.value32;
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::getFloat(float* v0)
{
    *v0 = 0;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_FIX_FLOAT)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *v0 = this->data->value.valuef;
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::optionalByteArray(uint8_t** buffer, uint32_t* len)
{
    *buffer = NULL;
    if (len) {
    	*len = 0;
    }
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_ARRAY_POINTER)
    {
        this->errors |= 1 << count;
    }
    else
    {
    	if (len) {
    		*len = this->data->value.value32;
		}
        *buffer = this->data->external_array;
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::getByteArray(uint8_t** buffer)
{
    *buffer = NULL;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_ARRAY_POINTER)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->value.value32 == 0 || this->data->external_array == NULL)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *buffer = this->data->external_array;
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::getIntArray(uint32_t** buffer, int elements)
{
    *buffer = NULL;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_ARRAY_POINTER)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->value.value32 != elements * sizeof(uint32_t))
    {
        this->errors |= 1 << count;
    }
    else
    {
        *buffer = (uint32_t*) this->data->external_array;
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::getFloatArray(float** buffer, int elements)
{
    *buffer = NULL;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_ARRAY_POINTER)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->value.value32 != elements * sizeof(float))
    {
        this->errors |= 1 << count;
    }
    else
    {
        *buffer = (float*) this->data->external_array;
    }

    count++;
    this->data = this->data != NULL ? this->data->nextData : NULL;
    return *this;
}

void CmdDataIterator::print(const char* lctx)
{
    DEBUG_L(L_PRINT_CMD, lctx, "Iterator: %p %u %08x", this->data, this->count, this->errors);
}

