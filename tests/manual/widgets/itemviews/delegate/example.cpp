/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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

#include <QLineEdit>
#include <QApplication>
#include <QTableView>
#include <QStandardItemModel>
#include <QItemDelegate>
#include <QDebug>

class ExampleEditor : public QLineEdit
{
public:
    ExampleEditor(QWidget *parent = 0):QLineEdit(parent) { qDebug() << "ctor"; }
    ~ExampleEditor() { QApplication::instance()->quit(); }
};

class ExampleDelegate : public QItemDelegate
{
public:
    ExampleDelegate():QItemDelegate() { m_editor = new ExampleEditor(0); }
protected:
    QWidget* createEditor(QWidget *p, const QStyleOptionViewItem &o, const QModelIndex &) const
    {
        m_editor->setParent(p);
        m_editor->setGeometry(o.rect);
        return m_editor;
    }
    void destroyEditor(QWidget *editor, const QModelIndex &) const
    {
        editor->setParent(0);
        qDebug() << "intercepted destroy :)";
    }

    // Avoid setting data - and therefore show that the editor keeps its state.
    void setEditorData(QWidget*, const QModelIndex &) const { }

    ~ExampleDelegate() { delete m_editor; }
    mutable ExampleEditor *m_editor;
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
