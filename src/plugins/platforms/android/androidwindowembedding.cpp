// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "androidwindowembedding.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnitypes.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(QtView, "org/qtproject/qt/android/QtView");
Q_DECLARE_JNI_CLASS(QtEmbeddedDelegate, "org/qtproject/qt/android/QtEmbeddedDelegate");

namespace QtAndroidWindowEmbedding {
    void createRootWindow(JNIEnv *, jclass, QtJniTypes::View rootView)
    {
        // QWindow should be constructed on the Qt thread rather than directly in the caller thread
        // To avoid hitting checkReceiverThread assert in QCoreApplication::doNotify
        QMetaObject::invokeMethod(qApp, [rootView] {
            QWindow *parentWindow = QWindow::fromWinId(reinterpret_cast<WId>(rootView.object()));
            rootView.callMethod<void>("createWindow", reinterpret_cast<jlong>(parentWindow));
        });
    }

    void deleteWindow(JNIEnv *, jclass, jlong windowRef)
    {
        QWindow *window = reinterpret_cast<QWindow*>(windowRef);
        window->deleteLater();
    }

    void setWindowVisible(JNIEnv *, jclass, jlong windowRef, jboolean visible)
    {
        QMetaObject::invokeMethod(qApp, [windowRef, visible] {
            QWindow *window = reinterpret_cast<QWindow*>(windowRef);
            if (visible) {
                window->showNormal();
                if (!window->parent()->isVisible())
                    window->parent()->show();
            } else {
                window->hide();
            }
        });
    }

    void resizeWindow(JNIEnv *, jclass, jlong windowRef, jint width, jint height)
    {
        QWindow *window = reinterpret_cast<QWindow*>(windowRef);
        window->setWidth(width);
        window->setHeight(height);
    }

    bool registerNatives(QJniEnvironment& env) {
        using namespace QtJniTypes;
        bool success = env.registerNativeMethods(Traits<QtEmbeddedDelegate>::className(),
                            {Q_JNI_NATIVE_SCOPED_METHOD(createRootWindow, QtAndroidWindowEmbedding),
                             Q_JNI_NATIVE_SCOPED_METHOD(deleteWindow, QtAndroidWindowEmbedding)});

        success &= env.registerNativeMethods(Traits<QtView>::className(),
                            {Q_JNI_NATIVE_SCOPED_METHOD(setWindowVisible, QtAndroidWindowEmbedding),
                             Q_JNI_NATIVE_SCOPED_METHOD(resizeWindow, QtAndroidWindowEmbedding)});
        return success;

    }
}

QT_END_NAMESPACE
