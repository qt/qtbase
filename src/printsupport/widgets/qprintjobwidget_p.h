// Copyright (C) 2022-2023 Gaurav Guleria <tinytrebuchet@protonmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#ifndef QPRINTJOBWIDGET_P_H
#define QPRINTJOBWIDGET_P_H

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

#include <ui_qprintjobwidget.h>

QT_BEGIN_NAMESPACE

class QString;
class QTime;
class QPrinter;
class QPrintDevice;

class QPrintJobWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QPrintJobWidget(QPrinter *printer, QPrintDevice *printDevice, QWidget *parent = nullptr);
    ~QPrintJobWidget();
    void setupPrinter();

    void updateSavedValues();
    void revertToSavedValues();

    Q_DISABLE_COPY_MOVE(QPrintJobWidget)

private Q_SLOTS:
    void toggleJobHoldTime();

private:

    QByteArray jobHold() const;
    QTime jobHoldTime() const;
    QString jobBilling() const;
    int jobPriority() const;
    QByteArray startBannerPage() const;
    QByteArray endBannerPage() const;

    void setJobHold(const QByteArray &jobHold, const QTime &holdUntilTime);
    void setJobBilling(const QString &jobBilling = QString());
    void setJobPriority(int priority = 50);
    void setStartBannerPage(const QByteArray &bannerPage);
    void setEndBannerPage(const QByteArray &bannerPage);

    void initJobHold();
    void initJobBilling();
    void initJobPriority();
    void initBannerPages();

    QPrinter *m_printer;
    QPrintDevice *m_printDevice;
    Ui::QPrintJobWidget m_ui;

    QPair<QByteArray,QTime> m_savedJobHoldWithTime;
    QString m_savedJobBilling;
    int m_savedPriority;
    QPair<QByteArray,QByteArray> m_savedJobSheets;
};

QT_END_NAMESPACE

#endif  // QPRINTJOBWIDGET_P_H
