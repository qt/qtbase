// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QCPDBJOBWIDGET_P_H
#define QCPDBJOBWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// to version without notice, or even be removed.
//
// We mean it.
//
//

#include <QtPrintSupport/private/qtprintsupportglobal_p.h>
#include <private/qcpdb_p.h>

QT_REQUIRE_CONFIG(cpdbjobwidget);

// Use the same UI as CUPS job widget
#include <ui_qcupsjobwidget.h>

QT_BEGIN_NAMESPACE

class QString;
class QTime;
class QPrinter;
class QPrintDevice;

class QCpdbJobWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QCpdbJobWidget(QPrinter *printer, QPrintDevice *printDevice, QWidget *parent = nullptr);
    ~QCpdbJobWidget();
    void setupPrinter();

    void updateSavedValues();
    void revertToSavedValues();

    Q_DISABLE_COPY_MOVE(QCpdbJobWidget)

private Q_SLOTS:
    void toggleJobHoldTime();

private:

    QTime jobHoldTime() const;
    QString jobBilling() const;
    int jobPriority() const;

    void setJobBilling(const QString &jobBilling = QString());
    void setJobPriority(int priority = 50);

    QByteArray startBannerPage() const;
    QByteArray endBannerPage() const;
    QByteArray jobHold() const;

    void initJobHold();
    void initJobBilling();
    void initJobPriority();
    void initBannerPages();

    QPrinter *m_printer;
    Ui::QCupsJobWidget m_ui;
    cpdb_printer_obj_t *m_printerObj;
};

QT_END_NAMESPACE

#endif  // QCPDBJOBWIDGET_P_H
