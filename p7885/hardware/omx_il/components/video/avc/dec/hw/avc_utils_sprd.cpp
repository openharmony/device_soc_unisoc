/*
 * Copyright 2016-2026 Unisoc (Shanghai) Technologies Co., Ltd.
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
#include <cstdlib>
#include <cstring>
#include "avc_utils_sprd.h"
#include <cstddef>
/* Profile identifiers */
#define AVC_PROFILE_BASELINE     0x42
#define AVC_PROFILE_MAIN         0x4D
#define AVC_PROFILE_HIGH         0x64
/* Chroma format */
#define CHROMA_FORMAT_420        1
/* NAL unit type mask */
#define NAL_UNIT_TYPE_MASK       0x1F
/* NAL unit types */
#define NAL_UNIT_TYPE_SPS        7
/* Emulation prevention byte */
#define EMULATION_PREVENTION_BYTE    0x03
/* Start code related */
#define START_CODE_ZERO_BYTE     0x00
#define START_CODE_LAST_BYTE     0x01
/* Bitstream related */
#define BITS_PER_BYTE            8
#define BITS_PER_WORD            32
#define MAX_LEADING_ZERO_BITS    16
/* Picture order count related */
#define MAX_POC_CYCLE_REF_FRAMES 255
/* Scaling matrix sizes */
#define SCALING_LIST_4X4_SIZE    16
#define SCALING_LIST_8X8_SIZE    64
/* Word alignment shift */
#define WORD_ALIGN_SHIFT         2
/* Start code length related */
#define START_CODE_LENGTH        4           /* Length of start code (0x00000001) */
#define START_CODE_PREFIX_LENGTH 3           /* Number of zero bytes before start code delimiter */
/* Start code byte indices */
#define START_CODE_FIRST_BYTE_INDEX  0       /* Index of first byte in start code */
#define START_CODE_SECOND_BYTE_INDEX 1       /* Index of second byte in start code */
#define START_CODE_THIRD_BYTE_INDEX  2       /* Index of third byte in start code */
#define START_CODE_FOURTH_BYTE_INDEX 3       /* Index of fourth byte (delimiter) in start code */
/* Scaling list default values */
#define DEFAULT_SCALING_LIST_LAST_VALUE  8   /* Default last value for scaling list decoding */
#define SCALING_LIST_MODULO_MASK         0xFF /* Modulo mask for scaling list value */
/* Reserved bits count in SPS */
#define RESERVED_ZERO_BITS_COUNT 4           /* Number of reserved zero bits in sequence parameter set */
/* Maximum reference frame number */
#define MAX_REF_FRAME_NUMBER     16          /* Maximum number of reference frames allowed */
/* Frame type flags */
#define FRAME_MBS_ONLY_FLAG_TRUE     1       /* Frame contains only frame macroblocks */
#define FRAME_MBS_ONLY_FLAG_FALSE    0       /* Frame contains field macroblocks */
/* Scaling matrix presence flag */
#define SEQUENCE_SCALING_MATRIX_PRESENT 1    /* Sequence scaling matrix is present */
/* Emulation prevention check threshold */
#define EMULATION_CHECK_ZERO_COUNT  2        /* Number of consecutive zero bytes before emulation check */
/* Word alignment related */
#define BYTES_PER_WORD             4         /* Number of bytes in a word */
#define INITIAL_BYTE_REST_COUNT    4         /* Initial counter for byte processing in word alignment */
/* Extra buffer size for start code */
#define EXTRA_BUFFER_FOR_START_CODE 4        /* Additional buffer size for start code prepending */
/* Default sequence parameter values */
#define DEFAULT_SEQUENCE_PARAMETER 0         /* Default value for sequence parameters */
/* Bitstream error flag */
#define BITSTREAM_ERROR_FLAG       1         /* Error flag for bitstream decoding */
/* Bit manipulation constants */
#define BIT_MASK_0X1               0x1       /* Mask for single bit extraction */
#define BIT_MASK_0XFF              0xFF      /* Mask for byte value extraction */
#define BIT_POSITION_31            31        /* Position of bit 31 in 32-bit word */
/* Value conversion constants */
#define VALUE_DOUBLE               2         /* Multiplier for value doubling */
#define VALUE_INCREMENT            1         /* Value increment by one */
#define VALUE_DECREMENT_BASE       1         /* Base value for decrement calculations */
/* Return values */
#define SUCCESS_RETURN_VALUE       0         /* Success return value */
#define ERROR_RETURN_VALUE        (-1)         /* Error return value */
namespace OHOS {
namespace OMX {
const unsigned int MSK[33] = {
    0x00000000, 0x00000001, 0x00000003, 0x00000007,
    0x0000000f, 0x0000001f, 0x0000003f, 0x0000007f,
    0x000000ff, 0x000001ff, 0x000003ff, 0x000007ff,
    0x00000fff, 0x00001fff, 0x00003fff, 0x00007fff,
    0x0000ffff, 0x0001ffff, 0x0003ffff, 0x0007ffff,
    0x000fffff, 0x001fffff, 0x003fffff, 0x007fffff,
    0x00ffffff, 0x01ffffff, 0x03ffffff, 0x07ffffff,
    0x0fffffff, 0x1fffffff, 0x3fffffff, 0x7fffffff,
    0xffffffff
};

static unsigned int BitstreamShowBits(const DEC_BS_T *bitstream, unsigned int nBits)
{
    if ((nBits) <= (bitstream)->bitsLeft) {
        return ((((bitstream)->rdptr[0]) >> ((bitstream)->bitsLeft - (nBits))) & MSK[(nBits)]);
    } else {
        return (((((bitstream)->rdptr[0]) << ((nBits) - (bitstream)->bitsLeft)) |
                  (((bitstream)->rdptr[1]) >> (BITS_PER_WORD - (nBits) + (bitstream)->bitsLeft))) & MSK[(nBits)]);
    }
}

static void BitstreamFlushBits(DEC_BS_T *stream, unsigned int nbits)
{
    (stream)->bitcnt += (nbits);
    if ((nbits) < (stream)->bitsLeft) {
        (stream)->bitsLeft -= (nbits);
    } else {
        (stream)->bitsLeft += BITS_PER_WORD - (nbits);
        (stream)->rdptr++;
    }
}

static unsigned int READ_BITS1(DEC_BS_T *stream)
{
    unsigned int val = ((*stream->rdptr >> (stream->bitsLeft - VALUE_INCREMENT)) & BIT_MASK_0X1);
    stream->bitcnt++;
    stream->bitsLeft--;
    if (!stream->bitsLeft) {
        stream->bitsLeft = BITS_PER_WORD;
        stream->rdptr++;
    }
    return val;
}
static unsigned int READ_BITS(DEC_BS_T *stream, unsigned int nbits)
{
    unsigned int temp = 0;
    temp = BitstreamShowBits(stream, nbits);
    BitstreamFlushBits(stream, nbits);
    return temp;
}
static unsigned int READ_UE_V(DEC_BS_T *stream)
{
    int info = 0;
    unsigned int ret = 0;
    unsigned int tmp = 0;
    unsigned int leadingZero = 0;
    tmp = reinterpret_cast<unsigned int>(BitstreamShowBits(stream, BITS_PER_WORD));
    /*find the leading zero number*/
#ifndef _ARM_CLZ_OPT_
    if (!tmp) {
        stream->errorFlag |= BITSTREAM_ERROR_FLAG;
        return DEFAULT_SEQUENCE_PARAMETER;
    }
    while ((tmp & (VALUE_INCREMENT << (BIT_POSITION_31 - leadingZero))) == 0) {
        leadingZero++;
    }
#else
#if defined(__GNUC__)
    __asm__("clz %0, %1":"=&r"(leadingZero):"r"(tmp):"cc");
#else
    __asm {
        clz leadingZero, tmp
    }
#endif
#endif
    //must! because BITSTRM may be error and can't find the leading zero, xw@20100527
    if (leadingZero > MAX_LEADING_ZERO_BITS) {
        stream->errorFlag |= BITSTREAM_ERROR_FLAG;
        return DEFAULT_SEQUENCE_PARAMETER;
    }
    BitstreamFlushBits(stream, leadingZero * VALUE_DOUBLE + VALUE_INCREMENT);
    info = (tmp >> (BITS_PER_WORD - VALUE_DOUBLE * leadingZero - VALUE_INCREMENT)) & MSK [leadingZero];
    ret = (VALUE_INCREMENT << leadingZero) + info - VALUE_INCREMENT;
    return ret;
}
static int READ_SE_V(DEC_BS_T *stream)
{
    int ret;
    int info;
    int tmp;
    unsigned int leadingZero = 0;
    tmp = BitstreamShowBits(stream, BITS_PER_WORD);
    /*find the leading zero number*/
#ifndef _ARM_CLZ_OPT_
    if (!tmp) {
        stream->errorFlag |= BITSTREAM_ERROR_FLAG;
        return DEFAULT_SEQUENCE_PARAMETER;
    }
    while ((tmp & (VALUE_INCREMENT << (BIT_POSITION_31 - leadingZero))) == 0) {
        leadingZero++;
    }
#else
#if defined(__GNUC__)
    __asm__("clz %0, %1":"=&r"(leadingZero):"r"(tmp):"cc");
#else
    __asm {
        clz leadingZero, tmp
    }
#endif
#endif
    //must! because BITSTRM may be error and can't find the leading zero, xw@20100527
    if (leadingZero > MAX_LEADING_ZERO_BITS) {
        stream->errorFlag |= BITSTREAM_ERROR_FLAG;
        return DEFAULT_SEQUENCE_PARAMETER;
    }
    BitstreamFlushBits (stream, leadingZero * VALUE_DOUBLE + VALUE_INCREMENT);
    info = (tmp >> (BITS_PER_WORD - VALUE_DOUBLE * leadingZero - VALUE_INCREMENT)) & MSK [leadingZero];
    tmp = (VALUE_INCREMENT << leadingZero) + info - VALUE_INCREMENT;
    ret = (tmp + VALUE_INCREMENT) / VALUE_DOUBLE;

    if ((tmp & VALUE_INCREMENT) == 0) {
        ret = -ret;
    }
    return ret;
}
static int H264DecLongSev(DEC_BS_T *stream)
{
    unsigned int tmp;
    int leadingZero = 0;
    tmp = BitstreamShowBits (stream, MAX_LEADING_ZERO_BITS);
    if (tmp == SUCCESS_RETURN_VALUE) {
        READ_BITS (stream, MAX_LEADING_ZERO_BITS);
        leadingZero = MAX_LEADING_ZERO_BITS;
        do {
            tmp = READ_BITS1 (stream);
            leadingZero++;
        } while (!tmp);
        leadingZero--;
        tmp = READ_BITS (stream, leadingZero);
        return tmp;
    } else {
        return READ_SE_V (stream);
    }
}
void DecodeScalingList(DEC_BS_T *stream, int size)
{
    int i = 0;
    int last = DEFAULT_SCALING_LIST_LAST_VALUE;
    int next = DEFAULT_SCALING_LIST_LAST_VALUE;
    /* matrix not written, we use the predicted one */
    if (READ_BITS(stream, VALUE_INCREMENT) != 0) {
        for (i = 0; i < size; i++) {
            if (next != 0) {
                next = (last +  READ_SE_V (stream)) & SCALING_LIST_MODULO_MASK;
            }
            if (!i && !next) { /* matrix not written, we use the preset one */
                break;
            }
            last = (next != 0) ? next : last;
        }
    }
}
void DecodeScalingMatrices(DEC_BS_T *stream, int isSps)
{
    DecodeScalingList(stream, SCALING_LIST_4X4_SIZE);    // Intra, Y
    DecodeScalingList(stream, SCALING_LIST_4X4_SIZE);    // Intra, Cr
    DecodeScalingList(stream, SCALING_LIST_4X4_SIZE);    // Intra, Cb
    DecodeScalingList(stream, SCALING_LIST_4X4_SIZE);    // Inter, Y
    DecodeScalingList(stream, SCALING_LIST_4X4_SIZE);    // Inter, Cr
    DecodeScalingList(stream, SCALING_LIST_4X4_SIZE);    // Inter, Cb
    if (isSps != 0) {
        DecodeScalingList(stream, SCALING_LIST_8X8_SIZE);    // Intra, Y
        DecodeScalingList(stream, SCALING_LIST_8X8_SIZE);    // Inter, Y
    }
}
void H264DecInitBitstream(DEC_BS_T *stream, void *pOneFrameBitstream, int length)
{
    stream->bitcnt = SUCCESS_RETURN_VALUE;
    stream->rdptr = (unsigned int *)pOneFrameBitstream;
    stream->bitsLeft   = BITS_PER_WORD;
    stream->bitcntBeforeVld = stream->bitcnt;
    length = SUCCESS_RETURN_VALUE;
}

static bool IsSupportedProfile(char profileIdc)
{
    return profileIdc == AVC_PROFILE_BASELINE ||
        profileIdc == AVC_PROFILE_MAIN ||
        profileIdc == AVC_PROFILE_HIGH;
}

static int ParseHighProfileFields(DEC_BS_T *stream, char profileIdc)
{
    if (profileIdc != AVC_PROFILE_HIGH) {
        return SUCCESS_RETURN_VALUE;
    }
    char chromaFormatIdc = READ_UE_V(stream);
    if (chromaFormatIdc != CHROMA_FORMAT_420) {
        return ERROR_RETURN_VALUE;
    }
    READ_UE_V(stream);
    READ_UE_V(stream);
    READ_BITS(stream, VALUE_INCREMENT);
    if (READ_BITS1(stream) != 0) {
        DecodeScalingMatrices(stream, SEQUENCE_SCALING_MATRIX_PRESENT);
    }
    return SUCCESS_RETURN_VALUE;
}

static int ParsePicOrderCount(DEC_BS_T *stream, char picOrderCntType)
{
    if (picOrderCntType == SUCCESS_RETURN_VALUE) {
        READ_UE_V(stream);
        return SUCCESS_RETURN_VALUE;
    }
    if (picOrderCntType != VALUE_INCREMENT) {
        return SUCCESS_RETURN_VALUE;
    }
    READ_BITS1(stream);
    H264DecLongSev(stream);
    H264DecLongSev(stream);
    unsigned int numRefFrames = READ_UE_V(stream);
    if (numRefFrames > MAX_POC_CYCLE_REF_FRAMES) {
        return ERROR_RETURN_VALUE;
    }
    for (unsigned int i = 0; i < numRefFrames; i++) {
        H264DecLongSev(stream);
    }
    return SUCCESS_RETURN_VALUE;
}

static int ParseFrameMode(DEC_BS_T *stream)
{
    char frameMbsOnlyFlag = READ_BITS1(stream);
    SCI_TRACE_LOW("%s, %d, frameMbsOnlyFlag = %d\n", __FUNCTION__, __LINE__, frameMbsOnlyFlag);
    if (frameMbsOnlyFlag != 0) {
        return FRAME_MBS_ONLY_FLAG_FALSE;
    }
    char frameIsMbaff = READ_BITS1(stream);
    SCI_TRACE_LOW("%s, %d, frameIsMbaff = %d\n", __FUNCTION__, __LINE__, frameIsMbaff);
    return FRAME_MBS_ONLY_FLAG_TRUE;
}

int  InterpretH264Header(DEC_BS_T *stream)
{
    char profileIdc = READ_BITS(stream, BITS_PER_BYTE);
    SCI_TRACE_LOW("%s, %d, profileIdc = %d\n", __FUNCTION__, __LINE__, profileIdc);
    if (!IsSupportedProfile(profileIdc)) {
        return ERROR_RETURN_VALUE;
    }
    READ_BITS1(stream);
    READ_BITS1(stream);
    READ_BITS1(stream);
    READ_BITS1(stream);
    READ_BITS(stream, RESERVED_ZERO_BITS_COUNT);
    READ_BITS(stream, BITS_PER_BYTE);
    READ_UE_V(stream);
    if (ParseHighProfileFields(stream, profileIdc) != SUCCESS_RETURN_VALUE) {
        return ERROR_RETURN_VALUE;
    }
    READ_UE_V(stream);
    char picOrderCntType = READ_UE_V(stream);
    if (ParsePicOrderCount(stream, picOrderCntType) != SUCCESS_RETURN_VALUE) {
        return ERROR_RETURN_VALUE;
    }
    char numRefFrames = READ_UE_V(stream);
    SCI_TRACE_LOW("%s, %d, numRefFrames = %d\n", __FUNCTION__, __LINE__, numRefFrames);
    if (numRefFrames > MAX_REF_FRAME_NUMBER) {
        return ERROR_RETURN_VALUE;
    }
    READ_BITS1(stream);
    READ_UE_V(stream);
    READ_UE_V(stream);
    return ParseFrameMode(stream);
}

static int ConsumeStartCode(unsigned char **ptr)
{
    int len = 0;
    unsigned char data = 0;
    while ((data = *(*ptr)++) == START_CODE_ZERO_BYTE) {
        len++;
    }
    return len + VALUE_INCREMENT;
}

static void AppendByteToWord(
    int *code, unsigned long *byteRest, int **streamPtr, unsigned char data)
{
    (*byteRest)--;
    *code = (*code << BITS_PER_BYTE) | data;
    if (*byteRest == 0) {
        *byteRest = BYTES_PER_WORD;
        *(*streamPtr)++ = *code;
    }
}

struct UnitParseState {
    unsigned char *ptr;
    int declen;
    int zeroNum;
    int startCodeLen;
    int stuffingNum;
    int *streamPtr;
    int code;
    unsigned long byteRest;
    int len;
};

static bool HandleNaluByte(unsigned char data, UnitParseState *state)
{
    if (state->zeroNum < EMULATION_CHECK_ZERO_COUNT) {
        state->zeroNum++;
        AppendByteToWord(&state->code, &state->byteRest, &state->streamPtr, data);
        if (data != START_CODE_ZERO_BYTE) {
            state->zeroNum = SUCCESS_RETURN_VALUE;
        }
        return false;
    }
    if (data == EMULATION_PREVENTION_BYTE) {
        state->zeroNum = SUCCESS_RETURN_VALUE;
        state->stuffingNum++;
        return false;
    }
    AppendByteToWord(&state->code, &state->byteRest, &state->streamPtr, data);
    if (data == START_CODE_LAST_BYTE) {
        state->startCodeLen = state->zeroNum + VALUE_INCREMENT;
        return true;
    }
    state->zeroNum = (data == START_CODE_ZERO_BYTE) ? (state->zeroNum + VALUE_INCREMENT) : SUCCESS_RETURN_VALUE;
    return false;
}

static void FinalizeUnitLengths(const UnitParseState *state, unsigned int *sliceUnitLen, unsigned int *pNaluLen)
{
    int code = state->code;
    int naluLen = state->len - state->startCodeLen - state->stuffingNum;
    *sliceUnitLen = state->declen - state->startCodeLen;
    while (code && !(code & BIT_MASK_0XFF)) {
        naluLen--;
        code >>= BITS_PER_BYTE;
    }
    *pNaluLen = naluLen;
}

static void InitUnitParseState(UnitParseState *state, unsigned char **naluBuf, unsigned char *pInStream,
    unsigned int frmBsLen)
{
    state->ptr = pInStream;
    state->declen = 0;
    state->zeroNum = 0;
    state->startCodeLen = 0;
    state->stuffingNum = 0;
    state->streamPtr = nullptr;
    state->code = 0;
    state->len = ConsumeStartCode(&state->ptr);
    state->declen += state->len;
    state->byteRest = reinterpret_cast<unsigned long>(pInStream);
    state->byteRest = (state->byteRest >> WORD_ALIGN_SHIFT) << WORD_ALIGN_SHIFT;
    state->streamPtr = reinterpret_cast<int*>(state->byteRest);
    *naluBuf = reinterpret_cast<unsigned char*>(state->byteRest);
    state->byteRest = INITIAL_BYTE_REST_COUNT;
    state->declen = frmBsLen - state->len;
}

static void ConsumeUnitBytes(UnitParseState *state)
{
    do {
        if (HandleNaluByte(*state->ptr++, state)) {
            break;
        }
        state->declen--;
    } while (state->declen != 0);
}

static int CompleteUnitParse(UnitParseState *state, unsigned int frmBsLen, unsigned int *sliceUnitLen,
    unsigned int *pNaluLen)
{
    *state->streamPtr++ = state->code << (state->byteRest * BITS_PER_BYTE);
    if (state->declen == 0) {
        state->declen = VALUE_INCREMENT;
    }
    state->declen = frmBsLen - state->declen;
    state->declen++;
    state->len = state->declen - state->len;
    FinalizeUnitLengths(state, sliceUnitLen, pNaluLen);
    state->declen -= state->startCodeLen;
    return static_cast<unsigned long>(state->declen) >= frmBsLen ? VALUE_INCREMENT : SUCCESS_RETURN_VALUE;
}

static int GetUnit(unsigned char** naluBuf, unsigned int* pNaluLen,
                   unsigned char *pInStream, unsigned int frmBsLen, unsigned int *sliceUnitLen)
{
    UnitParseState state;
    InitUnitParseState(&state, naluBuf, pInStream, frmBsLen);
    ConsumeUnitBytes(&state);
    return CompleteUnitParse(&state, frmBsLen, sliceUnitLen, pNaluLen);
}

static bool HasStartCodePrefix(const unsigned char *bitstrmPtr, size_t bitstrmLen)
{
    return bitstrmLen >= EXTRA_BUFFER_FOR_START_CODE &&
        *(bitstrmPtr + START_CODE_FIRST_BYTE_INDEX) == START_CODE_ZERO_BYTE &&
        *(bitstrmPtr + START_CODE_SECOND_BYTE_INDEX) == START_CODE_ZERO_BYTE &&
        *(bitstrmPtr + START_CODE_THIRD_BYTE_INDEX) == START_CODE_ZERO_BYTE &&
        *(bitstrmPtr + START_CODE_FOURTH_BYTE_INDEX) == START_CODE_LAST_BYTE;
}

static int CopyBitstreamWithStartCode(unsigned char *pstream, const unsigned char *bitstrmPtr, size_t bitstrmLen)
{
    if (HasStartCodePrefix(bitstrmPtr, bitstrmLen)) {
        errno_t err = memmove_s(pstream, bitstrmLen + EXTRA_BUFFER_FOR_START_CODE, bitstrmPtr, bitstrmLen);
        if (err == 0) {
            return SUCCESS_RETURN_VALUE;
        }
        OMX_LOGE("memmove_s failed at line %d, err=%d", __LINE__, err);
        return ERROR_RETURN_VALUE;
    }
    errno_t err = memmove_s(pstream + EXTRA_BUFFER_FOR_START_CODE, bitstrmLen, bitstrmPtr, bitstrmLen);
    if (err != 0) {
        OMX_LOGE("memmove_s failed at line %d, err=%d", __LINE__, err);
        return ERROR_RETURN_VALUE;
    }
    *pstream = START_CODE_ZERO_BYTE;
    *(pstream + START_CODE_SECOND_BYTE_INDEX) = START_CODE_ZERO_BYTE;
    *(pstream + START_CODE_THIRD_BYTE_INDEX) = START_CODE_ZERO_BYTE;
    *(pstream + START_CODE_FOURTH_BYTE_INDEX) = START_CODE_LAST_BYTE;
    return SUCCESS_RETURN_VALUE;
}
static int ParseInterlacedNal(const unsigned char *pstream, unsigned int streamLen)
{
    DEC_BS_T stream;
    unsigned int sliceUnitLen = 0;
    unsigned int naluLen = 0;
    unsigned char *naluBuf = nullptr;
    GetUnit(&naluBuf, &naluLen, const_cast<unsigned char *>(pstream), streamLen, &sliceUnitLen);
    H264DecInitBitstream(&stream, naluBuf, naluLen);
    char nalUnitType = READ_BITS(&stream, BITS_PER_BYTE) & NAL_UNIT_TYPE_MASK;
    return nalUnitType == NAL_UNIT_TYPE_SPS ? InterpretH264Header(&stream) : ERROR_RETURN_VALUE;
}
/*********************************************************
*bitstrmPtr: input bitstream buffer
*bitstrmLen: input bitstream length
*return value:
*  1      : interlaced
*  0      : not interlaced
* -1     :error
*********************************************************/
int IsInterlacedSequence(const unsigned char *bitstrmPtr, size_t bitstrmLen)
{
    if (bitstrmLen == 0 || bitstrmLen > UINT32_MAX - EXTRA_BUFFER_FOR_START_CODE) {
        SCI_TRACE_LOW("IsInterlacedSequence invalid bitstrmLen: %u", bitstrmLen);
        return ERROR_RETURN_VALUE;
    }
    size_t streamLen = bitstrmLen + EXTRA_BUFFER_FOR_START_CODE;
    unsigned char *pstream = static_cast<unsigned char*>(malloc(streamLen));
    if (pstream == nullptr) {
        SCI_TRACE_LOW("IsInterlacedSequence malloc failed");
        return ERROR_RETURN_VALUE;
    }
    if (CopyBitstreamWithStartCode(pstream, bitstrmPtr, bitstrmLen) != SUCCESS_RETURN_VALUE) {
        free(pstream);
        return ERROR_RETURN_VALUE;
    }
    int ret = ParseInterlacedNal(pstream, static_cast<unsigned int>(streamLen));
    SCI_TRACE_LOW("%s, Interlaced: %d\n", __FUNCTION__, ret);
    free(pstream);
    return ret;
}
}  // namespace OMX
}  // namespace OHOS