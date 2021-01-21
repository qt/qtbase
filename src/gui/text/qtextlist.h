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

#ifndef QTEXTLIST_H
#define QTEXTLIST_H

#include <QtGui/qtguiglobal.h>
#include <QtGui/qtextobject.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QTextListPrivate;
class QTextCursor;

class Q_GUI_EXPORT QTextList : public QTextBlockGroup
{
    Q_OBJECT
public:
    explicit QTextList(QTextDocument *doc);
    ~QTextList();

    int count() const;

#if QT_DEPRECATED_SINCE(5, 13)
    QT_DEPRECATED_X("Use count() instead")
    inline bool isEmpty() const
    { return count() == 0; }
#endif

    QTextBlock item(int i) const;

    int itemNumber(const QTextBlock &) const;
    QString itemText(const QTextBlock &) const;

    void removeItem(int i);
    void remove(const QTextBlock &);

    void add(const QTextBlock &block);

    inline void setFormat(const QTextListFormat &format);
    QTextListFormat format() const { return QTextObject::format().toListFormat(); }

private:
    Q_DISABLE_COPY(QTextList)
    Q_DECLARE_PRIVATE(QTextList)
};

inline void QTextList::setFormat(const QTextListFormat &aformat)
{ QTextObject::setFormat(aformat); }

QT_END_NAMESPACE

#endif // QTEXTLIST_H
