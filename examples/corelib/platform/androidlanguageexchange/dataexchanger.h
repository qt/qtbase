// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DATAEXCHANGER_H
#define DATAEXCHANGER_H

#include <QObject>
#include <QString>

//! [Qt DataExchanger Class]
// A simple data exchanger. All this class does is convey semi-abstract data
// back and forth. For the purposes of this example, treat it as an opaque
// and not necessarily modifiable engine object that just happens to have
// a signal that's directly consumable by the code in other languages. The
// rest of the glue is done in OtherLanguageHandler.
class DataExchanger : public QObject
{
    Q_OBJECT
public:
    explicit DataExchanger(QObject *parent = nullptr);

    enum OtherLanguageType {Java, Kotlin};
signals:
    void fromCpp(OtherLanguageType type, const QString& str);
    void fromOther(const QString& str);
};
//! [Qt DataExchanger Class]

#endif // DATAEXCHANGER_H
