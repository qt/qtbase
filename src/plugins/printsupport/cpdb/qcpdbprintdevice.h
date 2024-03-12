// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCPDBPRINTDEVICE_H
#define QCPDBPRINTDEVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qcpdb_p.h>
#include <private/qprint_p.h>

#include <qpa/qplatformprintdevice.h>

#include <QSet>
#include <QtCore/qmargins.h>

QT_BEGIN_NAMESPACE

class QCpdbPrintDevice : public QPlatformPrintDevice
{
public:
    explicit QCpdbPrintDevice(cpdb_printer_obj_t * const printerObj);
    virtual ~QCpdbPrintDevice();

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
    cpdb_printer_obj_t *m_printerObj;
    // options exlcuded from the advanced tab in print properties dialog,
    // because they have been displayed elsewhere in the dialog
    const QSet<QByteArray> m_nonAdvancedOptions{
        CPDB_OPTION_PAGE_RANGES,
        CPDB_OPTION_PAGE_SET,
        CPDB_OPTION_COPIES,
        CPDB_OPTION_PAGE_DELIVERY,
        CPDB_OPTION_COLLATE,
        CPDB_OPTION_SIDES,
        CPDB_OPTION_COLOR_MODE,
        CPDB_OPTION_MEDIA,
        CPDB_OPTION_ORIENTATION,
        CPDB_OPTION_MARGIN_LEFT,
        CPDB_OPTION_MARGIN_RIGHT,
        CPDB_OPTION_MARGIN_TOP,
        CPDB_OPTION_MARGIN_BOTTOM,
        CPDB_OPTION_NUMBER_UP,
        CPDB_OPTION_NUMBER_UP_LAYOUT,
        CPDB_OPTION_JOB_HOLD_UNTIL,
        CPDB_OPTION_JOB_PRIORITY,
        CPDB_OPTION_BILLING_INFO,
        CPDB_OPTION_JOB_SHEETS,
        CPDB_OPTION_FIDELITY,
        "borderless"
    };
};

QT_END_NAMESPACE

#endif // QCPDBPRINTDEVICE_H
