/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
    virtual void initPlugin() const {};
    virtual UIViewController* createImagePickerController(QIOSFileDialog *) const { return nullptr; };
};

Q_DECLARE_INTERFACE(QIosOptionalPluginInterface, QIosOptionalPluginInterface_iid)

QT_END_NAMESPACE

#endif // QIOPLUGININTERFACE_H
