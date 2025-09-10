// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include <AppKit/AppKit.h>

#include "qpagesetupdialog.h"

#include "qpagesetupdialog_p.h"

#include <qpa/qplatformnativeinterface.h>
#include <QtPrintSupport/qprintengine.h>

#include <QtPrintSupport/private/qprintengine_mac_p.h>

#include <QtCore/private/qcore_mac_p.h>

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate);

@interface QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) : NSObject
@end

@implementation QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) {
    NSPrintInfo *printInfo;
}

- (instancetype)initWithNSPrintInfo:(NSPrintInfo *)nsPrintInfo
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

    if (returnCode == NSModalResponseOK) {
        PMPageFormat format = static_cast<PMPageFormat>([printInfo PMPageFormat]);
        PMRect paperRect;
        PMGetUnadjustedPaperRect(format, &paperRect);
        PMOrientation orientation;
        PMGetOrientation(format, &orientation);
        QSizeF paperSize = QSizeF(paperRect.right - paperRect.left, paperRect.bottom - paperRect.top);
        printer->printEngine()->setProperty(QPrintEngine::PPK_CustomPaperSize, paperSize);
        printer->printEngine()->setProperty(QPrintEngine::PPK_Orientation, orientation == kPMLandscape ? QPageLayout::Landscape : QPageLayout::Portrait);
    }

    dialog->done((returnCode == NSModalResponseOK) ? QDialog::Accepted : QDialog::Rejected);
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

    printInfo = static_cast<QMacPrintEngine *>(printer->printEngine())->printInfo();
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
