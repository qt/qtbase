/****************************************************************************
**
** Copyright (C) 2018 Intel Corporation
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
** IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
** AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
** LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
** OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
** THE SOFTWARE.
**
****************************************************************************/

#ifndef CBORINTERNAL_P_H
#define CBORINTERNAL_P_H

#include "compilersupport_p.h"

#ifndef CBOR_INTERNAL_API
#  define CBOR_INTERNAL_API
#endif

#ifndef CBOR_PARSER_MAX_RECURSIONS
#  define CBOR_PARSER_MAX_RECURSIONS 1024
#endif

#ifndef CBOR_ENCODER_WRITER_CONTROL
#  define CBOR_ENCODER_WRITER_CONTROL   0
#endif
#ifndef CBOR_PARSER_READER_CONTROL
#  define CBOR_PARSER_READER_CONTROL    0
#endif

/*
 * CBOR Major types
 * Encoded in the high 3 bits of the descriptor byte
 * See http://tools.ietf.org/html/rfc7049#section-2.1
 */
typedef enum CborMajorTypes {
    UnsignedIntegerType = 0U,
    NegativeIntegerType = 1U,
    ByteStringType = 2U,
    TextStringType = 3U,
    ArrayType = 4U,
    MapType = 5U,           /* a.k.a. object */
    TagType = 6U,
    SimpleTypesType = 7U
} CborMajorTypes;

/*
 * CBOR simple and floating point types
 * Encoded in the low 8 bits of the descriptor byte when the
 * Major Type is 7.
 */
typedef enum CborSimpleTypes {
    FalseValue              = 20,
    TrueValue               = 21,
    NullValue               = 22,
    UndefinedValue          = 23,
    SimpleTypeInNextByte    = 24,   /* not really a simple type */
    HalfPrecisionFloat      = 25,   /* ditto */
    SinglePrecisionFloat    = 26,   /* ditto */
    DoublePrecisionFloat    = 27,   /* ditto */
    Break                   = 31
} CborSimpleTypes;

enum {
    SmallValueBitLength     = 5U,
    SmallValueMask          = (1U << SmallValueBitLength) - 1,      /* 31 */
    Value8Bit               = 24U,
    Value16Bit              = 25U,
    Value32Bit              = 26U,
    Value64Bit              = 27U,
    IndefiniteLength        = 31U,

    MajorTypeShift          = SmallValueBitLength,
    MajorTypeMask           = (int) (~0U << MajorTypeShift),

    BreakByte               = (unsigned)Break | (SimpleTypesType << MajorTypeShift)
};

static inline void copy_current_position(CborValue *dst, const CborValue *src)
{
    // This "if" is here for pedantry only: the two branches should perform
    // the same memory operation.
    if (src->parser->flags & CborParserFlag_ExternalSource)
        dst->source.token = src->source.token;
    else
        dst->source.ptr = src->source.ptr;
}

static inline bool can_read_bytes(const CborValue *it, size_t n)
{
    if (CBOR_PARSER_READER_CONTROL >= 0) {
        if (it->parser->flags & CborParserFlag_ExternalSource || CBOR_PARSER_READER_CONTROL != 0) {
#ifdef CBOR_PARSER_CAN_READ_BYTES_FUNCTION
            return CBOR_PARSER_CAN_READ_BYTES_FUNCTION(it->source.token, n);
#else
            return it->parser->source.ops->can_read_bytes(it->source.token, n);
#endif
        }
    }

    /* Convert the pointer subtraction to size_t since end >= ptr
     * (this prevents issues with (ptrdiff_t)n becoming negative).
     */
    return (size_t)(it->parser->source.end - it->source.ptr) >= n;
}

static inline void advance_bytes(CborValue *it, size_t n)
{
    if (CBOR_PARSER_READER_CONTROL >= 0) {
        if (it->parser->flags & CborParserFlag_ExternalSource || CBOR_PARSER_READER_CONTROL != 0) {
#ifdef CBOR_PARSER_ADVANCE_BYTES_FUNCTION
            CBOR_PARSER_ADVANCE_BYTES_FUNCTION(it->source.token, n);
#else
            it->parser->source.ops->advance_bytes(it->source.token, n);
#endif
            return;
        }
    }

    it->source.ptr += n;
}

static inline CborError transfer_string(CborValue *it, const void **ptr, size_t offset, size_t len)
{
    if (CBOR_PARSER_READER_CONTROL >= 0) {
        if (it->parser->flags & CborParserFlag_ExternalSource || CBOR_PARSER_READER_CONTROL != 0) {
#ifdef CBOR_PARSER_TRANSFER_STRING_FUNCTION
            return CBOR_PARSER_TRANSFER_STRING_FUNCTION(it->source.token, ptr, offset, len);
#else
            return it->parser->source.ops->transfer_string(it->source.token, ptr, offset, len);
#endif
        }
    }

    it->source.ptr += offset;
    if (can_read_bytes(it, len)) {
        *CONST_CAST(const void **, ptr) = it->source.ptr;
        it->source.ptr += len;
        return CborNoError;
    }
    return CborErrorUnexpectedEOF;
}

static inline void *read_bytes_unchecked(const CborValue *it, void *dst, size_t offset, size_t n)
{
    if (CBOR_PARSER_READER_CONTROL >= 0) {
        if (it->parser->flags & CborParserFlag_ExternalSource || CBOR_PARSER_READER_CONTROL != 0) {
#ifdef CBOR_PARSER_READ_BYTES_FUNCTION
            return CBOR_PARSER_READ_BYTES_FUNCTION(it->source.token, dst, offset, n);
#else
            return it->parser->source.ops->read_bytes(it->source.token, dst, offset, n);
#endif
        }
    }

    return memcpy(dst, it->source.ptr + offset, n);
}

#ifdef __GNUC__
__attribute__((warn_unused_result))
#endif
static inline void *read_bytes(const CborValue *it, void *dst, size_t offset, size_t n)
{
    if (can_read_bytes(it, offset + n))
        return read_bytes_unchecked(it, dst, offset, n);
    return NULL;
}

static inline uint16_t read_uint8(const CborValue *it, size_t offset)
{
    uint8_t result;
    read_bytes_unchecked(it, &result, offset, sizeof(result));
    return result;
}

static inline uint16_t read_uint16(const CborValue *it, size_t offset)
{
    uint16_t result;
    read_bytes_unchecked(it, &result, offset, sizeof(result));
    return cbor_ntohs(result);
}

static inline uint32_t read_uint32(const CborValue *it, size_t offset)
{
    uint32_t result;
    read_bytes_unchecked(it, &result, offset, sizeof(result));
    return cbor_ntohl(result);
}

static inline uint64_t read_uint64(const CborValue *it, size_t offset)
{
    uint64_t result;
    read_bytes_unchecked(it, &result, offset, sizeof(result));
    return cbor_ntohll(result);
}

static inline CborError extract_number_checked(const CborValue *it, uint64_t *value, size_t *bytesUsed)
{
    uint8_t descriptor;
    size_t bytesNeeded = 0;

    /* We've already verified that there's at least one byte to be read */
    read_bytes_unchecked(it, &descriptor, 0, 1);
    descriptor &= SmallValueMask;
    if (descriptor < Value8Bit) {
        *value = descriptor;
    } else if (unlikely(descriptor > Value64Bit)) {
        return CborErrorIllegalNumber;
    } else {
        bytesNeeded = (size_t)(1 << (descriptor - Value8Bit));
        if (!can_read_bytes(it, 1 + bytesNeeded))
            return CborErrorUnexpectedEOF;
        if (descriptor <= Value16Bit) {
            if (descriptor == Value16Bit)
                *value = read_uint16(it, 1);
            else
                *value = read_uint8(it, 1);
        } else {
            if (descriptor == Value32Bit)
                *value = read_uint32(it, 1);
            else
                *value = read_uint64(it, 1);
        }
    }

    if (bytesUsed)
        *bytesUsed = bytesNeeded;
    return CborNoError;
}

#endif /* CBORINTERNAL_P_H */
