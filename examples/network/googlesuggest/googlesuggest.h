// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef GOOGLESUGGEST_H
#define GOOGLESUGGEST_H

#include <QtWidgets>
#include <QtNetwork>
#include <QtCore>

//! [1]
class GSuggestCompletion : public QObject
{
    Q_OBJECT

public:
    explicit GSuggestCompletion(QLineEdit *parent = nullptr);
    ~GSuggestCompletion();
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void showCompletion(const QList<QString> &choices);

public slots:

    void doneCompletion();
    void preventSuggest();
    void autoSuggest();
    void handleNetworkData(QNetworkReply *networkReply);

private:
    QLineEdit *editor = nullptr;
    QTreeWidget *popup = nullptr;
    QTimer timer;
    QNetworkAccessManager networkManager;
};
//! [1]
#endif // GOOGLESUGGEST_H

