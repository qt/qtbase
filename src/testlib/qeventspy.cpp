// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qeventspy.h"

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>

#include <algorithm>
#include <vector>

QT_BEGIN_NAMESPACE

class QEventSpyPrivate : public QObject
{
public:
    QEventSpyPrivate(QObject *object, QEventSpy::Filter filter)
        : object(object), filter(std::move(filter)), valid(object != nullptr)
    {
        if (object)
            object->installEventFilter(this);
        else
            qWarning("QEventSpy: Cannot spy on a null object");
    }

    ~QEventSpyPrivate() override
    {
        if (object)
            object->removeEventFilter(this);
    }

    bool eventFilter(QObject *receiver, QEvent *event) override
    {
        if (filter && !filter(receiver, event))
            return false;

        records.push_back({ receiver, std::unique_ptr<QEvent>(event->clone()) });
        return false;
    }

    struct Record
    {
        QObject *receiver;
        std::unique_ptr<QEvent> event;
    };

    QPointer<QObject> object;
    const QEventSpy::Filter filter;
    std::vector<Record> records;
    const bool valid;
};

/*!
    \class QEventSpy
    \inmodule QtTest
    \since 6.13

    \brief The QEventSpy class enables introspection of event delivery.

    QEventSpy installs itself as an event filter on a QObject and records
    the events delivered to it, in the order they arrive. QEventSpy never
    filters out or modifies the events it observes.

    The following example verifies that showing a widget delivers a single
    \l{QEvent::}{Show} event to it:

    \code
    QWidget widget;
    QEventSpy spy(&widget, QEvent::Show);
    widget.show();
    QCOMPARE(spy.count(), 1);
    \endcode

    To record several event types, or all events, pass a list of types,
    or an empty list:

    \code
    QEventSpy spy(&widget, { QEvent::FocusIn, QEvent::FocusOut });
    ...
    QCOMPARE(spy.eventList(), QList({ QEvent::FocusIn, QEvent::FocusOut }));
    \endcode

    For more fine-grained control, pass a QEventSpy::Filter. This is
    useful when spying on the application object, which sees the events
    of all objects:

    \code
    QEventSpy spy(qApp, [&](QObject *receiver, const QEvent *event) {
        return receiver == &widget && !event->spontaneous();
    });
    \endcode

    QEventSpy stores a copy of each recorded event, made with
    QEvent::clone() at the time the event filter sees it. The copy can be
    inspected using at():

    \code
    QEventSpy spy(&widget, QEvent::MouseButtonPress);
    QTest::mouseClick(&widget, Qt::LeftButton, {}, QPoint(10, 10));
    QCOMPARE(spy.at<QMouseEvent>(0)->position(), QPointF(10, 10));
    \endcode

    Event types that do not reimplement QEvent::clone() are recorded as a
    plain QEvent of the same type.

    To wait for events that are delivered asynchronously, use one of the
    QTRY_ macros, or QTest::qWaitFor(), with a condition that states what
    the test is waiting for:

    \code
    QEventSpy spy(&window, QEvent::Expose);
    window.show();
    QTRY_VERIFY(spy.contains(QEvent::Expose));
    \endcode

    QEventSpy must be created in the thread of the object it spies on.

    \sa QSignalSpy, QObject::installEventFilter()
*/

/*!
    \typedef QEventSpy::Filter

    A synonym for \c{std::function<bool(QObject *receiver, const QEvent *event)>}.

    A filter returns \c true for each event that the spy should record.
*/

/*!
    Constructs a QEventSpy that records all events delivered to \a object.

    If \a object is \nullptr, a warning is output using qWarning()
    and subsequent calls to isValid() return \c false.
*/
QEventSpy::QEventSpy(QObject *object)
    : QEventSpy(object, Filter{})
{
}

/*!
    Constructs a QEventSpy that records events of the given \a type
    delivered to \a object.

    If \a object is \nullptr, a warning is output using qWarning()
    and subsequent calls to isValid() return \c false.
*/
QEventSpy::QEventSpy(QObject *object, QEvent::Type type)
    : QEventSpy(object, [type](QObject *, const QEvent *event) {
        return event->type() == type;
    })
{
}

/*!
    Constructs a QEventSpy that records events of the given \a types
    delivered to \a object. If \a types is empty, all events are recorded.

    If \a object is \nullptr, a warning is output using qWarning()
    and subsequent calls to isValid() return \c false.
*/
QEventSpy::QEventSpy(QObject *object, const QList<QEvent::Type> &types)
    : QEventSpy(object, types.isEmpty() ? Filter{} : Filter([types](QObject *, const QEvent *event) {
        return types.contains(event->type());
    }))
{
}

/*!
    Constructs a QEventSpy that records the events delivered to \a object
    for which \a filter returns \c true. If \a filter is empty, all events
    are recorded.

    If \a object is \nullptr, a warning is output using qWarning()
    and subsequent calls to isValid() return \c false.
*/
QEventSpy::QEventSpy(QObject *object, Filter filter)
    : d_ptr(std::make_unique<QEventSpyPrivate>(object, std::move(filter)))
{
}

/*!
    Destroys the QEventSpy, and removes it as an event filter
    from the spied-on object.
*/
QEventSpy::~QEventSpy() = default;

/*!
    Returns \c true if the spy was constructed with a valid object,
    otherwise \c false.

    The spy remains valid if the object is later destroyed.
*/
bool QEventSpy::isValid() const noexcept
{
    return d_ptr->valid;
}

/*!
    Returns the object the spy was installed on, or \nullptr
    if the object has been destroyed.
*/
QObject *QEventSpy::object() const
{
    return d_ptr->object;
}

/*!
    \fn qsizetype QEventSpy::size() const

    Same as count().
*/

/*!
    \fn bool QEventSpy::isEmpty() const

    Returns \c true if the spy has not recorded any events,
    otherwise \c false.
*/

/*!
    Returns the number of recorded events.
*/
qsizetype QEventSpy::count() const noexcept
{
    return qsizetype(d_ptr->records.size());
}

/*!
    \overload

    Returns the number of recorded events of the given \a type.
*/
qsizetype QEventSpy::count(QEvent::Type type) const noexcept
{
    return std::count_if(d_ptr->records.cbegin(), d_ptr->records.cend(), [type](const auto &record) {
        return record.event->type() == type;
    });
}

/*!
    \fn bool QEventSpy::contains(QEvent::Type type) const

    Returns \c true if the spy has recorded at least one event
    of the given \a type, otherwise \c false.
*/

/*!
    Returns the types of the recorded events, in the order
    the spy saw them, including repeated types.

    \code
    QCOMPARE(spy.eventList(), QList({ QEvent::ChildAdded, QEvent::ChildAdded }));
    \endcode
*/
QList<QEvent::Type> QEventSpy::eventList() const
{
    QList<QEvent::Type> eventList;
    eventList.reserve(count());
    for (const auto &record : d_ptr->records)
        eventList.append(record.event->type());
    return eventList;
}

/*!
    Returns a copy of the recorded event at index position \a i.

    \a i must be a valid index position (0 <= \a i < count()).
    The returned pointer is valid until the event is removed from
    the spy, or the spy is destroyed.
*/
const QEvent *QEventSpy::at(qsizetype i) const
{
    Q_ASSERT_X(i >= 0 && i < count(), "QEventSpy::at", "index out of range");
    return d_ptr->records[size_t(i)].event.get();
}

/*!
    \fn template <typename EventType> const EventType *QEventSpy::at(qsizetype i) const
    \overload

    Returns a copy of the recorded event at index position \a i,
    cast to \c EventType.

    The caller must ensure that the event is of type \c EventType,
    for example by checking QEvent::type().
*/

/*!
    Returns the object that the event at index position \a i
    was delivered to.

    \a i must be a valid index position (0 <= \a i < count()).
    The object may have been destroyed since the event was recorded.
*/
QObject *QEventSpy::receiver(qsizetype i) const
{
    Q_ASSERT_X(i >= 0 && i < count(), "QEventSpy::receiver", "index out of range");
    return d_ptr->records[size_t(i)].receiver;
}

/*!
    Removes all recorded events from the spy.
*/
void QEventSpy::clear()
{
    d_ptr->records.clear();
}

QT_END_NAMESPACE
