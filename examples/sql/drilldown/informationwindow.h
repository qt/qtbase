// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef INFORMATIONWINDOW_H
#define INFORMATIONWINDOW_H

#include <QtWidgets>
#include <QtSql>

//! [0]
class InformationWindow : public QDialog
{
    Q_OBJECT

public:
    InformationWindow(int id, QSqlRelationalTableModel *items,
                      QWidget *parent = nullptr);

    int id() const;

signals:
    void imageChanged(int id, const QString &fileName);
//! [0]

//! [1]
private slots:
    void revert();
    void submit();
    void enableButtons(bool enable = true);
//! [1]

//! [2]
private:
    void createButtons();

    int itemId;
    QString displayedImage;

    QComboBox *imageFileEditor = nullptr;
    QLabel *itemText = nullptr;
    QTextEdit *descriptionEditor = nullptr;

    QPushButton *closeButton = nullptr;
    QPushButton *submitButton = nullptr;
    QPushButton *revertButton = nullptr;
    QDialogButtonBox *buttonBox = nullptr;

    QDataWidgetMapper *mapper = nullptr;
};
//! [2]

#endif
