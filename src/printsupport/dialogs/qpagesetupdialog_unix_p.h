/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef QPAGESETUPDIALOG_UNIX_P_H
#define QPAGESETUPDIALOG_UNIX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#include "qprinter.h"
#include "kernel/qprint_p.h"

#include <QtGui/qpagelayout.h>

#include <ui_qpagesetupwidget.h>

QT_REQUIRE_CONFIG(printdialog);

QT_BEGIN_NAMESPACE

class QPrinter;
class QPrintDevice;
class QPagePreview;

class QPageSetupWidget : public QWidget {
    Q_OBJECT
public:
    explicit QPageSetupWidget(QWidget *parent = nullptr);

    void setPrinter(QPrinter *printer, QPrintDevice *printDevice,
                    QPrinter::OutputFormat outputFormat, const QString &printerName);
    void setupPrinter() const;
    void updateSavedValues();
    void revertToSavedValues();

#if QT_CONFIG(cups)
    bool hasPpdConflict() const;

signals:
    void ppdOptionChanged();
#endif

private slots:
    void pageSizeChanged();
    void pageOrientationChanged();
    void pagesPerSheetChanged();
    void unitChanged();
    void topMarginChanged(double newValue);
    void bottomMarginChanged(double newValue);
    void leftMarginChanged(double newValue);
    void rightMarginChanged(double newValue);

private:
    friend class QUnixPrintWidgetPrivate;  // Needed by checkFields()

    void updateWidget();
    void initUnits();
    void initPagesPerSheet();
    void initPageSizes();

    Ui::QPageSetupWidget m_ui;
    QPagePreview *m_pagePreview;
    QPrinter *m_printer;
    QPrintDevice *m_printDevice;
#if QT_CONFIG(cups)
    ppd_option_t *m_pageSizePpdOption;
#endif
    QPrinter::OutputFormat m_outputFormat;
    QString m_printerName;
    QPageLayout m_pageLayout;
    QPageLayout m_savedPageLayout;
    QPageLayout::Unit m_units;
    QPageLayout::Unit m_savedUnits;
    int m_savedPagesPerSheet;
    int m_savedPagesPerSheetLayout;
    bool m_blockSignals;
    int m_realCustomPageSizeIndex;
};

QT_END_NAMESPACE

#endif
