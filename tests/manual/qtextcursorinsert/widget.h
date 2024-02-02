// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WIDGET_H
#define WIDGET_H

#include <QFileDialog>
#include <QMap>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_insertMarkdownButton_clicked();
    void on_insertHtmlButton_clicked();
    void on_insertPlainButton_clicked();
    void on_plainTextCB_activated(int index);
    void on_richTextCB_activated(int index);
    void on_textEdit_cursorPositionChanged();
    void on_saveButton_clicked();
    void onSave(const QString &file);

private:
    Ui::Widget *ui;
    QMap<QString, QString> m_texts;
    QFileDialog m_fileDialog;
};
#endif // WIDGET_H
