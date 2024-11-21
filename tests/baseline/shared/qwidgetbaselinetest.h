// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QObject>
#include <QImage>

QT_BEGIN_NAMESPACE

class QWidget;

class QWidgetBaselineTest : public QObject
{
    Q_OBJECT

public:
    QWidgetBaselineTest();

    void takeStandardSnapshots();
    QWidget *testWindow() const { return window; }

protected:
    virtual void doInit() {}
    virtual void doCleanup() {}

private slots:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();

protected:
    void makeVisible();
    QImage takeSnapshot();
    QImage takeScreenSnapshot(const QRect& rect = QRect());

private:
    QWidget *background = nullptr;
    QWidget *window = nullptr;
};

QT_END_NAMESPACE
