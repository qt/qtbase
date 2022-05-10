// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qlabel.h>
#include <QtCore/qmetaobject.h>

#include <QtNetwork/qnetworkinformation.h>

template<typename QEnum>
QString enumToString(const QEnum value)
{
    return QString::fromUtf8(QMetaEnum::fromType<QEnum>().valueToKey(int(value)));
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    using Reachability = QNetworkInformation::Reachability;
    using TransportMedium = QNetworkInformation::TransportMedium;

public:
    MainWindow() : QMainWindow(nullptr)
    {
        label->setText("hello");
        setCentralWidget(label);
    }

public slots:
    void updateReachability(Reachability newValue)
    {
        reachability = newValue;
        updateText();
    }

    void updateCaptiveState(bool newValue)
    {
        captive = newValue;
        updateText();
    }

    void updateTransportMedium(TransportMedium newValue)
    {
        transportMedium = newValue;
        updateText();
    }

    void updateMetered(bool newValue)
    {
        metered = newValue;
        updateText();
    }

private:
    void updateText()
    {
        QString str =
                QLatin1String("Reachability: %1\nBehind captive portal: %2\nTransport medium: %3"
                              "\nMetered: %4")
                        .arg(enumToString(reachability), captive ? u"true" : u"false",
                             enumToString(transportMedium), metered ? u"true" : u"false");
        label->setText(str);
    }

    QLabel *const label = new QLabel(this);
    Reachability reachability = Reachability::Unknown;
    TransportMedium transportMedium = TransportMedium::Unknown;
    bool captive = false;
    bool metered = false;
};

#endif
