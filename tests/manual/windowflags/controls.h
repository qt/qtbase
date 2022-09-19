// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CONTROLS_H
#define CONTROLS_H

#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QRadioButton;
class QButtonGroup;
QT_END_NAMESPACE

enum { ControlLayoutMargin = 4 };

// Control for the hint part of Qt::WindowFlags
class HintControl : public QGroupBox
{
    Q_OBJECT
public:
    explicit HintControl(QWidget *parent= 0);

    Qt::WindowFlags hints() const;
    void setHints(Qt::WindowFlags hints);

signals:
    void changed(Qt::WindowFlags);

private slots:
    void slotCheckBoxChanged();

private:
    QCheckBox *msWindowsFixedSizeDialogCheckBox;
    QCheckBox *x11BypassWindowManagerCheckBox;
    QCheckBox *framelessWindowCheckBox;
    QCheckBox *windowTitleCheckBox;
    QCheckBox *windowSystemMenuCheckBox;
    QCheckBox *windowMinimizeButtonCheckBox;
    QCheckBox *windowMaximizeButtonCheckBox;
    QCheckBox *windowFullscreenButtonCheckBox;
    QCheckBox *windowCloseButtonCheckBox;
    QCheckBox *windowContextHelpButtonCheckBox;
    QCheckBox *windowShadeButtonCheckBox;
    QCheckBox *windowStaysOnTopCheckBox;
    QCheckBox *windowStaysOnBottomCheckBox;
    QCheckBox *customizeWindowHintCheckBox;
    QCheckBox *transparentForInputCheckBox;
    QCheckBox *noDropShadowCheckBox;
};

// Control for the Qt::WindowState enum, optional with a "visible" QCheckbox
class WindowStateControl : public QWidget {
    Q_OBJECT
public:
    explicit WindowStateControl(QWidget *parent= 0);

    Qt::WindowStates state() const;
    void setState(Qt::WindowStates s);

signals:
    void stateChanged(int);

private:
    QButtonGroup *group;
    QCheckBox *restoreButton;
    QCheckBox *minimizeButton;
    QCheckBox *maximizeButton;
    QCheckBox *fullscreenButton;
};

// Control for the Qt::WindowStates flags (normal, maximized, fullscreen exclusively
// combined with minimized and optionally, with a "visible" QCheckbox)
class WindowStatesControl : public QGroupBox
{
    Q_OBJECT
public:
    explicit WindowStatesControl(QWidget *parent= 0);

    Qt::WindowStates states() const;
    void setStates(Qt::WindowStates s);

    bool visibleValue() const;
    void setVisibleValue(bool);
    bool activeValue() const;
    void setActiveValue(bool v);

signals:
    void changed();

private:
    QCheckBox *visibleCheckBox;
    QCheckBox *activeCheckBox;
    WindowStateControl *stateControl;
};

// Control for the type part of Qt::WindowFlags
class TypeControl : public QGroupBox
{
    Q_OBJECT
public:
    explicit TypeControl(QWidget *parent= 0);

    Qt::WindowFlags type() const;
    void setType(Qt::WindowFlags);

signals:
    void changed(Qt::WindowFlags);

private slots:
    void slotChanged();

private:
    QButtonGroup *group;
    QRadioButton *windowRadioButton;
    QRadioButton *dialogRadioButton;
    QRadioButton *sheetRadioButton;
    QRadioButton *drawerRadioButton;
    QRadioButton *popupRadioButton;
    QRadioButton *toolRadioButton;
    QRadioButton *toolTipRadioButton;
    QRadioButton *splashScreenRadioButton;
};

#endif // CONTROLS_H
