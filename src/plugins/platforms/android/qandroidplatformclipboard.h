// Copyright (C) 2023 The Qt Company Ltd.
// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMCLIPBOARD_H
#define QANDROIDPLATFORMCLIPBOARD_H

#include <qpa/qplatformclipboard.h>
#include <QMimeData>
#include <QtCore/qjnitypes.h>

#ifndef QT_NO_CLIPBOARD

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtClipboardManager, "org/qtproject/qt/android/QtClipboardManager");

class QAndroidPlatformClipboard : public QPlatformClipboard
{
public:
    QAndroidPlatformClipboard();
    ~QAndroidPlatformClipboard();
    QMimeData *mimeData(QClipboard::Mode mode = QClipboard::Clipboard) override;
    void setMimeData(QMimeData *data, QClipboard::Mode mode = QClipboard::Clipboard) override;
    bool supportsMode(QClipboard::Mode mode) const override;

    static bool registerNatives(QJniEnvironment &env);

private:
    QMimeData *getClipboardMimeData();
    void setClipboardMimeData(QMimeData *data);
    void clearClipboardData();

    static void onClipboardDataChanged(JNIEnv *env, jobject obj, jlong nativePointer);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onClipboardDataChanged)

    QMimeData *data = nullptr;
    QtJniTypes::QtClipboardManager m_clipboardManager = nullptr;
};

QT_END_NAMESPACE
#endif // QT_NO_CLIPBOARD

#endif // QANDROIDPLATFORMCLIPBOARD_H
