/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QFILESELECTOR_H
#define QFILESELECTOR_H

#include <QtCore/QObject>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE

class QFileSelectorPrivate;
class Q_CORE_EXPORT QFileSelector : public QObject
{
    Q_OBJECT
public:
    explicit QFileSelector(QObject *parent = nullptr);
    ~QFileSelector();

    QString select(const QString &filePath) const;
    QUrl select(const QUrl &filePath) const;

    QStringList extraSelectors() const;
    void setExtraSelectors(const QStringList &list);

    QStringList allSelectors() const;

private:
    Q_DECLARE_PRIVATE(QFileSelector)
};

QT_END_NAMESPACE

#endif
