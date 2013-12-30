/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qpagesetupdialog.h"

#ifndef QT_NO_PRINTDIALOG
#include "qpagesetupdialog_unix_p.h"

#include "qpainter.h"
#include "qprintdialog.h"
#include "qdialogbuttonbox.h"
#include <ui_qpagesetupwidget.h>

#include <QtPrintSupport/qprinter.h>
#include <private/qpagesetupdialog_p.h>
#include <private/qprinter_p.h>
#include <private/qprintengine_pdf_p.h>

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
#  include <private/qcups_p.h>
#endif

QT_BEGIN_NAMESPACE

QSizeF qt_printerPaperSize(QPrinter::Orientation, QPrinter::PaperSize, QPrinter::Unit, int);

// Disabled until we have support for papersources on unix
// #define PSD_ENABLE_PAPERSOURCE

static void populatePaperSizes(QComboBox* cb)
{
    cb->addItem(QPrintDialog::tr("A0"), QPrinter::A0);
    cb->addItem(QPrintDialog::tr("A1"), QPrinter::A1);
    cb->addItem(QPrintDialog::tr("A2"), QPrinter::A2);
    cb->addItem(QPrintDialog::tr("A3"), QPrinter::A3);
    cb->addItem(QPrintDialog::tr("A4"), QPrinter::A4);
    cb->addItem(QPrintDialog::tr("A5"), QPrinter::A5);
    cb->addItem(QPrintDialog::tr("A6"), QPrinter::A6);
    cb->addItem(QPrintDialog::tr("A7"), QPrinter::A7);
    cb->addItem(QPrintDialog::tr("A8"), QPrinter::A8);
    cb->addItem(QPrintDialog::tr("A9"), QPrinter::A9);
    cb->addItem(QPrintDialog::tr("B0"), QPrinter::B0);
    cb->addItem(QPrintDialog::tr("B1"), QPrinter::B1);
    cb->addItem(QPrintDialog::tr("B2"), QPrinter::B2);
    cb->addItem(QPrintDialog::tr("B3"), QPrinter::B3);
    cb->addItem(QPrintDialog::tr("B4"), QPrinter::B4);
    cb->addItem(QPrintDialog::tr("B5"), QPrinter::B5);
    cb->addItem(QPrintDialog::tr("B6"), QPrinter::B6);
    cb->addItem(QPrintDialog::tr("B7"), QPrinter::B7);
    cb->addItem(QPrintDialog::tr("B8"), QPrinter::B8);
    cb->addItem(QPrintDialog::tr("B9"), QPrinter::B9);
    cb->addItem(QPrintDialog::tr("B10"), QPrinter::B10);
    cb->addItem(QPrintDialog::tr("C5E"), QPrinter::C5E);
    cb->addItem(QPrintDialog::tr("DLE"), QPrinter::DLE);
    cb->addItem(QPrintDialog::tr("Executive"), QPrinter::Executive);
    cb->addItem(QPrintDialog::tr("Folio"), QPrinter::Folio);
    cb->addItem(QPrintDialog::tr("Ledger"), QPrinter::Ledger);
    cb->addItem(QPrintDialog::tr("Legal"), QPrinter::Legal);
    cb->addItem(QPrintDialog::tr("Letter"), QPrinter::Letter);
    cb->addItem(QPrintDialog::tr("Tabloid"), QPrinter::Tabloid);
    cb->addItem(QPrintDialog::tr("US Common #10 Envelope"), QPrinter::Comm10E);
    cb->addItem(QPrintDialog::tr("Custom"), QPrinter::Custom);
}


static QSizeF sizeForOrientation(QPrinter::Orientation orientation, const QSizeF &size)
{
    return (orientation == QPrinter::Portrait) ? size : QSizeF(size.height(), size.width());
}

#ifdef PSD_ENABLE_PAPERSOURCE
static const char *paperSourceNames[] = {
    "Only One",
    "Lower",
    "Middle",
    "Manual",
    "Envelope",
    "Envelope manual",
    "Auto",
    "Tractor",
    "Small format",
    "Large format",
    "Large capacity",
    "Cassette",
    "Form source",
    0
};

struct PaperSourceNames
{
    PaperSourceNames(const char *nam, QPrinter::PaperSource ps)
        : paperSource(ps), name(nam) {}
    QPrinter::PaperSource paperSource;
    const char *name;
};
#endif


class QPagePreview : public QWidget
{
public:
    QPagePreview(QWidget *parent) : QWidget(parent)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setMinimumSize(50, 50);
    }

    void setPaperSize(const QSizeF& size)
    {
        m_size = size;
        update();
    }

    void setMargins(qreal left, qreal top, qreal right, qreal bottom)
    {
        m_left = left;
        m_top = top;
        m_right = right;
        m_bottom = bottom;
        update();
    }

    void setPagePreviewLayout(int columns, int rows)
    {
      m_pagePreviewColumns = columns;
      m_pagePreviewRows = rows;
      update();
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        QRect pageRect;
        QSizeF adjustedSize(m_size);
        adjustedSize.scale(width()-10, height()-10, Qt::KeepAspectRatio);
        pageRect = QRect(QPoint(0,0), adjustedSize.toSize());
        pageRect.moveCenter(rect().center());

        qreal width_factor = pageRect.width() / m_size.width();
        qreal height_factor = pageRect.height() / m_size.height();
        int leftSize = qRound(m_left*width_factor);
        int topSize = qRound(m_top*height_factor);
        int rightSize = qRound(m_right*width_factor);
        int bottomSize = qRound(m_bottom * height_factor);
        QRect marginRect(pageRect.x()+leftSize,
                         pageRect.y()+topSize,
                         pageRect.width() - (leftSize+rightSize+1),
                         pageRect.height() - (topSize+bottomSize+1));

        QPainter p(this);
        QColor shadow(palette().mid().color());
        for (int i=1; i<6; ++i) {
            shadow.setAlpha(180-i*30);
            QRect offset(pageRect.adjusted(i, i, i, i));
            p.setPen(shadow);
            p.drawLine(offset.left(), offset.bottom(), offset.right(), offset.bottom());
            p.drawLine(offset.right(), offset.top(), offset.right(), offset.bottom()-1);
        }
        p.fillRect(pageRect, palette().light());

        if (marginRect.isValid()) {
            p.setPen(QPen(palette().color(QPalette::Dark), 0, Qt::DotLine));
            p.drawRect(marginRect);

            marginRect.adjust(2, 2, -1, -1);
            p.setClipRect(marginRect);
            QFont font;
            font.setPointSizeF(font.pointSizeF()*0.25);
            p.setFont(font);
            p.setPen(palette().color(QPalette::Dark));
            QString text(QLatin1String("Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi."));
            for (int i=0; i<3; ++i)
                text += text;

            const int spacing = pageRect.width() * 0.1;
            const int textWidth = (marginRect.width() - (spacing * (m_pagePreviewColumns-1))) / m_pagePreviewColumns;
            const int textHeight = (marginRect.height() - (spacing * (m_pagePreviewRows-1))) / m_pagePreviewRows;

            for (int x = 0 ; x < m_pagePreviewColumns; ++x) {
                for (int y = 0 ; y < m_pagePreviewRows; ++y) {
                    QRect textRect(marginRect.left() + x * (textWidth + spacing),
                                   marginRect.top() + y * (textHeight + spacing),
                                   textWidth, textHeight);
                    p.drawText(textRect, Qt::TextWordWrap|Qt::AlignVCenter, text);
                }
            }
        }
    }

private:
    // all these are in points
    qreal m_left, m_top, m_right, m_bottom;
    // specify width / height of one page in preview
    int m_pagePreviewColumns, m_pagePreviewRows;
    QSizeF m_size;
};


class QUnixPageSetupDialogPrivate : public QPageSetupDialogPrivate
{
    Q_DECLARE_PUBLIC(QPageSetupDialog)

public:
    QUnixPageSetupDialogPrivate(QPrinter *printer);
    ~QUnixPageSetupDialogPrivate();
    void init();

    QPageSetupWidget *widget;
};

QUnixPageSetupDialogPrivate::QUnixPageSetupDialogPrivate(QPrinter *printer) : QPageSetupDialogPrivate(printer)
{
}

QUnixPageSetupDialogPrivate::~QUnixPageSetupDialogPrivate()
{
}

void QUnixPageSetupDialogPrivate::init()
{
    Q_Q(QPageSetupDialog);

    widget = new QPageSetupWidget(q);
    widget->setPrinter(printer);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok
                                                     | QDialogButtonBox::Cancel,
                                                     Qt::Horizontal, q);
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), q, SLOT(reject()));

    QVBoxLayout *lay = new QVBoxLayout(q);
    lay->addWidget(widget);
    lay->addWidget(buttons);
}

QPageSetupWidget::QPageSetupWidget(QWidget *parent)
    : QWidget(parent),
    m_printer(0),
    m_blockSignals(false),
    m_cups(false)
{
    widget.setupUi(this);

    QString suffix = (QLocale::system().measurementSystem() == QLocale::ImperialSystem)
                     ? QString::fromLatin1(" in")
                     : QString::fromLatin1(" mm");
    widget.topMargin->setSuffix(suffix);
    widget.bottomMargin->setSuffix(suffix);
    widget.leftMargin->setSuffix(suffix);
    widget.rightMargin->setSuffix(suffix);
    widget.paperWidth->setSuffix(suffix);
    widget.paperHeight->setSuffix(suffix);

    QVBoxLayout *lay = new QVBoxLayout(widget.preview);
    widget.preview->setLayout(lay);
    m_pagePreview = new QPagePreview(widget.preview);
    m_pagePreview->setPagePreviewLayout(1, 1);

    lay->addWidget(m_pagePreview);

    setAttribute(Qt::WA_WState_Polished, false);

#ifdef PSD_ENABLE_PAPERSOURCE
    for (int i=0; paperSourceNames[i]; ++i)
        widget.paperSource->insertItem(paperSourceNames[i]);
#else
    widget.paperSourceLabel->setVisible(false);
    widget.paperSource->setVisible(false);
#endif

    widget.reverseLandscape->setVisible(false);
    widget.reversePortrait->setVisible(false);

    populatePaperSizes(widget.paperSize);
    initPagesPerSheet();
    QStringList units;
    units << tr("Centimeters (cm)") << tr("Millimeters (mm)") << tr("Inches (in)") << tr("Points (pt)");
    widget.unit->addItems(units);
    connect(widget.unit, SIGNAL(activated(int)), this, SLOT(unitChanged(int)));
    widget.unit->setCurrentIndex((QLocale::system().measurementSystem() == QLocale::ImperialSystem) ? 2 : 1);

    connect(widget.paperSize, SIGNAL(currentIndexChanged(int)), this, SLOT(_q_paperSizeChanged()));
    connect(widget.paperWidth, SIGNAL(valueChanged(double)), this, SLOT(_q_paperSizeChanged()));
    connect(widget.paperHeight, SIGNAL(valueChanged(double)), this, SLOT(_q_paperSizeChanged()));

    connect(widget.leftMargin, SIGNAL(valueChanged(double)), this, SLOT(setLeftMargin(double)));
    connect(widget.topMargin, SIGNAL(valueChanged(double)), this, SLOT(setTopMargin(double)));
    connect(widget.rightMargin, SIGNAL(valueChanged(double)), this, SLOT(setRightMargin(double)));
    connect(widget.bottomMargin, SIGNAL(valueChanged(double)), this, SLOT(setBottomMargin(double)));

    connect(widget.portrait, SIGNAL(clicked()), this, SLOT(_q_pageOrientationChanged()));
    connect(widget.landscape, SIGNAL(clicked()), this, SLOT(_q_pageOrientationChanged()));
    connect(widget.pagesPerSheetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(_q_pagesPerSheetChanged()));

}

void QPageSetupWidget::setPrinter(QPrinter *printer)
{
    m_printer = printer;
    m_blockSignals = true;
    selectPdfPsPrinter(printer);
    printer->getPageMargins(&m_leftMargin, &m_topMargin, &m_rightMargin, &m_bottomMargin, QPrinter::Point);
    unitChanged(widget.unit->currentIndex());
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
    m_paperSize = printer->paperSize(QPrinter::Point);
    widget.paperWidth->setValue(m_paperSize.width() / m_currentMultiplier);
    widget.paperHeight->setValue(m_paperSize.height() / m_currentMultiplier);

    widget.landscape->setChecked(printer->orientation() == QPrinter::Landscape);

#ifdef PSD_ENABLE_PAPERSOURCE
    widget.paperSource->setCurrentItem(printer->paperSource());
#endif
    Q_ASSERT(m_blockSignals);
    m_blockSignals = false;
    _q_paperSizeChanged();
}

// set gui data on printer
void QPageSetupWidget::setupPrinter() const
{
    QPrinter::Orientation orientation = widget.portrait->isChecked()
                                        ? QPrinter::Portrait
                                        : QPrinter::Landscape;
    m_printer->setOrientation(orientation);
    // paper format
    QVariant val = widget.paperSize->itemData(widget.paperSize->currentIndex());
    int ps = m_printer->pageSize();
    if (val.type() == QVariant::Int) {
        ps = val.toInt();
    }
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    else if (val.type() == QVariant::ByteArray) {
        for (int papersize = 0; papersize < QPrinter::NPageSize; ++papersize) {
            QSize size = QPageSize(QPageSize::PageSizeId(papersize)).sizePoints();
            if (size.width() == m_paperSize.width() && size.height() == m_paperSize.height()) {
                ps = static_cast<QPrinter::PaperSize>(papersize);
                break;
            }
        }
    }
#endif
    if (ps == QPrinter::Custom) {
        m_printer->setPaperSize(sizeForOrientation(orientation, m_paperSize), QPrinter::Point);
    }
    else {
        m_printer->setPaperSize(static_cast<QPrinter::PaperSize>(ps));
    }
    m_printer->setPaperName(widget.paperSize->currentText());
#ifdef PSD_ENABLE_PAPERSOURCE
    m_printer->setPaperSource((QPrinter::PaperSource)widget.paperSource->currentIndex());
#endif
    m_printer->setPageMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin, QPrinter::Point);

#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QCUPSSupport::PagesPerSheet pagesPerSheet = widget.pagesPerSheetCombo->currentData()
                                                        .value<QCUPSSupport::PagesPerSheet>();
    QCUPSSupport::PagesPerSheetLayout pagesPerSheetLayout = widget.pagesPerSheetLayoutCombo->currentData()
                                                        .value<QCUPSSupport::PagesPerSheetLayout>();

    QCUPSSupport::setPagesPerSheetLayout(m_printer, pagesPerSheet, pagesPerSheetLayout);
#endif
}

void QPageSetupWidget::selectPrinter()
{
    widget.paperSize->clear();
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    if (QCUPSSupport::isAvailable()) {
        m_cups = true;
        QCUPSSupport cups;
        cups.setCurrentPrinter(m_printer->printerName());
        const ppd_option_t* pageSizes = cups.pageSizes();
        const int numChoices = pageSizes ? pageSizes->num_choices : 0;

        int cupsDefaultSize = 0;
        QSize qtPreferredSize = m_printer->paperSize(QPrinter::Point).toSize();
        QString qtPaperName = m_printer->paperName();
        bool preferredSizeMatched = false;
        for (int i = 0; i < numChoices; ++i) {
            widget.paperSize->addItem(QString::fromLocal8Bit(pageSizes->choices[i].text), QByteArray(pageSizes->choices[i].choice));
            if (static_cast<int>(pageSizes->choices[i].marked) == 1)
                cupsDefaultSize = i;
            if (qtPaperName == QString::fromLocal8Bit(pageSizes->choices[i].text)) {
                widget.paperSize->setCurrentIndex(i);
                preferredSizeMatched = true;
            } else {
                QRect cupsPaperSize = cups.paperRect(pageSizes->choices[i].choice);
                QSize diff = cupsPaperSize.size() - qtPreferredSize;
                if (qAbs(diff.width()) < 5 && qAbs(diff.height()) < 5) {
                    widget.paperSize->setCurrentIndex(i);
                    preferredSizeMatched = true;
                }
            }
        }
        if (!preferredSizeMatched)
            widget.paperSize->setCurrentIndex(cupsDefaultSize);
        m_printer->getPageMargins(&m_leftMargin, &m_topMargin, &m_rightMargin, &m_bottomMargin, QPrinter::Point);
    } else
        m_cups = false;
#endif
    if (widget.paperSize->count() == 0) {
        populatePaperSizes(widget.paperSize);
        widget.paperSize->setCurrentIndex(widget.paperSize->findData(
            QLocale::system().measurementSystem() == QLocale::ImperialSystem ? QPrinter::Letter : QPrinter::A4));
    }

    unitChanged(widget.unit->currentIndex());
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::selectPdfPsPrinter(const QPrinter *p)
{
    m_cups = false;
    widget.paperSize->clear();
    populatePaperSizes(widget.paperSize);
    widget.paperSize->setCurrentIndex(widget.paperSize->findData(p->paperSize()));
    unitChanged(widget.unit->currentIndex());
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

// Updates size/preview after the combobox has been changed.
void QPageSetupWidget::_q_paperSizeChanged()
{
    if (m_blockSignals) return;
    m_blockSignals = true;

    bool custom = false;
    QVariant val = widget.paperSize->itemData(widget.paperSize->currentIndex());
    QPrinter::Orientation orientation = widget.portrait->isChecked() ? QPrinter::Portrait : QPrinter::Landscape;

    if (m_cups) {
        // OutputFormat == NativeFormat, data is QString Cups paper name
        QByteArray cupsPageSize = val.toByteArray();
        custom = (cupsPageSize == QByteArrayLiteral("Custom"));
#ifndef QT_NO_CUPS
        if (!custom) {
            QCUPSSupport cups;
            cups.setCurrentPrinter(m_printer->printerName());
            m_paperSize = sizeForOrientation(orientation, cups.paperRect(cupsPageSize).size());
        }
#endif
    } else {
        // OutputFormat == PdfFormat, data is QPrinter::PageSize
        QPrinter::PaperSize size = QPrinter::PaperSize(val.toInt());
        custom = size == QPrinter::Custom;
        if (!custom)
            m_paperSize = qt_printerPaperSize(orientation, size, QPrinter::Point, 1);
    }

    if (custom) {
        // Convert input custom size Units to Points
        m_paperSize = QSizeF(widget.paperWidth->value() * m_currentMultiplier,
                             widget.paperHeight->value() * m_currentMultiplier);
    } else {
        // Display standard size Points as Units
        widget.paperWidth->setValue(m_paperSize.width() / m_currentMultiplier);
        widget.paperHeight->setValue(m_paperSize.height() / m_currentMultiplier);
    }

    m_pagePreview->setPaperSize(m_paperSize);

    widget.paperWidth->setEnabled(custom);
    widget.paperHeight->setEnabled(custom);
    widget.widthLabel->setEnabled(custom);
    widget.heightLabel->setEnabled(custom);

    m_blockSignals = false;
}

void QPageSetupWidget::_q_pageOrientationChanged()
{
    bool custom = false;
    QVariant val = widget.paperSize->itemData(widget.paperSize->currentIndex());

    if (m_cups) {
        // OutputFormat == NativeFormat, data is QString Cups paper name
        QByteArray cupsPageSize = val.toByteArray();
        custom = (cupsPageSize == QByteArrayLiteral("Custom"));
    } else {
        // OutputFormat == PdfFormat, data is QPrinter::PageSize
        QPrinter::PaperSize size = QPrinter::PaperSize(val.toInt());
        custom = size == QPrinter::Custom;
    }

    if (custom) {
        double tmp = widget.paperWidth->value();
        widget.paperWidth->setValue(widget.paperHeight->value());
        widget.paperHeight->setValue(tmp);
    }
    _q_paperSizeChanged();
}

void QPageSetupWidget::_q_pagesPerSheetChanged()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    QCUPSSupport::PagesPerSheet pagesPerSheet = widget.pagesPerSheetCombo->currentData()
    .value<QCUPSSupport::PagesPerSheet>();

    switch (pagesPerSheet) {
    case QCUPSSupport::TwoPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(1, 2);
        break;
    case QCUPSSupport::FourPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(2, 2);
        break;
    case QCUPSSupport::SixPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(3, 2);
        break;
    case QCUPSSupport::NinePagesPerSheet:
        m_pagePreview->setPagePreviewLayout(3, 3);
        break;
    case QCUPSSupport::SixteenPagesPerSheet:
        m_pagePreview->setPagePreviewLayout(4, 4);
        break;
    case QCUPSSupport::OnePagePerSheet:
    default:
        m_pagePreview->setPagePreviewLayout(1, 1);
        break;
    }
#endif
}

extern double qt_multiplierForUnit(QPrinter::Unit unit, int resolution);

void QPageSetupWidget::unitChanged(int item)
{
    QString suffix;
    switch(item) {
    case 0:
        m_currentMultiplier = 10 * qt_multiplierForUnit(QPrinter::Millimeter, 1);
        suffix = QString::fromLatin1(" cm");
        break;
    case 2:
        m_currentMultiplier = qt_multiplierForUnit(QPrinter::Inch, 1);
        suffix = QString::fromLatin1(" in");
        break;
    case 3:
        m_currentMultiplier = qt_multiplierForUnit(QPrinter::Point, 1);
        suffix = QString::fromLatin1(" pt");
        break;
    case 1:
    default:
        m_currentMultiplier = qt_multiplierForUnit(QPrinter::Millimeter, 1);
        suffix = QString::fromLatin1(" mm");
        break;
    }
    const bool old = m_blockSignals;
    m_blockSignals = true;
    widget.topMargin->setSuffix(suffix);
    widget.leftMargin->setSuffix(suffix);
    widget.rightMargin->setSuffix(suffix);
    widget.bottomMargin->setSuffix(suffix);
    widget.paperWidth->setSuffix(suffix);
    widget.paperHeight->setSuffix(suffix);
    widget.topMargin->setValue(m_topMargin / m_currentMultiplier);
    widget.leftMargin->setValue(m_leftMargin / m_currentMultiplier);
    widget.rightMargin->setValue(m_rightMargin / m_currentMultiplier);
    widget.bottomMargin->setValue(m_bottomMargin / m_currentMultiplier);
    widget.paperWidth->setValue(m_paperSize.width() / m_currentMultiplier);
    widget.paperHeight->setValue(m_paperSize.height() / m_currentMultiplier);
    m_blockSignals = old;
}

void QPageSetupWidget::setTopMargin(double newValue)
{
    if (m_blockSignals) return;
    m_topMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::setBottomMargin(double newValue)
{
    if (m_blockSignals) return;
    m_bottomMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::setLeftMargin(double newValue)
{
    if (m_blockSignals) return;
    m_leftMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}

void QPageSetupWidget::setRightMargin(double newValue)
{
    if (m_blockSignals) return;
    m_rightMargin = newValue * m_currentMultiplier;
    m_pagePreview->setMargins(m_leftMargin, m_topMargin, m_rightMargin, m_bottomMargin);
}



QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QDialog(*(new QUnixPageSetupDialogPrivate(printer)), parent)
{
    Q_D(QPageSetupDialog);
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    static_cast<QUnixPageSetupDialogPrivate *>(d)->init();
}


QPageSetupDialog::QPageSetupDialog(QWidget *parent)
    : QDialog(*(new QUnixPageSetupDialogPrivate(0)), parent)
{
    Q_D(QPageSetupDialog);
    setWindowTitle(QCoreApplication::translate("QPrintPreviewDialog", "Page Setup"));
    static_cast<QUnixPageSetupDialogPrivate *>(d)->init();
}

int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    int ret = QDialog::exec();
    if (ret == Accepted)
        static_cast <QUnixPageSetupDialogPrivate*>(d)->widget->setupPrinter();
    return ret;
}

void QPageSetupWidget::initPagesPerSheet()
{
#if !defined(QT_NO_CUPS) && !defined(QT_NO_LIBRARY)
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Left to Right, Top to Bottom"), QVariant::fromValue(QCUPSSupport::LeftToRightTopToBottom));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Left to Right, Bottom to Top"), QVariant::fromValue(QCUPSSupport::LeftToRightBottomToTop));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Right to Left, Bottom to Top"), QVariant::fromValue(QCUPSSupport::RightToLeftBottomToTop));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Right to Left, Top to Bottom"), QVariant::fromValue(QCUPSSupport::RightToLeftTopToBottom));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Bottom to Top, Left to Right"), QVariant::fromValue(QCUPSSupport::BottomToTopLeftToRight));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Bottom to Top, Right to Left"), QVariant::fromValue(QCUPSSupport::BottomToTopRightToLeft));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Top to Bottom, Left to Right"), QVariant::fromValue(QCUPSSupport::TopToBottomLeftToRight));
    widget.pagesPerSheetLayoutCombo->addItem(QPrintDialog::tr("Top to Bottom, Right to Left"), QVariant::fromValue(QCUPSSupport::TopToBottomRightToLeft));

    widget.pagesPerSheetCombo->addItem(QPrintDialog::tr("1 (1x1)"), QVariant::fromValue(QCUPSSupport::OnePagePerSheet));
    widget.pagesPerSheetCombo->addItem(QPrintDialog::tr("2 (2x1)"), QVariant::fromValue(QCUPSSupport::TwoPagesPerSheet));
    widget.pagesPerSheetCombo->addItem(QPrintDialog::tr("4 (2x2)"), QVariant::fromValue(QCUPSSupport::FourPagesPerSheet));
    widget.pagesPerSheetCombo->addItem(QPrintDialog::tr("6 (2x3)"), QVariant::fromValue(QCUPSSupport::SixPagesPerSheet));
    widget.pagesPerSheetCombo->addItem(QPrintDialog::tr("9 (3x3)"), QVariant::fromValue(QCUPSSupport::NinePagesPerSheet));
    widget.pagesPerSheetCombo->addItem(QPrintDialog::tr("16 (4x4)"), QVariant::fromValue(QCUPSSupport::SixteenPagesPerSheet));

    // Set the combo to "1 (1x1)" -- QCUPSSupport::OnePagePerSheet
    widget.pagesPerSheetCombo->setCurrentIndex(0);
    // Set the layout combo to QCUPSSupport::LeftToRightTopToBottom
    widget.pagesPerSheetLayoutCombo->setCurrentIndex(0);
#else
    // Disable if CUPS wasn't found
    widget.pagesPerSheetButtonGroup->hide();
#endif
}

QT_END_NAMESPACE

#include "moc_qpagesetupdialog.cpp"

#endif // QT_NO_PRINTDIALOG
