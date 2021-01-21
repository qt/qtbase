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

#ifndef QTEXTDOCUMENTFRAGMENT_H
#define QTEXTDOCUMENTFRAGMENT_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE


class QTextStream;
class QTextDocument;
class QTextDocumentFragmentPrivate;
class QTextCursor;

class Q_GUI_EXPORT QTextDocumentFragment
{
public:
    QTextDocumentFragment();
    explicit QTextDocumentFragment(const QTextDocument *document);
    explicit QTextDocumentFragment(const QTextCursor &range);
    QTextDocumentFragment(const QTextDocumentFragment &rhs);
    QTextDocumentFragment &operator=(const QTextDocumentFragment &rhs);
    ~QTextDocumentFragment();

    bool isEmpty() const;

    QString toPlainText() const;
#ifndef QT_NO_TEXTHTMLPARSER
    QString toHtml(const QByteArray &encoding = QByteArray()) const;
#endif // QT_NO_TEXTHTMLPARSER

    static QTextDocumentFragment fromPlainText(const QString &plainText);
#ifndef QT_NO_TEXTHTMLPARSER
    static QTextDocumentFragment fromHtml(const QString &html);
    static QTextDocumentFragment fromHtml(const QString &html, const QTextDocument *resourceProvider);
#endif // QT_NO_TEXTHTMLPARSER

private:
    QTextDocumentFragmentPrivate *d;
    friend class QTextCursor;
    friend class QTextDocumentWriter;
};

QT_END_NAMESPACE

#endif // QTEXTDOCUMENTFRAGMENT_H
