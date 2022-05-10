// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>


class MouseMonitor : public QLabel {
    Q_OBJECT
public:
    MouseMonitor() : m_grabbed(false) {
        setMinimumSize(540, 240);
        setAlignment(Qt::AlignCenter);
        setMouseTracking(true);
        setWindowTitle(QLatin1String("Mouse Monitor"));
        updateText();
    }

    void updateText() {
        QString txt = m_grabbed ?
                QLatin1String("Left-click to test QGuiApplication::topLevelAt(click pos)\nRight-click to ungrab\n") :
                QLatin1String("Left-click to grab mouse\n");
        if (!m_cursorPos.isNull()) {
            const auto screen = QGuiApplication::screenAt(m_cursorPos);
            const auto screenNum = screen ? QGuiApplication::screens().indexOf(screen) : 0;
            txt += QString(QLatin1String("Current mouse position: %1, %2 on screen %3\n"))
                    .arg(m_cursorPos.x()).arg(m_cursorPos.y()).arg(screenNum);
            if (QGuiApplication::mouseButtons() & Qt::LeftButton) {
                QWindow *win = QGuiApplication::topLevelAt(m_cursorPos);
                txt += QString(QLatin1String("Top-level window found? %1\n"))
                        .arg(win ? (win->title().isEmpty() ? "no title" : win->title()) : "none");
            }
        }
        setText(txt);
    }

protected:
    void mouseMoveEvent(QMouseEvent *ev) override {
        m_cursorPos = ev->screenPos().toPoint();
        updateText();
    }

    void mousePressEvent(QMouseEvent *ev) override {
        m_cursorPos = ev->screenPos().toPoint();
        qDebug() << "top level @" << m_cursorPos << ":" << QGuiApplication::topLevelAt(m_cursorPos);
        updateText();
        if (!m_grabbed) {
            grabMouse(Qt::CrossCursor);
            m_grabbed = true;
        } else if (ev->button() == Qt::RightButton) {
            setVisible(false);
            deleteLater();
        }
    }

private:
    QPoint m_cursorPos;
    bool m_grabbed;
};

class ScreenPropertyWatcher : public PropertyWatcher
{
    Q_OBJECT
public:
    ScreenPropertyWatcher(QWidget *wp = nullptr) : PropertyWatcher(nullptr, QString(), wp)
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
    bool event(QEvent *event) override;
    void startMouseMonitor();

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
    a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    a = fileMenu->addAction(QLatin1String("Quit"));
    a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    connect(a, SIGNAL(triggered()), qApp, SLOT(quit()));

    QMenu *toolsMenu = menuBar()->addMenu(QLatin1String("&Tools"));
    a = toolsMenu->addAction(QLatin1String("Mouse Monitor"));
    a->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_M));
    connect(a, &QAction::triggered, this, &ScreenWatcherMainWindow::startMouseMonitor);
}

static inline QString msgScreenChange(const QWidget *w, const QScreen *oldScreen, const QScreen *newScreen)
{
    QString result;
    const QRect geometry = w->geometry();
    const QPoint pos = QCursor::pos();
    if (!newScreen) {
        result = QLatin1String("Screen changed --> null");
    } else if (!oldScreen) {
        QTextStream(&result) << "Screen changed null --> \"" << newScreen->name() << "\" at "
                             << pos.x() << ',' << pos.y() << " geometry: " << geometry.width()
                             << 'x' << geometry.height() << Qt::forcesign << geometry.x()
                             << geometry.y() << '.';
    } else {
        QTextStream(&result) << "Screen changed \"" << oldScreen->name() << "\" --> \""
                             << newScreen->name() << "\" at " << pos.x() << ',' << pos.y()
                             << " geometry: " << geometry.width() << 'x' << geometry.height()
                             << Qt::forcesign << geometry.x() << geometry.y() << '.';
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

void ScreenWatcherMainWindow::startMouseMonitor()
{
    MouseMonitor *mm = new MouseMonitor();
    mm->show();
}

void screenAdded(QScreen* screen)
{
    qDebug("\nscreenAdded %s siblings %d fast %s", qPrintable(screen->name()), screen->virtualSiblings().count(),
        (screen->virtualSiblings().isEmpty() ? "none" : qPrintable(screen->virtualSiblings().first()->name())));
    ScreenWatcherMainWindow *w = new ScreenWatcherMainWindow(screen);

    w->setScreen(screen);
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
