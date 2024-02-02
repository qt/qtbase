// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_setTextButton_clicked();

    void on_pasteImageButton_clicked();
    void setImage(const QImage &newImage);
    void on_pasteTextButton_clicked();


    void on_copyBinaryButton_clicked();

    void on_pasteBinaryButton_clicked();

    void on_comboBox_textActivated(const QString &arg1);

    void on_pasteHtmlButton_clicked();

    void on_clearButton_clicked();

private:
    Ui::MainWindow *ui;
    QImage image;
    QClipboard *clipboard;
    bool eventFilter(QObject *obj, QEvent *event) override;

    QColor generateRandomColor();

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

};
#endif // MAINWINDOW_H
