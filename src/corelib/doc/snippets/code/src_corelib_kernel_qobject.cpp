/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

//! [1]
QObject *obj = new QPushButton;
obj->metaObject()->className();             // returns "QPushButton"

QPushButton::staticMetaObject.className();  // returns "QPushButton"
//! [1]


//! [2]
QPushButton::staticMetaObject.className();  // returns "QPushButton"

QObject *obj = new QPushButton;
obj->metaObject()->className();             // returns "QPushButton"
//! [2]


//! [3]
QObject *obj = new QTimer;          // QTimer inherits QObject

QTimer *timer = qobject_cast<QTimer *>(obj);
// timer == (QObject *)obj

QAbstractButton *button = qobject_cast<QAbstractButton *>(obj);
// button == nullptr
//! [3]


//! [4]
QTimer *timer = new QTimer;         // QTimer inherits QObject
timer->inherits("QTimer");          // returns true
timer->inherits("QObject");         // returns true
timer->inherits("QAbstractButton"); // returns false

// QVBoxLayout inherits QObject and QLayoutItem
QVBoxLayout *layout = new QVBoxLayout;
layout->inherits("QObject");        // returns true
layout->inherits("QLayoutItem");    // returns true (even though QLayoutItem is not a QObject)
//! [4]


//! [5]
qDebug("MyClass::setPrecision(): (%s) invalid precision %f",
       qPrintable(objectName()), newPrecision);
//! [5]


//! [6]
class MainWindow : public QMainWindow
{
public:
    MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;

private:
    QTextEdit *textEdit;
};

MainWindow::MainWindow()
{
    textEdit = new QTextEdit;
    setCentralWidget(textEdit);

    textEdit->installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == textEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            qDebug() << "Ate key press" << keyEvent->key();
            return true;
        } else {
            return false;
        }
    } else {
        // pass the event on to the parent class
        return QMainWindow::eventFilter(obj, event);
    }
}
//! [6]


//! [7]
myObject->moveToThread(QApplication::instance()->thread());
//! [7]


//! [8]
class MyObject : public QObject
{
    Q_OBJECT

public:
    MyObject(QObject *parent = nullptr);

protected:
    void timerEvent(QTimerEvent *event) override;
};

MyObject::MyObject(QObject *parent)
    : QObject(parent)
{
    startTimer(50);     // 50-millisecond timer
    startTimer(1000);   // 1-second timer
    startTimer(60000);  // 1-minute timer

    using namespace std::chrono;
    startTimer(milliseconds(50));
    startTimer(seconds(1));
    startTimer(minutes(1));

    // since C++14 we can use std::chrono::duration literals, e.g.:
    startTimer(100ms);
    startTimer(5s);
    startTimer(2min);
    startTimer(1h);
}

void MyObject::timerEvent(QTimerEvent *event)
{
    qDebug() << "Timer ID:" << event->timerId();
}
//! [8]


//! [9]
QList<QObject *> list = window()->queryList("QAbstractButton"));
foreach (QObject *obj, list)
    static_cast<QAbstractButton *>(obj)->setEnabled(false);
//! [9]


//! [10]
QPushButton *button = parentWidget->findChild<QPushButton *>("button1");
//! [10]


//! [11]
QListWidget *list = parentWidget->findChild<QListWidget *>();
//! [11]


//! [12]
QList<QWidget *> widgets = parentWidget.findChildren<QWidget *>("widgetname");
//! [12]


//! [13]
QList<QPushButton *> allPButtons = parentWidget.findChildren<QPushButton *>();
//! [13]


//! [14]
monitoredObj->installEventFilter(filterObj);
//! [14]


//! [15]
class KeyPressEater : public QObject
{
    Q_OBJECT
    ...

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        qDebug("Ate key press %d", keyEvent->key());
        return true;
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}
//! [15]


//! [16]
KeyPressEater *keyPressEater = new KeyPressEater(this);
QPushButton *pushButton = new QPushButton(this);
QListView *listView = new QListView(this);

pushButton->installEventFilter(keyPressEater);
listView->installEventFilter(keyPressEater);
//! [16]


//! [17]
MyWindow::MyWindow()
{
    QLabel *senderLabel = new QLabel(tr("Name:"));
    QLabel *recipientLabel = new QLabel(tr("Name:", "recipient"));
//! [17]
}


//! [18]
int n = messages.count();
showMessage(tr("%n message(s) saved", "", n));
//! [18]


//! [19]
n == 1 ? tr("%n message saved") : tr("%n messages saved")
//! [19]


//! [20]
label->setText(tr("F\374r \310lise"));
//! [20]


//! [21]
if (receivers(SIGNAL(valueChanged(QByteArray))) > 0) {
    QByteArray data;
    get_the_value(&data);       // expensive operation
    emit valueChanged(data);
}
//! [21]


//! [22]
QLabel *label = new QLabel;
QScrollBar *scrollBar = new QScrollBar;
QObject::connect(scrollBar, SIGNAL(valueChanged(int)),
                 label,  SLOT(setNum(int)));
//! [22]


//! [23]
// WRONG
QObject::connect(scrollBar, SIGNAL(valueChanged(int value)),
                 label, SLOT(setNum(int value)));
//! [23]


//! [24]
class MyWidget : public QWidget
{
    Q_OBJECT

public:
    MyWidget();

signals:
    void buttonClicked();

private:
    QPushButton *myButton;
};

MyWidget::MyWidget()
{
    myButton = new QPushButton(this);
    connect(myButton, SIGNAL(clicked()),
            this, SIGNAL(buttonClicked()));
}
//! [24]


//! [25]
QObject::connect: Cannot queue arguments of type 'MyType'
(Make sure 'MyType' is registered using qRegisterMetaType().)
//! [25]


//! [26]
disconnect(myObject, nullptr, nullptr, nullptr);
//! [26]


//! [27]
myObject->disconnect();
//! [27]


//! [28]
disconnect(myObject, SIGNAL(mySignal()), nullptr, nullptr);
//! [28]


//! [29]
myObject->disconnect(SIGNAL(mySignal()));
//! [29]


//! [30]
disconnect(myObject, nullptr, myReceiver, nullptr);
//! [30]


//! [31]
myObject->disconnect(myReceiver);
//! [31]


//! [32]
if (signal == QMetaMethod::fromSignal(&MyObject::valueChanged)) {
    // signal is valueChanged
}
//! [32]


//! [33]
void on_<object name>_<signal name>(<signal parameters>);
//! [33]


//! [34]
void on_button1_clicked();
//! [34]


//! [35]
class MyClass : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("Author", "Pierre Gendron")
    Q_CLASSINFO("URL", "http://www.my-organization.qc.ca")

public:
    ...
};
//! [35]

//! [37]
Q_PROPERTY(QString title READ title WRITE setTitle USER true)
//! [37]


//! [38]
class MyClass : public QObject
{
    Q_OBJECT

public:
    MyClass(QObject *parent = nullptr);
    ~MyClass();

    enum Priority { High, Low, VeryHigh, VeryLow };
    Q_ENUM(Priority)
    void setPriority(Priority priority);
    Priority priority() const;
};
//! [38]


//! [39]
class QItemSelectionModel : public QObject
{
    Q_OBJECT

public:
    ...
    enum SelectionFlag {
        NoUpdate       = 0x0000,
        Clear          = 0x0001,
        Select         = 0x0002,
        Deselect       = 0x0004,
        Toggle         = 0x0008,
        Current        = 0x0010,
        Rows           = 0x0020,
        Columns        = 0x0040,
        SelectCurrent  = Select | Current,
        ToggleCurrent  = Toggle | Current,
        ClearAndSelect = Clear | Select
    };

    Q_DECLARE_FLAGS(SelectionFlags, SelectionFlag)
    Q_FLAG(SelectionFlags)
    ...
}
//! [39]


//! [40]
//: This name refers to a host name.
hostNameLabel->setText(tr("Name:"));

/*: This text refers to a C++ code example. */
QString example = tr("Example");
//! [40]

//! [41]
QPushButton *button = parentWidget->findChild<QPushButton *>("button1", Qt::FindDirectChildrenOnly);
//! [41]


//! [42]
QListWidget *list = parentWidget->findChild<QListWidget *>(QString(), Qt::FindDirectChildrenOnly);
//! [42]


//! [43]
QList<QPushButton *> childButtons = parentWidget.findChildren<QPushButton *>(QString(), Qt::FindDirectChildrenOnly);
//! [43]

//! [44]
QLabel *label = new QLabel;
QLineEdit *lineEdit = new QLineEdit;
QObject::connect(lineEdit, &QLineEdit::textChanged,
                 label,  &QLabel::setText);
//! [44]

//! [45]
void someFunction();
QPushButton *button = new QPushButton;
QObject::connect(button, &QPushButton::clicked, someFunction);
//! [45]

//! [46]
QByteArray page = ...;
QTcpSocket *socket = new QTcpSocket;
socket->connectToHost("qt-project.org", 80);
QObject::connect(socket, &QTcpSocket::connected, [=] () {
        socket->write("GET " + page + "\r\n");
    });
//! [46]

//! [47]
disconnect(myObject, &MyObject::mySignal(), nullptr, nullptr);
//! [47]

//! [48]
QObject::disconnect(lineEdit, &QLineEdit::textChanged,
                 label,  &QLabel::setText);
//! [48]

//! [49]
static const QMetaMethod valueChangedSignal = QMetaMethod::fromSignal(&MyObject::valueChanged);
if (isSignalConnected(valueChangedSignal)) {
    QByteArray data;
    data = get_the_value();       // expensive operation
    emit valueChanged(data);
}
//! [49]

//! [50]
void someFunction();
QPushButton *button = new QPushButton;
QObject::connect(button, &QPushButton::clicked, this, someFunction, Qt::QueuedConnection);
//! [50]

//! [51]
QByteArray page = ...;
QTcpSocket *socket = new QTcpSocket;
socket->connectToHost("qt-project.org", 80);
QObject::connect(socket, &QTcpSocket::connected, this, [=] () {
        socket->write("GET " + page + "\r\n");
    }, Qt::AutoConnection);
//! [51]

//! [52]
class MyClass : public QWidget
{
    Q_OBJECT

public:
    MyClass(QWidget *parent = nullptr);
    ~MyClass();

    bool event(QEvent* ev) override
    {
        if (ev->type() == QEvent::PolishRequest) {
            // overwrite handling of PolishRequest if any
            doThings();
            return true;
        } else  if (ev->type() == QEvent::Show) {
            // complement handling of Show if any
            doThings2();
            QWidget::event(ev);
            return true;
        }
        // Make sure the rest of events are handled
        return QWidget::event(ev);
    }
};
//! [52]

//! [meta data]
//: This is a comment for the translator.
//= qtn_foo_bar
//~ loc-layout_id foo_dialog
//~ loc-blank False
//~ magic-stuff This might mean something magic.
QString text = MyMagicClass::tr("Sim sala bim.");
//! [meta data]

//! [explicit tr context]
QString text = QScrollBar::tr("Page up");
//! [explicit tr context]

//! [53]
{
const QSignalBlocker blocker(someQObject);
// no signals here
}
//! [53]

//! [54]
const bool wasBlocked = someQObject->blockSignals(true);
// no signals here
someQObject->blockSignals(wasBlocked);
//! [54]
