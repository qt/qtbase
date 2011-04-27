/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qmouse_qws.h"
#include "qwindowsystem_qws.h"
#include "qscreen_qws.h"
#include "qapplication.h"
#include "qtextstream.h"
#include "qfile.h"
#include "qdebug.h"
#include "qscreen_qws.h"

QT_BEGIN_NAMESPACE

/*!
    \class QWSPointerCalibrationData
    \ingroup qws

    \brief The QWSPointerCalibrationData class is a container for
    mouse calibration data in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    QWSPointerCalibrationData stores device and screen coordinates in
    the devPoints and screenPoints variables, respectively.

    A calibration program should create a QWSPointerCalibrationData
    object, fill the devPoints and screenPoints variables with its
    device and screen coordinates, and pass the object to the mouse
    driver using the QWSMouseHandler::calibrate() function.

    \sa QWSCalibratedMouseHandler, {Mouse Calibration Example}
*/

/*!
    \variable QWSPointerCalibrationData::devPoints
    \brief the raw device coordinates for each value of the Location enum.
*/

/*!
    \variable QWSPointerCalibrationData::screenPoints
    \brief the logical screen coordinates for each value of the Location enum.
*/

/*!
    \enum QWSPointerCalibrationData::Location

    This enum describes the various logical positions that can be
    specified by the devPoints and screenPoints variables.

    \value TopLeft           Index of the top left corner of the screen.
    \value BottomLeft     Index of the bottom left corner of the screen.
    \value BottomRight   Index of the bottom right corner of the screen.
    \value TopRight         Index of the top right corner of the screen.
    \value Center            Index of the center of the screen.
    \value LastLocation   Last index in the pointer arrays.
*/

class QWSMouseHandlerPrivate
{
public:
    QWSMouseHandlerPrivate() : screen(qt_screen) {}

    const QScreen *screen;
};

/*!
    \class QWSMouseHandler
    \ingroup qws

    \brief The QWSMouseHandler class is a base class for mouse drivers in
    Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    \l{Qt for Embedded Linux} provides ready-made drivers for several mouse
    protocols, see the \l{Qt for Embedded Linux Pointer Handling}{pointer
    handling} documentation for details. Custom mouse drivers can be
    implemented by subclassing the QWSMouseHandler class and creating
    a mouse driver plugin (derived from QMouseDriverPlugin).
    The default implementation of the QMouseDriverFactory class
    will automatically detect the plugin, and load the driver into the
    server application at run-time using Qt's \l {How to Create Qt
    Plugins}{plugin system}.

    The mouse driver receives mouse events from the system device and
    encapsulates each event with an instance of the QWSEvent class
    which it then passes to the server application (the server is
    responsible for propagating the event to the appropriate
    client). To receive mouse events, a QWSMouseHandler object will
    usually create a QSocketNotifier object for the given device. The
    QSocketNotifier class provides support for monitoring activity on
    a file descriptor. When the socket notifier receives data, it will
    call the mouse driver's mouseChanged() function to send the event
    to the \l{Qt for Embedded Linux} server application for relaying to
    clients.

    If you are creating a driver for a device that needs calibration
    or noise reduction, such as a touchscreen, use the
    QWSCalibratedMouseHandler subclass instead to take advantage of
    the calibrate() and clearCalibration() functions. The \l
    {qws/mousecalibration}{Mouse Calibration}
    demonstrates how to write a simple program using the mechanisms
    provided by the QWSMouseHandler class to calibrate a mouse driver.

    Note that when deriving from the QWSMouseHandler class, the
    resume() and suspend() functions must be reimplemented to control
    the flow of mouse input, i.e., the default implementation does
    nothing. Reimplementations of these functions typically call the
    QSocketNotifier::setEnabled() function to enable or disable the
    socket notifier, respectively.

    In addition, QWSMouseHandler provides the setScreen() function
    that allows you to specify a screen for your mouse driver and the
    limitToScreen() function that ensures that a given position is
    within this screen's boundaries (changing the position if
    necessary). Finally, QWSMouseHandler provides the pos() function
    returning the current mouse position.

    \sa QMouseDriverPlugin, QMouseDriverFactory, {Qt for Embedded Linux Pointer
    Handling}
*/


/*!
    \fn void QWSMouseHandler::suspend()

    Implement this function to suspend reading and handling of mouse
    events, e.g., call the QSocketNotifier::setEnabled() function to
    disable the socket notifier.

    \sa resume()
*/

/*!
    \fn void QWSMouseHandler::resume()

    Implement this function to resume reading and handling mouse
    events, e.g., call the QSocketNotifier::setEnabled() function to
    enable the socket notifier.

    \sa suspend()
*/

/*!
    \fn const QPoint &QWSMouseHandler::pos() const

    Returns the current mouse position.

    \sa mouseChanged(), limitToScreen()
*/

/*!
    Constructs a mouse driver. The \a driver and \a device arguments
    are passed by the QWS_MOUSE_PROTO environment variable.

    Call the QWSServer::setMouseHandler() function to make the newly
    created mouse driver, the primary driver. Note that the primary
    driver is controlled by the system, i.e., the system will delete
    it upon exit.
*/
QWSMouseHandler::QWSMouseHandler(const QString &, const QString &)
    : mousePos(QWSServer::mousePosition), d_ptr(new QWSMouseHandlerPrivate)
{
}

/*!
    Destroys this mouse driver.

    Do not call this function if this driver is the primary mouse
    driver, i.e., if QWSServer::setMouseHandler() function has been
    called passing this driver as argument. The primary mouse
    driver is deleted by the system.
*/
QWSMouseHandler::~QWSMouseHandler()
{
    delete d_ptr;
}

/*!
    Ensures that the given \a position is within the screen's
    boundaries, changing the \a position if necessary.

    \sa pos(), setScreen()
*/

void QWSMouseHandler::limitToScreen(QPoint &position)
{
    position.setX(qMin(d_ptr->screen->deviceWidth() - 1, qMax(0, position.x())));
    position.setY(qMin(d_ptr->screen->deviceHeight() - 1, qMax(0, position.y())));
}

/*!
    \since 4.2

    Sets the screen for this mouse driver to be the given \a screen.

    \sa limitToScreen()
*/
void QWSMouseHandler::setScreen(const QScreen *screen)
{
    d_ptr->screen = (screen ? screen : qt_screen);
}

/*!
    Notifies the system of a new mouse event.

    This function updates the current mouse position and sends the
    event to the \l{Qt for Embedded Linux} server application for
    delivery to the correct widget. Note that a custom mouse driver must call
    this function whenever it wants to deliver a new mouse event.

    The given \a position is the global position of the mouse cursor.
    The \a state parameter is a bitmask of the Qt::MouseButton enum's
    values, indicating which mouse buttons are pressed. The \a wheel
    parameter is the delta value of the mouse wheel as returned by
    QWheelEvent::delta().

    \sa pos()
*/
void QWSMouseHandler::mouseChanged(const QPoint &position, int state, int wheel)
{
    mousePos = position + d_ptr->screen->offset();
    QWSServer::sendMouseEvent(mousePos, state, wheel);
}

/*!
    \fn QWSMouseHandler::clearCalibration()

    This virtual function allows subclasses of QWSMouseHandler to
    clear the calibration information. Note that the default
    implementation does nothing.

    \sa QWSCalibratedMouseHandler::clearCalibration(), calibrate()
*/

/*!
    \fn QWSMouseHandler::calibrate(const QWSPointerCalibrationData *data)

    This virtual function allows subclasses of QWSMouseHandler to set
    the calibration information passed in the given \a data. Note that
    the default implementation does nothing.

    \sa QWSCalibratedMouseHandler::calibrate(), clearCalibration()
*/

/*! \fn QWSMouseHandler::getCalibration(QWSPointerCalibrationData *data) const
    This virtual function allows subclasses of QWSMouseHandler
    to fill in the device coordinates in \a data with values
    that correspond to screen coordinates that are already in
    \a data. Note that the default implementation does nothing.
 */

/*!
    \class QWSCalibratedMouseHandler
    \ingroup qws

    \brief The QWSCalibratedMouseHandler class provides mouse
    calibration and noise reduction in Qt for Embedded Linux.

    Note that this class is only available in \l{Qt for Embedded Linux}.

    \l{Qt for Embedded Linux} provides ready-made drivers for several mouse
    protocols, see the \l{Qt for Embedded Linux Pointer Handling}{pointer
    handling} documentation for details. In general, custom mouse
    drivers can be implemented by subclassing the QWSMouseHandler
    class. But when the system device does not have a fixed mapping
    between device and screen coordinates and/or produces noisy events
    (e.g., a touchscreen), you should derive from the
    QWSCalibratedMouseHandler class instead to take advantage of its
    calibration functionality. As always, you must also create a mouse
    driver plugin (derived from QMouseDriverPlugin);
    the implementation of the QMouseDriverFactory class will then
    automatically detect the plugin, and load the driver into the
    server application at run-time using Qt's
    \l{How to Create Qt Plugins}{plugin system}.

    QWSCalibratedMouseHandler provides an implementation of the
    calibrate() function to update the calibration parameters based on
    coordinate mapping of the given calibration data. The calibration
    data is represented by an QWSPointerCalibrationData object. The
    linear transformation between device coordinates and screen
    coordinates is performed by calling the transform() function
    explicitly on the points passed to the
    QWSMouseHandler::mouseChanged() function. Use the
    clearCalibration() function to make the mouse driver return mouse
    events in raw device coordinates and not in screen coordinates.

    The calibration parameters are recalculated whenever calibrate()
    is called, and they can be stored using the writeCalibration()
    function. Previously written parameters can be retrieved at any
    time using the readCalibration() function (calibration parameters
    are always read when the class is instantiated). Note that the
    calibration parameters is written to and read from the file
    currently specified by the POINTERCAL_FILE environment variable;
    the default file is \c /etc/pointercal.

    To achieve noise reduction, QWSCalibratedMouseHandler provides the
    sendFiltered() function. Use this function instead of
    mouseChanged() whenever a mouse event occurs. The filter's size
    can be manipulated using the setFilterSize() function.

    \sa QWSMouseHandler, QWSPointerCalibrationData,
    {Mouse Calibration Example}
*/


/*!
    \internal
 */

QWSCalibratedMouseHandler::QWSCalibratedMouseHandler(const QString &, const QString &)
    : samples(5), currSample(0), numSamples(0)
{
    clearCalibration();
    readCalibration();
}

/*!
    Fills \a cd with the device coordinates corresponding to the given
    screen coordinates.

    \internal
*/
void QWSCalibratedMouseHandler::getCalibration(QWSPointerCalibrationData *cd) const
{
    const qint64 scale = qint64(a) * qint64(e) - qint64(b) * qint64(d);
    const qint64 xOff = qint64(b) * qint64(f) - qint64(c) * qint64(e);
    const qint64 yOff = qint64(c) * qint64(d) - qint64(a) * qint64(f);
    for (int i = 0; i <= QWSPointerCalibrationData::LastLocation; ++i) {
        const qint64 sX = cd->screenPoints[i].x();
        const qint64 sY = cd->screenPoints[i].y();
        const qint64 dX = (s*(e*sX - b*sY) + xOff) / scale;
        const qint64 dY = (s*(a*sY - d*sX) + yOff) / scale;
        cd->devPoints[i] = QPoint(dX, dY);
    }
}

/*!
    Clears the current calibration, i.e., makes the mouse
    driver return mouse events in raw device coordinates instead of
    screen coordinates.

    \sa calibrate()
*/
void QWSCalibratedMouseHandler::clearCalibration()
{
    a = 1;
    b = 0;
    c = 0;
    d = 0;
    e = 1;
    f = 0;
    s = 1;
}


/*!
    Saves the current calibration parameters in \c /etc/pointercal
    (separated by whitespace and in alphabetical order).

    You can override the default \c /etc/pointercal by specifying
    another file using the POINTERCAL_FILE environment variable.

    \sa readCalibration()
*/
void QWSCalibratedMouseHandler::writeCalibration()
{
    QString calFile;
    calFile = QString::fromLocal8Bit(qgetenv("POINTERCAL_FILE"));
    if (calFile.isEmpty())
        calFile = QLatin1String("/etc/pointercal");

#ifndef QT_NO_TEXTSTREAM
    QFile file(calFile);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream t(&file);
        t << a << ' ' << b << ' ' << c << ' ';
        t << d << ' ' << e << ' ' << f << ' ' << s << endl;
    } else
#endif
    {
        qCritical("QWSCalibratedMouseHandler::writeCalibration: "
                  "Could not save calibration into %s", qPrintable(calFile));
    }
}

/*!
    Reads previously written calibration parameters which are stored
    in \c /etc/pointercal (separated by whitespace and in alphabetical
    order).

    You can override the default \c /etc/pointercal by specifying
    another file using the POINTERCAL_FILE environment variable.


    \sa writeCalibration()
*/
void QWSCalibratedMouseHandler::readCalibration()
{
    QString calFile = QString::fromLocal8Bit(qgetenv("POINTERCAL_FILE"));
    if (calFile.isEmpty())
        calFile = QLatin1String("/etc/pointercal");

#ifndef QT_NO_TEXTSTREAM
    QFile file(calFile);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream t(&file);
        t >> a >> b >> c >> d >> e >> f >> s;
        if (s == 0 || t.status() != QTextStream::Ok) {
            qCritical("Corrupt calibration data");
            clearCalibration();
        }
    } else
#endif
    {
        qDebug() << "Could not read calibration:" <<calFile;
    }
}

static int ilog2(quint32 n)
{
    int result = 0;

    if (n & 0xffff0000) {
        n >>= 16;
        result += 16;
    }
    if (n & 0xff00) {
        n >>= 8;
        result += 8;}
    if (n & 0xf0) {
        n >>= 4;
        result += 4;
    }
    if (n & 0xc) {
        n >>= 2;
        result += 2;
    }
    if (n & 0x2)
        result += 1;

    return result;
}

/*!
    Updates the calibration parameters based on coordinate mapping of
    the given \a data.

    Create an instance of the QWSPointerCalibrationData class, fill in
    the device and screen coordinates and pass that object to the mouse
    driver using this function.

    \sa clearCalibration(), transform()
*/
void QWSCalibratedMouseHandler::calibrate(const QWSPointerCalibrationData *data)
{
    // Algorithm derived from
    // "How To Calibrate Touch Screens" by Carlos E. Vidales,
    // printed in Embedded Systems Programming, Vol. 15 no 6, June 2002
    // URL: http://www.embedded.com/showArticle.jhtml?articleID=9900629

    const QPoint pd0 = data->devPoints[QWSPointerCalibrationData::TopLeft];
    const QPoint pd1 = data->devPoints[QWSPointerCalibrationData::TopRight];
    const QPoint pd2 = data->devPoints[QWSPointerCalibrationData::BottomRight];
    const QPoint p0 = data->screenPoints[QWSPointerCalibrationData::TopLeft];
    const QPoint p1 = data->screenPoints[QWSPointerCalibrationData::TopRight];
    const QPoint p2 = data->screenPoints[QWSPointerCalibrationData::BottomRight];

    const qint64 xd0 = pd0.x();
    const qint64 xd1 = pd1.x();
    const qint64 xd2 = pd2.x();
    const qint64 yd0 = pd0.y();
    const qint64 yd1 = pd1.y();
    const qint64 yd2 = pd2.y();
    const qint64 x0 = p0.x();
    const qint64 x1 = p1.x();
    const qint64 x2 = p2.x();
    const qint64 y0 = p0.y();
    const qint64 y1 = p1.y();
    const qint64 y2 = p2.y();

    qint64 scale = ((xd0 - xd2)*(yd1 - yd2) - (xd1 - xd2)*(yd0 - yd2));
    int shift = 0;
    qint64 absScale = qAbs(scale);
    // use maximum 16 bit precision to reduce risk of integer overflow
    if (absScale > (1 << 16)) {
        shift = ilog2(absScale >> 16) + 1;
        scale >>= shift;
    }

    s = scale;
    a = ((x0 - x2)*(yd1 - yd2) - (x1 - x2)*(yd0 - yd2)) >> shift;
    b = ((xd0 - xd2)*(x1 - x2) - (x0 - x2)*(xd1 - xd2)) >> shift;
    c = (yd0*(xd2*x1 - xd1*x2) + yd1*(xd0*x2 - xd2*x0) + yd2*(xd1*x0 - xd0*x1)) >> shift;
    d = ((y0 - y2)*(yd1 - yd2) - (y1 - y2)*(yd0 - yd2)) >> shift;
    e = ((xd0 - xd2)*(y1 - y2) - (y0 - y2)*(xd1 - xd2)) >> shift;
    f = (yd0*(xd2*y1 - xd1*y2) + yd1*(xd0*y2 - xd2*y0) + yd2*(xd1*y0 - xd0*y1)) >> shift;

    writeCalibration();
}

/*!
    Transforms the given \a position from device coordinates to screen
    coordinates, and returns the transformed position.

    This function is typically called explicitly on the points passed
    to the QWSMouseHandler::mouseChanged() function.

    This implementation is a linear transformation using 7 parameters
    (\c a, \c b, \c c, \c d, \c e, \c f and \c s) to transform the
    device coordinates (\c Xd, \c Yd) into screen coordinates (\c Xs,
    \c Ys) using the following equations:

    \snippet doc/src/snippets/code/src_gui_embedded_qmouse_qws.cpp 0

    \sa mouseChanged()
*/
QPoint QWSCalibratedMouseHandler::transform(const QPoint &position)
{
    QPoint tp;

    tp.setX((a * position.x() + b * position.y() + c) / s);
    tp.setY((d * position.x() + e * position.y() + f) / s);

    return tp;
}

/*!
    Sets the size of the filter used in noise reduction to the given
    \a size.

    The sendFiltered() function reduces noice by calculating an
    average position from a collection of mouse event positions. The
    filter size determines the number of positions that forms the
    basis for these calculations.

    \sa sendFiltered()
*/
void QWSCalibratedMouseHandler::setFilterSize(int size)
{
    samples.resize(qMax(1, size));
    numSamples = 0;
    currSample = 0;
}

/*!
    \fn bool QWSCalibratedMouseHandler::sendFiltered(const QPoint &position, int state)

    Notifies the system of a new mouse event \e after applying a noise
    reduction filter. Returns true if the filtering process is
    successful; otherwise returns false. Note that if the filtering
    process failes, the system is not notified about the event.

    The given \a position is the global position of the mouse. The \a
    state parameter is a bitmask of the Qt::MouseButton enum's values
    indicating which mouse buttons are pressed.

    The noice is reduced by calculating an average position from a
    collection of mouse event positions and then calling the
    mouseChanged() function with the new position. The number of
    positions that is used is determined by the filter size.

    \sa mouseChanged(), setFilterSize()
*/
bool QWSCalibratedMouseHandler::sendFiltered(const QPoint &position, int button)
{
    if (!button) {
        if (numSamples >= samples.count())
            mouseChanged(transform(position), 0);
        currSample = 0;
        numSamples = 0;
        return true;
    }

    bool sent = false;
    samples[currSample] = position;
    numSamples++;
    if (numSamples >= samples.count()) {

        int ignore = -1;
        if (samples.count() > 2) { // throw away the "worst" sample
            int maxd = 0;
            for (int i = 0; i < samples.count(); i++) {
                int d = (mousePos - samples[i]).manhattanLength();
                if (d > maxd) {
                    maxd = d;
                    ignore = i;
                }
            }
        }

        // average the rest
        QPoint pos(0, 0);
        int numAveraged = 0;
        for (int i = 0; i < samples.count(); i++) {
            if (ignore == i)
                continue;
            pos += samples[i];
            ++numAveraged;
        }
        if (numAveraged)
            pos /= numAveraged;

        mouseChanged(transform(pos), button);
        sent = true;
    }
    currSample++;
    if (currSample >= samples.count())
        currSample = 0;

    return sent;
}

QT_END_NAMESPACE
