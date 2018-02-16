/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "controllerwindow.h"
#include "controls.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>

#include <QMoveEvent>

#if QT_VERSION >= 0x050000
#  include <QWindow>
#  include <qlogging.h>
#  include <QLibraryInfo>
#endif
#include <QDebug>

ControllerWidget::ControllerWidget(QWidget *parent)
    : QWidget(parent)
    , previewWidget(0)
{
    parentWindow = new QMainWindow;
    parentWindow->setWindowTitle(tr("Preview parent window"));
    QLabel *label = new QLabel(tr("Parent window"));
    parentWindow->setCentralWidget(label);

    previewWindow = new PreviewWindow;
    previewWindow->installEventFilter(this);
    previewWidget = new PreviewWidget;
    previewWidget->installEventFilter(this);
    previewDialog = new PreviewDialog;
    previewDialog->installEventFilter(this);

    createTypeGroupBox();

    hintsControl = new HintControl;
    hintsControl->setHints(previewWidget->windowFlags());
    connect(hintsControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(updatePreview()));

    statesControl = new WindowStatesControl;
    statesControl->setStates(previewWidget->windowState());
    statesControl->setVisibleValue(true);
    connect(statesControl, SIGNAL(changed()), this, SLOT(updatePreview()));

    typeControl = new TypeControl;
    typeControl->setType(previewWidget->windowFlags());
    connect(typeControl, SIGNAL(changed(Qt::WindowFlags)), this, SLOT(updatePreview()));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(widgetTypeGroupBox);
    mainLayout->addWidget(additionalOptionsGroupBox);
    mainLayout->addWidget(typeControl);
    mainLayout->addWidget(hintsControl);
    mainLayout->addWidget(statesControl);

    updatePreview();
}

bool ControllerWidget::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::WindowStateChange)
        updateStateControl();
    return false;
}

void ControllerWidget::updateStateControl()
{
    if (activePreview)
        statesControl->setStates(activePreview->windowStates());
}

void ControllerWidget::updatePreview(QWindow *preview)
{
    activePreview = preview;

    const Qt::WindowFlags flags = typeControl->type() | hintsControl->hints();

    if (modalWindowCheckBox->isChecked()) {
        parentWindow->show();
        preview->setModality(Qt::WindowModal);
        preview->setParent(parentWindow->windowHandle());
    } else {
        preview->setModality(Qt::NonModal);
        preview->setParent(0);
        parentWindow->hide();
    }

    preview->setFlags(flags);

    if (fixedSizeWindowCheckBox->isChecked()) {
        preview->setMinimumSize(preview->size());
        preview->setMaximumSize(preview->size());
    } else {
        preview->setMinimumSize(QSize(0, 0));
        preview->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    }

    preview->setWindowStates(statesControl->states());
    preview->setVisible(statesControl->visibleValue());
}

void ControllerWidget::updatePreview(QWidget *preview)
{
    activePreview = preview->windowHandle();

    const Qt::WindowFlags flags = typeControl->type() | hintsControl->hints();

    if (modalWindowCheckBox->isChecked()) {
        parentWindow->show();
        preview->setWindowModality(Qt::WindowModal);
        preview->setParent(parentWindow);
    } else {
        preview->setWindowModality(Qt::NonModal);
        preview->setParent(0);
        parentWindow->hide();
    }

    preview->setWindowFlags(flags);

    QSize fixedSize = fixedSizeWindowCheckBox->isChecked() ?
        preview->size() : QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    preview->setFixedSize(fixedSize);

    QPoint pos = preview->pos();
    if (pos.x() < 0)
        pos.setX(0);
    if (pos.y() < 0)
        pos.setY(0);
    preview->move(pos);

    preview->setWindowState(statesControl->states());
    preview->setVisible(statesControl->visibleValue());
}

void ControllerWidget::updatePreview()
{
    if (previewWindowButton->isChecked()) {
        previewDialog->hide();
        previewWidget->close();
        updatePreview(previewWindow);
    } else if (previewWidgetButton->isChecked()) {
        previewWindow->hide();
        previewDialog->hide();
        updatePreview(previewWidget);
    } else {
        previewWindow->hide();
        previewWidget->close();
        updatePreview(previewDialog);
    }
}

void ControllerWidget::createTypeGroupBox()
{
    widgetTypeGroupBox = new QGroupBox(tr("Window Type"));
    previewWindowButton = createRadioButton(tr("QWindow"));
    previewWidgetButton = createRadioButton(tr("QWidget"));
    previewDialogButton = createRadioButton(tr("QDialog"));
    previewWindowButton->setChecked(true);
    QHBoxLayout *l = new QHBoxLayout;
    l->addWidget(previewWindowButton);
    l->addWidget(previewWidgetButton);
    l->addWidget(previewDialogButton);
    widgetTypeGroupBox->setLayout(l);

    additionalOptionsGroupBox = new QGroupBox(tr("Additional options"));
    l = new QHBoxLayout;
    modalWindowCheckBox = createCheckBox(tr("Modal window"));
    fixedSizeWindowCheckBox = createCheckBox(tr("Fixed size window"));
    l->addWidget(modalWindowCheckBox);
    l->addWidget(fixedSizeWindowCheckBox);
    additionalOptionsGroupBox->setLayout(l);
}

QCheckBox *ControllerWidget::createCheckBox(const QString &text)
{
    QCheckBox *checkBox = new QCheckBox(text);
    connect(checkBox, SIGNAL(clicked()), this, SLOT(updatePreview()));
    return checkBox;
}

QRadioButton *ControllerWidget::createRadioButton(const QString &text)
{
    QRadioButton *button = new QRadioButton(text);
    connect(button, SIGNAL(clicked()), this, SLOT(updatePreview()));
    return button;
}

static bool isTopLevel(const QObject *o)
{
    if (o->isWidgetType())
        return static_cast<const QWidget *>(o)->isWindow();
#if QT_VERSION >= 0x050000
    if (o->isWindowType())
        return static_cast<const QWindow *>(o)->isTopLevel();
#endif
    return false;
}

static Qt::WindowStates windowState(const QObject *o)
{
    if (o->isWidgetType()) {
        Qt::WindowStates states = static_cast<const QWidget *>(o)->windowState();
        states &= ~Qt::WindowActive;
        return states;
    }
#if QT_VERSION >= 0x050000
    if (o->isWindowType())
        return static_cast<const QWindow *>(o)->windowState();
#endif
    return Qt::WindowNoState;
}

class EventFilter : public QObject {
public:
    explicit EventFilter(QObject *parent = 0) : QObject(parent) {}

    bool eventFilter(QObject *o, QEvent *e)
    {
        switch (e->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::WindowStateChange:
        case QEvent::ApplicationActivate:
        case QEvent::ApplicationDeactivate:
#if QT_VERSION >= 0x050000
        case QEvent::ApplicationStateChange:
#endif
            if (isTopLevel(o))
                formatEvent(o, e);
            break;
        default:
            break;
        }
        return QObject::eventFilter(o ,e);
    }

private:
    void formatEvent(QObject *o, QEvent *e)
    {
        static int n = 0;
        QDebug debug = qDebug().nospace();
#if QT_VERSION >= 0x050000
        debug.noquote();
#endif
        debug << '#' << n++ << ' ' << o->metaObject()->className();
        const QString name = o->objectName();
        if (!name.isEmpty())
            debug << "/\"" << name << '"';
        debug << ' ' << e;
        if (e->type() == QEvent::WindowStateChange)
            debug << ' ' << windowState(o);
    }
};

LogWidget *LogWidget::m_instance = 0;

#if QT_VERSION >= 0x050000
static void qt5MessageHandler(QtMsgType, const QMessageLogContext &, const QString &text)
{
    if (LogWidget *lw = LogWidget::instance())
        lw->appendText(text);
}
#else // Qt 5
static void qt4MessageHandler(QtMsgType, const char *text)
{
    if (LogWidget *lw = LogWidget::instance())
        lw->appendText(QString::fromLocal8Bit(text));
}
#endif // Qt 4

LogWidget::LogWidget(QWidget *parent)
    : QPlainTextEdit(parent)
{
    LogWidget::m_instance = this;
    setReadOnly(true);
    appendText(startupMessage());
}

LogWidget::~LogWidget()
{
    LogWidget::m_instance = 0;
}

void LogWidget::install()
{
#if QT_VERSION >= 0x050000
    qInstallMessageHandler(qt5MessageHandler);
#else
    qInstallMsgHandler(qt4MessageHandler);
#endif
}

QString LogWidget::startupMessage()
{
    QString result;
#if QT_VERSION >= 0x050300
    result += QLatin1String(QLibraryInfo::build());
#else
    result += QLatin1String("Qt ") + QLatin1String(QT_VERSION_STR);
#endif
#if QT_VERSION >= 0x050000
    result += QLatin1Char(' ');
    result += QGuiApplication::platformName();
#endif
    return result;
}

void LogWidget::appendText(const QString &message)
{
    appendPlainText(message);
    ensureCursorVisible();
}

ControllerWindow::ControllerWindow()
{
    setWindowTitle(tr("Window Flags (Qt version %1, %2)")
                   .arg(QLatin1String(qVersion()),
#if QT_VERSION >= 0x050000
                        qApp->platformName()));
#else
                        QLatin1String("<unknown>")));
#endif

    QVBoxLayout *layout = new QVBoxLayout(this);
    QTabWidget *tabWidget = new QTabWidget(this);
    ControllerWidget *controllerWidget = new ControllerWidget(tabWidget);
    tabWidget->addTab(controllerWidget, tr("Control"));
    LogWidget *logWidget = new LogWidget(tabWidget);
    tabWidget->addTab(logWidget, tr("Event log"));
    layout->addWidget(tabWidget);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    layout->addLayout(bottomLayout);
    bottomLayout->addStretch();
    QPushButton *updateControlsButton = new QPushButton(tr("&Update"));
    connect(updateControlsButton, SIGNAL(clicked()), controllerWidget, SLOT(updateStateControl()));
    bottomLayout->addWidget(updateControlsButton);
    QPushButton *clearLogButton = new QPushButton(tr("Clear &Log"));
    connect(clearLogButton, SIGNAL(clicked()), logWidget, SLOT(clear()));
    bottomLayout->addWidget(clearLogButton);
    QPushButton *quitButton = new QPushButton(tr("&Quit"));
    connect(quitButton, SIGNAL(clicked()), qApp, SLOT(quit()));
    quitButton->setShortcut(Qt::CTRL + Qt::Key_Q);
    bottomLayout->addWidget(quitButton);
}

void ControllerWindow::registerEventFilter()
{
    qApp->installEventFilter(new EventFilter(qApp));
}
