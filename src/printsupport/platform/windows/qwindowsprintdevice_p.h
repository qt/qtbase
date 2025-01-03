// Copyright (C) 2014 John Layt <jlayt@kde.org>
// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSPRINTDEVICE_H
#define QWINDOWSPRINTDEVICE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists for the convenience
// of internal files. This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformprintdevice.h>

#include <QtPrintSupport/qtprintsupportglobal.h>
#include <QtCore/qt_windows.h>
#include <QtCore/private/qglobal_p.h>

QT_BEGIN_NAMESPACE

class Q_PRINTSUPPORT_EXPORT QWindowsPrinterInfo
{
public:
    bool operator==(const QWindowsPrinterInfo &other) const
    {
        // We only need to check if these are the same for matching up
        return m_id == other.m_id && m_name == other.m_name &&
               m_location == other.m_location &&
               m_makeAndModel == other.m_makeAndModel &&
               m_isRemote == other.m_isRemote;
    }
    QString m_id;
    QString m_name;
    QString m_location;
    QString m_makeAndModel;
    QList<QPageSize> m_pageSizes;
    QList<int> m_resolutions;
    QList<QPrint::InputSlot> m_inputSlots;
    QList<QPrint::OutputBin> m_outputBins;
    QList<QPrint::DuplexMode> m_duplexModes;
    QList<QPrint::ColorMode> m_colorModes;
    QSize m_minimumPhysicalPageSize;
    QSize m_maximumPhysicalPageSize;
    bool m_isRemote = false;
    bool m_havePageSizes = false;
    bool m_haveResolutions = false;
    bool m_haveCopies = false;
    bool m_supportsMultipleCopies = false;
    bool m_supportsCollateCopies = false;
    bool m_haveMinMaxPageSizes = false;
    bool m_supportsCustomPageSizes = false;
    bool m_haveInputSlots = false;
    bool m_haveOutputBins = false;
    bool m_haveDuplexModes = false;
    bool m_haveColorModes = false;
};

class Q_PRINTSUPPORT_EXPORT QWindowsPrintDevice : public QPlatformPrintDevice
{
public:
    QWindowsPrintDevice();
    explicit QWindowsPrintDevice(const QString &id);
    virtual ~QWindowsPrintDevice();

    bool isValid() const override;
    bool isDefault() const override;

    QPrint::DeviceState state() const override;

    QPageSize defaultPageSize() const override;

    QMarginsF printableMargins(const QPageSize &pageSize, QPageLayout::Orientation orientation,
                               int resolution) const override;

    int defaultResolution() const override;

    QPrint::InputSlot defaultInputSlot() const override;

    QPrint::DuplexMode defaultDuplexMode() const override;

    QPrint::ColorMode defaultColorMode() const override;

    static QStringList availablePrintDeviceIds();
    static QString defaultPrintDeviceId();

    bool supportsCollateCopies() const override;
    bool supportsMultipleCopies() const override;
    bool supportsCustomPageSizes() const override;
    QSize minimumPhysicalPageSize() const override;
    QSize maximumPhysicalPageSize() const override;

protected:
    void loadPageSizes() const override;
    void loadResolutions() const override;
    void loadInputSlots() const override;
    void loadOutputBins() const override;
    void loadDuplexModes() const override;
    void loadColorModes() const override;
    void loadCopiesSupport() const;
    void loadMinMaxPageSizes() const;

private:
    LPCWSTR wcharId() const { return reinterpret_cast<LPCWSTR>(m_id.utf16()); }

    HANDLE m_hPrinter;
    mutable bool m_haveCopies;
    mutable bool m_haveMinMaxPageSizes;
    int m_infoIndex;
};

QT_END_NAMESPACE

#endif // QWINDOWSPRINTDEVICE_H
