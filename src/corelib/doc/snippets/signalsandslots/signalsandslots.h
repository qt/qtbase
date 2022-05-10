// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SIGNALSANDSLOTS_H
#define SIGNALSANDSLOTS_H

#define Counter PlainCounter

//! [0]
class Counter
{
public:
    Counter() { m_value = 0; }

    int value() const { return m_value; }
    void setValue(int value);

private:
    int m_value;
};
//! [0]

#undef Counter
#define Counter ObjectCounter

//! [1]
#include <QObject>
//! [1]

//! [2]
class Counter : public QObject
//! [2] //! [3]
{
    Q_OBJECT

public:
    Counter() { m_value = 0; }

    int value() const { return m_value; }

public slots:
    void setValue(int value);

signals:
    void valueChanged(int newValue);

private:
    int m_value;
};
//! [3]

#endif
