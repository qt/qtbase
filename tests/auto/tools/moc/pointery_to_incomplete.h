// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef POINTERY_TO_INCOMPLETE_H
#define POINTERY_TO_INCOMPLETE_H

#include <QObject>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QPointer>

class FwdClass;

class TestPointeeCanBeIncomplete : public QObject
{
    Q_OBJECT
public:
    void setProp1(QPointer<FwdClass>) {}
    void setProp2(QSharedPointer<FwdClass>) {}
    void setProp3(const QWeakPointer<FwdClass> &) {}
};

#endif // POINTERY_TO_INCOMPLETE_H
