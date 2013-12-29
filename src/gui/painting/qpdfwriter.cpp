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

#include <qpdfwriter.h>

#ifndef QT_NO_PDF

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
        output = 0;
    }
    ~QPdfWriterPrivate()
    {
        delete engine;
        delete output;
    }

    QPdfEngine *engine;
    QFile *output;
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
    : QObject(*new QPdfWriterPrivate)
{
    Q_D(QPdfWriter);

    d->engine->setOutputFilename(filename);
}

/*!
  Constructs a PDF writer that will write the pdf to \a device.
  */
QPdfWriter::QPdfWriter(QIODevice *device)
    : QObject(*new QPdfWriterPrivate)
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
  \reimp
  */
void QPdfWriter::setPageSize(PageSize size)
{
    Q_D(const QPdfWriter);

    QPagedPaintDevice::setPageSize(size);
    d->engine->setPageSize(QPageSize(QPageSize::PageSizeId(size)));
}

/*!
  \reimp
  */
void QPdfWriter::setPageSizeMM(const QSizeF &size)
{
    Q_D(const QPdfWriter);

    QPagedPaintDevice::setPageSizeMM(size);
    d->engine->setPageSize(QPageSize(size, QPageSize::Millimeter));
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
  \reimp
  */
void QPdfWriter::setMargins(const Margins &m)
{
    Q_D(QPdfWriter);

    QPagedPaintDevice::setMargins(m);

    const qreal multiplier = 72./25.4;
    d->engine->setPageMargins(QMarginsF(m.left * multiplier, m.top * multiplier,
                                        m.right * multiplier, m.bottom * multiplier), QPageLayout::Point);
}

QT_END_NAMESPACE

#endif // QT_NO_PDF
