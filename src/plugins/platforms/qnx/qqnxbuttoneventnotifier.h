// Copyright (C) 2012 Research In Motion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQNXBUTTONSEVENTNOTIFIER_H
#define QQNXBUTTONSEVENTNOTIFIER_H

#include <QObject>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(lcQpaInputHwButton);

class QSocketNotifier;

class QQnxButtonEventNotifier : public QObject
{
    Q_OBJECT
    Q_ENUMS(ButtonId)
public:
    enum ButtonId {
        bid_minus = 0,
        bid_playpause,
        bid_plus,
        bid_power,
        ButtonCount
    };

    enum ButtonState {
        ButtonUp,
        ButtonDown
    };

    explicit QQnxButtonEventNotifier(QObject *parent = nullptr);
    ~QQnxButtonEventNotifier();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void updateButtonStates();

private:
    void close();
    bool parsePPS(const QByteArray &ppsData, QHash<QByteArray, QByteArray> *messageFields) const;

    int m_fd;
    QSocketNotifier *m_readNotifier;
    ButtonState m_state[ButtonCount];
    QList<QByteArray> m_buttonKeys;

    static const char *ppsPath;
    static const size_t ppsBufferSize;
};

QT_END_NAMESPACE

#endif // QQNXBUTTONSEVENTNOTIFIER_H
