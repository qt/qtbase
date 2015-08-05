/*
 * Copyright (C) 2014 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UBUNTU_CLIPBOARD_H
#define UBUNTU_CLIPBOARD_H

#include <qpa/qplatformclipboard.h>

#include <QMimeData>
#include <QPointer>
class QDBusInterface;
class QDBusPendingCallWatcher;

class UbuntuClipboard : public QObject, public QPlatformClipboard
{
    Q_OBJECT
public:
    UbuntuClipboard();
    virtual ~UbuntuClipboard();

    // QPlatformClipboard methods.
    QMimeData* mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData* data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;
    bool ownsMode(QClipboard::Mode mode) const override;

    void requestDBusClipboardContents();

private Q_SLOTS:
    void onDBusClipboardGetContentsFinished(QDBusPendingCallWatcher*);
    void onDBusClipboardSetContentsFinished(QDBusPendingCallWatcher*);
    void updateMimeData(const QByteArray &serializedMimeData);

private:
    void setupDBus();

    QByteArray serializeMimeData(QMimeData *mimeData) const;
    QMimeData *deserializeMimeData(const QByteArray &serializedMimeData) const;

    void setDBusClipboardContents(const QByteArray &clipboardContents);

    QMimeData *mMimeData;
    bool mIsOutdated;

    QPointer<QDBusInterface> mDBusClipboard;

    QPointer<QDBusPendingCallWatcher> mPendingGetContentsCall;
    QPointer<QDBusPendingCallWatcher> mPendingSetContentsCall;

    bool mUpdatesDisabled;
    bool mDBusSetupDone;
};

#endif // UBUNTU_CLIPBOARD_H
