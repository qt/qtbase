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
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qglobal.h>
#include <QtCore/qtextcodec.h>
#include <QtGui/qevent.h>
#if QT_CONFIG(filesystemmodel)
#include <QtWidgets/qfilesystemmodel.h>
#endif
#include <QtWidgets/qstyleditemdelegate.h>
#include <QtWidgets/qformlayout.h>
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
Q_DECLARE_METATYPE(const ppd_option_t *)
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
*/

static void initResources()
{
    Q_INIT_RESOURCE(qprintdialog);
}

QT_BEGIN_NAMESPACE

class QPrintPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    QPrintPropertiesDialog(QPrinter *printer, QPrintDevice *currentPrintDevice,
                           QPrinter::OutputFormat outputFormat, const QString &printerName,
                           QAbstractPrintDialog *parent);
    ~QPrintPropertiesDialog();

    void setupPrinter() const;

private slots:
    void reject() override;
    void accept() override;

private:
    void showEvent(QShowEvent *event) override;

    friend class QUnixPrintWidgetPrivate;
#if QT_CONFIG(cups)
    QPrinter *m_printer;
#endif
    Ui::QPrintPropertiesWidget widget;
    QDialogButtonBox *m_buttons;
#if QT_CONFIG(cupsjobwidget)
    QCupsJobWidget *m_jobOptions;
#endif

#if QT_CONFIG(cups)
    bool createAdvancedOptionsWidget();
    void setPrinterAdvancedCupsOptions() const;
    void revertAdvancedOptionsToSavedValues() const;
    void advancedOptionsUpdateSavedValues() const;
    bool anyPpdOptionConflict() const;
    bool anyAdvancedOptionConflict() const;

    QPrintDevice *m_currentPrintDevice;
    QTextCodec *m_cupsCodec = nullptr;
    QVector<QComboBox*> m_advancedOptionsCombos;
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
    friend class QPrintDialog;
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

#if QT_CONFIG(cups)
    void setPpdDuplex(QPrinter::DuplexMode mode);
    ppd_option_t *m_duplexPpdOption;
#endif

private:
    QPrintDialogPrivate *optionsPane;
    bool filePrintersAdded;
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

#if QT_CONFIG(cups)
    void updatePpdDuplexOption(QRadioButton *radio);
#endif
    void setupPrinter();
    void updateWidgets();

    virtual void setTabs(const QList<QWidget*> &tabs) override;

    Ui::QPrintSettingsOutput options;
    QUnixPrintWidget *top;
    QWidget *bottom;
    QDialogButtonBox *buttons;
    QPushButton *collapseButton;
    QPrinter::OutputFormat printerOutputFormat;
private:
    void setExplicitDuplexMode(QPrint::DuplexMode duplexMode);
    // duplex mode explicitly set by user, QPrint::DuplexAuto otherwise
    QPrint::DuplexMode explicitDuplexMode;
};

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
#if QT_CONFIG(cups)
    , m_printer(printer)
#endif
{
    setWindowTitle(tr("Printer Properties"));
    QVBoxLayout *lay = new QVBoxLayout(this);
    QWidget *content = new QWidget(this);
    widget.setupUi(content);
    m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    lay->addWidget(content);
    lay->addWidget(m_buttons);

    connect(m_buttons->button(QDialogButtonBox::Ok), &QPushButton::clicked, this, &QPrintPropertiesDialog::accept);
    connect(m_buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, this, &QPrintPropertiesDialog::reject);

    widget.pageSetup->setPrinter(printer, currentPrintDevice, outputFormat, printerName);

#if QT_CONFIG(cupsjobwidget)
    m_jobOptions = new QCupsJobWidget(printer, currentPrintDevice);
    widget.tabs->insertTab(1, m_jobOptions, tr("Job Options"));
#endif

    const int advancedTabIndex = widget.tabs->indexOf(widget.cupsPropertiesPage);
#if QT_CONFIG(cups)
    m_currentPrintDevice = currentPrintDevice;
    const bool anyWidgetCreated = createAdvancedOptionsWidget();

    widget.tabs->setTabEnabled(advancedTabIndex, anyWidgetCreated);

    connect(widget.pageSetup, &QPageSetupWidget::ppdOptionChanged, this, [this] {
        widget.conflictsLabel->setVisible(anyPpdOptionConflict());
    });

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
#if QT_CONFIG(cups)
    QCUPSSupport::clearCupsOptions(m_printer);
#endif

    widget.pageSetup->setupPrinter();
#if QT_CONFIG(cupsjobwidget)
    m_jobOptions->setupPrinter();
#endif

#if QT_CONFIG(cups)
    // Set Color by default, that will change if the "ColorModel" property is available
    m_printer->setColorMode(QPrinter::Color);

    setPrinterAdvancedCupsOptions();
#endif
}

void QPrintPropertiesDialog::reject()
{
    widget.pageSetup->revertToSavedValues();

#if QT_CONFIG(cupsjobwidget)
    m_jobOptions->revertToSavedValues();
#endif

#if QT_CONFIG(cups)
    revertAdvancedOptionsToSavedValues();
#endif
    QDialog::reject();
}

void QPrintPropertiesDialog::accept()
{
#if QT_CONFIG(cups)
    if (widget.pageSetup->hasPpdConflict()) {
        widget.tabs->setCurrentWidget(widget.tabPage);
        const QMessageBox::StandardButton answer = QMessageBox::warning(this, tr("Page Setup Conflicts"),
                                                                        tr("There are conflicts in page setup options. Do you want to fix them?"),
                                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::No)
            return;
    } else if (anyAdvancedOptionConflict()) {
        widget.tabs->setCurrentWidget(widget.cupsPropertiesPage);
        const QMessageBox::StandardButton answer = QMessageBox::warning(this, tr("Advanced Option Conflicts"),
                                                                        tr("There are conflicts in some advanced options. Do you want to fix them?"),
                                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::No)
            return;
    }
    advancedOptionsUpdateSavedValues();
#endif

#if QT_CONFIG(cupsjobwidget)
    m_jobOptions->updateSavedValues();
#endif

    widget.pageSetup->updateSavedValues();

    QDialog::accept();
}

void QPrintPropertiesDialog::showEvent(QShowEvent *event)
{
#if QT_CONFIG(cups)
    widget.conflictsLabel->setVisible(anyPpdOptionConflict());
#endif
    QDialog::showEvent(event);
}

#if QT_CONFIG(cups)

// Used to store the ppd_option_t for each QComboBox that represents an advanced option
static const char *ppdOptionProperty = "_q_ppd_option";

// Used to store the originally selected choice index for each QComboBox that represents an advanced option
static const char *ppdOriginallySelectedChoiceProperty = "_q_ppd_originally_selected_choice";

// Used to store the warning label pointer for each QComboBox that represents an advanced option
static const char *warningLabelProperty = "_q_warning_label";

static bool isBlacklistedGroup(const ppd_group_t *group) noexcept
{
    return qstrcmp(group->name, "InstallableOptions") == 0;
};

static bool isBlacklistedOption(const char *keyword) noexcept
{
    // We already let the user set these options elsewhere
    const char *cupsOptionBlacklist[] = {
        "Collate",
        "Copies",
        "OutputOrder",
        "PageRegion",
        "PageSize",
        "Duplex" // handled by the main dialog
    };
    auto equals = [](const char *keyword) {
        return [keyword](const char *candidate) {
            return qstrcmp(keyword, candidate) == 0;
        };
    };
    return std::any_of(std::begin(cupsOptionBlacklist), std::end(cupsOptionBlacklist), equals(keyword));
};

bool QPrintPropertiesDialog::createAdvancedOptionsWidget()
{
    bool anyWidgetCreated = false;

    ppd_file_t *ppd = qvariant_cast<ppd_file_t*>(m_currentPrintDevice->property(PDPK_PpdFile));

    if (ppd) {
        m_cupsCodec = QTextCodec::codecForName(ppd->lang_encoding);

        QWidget *holdingWidget = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(holdingWidget);

        for (int i = 0; i < ppd->num_groups; ++i) {
            const ppd_group_t *group = &ppd->groups[i];

            if (!isBlacklistedGroup(group)) {
                QFormLayout *groupLayout = new QFormLayout();

                for (int i = 0; i < group->num_options; ++i) {
                    const ppd_option_t *option = &group->options[i];

                    if (!isBlacklistedOption(option->keyword)) {
                        QComboBox *choicesCb = new QComboBox();

                        const auto setPpdOptionFromCombo = [this, choicesCb, option] {
                            // We can't use choicesCb->currentIndex() to know the index of the option in the choices[] array
                            // because some of them may not be present in the list because they conflict with the
                            // installable options so use the index passed on addItem
                            const int selectedChoiceIndex = choicesCb->currentData().toInt();
                            const auto values = QStringList{} << QString::fromLatin1(option->keyword)
                                                                << QString::fromLatin1(option->choices[selectedChoiceIndex].choice);
                            m_currentPrintDevice->setProperty(PDPK_PpdOption, values);
                            widget.conflictsLabel->setVisible(anyPpdOptionConflict());
                        };

                        bool foundMarkedChoice = false;
                        bool markedChoiceNotAvailable = false;
                        for (int i = 0; i < option->num_choices; ++i) {
                            const ppd_choice_t *choice = &option->choices[i];
                            const auto values = QStringList{} << QString::fromLatin1(option->keyword) << QString::fromLatin1(choice->choice);
                            const bool choiceIsInstallableConflict = m_currentPrintDevice->isFeatureAvailable(PDPK_PpdChoiceIsInstallableConflict, values);
                            if (choiceIsInstallableConflict && static_cast<int>(choice->marked) == 1) {
                                markedChoiceNotAvailable = true;
                            } else if (!choiceIsInstallableConflict) {
                                choicesCb->addItem(m_cupsCodec->toUnicode(choice->text), i);
                                if (static_cast<int>(choice->marked) == 1) {
                                    choicesCb->setCurrentIndex(choicesCb->count() - 1);
                                    choicesCb->setProperty(ppdOriginallySelectedChoiceProperty, QVariant(i));
                                    foundMarkedChoice = true;
                                } else if (!foundMarkedChoice && qstrcmp(choice->choice, option->defchoice) == 0) {
                                    choicesCb->setCurrentIndex(choicesCb->count() - 1);
                                    choicesCb->setProperty(ppdOriginallySelectedChoiceProperty, QVariant(i));
                                }
                            }
                        }

                        if (markedChoiceNotAvailable) {
                            // If the user default option is not available because of it conflicting with
                            // the installed options, we need to set the internal ppd value to the value
                            // being shown in the combo
                            setPpdOptionFromCombo();
                        }

                        if (choicesCb->count() > 1) {

                            connect(choicesCb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, setPpdOptionFromCombo);

                            // We need an extra label at the end to show the conflict warning
                            QWidget *choicesCbWithLabel = new QWidget();
                            QHBoxLayout *choicesCbWithLabelLayout = new QHBoxLayout(choicesCbWithLabel);
                            choicesCbWithLabelLayout->setContentsMargins(0, 0, 0, 0);
                            QLabel *warningLabel = new QLabel();
                            choicesCbWithLabelLayout->addWidget(choicesCb);
                            choicesCbWithLabelLayout->addWidget(warningLabel);

                            QLabel *optionLabel = new QLabel(m_cupsCodec->toUnicode(option->text));
                            groupLayout->addRow(optionLabel, choicesCbWithLabel);
                            anyWidgetCreated = true;
                            choicesCb->setProperty(ppdOptionProperty, QVariant::fromValue(option));
                            choicesCb->setProperty(warningLabelProperty, QVariant::fromValue(warningLabel));
                            m_advancedOptionsCombos << choicesCb;
                        } else {
                            delete choicesCb;
                        }
                    }
                }

                if (groupLayout->rowCount() > 0) {
                    QGroupBox *groupBox = new QGroupBox(m_cupsCodec->toUnicode(group->text));
                    groupBox->setLayout(groupLayout);
                    layout->addWidget(groupBox);
                } else {
                    delete groupLayout;
                }
            }
        }

        layout->addStretch();
        widget.scrollArea->setWidget(holdingWidget);
    }

    if (!m_cupsCodec)
        m_cupsCodec = QTextCodec::codecForLocale();

    return anyWidgetCreated;
}

void QPrintPropertiesDialog::setPrinterAdvancedCupsOptions() const
{
    for (const QComboBox *choicesCb : m_advancedOptionsCombos) {
        const ppd_option_t *option = qvariant_cast<const ppd_option_t *>(choicesCb->property(ppdOptionProperty));

        // We can't use choicesCb->currentIndex() to know the index of the option in the choices[] array
        // because some of them may not be present in the list because they conflict with the
        // installable options so use the index passed on addItem
        const int selectedChoiceIndex = choicesCb->currentData().toInt();
        const char *selectedChoice = option->choices[selectedChoiceIndex].choice;

        if (qstrcmp(option->keyword, "ColorModel") == 0)
            m_printer->setColorMode(qstrcmp(selectedChoice, "Gray") == 0 ? QPrinter::GrayScale : QPrinter::Color);

        if (qstrcmp(option->defchoice, selectedChoice) != 0)
            QCUPSSupport::setCupsOption(m_printer, QString::fromLatin1(option->keyword), QString::fromLatin1(selectedChoice));
    }
}

void QPrintPropertiesDialog::revertAdvancedOptionsToSavedValues() const
{
    for (QComboBox *choicesCb : m_advancedOptionsCombos) {
        const int originallySelectedChoice = qvariant_cast<int>(choicesCb->property(ppdOriginallySelectedChoiceProperty));
        const int newComboIndexToSelect = choicesCb->findData(originallySelectedChoice);
        choicesCb->setCurrentIndex(newComboIndexToSelect);
        // The currentIndexChanged lambda takes care of resetting the ppd option
    }
    widget.conflictsLabel->setVisible(anyPpdOptionConflict());
}

void QPrintPropertiesDialog::advancedOptionsUpdateSavedValues() const
{
    for (QComboBox *choicesCb : m_advancedOptionsCombos)
        choicesCb->setProperty(ppdOriginallySelectedChoiceProperty, choicesCb->currentData());
}

bool QPrintPropertiesDialog::anyPpdOptionConflict() const
{
    // we need to execute both since besides returning true/false they update the warning icons
    const bool pageSetupConflicts = widget.pageSetup->hasPpdConflict();
    const bool advancedOptionConflicts = anyAdvancedOptionConflict();
    return pageSetupConflicts || advancedOptionConflicts;
}

bool QPrintPropertiesDialog::anyAdvancedOptionConflict() const
{
    const QIcon warning = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, nullptr);

    bool anyConflicted = false;

    for (const QComboBox *choicesCb : m_advancedOptionsCombos) {
        const ppd_option_t *option = qvariant_cast<const ppd_option_t *>(choicesCb->property(ppdOptionProperty));
        QLabel *warningLabel = qvariant_cast<QLabel *>(choicesCb->property(warningLabelProperty));
        if (option->conflicted) {
            anyConflicted = true;
            const int pixmap_size = choicesCb->sizeHint().height() * .75;
            warningLabel->setPixmap(warning.pixmap(pixmap_size, pixmap_size));
        } else {
            warningLabel->setPixmap(QPixmap());
        }
    }

    return anyConflicted;
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
    : top(nullptr), bottom(nullptr), buttons(nullptr), collapseButton(nullptr),
      explicitDuplexMode(QPrint::DuplexAuto)
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
#else
    delete options.pagesRadioButton;
    delete options.pagesLineEdit;
    options.pagesRadioButton = nullptr;
    options.pagesLineEdit = nullptr;
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

    QObject::connect(options.noDuplex, &QAbstractButton::clicked, q, [this] { setExplicitDuplexMode(QPrint::DuplexNone); });
    QObject::connect(options.duplexLong, &QAbstractButton::clicked, q, [this] { setExplicitDuplexMode(QPrint::DuplexLongSide); });
    QObject::connect(options.duplexShort, &QAbstractButton::clicked, q, [this] { setExplicitDuplexMode(QPrint::DuplexShortSide); });

#if QT_CONFIG(cups)
    QObject::connect(options.noDuplex, &QAbstractButton::toggled, q, [this] { updatePpdDuplexOption(options.noDuplex); });
    QObject::connect(options.duplexLong, &QAbstractButton::toggled, q, [this] { updatePpdDuplexOption(options.duplexLong); });
    QObject::connect(options.duplexShort, &QAbstractButton::toggled, q, [this] { updatePpdDuplexOption(options.duplexShort); });
#endif
}

// initialize printer options
void QPrintDialogPrivate::selectPrinter(const QPrinter::OutputFormat outputFormat)
{
        Q_Q(QPrintDialog);
        QPrinter *p = q->printer();
        printerOutputFormat = outputFormat;

        // printer supports duplex mode?
        const auto supportedDuplexMode = top->d->m_currentPrintDevice.supportedDuplexModes();
        options.duplexLong->setEnabled(supportedDuplexMode.contains(QPrint::DuplexLongSide));
        options.duplexShort->setEnabled(supportedDuplexMode.contains(QPrint::DuplexShortSide));

        if (p->colorMode() == QPrinter::Color)
            options.color->setChecked(true);
        else
            options.grayscale->setChecked(true);

        // keep duplex value explicitly set by user, if any, and selected printer supports it;
        // use device default otherwise
        QPrint::DuplexMode duplex;
        if (explicitDuplexMode != QPrint::DuplexAuto && supportedDuplexMode.contains(explicitDuplexMode))
            duplex = explicitDuplexMode;
        else
            duplex = top->d->m_currentPrintDevice.defaultDuplexMode();
        switch (duplex) {
        case QPrint::DuplexNone:
            options.noDuplex->setChecked(true); break;
        case QPrint::DuplexLongSide:
        case QPrint::DuplexAuto:
            options.duplexLong->setChecked(true); break;
        case QPrint::DuplexShortSide:
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

#if QT_CONFIG(cups)
        // Disable complex page ranges widget when printing to pdf
        // It doesn't work since it relies on cups to do the heavy lifting and cups
        // is not used when printing to PDF
        options.pagesRadioButton->setEnabled(outputFormat != QPrinter::PdfFormat);

        // Disable color options on main dialog if not printing to file, it will be handled by CUPS advanced dialog
        options.colorMode->setVisible(outputFormat == QPrinter::PdfFormat);
#endif
}

#if QT_CONFIG(cups)
static std::vector<std::pair<int, int>> pageRangesFromString(const QString &pagesString) noexcept
{
    std::vector<std::pair<int, int>> result;
    const QStringList items = pagesString.split(',');
    for (const QString &item : items) {
        if (item.isEmpty())
            return {};

        if (item.contains(QLatin1Char('-'))) {
            const QStringList rangeItems = item.split('-');
            if (rangeItems.count() != 2)
                return {};

            bool ok;
            const int number1 = rangeItems[0].toInt(&ok);
            if (!ok)
                return {};

            const int number2 = rangeItems[1].toInt(&ok);
            if (!ok)
                return {};

            if (number1 < 1 || number2 < 1 || number2 < number1)
                return {};

            result.push_back(std::make_pair(number1, number2));

        } else {
            bool ok;
            const int number = item.toInt(&ok);
            if (!ok)
                return {};

            if (number < 1)
                return {};

            result.push_back(std::make_pair(number, number));
        }
    }

    // check no range intersects with the next
    std::sort(result.begin(), result.end(),
              [](const std::pair<int, int> &it1, const std::pair<int, int> &it2) { return it1.first < it2.first; });
    int previousSecond = -1;
    for (auto pair : result) {
        if (pair.first <= previousSecond)
            return {};

        previousSecond = pair.second;
    }

    return result;
}

static QString stringFromPageRanges(const std::vector<std::pair<int, int>> &pageRanges) noexcept
{
    QString result;

    for (auto pair : pageRanges) {
        if (!result.isEmpty())
            result += QLatin1Char(',');

        if (pair.first == pair.second)
            result += QString::number(pair.first);
        else
            result += QStringLiteral("%1-%2").arg(pair.first).arg(pair.second);
    }

    return result;
}

static bool isValidPagesString(const QString &pagesString) noexcept
{
    if (pagesString.isEmpty())
        return false;

    auto pagesRanges = pageRangesFromString(pagesString);
    return !pagesRanges.empty();
}

void QPrintDialogPrivate::updatePpdDuplexOption(QRadioButton *radio)
{
    const bool checked = radio->isChecked();
    if (checked) {
        if (radio == options.noDuplex) top->d->setPpdDuplex(QPrinter::DuplexNone);
        else if (radio == options.duplexLong) top->d->setPpdDuplex(QPrinter::DuplexLongSide);
        else if (radio == options.duplexShort) top->d->setPpdDuplex(QPrinter::DuplexShortSide);
    }
    const bool conflict = checked && top->d->m_duplexPpdOption && top->d->m_duplexPpdOption->conflicted;
    radio->setIcon(conflict ? QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, nullptr) : QIcon());
}

#endif

void QPrintDialogPrivate::setExplicitDuplexMode(const QPrint::DuplexMode duplexMode)
{
    explicitDuplexMode = duplexMode;
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

#if QT_CONFIG(cups)
    // When printing to a device the colorMode will be set by the advanced panel
    if (p->outputFormat() == QPrinter::PdfFormat)
#endif
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
    if (options.pagesRadioButton->isChecked()) {
        auto pageRanges = pageRangesFromString(options.pagesLineEdit->text());

        p->setPrintRange(QPrinter::AllPages);
        p->setFromTo(0, 0);

        // server-side page filtering
        QCUPSSupport::setPageRange(p, stringFromPageRanges(pageRanges));
    }

    // page set
    if (p->printRange() == QPrinter::AllPages || p->printRange() == QPrinter::PageRange) {
        //If the application is selecting pages and the first page number is even then need to adjust the odd-even accordingly
        QCUPSSupport::PageSet pageSet = qvariant_cast<QCUPSSupport::PageSet>(options.pageSetCombo->itemData(options.pageSetCombo->currentIndex()));
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
    : QAbstractPrintDialog(*(new QPrintDialogPrivate), nullptr, parent)
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
    return QAbstractPrintDialog::exec();
}

void QPrintDialog::accept()
{
    Q_D(QPrintDialog);
#if QT_CONFIG(cups)
    if (d->options.pagesRadioButton->isChecked() && !isValidPagesString(d->options.pagesLineEdit->text())) {
        QMessageBox::critical(this, tr("Invalid Pages Definition"),
                              tr("%1 does not follow the correct syntax. Please use ',' to separate "
                              "ranges and pages, '-' to define ranges and make sure ranges do "
                              "not intersect with each other.").arg(d->options.pagesLineEdit->text()),
                              QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    if (d->top->d->m_duplexPpdOption && d->top->d->m_duplexPpdOption->conflicted) {
        const QMessageBox::StandardButton answer = QMessageBox::warning(this, tr("Duplex Settings Conflicts"),
                                                                        tr("There are conflicts in duplex settings. Do you want to fix them?"),
                                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::No)
            return;
    }
#endif
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
    : parent(p), propertiesDialog(nullptr), printer(prn),
#if QT_CONFIG(cups)
      m_duplexPpdOption(nullptr),
#endif
      optionsPane(nullptr), filePrintersAdded(false)
{
    q = nullptr;
    if (parent)
        q = qobject_cast<QAbstractPrintDialog*> (parent->parent());

    widget.setupUi(parent);

    int currentPrinterIndex = 0;
    QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
    if (ps) {
        const QStringList printers = ps->availablePrintDeviceIds();
        const QString defaultPrinter = ps->defaultPrintDeviceId();

        widget.printers->addItems(printers);

        const QString selectedPrinter = prn && !prn->printerName().isEmpty() ? prn->printerName() : defaultPrinter;
        const int idx = printers.indexOf(selectedPrinter);

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
    const bool printToFile = q == nullptr || q->isOptionEnabled(QPrintDialog::PrintToFile);
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
    }

#if QT_CONFIG(cups)
    m_duplexPpdOption = nullptr;
#endif

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
            printer->setOutputFormat(QPrinter::PdfFormat);
            m_currentPrintDevice = QPrintDevice();
            return;
        }
    }

    if (printer) {
        printer->setOutputFormat(QPrinter::NativeFormat);

        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        if (ps)
            m_currentPrintDevice = ps->createPrintDevice(widget.printers->itemText(index));
        else
            m_currentPrintDevice = QPrintDevice();

        printer->setPrinterName(m_currentPrintDevice.id());

        widget.location->setText(m_currentPrintDevice.location());
        widget.type->setText(m_currentPrintDevice.makeAndModel());
        if (optionsPane)
            optionsPane->selectPrinter(QPrinter::NativeFormat);
    }

#if QT_CONFIG(cups)
    m_duplexPpdOption = QCUPSSupport::findPpdOption("Duplex", &m_currentPrintDevice);
#endif
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
                                            QString(), nullptr, QFileDialog::DontConfirmOverwrite);
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
    if (propertiesDialog) {
        QCUPSSupport::PagesPerSheet pagesPerSheet = qvariant_cast<QCUPSSupport::PagesPerSheet>(propertiesDialog->widget.pageSetup->m_ui.pagesPerSheetCombo
                                                                    ->currentData());

        QCUPSSupport::PageSet pageSet = qvariant_cast<QCUPSSupport::PageSet>(optionsPane->options.pageSetCombo->currentData());


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
}

#if QT_CONFIG(cups)
void QUnixPrintWidgetPrivate::setPpdDuplex(QPrinter::DuplexMode mode)
{
    auto values = QStringList{} << QStringLiteral("Duplex");
    if (mode == QPrinter::DuplexNone) values << QStringLiteral("None");
    else if (mode == QPrinter::DuplexLongSide) values << QStringLiteral("DuplexNoTumble");
    else if (mode == QPrinter::DuplexShortSide) values << QStringLiteral("DuplexTumble");

    m_currentPrintDevice.setProperty(PDPK_PpdOption, values);
}
#endif

void QUnixPrintWidgetPrivate::_q_btnPropertiesClicked()
{
    if (!propertiesDialog)
        setupPrinterProperties();
    propertiesDialog->exec();

#if QT_CONFIG(cups)
    // update the warning icon on the duplex options if needed
    optionsPane->updatePpdDuplexOption(optionsPane->options.noDuplex);
    optionsPane->updatePpdDuplexOption(optionsPane->options.duplexLong);
    optionsPane->updatePpdDuplexOption(optionsPane->options.duplexShort);
#endif
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

#if QT_CONFIG(cups)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#endif // QT_CONFIG(cups)
#endif // defined (Q_OS_UNIX)

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"
#include "qprintdialog_unix.moc"
