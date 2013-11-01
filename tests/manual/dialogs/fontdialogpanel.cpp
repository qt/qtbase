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

#include "fontdialogpanel.h"

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

static inline QPushButton *addButton(const QString &description, QVBoxLayout *layout,
                                     QObject *receiver, const char *slotFunc)
{
    QPushButton *button = new QPushButton(description);
    QObject::connect(button, SIGNAL(clicked()), receiver, slotFunc);
    layout->addWidget(button);
    return button;
}

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
    addButton(tr("Show modal"), buttonsLayout, this, SLOT(showModal()));
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

void FontDialogPanel::showModal()
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
#if QT_VERSION >= 0x050000
    d->setOption(QFontDialog::ScalableFonts, m_scalableFilter->isChecked());
    d->setOption(QFontDialog::NonScalableFonts, m_nonScalableFilter->isChecked());
    d->setOption(QFontDialog::MonospacedFonts, m_monospacedFilter->isChecked());
    d->setOption(QFontDialog::ProportionalFonts, m_proportionalFilter->isChecked());
#endif // Qt 5

    QFont font = m_fontFamilyBox->currentFont();
    font.setPointSizeF(m_fontSizeBox->value());
    d->setCurrentFont(font);
}
