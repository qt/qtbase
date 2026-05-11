// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qohosplatformdrag.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qobject.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qdrag.h>
#include <QtGui/qwindow.h>
#include <qohosinputmethodeventhandler.h>
#include <qohosjsmain.h>
#include <qohosplatformwindow.h>
#include <qohosutils.h>
#include <render/qohosview.h>

QT_BEGIN_NAMESPACE

namespace {

class DragEventFilterObject : public QObject
{
public:
    DragEventFilterObject();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
};

class QOhosPlatformDragImpl : public QOhosPlatformDrag
{
public:
    QOhosPlatformDragImpl();
    ~QOhosPlatformDragImpl() override;

    void handlePreDrop() override;

    void updateDropAction(Qt::DropAction dropAction) override;

protected:
    Qt::DropAction drag(QDrag *drag) override;

private:
    void startDrag();

    Qt::DropAction m_dropAction = Qt::IgnoreAction;
    std::shared_ptr<void> m_activeEventFilterHandle;
};

std::shared_ptr<void> installDragEventFilter()
{
    auto eventFilterObject = std::make_shared<DragEventFilterObject>();

    qApp->installEventFilter(eventFilterObject.get());

    return QtOhos::makeDestroyNotifier(
        [eventFilterObject]() {
            qApp->removeEventFilter(eventFilterObject.get());
        });
}

QOhosView *findInitiatorViewForDragOrNull()
{
    QWindow *currentMouseWindow = QGuiApplicationPrivate::currentMouseWindow;
    auto *touchedWindow = QOhosPlatformIntegration::instance()->inputMethodEventHandler()->lastTouchedWindowOrNull();
    auto *qWindow = touchedWindow != nullptr ? touchedWindow : currentMouseWindow;
    if (qWindow != nullptr) {
        auto *platformWindow = QOhosPlatformWindow::fromQWindowOrNull(qWindow);
        return platformWindow != nullptr
            ? platformWindow->ownedViewOrNull()
            : nullptr;
    } else {
        return nullptr;
    }
}

DragEventFilterObject::DragEventFilterObject() = default;

bool DragEventFilterObject::eventFilter(QObject *, QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        return true;
    default:
        return false;
    }
}

QOhosPlatformDragImpl::QOhosPlatformDragImpl() = default;

QOhosPlatformDragImpl::~QOhosPlatformDragImpl() = default;

void QOhosPlatformDragImpl::handlePreDrop()
{
    m_activeEventFilterHandle.reset();
}

void QOhosPlatformDragImpl::updateDropAction(Qt::DropAction dropAction)
{
    m_dropAction = dropAction;
}

Qt::DropAction QOhosPlatformDragImpl::drag(QDrag *drag)
{
    auto *initiatorView = findInitiatorViewForDragOrNull();
    if (initiatorView == nullptr || drag == nullptr || drag->mimeData() == nullptr)
        return Qt::IgnoreAction;

    m_activeEventFilterHandle = installDragEventFilter();

    auto eventLoop = std::make_shared<QEventLoop>();

    m_dropAction = drag->defaultAction();

    auto dragPixmap = !drag->pixmap().isNull() ? drag->pixmap() : QPlatformDrag::defaultPixmap();
    initiatorView->startDrag(
        {dragPixmap.toImage()}, drag->hotSpot(),
        *drag->mimeData(),
        [this, eventLoop](Qt::DropAction dropAction) {
            m_dropAction = dropAction;
            eventLoop->quit();
        });

    eventLoop->exec();

    m_activeEventFilterHandle.reset();

    return m_dropAction;
}

}

QOhosPlatformDrag::QOhosPlatformDrag() = default;

QOhosPlatformDrag::~QOhosPlatformDrag() = default;

std::unique_ptr<QOhosPlatformDrag> makeQOhosPlatformDrag()
{
    return std::make_unique<QOhosPlatformDragImpl>();
}

QT_END_NAMESPACE
