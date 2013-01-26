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

#ifndef QMARGINS_H
#define QMARGINS_H

#include <QtCore/qrect.h>

QT_BEGIN_NAMESPACE


class QMargins
{
public:
    Q_DECL_CONSTEXPR QMargins();
    Q_DECL_CONSTEXPR QMargins(int left, int top, int right, int bottom);

    Q_DECL_CONSTEXPR bool isNull() const;

    Q_DECL_CONSTEXPR int left() const;
    Q_DECL_CONSTEXPR int top() const;
    Q_DECL_CONSTEXPR int right() const;
    Q_DECL_CONSTEXPR int bottom() const;

    void setLeft(int left);
    void setTop(int top);
    void setRight(int right);
    void setBottom(int bottom);

    QMargins &operator+=(const QMargins &margins);
    QMargins &operator-=(const QMargins &margins);
    QMargins &operator+=(int);
    QMargins &operator-=(int);
    QMargins &operator*=(int);
    QMargins &operator/=(int);
    QMargins &operator*=(qreal);
    QMargins &operator/=(qreal);

private:
    int m_left;
    int m_top;
    int m_right;
    int m_bottom;

    friend Q_DECL_CONSTEXPR inline bool operator==(const QMargins &, const QMargins &);
    friend Q_DECL_CONSTEXPR inline bool operator!=(const QMargins &, const QMargins &);
};

Q_DECLARE_TYPEINFO(QMargins, Q_MOVABLE_TYPE);

/*****************************************************************************
  QMargins stream functions
 *****************************************************************************/
#ifndef QT_NO_DATASTREAM
Q_CORE_EXPORT QDataStream &operator<<(QDataStream &, const QMargins &);
Q_CORE_EXPORT QDataStream &operator>>(QDataStream &, QMargins &);
#endif

/*****************************************************************************
  QMargins inline functions
 *****************************************************************************/

Q_DECL_CONSTEXPR inline QMargins::QMargins() : m_left(0), m_top(0), m_right(0), m_bottom(0) {}

Q_DECL_CONSTEXPR inline QMargins::QMargins(int aleft, int atop, int aright, int abottom)
    : m_left(aleft), m_top(atop), m_right(aright), m_bottom(abottom) {}

Q_DECL_CONSTEXPR inline bool QMargins::isNull() const
{ return m_left==0 && m_top==0 && m_right==0 && m_bottom==0; }

Q_DECL_CONSTEXPR inline int QMargins::left() const
{ return m_left; }

Q_DECL_CONSTEXPR inline int QMargins::top() const
{ return m_top; }

Q_DECL_CONSTEXPR inline int QMargins::right() const
{ return m_right; }

Q_DECL_CONSTEXPR inline int QMargins::bottom() const
{ return m_bottom; }


inline void QMargins::setLeft(int aleft)
{ m_left = aleft; }

inline void QMargins::setTop(int atop)
{ m_top = atop; }

inline void QMargins::setRight(int aright)
{ m_right = aright; }

inline void QMargins::setBottom(int abottom)
{ m_bottom = abottom; }

Q_DECL_CONSTEXPR inline bool operator==(const QMargins &m1, const QMargins &m2)
{
    return
            m1.m_left == m2.m_left &&
            m1.m_top == m2.m_top &&
            m1.m_right == m2.m_right &&
            m1.m_bottom == m2.m_bottom;
}

Q_DECL_CONSTEXPR inline bool operator!=(const QMargins &m1, const QMargins &m2)
{
    return
            m1.m_left != m2.m_left ||
            m1.m_top != m2.m_top ||
            m1.m_right != m2.m_right ||
            m1.m_bottom != m2.m_bottom;
}

Q_DECL_CONSTEXPR inline QRect operator+(const QRect &rectangle, const QMargins &margins)
{
    return QRect(QPoint(rectangle.left() - margins.left(), rectangle.top() - margins.top()),
                 QPoint(rectangle.right() + margins.right(), rectangle.bottom() + margins.bottom()));
}

Q_DECL_CONSTEXPR inline QRect operator+(const QMargins &margins, const QRect &rectangle)
{
    return QRect(QPoint(rectangle.left() - margins.left(), rectangle.top() - margins.top()),
                 QPoint(rectangle.right() + margins.right(), rectangle.bottom() + margins.bottom()));
}

inline QRect QRect::marginsAdded(const QMargins &margins) const
{
    return *this + margins;
}

inline QRect QRect::marginsRemoved(const QMargins &margins) const
{
    return QRect(QPoint(x1 + margins.left(), y1 + margins.top()),
                 QPoint(x2 - margins.right(), y2 - margins.bottom()));
}

inline QRect &QRect::operator+=(const QMargins &margins)
{
    *this = marginsAdded(margins);
    return *this;
}

inline QRect &QRect::operator-=(const QMargins &margins)
{
    *this = marginsRemoved(margins);
    return *this;
}

Q_DECL_CONSTEXPR inline QMargins operator+(const QMargins &m1, const QMargins &m2)
{
    return QMargins(m1.left() + m2.left(), m1.top() + m2.top(),
                    m1.right() + m2.right(), m1.bottom() + m2.bottom());
}

Q_DECL_CONSTEXPR inline QMargins operator-(const QMargins &m1, const QMargins &m2)
{
    return QMargins(m1.left() - m2.left(), m1.top() - m2.top(),
                    m1.right() - m2.right(), m1.bottom() - m2.bottom());
}

Q_DECL_CONSTEXPR inline QMargins operator*(const QMargins &margins, int factor)
{
    return QMargins(margins.left() * factor, margins.top() * factor,
                    margins.right() * factor, margins.bottom() * factor);
}

Q_DECL_CONSTEXPR inline QMargins operator*(int factor, const QMargins &margins)
{
    return QMargins(margins.left() * factor, margins.top() * factor,
                    margins.right() * factor, margins.bottom() * factor);
}

Q_DECL_CONSTEXPR inline QMargins operator*(const QMargins &margins, qreal factor)
{
    return QMargins(qRound(margins.left() * factor), qRound(margins.top() * factor),
                    qRound(margins.right() * factor), qRound(margins.bottom() * factor));
}

Q_DECL_CONSTEXPR inline QMargins operator*(qreal factor, const QMargins &margins)
{
    return QMargins(qRound(margins.left() * factor), qRound(margins.top() * factor),
                    qRound(margins.right() * factor), qRound(margins.bottom() * factor));
}

Q_DECL_CONSTEXPR inline QMargins operator/(const QMargins &margins, int divisor)
{
    return QMargins(margins.left() / divisor, margins.top() / divisor,
                    margins.right() / divisor, margins.bottom() / divisor);
}

Q_DECL_CONSTEXPR inline QMargins operator/(const QMargins &margins, qreal divisor)
{
    return QMargins(qRound(margins.left() / divisor), qRound(margins.top() / divisor),
                    qRound(margins.right() / divisor), qRound(margins.bottom() / divisor));
}

inline QMargins &QMargins::operator+=(const QMargins &margins)
{
    return *this = *this + margins;
}

inline QMargins &QMargins::operator-=(const QMargins &margins)
{
    return *this = *this - margins;
}

inline QMargins &QMargins::operator*=(int factor)
{
    return *this = *this * factor;
}

inline QMargins &QMargins::operator/=(int divisor)
{
    return *this = *this / divisor;
}

inline QMargins &QMargins::operator*=(qreal factor)
{
    return *this = *this * factor;
}

inline QMargins &QMargins::operator/=(qreal divisor)
{
    return *this = *this / divisor;
}

Q_DECL_CONSTEXPR inline QMargins operator-(const QMargins &margins)
{
    return QMargins(-margins.left(), -margins.top(), -margins.right(), -margins.bottom());
}

#ifndef QT_NO_DEBUG_STREAM
Q_CORE_EXPORT QDebug operator<<(QDebug, const QMargins &);
#endif

QT_END_NAMESPACE

#endif // QMARGINS_H
