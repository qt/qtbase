// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "openglwidget.h"
#include <QApplication>
#include <QPushButton>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMainWindow>
#include <QLCDNumber>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>
#include <QSurfaceFormat>
#include <QDebug>
#include <private/qwindow_p.h>

class Tools : public QObject
{
    Q_OBJECT

public:
    Tools(QWidget *root, QWidget *widgetToTurn, const QWidgetList &glwidgets)
        : m_root(root), m_widgetToTurn(widgetToTurn), m_glWidgets(glwidgets) { }
    void dump();

private slots:
    void turnNative();
    void hideShowAllGL();

signals:
    void aboutToShowGLWidgets();

private:
    void dumpWidget(QWidget *w, int indent = 0);

    QWidget *m_root;
    QWidget *m_widgetToTurn;
    QWidgetList m_glWidgets;
};

void Tools::turnNative()
{
    qDebug("Turning into native");
    m_widgetToTurn->winId();
    dump();
}

void Tools::hideShowAllGL()
{
    if (m_glWidgets[0]->isVisible()) {
        qDebug("Hiding all render-to-texture widgets");
        foreach (QWidget *w, m_glWidgets)
            w->hide();
    } else {
        qDebug("Showing all render-to-texture widgets");
        emit aboutToShowGLWidgets();
        foreach (QWidget *w, m_glWidgets)
            w->show();
    }
}

void Tools::dump()
{
    qDebug() << "Widget hierarchy";
    dumpWidget(m_root);
    qDebug() << "========";
}

void Tools::dumpWidget(QWidget *w, int indent)
{
    QString indentStr;
    indentStr.fill(' ', indent);
    qDebug().noquote() << indentStr << w << "winId =" << w->internalWinId();
    foreach (QObject *obj, w->children()) {
        if (QWidget *cw = qobject_cast<QWidget *>(obj))
            dumpWidget(cw, indent + 4);
    }
}

class TabWidgetResetter : public QObject
{
    Q_OBJECT
public:
    TabWidgetResetter(QTabWidget *tw) : m_tw(tw) { }
public slots:
    void reset() { m_tw->setCurrentIndex(0); }
private:
    QTabWidget *m_tw;
};

int main(int argc, char *argv[])
{
    if (argc > 1 && !strcmp(argv[1], "--sharecontext")) {
        qDebug("Requesting all contexts to share");
        QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    }

    QApplication a(argc, argv);

    QSurfaceFormat format;
    if (QCoreApplication::arguments().contains(QLatin1String("--multisample")))
        format.setSamples(4);
    if (QCoreApplication::arguments().contains(QLatin1String("--coreprofile"))) {
        format.setVersion(3, 2);
        format.setProfile(QSurfaceFormat::CoreProfile);
    }
    qDebug() << "Requesting" << format;

    QMainWindow wnd;
    wnd.setObjectName("Main Window");
    wnd.resize(1024, 768);

    QMdiArea *w = new QMdiArea;
    w->setObjectName("MDI area");
    w->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    wnd.setCentralWidget(w);

    OpenGLWidget *glw = new OpenGLWidget(33, QVector3D(0, 0, 1));
    glw->setObjectName("First GL Widget with 33 ms timer");
    glw->setFormat(format);
    glw->setMinimumSize(100, 100);
    QMdiSubWindow *sw = w->addSubWindow(glw);
    sw->setObjectName("First MDI Sub-Window");
    sw->setWindowTitle("33 ms timer");

    OpenGLWidget *glw2 = new OpenGLWidget(16);
    glw2->setObjectName("Second GL Widget with 16 ms timer");
    glw2->setFormat(format);
    glw2->setMinimumSize(100, 100);
    QOpenGLWidget *glw22 = new OpenGLWidget(16);
    glw22->setObjectName("Second #2 GLWidget");
    glw22->setParent(glw2);
    glw22->resize(40, 40);
    sw = w->addSubWindow(glw2);
    sw->setObjectName("Second MDI Sub-Window");
    sw->setWindowTitle("16 ms timer");

    OpenGLWidget *glw3 = new OpenGLWidget(0); // trigger updates continuously, no timer
    glw3->setObjectName("GL widget in scroll area (possibly native)");
    glw3->setFormat(format);
    glw3->setFixedSize(600, 600);
    OpenGLWidget *glw3child = new OpenGLWidget(0);
    const float glw3ClearColor[] = { 0.5f, 0.2f, 0.8f };
    glw3child->setClearColor(glw3ClearColor);
    glw3child->setObjectName("Child widget of GL Widget in scroll area");
    glw3child->setFormat(format);
    glw3child->setParent(glw3);
    glw3child->setGeometry(500, 500, 100, 100); // lower right corner of parent
    QScrollArea *sa = new QScrollArea;
    sa->setWidget(glw3);
    sa->setMinimumSize(100, 100);
    sa->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sw = w->addSubWindow(sa);
    sw->setObjectName("MDI Sub-Window for scroll area");
    sw->setWindowTitle("Cont. update");
    sw->resize(300, 300);
    sa->verticalScrollBar()->setValue(300);

    QLCDNumber *lcd = new QLCDNumber;
    lcd->display(1337);
    lcd->setMinimumSize(300, 100);
    sw = w->addSubWindow(lcd);
    sw->setObjectName("MDI Sub-Window for LCD widget");
    sw->setWindowTitle("Ordinary widget");

    QTabWidget *tw = new QTabWidget;
    QOpenGLWidget *glw4 = new OpenGLWidget(16, QVector3D(1, 0, 0));
    glw4->setObjectName("GL widget in tab widget");
    tw->addTab(glw4, "OpenGL");
    QLabel *label = new QLabel("Another tab");
    tw->addTab(label, "Not OpenGL");
    tw->setMinimumSize(100, 100);
    sw = w->addSubWindow(tw);
    sw->setObjectName("MDI Sub-Window for tab widget");
    sw->setWindowTitle("Tabs");

    TabWidgetResetter twr(tw);
    Tools t(&wnd, glw3, QWidgetList { glw, glw2, glw3, glw4 });
    QObject::connect(&t, SIGNAL(aboutToShowGLWidgets()), &twr, SLOT(reset()));
    QMenu *toolsMenu = wnd.menuBar()->addMenu("&Tools");
    toolsMenu->addAction("&Turn widgets (or some parent) into native", &t, SLOT(turnNative()));
    toolsMenu->addAction("&Hide/show all OpenGL widgets", &t, SLOT(hideShowAllGL()));

    wnd.show();

    if (glw->isValid())
        qDebug() << "Got" << glw->format();

    t.dump();

    return a.exec();
}

#include "main.moc"
