/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpagedpaintdevice_p.h"
#include <qpagedpaintdevice.h>

QT_BEGIN_NAMESPACE

static const struct {
    float width;
    float height;
} pageSizes[] = {
    {210, 297}, // A4
    {176, 250}, // B5
    {215.9f, 279.4f}, // Letter
    {215.9f, 355.6f}, // Legal
    {190.5f, 254}, // Executive
    {841, 1189}, // A0
    {594, 841}, // A1
    {420, 594}, // A2
    {297, 420}, // A3
    {148, 210}, // A5
    {105, 148}, // A6
    {74, 105}, // A7
    {52, 74}, // A8
    {37, 52}, // A8
    {1000, 1414}, // B0
    {707, 1000}, // B1
    {31, 44}, // B10
    {500, 707}, // B2
    {353, 500}, // B3
    {250, 353}, // B4
    {125, 176}, // B6
    {88, 125}, // B7
    {62, 88}, // B8
    {33, 62}, // B9
    {163, 229}, // C5E
    {105, 241}, // US Common
    {110, 220}, // DLE
    {210, 330}, // Folio
    {431.8f, 279.4f}, // Ledger
    {279.4f, 431.8f} // Tabloid
};

/*!
    \class QPagedPaintDevice
    \inmodule QtGui

    \brief The QPagedPaintDevice class is a represents a paintdevice that supports
    multiple pages.

    \ingroup painting

    Paged paint devices are used to generate output for printing or for formats like PDF.
    QPdfWriter and QPrinter inherit from it.
  */

/*!
  Constructs a new paged paint device.
  */
QPagedPaintDevice::QPagedPaintDevice()
    : d(new QPagedPaintDevicePrivate)
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
  \enum QPagedPaintDevice::PageSize

  This enum type specifies the page size of the paint device.

  \value A0 841 x 1189 mm
  \value A1 594 x 841 mm
  \value A2 420 x 594 mm
  \value A3 297 x 420 mm
  \value A4 210 x 297 mm, 8.26 x 11.69 inches
  \value A5 148 x 210 mm
  \value A6 105 x 148 mm
  \value A7 74 x 105 mm
  \value A8 52 x 74 mm
  \value A9 37 x 52 mm
  \value B0 1000 x 1414 mm
  \value B1 707 x 1000 mm
  \value B2 500 x 707 mm
  \value B3 353 x 500 mm
  \value B4 250 x 353 mm
  \value B5 176 x 250 mm, 6.93 x 9.84 inches
  \value B6 125 x 176 mm
  \value B7 88 x 125 mm
  \value B8 62 x 88 mm
  \value B9 33 x 62 mm
  \value B10 31 x 44 mm
  \value C5E 163 x 229 mm
  \value Comm10E 105 x 241 mm, U.S. Common 10 Envelope
  \value DLE 110 x 220 mm
  \value Executive 7.5 x 10 inches, 190.5 x 254 mm
  \value Folio 210 x 330 mm
  \value Ledger 431.8 x 279.4 mm
  \value Legal 8.5 x 14 inches, 215.9 x 355.6 mm
  \value Letter 8.5 x 11 inches, 215.9 x 279.4 mm
  \value Tabloid 279.4 x 431.8 mm
  \value Custom Unknown, or a user defined size.

  \omitvalue NPageSize

  The page size can also be specified in millimeters using setPageSizeMM(). In this case the
  page size enum is set to Custom.
*/

/*!
  \fn bool QPagedPaintDevice::newPage()

  Starts a new page. Returns \c true on success.
*/


/*!
  Sets the size of the a page to \a size.

  \sa setPageSizeMM
  */
void QPagedPaintDevice::setPageSize(PageSize size)
{
    if (size >= Custom)
        return;
    d->pageSize = size;
    d->pageSizeMM = QSizeF(pageSizes[size].width, pageSizes[size].height);
}

/*!
  Returns the currently used page size.
  */
QPagedPaintDevice::PageSize QPagedPaintDevice::pageSize() const
{
    return d->pageSize;
}

/*!
  Sets the page size to \a size. \a size is specified in millimeters.
  */
void QPagedPaintDevice::setPageSizeMM(const QSizeF &size)
{
    d->pageSize = Custom;
    d->pageSizeMM = size;
}

/*!
  Returns the page size in millimeters.
  */
QSizeF QPagedPaintDevice::pageSizeMM() const
{
    return d->pageSizeMM;
}

/*!
  Sets the margins to be used to \a margins.

  Margins are specified in millimeters.

  The margins are purely a hint to the drawing method. They don't affect the
  coordinate system or clipping.

  \sa margins
  */
void QPagedPaintDevice::setMargins(const Margins &margins)
{
    d->margins = margins;
}

/*!
  returns the current margins of the paint device. The default is 0.

  \sa setMargins
  */
QPagedPaintDevice::Margins QPagedPaintDevice::margins() const
{
    return d->margins;
}

QT_END_NAMESPACE
