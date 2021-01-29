/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#ifndef PROPERTYTESTER_H
#define PROPERTYTESTER_H

#include <QObject>
#include <QProperty>

class PropertyTester : public QObject
{
    Q_OBJECT
signals:
    void xOldChanged();
    void yOldChanged();
    void xNotifiedChanged();
    void yNotifiedChanged();

public:
    PropertyTester() = default;
    Q_PROPERTY(int xOld READ xOld WRITE setXOld NOTIFY xOldChanged)
    Q_PROPERTY(int yOld READ yOld WRITE setYOld NOTIFY yOldChanged)
    Q_PROPERTY(int x MEMBER x BINDABLE xBindable)
    Q_PROPERTY(int y MEMBER y BINDABLE yBindable)
    Q_PROPERTY(int xNotified MEMBER xNotified NOTIFY xNotifiedChanged BINDABLE xNotifiedBindable)
    Q_PROPERTY(int yNotified MEMBER yNotified NOTIFY yNotifiedChanged BINDABLE yNotifiedBindable)
    void setXOld(int i) {
        if (m_xOld != i) {
            m_xOld = i;
            emit xOldChanged();
        }
    }
    void setYOld(int i) {
        if (m_yOld != i) {
            m_yOld = i;
            emit yOldChanged();
        }
    }
    int xOld() { return m_xOld; }
    int yOld() { return m_yOld; }
    QProperty<int> x;
    QProperty<int> y;

    QBindable<int> xBindable() { return QBindable<int>(&x); }
    QBindable<int> yBindable() { return QBindable<int>(&y); }

    Q_OBJECT_BINDABLE_PROPERTY(PropertyTester, int, xNotified, &PropertyTester::xNotifiedChanged)
    Q_OBJECT_BINDABLE_PROPERTY(PropertyTester, int, yNotified, &PropertyTester::yNotifiedChanged)

    QBindable<int> xNotifiedBindable() { return QBindable<int>(&xNotified); }
    QBindable<int> yNotifiedBindable() { return QBindable<int>(&yNotified); }

private:
    int m_xOld = 0;
    int m_yOld = 0;
};

#endif // PROPERTYTESTER_H
