// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPRINTDEVICE_H_
#define QOHOSPRINTDEVICE_H_

#include <qpa/qplatformprintdevice.h>

QT_BEGIN_NAMESPACE

class QOhosPrintDevice : public QPlatformPrintDevice
{
public:
    QOhosPrintDevice();
    explicit QOhosPrintDevice(const QString &id);
    virtual ~QOhosPrintDevice();

    bool isValid() const override;
    bool isDefault() const override;

    QPrint::DeviceState state() const override;

    QPageSize defaultPageSize() const override;
    QPrint::DuplexMode defaultDuplexMode() const override;
    QPrint::ColorMode defaultColorMode() const override;
    int defaultResolution() const override;

    static QStringList availablePrintDeviceIds();
    static QString defaultPrintDeviceId();

protected:
    void loadPageSizes() const override;
    void loadDuplexModes() const override;
    void loadColorModes() const override;
    void loadResolutions() const override;
};

QT_END_NAMESPACE

#endif // QOHOSPRINTDEVICE_H_
