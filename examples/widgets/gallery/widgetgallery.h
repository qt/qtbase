// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WIDGETGALLERY_H
#define WIDGETGALLERY_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QGroupBox;
class QProgressBar;
class QTabWidget;
class QTextBrowser;
class QToolBox;
QT_END_NAMESPACE

class WidgetGallery : public QDialog
{
    Q_OBJECT

public:
    explicit WidgetGallery(QWidget *parent = nullptr);

    void setVisible(bool visible) override;

private slots:
    void changeStyle(const QString &styleName);
    void advanceProgressBar();
    void helpOnCurrentWidget();
    void updateSystemInfo();

private:
    static QGroupBox *createButtonsGroupBox();
    static QTabWidget *createItemViewTabWidget();
    static QGroupBox *createSimpleInputWidgetsGroupBox();
    QToolBox *createTextToolBox();
    QProgressBar *createProgressBar();

    QProgressBar *progressBar;
    QTextBrowser *systemInfoTextBrowser;
};

#endif // WIDGETGALLERY_H
