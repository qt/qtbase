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

#include "controls.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDebug>

HintControl::HintControl(QWidget *parent)
    : QGroupBox(tr("Hints"), parent)
    , msWindowsFixedSizeDialogCheckBox(new QCheckBox(tr("MS Windows fixed size dialog")))
    , x11BypassWindowManagerCheckBox(new QCheckBox(tr("X11 bypass window manager")))
    , framelessWindowCheckBox(new QCheckBox(tr("Frameless window")))
    , windowTitleCheckBox(new QCheckBox(tr("Window title")))
    , windowSystemMenuCheckBox(new QCheckBox(tr("Window system menu")))
    , windowMinimizeButtonCheckBox(new QCheckBox(tr("Window minimize button")))
    , windowMaximizeButtonCheckBox(new QCheckBox(tr("Window maximize button")))
    , windowFullscreenButtonCheckBox(new QCheckBox(tr("Window fullscreen button")))
    , windowCloseButtonCheckBox(new QCheckBox(tr("Window close button")))
    , windowContextHelpButtonCheckBox(new QCheckBox(tr("Window context help button")))
    , windowShadeButtonCheckBox(new QCheckBox(tr("Window shade button")))
    , windowStaysOnTopCheckBox(new QCheckBox(tr("Window stays on top")))
    , windowStaysOnBottomCheckBox(new QCheckBox(tr("Window stays on bottom")))
    , customizeWindowHintCheckBox(new QCheckBox(tr("Customize window")))
    , transparentForInputCheckBox(new QCheckBox(tr("Transparent for input")))
{
    connect(msWindowsFixedSizeDialogCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(x11BypassWindowManagerCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(framelessWindowCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowTitleCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowSystemMenuCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowMinimizeButtonCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowMaximizeButtonCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowFullscreenButtonCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowCloseButtonCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowContextHelpButtonCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowShadeButtonCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowStaysOnTopCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(windowStaysOnBottomCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(customizeWindowHintCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    connect(transparentForInputCheckBox, SIGNAL(clicked()), this, SLOT(slotCheckBoxChanged()));
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setMargin(ControlLayoutMargin);
    layout->addWidget(msWindowsFixedSizeDialogCheckBox, 0, 0);
    layout->addWidget(x11BypassWindowManagerCheckBox, 1, 0);
    layout->addWidget(framelessWindowCheckBox, 2, 0);
    layout->addWidget(windowTitleCheckBox, 3, 0);
    layout->addWidget(windowSystemMenuCheckBox, 4, 0);
    layout->addWidget(windowMinimizeButtonCheckBox, 0, 1);
    layout->addWidget(windowMaximizeButtonCheckBox, 1, 1);
    layout->addWidget(windowFullscreenButtonCheckBox, 2, 1);
    layout->addWidget(windowCloseButtonCheckBox, 3, 1);
    layout->addWidget(windowContextHelpButtonCheckBox, 4, 1);
    layout->addWidget(windowShadeButtonCheckBox, 5, 1);
    layout->addWidget(windowStaysOnTopCheckBox, 6, 1);
    layout->addWidget(windowStaysOnBottomCheckBox, 7, 1);
    layout->addWidget(customizeWindowHintCheckBox, 5, 0);
    layout->addWidget(transparentForInputCheckBox, 6, 0);
#if QT_VERSION < 0x050000
    transparentForInputCheckBox->setEnabled(false);
#endif
}

Qt::WindowFlags HintControl::hints() const
{
    Qt::WindowFlags flags = 0;
    if (msWindowsFixedSizeDialogCheckBox->isChecked())
        flags |= Qt::MSWindowsFixedSizeDialogHint;
    if (x11BypassWindowManagerCheckBox->isChecked())
        flags |= Qt::X11BypassWindowManagerHint;
    if (framelessWindowCheckBox->isChecked())
        flags |= Qt::FramelessWindowHint;
    if (windowTitleCheckBox->isChecked())
        flags |= Qt::WindowTitleHint;
    if (windowSystemMenuCheckBox->isChecked())
        flags |= Qt::WindowSystemMenuHint;
    if (windowMinimizeButtonCheckBox->isChecked())
        flags |= Qt::WindowMinimizeButtonHint;
    if (windowMaximizeButtonCheckBox->isChecked())
        flags |= Qt::WindowMaximizeButtonHint;
#if QT_VERSION >= 0x050000
    if (windowFullscreenButtonCheckBox->isChecked())
        flags |= Qt::WindowFullscreenButtonHint;
#endif
    if (windowCloseButtonCheckBox->isChecked())
        flags |= Qt::WindowCloseButtonHint;
    if (windowContextHelpButtonCheckBox->isChecked())
        flags |= Qt::WindowContextHelpButtonHint;
    if (windowShadeButtonCheckBox->isChecked())
        flags |= Qt::WindowShadeButtonHint;
    if (windowStaysOnTopCheckBox->isChecked())
        flags |= Qt::WindowStaysOnTopHint;
    if (windowStaysOnBottomCheckBox->isChecked())
        flags |= Qt::WindowStaysOnBottomHint;
    if (customizeWindowHintCheckBox->isChecked())
        flags |= Qt::CustomizeWindowHint;
#if QT_VERSION >= 0x050000
    if (transparentForInputCheckBox->isChecked())
        flags |= Qt::WindowTransparentForInput;
#endif
    return flags;
}

void HintControl::setHints(Qt::WindowFlags flags)
{
    msWindowsFixedSizeDialogCheckBox->setChecked(flags & Qt::MSWindowsFixedSizeDialogHint);
    x11BypassWindowManagerCheckBox->setChecked(flags & Qt::X11BypassWindowManagerHint);
    framelessWindowCheckBox->setChecked(flags & Qt::FramelessWindowHint);
    windowTitleCheckBox->setChecked(flags & Qt::WindowTitleHint);
    windowSystemMenuCheckBox->setChecked(flags & Qt::WindowSystemMenuHint);
    windowMinimizeButtonCheckBox->setChecked(flags & Qt::WindowMinimizeButtonHint);
    windowMaximizeButtonCheckBox->setChecked(flags & Qt::WindowMaximizeButtonHint);
#if QT_VERSION >= 0x050000
    windowFullscreenButtonCheckBox->setChecked(flags & Qt::WindowFullscreenButtonHint);
#endif
    windowCloseButtonCheckBox->setChecked(flags & Qt::WindowCloseButtonHint);
    windowContextHelpButtonCheckBox->setChecked(flags & Qt::WindowContextHelpButtonHint);
    windowShadeButtonCheckBox->setChecked(flags & Qt::WindowShadeButtonHint);
    windowStaysOnTopCheckBox->setChecked(flags & Qt::WindowStaysOnTopHint);
    windowStaysOnBottomCheckBox->setChecked(flags & Qt::WindowStaysOnBottomHint);
    customizeWindowHintCheckBox->setChecked(flags & Qt::CustomizeWindowHint);
#if QT_VERSION >= 0x050000
    transparentForInputCheckBox->setChecked(flags & Qt::WindowTransparentForInput);
#endif
}

void HintControl::slotCheckBoxChanged()
{
    emit changed(hints());
}

WindowStateControl::WindowStateControl(QWidget *parent)
    : QWidget(parent)
    , group(new QButtonGroup)
    , restoreButton(new QCheckBox(tr("Normal")))
    , minimizeButton(new QCheckBox(tr("Minimized")))
    , maximizeButton(new QCheckBox(tr("Maximized")))
    , fullscreenButton(new QCheckBox(tr("Fullscreen")))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    group->setExclusive(false);
    layout->setMargin(ControlLayoutMargin);
    group->addButton(restoreButton, Qt::WindowNoState);
    restoreButton->setEnabled(false);
    layout->addWidget(restoreButton);
    group->addButton(minimizeButton, Qt::WindowMinimized);
    layout->addWidget(minimizeButton);
    group->addButton(maximizeButton, Qt::WindowMaximized);
    layout->addWidget(maximizeButton);
    group->addButton(fullscreenButton, Qt::WindowFullScreen);
    layout->addWidget(fullscreenButton);
    connect(group, SIGNAL(buttonReleased(int)), this, SIGNAL(stateChanged(int)));
}

Qt::WindowStates WindowStateControl::state() const
{
    Qt::WindowStates states;
    foreach (QAbstractButton *button, group->buttons()) {
        if (button->isChecked())
            states |= Qt::WindowState(group->id(button));
    }
    return states;
}

void WindowStateControl::setState(Qt::WindowStates s)
{
    group->blockSignals(true);
    foreach (QAbstractButton *button, group->buttons())
        button->setChecked(s & Qt::WindowState(group->id(button)));

    if (!(s & (Qt::WindowMaximized | Qt::WindowFullScreen)))
        restoreButton->setChecked(true);

    group->blockSignals(false);
}

WindowStatesControl::WindowStatesControl(QWidget *parent)
    : QGroupBox(tr("States"), parent)
    , visibleCheckBox(new QCheckBox(tr("Visible")))
    , activeCheckBox(new QCheckBox(tr("Active")))
    , stateControl(new WindowStateControl)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(ControlLayoutMargin);
    connect(visibleCheckBox, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    layout->addWidget(visibleCheckBox);
    connect(activeCheckBox, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    layout->addWidget(activeCheckBox);
    layout->addWidget(stateControl);
    connect(stateControl, SIGNAL(stateChanged(int)), this, SIGNAL(changed()));
}

Qt::WindowStates WindowStatesControl::states() const
{
    Qt::WindowStates s = stateControl->state();
    if (activeValue())
        s |= Qt::WindowActive;
    return s;
}

void WindowStatesControl::setStates(Qt::WindowStates s)
{
    stateControl->setState(s);
    setActiveValue(s & Qt::WindowActive);
}

bool WindowStatesControl::visibleValue() const
{
    return visibleCheckBox && visibleCheckBox->isChecked();
}

void WindowStatesControl::setVisibleValue(bool v)
{
    if (visibleCheckBox) {
        visibleCheckBox->blockSignals(true);
        visibleCheckBox->setChecked(v);
        visibleCheckBox->blockSignals(false);
    }
}

bool WindowStatesControl::activeValue() const
{
    return activeCheckBox && activeCheckBox->isChecked();
}

void WindowStatesControl::setActiveValue(bool v)
{
    if (activeCheckBox) {
        activeCheckBox->blockSignals(true);
        activeCheckBox->setChecked(v);
        activeCheckBox->blockSignals(false);
    }
}

TypeControl::TypeControl(QWidget *parent)
    : QGroupBox(tr("Type"), parent)
    , group(new QButtonGroup)
    , windowRadioButton(new QRadioButton(tr("Window")))
    , dialogRadioButton(new QRadioButton(tr("Dialog")))
    , sheetRadioButton(new QRadioButton(tr("Sheet")))
    , drawerRadioButton(new QRadioButton(tr("Drawer")))
    , popupRadioButton(new QRadioButton(tr("Popup")))
    , toolRadioButton(new QRadioButton(tr("Tool")))
    , toolTipRadioButton(new QRadioButton(tr("Tooltip")))
    , splashScreenRadioButton(new QRadioButton(tr("Splash screen")))
{
    group->setExclusive(true);
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setMargin(ControlLayoutMargin);
    group->addButton(windowRadioButton, Qt::Window);
    layout->addWidget(windowRadioButton, 0, 0);
    group->addButton(dialogRadioButton, Qt::Dialog);
    layout->addWidget(dialogRadioButton, 1, 0);
    group->addButton(sheetRadioButton, Qt::Sheet);
    layout->addWidget(sheetRadioButton, 2, 0);
    group->addButton(drawerRadioButton, Qt::Drawer);
    layout->addWidget(drawerRadioButton, 3, 0);
    group->addButton(popupRadioButton, Qt::Popup);
    layout->addWidget(popupRadioButton, 0, 1);
    group->addButton(toolRadioButton, Qt::Tool);
    layout->addWidget(toolRadioButton, 1, 1);
    group->addButton(toolTipRadioButton, Qt::ToolTip);
    layout->addWidget(toolTipRadioButton, 2, 1);
    group->addButton(splashScreenRadioButton, Qt::SplashScreen);
    layout->addWidget(splashScreenRadioButton, 3, 1);
    connect(group, SIGNAL(buttonReleased(int)), this, SLOT(slotChanged()));
}

Qt::WindowFlags TypeControl::type() const
{
    return Qt::WindowFlags(group->checkedId());
}

void TypeControl::setType(Qt::WindowFlags s)
{
    if (QAbstractButton *b = group->button(s & Qt::WindowType_Mask))
        b->setChecked(true);
}

void TypeControl::slotChanged()
{
    emit changed(type());
}
