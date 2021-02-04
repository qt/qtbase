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
#include <QSize>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQtGuiTextureIO, "qt.gui.textureio");

constexpr size_t MAX_FACES = 6;

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
          format(other.format),
          numFaces(other.numFaces),
          numLevels(other.numLevels)
    {
    }

    ~QTextureFileDataPrivate()
    {
    }

    void ensureSize(int levels, int faces, bool force = false)
    {
        numLevels = force ? levels : qMax(numLevels, levels);
        numFaces = force ? faces : qMax(numFaces, faces);

        offsets.resize(numFaces);
        lengths.resize(numFaces);

        for (auto faceList : { &offsets, &lengths })
            for (auto &levelList : *faceList)
                levelList.resize(numLevels);
    }

    bool isValid(int level, int face) const { return level < numLevels && face < numFaces; }

    int getOffset(int level, int face) const { return offsets[face][level]; }
    void setOffset(int value, int level, int face) { offsets[face][level] = value; }
    int getLength(int level, int face) const { return lengths[face][level]; }
    void setLength(int value, int level, int face) { lengths[face][level] = value; }

    QByteArray logName;
    QByteArray data;
    QVarLengthArray<QList<int>, MAX_FACES> offsets; // [Face][Level] = offset
    QVarLengthArray<QList<int>, MAX_FACES> lengths; // [Face][Level] = length
    QSize size;
    quint32 format = 0;
    quint32 internalFormat = 0;
    quint32 baseInternalFormat = 0;
    int numFaces = 0;
    int numLevels = 0;
    QMap<QByteArray, QByteArray> keyValues;
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

    const int numFacesOffset = d->offsets.length();
    const int numFacesLength = d->lengths.length();
    if (numFacesOffset == 0 || numFacesLength == 0 || d->numFaces != numFacesOffset
        || d->numFaces != numFacesLength)
        return false;

    const qint64 dataSize = d->data.size();

    // Go through all faces and levels and check that the range is inside the data size.
    for (int face = 0; face < d->numFaces; face++) {
        const int numLevelsOffset = d->offsets.at(face).size();
        const int numLevelsLength = d->lengths.at(face).size();
        if (numLevelsOffset == 0 || numLevelsLength == 0 || d->numLevels != numLevelsOffset
            || d->numLevels != numLevelsLength)
            return false;

        for (int level = 0; level < d->numLevels; level++) {
            const qint64 offset = d->getOffset(level, face);
            const qint64 length = d->getLength(level, face);
            if (offset < 0 || offset >= dataSize || length <= 0 || (offset + length > dataSize))
                return false;
        }
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

int QTextureFileData::dataOffset(int level, int face) const
{
    return (d && d->isValid(level, face)) ? d->getOffset(level, face) : 0;
}

void QTextureFileData::setDataOffset(int offset, int level, int face)
{
    if (d.constData() && level >= 0) {
        d->ensureSize(level + 1, face + 1);
        d->setOffset(offset, level, face);
    }
}

int QTextureFileData::dataLength(int level, int face) const
{
    return (d && d->isValid(level, face)) ? d->getLength(level, face) : 0;
}

QByteArrayView QTextureFileData::getDataView(int level, int face) const
{
    const int dataLength = this->dataLength(level, face);
    const int dataOffset = this->dataOffset(level, face);

    if (d == nullptr || dataLength == 0)
        return QByteArrayView();

    return QByteArrayView(d->data.constData() + dataOffset, dataLength);
}

void QTextureFileData::setDataLength(int length, int level, int face)
{
    if (d.constData() && level >= 0) {
        d->ensureSize(level + 1, face + 1);
        d->setLength(length, level, face);
    }
}

int QTextureFileData::numLevels() const
{
    return d ? d->numLevels : 0;
}

void QTextureFileData::setNumLevels(int numLevels)
{
    if (d && numLevels >= 0)
        d->ensureSize(numLevels, d->numFaces, true);
}

int QTextureFileData::numFaces() const
{
    return d ? d->numFaces : 0;
}

void QTextureFileData::setNumFaces(int numFaces)
{
    if (d && numFaces >= 0)
        d->ensureSize(d->numLevels, numFaces, true);
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

QMap<QByteArray, QByteArray> QTextureFileData::keyValueMetadata() const
{
    return d ? d->keyValues : QMap<QByteArray, QByteArray>();
}

void QTextureFileData::setKeyValueMetadata(const QMap<QByteArray, QByteArray> &keyValues)
{
    if (d)
        d->keyValues = keyValues;
}

static QByteArray glFormatName(quint32 fmt)
{
    return QByteArray("0x" + QByteArray::number(fmt, 16).rightJustified(4, '0'));
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
