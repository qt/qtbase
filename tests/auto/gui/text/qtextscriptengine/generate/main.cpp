/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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


#include <QApplication>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QFontDialog>
#include <QPushButton>

#define private public
#include <qfont.h>
#include <private/qtextengine_p.h>
#include <private/qfontengine_p.h>
#include <qtextlayout.h>
#undef private


class MyEdit : public QTextEdit {
    Q_OBJECT
public:
    MyEdit(QWidget *p) : QTextEdit(p) { setReadOnly(true); }
public slots:
    void setText(const QString &str);
    void changeFont();
public:
    QLineEdit *lineEdit;
};

void MyEdit::setText(const QString &str)
{
    if (str.isEmpty()) {
        clear();
        return;
    }
    QTextLayout layout(str, lineEdit->font());
    QTextEngine *e = layout.d;
    e->itemize();
    e->shape(0);

    QString result;
    result = "Using font '" + e->fontEngine(e->layoutData->items[0])->fontDef.family + "'\n\n";

    result += "{ { ";
    for (int i = 0; i < str.length(); ++i) 
        result += "0x" + QString::number(str.at(i).unicode(), 16) + ", ";
    result += "0x0 },\n  { ";
    for (int i = 0; i < e->layoutData->items[0].num_glyphs; ++i)
        result += "0x" + QString::number(e->layoutData->glyphLayout.glyphs[i], 16) + ", ";
    result += "0x0 } }";

    setPlainText(result);
}

void MyEdit::changeFont()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, lineEdit->font(), topLevelWidget());
    if (ok)
        lineEdit->setFont(f);
}

int main(int argc,  char **argv)
{
    QApplication a(argc, argv);

    QWidget *mw = new QWidget(0);
    QVBoxLayout *l = new QVBoxLayout(mw);

    QLineEdit *le = new QLineEdit(mw);
    l->addWidget(le);

    MyEdit *view = new MyEdit(mw);
    view->lineEdit = le;
    l->addWidget(view);

    QPushButton *button = new QPushButton("Change Font", mw);
    l->addWidget(button);

    QObject::connect(le, SIGNAL(textChanged(QString)), view, SLOT(setText(QString)));
    QObject::connect(button, SIGNAL(clicked()), view, SLOT(changeFont()));

    mw->resize(500, 300);
    mw->show();

    return a.exec();
}


#include <main.moc>
