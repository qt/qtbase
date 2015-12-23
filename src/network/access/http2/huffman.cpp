/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bitstreams_p.h"
#include "huffman_p.h"

#include <QtCore/qbytearray.h>

#include <algorithm>
#include <limits>

QT_BEGIN_NAMESPACE

namespace HPack
{

/*
    The static Huffman code used here was extracted from:
    https://http2.github.io/http2-spec/compression.html#huffman.code

    This code was generated from statistics obtained on a large
    sample of HTTP headers. It is a canonical Huffman code
    with some tweaking to ensure that no symbol has a unique
    code length. All codes were left-aligned - for implementation
    convenience.

    Using binary trees to implement decoding would be prohibitively
    expensive (both memory and time-wise). Instead we use a table-based
    approach and any given code itself works as an index into such table(s).
    We have 256 possible byte values and code lengths in
    a range [5, 26]. This would require a huge table (and most of entries
    would be 'wasted', since we only have to encode 256 elements).
    Instead we use multi-level tables. The first level table
    is using 9-bit length index; some entries in this table are 'terminal',
    some reference the next level table(s).

    For example, bytes with values 48 and 49 (ASCII codes for '0' and '1')
    both have code length 5, Huffman codes are: 00000 and 00001. They
    both are placed in the 'root' table,
    the 'root' table has index length == 9:
    [00000 | 4 remaining bits]
     ...
    [00001 | 4 remaining bits]

    All entires with indices between these two will 'point' to value 48
    with bitLength == 5 so that bit stream (for example) 000001010 will be
    decoded as: 48 + "put 1010 back into bitstream".

    A good description can be found here:
    http://commandlinefanatic.com/cgi-bin/showarticle.cgi?article=art007
    or just google "Efficient Huffman Decoding".
    Also see comments below about 'filling holes'.
*/

namespace
{

const CodeEntry staticHuffmanCodeTable[]
{
    {  0, 0xffc00000ul, 13},  //     11111111|11000
    {  1, 0xffffb000ul, 23},  //     11111111|11111111|1011000
    {  2, 0xfffffe20ul, 28},  //     11111111|11111111|11111110|0010
    {  3, 0xfffffe30ul, 28},  //     11111111|11111111|11111110|0011
    {  4, 0xfffffe40ul, 28},  //     11111111|11111111|11111110|0100
    {  5, 0xfffffe50ul, 28},  //     11111111|11111111|11111110|0101
    {  6, 0xfffffe60ul, 28},  //     11111111|11111111|11111110|0110
    {  7, 0xfffffe70ul, 28},  //     11111111|11111111|11111110|0111
    {  8, 0xfffffe80ul, 28},  //     11111111|11111111|11111110|1000
    {  9, 0xffffea00ul, 24},  //     11111111|11111111|11101010
    { 10, 0xfffffff0ul, 30},  //     11111111|11111111|11111111|111100
    { 11, 0xfffffe90ul, 28},  //     11111111|11111111|11111110|1001
    { 12, 0xfffffea0ul, 28},  //     11111111|11111111|11111110|1010
    { 13, 0xfffffff4ul, 30},  //     11111111|11111111|11111111|111101
    { 14, 0xfffffeb0ul, 28},  //     11111111|11111111|11111110|1011
    { 15, 0xfffffec0ul, 28},  //     11111111|11111111|11111110|1100
    { 16, 0xfffffed0ul, 28},  //     11111111|11111111|11111110|1101
    { 17, 0xfffffee0ul, 28},  //     11111111|11111111|11111110|1110
    { 18, 0xfffffef0ul, 28},  //     11111111|11111111|11111110|1111
    { 19, 0xffffff00ul, 28},  //     11111111|11111111|11111111|0000
    { 20, 0xffffff10ul, 28},  //     11111111|11111111|11111111|0001
    { 21, 0xffffff20ul, 28},  //     11111111|11111111|11111111|0010
    { 22, 0xfffffff8ul, 30},  //     11111111|11111111|11111111|111110
    { 23, 0xffffff30ul, 28},  //     11111111|11111111|11111111|0011
    { 24, 0xffffff40ul, 28},  //     11111111|11111111|11111111|0100
    { 25, 0xffffff50ul, 28},  //     11111111|11111111|11111111|0101
    { 26, 0xffffff60ul, 28},  //     11111111|11111111|11111111|0110
    { 27, 0xffffff70ul, 28},  //     11111111|11111111|11111111|0111
    { 28, 0xffffff80ul, 28},  //     11111111|11111111|11111111|1000
    { 29, 0xffffff90ul, 28},  //     11111111|11111111|11111111|1001
    { 30, 0xffffffa0ul, 28},  //     11111111|11111111|11111111|1010
    { 31, 0xffffffb0ul, 28},  //     11111111|11111111|11111111|1011
    { 32, 0x50000000ul,  6},  // ' ' 010100
    { 33, 0xfe000000ul, 10},  // '!' 11111110|00
    { 34, 0xfe400000ul, 10},  // '"' 11111110|01
    { 35, 0xffa00000ul, 12},  // '#' 11111111|1010
    { 36, 0xffc80000ul, 13},  // '$' 11111111|11001
    { 37, 0x54000000ul,  6},  // '%' 010101
    { 38, 0xf8000000ul,  8},  // '&' 11111000
    { 39, 0xff400000ul, 11},  // ''' 11111111|010
    { 40, 0xfe800000ul, 10},  // '(' 11111110|10
    { 41, 0xfec00000ul, 10},  // ')' 11111110|11
    { 42, 0xf9000000ul,  8},  // '*' 11111001
    { 43, 0xff600000ul, 11},  // '+' 11111111|011
    { 44, 0xfa000000ul,  8},  // ',' 11111010
    { 45, 0x58000000ul,  6},  // '-' 010110
    { 46, 0x5c000000ul,  6},  // '.' 010111
    { 47, 0x60000000ul,  6},  // '/' 011000
    { 48, 0x00000000ul,  5},  // '0' 00000
    { 49, 0x08000000ul,  5},  // '1' 00001
    { 50, 0x10000000ul,  5},  // '2' 00010
    { 51, 0x64000000ul,  6},  // '3' 011001
    { 52, 0x68000000ul,  6},  // '4' 011010
    { 53, 0x6c000000ul,  6},  // '5' 011011
    { 54, 0x70000000ul,  6},  // '6' 011100
    { 55, 0x74000000ul,  6},  // '7' 011101
    { 56, 0x78000000ul,  6},  // '8' 011110
    { 57, 0x7c000000ul,  6},  // '9' 011111
    { 58, 0xb8000000ul,  7},  // ':' 1011100
    { 59, 0xfb000000ul,  8},  // ';' 11111011
    { 60, 0xfff80000ul, 15},  // '<' 11111111|1111100
    { 61, 0x80000000ul,  6},  // '=' 100000
    { 62, 0xffb00000ul, 12},  // '>' 11111111|1011
    { 63, 0xff000000ul, 10},  // '?' 11111111|00
    { 64, 0xffd00000ul, 13},  // '@' 11111111|11010
    { 65, 0x84000000ul,  6},  // 'A' 100001
    { 66, 0xba000000ul,  7},  // 'B' 1011101
    { 67, 0xbc000000ul,  7},  // 'C' 1011110
    { 68, 0xbe000000ul,  7},  // 'D' 1011111
    { 69, 0xc0000000ul,  7},  // 'E' 1100000
    { 70, 0xc2000000ul,  7},  // 'F' 1100001
    { 71, 0xc4000000ul,  7},  // 'G' 1100010
    { 72, 0xc6000000ul,  7},  // 'H' 1100011
    { 73, 0xc8000000ul,  7},  // 'I' 1100100
    { 74, 0xca000000ul,  7},  // 'J' 1100101
    { 75, 0xcc000000ul,  7},  // 'K' 1100110
    { 76, 0xce000000ul,  7},  // 'L' 1100111
    { 77, 0xd0000000ul,  7},  // 'M' 1101000
    { 78, 0xd2000000ul,  7},  // 'N' 1101001
    { 79, 0xd4000000ul,  7},  // 'O' 1101010
    { 80, 0xd6000000ul,  7},  // 'P' 1101011
    { 81, 0xd8000000ul,  7},  // 'Q' 1101100
    { 82, 0xda000000ul,  7},  // 'R' 1101101
    { 83, 0xdc000000ul,  7},  // 'S' 1101110
    { 84, 0xde000000ul,  7},  // 'T' 1101111
    { 85, 0xe0000000ul,  7},  // 'U' 1110000
    { 86, 0xe2000000ul,  7},  // 'V' 1110001
    { 87, 0xe4000000ul,  7},  // 'W' 1110010
    { 88, 0xfc000000ul,  8},  // 'X' 11111100
    { 89, 0xe6000000ul,  7},  // 'Y' 1110011
    { 90, 0xfd000000ul,  8},  // 'Z' 11111101
    { 91, 0xffd80000ul, 13},  // '[' 11111111|11011
    { 92, 0xfffe0000ul, 19},  // '\' 11111111|11111110|000
    { 93, 0xffe00000ul, 13},  // ']' 11111111|11100
    { 94, 0xfff00000ul, 14},  // '^' 11111111|111100
    { 95, 0x88000000ul,  6},  // '_' 100010
    { 96, 0xfffa0000ul, 15},  // '`' 11111111|1111101
    { 97, 0x18000000ul,  5},  // 'a' 00011
    { 98, 0x8c000000ul,  6},  // 'b' 100011
    { 99, 0x20000000ul,  5},  // 'c' 00100
    {100, 0x90000000ul,  6},  // 'd' 100100
    {101, 0x28000000ul,  5},  // 'e' 00101
    {102, 0x94000000ul,  6},  // 'f' 100101
    {103, 0x98000000ul,  6},  // 'g' 100110
    {104, 0x9c000000ul,  6},  // 'h' 100111
    {105, 0x30000000ul,  5},  // 'i' 00110
    {106, 0xe8000000ul,  7},  // 'j' 1110100
    {107, 0xea000000ul,  7},  // 'k' 1110101
    {108, 0xa0000000ul,  6},  // 'l' 101000
    {109, 0xa4000000ul,  6},  // 'm' 101001
    {110, 0xa8000000ul,  6},  // 'n' 101010
    {111, 0x38000000ul,  5},  // 'o' 00111
    {112, 0xac000000ul,  6},  // 'p' 101011
    {113, 0xec000000ul,  7},  // 'q' 1110110
    {114, 0xb0000000ul,  6},  // 'r' 101100
    {115, 0x40000000ul,  5},  // 's' 01000
    {116, 0x48000000ul,  5},  // 't' 01001
    {117, 0xb4000000ul,  6},  // 'u' 101101
    {118, 0xee000000ul,  7},  // 'v' 1110111
    {119, 0xf0000000ul,  7},  // 'w' 1111000
    {120, 0xf2000000ul,  7},  // 'x' 1111001
    {121, 0xf4000000ul,  7},  // 'y' 1111010
    {122, 0xf6000000ul,  7},  // 'z' 1111011
    {123, 0xfffc0000ul, 15},  // '{' 11111111|1111110
    {124, 0xff800000ul, 11},  // '|' 11111111|100
    {125, 0xfff40000ul, 14},  // '}' 11111111|111101
    {126, 0xffe80000ul, 13},  // '~' 11111111|11101
    {127, 0xffffffc0ul, 28},  //     11111111|11111111|11111111|1100
    {128, 0xfffe6000ul, 20},  //     11111111|11111110|0110
    {129, 0xffff4800ul, 22},  //     11111111|11111111|010010
    {130, 0xfffe7000ul, 20},  //     11111111|11111110|0111
    {131, 0xfffe8000ul, 20},  //     11111111|11111110|1000
    {132, 0xffff4c00ul, 22},  //     11111111|11111111|010011
    {133, 0xffff5000ul, 22},  //     11111111|11111111|010100
    {134, 0xffff5400ul, 22},  //     11111111|11111111|010101
    {135, 0xffffb200ul, 23},  //     11111111|11111111|1011001
    {136, 0xffff5800ul, 22},  //     11111111|11111111|010110
    {137, 0xffffb400ul, 23},  //     11111111|11111111|1011010
    {138, 0xffffb600ul, 23},  //     11111111|11111111|1011011
    {139, 0xffffb800ul, 23},  //     11111111|11111111|1011100
    {140, 0xffffba00ul, 23},  //     11111111|11111111|1011101
    {141, 0xffffbc00ul, 23},  //     11111111|11111111|1011110
    {142, 0xffffeb00ul, 24},  //     11111111|11111111|11101011
    {143, 0xffffbe00ul, 23},  //     11111111|11111111|1011111
    {144, 0xffffec00ul, 24},  //     11111111|11111111|11101100
    {145, 0xffffed00ul, 24},  //     11111111|11111111|11101101
    {146, 0xffff5c00ul, 22},  //     11111111|11111111|010111
    {147, 0xffffc000ul, 23},  //     11111111|11111111|1100000
    {148, 0xffffee00ul, 24},  //     11111111|11111111|11101110
    {149, 0xffffc200ul, 23},  //     11111111|11111111|1100001
    {150, 0xffffc400ul, 23},  //     11111111|11111111|1100010
    {151, 0xffffc600ul, 23},  //     11111111|11111111|1100011
    {152, 0xffffc800ul, 23},  //     11111111|11111111|1100100
    {153, 0xfffee000ul, 21},  //     11111111|11111110|11100
    {154, 0xffff6000ul, 22},  //     11111111|11111111|011000
    {155, 0xffffca00ul, 23},  //     11111111|11111111|1100101
    {156, 0xffff6400ul, 22},  //     11111111|11111111|011001
    {157, 0xffffcc00ul, 23},  //     11111111|11111111|1100110
    {158, 0xffffce00ul, 23},  //     11111111|11111111|1100111
    {159, 0xffffef00ul, 24},  //     11111111|11111111|11101111
    {160, 0xffff6800ul, 22},  //     11111111|11111111|011010
    {161, 0xfffee800ul, 21},  //     11111111|11111110|11101
    {162, 0xfffe9000ul, 20},  //     11111111|11111110|1001
    {163, 0xffff6c00ul, 22},  //     11111111|11111111|011011
    {164, 0xffff7000ul, 22},  //     11111111|11111111|011100
    {165, 0xffffd000ul, 23},  //     11111111|11111111|1101000
    {166, 0xffffd200ul, 23},  //     11111111|11111111|1101001
    {167, 0xfffef000ul, 21},  //     11111111|11111110|11110
    {168, 0xffffd400ul, 23},  //     11111111|11111111|1101010
    {169, 0xffff7400ul, 22},  //     11111111|11111111|011101
    {170, 0xffff7800ul, 22},  //     11111111|11111111|011110
    {171, 0xfffff000ul, 24},  //     11111111|11111111|11110000
    {172, 0xfffef800ul, 21},  //     11111111|11111110|11111
    {173, 0xffff7c00ul, 22},  //     11111111|11111111|011111
    {174, 0xffffd600ul, 23},  //     11111111|11111111|1101011
    {175, 0xffffd800ul, 23},  //     11111111|11111111|1101100
    {176, 0xffff0000ul, 21},  //     11111111|11111111|00000
    {177, 0xffff0800ul, 21},  //     11111111|11111111|00001
    {178, 0xffff8000ul, 22},  //     11111111|11111111|100000
    {179, 0xffff1000ul, 21},  //     11111111|11111111|00010
    {180, 0xffffda00ul, 23},  //     11111111|11111111|1101101
    {181, 0xffff8400ul, 22},  //     11111111|11111111|100001
    {182, 0xffffdc00ul, 23},  //     11111111|11111111|1101110
    {183, 0xffffde00ul, 23},  //     11111111|11111111|1101111
    {184, 0xfffea000ul, 20},  //     11111111|11111110|1010
    {185, 0xffff8800ul, 22},  //     11111111|11111111|100010
    {186, 0xffff8c00ul, 22},  //     11111111|11111111|100011
    {187, 0xffff9000ul, 22},  //     11111111|11111111|100100
    {188, 0xffffe000ul, 23},  //     11111111|11111111|1110000
    {189, 0xffff9400ul, 22},  //     11111111|11111111|100101
    {190, 0xffff9800ul, 22},  //     11111111|11111111|100110
    {191, 0xffffe200ul, 23},  //     11111111|11111111|1110001
    {192, 0xfffff800ul, 26},  //     11111111|11111111|11111000|00
    {193, 0xfffff840ul, 26},  //     11111111|11111111|11111000|01
    {194, 0xfffeb000ul, 20},  //     11111111|11111110|1011
    {195, 0xfffe2000ul, 19},  //     11111111|11111110|001
    {196, 0xffff9c00ul, 22},  //     11111111|11111111|100111
    {197, 0xffffe400ul, 23},  //     11111111|11111111|1110010
    {198, 0xffffa000ul, 22},  //     11111111|11111111|101000
    {199, 0xfffff600ul, 25},  //     11111111|11111111|11110110|0
    {200, 0xfffff880ul, 26},  //     11111111|11111111|11111000|10
    {201, 0xfffff8c0ul, 26},  //     11111111|11111111|11111000|11
    {202, 0xfffff900ul, 26},  //     11111111|11111111|11111001|00
    {203, 0xfffffbc0ul, 27},  //     11111111|11111111|11111011|110
    {204, 0xfffffbe0ul, 27},  //     11111111|11111111|11111011|111
    {205, 0xfffff940ul, 26},  //     11111111|11111111|11111001|01
    {206, 0xfffff100ul, 24},  //     11111111|11111111|11110001
    {207, 0xfffff680ul, 25},  //     11111111|11111111|11110110|1
    {208, 0xfffe4000ul, 19},  //     11111111|11111110|010
    {209, 0xffff1800ul, 21},  //     11111111|11111111|00011
    {210, 0xfffff980ul, 26},  //     11111111|11111111|11111001|10
    {211, 0xfffffc00ul, 27},  //     11111111|11111111|11111100|000
    {212, 0xfffffc20ul, 27},  //     11111111|11111111|11111100|001
    {213, 0xfffff9c0ul, 26},  //     11111111|11111111|11111001|11
    {214, 0xfffffc40ul, 27},  //     11111111|11111111|11111100|010
    {215, 0xfffff200ul, 24},  //     11111111|11111111|11110010
    {216, 0xffff2000ul, 21},  //     11111111|11111111|00100
    {217, 0xffff2800ul, 21},  //     11111111|11111111|00101
    {218, 0xfffffa00ul, 26},  //     11111111|11111111|11111010|00
    {219, 0xfffffa40ul, 26},  //     11111111|11111111|11111010|01
    {220, 0xffffffd0ul, 28},  //     11111111|11111111|11111111|1101
    {221, 0xfffffc60ul, 27},  //     11111111|11111111|11111100|011
    {222, 0xfffffc80ul, 27},  //     11111111|11111111|11111100|100
    {223, 0xfffffca0ul, 27},  //     11111111|11111111|11111100|101
    {224, 0xfffec000ul, 20},  //     11111111|11111110|1100
    {225, 0xfffff300ul, 24},  //     11111111|11111111|11110011
    {226, 0xfffed000ul, 20},  //     11111111|11111110|1101
    {227, 0xffff3000ul, 21},  //     11111111|11111111|00110
    {228, 0xffffa400ul, 22},  //     11111111|11111111|101001
    {229, 0xffff3800ul, 21},  //     11111111|11111111|00111
    {230, 0xffff4000ul, 21},  //     11111111|11111111|01000
    {231, 0xffffe600ul, 23},  //     11111111|11111111|1110011
    {232, 0xffffa800ul, 22},  //     11111111|11111111|101010
    {233, 0xffffac00ul, 22},  //     11111111|11111111|101011
    {234, 0xfffff700ul, 25},  //     11111111|11111111|11110111|0
    {235, 0xfffff780ul, 25},  //     11111111|11111111|11110111|1
    {236, 0xfffff400ul, 24},  //     11111111|11111111|11110100
    {237, 0xfffff500ul, 24},  //     11111111|11111111|11110101
    {238, 0xfffffa80ul, 26},  //     11111111|11111111|11111010|10
    {239, 0xffffe800ul, 23},  //     11111111|11111111|1110100
    {240, 0xfffffac0ul, 26},  //     11111111|11111111|11111010|11
    {241, 0xfffffcc0ul, 27},  //     11111111|11111111|11111100|110
    {242, 0xfffffb00ul, 26},  //     11111111|11111111|11111011|00
    {243, 0xfffffb40ul, 26},  //     11111111|11111111|11111011|01
    {244, 0xfffffce0ul, 27},  //     11111111|11111111|11111100|111
    {245, 0xfffffd00ul, 27},  //     11111111|11111111|11111101|000
    {246, 0xfffffd20ul, 27},  //     11111111|11111111|11111101|001
    {247, 0xfffffd40ul, 27},  //     11111111|11111111|11111101|010
    {248, 0xfffffd60ul, 27},  //     11111111|11111111|11111101|011
    {249, 0xffffffe0ul, 28},  //     11111111|11111111|11111111|1110
    {250, 0xfffffd80ul, 27},  //     11111111|11111111|11111101|100
    {251, 0xfffffda0ul, 27},  //     11111111|11111111|11111101|101
    {252, 0xfffffdc0ul, 27},  //     11111111|11111111|11111101|110
    {253, 0xfffffde0ul, 27},  //     11111111|11111111|11111101|111
    {254, 0xfffffe00ul, 27},  //     11111111|11111111|11111110|000
    {255, 0xfffffb80ul, 26},  //     11111111|11111111|11111011|10
    {256, 0xfffffffcul, 30}   // EOS 11111111|11111111|11111111|111111
};

void write_huffman_code(BitOStream &outputStream, const CodeEntry &code)
{
    // Append octet by octet.
    auto bitLength = code.bitLength;
    const auto hc = code.huffmanCode >> (32 - bitLength);

    if (bitLength > 24) {
        outputStream.writeBits(uchar(hc >> 24), bitLength - 24);
        bitLength = 24;
    }

    if (bitLength > 16) {
        outputStream.writeBits(uchar(hc >> 16), bitLength - 16);
        bitLength = 16;
    }

    if (bitLength > 8) {
        outputStream.writeBits(uchar(hc >> 8), bitLength - 8);
        bitLength = 8;
    }

    outputStream.writeBits(uchar(hc), bitLength);
}

}

// That's from HPACK's specs - we deal with octets.
static_assert(std::numeric_limits<uchar>::digits == 8, "octets expected");

quint64 huffman_encoded_bit_length(const QByteArray &inputData)
{
    quint64 bitLength = 0;
    for (int i = 0, e = inputData.size(); i < e; ++i)
        bitLength += staticHuffmanCodeTable[int(inputData[i])].bitLength;

    return bitLength;
}

void huffman_encode_string(const QByteArray &inputData, BitOStream &outputStream)
{
    for (int i = 0, e = inputData.size(); i < e; ++i)
        write_huffman_code(outputStream, staticHuffmanCodeTable[int(inputData[i])]);

    // Pad bits ...
    if (outputStream.bitLength() % 8)
        outputStream.writeBits(0xff, 8 - outputStream.bitLength() % 8);
}

bool padding_is_valid(quint32 chunk, quint32 nBits)
{
    Q_ASSERT(nBits);

    // HPACK, 5.2: "A padding strictly longer than 7 bits MUST be
    // treated as a decoding error."
    if (nBits > 7)
        return false;
    // HPACK, 5.2:
    // "A padding not corresponding to the most significant bits
    // of the code for the EOS symbol MUST be treated as a decoding error."
    return (chunk >> (32 - nBits)) == quint32((1 << nBits) - 1);
}

HuffmanDecoder::HuffmanDecoder()
    : minCodeLength()
{
    const auto nCodes = sizeof staticHuffmanCodeTable / sizeof staticHuffmanCodeTable[0];

    std::vector<CodeEntry> symbols(staticHuffmanCodeTable, staticHuffmanCodeTable + nCodes);
    // Now we sort: by bit length first (in the descending order) and by the symbol
    // value (descending). Descending order: to make sure we do not create prefix tables with
    // short 'indexLength' first and having longer codes that do not fit into such tables later.
    std::sort(symbols.begin(), symbols.end(), [](const CodeEntry &code1, const CodeEntry &code2) {
        if (code1.bitLength == code2.bitLength)
            return code1.byteValue > code2.byteValue;
        return code1.bitLength > code2.bitLength;
    });

    minCodeLength = symbols.back().bitLength; // The shortest one, currently it's 5.

    // TODO: add a verification - Huffman codes
    // within a given bit length range also
    // should be in descending order.

    // Initialize 'prefixTables' and 'tableData'.
    addTable(0, quint32(BitConstants::rootPrefix)); // 'root' table.

    for (const auto &s : symbols) {
        quint32 tableIndex = 0;
        while (true) {
            Q_ASSERT(tableIndex < prefixTables.size());
            // Note, by value - since prefixTables will be updated in between.
            const auto table = prefixTables[tableIndex];
            // We skip prefixed bits (if any) and use indexed bits only:
            const auto entryIndex = s.huffmanCode << table.prefixLength >> (32 - table.indexLength);
            // Again, by value.
            PrefixTableEntry entry = tableEntry(table, entryIndex);
            // How many bits were coded by previous tables and this table:
            const auto codedLength = table.prefixLength + table.indexLength;
            if (codedLength < s.bitLength) {
                // We have to add a new prefix table ... (if it's not done yet).
                if (!entry.bitLength) {
                    entry.nextTable = addTable(codedLength, std::min<quint32>(quint32(BitConstants::childPrefix),
                                                                              s.bitLength - codedLength));
                    entry.bitLength = s.bitLength;
                    entry.byteValue = s.byteValue;
                    setTableEntry(table, entryIndex, entry);
                }
                tableIndex = entry.nextTable;
            } else {
                // We found the slot for our code (terminal entry):
                entry.byteValue = s.byteValue;
                entry.bitLength = s.bitLength;
                // Refer to our own table as 'nextTable':
                entry.nextTable = tableIndex;
                setTableEntry(table, entryIndex, entry);
                break;
            }
        }
    }

    // Now, we have a table(s) and have to fill 'holes' to
    // 'fix' terminal entries.
    for (const auto &table : prefixTables) {
        const quint32 codedLength = table.prefixLength + table.indexLength;
        for (quint32 j = 0; j < table.size();) {
            const PrefixTableEntry &entry = tableEntry(table, j);
            if (entry.bitLength && entry.bitLength < codedLength) {
                const quint32 range = 1 << (codedLength - entry.bitLength);
                for (quint32 k = 1; k < range; ++k)
                    setTableEntry(table, j + k, entry);
                j += range;
            } else {
                ++j;
            }
        }
    }
}

bool HuffmanDecoder::decodeStream(BitIStream &inputStream, QByteArray &outputBuffer)
{
    while (true) {
        quint32 chunk = 0;
        const quint32 readBits = inputStream.peekBits(inputStream.streamOffset(), 32, &chunk);
        if (!readBits)
            return !inputStream.hasMoreBits();

        if (readBits < minCodeLength) {
            inputStream.skipBits(readBits);
            return padding_is_valid(chunk, readBits);
        }

        quint32 tableIndex = 0;
        const PrefixTable *table = &prefixTables[tableIndex];
        quint32 entryIndex = chunk >> (32 - table->indexLength);
        PrefixTableEntry entry = tableEntry(*table, entryIndex);

        while (true) {
            if (entry.nextTable == tableIndex)
                break;

            tableIndex = entry.nextTable;
            table = &prefixTables[tableIndex];
            entryIndex = chunk << table->prefixLength >> (32 - table->indexLength);
            entry = tableEntry(*table, entryIndex);
        }

        if (entry.bitLength > readBits) {
            inputStream.skipBits(readBits);
            return padding_is_valid(chunk, readBits);
        }

        if (!entry.bitLength || entry.byteValue == 256) {
            //EOS (256) == compression error (HPACK).
            inputStream.skipBits(readBits);
            return false;
        }

        outputBuffer.append(entry.byteValue);
        inputStream.skipBits(entry.bitLength);
    }

    return false;
}

quint32 HuffmanDecoder::addTable(quint32 prefix, quint32 index)
{
    PrefixTable newTable{prefix, index};
    newTable.offset = quint32(tableData.size());
    prefixTables.push_back(newTable);
    // Add entries for this table:
    tableData.resize(tableData.size() + newTable.size());

    return quint32(prefixTables.size() - 1);
}

PrefixTableEntry HuffmanDecoder::tableEntry(const PrefixTable &table, quint32 index)
{
    Q_ASSERT(index < table.size());
    return tableData[table.offset + index];
}

void HuffmanDecoder::setTableEntry(const PrefixTable &table, quint32 index,
                                   const PrefixTableEntry &entry)
{
    Q_ASSERT(index < table.size());
    tableData[table.offset + index] = entry;
}

bool huffman_decode_string(BitIStream &inputStream, QByteArray *outputBuffer)
{
    Q_ASSERT(outputBuffer);

    static HuffmanDecoder decoder;
    return decoder.decodeStream(inputStream, *outputBuffer);
}

}

QT_END_NAMESPACE
