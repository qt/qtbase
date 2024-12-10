// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef PREVIEWWINDOW_H
#define PREVIEWWINDOW_H

#include <QDialog>
#include <QRasterWindow>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

class PreviewWindow : public QRasterWindow
{
public:
    PreviewWindow(QWindow *parent = nullptr);
    void setVisualizeSafeAreas(bool enable)
    {
        m_visualizeSafeAreas = enable;
    }

protected:
    void paintEvent(QPaintEvent *event);
    bool m_visualizeSafeAreas = false;
};

class PreviewWidget : public QWidget
{
    Q_OBJECT

public:
    PreviewWidget(QWidget *parent = nullptr);

    void setWindowFlags(Qt::WindowFlags flags);

public slots:
    void updateInfo();

protected:
    bool event(QEvent *) override;

private:
    QPlainTextEdit *textEdit;
};

class PreviewDialog : public QDialog
{
    Q_OBJECT

public:
    PreviewDialog(QWidget *parent = nullptr);

    void setWindowFlags(Qt::WindowFlags flags);

public slots:
    void updateInfo();

protected:
    bool event(QEvent *) override;

private:
    QPlainTextEdit *textEdit;
};

#endif
