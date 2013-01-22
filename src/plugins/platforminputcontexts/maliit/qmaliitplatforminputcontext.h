/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QMALIITPLATFORMINPUTCONTEXT_H
#define QMALIITPLATFORMINPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <QDBusArgument>

QT_BEGIN_NAMESPACE

class QMaliitPlatformInputContextPrivate;
class QDBusVariant;
class QDBusMessage;

class QMaliitPlatformInputContext : public QPlatformInputContext
{
    Q_OBJECT
public:
    QMaliitPlatformInputContext();
    ~QMaliitPlatformInputContext();

    bool isValid() const;

    void invokeAction(QInputMethod::Action action, int x);
    void reset(void);
    void update(Qt::InputMethodQueries);
    virtual QRectF keyboardRect() const;

    virtual void showInputPanel();
    virtual void hideInputPanel();
    virtual bool isInputPanelVisible() const;
    void setFocusObject(QObject *object);

public Q_SLOTS:
    void activationLostEvent();
    void commitString(const QString &in0, int in1, int in2, int in3);
    void updatePreedit(const QDBusMessage &message);
    void copy();
    void imInitiatedHide();
    void keyEvent(int type, int key, int modifiers, const QString &text, bool autoRepeat, int count, uchar requestType_);
    void paste();
    bool preeditRectangle(int &x, int &y, int &width, int &height);
    bool selection(QString &selection);
    void setDetectableAutoRepeat(bool in0);
    void setGlobalCorrectionEnabled(bool enable);
    void setLanguage(const QString &);
    void setRedirectKeys(bool );
    void setSelection(int start, int length);
    void updateInputMethodArea(int x, int y, int width, int height);
    void updateServerWindowOrientation(Qt::ScreenOrientation orientation);

private:
    QMaliitPlatformInputContextPrivate *d;
};

QT_END_NAMESPACE

#endif
