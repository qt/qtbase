/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest/QtTest>

#include <private/qguiapplication_p.h>
#include <private/qinputmethod_p.h>
#include <qpa/qplatforminputcontext.h>
#include "../../../shared/platforminputcontext.h"

class InputItem : public QObject
{
    Q_OBJECT
public:
    InputItem() : cursorRectangle(1, 2, 3, 4), m_enabled(true) {}

    bool event(QEvent *event)
    {
        if (event->type() == QEvent::InputMethodQuery) {
            QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent *>(event);
            if (query->queries() & Qt::ImEnabled)
                query->setValue(Qt::ImEnabled, m_enabled);
            if (query->queries() & Qt::ImCursorRectangle)
                query->setValue(Qt::ImCursorRectangle, cursorRectangle);
            if (query->queries() & Qt::ImPreferredLanguage)
                query->setValue(Qt::ImPreferredLanguage, QString("English"));
            m_lastQueries = query->queries();
            query->accept();
            return true;
        }
        return false;
    }

    void setEnabled(bool enabled) {
        if (enabled != m_enabled) {
            m_enabled = enabled;
            qApp->inputMethod()->update(Qt::ImEnabled);
        }
    }

    QRectF cursorRectangle;
    Qt::InputMethodQueries m_lastQueries;
    bool m_enabled;
};


class DummyWindow : public QWindow
{
public:
    DummyWindow() : m_focusObject(0) {}

    virtual QObject *focusObject() const
    {
        return m_focusObject;
    }

    void setFocusObject(QObject *object)
    {
        m_focusObject = object;
        emit focusObjectChanged(object);
    }

    QObject *m_focusObject;
};


class tst_qinputmethod : public QObject
{
    Q_OBJECT
public:
    tst_qinputmethod() {}
    virtual ~tst_qinputmethod() {}
private slots:
    void initTestCase();
    void isVisible();
    void animating();
    void keyboarRectangle();
    void inputItemTransform();
    void cursorRectangle();
    void invokeAction();
    void reset();
    void commit();
    void update();
    void query();
    void inputDirection();
    void inputMethodAccepted();

private:
    InputItem m_inputItem;
    PlatformInputContext m_platformInputContext;
};

void tst_qinputmethod::initTestCase()
{
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = &m_platformInputContext;
}

void tst_qinputmethod::isVisible()
{
    QCOMPARE(qApp->inputMethod()->isVisible(), false);
    qApp->inputMethod()->show();
    QCOMPARE(qApp->inputMethod()->isVisible(), true);

    qApp->inputMethod()->hide();
    QCOMPARE(qApp->inputMethod()->isVisible(), false);

    qApp->inputMethod()->setVisible(true);
    QCOMPARE(qApp->inputMethod()->isVisible(), true);

    qApp->inputMethod()->setVisible(false);
    QCOMPARE(qApp->inputMethod()->isVisible(), false);
}

void tst_qinputmethod::animating()
{
    QCOMPARE(qApp->inputMethod()->isAnimating(), false);

    m_platformInputContext.m_animating = true;
    QCOMPARE(qApp->inputMethod()->isAnimating(), true);

    m_platformInputContext.m_animating = false;
    QCOMPARE(qApp->inputMethod()->isAnimating(), false);

    QSignalSpy spy(qApp->inputMethod(), SIGNAL(animatingChanged()));
    m_platformInputContext.emitAnimatingChanged();
    QCOMPARE(spy.count(), 1);
}

void tst_qinputmethod::keyboarRectangle()
{
    QCOMPARE(qApp->inputMethod()->keyboardRectangle(), QRectF());

    m_platformInputContext.m_keyboardRect = QRectF(10, 20, 30, 40);
    QCOMPARE(qApp->inputMethod()->keyboardRectangle(), QRectF(10, 20, 30, 40));

    QSignalSpy spy(qApp->inputMethod(), SIGNAL(keyboardRectangleChanged()));
    m_platformInputContext.emitKeyboardRectChanged();
    QCOMPARE(spy.count(), 1);
}

void tst_qinputmethod::inputItemTransform()
{
    QCOMPARE(qApp->inputMethod()->inputItemTransform(), QTransform());
    QSignalSpy spy(qApp->inputMethod(), SIGNAL(cursorRectangleChanged()));

    QTransform transform;
    transform.translate(10, 10);
    transform.scale(2, 2);
    transform.shear(2, 2);
    qApp->inputMethod()->setInputItemTransform(transform);

    QCOMPARE(qApp->inputMethod()->inputItemTransform(), transform);
    QCOMPARE(spy.count(), 1);

    // reset
    qApp->inputMethod()->setInputItemTransform(QTransform());
}

void tst_qinputmethod::cursorRectangle()
{
    QCOMPARE(qApp->inputMethod()->cursorRectangle(), QRectF());

    DummyWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.requestActivate();
    QTRY_COMPARE(qApp->focusWindow(), &window);
    window.setFocusObject(&m_inputItem);

    QTransform transform;
    transform.translate(10, 10);
    transform.scale(2, 2);
    transform.shear(2, 2);
    qApp->inputMethod()->setInputItemTransform(transform);
    QCOMPARE(qApp->inputMethod()->cursorRectangle(), transform.mapRect(QRectF(1, 2, 3, 4)));

    m_inputItem.cursorRectangle = QRectF(1.5, 2, 1, 8);
    QCOMPARE(qApp->inputMethod()->cursorRectangle(), transform.mapRect(QRectF(1.5, 2, 1, 8)));

    // reset
    m_inputItem.cursorRectangle = QRectF(1, 2, 3, 4);
    qApp->inputMethod()->setInputItemTransform(QTransform());
}

void tst_qinputmethod::invokeAction()
{
    QCOMPARE(m_platformInputContext.m_action, QInputMethod::Click);
    QCOMPARE(m_platformInputContext.m_cursorPosition, 0);

    qApp->inputMethod()->invokeAction(QInputMethod::ContextMenu, 5);
    QCOMPARE(m_platformInputContext.m_action, QInputMethod::ContextMenu);
    QCOMPARE(m_platformInputContext.m_cursorPosition, 5);
}

void tst_qinputmethod::reset()
{
    QCOMPARE(m_platformInputContext.m_resetCallCount, 0);

    qApp->inputMethod()->reset();
    QCOMPARE(m_platformInputContext.m_resetCallCount, 1);

    qApp->inputMethod()->reset();
    QCOMPARE(m_platformInputContext.m_resetCallCount, 2);
}

void tst_qinputmethod::commit()
{
    QCOMPARE(m_platformInputContext.m_commitCallCount, 0);

    qApp->inputMethod()->commit();
    QCOMPARE(m_platformInputContext.m_commitCallCount, 1);

    qApp->inputMethod()->commit();
    QCOMPARE(m_platformInputContext.m_commitCallCount, 2);
}

void tst_qinputmethod::update()
{
    QCOMPARE(m_platformInputContext.m_updateCallCount, 0);
    QCOMPARE(int(m_platformInputContext.m_lastQueries), int(Qt::ImhNone));

    qApp->inputMethod()->update(Qt::ImQueryInput);
    QCOMPARE(m_platformInputContext.m_updateCallCount, 1);
    QCOMPARE(int(m_platformInputContext.m_lastQueries), int(Qt::ImQueryInput));

    qApp->inputMethod()->update(Qt::ImQueryAll);
    QCOMPARE(m_platformInputContext.m_updateCallCount, 2);
    QCOMPARE(int(m_platformInputContext.m_lastQueries), int(Qt::ImQueryAll));

    QCOMPARE(qApp->inputMethod()->keyboardRectangle(), QRectF(10, 20, 30, 40));
}

void tst_qinputmethod::query()
{
    QInputMethodQueryEvent query(Qt::InputMethodQueries(Qt::ImPreferredLanguage | Qt::ImCursorRectangle));
    QGuiApplication::sendEvent(&m_inputItem, &query);

    QString language = query.value(Qt::ImPreferredLanguage).toString();
    QCOMPARE(language, QString("English"));

    QRect cursorRectangle = query.value(Qt::ImCursorRectangle).toRect();
    QCOMPARE(cursorRectangle, QRect(1,2,3,4));
}

void tst_qinputmethod::inputDirection()
{
    QCOMPARE(m_platformInputContext.m_inputDirectionCallCount, 0);
    qApp->inputMethod()->inputDirection();
    QCOMPARE(m_platformInputContext.m_inputDirectionCallCount, 1);

    QCOMPARE(m_platformInputContext.m_localeCallCount, 0);
    qApp->inputMethod()->locale();
    QCOMPARE(m_platformInputContext.m_localeCallCount, 1);
}

void tst_qinputmethod::inputMethodAccepted()
{
    InputItem disabledItem;
    disabledItem.setEnabled(false);

    DummyWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    window.requestActivate();
    QTRY_COMPARE(qApp->focusWindow(), &window);
    window.setFocusObject(&disabledItem);

    QCOMPARE(m_platformInputContext.inputMethodAccepted(), false);

    window.setFocusObject(&m_inputItem);
    QCOMPARE(m_platformInputContext.inputMethodAccepted(), true);
}

QTEST_MAIN(tst_qinputmethod)
#include "tst_qinputmethod.moc"
