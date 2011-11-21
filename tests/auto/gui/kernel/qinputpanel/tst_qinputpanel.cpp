/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>

#include <private/qguiapplication_p.h>
#include <private/qinputpanel_p.h>
#include <qplatforminputcontext_qpa.h>

class PlatformInputContext : public QPlatformInputContext
{
public:
    PlatformInputContext() :
        m_animating(false),
        m_visible(false),
        m_updateCallCount(0),
        m_resetCallCount(0),
        m_commitCallCount(0),
        m_lastQueries(Qt::ImhNone),
        m_action(QInputPanel::Click),
        m_cursorPosition(0),
        m_lastEventType(QEvent::None)
    {}

    virtual QRectF keyboardRect() const { return m_keyboardRect; }
    virtual bool isAnimating() const { return m_animating; }
    virtual void reset() { m_resetCallCount++; }
    virtual void commit() { m_commitCallCount++; }

    virtual void update(Qt::InputMethodQueries queries)
    {
        m_updateCallCount++;
        m_lastQueries = queries;
    }
    virtual void invokeAction(QInputPanel::Action action, int cursorPosition)
    {
        m_action = action;
        m_cursorPosition = cursorPosition;
    }
    virtual bool filterEvent(const QEvent *event)
    {
        m_lastEventType = event->type(); return false;
    }
    virtual void showInputPanel()
    {
        m_visible = true;
    }
    virtual void hideInputPanel()
    {
        m_visible = false;
    }
    virtual bool isInputPanelVisible() const
    {
        return m_visible;
    }

    bool m_animating;
    bool m_visible;
    int m_updateCallCount;
    int m_resetCallCount;
    int m_commitCallCount;
    Qt::InputMethodQueries m_lastQueries;
    QInputPanel::Action m_action;
    int m_cursorPosition;
    int m_lastEventType;
    QRectF m_keyboardRect;
};

class InputItem : public QObject
{
    Q_OBJECT
public:
    bool event(QEvent *event)
    {
        if (event->type() == QEvent::InputMethodQuery) {
            QInputMethodQueryEvent *query = static_cast<QInputMethodQueryEvent *>(event);
            if (query->queries() & Qt::ImCursorRectangle)
                query->setValue(Qt::ImCursorRectangle, QRectF(1, 2, 3, 4));
            if (query->queries() & Qt::ImPreferredLanguage)
                query->setValue(Qt::ImPreferredLanguage, QString("English"));
            m_lastQueries = query->queries();
            query->accept();
            return true;
        }
        return false;
    }
    Qt::InputMethodQueries m_lastQueries;
};

class tst_qinputpanel : public QObject
{
    Q_OBJECT
public:
    tst_qinputpanel() {}
    virtual ~tst_qinputpanel() {}
private slots:
    void initTestCase();
    void visible();
    void animating();
    void keyboarRectangle();
    void inputItem();
    void inputItemTransform();
    void cursorRectangle();
    void invokeAction();
    void reset();
    void commit();
    void update();
    void query();
private:
    InputItem m_inputItem;
    PlatformInputContext m_platformInputContext;
};

void tst_qinputpanel::initTestCase()
{
    QInputPanelPrivate *inputPanelPrivate = QInputPanelPrivate::get(qApp->inputPanel());
    inputPanelPrivate->testContext = &m_platformInputContext;
}

void tst_qinputpanel::visible()
{
    QCOMPARE(qApp->inputPanel()->visible(), false);
    qApp->inputPanel()->show();
    QCOMPARE(qApp->inputPanel()->visible(), true);

    qApp->inputPanel()->hide();
    QCOMPARE(qApp->inputPanel()->visible(), false);

    qApp->inputPanel()->setVisible(true);
    QCOMPARE(qApp->inputPanel()->visible(), true);

    qApp->inputPanel()->setVisible(false);
    QCOMPARE(qApp->inputPanel()->visible(), false);
}

void tst_qinputpanel::animating()
{
    QCOMPARE(qApp->inputPanel()->isAnimating(), false);

    m_platformInputContext.m_animating = true;
    QCOMPARE(qApp->inputPanel()->isAnimating(), true);

    m_platformInputContext.m_animating = false;
    QCOMPARE(qApp->inputPanel()->isAnimating(), false);

    QSignalSpy spy(qApp->inputPanel(), SIGNAL(animatingChanged()));
    m_platformInputContext.emitAnimatingChanged();
    QCOMPARE(spy.count(), 1);
}

void tst_qinputpanel::keyboarRectangle()
{
    QCOMPARE(qApp->inputPanel()->keyboardRectangle(), QRectF());

    m_platformInputContext.m_keyboardRect = QRectF(10, 20, 30, 40);
    QCOMPARE(qApp->inputPanel()->keyboardRectangle(), QRectF(10, 20, 30, 40));

    QSignalSpy spy(qApp->inputPanel(), SIGNAL(keyboardRectangleChanged()));
    m_platformInputContext.emitKeyboardRectChanged();
    QCOMPARE(spy.count(), 1);
}

void tst_qinputpanel::inputItem()
{
    QVERIFY(!qApp->inputPanel()->inputItem());
    QSignalSpy spy(qApp->inputPanel(), SIGNAL(inputItemChanged()));

    qApp->inputPanel()->setInputItem(&m_inputItem);

    QCOMPARE(qApp->inputPanel()->inputItem(), &m_inputItem);
    QCOMPARE(spy.count(), 1);

    // reset
    qApp->inputPanel()->setInputItem(0);
}

void tst_qinputpanel::inputItemTransform()
{
    QCOMPARE(qApp->inputPanel()->inputItemTransform(), QTransform());
    QSignalSpy spy(qApp->inputPanel(), SIGNAL(cursorRectangleChanged()));

    QTransform transform;
    transform.translate(10, 10);
    transform.scale(2, 2);
    transform.shear(2, 2);
    qApp->inputPanel()->setInputItemTransform(transform);

    QCOMPARE(qApp->inputPanel()->inputItemTransform(), transform);
    QCOMPARE(spy.count(), 1);

    // reset
    qApp->inputPanel()->setInputItemTransform(QTransform());
}

void tst_qinputpanel::cursorRectangle()
{
    QCOMPARE(qApp->inputPanel()->cursorRectangle(), QRectF());

    QTransform transform;
    transform.translate(10, 10);
    transform.scale(2, 2);
    transform.shear(2, 2);
    qApp->inputPanel()->setInputItemTransform(transform);
    qApp->inputPanel()->setInputItem(&m_inputItem);

    QCOMPARE(qApp->inputPanel()->cursorRectangle(), transform.mapRect(QRectF(1, 2, 3, 4)));

    // reset
    qApp->inputPanel()->setInputItem(0);
    qApp->inputPanel()->setInputItemTransform(QTransform());
}

void tst_qinputpanel::invokeAction()
{
    QCOMPARE(m_platformInputContext.m_action, QInputPanel::Click);
    QCOMPARE(m_platformInputContext.m_cursorPosition, 0);

    qApp->inputPanel()->invokeAction(QInputPanel::ContextMenu, 5);
    QCOMPARE(m_platformInputContext.m_action, QInputPanel::ContextMenu);
    QCOMPARE(m_platformInputContext.m_cursorPosition, 5);
}

void tst_qinputpanel::reset()
{
    QCOMPARE(m_platformInputContext.m_resetCallCount, 0);

    qApp->inputPanel()->reset();
    QCOMPARE(m_platformInputContext.m_resetCallCount, 1);

    qApp->inputPanel()->reset();
    QCOMPARE(m_platformInputContext.m_resetCallCount, 2);
}

void tst_qinputpanel::commit()
{
    QCOMPARE(m_platformInputContext.m_commitCallCount, 0);

    qApp->inputPanel()->commit();
    QCOMPARE(m_platformInputContext.m_commitCallCount, 1);

    qApp->inputPanel()->commit();
    QCOMPARE(m_platformInputContext.m_commitCallCount, 2);
}

void tst_qinputpanel::update()
{
    qApp->inputPanel()->setInputItem(&m_inputItem);
    QCOMPARE(m_platformInputContext.m_updateCallCount, 0);
    QCOMPARE(int(m_platformInputContext.m_lastQueries), int(Qt::ImhNone));

    qApp->inputPanel()->update(Qt::ImQueryInput);
    QCOMPARE(m_platformInputContext.m_updateCallCount, 1);
    QCOMPARE(int(m_platformInputContext.m_lastQueries), int(Qt::ImQueryInput));

    qApp->inputPanel()->update(Qt::ImQueryAll);
    QCOMPARE(m_platformInputContext.m_updateCallCount, 2);
    QCOMPARE(int(m_platformInputContext.m_lastQueries), int(Qt::ImQueryAll));

    QCOMPARE(qApp->inputPanel()->keyboardRectangle(), QRectF(10, 20, 30, 40));

    // reset
    qApp->inputPanel()->setInputItem(0);
}

void tst_qinputpanel::query()
{
    QInputMethodQueryEvent query(Qt::InputMethodQueries(Qt::ImPreferredLanguage | Qt::ImCursorRectangle));
    QGuiApplication::sendEvent(&m_inputItem, &query);

    QString language = query.value(Qt::ImPreferredLanguage).toString();
    QCOMPARE(language, QString("English"));

    QRect cursorRectangle = query.value(Qt::ImCursorRectangle).toRect();
    QCOMPARE(cursorRectangle, QRect(1,2,3,4));
}

QTEST_MAIN(tst_qinputpanel)
#include "tst_qinputpanel.moc"
