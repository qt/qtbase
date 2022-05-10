// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QT_NO_PRINTER

#include "printdialogpanel.h"
#include "utils.h"

#include <QPrinter>
#include <QPrinterInfo>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPageSetupDialog>
#include <QApplication>
#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDoubleSpinBox>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QScreen>

const FlagData printerModeComboData[] =
{
    {"ScreenResolution", QPrinter::ScreenResolution},
    {"PrinterResolution", QPrinter::PrinterResolution},
    {"HighResolution", QPrinter::HighResolution}
};


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
    s << rect.width() << 'x' << rect.height() << Qt::forcesign << rect.x() << rect.y()
      << Qt::noforcesign;
    return s;
}

QTextStream &operator<<(QTextStream &s, const QPrinter &printer)
{
    const auto pageLayout = printer.pageLayout();
    const auto pageSize = pageLayout.pageSize();
    s << '"' << printer.printerName() << "\"\nPaper #" << pageSize.id()
        << " \"" << pageSize.name() << '"'
      << (pageLayout.orientation() == QPageLayout::Portrait ? ", Portrait" : ", Landscape");
    if (printer.fullPage())
        s << ", full page";
    s << "\nPaper size: "
        << pageSize.sizePoints() << "pt "
        << pageSize.size(QPageSize::Millimeter) << "mm "
      << "\n            " << pageSize.sizePixels(printer.resolution()) << " device pt "
        <<  pageSize.size(QPageSize::Inch) << "inch "
      << "\n            " <<   pageSize.size(QPageSize::Millimeter) << "mm"
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

static bool print(QPrinter *printer, QString *errorMessage)
{
    QPainter painter;

    if (!printer->isValid()) {
        *errorMessage = QLatin1String("Invalid printer.");
        return false;
    }

    if (printer->printerState() != QPrinter::Idle) {
        *errorMessage = QLatin1String("Printer not idle (state ")
            + QString::number(printer->printerState())
            + QLatin1String(").");
        return false;
    }

    if (!painter.begin(printer)) {
        *errorMessage = QLatin1String("QPainter::begin() failed.");
        return false;
    }

    const QRectF pageF = printer->pageRect(QPrinter::DevicePixel);

    QFont font = painter.font();
    font.setFamily("Courier");
    font.setPointSize(10);

    // Format message.
    const int charHeight = QFontMetrics(font).boundingRect('X').height();
    QString msg;
    QTextStream str(&msg);
    str << "Qt "<< QT_VERSION_STR;
    str << ' ' << QGuiApplication::platformName();
    str << ' ' << QDateTime::currentDateTime().toString()
        << "\nFont: " << font.family() << ' ' << font.pointSize() << '\n'
        << *printer;

    if (!painter.device()->logicalDpiY() || !painter.device()->logicalDpiX()) {
        *errorMessage = QLatin1String("Bailing out due to invalid DPI.");
        return false;
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

    if (!painter.end()) {
        *errorMessage = QLatin1String("QPainter::end() failed.");
        return false;
    }

    return true;
}

static bool print(QPrinter *printer, QWidget *dialogParent)
{
    QString errorMessage;
    const bool result = print(printer, &errorMessage);
    if (!result)
        QMessageBox::warning(dialogParent, QLatin1String("Printing Failed"), errorMessage);
    return result;
}

class PrintPreviewDialog : public QPrintPreviewDialog {
    Q_OBJECT
public:
    explicit PrintPreviewDialog(QPrinter *printer, QWidget *parent = nullptr) : QPrintPreviewDialog(printer, parent)
    {
        connect(this, SIGNAL(paintRequested(QPrinter*)), this, SLOT(slotPaintRequested(QPrinter*)));
    }

public slots:
    void slotPaintRequested(QPrinter *p) { print(p, this); }
};

PrintDialogPanel::PrintDialogPanel(QWidget *parent)
    : QWidget(parent), m_blockSignals(true)
{
    m_panel.setupUi(this);

    // Setup the Create box
    populateCombo(m_panel.m_printerModeCombo, printerModeComboData, sizeof(printerModeComboData)/sizeof(FlagData));
    connect(m_panel.m_createButton, SIGNAL(clicked()), this, SLOT(createPrinter()));
    connect(m_panel.m_deleteButton, SIGNAL(clicked()), this, SLOT(deletePrinter()));

    // Setup the Page Layout box
    populateCombo(m_panel.m_unitsCombo, unitsComboData, sizeof(unitsComboData)/sizeof(FlagData));
    connect(m_panel.m_unitsCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(unitsChanged()));
    for (int i = QPageSize::A4; i < QPageSize::LastPageSize; ++i) {
        QPageSize::PageSizeId id = QPageSize::PageSizeId(i);
        m_panel.m_pageSizeCombo->addItem(QPageSize::name(id), QVariant(id));
    }
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
    foreach (const QString &name, QPrinterInfo::availablePrinterNames())
        m_panel.m_printerCombo->addItem(name, QVariant(name));
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
    const int currentIndex = m_panel.m_printerCombo->currentIndex();
    QString printerName = m_panel.m_printerCombo->itemData(currentIndex).toString();
    if (printerName == QLatin1String("PdfFormat"))
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

    printer->setPageLayout(m_pageLayout);
}

// Retrieve the settings from the QPrinter
void PrintDialogPanel::retrieveSettings(const QPrinter *printer)
{
    if (printer->outputFormat() == QPrinter::NativeFormat) {
        m_panel.m_printerCombo->setCurrentIndex(m_panel.m_printerCombo->findData(QVariant(printer->printerName())));
        m_panel.m_fileName->setEnabled(false);
    } else {
        m_panel.m_printerCombo->setCurrentIndex(m_panel.m_printerCombo->findData(QVariant(QLatin1String("PdfFormat"))));
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

    m_pageLayout = printer->pageLayout();
    updatePageLayoutWidgets();
}

void PrintDialogPanel::updatePageLayoutWidgets()
{
    m_blockSignals = true;
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
    m_pageLayout.setUnits(comboBoxValue<QPageLayout::Unit>(m_panel.m_unitsCombo));
    updatePageLayoutWidgets();
}

void PrintDialogPanel::pageSizeChanged()
{
    if (m_blockSignals)
        return;
    const QPageSize::PageSizeId pageSizeId = comboBoxValue<QPageSize::PageSizeId>(m_panel.m_pageSizeCombo);
    QPageSize pageSize;
    if (pageSizeId == QPageSize::Custom)
        pageSize = QPageSize(QSizeF(200, 200), QPageSize::Unit(m_pageLayout.units()));
    else
        pageSize = QPageSize(pageSizeId);
    m_pageLayout.setPageSize(pageSize);
    updatePageLayoutWidgets();
}

void PrintDialogPanel::pageDimensionsChanged()
{
    if (m_blockSignals)
        return;
    m_pageLayout.setPageSize(QPageSize(customPageSize(), QPageSize::Unit(m_pageLayout.units())));
    updatePageLayoutWidgets();
}

void PrintDialogPanel::orientationChanged()
{
    if (m_blockSignals)
        return;
    m_pageLayout.setOrientation(comboBoxValue<QPageLayout::Orientation>(m_panel.m_orientationCombo));
    updatePageLayoutWidgets();
}

void PrintDialogPanel::marginsChanged()
{
    if (m_blockSignals)
        return;
    m_pageLayout.setMargins(QMarginsF(m_panel.m_leftMargin->value(), m_panel.m_topMargin->value(),
                                      m_panel.m_rightMargin->value(), m_panel.m_bottomMargin->value()));
    updatePageLayoutWidgets();
}

void PrintDialogPanel::layoutModeChanged()
{
    if (m_blockSignals)
        return;
    m_pageLayout.setMode(comboBoxValue<QPageLayout::Mode>(m_panel.m_layoutModeCombo));
    updatePageLayoutWidgets();
}

void PrintDialogPanel::printerChanged()
{
    const int currentIndex = m_panel.m_printerCombo->currentIndex();
    const bool isPdf = (m_panel.m_printerCombo->itemData(currentIndex).toString() == QLatin1String("PdfFormat"));
    m_panel.m_fileName->setEnabled(isPdf);
    if (isPdf && m_panel.m_fileName->text().isEmpty())
        m_panel.m_fileName->setText(QDir::homePath() + QDir::separator() + QLatin1String("print.pdf"));
}

void PrintDialogPanel::showPrintDialog()
{
    applySettings(m_printer.data());
    QPrintDialog dialog(m_printer.data(), this);
    dialog.setOptions(m_panel.m_dialogOptionsGroupBox->value<QPrintDialog::PrintDialogOptions>());
    if (dialog.exec() == QDialog::Accepted) {
        retrieveSettings(m_printer.data());
        print(m_printer.data(), this);
    }
}

void PrintDialogPanel::showPreviewDialog()
{
    applySettings(m_printer.data());
    PrintPreviewDialog dialog(m_printer.data(), this);
    const QSize availableSize = screen()->availableSize();
    dialog.resize(availableSize * 4/ 5);
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
    print(m_printer.data(), this);
    retrieveSettings(m_printer.data());
}

#include "moc_printdialogpanel.cpp"
#include "printdialogpanel.moc"

#endif // !QT_NO_PRINTER
