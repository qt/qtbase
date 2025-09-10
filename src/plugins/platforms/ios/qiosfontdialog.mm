// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#import <UIKit/UIKit.h>

#include <QtGui/qwindow.h>
#include <QtGui/qfontdatabase.h>
#include <QDebug>

#include <QtCore/private/qcore_mac_p.h>
#include <QtGui/private/qfont_p.h>
#include <QtGui/private/qfontengine_p.h>

#include "qiosglobal.h"
#include "qiosfontdialog.h"
#include "qiosintegration.h"

@implementation QIOSFontDialogController {
    QIOSFontDialog *m_fontDialog;
}

- (instancetype)initWithQIOSFontDialog:(QIOSFontDialog *)dialog
{
    UIFontPickerViewControllerConfiguration *configuration = [[UIFontPickerViewControllerConfiguration alloc] init];
    if (dialog->options()->testOption(QFontDialogOptions::MonospacedFonts)) {
        UIFontDescriptorSymbolicTraits traits = {};
        traits |= UIFontDescriptorTraitMonoSpace;
        configuration.filteredTraits = traits;
    }
    configuration.includeFaces = YES;
    if (self = [super initWithConfiguration:configuration]) {
        m_fontDialog = dialog;
        self.delegate = self;
        self.presentationController.delegate = self;
    }
    [configuration release];
    return self;
}

- (void)setQFont:(const QFont &)font
{
    QFontInfo fontInfo(font);
    auto family = fontInfo.family().toNSString();
    auto size = fontInfo.pointSize();

    NSDictionary *dictionary = @{
        static_cast<NSString *>(UIFontDescriptorFamilyAttribute): family,
        static_cast<NSString *>(UIFontDescriptorSizeAttribute): [NSNumber numberWithInt:size]
    };
    UIFontDescriptor *fd = [UIFontDescriptor fontDescriptorWithFontAttributes:dictionary];

    UIFontDescriptorSymbolicTraits traits = 0;
    if (font.style() == QFont::StyleItalic)
        traits |= UIFontDescriptorTraitItalic;
    if (font.weight() == QFont::Bold)
        traits |= UIFontDescriptorTraitBold;
    fd = [fd fontDescriptorWithSymbolicTraits:traits];

    self.selectedFontDescriptor = fd;
}

- (void)updateQFont
{
    UIFontDescriptor *font = self.selectedFontDescriptor;
    if (!font)
        return;

    NSDictionary *attributes = font.fontAttributes;
    UIFontDescriptorSymbolicTraits traits = font.symbolicTraits;

    QFont newFont;
    int size = qRound(font.pointSize);
    QString family = QString::fromNSString([attributes objectForKey:UIFontDescriptorFamilyAttribute]);
    if (family.isEmpty()) {
        // If includeFaces is true, then the font descriptor won't
        // have the UIFontDescriptorFamilyAttribute key set so we
        // need to create a UIFont to get the font family
        UIFont *f = [UIFont fontWithDescriptor:font size:size];
        family = QString::fromNSString(f.familyName);
    }

    QString style;
    if ((traits & (UIFontDescriptorTraitItalic | UIFontDescriptorTraitBold)) == (UIFontDescriptorTraitItalic | UIFontDescriptorTraitBold))
        style = "Bold Italic";
    else if (traits & UIFontDescriptorTraitItalic)
        style = "Italic";
    else if (traits & UIFontDescriptorTraitBold)
        style = "Bold";

    newFont = QFontDatabase::font(family, style, size);

    if (m_fontDialog) {
        m_fontDialog->updateCurrentFont(newFont);
        emit m_fontDialog->currentFontChanged(newFont);
    }
}

// ----------------------UIFontPickerViewControllerDelegate--------------------------
- (void)fontPickerViewControllerDidPickFont:(UIFontPickerViewController *)viewController
{
    [self updateQFont];
    emit m_fontDialog->accept();
}

- (void)fontPickerViewControllerDidCancel:(UIFontPickerViewController *)viewController
{
    Q_UNUSED(viewController);
    emit m_fontDialog->reject();
}

// ----------------------UIAdaptivePresentationControllerDelegate--------------------------
- (void)presentationControllerDidDismiss:(UIPresentationController *)presentationController
{
    Q_UNUSED(presentationController);
    emit m_fontDialog->reject();
}

@end

QIOSFontDialog::QIOSFontDialog()
    : m_viewController(nullptr)
{
}

QIOSFontDialog::~QIOSFontDialog()
{
    hide();
}

void QIOSFontDialog::exec()
{
    m_eventLoop.exec(QEventLoop::DialogExec);
}

bool QIOSFontDialog::show(Qt::WindowFlags windowFlags, Qt::WindowModality windowModality, QWindow *parent)
{
    Q_UNUSED(windowFlags);
    Q_UNUSED(windowModality);

    if (!m_viewController) {
        m_viewController = [[QIOSFontDialogController alloc] initWithQIOSFontDialog:this];
        [m_viewController setQFont:m_currentFont];
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

void QIOSFontDialog::hide()
{
    [m_viewController dismissViewControllerAnimated:YES completion:nil];
    [m_viewController release];
    m_viewController = nullptr;
    if (m_eventLoop.isRunning())
        m_eventLoop.exit();
}

void QIOSFontDialog::setCurrentFont(const QFont &font)
{
    if (m_currentFont == font)
        return;

    m_currentFont = font;
    if (m_viewController)
        [m_viewController setQFont:font];
}

QFont QIOSFontDialog::currentFont() const
{
    return m_currentFont;
}

void QIOSFontDialog::updateCurrentFont(const QFont &font)
{
    m_currentFont = font;
}


