/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

struct idna
{
    char *name;
    size_t inlen;
    unsigned long in[100];
    char *out;
    int allowunassigned;
    int usestd3asciirules;
    int toasciirc;
    int tounicoderc;
} idnalist[] =
{
    {
        "Arabic (Egyptian)", 17,
        {
            0x0644, 0x064A, 0x0647, 0x0645, 0x0627, 0x0628, 0x062A, 0x0643,
            0x0644, 0x0645, 0x0648, 0x0634, 0x0639, 0x0631, 0x0628, 0x064A,
            0x061F},
        IDNA_ACE_PREFIX "egbpdaj6bu4bxfgehfvwxn", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Chinese (simplified)", 9,
        {
            0x4ED6, 0x4EEC, 0x4E3A, 0x4EC0, 0x4E48, 0x4E0D, 0x8BF4, 0x4E2D, 0x6587},
        IDNA_ACE_PREFIX "ihqwcrb4cv8a8dqg056pqjye", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Chinese (traditional)", 9,
        {
            0x4ED6, 0x5011, 0x7232, 0x4EC0, 0x9EBD, 0x4E0D, 0x8AAA, 0x4E2D, 0x6587},
        IDNA_ACE_PREFIX "ihqwctvzc91f659drss3x8bo0yb", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Czech", 22,
        {
            0x0050, 0x0072, 0x006F, 0x010D, 0x0070, 0x0072, 0x006F, 0x0073,
            0x0074, 0x011B, 0x006E, 0x0065, 0x006D, 0x006C, 0x0075, 0x0076,
            0x00ED, 0x010D, 0x0065, 0x0073, 0x006B, 0x0079},
        IDNA_ACE_PREFIX "Proprostnemluvesky-uyb24dma41a", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Hebrew", 22,
        {
            0x05DC, 0x05DE, 0x05D4, 0x05D4, 0x05DD, 0x05E4, 0x05E9, 0x05D5,
            0x05D8, 0x05DC, 0x05D0, 0x05DE, 0x05D3, 0x05D1, 0x05E8, 0x05D9,
            0x05DD, 0x05E2, 0x05D1, 0x05E8, 0x05D9, 0x05EA},
        IDNA_ACE_PREFIX "4dbcagdahymbxekheh6e0a7fei0b", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Hindi (Devanagari)", 30,
        {
            0x092F, 0x0939, 0x0932, 0x094B, 0x0917, 0x0939, 0x093F, 0x0928,
            0x094D, 0x0926, 0x0940, 0x0915, 0x094D, 0x092F, 0x094B, 0x0902,
            0x0928, 0x0939, 0x0940, 0x0902, 0x092C, 0x094B, 0x0932, 0x0938,
            0x0915, 0x0924, 0x0947, 0x0939, 0x0948, 0x0902},
        IDNA_ACE_PREFIX "i1baa7eci9glrd9b2ae1bj0hfcgg6iyaf8o0a1dig0cd", 0, 0,
        IDNA_SUCCESS
    }, {
        "Japanese (kanji and hiragana)", 18,
        {
            0x306A, 0x305C, 0x307F, 0x3093, 0x306A, 0x65E5, 0x672C, 0x8A9E,
            0x3092, 0x8A71, 0x3057, 0x3066, 0x304F, 0x308C, 0x306A, 0x3044,
            0x306E, 0x304B},
        IDNA_ACE_PREFIX "n8jok5ay5dzabd5bym9f0cm5685rrjetr6pdxa", 0, 0,
        IDNA_SUCCESS
    }, {
        "Russian (Cyrillic)", 28,
        {
            0x043F, 0x043E, 0x0447, 0x0435, 0x043C, 0x0443, 0x0436, 0x0435,
            0x043E, 0x043D, 0x0438, 0x043D, 0x0435, 0x0433, 0x043E, 0x0432,
            0x043E, 0x0440, 0x044F, 0x0442, 0x043F, 0x043E, 0x0440, 0x0443,
            0x0441, 0x0441, 0x043A, 0x0438},
        IDNA_ACE_PREFIX "b1abfaaepdrnnbgefbadotcwatmq2g4l", 0, 0,
        IDNA_SUCCESS, IDNA_SUCCESS
    }, {
        "Spanish", 40,
        {
            0x0050, 0x006F, 0x0072, 0x0071, 0x0075, 0x00E9, 0x006E, 0x006F,
            0x0070, 0x0075, 0x0065, 0x0064, 0x0065, 0x006E, 0x0073, 0x0069,
            0x006D, 0x0070, 0x006C, 0x0065, 0x006D, 0x0065, 0x006E, 0x0074,
            0x0065, 0x0068, 0x0061, 0x0062, 0x006C, 0x0061, 0x0072, 0x0065,
            0x006E, 0x0045, 0x0073, 0x0070, 0x0061, 0x00F1, 0x006F, 0x006C},
        IDNA_ACE_PREFIX "PorqunopuedensimplementehablarenEspaol-fmd56a", 0, 0,
        IDNA_SUCCESS
    }, {
        "Vietnamese", 31,
        {
            0x0054, 0x1EA1, 0x0069, 0x0073, 0x0061, 0x006F, 0x0068, 0x1ECD,
            0x006B, 0x0068, 0x00F4, 0x006E, 0x0067, 0x0074, 0x0068, 0x1EC3,
            0x0063, 0x0068, 0x1EC9, 0x006E, 0x00F3, 0x0069, 0x0074, 0x0069,
            0x1EBF, 0x006E, 0x0067, 0x0056, 0x0069, 0x1EC7, 0x0074},
        IDNA_ACE_PREFIX "TisaohkhngthchnitingVit-kjcr8268qyxafd2f1b9g", 0, 0,
        IDNA_SUCCESS
    }, {
        "Japanese", 8,
        {
            0x0033, 0x5E74, 0x0042, 0x7D44, 0x91D1, 0x516B, 0x5148, 0x751F},
        IDNA_ACE_PREFIX "3B-ww4c5e180e575a65lsy2b", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Japanese", 24,
        {
            0x5B89, 0x5BA4, 0x5948, 0x7F8E, 0x6075, 0x002D, 0x0077, 0x0069,
            0x0074, 0x0068, 0x002D, 0x0053, 0x0055, 0x0050, 0x0045, 0x0052,
            0x002D, 0x004D, 0x004F, 0x004E, 0x004B, 0x0045, 0x0059, 0x0053},
        IDNA_ACE_PREFIX "-with-SUPER-MONKEYS-pc58ag80a8qai00g7n9n", 0, 0,
        IDNA_SUCCESS
    }, {
        "Japanese", 25,
        {
            0x0048, 0x0065, 0x006C, 0x006C, 0x006F, 0x002D, 0x0041, 0x006E,
            0x006F, 0x0074, 0x0068, 0x0065, 0x0072, 0x002D, 0x0057, 0x0061,
            0x0079, 0x002D, 0x305D, 0x308C, 0x305E, 0x308C, 0x306E, 0x5834,
            0x6240},
        IDNA_ACE_PREFIX "Hello-Another-Way--fc4qua05auwb3674vfr0b", 0, 0,
        IDNA_SUCCESS
    }, {
        "Japanese", 8,
        {
            0x3072, 0x3068, 0x3064, 0x5C4B, 0x6839, 0x306E, 0x4E0B, 0x0032},
        IDNA_ACE_PREFIX "2-u9tlzr9756bt3uc0v", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Japanese", 13,
        {
            0x004D, 0x0061, 0x006A, 0x0069, 0x3067, 0x004B, 0x006F, 0x0069,
            0x3059, 0x308B, 0x0035, 0x79D2, 0x524D},
        IDNA_ACE_PREFIX "MajiKoi5-783gue6qz075azm5e", 0, 0, IDNA_SUCCESS,
        IDNA_SUCCESS
    }, {
        "Japanese", 9,
        {
            0x30D1, 0x30D5, 0x30A3, 0x30FC, 0x0064, 0x0065, 0x30EB, 0x30F3, 0x30D0},
        IDNA_ACE_PREFIX "de-jg4avhby1noc0d", 0, 0, IDNA_SUCCESS, IDNA_SUCCESS
    }, {
        "Japanese", 7,
        {
            0x305D, 0x306E, 0x30B9, 0x30D4, 0x30FC, 0x30C9, 0x3067},
        IDNA_ACE_PREFIX "d9juau41awczczp", 0, 0, IDNA_SUCCESS, IDNA_SUCCESS
    }, {
        "Greek", 8,
        {0x03b5, 0x03bb, 0x03bb, 0x03b7, 0x03bd, 0x03b9, 0x03ba, 0x03ac},
        IDNA_ACE_PREFIX "hxargifdar", 0, 0, IDNA_SUCCESS, IDNA_SUCCESS
    }, {
        "Maltese (Malti)", 10,
        {0x0062, 0x006f, 0x006e, 0x0121, 0x0075, 0x0073, 0x0061, 0x0127,
         0x0127, 0x0061},
        IDNA_ACE_PREFIX "bonusaa-5bb1da", 0, 0, IDNA_SUCCESS, IDNA_SUCCESS
    }, {
        "Russian (Cyrillic)", 28,
        {0x043f, 0x043e, 0x0447, 0x0435, 0x043c, 0x0443, 0x0436, 0x0435,
         0x043e, 0x043d, 0x0438, 0x043d, 0x0435, 0x0433, 0x043e, 0x0432,
         0x043e, 0x0440, 0x044f, 0x0442, 0x043f, 0x043e, 0x0440, 0x0443,
         0x0441, 0x0441, 0x043a, 0x0438},
        IDNA_ACE_PREFIX "b1abfaaepdrnnbgefbadotcwatmq2g4l", 0, 0,
        IDNA_SUCCESS, IDNA_SUCCESS
    }
};
