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

#include "qplatformdefs.h"
#include <QtPrintSupport/private/qtprintsupportglobal_p.h>

#include "private/qabstractprintdialog_p.h"
#if QT_CONFIG(messagebox)
#include <QtWidgets/qmessagebox.h>
#endif
#include "qprintdialog.h"
#if QT_CONFIG(filedialog)
#include "qfiledialog.h"
#endif
#include <QtCore/qdir.h>
#include <QtGui/qevent.h>
#if QT_CONFIG(filesystemmodel)
#include <QtWidgets/qfilesystemmodel.h>
#endif
#include <QtWidgets/qstyleditemdelegate.h>
#include <QtPrintSupport/qprinter.h>

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <private/qprintdevice_p.h>

#include <QtWidgets/qdialogbuttonbox.h>

#if QT_CONFIG(completer)
#include <private/qcompleter_p.h>
#endif
#include "ui_qprintpropertieswidget.h"
#include "ui_qprintsettingsoutput.h"
#include "ui_qprintwidget.h"

#if QT_CONFIG(cups)
#include <private/qcups_p.h>
#if QT_CONFIG(cupsjobwidget)
#include "qcupsjobwidget_p.h"
#endif
#endif

/*

Print dialog class declarations

    QPrintDialog:            The main Print Dialog, nothing really held here.

    QUnixPrintWidget:
    QUnixPrintWidgetPrivate: The real Unix Print Dialog implementation.

                             Directly includes the upper half of the Print Dialog
                             containing the Printer Selection widgets and
                             Properties button.

                             Embeds the Properties pop-up dialog from
                             QPrintPropertiesDialog

                             Embeds the lower half from separate widget class
                             QPrintDialogPrivate

                             Layout in qprintwidget.ui

    QPrintDialogPrivate:     The lower half of the Print Dialog containing the
                             Copies and Options tabs that expands when the
                             Options button is selected.

                             Layout in qprintsettingsoutput.ui

    QPrintPropertiesDialog:  Dialog displayed when clicking on Properties button to
                             allow editing of Page and Advanced tabs.

                             Layout in qprintpropertieswidget.ui

    QPPDOptionsModel:        Holds the PPD Options for the printer.

    QPPDOptionsEditor:       Edits the PPD Options for the printer.

*/

static void initResources()
{
    Q_INIT_RESOURCE(qprintdialog);
}

QT_BEGIN_NAMESPACE

class QOptionTreeItem;
class QPPDOptionsModel;

class QPrintPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    QPrintPropertiesDialog(QPrinter *printer, QPrintDevice *currentPrintDevice,
                           QPrinter::OutputFormat outputFormat, const QString &printerName,
                           QAbstractPrintDialog *parent);
    ~QPrintPropertiesDialog();

    void setupPrinter() const;

    void showEvent(QShowEvent *event) override;

private:
    friend class QUnixPrintWidgetPrivate;
    QPrinter *m_printer;
    Ui::QPrintPropertiesWidget widget;
    QDialogButtonBox *m_buttons;
#if QT_CONFIG(cupsjobwidget)
    QCupsJobWidget *m_jobOptions;
#endif

#if QT_CONFIG(cups)
    void setCupsOptionsFromItems(QOptionTreeItem *parent) const;

    QPPDOptionsModel *m_cupsOptionsModel;
#endif
};

class QUnixPrintWidgetPrivate;

class QUnixPrintWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QUnixPrintWidget(QPrinter *printer, QWidget *parent = nullptr);
    ~QUnixPrintWidget();
    void updatePrinter();

private:
    friend class QPrintDialogPrivate;
    friend class QUnixPrintWidgetPrivate;
    QUnixPrintWidgetPrivate *d;
    Q_PRIVATE_SLOT(d, void _q_printerChanged(int))
    Q_PRIVATE_SLOT(d, void _q_btnBrowseClicked())
    Q_PRIVATE_SLOT(d, void _q_btnPropertiesClicked())
};

class QUnixPrintWidgetPrivate
{
public:
    QUnixPrintWidgetPrivate(QUnixPrintWidget *q, QPrinter *prn);
    ~QUnixPrintWidgetPrivate();

    bool checkFields();
    void setupPrinter();
    void setOptionsPane(QPrintDialogPrivate *pane);
    void setupPrinterProperties();
// slots
    void _q_printerChanged(int index);
    void _q_btnPropertiesClicked();
    void _q_btnBrowseClicked();

    QUnixPrintWidget * const parent;
    QPrintPropertiesDialog *propertiesDialog;
    Ui::QPrintWidget widget;
    QAbstractPrintDialog * q;
    QPrinter *printer;
    QPrintDevice m_currentPrintDevice;

    void updateWidget();

private:
    QPrintDialogPrivate *optionsPane;
    bool filePrintersAdded;
    bool propertiesDialogShown;
};

class QPrintDialogPrivate : public QAbstractPrintDialogPrivate
{
    Q_DECLARE_PUBLIC(QPrintDialog)
    Q_DECLARE_TR_FUNCTIONS(QPrintDialog)
public:
    QPrintDialogPrivate();
    ~QPrintDialogPrivate();

    void init();

    void selectPrinter(const QPrinter::OutputFormat outputFormat);

    void _q_togglePageSetCombo(bool);
#if QT_CONFIG(messagebox)
    void _q_checkFields();
#endif
    void _q_collapseOrExpandDialog();

    void setupPrinter();
    void updateWidgets();

    virtual void setTabs(const QList<QWidget*> &tabs) override;

    Ui::QPrintSettingsOutput options;
    QUnixPrintWidget *top;
    QWidget *bottom;
    QDialogButtonBox *buttons;
    QPushButton *collapseButton;
    QPrinter::OutputFormat printerOutputFormat;
};

#if QT_CONFIG(cups)
class QOptionTreeItem
{
public:
    enum ItemType { Root, Group, Option, Choice };

    QOptionTreeItem(ItemType t, int i, const void *p, const char *desc, QOptionTreeItem *pi)
        : type(t),
          index(i),
          ptr(p),
          description(desc),
          selected(-1),
          selDescription(nullptr),
          parentItem(pi) {}

    ~QOptionTreeItem() {
        qDeleteAll(childItems);
    }

    ItemType type;
    int index;
    const void *ptr;
    const char *description;
    int selected;
    const char *selDescription;
    QOptionTreeItem *parentItem;
    QList<QOptionTreeItem*> childItems;
};

class QPPDOptionsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit QPPDOptionsModel(QPrintDevice *currentPrintDevice, QObject *parent);

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;

    QPrintDevice *m_currentPrintDevice;
    QTextCodec *cupsCodec;
    QOptionTreeItem *rootItem;
    void parseGroups(QOptionTreeItem *parent);
    void parseOptions(QOptionTreeItem *parent);
    void parseChoices(QOptionTreeItem *parent);
};

class QPPDOptionsEditor : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit QPPDOptionsEditor(QObject *parent) : QStyledItemDelegate(parent) {}

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
};

#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPrintPropertiesDialog

    Dialog displayed when clicking on Properties button to allow editing of Page
    and Advanced tabs.

*/

QPrintPropertiesDialog::QPrintPropertiesDialog(QPrinter *printer, QPrintDevice *currentPrintDevice,
                                               QPrinter::OutputFormat outputFormat, const QString &printerName,
                                               QAbstractPrintDialog *parent)
    : QDialog(parent)
    , m_printer(printer)
{
    setWindowTitle(tr("Printer Properties"));
    QVBoxLayout *lay = new QVBoxLayout(this);
    QWidget *content = new QWidget(this);
    widget.setupUi(content);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    lay->addWidget(content);
    lay->addWidget(m_buttons);

    connect(m_buttons->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_buttons->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), this, SLOT(reject()));

    widget.pageSetup->setPrinter(printer, outputFormat, printerName);

#if QT_CONFIG(cupsjobwidget)
    m_jobOptions = new QCupsJobWidget(printer);
    widget.tabs->insertTab(1, m_jobOptions, tr("Job Options"));
#endif

    const int advancedTabIndex = widget.tabs->indexOf(widget.cupsPropertiesPage);
#if QT_CONFIG(cups)
    m_cupsOptionsModel = new QPPDOptionsModel(currentPrintDevice, this);

    widget.treeView->setItemDelegate(new QPPDOptionsEditor(this));

    if (m_cupsOptionsModel->rowCount() > 0) {
        widget.treeView->setModel(m_cupsOptionsModel);

        for (int i = 0; i < m_cupsOptionsModel->rowCount(); ++i)
            widget.treeView->expand(m_cupsOptionsModel->index(i, 0));

        widget.tabs->setTabEnabled(advancedTabIndex, true);
    } else {
        widget.treeView->setModel(nullptr);
        widget.tabs->setTabEnabled(advancedTabIndex, false);
    }
#else
    Q_UNUSED(currentPrintDevice)
    widget.tabs->setTabEnabled(advancedTabIndex, false);
#endif
}

QPrintPropertiesDialog::~QPrintPropertiesDialog()
{
}

void QPrintPropertiesDialog::setupPrinter() const
{
    widget.pageSetup->setupPrinter();
#if QT_CONFIG(cupsjobwidget)
    m_jobOptions->setupPrinter();
#endif

#if QT_CONFIG(cups)
    setCupsOptionsFromItems(m_cupsOptionsModel->rootItem);
#endif
}

void QPrintPropertiesDialog::showEvent(QShowEvent *event)
{
    widget.treeView->resizeColumnToContents(0);
    QDialog::showEvent(event);
}

#if QT_CONFIG(cups)
void QPrintPropertiesDialog::setCupsOptionsFromItems(QOptionTreeItem *parent) const
{
    for (QOptionTreeItem *itm : qAsConst(parent->childItems)) {
        if (itm->type == QOptionTreeItem::Option) {
            const ppd_option_t *opt = static_cast<const ppd_option_t*>(itm->ptr);
            if (qstrcmp(opt->defchoice, opt->choices[itm->selected].choice) != 0) {
                QStringList cupsOptions = QCUPSSupport::cupsOptionsList(m_printer);
                QCUPSSupport::setCupsOption(cupsOptions, QString::fromLatin1(opt->keyword), QString::fromLatin1(opt->choices[itm->selected].choice));
                QCUPSSupport::setCupsOptions(m_printer, cupsOptions);
            }
        } else {
            setCupsOptionsFromItems(itm);
        }
    }
}
#endif

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPrintDialogPrivate

    The lower half of the Print Dialog containing the Copies and Options
    tabs that expands when the Options button is selected.

*/
QPrintDialogPrivate::QPrintDialogPrivate()
    : top(nullptr), bottom(nullptr), buttons(nullptr), collapseButton(nullptr)
{
    initResources();
}

QPrintDialogPrivate::~QPrintDialogPrivate()
{
}

void QPrintDialogPrivate::init()
{
    Q_Q(QPrintDialog);

    top = new QUnixPrintWidget(q->printer(), q);
    bottom = new QWidget(q);
    options.setupUi(bottom);
    options.color->setIconSize(QSize(32, 32));
    options.color->setIcon(QIcon(QLatin1String(":/qt-project.org/dialogs/qprintdialog/images/status-color.png")));
    options.grayscale->setIconSize(QSize(32, 32));
    options.grayscale->setIcon(QIcon(QLatin1String(":/qt-project.org/dialogs/qprintdialog/images/status-gray-scale.png")));

#if QT_CONFIG(cups)
    // Add Page Set widget if CUPS is available
    options.pageSetCombo->addItem(tr("All Pages"), QVariant::fromValue(QCUPSSupport::AllPages));
    options.pageSetCombo->addItem(tr("Odd Pages"), QVariant::fromValue(QCUPSSupport::OddPages));
    options.pageSetCombo->addItem(tr("Even Pages"), QVariant::fromValue(QCUPSSupport::EvenPages));
#endif

    top->d->setOptionsPane(this);

    buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, q);
    collapseButton = new QPushButton(QPrintDialog::tr("&Options >>"), buttons);
    buttons->addButton(collapseButton, QDialogButtonBox::ResetRole);
    bottom->setVisible(false);

    QPushButton *printButton = buttons->button(QDialogButtonBox::Ok);
    printButton->setText(QPrintDialog::tr("&Print"));
    printButton->setDefault(true);

    QVBoxLayout *lay = new QVBoxLayout(q);
    lay->addWidget(top);
    lay->addWidget(bottom);
    lay->addWidget(buttons);

#if !QT_CONFIG(messagebox)
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(accept()));
#else
    QObject::connect(buttons, SIGNAL(accepted()), q, SLOT(_q_checkFields()));
#endif
    QObject::connect(buttons, SIGNAL(rejected()), q, SLOT(reject()));

    QObject::connect(options.printSelection, SIGNAL(toggled(bool)),
                     q, SLOT(_q_togglePageSetCombo(bool)));

    QObject::connect(options.printCurrentPage, SIGNAL(toggled(bool)),
                     q, SLOT(_q_togglePageSetCombo(bool)));

    QObject::connect(collapseButton, SIGNAL(released()), q, SLOT(_q_collapseOrExpandDialog()));
}

// initialize printer options
void QPrintDialogPrivate::selectPrinter(const QPrinter::OutputFormat outputFormat)
{
        Q_Q(QPrintDialog);
        QPrinter *p = q->printer();
        printerOutputFormat = outputFormat;

        if (p->colorMode() == QPrinter::Color)
            options.color->setChecked(true);
        else
            options.grayscale->setChecked(true);

        switch (p->duplex()) {
        case QPrinter::DuplexNone:
            options.noDuplex->setChecked(true); break;
        case QPrinter::DuplexLongSide:
        case QPrinter::DuplexAuto:
            options.duplexLong->setChecked(true); break;
        case QPrinter::DuplexShortSide:
            options.duplexShort->setChecked(true); break;
        }
        options.copies->setValue(p->copyCount());
        options.collate->setChecked(p->collateCopies());
        options.reverse->setChecked(p->pageOrder() == QPrinter::LastPageFirst);

        if (outputFormat == QPrinter::PdfFormat || options.printSelection->isChecked()
            || options.printCurrentPage->isChecked())

            options.pageSetCombo->setEnabled(false);
        else
            options.pageSetCombo->setEnabled(true);
}

void QPrintDialogPrivate::setupPrinter()
{
    // First setup the requested OutputFormat, Printer and Page Size first
    top->d->setupPrinter();

    // Then setup Print Job options
    Q_Q(QPrintDialog);
    QPrinter* p = q->printer();

    if (options.duplex->isEnabled()) {
        if (options.noDuplex->isChecked())
            p->setDuplex(QPrinter::DuplexNone);
        else if (options.duplexLong->isChecked())
            p->setDuplex(QPrinter::DuplexLongSide);
        else
            p->setDuplex(QPrinter::DuplexShortSide);
    }

    p->setColorMode(options.color->isChecked() ? QPrinter::Color : QPrinter::GrayScale);
    p->setPageOrder(options.reverse->isChecked() ? QPrinter::LastPageFirst : QPrinter::FirstPageFirst);

    // print range
    if (options.printAll->isChecked()) {
        p->setPrintRange(QPrinter::AllPages);
        p->setFromTo(0,0);
    } else if (options.printSelection->isChecked()) {
        p->setPrintRange(QPrinter::Selection);
        p->setFromTo(0,0);
    } else if (options.printCurrentPage->isChecked()) {
        p->setPrintRange(QPrinter::CurrentPage);
        p->setFromTo(0,0);
    } else if (options.printRange->isChecked()) {
        if (q->isOptionEnabled(QPrintDialog::PrintPageRange)) {
            p->setPrintRange(QPrinter::PageRange);
            p->setFromTo(options.from->value(), qMax(options.from->value(), options.to->value()));
        } else {
            // This case happens when CUPS server-side page range is enabled
            // Setting the range to the printer occurs below
            p->setPrintRange(QPrinter::AllPages);
            p->setFromTo(0,0);
        }
    }

#if QT_CONFIG(cups)
    // page set
    if (p->printRange() == QPrinter::AllPages || p->printRange() == QPrinter::PageRange) {
        //If the application is selecting pages and the first page number is even then need to adjust the odd-even accordingly
        QCUPSSupport::PageSet pageSet = options.pageSetCombo->itemData(options.pageSetCombo->currentIndex()).value<QCUPSSupport::PageSet>();
        if (q->isOptionEnabled(QPrintDialog::PrintPageRange)
            && p->printRange() == QPrinter::PageRange
            && (q->fromPage() % 2 == 0)) {

            switch (pageSet) {
            case QCUPSSupport::AllPages:
                break;
            case QCUPSSupport::OddPages:
                QCUPSSupport::setPageSet(p, QCUPSSupport::EvenPages);
                break;
            case QCUPSSupport::EvenPages:
                QCUPSSupport::setPageSet(p, QCUPSSupport::OddPages);
                break;
            }
        } else if (pageSet != QCUPSSupport::AllPages) {
            QCUPSSupport::setPageSet(p, pageSet);
        }

        // server-side page range, since we set the page range on the printer to 0-0/AllPages above,
        // we need to take the values directly from the widget as q->fromPage() will return 0
        if (!q->isOptionEnabled(QPrintDialog::PrintPageRange) && options.printRange->isChecked())
            QCUPSSupport::setPageRange(p, options.from->value(), qMax(options.from->value(), options.to->value()));
    }
#endif

    // copies
    p->setCopyCount(options.copies->value());
    p->setCollateCopies(options.collate->isChecked());
}

void QPrintDialogPrivate::_q_togglePageSetCombo(bool checked)
{
    if (printerOutputFormat == QPrinter::PdfFormat)
        return;

    options.pageSetCombo->setDisabled(checked);
}

void QPrintDialogPrivate::_q_collapseOrExpandDialog()
{
    int collapseHeight = 0;
    Q_Q(QPrintDialog);
    QWidget *widgetToHide = bottom;
    if (widgetToHide->isVisible()) {
        collapseButton->setText(QPrintDialog::tr("&Options >>"));
        collapseHeight = widgetToHide->y() + widgetToHide->height() - (top->y() + top->height());
    }
    else
        collapseButton->setText(QPrintDialog::tr("&Options <<"));
    widgetToHide->setVisible(! widgetToHide->isVisible());
    if (! widgetToHide->isVisible()) { // make it shrink
        q->layout()->activate();
        q->resize( QSize(q->width(), q->height() - collapseHeight) );
    }
}

#if QT_CONFIG(messagebox)
void QPrintDialogPrivate::_q_checkFields()
{
    Q_Q(QPrintDialog);
    if (top->d->checkFields())
        q->accept();
}
#endif // QT_CONFIG(messagebox)


void QPrintDialogPrivate::updateWidgets()
{
    Q_Q(QPrintDialog);
    options.gbPrintRange->setVisible(q->isOptionEnabled(QPrintDialog::PrintPageRange) ||
                                     q->isOptionEnabled(QPrintDialog::PrintSelection) ||
                                     q->isOptionEnabled(QPrintDialog::PrintCurrentPage));

    options.printRange->setEnabled(q->isOptionEnabled(QPrintDialog::PrintPageRange));
    options.printSelection->setVisible(q->isOptionEnabled(QPrintDialog::PrintSelection));
    options.printCurrentPage->setVisible(q->isOptionEnabled(QPrintDialog::PrintCurrentPage));
    options.collate->setVisible(q->isOptionEnabled(QPrintDialog::PrintCollateCopies));

#if QT_CONFIG(cups)
    // Don't display Page Set if only Selection or Current Page are enabled
    if (!q->isOptionEnabled(QPrintDialog::PrintPageRange)
        && (q->isOptionEnabled(QPrintDialog::PrintSelection) || q->isOptionEnabled(QPrintDialog::PrintCurrentPage))) {
        options.pageSetCombo->setVisible(false);
        options.pageSetLabel->setVisible(false);
    } else {
        options.pageSetCombo->setVisible(true);
        options.pageSetLabel->setVisible(true);
    }

    if (!q->isOptionEnabled(QPrintDialog::PrintPageRange)) {
        // If we can do CUPS server side pages selection,
        // display the page range widgets
        options.gbPrintRange->setVisible(true);
        options.printRange->setEnabled(true);
    }
#endif

    switch (q->printRange()) {
    case QPrintDialog::AllPages:
        options.printAll->setChecked(true);
        options.pageSetCombo->setEnabled(true);
        break;
    case QPrintDialog::Selection:
        options.printSelection->setChecked(true);
        options.pageSetCombo->setEnabled(false);
        break;
    case QPrintDialog::PageRange:
        options.printRange->setChecked(true);
        options.pageSetCombo->setEnabled(true);
        break;
    case QPrintDialog::CurrentPage:
        if (q->isOptionEnabled(QPrintDialog::PrintCurrentPage)) {
            options.printCurrentPage->setChecked(true);
            options.pageSetCombo->setEnabled(false);
        }
        break;
    default:
        break;
    }
    const int minPage = qMax(1, qMin(q->minPage() , q->maxPage()));
    const int maxPage = qMax(1, q->maxPage() == INT_MAX ? 9999 : q->maxPage());

    options.from->setMinimum(minPage);
    options.to->setMinimum(minPage);
    options.from->setMaximum(maxPage);
    options.to->setMaximum(maxPage);

    options.from->setValue(q->fromPage());
    options.to->setValue(q->toPage());
    top->d->updateWidget();
}

void QPrintDialogPrivate::setTabs(const QList<QWidget*> &tabWidgets)
{
    QList<QWidget*>::ConstIterator iter = tabWidgets.begin();
    while(iter != tabWidgets.constEnd()) {
        QWidget *tab = *iter;
        options.tabs->addTab(tab, tab->windowTitle());
        ++iter;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPrintDialog

    The main Print Dialog.

*/

QPrintDialog::QPrintDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), printer, parent)
{
    Q_D(QPrintDialog);
    d->init();
}

/*!
    Constructs a print dialog with the given \a parent.
*/
QPrintDialog::QPrintDialog(QWidget *parent)
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), 0, parent)
{
    Q_D(QPrintDialog);
    d->init();
}

QPrintDialog::~QPrintDialog()
{
}

void QPrintDialog::setVisible(bool visible)
{
    Q_D(QPrintDialog);

    if (visible)
        d->updateWidgets();

    QAbstractPrintDialog::setVisible(visible);
}

int QPrintDialog::exec()
{
    return QDialog::exec();
}

void QPrintDialog::accept()
{
    Q_D(QPrintDialog);
    d->setupPrinter();
    QDialog::accept();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QUnixPrintWidget && QUnixPrintWidgetPrivate

    The upper half of the Print Dialog containing the Printer Selection widgets

*/

#if defined (Q_OS_UNIX)

/*! \internal
*/
QUnixPrintWidgetPrivate::QUnixPrintWidgetPrivate(QUnixPrintWidget *p, QPrinter *prn)
    : parent(p), propertiesDialog(0), printer(prn), optionsPane(0),
      filePrintersAdded(false), propertiesDialogShown(false)
{
    q = 0;
    if (parent)
        q = qobject_cast<QAbstractPrintDialog*> (parent->parent());

    widget.setupUi(parent);

    int currentPrinterIndex = 0;
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps) {
        const QStringList printers = ps->availablePrintDeviceIds();
        const QString defaultPrinter = ps->defaultPrintDeviceId();

        widget.printers->addItems(printers);

        const int idx = printers.indexOf(defaultPrinter);
        if (idx >= 0)
            currentPrinterIndex = idx;
    }
    widget.properties->setEnabled(true);

#if QT_CONFIG(filesystemmodel) && QT_CONFIG(completer)
    QFileSystemModel *fsm = new QFileSystemModel(widget.filename);
    fsm->setRootPath(QDir::homePath());
    widget.filename->setCompleter(new QCompleter(fsm, widget.filename));
#endif
    _q_printerChanged(currentPrinterIndex);

    QObject::connect(widget.printers, SIGNAL(currentIndexChanged(int)),
                     parent, SLOT(_q_printerChanged(int)));
    QObject::connect(widget.fileBrowser, SIGNAL(clicked()), parent, SLOT(_q_btnBrowseClicked()));
    QObject::connect(widget.properties, SIGNAL(clicked()), parent, SLOT(_q_btnPropertiesClicked()));

    // disable features that QPrinter does not yet support.
    widget.preview->setVisible(false);
}

void QUnixPrintWidgetPrivate::updateWidget()
{
    const bool printToFile = q == 0 || q->isOptionEnabled(QPrintDialog::PrintToFile);
    if (printToFile && !filePrintersAdded) {
        if (widget.printers->count())
            widget.printers->insertSeparator(widget.printers->count());
        widget.printers->addItem(QPrintDialog::tr("Print to File (PDF)"));
        filePrintersAdded = true;
    }
    if (!printToFile && filePrintersAdded) {
        widget.printers->removeItem(widget.printers->count()-1);
        widget.printers->removeItem(widget.printers->count()-1);
        if (widget.printers->count())
            widget.printers->removeItem(widget.printers->count()-1); // remove separator
        filePrintersAdded = false;
    }
    if (printer && filePrintersAdded && (printer->outputFormat() != QPrinter::NativeFormat
                                         || printer->printerName().isEmpty()))
    {
        if (printer->outputFormat() == QPrinter::PdfFormat)
            widget.printers->setCurrentIndex(widget.printers->count() - 1);
        widget.filename->setEnabled(true);
        widget.lOutput->setEnabled(true);
    }

    widget.filename->setVisible(printToFile);
    widget.lOutput->setVisible(printToFile);
    widget.fileBrowser->setVisible(printToFile);

    widget.properties->setVisible(q->isOptionEnabled(QAbstractPrintDialog::PrintShowPageSize));
}

QUnixPrintWidgetPrivate::~QUnixPrintWidgetPrivate()
{
}

void QUnixPrintWidgetPrivate::_q_printerChanged(int index)
{
    if (index < 0)
        return;
    const int printerCount = widget.printers->count();
    widget.filename->setEnabled(false);
    widget.lOutput->setEnabled(false);

    // Reset properties dialog when printer is changed
    if (propertiesDialog){
        delete propertiesDialog;
        propertiesDialog = nullptr;
        propertiesDialogShown = false;
    }

    if (filePrintersAdded) {
        Q_ASSERT(index != printerCount - 2); // separator
        if (index == printerCount - 1) { // PDF
            widget.location->setText(QPrintDialog::tr("Local file"));
            widget.type->setText(QPrintDialog::tr("Write PDF file"));
            widget.properties->setEnabled(true);
            widget.filename->setEnabled(true);
            QString filename = widget.filename->text();
            widget.filename->setText(filename);
            widget.lOutput->setEnabled(true);
            if (optionsPane)
                optionsPane->selectPrinter(QPrinter::PdfFormat);
            return;
        }
    }

    if (printer) {
        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        if (ps)
            m_currentPrintDevice = ps->createPrintDevice(widget.printers->itemText(index));

        printer->setPrinterName(m_currentPrintDevice.id());

        widget.location->setText(m_currentPrintDevice.location());
        widget.type->setText(m_currentPrintDevice.makeAndModel());
        if (optionsPane)
            optionsPane->selectPrinter(QPrinter::NativeFormat);
    }
}

void QUnixPrintWidgetPrivate::setOptionsPane(QPrintDialogPrivate *pane)
{
    optionsPane = pane;
    if (optionsPane)
        optionsPane->selectPrinter(QPrinter::NativeFormat);
}

void QUnixPrintWidgetPrivate::_q_btnBrowseClicked()
{
    QString filename = widget.filename->text();
#if QT_CONFIG(filedialog)
    filename = QFileDialog::getSaveFileName(parent, QPrintDialog::tr("Print To File ..."), filename,
                                            QString(), 0, QFileDialog::DontConfirmOverwrite);
#else
    filename.clear();
#endif
    if (!filename.isEmpty()) {
        widget.filename->setText(filename);
        widget.printers->setCurrentIndex(widget.printers->count() - 1); // the pdf one
    }
}

#if QT_CONFIG(messagebox)
bool QUnixPrintWidgetPrivate::checkFields()
{
    if (widget.filename->isEnabled()) {
        QString file = widget.filename->text();
        QFile f(file);
        QFileInfo fi(f);
        bool exists = fi.exists();
        bool opened = false;
        if (exists && fi.isDir()) {
            QMessageBox::warning(q, q->windowTitle(),
                            QPrintDialog::tr("%1 is a directory.\nPlease choose a different file name.").arg(file));
            return false;
        } else if ((exists && !fi.isWritable()) || !(opened = f.open(QFile::Append))) {
            QMessageBox::warning(q, q->windowTitle(),
                            QPrintDialog::tr("File %1 is not writable.\nPlease choose a different file name.").arg(file));
            return false;
        } else if (exists) {
            int ret = QMessageBox::question(q, q->windowTitle(),
                                            QPrintDialog::tr("%1 already exists.\nDo you want to overwrite it?").arg(file),
                                            QMessageBox::Yes|QMessageBox::No, QMessageBox::No);
            if (ret == QMessageBox::No)
                return false;
        }
        if (opened) {
            f.close();
            if (!exists)
                f.remove();
        }
    }

#if QT_CONFIG(cups)
    if (propertiesDialogShown) {
        QCUPSSupport::PagesPerSheet pagesPerSheet = propertiesDialog->widget.pageSetup->m_ui.pagesPerSheetCombo
                                                                    ->currentData().value<QCUPSSupport::PagesPerSheet>();

        QCUPSSupport::PageSet pageSet = optionsPane->options.pageSetCombo->currentData().value<QCUPSSupport::PageSet>();


        if (pagesPerSheet != QCUPSSupport::OnePagePerSheet
            && pageSet != QCUPSSupport::AllPages) {
            QMessageBox::warning(q, q->windowTitle(),
                                 QPrintDialog::tr("Options 'Pages Per Sheet' and 'Page Set' cannot be used together.\nPlease turn one of those options off."));
            return false;
        }
    }
#endif

    // Every test passed. Accept the dialog.
    return true;
}
#endif // QT_CONFIG(messagebox)

void QUnixPrintWidgetPrivate::setupPrinterProperties()
{
    if (propertiesDialog)
        delete propertiesDialog;

    QPrinter::OutputFormat outputFormat;
    QString printerName;

    if (q->isOptionEnabled(QPrintDialog::PrintToFile)
        && (widget.printers->currentIndex() == widget.printers->count() - 1)) {// PDF
        outputFormat = QPrinter::PdfFormat;
    } else {
        outputFormat = QPrinter::NativeFormat;
        printerName = widget.printers->currentText();
    }

    propertiesDialog = new QPrintPropertiesDialog(q->printer(), &m_currentPrintDevice, outputFormat, printerName, q);
    propertiesDialog->setResult(QDialog::Rejected);
    propertiesDialogShown = false;
}

void QUnixPrintWidgetPrivate::_q_btnPropertiesClicked()
{
    if (!propertiesDialog)
        setupPrinterProperties();
    propertiesDialog->exec();
    if (!propertiesDialogShown && propertiesDialog->result() == QDialog::Rejected) {
        // If properties dialog was rejected the dialog is deleted and
        // the properties are set to defaults when printer is setup
        delete propertiesDialog;
        propertiesDialog = nullptr;
        propertiesDialogShown = false;
    } else
        // properties dialog was shown and accepted
        propertiesDialogShown = true;
}

void QUnixPrintWidgetPrivate::setupPrinter()
{
    const int printerCount = widget.printers->count();
    const int index = widget.printers->currentIndex();

    if (filePrintersAdded && index == printerCount - 1) { // PDF
        printer->setPrinterName(QString());
        Q_ASSERT(index != printerCount - 2); // separator
        printer->setOutputFormat(QPrinter::PdfFormat);
        QString path = widget.filename->text();
        if (QDir::isRelativePath(path))
            path = QDir::homePath() + QDir::separator() + path;
        printer->setOutputFileName(path);
    }
    else {
        printer->setPrinterName(widget.printers->currentText());
        printer->setOutputFileName(QString());
    }

    if (!propertiesDialog)
        setupPrinterProperties();

    if (propertiesDialog->result() == QDialog::Accepted || !propertiesDialogShown)
        propertiesDialog->setupPrinter();
}

/*! \internal
*/
QUnixPrintWidget::QUnixPrintWidget(QPrinter *printer, QWidget *parent)
    : QWidget(parent), d(new QUnixPrintWidgetPrivate(this, printer))
{
    if (printer == nullptr)
        return;
    if (printer->outputFileName().isEmpty()) {
        QString home = QDir::homePath();
        QString cur = QDir::currentPath();
        if (!home.endsWith(QLatin1Char('/')))
            home += QLatin1Char('/');
        if (!cur.startsWith(home))
            cur = home;
        else if (!cur.endsWith(QLatin1Char('/')))
            cur += QLatin1Char('/');
        if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
            if (printer->docName().isEmpty()) {
                cur += QStringLiteral("print.pdf");
            } else {
                const QRegExp re(QStringLiteral("(.*)\\.\\S+"));
                if (re.exactMatch(printer->docName()))
                    cur += re.cap(1);
                else
                    cur += printer->docName();
                cur += QStringLiteral(".pdf");
            }
        } // xcb

        d->widget.filename->setText(cur);
    }
    else
        d->widget.filename->setText(printer->outputFileName());
    const QString printerName = printer->printerName();
    if (!printerName.isEmpty()) {
        const int i = d->widget.printers->findText(printerName);
        if (i >= 0)
            d->widget.printers->setCurrentIndex(i);
    }
    // PDF printer not added to the dialog yet, we'll handle those cases in QUnixPrintWidgetPrivate::updateWidget
}

/*! \internal
*/
QUnixPrintWidget::~QUnixPrintWidget()
{
    delete d;
}

/*! \internal

    Updates the printer with the states held in the QUnixPrintWidget.
*/
void QUnixPrintWidget::updatePrinter()
{
    d->setupPrinter();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPPDOptionsModel

    Holds the PPD Options for the printer.

*/

#if QT_CONFIG(cups)

QPPDOptionsModel::QPPDOptionsModel(QPrintDevice *currentPrintDevice, QObject *parent)
    : QAbstractItemModel(parent)
    , m_currentPrintDevice(currentPrintDevice)
{
    ppd_file_t *ppd = m_currentPrintDevice->property(PDPK_PpdFile).value<ppd_file_t*>();
    rootItem = new QOptionTreeItem(QOptionTreeItem::Root, 0, ppd, "Root Item", 0);

    if (ppd) {
        cupsCodec = QTextCodec::codecForName(ppd->lang_encoding);
        for (int i = 0; i < ppd->num_groups; ++i) {
            QOptionTreeItem *group = new QOptionTreeItem(QOptionTreeItem::Group, i, &ppd->groups[i], ppd->groups[i].text, rootItem);
            rootItem->childItems.append(group);
            parseGroups(group); // parse possible subgroups
            parseOptions(group); // parse options
        }
    }

    if (!cupsCodec)
        cupsCodec = QTextCodec::codecForLocale();
}

int QPPDOptionsModel::columnCount(const QModelIndex &) const
{
    return 2;
}

int QPPDOptionsModel::rowCount(const QModelIndex &parent) const
{
    QOptionTreeItem *itm;
    if (!parent.isValid())
        itm = rootItem;
    else
        itm = static_cast<QOptionTreeItem*>(parent.internalPointer());

    if (itm->type == QOptionTreeItem::Option)
        return 0;

    return itm->childItems.count();
}

QVariant QPPDOptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    QOptionTreeItem *itm = static_cast<QOptionTreeItem*>(index.internalPointer());

    switch (role) {
    case Qt::FontRole: {
        if (itm->type == QOptionTreeItem::Group){
            QFont font;
            font.setBold(true);
            return QVariant(font);
        }
        return QVariant();
    }
    break;

    case Qt::DisplayRole: {
        if (index.column() == 0)
            return cupsCodec->toUnicode(itm->description);
        else if (itm->type == QOptionTreeItem::Option && itm->selected > -1)
            return cupsCodec->toUnicode(itm->selDescription);
        else
            return QVariant();
    }
    break;

    }

    return QVariant();
}

QModelIndex QPPDOptionsModel::index(int row, int column, const QModelIndex &parent) const
{
    QOptionTreeItem *itm;
    if (!parent.isValid())
        itm = rootItem;
    else
        itm = static_cast<QOptionTreeItem*>(parent.internalPointer());

    return createIndex(row, column, itm->childItems.at(row));
}


QModelIndex QPPDOptionsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    QOptionTreeItem *itm = static_cast<QOptionTreeItem*>(index.internalPointer());

    if (itm->parentItem && itm->parentItem != rootItem)
        return createIndex(itm->parentItem->index, 0, itm->parentItem);

    return QModelIndex();
}

Qt::ItemFlags QPPDOptionsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid() || static_cast<QOptionTreeItem*>(index.internalPointer())->type == QOptionTreeItem::Group)
        return Qt::ItemIsEnabled;

    if (index.column() == 1)
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

void QPPDOptionsModel::parseGroups(QOptionTreeItem *parent)
{
    const ppd_group_t *group = static_cast<const ppd_group_t*>(parent->ptr);

    if (group) {
        for (int i = 0; i < group->num_subgroups; ++i) {
            QOptionTreeItem *subgroup = new QOptionTreeItem(QOptionTreeItem::Group, i, &group->subgroups[i], group->subgroups[i].text, parent);
            parent->childItems.append(subgroup);
            parseGroups(subgroup); // parse possible subgroups
            parseOptions(subgroup); // parse options
        }
    }
}

static bool isBlacklistedOption(const char *keyword) Q_DECL_NOTHROW
{
    // We already let the user set these options elsewhere
    const char *cupsOptionBlacklist[] = {
        "Collate",
        "Copies",
        "OutputOrder",
        "PageRegion",
        "PageSize"
    };
    auto equals = [](const char *keyword) {
        return [keyword](const char *candidate) {
            return qstrcmp(keyword, candidate) == 0;
        };
    };
    return std::any_of(std::begin(cupsOptionBlacklist), std::end(cupsOptionBlacklist), equals(keyword));
};

void QPPDOptionsModel::parseOptions(QOptionTreeItem *parent)
{
    const ppd_group_t *group = static_cast<const ppd_group_t*>(parent->ptr);
    for (int i = 0; i < group->num_options; ++i) {
        if (!isBlacklistedOption(group->options[i].keyword)) {
            QOptionTreeItem *opt = new QOptionTreeItem(QOptionTreeItem::Option, i, &group->options[i], group->options[i].text, parent);
            parent->childItems.append(opt);
            parseChoices(opt);
        }
    }
}

void QPPDOptionsModel::parseChoices(QOptionTreeItem *parent)
{
    const ppd_option_t *option = static_cast<const ppd_option_t*>(parent->ptr);
    bool marked = false;
    for (int i = 0; i < option->num_choices; ++i) {
        QOptionTreeItem *choice = new QOptionTreeItem(QOptionTreeItem::Choice, i, &option->choices[i], option->choices[i].text, parent);
        if (static_cast<int>(option->choices[i].marked) == 1) {
            parent->selected = i;
            parent->selDescription = option->choices[i].text;
            marked = true;
        } else if (!marked && qstrcmp(option->choices[i].choice, option->defchoice) == 0) {
            parent->selected = i;
            parent->selDescription = option->choices[i].text;
        }
        parent->childItems.append(choice);
    }
}

QVariant QPPDOptionsModel::headerData(int section, Qt::Orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
    case 0:
        return QVariant(tr("Name"));
    case 1:
        return QVariant(tr("Value"));
    }

    return QVariant();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*

    QPPDOptionsEditor

    Edits the PPD Options for the printer.

*/

QWidget *QPPDOptionsEditor::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option)

    if (index.column() == 1 && static_cast<QOptionTreeItem*>(index.internalPointer())->type == QOptionTreeItem::Option)
        return new QComboBox(parent);

    return nullptr;
}

void QPPDOptionsEditor::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() != 1)
        return;

    QComboBox *cb = static_cast<QComboBox*>(editor);
    QOptionTreeItem *itm = static_cast<QOptionTreeItem*>(index.internalPointer());

    if (itm->selected == -1)
        cb->addItem(QString());

    const QPPDOptionsModel *m = static_cast<const QPPDOptionsModel*>(index.model());
    for (auto *childItem : qAsConst(itm->childItems))
        cb->addItem(m->cupsCodec->toUnicode(childItem->description));

    if (itm->selected > -1)
        cb->setCurrentIndex(itm->selected);
}

void QPPDOptionsEditor::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    QComboBox *cb = static_cast<QComboBox*>(editor);
    QOptionTreeItem *itm = static_cast<QOptionTreeItem*>(index.internalPointer());

    if (itm->selected == cb->currentIndex())
        return;

    const ppd_option_t *opt = static_cast<const ppd_option_t*>(itm->ptr);
    QPPDOptionsModel *m = static_cast<QPPDOptionsModel*>(model);

    const auto values = QStringList{} << QString::fromLatin1(opt->keyword) << QString::fromLatin1(opt->choices[cb->currentIndex()].choice);
    if (m->m_currentPrintDevice->setProperty(PDPK_PpdOption, values)) {
        itm->selected = cb->currentIndex();
        itm->selDescription = static_cast<const ppd_option_t*>(itm->ptr)->choices[itm->selected].text;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // QT_CONFIG(cups)
#endif // defined (Q_OS_UNIX)

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"
#include "qprintdialog_unix.moc"
