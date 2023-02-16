// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PRIMECOUNTER_H
#define PRIMECOUNTER_H

#include <QtWidgets/qdialog.h>
#include <QtCore/qfuturewatcher.h>
#include <QtConcurrent/qtconcurrentfilter.h>
#include <QtConcurrent/qtconcurrentreducekernel.h>

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
namespace Ui {
class PrimeCounter;
}

QT_END_NAMESPACE

class PrimeCounter : public QDialog
{
    Q_OBJECT
    using Element = unsigned long long;
public:
    explicit PrimeCounter(QWidget* parent = nullptr);
    ~PrimeCounter() override;

private:
    static bool filterFunction(const Element &element);
    static void reduceFunction(Element &out, const Element &value);
    void fillElementList(unsigned int count);
    Ui::PrimeCounter* setupUi();

private slots:
    void start();
    void finish();

private:
    QList<Element> elementList;
    QFutureWatcher<Element> watcher;
    QtConcurrent::ReduceOptions currentReduceOpt;
    QElapsedTimer timer;
    QThreadPool pool;
    unsigned int stepSize;
    Ui::PrimeCounter *ui;
};

#endif //PRIMECOUNTER_H
