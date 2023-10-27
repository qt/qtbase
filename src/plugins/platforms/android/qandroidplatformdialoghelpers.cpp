// Copyright (C) 2022 The Qt Company Ltd.
// Copyright (C) 2013 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qandroidplatformdialoghelpers.h"
#include "androidjnimain.h"

#include <QTextDocument>

#include <private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QtAndroidDialogHelpers {
static jclass g_messageDialogHelperClass = nullptr;

QAndroidPlatformMessageDialogHelper::QAndroidPlatformMessageDialogHelper()
    : m_javaMessageDialog(g_messageDialogHelperClass, "(Landroid/app/Activity;)V",
                          QtAndroidPrivate::activity().object())
{
}

QAndroidPlatformMessageDialogHelper::~QAndroidPlatformMessageDialogHelper()
{
    hide();
}

void QAndroidPlatformMessageDialogHelper::exec()
{
    if (!m_shown)
        show(Qt::Dialog, Qt::ApplicationModal, 0);
    m_loop.exec();
}

static QString htmlText(QString text)
{
    if (Qt::mightBeRichText(text))
        return text;
    text.remove(u'\r');
    return text.toHtmlEscaped().replace(u'\n', "<br />"_L1);
}

bool QAndroidPlatformMessageDialogHelper::show(Qt::WindowFlags windowFlags,
                                               Qt::WindowModality windowModality,
                                               QWindow *parent)
{
    Q_UNUSED(windowFlags);
    Q_UNUSED(windowModality);
    Q_UNUSED(parent);
    QSharedPointer<QMessageDialogOptions> opt = options();
    if (!opt.data())
        return false;

    if (!opt->checkBoxLabel().isNull())
        return false; // Can't support

    m_javaMessageDialog.callMethod<void>("setStandardIcon", "(I)V", opt->standardIcon());

    QString str = htmlText(opt->windowTitle());
    if (!str.isEmpty()) {
        m_javaMessageDialog.callMethod<void>("setTile", "(Ljava/lang/String;)V",
                                             QJniObject::fromString(str).object());
    }

    str = htmlText(opt->text());
    if (!str.isEmpty()) {
        m_javaMessageDialog.callMethod<void>("setText", "(Ljava/lang/String;)V",
                                             QJniObject::fromString(str).object());
    }

    str = htmlText(opt->informativeText());
    if (!str.isEmpty()) {
        m_javaMessageDialog.callMethod<void>("setInformativeText", "(Ljava/lang/String;)V",
                                             QJniObject::fromString(str).object());
    }

    str = htmlText(opt->detailedText());
    if (!str.isEmpty()) {
        m_javaMessageDialog.callMethod<void>("setDetailedText", "(Ljava/lang/String;)V",
                                             QJniObject::fromString(str).object());
    }

    const int *currentLayout = buttonLayout(Qt::Horizontal, AndroidLayout);
    while (*currentLayout != QPlatformDialogHelper::EOL) {
        const int role = (*currentLayout & ~QPlatformDialogHelper::Reverse);
        addButtons(opt, static_cast<ButtonRole>(role));
        ++currentLayout;
    }

    m_javaMessageDialog.callMethod<void>("show", "(J)V", jlong(static_cast<QObject*>(this)));
    m_shown = true;
    return true;
}

void QAndroidPlatformMessageDialogHelper::addButtons(QSharedPointer<QMessageDialogOptions> opt,
                                                     ButtonRole role)
{
    for (const QMessageDialogOptions::CustomButton &b : opt->customButtons()) {
        if (b.role == role) {
            QString label = b.label;
            label.remove(QChar('&'));
            m_javaMessageDialog.callMethod<void>("addButton", "(ILjava/lang/String;)V", b.id,
                                                 QJniObject::fromString(label).object());
        }
    }

    for (int i = QPlatformDialogHelper::FirstButton; i < QPlatformDialogHelper::LastButton; i<<=1) {
        StandardButton b = static_cast<StandardButton>(i);
        if (buttonRole(b) == role && (opt->standardButtons() & i)) {
            const QString text = QGuiApplicationPrivate::platformTheme()->standardButtonText(b);
            m_javaMessageDialog.callMethod<void>("addButton", "(ILjava/lang/String;)V", i,
                                                 QJniObject::fromString(text).object());
        }
    }
}

void QAndroidPlatformMessageDialogHelper::hide()
{
    m_javaMessageDialog.callMethod<void>("hide", "()V");
    m_shown = false;
}

void QAndroidPlatformMessageDialogHelper::dialogResult(int buttonID)
{
    if (m_loop.isRunning())
        m_loop.exit();
    if (buttonID < 0) {
        emit reject();
        return;
    }

    const StandardButton standardButton = static_cast<StandardButton>(buttonID);
    ButtonRole role = QPlatformDialogHelper::buttonRole(standardButton);
    if (buttonID > QPlatformDialogHelper::LastButton) {
        // In case of a custom button
        const QMessageDialogOptions::CustomButton *custom = options()->customButton(buttonID);
        Q_ASSERT(custom);
        role = custom->role;
    }

    emit clicked(standardButton, role);
}

static void dialogResult(JNIEnv * /*env*/, jobject /*thiz*/, jlong handler, int buttonID)
{
    QObject *object = reinterpret_cast<QObject *>(handler);
    QMetaObject::invokeMethod(object, "dialogResult", Qt::QueuedConnection, Q_ARG(int, buttonID));
}

static const JNINativeMethod methods[] = {
    {"dialogResult", "(JI)V", (void *)dialogResult}
};


#define FIND_AND_CHECK_CLASS(CLASS_NAME) \
    clazz = env->FindClass(CLASS_NAME); \
    if (!clazz) { \
        __android_log_print(ANDROID_LOG_FATAL, QtAndroid::qtTagText(), QtAndroid::classErrorMsgFmt(), CLASS_NAME); \
        return false; \
    }

bool registerNatives(QJniEnvironment &env)
{
    const char QtMessageHandlerHelperClassName[] = "org/qtproject/qt/android/QtMessageDialogHelper";
    jclass clazz = env.findClass(QtMessageHandlerHelperClassName);
    if (!clazz) {
        __android_log_print(ANDROID_LOG_FATAL, QtAndroid::qtTagText(), QtAndroid::classErrorMsgFmt()
                            , QtMessageHandlerHelperClassName);
        return false;
    }
    g_messageDialogHelperClass = static_cast<jclass>(env->NewGlobalRef(clazz));

    if (!env.registerNativeMethods("org/qtproject/qt/android/QtNativeDialogHelper",
                                  methods, sizeof(methods) / sizeof(methods[0]))) {
        __android_log_print(ANDROID_LOG_FATAL, "Qt", "RegisterNatives failed");
        return false;
    }

    return true;
}
}

QT_END_NAMESPACE
