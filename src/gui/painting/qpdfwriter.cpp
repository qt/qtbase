// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qpdfwriter.h>

#ifndef QT_NO_PDF

#include "qpagedpaintdevice_p.h"
#include "qpdf_p.h"
#include "qpdfoutputintent.h"

#include <QtCore/qfile.h>
#include <QtCore/private/qobject_p.h>

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
  \since 6.8
  Returns the ID of the document. By default, the ID is a
  randomly generated UUID.
  */
QUuid QPdfWriter::documentId() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->documentId;
}

/*!
  \since 6.8
  Sets the ID of the document to \a documentId.
  */
void QPdfWriter::setDocumentId(QUuid documentId)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->documentId = documentId;
}

/*!
  \since 6.9
  Returns the author of the document.
  */
QString QPdfWriter::author() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->author;
}

/*!
  \since 6.9
  Sets the author of the document to \a author.
  */
void QPdfWriter::setAuthor(const QString &author)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->author = author;
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

/*!
  \enum QPdfWriter::ColorModel
  \since 6.8

  This enumeration describes the way in which the PDF engine interprets
  stroking and filling colors, set as a QPainter's pen or brush (via
  QPen and QBrush).

  \value RGB All colors are converted to RGB and saved as such in the
  PDF.

  \value Grayscale All colors are converted to grayscale. For backwards
  compatibility, they are emitted in the PDF output as RGB colors, with
  identical quantities of red, green and blue.

  \value CMYK All colors are converted to CMYK and saved as such.

  \value Auto RGB colors are emitted as RGB; CMYK colors are emitted as
  CMYK. Colors of any other color spec are converted to RGB.
  This is the default since Qt 6.8.

  \sa QColor, QGradient
*/

/*!
  \since 6.8

  Returns the color model used by this PDF writer.
  The default is QPdfWriter::ColorModel::Auto.
*/
QPdfWriter::ColorModel QPdfWriter::colorModel() const
{
    Q_D(const QPdfWriter);
    return static_cast<ColorModel>(d->engine->d_func()->colorModel);
}

/*!
  \since 6.8

  Sets the color model used by this PDF writer to \a model.
*/
void QPdfWriter::setColorModel(ColorModel model)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->colorModel = static_cast<QPdfEngine::ColorModel>(model);
}

/*!
  \since 6.8

  Returns the output intent used by this PDF writer.
*/
QPdfOutputIntent QPdfWriter::outputIntent() const
{
    Q_D(const QPdfWriter);
    return d->engine->d_func()->outputIntent;
}

/*!
  \since 6.8

  Sets the output intent used by this PDF writer to \a intent.
*/
void QPdfWriter::setOutputIntent(const QPdfOutputIntent &intent)
{
    Q_D(QPdfWriter);
    d->engine->d_func()->outputIntent = intent;
}

QT_END_NAMESPACE

#include "moc_qpdfwriter.cpp"

#endif // QT_NO_PDF
