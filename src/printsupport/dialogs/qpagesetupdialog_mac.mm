/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qpagesetupdialog.h"

#include <qhash.h>
#include <private/qapplication_p.h>
#include <private/qprintengine_mac_p.h>
#include <private/qabstractpagesetupdialog_p.h>

#ifndef QT_NO_PRINTDIALOG

QT_USE_NAMESPACE

@class QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate);

@interface QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) : NSObject {
    QMacPrintEnginePrivate *pe;
}
- (id)initWithMacPrintEngine:(QMacPrintEnginePrivate *)printEngine;
- (void)pageLayoutDidEnd:(NSPageLayout *)pageLayout
        returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate)
- (id)initWithMacPrintEngine:(QMacPrintEnginePrivate *)printEngine
{
    self = [super init];
    if (self) {
        pe = printEngine;
    }
    return self;

}
- (void)pageLayoutDidEnd:(NSPageLayout *)pageLayout
        returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    Q_UNUSED(pageLayout);
    QPageSetupDialog *dialog = static_cast<QPageSetupDialog *>(contextInfo);
    if (returnCode == NSOKButton) {
        PMRect paperRect;
        PMGetUnadjustedPaperRect(pe->format, &paperRect);
        pe->customSize = QSizeF(paperRect.right - paperRect.left,
                                paperRect.bottom - paperRect.top);
    }
    dialog->done((returnCode == NSOKButton) ? QDialog::Accepted : QDialog::Rejected);
}
@end

QT_BEGIN_NAMESPACE

extern void macStartInterceptWindowTitle(QWidget *window);
extern void macStopInterceptWindowTitle();

class QPageSetupDialogPrivate : public QAbstractPageSetupDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    QPageSetupDialogPrivate() : ep(0)
    ,pageLayout(0)
    {}

    ~QPageSetupDialogPrivate() {
    }

    void openCocoaPageLayout(Qt::WindowModality modality);
    void closeCocoaPageLayout();

    QMacPrintEnginePrivate *ep;
    NSPageLayout *pageLayout;
};

void QPageSetupDialogPrivate::openCocoaPageLayout(Qt::WindowModality modality)
{
    Q_Q(QPageSetupDialog);

    // If someone is reusing a QPrinter object, the end released all our old
    // information. In this case, we must reinitialize.
    if (ep->session == 0)
        ep->initialize();

    macStartInterceptWindowTitle(q);
    pageLayout = [NSPageLayout pageLayout];
    // Keep a copy to this since we plan on using it for a bit.
    [pageLayout retain];
    QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) *delegate = [[QT_MANGLE_NAMESPACE(QCocoaPageLayoutDelegate) alloc] initWithMacPrintEngine:ep];

    if (modality == Qt::ApplicationModal) {
        int rval = [pageLayout runModalWithPrintInfo:ep->printInfo];
        [delegate pageLayoutDidEnd:pageLayout returnCode:rval contextInfo:q];
    } else {
        Q_ASSERT(q->parentWidget());
        [pageLayout beginSheetWithPrintInfo:ep->printInfo
                             modalForWindow:qt_mac_window_for(q->parentWidget())
                                   delegate:delegate
                             didEndSelector:@selector(pageLayoutDidEnd:returnCode:contextInfo:)
                                contextInfo:q];
    }

    macStopInterceptWindowTitle();
}

void QPageSetupDialogPrivate::closeCocoaPageLayout()
{
    // NSPageLayout can change the session behind our back and then our
    // d->ep->session object will become a dangling pointer. Update it
    // based on the "current" session
    ep->session = static_cast<PMPrintSession>([ep->printInfo PMPrintSession]);

    [pageLayout release];
    pageLayout = 0;
}

QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), printer, parent)
{
    Q_D(QPageSetupDialog);
    d->ep = static_cast<QMacPrintEngine *>(d->printer->paintEngine())->d_func();
}

QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), 0, parent)
{
    Q_D(QPageSetupDialog);
    d->ep = static_cast<QMacPrintEngine *>(d->printer->paintEngine())->d_func();
}

void QPageSetupDialog::setVisible(bool visible)
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return;

    bool isCurrentlyVisible = (d->pageLayout != 0);
    if (!visible == !isCurrentlyVisible)
        return;

    if (visible) {
        d->openCocoaPageLayout(parentWidget() ? Qt::WindowModal
                                              : Qt::ApplicationModal);
        return;
    } else {
        if (d->pageLayout) {
            d->closeCocoaPageLayout();
            return;
        }
    }
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return Rejected;

    QMacCocoaAutoReleasePool pool;
    d->openCocoaPageLayout(Qt::ApplicationModal);
    d->closeCocoaPageLayout();
    return result();
}

QT_END_NAMESPACE

#endif /* QT_NO_PRINTDIALOG */
