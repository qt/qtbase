// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>

#include <QtGui/qwindow.h>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformtheme.h>

#include <QtCore/private/qcore_mac_p.h>

#include "qiosglobal.h"
#include "quiview.h"
#include "qiosscreen.h"
#include "qiosmessagedialog.h"

using namespace Qt::StringLiterals;

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

    text.replace("<p>"_L1, QStringLiteral("\n"), Qt::CaseInsensitive);
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

    if (!options()->checkBoxLabel().isNull())
        return false; // Can't support

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
    if (!window) {
        qCDebug(lcQpaWindow, "Attempting to exec a dialog without any window/widget visible.");

        auto *primaryScreen = static_cast<QIOSScreen*>(QGuiApplication::primaryScreen()->handle());
        Q_ASSERT(primaryScreen);

        window = primaryScreen->uiWindow();
        if (window.hidden) {
            // With a window hidden, an attempt to present view controller
            // below fails with a warning, that a view "is not a part of
            // any view hierarchy". The UIWindow is initially hidden,
            // as unhiding it is what hides the splash screen.
            window.hidden = NO;
        }
    }

    if (!window)
        return false;

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
