// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOPLUGININTERFACE_H
#define QIOPLUGININTERFACE_H

#include <QtCore/QtPlugin>

#include "qiosfiledialog.h"

Q_FORWARD_DECLARE_OBJC_CLASS(UIViewController);

QT_BEGIN_NAMESPACE

#define QIosOptionalPluginInterface_iid "org.qt-project.Qt.QPA.ios.optional"

class QIosOptionalPluginInterface
{
public:
    virtual ~QIosOptionalPluginInterface() {}
    virtual void initPlugin() const {}
    virtual UIViewController* createImagePickerController(QIOSFileDialog *) const { return nullptr; }
};

Q_DECLARE_INTERFACE(QIosOptionalPluginInterface, QIosOptionalPluginInterface_iid)

QT_END_NAMESPACE

#endif // QIOPLUGININTERFACE_H
