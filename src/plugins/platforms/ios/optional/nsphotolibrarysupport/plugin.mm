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

#include "../../qiosoptionalplugininterface.h"
#include "../../qiosfiledialog.h"

#include "qiosimagepickercontroller.h"
#include "qiosfileenginefactory.h"

QT_BEGIN_NAMESPACE

class QIosOptionalPlugin_NSPhotoLibrary : public QObject, QIosOptionalPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QIosOptionalPluginInterface_iid FILE "plugin.json")
    Q_INTERFACES(QIosOptionalPluginInterface)

public:
    explicit QIosOptionalPlugin_NSPhotoLibrary(QObject* = 0) {};
    ~QIosOptionalPlugin_NSPhotoLibrary() {}

    UIViewController* createImagePickerController(QIOSFileDialog *fileDialog) const override
    {
        return [[[QIOSImagePickerController alloc] initWithQIOSFileDialog:fileDialog] autorelease];
    }

private:
    QIOSFileEngineFactory m_fileEngineFactory;

};

QT_END_NAMESPACE

#include "plugin.moc"
