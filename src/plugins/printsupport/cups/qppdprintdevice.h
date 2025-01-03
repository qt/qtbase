// Copyright (C) 2014 John Layt <jlayt@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QPPDPRINTDEVICE_H
#define QPPDPRINTDEVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of internal files.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformprintdevice.h>

#include <QtCore/qbytearray.h>
#include <QtCore/qhash.h>
#include <QtCore/qmargins.h>

#include <cups/cups.h>
#include <cups/ppd.h>

QT_BEGIN_NAMESPACE

class QPpdPrintDevice : public QPlatformPrintDevice
{
public:
    explicit QPpdPrintDevice(const QString &id);
    virtual ~QPpdPrintDevice();

    bool isValid() const override;
    bool isDefault() const override;

    QPrint::DeviceState state() const override;

    QPageSize defaultPageSize() const override;

    QMarginsF printableMargins(const QPageSize &pageSize, QPageLayout::Orientation orientation,
                               int resolution) const override;

    int defaultResolution() const override;

    QPrint::InputSlot defaultInputSlot() const override;

    QPrint::OutputBin defaultOutputBin() const override;

    QPrint::DuplexMode defaultDuplexMode() const override;

    QPrint::ColorMode defaultColorMode() const override;

    QVariant property(QPrintDevice::PrintDevicePropertyKey key) const override;
    bool setProperty(QPrintDevice::PrintDevicePropertyKey key, const QVariant &value) override;
    bool isFeatureAvailable(QPrintDevice::PrintDevicePropertyKey key, const QVariant &params) const override;

protected:
    void loadPageSizes() const override;
    void loadResolutions() const override;
    void loadInputSlots() const override;
    void loadOutputBins() const override;
    void loadDuplexModes() const override;
    void loadColorModes() const override;
#if QT_CONFIG(mimetype)
    void loadMimeTypes() const override;
#endif

private:
    QString printerOption(const QString &key) const;
    cups_ptype_e printerTypeFlags() const;

    cups_dest_t *m_cupsDest;
    ppd_file_t *m_ppd;
    QByteArray m_cupsName;
    QByteArray m_cupsInstance;
    QMarginsF m_customMargins;
    mutable QHash<QString, QMarginsF> m_printableMargins;
};

QT_END_NAMESPACE

#endif // QPPDPRINTDEVICE_H
