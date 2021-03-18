/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

// ### Qt 6: remove when the deprecated constructor is removed
class QDummyPagedPaintDevicePrivate : public QPagedPaintDevicePrivate
{
    bool setPageLayout(const QPageLayout &newPageLayout) override
    {
        m_pageLayout = newPageLayout;
        return m_pageLayout.isEquivalentTo(newPageLayout);
    }

    bool setPageSize(const QPageSize &pageSize) override
    {
        m_pageLayout.setPageSize(pageSize);
        return m_pageLayout.pageSize().isEquivalentTo(pageSize);
    }

    bool setPageOrientation(QPageLayout::Orientation orientation) override
    {
        m_pageLayout.setOrientation(orientation);
        return m_pageLayout.orientation() == orientation;
    }

    bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units) override
    {
        m_pageLayout.setUnits(units);
        m_pageLayout.setMargins(margins);
        return m_pageLayout.margins() == margins && m_pageLayout.units() == units;
    }

    QPageLayout pageLayout() const override
    {
        return m_pageLayout;
    }

    QPageLayout m_pageLayout;
};

QPagedPaintDevicePrivate::~QPagedPaintDevicePrivate()
{
}

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
  Constructs a new paged paint device.

  \deprecated
  */
QPagedPaintDevice::QPagedPaintDevice()
    : d(new QDummyPagedPaintDevicePrivate)
{
}

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
    \enum QPagedPaintDevice::PageSize

    This enum type lists the available page sizes as defined in the Postscript
    PPD standard.  These values are duplicated in QPageSize and QPrinter and
    those types and enum will be merged in Qt 6.

    The defined sizes are:

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
    \value A10
    \value A3Extra
    \value A4Extra
    \value A4Plus
    \value A4Small
    \value A5Extra
    \value B5Extra
    \value JisB0
    \value JisB1
    \value JisB2
    \value JisB3
    \value JisB4
    \value JisB5
    \value JisB6,
    \value JisB7
    \value JisB8
    \value JisB9
    \value JisB10
    \value AnsiA = Letter
    \value AnsiB = Ledger
    \value AnsiC
    \value AnsiD
    \value AnsiE
    \value LegalExtra
    \value LetterExtra
    \value LetterPlus
    \value LetterSmall
    \value TabloidExtra
    \value ArchA
    \value ArchB
    \value ArchC
    \value ArchD
    \value ArchE
    \value Imperial7x9
    \value Imperial8x10
    \value Imperial9x11
    \value Imperial9x12
    \value Imperial10x11
    \value Imperial10x13
    \value Imperial10x14
    \value Imperial12x11
    \value Imperial15x11
    \value ExecutiveStandard
    \value Note
    \value Quarto
    \value Statement
    \value SuperA
    \value SuperB
    \value Postcard
    \value DoublePostcard
    \value Prc16K
    \value Prc32K
    \value Prc32KBig
    \value FanFoldUS
    \value FanFoldGerman
    \value FanFoldGermanLegal
    \value EnvelopeB4
    \value EnvelopeB5
    \value EnvelopeB6
    \value EnvelopeC0
    \value EnvelopeC1
    \value EnvelopeC2
    \value EnvelopeC3
    \value EnvelopeC4
    \value EnvelopeC5 = C5E
    \value EnvelopeC6
    \value EnvelopeC65
    \value EnvelopeC7
    \value EnvelopeDL = DLE
    \value Envelope9
    \value Envelope10 = Comm10E
    \value Envelope11
    \value Envelope12
    \value Envelope14
    \value EnvelopeMonarch
    \value EnvelopePersonal
    \value EnvelopeChou3
    \value EnvelopeChou4
    \value EnvelopeInvite
    \value EnvelopeItalian
    \value EnvelopeKaku2
    \value EnvelopeKaku3
    \value EnvelopePrc1
    \value EnvelopePrc2
    \value EnvelopePrc3
    \value EnvelopePrc4
    \value EnvelopePrc5
    \value EnvelopePrc6
    \value EnvelopePrc7
    \value EnvelopePrc8
    \value EnvelopePrc9
    \value EnvelopePrc10
    \value EnvelopeYou4
    \value LastPageSize = EnvelopeYou4
    \omitvalue NPageSize
    \omitvalue NPaperSize

    Due to historic reasons QPageSize::Executive is not the same as the standard
    Postscript and Windows Executive size, use QPageSize::ExecutiveStandard instead.

    The Postscript standard size QPageSize::Folio is different to the Windows
    DMPAPER_FOLIO size, use the Postscript standard size QPageSize::FanFoldGermanLegal
    if needed.
*/

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
  Sets the size of the a page to \a size.

  \sa setPageSizeMM()
  */
void QPagedPaintDevice::setPageSize(PageSize size)
{
    d->setPageSize(QPageSize(QPageSize::PageSizeId(size)));
}

/*!
  Returns the currently used page size.
  */
QPagedPaintDevice::PageSize QPagedPaintDevice::pageSize() const
{
    return PageSize(d->pageLayout().pageSize().id());
}

/*!
    \obsolete Use setPageSize(QPageSize) instead.
    Sets the page size to \a size. \a size is specified in millimeters.

    If the size matches a standard QPagedPaintDevice::PageSize then that page
    size will be used, otherwise QPagedPaintDevice::Custom will be set.
*/
void QPagedPaintDevice::setPageSizeMM(const QSizeF &size)
{
    d->setPageSize(QPageSize(size, QPageSize::Millimeter));
}

/*!
  \obsolete Use pageLayout().pageSize() instead.

  Returns the page size in millimeters.
  */
QSizeF QPagedPaintDevice::pageSizeMM() const
{
    return d->pageLayout().pageSize().size(QPageSize::Millimeter);
}

/*!
  \obsolete Use setPageMargins(QMarginsF, QPageLayout::Unit) instead.
  Sets the margins to be used to \a margins.

  Margins are specified in millimeters.

  The margins are purely a hint to the drawing method. They don't affect the
  coordinate system or clipping.

  \sa margins()
  */
void QPagedPaintDevice::setMargins(const Margins &margins)
{
    d->setPageMargins(QMarginsF(margins.left, margins.top, margins.right, margins.bottom), QPageLayout::Millimeter);
}

/*!
  \obsolete Use pageLayout().margins() instead.
  Returns the current margins of the paint device. The default is 0.

  Margins are specified in millimeters.

  \sa setMargins()
  */
QPagedPaintDevice::Margins QPagedPaintDevice::margins() const
{
    QMarginsF margins = d->pageLayout().margins(QPageLayout::Millimeter);
    Margins result;
    result.left = margins.left();
    result.top = margins.top();
    result.right = margins.right();
    result.bottom = margins.bottom();
    return result;
}

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

    Set the page \a margins in the current page layout units.

    You should call this before calling QPainter::begin(), or immediately
    before calling newPage() to apply the new margins to a new page.
    You should not call any painting methods between a call to setPageMargins()
    and newPage() as the wrong paint metrics may be used.

    To get the current page margins use pageLayout().margins().

    Returns true if the page margins were successfully set to \a margins.

    \sa pageLayout()
*/

bool QPagedPaintDevice::setPageMargins(const QMarginsF &margins)
{
    return setPageMargins(margins, pageLayout().units());
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
    \internal

    \deprecated

    Returns the internal device page layout.
*/

QPageLayout QPagedPaintDevice::devicePageLayout() const
{
    qWarning("QPagedPaintDevice::devicePageLayout() is deprecated, just use QPagedPaintDevice::pageLayout()");
    return d->pageLayout();
}

/*!
    \internal

    \deprecated

    Returns the internal device page layout.
*/

QPageLayout &QPagedPaintDevice::devicePageLayout()
{
    qWarning("QPagedPaintDevice::devicePageLayout() is deprecated, you shouldn't be using this at all.");
    static QPageLayout dummy;
    return dummy;
}

QT_END_NAMESPACE
