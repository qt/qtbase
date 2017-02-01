/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QSIZEPOLICY_H
#define QSIZEPOLICY_H

#include <QtWidgets/qtwidgetsglobal.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE

// gcc < 4.8.0 has problems with init'ing variant members in constexpr ctors
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54922
#if !defined(Q_CC_GNU) || defined(Q_CC_INTEL) || defined(Q_CC_CLANG) || Q_CC_GNU >= 408
# define QT_SIZEPOLICY_CONSTEXPR Q_DECL_CONSTEXPR
# if defined(Q_COMPILER_UNIFORM_INIT)
#  define QT_SIZEPOLICY_CONSTEXPR_AND_UNIFORM_INIT Q_DECL_CONSTEXPR
#  if defined(Q_COMPILER_CONSTEXPR)
#   define QT_SIZEPOLICY_RETURN_BITS(E1, E2, E3, E4, E5, E6, E7, E8) \
        return Bits{ E1, E2, E3, E4, E5, E6, E7, E8 }
#  endif // constexpr && uniform-init
# endif // uniform-init
#endif

#ifndef QT_SIZEPOLICY_CONSTEXPR
# define QT_SIZEPOLICY_CONSTEXPR
#endif
#ifndef QT_SIZEPOLICY_CONSTEXPR_AND_UNIFORM_INIT
# define QT_SIZEPOLICY_CONSTEXPR_AND_UNIFORM_INIT
#endif
#ifndef QT_SIZEPOLICY_RETURN_BITS
# define QT_SIZEPOLICY_RETURN_BITS(E1, E2, E3, E4, E5, E6, E7, E8) \
    const Bits result = { E1, E2, E3, E4, E5, E6, E7, E8 }; return result
#endif

class QVariant;
class QSizePolicy;

Q_DECL_CONST_FUNCTION inline uint qHash(QSizePolicy key, uint seed = 0) Q_DECL_NOTHROW;

class Q_WIDGETS_EXPORT QSizePolicy
{
    Q_GADGET

public:
    enum PolicyFlag {
        GrowFlag = 1,
        ExpandFlag = 2,
        ShrinkFlag = 4,
        IgnoreFlag = 8
    };

    enum Policy {
        Fixed = 0,
        Minimum = GrowFlag,
        Maximum = ShrinkFlag,
        Preferred = GrowFlag | ShrinkFlag,
        MinimumExpanding = GrowFlag | ExpandFlag,
        Expanding = GrowFlag | ShrinkFlag | ExpandFlag,
        Ignored = ShrinkFlag | GrowFlag | IgnoreFlag
    };
    Q_ENUM(Policy)

    enum ControlType {
        DefaultType      = 0x00000001,
        ButtonBox        = 0x00000002,
        CheckBox         = 0x00000004,
        ComboBox         = 0x00000008,
        Frame            = 0x00000010,
        GroupBox         = 0x00000020,
        Label            = 0x00000040,
        Line             = 0x00000080,
        LineEdit         = 0x00000100,
        PushButton       = 0x00000200,
        RadioButton      = 0x00000400,
        Slider           = 0x00000800,
        SpinBox          = 0x00001000,
        TabWidget        = 0x00002000,
        ToolButton       = 0x00004000
    };
    Q_DECLARE_FLAGS(ControlTypes, ControlType)
    Q_FLAG(ControlTypes)

    QT_SIZEPOLICY_CONSTEXPR QSizePolicy() : data(0) { }

    QSizePolicy(Policy horizontal, Policy vertical, ControlType type = DefaultType)
        : data(0) {
        bits.horPolicy = horizontal;
        bits.verPolicy = vertical;
        setControlType(type);
    }
    QT_SIZEPOLICY_CONSTEXPR Policy horizontalPolicy() const { return static_cast<Policy>(bits.horPolicy); }
    QT_SIZEPOLICY_CONSTEXPR Policy verticalPolicy() const { return static_cast<Policy>(bits.verPolicy); }
    ControlType controlType() const;

    void setHorizontalPolicy(Policy d) { bits.horPolicy = d; }
    void setVerticalPolicy(Policy d) { bits.verPolicy = d; }
    void setControlType(ControlType type);

    QT_SIZEPOLICY_CONSTEXPR Qt::Orientations expandingDirections() const {
        return ( (verticalPolicy()   & ExpandFlag) ? Qt::Vertical   : Qt::Orientations() )
             | ( (horizontalPolicy() & ExpandFlag) ? Qt::Horizontal : Qt::Orientations() ) ;
    }

    void setHeightForWidth(bool b) { bits.hfw = b; }
    QT_SIZEPOLICY_CONSTEXPR bool hasHeightForWidth() const { return bits.hfw; }
    void setWidthForHeight(bool b) { bits.wfh = b;  }
    QT_SIZEPOLICY_CONSTEXPR bool hasWidthForHeight() const { return bits.wfh; }

    QT_SIZEPOLICY_CONSTEXPR bool operator==(const QSizePolicy& s) const { return data == s.data; }
    QT_SIZEPOLICY_CONSTEXPR bool operator!=(const QSizePolicy& s) const { return data != s.data; }

    friend Q_DECL_CONST_FUNCTION uint qHash(QSizePolicy key, uint seed) Q_DECL_NOTHROW { return qHash(key.data, seed); }

    operator QVariant() const;

    QT_SIZEPOLICY_CONSTEXPR int horizontalStretch() const { return static_cast<int>(bits.horStretch); }
    QT_SIZEPOLICY_CONSTEXPR int verticalStretch() const { return static_cast<int>(bits.verStretch); }
    void setHorizontalStretch(int stretchFactor) { bits.horStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }
    void setVerticalStretch(int stretchFactor) { bits.verStretch = static_cast<quint32>(qBound(0, stretchFactor, 255)); }

    QT_SIZEPOLICY_CONSTEXPR bool retainSizeWhenHidden() const { return bits.retainSizeWhenHidden; }
    void setRetainSizeWhenHidden(bool retainSize) { bits.retainSizeWhenHidden = retainSize; }

    void transpose() { *this = transposed(); }
#ifndef Q_QDOC
    QT_SIZEPOLICY_CONSTEXPR_AND_UNIFORM_INIT
#endif
    QSizePolicy transposed() const Q_DECL_NOTHROW Q_REQUIRED_RESULT
    {
        return QSizePolicy(bits.transposed());
    }

private:
#ifndef QT_NO_DATASTREAM
    friend Q_WIDGETS_EXPORT QDataStream &operator<<(QDataStream &, const QSizePolicy &);
    friend Q_WIDGETS_EXPORT QDataStream &operator>>(QDataStream &, QSizePolicy &);
#endif
    QT_SIZEPOLICY_CONSTEXPR QSizePolicy(int i) : data(i) { }
    struct Bits;
    QT_SIZEPOLICY_CONSTEXPR explicit QSizePolicy(Bits b) Q_DECL_NOTHROW : bits(b) { }

    struct Bits {
        quint32 horStretch : 8;
        quint32 verStretch : 8;
        quint32 horPolicy : 4;
        quint32 verPolicy : 4;
        quint32 ctype : 5;
        quint32 hfw : 1;
        quint32 wfh : 1;
        quint32 retainSizeWhenHidden : 1;

        QT_SIZEPOLICY_CONSTEXPR_AND_UNIFORM_INIT
        Bits transposed() const Q_DECL_NOTHROW
        {
            QT_SIZEPOLICY_RETURN_BITS(verStretch, // \ swap
                                      horStretch, // /
                                      verPolicy, // \ swap
                                      horPolicy, // /
                                      ctype,
                                      hfw, // \ don't swap (historic behavior)
                                      wfh, // /
                                      retainSizeWhenHidden);
        }
    };
    union {
        Bits bits;
        quint32 data;
    };
};
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
// Can't add in Qt 5, as QList<QSizePolicy> would be BiC:
Q_DECLARE_TYPEINFO(QSizePolicy, Q_PRIMITIVE_TYPE);
#endif

Q_DECLARE_OPERATORS_FOR_FLAGS(QSizePolicy::ControlTypes)

#ifndef QT_NO_DATASTREAM
Q_WIDGETS_EXPORT QDataStream &operator<<(QDataStream &, const QSizePolicy &);
Q_WIDGETS_EXPORT QDataStream &operator>>(QDataStream &, QSizePolicy &);
#endif

#ifndef QT_NO_DEBUG_STREAM
Q_WIDGETS_EXPORT QDebug operator<<(QDebug dbg, const QSizePolicy &);
#endif


#undef QT_SIZEPOLICY_CONSTEXPR
#undef QT_SIZEPOLICY_CONSTEXPR_AND_UNIFORM_INIT
#undef QT_SIZEPOLICY_RETURN_BITS

QT_END_NAMESPACE

#endif // QSIZEPOLICY_H
