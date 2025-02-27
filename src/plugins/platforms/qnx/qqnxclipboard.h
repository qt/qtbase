// Copyright (C) 2011 - 2012 Research In Motion
// Copyright (c) 2020 BlackBerry Limited
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQNXCLIPBOARD_H
#define QQNXCLIPBOARD_H

#include <QtCore/qglobal.h>
#include <QtCore/qdir.h>
#include <QtCore/qloggingcategory.h>

#if !defined(QT_NO_CLIPBOARD)
#include <qpa/qplatformclipboard.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaClipboard);

class QQnxClipboard : public QPlatformClipboard
{
public:
    QQnxClipboard();
    ~QQnxClipboard();
    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;

private:
    class MimeData;
    MimeData *m_mimeData;
    QDir m_clipboardDir;
    bool m_serverAvailable;

    [[nodiscard]] QFile fileforFormat(const QString &format) const;
    [[nodiscard]] bool hasFormat(const QString &format) const;
    [[nodiscard]] QByteArray read(const QString &format) const;
    [[nodiscard]] bool write(const QString &format, const QByteArray &data) const;
    void clear();
};

QT_END_NAMESPACE

#endif //QT_NO_CLIPBOARD
#endif //QQNXCLIPBOARD_H
