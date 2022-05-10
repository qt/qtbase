// Copyright (C) 2016 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMetaProperty>
#include <QDebug>
#include "window.h"

void exposeMethod(const QMetaMethod &)
{
}

void exposeProperty(const QMetaProperty &)
{
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
//! [Window class using revision]
    Window window;
    int expectedRevision = 0;
    const QMetaObject *windowMetaObject = window.metaObject();
    for (int i=0; i < windowMetaObject->methodCount(); i++)
        if (windowMetaObject->method(i).revision() <= expectedRevision)
            exposeMethod(windowMetaObject->method(i));
    for (int i=0; i < windowMetaObject->propertyCount(); i++)
        if (windowMetaObject->property(i).revision() <= expectedRevision)
            exposeProperty(windowMetaObject->property(i));
//! [Window class using revision]
    window.show();
    return app.exec();
}
