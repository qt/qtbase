/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qiconengine.h"
#include "qpainter.h"

QT_BEGIN_NAMESPACE

/*!
  \class QIconEngine

  \brief The QIconEngine class provides an abstract base class for QIcon renderers.

  \ingroup painting
  \inmodule QtGui

  An icon engine provides the rendering functions for a QIcon. Each icon has a
  corresponding icon engine that is responsible for drawing the icon with a
  requested size, mode and state.

  The icon is rendered by the paint() function, and the icon can additionally be
  obtained as a pixmap with the pixmap() function (the default implementation
  simply uses paint() to achieve this). The addPixmap() function can be used to
  add new pixmaps to the icon engine, and is used by QIcon to add specialized
  custom pixmaps.

  The paint(), pixmap(), and addPixmap() functions are all virtual, and can
  therefore be reimplemented in subclasses of QIconEngine.

  \sa QIconEnginePlugin

*/

/*!
  \fn virtual void QIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) = 0;

  Uses the given \a painter to paint the icon with the required \a mode and
  \a state into the rectangle \a rect.
*/

/*!  Returns the actual size of the icon the engine provides for the
  requested \a size, \a mode and \a state. The default implementation
  returns the given \a size.
 */
QSize QIconEngine::actualSize(const QSize &size, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
    return size;
}

/*!
    \since 5.6
    Constructs the icon engine.
 */
QIconEngine::QIconEngine()
{
}

/*!
    \since 5.8
    \internal
 */
QIconEngine::QIconEngine(const QIconEngine &)
{
}

/*!
  Destroys the icon engine.
 */
QIconEngine::~QIconEngine()
{
}


/*!
  Returns the icon as a pixmap with the required \a size, \a mode,
  and \a state. The default implementation creates a new pixmap and
  calls paint() to fill it.
*/
QPixmap QIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm(size);
    {
        QPainter p(&pm);
        paint(&p, QRect(QPoint(0,0),size), mode, state);
    }
    return pm;
}

/*!
  Called by QIcon::addPixmap(). Adds a specialized \a pixmap for the given
  \a mode and \a state. The default pixmap-based engine stores any supplied
  pixmaps, and it uses them instead of scaled pixmaps if the size of a pixmap
  matches the size of icon requested. Custom icon engines that implement
  scalable vector formats are free to ignores any extra pixmaps.
 */
void QIconEngine::addPixmap(const QPixmap &/*pixmap*/, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
}


/*!  Called by QIcon::addFile(). Adds a specialized pixmap from the
  file with the given \a fileName, \a size, \a mode and \a state. The
  default pixmap-based engine stores any supplied file names, and it
  loads the pixmaps on demand instead of using scaled pixmaps if the
  size of a pixmap matches the size of icon requested. Custom icon
  engines that implement scalable vector formats are free to ignores
  any extra files.
 */
void QIconEngine::addFile(const QString &/*fileName*/, const QSize &/*size*/, QIcon::Mode /*mode*/, QIcon::State /*state*/)
{
}


/*!
    \enum QIconEngine::IconEngineHook
    \since 4.5

    These enum values are used for virtual_hook() to allow additional
    queries to icon engine without breaking binary compatibility.

    \value AvailableSizesHook Allows to query the sizes of the
    contained pixmaps for pixmap-based engines. The \a data argument
    of the virtual_hook() function is a AvailableSizesArgument pointer
    that should be filled with icon sizes. Engines that work in terms
    of a scalable, vectorial format normally return an empty list.

    \value IconNameHook Allows to query the name used to create the
    icon, for example when instantiating an icon using
    QIcon::fromTheme().

    \value IsNullHook Allow to query if this engine represents a null
    icon. The \a data argument of the virtual_hook() is a pointer to a
    bool that can be set to true if the icon is null. This enum value
    was added in Qt 5.7.

    \value ScaledPixmapHook Provides a way to get a pixmap that is scaled
    according to the given scale (typically equal to the \l {Glossary Of High
    DPI Terms}{device pixel ratio}). The \a data argument of the virtual_hook()
    function is a \l ScaledPixmapArgument pointer that contains both the input and
    output arguments. This enum value was added in Qt 5.9.

    \sa virtual_hook()
 */

/*!
    \class QIconEngine::AvailableSizesArgument
    \since 4.5

    \inmodule QtGui

    This struct represents arguments to virtual_hook() function when
    \a id parameter is QIconEngine::AvailableSizesHook.

    \sa virtual_hook(), QIconEngine::IconEngineHook
 */

/*!
    \variable QIconEngine::AvailableSizesArgument::mode
    \brief the requested mode of an image.

    \sa QIcon::Mode
*/

/*!
    \variable QIconEngine::AvailableSizesArgument::state
    \brief the requested state of an image.

    \sa QIcon::State
*/

/*!
    \variable QIconEngine::AvailableSizesArgument::sizes

    \brief image sizes that are available with specified \a mode and
    \a state. This is an output parameter and is filled after call to
    virtual_hook(). Engines that work in terms of a scalable,
    vectorial format normally return an empty list.
*/

/*!
    \class QIconEngine::ScaledPixmapArgument
    \since 5.9

    \inmodule QtGui

    This struct represents arguments to the virtual_hook() function when
    the \a id parameter is QIconEngine::ScaledPixmapHook.

    The struct provides a way for icons created via \l QIcon::fromTheme()
    to return pixmaps that are designed for the current \l {Glossary Of High
    DPI Terms}{device pixel ratio}. The scale for such an icon is specified
    using the \l {Icon Theme Specification - Directory Layout}{Scale directory key}
    in the appropriate \c index.theme file.

    Icons created via other approaches will return the same result as a call to
    \l pixmap() would, and continue to benefit from Qt's \l {High Resolution
    Versions of Images}{"@nx" high DPI syntax}.

    \sa virtual_hook(), QIconEngine::IconEngineHook, {High DPI Icons}
 */

/*!
    \variable QIconEngine::ScaledPixmapArgument::size
    \brief The requested size of the pixmap.
*/

/*!
    \variable QIconEngine::ScaledPixmapArgument::mode
    \brief The requested mode of the pixmap.

    \sa QIcon::Mode
*/

/*!
    \variable QIconEngine::ScaledPixmapArgument::state
    \brief The requested state of the pixmap.

    \sa QIcon::State
*/

/*!
    \variable QIconEngine::ScaledPixmapArgument::scale
    \brief The requested scale of the pixmap.
*/

/*!
    \variable QIconEngine::ScaledPixmapArgument::pixmap

    \brief The pixmap that is the best match for the given \l size, \l mode, \l
    state, and \l scale. This is an output parameter that is set after calling
    \l virtual_hook().
*/


/*!
    Returns a key that identifies this icon engine.
 */
QString QIconEngine::key() const
{
    return QString();
}

/*! \fn QIconEngine *QIconEngine::clone() const

    Reimplement this method to return a clone of this icon engine.
 */

/*!
    Reads icon engine contents from the QDataStream \a in. Returns
    true if the contents were read; otherwise returns \c false.

    QIconEngine's default implementation always return false.
 */
bool QIconEngine::read(QDataStream &)
{
    return false;
}

/*!
    Writes the contents of this engine to the QDataStream \a out.
    Returns \c true if the contents were written; otherwise returns \c false.

    QIconEngine's default implementation always return false.
 */
bool QIconEngine::write(QDataStream &) const
{
    return false;
}

/*!
    \since 4.5

    Additional method to allow extending QIconEngine without
    adding new virtual methods (and without breaking binary compatibility).
    The actual action and format of \a data depends on \a id argument
    which is in fact a constant from IconEngineHook enum.

    \sa IconEngineHook
*/
void QIconEngine::virtual_hook(int id, void *data)
{
    switch (id) {
    case QIconEngine::AvailableSizesHook: {
        QIconEngine::AvailableSizesArgument &arg =
            *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
        arg.sizes.clear();
        break;
    }
    case QIconEngine::ScaledPixmapHook: {
        // We don't have any notion of scale besides "@nx", so just call pixmap() here.
        QIconEngine::ScaledPixmapArgument &arg =
            *reinterpret_cast<QIconEngine::ScaledPixmapArgument*>(data);
        arg.pixmap = pixmap(arg.size, arg.mode, arg.state);
        break;
    }
    default:
        break;
    }
}

/*!
    \since 4.5

    Returns sizes of all images that are contained in the engine for the
    specific \a mode and \a state.

    \include qiconengine-virtualhookhelper.qdocinc
 */
QList<QSize> QIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state) const
{
    AvailableSizesArgument arg;
    arg.mode = mode;
    arg.state = state;
    const_cast<QIconEngine *>(this)->virtual_hook(QIconEngine::AvailableSizesHook, reinterpret_cast<void*>(&arg));
    return arg.sizes;
}

/*!
    \since 4.7

    Returns the name used to create the engine, if available.

    \include qiconengine-virtualhookhelper.qdocinc
 */
QString QIconEngine::iconName() const
{
    QString name;
    const_cast<QIconEngine *>(this)->virtual_hook(QIconEngine::IconNameHook, reinterpret_cast<void*>(&name));
    return name;
}

/*!
    \since 5.7

    Returns true if this icon engine represent a null QIcon.

    \include qiconengine-virtualhookhelper.qdocinc
 */
bool QIconEngine::isNull() const
{
    bool isNull = false;
    const_cast<QIconEngine *>(this)->virtual_hook(QIconEngine::IsNullHook, &isNull);
    return isNull;
}

/*!
    \since 5.9

    Returns a pixmap for the given \a size, \a mode, \a state and \a scale.

    The \a scale argument is typically equal to the \l {Glossary Of High DPI
    Terms}{device pixel ratio} of the display.

    \include qiconengine-virtualhookhelper.qdocinc

    \note Some engines may cast \a scale to an integer.

    \sa ScaledPixmapArgument
*/
QPixmap QIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{
    ScaledPixmapArgument arg;
    arg.size = size;
    arg.mode = mode;
    arg.state = state;
    arg.scale = scale;
    const_cast<QIconEngine *>(this)->virtual_hook(QIconEngine::ScaledPixmapHook, reinterpret_cast<void*>(&arg));
    return arg.pixmap;
}

QT_END_NAMESPACE
