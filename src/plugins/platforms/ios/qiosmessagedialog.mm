/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#import <UIKit/UIKit.h>

#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include "qiosglobal.h"
#include "quiview.h"
#include "qiosmessagedialog.h"

QIOSMessageDialog::QIOSMessageDialog()
    : m_alertController(Q_NULLPTR)
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
            || windowModality != Qt::ApplicationModal // We can only do app modal dialogs
            || QSysInfo::MacintoshVersion < QSysInfo::MV_IOS_8_0) // API limitation
        return false;

    m_alertController = [[UIAlertController
        alertControllerWithTitle:options()->windowTitle().toNSString()
        message:messageTextPlain().toNSString()
        preferredStyle:UIAlertControllerStyleAlert] retain];

    if (StandardButtons buttons = options()->standardButtons()) {
        for (int i = FirstButton; i < LastButton; i<<=1) {
            if (i & buttons)
                [m_alertController addAction:createAction(StandardButton(i))];
        }
    } else {
        // We need at least one button to allow the user close the dialog
        [m_alertController addAction:createAction(NoButton)];
    }

    UIWindow *window = parent ? reinterpret_cast<UIView *>(parent->winId()).window : [UIApplication sharedApplication].keyWindow;
    [window.rootViewController presentViewController:m_alertController animated:YES completion:nil];
    return true;
}

void QIOSMessageDialog::hide()
{
    m_eventLoop.exit();
    [m_alertController dismissViewControllerAnimated:YES completion:nil];
    [m_alertController release];
    m_alertController = Q_NULLPTR;
}
