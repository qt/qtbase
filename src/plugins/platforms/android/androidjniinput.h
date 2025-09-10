// Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDJNIINPUT_H
#define ANDROIDJNIINPUT_H

#include <jni.h>
#include <QtCore/qglobal.h>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRect>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaInputMethods);

class QJniEnvironment;

namespace QtAndroidInput
{
    // Software keyboard support
    void showSoftwareKeyboard(int top, int left, int width, int height, int inputHints, int enterKeyType);
    void resetSoftwareKeyboard();
    void hideSoftwareKeyboard();
    bool isSoftwareKeyboardVisible();
    QRect softwareKeyboardRect();
    void updateSelection(int selStart, int selEnd, int candidatesStart, int candidatesEnd);
    // Software keyboard support

    // cursor/selection handles
    void updateHandles(int handleCount, QPoint editMenuPos = QPoint(), uint32_t editButtons = 0,
                       QPoint cursor = QPoint(), QPoint anchor = QPoint(), bool rtl = false);
    int getSelectHandleWidth();

    bool registerNatives(QJniEnvironment &env);
}

QT_END_NAMESPACE

#endif // ANDROIDJNIINPUT_H
