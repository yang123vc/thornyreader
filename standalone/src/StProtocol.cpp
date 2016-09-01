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

#include "StProtocol.h"
#include "StLog.h"

#define L_PRINT_CMD false

CmdData::CmdData()
{
    type = TYPE_NONE;
    value.value32 = 0;
    external = NULL;
    next = NULL;
}

CmdData::CmdData(uint32_t val)
{
    type = TYPE_FIX_INT;
    value.value32 = val;
    owned_external = false;
    external = NULL;
    next = NULL;
}

CmdData::CmdData(uint16_t val0, uint16_t val1)
{
    type = TYPE_FIX_WORDS;
    value.value16[0] = val0;
    value.value16[1] = val1;
    owned_external = false;
    external = NULL;
    next = NULL;
}

CmdData::CmdData(float val)
{
    type = TYPE_FIX_FLOAT;
    value.valuef = val;
    owned_external = false;
    external = NULL;
    next = NULL;
}

CmdData::CmdData(int n)
{
    type = TYPE_VAR;
    value.value32 = n;
    owned_external = true;
    external = (uint8_t*) malloc(value.value32);
    next = NULL;
}

CmdData::CmdData(int n, uint8_t* ptr, bool owned)
{
    type = TYPE_VAR;
    value.value32 = n;
    owned_external = owned;
    if (owned)
    {
        external = (uint8_t*) malloc(value.value32);
        memcpy(external, ptr, value.value32);
    }
    else
    {
        external = ptr;
    }
    next = NULL;
}

CmdData::CmdData(int n, uint16_t* ptr, bool owned)
{
    type = TYPE_VAR;
    value.value32 = n * sizeof(uint16_t);
    owned_external = owned;
    if (owned)
    {
        external = (uint8_t*) malloc(value.value32);
        memcpy(external, ptr, value.value32);
    }
    else
    {
        external = (uint8_t*) ptr;
    }
    next = NULL;
}

CmdData::CmdData(int n, int* ptr, bool owned)
{
    type = TYPE_VAR;
    value.value32 = n * sizeof(int);
    owned_external = owned;
    if (owned)
    {
        external = (uint8_t*) malloc(value.value32);
        memcpy(external, ptr, value.value32);
    }
    else
    {
        external = (uint8_t*) ptr;
    }
    next = NULL;
}

CmdData::CmdData(int n, float* ptr, bool owned)
{
    type = TYPE_VAR;
    value.value32 = n * sizeof(float);
    owned_external = owned;
    if (owned)
    {
        external = (uint8_t*) malloc(value.value32);
        memcpy(external, ptr, value.value32);
    }
    else
    {
        external = (uint8_t*) ptr;
    }
    next = NULL;
}

CmdData::CmdData(const char* data, bool owned)
{
    type = TYPE_VAR;
    owned_external = owned;
    if (data != NULL)
    {
        value.value32 = strlen(data) + 1;
        external = owned ? (uint8_t*) strdup(data) : (uint8_t*) data;
    }
    else
    {
        value.value32 = 0;
        external = NULL;
    }
    next = NULL;
}

CmdData::~CmdData()
{
    freeExternal();
    type = TYPE_NONE;
    value.value32 = 0;
    external = NULL;
    if (next != NULL)
    {
        delete next;
        next = NULL;
    }
}

void CmdData::setExternal(int size, uint8_t* ptr, bool owned)
{
    type = TYPE_VAR;
    if (owned_external && external != NULL)
    {
        free(external);
    }
    value.value32 = size;
    owned_external = owned;
    if (owned)
    {
        external = (uint8_t*) malloc(value.value32);
        memcpy(external, ptr, value.value32);
    }
    else
    {
        external = ptr;
    }
}

void CmdData::freeExternal()
{
    if (owned_external)
    {
        if (external != NULL)
        {
            free(external);
        }
        external = NULL;
    }
    else
    {
        external = NULL;
    }
    owned_external = true;
}

void CmdData::print(const char* lctx)
{
    DEBUG_L(L_PRINT_CMD, lctx, "Data: %p %u %08x %p", this, this->type, this->value.value32, this->external);
}

CmdDataList::CmdDataList()
{
    dataCount = 0;
    first = last = NULL;
}

CmdDataList& CmdDataList::add(CmdData* data)
{
    if (data != NULL)
    {
        if (last == NULL)
        {
            first = last = data;
        }
        else
        {
            last->next = data;
            last = data;
        }
        dataCount++;
    }
    return *this;
}

CmdDataList& CmdDataList::add(uint32_t val)
{
    return add(new CmdData(val));
}

CmdDataList& CmdDataList::add(uint16_t val0, uint16_t val1)
{
    return add(new CmdData(val0, val1));
}

CmdDataList& CmdDataList::add(float val)
{
    return add(new CmdData(val));
}

CmdDataList& CmdDataList::add(double val)
{
    return add(new CmdData((float)val));
}

CmdDataList& CmdDataList::add(int n, uint8_t* ptr, bool owned)
{
    return add(new CmdData(n, ptr, owned));
}

CmdDataList& CmdDataList::add(int n, uint16_t* ptr, bool owned)
{
    return add(new CmdData(n, ptr, owned));
}

CmdDataList& CmdDataList::add(int n, int* ptr, bool owned)
{
    return add(new CmdData(n, ptr, owned));
}
CmdDataList& CmdDataList::add(int n, float* ptr, bool owned)
{
    return add(new CmdData(n, ptr, owned));
}

CmdDataList& CmdDataList::add(const char* data, bool owned)
{
    return add(new CmdData(data, owned));
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
    for (data = this->first; data != NULL; data = data->next)
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
    for (data = this->first; data != NULL; data = data->next)
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

uint32_t CmdDataIterator::getErrors()
{
    return errors;
}

CmdDataIterator& CmdDataIterator::bytes(uint8_t* v0, uint8_t* v1, uint8_t* v2, uint8_t* v3)
{
    *v0 = *v1 = *v2 = *v3 = 0;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_FIX_BYTES)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *v0 = this->data->value.value8[0];
        *v1 = this->data->value.value8[1];
        *v2 = this->data->value.value8[2];
        *v3 = this->data->value.value8[3];
    }

    count++;
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::words(uint16_t* v0, uint16_t* v1)
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
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;

}

CmdDataIterator& CmdDataIterator::integer(uint32_t* v0)
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
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::floater(float* v0)
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
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::optional(uint8_t** buffer, uint32_t* len)
{
    *buffer = NULL;
    *len = 0;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_VAR)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *len = this->data->value.value32;
        *buffer = this->data->external;
    }

    count++;
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::optional(uint16_t** buffer, uint32_t* len)
{
    *buffer = NULL;
    *len = 0;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_VAR)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *len = this->data->value.value32/sizeof(uint16_t);
        *buffer = (uint16_t*)this->data->external;
    }

    count++;
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}


CmdDataIterator& CmdDataIterator::required(uint8_t** buffer)
{
    *buffer = NULL;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_VAR)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->value.value32 == 0 || this->data->external == NULL)
    {
        this->errors |= 1 << count;
    }
    else
    {
        *buffer = this->data->external;
    }

    count++;
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::required(uint32_t** buffer, int elements)
{
    *buffer = NULL;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_VAR)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->value.value32 != elements * sizeof(uint32_t))
    {
        this->errors |= 1 << count;
    }
    else
    {
        *buffer = (uint32_t*) this->data->external;
    }

    count++;
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

CmdDataIterator& CmdDataIterator::required(float** buffer, int elements)
{
    *buffer = NULL;
    if (this->data == NULL)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->type != TYPE_VAR)
    {
        this->errors |= 1 << count;
    }
    else if (this->data->value.value32 != elements * sizeof(float))
    {
        this->errors |= 1 << count;
    }
    else
    {
        *buffer = (float*) this->data->external;
    }

    count++;
    this->data = this->data != NULL ? this->data->next : NULL;
    return *this;
}

void CmdDataIterator::print(const char* lctx)
{
    DEBUG_L(L_PRINT_CMD, lctx, "Iterator: %p %u %08x", this->data, this->count, this->errors);
}

