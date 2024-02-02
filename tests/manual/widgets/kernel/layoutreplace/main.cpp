// Copyright (C) 2013 Thorbjørn Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QGridLayout>
#include <QFormLayout>
#include <QStackedLayout>
#include <QPushButton>
#include <QLabel>
#include <QApplication>

class ReplaceButton : public QPushButton
{
    Q_OBJECT
public:
    ReplaceButton(const QString &text = QString("click to replace")) : QPushButton(text)
    {
        static int v = 0;
        ++v;
        QString txt = QString("click to replace %1").arg(v);
        setText(txt);
        connect(this, SIGNAL(clicked()), this, SLOT(replace()));
    }
protected slots:
    void replace()
    {
        if (!parentWidget())
            return;
        static int n = 0;
        ++n;
        if (parentWidget()->layout()->replaceWidget(this, new QLabel(QString("replaced(%1)").arg(n))))
            deleteLater();
    }
};

class StackButtonChange : public QPushButton
{
    Q_OBJECT
public:
    StackButtonChange(QStackedLayout *l) : QPushButton("stack wdg change")
    {
        sl = l;
        connect(this, SIGNAL(clicked()), this, SLOT(changeWdg()));
    }
protected slots:
    void changeWdg()
    {
        int index = sl->indexOf(sl->currentWidget());
        ++index;
        if (index >= sl->count())
            index = 0;
        sl->setCurrentWidget(sl->itemAt(index)->widget());
        sl->parentWidget()->update();
    }
protected:
    QStackedLayout *sl;
};

int main(int argc, char **argv)                                                                                                                                                               {
    QApplication app(argc, argv);
    QWidget wdg1;
    QGridLayout *l1 = new QGridLayout();
    l1->addWidget(new ReplaceButton(), 1, 1, 2, 2, Qt::AlignCenter);
    l1->addWidget(new ReplaceButton(), 3, 1, 1, 1, Qt::AlignRight);
    l1->addWidget(new ReplaceButton(), 1, 3, 1, 1, Qt::AlignLeft);
    l1->addWidget(new ReplaceButton(), 2, 3, 1, 1, Qt::AlignLeft);
    l1->addWidget(new ReplaceButton(), 3, 2, 1, 1, Qt::AlignRight);
    wdg1.setLayout(l1);
    wdg1.setWindowTitle("QGridLayout");
    wdg1.setGeometry(100, 100, 100, 100);
    wdg1.show();

    QWidget wdg2;
    QFormLayout *l2 = new QFormLayout();
    l2->addRow(QString("Label1"), new ReplaceButton());
    l2->addRow(QString("Label2"), new ReplaceButton());
    l2->addRow(new ReplaceButton(), new ReplaceButton());
    wdg2.setLayout(l2);
    wdg2.setWindowTitle("QFormLayout");
    wdg2.setGeometry(100 + wdg1.sizeHint().width() + 5, 100, 100, 100);
    wdg2.show();

    QWidget wdg3;
    QBoxLayout *l3 = new QVBoxLayout(); // new QHBoxLayout()
    QStackedLayout *sl = new QStackedLayout();
    sl->addWidget(new ReplaceButton());
    sl->addWidget(new ReplaceButton());
    sl->addWidget(new ReplaceButton());
    l3->addLayout(sl);
    l3->addWidget(new StackButtonChange(sl));
    l3->addWidget(new ReplaceButton());
    l3->addWidget(new ReplaceButton());
    wdg3.setLayout(l3);
    wdg3.setWindowTitle("QStackedLayout + BoxLayout");
    wdg3.setGeometry(100, 100 + wdg1.sizeHint().height() + 30, 100 , 100);
    wdg3.show();

    app.exec();
}
#include "main.moc"
