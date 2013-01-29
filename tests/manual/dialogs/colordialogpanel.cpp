/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
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

#include "colordialogpanel.h"

#include <QGroupBox>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
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

ColorDialogPanel::ColorDialogPanel(QWidget *parent)
    : QWidget(parent)
    , m_showAlphaChannel(new QCheckBox(tr("Show alpha channel")))
    , m_noButtons(new QCheckBox(tr("Don't display OK/Cancel buttons")))
    , m_dontUseNativeDialog(new QCheckBox(tr("Don't use native dialog")))
{
    // Options
    QGroupBox *optionsGroupBox = new QGroupBox(tr("Options"), this);
    QVBoxLayout *optionsLayout = new QVBoxLayout(optionsGroupBox);
    optionsLayout->addWidget(m_showAlphaChannel);
    optionsLayout->addWidget(m_noButtons);
    optionsLayout->addWidget(m_dontUseNativeDialog);
    optionsLayout->addStretch();

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
    mainLayout->addWidget(optionsGroupBox);
    mainLayout->addWidget(buttonsGroupBox);

    enableDeleteNonModalDialogButton();
    restoreDefaults();
}

void ColorDialogPanel::execModal()
{
    QColorDialog dialog(this);
    applySettings(&dialog);
    connect(&dialog, SIGNAL(accepted()), this, SLOT(accepted()));
    dialog.setWindowTitle(tr("Modal Color Dialog Qt %1").arg(QLatin1String(QT_VERSION_STR)));
    dialog.exec();
}

void ColorDialogPanel::showModal()
{
    if (m_modalDialog.isNull()) {
        static int  n = 0;
        m_modalDialog = new QColorDialog(this);
        m_modalDialog->setModal(true);
        connect(m_modalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        m_modalDialog->setWindowTitle(tr("Modal Color Dialog #%1 Qt %2")
                                      .arg(++n)
                                      .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteModalDialogButton();
    }
    applySettings(m_modalDialog);
    m_modalDialog->show();
}

void ColorDialogPanel::showNonModal()
{
    if (m_nonModalDialog.isNull()) {
        static int  n = 0;
        m_nonModalDialog = new QColorDialog(this);
        connect(m_nonModalDialog.data(), SIGNAL(accepted()), this, SLOT(accepted()));
        m_nonModalDialog->setWindowTitle(tr("Non-Modal Color Dialog #%1 Qt %2")
                                         .arg(++n)
                                         .arg(QLatin1String(QT_VERSION_STR)));
        enableDeleteNonModalDialogButton();
    }
    applySettings(m_nonModalDialog);
    m_nonModalDialog->show();
}

void ColorDialogPanel::deleteNonModalDialog()
{
    if (!m_nonModalDialog.isNull())
        delete m_nonModalDialog;
    enableDeleteNonModalDialogButton();
}

void ColorDialogPanel::deleteModalDialog()
{
    if (!m_modalDialog.isNull())
        delete m_modalDialog;
    enableDeleteModalDialogButton();
}

void ColorDialogPanel::accepted()
{
    const QColorDialog *d = qobject_cast<const QColorDialog *>(sender());
    Q_ASSERT(d);
    m_result.clear();
    QDebug(&m_result).nospace()
        << "Current color: " << d->currentColor()
        << "\nSelected color: " << d->selectedColor();
    QTimer::singleShot(0, this, SLOT(showAcceptedResult())); // Avoid problems with the closing (modal) dialog as parent.
}

void ColorDialogPanel::showAcceptedResult()
{
    QMessageBox::information(this, tr("Color Dialog Accepted"), m_result, QMessageBox::Ok);
}

void ColorDialogPanel::restoreDefaults()
{
    QColorDialog d;
    m_showAlphaChannel->setChecked(d.testOption(QColorDialog::ShowAlphaChannel));
    m_noButtons->setChecked(d.testOption(QColorDialog::NoButtons));
    m_dontUseNativeDialog->setChecked(d.testOption(QColorDialog::DontUseNativeDialog));
}

void ColorDialogPanel::enableDeleteNonModalDialogButton()
{
    m_deleteNonModalDialogButton->setEnabled(!m_nonModalDialog.isNull());
}

void ColorDialogPanel::enableDeleteModalDialogButton()
{
    m_deleteModalDialogButton->setEnabled(!m_modalDialog.isNull());
}

void ColorDialogPanel::applySettings(QColorDialog *d) const
{
    d->setOption(QColorDialog::ShowAlphaChannel, m_showAlphaChannel->isChecked());
    d->setOption(QColorDialog::NoButtons, m_noButtons->isChecked());
    d->setOption(QColorDialog::DontUseNativeDialog, m_dontUseNativeDialog->isChecked());
}
