// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qprintjobwidget_p.h"

#include <QtWidgets/qcheckbox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qlayout.h>
#include <QtWidgets/qheaderview.h>
#include <QtWidgets/qtablewidget.h>
#include <QtGui/qfontdatabase.h>
#include <QtCore/qdatetime.h>
#include <QtPrintSupport/qprinter.h>
#include <QtPrintSupport/qprintengine.h>

#include <kernel/qprintdevice_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal
    \class QPrintJobWidget

    A widget to add to QPrintDialog to enable extra job options
    such as Job Scheduling, Job Priority or Job Billing
    \ingroup printing
    \inmodule QtPrintSupport
 */

QPrintJobWidget::QPrintJobWidget(QPrinter *printer, QPrintDevice *printDevice, QWidget *parent)
    : QWidget(parent),
      m_printer(printer),
      m_printDevice(printDevice)
{
    m_ui.setupUi(this);

    // set all the default values
    initJobHold();
    initJobBilling();
    initJobPriority();
    initBannerPages();

    updateSavedValues();
}

QPrintJobWidget::~QPrintJobWidget()
{
}

void QPrintJobWidget::updateSavedValues()
{
    m_savedJobHoldWithTime = QPair<QByteArray,QTime>(jobHold(),jobHoldTime());
    m_savedJobBilling = jobBilling();
    m_savedPriority = jobPriority();
    m_savedJobSheets = QPair<QByteArray,QByteArray>(startBannerPage(),endBannerPage());
}

void QPrintJobWidget::revertToSavedValues()
{
    setJobHold(m_savedJobHoldWithTime.first, m_savedJobHoldWithTime.second);
    toggleJobHoldTime();
    setJobBilling(m_savedJobBilling);
    setJobPriority(m_savedPriority);
    setStartBannerPage(m_savedJobSheets.first);
    setEndBannerPage(m_savedJobSheets.second);
}

void QPrintJobWidget::setupPrinter()
{
    if (m_ui.startBannerPageCombo->isEnabled() && m_ui.endBannerPageCombo->isEnabled()) {
        m_printDevice->setProperty(QPrintDevice::PDPK_JobStartCoverPage, QVariant(startBannerPage()));
        m_printDevice->setProperty(QPrintDevice::PDPK_JobEndCoverPage, QVariant(endBannerPage()));
    }

    if (m_ui.jobHoldComboBox->isEnabled()) {
        QByteArray jobHoldVal = jobHold();
        QByteArray jobHoldUntil = jobHoldVal;
        if (jobHoldVal == "#specific#") {
            QTime holdUntilTime = jobHoldTime();
            QDateTime localDateTime = QDateTime::currentDateTime();

            if (holdUntilTime < localDateTime.time())
                localDateTime = localDateTime.addDays(1);
            localDateTime.setTime(holdUntilTime);
            jobHoldUntil = localDateTime.toUTC().time().toString(u"HH:mm").toLocal8Bit();
        }
        m_printDevice->setProperty(QPrintDevice::PDPK_JobHold, QVariant(jobHoldUntil));
    }

    if (m_ui.jobPrioritySpinBox->isEnabled())
        m_printDevice->setProperty(QPrintDevice::PDPK_JobPriority, QVariant(jobPriority()));

    if (m_ui.jobBillingLineEdit->isEnabled())
        m_printDevice->setProperty(QPrintDevice::PDPK_JobBillingInfo, QVariant(jobBilling()));
}

void QPrintJobWidget::initJobHold()
{
    if (!m_printDevice->isFeatureAvailable(QPrintDevice::PDPK_JobHold, QVariant())) {
        m_ui.jobHoldComboBox->setEnabled(false);
        return;
    }

    auto jobHold = qvariant_cast<QPrint::OptionCombo>(m_printDevice->property(QPrintDevice::PDPK_JobHold));
    for (int i = 0; i < jobHold.choices.size(); i++) {
        m_ui.jobHoldComboBox->addItem(jobHold.displayChoices[i], QVariant(jobHold.choices[i]));
    }
    m_ui.jobHoldComboBox->setCurrentIndex(jobHold.defaultChoice);

    auto specificVal = QVariant(QByteArray("#specific#"));
    if (m_printDevice->isFeatureAvailable(QPrintDevice::PDPK_JobHold, specificVal)) {
        m_ui.jobHoldComboBox->addItem(tr("Specific Time"), specificVal);
    }

    toggleJobHoldTime();
    connect(m_ui.jobHoldComboBox, &QComboBox::currentIndexChanged, this, &QPrintJobWidget::toggleJobHoldTime);
}

void QPrintJobWidget::initJobBilling()
{
    if (!m_printDevice->isFeatureAvailable(QPrintDevice::PDPK_JobBillingInfo, QVariant())) {
        m_ui.jobBillingLineEdit->setEnabled(false);
        return;
    }
    setJobBilling(m_printDevice->property(QPrintDevice::PDPK_JobBillingInfo).toString());
}

void QPrintJobWidget::initJobPriority()
{
    if (!m_printDevice->isFeatureAvailable(QPrintDevice::PDPK_JobPriority, QVariant())) {
        m_ui.jobPrioritySpinBox->setEnabled(false);
        return;
    }
    bool ok;
    int defaultPriority = m_printDevice->property(QPrintDevice::PDPK_JobPriority).toInt(&ok);
    if (ok && defaultPriority > 0)
        setJobPriority(defaultPriority);
    else
        setJobPriority(50);
}

void QPrintJobWidget::initBannerPages()
{
    if (!m_printDevice->isFeatureAvailable(QPrintDevice::PDPK_JobStartCoverPage, QVariant())) {
        m_ui.startBannerPageCombo->setEnabled(false);
    } else {
        auto startCover = qvariant_cast<QPrint::OptionCombo>(m_printDevice->property(QPrintDevice::PDPK_JobStartCoverPage));
        for (int i = 0; i < startCover.choices.size(); i++) {
            m_ui.startBannerPageCombo->addItem(startCover.displayChoices[i], QVariant(startCover.choices[i]));
        }
        m_ui.startBannerPageCombo->setCurrentIndex(startCover.defaultChoice);
    }

    if (!m_printDevice->isFeatureAvailable(QPrintDevice::PDPK_JobEndCoverPage, QVariant())) {
        m_ui.endBannerPageCombo->setEnabled(false);
    } else {
        auto endCover = qvariant_cast<QPrint::OptionCombo>(m_printDevice->property(QPrintDevice::PDPK_JobEndCoverPage));
        for (int i = 0; i < endCover.choices.size(); i++) {
            m_ui.endBannerPageCombo->addItem(endCover.displayChoices[i], QVariant(endCover.choices[i]));
        }
        m_ui.endBannerPageCombo->setCurrentIndex(endCover.defaultChoice);
    }
}

QByteArray QPrintJobWidget::jobHold() const
{
    return qvariant_cast<QByteArray>(m_ui.jobHoldComboBox->itemData(m_ui.jobHoldComboBox->currentIndex()));
}

QTime QPrintJobWidget::jobHoldTime() const
{
    return m_ui.jobHoldTimeEdit->time();
}

QString QPrintJobWidget::jobBilling() const
{
    return m_ui.jobBillingLineEdit->text();
}

int QPrintJobWidget::jobPriority() const
{
    return m_ui.jobPrioritySpinBox->value();
}

QByteArray QPrintJobWidget::startBannerPage() const
{
    return qvariant_cast<QByteArray>(m_ui.startBannerPageCombo->itemData(m_ui.startBannerPageCombo->currentIndex()));
}

QByteArray QPrintJobWidget::endBannerPage() const
{
    return qvariant_cast<QByteArray>(m_ui.endBannerPageCombo->itemData(m_ui.endBannerPageCombo->currentIndex()));
}

void QPrintJobWidget::setJobBilling(const QString &jobBilling)
{
    m_ui.jobBillingLineEdit->setText(jobBilling);
}

void QPrintJobWidget::setJobPriority(int jobPriority)
{
    m_ui.jobPrioritySpinBox->setValue(jobPriority);
}

void QPrintJobWidget::setStartBannerPage(const QByteArray &bannerPage)
{
    int index = m_ui.startBannerPageCombo->findData(QVariant(bannerPage));
    if (index > 0)
        m_ui.startBannerPageCombo->setCurrentIndex(index);
}

void QPrintJobWidget::setEndBannerPage(const QByteArray &bannerPage)
{
    int index = m_ui.endBannerPageCombo->findData(QVariant(bannerPage));
    if (index > 0)
        m_ui.endBannerPageCombo->setCurrentIndex(index);
}

void QPrintJobWidget::setJobHold(const QByteArray &jobHold, const QTime &holdUntilTime)
{
    m_ui.jobHoldComboBox->setCurrentIndex(m_ui.jobHoldComboBox->findData(QVariant(jobHold)));
    m_ui.jobHoldTimeEdit->setTime(holdUntilTime);
}

void QPrintJobWidget::toggleJobHoldTime()
{
    if (jobHold() == "#specific#")
        m_ui.jobHoldTimeEdit->setEnabled(true);
    else
        m_ui.jobHoldTimeEdit->setEnabled(false);
}

QT_END_NAMESPACE

#include "moc_qprintjobwidget_p.cpp"
