/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQNXINPUTCONTEXT_H
#define QQNXINPUTCONTEXT_H

#include <QtCore/QLocale>
#include <QtGui/QPlatformInputContext>
#include <QtGui/QPlatformIntegration>

QT_BEGIN_NAMESPACE

class QQnxAbstractVirtualKeyboard;

class QQnxInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    explicit QQnxInputContext(QQnxAbstractVirtualKeyboard &keyboard);
    ~QQnxInputContext();

    bool isValid() const;

    void reset();
    bool filterEvent( const QEvent *event );
    bool handleKeyboardEvent(int flags, int sym, int mod, int scan, int cap);

    void showInputPanel();
    void hideInputPanel();
    bool isInputPanelVisible() const;

    QLocale locale() const;
    void setFocusObject(QObject *object);

private Q_SLOTS:
    void keyboardVisibilityChanged(bool visible);
    void keyboardLocaleChanged(const QLocale &locale);

private:
    bool hasPhysicalKeyboard();

    bool m_inputPanelVisible;
    QLocale m_inputPanelLocale;
    QQnxAbstractVirtualKeyboard &m_virtualKeyboard;
};

QT_END_NAMESPACE

#endif // QQNXINPUTCONTEXT_H
