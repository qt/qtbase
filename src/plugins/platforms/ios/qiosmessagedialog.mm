/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <UIKit/UIKit.h>

#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include <QtCore/private/qcore_mac_p.h>

#include "qiosglobal.h"
#include "quiview.h"
#include "qiosmessagedialog.h"

QIOSMessageDialog::QIOSMessageDialog()
    : m_alertController(nullptr)
{
}

QIOSMessageDialog::~QIOSMessageDialog()
{
    hide();
}

inline QString QIOSMessageDialog::messageTextPlain()
{
    // Concatenate text fragments, and remove HTML tags
    const QSharedPointer<QMessageDialogOptions> &opt = options();
    const QString &lineShift = QStringLiteral("\n\n");
    const QString &informativeText = opt->informativeText();
    const QString &detailedText = opt->detailedText();

    QString text = opt->text();
    if (!informativeText.isEmpty())
        text += lineShift + informativeText;
    if (!detailedText.isEmpty())
        text += lineShift + detailedText;

    text.replace(QLatin1String("<p>"), QStringLiteral("\n"), Qt::CaseInsensitive);
    text.remove(QRegularExpression(QStringLiteral("<[^>]*>")));

    return text;
}

inline UIAlertAction *QIOSMessageDialog::createAction(
        const QMessageDialogOptions::CustomButton &customButton)
{
    const QString label = QPlatformTheme::removeMnemonics(customButton.label);
    const UIAlertActionStyle style = UIAlertActionStyleDefault;

    return [UIAlertAction actionWithTitle:label.toNSString() style:style handler:^(UIAlertAction *) {
        hide();
        emit clicked(static_cast<QPlatformDialogHelper::StandardButton>(customButton.id), customButton.role);
    }];
}

inline UIAlertAction *QIOSMessageDialog::createAction(StandardButton button)
{
    const StandardButton labelButton = button == NoButton ? Ok : button;
    const QString &standardLabel = QGuiApplicationPrivate::platformTheme()->standardButtonText(labelButton);
    const QString &label = QPlatformTheme::removeMnemonics(standardLabel);

    UIAlertActionStyle style = UIAlertActionStyleDefault;
    if (button == Cancel)
        style = UIAlertActionStyleCancel;
    else if (button == Discard)
        style = UIAlertActionStyleDestructive;

    return [UIAlertAction actionWithTitle:label.toNSString() style:style handler:^(UIAlertAction *) {
        hide();
        if (button == NoButton)
            emit reject();
        else
            emit clicked(button, buttonRole(button));
    }];
}

void QIOSMessageDialog::exec()
{
    m_eventLoop.exec(QEventLoop::DialogExec);
}

bool QIOSMessageDialog::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags);
    if (m_alertController // Ensure that the dialog is not showing already
            || !options() // Some message dialogs don't have options (QErrorMessage)
            || windowModality == Qt::NonModal) // We can only do modal dialogs
        return false;

    m_alertController = [[UIAlertController
        alertControllerWithTitle:options()->windowTitle().toNSString()
        message:messageTextPlain().toNSString()
        preferredStyle:UIAlertControllerStyleAlert] retain];

    const QVector<QMessageDialogOptions::CustomButton> customButtons = options()->customButtons();
    for (const QMessageDialogOptions::CustomButton &button : customButtons) {
        UIAlertAction *act = createAction(button);
        [m_alertController addAction:act];
    }

    if (StandardButtons buttons = options()->standardButtons()) {
        for (int i = FirstButton; i < LastButton; i<<=1) {
            if (i & buttons)
                [m_alertController addAction:createAction(StandardButton(i))];
        }
    } else if (customButtons.isEmpty()) {
        // We need at least one button to allow the user close the dialog
        [m_alertController addAction:createAction(NoButton)];
    }

    UIWindow *window = parent ? reinterpret_cast<UIView *>(parent->winId()).window : qt_apple_sharedApplication().keyWindow;
    [window.rootViewController presentViewController:m_alertController animated:YES completion:nil];
    return true;
}

void QIOSMessageDialog::hide()
{
    m_eventLoop.exit();
    [m_alertController dismissViewControllerAnimated:YES completion:nil];
    [m_alertController release];
    m_alertController = nullptr;
}
