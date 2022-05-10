// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SPLITTER_H
#define SPLITTER_H

#include <QLinearGradient>
#include <QSplitter>
#include <QSplitterHandle>

class QPaintEvent;

//! [0]
class Splitter : public QSplitter
{
public:
    Splitter(Qt::Orientation orientation, QWidget *parent = nullptr);

protected:
    QSplitterHandle *createHandle() override;
};
//! [0]

class SplitterHandle : public QSplitterHandle
{
public:
    SplitterHandle(Qt::Orientation orientation, QSplitter *parent);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLinearGradient gradient;
};

#endif
