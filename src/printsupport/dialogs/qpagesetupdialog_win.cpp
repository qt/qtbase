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

#include "qpagesetupdialog.h"

#ifndef QT_NO_PRINTDIALOG
#include <qapplication.h>

#include "../kernel/qprintengine_win_p.h"
#include "qpagesetupdialog_p.h"
#include "qprinter.h"
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QPageSetupDialogPrivate(printer)), parent)
{
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    setAttribute(Qt::WA_DontShowOnScreen);
}

QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QDialog(*(new QPageSetupDialogPrivate(0)), parent)
{
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    setAttribute(Qt::WA_DontShowOnScreen);
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat)
        return Rejected;

    QWin32PrintEngine *engine = static_cast<QWin32PrintEngine*>(d->printer->paintEngine());
    QWin32PrintEnginePrivate *ep = static_cast<QWin32PrintEnginePrivate *>(engine->d_ptr.data());

    PAGESETUPDLG psd;
    memset(&psd, 0, sizeof(PAGESETUPDLG));
    psd.lStructSize = sizeof(PAGESETUPDLG);

    // we need a temp DEVMODE struct if we don't have a global DEVMODE
    HGLOBAL hDevMode = 0;
    int devModeSize = 0;
    if (!engine->globalDevMode()) {
        devModeSize = sizeof(DEVMODE) + ep->devMode->dmDriverExtra;
        hDevMode = GlobalAlloc(GHND, devModeSize);
        if (hDevMode) {
            void *dest = GlobalLock(hDevMode);
            memcpy(dest, ep->devMode, devModeSize);
            GlobalUnlock(hDevMode);
        }
        psd.hDevMode = hDevMode;
    } else {
        psd.hDevMode = engine->globalDevMode();
    }

    HGLOBAL *tempDevNames = engine->createGlobalDevNames();
    psd.hDevNames = tempDevNames;

    QWidget *parent = parentWidget();
    parent = parent ? parent->window() : QApplication::activeWindow();
    Q_ASSERT(!parent ||parent->testAttribute(Qt::WA_WState_Created));

    QWindow *parentWindow = parent ? parent->windowHandle() : 0;
    psd.hwndOwner = parentWindow ? (HWND)QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", parentWindow) : 0;

    psd.Flags = PSD_MARGINS;
    QPageLayout layout = d->printer->pageLayout();
    switch (layout.units()) {
    case QPageLayout::Millimeter:
    case QPageLayout::Inch:
        break;
    case QPageLayout::Point:
    case QPageLayout::Pica:
    case QPageLayout::Didot:
    case QPageLayout::Cicero:
        layout.setUnits(QLocale::system().measurementSystem() == QLocale::MetricSystem ? QPageLayout::Millimeter
                                                                                       : QPageLayout::Inch);
        break;
    }
    qreal multiplier = 1.0;
    if (layout.units() == QPageLayout::Millimeter) {
        psd.Flags |= PSD_INHUNDREDTHSOFMILLIMETERS;
        multiplier = 100.0;
    } else { // QPageLayout::Inch)
        psd.Flags |= PSD_INTHOUSANDTHSOFINCHES;
        multiplier = 1000.0;
    }
    psd.rtMargin.left   = layout.margins().left() * multiplier;
    psd.rtMargin.top    = layout.margins().top() * multiplier;
    psd.rtMargin.right  = layout.margins().right() * multiplier;
    psd.rtMargin.bottom = layout.margins().bottom() * multiplier;

    QDialog::setVisible(true);
    bool result = PageSetupDlg(&psd);
    QDialog::setVisible(false);
    if (result) {
        engine->setGlobalDevMode(psd.hDevNames, psd.hDevMode);
        const QMarginsF margins(psd.rtMargin.left, psd.rtMargin.top, psd.rtMargin.right, psd.rtMargin.bottom);
        d->printer->setPageMargins(margins / multiplier, layout.units());

        // copy from our temp DEVMODE struct
        if (!engine->globalDevMode() && hDevMode) {
            // Make sure memory is allocated
            if (ep->ownsDevMode && ep->devMode)
                free(ep->devMode);
            ep->devMode = (DEVMODE *) malloc(devModeSize);
            ep->ownsDevMode = true;

            // Copy
            void *src = GlobalLock(hDevMode);
            memcpy(ep->devMode, src, devModeSize);
            GlobalUnlock(hDevMode);
        }
    }

    if (!engine->globalDevMode() && hDevMode)
        GlobalFree(hDevMode);
    GlobalFree(tempDevNames);
    done(result);
    return result;
}

void QPageSetupDialog::setVisible(bool visible)
{
    if (!visible)
        return;
    exec();
}

QT_END_NAMESPACE
#endif
