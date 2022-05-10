// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CONTROLLERWINDOW_H
#define CONTROLLERWINDOW_H

#include <QPlainTextEdit>

#include "previewwindow.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QMainWindow;
QT_END_NAMESPACE

class HintControl;
class WindowStatesControl;
class TypeControl;

class ControllerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControllerWidget(QWidget *parent = nullptr);

    virtual bool eventFilter(QObject *o, QEvent *e);

private slots:
    void updatePreview();
    void updateStateControl();

private:
    void updatePreview(QWindow *);
    void updatePreview(QWidget *);
    void createTypeGroupBox();
    QCheckBox *createCheckBox(const QString &text);
    QRadioButton *createRadioButton(const QString &text);

    QMainWindow *parentWindow;

    QWindow *previewWindow;
    PreviewWidget *previewWidget;
    PreviewDialog *previewDialog;

    QWindow *activePreview;

    QGroupBox *widgetTypeGroupBox;
    QGroupBox *additionalOptionsGroupBox;
    TypeControl *typeControl;
    HintControl *hintsControl;
    WindowStatesControl *statesControl;

    QRadioButton *previewWindowButton;
    QRadioButton *previewWidgetButton;
    QRadioButton *previewDialogButton;
    QCheckBox *modalWindowCheckBox;
    QCheckBox *fixedSizeWindowCheckBox;
};

class LogWidget : public QPlainTextEdit
{
    Q_OBJECT
public:
    explicit LogWidget(QWidget *parent = nullptr);
    ~LogWidget();

    static LogWidget *instance() { return m_instance; }
    static void install();

public slots:
    void appendText(const QString &);

private:
    static QString startupMessage();

    static LogWidget *m_instance;
};

class ControllerWindow : public QWidget {
    Q_OBJECT
public:
    ControllerWindow();

    void registerEventFilter();
};

#endif // CONTROLLERWINDOW_H
