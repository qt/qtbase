// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef PRINTDIALOGPANEL_H
#define PRINTDIALOGPANEL_H

#ifndef QT_NO_PRINTER

#include "ui_printdialogpanel.h"

#include <QPageLayout>
#include <QPrinter>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QPrinter;
class QComboBox;
class QGroupBox;
class QPushButton;
class QCheckBox;
QT_END_NAMESPACE

class PageSizeControl;
class OptionsControl;

class PrintDialogPanel  : public QWidget
{
    Q_OBJECT
public:
    explicit PrintDialogPanel(QWidget *parent = nullptr);
    ~PrintDialogPanel();

private slots:
    void createPrinter();
    void deletePrinter();
    void showPrintDialog();
    void showPreviewDialog();
    void showPageSetupDialog();
    void directPrint();
    void unitsChanged();
    void pageSizeChanged();
    void pageDimensionsChanged();
    void orientationChanged();
    void marginsChanged();
    void layoutModeChanged();
    void printerChanged();

private:
    QSizeF customPageSize() const;
    void applySettings(QPrinter *printer) const;
    void retrieveSettings(const QPrinter *printer);
    void updatePageLayoutWidgets();
    void enablePanels();

    bool m_blockSignals;
    Ui::PrintDialogPanel m_panel;

    QPageLayout m_pageLayout;
    QScopedPointer<QPrinter> m_printer;
};

#endif // !QT_NO_PRINTER
#endif // PRINTDIALOGPANEL_H
