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

namespace QtAndroidWindowEmbedding {
    void createRootWindow(JNIEnv *, jclass, QtJniTypes::View rootView,
                          jint x, jint y, jint width, jint height)
    {
        // QWindow should be constructed on the Qt thread rather than directly in the caller thread
        // To avoid hitting checkReceiverThread assert in QCoreApplication::doNotify
        QMetaObject::invokeMethod(qApp, [rootView, x, y, width, height] {
            QWindow *parentWindow = QWindow::fromWinId(reinterpret_cast<WId>(rootView.object()));
            parentWindow->setGeometry(x, y, width, height);
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
                    window->parent()->showNormal();
            } else {
                window->hide();
            }
        });
    }

    void resizeWindow(JNIEnv *, jclass, jlong windowRef, jint x, jint y, jint width, jint height)
    {
        QMetaObject::invokeMethod(qApp, [windowRef, x, y, width, height] {
            QWindow *window = reinterpret_cast<QWindow*>(windowRef);
            QWindow *parent = window->parent();
            if (parent)
                parent->setGeometry(x, y, width, height);
            window->setGeometry(0, 0, width, height);
        });
    }

    bool registerNatives(QJniEnvironment& env) {
        return env.registerNativeMethods(
                QtJniTypes::Traits<QtJniTypes::QtView>::className(),
                { Q_JNI_NATIVE_SCOPED_METHOD(createRootWindow, QtAndroidWindowEmbedding),
                  Q_JNI_NATIVE_SCOPED_METHOD(deleteWindow, QtAndroidWindowEmbedding),
                  Q_JNI_NATIVE_SCOPED_METHOD(setWindowVisible, QtAndroidWindowEmbedding),
                  Q_JNI_NATIVE_SCOPED_METHOD(resizeWindow, QtAndroidWindowEmbedding) });
    }
}

QT_END_NAMESPACE
