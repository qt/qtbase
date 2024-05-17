// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef _EVENTFILTER_
#define _EVENTFILTER_

#include <QtCore/QObject>
#include <QtCore/QEvent>
#include <QtCore/QList>

QT_FORWARD_DECLARE_CLASS(QDebug)

namespace QtDiag {

// Event filter that can for example be installed on QApplication
// to log relevant events.

class EventFilter : public QObject {
public:
    enum EventCategory {
        MouseEvents        = 0x00001,
        MouseMoveEvents    = 0x00002,
        TouchEvents        = 0x00004,
        TabletEvents       = 0x00008,
        DragAndDropEvents  = 0x00010,
        KeyEvents          = 0x00020,
        FocusEvents        = 0x00040,
        GeometryEvents     = 0x00080,
        PaintEvents        = 0x00100,
        StateChangeEvents  = 0x00200,
        InputMethodEvents  = 0x00400,
        TimerEvents        = 0x00800,
        ObjectEvents       = 0x01000,
        GestureEvents      = 0x02000,
        AllEvents          = 0xFFFFF
    };
    Q_DECLARE_FLAGS(EventCategories, EventCategory)

    enum ObjectType {
        QWindowType = 0x1,
        QWidgetType = 0x2,
        OtherType   = 0x4
    };
    Q_DECLARE_FLAGS(ObjectTypes, ObjectType)

    explicit EventFilter(EventCategories eventCategories, QObject *p = nullptr);
    explicit EventFilter(QObject *p = nullptr);

    bool eventFilter(QObject *, QEvent *) override;

    ObjectTypes objectTypes() const { return m_objectTypes; }
    void setObjectTypes(ObjectTypes objectTypes) { m_objectTypes = objectTypes; }

    static void formatObject(const QObject *o, QDebug debug);

private:
    void init(EventCategories eventCategories);

    QList<QEvent::Type> m_eventTypes;
    ObjectTypes m_objectTypes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EventFilter::EventCategories)
Q_DECLARE_OPERATORS_FOR_FLAGS(EventFilter::ObjectTypes)

struct formatQObject
{
    explicit formatQObject(const QObject *o) : m_object(o) {}

    const QObject *m_object;
};

QDebug operator<<(QDebug d, const formatQObject &fo);

} // namespace QtDiag

#endif
