/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include <QPrinterInfo>
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
#include <QDir>

const FlagData printerModeComboData[] =
{
    {"ScreenResolution", QPrinter::ScreenResolution},
    {"PrinterResolution", QPrinter::PrinterResolution},
    {"HighResolution", QPrinter::HighResolution}
};

#if QT_VERSION < 0x050300
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
#endif

const FlagData printRangeComboData[] =
{
    {"AllPages", QPrinter::AllPages},
    {"Selection", QPrinter::Selection},
    {"PageRange", QPrinter::PageRange},
    {"CurrentPage", QPrinter::CurrentPage}
};

const FlagData pageOrderComboData[] =
{
    {"FirstPageFirst", QPrinter::FirstPageFirst},
    {"LastPageFirst", QPrinter::LastPageFirst},
};

const FlagData duplexModeComboData[] =
{
    {"DuplexNone", QPrinter::DuplexNone},
    {"DuplexAuto", QPrinter::DuplexAuto},
    {"DuplexLongSide", QPrinter::DuplexLongSide},
    {"DuplexShortSide", QPrinter::DuplexShortSide},
};

const FlagData paperSourceComboData[] =
{
    {"OnlyOne", QPrinter::OnlyOne},
    {"Lower", QPrinter::Lower},
    {"Middle", QPrinter::Middle},
    {"Manual", QPrinter::Manual},
    {"Envelope", QPrinter::Envelope},
    {"EnvelopeManual", QPrinter::EnvelopeManual},
    {"Auto", QPrinter::Auto},
    {"Tractor", QPrinter::Tractor},
    {"SmallFormat", QPrinter::SmallFormat},
    {"LargeFormat", QPrinter::LargeFormat},
    {"LargeCapacity", QPrinter::LargeCapacity},
    {"Cassette", QPrinter::Cassette},
    {"FormSource", QPrinter::FormSource},
    {"DuplexLongSide", QPrinter::DuplexLongSide},
    {"DuplexShortSide", QPrinter::DuplexShortSide},
};

const FlagData colorModeComboData[] =
{
    {"GrayScale", QPrinter::GrayScale},
    {"Color", QPrinter::Color},
};

const FlagData unitsComboData[] =
{
    {"Millimeter", QPageLayout::Millimeter},
    {"Inch", QPageLayout::Inch},
    {"Point", QPageLayout::Point},
    {"Pica", QPageLayout::Pica},
    {"Didot", QPageLayout::Didot},
    {"Cicero", QPageLayout::Cicero},
};

const FlagData orientationComboData[] =
{
    {"Portrait", QPageLayout::Portrait},
    {"Landscape", QPageLayout::Landscape},
};

const FlagData layoutModeComboData[] =
{
    {"StandardMode", QPageLayout::StandardMode},
    {"FullPageMode", QPageLayout::FullPageMode},
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
    : QWidget(parent), m_blockSignals(true)
{
#if QT_VERSION < 0x050300
    m_printerLayout.setOutputFormat(QPrinter::PdfFormat);
#endif

    m_panel.setupUi(this);

    // Setup the Create box
    populateCombo(m_panel.m_printerModeCombo, printerModeComboData, sizeof(printerModeComboData)/sizeof(FlagData));
    connect(m_panel.m_createButton, SIGNAL(clicked()), this, SLOT(createPrinter()));
    connect(m_panel.m_deleteButton, SIGNAL(clicked()), this, SLOT(deletePrinter()));

    // Setup the Page Layout box
    populateCombo(m_panel.m_unitsCombo, unitsComboData, sizeof(unitsComboData)/sizeof(FlagData));
    connect(m_panel.m_unitsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(unitsChanged()));
#if QT_VERSION >= 0x050300
    for (int i = QPageSize::A4; i < QPageSize::LastPageSize; ++i) {
        QPageSize::PageSizeId id = QPageSize::PageSizeId(i);
        m_panel.m_pageSizeCombo->addItem(QPageSize::name(id), QVariant(id));
    }
#else
    populateCombo(m_panel.m_pageSizeCombo, pageSizeComboData, sizeof(pageSizeComboData)/sizeof(FlagData));
#endif
    connect(m_panel.m_pageSizeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(pageSizeChanged()));
    connect(m_panel.m_pageWidth, SIGNAL(valueChanged(double)), this, SLOT(pageDimensionsChanged()));
    connect(m_panel.m_pageHeight, SIGNAL(valueChanged(double)), this, SLOT(pageDimensionsChanged()));
    populateCombo(m_panel.m_orientationCombo, orientationComboData, sizeof(orientationComboData)/sizeof(FlagData));
    connect(m_panel.m_orientationCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(orientationChanged()));
    connect(m_panel.m_leftMargin, SIGNAL(valueChanged(double)), this, SLOT(marginsChanged()));
    connect(m_panel.m_topMargin, SIGNAL(valueChanged(double)), this, SLOT(marginsChanged()));
    connect(m_panel.m_rightMargin, SIGNAL(valueChanged(double)), this, SLOT(marginsChanged()));
    connect(m_panel.m_bottomMargin, SIGNAL(valueChanged(double)), this, SLOT(marginsChanged()));
    populateCombo(m_panel.m_layoutModeCombo, layoutModeComboData, sizeof(layoutModeComboData)/sizeof(FlagData));
    connect(m_panel.m_layoutModeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(layoutModeChanged()));

    // Setup the Print Job box
    m_panel.m_printerCombo->addItem(tr("Print to PDF"), QVariant("PdfFormat"));
#if QT_VERSION >= 0x050300
    foreach (const QString &name, QPrinterInfo::availablePrinterNames())
        m_panel.m_printerCombo->addItem(name, QVariant(name));
#else
    foreach (const QPrinterInfo &printer, QPrinterInfo::availablePrinters())
        m_panel.m_printerCombo->addItem(printer.printerName(), QVariant(printer.printerName()));
#endif
    connect(m_panel.m_printerCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(printerChanged()));
    populateCombo(m_panel.m_printRangeCombo, printRangeComboData, sizeof(printRangeComboData)/sizeof(FlagData));
    populateCombo(m_panel.m_pageOrderCombo, pageOrderComboData, sizeof(pageOrderComboData)/sizeof(FlagData));
    populateCombo(m_panel.m_duplexModeCombo, duplexModeComboData, sizeof(duplexModeComboData)/sizeof(FlagData));
    populateCombo(m_panel.m_paperSourceCombo, paperSourceComboData, sizeof(paperSourceComboData)/sizeof(FlagData));
    populateCombo(m_panel.m_colorModeCombo, colorModeComboData, sizeof(colorModeComboData)/sizeof(FlagData));

    // Setup the Dialogs box
    m_panel.m_dialogOptionsGroupBox->populateOptions(printDialogOptions, sizeof(printDialogOptions) / sizeof(FlagData));
    QPrintDialog dialog;
    m_panel.m_dialogOptionsGroupBox->setValue(dialog.options());
    connect(m_panel.m_printButton, SIGNAL(clicked()), this, SLOT(showPrintDialog()));
    connect(m_panel.m_printPreviewButton, SIGNAL(clicked()), this, SLOT(showPreviewDialog()));
    connect(m_panel.m_pageSetupButton, SIGNAL(clicked()), this, SLOT(showPageSetupDialog()));
    connect(m_panel.m_directPrintButton, SIGNAL(clicked()), this, SLOT(directPrint()));

    enablePanels();
    m_blockSignals = false;
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
    m_panel.m_pageLayoutGroupBox->setEnabled(exists);
    m_panel.m_printJobGroupBox->setEnabled(exists);
    m_panel.m_dialogsGroupBox->setEnabled(exists);
}

void PrintDialogPanel::createPrinter()
{
    const QPrinter::PrinterMode mode = comboBoxValue<QPrinter::PrinterMode>(m_panel.m_printerModeCombo);
    m_printer.reset(new QPrinter(mode)); // Can set only once.
    retrieveSettings(m_printer.data());
    enablePanels();
}

void PrintDialogPanel::deletePrinter()
{
    m_printer.reset();
    enablePanels();
}

QSizeF PrintDialogPanel::customPageSize() const
{
    return QSizeF(m_panel.m_pageWidth->value(), m_panel.m_pageHeight->value());
}

// Apply the settings to the QPrinter
void PrintDialogPanel::applySettings(QPrinter *printer) const
{
    QString printerName = m_panel.m_printerCombo->currentData().toString();
    if (printerName == QStringLiteral("PdfFormat"))
        printer->setOutputFileName(m_panel.m_fileName->text());
    else
        printer->setPrinterName(printerName);
    printer->setPrintRange(comboBoxValue<QPrinter::PrintRange>(m_panel.m_printRangeCombo));
    printer->setFromTo(m_panel.m_fromPage->value(), m_panel.m_toPage->value());
    printer->setPageOrder(comboBoxValue<QPrinter::PageOrder>(m_panel.m_pageOrderCombo));
    printer->setCopyCount(m_panel.m_copyCount->value());
    printer->setCollateCopies(m_panel.m_collateCopies->isChecked());
    printer->setDuplex(comboBoxValue<QPrinter::DuplexMode>(m_panel.m_duplexModeCombo));
    printer->setPaperSource(comboBoxValue<QPrinter::PaperSource>(m_panel.m_paperSourceCombo));
    printer->setColorMode(comboBoxValue<QPrinter::ColorMode>(m_panel.m_colorModeCombo));
    printer->setResolution(m_panel.m_resolution->value());

#if QT_VERSION >= 0x050300
    printer->setPageLayout(m_pageLayout);
#else
    if (m_printerLayout.pageSize() == QPrinter::Custom)
        printer->setPaperSize(customPageSize(), m_units);
    else
        printer->setPageSize(m_printerLayout.pageSize());
    printer->setOrientation(m_printerLayout.orientation());
    printer->setFullPage(m_printerLayout.fullPage());
    double left, top, right, bottom;
    m_printerLayout.getPageMargins(&left, &top, &right, &bottom, m_units);
    printer->setPageMargins(left, top, right, bottom, m_units);
#endif
}

// Retrieve the settings from the QPrinter
void PrintDialogPanel::retrieveSettings(const QPrinter *printer)
{
    if (printer->outputFormat() == QPrinter::NativeFormat) {
        m_panel.m_printerCombo->setCurrentIndex(m_panel.m_printerCombo->findData(QVariant(printer->printerName())));
        m_panel.m_fileName->setEnabled(false);
    } else {
        m_panel.m_printerCombo->setCurrentIndex(m_panel.m_printerCombo->findData(QVariant(QStringLiteral("PdfFormat"))));
        m_panel.m_fileName->setEnabled(true);
    }
    m_panel.m_fileName->setText(printer->outputFileName());
    setComboBoxValue(m_panel.m_printRangeCombo, printer->printRange());
    m_panel.m_fromPage->setValue(printer->fromPage());
    m_panel.m_toPage->setValue(printer->toPage());
    setComboBoxValue(m_panel.m_pageOrderCombo, printer->pageOrder());
    m_panel.m_copyCount->setValue(printer->copyCount());
    m_panel.m_collateCopies->setChecked(printer->collateCopies());
    setComboBoxValue(m_panel.m_duplexModeCombo, printer->duplex());
    setComboBoxValue(m_panel.m_paperSourceCombo, printer->paperSource());
    setComboBoxValue(m_panel.m_colorModeCombo, printer->colorMode());
    m_panel.m_resolution->setValue(printer->resolution());

#ifdef Q_OS_WIN
    QString availPaperSources;
    foreach (QPrinter::PaperSource ps, printer->supportedPaperSources())
        availPaperSources += QString::number(int(ps)) + QLatin1Char(' ');
    m_panel.availPaperSourceLabel->setText(availPaperSources);
#else
    m_panel.availPaperSourceLabel->setText(QLatin1String("N/A"));
#endif

#if QT_VERSION >= 0x050300
    m_pageLayout = printer->pageLayout();
#else
    if (printer->pageSize() == QPrinter::Custom)
        m_printerLayout.setPaperSize(customPageSize(), m_units);
    else
        m_printerLayout.setPageSize(printer->pageSize());
    m_printerLayout.setOrientation(printer->orientation());
    m_printerLayout.setFullPage(printer->fullPage());
    double left, top, right, bottom;
    printer->getPageMargins(&left, &top, &right, &bottom, m_units);
    m_printerLayout.setPageMargins(left, top, right, bottom, m_units);
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::updatePageLayoutWidgets()
{
    m_blockSignals = true;
#if QT_VERSION >= 0x050300
    setComboBoxValue(m_panel.m_unitsCombo, m_pageLayout.units());
    setComboBoxValue(m_panel.m_pageSizeCombo, m_pageLayout.pageSize().id());
    QSizeF sizef = m_pageLayout.pageSize().size(QPageSize::Unit(m_pageLayout.units()));
    bool custom = (m_pageLayout.pageSize().id() == QPageSize::Custom);
    setComboBoxValue(m_panel.m_orientationCombo, m_pageLayout.orientation());
    m_panel.m_leftMargin->setValue(m_pageLayout.margins().left());
    m_panel.m_topMargin->setValue(m_pageLayout.margins().top());
    m_panel.m_rightMargin->setValue(m_pageLayout.margins().right());
    m_panel.m_bottomMargin->setValue(m_pageLayout.margins().bottom());
    setComboBoxValue(m_panel.m_layoutModeCombo, m_pageLayout.mode());
    QRectF rectf = m_pageLayout.paintRect();
#else
    setComboBoxValue(m_panel.m_unitsCombo, m_units);
    setComboBoxValue(m_panel.m_pageSizeCombo, m_printerLayout.pageSize());
    QSizeF sizef = m_printerLayout.paperSize(m_units);
    bool custom = (m_printerLayout.pageSize() == QPrinter::Custom);
    setComboBoxValue(m_panel.m_orientationCombo, m_printerLayout.orientation());
    double left, top, right, bottom;
    m_printerLayout.getPageMargins(&left, &top, &right, &bottom, m_units);
    m_panel.m_leftMargin->setValue(left);
    m_panel.m_topMargin->setValue(top);
    m_panel.m_rightMargin->setValue(right);
    m_panel.m_bottomMargin->setValue(bottom);
    if (m_printerLayout.fullPage())
        setComboBoxValue(m_panel.m_layoutModeCombo, QPageLayout::FullPageMode);
    else
        setComboBoxValue(m_panel.m_layoutModeCombo, QPageLayout::StandardMode);
    QRectF rectf = m_printerLayout.pageRect(m_units);
#endif
    m_panel.m_pageWidth->setValue(sizef.width());
    m_panel.m_pageHeight->setValue(sizef.height());
    m_panel.m_pageWidth->setEnabled(custom);
    m_panel.m_pageHeight->setEnabled(custom);
    m_panel.m_rectX->setValue(rectf.x());
    m_panel.m_rectY->setValue(rectf.y());
    m_panel.m_rectWidth->setValue(rectf.width());
    m_panel.m_rectHeight->setValue(rectf.height());
    QString suffix;
    switch (comboBoxValue<QPageLayout::Unit>(m_panel.m_unitsCombo)) {
    case QPageLayout::Millimeter:
        suffix = tr(" mm");
        break;
    case QPageLayout::Point:
        suffix = tr(" pt");
        break;
    case QPageLayout::Inch:
        suffix = tr(" in");
        break;
    case QPageLayout::Pica:
        suffix = tr(" pc");
        break;
    case QPageLayout::Didot:
        suffix = tr(" DD");
        break;
    case QPageLayout::Cicero:
        suffix = tr(" CC");
        break;
    }
    m_panel.m_pageWidth->setSuffix(suffix);
    m_panel.m_pageHeight->setSuffix(suffix);
    m_panel.m_leftMargin->setSuffix(suffix);
    m_panel.m_topMargin->setSuffix(suffix);
    m_panel.m_rightMargin->setSuffix(suffix);
    m_panel.m_bottomMargin->setSuffix(suffix);
    m_panel.m_rectX->setSuffix(suffix);
    m_panel.m_rectY->setSuffix(suffix);
    m_panel.m_rectWidth->setSuffix(suffix);
    m_panel.m_rectHeight->setSuffix(suffix);
    m_blockSignals = false;
}

void PrintDialogPanel::unitsChanged()
{
    if (m_blockSignals)
        return;
#if QT_VERSION >= 0x050300
    m_pageLayout.setUnits(comboBoxValue<QPageLayout::Unit>(m_panel.m_unitsCombo));
#else
    m_units = comboBoxValue<QPrinter::Unit>(m_panel.m_unitsCombo);
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::pageSizeChanged()
{
    if (m_blockSignals)
        return;
#if QT_VERSION >= 0x050300
    const QPageSize::PageSizeId pageSizeId = comboBoxValue<QPageSize::PageSizeId>(m_panel.m_pageSizeCombo);
    QPageSize pageSize;
    if (pageSizeId == QPageSize::Custom)
        pageSize = QPageSize(QSizeF(200, 200), QPageSize::Unit(m_pageLayout.units()));
    else
        pageSize = QPageSize(pageSizeId);
    m_pageLayout.setPageSize(pageSize);
#else
    const QPrinter::PageSize pageSize = comboBoxValue<QPrinter::PageSize>(m_panel.m_pageSizeCombo);
    if (pageSize == QPrinter::Custom)
        m_printerLayout.setPaperSize(QSizeF(200, 200), m_units);
    else
        m_printerLayout.setPageSize(pageSize);
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::pageDimensionsChanged()
{
    if (m_blockSignals)
        return;
#if QT_VERSION >= 0x050300
    m_pageLayout.setPageSize(QPageSize(customPageSize(), QPageSize::Unit(m_pageLayout.units())));
#else
    m_printerLayout.setPaperSize(customPageSize(), m_units);
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::orientationChanged()
{
    if (m_blockSignals)
        return;
#if QT_VERSION >= 0x050300
    m_pageLayout.setOrientation(comboBoxValue<QPageLayout::Orientation>(m_panel.m_orientationCombo));
#else
    m_printerLayout.setOrientation(comboBoxValue<QPrinter::Orientation>(m_panel.m_orientationCombo));
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::marginsChanged()
{
    if (m_blockSignals)
        return;
#if QT_VERSION >= 0x050300
    m_pageLayout.setMargins(QMarginsF(m_panel.m_leftMargin->value(), m_panel.m_topMargin->value(),
                                      m_panel.m_rightMargin->value(), m_panel.m_bottomMargin->value()));
#else
    m_printerLayout.setPageMargins(m_panel.m_leftMargin->value(), m_panel.m_topMargin->value(),
                                   m_panel.m_rightMargin->value(), m_panel.m_bottomMargin->value(),
                                   m_units);
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::layoutModeChanged()
{
    if (m_blockSignals)
        return;
#if QT_VERSION >= 0x050300
    m_pageLayout.setMode(comboBoxValue<QPageLayout::Mode>(m_panel.m_layoutModeCombo));
#else
    bool fullPage = (comboBoxValue<QPageLayout::Mode>(m_panel.m_layoutModeCombo) == QPageLayout::FullPageMode);
    m_printerLayout.setFullPage(fullPage);
#endif
    updatePageLayoutWidgets();
}

void PrintDialogPanel::printerChanged()
{
    bool isPdf = (m_panel.m_printerCombo->currentData().toString() == QStringLiteral("PdfFormat"));
    m_panel.m_fileName->setEnabled(isPdf);
    if (isPdf && m_panel.m_fileName->text().isEmpty())
        m_panel.m_fileName->setText(QDir::homePath() + QDir::separator() + QStringLiteral("print.pdf"));
}

void PrintDialogPanel::showPrintDialog()
{
    applySettings(m_printer.data());
    QPrintDialog dialog(m_printer.data(), this);
    dialog.setOptions(m_panel.m_dialogOptionsGroupBox->value<QPrintDialog::PrintDialogOptions>());
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

void PrintDialogPanel::directPrint()
{
    applySettings(m_printer.data());
    print(m_printer.data());
    retrieveSettings(m_printer.data());
}

#include "moc_printdialogpanel.cpp"
#include "printdialogpanel.moc"

#endif // !QT_NO_PRINTER
