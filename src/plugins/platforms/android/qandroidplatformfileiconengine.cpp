// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qandroidplatformfileiconengine.h"

#ifndef QT_NO_ICON

#include "androidjnimain.h"

#include <QtCore/qdebug.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qscopeguard.h>

#include <android/bitmap.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(lcAndroidFileIconEngine, "qt.qpa.theme.fileiconengine")

using namespace Qt::StringLiterals;
using namespace QtJniTypes;

Q_DECLARE_JNI_CLASS(CharSequence, "java/lang/CharSequence")
Q_DECLARE_JNI_CLASS(Icon, "android/graphics/drawable/Icon")
Q_DECLARE_JNI_CLASS(Bitmap, "android/graphics/Bitmap")
Q_DECLARE_JNI_CLASS(Canvas, "android/graphics/Canvas")
Q_DECLARE_JNI_CLASS(MimeTypeMap, "android/webkit/MimeTypeMap")
Q_DECLARE_JNI_CLASS(MimeTypeInfo, "android/content/ContentResolver$MimeTypeInfo")

QAndroidPlatformFileIconEngine::QAndroidPlatformFileIconEngine(const QFileInfo &fileInfo,
                                                               QPlatformTheme::IconOptions opts)
    : QAbstractFileIconEngine(fileInfo, opts)
{
    // MimeTypeInfo requires API level 29
    static bool hasMimeTypeInfo = []{
        if (!MimeTypeInfo::isClassAvailable()) {
            qCWarning(lcAndroidFileIconEngine) << "MimeTypeInfo not available, requires API level 29";
            return false;
        }
        return true;
    }();
    if (!hasMimeTypeInfo)
        return;

    const auto context = QtAndroidPrivate::context();
    if (!context.isValid()) {
        qCWarning(lcAndroidFileIconEngine) << "Couldn't get context";
        return;
    }
    const auto contentResolver = context.callMethod<ContentResolver>("getContentResolver");
    if (!contentResolver.isValid()) {
        qCWarning(lcAndroidFileIconEngine) << "Couldn't get content resolver";
        return;
    }

    const auto mimeTypeMap = MimeTypeMap::callStaticMethod<MimeTypeMap>("getSingleton");
    const QString mimeType = mimeTypeMap.callMethod<QString>("getMimeTypeFromExtension",
                                                             fileInfo.suffix());

    const auto mimeTypeInfo = contentResolver.callMethod<MimeTypeInfo>("getTypeInfo", mimeType);
    qCDebug(lcAndroidFileIconEngine) << "MimeTypeInfo" << mimeType
                                     << mimeTypeInfo.callMethod<CharSequence>("getLabel").toString()
                                     << mimeTypeInfo.callMethod<CharSequence>("getContentDescription").toString();
    const auto icon = mimeTypeInfo.callMethod<Icon>("getIcon");
    if (!icon.isValid()) {
        qCDebug(lcAndroidFileIconEngine) << "No valid icon in type info";
        return;
    }
    m_drawable = icon.callMethod<Drawable>("loadDrawable", context);
    if (!m_drawable || !m_drawable->isValid())
        qCWarning(lcAndroidFileIconEngine) << "Failed to load drawable for icon";
}

QAndroidPlatformFileIconEngine::~QAndroidPlatformFileIconEngine() = default;

bool QAndroidPlatformFileIconEngine::isNull()
{
    return !m_drawable || !m_drawable->isValid();
}

QPixmap QAndroidPlatformFileIconEngine::filePixmap(const QSize &size, QIcon::Mode, QIcon::State)
{
    if (m_pixmap.size() == size)
        return m_pixmap;
    if (isNull())
        return QPixmap();

    JNIEnv *jniEnv = QJniEnvironment::getJniEnv();
    // createBitmap doesn't support ARGB32, but it doesn't matter here
    Bitmap bitmap = QtAndroid::createBitmap(size.width(), size.height(),
                                            QImage::Format_RGBA8888, jniEnv);
    if (!bitmap.isValid()) {
        qCWarning(lcAndroidFileIconEngine) << "Failed to create bitmap";
        return QPixmap();
    }
    Canvas canvas(bitmap);
    m_drawable->callMethod("setBounds", 0, 0, size.width(), size.height());
    m_drawable->callMethod("draw", canvas);

    void *pixels = nullptr;
    if (ANDROID_BITMAP_RESULT_SUCCESS != AndroidBitmap_lockPixels(jniEnv, bitmap.object(), &pixels)) {
        qCWarning(lcAndroidFileIconEngine) << "Failed to lock bitmap pixels";
        return QPixmap();
    }

    // this makes a deep copy of the pixel data
    m_pixmap = QPixmap::fromImage(QImage(reinterpret_cast<const uchar *>(pixels),
                                         size.width(), size.height(), QImage::Format_RGBA8888));
    if (ANDROID_BITMAP_RESULT_SUCCESS != AndroidBitmap_unlockPixels(jniEnv, bitmap.object()))
        qCWarning(lcAndroidFileIconEngine) << "Failed to unlock bitmap pixels";
    return m_pixmap;
}

QT_END_NAMESPACE

#endif // QT_NO_ICON
