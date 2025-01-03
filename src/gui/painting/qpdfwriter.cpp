// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpdfwriter.h>

#ifndef QT_NO_PDF

#include "qpagedpaintdevice_p.h"
#include <QtCore/private/qobject_p.h>
#include "private/qpdf_p.h"
#include <QtCore/qfile.h>

QT_BEGIN_NAMESPACE

class QPdfWriterPrivate : public QObjectPrivate
{
public:
    QPdfWriterPrivate()
        : QObjectPrivate()
    {
        engine = new QPdfEngine();
        output = nullptr;
        pdfVersion = QPdfWriter::PdfVersion_1_4;
    }
    ~QPdfWriterPrivate()
    {
        delete engine;
        delete output;
    }

    QPdfEngine *engine;
    QFile *output;
    QPdfWriter::PdfVersion pdfVersion;
};

class QPdfPagedPaintDevicePrivate : public QPagedPaintDevicePrivate
{
public:
    QPdfPagedPaintDevicePrivate(QPdfWriterPrivate *d)
        : QPagedPaintDevicePrivate(), pd(d)
    {}

    ~QPdfPagedPaintDevicePrivate()
    {}

    bool setPageLayout(const QPageLayout &newPageLayout) override
    {
        // Try to set the paint engine page layout
        pd->engine->setPageLayout(newPageLayout);
        return pageLayout().isEquivalentTo(newPageLayout);
    }

    bool setPageSize(const QPageSize &pageSize) override
    {
        // Try to set the paint engine page size
        pd->engine->setPageSize(pageSize);
        return pageLayout().pageSize().isEquivalentTo(pageSize);
    }

    bool setPageOrientation(QPageLayout::Orientation orientation) override
    {
        // Set the print engine value
        pd->engine->setPageOrientation(orientation);
        return pageLayout().orientation() == orientation;
    }

    bool setPageMargins(const QMarginsF &margins, QPageLayout::Unit units) override
    {
        // Try to set engine margins
        pd->engine->setPageMargins(margins, units);
        return pageLayout().margins() == margins && pageLayout().units() == units;
    }

    QPageLayout pageLayout() const override
    {
        return pd->engine->pageLayout();
    }

    QPdfWriterPrivate *pd;
};

/*! \class QPdfWriter
    \inmodule QtGui

    \brief The QPdfWriter class is a class to generate PDFs
    that can be used as a paint device.

    \ingroup painting

    QPdfWriter generates PDF out of a series of drawing commands using QPainter.
    The newPage() method can be used to create several pages.
  */

/*!
  Constructs a PDF writer that will write the pdf to \a filename.
  */
QPdfWriter::QPdfWriter(const QString &filename)
    : QObject(*new QPdfWriterPrivate),
      QPagedPaintDevice(new QPdfPagedPaintDevicePrivate(d_func()))
{
    Q_D(QPdfWriter);

    d->engine->setOutputFilename(filename);
}

/*!
  Constructs a PDF writer that will write the pdf to \a device.
  */
QPdfWriter::QPdfWriter(QIODevice *device)
    : QObject(*new QPdfWriterPrivate),
      QPagedPaintDevice(new QPdfPagedPaintDevicePrivate(d_func()))
{
    Q_D(QPdfWriter);

    d->engine->d_func()->outDevice = device;
}

/*!
  Destroys the pdf writer.
  */
QPdfWriter::~QPdfWriter()
{

}

/*!
    \since 5.10

    Sets the PDF version for this writer to \a version.

    If \a version is the same value as currently set then no change will be made.
*/
void QPdfWriter::setPdfVersion(PdfVersion version)
{
    Q_D(QPdfWriter);

    if (d->pdfVersion == version)
        return;

    d->pdfVersion = version;
    d->engine->setPdfVersion(static_cast<QPdfEngine::PdfVersion>(static_cast<int>(version)));
}

/*!
    \since 5.10

    Returns the PDF version for this writer. The default is \c PdfVersion_1_4.
*/
QPdfWriter::PdfVersion QPdfWriter::pdfVersion() const
{
    Q_D(const QPdfWriter);
    return d->pdfVersion;
}

/*!
  Returns the title of the document.
  */
QString QPdfWriter::title() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->title;
}

/*!
  Sets the title of the document being created to \a title.
  */
void QPdfWriter::setTitle(const QString &title)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->title = title;
}

/*!
  Returns the creator of the document.
  */
QString QPdfWriter::creator() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->creator;
}

/*!
  Sets the creator of the document to \a creator.
  */
void QPdfWriter::setCreator(const QString &creator)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->creator = creator;
}

/*!
  \reimp
  */
QPaintEngine *QPdfWriter::paintEngine() const
{
    Q_D(const QPdfWriter);

    return d->engine;
}

/*!
    \since 5.3

    Sets the PDF \a resolution in DPI.

    This setting affects the coordinate system as returned by, for
    example QPainter::viewport().

    \sa resolution()
*/

void QPdfWriter::setResolution(int resolution)
{
    Q_D(const QPdfWriter);
    if (resolution > 0)
        d->engine->setResolution(resolution);
}

/*!
    \since 5.3

    Returns the resolution of the PDF in DPI.

    \sa setResolution()
*/

int QPdfWriter::resolution() const
{
    Q_D(const QPdfWriter);
    return d->engine->resolution();
}

/*!
    \since 5.15

    Sets the document metadata. This metadata is not influenced by the setTitle / setCreator methods,
    so is up to the user to keep it consistent.
    \a xmpMetadata contains XML formatted metadata to embed into the PDF file.

    \sa documentXmpMetadata()
*/

void QPdfWriter::setDocumentXmpMetadata(const QByteArray &xmpMetadata)
{
    Q_D(const QPdfWriter);
    d->engine->setDocumentXmpMetadata(xmpMetadata);
}

/*!
    \since 5.15

    Gets the document metadata, as it was provided with a call to setDocumentXmpMetadata. It will not
    return the default metadata.

    \sa setDocumentXmpMetadata()
*/

QByteArray QPdfWriter::documentXmpMetadata() const
{
    Q_D(const QPdfWriter);
    return d->engine->documentXmpMetadata();
}

/*!
    \since 5.15

    Adds \a fileName attachment to the PDF with (optional) \a mimeType.
    \a data contains the raw file data to embed into the PDF file.
*/

void QPdfWriter::addFileAttachment(const QString &fileName, const QByteArray &data, const QString &mimeType)
{
    Q_D(QPdfWriter);
    d->engine->addFileAttachment(fileName, data, mimeType);
}

/*!
    \internal

    Returns the metric for the given \a id.
*/
int QPdfWriter::metric(PaintDeviceMetric id) const
{
    Q_D(const QPdfWriter);
    return d->engine->metric(id);
}

/*!
  \reimp
*/
bool QPdfWriter::newPage()
{
    Q_D(QPdfWriter);

    return d->engine->newPage();
}

QT_END_NAMESPACE

#include "moc_qpdfwriter.cpp"

#endif // QT_NO_PDF
