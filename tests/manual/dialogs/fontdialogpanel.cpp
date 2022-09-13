// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fontdialogpanel.h"
#include "utils.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFontComboBox>
#include <QDoubleSpinBox>
#include <QTimer>
#include <QDebug>

FontDialogPanel::FontDialogPanel(QWidget *parent)
    : QWidget(parent)
    , m_fontFamilyBox(new QFontComboBox)
    , m_fontSizeBox(new QDoubleSpinBox)
    , m_noButtons(new QCheckBox(tr("Don't display OK/Cancel buttons")))
    , m_dontUseNativeDialog(new QCheckBox(tr("Don't use native dialog")))
    , m_scalableFilter(new QCheckBox(tr("Filter scalable fonts")))
    , m_nonScalableFilter(new QCheckBox(tr("Filter non scalable fonts")))
    , m_monospacedFilter(new QCheckBox(tr("Filter monospaced fonts")))
    , m_proportionalFilter(new QCheckBox(tr("Filter proportional fonts")))
{
    // Options
    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);
    optionsLayout->addWidget(m_noButtons);
    optionsLayout->addWidget(m_dontUseNativeDialog);
    optionsLayout->addWidget(m_scalableFilter);
    optionsLayout->addWidget(m_nonScalableFilter);
    optionsLayout->addWidget(m_monospacedFilter);
    optionsLayout->addWidget(m_proportionalFilter);

    // Font
    QGroupBox *fontGroupBox = new QGroupBox(tr("Font"), this);
    QHBoxLayout *fontLayout = new QHBoxLayout(fontGroupBox);
    fontLayout->addWidget(m_fontFamilyBox);
    fontLayout->addWidget(m_fontSizeBox);
    m_fontSizeBox->setValue(QFont().pointSizeF());

    // Buttons
    QGroupBox *buttonsGroupBox = new QGroupBox(tr("Show"));
    QVBoxLayout *buttonsLayout = new QVBoxLayout(buttonsGroupBox);
    addButton(tr("Exec modal"), buttonsLayout, this, SLOT(execModal()));
    addButton(tr("Show application modal"), buttonsLayout,
              [this]() { showModal(Qt::ApplicationModal); });
    addButton(tr("Show window modal"), buttonsLayout, [this]() { showModal(Qt::WindowModal); });
    m_deleteModalDialogButton =
        addButton(tr("Delete modal"), buttonsLayout, this, SLOT(deleteModalDialog()));
    addButton(tr("Show non-modal"), buttonsLayout, this, SLOT(showNonModal()));
    m_deleteNonModalDialogButton =
        addButton(tr("Delete non-modal"), buttonsLayout, this, SLOT(deleteNonModalDialog()));
    addButton(tr("Restore defaults"), buttonsLayout, this, SLOT(restoreDefaults()));
    buttonsLayout->addStretch();

    // Main layout
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QVBoxLayout *leftLayout = new QVBoxLayout;
    leftLayout->addWidget(optionsGroupBox);
    leftLayout->addWidget(fontGroupBox);
    leftLayout->addStretch();
    mainLayout->addLayout(leftLayout);
    mainLayout->addWidget(buttonsGroupBox);

    enableDeleteModalDialogButton();
    enableDeleteNonModalDialogButton();
    restoreDefaults();
}

void FontDialogPanel::execModal()
{
    QFontDialog dialog(this);
    applySettings(&dialog);
    connect(&dialog, SIGNAL(accepted()), this, SLOT(accepted()));
    dialog.setWindowTitle(tr("Modal Font Dialog Qt %1").arg(QLatin1String(QT_VERSION_STR)));
    dialog.exec();
}

void FontDialogPanel::showModal(Qt::WindowModality modality)
{
    if (m_modalDialog.isNull()) {
        static int  n = 0;
        m_modalDialog = new QFontDialog(this);
        m_modalDialog->setModal(true);
        connect(m_modalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        m_modalDialog->setWindowTitle(tr("Modal Font Dialog #%1 Qt %2")
                                      .arg(++n)
                                      .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteModalDialogButton();
    }
    m_modalDialog->setWindowModality(modality);
    applySettings(m_modalDialog);
    m_modalDialog->show();
}

void FontDialogPanel::showNonModal()
{
    if (m_nonModalDialog.isNull()) {
        static int  n = 0;
        m_nonModalDialog = new QFontDialog(this);
        connect(m_nonModalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        m_nonModalDialog->setWindowTitle(tr("Non-Modal Font Dialog #%1 Qt %2")
                                         .arg(++n)
                                         .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteNonModalDialogButton();
    }
    applySettings(m_nonModalDialog);
    m_nonModalDialog->show();
}

void FontDialogPanel::deleteNonModalDialog()
{
    if (!m_nonModalDialog.isNull())
        delete m_nonModalDialog;
    enableDeleteNonModalDialogButton();
}

void FontDialogPanel::deleteModalDialog()
{
    if (!m_modalDialog.isNull())
        delete m_modalDialog;
    enableDeleteModalDialogButton();
}

void FontDialogPanel::accepted()
{
    const QFontDialog *d = qobject_cast<const QFontDialog *>(sender());
    Q_ASSERT(d);
    m_result.clear();
    QDebug(&m_result).nospace()
        << "Current font: " << d->currentFont()
        << "\nSelected font: " << d->selectedFont();
    QTimer::singleShot(0, this, SLOT(showAcceptedResult())); // Avoid problems with the closing (modal) dialog as parent.
}

void FontDialogPanel::showAcceptedResult()
{
    QMessageBox::information(this, tr("Color Dialog Accepted"), m_result, QMessageBox::Ok);
}

void FontDialogPanel::restoreDefaults()
{
    QFontDialog d;
    m_noButtons->setChecked(d.testOption(QFontDialog::NoButtons));
    m_dontUseNativeDialog->setChecked(d.testOption(QFontDialog::DontUseNativeDialog));
    m_fontFamilyBox->setCurrentFont(QFont());
    m_fontSizeBox->setValue(QFont().pointSizeF());
}

void FontDialogPanel::enableDeleteNonModalDialogButton()
{
    m_deleteNonModalDialogButton->setEnabled(!m_nonModalDialog.isNull());
}

void FontDialogPanel::enableDeleteModalDialogButton()
{
    m_deleteModalDialogButton->setEnabled(!m_modalDialog.isNull());
}

void FontDialogPanel::applySettings(QFontDialog *d) const
{
    d->setOption(QFontDialog::NoButtons, m_noButtons->isChecked());
    d->setOption(QFontDialog::DontUseNativeDialog, m_dontUseNativeDialog->isChecked());
    d->setOption(QFontDialog::ScalableFonts, m_scalableFilter->isChecked());
    d->setOption(QFontDialog::NonScalableFonts, m_nonScalableFilter->isChecked());
    d->setOption(QFontDialog::MonospacedFonts, m_monospacedFilter->isChecked());
    d->setOption(QFontDialog::ProportionalFonts, m_proportionalFilter->isChecked());

    QFont font = m_fontFamilyBox->currentFont();
    font.setPointSizeF(m_fontSizeBox->value());
    d->setCurrentFont(font);
}
