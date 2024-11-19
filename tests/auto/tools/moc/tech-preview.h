// Copyright (C) 2024 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TECH_PREVIEW_H
#define TECH_PREVIEW_H

#include <QObject>

#ifdef Q_MOC_RUN
#  undef QT_TECH_PREVIEW_API
#endif

class QT_TECH_PREVIEW_API MyTechPreviewObject : public QObject
{
    QT_TECH_PREVIEW_API
    Q_OBJECT

    QT_TECH_PREVIEW_API
    Q_PROPERTY(int status MEMBER m_status)

    int m_status = 0;

public:
    void myMethod() {}
    QT_TECH_PREVIEW_API void myTPMethod() {}

    Q_INVOKABLE QT_TECH_PREVIEW_API void myTPInvokable1() {}
    QT_TECH_PREVIEW_API Q_INVOKABLE void myTPInvokable2() {}

    enum class QT_TECH_PREVIEW_API MyTechPreviewEnum
    {
        A, B, C,
        TP QT_TECH_PREVIEW_API,
        X, Y, Z
    };

signals:
    void mySignal();
    QT_TECH_PREVIEW_API void myTPSignal();

public Q_SLOTS:
    void mySlot() {}
    QT_TECH_PREVIEW_API void myTPSlot() {}
};


#endif // TECH_PREVIEW_H
