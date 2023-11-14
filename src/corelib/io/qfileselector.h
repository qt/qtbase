// Copyright (C) 2013 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

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
