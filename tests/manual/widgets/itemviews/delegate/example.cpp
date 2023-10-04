// Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QLineEdit>
#include <QApplication>
#include <QTableView>
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QDebug>
#include <QComboBox>

class ExampleEditor : public QLineEdit
{
public:
    ExampleEditor(QWidget *parent = nullptr):QLineEdit(parent) { qDebug() << "ctor"; }
    ~ExampleEditor() { QApplication::instance()->quit(); }
};

class ExampleDelegate : public QStyledItemDelegate
{
public:
    ExampleDelegate() : QStyledItemDelegate()
    {
        m_editor = new ExampleEditor(0);
        m_combobox = new QComboBox(0);
        m_combobox->addItem(QString::fromUtf8("item1"));
        m_combobox->addItem(QString::fromUtf8("item2"));
    }
protected:
    QWidget* createEditor(QWidget *p, const QStyleOptionViewItem &o, const QModelIndex &i) const
    {
        // doubleclick rownumber 3 (last row) to see the difference.
        if (i.row() == 3) {
            m_combobox->setParent(p);
            m_combobox->setGeometry(o.rect);
            return m_combobox;
        } else {
            m_editor->setParent(p);
            m_editor->setGeometry(o.rect);
            return m_editor;
        }
    }
    void destroyEditor(QWidget *editor, const QModelIndex &) const
    {
        editor->setParent(0);
        qDebug() << "intercepted destroy :)";
    }

    // Avoid setting data - and therefore show that the editor keeps its state.
    void setEditorData(QWidget* w, const QModelIndex &) const
    {
        QComboBox *combobox = qobject_cast<QComboBox*>(w);
        if (combobox) {
            qDebug() << "Try to show popup at once";
            // Now we could try to make a call to
            // QCoreApplication::processEvents();
            // But it does not matter. The fix:
            // https://codereview.qt-project.org/40608
            // is blocking QComboBox from reacting to this doubleclick edit event
            // and we need to do that since the mouseReleaseEvent has not yet happened,
            // and therefore cannot be processed.
            combobox->showPopup();
        }
    }

    ~ExampleDelegate() { delete m_editor; }
    mutable ExampleEditor *m_editor;
    mutable QComboBox *m_combobox;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTableView tv;
    QStandardItemModel m;
    m.setRowCount(4);
    m.setColumnCount(2);
    tv.setModel(&m);
    tv.show();
    tv.setItemDelegate(new ExampleDelegate());
    app.exec();
}
