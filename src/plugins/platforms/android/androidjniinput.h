/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
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

#ifndef ANDROIDJNIINPUT_H
#define ANDROIDJNIINPUT_H

#include <jni.h>
#include <QtCore/qglobal.h>
#include <QtCore/QRect>

QT_BEGIN_NAMESPACE

namespace QtAndroidInput
{
    // Software keyboard support
    void showSoftwareKeyboard(int top, int left, int width, int editorHeight, int height, int inputHints, int enterKeyType);
    void resetSoftwareKeyboard();
    void hideSoftwareKeyboard();
    bool isSoftwareKeyboardVisible();
    QRect softwareKeyboardRect();
    void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd);
    // Software keyboard support

    // edit field resize
    void updateInputItemRectangle(int left, int top, int width, int height);
    // cursor/selection handles
    void updateHandles(int handleCount, QPoint editMenuPos = QPoint(), uint32_t editButtons = 0, QPoint cursor = QPoint(), QPoint anchor = QPoint(), bool rtl = false);

    bool registerNatives(JNIEnv *env);
}

QT_END_NAMESPACE

#endif // ANDROIDJNIINPUT_H
