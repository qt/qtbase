/****************************************************************************
**
** Copyright (C) 2014 John Layt <jlayt@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qplatformprintdevice.h"

#include "qprintdevice_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtGui/qpagelayout.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

QPlatformPrintDevice::QPlatformPrintDevice(const QString &id)
    : m_id(id),
      m_isRemote(false),
      m_supportsMultipleCopies(false),
      m_supportsCollateCopies(false),
      m_havePageSizes(false),
      m_supportsCustomPageSizes(false),
      m_haveResolutions(false),
      m_haveInputSlots(false),
      m_haveOutputBins(false),
      m_haveDuplexModes(false),
      m_haveColorModes(false)
#if QT_CONFIG(mimetype)
    , m_haveMimeTypes(false)
#endif
{
}

QPlatformPrintDevice::~QPlatformPrintDevice()
{
}

QString QPlatformPrintDevice::id() const
{
    return m_id;
}

QString QPlatformPrintDevice::name() const
{
    return m_name;
}

QString QPlatformPrintDevice::location() const
{
    return m_location;
}

QString QPlatformPrintDevice::makeAndModel() const
{
    return m_makeAndModel;
}

bool QPlatformPrintDevice::isValid() const
{
    return false;
}

bool QPlatformPrintDevice::isDefault() const
{
    return false;
}

bool QPlatformPrintDevice::isRemote() const
{
    return m_isRemote;
}

bool QPlatformPrintDevice::isValidPageLayout(const QPageLayout &layout, int resolution) const
{
    // Check the page size is supported
    if (!supportedPageSize(layout.pageSize()).isValid())
        return false;

    // Check the margins are valid
    QMarginsF pointMargins = layout.margins(QPageLayout::Point);
    QMarginsF printMargins = printableMargins(layout.pageSize(), layout.orientation(), resolution);
    return pointMargins.left() >= printMargins.left()
           && pointMargins.right() >= printMargins.right()
           && pointMargins.top() >= printMargins.top()
           && pointMargins.bottom() >= printMargins.bottom();
}

QPrint::DeviceState QPlatformPrintDevice::state() const
{
    return QPrint::Error;
}

bool QPlatformPrintDevice::supportsMultipleCopies() const
{
    return m_supportsMultipleCopies;
}

bool QPlatformPrintDevice::supportsCollateCopies() const
{
    return m_supportsCollateCopies;
}

void QPlatformPrintDevice::loadPageSizes() const
{
}

QPageSize QPlatformPrintDevice::defaultPageSize() const
{
    return QPageSize();
}

QList<QPageSize> QPlatformPrintDevice::supportedPageSizes() const
{
    if (!m_havePageSizes)
        loadPageSizes();
    return m_pageSizes;
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QPageSize &pageSize) const
{
    if (!pageSize.isValid())
        return QPageSize();

    if (!m_havePageSizes)
        loadPageSizes();

    // First try match on name and id for case where printer defines same size twice with different names
    // e.g. Windows defines DMPAPER_11X17 and DMPAPER_TABLOID with names "11x17" and "Tabloid", but both
    // map to QPageSize::Tabloid / PPD Key "Tabloid" / ANSI B Tabloid
    if (pageSize.id() != QPageSize::Custom) {
        for (const QPageSize &ps : qAsConst(m_pageSizes)) {
            if (ps.id() == pageSize.id() && ps.name() == pageSize.name())
                return ps;
        }
    }

    // Next try match on id only if not custom
    if (pageSize.id() != QPageSize::Custom) {
        for (const QPageSize &ps : qAsConst(m_pageSizes)) {
            if (ps.id() == pageSize.id())
                return ps;
        }
    }

    // Next try a match on size, in case it's a custom with a different name
    return supportedPageSizeMatch(pageSize);
}

QPageSize QPlatformPrintDevice::supportedPageSize(QPageSize::PageSizeId pageSizeId) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    for (const QPageSize &ps : qAsConst(m_pageSizes)) {
        if (ps.id() == pageSizeId)
            return ps;
    }

    // If no supported page size found, try use a custom size instead if supported
    return supportedPageSizeMatch(QPageSize(pageSizeId));
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QString &pageName) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    for (const QPageSize &ps : qAsConst(m_pageSizes)) {
        if (ps.name() == pageName)
            return ps;
    }

    return QPageSize();
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QSize &sizePoints) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    // Try to find a supported page size based on fuzzy-matched point size
    return supportedPageSizeMatch(QPageSize(sizePoints));
}

QPageSize QPlatformPrintDevice::supportedPageSize(const QSizeF &size, QPageSize::Unit units) const
{
    if (!m_havePageSizes)
        loadPageSizes();

    // Try to find a supported page size based on fuzzy-matched unit size
    return supportedPageSizeMatch(QPageSize(size, units));
}

QPageSize QPlatformPrintDevice::supportedPageSizeMatch(const QPageSize &pageSize) const
{
    // If it's a known page size, just return itself
    if (m_pageSizes.contains(pageSize))
        return pageSize;

    // Try to find a supported page size based on point size
    for (const QPageSize &ps : qAsConst(m_pageSizes)) {
        if (ps.sizePoints() == pageSize.sizePoints())
            return ps;
    }
    return QPageSize();
}

bool QPlatformPrintDevice::supportsCustomPageSizes() const
{
    return m_supportsCustomPageSizes;
}

QSize QPlatformPrintDevice::minimumPhysicalPageSize() const
{
    return m_minimumPhysicalPageSize;
}

QSize QPlatformPrintDevice::maximumPhysicalPageSize() const
{
    return m_maximumPhysicalPageSize;
}

QMarginsF QPlatformPrintDevice::printableMargins(const QPageSize &pageSize,
                                                 QPageLayout::Orientation orientation,
                                                 int resolution) const
{
    Q_UNUSED(pageSize)
    Q_UNUSED(orientation)
    Q_UNUSED(resolution)
    return QMarginsF(0, 0, 0, 0);
}

void QPlatformPrintDevice::loadResolutions() const
{
}

int QPlatformPrintDevice::defaultResolution() const
{
    return 0;
}

QList<int> QPlatformPrintDevice::supportedResolutions() const
{
    if (!m_haveResolutions)
        loadResolutions();
    return m_resolutions;
}

void QPlatformPrintDevice::loadInputSlots() const
{
}

QPrint::InputSlot QPlatformPrintDevice::defaultInputSlot() const
{
    QPrint::InputSlot input;
    input.key = QByteArrayLiteral("Auto");
    input.name = QCoreApplication::translate("Print Device Input Slot", "Automatic");
    input.id = QPrint::Auto;
    return input;
}

QVector<QPrint::InputSlot> QPlatformPrintDevice::supportedInputSlots() const
{
    if (!m_haveInputSlots)
        loadInputSlots();
    return m_inputSlots;
}

void QPlatformPrintDevice::loadOutputBins() const
{
}

QPrint::OutputBin QPlatformPrintDevice::defaultOutputBin() const
{
    QPrint::OutputBin output;
    output.key = QByteArrayLiteral("Auto");
    output.name = QCoreApplication::translate("Print Device Output Bin", "Automatic");
    output.id = QPrint::AutoOutputBin;
    return output;
}

QVector<QPrint::OutputBin> QPlatformPrintDevice::supportedOutputBins() const
{
    if (!m_haveOutputBins)
        loadOutputBins();
    return m_outputBins;
}

void QPlatformPrintDevice::loadDuplexModes() const
{
}

QPrint::DuplexMode QPlatformPrintDevice::defaultDuplexMode() const
{
    return QPrint::DuplexNone;
}

QVector<QPrint::DuplexMode> QPlatformPrintDevice::supportedDuplexModes() const
{
    if (!m_haveDuplexModes)
        loadDuplexModes();
    return m_duplexModes;
}

void QPlatformPrintDevice::loadColorModes() const
{
}

QPrint::ColorMode QPlatformPrintDevice::defaultColorMode() const
{
    return QPrint::GrayScale;
}

QVector<QPrint::ColorMode> QPlatformPrintDevice::supportedColorModes() const
{
    if (!m_haveColorModes)
        loadColorModes();
    return m_colorModes;
}

#if QT_CONFIG(mimetype)
void QPlatformPrintDevice::loadMimeTypes() const
{
}
#endif // mimetype

QVariant QPlatformPrintDevice::property(QPrintDevice::PrintDevicePropertyKey key) const
{
    Q_UNUSED(key)

    return QVariant();
}

bool QPlatformPrintDevice::setProperty(QPrintDevice::PrintDevicePropertyKey key, const QVariant &value)
{
    Q_UNUSED(key)
    Q_UNUSED(value)

    return false;
}

bool QPlatformPrintDevice::isFeatureAvailable(QPrintDevice::PrintDevicePropertyKey key, const QVariant &params) const
{
    Q_UNUSED(key)
    Q_UNUSED(params)

    return false;
}

#if QT_CONFIG(mimetype)
QList<QMimeType> QPlatformPrintDevice::supportedMimeTypes() const
{
    if (!m_haveMimeTypes)
        loadMimeTypes();
    return m_mimeTypes;
}
#endif // mimetype

QPageSize QPlatformPrintDevice::createPageSize(const QString &key, const QSize &size, const QString &localizedName)
{
    return QPageSize(key, size, localizedName);
}

QPageSize QPlatformPrintDevice::createPageSize(int windowsId, const QSize &size, const QString &localizedName)
{
    return QPageSize(windowsId, size, localizedName);
}

#endif // QT_NO_PRINTER

QT_END_NAMESPACE
