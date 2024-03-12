// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
#include <QtCore/qstringconverter.h>
#include <QtGui/qevent.h>
#if QT_CONFIG(filesystemmodel)
#include <QtGui/qfilesystemmodel.h>
#endif
#include <QtWidgets/qstyleditemdelegate.h>
#include <QtWidgets/qformlayout.h>
#include <QtPrintSupport/qprinter.h>

#include <qpa/qplatformprintplugin.h>
#include <qpa/qplatformprintersupport.h>

#include <private/qprintdevice_p.h>

#include <QtWidgets/qdialogbuttonbox.h>

#if QT_CONFIG(regularexpression)
#include <qregularexpression.h>
#endif

#if QT_CONFIG(completer)
#include <private/qcompleter_p.h>
#endif
#include "ui_qprintpropertieswidget.h"
#include "ui_qprintsettingsoutput.h"
#include "ui_qprintwidget.h"


#endif
#endif

Q_DECLARE_METATYPE(const ppd_option_t *)
#endif
#endif
#include "qprintjobwidget_p.h"

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

static void _q_pdu_initResources()
{
    Q_INIT_RESOURCE(qprintdialog);
}

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

class QPrintPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    QPrintPropertiesDialog(QPrinter *printer, QPrintDevice *currentPrintDevice,
                           QPrinter::OutputFormat outputFormat, const QString &printerName,
                           QAbstractPrintDialog *parent);
    ~QPrintPropertiesDialog();

    void setupPrinter() const;

private Q_SLOTS:
    void reject() override;
    void accept() override;

private:
    void showEvent(QShowEvent *event) override;

    friend class QUnixPrintWidgetPrivate;
#endif
    QPrinter *m_printer;
#endif
    Ui::QPrintPropertiesWidget widget;
    QDialogButtonBox *m_buttons;
#endif

    bool optionBlackListed(const QByteArray &optionName) const;
    bool createAdvancedOptionsWidget();

    QSet<QByteArray> m_blackListedOptions;
    QList<QComboBox *> m_advancedOptionsCombos;
#endif

    bool createAdvancedOptionsWidget();
    void revertAdvancedOptionsToSavedValues() const;
    void advancedOptionsUpdateSavedValues() const;
    bool anyPpdOptionConflict() const;
    bool anyAdvancedOptionConflict() const;


    Ui::QPrintPropertiesWidget widget;
    QDialogButtonBox *m_buttons;
    QPrintDevice *m_currentPrintDevice;
    QList<QComboBox*> m_advancedOptionsCombos;
    QPrintJobWidget *m_jobOptions;

    QWidget* createJobOptionsWidget(QPrintDevice *currentPrintDevice);
    QWidget* createAdvancedOptionsWidget(QPrintDevice *currentPrintDevice);
    void setPrinterAdvancedOptions() const;
    void advancedOptionsUpdateSavedValues() const;
    void revertAdvancedOptionsToSavedValues() const;
    bool anyOptionConflict() const;
    bool anyAdvancedOptionConflict() const;
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
    QPrintDialog * q;
    QPrinter *printer;
    QPrintDevice m_currentPrintDevice;

    void updateWidget();

#endif

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
    friend class QUnixPrintWidgetPrivate;

    void setExplicitDuplexMode(QRadioButton *radio);
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
    , m_printerObj(nullptr)
#endif
    , m_printer(printer)
#endif
    , m_currentPrintDevice(currentPrintDevice)
    , m_jobOptions(nullptr)
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

#endif

        widget.tabs->insertTab(1, m_jobOptions, tr("Job Options"));
    }
#endif
    widget.tabs->insertTab(1, m_jobOptions, tr("Job Options"));
#endif


    m_currentPrintDevice = currentPrintDevice;
    const bool anyWidgetCreated = createAdvancedOptionsWidget();

    widget.tabs->setTabEnabled(advancedTabIndex, anyWidgetCreated);

    connect(widget.pageSetup, &QPageSetupWidget::ppdOptionChanged, this, [this] {
        widget.conflictsLabel->setVisible(anyPpdOptionConflict());
    });

    // blacklist options for advanced tab, which have been added at other places
    m_blackListedOptions
    << "borderless";

    const bool anyWidgetCreated = createAdvancedOptionsWidget();
    widget.tabs->setTabEnabled(advancedTabIndex, anyWidgetCreated);
    widget.conflictsLabel->setVisible(false);
#else
    Q_UNUSED(currentPrintDevice);
#endif
    if (currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_JobHold, QVariant())
            || currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_JobBillingInfo, QVariant())
            || currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_JobPriority, QVariant())
            || currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_JobStartCoverPage, QVariant())
            || currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_JobEndCoverPage, QVariant()))
    {
        m_jobOptions = new QPrintJobWidget(printer, currentPrintDevice, this);
        widget.tabs->insertTab(1, m_jobOptions, tr("Job Options"));
    }

    if (currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_AdvancedOptions, QVariant())) {
        widget.tabs->setTabEnabled(advancedTabIndex, true);
        widget.scrollArea->setWidget(createAdvancedOptionsWidget(currentPrintDevice));
    } else {
        widget.tabs->setTabEnabled(advancedTabIndex, false);
    }

    widget.conflictsLabel->setVisible(anyAdvancedOptionConflict());
}

QPrintPropertiesDialog::~QPrintPropertiesDialog()
{
}

void QPrintPropertiesDialog::setupPrinter() const
{
#endif

    // setup advanced options first, so as to process borderless option
#endif
    widget.pageSetup->setupPrinter();
    if (m_jobOptions)
    m_currentPrintDevice->setProperty(QPrintDevice::PDPK_AdvancedOptions, QVariant(QByteArray("#clear#")));

    widget.pageSetup->setupPrinter();

    if (m_jobOptions) {
        m_jobOptions->setupPrinter();
    }

    // Set Color by default, that will change if the "ColorModel" property is available
    m_printer->setColorMode(QPrinter::Color);

#endif
    setPrinterAdvancedOptions();
}

void QPrintPropertiesDialog::reject()
{
    widget.pageSetup->revertToSavedValues();

    if (m_jobOptions)
        m_jobOptions->revertToSavedValues();

    revertAdvancedOptionsToSavedValues();

    QDialog::reject();
}

void QPrintPropertiesDialog::accept()
{
    if (widget.pageSetup->hasPpdConflict()) {
#if QT_CONFIG(messagebox)
    if (widget.pageSetup->hasOptionConflict()) {
        widget.tabs->setCurrentWidget(widget.tabPage);
        const QMessageBox::StandardButton answer = QMessageBox::warning(this, tr("Page Setup Conflicts"),
                                                                        tr("There are conflicts in page setup options. Do you want to fix them?"),
                                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::No)
            return;
    } else if (anyAdvancedOptionConflict()) {
        const QMessageBox::StandardButton answer = QMessageBox::warning(this, tr("Advanced Option Conflicts"),
                                                                        tr("There are conflicts in some advanced options. Do you want to fix them?"),
                                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::No)
            return;
    }
    advancedOptionsUpdateSavedValues();
#endif

    if (m_jobOptions)
        m_jobOptions->updateSavedValues();
#endif

    widget.pageSetup->updateSavedValues();

    if (m_jobOptions)
        m_jobOptions->updateSavedValues();

    advancedOptionsUpdateSavedValues();

    QDialog::accept();
}

void QPrintPropertiesDialog::showEvent(QShowEvent *event)
{
    widget.conflictsLabel->setVisible(anyPpdOptionConflict());
#endif
    QDialog::showEvent(event);
}



bool QPrintPropertiesDialog::optionBlackListed(const QByteArray &optionName) const
{
    return m_blackListedOptions.contains(optionName);
}

bool QPrintPropertiesDialog::createAdvancedOptionsWidget()
{
    bool anyWidgetCreated = false;
    auto *holdingWidget = new QWidget();
    auto vboxLayout = new QVBoxLayout(holdingWidget);
    holdingWidget->setLayout(vboxLayout);

    QHash<QByteArray,QFormLayout *> groupsTable;
    auto getFormLayout = [&groupsTable, &vboxLayout] (const QByteArray &groupName, const QByteArray &displayGroup) {
        QFormLayout *formLayout = nullptr;
        if (groupsTable.find(groupName) == groupsTable.end()) {
            auto groupBox = new QGroupBox(QString::fromLocal8Bit(displayGroup));
            formLayout = new QFormLayout(groupBox);
            groupBox->setLayout(formLayout);
            vboxLayout->addWidget(groupBox);
            groupsTable[groupName] = formLayout;
        } else {
            formLayout = groupsTable[groupName];
        }
        return formLayout;
    };

    if (opts) {
        GHashTableIter iter;
        gpointer key, value;
        g_hash_table_iter_init(&iter, opts->table);
        while (g_hash_table_iter_next(&iter, &key, &value)) {
            QByteArray optionName = static_cast<char *>(key);

            if (!optionBlackListed(optionName) && opt && opt->num_supported > 1) {
                anyWidgetCreated = true;

                QByteArray groupName = opt->group_name;
                auto formLayout = getFormLayout(groupName, displayGroup);

                auto *optionLabel = new QLabel(QString::fromLocal8Bit(displayName));

                auto *choicesCb = new QComboBox();
                for (int i = 0; i < opt->num_supported; i++) {
                    QByteArray value = opt->supported_values[i];
                    choicesCb->addItem(QString::fromLocal8Bit(displayVal), QVariant::fromValue(value));
                }

                QByteArray defaultVal = opt->default_value;
                int idx = choicesCb->findData(QVariant::fromValue(defaultVal));
                if (idx >= 0)
                    choicesCb->setCurrentIndex(idx);


                m_advancedOptionsCombos << choicesCb;
                formLayout->addRow(optionLabel, choicesCb);
            }
        }

        bool supportsBorderless = true;
        for (QByteArray &optName : optNames) {
            bool found = false;
            for (int i = 0; i < opt->num_supported; i++) {
                if (qstrcmp(opt->supported_values[i], "0") == 0) {
                    found = true;
                    break;
                }
            }
            if (!found || opt->num_supported <= 1) {
                supportsBorderless = false;
                break;
            }
        }

        if (supportsBorderless) {
            anyWidgetCreated = true;

            auto formLayout = getFormLayout(groupName, displayGroup);

            QByteArray optionName = "borderless";
            QString displayName = tr("Borderless");
            auto *optionLabel = new QLabel(displayName);

            auto *choicesCb = new QComboBox();
            choicesCb->addItem(tr("On"), QVariant::fromValue(QByteArray("true")));
            choicesCb->addItem(tr("Off"), QVariant::fromValue(QByteArray("false")));
            choicesCb->setCurrentIndex(1);


            m_advancedOptionsCombos << choicesCb;
            formLayout->addRow(optionLabel, choicesCb);
        }



    }

    if (anyWidgetCreated) {
        vboxLayout->addStretch();
        widget.scrollArea->setWidget(holdingWidget);
    } else {
        delete vboxLayout;
        delete holdingWidget;
    }

    return anyWidgetCreated;
}

{
    for (const QComboBox *choicesCb : m_advancedOptionsCombos) {
        QByteArray value = qvariant_cast<QByteArray>(choicesCb->currentData());
    }
}

#endif


// Used to store the ppd_option_t for each QComboBox that represents an advanced option
static const char *ppdOptionProperty = "_q_ppd_option";
    widget.conflictsLabel->setVisible(anyAdvancedOptionConflict());
    QDialog::showEvent(event);
}

// Used to store the option name for each QComboBox that represents an advanced option
static const char *optionNameProperty = "_q_print_option_name";

// Used to store the originally selected choice index for each QComboBox that represents an advanced option
static const char *originallySelectedChoiceProperty = "_q_print_originally_selected_choice";

// Used to store the warning label pointer for each QComboBox that represents an advanced option
static const char *warningLabelProperty = "_q_warning_label";

QWidget* QPrintPropertiesDialog::createAdvancedOptionsWidget(QPrintDevice *currentPrintDevice)
{
    auto advancedOptionsGroups =
            qvariant_cast<QList<QPrint::OptionCombosGroup>>(currentPrintDevice->property(QPrintDevice::PDPK_AdvancedOptions));

static bool isBlacklistedOption(const char *keyword) noexcept
{
    // We already let the user set these options elsewhere
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
};

bool QPrintPropertiesDialog::createAdvancedOptionsWidget()
{
    bool anyWidgetCreated = false;
    auto *holdingWidget = new QWidget();
    auto vboxLayout = new QVBoxLayout(holdingWidget);
    holdingWidget->setLayout(vboxLayout);

    for (const auto& advancedOptionsGroup : advancedOptionsGroups) {
        auto groupBox = new QGroupBox(advancedOptionsGroup.displayGroup);
        auto formLayout = new QFormLayout(groupBox);
        groupBox->setLayout(formLayout);
        
        for (const auto& advancedOption : advancedOptionsGroup.options) {
            if (advancedOption.choices.size() <= 1)
                continue;
            anyWidgetCreated = true;

    if (ppd) {
        toUnicode = QStringDecoder(ppd->lang_encoding, QStringDecoder::Flag::Stateless);
        if (!toUnicode.isValid()) {
            toUnicode = QStringDecoder(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
        }
            auto *optionLabel = new QLabel(advancedOption.displayName);
            auto *choicesCb = new QComboBox();

            for (int i = 0; i < advancedOption.choices.size(); i++) {
                choicesCb->addItem(advancedOption.displayChoices[i], QVariant::fromValue(advancedOption.choices[i]));
            }
            choicesCb->setCurrentIndex(advancedOption.defaultChoice);
            widget.conflictsLabel->setVisible(anyOptionConflict());

            choicesCb->setProperty(originallySelectedChoiceProperty, choicesCb->currentData());
            choicesCb->setProperty(optionNameProperty, QVariant::fromValue(advancedOption.name));

            const auto setAdvancedOptionFromCombo = [this, choicesCb] {
                QByteArray optionName = qvariant_cast<QByteArray>(choicesCb->property(optionNameProperty));
                QByteArray optionChoice = qvariant_cast<QByteArray>(choicesCb->currentData());
                QPrint::OptionSetting optionSetting = {optionName, optionChoice};
                m_currentPrintDevice->setProperty(QPrintDevice::PDPK_AdvancedOptions, QVariant::fromValue(optionSetting));
                widget.conflictsLabel->setVisible(anyOptionConflict());
            };
            connect(choicesCb, &QComboBox::currentIndexChanged, this, setAdvancedOptionFromCombo);

            // We need an extra label at the end to show the conflict warning
            QWidget *choicesCbWithLabel = new QWidget();
            QHBoxLayout *choicesCbWithLabelLayout = new QHBoxLayout(choicesCbWithLabel);
            choicesCbWithLabelLayout->setContentsMargins(0, 0, 0, 0);
            QLabel *warningLabel = new QLabel();
            choicesCbWithLabelLayout->addWidget(choicesCb);
            choicesCbWithLabelLayout->addWidget(warningLabel);
            choicesCb->setProperty(warningLabelProperty, QVariant::fromValue(warningLabel));

            m_advancedOptionsCombos << choicesCb;
            formLayout->addRow(optionLabel, choicesCbWithLabel);
        }
        
        if (formLayout->rowCount() > 0)
            vboxLayout->addWidget(groupBox);
        else
            delete groupBox;

    }

    if (!anyWidgetCreated) {
        delete holdingWidget;
        return nullptr;
    }

    vboxLayout->addStretch();
    return holdingWidget;
}

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
void QPrintPropertiesDialog::setPrinterAdvancedOptions() const
{
    for (const QComboBox *choicesCb : m_advancedOptionsCombos) {
        QByteArray optionName = qvariant_cast<QByteArray>(choicesCb->property(optionNameProperty));
        QByteArray optionChoice = qvariant_cast<QByteArray>(choicesCb->currentData());
        QPrint::OptionSetting optionSetting = {optionName, optionChoice};
        m_currentPrintDevice->setProperty(QPrintDevice::PDPK_AdvancedOptions, QVariant::fromValue(optionSetting));
    }
}

void QPrintPropertiesDialog::revertAdvancedOptionsToSavedValues() const
{
    for (QComboBox *choicesCb : m_advancedOptionsCombos) {
        const QVariant originallySelectedChoice = choicesCb->property(originallySelectedChoiceProperty);
        const int index = choicesCb->findData(originallySelectedChoice);
        if (index > 0)
            choicesCb->setCurrentIndex(index);
    }
    widget.conflictsLabel->setVisible(anyOptionConflict());
}

void QPrintPropertiesDialog::advancedOptionsUpdateSavedValues() const
{
    for (QComboBox *choicesCb : m_advancedOptionsCombos)
        choicesCb->setProperty(originallySelectedChoiceProperty, choicesCb->currentData());
}

bool QPrintPropertiesDialog::anyOptionConflict() const
{
    // we need to execute both since besides returning true/false they update the warning icons
    const bool pageSetupConflicts = widget.pageSetup->hasOptionConflict();
    const bool advancedOptionConflicts = anyAdvancedOptionConflict();
    return pageSetupConflicts || advancedOptionConflicts;
}

bool QPrintPropertiesDialog::anyAdvancedOptionConflict() const
{
    const QIcon warning = QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, nullptr);

    bool anyConflicted = false;

    for (const QComboBox *choicesCb : m_advancedOptionsCombos) {
        QByteArray optionName = qvariant_cast<QByteArray>(choicesCb->property(optionNameProperty));
        QByteArray optionChoice = qvariant_cast<QByteArray>(choicesCb->currentData());
        QPrint::OptionSetting optionSetting = {optionName, optionChoice};

        QLabel *warningLabel = qvariant_cast<QLabel *>(choicesCb->property(warningLabelProperty));
        const bool conflict = m_currentPrintDevice->isFeatureAvailable(QPrintDevice::PDPK_OptionConflict, QVariant::fromValue(optionSetting));
        if (conflict) {
            anyConflicted = true;
            const int pixmap_size = choicesCb->sizeHint().height() * .75;
            warningLabel->setPixmap(warning.pixmap(pixmap_size, pixmap_size));
        } else {
            warningLabel->setPixmap(QPixmap());
        }
    }

    return anyConflicted;
}


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
    _q_pdu_initResources();
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
    options.color->setIcon(QIcon(":/qt-project.org/dialogs/qprintdialog/images/status-color.png"_L1));
    options.grayscale->setIconSize(QSize(32, 32));
    options.grayscale->setIcon(QIcon(":/qt-project.org/dialogs/qprintdialog/images/status-gray-scale.png"_L1));

#endif
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

    QObject::connect(options.noDuplex, &QAbstractButton::toggled, q, [this] { updatePpdDuplexOption(options.noDuplex); });
    QObject::connect(options.duplexLong, &QAbstractButton::toggled, q, [this] { updatePpdDuplexOption(options.duplexLong); });
    QObject::connect(options.duplexShort, &QAbstractButton::toggled, q, [this] { updatePpdDuplexOption(options.duplexShort); });
#endif
    QObject::connect(options.noDuplex, &QAbstractButton::clicked, q, [this] { setExplicitDuplexMode(options.noDuplex); });
    QObject::connect(options.duplexLong, &QAbstractButton::clicked, q, [this] { setExplicitDuplexMode(options.duplexLong); });
    QObject::connect(options.duplexShort, &QAbstractButton::clicked, q, [this] { setExplicitDuplexMode(options.duplexShort); });
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

    // support feature PDPK_AdvancedColorMode if you want to display Color option separately
    if (top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_AdvancedColorMode, QVariant())) {
        options.colorMode->setEnabled(false);
    } else {
        options.colorMode->setEnabled(true);
        if (p->colorMode() == QPrinter::Color)
            options.color->setChecked(true);
        else
            options.grayscale->setChecked(true);
    }

    // duplex priorities to be as follows:
    // 1) a user-selected duplex value in the dialog has highest priority
    // 2) duplex value set in the QPrinter
    QPrint::DuplexMode duplex;
    if (explicitDuplexMode != QPrint::DuplexAuto && supportedDuplexMode.contains(explicitDuplexMode))
        duplex = explicitDuplexMode;
    else
        duplex = static_cast<QPrint::DuplexMode>(p->duplex());
    switch (duplex) {
    case QPrint::DuplexNone:
        options.noDuplex->setChecked(true);
        break;
    case QPrint::DuplexLongSide:
    case QPrint::DuplexAuto:
        options.duplexLong->setChecked(true);
        break;
    case QPrint::DuplexShortSide:
        options.duplexShort->setChecked(true);
        break;
    }
    options.copies->setValue(p->copyCount());
    options.collate->setChecked(p->collateCopies());
    options.reverse->setChecked(p->pageOrder() == QPrinter::LastPageFirst);

    if (outputFormat == QPrinter::PdfFormat || options.printSelection->isChecked()
        || options.printCurrentPage->isChecked())

        options.pageSetCombo->setEnabled(false);
    else
        options.pageSetCombo->setEnabled(top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_PageSet, QVariant()));

    bool showPageSet = false;
    if (opt && opt->num_supported > 1) {
        showPageSet = true;
        options.pageSetLabel->setEnabled(true);
        options.pageSetCombo->clear();
        for (int i = 0; i < opt->num_supported; i++) {
            QByteArray value = opt->supported_values[i];
                                                                  opt->option_name,
                                                                  opt->supported_values[i]);
            options.pageSetCombo->addItem(QString::fromLocal8Bit(displayVal), QVariant::fromValue(value));
        }

        QByteArray defaultVal = opt->default_value;
        int idx = options.pageSetCombo->findData(QVariant::fromValue(defaultVal));
        if (idx >= 0)
            options.pageSetCombo->setCurrentIndex(idx);
    auto pageSetOption =
            qvariant_cast<QPrint::OptionCombo>(top->d->m_currentPrintDevice.property(QPrintDevice::PDPK_PageSet));
    for (int i = 0; i < pageSetOption.choices.size(); i++) {
        options.pageSetCombo->addItem(pageSetOption.displayChoices[i], QVariant::fromValue(pageSetOption.choices[i]));
    }
    options.pageSetCombo->setCurrentIndex(pageSetOption.defaultChoice);

    options.pagesRadioButton->setEnabled(showPageRanges);
    options.pagesLineEdit->setEnabled(showPageRanges);
#endif

    // Disable complex page ranges widget when printing to pdf
    // is not used when printing to PDF
    options.pagesRadioButton->setEnabled(outputFormat != QPrinter::PdfFormat);

    options.colorMode->setVisible(outputFormat == QPrinter::PdfFormat);
#endif
}


void QPrintDialogPrivate::updatePpdDuplexOption(QRadioButton *radio)
    options.pagesRadioButton->setEnabled(outputFormat != QPrinter::PdfFormat
            && top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_PageRange, QVariant()));
}

void QPrintDialogPrivate::setExplicitDuplexMode(QRadioButton *radio)
{
    const bool checked = radio->isChecked();
    if (checked) {
        if (radio == options.noDuplex)
            explicitDuplexMode = QPrint::DuplexNone;
        else if (radio == options.duplexLong)
            explicitDuplexMode = QPrint::DuplexLongSide;
        else if (radio == options.duplexShort)
            explicitDuplexMode = QPrint::DuplexShortSide;
        top->d->m_currentPrintDevice.setProperty(QPrintDevice::PDPK_Duplex, QVariant::fromValue(explicitDuplexMode));
    }

    const bool conflict = checked && top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_Duplex, QVariant(QByteArray("conflict")));
    radio->setIcon(conflict ? QApplication::style()->standardIcon(QStyle::SP_MessageBoxWarning, nullptr, nullptr) : QIcon());
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

    // When printing to a device the colorMode will be set by the advanced panel
    if (p->outputFormat() == QPrinter::PdfFormat)
#endif
        p->setColorMode(options.color->isChecked() ? QPrinter::Color : QPrinter::GrayScale);

    p->setPageOrder(options.reverse->isChecked() ? QPrinter::LastPageFirst : QPrinter::FirstPageFirst);

    // print range
    if (options.printAll->isChecked()) {
        p->setPrintRange(QPrinter::AllPages);
        p->setPageRanges(QPageRanges());
    } else if (options.printSelection->isChecked()) {
        p->setPrintRange(QPrinter::Selection);
        p->setPageRanges(QPageRanges());
    } else if (options.printCurrentPage->isChecked()) {
        p->setPrintRange(QPrinter::CurrentPage);
        p->setPageRanges(QPageRanges());
    } else if (options.printRange->isChecked()) {
        if (q->testOption(QPrintDialog::PrintPageRange)) {
            p->setPrintRange(QPrinter::PageRange);
            p->setFromTo(options.from->value(), qMax(options.from->value(), options.to->value()));
        } else {
            // Setting the range to the printer occurs below
            p->setPrintRange(QPrinter::AllPages);
            p->setPageRanges(QPageRanges());
        }
    }

    if (options.pagesRadioButton->isChecked()) {
        const QPageRanges ranges = QPageRanges::fromString(options.pagesLineEdit->text());
        p->setPrintRange(QPrinter::AllPages);
        p->setPageRanges(QPageRanges());

        // server-side page filtering
    }

    // page set
    if (p->printRange() == QPrinter::AllPages || p->printRange() == QPrinter::PageRange) {
        //If the application is selecting pages and the first page number is even then need to adjust the odd-even accordingly
        if (q->testOption(QPrintDialog::PrintPageRange)
            && p->printRange() == QPrinter::PageRange
            && (q->fromPage() % 2 == 0)) {

            switch (pageSet) {
                break;
                break;
                break;
            }
        }

        // server-side page range, since we set the page range on the printer to 0-0/AllPages above,
        // we need to take the values directly from the widget as q->fromPage() will return 0
        if (!q->testOption(QPrintDialog::PrintPageRange) && options.printRange->isChecked())
    }
#endif

    if (options.pagesRadioButton->isEnabled()) {
        QString pageRange;
        if (options.pagesRadioButton->isChecked()) {
            pageRange = options.pagesLineEdit->text();
            const QPageRanges ranges = QPageRanges::fromString(pageRange);
            if (!ranges.isEmpty()) {
                p->setPrintRange(QPrinter::PageRange);
                p->setPageRanges(ranges);
            }
        } else if (options.printRange->isChecked() && !q->testOption(QPrintDialog::PrintPageRange)) {
            pageRange = tr("%1-%2").arg(options.from->value()).arg(qMax(options.from->value(),options.to->value()));
        }
    }

    QByteArray pageSet = qvariant_cast<QByteArray>(options.pageSetCombo->itemData(options.pageSetCombo->currentIndex()));

    if (options.color->isChecked())
    else
#endif
    if (options.pagesRadioButton->isEnabled() && options.pagesRadioButton->isChecked()) {
        QString pageRange = options.pagesLineEdit->text();
        const QPageRanges ranges = QPageRanges::fromString(pageRange);
        if (!ranges.isEmpty()) {
            p->setPrintRange(QPrinter::PageRange);
            p->setPageRanges(ranges);
        }
        top->d->m_currentPrintDevice.setProperty(QPrintDevice::PDPK_PageRange, pageRange);
    }
    if (!q->testOption(QPrintDialog::PrintPageRange) && options.printRange->isChecked()) {
        QString pageRange = tr("%1-%2").arg(options.from->value()).arg(qMax(options.from->value(),options.to->value()));
        top->d->m_currentPrintDevice.setProperty(QPrintDevice::PDPK_PageRange, pageRange);
    }

    if (options.pageSetCombo->isEnabled()) {
        top->d->m_currentPrintDevice.setProperty(QPrintDevice::PDPK_PageSet,
                                                 options.pageSetCombo->itemData(options.pageSetCombo->currentIndex()));
    }

    if (options.colorMode->isEnabled()) {
        p->setColorMode(options.color->isChecked() ? QPrinter::Color : QPrinter::GrayScale);
    }

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
    options.gbPrintRange->setVisible(q->testOption(QPrintDialog::PrintPageRange) ||
                                     q->testOption(QPrintDialog::PrintSelection) ||
                                     q->testOption(QPrintDialog::PrintCurrentPage));

    options.printRange->setEnabled(q->testOption(QPrintDialog::PrintPageRange));
    options.printSelection->setVisible(q->testOption(QPrintDialog::PrintSelection));
    options.printCurrentPage->setVisible(q->testOption(QPrintDialog::PrintCurrentPage));

    options.collate->setVisible(q->testOption(QPrintDialog::PrintCollateCopies));

    bool showPageSet = (opt && opt->num_supported > 1);
    options.pageSetLabel->setVisible(showPageSet);
    options.pageSetCombo->setVisible(showPageSet);
#endif
    bool showPageSet = top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_PageSet, QVariant());
    // Don't display Page Set if only Selection or Current Page are enabled
    if (!q->testOption(QPrintDialog::PrintPageRange)
        && (q->testOption(QPrintDialog::PrintSelection) || q->testOption(QPrintDialog::PrintCurrentPage))) {
        options.pageSetCombo->setVisible(false);
        options.pageSetLabel->setVisible(false);
    } else {
        options.pageSetCombo->setVisible(showPageSet);
        options.pageSetLabel->setVisible(showPageSet);
    }

    // If the print device can handle server side pages selection,
    // display the page range widgets
    if (!q->testOption(QPrintDialog::PrintPageRange)
            && top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_PageRange, QVariant())) {
        options.gbPrintRange->setVisible(true);
        options.printRange->setEnabled(true);
    }

    // Don't display Page Set if only Selection or Current Page are enabled
    if (!q->testOption(QPrintDialog::PrintPageRange)
        && (q->testOption(QPrintDialog::PrintSelection) || q->testOption(QPrintDialog::PrintCurrentPage))) {
        options.pageSetCombo->setVisible(false);
        options.pageSetLabel->setVisible(false);
    } else {
        options.pageSetCombo->setVisible(true);
        options.pageSetLabel->setVisible(true);
    }

    if (!q->testOption(QPrintDialog::PrintPageRange)) {
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
        if (q->testOption(QPrintDialog::PrintCurrentPage)) {
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
    if (d->options.pagesRadioButton->isChecked()) {
        const QString rangesText = d->options.pagesLineEdit->text();
        if (rangesText.isEmpty() || QPageRanges::fromString(rangesText).isEmpty()) {
            QMessageBox::critical(this, tr("Invalid Pages Definition"),
                                  tr("%1 does not follow the correct syntax. Please use ',' to separate "
                                     "ranges and pages, '-' to define ranges and make sure ranges do "
                                     "not intersect with each other.").arg(rangesText),
                                  QMessageBox::Ok, QMessageBox::Ok);
            return;
        }
    d->setupPrinter();

#if QT_CONFIG(messagebox)
    if (d->options.pagesRadioButton->isChecked() && printer()->pageRanges().isEmpty()) {
        QMessageBox::critical(this, tr("Invalid Pages Definition"),
                              tr("%1 does not follow the correct syntax. Please use ',' to separate "
                              "ranges and pages, '-' to define ranges and make sure ranges do "
                              "not intersect with each other.").arg(d->options.pagesLineEdit->text()),
                              QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
#endif

    if (d->top->d->m_duplexPpdOption && d->top->d->m_duplexPpdOption->conflicted) {
    if (qvariant_cast<bool>(d->top->d->m_currentPrintDevice.isFeatureAvailable(QPrintDevice::PDPK_Duplex, QVariant(QByteArray("conflict"))))) {
        const QMessageBox::StandardButton answer = QMessageBox::warning(this, tr("Duplex Settings Conflicts"),
                                                                        tr("There are conflicts in duplex settings. Do you want to fix them?"),
                                                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (answer != QMessageBox::No)
            return;
    }
#endif

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
      m_printerObj(nullptr),
#endif
      m_duplexPpdOption(nullptr),
#endif
      optionsPane(nullptr), filePrintersAdded(false)
{
    q = nullptr;
    if (parent)
        q = qobject_cast<QPrintDialog*> (parent->parent());

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
    const bool printToFile = q == nullptr || q->testOption(QPrintDialog::PrintToFile);

    if (printToFile && !filePrintersAdded) {
        if (widget.printers->count())
            widget.printers->insertSeparator(widget.printers->count());
        widget.printers->addItem(QPrintDialog::tr("Print to File (PDF)"));
        filePrintersAdded = true;
        if (widget.printers->count() == 1)
            _q_printerChanged(0);
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

    if (q)
        widget.properties->setVisible(q->testOption(QAbstractPrintDialog::PrintShowPageSize));
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

    m_printerObj = nullptr;
#endif
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
            printer->setOutputFormat(QPrinter::PdfFormat);
            m_currentPrintDevice = QPrintDevice();
            if (optionsPane)
                optionsPane->selectPrinter(QPrinter::PdfFormat);
            return;
        }
    }

    if (printer) {
        printer->setOutputFormat(QPrinter::NativeFormat);

        QPlatformPrinterSupport *ps = QPlatformPrinterSupportPlugin::get();
        if (ps) {
            m_currentPrintDevice = ps->createPrintDevice(widget.printers->itemText(index));
#endif
        }
        else
            m_currentPrintDevice = QPrintDevice();

        printer->setPrinterName(m_currentPrintDevice.id());

        widget.location->setText(m_currentPrintDevice.location());
        widget.type->setText(m_currentPrintDevice.makeAndModel());
        if (optionsPane)
            optionsPane->selectPrinter(QPrinter::NativeFormat);
    }

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

    if (propertiesDialog) {
                                                                    ->currentData());



            QMessageBox::warning(q, q->windowTitle(),
                                 QPrintDialog::tr("Options 'Pages Per Sheet' and 'Page Set' cannot be used together.\nPlease turn one of those options off."));
        QVariant conflict = m_currentPrintDevice.property(QPrintDevice::PDPK_OptionConflict);
        if (!conflict.isNull()) {
            QMessageBox::warning(q, q->windowTitle(), conflict.toString());
            return false;
        }
    }

    // Every test passed. Accept the dialog.
    return true;
}
#endif // QT_CONFIG(messagebox)

void QUnixPrintWidgetPrivate::setupPrinterProperties()
{
    delete propertiesDialog;

    QPrinter::OutputFormat outputFormat;
    QString printerName;

    if (q->testOption(QPrintDialog::PrintToFile)
        && (widget.printers->currentIndex() == widget.printers->count() - 1)) {// PDF
        outputFormat = QPrinter::PdfFormat;
    } else {
        outputFormat = QPrinter::NativeFormat;
        printerName = widget.printers->currentText();
    }

    propertiesDialog = new QPrintPropertiesDialog(q->printer(), &m_currentPrintDevice, outputFormat, printerName, q);
}

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

    // update the warning icon on the duplex options if needed
    optionsPane->updatePpdDuplexOption(optionsPane->options.noDuplex);
    optionsPane->updatePpdDuplexOption(optionsPane->options.duplexLong);
    optionsPane->updatePpdDuplexOption(optionsPane->options.duplexShort);
#endif
    optionsPane->setExplicitDuplexMode(optionsPane->options.noDuplex);
    optionsPane->setExplicitDuplexMode(optionsPane->options.duplexLong);
    optionsPane->setExplicitDuplexMode(optionsPane->options.duplexShort);
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
        if (!home.endsWith(u'/'))
            home += u'/';
        if (!cur.startsWith(home))
            cur = home;
        else if (!cur.endsWith(u'/'))
            cur += u'/';
        if (QGuiApplication::platformName() == "xcb"_L1) {
            if (printer->docName().isEmpty()) {
                cur += "print.pdf"_L1;
            } else {
#if QT_CONFIG(regularexpression)
                const QRegularExpression re(QStringLiteral("(.*)\\.\\S+"));
                auto match = re.match(printer->docName());
                if (match.hasMatch())
                    cur += match.captured(1);
                else
#endif
                    cur += printer->docName();
                cur += ".pdf"_L1;
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

#endif // defined (Q_OS_UNIX)

QT_END_NAMESPACE

#include "moc_qprintdialog.cpp"
#include "qprintdialog_unix.moc"
