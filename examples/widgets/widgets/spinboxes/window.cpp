/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "window.h"

//! [0]
Window::Window()
{
    createSpinBoxes();
    createDateTimeEdits();
    createDoubleSpinBoxes();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(spinBoxesGroup);
    layout->addWidget(editsGroup);
    layout->addWidget(doubleSpinBoxesGroup);
    setLayout(layout);

    setWindowTitle(tr("Spin Boxes"));
}
//! [0]

//! [1]
void Window::createSpinBoxes()
{
    spinBoxesGroup = new QGroupBox(tr("Spinboxes"));

    QLabel *integerLabel = new QLabel(tr("Enter a value between "
        "%1 and %2:").arg(-20).arg(20));
    QSpinBox *integerSpinBox = new QSpinBox;
    integerSpinBox->setRange(-20, 20);
    integerSpinBox->setSingleStep(1);
    integerSpinBox->setValue(0);
//! [1]

//! [2]
    QLabel *zoomLabel = new QLabel(tr("Enter a zoom value between "
        "%1 and %2:").arg(0).arg(1000));
//! [3]
    QSpinBox *zoomSpinBox = new QSpinBox;
    zoomSpinBox->setRange(0, 1000);
    zoomSpinBox->setSingleStep(10);
    zoomSpinBox->setSuffix("%");
    zoomSpinBox->setSpecialValueText(tr("Automatic"));
    zoomSpinBox->setValue(100);
//! [2] //! [3]

//! [4]
    QLabel *priceLabel = new QLabel(tr("Enter a price between "
        "%1 and %2:").arg(0).arg(999));
    QSpinBox *priceSpinBox = new QSpinBox;
    priceSpinBox->setRange(0, 999);
    priceSpinBox->setSingleStep(1);
    priceSpinBox->setPrefix("$");
    priceSpinBox->setValue(99);
//! [4] //! [5]

    QVBoxLayout *spinBoxLayout = new QVBoxLayout;
    spinBoxLayout->addWidget(integerLabel);
    spinBoxLayout->addWidget(integerSpinBox);
    spinBoxLayout->addWidget(zoomLabel);
    spinBoxLayout->addWidget(zoomSpinBox);
    spinBoxLayout->addWidget(priceLabel);
    spinBoxLayout->addWidget(priceSpinBox);
    spinBoxesGroup->setLayout(spinBoxLayout);
}
//! [5]

//! [6]
void Window::createDateTimeEdits()
{
    editsGroup = new QGroupBox(tr("Date and time spin boxes"));

    QLabel *dateLabel = new QLabel;
    QDateEdit *dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setDateRange(QDate(2005, 1, 1), QDate(2010, 12, 31));
    dateLabel->setText(tr("Appointment date (between %0 and %1):")
                       .arg(dateEdit->minimumDate().toString(Qt::ISODate))
                       .arg(dateEdit->maximumDate().toString(Qt::ISODate)));
//! [6]

//! [7]
    QLabel *timeLabel = new QLabel;
    QTimeEdit *timeEdit = new QTimeEdit(QTime::currentTime());
    timeEdit->setTimeRange(QTime(9, 0, 0, 0), QTime(16, 30, 0, 0));
    timeLabel->setText(tr("Appointment time (between %0 and %1):")
                       .arg(timeEdit->minimumTime().toString(Qt::ISODate))
                       .arg(timeEdit->maximumTime().toString(Qt::ISODate)));
//! [7]

//! [8]
    meetingLabel = new QLabel;
    meetingEdit = new QDateTimeEdit(QDateTime::currentDateTime());
//! [8]

//! [9]
    QLabel *formatLabel = new QLabel(tr("Format string for the meeting date "
                                        "and time:"));
    QComboBox *formatComboBox = new QComboBox;
    formatComboBox->addItem("yyyy-MM-dd hh:mm:ss (zzz 'ms')");
    formatComboBox->addItem("hh:mm:ss MM/dd/yyyy");
    formatComboBox->addItem("hh:mm:ss dd/MM/yyyy");
    formatComboBox->addItem("hh:mm:ss");
    formatComboBox->addItem("hh:mm ap");
//! [9] //! [10]

    connect(formatComboBox, SIGNAL(activated(QString)),
            this, SLOT(setFormatString(QString)));
//! [10]

    setFormatString(formatComboBox->currentText());

//! [11]
    QVBoxLayout *editsLayout = new QVBoxLayout;
    editsLayout->addWidget(dateLabel);
    editsLayout->addWidget(dateEdit);
    editsLayout->addWidget(timeLabel);
    editsLayout->addWidget(timeEdit);
    editsLayout->addWidget(meetingLabel);
    editsLayout->addWidget(meetingEdit);
    editsLayout->addWidget(formatLabel);
    editsLayout->addWidget(formatComboBox);
    editsGroup->setLayout(editsLayout);
}
//! [11]

//! [12]
void Window::setFormatString(const QString &formatString)
{
    meetingEdit->setDisplayFormat(formatString);
//! [12] //! [13]
    if (meetingEdit->displayedSections() & QDateTimeEdit::DateSections_Mask) {
        meetingEdit->setDateRange(QDate(2004, 11, 1), QDate(2005, 11, 30));
        meetingLabel->setText(tr("Meeting date (between %0 and %1):")
            .arg(meetingEdit->minimumDate().toString(Qt::ISODate))
	    .arg(meetingEdit->maximumDate().toString(Qt::ISODate)));
    } else {
        meetingEdit->setTimeRange(QTime(0, 7, 20, 0), QTime(21, 0, 0, 0));
        meetingLabel->setText(tr("Meeting time (between %0 and %1):")
            .arg(meetingEdit->minimumTime().toString(Qt::ISODate))
	    .arg(meetingEdit->maximumTime().toString(Qt::ISODate)));
    }
}
//! [13]

//! [14]
void Window::createDoubleSpinBoxes()
{
    doubleSpinBoxesGroup = new QGroupBox(tr("Double precision spinboxes"));

    QLabel *precisionLabel = new QLabel(tr("Number of decimal places "
                                           "to show:"));
    QSpinBox *precisionSpinBox = new QSpinBox;
    precisionSpinBox->setRange(0, 100);
    precisionSpinBox->setValue(2);
//! [14]

//! [15]
    QLabel *doubleLabel = new QLabel(tr("Enter a value between "
        "%1 and %2:").arg(-20).arg(20));
    doubleSpinBox = new QDoubleSpinBox;
    doubleSpinBox->setRange(-20.0, 20.0);
    doubleSpinBox->setSingleStep(1.0);
    doubleSpinBox->setValue(0.0);
//! [15]

//! [16]
    QLabel *scaleLabel = new QLabel(tr("Enter a scale factor between "
        "%1 and %2:").arg(0).arg(1000.0));
    scaleSpinBox = new QDoubleSpinBox;
    scaleSpinBox->setRange(0.0, 1000.0);
    scaleSpinBox->setSingleStep(10.0);
    scaleSpinBox->setSuffix("%");
    scaleSpinBox->setSpecialValueText(tr("No scaling"));
    scaleSpinBox->setValue(100.0);
//! [16]

//! [17]
    QLabel *priceLabel = new QLabel(tr("Enter a price between "
        "%1 and %2:").arg(0).arg(1000));
    priceSpinBox = new QDoubleSpinBox;
    priceSpinBox->setRange(0.0, 1000.0);
    priceSpinBox->setSingleStep(1.0);
    priceSpinBox->setPrefix("$");
    priceSpinBox->setValue(99.99);

    connect(precisionSpinBox, SIGNAL(valueChanged(int)),
//! [17]
            this, SLOT(changePrecision(int)));

//! [18]
    QVBoxLayout *spinBoxLayout = new QVBoxLayout;
    spinBoxLayout->addWidget(precisionLabel);
    spinBoxLayout->addWidget(precisionSpinBox);
    spinBoxLayout->addWidget(doubleLabel);
    spinBoxLayout->addWidget(doubleSpinBox);
    spinBoxLayout->addWidget(scaleLabel);
    spinBoxLayout->addWidget(scaleSpinBox);
    spinBoxLayout->addWidget(priceLabel);
    spinBoxLayout->addWidget(priceSpinBox);
    doubleSpinBoxesGroup->setLayout(spinBoxLayout);
}
//! [18]

//! [19]
void Window::changePrecision(int decimals)
{
    doubleSpinBox->setDecimals(decimals);
    scaleSpinBox->setDecimals(decimals);
    priceSpinBox->setDecimals(decimals);
}
//! [19]
