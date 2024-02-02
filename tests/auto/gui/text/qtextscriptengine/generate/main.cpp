// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only


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
