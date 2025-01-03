// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    explicit QIosOptionalPlugin_NSPhotoLibrary(QObject * = nullptr) {};
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
