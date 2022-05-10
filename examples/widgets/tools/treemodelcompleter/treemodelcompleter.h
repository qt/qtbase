// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TREEMODELCOMPLETER_H
#define TREEMODELCOMPLETER_H

#include <QCompleter>

//! [0]
class TreeModelCompleter : public QCompleter
{
    Q_OBJECT
    Q_PROPERTY(QString separator READ separator WRITE setSeparator)

public:
    explicit TreeModelCompleter(QObject *parent = nullptr);
    explicit TreeModelCompleter(QAbstractItemModel *model, QObject *parent = nullptr);

    QString separator() const;
public slots:
    void setSeparator(const QString &separator);

protected:
    QStringList splitPath(const QString &path) const override;
    QString pathFromIndex(const QModelIndex &index) const override;

private:
    QString sep;
};
//! [0]

#endif // TREEMODELCOMPLETER_H

