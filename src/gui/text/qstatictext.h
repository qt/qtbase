/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QSTATICTEXT_H
#define QSTATICTEXT_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qsize.h>
#include <QtCore/qstring.h>
#include <QtCore/qmetatype.h>

#include <QtGui/qtransform.h>
#include <QtGui/qfont.h>
#include <QtGui/qtextoption.h>

QT_BEGIN_NAMESPACE


class QStaticTextPrivate;
class Q_GUI_EXPORT QStaticText
{
public:
    enum PerformanceHint {
        ModerateCaching,
        AggressiveCaching
    };

    QStaticText();
    explicit QStaticText(const QString &text);
    QStaticText(const QStaticText &other);
    QStaticText &operator=(QStaticText &&other) noexcept { swap(other); return *this; }
    QStaticText &operator=(const QStaticText &);
    ~QStaticText();

    void swap(QStaticText &other) noexcept { qSwap(data, other.data); }

    void setText(const QString &text);
    QString text() const;

    void setTextFormat(Qt::TextFormat textFormat);
    Qt::TextFormat textFormat() const;

    void setTextWidth(qreal textWidth);
    qreal textWidth() const;

    void setTextOption(const QTextOption &textOption);
    QTextOption textOption() const;

    QSizeF size() const;

    void prepare(const QTransform &matrix = QTransform(), const QFont &font = QFont());

    void setPerformanceHint(PerformanceHint performanceHint);
    PerformanceHint performanceHint() const;

    bool operator==(const QStaticText &) const;
    bool operator!=(const QStaticText &) const;

private:
    void detach();

    QExplicitlySharedDataPointer<QStaticTextPrivate> data;
    friend class QStaticTextPrivate;
};

Q_DECLARE_SHARED(QStaticText)

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QStaticText)

#endif // QSTATICTEXT_H
