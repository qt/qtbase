/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "propertywatcher.h"
#include <QApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>
#include <QTextStream>
#include <QFormLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <QStatusBar>
#include <QLineEdit>
#include <QDesktopWidget>

class ScreenPropertyWatcher : public PropertyWatcher
{
    Q_OBJECT
public:
    ScreenPropertyWatcher(QWidget *wp = Q_NULLPTR) : PropertyWatcher(Q_NULLPTR, QString(), wp)
    {
        // workaround for the fact that virtualSiblings is not a property,
        // thus there is no change notification:
        // allow the user to update the field manually
        connect(this, &PropertyWatcher::updatedAllFields, this, &ScreenPropertyWatcher::updateSiblings);
    }

    QScreen *screenSubject() const { return qobject_cast<QScreen *>(subject()); }
    void setScreenSubject(QScreen *s, const QString &annotation = QString())
    {
        setSubject(s, annotation);
        updateSiblings();
    }

public slots:
    void updateSiblings();
};

void ScreenPropertyWatcher::updateSiblings()
{
    const QScreen *screen = screenSubject();
    if (!screen)
        return;
    const QString objectName = QLatin1String("siblings");
    QLineEdit *siblingsField = findChild<QLineEdit *>(objectName);
    if (!siblingsField) {
        siblingsField = new QLineEdit(this);
        siblingsField->setObjectName(objectName);
        siblingsField->setReadOnly(true);
        formLayout()->insertRow(0, QLatin1String("virtualSiblings"), siblingsField);
    }
    QString text;
    foreach (const QScreen *sibling, screen->virtualSiblings()) {
        if (!text.isEmpty())
            text += QLatin1String(", ");
        text += sibling->name();
    }
    siblingsField->setText(text);
}

class ScreenWatcherMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit ScreenWatcherMainWindow(QScreen *screen);

    QScreen *screenSubject() const { return m_watcher->screenSubject(); }

protected:
    bool event(QEvent *event) Q_DECL_OVERRIDE;

private:
    const QString m_annotation;
    ScreenPropertyWatcher *m_watcher;
};

static int i = 0;

ScreenWatcherMainWindow::ScreenWatcherMainWindow(QScreen *screen)
    : m_annotation(QLatin1Char('#') + QString::number(i++))
    ,  m_watcher(new ScreenPropertyWatcher(this))
{
    setAttribute(Qt::WA_DeleteOnClose);
    setCentralWidget(m_watcher);
    m_watcher->setScreenSubject(screen, m_annotation);

    QMenu *fileMenu = menuBar()->addMenu(QLatin1String("&File"));
    QAction *a = fileMenu->addAction(QLatin1String("Close"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_W));
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    a = fileMenu->addAction(QLatin1String("Quit"));
    a->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    connect(a, SIGNAL(triggered()), qApp, SLOT(quit()));
}

static inline QString msgScreenChange(const QWidget *w, const QScreen *oldScreen, const QScreen *newScreen)
{
    QString result;
    const QRect geometry = w->geometry();
    const QPoint pos = QCursor::pos();
    if (!newScreen) {
        result = QLatin1String("Screen changed --> null");
    } else if (!oldScreen) {
        QTextStream(&result) << "Screen changed null --> \""
            << newScreen->name() << "\" at " << pos.x() << ',' << pos.y() << " geometry: "
            << geometry.width() << 'x' << geometry.height() << forcesign << geometry.x()
            << geometry.y() << '.';
    } else {
        QTextStream(&result) << "Screen changed \"" << oldScreen->name() << "\" --> \""
            << newScreen->name() << "\" at " << pos.x() << ',' << pos.y() << " geometry: "
            << geometry.width() << 'x' << geometry.height() << forcesign << geometry.x()
            << geometry.y() << '.';
    }
    return result;
}

bool ScreenWatcherMainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::ScreenChangeInternal) {
        QScreen *newScreen = windowHandle()->screen();
        const QString message = msgScreenChange(this, m_watcher->screenSubject(), newScreen);
        qDebug().noquote() << message;
        statusBar()->showMessage(message);
        m_watcher->setScreenSubject(newScreen, m_annotation);
    }
    return QMainWindow::event(event);
}

void screenAdded(QScreen* screen)
{
    screen->setOrientationUpdateMask((Qt::ScreenOrientations)0x0F);
    qDebug("\nscreenAdded %s siblings %d first %s", qPrintable(screen->name()), screen->virtualSiblings().count(),
        (screen->virtualSiblings().isEmpty() ? "none" : qPrintable(screen->virtualSiblings().first()->name())));
    ScreenWatcherMainWindow *w = new ScreenWatcherMainWindow(screen);

    // Set the screen via QDesktopWidget. This corresponds to setScreen() for the underlying
    // QWindow. This is essential when having separate X screens since the the positioning below is
    // not sufficient to get the windows show up on the desired screen.
    QList<QScreen *> screens = QGuiApplication::screens();
    int screenNumber = screens.indexOf(screen);
    Q_ASSERT(screenNumber >= 0);
    w->setParent(qApp->desktop()->screen(screenNumber));

    w->show();

    // Position the windows so that they end up at the center of the corresponding screen.
    QRect geom = w->geometry();
    geom.setSize(w->sizeHint());
    if (geom.height() > screen->geometry().height())
        geom.setHeight(screen->geometry().height() * 9 / 10);
    geom.moveCenter(screen->geometry().center());
    w->setGeometry(geom);
}

void screenRemoved(QScreen* screen)
{
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    for (int i = topLevels.size() - 1; i >= 0; --i) {
        if (ScreenWatcherMainWindow *sw = qobject_cast<ScreenWatcherMainWindow *>(topLevels.at(i))) {
            if (sw->screenSubject() == screen)
                sw->close();
        }
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QList<QScreen *> screens = QGuiApplication::screens();
    foreach (QScreen *screen, screens)
        screenAdded(screen);
    QObject::connect((const QGuiApplication*)QGuiApplication::instance(), &QGuiApplication::screenAdded, &screenAdded);
    QObject::connect((const QGuiApplication*)QGuiApplication::instance(), &QGuiApplication::screenRemoved, &screenRemoved);
    return a.exec();
}

#include "main.moc"
