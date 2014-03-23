/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QT_NO_PRINTER

#include "printdialogpanel.h"
#include "utils.h"

#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QApplication>
#include <QDesktopWidget>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDoubleSpinBox>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
#include <QDebug>
#include <QTextStream>

const FlagData printerModeComboData[] =
{
    {"ScreenResolution", QPrinter::ScreenResolution},
    {"PrinterResolution", QPrinter::PrinterResolution},
    {"HighResolution", QPrinter::HighResolution}
};

const FlagData orientationComboData[] =
{
    {"Portrait", QPrinter::Portrait},
    {"Landscape", QPrinter::Landscape},
};

const FlagData pageSizeComboData[] =
{
    {"A4", QPrinter::A4},
    {"B5", QPrinter::B5},
    {"Letter", QPrinter::Letter},
    {"Legal", QPrinter::Legal},
    {"Executive", QPrinter::Executive},
    {"A0", QPrinter::A0},
    {"A1", QPrinter::A1},
    {"A2", QPrinter::A2},
    {"A3", QPrinter::A3},
    {"A5", QPrinter::A5},
    {"A6", QPrinter::A6},
    {"A7", QPrinter::A7},
    {"A8", QPrinter::A8},
    {"A9", QPrinter::A9},
    {"B0", QPrinter::B0},
    {"B1", QPrinter::B1},
    {"B10", QPrinter::B10},
    {"B2", QPrinter::B2},
    {"B3", QPrinter::B3},
    {"B4", QPrinter::B4},
    {"B6", QPrinter::B6},
    {"B7", QPrinter::B7},
    {"B8", QPrinter::B8},
    {"B9", QPrinter::B9},
    {"C5E", QPrinter::C5E},
    {"Comm10E", QPrinter::Comm10E},
    {"DLE", QPrinter::DLE},
    {"Folio", QPrinter::Folio},
    {"Ledger", QPrinter::Ledger},
    {"Tabloid", QPrinter::Tabloid},
    {"Custom", QPrinter::Custom}
};

const FlagData printDialogOptions[] =
{
    {"PrintToFile", QPrintDialog::PrintToFile},
    {"PrintSelection", QPrintDialog::PrintSelection},
    {"PrintPageRange", QPrintDialog::PrintPageRange},
    {"PrintShowPageSize", QPrintDialog::PrintShowPageSize},
    {"PrintCollateCopies", QPrintDialog::PrintCollateCopies},
    {"PrintCurrentPage", QPrintDialog::PrintCurrentPage}
};

const FlagData printRangeOptions[] =
{
    {"AllPages", QPrintDialog::AllPages},
    {"Selection", QPrintDialog::Selection},
    {"PageRange", QPrintDialog::PageRange},
    {"CurrentPage", QPrintDialog::CurrentPage}
};

QTextStream &operator<<(QTextStream &s, const QSizeF &size)
{
    s << size.width() << 'x' << size.height();
    return s;
}

QTextStream &operator<<(QTextStream &s, const QRectF &rect)
{
    s << rect.width() << 'x' << rect.height() << forcesign << rect.x() << rect.y() << noforcesign;
    return s;
}

QTextStream &operator<<(QTextStream &s, const QPrinter &printer)
{
    s << '"' << printer.printerName() << "\"\nPaper #" <<printer.paperSize()
#if QT_VERSION >= 0x050000
        << " \"" << printer.paperName() << '"'
#endif
      << (printer.orientation() == QPrinter::Portrait ? ", Portrait" : ", Landscape");
    if (printer.fullPage())
        s << ", full page";
    s << "\nPaper size: "
        << printer.paperSize(QPrinter::Point) << "pt "
        << printer.paperSize(QPrinter::Millimeter) << "mm "
      << "\n            " << printer.paperSize(QPrinter::DevicePixel) << "device pt "
        << printer.paperSize(QPrinter::Inch) << "inch "
#if QT_VERSION >= 0x050000
      << "\nPagedPaintDevSize: " <<   printer.pageSizeMM() << "mm"
#endif
      << "\nLogical resolution : " << printer.logicalDpiX() << ',' << printer.logicalDpiY() << "DPI"
      << "\nPhysical resolution: " << printer.physicalDpiX() << ',' << printer.physicalDpiY() << "DPI"
      << "\nPaperRect: " << printer.paperRect(QPrinter::Point) << "pt "
        << printer.paperRect(QPrinter::Millimeter) << "mm "
      << "\n           " << printer.paperRect(QPrinter::DevicePixel) << "device pt"
      << "\nPageRect:  " << printer.pageRect(QPrinter::Point) << "pt "
        << printer.pageRect(QPrinter::Millimeter) << "mm "
      << "\n           " << printer.pageRect(QPrinter::DevicePixel) << "device pt";
    return s;
}

// Print a page with a rectangular frame, vertical / horizontal rulers in cm and printer info.

static void drawHorizCmRuler(QPainter &painter, int x1, int x2, int y)
{
    painter.drawLine(x1, y, x2, y);
    const int dpI = painter.device()->logicalDpiX();
    const int dpCm = qRound(double(dpI) / 2.54);
    const int h = dpCm / 2;
    const QFontMetrics fm(painter.font());
    for (int cm = 0, x = x1; x < x2; x += dpCm, ++cm) {
        painter.drawLine(x, y, x, y - h);
        if (cm) {
            const QString n = QString::number(cm);
            const QRect br = fm.boundingRect(n);
            painter.drawText(x - br.width() / 2, y - h - 10, n);
        }
    }
}

static void drawVertCmRuler(QPainter &painter, int x, int y1, int y2)
{
    painter.drawLine(x, y1, x, y2);
    const int dpI = painter.device()->logicalDpiY();
    const int dpCm = qRound(double(dpI) / 2.54);
    const int h = dpCm / 2;
    const QFontMetrics fm(painter.font());
    for (int cm = 0, y = y1; y < y2; y += dpCm, ++cm) {
        painter.drawLine(x, y, x + h, y);
        if (cm) {
            const QString n = QString::number(cm);
            const QRect br = fm.boundingRect(n);
            painter.drawText(x + h + 10, y + br.height() / 2, n);
        }
    }
}

static void print(QPrinter *printer)
{
    QPainter painter(printer);
    const QRectF pageF = printer->pageRect();

    QFont font = painter.font();
    font.setFamily("Courier");
    font.setPointSize(10);

    // Format message.
    const int charHeight = QFontMetrics(font).boundingRect('X').height();
    QString msg;
    QTextStream str(&msg);
    str << "Qt "<< QT_VERSION_STR;
#if QT_VERSION >= 0x050000
    str << ' ' << QGuiApplication::platformName();
#endif
    str << ' ' << QDateTime::currentDateTime().toString()
        << "\nFont: " << font.family() << ' ' << font.pointSize() << '\n'
        << *printer;

    if (!painter.device()->logicalDpiY() || !painter.device()->logicalDpiX()) {
        qWarning() << Q_FUNC_INFO << "Bailing out due to invalid DPI: " << msg;
        return;
    }

    painter.drawRect(pageF);

    drawHorizCmRuler(painter, pageF.x(), pageF.right(), pageF.height() /2);
    drawVertCmRuler(painter, pageF.x() + pageF.width() / 2, pageF.top(), pageF.bottom());

    painter.setFont(font);
    QPointF textPoint = pageF.topLeft() + QPoint(10, charHeight + 10);
    foreach (const QString &line, msg.split('\n')) {
        painter.drawText(textPoint, line);
        textPoint.ry() += (15 * charHeight) / 10;
    }

    painter.end();
}

class PrintPreviewDialog : public QPrintPreviewDialog {
    Q_OBJECT
public:
    explicit PrintPreviewDialog(QPrinter *printer, QWidget *parent = 0) : QPrintPreviewDialog(printer, parent)
    {
        connect(this, SIGNAL(paintRequested(QPrinter*)), this, SLOT(slotPaintRequested(QPrinter*)));
    }

public slots:
    void slotPaintRequested(QPrinter *p) { print(p); }
};

PrintDialogPanel::PrintDialogPanel(QWidget *parent)
    : QWidget(parent)
{
    m_panel.setupUi(this);

    // Setup the Create box
    populateCombo(m_panel.m_printerModeCombo, printerModeComboData, sizeof(printerModeComboData)/sizeof(FlagData));
    connect(m_panel.m_createButton, SIGNAL(clicked()), this, SLOT(createPrinter()));
    connect(m_panel.m_deleteButton, SIGNAL(clicked()), this, SLOT(deletePrinter()));

    // Setup the Settings box
    populateCombo(m_panel.m_pageSizeCombo, pageSizeComboData, sizeof(pageSizeComboData)/sizeof(FlagData));
    connect(m_panel.m_pageSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(enableCustomSizeControl()));
    populateCombo(m_panel.m_orientationCombo, orientationComboData, sizeof(orientationComboData)/sizeof(FlagData));

    // Setup the Dialogs box
    m_panel.m_dialogOptionsGroupBox->populateOptions(printDialogOptions, sizeof(printDialogOptions) / sizeof(FlagData));
    populateCombo(m_panel.m_printDialogRangeCombo, printRangeOptions, sizeof(printRangeOptions) / sizeof(FlagData));
    QPrintDialog dialog;
    m_panel.m_dialogOptionsGroupBox->setValue(dialog.options());
    m_panel.m_printDialogRangeCombo->setCurrentIndex(dialog.printRange());
    connect(m_panel.m_printButton, SIGNAL(clicked()), this, SLOT(showPrintDialog()));
    connect(m_panel.m_printPreviewButton, SIGNAL(clicked()), this, SLOT(showPreviewDialog()));
    connect(m_panel.m_pageSetupButton, SIGNAL(clicked()), this, SLOT(showPageSetupDialog()));

    enablePanels();
}

PrintDialogPanel::~PrintDialogPanel()
{
}

void PrintDialogPanel::enablePanels()
{
    const bool exists = !m_printer.isNull();
    m_panel.m_createButton->setEnabled(!exists);
    m_panel.m_printerModeCombo->setEnabled(!exists);
    m_panel.m_deleteButton->setEnabled(exists);
    m_panel.m_settingsGroupBox->setEnabled(exists);
    m_panel.m_dialogsGroupBox->setEnabled(exists);
}

void PrintDialogPanel::createPrinter()
{
    const QPrinter::PrinterMode mode = comboBoxValue<QPrinter::PrinterMode>(m_panel.m_printerModeCombo);
    m_printer.reset(new QPrinter(mode)); // Can set only once.
    retrieveSettings(m_printer.data());
    enablePanels();
    enableCustomSizeControl();
}

void PrintDialogPanel::deletePrinter()
{
    m_printer.reset();
    enablePanels();
}

QSizeF PrintDialogPanel::pageSize() const
{
    return QSizeF(m_panel.m_pageWidth->value(), m_panel.m_pageHeight->value());
}

void PrintDialogPanel::setPageSize(const QSizeF &sizef)
{
    m_panel.m_pageWidth->setValue(sizef.width());
    m_panel.m_pageHeight->setValue(sizef.height());
}

void PrintDialogPanel::applySettings(QPrinter *printer) const
{
    const QPrinter::PageSize pageSizeId = comboBoxValue<QPrinter::PageSize>(m_panel.m_pageSizeCombo);
    if (pageSizeId == QPrinter::Custom)
        printer->setPaperSize(pageSize(), QPrinter::Millimeter);
    else
        printer->setPageSize(pageSizeId);
    printer->setOrientation(comboBoxValue<QPrinter::Orientation>(m_panel.m_orientationCombo));
    printer->setFullPage(m_panel.m_fullPageCheckBox->isChecked());
}

void PrintDialogPanel::retrieveSettings(const QPrinter *printer)
{
    setComboBoxValue(m_panel.m_pageSizeCombo, printer->pageSize());
    setComboBoxValue(m_panel.m_orientationCombo, printer->orientation());
    m_panel.m_fullPageCheckBox->setChecked(printer->fullPage());
    setPageSize(m_printer->paperSize(QPrinter::Millimeter));
}

void PrintDialogPanel::enableCustomSizeControl()
{
    bool custom = (m_panel.m_pageSizeCombo->currentIndex() == QPrinter::Custom);
    m_panel.m_pageWidth->setEnabled(custom);
    m_panel.m_pageHeight->setEnabled(custom);
}

void PrintDialogPanel::showPrintDialog()
{
    applySettings(m_printer.data());
    QPrintDialog dialog(m_printer.data(), this);
    dialog.setOptions(m_panel.m_dialogOptionsGroupBox->value<QPrintDialog::PrintDialogOptions>());
    dialog.setPrintRange(comboBoxValue<QPrintDialog::PrintRange>(m_panel.m_printDialogRangeCombo));
    if (dialog.exec() == QDialog::Accepted) {
        retrieveSettings(m_printer.data());
        print(m_printer.data());
    }
}

void PrintDialogPanel::showPreviewDialog()
{
    applySettings(m_printer.data());
    PrintPreviewDialog dialog(m_printer.data(), this);
    dialog.resize(QApplication::desktop()->availableGeometry().size() * 4/ 5);
    if (dialog.exec() == QDialog::Accepted)
        retrieveSettings(m_printer.data());
}

void PrintDialogPanel::showPageSetupDialog()
{
    applySettings(m_printer.data());
    QPageSetupDialog dialog(m_printer.data(), this);
    if (dialog.exec() == QDialog::Accepted)
        retrieveSettings(m_printer.data());
}

#include "moc_printdialogpanel.cpp"
#include "printdialogpanel.moc"

#endif // !QT_NO_PRINTER
