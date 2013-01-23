/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#ifndef QHARFBUZZ_P_H
#define QHARFBUZZ_P_H

#include <QtCore/qchar.h>
#include <private/harfbuzz-shaper.h>

QT_BEGIN_NAMESPACE

static inline HB_Script script_to_hbscript(uchar script)
{
    switch (script) {
    case QChar::Script_Inherited: return HB_Script_Inherited;
    case QChar::Script_Common: return HB_Script_Common;
    case QChar::Script_Arabic: return HB_Script_Arabic;
    case QChar::Script_Armenian: return HB_Script_Armenian;
    case QChar::Script_Bengali: return HB_Script_Bengali;
    case QChar::Script_Cyrillic: return HB_Script_Cyrillic;
    case QChar::Script_Devanagari: return HB_Script_Devanagari;
    case QChar::Script_Georgian: return HB_Script_Georgian;
    case QChar::Script_Greek: return HB_Script_Greek;
    case QChar::Script_Gujarati: return HB_Script_Gujarati;
    case QChar::Script_Gurmukhi: return HB_Script_Gurmukhi;
    case QChar::Script_Hangul: return HB_Script_Hangul;
    case QChar::Script_Hebrew: return HB_Script_Hebrew;
    case QChar::Script_Kannada: return HB_Script_Kannada;
    case QChar::Script_Khmer: return HB_Script_Khmer;
    case QChar::Script_Lao: return HB_Script_Lao;
    case QChar::Script_Malayalam: return HB_Script_Malayalam;
    case QChar::Script_Myanmar: return HB_Script_Myanmar;
    case QChar::Script_Ogham: return HB_Script_Ogham;
    case QChar::Script_Oriya: return HB_Script_Oriya;
    case QChar::Script_Runic: return HB_Script_Runic;
    case QChar::Script_Sinhala: return HB_Script_Sinhala;
    case QChar::Script_Syriac: return HB_Script_Syriac;
    case QChar::Script_Tamil: return HB_Script_Tamil;
    case QChar::Script_Telugu: return HB_Script_Telugu;
    case QChar::Script_Thaana: return HB_Script_Thaana;
    case QChar::Script_Thai: return HB_Script_Thai;
    case QChar::Script_Tibetan: return HB_Script_Tibetan;
    case QChar::Script_Nko: return HB_Script_Nko;
    default: break;
    };
    return HB_Script_Common;
}

static inline uchar hbscript_to_script(uchar script)
{
    switch (script) {
    case HB_Script_Inherited: return QChar::Script_Inherited;
    case HB_Script_Common: return QChar::Script_Common;
    case HB_Script_Arabic: return QChar::Script_Arabic;
    case HB_Script_Armenian: return QChar::Script_Armenian;
    case HB_Script_Bengali: return QChar::Script_Bengali;
    case HB_Script_Cyrillic: return QChar::Script_Cyrillic;
    case HB_Script_Devanagari: return QChar::Script_Devanagari;
    case HB_Script_Georgian: return QChar::Script_Georgian;
    case HB_Script_Greek: return QChar::Script_Greek;
    case HB_Script_Gujarati: return QChar::Script_Gujarati;
    case HB_Script_Gurmukhi: return QChar::Script_Gurmukhi;
    case HB_Script_Hangul: return QChar::Script_Hangul;
    case HB_Script_Hebrew: return QChar::Script_Hebrew;
    case HB_Script_Kannada: return QChar::Script_Kannada;
    case HB_Script_Khmer: return QChar::Script_Khmer;
    case HB_Script_Lao: return QChar::Script_Lao;
    case HB_Script_Malayalam: return QChar::Script_Malayalam;
    case HB_Script_Myanmar: return QChar::Script_Myanmar;
    case HB_Script_Ogham: return QChar::Script_Ogham;
    case HB_Script_Oriya: return QChar::Script_Oriya;
    case HB_Script_Runic: return QChar::Script_Runic;
    case HB_Script_Sinhala: return QChar::Script_Sinhala;
    case HB_Script_Syriac: return QChar::Script_Syriac;
    case HB_Script_Tamil: return QChar::Script_Tamil;
    case HB_Script_Telugu: return QChar::Script_Telugu;
    case HB_Script_Thaana: return QChar::Script_Thaana;
    case HB_Script_Thai: return QChar::Script_Thai;
    case HB_Script_Tibetan: return QChar::Script_Tibetan;
    case HB_Script_Nko: return QChar::Script_Nko;
    default: break;
    };
    return QChar::Script_Common;
}

Q_CORE_EXPORT HB_Bool qShapeItem(HB_ShaperItem *item);

// ### temporary
Q_CORE_EXPORT HB_Face qHBNewFace(void *font, HB_GetFontTableFunc tableFunc);
Q_CORE_EXPORT void qHBFreeFace(HB_Face);
Q_CORE_EXPORT HB_Face qHBLoadFace(HB_Face face);

Q_DECLARE_TYPEINFO(HB_GlyphAttributes, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(HB_FixedPoint, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif
