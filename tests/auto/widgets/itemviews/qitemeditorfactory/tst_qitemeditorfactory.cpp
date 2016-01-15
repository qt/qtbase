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
#include <QtGui/QtGui>
#include <QtWidgets/QtWidgets>
#include <QtTest/QtTest>

class tst_QItemEditorFactory: public QObject
{
    Q_OBJECT
private slots:
    void createEditor();
    void createCustomEditor();
    void uintValues();
};

void tst_QItemEditorFactory::createEditor()
{
    const QItemEditorFactory *factory = QItemEditorFactory::defaultFactory();

    QWidget parent;

    QWidget *w = factory->createEditor(QVariant::String, &parent);
    QCOMPARE(w->metaObject()->className(), "QExpandingLineEdit");
}

//we make it inherit from QObject so that we can use QPointer
class MyEditor : public QObject, public QStandardItemEditorCreator<QDoubleSpinBox>
{
};

void tst_QItemEditorFactory::createCustomEditor()
{
    QPointer<MyEditor> creator = new MyEditor;
    QPointer<MyEditor> creator2 = new MyEditor;

    {
        QItemEditorFactory editorFactory;

        editorFactory.registerEditor(QVariant::Rect, creator);
        editorFactory.registerEditor(QVariant::RectF, creator);

        //creator should not be deleted as a result of calling the next line
        editorFactory.registerEditor(QVariant::Rect, creator2);
        QVERIFY(creator);

        //this should erase creator2
        editorFactory.registerEditor(QVariant::Rect, creator);
        QVERIFY(creator2.isNull());


        QWidget parent;

        QWidget *w = editorFactory.createEditor(QVariant::Rect, &parent);
        QCOMPARE(w->metaObject()->className(), "QDoubleSpinBox");
        QCOMPARE(w->metaObject()->userProperty().type(), QVariant::Double);
    }

    //editorFactory has been deleted, so should be creator
    //because editorFActory has the ownership
    QVERIFY(creator.isNull());
    QVERIFY(creator2.isNull());

    delete creator;
}

void tst_QItemEditorFactory::uintValues()
{
    QItemEditorFactory editorFactory;

    QWidget parent;

    {
        QWidget *editor = editorFactory.createEditor(QMetaType::UInt, &parent);
        QCOMPARE(editor->metaObject()->className(), "QUIntSpinBox");
        QCOMPARE(editor->metaObject()->userProperty().type(), QVariant::UInt);
    }
    {
        QWidget *editor = editorFactory.createEditor(QMetaType::Int, &parent);
        QCOMPARE(editor->metaObject()->className(), "QSpinBox");
        QCOMPARE(editor->metaObject()->userProperty().type(), QVariant::Int);
    }
}

QTEST_MAIN(tst_QItemEditorFactory)
#include "tst_qitemeditorfactory.moc"

