/****************************************************************************
**
** Copyright (C) 2013 BogDan Vatra <bogdan@kde.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QANDROIDPLATFORMDIALOGHELPERS_H
#define QANDROIDPLATFORMDIALOGHELPERS_H
#include <jni.h>
#include <qpa/qplatformdialoghelper.h>
#include <QEventLoop>
#include <private/qjni_p.h>

QT_BEGIN_NAMESPACE

namespace QtAndroidDialogHelpers {

class QAndroidPlatformMessageDialogHelper: public QPlatformMessageDialogHelper
{
    Q_OBJECT
public:
    QAndroidPlatformMessageDialogHelper();
    void exec() override;
    bool show(Qt::WindowFlags windowFlags,
              Qt::WindowModality windowModality,
              QWindow *parent) override;
    void hide() override;

public slots:
    void dialogResult(int buttonID);

private:
    void addButtons(QSharedPointer<QMessageDialogOptions> opt, ButtonRole role);

private:
    int m_buttonId;
    QEventLoop m_loop;
    QJNIObjectPrivate m_javaMessageDialog;
    bool m_shown;
};


bool registerNatives(JNIEnv *env);

}

QT_END_NAMESPACE

#endif // QANDROIDPLATFORMDIALOGHELPERS_H
