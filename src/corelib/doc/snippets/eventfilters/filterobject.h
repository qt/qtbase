// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FILTEROBJECT_H
#define FILTEROBJECT_H

#include <QObject>

class FilterObject : public QObject
{
    Q_OBJECT

public:
    FilterObject(QObject *parent = nullptr);
    bool eventFilter(QObject *object, QEvent *event) override;
    void setFilteredObject(QObject *object);

private:
    QObject *target;
};

#endif
