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

#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QStatusBar>
#include <QPlainTextEdit>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QTimer>
#include <QLineEdit>

// Compiles with Qt 4.8 and Qt 5.

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow();

    bool eventFilter(QObject *, QEvent *);

private slots:
    void showModalDialog();
    void mouseGrabToggled(bool);
    void delayedMouseGrab();
    void grabMouseWindowToggled(bool);
    void delayedMouseWindowGrab();
    void keyboardGrabToggled(bool);
    void grabKeyboardWindowToggled(bool);
    void forceNativeWidgets();

private:
    void toggleMouseWidgetGrab(QWidget *w, bool on);
    void toggleKeyboardWidgetGrab(QWidget *w, bool on);

    int m_mouseEventCount;
    int m_enterLeaveEventCount;
    QPlainTextEdit *m_logEdit;
    QCheckBox *m_grabMouseCheckBox;
    QCheckBox *m_grabMouseWindowCheckBox;
    QCheckBox *m_grabKeyboardCheckBox;
    QCheckBox *m_grabKeyboardWindowCheckBox;
    QPushButton *m_forceNativeButton;

    QString m_lastMouseMoveEvent;
};

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(const QString &text, QWidget *parent = 0) : QLabel(text, parent) {}

signals:
    void pressed();

protected:
    void mousePressEvent(QMouseEvent *ev)
    {
        emit pressed();
        QLabel::mousePressEvent(ev);
    }
};

static const char testCasesC[] =
"- Drag a scrollbar, move mouse out of the window. The scrollbar should still react.\n\n"
"- Press mouse inside window, move outside while pressing the button. Mouse events"
" should be reported until the button is released.\n\n"
"- 'Show modal dialog on press' opens a modal dialog on mouse press. This should not lock up.\n\n"
"- Check the 'Grab Mouse' box. Only the checkbox should then receive mouse move events.\n\n"
"- Open popup menu. Mouse events should all go to popup menu while it is visible.\n\n"
"- Click delayed grab and then open popup immediately. Wait for Grab checkbox to be marked. "
"Click on popup menu to close it. Mouse events should be going to Grab checkbox. "
"Click on Grab checkbox to clear it. UI should respond normally after that. \n\n"
;

MainWindow::MainWindow()
    : m_mouseEventCount(0)
    , m_enterLeaveEventCount(0)
    , m_logEdit(new QPlainTextEdit(this))
    , m_grabMouseCheckBox(new QCheckBox(QLatin1String("Grab Mouse")))
    , m_grabMouseWindowCheckBox(new QCheckBox(QLatin1String("Grab Mouse Window Ctrl+W")))
    , m_grabKeyboardCheckBox(new QCheckBox(QLatin1String("Grab Keyboard")))
    , m_grabKeyboardWindowCheckBox(new QCheckBox(QLatin1String("Grab Keyboard Window")))
    , m_forceNativeButton(new QPushButton(QLatin1String("Force native widgets")))
{
    setObjectName(QLatin1String("MainWindow"));
    setMinimumWidth(800);
    setWindowTitle(QString::fromLatin1("Manual Grab Test %1").arg(QLatin1String(QT_VERSION_STR)));

    QMenu *fileMenu = menuBar()->addMenu(QLatin1String("File"));
    fileMenu->setObjectName("FileMenu");
    QAction *quit = fileMenu->addAction(QLatin1String("Quit"));
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, SIGNAL(triggered()), this, SLOT(close()));

    QMenu *editMenu = menuBar()->addMenu(QLatin1String("Edit"));
    editMenu->setObjectName("EditMenu");
    QAction *clearLog = editMenu->addAction(QLatin1String("Clear Log"));
    connect(clearLog, SIGNAL(triggered()), m_logEdit, SLOT(clear()));

    QWidget *w = new QWidget(this);
    w->setObjectName(QLatin1String("CentralWidget"));
    QVBoxLayout *layout = new QVBoxLayout(w);
    QPlainTextEdit *instructions = new QPlainTextEdit(this);
    instructions->setObjectName(QLatin1String("InstructionsEdit"));
    instructions->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    instructions->setPlainText(QLatin1String(testCasesC));
    instructions->setReadOnly(true);
    layout->addWidget(instructions);

    int row = 0;
    QGridLayout *controlLayout = new QGridLayout;
    layout->addLayout(controlLayout);
    QPushButton *modalDialogButton = new QPushButton("Show modal dialog on release");
    modalDialogButton->setObjectName(QLatin1String("ModalDialogButton"));
    connect(modalDialogButton, SIGNAL(clicked()), this, SLOT(showModalDialog()));
    controlLayout->addWidget(modalDialogButton, row, 0);
    ClickableLabel *modalDialogLabel = new ClickableLabel("Show modal dialog on press");
    modalDialogLabel->setObjectName(QLatin1String("ModalDialogLabel"));
    controlLayout->addWidget(modalDialogLabel, row, 1);
    connect(modalDialogLabel, SIGNAL(pressed()), this, SLOT(showModalDialog()));

    row++;
    m_grabMouseCheckBox->setObjectName(QLatin1String("GrabCheckBox"));
    connect(m_grabMouseCheckBox, SIGNAL(toggled(bool)), this, SLOT(mouseGrabToggled(bool)));
    controlLayout->addWidget(m_grabMouseCheckBox, row, 0);
    QPushButton *delayedGrabButton = new QPushButton("Delayed grab");
    delayedGrabButton->setObjectName(QLatin1String("DelayedGrabButton"));
    connect(delayedGrabButton, SIGNAL(clicked()), this, SLOT(delayedMouseGrab()));
    controlLayout->addWidget(delayedGrabButton, row, 1);

    row++;
    m_grabMouseWindowCheckBox->setObjectName(QLatin1String("GrabWindowCheckBox"));
    m_grabMouseWindowCheckBox->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    connect(m_grabMouseWindowCheckBox, SIGNAL(toggled(bool)), this, SLOT(grabMouseWindowToggled(bool)));
    controlLayout->addWidget(m_grabMouseWindowCheckBox, row, 0);
    QPushButton *delayedWindowGrabButton = new QPushButton("Delayed window grab");
    delayedWindowGrabButton->setObjectName(QLatin1String("DelayedWindowGrabButton"));
    connect(delayedWindowGrabButton, SIGNAL(clicked()), this, SLOT(delayedMouseWindowGrab()));
    controlLayout->addWidget(delayedWindowGrabButton, row, 1);

    row++;
    m_grabKeyboardCheckBox->setObjectName(QLatin1String("GrabKeyboardBox"));
    connect(m_grabKeyboardCheckBox, SIGNAL(toggled(bool)), this, SLOT(keyboardGrabToggled(bool)));
    controlLayout->addWidget(m_grabKeyboardCheckBox, row, 0);
    m_grabKeyboardWindowCheckBox->setObjectName(QLatin1String("GrabKeyboardWindowBox"));
    connect(m_grabKeyboardWindowCheckBox, SIGNAL(toggled(bool)), this, SLOT(grabKeyboardWindowToggled(bool)));
    controlLayout->addWidget(m_grabKeyboardWindowCheckBox, row, 1);

    row++;
    QComboBox *combo = new QComboBox;
    combo->addItems(QStringList() << QLatin1String("Popup test 1") << QLatin1String("Popup test 2"));
    controlLayout->addWidget(combo, row, 0);

    QPushButton *popupMenuButton = new QPushButton("Popup menu");
    popupMenuButton->setObjectName(QLatin1String("PopupMenuButton"));
    QMenu *popupMenu = new QMenu(this);
    popupMenu->setObjectName(QLatin1String("PopupMenu"));
    popupMenu->addAction(tr("&First Item"));
    popupMenu->addAction(tr("&Second Item"));
    popupMenu->addAction(tr("&Third Item"));
    popupMenu->addAction(tr("F&ourth Item"));
    popupMenuButton->setMenu(popupMenu);
    controlLayout->addWidget(popupMenuButton, row, 1);

    row++;
    m_forceNativeButton->setObjectName("ForceNativeWidgetsButton");
    controlLayout->addWidget(m_forceNativeButton, row, 0);
    connect(m_forceNativeButton, SIGNAL(clicked()), this, SLOT(forceNativeWidgets()));

    row++;
    QLineEdit *lineEdit = new QLineEdit(this);
    lineEdit->setObjectName(QLatin1String("LineEdit"));
    controlLayout->addWidget(lineEdit, row, 0, 1, 2);

    m_logEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_logEdit->setObjectName(QLatin1String("LogEdit"));
    layout->addWidget(m_logEdit);
    setCentralWidget(w);

    qApp->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *o, QEvent *e)
{
    if (o->isWidgetType()) {
        switch (e->type()) {
        case QEvent::Enter: {
            QString message;
            QDebug debug(&message);
#if QT_VERSION >= 0x050000
            const QEnterEvent *ee = static_cast<QEnterEvent *>(e);
            debug.nospace()  << '#' << m_enterLeaveEventCount++ << " Enter for " << o->objectName()
                             << " at " << ee->localPos() << " global: " << ee->globalPos();
#else
            debug.nospace()  << '#' << m_enterLeaveEventCount++ << " Enter for " << o->objectName();
#endif
            m_logEdit->appendPlainText(message);
        }
            break;
        case QEvent::Leave: {
            QString message;
            QDebug debug(&message);
            debug.nospace()  << '#' << m_enterLeaveEventCount++ << " Leave for " << o->objectName();
            m_logEdit->appendPlainText(message);
        }
            break;
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease: {
            const QMouseEvent *me = static_cast<QMouseEvent *>(e);
            QString message;
            QDebug debug = QDebug(&message).nospace();
            debug << '#' << m_mouseEventCount++ << ' ';
            if (e->type() == QEvent::MouseButtonPress) {
                if (me->buttons() & Qt::LeftButton)
                    debug << "Left button press";
                if (me->buttons() & Qt::MiddleButton)
                    debug << "Middle button press";
                if (me->buttons() & Qt::RightButton)
                    debug << "Right button press";
            } else {
                debug << "Button release";
            }
            debug << " on " << o->objectName() << " Mousegrabber " << QWidget::mouseGrabber();
            m_logEdit->appendPlainText(message);
        }
            break;
        case QEvent::MouseMove: {
            const QMouseEvent *me = static_cast<const QMouseEvent *>(e);
            const QWidget *widgetUnderMouse = QApplication::widgetAt(me->globalPos());
            QString message;
            QDebug d = QDebug(&message).nospace();
            d << " Mouse move reported for " << o->objectName();
            if (widgetUnderMouse) {
                d << " over " << widgetUnderMouse;
            } else {
                d << " outside ";
            }
            d << " mouse grabber " << QWidget::mouseGrabber();
            // Compress mouse move event logging.
            if (message != m_lastMouseMoveEvent) {
                m_lastMouseMoveEvent = message;
                m_logEdit->appendPlainText(QString::fromLatin1("#%1 %2").arg(m_mouseEventCount++).arg(message));
            }
        }
            break;
        case QEvent::KeyRelease:
        case QEvent::KeyPress: {
            const QKeyEvent *ke = static_cast<const  QKeyEvent *>(e);
            QString message;
            QDebug d = QDebug(&message).nospace();
            d << (e->type() == QEvent::KeyPress ? "Key press" : "Key release")
              << ' ' << ke->text() << " on " << o << " key grabber " << QWidget::keyboardGrabber();
            m_logEdit->appendPlainText(message);
        }
            break;
        default:
            break;
        }
    }
    return QMainWindow::eventFilter(o ,e);
}

void MainWindow::showModalDialog()
{
    QMessageBox::information(this, QLatin1String("Information"), QLatin1String("Modal Dialog"));
}

void MainWindow::toggleMouseWidgetGrab(QWidget *w, bool on)
{
    if (on) {
        m_logEdit->appendPlainText(w->objectName() + QLatin1String(" grabbed mouse."));
        w->grabMouse();
    } else {
        w->releaseMouse();
        m_logEdit->appendPlainText(w->objectName() + QLatin1String(" released mouse."));
    }
}

void MainWindow::toggleKeyboardWidgetGrab(QWidget *w, bool on)
{
    if (on) {
        m_logEdit->appendPlainText(w->objectName() + QLatin1String(" grabbed keyboard."));
        w->grabKeyboard();
    } else {
        w->releaseKeyboard();
        m_logEdit->appendPlainText(w->objectName() + QLatin1String(" released keyboard."));
    }
}

void MainWindow::mouseGrabToggled(bool g)
{
    toggleMouseWidgetGrab(m_grabMouseCheckBox, g);
}

void MainWindow::delayedMouseGrab()
{
    QTimer::singleShot(2000, m_grabMouseCheckBox, SLOT(animateClick()));
}

void MainWindow::grabMouseWindowToggled(bool g)
{
    toggleMouseWidgetGrab(this, g);
}

void MainWindow::delayedMouseWindowGrab()
{
    QTimer::singleShot(2000, m_grabMouseWindowCheckBox, SLOT(animateClick()));
}

void MainWindow::keyboardGrabToggled(bool g)
{
    toggleKeyboardWidgetGrab(m_grabKeyboardCheckBox, g);
}

void MainWindow::grabKeyboardWindowToggled(bool g)
{
    toggleKeyboardWidgetGrab(this, g);
}

void MainWindow::forceNativeWidgets()
{
    const WId platformWid = m_forceNativeButton->winId();
#if QT_VERSION < 0x050000 && defined(Q_OS_WIN)
    const quintptr wid = quintptr(platformWid); // HWND on Qt 4.8/Windows.
#else
    const WId wid = platformWid;
#endif
    m_logEdit->appendPlainText(QString::fromLatin1("Created native widget %1").arg(wid));
    m_forceNativeButton->setEnabled(false);
    m_forceNativeButton->setText(QLatin1String("Native widgets created"));
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

#include "main.moc"
