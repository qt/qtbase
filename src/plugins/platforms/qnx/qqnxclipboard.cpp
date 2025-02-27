// Copyright (C) 2011 - 2012 Research In Motion
// Copyright (c) 2020 BlackBerry Limited
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#if !defined(QT_NO_CLIPBOARD)

#include "qqnxclipboard.h"

#include <QtGui/QColor>
#include <QtGui/QImage>

#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QDirIterator>
#include <QtCore/QMimeData>
#include <QtCore/QSaveFile>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QUrl>


#include <errno.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaClipboard, "qt.qpa.clipboard");

using namespace Qt::StringLiterals;

class QQnxClipboard::MimeData : public QMimeData
{
    Q_OBJECT
public:
    MimeData(QQnxClipboard *clipboard)
        : QMimeData(),
          m_clipboard(clipboard),
          m_userMimeData(nullptr),
          m_formatsToCheck({u"text/html"_s,
                            u"text/plain"_s,
                            u"image/png"_s,
                            u"image/jpeg"_s,
                            u"application/x-color"_s})
    {
        Q_ASSERT(clipboard);
    }

    ~MimeData()
    {
    }

    void addFormatToCheck(const QString &format) {
        m_formatsToCheck << format;
        qCDebug(lcQpaClipboard) << "formats=" << m_formatsToCheck;
    }

    bool hasFormat(const QString &mimetype) const override
    {
        const bool result = m_clipboard->hasFormat(mimetype);
        qCDebug(lcQpaClipboard) << "mimetype=" << mimetype << "result=" << result;
        return result;
    }

    QStringList formats() const override
    {
        QStringList result;

        for (const QString &format : m_formatsToCheck) {
            if (m_clipboard->hasFormat(format))
                result << format;
        }

        qCDebug(lcQpaClipboard) << "result=" << result;
        return result;
    }

    void setUserMimeData(QMimeData *userMimeData)
    {
        m_userMimeData.reset(userMimeData);

        // system clipboard API doesn't allow detection of changes by other applications
        // simulate an owner change through delayed invocation
        // basically transfer ownership of data to the system clipboard once event processing resumes
        if (m_userMimeData)
            QMetaObject::invokeMethod(this, "releaseOwnership", Qt::QueuedConnection);
    }

    QMimeData *userMimeData()
    {
        return m_userMimeData.get();
    }

protected:
    QVariant retrieveData(const QString &mimetype, QMetaType preferredType) const override
    {
        qCDebug(lcQpaClipboard) << "mimetype=" << mimetype << "preferredType=" << preferredType;
        if (!m_clipboard->hasFormat(mimetype))
            return QMimeData::retrieveData(mimetype, preferredType);

        const QByteArray data = m_clipboard->read(mimetype);
        if (mimetype == "application/x-qt-image") {
            QBuffer buffer;
            buffer.setData(data);
            buffer.open(QIODevice::ReadOnly);
            QImage image;
            if (image.load(&buffer, "PNG")) {
                return QVariant::fromValue(image);
            }
        }
        return QVariant::fromValue(data);
    }

private Q_SLOTS:
    void releaseOwnership()
    {
        if (m_userMimeData) {
            qCDebug(lcQpaClipboard) << "user data formats=" << m_userMimeData->formats() << "system formats=" << formats();
            m_userMimeData.reset();
            m_clipboard->emitChanged(QClipboard::Clipboard);
        }
    }

private:
    QQnxClipboard * const m_clipboard;

    QSet<QString> m_formatsToCheck;
    std::unique_ptr<QMimeData> m_userMimeData;
};

QQnxClipboard::QQnxClipboard()
    : m_mimeData(new MimeData(this)), m_serverAvailable(false)
{
    // The clipboard is only implemented if the desktop option is specified
    // and if the QQNX_CLIPBOARD environment variable points to a directory
    // that contains a .qnxclipboard node.
    if (qEnvironmentVariableIsSet("QQNX_CLIPBOARD")) {
        m_clipboardDir = QDir(qEnvironmentVariable("QQNX_CLIPBOARD"));
        if (m_clipboardDir.exists(".qnxclipboard"))
            m_serverAvailable = true;
    }
}

QQnxClipboard::~QQnxClipboard()
{
    delete m_mimeData;
}

void QQnxClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return;

    if (m_mimeData == data)
        return;

    if (m_mimeData->userMimeData() && m_mimeData->userMimeData() == data)
        return;

    clear();

    m_mimeData->clear();
    m_mimeData->setUserMimeData(data);

    if (!data) {
        emitChanged(QClipboard::Clipboard);
        return;
    }

    const QStringList formats = data->formats();
    qCDebug(lcQpaClipboard) << "formats=" << formats;

    for (const QString &format : formats) {
        QByteArray buf;

        // Handling for image data
        if (format == "application/x-qt-image") {
            QBuffer buffer(&buf);
            buffer.open(QIODevice::WriteOnly);
            QImage image = qvariant_cast<QImage>(data->imageData());
            image.save(&buffer, "PNG");
        } else {
            buf = data->data(format);
        }

        if (buf.isEmpty())
            continue;

        bool ret = write(format, buf);
        qCDebug(lcQpaClipboard) << "set " << format << "to clipboard, size=" << buf.size() << ";ret=" << ret;
        if (ret)
            m_mimeData->addFormatToCheck(format);
    }

    emitChanged(QClipboard::Clipboard);
}

QMimeData *QQnxClipboard::mimeData(QClipboard::Mode mode)
{
    if (mode != QClipboard::Clipboard)
        return 0;

    if (m_mimeData->userMimeData())
        return m_mimeData->userMimeData();

    m_mimeData->clear();

    return m_mimeData;
}

QFile QQnxClipboard::fileforFormat(const QString &format) const
{
    // If using a standard file system directory, format names of the standard
    // "foo/bar" style cannot be handled properly.
    QString fileName = format;
    fileName.replace('/', '@');
    return QFile(m_clipboardDir.filePath(fileName));
}

bool QQnxClipboard::hasFormat(const QString &format) const
{
    if (!m_serverAvailable)
        return false;

    QFile file = fileforFormat(format);
    return file.exists() && file.size() > 0;
}

QByteArray QQnxClipboard::read(const QString &format) const
{
    if (!m_serverAvailable)
        return QByteArray();

    QFile file = fileforFormat(format);
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();

    return file.readAll();
}

bool QQnxClipboard::write(const QString &format, const QByteArray &data) const
{
    if (!m_serverAvailable)
        return false;

    QFile file = fileforFormat(format);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    return file.write(data) == data.size();
}

void QQnxClipboard::clear()
{
    if (!m_serverAvailable)
        return;

    // Warning!
    // Deletes all files in the given directory. Make sure QQNX_CLIPBOARD is set
    // to the mount point of a clipboard service or to a dedicated directory.
    // This is double-checked in the constructor, which will not start the
    // clipboard unless it points to a directory that contains a ".qnxclipboard"
    // node.
    m_clipboardDir.refresh();
    QDirIterator itr(m_clipboardDir);
    while (itr.hasNext()) {
        itr.next();
        QString format = itr.fileName();
        if (format == ".qnxclipboard"_L1)
            continue;
        if (m_clipboardDir.exists(format)) {
            qCDebug(lcQpaClipboard) << "deleting" << format;
            bool removed = m_clipboardDir.remove(format);
            if (!removed)
                qCDebug(lcQpaClipboard) << "Failed to remove" << format;
        }
    }
}

QT_END_NAMESPACE

#include "qqnxclipboard.moc"

#endif //QT_NO_CLIPBOARD
