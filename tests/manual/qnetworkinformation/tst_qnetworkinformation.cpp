/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifdef MOBILE
#include <QtWidgets/qapplication.h>
#include <QtWidgets/qmainwindow.h>
#include <QtWidgets/qlabel.h>
#include <QtCore/qmetaobject.h>
#else
#include <QtCore/qcoreapplication.h>
#endif
#include <QtCore/qdebug.h>
#include <QtNetwork/qnetworkinformation.h>

#ifdef MOBILE
template<typename QEnum>
QString enumToString (const QEnum value)
{
  return QString::fromUtf8(QMetaEnum::fromType<QEnum>().valueToKey(int(value)));
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow() : QMainWindow(nullptr)
    {
        label = new QLabel(this);
        label->setText("hello");
        setCentralWidget(label);
    }

    void updateReachability(QNetworkInformation::Reachability newValue)
    {
        reachability = newValue;
        updateText();
    }

    void updateCaptiveState(bool newValue)
    {
        captive = newValue;
        updateText();
    }

private:
    void updateText()
    {
        QString str =
                QLatin1String("Reachability: %1\nBehind captive portal: %2")
                        .arg(enumToString(reachability), QStringView(captive ? u"true" : u"false"));
        label->setText(str);
    }

    QLabel *label;
    QNetworkInformation::Reachability reachability;
    bool captive;
};
#endif

int main(int argc, char **argv)
{
#ifdef MOBILE
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
#else
    QCoreApplication app(argc, argv);
#endif

    if (!QNetworkInformation::load(QNetworkInformation::Feature::Reachability)) {
        qWarning("Failed to load any backend");
        qDebug() << "Backends available:" << QNetworkInformation::availableBackends().join(", ");
        return -1;
    }
    QNetworkInformation *info = QNetworkInformation::instance();
    qDebug() << "Backend loaded:" << info->backendName();
    qDebug() << "Now you can make changes to the current network connection. Qt should see the "
                "changes and notify about it.";
    QObject::connect(info, &QNetworkInformation::reachabilityChanged,
                     [&](QNetworkInformation::Reachability newStatus) {
                         qDebug() << "Updated:" << newStatus;
#ifdef MOBILE
                         window.updateReachability(newStatus);
#endif
                     });

    QObject::connect(info, &QNetworkInformation::isBehindCaptivePortalChanged,
                     [&](bool status) {
                         qDebug() << "Updated, behind captive portal:" << status;
#ifdef MOBILE
                         window.updateCaptiveState(status);
#endif
                     });

    qDebug() << "Initial reachability:" << info->reachability();
    qDebug() << "Behind captive portal:" << info->isBehindCaptivePortal();

    return app.exec();
}

#ifdef MOBILE
#include "tst_qnetworkinformation.moc"
#endif
