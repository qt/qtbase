// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QPlainTextEdit>
#include <QVulkanInstance>
#include <QLibraryInfo>
#include <QLoggingCategory>
#include <QPointer>
#include "hellovulkanwidget.h"

Q_LOGGING_CATEGORY(lcVk, "qt.vulkan")

static QPointer<QPlainTextEdit> messageLogWidget;
static QtMessageHandler oldMessageHandler = nullptr;

static void messageHandler(QtMsgType msgType, const QMessageLogContext &logContext, const QString &text)
{
    if (!messageLogWidget.isNull())
        messageLogWidget->appendPlainText(text);
    if (oldMessageHandler)
        oldMessageHandler(msgType, logContext, text);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    messageLogWidget = new QPlainTextEdit(QLatin1String(QLibraryInfo::build()) + QLatin1Char('\n'));
    messageLogWidget->setReadOnly(true);

    oldMessageHandler = qInstallMessageHandler(messageHandler);

    QLoggingCategory::setFilterRules(QStringLiteral("qt.vulkan=true"));

    QVulkanInstance inst;
    inst.setLayers({ "VK_LAYER_KHRONOS_validation" });

    if (!inst.create())
        qFatal("Failed to create Vulkan instance: %d", inst.errorCode());

    VulkanWindow *vulkanWindow = new VulkanWindow;
    vulkanWindow->setVulkanInstance(&inst);

    MainWindow mainWindow(vulkanWindow, messageLogWidget.data());
    QObject::connect(vulkanWindow, &VulkanWindow::vulkanInfoReceived, &mainWindow, &MainWindow::onVulkanInfoReceived);
    QObject::connect(vulkanWindow, &VulkanWindow::frameQueued, &mainWindow, &MainWindow::onFrameQueued);

    mainWindow.resize(1024, 768);
    mainWindow.show();

    return app.exec();
}
