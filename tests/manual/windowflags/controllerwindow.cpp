// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "controllerwindow.h"
#include "controls.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDebug>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLibraryInfo>
#include <qlogging.h>
#include <QMainWindow>
#include <QMenu>
#include <QMoveEvent>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>
#include <QWindow>

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
    if (o->isWindowType())
        return static_cast<const QWindow *>(o)->isTopLevel();
    return false;
}

static Qt::WindowStates windowState(const QObject *o)
{
    if (o->isWidgetType()) {
        Qt::WindowStates states = static_cast<const QWidget *>(o)->windowState();
        states &= ~Qt::WindowActive;
        return states;
    }
    if (o->isWindowType())
        return static_cast<const QWindow *>(o)->windowStates();
    return Qt::WindowNoState;
}

class EventFilter : public QObject {
public:
    explicit EventFilter(QObject *parent = nullptr) : QObject(parent) {}

    bool eventFilter(QObject *o, QEvent *e)
    {
        switch (e->type()) {
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::WindowStateChange:
        case QEvent::ApplicationActivate:
        case QEvent::ApplicationDeactivate:
        case QEvent::ApplicationStateChange:
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
        debug.noquote();
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

static QtMessageHandler originalMessageHandler = nullptr;

static void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &text)
{
    if (LogWidget *lw = LogWidget::instance())
        lw->appendText(text);

    originalMessageHandler(type, context, text);
}

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
    originalMessageHandler = qInstallMessageHandler(messageHandler);
}

QString LogWidget::startupMessage()
{
    QString result;
    result += QLatin1String(QLibraryInfo::build());
    result += QLatin1Char(' ');
    result += QGuiApplication::platformName();
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
                        qApp->platformName()));

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
    quitButton->setShortcut(Qt::CTRL | Qt::Key_Q);
    bottomLayout->addWidget(quitButton);
}

void ControllerWindow::registerEventFilter()
{
    qApp->installEventFilter(new EventFilter(qApp));
}
