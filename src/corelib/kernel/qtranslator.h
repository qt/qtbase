/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QTRANSLATOR_H
#define QTRANSLATOR_H

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_TRANSLATION

class QLocale;
class QTranslatorPrivate;

class Q_CORE_EXPORT QTranslator : public QObject
{
    Q_OBJECT
public:
    explicit QTranslator(QObject *parent = nullptr);
    ~QTranslator();

    virtual QString translate(const char *context, const char *sourceText,
                              const char *disambiguation = nullptr, int n = -1) const;

    virtual bool isEmpty() const;

    QString language() const;
    QString filePath() const;

    bool load(const QString & filename,
              const QString & directory = QString(),
              const QString & search_delimiters = QString(),
              const QString & suffix = QString());
    bool load(const QLocale & locale,
              const QString & filename,
              const QString & prefix = QString(),
              const QString & directory = QString(),
              const QString & suffix = QString());
    bool load(const uchar *data, int len, const QString &directory = QString());

private:
    Q_DISABLE_COPY(QTranslator)
    Q_DECLARE_PRIVATE(QTranslator)
};

#endif // QT_NO_TRANSLATION

QT_END_NAMESPACE

#endif // QTRANSLATOR_H
