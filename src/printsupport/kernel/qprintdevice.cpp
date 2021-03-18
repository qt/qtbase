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

#include "qprintdevice_p.h"
#include "qplatformprintdevice.h"

#include <private/qdebug_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_PRINTER

QPrintDevice::QPrintDevice()
    : d(new QPlatformPrintDevice())
{
}

QPrintDevice::QPrintDevice(const QString &id)
    : d(new QPlatformPrintDevice(id))
{
}

QPrintDevice::QPrintDevice(QPlatformPrintDevice *dd)
    : d(dd)
{
}

QPrintDevice::QPrintDevice(const QPrintDevice &other)
    : d(other.d)
{
}

QPrintDevice::~QPrintDevice()
{
}

QPrintDevice &QPrintDevice::operator=(const QPrintDevice &other)
{
    d = other.d;
    return *this;
}

bool QPrintDevice::operator==(const QPrintDevice &other) const
{
    if (d && other.d)
        return d->id() == other.d->id();
    return d == other.d;
}

QString QPrintDevice::id() const
{
    return isValid() ? d->id() : QString();
}

QString QPrintDevice::name() const
{
    return isValid() ? d->name() : QString();
}

QString QPrintDevice::location() const
{
    return isValid() ? d->location() : QString();
}

QString QPrintDevice::makeAndModel() const
{
    return isValid() ? d->makeAndModel() : QString();
}

bool QPrintDevice::isValid() const
{
    return d && d->isValid();
}

bool QPrintDevice::isDefault() const
{
    return isValid() && d->isDefault();
}

bool QPrintDevice::isRemote() const
{
    return isValid() && d->isRemote();
}

QPrint::DeviceState QPrintDevice::state() const
{
    return isValid() ? d->state() : QPrint::Error;
}

bool QPrintDevice::isValidPageLayout(const QPageLayout &layout, int resolution) const
{
    return isValid() && d->isValidPageLayout(layout, resolution);
}

bool QPrintDevice::supportsMultipleCopies() const
{
    return isValid() && d->supportsMultipleCopies();
}

bool QPrintDevice::supportsCollateCopies() const
{
    return isValid() && d->supportsCollateCopies();
}

QPageSize QPrintDevice::defaultPageSize() const
{
    return isValid() ? d->defaultPageSize() : QPageSize();
}

QList<QPageSize> QPrintDevice::supportedPageSizes() const
{
    return isValid() ? d->supportedPageSizes() : QList<QPageSize>();
}

QPageSize QPrintDevice::supportedPageSize(const QPageSize &pageSize) const
{
    return isValid() ? d->supportedPageSize(pageSize) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(QPageSize::PageSizeId pageSizeId) const
{
    return isValid() ? d->supportedPageSize(pageSizeId) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(const QString &pageName) const
{
    return isValid() ? d->supportedPageSize(pageName) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(const QSize &pointSize) const
{
    return isValid() ? d->supportedPageSize(pointSize) : QPageSize();
}

QPageSize QPrintDevice::supportedPageSize(const QSizeF &size, QPageSize::Unit units) const
{
    return isValid() ? d->supportedPageSize(size, units) : QPageSize();
}

bool QPrintDevice::supportsCustomPageSizes() const
{
    return isValid() && d->supportsCustomPageSizes();
}

QSize QPrintDevice::minimumPhysicalPageSize() const
{
    return isValid() ? d->minimumPhysicalPageSize() : QSize();
}

QSize QPrintDevice::maximumPhysicalPageSize() const
{
    return isValid() ? d->maximumPhysicalPageSize() : QSize();
}

QMarginsF QPrintDevice::printableMargins(const QPageSize &pageSize,
                                         QPageLayout::Orientation orientation,
                                         int resolution) const
{
    return isValid() ? d->printableMargins(pageSize, orientation, resolution) : QMarginsF();
}

int QPrintDevice::defaultResolution() const
{
    return isValid() ? d->defaultResolution() : 0;
}

QList<int> QPrintDevice::supportedResolutions() const
{
    return isValid() ? d->supportedResolutions() : QList<int>();
}

QPrint::InputSlot QPrintDevice::defaultInputSlot() const
{
    return isValid() ? d->defaultInputSlot() : QPrint::InputSlot();
}

QVector<QPrint::InputSlot> QPrintDevice::supportedInputSlots() const
{
    return isValid() ? d->supportedInputSlots() : QVector<QPrint::InputSlot>{};
}

QPrint::OutputBin QPrintDevice::defaultOutputBin() const
{
    return isValid() ? d->defaultOutputBin() : QPrint::OutputBin();
}

QVector<QPrint::OutputBin> QPrintDevice::supportedOutputBins() const
{
    return isValid() ? d->supportedOutputBins() : QVector<QPrint::OutputBin>{};
}

QPrint::DuplexMode QPrintDevice::defaultDuplexMode() const
{
    return isValid() ? d->defaultDuplexMode() : QPrint::DuplexNone;
}

QVector<QPrint::DuplexMode> QPrintDevice::supportedDuplexModes() const
{
    return isValid() ? d->supportedDuplexModes() : QVector<QPrint::DuplexMode>{};
}

QPrint::ColorMode QPrintDevice::defaultColorMode() const
{
    return isValid() ? d->defaultColorMode() : QPrint::GrayScale;
}

QVector<QPrint::ColorMode> QPrintDevice::supportedColorModes() const
{
    return isValid() ? d->supportedColorModes() : QVector<QPrint::ColorMode>{};
}

QVariant QPrintDevice::property(PrintDevicePropertyKey key) const
{
    return isValid() ? d->property(key) : QVariant();
}

bool QPrintDevice::setProperty(PrintDevicePropertyKey key, const QVariant &value)
{
    return isValid() ? d->setProperty(key, value) : false;
}

bool QPrintDevice::isFeatureAvailable(PrintDevicePropertyKey key, const QVariant &params) const
{
    return isValid() ? d->isFeatureAvailable(key, params) : false;
}

#if QT_CONFIG(mimetype)
QList<QMimeType> QPrintDevice::supportedMimeTypes() const
{
    return isValid() ? d->supportedMimeTypes() : QList<QMimeType>();
}
#endif // mimetype

#  ifndef QT_NO_DEBUG_STREAM
void QPrintDevice::format(QDebug debug) const
{
    QDebugStateSaver saver(debug);
    debug.noquote();
    debug.nospace();
    if (isValid()) {
        const QString deviceId = id();
        const QString deviceName = name();
        debug << "id=\"" << deviceId << "\", state=" << state();
        if (!deviceName.isEmpty() && deviceName != deviceId)
            debug << ", name=\"" << deviceName << '"';
        if (!location().isEmpty())
            debug << ", location=\"" << location() << '"';
        debug << ", makeAndModel=\"" << makeAndModel() << '"';
        if (isDefault())
            debug << ", default";
        if (isRemote())
            debug << ", remote";
        debug << ", defaultPageSize=" << defaultPageSize();
        if (supportsCustomPageSizes())
            debug << ", supportsCustomPageSizes";
        debug << ", physicalPageSize=(";
        QtDebugUtils::formatQSize(debug, minimumPhysicalPageSize());
        debug << ")..(";
        QtDebugUtils::formatQSize(debug, maximumPhysicalPageSize());
        debug << "), defaultResolution=" << defaultResolution()
              << ", defaultDuplexMode=" << defaultDuplexMode()
              << ", defaultColorMode="<< defaultColorMode();
#    if QT_CONFIG(mimetype)
        const QList<QMimeType> mimeTypes = supportedMimeTypes();
        if (!mimeTypes.isEmpty()) {
            debug << ", supportedMimeTypes=(";
            for (const auto &mimeType : mimeTypes)
                debug << " \"" << mimeType.name() << '"';
            debug << ')';
        }
#    endif // mimetype
    } else {
        debug << "null";
    }
}

QDebug operator<<(QDebug debug, const QPrintDevice &p)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    debug << "QPrintDevice(";
    p.format(debug);
    debug << ')';
    return debug;
}
#  endif // QT_NO_DEBUG_STREAM
#endif // QT_NO_PRINTER

QT_END_NAMESPACE
