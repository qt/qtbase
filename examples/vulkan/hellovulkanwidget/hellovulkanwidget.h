// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "../shared/trianglerenderer.h"
#include <QWidget>

class VulkanWindow;

QT_BEGIN_NAMESPACE
class QTabWidget;
class QPlainTextEdit;
class QLCDNumber;
QT_END_NAMESPACE

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
