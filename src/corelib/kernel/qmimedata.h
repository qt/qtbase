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

#ifndef QMIMEDATA_H
#define QMIMEDATA_H

#include <QtCore/qvariant.h>
#include <QtCore/qobject.h>

QT_BEGIN_NAMESPACE


class QUrl;
class QMimeDataPrivate;

class Q_CORE_EXPORT QMimeData : public QObject
{
    Q_OBJECT
public:
    QMimeData();
    ~QMimeData();

    QList<QUrl> urls() const;
    void setUrls(const QList<QUrl> &urls);
    bool hasUrls() const;

    QString text() const;
    void setText(const QString &text);
    bool hasText() const;

    QString html() const;
    void setHtml(const QString &html);
    bool hasHtml() const;

    QVariant imageData() const;
    void setImageData(const QVariant &image);
    bool hasImage() const;

    QVariant colorData() const;
    void setColorData(const QVariant &color);
    bool hasColor() const;

    QByteArray data(const QString &mimetype) const;
    void setData(const QString &mimetype, const QByteArray &data);
    void removeFormat(const QString &mimetype);

    virtual bool hasFormat(const QString &mimetype) const;
    virtual QStringList formats() const;

    void clear();
protected:
    virtual QVariant retrieveData(const QString &mimetype,
                                      QVariant::Type preferredType) const;
private:
    Q_DISABLE_COPY(QMimeData)
    Q_DECLARE_PRIVATE(QMimeData)
};

QT_END_NAMESPACE

#endif // QMIMEDATA_H
