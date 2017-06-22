/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    explicit ControllerWidget(QWidget *parent = 0);

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
    explicit LogWidget(QWidget *parent = 0);
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
