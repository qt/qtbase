// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QtWidgets>
#include <QtConcurrent>

#include <functional>

using namespace QtConcurrent;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    const int iterations = 20;

    // Prepare the list.
    QList<int> list;
    for (int i = 0; i < iterations; ++i)
        list.append(i);

    // Create a progress dialog.
    QProgressDialog dialog;
    dialog.setLabelText(QString("Progressing using %1 thread(s)...").arg(QThread::idealThreadCount()));

    // Create a QFutureWatcher and connect signals and slots.
    QFutureWatcher<void> futureWatcher;
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
    QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
    QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
    QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);

    // Our function to compute
    std::function<void(int&)> spin = [](int &iteration) {
        const int work = 1000 * 1000 * 40;
        volatile int v = 0;
        for (int j = 0; j < work; ++j)
            ++v;

        qDebug() << "iteration" << iteration << "in thread" << QThread::currentThreadId();
    };

    // Start the computation.
    futureWatcher.setFuture(QtConcurrent::map(list, spin));

    // Display the dialog and start the event loop.
    dialog.exec();

    futureWatcher.waitForFinished();

    // Query the future to check if was canceled.
    qDebug() << "Canceled?" << futureWatcher.future().isCanceled();
}
