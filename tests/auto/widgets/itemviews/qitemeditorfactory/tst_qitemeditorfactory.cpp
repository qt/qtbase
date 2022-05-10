// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QDoubleSpinBox>
#include <QItemEditorFactory>
#include <QTest>

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

    QWidget *w = factory->createEditor(QMetaType::QString, &parent);
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

        editorFactory.registerEditor(QMetaType::QRect, creator);
        editorFactory.registerEditor(QMetaType::QRectF, creator);

        //creator should not be deleted as a result of calling the next line
        editorFactory.registerEditor(QMetaType::QRect, creator2);
        QVERIFY(creator);

        //this should erase creator2
        editorFactory.registerEditor(QMetaType::QRect, creator);
        QVERIFY(creator2.isNull());


        QWidget parent;

        QWidget *w = editorFactory.createEditor(QMetaType::QRect, &parent);
        QCOMPARE(w->metaObject()->className(), "QDoubleSpinBox");
        QCOMPARE(w->metaObject()->userProperty().userType(), QMetaType::Double);
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
        QCOMPARE(editor->metaObject()->userProperty().userType(), QMetaType::UInt);
    }
    {
        QWidget *editor = editorFactory.createEditor(QMetaType::Int, &parent);
        QCOMPARE(editor->metaObject()->className(), "QSpinBox");
        QCOMPARE(editor->metaObject()->userProperty().userType(), QMetaType::Int);
    }
}

QTEST_MAIN(tst_QItemEditorFactory)
#include "tst_qitemeditorfactory.moc"

