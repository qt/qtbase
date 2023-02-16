// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "primecounter.h"
#include "ui_primecounter.h"

PrimeCounter::PrimeCounter(QWidget *parent)
    : QDialog(parent), stepSize(100000), ui(setupUi())
{
    // Control the concurrent operation with the QFutureWatcher
    //! [1]
    connect(ui->pushButton, &QPushButton::clicked,
            this, [this] { start(); });
    connect(&watcher, &QFutureWatcher<Element>::finished,
            this, [this] { finish(); });
    connect(&watcher, &QFutureWatcher<Element>::progressRangeChanged,
            ui->progressBar, &QProgressBar::setRange);
    connect(&watcher, &QFutureWatcher<Element>::progressValueChanged,
            ui->progressBar, &QProgressBar::setValue);
    //! [1]
}

PrimeCounter::~PrimeCounter()
{
    watcher.cancel();
    delete ui;
}

//! [3]
bool PrimeCounter::filterFunction(const Element &element)
{
    // Filter for primes
    if (element <= 1)
        return false;
    for (Element i = 2; i*i <= element; ++i) {
        if (element % i == 0)
            return false;
    }
    return true;
}
//! [3]

//! [4]
void PrimeCounter::reduceFunction(Element &out, const Element &value)
{
    // Count the amount of primes.
    Q_UNUSED(value);
    ++out;
}
//! [4]

//! [2]
void PrimeCounter::start()
{
    if (ui->pushButton->isChecked()) {
        ui->comboBox->setEnabled(false);
        ui->pushButton->setText(tr("Cancel"));
        ui->labelResult->setText(tr("Calculating ..."));
        ui->labelFilter->setText(tr("Selected Reduce Option: %1").arg(ui->comboBox->currentText()));
        fillElementList(ui->horizontalSlider->value() * stepSize);

        timer.start();
        watcher.setFuture(
            QtConcurrent::filteredReduced(
                &pool,
                elementList,
                filterFunction,
                reduceFunction,
                currentReduceOpt | QtConcurrent::SequentialReduce));
//! [2]
    } else {
        watcher.cancel();
        ui->progressBar->setValue(0);
        ui->comboBox->setEnabled(true);
        ui->labelResult->setText(tr(""));
        ui->pushButton->setText(tr("Start"));
        ui->labelFilter->setText(tr("Operation Canceled"));
    }
}

void PrimeCounter::finish()
{
    // The finished signal from the QFutureWatcher is also emitted when cancelling.
    if (watcher.isCanceled())
        return;

    auto elapsedTime = timer.elapsed();
    ui->progressBar->setValue(0);
    ui->comboBox->setEnabled(true);
    ui->pushButton->setChecked(false);
    ui->pushButton->setText(tr("Start"));
    ui->labelFilter->setText(
            tr("Filter '%1' took %2 ms to calculate").arg(ui->comboBox->currentText())
                .arg(elapsedTime));
    ui->labelResult->setText(
            tr("Found %1 primes in the range of elements").arg(watcher.result()));
}

void PrimeCounter::fillElementList(unsigned int count)
{
    // Fill elementList with values from [1, count] when starting the calculations.
    auto prevSize = elementList.size();
    if (prevSize == count)
        return; // Nothing to do here.

    auto startVal = elementList.empty() ? 1 : elementList.back() + 1;
    elementList.resize(count);
    if (elementList.begin() + prevSize < elementList.end())
        std::iota(elementList.begin() + prevSize, elementList.end(), startVal);
}

Ui::PrimeCounter* PrimeCounter::setupUi()
{
    Ui::PrimeCounter *setupUI = new Ui::PrimeCounter;
    setupUI->setupUi(this);
    setModal(true);

    // Set up the slider
    connect(setupUI->horizontalSlider, &QSlider::valueChanged,
            this, [setupUI, this] (const int &pos) {
        setupUI->labelResult->setText("");
        setupUI->labelSize->setText(tr("Elements in list: %1").arg(pos * stepSize));
    });
    setupUI->horizontalSlider->setValue(30);

    // Set up the combo box
    setupUI->comboBox->insertItem(0, tr("Unordered Reduce"), QtConcurrent::UnorderedReduce);
    setupUI->comboBox->insertItem(1, tr("Ordered Reduce"), QtConcurrent::OrderedReduce);

    auto comboBoxChange = [this, setupUI](int pos) {
        currentReduceOpt = setupUI->comboBox->itemData(pos).value<QtConcurrent::ReduceOptions>();
        setupUI->labelFilter->setText(tr("Selected Reduce Option: %1")
                                          .arg(setupUI->comboBox->currentText()));
    };
    comboBoxChange(setupUI->comboBox->currentIndex());
    connect(setupUI->comboBox, &QComboBox::currentIndexChanged, this, comboBoxChange);

    return setupUI;
}
