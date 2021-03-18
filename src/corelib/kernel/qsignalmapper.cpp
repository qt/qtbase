/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qglobal.h"

#include "qsignalmapper.h"
#include "qhash.h"
#include "qobject_p.h"

QT_BEGIN_NAMESPACE

class QSignalMapperPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QSignalMapper)
public:
    void _q_senderDestroyed() {
        Q_Q(QSignalMapper);
        q->removeMappings(q->sender());
    }

    template <class Signal, class Container>
    void emitMappedValue(QObject *sender, Signal signal, const Container &mappedValues)
    {
        Q_Q(QSignalMapper);

        auto it = mappedValues.find(sender);
        if (it != mappedValues.end()) {
#if QT_DEPRECATED_SINCE(5, 15)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
            Q_EMIT q->mapped(*it);
QT_WARNING_POP
#endif
            Q_EMIT (q->*signal)(*it);
        }
    }

    void emitMappedValues(QObject *sender)
    {
        emitMappedValue(sender, &QSignalMapper::mappedInt, intHash);
        emitMappedValue(sender, &QSignalMapper::mappedString, stringHash);
        emitMappedValue(sender, &QSignalMapper::mappedWidget, widgetHash);
        emitMappedValue(sender, &QSignalMapper::mappedObject, objectHash);
    }

    QHash<QObject *, int> intHash;
    QHash<QObject *, QString> stringHash;
    QHash<QObject *, QWidget*> widgetHash;
    QHash<QObject *, QObject*> objectHash;
};

/*!
    \class QSignalMapper
    \inmodule QtCore
    \brief The QSignalMapper class bundles signals from identifiable senders.

    \ingroup objectmodel


    This class collects a set of parameterless signals, and re-emits
    them with integer, string or widget parameters corresponding to
    the object that sent the signal. Note that in most cases you can
    use lambdas for passing custom parameters to slots. This is less
    costly and will simplify the code.

    The class supports the mapping of particular strings, integers,
    objects and widgets with particular objects using setMapping().
    The objects' signals can then be connected to the map() slot which
    will emit a signal (it could be mappedInt(), mappedString(),
    mappedWidget() and mappedObject()) with a value associated with
    the original signalling object. Mappings can be removed later using
    removeMappings().

    Example: Suppose we want to create a custom widget that contains
    a group of buttons (like a tool palette). One approach is to
    connect each button's \c clicked() signal to its own custom slot;
    but in this example we want to connect all the buttons to a
    single slot and parameterize the slot by the button that was
    clicked.

    Here's the definition of a simple custom widget that has a single
    signal, \c clicked(), which is emitted with the text of the button
    that was clicked:

    \snippet qsignalmapper/buttonwidget.h 0
    \snippet qsignalmapper/buttonwidget.h 1

    The only function that we need to implement is the constructor:

    \snippet qsignalmapper/buttonwidget.cpp 0
    \snippet qsignalmapper/buttonwidget.cpp 1
    \snippet qsignalmapper/buttonwidget.cpp 2

    A list of texts is passed to the constructor. A signal mapper is
    constructed and for each text in the list a QPushButton is
    created. We connect each button's \c clicked() signal to the
    signal mapper's map() slot, and create a mapping in the signal
    mapper from each button to the button's text. Finally we connect
    the signal mapper's mappedString() signal to the custom widget's
    \c clicked() signal. When the user clicks a button, the custom
    widget will emit a single \c clicked() signal whose argument is
    the text of the button the user clicked.

    This class was mostly useful before lambda functions could be used as
    slots. The example above can be rewritten simpler without QSignalMapper
    by connecting to a lambda function.

    \snippet qsignalmapper/buttonwidget.cpp 3

    \sa QObject, QButtonGroup, QActionGroup
*/

/*!
    Constructs a QSignalMapper with parent \a parent.
*/
QSignalMapper::QSignalMapper(QObject* parent)
    : QObject(*new QSignalMapperPrivate, parent)
{
}

/*!
    Destroys the QSignalMapper.
*/
QSignalMapper::~QSignalMapper()
{
}

/*!
    Adds a mapping so that when map() is signalled from the given \a
    sender, the signal mappedInt(\a id) is emitted.

    There may be at most one integer ID for each sender.

    \sa mapping()
*/
void QSignalMapper::setMapping(QObject *sender, int id)
{
    Q_D(QSignalMapper);
    d->intHash.insert(sender, id);
    connect(sender, SIGNAL(destroyed()), this, SLOT(_q_senderDestroyed()));
}

/*!
    Adds a mapping so that when map() is signalled from the \a sender,
    the signal mappedString(\a text ) is emitted.

    There may be at most one text for each sender.
*/
void QSignalMapper::setMapping(QObject *sender, const QString &text)
{
    Q_D(QSignalMapper);
    d->stringHash.insert(sender, text);
    connect(sender, SIGNAL(destroyed()), this, SLOT(_q_senderDestroyed()));
}

/*!
    Adds a mapping so that when map() is signalled from the \a sender,
    the signal mappedWidget(\a widget ) is emitted.

    There may be at most one widget for each sender.
*/
void QSignalMapper::setMapping(QObject *sender, QWidget *widget)
{
    Q_D(QSignalMapper);
    d->widgetHash.insert(sender, widget);
    connect(sender, SIGNAL(destroyed()), this, SLOT(_q_senderDestroyed()));
}

/*!
    Adds a mapping so that when map() is signalled from the \a sender,
    the signal mappedObject(\a object ) is emitted.

    There may be at most one object for each sender.
*/
void QSignalMapper::setMapping(QObject *sender, QObject *object)
{
    Q_D(QSignalMapper);
    d->objectHash.insert(sender, object);
    connect(sender, SIGNAL(destroyed()), this, SLOT(_q_senderDestroyed()));
}

/*!
    Returns the sender QObject that is associated with the \a id.

    \sa setMapping()
*/
QObject *QSignalMapper::mapping(int id) const
{
    Q_D(const QSignalMapper);
    return d->intHash.key(id);
}

/*!
    \overload mapping()
*/
QObject *QSignalMapper::mapping(const QString &id) const
{
    Q_D(const QSignalMapper);
    return d->stringHash.key(id);
}

/*!
    \overload mapping()

    Returns the sender QObject that is associated with the \a widget.
*/
QObject *QSignalMapper::mapping(QWidget *widget) const
{
    Q_D(const QSignalMapper);
    return d->widgetHash.key(widget);
}

/*!
    \overload mapping()

    Returns the sender QObject that is associated with the \a object.
*/
QObject *QSignalMapper::mapping(QObject *object) const
{
    Q_D(const QSignalMapper);
    return d->objectHash.key(object);
}

/*!
    Removes all mappings for \a sender.

    This is done automatically when mapped objects are destroyed.

    \note This does not disconnect any signals. If \a sender is not destroyed
    then this will need to be done explicitly if required.
*/
void QSignalMapper::removeMappings(QObject *sender)
{
    Q_D(QSignalMapper);

    d->intHash.remove(sender);
    d->stringHash.remove(sender);
    d->widgetHash.remove(sender);
    d->objectHash.remove(sender);
}

/*!
    This slot emits signals based on which object sends signals to it.
*/
void QSignalMapper::map() { map(sender()); }

/*!
    This slot emits signals based on the \a sender object.
*/
void QSignalMapper::map(QObject *sender)
{
    d_func()->emitMappedValues(sender);
}

#if QT_DEPRECATED_SINCE(5, 15)
/*!
    \fn void QSignalMapper::mapped(int i)
    \obsolete
    \overload

    This signal is emitted when map() is signalled from an object that
    has an integer mapping set. The object's mapped integer is passed
    in \a i.

    \sa setMapping()
*/

/*!
    \fn void QSignalMapper::mapped(const QString &text)
    \obsolete
    \overload

    This signal is emitted when map() is signalled from an object that
    has a string mapping set. The object's mapped string is passed in
    \a text.

    \sa setMapping()
*/

/*!
    \fn void QSignalMapper::mapped(QWidget *widget)
    \obsolete
    \overload

    This signal is emitted when map() is signalled from an object that
    has a widget mapping set. The object's mapped widget is passed in
    \a widget.

    \sa setMapping()
*/

/*!
    \fn void QSignalMapper::mapped(QObject *object)
    \obsolete
    \overload

    This signal is emitted when map() is signalled from an object that
    has an object mapping set. The object provided by the map is passed in
    \a object.

    \sa setMapping()
*/
#endif

/*!
    \fn void QSignalMapper::mappedInt(int i)
    \since 5.15

    This signal is emitted when map() is signalled from an object that
    has an integer mapping set. The object's mapped integer is passed
    in \a i.

    \sa setMapping()
*/

/*!
    \fn void QSignalMapper::mappedString(const QString &text)
    \since 5.15

    This signal is emitted when map() is signalled from an object that
    has a string mapping set. The object's mapped string is passed in
    \a text.

    \sa setMapping()
*/

/*!
    \fn void QSignalMapper::mappedWidget(QWidget *widget)
    \since 5.15

    This signal is emitted when map() is signalled from an object that
    has a widget mapping set. The object's mapped widget is passed in
    \a widget.

    \sa setMapping()
*/

/*!
    \fn void QSignalMapper::mappedObject(QObject *object)
    \since 5.15

    This signal is emitted when map() is signalled from an object that
    has an object mapping set. The object provided by the map is passed in
    \a object.

    \sa setMapping()
*/

QT_END_NAMESPACE

#include "moc_qsignalmapper.cpp"
