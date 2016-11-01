/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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


#include <AppKit/AppKit.h>

#include "qpagesetupdialog.h"

#ifndef QT_NO_PRINTDIALOG
#include "qpagesetupdialog_p.h"

#include <qpa/qplatformnativeinterface.h>
#include <QtPrintSupport/qprintengine.h>

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate);

@interface QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) : NSObject
{
    NSPrintInfo *printInfo;
}
- (id)initWithNSPrintInfo:(NSPrintInfo *)nsPrintInfo;
- (void)pageLayoutDidEnd:(NSPageLayout *)pageLayout
        returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate)
- (id)initWithNSPrintInfo:(NSPrintInfo *)nsPrintInfo
{
    self = [super init];
    if (self) {
        printInfo = nsPrintInfo;
    }
    return self;

}
- (void)pageLayoutDidEnd:(NSPageLayout *)pageLayout
        returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    Q_UNUSED(pageLayout);
    QPageSetupDialog *dialog = static_cast<QPageSetupDialog *>(contextInfo);
    QPrinter *printer = dialog->printer();

    if (returnCode == NSOKButton) {
        PMPageFormat format = static_cast<PMPageFormat>([printInfo PMPageFormat]);
        PMRect paperRect;
        PMGetUnadjustedPaperRect(format, &paperRect);
        PMOrientation orientation;
        PMGetOrientation(format, &orientation);
        QSizeF paperSize = QSizeF(paperRect.right - paperRect.left, paperRect.bottom - paperRect.top);
        printer->printEngine()->setProperty(QPrintEngine::PPK_CustomPaperSize, paperSize);
        printer->printEngine()->setProperty(QPrintEngine::PPK_Orientation, orientation == kPMLandscape ? QPrinter::Landscape : QPrinter::Portrait);
    }

    dialog->done((returnCode == NSOKButton) ? QDialog::Accepted : QDialog::Rejected);
}
@end

QT_BEGIN_NAMESPACE

class QMacPageSetupDialogPrivate : public QPageSetupDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    QMacPageSetupDialogPrivate(QPrinter *printer)
        :  QPageSetupDialogPrivate(printer), printInfo(0), pageLayout(0)
    { }

    ~QMacPageSetupDialogPrivate() {
    }

    void openCocoaPageLayout(Qt::WindowModality modality);
    void closeCocoaPageLayout();

    NSPrintInfo *printInfo;
    NSPageLayout *pageLayout;
};

void QMacPageSetupDialogPrivate::openCocoaPageLayout(Qt::WindowModality modality)
{
    Q_Q(QPageSetupDialog);

    // get the NSPrintInfo from the print engine in the platform plugin
    void *voidp = 0;
    (void) QMetaObject::invokeMethod(qApp->platformNativeInterface(),
                                     "NSPrintInfoForPrintEngine",
                                     Q_RETURN_ARG(void *, voidp),
                                     Q_ARG(QPrintEngine *, printer->printEngine()));
    printInfo = static_cast<NSPrintInfo *>(voidp);
    [printInfo retain];

    pageLayout = [NSPageLayout pageLayout];
    // Keep a copy to this since we plan on using it for a bit.
    [pageLayout retain];
    QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) alloc] initWithNSPrintInfo:printInfo];

    if (modality == Qt::ApplicationModal) {

        // Make sure we don't interrupt the runModalWithPrintInfo call.
        (void) QMetaObject::invokeMethod(qApp->platformNativeInterface(),
                                         "clearCurrentThreadCocoaEventDispatcherInterruptFlag");

        int rval = [pageLayout runModalWithPrintInfo:printInfo];
        [delegate pageLayoutDidEnd:pageLayout returnCode:rval contextInfo:q];
    } else {
        Q_ASSERT(q->parentWidget());
        QWindow *parentWindow = q->parentWidget()->windowHandle();
        NSWindow *window = static_cast<NSWindow *>(qApp->platformNativeInterface()->nativeResourceForWindow("nswindow", parentWindow));
        [pageLayout beginSheetWithPrintInfo:printInfo
                             modalForWindow:window
                                   delegate:delegate
                             didEndSelector:@selector(pageLayoutDidEnd:returnCode:contextInfo:)
                                contextInfo:q];
    }
}

void QMacPageSetupDialogPrivate::closeCocoaPageLayout()
{
    [printInfo release];
    printInfo = 0;
    [pageLayout release];
    pageLayout = 0;
}

QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QMacPageSetupDialogPrivate(printer)), parent)
{
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    setAttribute(Qt::WA_DontShowOnScreen);
}

QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QDialog(*(new QMacPageSetupDialogPrivate(0)), parent)
{
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    setAttribute(Qt::WA_DontShowOnScreen);
}

void QPageSetupDialog::setVisible(bool visible)
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return;

    bool isCurrentlyVisible = (static_cast <QMacPageSetupDialogPrivate*>(d)->pageLayout != 0);
    if (!visible == !isCurrentlyVisible)
        return;

    QDialog::setVisible(visible);

    if (visible) {
        Qt::WindowModality modality = windowModality();
        if (modality == Qt::NonModal) {
            // NSPrintPanels can only be modal, so we must pick a type
            modality = parentWidget() ? Qt::WindowModal : Qt::ApplicationModal;
        }
        static_cast <QMacPageSetupDialogPrivate*>(d)->openCocoaPageLayout(modality);
        return;
    } else {
        if (static_cast <QMacPageSetupDialogPrivate*>(d)->pageLayout) {
            static_cast <QMacPageSetupDialogPrivate*>(d)->closeCocoaPageLayout();
            return;
        }
    }
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return Rejected;

    QDialog::setVisible(true);

    QMacAutoReleasePool pool;
    static_cast <QMacPageSetupDialogPrivate*>(d)->openCocoaPageLayout(Qt::ApplicationModal);
    static_cast <QMacPageSetupDialogPrivate*>(d)->closeCocoaPageLayout();

    QDialog::setVisible(false);

    return result();
}

QT_END_NAMESPACE

#endif /* QT_NO_PRINTDIALOG */
