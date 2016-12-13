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

#include <QPlainTextEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QTextStream>
#include <QTimer>

#include "previewwindow.h"

static void formatWindowFlags(QTextStream &str, Qt::WindowFlags flags)
{
    str << "Window flags: " << hex << showbase << unsigned(flags) << noshowbase << dec << ' ';
    switch (flags & Qt::WindowType_Mask) {
    case Qt::Window:
        str << "Qt::Window";
        break;
    case Qt::Dialog:
        str << "Qt::Dialog";
        break;
    case Qt::Sheet:
        str << "Qt::Sheet";
        break;
    case Qt::Drawer:
        str << "Qt::Drawer";
        break;
    case Qt::Popup:
        str << "Qt::Popup";
        break;
    case Qt::Tool:
        str << "Qt::Tool";
        break;
    case Qt::ToolTip:
        str << "Qt::ToolTip";
        break;
    case Qt::SplashScreen:
        str << "Qt::SplashScreen";
        break;
    }

    if (flags & Qt::MSWindowsFixedSizeDialogHint)
        str << "\n| Qt::MSWindowsFixedSizeDialogHint";
#if QT_VERSION >= 0x050000
    if (flags & Qt::BypassWindowManagerHint)
        str << "\n| Qt::BypassWindowManagerHint";
#else
    if (flags & Qt::X11BypassWindowManagerHint)
        str << "\n| Qt::X11BypassWindowManagerHint";
#endif
    if (flags & Qt::FramelessWindowHint)
        str << "\n| Qt::FramelessWindowHint";
    if (flags & Qt::WindowTitleHint)
        str << "\n| Qt::WindowTitleHint";
    if (flags & Qt::WindowSystemMenuHint)
        str << "\n| Qt::WindowSystemMenuHint";
    if (flags & Qt::WindowMinimizeButtonHint)
        str << "\n| Qt::WindowMinimizeButtonHint";
    if (flags & Qt::WindowMaximizeButtonHint)
        str << "\n| Qt::WindowMaximizeButtonHint";
    if (flags & Qt::WindowCloseButtonHint)
        str << "\n| Qt::WindowCloseButtonHint";
    if (flags & Qt::WindowContextHelpButtonHint)
        str << "\n| Qt::WindowContextHelpButtonHint";
    if (flags & Qt::WindowShadeButtonHint)
        str << "\n| Qt::WindowShadeButtonHint";
    if (flags & Qt::WindowStaysOnTopHint)
        str << "\n| Qt::WindowStaysOnTopHint";
    if (flags & Qt::CustomizeWindowHint)
        str << "\n| Qt::CustomizeWindowHint";
    if (flags & Qt::WindowStaysOnBottomHint)
        str << "\n| Qt::WindowStaysOnBottomHint";
#if QT_VERSION >= 0x050000
    if (flags & Qt::WindowFullscreenButtonHint)
        str << "\n| Qt::WindowFullscreenButtonHint";
    if (flags & Qt::WindowTransparentForInput)
        str << "\n| Qt::WindowTransparentForInput";
    if (flags & Qt::WindowOverridesSystemGestures)
        str << "\n| Qt::WindowOverridesSystemGestures";
    if (flags & Qt::WindowDoesNotAcceptFocus)
        str << "\n| Qt::WindowDoesNotAcceptFocus";
    if (flags & Qt::MaximizeUsingFullscreenGeometryHint)
        str << "\n| Qt::MaximizeUsingFullscreenGeometryHint";
    if (flags & Qt::NoDropShadowWindowHint)
        str << "\n| Qt::NoDropShadowWindowHint";
#endif // Qt 5
}

static void formatWindowStates(QTextStream &str, Qt::WindowStates states)
{
    str << "Window states: " << hex << showbase << unsigned(states) << noshowbase << dec << ' ';
    if (states & Qt::WindowActive) {
        str << "Qt::WindowActive ";
        states &= ~Qt::WindowActive;
    }
    switch (states) {
    case Qt::WindowNoState:
        str << "Qt::WindowNoState";
        break;
    case Qt::WindowMinimized:
        str << "Qt::WindowMinimized";
        break;
    case Qt::WindowMaximized:
        str << "Qt::WindowMaximized";
        break;
    case Qt::WindowFullScreen:
        str << "Qt::WindowFullScreen";
        break;
    default:
        break;
    }
}

QTextStream &operator<<(QTextStream &str, const QRect &r)
{
    str << r.width() << 'x' << r.height() << forcesign << r.x() << r.y() << noforcesign;
    return str;
}

static QString formatWidgetInfo(const QWidget *w)
{
    QString result;
    QTextStream str(&result);
    formatWindowFlags(str, w->windowFlags());
    str << '\n';
    formatWindowStates(str, w->windowState());
    const QRect frame = w->frameGeometry();
    const QRect geometry = w->geometry();
    str << "\n\nFrame: " << frame << "\nGeometry: " << geometry << "\nMargins: "
        << (geometry.x() - frame.x()) << ", " << (geometry.top() - frame.top())
        << ", " << (frame.right() - geometry.right()) << ", "
        << (frame.bottom() - geometry.bottom());
    return result;
}

static QPlainTextEdit *createControlPanel(QWidget *widget)
{
    QVBoxLayout *layout = new QVBoxLayout(widget);
    QPlainTextEdit *textEdit = new QPlainTextEdit;
    textEdit->setReadOnly(true);
    textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    layout->addWidget(textEdit);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    layout ->addLayout(bottomLayout);
    QGridLayout *buttonLayout = new QGridLayout;
    bottomLayout->addStretch();
    bottomLayout->addLayout(buttonLayout);
    QPushButton *showNormalButton = new QPushButton(PreviewWindow::tr("Show normal"));
    QObject::connect(showNormalButton, SIGNAL(clicked()), widget, SLOT(showNormal()));
    buttonLayout->addWidget(showNormalButton, 0, 0);
    QPushButton *showMinimizedButton = new QPushButton(PreviewWindow::tr("Show minimized"));
    QObject::connect(showMinimizedButton, SIGNAL(clicked()), widget, SLOT(showMinimized()));
    buttonLayout->addWidget(showMinimizedButton, 0, 1);
    QPushButton *showMaximizedButton = new QPushButton(PreviewWindow::tr("Show maximized"));
    QObject::connect(showMaximizedButton, SIGNAL(clicked()), widget, SLOT(showMaximized()));
    buttonLayout->addWidget(showMaximizedButton, 0, 2);
    QPushButton *showFullScreenButton = new QPushButton(PreviewWindow::tr("Show fullscreen"));
    QObject::connect(showFullScreenButton, SIGNAL(clicked()), widget, SLOT(showFullScreen()));
    buttonLayout->addWidget(showFullScreenButton, 0, 3);

    QPushButton *updateInfoButton = new QPushButton(PreviewWindow::tr("&Update Info"));
    QObject::connect(updateInfoButton, SIGNAL(clicked()), widget, SLOT(updateInfo()));
    buttonLayout->addWidget(updateInfoButton, 1, 0);
    QPushButton *closeButton = new QPushButton(PreviewWindow::tr("&Close"));
    QObject::connect(closeButton, SIGNAL(clicked()), widget, SLOT(close()));
    buttonLayout->addWidget(closeButton, 1, 3);

    return textEdit;
}

PreviewWindow::PreviewWindow(QWidget *parent)
    : QWidget(parent)
{
    textEdit = createControlPanel(this);
    setWindowTitle(tr("Preview <QWidget> Qt %1").arg(QLatin1String(QT_VERSION_STR)));
}

void PreviewWindow::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    updateInfo();
}

void PreviewWindow::moveEvent(QMoveEvent *e)
{
    QWidget::moveEvent(e);
    updateInfo();
}

void PreviewWindow::setWindowFlags(Qt::WindowFlags flags)
{
    if (flags == windowFlags())
        return;
    QWidget::setWindowFlags(flags);
    QTimer::singleShot(0, this, SLOT(updateInfo()));
}

void PreviewWindow::updateInfo()
{
      textEdit->setPlainText(formatWidgetInfo(this));
}

PreviewDialog::PreviewDialog(QWidget *parent)
    : QDialog(parent)
{
    textEdit = createControlPanel(this);
    setWindowTitle(tr("Preview <QDialog> Qt %1").arg(QLatin1String(QT_VERSION_STR)));
}

void PreviewDialog::resizeEvent(QResizeEvent *e)
{
    QDialog::resizeEvent(e);
    updateInfo();
}

void PreviewDialog::moveEvent(QMoveEvent *e)
{
    QDialog::moveEvent(e);
    updateInfo();
}

void PreviewDialog::setWindowFlags(Qt::WindowFlags flags)
{
    if (flags == windowFlags())
        return;
    QWidget::setWindowFlags(flags);
    QTimer::singleShot(0, this, SLOT(updateInfo()));
}

void PreviewDialog::updateInfo()
{
    textEdit->setPlainText(formatWidgetInfo(this));
}
