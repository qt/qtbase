/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qtexturefiledata_p.h"
#include <QMetaEnum>
#include <QSize>
#if QT_CONFIG(opengl)
#include <QOpenGLTexture>
#endif

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQtGuiTextureIO, "qt.gui.textureio");

class QTextureFileDataPrivate : public QSharedData
{
public:
    QTextureFileDataPrivate()
    {
    }

    QTextureFileDataPrivate(const QTextureFileDataPrivate &other)
        : QSharedData(other),
          logName(other.logName),
          data(other.data),
          offsets(other.offsets),
          lengths(other.lengths),
          size(other.size),
          format(other.format)
    {
    }

    ~QTextureFileDataPrivate()
    {
    }

    void ensureLevels(int num, bool force = false)
    {
        const int newSize = force ? num : qMax(offsets.size(), num);
        offsets.resize(newSize);
        lengths.resize(newSize);
    }

    QByteArray logName;
    QByteArray data;
    QVector<int> offsets;
    QVector<int> lengths;
    QSize size;
    quint32 format = 0;
    quint32 internalFormat = 0;
    quint32 baseInternalFormat = 0;
};



QTextureFileData::QTextureFileData()
{
}

QTextureFileData::QTextureFileData(const QTextureFileData &other)
    : d(other.d)
{
}

QTextureFileData &QTextureFileData::operator=(const QTextureFileData &other)
{
    d = other.d;
    return *this;
}

QTextureFileData::~QTextureFileData()
{
}

bool QTextureFileData::isNull() const
{
    return !d;
}

bool QTextureFileData::isValid() const
{
    if (!d)
        return false;

    if (d->data.isEmpty() || d->size.isEmpty() || (!d->format && !d->internalFormat))
        return false;

    const int numChunks = d->offsets.size();
    if (numChunks == 0 || (d->lengths.size() != numChunks))
         return false;

    const qint64 sz = d->data.size();
    for (int i = 0; i < numChunks; i++) {
        qint64 offi = d->offsets.at(i);
        qint64 leni = d->lengths.at(i);
        if (offi < 0 || offi >= sz || leni <= 0 || (offi + leni > sz))
            return false;
    }
    return true;
}

void QTextureFileData::clear()
{
    d = nullptr;
}

QByteArray QTextureFileData::data() const
{
    return d ? d->data : QByteArray();
}

void QTextureFileData::setData(const QByteArray &data)
{
    if (!d.constData())  //### uh think about this design, this is the only way to create; should be constructor instead at least
        d = new QTextureFileDataPrivate;

    d->data = data;
}

int QTextureFileData::dataOffset(int level) const
{
    return (d && d->offsets.size() > level) ? d->offsets.at(level) : 0;
}

void QTextureFileData::setDataOffset(int offset, int level)
{
    if (d.constData() && level >= 0) {
        d->ensureLevels(level + 1);
        d->offsets[level] = offset;
    }
}

int QTextureFileData::dataLength(int level) const
{
    return (d && d->lengths.size() > level) ? d->lengths.at(level) : 0;
}

void QTextureFileData::setDataLength(int length, int level)
{
    if (d.constData() && level >= 0) {
        d->ensureLevels(level + 1);
        d->lengths[level] = length;
    }
}

int QTextureFileData::numLevels() const
{
    return d ? d->offsets.size() : 0;
}

void QTextureFileData::setNumLevels(int num)
{
    if (d && num >= 0)
        d->ensureLevels(num, true);
}

QSize QTextureFileData::size() const
{
    return d ? d->size : QSize();
}

void QTextureFileData::setSize(const QSize &size)
{
    if (d.constData())
        d->size = size;
}

quint32 QTextureFileData::glFormat() const
{
    return d ? d->format : 0;
}

void QTextureFileData::setGLFormat(quint32 format)
{
    if (d.constData())
        d->format = format;
}

quint32 QTextureFileData::glInternalFormat() const
{
    return d ? d->internalFormat : 0;
}

void QTextureFileData::setGLInternalFormat(quint32 format)
{
    if (d.constData())
        d->internalFormat = format;
}

quint32 QTextureFileData::glBaseInternalFormat() const
{
    return d ? d->baseInternalFormat : 0;
}

void QTextureFileData::setGLBaseInternalFormat(quint32 format)
{
    if (d.constData())
        d->baseInternalFormat = format;
}

QByteArray QTextureFileData::logName() const
{
    return d ? d->logName : QByteArray();
}

void QTextureFileData::setLogName(const QByteArray &name)
{
    if (d.constData())
        d->logName = name;
}

static QByteArray glFormatName(quint32 fmt)
{
    const char *id = nullptr;
#if QT_CONFIG(opengl)
    id = QMetaEnum::fromType<QOpenGLTexture::TextureFormat>().valueToKey(fmt);
#endif
    QByteArray res(id ? id : "(?)");
    res += " [0x" + QByteArray::number(fmt, 16).rightJustified(4, '0') + ']';
    return res;
}

QDebug operator<<(QDebug dbg, const QTextureFileData &d)
{
    QDebugStateSaver saver(dbg);

    dbg.nospace() << "QTextureFileData(";
    if (!d.isNull()) {
        dbg.space() << d.logName() << d.size();
        dbg << "glFormat:" << glFormatName(d.glFormat());
        dbg << "glInternalFormat:" << glFormatName(d.glInternalFormat());
        dbg << "glBaseInternalFormat:" << glFormatName(d.glBaseInternalFormat());
        dbg.nospace() << "Levels: " << d.numLevels();
        if (!d.isValid())
            dbg << " {Invalid}";
        dbg << ")";
    } else {
        dbg << "null)";
    }

    return dbg;
}

QT_END_NAMESPACE
