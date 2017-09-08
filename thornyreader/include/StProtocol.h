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

#ifndef __STPROTOCOL_H__
#define __STPROTOCOL_H__

#include <stdint.h>

#define REQ_HEADER_SIZE     1
#define RES_HEADER_SIZE     2
#define DATA_HEADER_SIZE    5

#define CMD_MASK_CMD        0x7F
#define CMD_MASK_HAS_DATA   0x80

#define CMD_UNKNOWN         0
#define CMD_NOTIF_READY     1
#define CMD_REQ_OPEN        2
#define CMD_RES_OPEN        3
#define CMD_REQ_QUIT        4
#define CMD_RES_QUIT        5
#define CMD_REQ_PAGE_INFO   6
#define CMD_RES_PAGE_INFO   7
#define CMD_REQ_PAGE        8
#define CMD_RES_PAGE        9
#define CMD_REQ_PAGE_RENDER 10
#define CMD_RES_PAGE_RENDER 11
#define CMD_REQ_PAGE_FREE   12
#define CMD_RES_PAGE_FREE   13
#define CMD_REQ_PAGE_TEXT   14
#define CMD_RES_PAGE_TEXT   15
#define CMD_REQ_OUTLINE     16
#define CMD_RES_OUTLINE     17

#define CMD_REQ_SET_CONFIG   			20
#define CMD_RES_SET_CONFIG   			21
#define CMD_REQ_SMART_CROP              22
#define CMD_RES_SMART_CROP              23
#define CMD_REQ_CRE_PAGE_BY_XPATH		24
#define CMD_RES_CRE_PAGE_BY_XPATH		25
#define CMD_REQ_CRE_PAGE_XPATH			26
#define CMD_RES_CRE_PAGE_XPATH			27
#define CMD_REQ_CRE_METADATA   			28
#define CMD_RES_CRE_METADATA			29
#define CMD_REQ_ALIVE   			    30
#define CMD_RES_ALIVE			        31
#define CMD_REQ_LINKS   			    32
#define CMD_RES_LINKS			        33

#define CMD_REQ_PDF_STORAGE 124
#define CMD_RES_PDF_STORAGE 125
#define CMD_REQ_PDF_FONTS   126
#define CMD_RES_PDF_FONTS   127

#define CMD_REQ_PDF_SYSTEM_FONT   122
#define CMD_RES_PDF_SYSTEM_FONT   123

#define CMD_REQ_PDF_GET_MISSED_FONTS   120
#define CMD_RES_PDF_GET_MISSED_FONTS   121

#define CMD_REQ_PDF_GET_LAYERS_LIST   118
#define CMD_RES_PDF_GET_LAYERS_LIST   119

#define CMD_REQ_PDF_SET_LAYERS_MASK   116
#define CMD_RES_PDF_SET_LAYERS_MASK   117

#define RES_OK              0
#define RES_UNKNOWN_CMD     1
#define RES_NO_FILE         2
#define RES_NO_CONTEXT      3
#define RES_DUP_OPEN        4
#define RES_NOT_OPENED      5
#define RES_BAD_REQ_DATA    6
#define RES_MUPDF_PWD_WRONG 251
#define RES_MUPDF_PWD_NEED  252
#define RES_MUPDF_FAIL      254

#define TYPE_NONE           0
#define TYPE_FIX_WORDS      2
#define TYPE_FIX_INT        3
#define TYPE_FIX_FLOAT      4
#define TYPE_ARRAY_POINTER  5

#define TYPE_MASK_TYPE      0x7F
#define TYPE_MASK_HAS_NEXT  0x80

#define OUTLINE_TARGET_PAGE 			1
#define OUTLINE_TARGET_URI				2
#define OUTLINE_TARGET_XPATH			10

#define LINK_TARGET_PAGE                1
#define LINK_TARGET_URI                 2
#define LINK_TARGET_OUTER_PAGE          3
#define LINK_TARGET_LAUNCH              4
#define LINK_TARGET_UNKNOWN             10

class CmdData
{
public:
    uint8_t type;
    union
    {
        uint8_t value8[4];
        uint16_t value16[2];
        uint32_t value32;
        float valuef;
    } value;

    bool owned_external;
    uint8_t* external_array = 0;
    CmdData* nextData = 0;

public:
    CmdData();
    ~CmdData();

public:
    CmdData* setInt(uint32_t val);
    CmdData* setWords(uint16_t val0, uint16_t val1);
    CmdData* setFloat(float val);
    CmdData* setByteArray(int n, uint8_t *ptr, bool owned);
    CmdData* setIntArray(int n, int* ptr, bool owned);
    CmdData* setFloatArray(int n, float* ptr, bool owned);
    CmdData* setIpcString(const char* data, bool owned);

    uint8_t* newByteArray(int n);
    void freeArray();

    void print(const char* lctx);
};

class CmdDataIterator
{
private:
    CmdData *data;
    int count;
    uint32_t errors;

public:
    CmdDataIterator(CmdData* d);
    ~CmdDataIterator();

    bool hasNext();
    bool isValid();
    bool isValid(int index);
    int getCount();
    uint32_t getErrors();

    CmdDataIterator& getWords(uint16_t* v0, uint16_t* v1);
    CmdDataIterator& getInt(uint32_t* v0);
    CmdDataIterator& getFloat(float* v0);
    CmdDataIterator& optionalByteArray(uint8_t** buffer, uint32_t* len);
    CmdDataIterator& getByteArray(uint8_t** buffer);
    CmdDataIterator& getIntArray(uint32_t** buffer, int elements);
    CmdDataIterator& getFloatArray(float** buffer, int elements);

    void print(const char* lctx);
};

class CmdDataList
{
public:
    int dataCount = 0;
    CmdData* first = 0;
    CmdData* last = 0;

public:
    CmdDataList();

    CmdDataList& addData(CmdData* data);
    CmdDataList& addInt(uint32_t val);
    CmdDataList& addWords(uint16_t val0, uint16_t val1);
    CmdDataList& addFloat(float val);
    CmdDataList& addByteArray(int n, uint8_t* ptr, bool owned);
    CmdDataList& addIntArray(int n, int* ptr, bool owned);
    CmdDataList& addFloatArray(int n, float* ptr, bool owned);
    CmdDataList& addIpcString(const char* data, bool owned);
};

class CmdRequest: public CmdDataList
{
public:
    uint8_t cmd;

public:
    CmdRequest();
    CmdRequest(uint8_t c);
    ~CmdRequest();

    void reset();

    void print(const char* lctx);
};

class CmdResponse: public CmdDataList
{
public:
    uint8_t cmd;
    uint8_t result;

public:
    CmdResponse();
    CmdResponse(uint8_t c);
    ~CmdResponse();

    void reset();

    void print(const char* lctx);
};

#endif
