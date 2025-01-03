// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "screenreader.h"
#include "optionswidget.h"
#include "accessibilityscenemanager.h"
#include <QtGui>

ScreenReader::ScreenReader(QObject *parent) :
    QObject(parent)
{
    m_selectedInterface = 0;
    m_rootInterface = 0;
    bool activateCalled = false;
}

ScreenReader::~ScreenReader()
{
}

void ScreenReader::setRootObject(QObject *rootObject)
{
    m_rootInterface = QAccessible::queryAccessibleInterface(rootObject);
}

void ScreenReader::setOptionsWidget(OptionsWidget *optionsWidget)
{
    m_optionsWidget = optionsWidget;
}

void ScreenReader::touchPoint(const QPoint &point)
{
    qDebug() << "touch" << point;
    // Wait and see if this touch is the start of a double-tap
    // (activate will then be called and cancel the touch processing)
    m_activateCalled = false;
    m_currentTouchPoint = point;
    QTimer::singleShot(200, this, SLOT(processTouchPoint()));
}

void ScreenReader::processTouchPoint()
{
    if (m_activateCalled) {
        return;
    }

    if (m_rootInterface == 0) {
        return;
    }

    QAccessibleInterface * currentInterface = m_rootInterface;

    int hit = -2;
    int guardCounter = 0;
    const int guardMax = 40;
    while (currentInterface != 0) {
        ++guardCounter;
        if (guardCounter > guardMax) {
            qDebug() << "touchPoint exit recursion overflow";
            return; // outside
        }

        QAccessibleInterface * hit = currentInterface->childAt(m_currentTouchPoint.x(), m_currentTouchPoint.y());
        if (!hit)
            break;
        currentInterface = hit;
    }

    m_selectedInterface = currentInterface;
    if (m_selectedInterface->object())
        emit selected(m_selectedInterface->object());
    if (m_optionsWidget->enableTextToSpeach())
        speak(m_selectedInterface->text(QAccessible::Name)
              /*+ "," + translateRole(m_selectedInterface->role(0)) */);

//    qDebug() << "touchPoint exit found" << m_selectedInterface->text(QAccessible::Name, 0) << m_selectedInterface->object() << m_selectedInterface->rect(0);
}

void ScreenReader::activate()
{
    qDebug() << "ScreenReader::activate";
    m_activateCalled = true;
    if (m_selectedInterface) {
        m_selectedInterface->actionInterface()->doAction(QAccessibleActionInterface::pressAction());
    }
}

void ScreenReader::speak(const QString &text, const QString &/*voice*/)
{
    QFile f("festivalspeachhack");
    f.open(QIODevice::WriteOnly);
    f.write(text.toLocal8Bit());
    f.close();

    QProcess *process = new QProcess;
    process->start("/usr/bin/festival", QStringList() << "--tts" << "festivalspeachhack");
}


