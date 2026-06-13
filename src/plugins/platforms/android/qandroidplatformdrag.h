// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QANDROIDPLATFORMDRAG_H
#define QANDROIDPLATFORMDRAG_H

#include <qpa/qplatformdrag.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjniarray.h>
#include <QtCore/qstringlist.h>

#include <memory>

#if QT_CONFIG(draganddrop)

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtDragManager, "org/qtproject/qt/android/QtDragManager");

class QEventLoop;
class QJniEnvironment;
class QMimeData;

class QAndroidPlatformDrag : public QPlatformDrag
{
public:
    QAndroidPlatformDrag();
    ~QAndroidPlatformDrag();

    Qt::DropAction drag(QDrag *drag) override;
    void cancelDrag() override;
    bool ownsDragObject() const override;

    static bool registerNatives(QJniEnvironment &env);

private:
    static bool handleNativeDragEvent(jlong nativePointer, int viewId, int action, float x,
                                      float y, const QStringList &mimeTypes,
                                      const QStringList &clipData, bool result);
    bool deliverDragEvent(int viewId, int action, float x, float y,
                          const QStringList &mimeTypes, const QStringList &clipData,
                          bool result);

    static QString shareableUri(const QString &value);

    static jboolean onDragEvent(JNIEnv *env, jobject obj, jlong nativePointer,
                                jint viewId, jint action, jfloat x, jfloat y,
                                QJniArray<QString> mimeTypes, QJniArray<QString> clipData,
                                jboolean result);
    Q_DECLARE_JNI_NATIVE_METHOD_IN_CURRENT_SCOPE(onDragEvent)

    QtJniTypes::QtDragManager m_dragManager = nullptr;
    std::unique_ptr<QMimeData> m_externalMimeData; // Mime data for an incoming cross-process drag
    Qt::DropAction m_executedAction = Qt::IgnoreAction;
    QEventLoop *m_dragLoop = nullptr;

    // The live drag handler, touched only on the Qt GUI thread.
    static QAndroidPlatformDrag *s_currentDrag;
};

QT_END_NAMESPACE

#endif // QT_CONFIG(draganddrop)

#endif // QANDROIDPLATFORMDRAG_H
