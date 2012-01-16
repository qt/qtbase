/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_NO_PRINTDIALOG

#include <private/qt_mac_p.h>

#include <qhash.h>
#include <qprintdialog.h>
#include <private/qapplication_p.h>
#include <private/qabstractprintdialog_p.h>
#include <private/qprintengine_mac_p.h>

QT_BEGIN_NAMESPACE

class QPrintDialogPrivate : public QAbstractPrintDialogPrivate
{
    Q_DECLARE_PUBLIC(QPrintDialog)

public:
    QPrintDialogPrivate() : ep(0), printPanel(0)
       {}

    void openCocoaPrintPanel(Qt::WindowModality modality);
    void closeCocoaPrintPanel();
    void initBeforeRun();

    inline QPrintDialog *printDialog() { return q_func(); }

    inline void _q_printToFileChanged(int) {}
    inline void _q_rbPrintRangeToggled(bool) {}
    inline void _q_printerChanged(int) {}
#ifndef QT_NO_MESSAGEBOX
    inline void _q_checkFields() {}
#endif
    inline void _q_chbPrintLastFirstToggled(bool) {}
    inline void _q_paperSizeChanged(int) {}
    inline void _q_btnBrowseClicked() {}
    inline void _q_btnPropertiesClicked() {}

    QMacPrintEnginePrivate *ep;
    NSPrintPanel *printPanel;
};

QT_END_NAMESPACE

QT_USE_NAMESPACE


@class QT_MANGLE_NAMESPACE(QCocoaPrintPanelDelegate);

@interface QT_MANGLE_NAMESPACE(QCocoaPrintPanelDelegate) : NSObject {
}
- (void)printPanelDidEnd:(NSPrintPanel *)printPanel
        returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation QT_MANGLE_NAMESPACE(QCocoaPrintPanelDelegate)
- (void)printPanelDidEnd:(NSPrintPanel *)printPanel
        returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    Q_UNUSED(printPanel);

    QPrintDialogPrivate *d = static_cast<QPrintDialogPrivate *>(contextInfo);
    QPrintDialog *dialog = d->printDialog();

    if (returnCode == NSOKButton) {
        UInt32 frompage, topage;
        PMGetFirstPage(d->ep->settings, &frompage);
        PMGetLastPage(d->ep->settings, &topage);
        topage = qMin(UInt32(INT_MAX), topage);
        dialog->setFromTo(frompage, topage);

        // OK, I need to map these values back let's see
        // If from is 1 and to is INT_MAX, then print it all
        // (Apologies to the folks with more than INT_MAX pages)
        if (dialog->fromPage() == 1 && dialog->toPage() == INT_MAX) {
            dialog->setPrintRange(QPrintDialog::AllPages);
            dialog->setFromTo(0, 0);
        } else {
            dialog->setPrintRange(QPrintDialog::PageRange); // In a way a lie, but it shouldn't hurt.
            // Carbon hands us back a very large number here even for ALL, set it to max
            // in that case to follow the behavior of the other print dialogs.
            if (dialog->maxPage() < dialog->toPage())
                dialog->setFromTo(dialog->fromPage(), dialog->maxPage());
        }
        // Keep us in sync with file output
        PMDestinationType dest;

        // If the user selected print to file, the session has been
        // changed behind our back and our d->ep->session object is a
        // dangling pointer. Update it based on the "current" session
        d->ep->session = static_cast<PMPrintSession>([d->ep->printInfo PMPrintSession]);

        PMSessionGetDestinationType(d->ep->session, d->ep->settings, &dest);
        if (dest == kPMDestinationFile) {
            QCFType<CFURLRef> file;
            PMSessionCopyDestinationLocation(d->ep->session, d->ep->settings, &file);
            UInt8 localFile[2048];  // Assuming there's a POSIX file system here.
            CFURLGetFileSystemRepresentation(file, true, localFile, sizeof(localFile));
            d->ep->outputFilename
                = QString::fromUtf8(reinterpret_cast<const char *>(localFile));
        } else {
            // Keep output format.
            QPrinter::OutputFormat format;
            format = d->printer->outputFormat();
            d->printer->setOutputFileName(QString());
            d->printer->setOutputFormat(format);
        }
    }

    dialog->done((returnCode == NSOKButton) ? QDialog::Accepted : QDialog::Rejected);
}
@end


QT_BEGIN_NAMESPACE

extern void macStartInterceptWindowTitle(QWidget *window);
extern void macStopInterceptWindowTitle();


void QPrintDialogPrivate::initBeforeRun()
{
    Q_Q(QPrintDialog);
    // If someone is reusing a QPrinter object, the end released all our old
    // information. In this case, we must reinitialize.
    if (ep->session == 0)
        ep->initialize();


    // It seems the only way that PM lets you use all is if the minimum
    // for the page range is 1. This _kind of_ makes sense if you think about
    // it. However, calling PMSetFirstPage() or PMSetLastPage() always enforces
    // the range.
    PMSetPageRange(ep->settings, q->minPage(), q->maxPage());
    if (q->printRange() == QAbstractPrintDialog::PageRange) {
        PMSetFirstPage(ep->settings, q->fromPage(), false);
        PMSetLastPage(ep->settings, q->toPage(), false);
    }
}

void QPrintDialogPrivate::openCocoaPrintPanel(Qt::WindowModality modality)
{
    Q_Q(QPrintDialog);

    initBeforeRun();

    QPrintDialog::PrintDialogOptions qtOptions = q->options();
    NSPrintPanelOptions macOptions = NSPrintPanelShowsCopies;
    if (qtOptions & QPrintDialog::PrintPageRange)
        macOptions |= NSPrintPanelShowsPageRange;
    if (qtOptions & QPrintDialog::PrintShowPageSize)
        macOptions |= NSPrintPanelShowsPaperSize | NSPrintPanelShowsPageSetupAccessory
                      | NSPrintPanelShowsOrientation;

    macStartInterceptWindowTitle(q);
    printPanel = [NSPrintPanel printPanel];
    QT_MANGLE_NAMESPACE(QCocoaPrintPanelDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QCocoaPrintPanelDelegate) alloc] init];
    [printPanel setOptions:macOptions];

    if (modality == Qt::ApplicationModal) {
        int rval = [printPanel runModalWithPrintInfo:ep->printInfo];
        [delegate printPanelDidEnd:printPanel returnCode:rval contextInfo:this];
    } else {
        Q_ASSERT(q->parentWidget());
        NSWindow *windowRef = qt_mac_window_for(q->parentWidget());
        [printPanel beginSheetWithPrintInfo:ep->printInfo
                             modalForWindow:windowRef
                                   delegate:delegate
                             didEndSelector:@selector(printPanelDidEnd:returnCode:contextInfo:)
                                contextInfo:this];
    }

    macStopInterceptWindowTitle();
}

void QPrintDialogPrivate::closeCocoaPrintPanel()
{
    // ###
}

static bool warnIfNotNative(QPrinter *printer)
{
    if (printer->outputFormat() != QPrinter::NativeFormat) {
        qWarning("QPrintDialog: Cannot be used on non-native printers");
        return false;
    }
    return true;
}


QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), printer, parent)
{
    Q_D(QPrintDialog);
    if (!warnIfNotNative(d->printer))
        return;
    d->ep = static_cast<QMacPrintEngine *>(d->printer->paintEngine())->d_func();
}

QPrintDialog::QPrintDialog(QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), 0, parent)
{
    Q_D(QPrintDialog);
    if (!warnIfNotNative(d->printer))
        return;
    d->ep = static_cast<QMacPrintEngine *>(d->printer->paintEngine())->d_func();
}

QPrintDialog::~QPrintDialog()
{
}

int QPrintDialog::exec()
{
    Q_D(QPrintDialog);
    if (!warnIfNotNative(d->printer))
        return QDialog::Rejected;

    QMacCocoaAutoReleasePool pool;

    d->openCocoaPrintPanel(Qt::ApplicationModal);
    d->closeCocoaPrintPanel();
    return result();
}


/*!
    \reimp
*/
void QPrintDialog::setVisible(bool visible)
{
    Q_D(QPrintDialog);

    bool isCurrentlyVisible = (d->printPanel != 0);

    if (!visible == !isCurrentlyVisible)
        return;

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return;

    if (visible) {
        d->openCocoaPrintPanel(parentWidget() ? Qt::WindowModal
                                              : Qt::ApplicationModal);
        return;
    } else {
        if (d->printPanel) {
            d->closeCocoaPrintPanel();
            return;
        }
    }
}

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"

#endif // QT_NO_PRINTDIALOG
