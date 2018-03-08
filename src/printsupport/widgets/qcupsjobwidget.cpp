/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtPrintSupport module of the Qt Toolkit.
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


#include "qcupsjobwidget_p.h"

#include <QCheckBox>
#include <QDateTime>
#include <QFontDatabase>
#include <QLabel>
#include <QLayout>
#include <QTime>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPrinter>
#include <QPrintEngine>

#include <kernel/qprintdevice_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QCupsJobWidget

    A widget to add to QPrintDialog to enable extra CUPS options
    such as Job Scheduling, Job Priority or Job Billing
    \ingroup printing
    \inmodule QtPrintSupport
 */

QCupsJobWidget::QCupsJobWidget(QPrinter *printer, QPrintDevice *printDevice, QWidget *parent)
    : QWidget(parent),
      m_printer(printer),
      m_printDevice(printDevice)
{
    m_ui.setupUi(this);
    //set all the default values
    initJobHold();
    initJobBilling();
    initJobPriority();
    initBannerPages();

    updateSavedValues();
}

QCupsJobWidget::~QCupsJobWidget()
{
}

void QCupsJobWidget::setupPrinter()
{
    QCUPSSupport::setJobHold(m_printer, jobHold(), jobHoldTime());
    QCUPSSupport::setJobBilling(m_printer, jobBilling());
    QCUPSSupport::setJobPriority(m_printer, jobPriority());
    QCUPSSupport::setBannerPages(m_printer, startBannerPage(), endBannerPage());
}

void QCupsJobWidget::updateSavedValues()
{
    m_savedJobHoldWithTime = { jobHold(), jobHoldTime() };
    m_savedJobBilling = jobBilling();
    m_savedPriority = jobPriority();
    m_savedJobSheets = { startBannerPage(), endBannerPage() };
}

void QCupsJobWidget::revertToSavedValues()
{
    setJobHold(m_savedJobHoldWithTime.jobHold, m_savedJobHoldWithTime.time);
    toggleJobHoldTime();

    setJobBilling(m_savedJobBilling);

    setJobPriority(m_savedPriority);

    setStartBannerPage(m_savedJobSheets.startBannerPage);
    setEndBannerPage(m_savedJobSheets.endBannerPage);
}

void QCupsJobWidget::initJobHold()
{
    m_ui.jobHoldComboBox->addItem(tr("Print Immediately"),             QVariant::fromValue(QCUPSSupport::NoHold));
    m_ui.jobHoldComboBox->addItem(tr("Hold Indefinitely"),             QVariant::fromValue(QCUPSSupport::Indefinite));
    m_ui.jobHoldComboBox->addItem(tr("Day (06:00 to 17:59)"),          QVariant::fromValue(QCUPSSupport::DayTime));
    m_ui.jobHoldComboBox->addItem(tr("Night (18:00 to 05:59)"),        QVariant::fromValue(QCUPSSupport::Night));
    m_ui.jobHoldComboBox->addItem(tr("Second Shift (16:00 to 23:59)"), QVariant::fromValue(QCUPSSupport::SecondShift));
    m_ui.jobHoldComboBox->addItem(tr("Third Shift (00:00 to 07:59)"),  QVariant::fromValue(QCUPSSupport::ThirdShift));
    m_ui.jobHoldComboBox->addItem(tr("Weekend (Saturday to Sunday)"),  QVariant::fromValue(QCUPSSupport::Weekend));
    m_ui.jobHoldComboBox->addItem(tr("Specific Time"),                 QVariant::fromValue(QCUPSSupport::SpecificTime));

    connect(m_ui.jobHoldComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QCupsJobWidget::toggleJobHoldTime);

    QCUPSSupport::JobHoldUntilWithTime jobHoldWithTime;

    if (m_printDevice) {
        const QString jobHoldUntilString = m_printDevice->property(PDPK_CupsJobHoldUntil).toString();
        jobHoldWithTime = QCUPSSupport::parseJobHoldUntil(jobHoldUntilString);
    }

    setJobHold(jobHoldWithTime.jobHold, jobHoldWithTime.time);
    toggleJobHoldTime();
}

void QCupsJobWidget::setJobHold(QCUPSSupport::JobHoldUntil jobHold, const QTime &holdUntilTime)
{
    if (jobHold == QCUPSSupport::SpecificTime && holdUntilTime.isNull()) {
        jobHold = QCUPSSupport::NoHold;
        toggleJobHoldTime();
    }
    m_ui.jobHoldComboBox->setCurrentIndex(m_ui.jobHoldComboBox->findData(QVariant::fromValue(jobHold)));
    m_ui.jobHoldTimeEdit->setTime(holdUntilTime);
}

QCUPSSupport::JobHoldUntil QCupsJobWidget::jobHold() const
{
    return m_ui.jobHoldComboBox->itemData(m_ui.jobHoldComboBox->currentIndex()).value<QCUPSSupport::JobHoldUntil>();
}

void QCupsJobWidget::toggleJobHoldTime()
{
    if (jobHold() == QCUPSSupport::SpecificTime)
        m_ui.jobHoldTimeEdit->setEnabled(true);
    else
        m_ui.jobHoldTimeEdit->setEnabled(false);
}

QTime QCupsJobWidget::jobHoldTime() const
{
    return m_ui.jobHoldTimeEdit->time();
}

void QCupsJobWidget::initJobBilling()
{
    QString jobBilling;
    if (m_printDevice)
        jobBilling = m_printDevice->property(PDPK_CupsJobBilling).toString();

    setJobBilling(jobBilling);
}

void QCupsJobWidget::setJobBilling(const QString &jobBilling)
{
    m_ui.jobBillingLineEdit->setText(jobBilling);
}

QString QCupsJobWidget::jobBilling() const
{
    return m_ui.jobBillingLineEdit->text();
}

void QCupsJobWidget::initJobPriority()
{
    int priority = -1;
    if (m_printDevice) {
        bool ok;
        priority = m_printDevice->property(PDPK_CupsJobPriority).toInt(&ok);
        if (!ok)
            priority = -1;
    }

    if (priority < 0 || priority > 100)
        priority = 50;

    setJobPriority(priority);
}

void QCupsJobWidget::setJobPriority(int jobPriority)
{
    m_ui.jobPrioritySpinBox->setValue(jobPriority);
}

int QCupsJobWidget::jobPriority() const
{
    return m_ui.jobPrioritySpinBox->value();
}

void QCupsJobWidget::initBannerPages()
{
    m_ui.startBannerPageCombo->addItem(tr("None", "CUPS Banner page"),         QVariant::fromValue(QCUPSSupport::NoBanner));
    m_ui.startBannerPageCombo->addItem(tr("Standard", "CUPS Banner page"),     QVariant::fromValue(QCUPSSupport::Standard));
    m_ui.startBannerPageCombo->addItem(tr("Unclassified", "CUPS Banner page"), QVariant::fromValue(QCUPSSupport::Unclassified));
    m_ui.startBannerPageCombo->addItem(tr("Confidential", "CUPS Banner page"), QVariant::fromValue(QCUPSSupport::Confidential));
    m_ui.startBannerPageCombo->addItem(tr("Classified", "CUPS Banner page"),   QVariant::fromValue(QCUPSSupport::Classified));
    m_ui.startBannerPageCombo->addItem(tr("Secret", "CUPS Banner page"),       QVariant::fromValue(QCUPSSupport::Secret));
    m_ui.startBannerPageCombo->addItem(tr("Top Secret", "CUPS Banner page"),   QVariant::fromValue(QCUPSSupport::TopSecret));

    m_ui.endBannerPageCombo->addItem(tr("None", "CUPS Banner page"),         QVariant::fromValue(QCUPSSupport::NoBanner));
    m_ui.endBannerPageCombo->addItem(tr("Standard", "CUPS Banner page"),     QVariant::fromValue(QCUPSSupport::Standard));
    m_ui.endBannerPageCombo->addItem(tr("Unclassified", "CUPS Banner page"), QVariant::fromValue(QCUPSSupport::Unclassified));
    m_ui.endBannerPageCombo->addItem(tr("Confidential", "CUPS Banner page"), QVariant::fromValue(QCUPSSupport::Confidential));
    m_ui.endBannerPageCombo->addItem(tr("Classified", "CUPS Banner page"),   QVariant::fromValue(QCUPSSupport::Classified));
    m_ui.endBannerPageCombo->addItem(tr("Secret", "CUPS Banner page"),       QVariant::fromValue(QCUPSSupport::Secret));
    m_ui.endBannerPageCombo->addItem(tr("Top Secret", "CUPS Banner page"),   QVariant::fromValue(QCUPSSupport::TopSecret));

    QCUPSSupport::JobSheets jobSheets;

    if (m_printDevice) {
        const QString jobSheetsString = m_printDevice->property(PDPK_CupsJobSheets).toString();
        jobSheets = QCUPSSupport::parseJobSheets(jobSheetsString);
    }

    setStartBannerPage(jobSheets.startBannerPage);
    setEndBannerPage(jobSheets.endBannerPage);
}

void QCupsJobWidget::setStartBannerPage(const QCUPSSupport::BannerPage bannerPage)
{
    m_ui.startBannerPageCombo->setCurrentIndex(m_ui.startBannerPageCombo->findData(QVariant::fromValue(bannerPage)));
}

QCUPSSupport::BannerPage QCupsJobWidget::startBannerPage() const
{
    return m_ui.startBannerPageCombo->itemData(m_ui.startBannerPageCombo->currentIndex()).value<QCUPSSupport::BannerPage>();
}

void QCupsJobWidget::setEndBannerPage(const QCUPSSupport::BannerPage bannerPage)
{
    m_ui.endBannerPageCombo->setCurrentIndex(m_ui.endBannerPageCombo->findData(QVariant::fromValue(bannerPage)));
}

QCUPSSupport::BannerPage QCupsJobWidget::endBannerPage() const
{
    return m_ui.endBannerPageCombo->itemData(m_ui.endBannerPageCombo->currentIndex()).value<QCUPSSupport::BannerPage>();
}

QT_END_NAMESPACE
