/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsdrag.h"
#include "qwindowscontext.h"
#include "qwindowsclipboard.h"
#include "qwindowsintegration.h"
#include "qwindowsole.h"
#include "qtwindows_additional.h"
#include "qwindowswindow.h"
#include "qwindowsmousehandler.h"
#include "qwindowscursor.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPixmap>
#include <QtGui/QPainter>
#include <QtGui/QGuiApplication>

#include <QtCore/QDebug>
#include <QtCore/QPoint>

#include <shlobj.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindowsDropMimeData
    \brief Special mime data class for data retrieval from Drag operations.

    Implementation of QWindowsInternalMimeDataBase which retrieves the
    current drop data object from QWindowsDrag.

    \sa QWindowsDrag
    \ingroup qt-lighthouse-win
*/

IDataObject *QWindowsDropMimeData::retrieveDataObject() const
{
    return QWindowsDrag::instance()->dropDataObject();
}

static inline Qt::DropActions translateToQDragDropActions(DWORD pdwEffects)
{
    Qt::DropActions actions = Qt::IgnoreAction;
    if (pdwEffects & DROPEFFECT_LINK)
        actions |= Qt::LinkAction;
    if (pdwEffects & DROPEFFECT_COPY)
        actions |= Qt::CopyAction;
    if (pdwEffects & DROPEFFECT_MOVE)
        actions |= Qt::MoveAction;
    return actions;
}

static inline Qt::DropAction translateToQDragDropAction(DWORD pdwEffect)
{
    if (pdwEffect & DROPEFFECT_LINK)
        return Qt::LinkAction;
    if (pdwEffect & DROPEFFECT_COPY)
        return Qt::CopyAction;
    if (pdwEffect & DROPEFFECT_MOVE)
        return Qt::MoveAction;
    return Qt::IgnoreAction;
}

static inline DWORD translateToWinDragEffects(Qt::DropActions action)
{
    DWORD effect = DROPEFFECT_NONE;
    if (action & Qt::LinkAction)
        effect |= DROPEFFECT_LINK;
    if (action & Qt::CopyAction)
        effect |= DROPEFFECT_COPY;
    if (action & Qt::MoveAction)
        effect |= DROPEFFECT_MOVE;
    return effect;
}

static inline Qt::KeyboardModifiers toQtKeyboardModifiers(DWORD keyState)
{
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    if (keyState & MK_SHIFT)
        modifiers |= Qt::ShiftModifier;
    if (keyState & MK_CONTROL)
        modifiers |= Qt::ControlModifier;
    if (keyState & MK_ALT)
        modifiers |= Qt::AltModifier;

    return modifiers;
}

/*!
    \class QWindowsOleDropSource
    \brief Implementation of IDropSource

    Used for drag operations.

    \sa QWindowsDrag
    \ingroup qt-lighthouse-win
*/

class QWindowsOleDropSource : public IDropSource
{
public:
    QWindowsOleDropSource();
    virtual ~QWindowsOleDropSource();

    void createCursors();

    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void ** ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);

    // IDropSource methods
    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD grfKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

private:
    typedef QMap <Qt::DropAction, HCURSOR> ActionCursorMap;

    inline void clearCursors();

    Qt::MouseButtons m_currentButtons;
    Qt::DropAction m_currentAction;
    ActionCursorMap m_cursors;

    ULONG m_refs;
};

QWindowsOleDropSource::QWindowsOleDropSource() :
    m_currentButtons(Qt::NoButton), m_currentAction(Qt::IgnoreAction),
    m_refs(1)
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s", __FUNCTION__);
}

QWindowsOleDropSource::~QWindowsOleDropSource()
{
    clearCursors();
    if (QWindowsContext::verboseOLE)
        qDebug("%s", __FUNCTION__);
}

void QWindowsOleDropSource::createCursors()
{
    QDragManager *manager = QDragManager::self();
    if (!manager || !manager->object)
        return;
    const QPixmap pixmap = manager->object->pixmap();
    const bool hasPixmap = !pixmap.isNull();
    if (!hasPixmap && manager->dragPrivate()->customCursors.isEmpty())
        return;

    QList<Qt::DropAction> actions;
    actions << Qt::MoveAction << Qt::CopyAction << Qt::LinkAction;
    if (hasPixmap)
        actions << Qt::IgnoreAction;
    const QPoint hotSpot = manager->object->hotSpot();
    for (int cnum = 0; cnum < actions.size(); ++cnum) {
        const QPixmap cpm = manager->dragCursor(actions.at(cnum));
        int w = cpm.width();
        int h = cpm.height();

        if (hasPixmap) {
            const int x1 = qMin(-hotSpot.x(), 0);
            const int x2 = qMax(pixmap.width() - hotSpot.x(), cpm.width());
            const int y1 = qMin(-hotSpot.y(), 0);
            const int y2 = qMax(pixmap.height() - hotSpot.y(), cpm.height());

            w = x2 - x1 + 1;
            h = y2 - y1 + 1;
        }

        const QRect srcRect = pixmap.rect();
        const QPoint pmDest = QPoint(qMax(0, -hotSpot.x()), qMax(0, -hotSpot.y()));
        const QPoint newHotSpot = hotSpot;
        QPixmap newCursor(w, h);
        if (hasPixmap) {
            newCursor.fill(QColor(0, 0, 0, 0));
            QPainter p(&newCursor);
            p.drawPixmap(pmDest, pixmap, srcRect);
            p.drawPixmap(qMax(0,newHotSpot.x()),qMax(0,newHotSpot.y()),cpm);
        } else {
            newCursor = cpm;
        }

        const int hotX = hasPixmap ? qMax(0,newHotSpot.x()) : 0;
        const int hotY = hasPixmap ? qMax(0,newHotSpot.y()) : 0;

        if (const HCURSOR sysCursor = QWindowsCursor::createPixmapCursor(newCursor, hotX, hotY))
            m_cursors.insert(actions.at(cnum), sysCursor);
    }
    if (QWindowsContext::verboseOLE)
        qDebug("%s %d cursors", __FUNCTION__, m_cursors.size());
}

void QWindowsOleDropSource::clearCursors()
{
    if (!m_cursors.isEmpty()) {
        const ActionCursorMap::const_iterator cend = m_cursors.constEnd();
        for (ActionCursorMap::const_iterator it = m_cursors.constBegin(); it != cend; ++it)
            DestroyCursor(it.value());
        m_cursors.clear();
    }
}

//---------------------------------------------------------------------
//                    IUnknown Methods
//---------------------------------------------------------------------

STDMETHODIMP
QWindowsOleDropSource::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDropSource) {
      *ppv = this;
      ++m_refs;
      return NOERROR;
    }
    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
QWindowsOleDropSource::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
QWindowsOleDropSource::Release(void)
{
    if (--m_refs == 0) {
      delete this;
      return 0;
    }
    return m_refs;
}

/*!
    \brief Check for cancel.
*/

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    HRESULT hr = S_OK;
    do {
        if (fEscapePressed || QWindowsDrag::instance()->dragBeingCancelled()) {
            hr = ResultFromScode(DRAGDROP_S_CANCEL);
            break;
        }

    // grfKeyState is broken on CE & some Windows XP versions,
    // therefore we need to check the state manually
    if ((GetAsyncKeyState(VK_LBUTTON) == 0)
        && (GetAsyncKeyState(VK_MBUTTON) == 0)
        && (GetAsyncKeyState(VK_RBUTTON) == 0)) {
        hr = ResultFromScode(DRAGDROP_S_DROP);
        break;
    }

    const Qt::MouseButtons buttons =  QWindowsMouseHandler::keyStateToMouseButtons(grfKeyState);
    if (m_currentButtons == Qt::NoButton) {
        m_currentButtons = buttons;
    } else {
        // Button changed: Complete Drop operation.
        if (!(m_currentButtons & buttons)) {
            hr = ResultFromScode(DRAGDROP_S_DROP);
            break;
        }
    }

    QGuiApplication::processEvents();

    } while (false);

    QDragManager::self()->willDrop = hr == DRAGDROP_S_DROP;

    if (QWindowsContext::verboseOLE
        && (QWindowsContext::verboseOLE > 1 || hr != S_OK))
        qDebug("%s fEscapePressed=%d, grfKeyState=%lu buttons=%d willDrop = %d returns 0x%x",
               __FUNCTION__, fEscapePressed,grfKeyState, int(m_currentButtons),
               QDragManager::self()->willDrop, int(hr));
    return hr;
}

/*!
    \brief Give feedback: Change cursor accoding to action.
*/

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropSource::GiveFeedback(DWORD dwEffect)
{
    const Qt::DropAction action = translateToQDragDropAction(dwEffect);

    if (QWindowsContext::verboseOLE > 2)
        qDebug("%s dwEffect=%lu, action=%d", __FUNCTION__, dwEffect, action);

    if (m_currentAction != action) {
        m_currentAction = action;
        QDragManager::self()->emitActionChanged(m_currentAction);
    }

    const ActionCursorMap::const_iterator it = m_cursors.constFind(m_currentAction);
    if (it != m_cursors.constEnd()) {
        SetCursor(it.value());
        return ResultFromScode(S_OK);
    }

    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}

/*!
    \class QWindowsOleDropTarget
    \brief Implementation of IDropTarget

    To be registered for each window. Currently, drop sites
    are enabled for top levels. The child window handling
    (sending DragEnter/Leave, etc) is handled in here.

    \sa QWindowsDrag
    \ingroup qt-lighthouse-win
*/

QWindowsOleDropTarget::QWindowsOleDropTarget(QWindow *w) :
    m_refs(1), m_window(w), m_currentWindow(0), m_chosenEffect(0), m_lastKeyState(0)
{
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ <<  this << w;
}

QWindowsOleDropTarget::~QWindowsOleDropTarget()
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s %p", __FUNCTION__, this);
}

STDMETHODIMP
QWindowsOleDropTarget::QueryInterface(REFIID iid, void FAR* FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IDropTarget) {
      *ppv = this;
      AddRef();
      return NOERROR;
    }
    *ppv = NULL;
    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
QWindowsOleDropTarget::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG)
QWindowsOleDropTarget::Release(void)
{
    if (--m_refs == 0) {
      delete this;
      return 0;
    }
    return m_refs;
}

QWindow *QWindowsOleDropTarget::findDragOverWindow(const POINTL &pt) const
{
    if (QWindowsWindow *child =
            QWindowsWindow::baseWindowOf(m_window)->childAtScreenPoint(QPoint(pt.x, pt.y)))
            return child->window();
    return m_window;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragEnter(LPDATAOBJECT pDataObj, DWORD grfKeyState,
                                 POINTL pt, LPDWORD pdwEffect)
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s widget=%p key=%lu, pt=%ld,%ld", __FUNCTION__, m_window, grfKeyState, pt.x, pt.y);

    QWindowsDrag::instance()->setDropDataObject(pDataObj);
    pDataObj->AddRef();
    m_currentWindow = m_window;
    sendDragEnterEvent(m_window, grfKeyState, pt, pdwEffect);
    *pdwEffect = m_chosenEffect;
    return NOERROR;
}

void QWindowsOleDropTarget::sendDragEnterEvent(QWindow *dragEnterWidget,
                                               DWORD grfKeyState,
                                               POINTL pt, LPDWORD pdwEffect)
{
    Q_ASSERT(dragEnterWidget);

    m_lastPoint = QWindowsGeometryHint::mapFromGlobal(dragEnterWidget, QPoint(pt.x,pt.y));
    m_lastKeyState = grfKeyState;

    m_chosenEffect = DROPEFFECT_NONE;

    QDragManager *manager = QDragManager::self();
    QMimeData *md = manager->dropData();
    const Qt::MouseButtons mouseButtons
        = QWindowsMouseHandler::keyStateToMouseButtons(grfKeyState);
    const Qt::DropActions actions = translateToQDragDropActions(*pdwEffect);
    const Qt::KeyboardModifiers keyMods = toQtKeyboardModifiers(grfKeyState);
    QDragEnterEvent enterEvent(m_lastPoint, actions, md, mouseButtons, keyMods);
    QGuiApplication::sendEvent(m_currentWindow, &enterEvent);
    m_answerRect = enterEvent.answerRect();
    if (QWindowsContext::verboseOLE)
        qDebug() << __FUNCTION__ << " sent drag enter to " << m_window
                 << *md << " actions=" << actions
                 << " mods=" << keyMods << " accepted: "
                 << enterEvent.isAccepted();

    if (enterEvent.isAccepted())
        m_chosenEffect = translateToWinDragEffects(enterEvent.dropAction());
    // Documentation states that a drag move event is sent immediately after
    // a drag enter event. This will honor widgets overriding dragMoveEvent only:
    if (enterEvent.isAccepted()) {
        QDragMoveEvent moveEvent(m_lastPoint, actions, md, mouseButtons, keyMods);
        m_answerRect = enterEvent.answerRect();
        moveEvent.setDropAction(enterEvent.dropAction());
        moveEvent.accept(); // accept by default, since enter event was accepted.

        QGuiApplication::sendEvent(dragEnterWidget, &moveEvent);
        if (moveEvent.isAccepted()) {
            m_answerRect = moveEvent.answerRect();
            m_chosenEffect = translateToWinDragEffects(moveEvent.dropAction());
        } else {
            m_chosenEffect = DROPEFFECT_NONE;
        }
    }
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragOver(DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    QWindow *dragOverWindow = findDragOverWindow(pt);

    const QPoint tmpPoint = QWindowsGeometryHint::mapFromGlobal(dragOverWindow, QPoint(pt.x,pt.y));
    // see if we should compress this event
    if ((tmpPoint == m_lastPoint || m_answerRect.contains(tmpPoint))
        && m_lastKeyState == grfKeyState) {
        *pdwEffect = m_chosenEffect;
        return NOERROR;
    }

    if (QWindowsContext::verboseOLE > 1)
        qDebug().nospace() << '>' << __FUNCTION__ << ' ' << m_window << " current "
                           << dragOverWindow << " key=" << grfKeyState
                           << " pt=" <<pt.x << ',' << pt.y;

    if (dragOverWindow != m_currentWindow) {
        QPointer<QWindow> dragOverWindowGuard(dragOverWindow);
        // Send drag leave event to the previous drag widget.
        // Drag-Over widget might be deleted in DragLeave,
        // (tasktracker 218353).
        QDragLeaveEvent dragLeave;
        if (m_currentWindow)
            QGuiApplication::sendEvent(m_currentWindow, &dragLeave);
        if (!dragOverWindowGuard) {
            dragOverWindow = findDragOverWindow(pt);
        }
        // Send drag enter event to the current drag widget.
        m_currentWindow = dragOverWindow;
        sendDragEnterEvent(dragOverWindow, grfKeyState, pt, pdwEffect);
    }

    QDragManager *manager = QDragManager::self();
    QMimeData *md = manager->dropData();

    const Qt::DropActions actions = translateToQDragDropActions(*pdwEffect);

    QDragMoveEvent oldEvent(m_lastPoint, actions, md,
                            QWindowsMouseHandler::keyStateToMouseButtons(m_lastKeyState),
                            toQtKeyboardModifiers(m_lastKeyState));

    m_lastPoint = tmpPoint;
    m_lastKeyState = grfKeyState;

    QDragMoveEvent e(tmpPoint, actions, md,
                     QWindowsMouseHandler::keyStateToMouseButtons(grfKeyState),
                     toQtKeyboardModifiers(grfKeyState));
    if (m_chosenEffect != DROPEFFECT_NONE) {
        if (oldEvent.dropAction() == e.dropAction() &&
            oldEvent.keyboardModifiers() == e.keyboardModifiers())
            e.setDropAction(translateToQDragDropAction(m_chosenEffect));
        e.accept();
    }
    QGuiApplication::sendEvent(dragOverWindow, &e);

    m_answerRect = e.answerRect();
    if (e.isAccepted())
        m_chosenEffect = translateToWinDragEffects(e.dropAction());
    else
        m_chosenEffect = DROPEFFECT_NONE;
    *pdwEffect = m_chosenEffect;

    if (QWindowsContext::verboseOLE > 1)
        qDebug("<%s effect=0x%lx", __FUNCTION__, m_chosenEffect);
    return NOERROR;
}

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::DragLeave()
{
    if (QWindowsContext::verboseOLE)
        qDebug().nospace() <<__FUNCTION__ << ' ' << m_window;

    m_currentWindow = 0;
    QDragLeaveEvent e;
    QGuiApplication::sendEvent(m_window, &e);
    QWindowsDrag::instance()->releaseDropDataObject();

    return NOERROR;
}

#define KEY_STATE_BUTTON_MASK (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON)

QT_ENSURE_STACK_ALIGNED_FOR_SSE STDMETHODIMP
QWindowsOleDropTarget::Drop(LPDATAOBJECT /*pDataObj*/, DWORD grfKeyState,
                            POINTL pt, LPDWORD pdwEffect)
{
    QWindow *dropWindow = findDragOverWindow(pt);

    if (QWindowsContext::verboseOLE)
        qDebug().nospace() << __FUNCTION__ << ' ' << m_window
                           << " on " << dropWindow
                           << " keys=" << grfKeyState << " pt="
                           << pt.x << ',' << pt.y;

    m_lastPoint = QWindowsGeometryHint::mapFromGlobal(dropWindow, QPoint(pt.x,pt.y));
    // grfKeyState does not all ways contain button state in the drop so if
    // it doesn't then use the last known button state;
    if ((grfKeyState & KEY_STATE_BUTTON_MASK) == 0)
        grfKeyState |= m_lastKeyState & KEY_STATE_BUTTON_MASK;
    m_lastKeyState = grfKeyState;

    QWindowsDrag *windowsDrag = QWindowsDrag::instance();
    QDragManager *manager = QDragManager::self();
    QMimeData *md = manager->dropData();
    QDropEvent e(m_lastPoint, translateToQDragDropActions(*pdwEffect), md,
                 QWindowsMouseHandler::keyStateToMouseButtons(grfKeyState),
                 toQtKeyboardModifiers(grfKeyState));
    if (m_chosenEffect != DROPEFFECT_NONE)
        e.setDropAction(translateToQDragDropAction(m_chosenEffect));

    QGuiApplication::sendEvent(dropWindow, &e);
    if (m_chosenEffect != DROPEFFECT_NONE)
        e.accept();

    if (e.isAccepted()) {
        if (e.dropAction() == Qt::MoveAction || e.dropAction() == Qt::TargetMoveAction) {
            if (e.dropAction() == Qt::MoveAction)
                m_chosenEffect = DROPEFFECT_MOVE;
            else
                m_chosenEffect = DROPEFFECT_COPY;
            HGLOBAL hData = GlobalAlloc(0, sizeof(DWORD));
            if (hData) {
                DWORD *moveEffect = (DWORD *)GlobalLock(hData);;
                *moveEffect = DROPEFFECT_MOVE;
                GlobalUnlock(hData);
                STGMEDIUM medium;
                memset(&medium, 0, sizeof(STGMEDIUM));
                medium.tymed = TYMED_HGLOBAL;
                medium.hGlobal = hData;
                FORMATETC format;
                format.cfFormat = RegisterClipboardFormat(CFSTR_PERFORMEDDROPEFFECT);
                format.tymed = TYMED_HGLOBAL;
                format.ptd = 0;
                format.dwAspect = 1;
                format.lindex = -1;
                windowsDrag->dropDataObject()->SetData(&format, &medium, true);
            }
        } else {
            m_chosenEffect = translateToWinDragEffects(e.dropAction());
        }
    } else {
        m_chosenEffect = DROPEFFECT_NONE;
    }
    *pdwEffect = m_chosenEffect;

    windowsDrag->releaseDropDataObject();
    return NOERROR;
}

/*!
    \class QWindowsDrag
    \brief Windows drag implementation.

    \ingroup qt-lighthouse-win
*/

QWindowsDrag::QWindowsDrag() : m_dropDataObject(0), m_dragBeingCancelled(false)
{
}

QWindowsDrag::~QWindowsDrag()
{
}

void QWindowsDrag::startDrag()
{
    // TODO: Accessibility handling?
    QDragManager *dragManager = QDragManager::self();
    QMimeData *dropData = dragManager->dropData();
    m_dragBeingCancelled = false;

    DWORD resultEffect;
    QWindowsOleDropSource *windowDropSource = new QWindowsOleDropSource();
    windowDropSource->createCursors();
    QWindowsOleDataObject *dropDataObject = new QWindowsOleDataObject(dropData);
    const  Qt::DropActions possibleActions = dragManager->possible_actions;
    const DWORD allowedEffects = translateToWinDragEffects(possibleActions);
    if (QWindowsContext::verboseOLE)
          qDebug(">%s possible Actions=%x, effects=0x%lx", __FUNCTION__,
                 int(possibleActions), allowedEffects);
    const HRESULT r = DoDragDrop(dropDataObject, windowDropSource, allowedEffects, &resultEffect);
    const DWORD  reportedPerformedEffect = dropDataObject->reportedPerformedEffect();
    Qt::DropAction ret = Qt::IgnoreAction;
    if (r == DRAGDROP_S_DROP) {
        if (reportedPerformedEffect == DROPEFFECT_MOVE && resultEffect != DROPEFFECT_MOVE) {
            ret = Qt::TargetMoveAction;
            resultEffect = DROPEFFECT_MOVE;
        } else {
            ret = translateToQDragDropAction(resultEffect);
        }
        // Force it to be a copy if an unsupported operation occurred.
        // This indicates a bug in the drop target.
        if (resultEffect != DROPEFFECT_NONE && !(resultEffect & allowedEffects))
            ret = Qt::CopyAction;
    } else {
        dragManager->setCurrentTarget(0);
    }

    // clean up
    dropDataObject->releaseQt();
    dropDataObject->Release();        // Will delete obj if refcount becomes 0
    windowDropSource->Release();        // Will delete src if refcount becomes 0
    if (QWindowsContext::verboseOLE)
        qDebug("<%s allowedEffects=0x%lx, reportedPerformedEffect=0x%lx, resultEffect=0x%lx, hr=0x%x, dropAction=%d",
               __FUNCTION__, allowedEffects, reportedPerformedEffect, resultEffect, int(r), ret);
}

void QWindowsDrag::move(const QMouseEvent *me)
{
    const QPoint pos = me->pos();
    if (QWindowsContext::verboseOLE)
        qDebug("%s %d %d", __FUNCTION__, pos.x(), pos.y());
}

void QWindowsDrag::drop(const QMouseEvent *me)
{
    const QPoint pos = me->pos();
    if (QWindowsContext::verboseOLE)
        qDebug("%s %d %d", __FUNCTION__, pos.x(), pos.y());
}

void QWindowsDrag::cancel()
{
    // TODO: Accessibility handling?
    if (QWindowsContext::verboseOLE)
        qDebug("%s", __FUNCTION__);
    m_dragBeingCancelled = true;
}

QWindowsDrag *QWindowsDrag::instance()
{
    return static_cast<QWindowsDrag *>(QWindowsIntegration::instance()->drag());
}

void QWindowsDrag::releaseDropDataObject()
{
    if (QWindowsContext::verboseOLE)
        qDebug("%s %p", __FUNCTION__, m_dropDataObject);
    if (m_dropDataObject) {
        m_dropDataObject->Release();
        m_dropDataObject = 0;
    }
}

QT_END_NAMESPACE
