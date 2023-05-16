// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef HELLOVULKANWIDGET_H
#define HELLOVULKANWIDGET_H

#include "../shared/trianglerenderer.h"
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QTabWidget)
QT_FORWARD_DECLARE_CLASS(QPlainTextEdit)
QT_FORWARD_DECLARE_CLASS(QLCDNumber)

class VulkanWindow;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(VulkanWindow *w, QPlainTextEdit *logWidget);

public slots:
    void onVulkanInfoReceived(const QString &text);
    void onFrameQueued(int colorValue);
    void onGrabRequested();

private:
    VulkanWindow *m_window;
    QTabWidget *m_infoTab;
    QPlainTextEdit *m_info;
    QLCDNumber *m_number;
};

class VulkanRenderer : public TriangleRenderer
{
public:
    VulkanRenderer(VulkanWindow *w);

    void initResources() override;
    void startNextFrame() override;
};

class VulkanWindow : public QVulkanWindow
{
    Q_OBJECT

public:
    QVulkanWindowRenderer *createRenderer() override;

signals:
    void vulkanInfoReceived(const QString &text);
    void frameQueued(int colorValue);
};

#endif // HELLOVULKANWIDGET_H
