// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>

#include <QtGui/qwindow.h>
#include <QDebug>

#include <QtCore/private/qcore_mac_p.h>

#include "qiosglobal.h"
#include "qioscolordialog.h"
#include "qiosintegration.h"

@interface QIOSColorDialogController : UIColorPickerViewController <UIColorPickerViewControllerDelegate,
                                                                    UIAdaptivePresentationControllerDelegate>
@end

@implementation QIOSColorDialogController {
    QIOSColorDialog *m_colorDialog;
}

- (instancetype)initWithQIOSColorDialog:(QIOSColorDialog *)dialog
{
    if (self = [super init]) {
        m_colorDialog = dialog;
        self.delegate = self;
        self.presentationController.delegate = self;
        self.supportsAlpha = dialog->options()->testOption(QColorDialogOptions::ShowAlphaChannel);
    }
    return self;
}

- (void)setQColor:(const QColor &)qColor
{
    UIColor *uiColor;
    const QColor::Spec spec = qColor.spec();
    if (spec == QColor::Hsv) {
        uiColor = [UIColor colorWithHue:qColor.hsvHueF()
                             saturation:qColor.hsvSaturationF()
                             brightness:qColor.valueF()
                                  alpha:qColor.alphaF()];
    } else {
        uiColor = [UIColor colorWithRed:qColor.redF()
                                  green:qColor.greenF()
                                   blue:qColor.blueF()
                                  alpha:qColor.alphaF()];
    }
    self.selectedColor = uiColor;
}

- (void)updateQColor
{
    UIColor *color = self.selectedColor;
    CGFloat red = 0, green = 0, blue = 0, alpha = 0;

    QColor newColor;
    if ([color getRed:&red green:&green blue:&blue alpha:&alpha])
        newColor.setRgbF(red, green, blue, alpha);
    else
        qWarning() << "Incompatible color space";


    if (m_colorDialog) {
        m_colorDialog->updateColor(newColor);
        emit m_colorDialog->currentColorChanged(newColor);
    }
}

// ----------------------UIColorPickerViewControllerDelegate--------------------------
- (void)colorPickerViewControllerDidSelectColor:(UIColorPickerViewController *)viewController
{
    Q_UNUSED(viewController);
    [self updateQColor];
}

- (void)colorPickerViewControllerDidFinish:(UIColorPickerViewController *)viewController
{
    Q_UNUSED(viewController);
    [self updateQColor];
    emit m_colorDialog->accept();
}

// ----------------------UIAdaptivePresentationControllerDelegate--------------------------
- (void)presentationControllerDidDismiss:(UIPresentationController *)presentationController
{
    Q_UNUSED(presentationController);
    emit m_colorDialog->reject();
}

@end

QIOSColorDialog::QIOSColorDialog()
    : m_viewController(nullptr)
{
}

QIOSColorDialog::~QIOSColorDialog()
{
    hide();
}

void QIOSColorDialog::exec()
{
    m_eventLoop.exec(QEventLoop::DialogExec);
}

bool QIOSColorDialog::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags);

    if (!m_viewController) {
        m_viewController = [[QIOSColorDialogController alloc] initWithQIOSColorDialog:this];
        if (m_currentColor.isValid())
            [m_viewController setQColor:m_currentColor];
    }

    if (windowModality == Qt::ApplicationModal || windowModality == Qt::WindowModal)
        m_viewController.modalInPresentation = YES;

    UIWindow *window = presentationWindow(parent);
    if (!window)
        return false;

    // We can't present from view controller if already presenting
    if (window.rootViewController.presentedViewController)
        return false;

    [window.rootViewController presentViewController:m_viewController animated:YES completion:nil];

    return true;
}

void QIOSColorDialog::hide()
{
    [m_viewController dismissViewControllerAnimated:YES completion:nil];
    [m_viewController release];
    m_viewController = nullptr;
    m_eventLoop.exit();
}

void QIOSColorDialog::setCurrentColor(const QColor &color)
{
    updateColor(color);
    if (m_viewController)
        [m_viewController setQColor:color];
}

QColor QIOSColorDialog::currentColor() const
{
    return m_currentColor;
}

void QIOSColorDialog::updateColor(const QColor &color)
{
    m_currentColor = color;
}


