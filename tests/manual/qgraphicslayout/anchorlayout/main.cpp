// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtWidgets>

static QGraphicsProxyWidget *createItem(const QSizeF &minimum = QSizeF(100.0, 100.0),
                                        const QSizeF &preferred = QSize(150.0, 100.0),
                                        const QSizeF &maximum = QSizeF(200.0, 100.0),
                                        const QString &name = "0")
{
    QGraphicsProxyWidget *w = new QGraphicsProxyWidget;
    w->setWidget(new QPushButton(name));
    w->setData(0, name);
    w->setMinimumSize(minimum);
    w->setPreferredSize(preferred);
    w->setMaximumSize(maximum);

    w->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    return w;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QGraphicsScene scene;
    scene.setSceneRect(0, 0, 800, 480);

    QSizeF minSize(30, 100);
    QSizeF prefSize(210, 100);
    QSizeF maxSize(300, 100);

    QGraphicsProxyWidget *a = createItem(minSize, prefSize, maxSize, "A");
    QGraphicsProxyWidget *b = createItem(minSize, prefSize, maxSize, "B");
    QGraphicsProxyWidget *c = createItem(minSize, prefSize, maxSize, "C");
    QGraphicsProxyWidget *d = createItem(minSize, prefSize, maxSize, "D");
    QGraphicsProxyWidget *e = createItem(minSize, prefSize, maxSize, "E");
    QGraphicsProxyWidget *f = createItem(QSizeF(30, 50), QSizeF(150, 50), maxSize, "F (overflow)");
    QGraphicsProxyWidget *g = createItem(QSizeF(30, 50), QSizeF(30, 100), maxSize, "G (overflow)");

    QGraphicsAnchorLayout *l = new QGraphicsAnchorLayout;
    l->setSpacing(0);

    QGraphicsWidget *w = new QGraphicsWidget(nullptr, Qt::Window);
    w->setPos(20, 20);
    w->setLayout(l);

    // vertical
    l->addAnchor(a, Qt::AnchorTop, l, Qt::AnchorTop);
    l->addAnchor(b, Qt::AnchorTop, l, Qt::AnchorTop);

    l->addAnchor(c, Qt::AnchorTop, a, Qt::AnchorBottom);
    l->addAnchor(c, Qt::AnchorTop, b, Qt::AnchorBottom);
    l->addAnchor(c, Qt::AnchorBottom, d, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, e, Qt::AnchorTop);

    l->addAnchor(d, Qt::AnchorBottom, l, Qt::AnchorBottom);
    l->addAnchor(e, Qt::AnchorBottom, l, Qt::AnchorBottom);

    l->addAnchor(c, Qt::AnchorTop, f, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorVerticalCenter, f, Qt::AnchorBottom);
    l->addAnchor(f, Qt::AnchorBottom, g, Qt::AnchorTop);
    l->addAnchor(c, Qt::AnchorBottom, g, Qt::AnchorBottom);

    // horizontal
    l->addAnchor(l, Qt::AnchorLeft, a, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorLeft, d, Qt::AnchorLeft);
    l->addAnchor(a, Qt::AnchorRight, b, Qt::AnchorLeft);

    l->addAnchor(a, Qt::AnchorRight, c, Qt::AnchorLeft);
    l->addAnchor(c, Qt::AnchorRight, e, Qt::AnchorLeft);

    l->addAnchor(b, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(e, Qt::AnchorRight, l, Qt::AnchorRight);
    l->addAnchor(d, Qt::AnchorRight, e, Qt::AnchorLeft);

    l->addAnchor(l, Qt::AnchorLeft, f, Qt::AnchorLeft);
    l->addAnchor(l, Qt::AnchorLeft, g, Qt::AnchorLeft);
    l->addAnchor(f, Qt::AnchorRight, g, Qt::AnchorRight);


    scene.addItem(w);
    scene.setBackgroundBrush(Qt::darkGreen);
    QGraphicsView view(&scene);

    view.show();

    return app.exec();
}
