/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qpagedpaintdevice_p.h"
#include <qpagedpaintdevice.h>

QT_BEGIN_NAMESPACE


QPagedPaintDevicePrivate::~QPagedPaintDevicePrivate() = default;

/*!
    \class QPagedPaintDevice
    \inmodule QtGui

    \brief The QPagedPaintDevice class represents a paint device that supports
    multiple pages.

    \ingroup painting

    Paged paint devices are used to generate output for printing or for formats like PDF.
    QPdfWriter and QPrinter inherit from it.
  */


/*!
    \internal
    Constructs a new paged paint device with the derived private class.
*/
QPagedPaintDevice::QPagedPaintDevice(QPagedPaintDevicePrivate *dd)
    : d(dd)
{
}

/*!
  Destroys the object.
  */
QPagedPaintDevice::~QPagedPaintDevice()
{
    delete d;
}

/*!
    \internal
    Returns the QPagedPaintDevicePrivate.
*/
QPagedPaintDevicePrivate *QPagedPaintDevice::dd()
{
    return d;
}

/*!
  \fn bool QPagedPaintDevice::newPage()

  Starts a new page. Returns \c true on success.
*/

/*!
    \enum QPagedPaintDevice::PdfVersion

    The PdfVersion enum describes the version of the PDF file that
    is produced by QPrinter or QPdfWriter.

    \value PdfVersion_1_4 A PDF 1.4 compatible document is produced.

    \value PdfVersion_A1b A PDF/A-1b compatible document is produced.

    \value PdfVersion_1_6 A PDF 1.6 compatible document is produced.
           This value was added in Qt 5.12.
*/

/*!
    \since 5.3

    Sets the page layout to \a newPageLayout.

    You should call this before calling QPainter::begin(), or immediately
    before calling newPage() to apply the new page layout to a new page.
    You should not call any painting methods between a call to setPageLayout()
    and newPage() as the wrong paint metrics may be used.

    Returns true if the page layout was successfully set to \a newPageLayout.

    \sa pageLayout()
*/

bool QPagedPaintDevice::setPageLayout(const QPageLayout &newPageLayout)
{
    return d->setPageLayout(newPageLayout);
}

/*!
    \since 5.3

    Sets the page size to \a pageSize.

    To get the current QPageSize use pageLayout().pageSize().

    You should call this before calling QPainter::begin(), or immediately
    before calling newPage() to apply the new page size to a new page.
    You should not call any painting methods between a call to setPageSize()
    and newPage() as the wrong paint metrics may be used.

    Returns true if the page size was successfully set to \a pageSize.

    \sa pageLayout()
*/

bool QPagedPaintDevice::setPageSize(const QPageSize &pageSize)
{
    return d->setPageSize(pageSize);
}

/*!
    \since 5.3

    Sets the page \a orientation.

    The page orientation is used to define the orientation of the
    page size when obtaining the page rect.

    You should call this before calling QPainter::begin(), or immediately
    before calling newPage() to apply the new orientation to a new page.
    You should not call any painting methods between a call to setPageOrientation()
    and newPage() as the wrong paint metrics may be used.

    To get the current QPageLayout::Orientation use pageLayout().orientation().

    Returns true if the page orientation was successfully set to \a orientation.

    \sa pageLayout()
*/

bool QPagedPaintDevice::setPageOrientation(QPageLayout::Orientation orientation)
{
    return d->setPageOrientation(orientation);
}

/*!
    \since 5.3

    Set the page \a margins defined in the given \a units.

    You should call this before calling QPainter::begin(), or immediately
    before calling newPage() to apply the new margins to a new page.
    You should not call any painting methods between a call to setPageMargins()
    and newPage() as the wrong paint metrics may be used.

    To get the current page margins use pageLayout().margins().

    Returns true if the page margins were successfully set to \a margins.

    \sa pageLayout()
*/

bool QPagedPaintDevice::setPageMargins(const QMarginsF &margins, QPageLayout::Unit units)
{
    return d->setPageMargins(margins, units);
}

/*!
    \since 5.3

    Returns the current page layout.  Use this method to access the current
    QPageSize, QPageLayout::Orientation, QMarginsF, fullRect() and paintRect().

    Note that you cannot use the setters on the returned object, you must either
    call the individual QPagedPaintDevice setters or use setPageLayout().

    \sa setPageLayout(), setPageSize(), setPageOrientation(), setPageMargins()
*/

QPageLayout QPagedPaintDevice::pageLayout() const
{
    return d->pageLayout();
}

/*!
    \since 6.0

    Returns the page ranges associated with this device.

    \sa QPageRanges, QPrinter::fromPage(), QPrinter::toPage()
*/
QPageRanges QPagedPaintDevice::pageRanges() const
{
    return d->pageRanges;
}

/*!
    \since 6.0

    Sets the page ranges for this device to \a ranges.
*/
void QPagedPaintDevice::setPageRanges(const QPageRanges &ranges)
{
    d->pageRanges = ranges;
}

QT_END_NAMESPACE
