/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
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

#include "qicon.h"
#include "qicon_p.h"
#include "qiconengine.h"
#include "qiconengineplugin.h"
#include "qimagereader.h"
#include "private/qfactoryloader_p.h"
#include "private/qiconloader_p.h"
#include "qpainter.h"
#include "qfileinfo.h"
#if QT_CONFIG(mimetype)
#include <qmimedatabase.h>
#include <qmimetype.h>
#endif
#include "qpixmapcache.h"
#include "qvariant.h"
#include "qcache.h"
#include "qdebug.h"
#include "qdir.h"
#include "qpalette.h"
#include "qmath.h"

#include "private/qhexstring_p.h"
#include "private/qguiapplication_p.h"
#include "qpa/qplatformtheme.h"

#ifndef QT_NO_ICON
QT_BEGIN_NAMESPACE

/*!
    \enum QIcon::Mode

    This enum type describes the mode for which a pixmap is intended
    to be used. The currently defined modes are:

    \value Normal
         Display the pixmap when the user is
        not interacting with the icon, but the
        functionality represented by the icon is available.
    \value Disabled
         Display the pixmap when the
        functionality represented by the icon is not available.
    \value Active
         Display the pixmap when the
        functionality represented by the icon is available and
        the user is interacting with the icon, for example, moving the
        mouse over it or clicking it.
   \value Selected
        Display the pixmap when the item represented by the icon is
        selected.
*/

/*!
  \enum QIcon::State

  This enum describes the state for which a pixmap is intended to be
  used. The \e state can be:

  \value Off  Display the pixmap when the widget is in an "off" state
  \value On  Display the pixmap when the widget is in an "on" state
*/

static int nextSerialNumCounter()
{
    static QBasicAtomicInt serial = Q_BASIC_ATOMIC_INITIALIZER(0);
    return 1 + serial.fetchAndAddRelaxed(1);
}

static void qt_cleanup_icon_cache();
namespace {
    struct IconCache : public QCache<QString, QIcon>
    {
        IconCache()
        {
            // ### note: won't readd if QApplication is re-created!
            qAddPostRoutine(qt_cleanup_icon_cache);
        }
    };
}

Q_GLOBAL_STATIC(IconCache, qtIconCache)

static void qt_cleanup_icon_cache()
{
    qtIconCache()->clear();
}

/*! \internal

    Returns the effective device pixel ratio, using
    the provided window pointer if possible.

    if Qt::AA_UseHighDpiPixmaps is not set this function
    returns 1.0 to keep non-hihdpi aware code working.
*/
static qreal qt_effective_device_pixel_ratio(QWindow *window = nullptr)
{
    if (!qApp->testAttribute(Qt::AA_UseHighDpiPixmaps))
        return qreal(1.0);

    if (window)
        return window->devicePixelRatio();

    return qApp->devicePixelRatio(); // Don't know which window to target.
}

QIconPrivate::QIconPrivate(QIconEngine *e)
    : engine(e), ref(1),
      serialNum(nextSerialNumCounter()),
    detach_no(0),
    is_mask(false)
{
}

/*! \internal
    Computes the displayDevicePixelRatio for a pixmap.

    If displayDevicePixelRatio is 1.0 the reurned value is 1.0, always.

    For a displayDevicePixelRatio of 2.0 the returned value will be between
    1.0 and 2.0, depending on requestedSize and actualsize:
    * If actualsize < requestedSize        : 1.0 (not enough pixels for a normal-dpi pixmap)
    * If actualsize == requestedSize * 2.0 : 2.0 (enough pixels for a high-dpi pixmap)
    * else : a scaled value between 1.0 and 2.0. (pixel count is between normal-dpi and high-dpi)
*/
qreal QIconPrivate::pixmapDevicePixelRatio(qreal displayDevicePixelRatio, const QSize &requestedSize, const QSize &actualSize)
{
    QSize targetSize = requestedSize * displayDevicePixelRatio;
    if ((actualSize.width() == targetSize.width() && actualSize.height() <= targetSize.height()) ||
        (actualSize.width() <= targetSize.width() && actualSize.height() == targetSize.height())) {
        // Correctly scaled for dpr, just having different aspect ratio
        return displayDevicePixelRatio;
    }
    qreal scale = 0.5 * (qreal(actualSize.width()) / qreal(targetSize.width()) +
                         qreal(actualSize.height() / qreal(targetSize.height())));
    return qMax(qreal(1.0), displayDevicePixelRatio *scale);
}

QPixmapIconEngine::QPixmapIconEngine()
{
}

QPixmapIconEngine::QPixmapIconEngine(const QPixmapIconEngine &other)
    : QIconEngine(other), pixmaps(other.pixmaps)
{
}

QPixmapIconEngine::~QPixmapIconEngine()
{
}

void QPixmapIconEngine::paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state)
{
    qreal dpr = 1.0;
    if (QCoreApplication::testAttribute(Qt::AA_UseHighDpiPixmaps)) {
      auto paintDevice = painter->device();
      dpr = paintDevice ? paintDevice->devicePixelRatioF() : qApp->devicePixelRatio();
    }
    const QSize pixmapSize = rect.size() * dpr;
    QPixmap px = pixmap(pixmapSize, mode, state);
    painter->drawPixmap(rect, px);
}

static inline int area(const QSize &s) { return s.width() * s.height(); }

// returns the smallest of the two that is still larger than or equal to size.
static QPixmapIconEngineEntry *bestSizeMatch( const QSize &size, QPixmapIconEngineEntry *pa, QPixmapIconEngineEntry *pb)
{
    int s = area(size);
    if (pa->size == QSize() && pa->pixmap.isNull()) {
        pa->pixmap = QPixmap(pa->fileName);
        pa->size = pa->pixmap.size();
    }
    int a = area(pa->size);
    if (pb->size == QSize() && pb->pixmap.isNull()) {
        pb->pixmap = QPixmap(pb->fileName);
        pb->size = pb->pixmap.size();
    }
    int b = area(pb->size);
    int res = a;
    if (qMin(a,b) >= s)
        res = qMin(a,b);
    else
        res = qMax(a,b);
    if (res == a)
        return pa;
    return pb;
}

QPixmapIconEngineEntry *QPixmapIconEngine::tryMatch(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmapIconEngineEntry *pe = nullptr;
    for (int i = 0; i < pixmaps.count(); ++i)
        if (pixmaps.at(i).mode == mode && pixmaps.at(i).state == state) {
            if (pe)
                pe = bestSizeMatch(size, &pixmaps[i], pe);
            else
                pe = &pixmaps[i];
        }
    return pe;
}


QPixmapIconEngineEntry *QPixmapIconEngine::bestMatch(const QSize &size, QIcon::Mode mode, QIcon::State state, bool sizeOnly)
{
    QPixmapIconEngineEntry *pe = tryMatch(size, mode, state);
    while (!pe){
        QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off : QIcon::On;
        if (mode == QIcon::Disabled || mode == QIcon::Selected) {
            QIcon::Mode oppositeMode = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;
            if ((pe = tryMatch(size, QIcon::Normal, state)))
                break;
            if ((pe = tryMatch(size, QIcon::Active, state)))
                break;
            if ((pe = tryMatch(size, mode, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Normal, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Active, oppositeState)))
                break;
            if ((pe = tryMatch(size, oppositeMode, state)))
                break;
            if ((pe = tryMatch(size, oppositeMode, oppositeState)))
                break;
        } else {
            QIcon::Mode oppositeMode = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
            if ((pe = tryMatch(size, oppositeMode, state)))
                break;
            if ((pe = tryMatch(size, mode, oppositeState)))
                break;
            if ((pe = tryMatch(size, oppositeMode, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Disabled, state)))
                break;
            if ((pe = tryMatch(size, QIcon::Selected, state)))
                break;
            if ((pe = tryMatch(size, QIcon::Disabled, oppositeState)))
                break;
            if ((pe = tryMatch(size, QIcon::Selected, oppositeState)))
                break;
        }

        if (!pe)
            return pe;
    }

    if (sizeOnly ? (pe->size.isNull() || !pe->size.isValid()) : pe->pixmap.isNull()) {
        pe->pixmap = QPixmap(pe->fileName);
        if (!pe->pixmap.isNull())
            pe->size = pe->pixmap.size();
    }

    return pe;
}

QPixmap QPixmapIconEngine::pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QPixmap pm;
    QPixmapIconEngineEntry *pe = bestMatch(size, mode, state, false);
    if (pe)
        pm = pe->pixmap;

    if (pm.isNull()) {
        int idx = pixmaps.count();
        while (--idx >= 0) {
            if (pe == &pixmaps.at(idx)) {
                pixmaps.remove(idx);
                break;
            }
        }
        if (pixmaps.isEmpty())
            return pm;
        else
            return pixmap(size, mode, state);
    }

    QSize actualSize = pm.size();
    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);

    QString key = QLatin1String("qt_")
                  % HexString<quint64>(pm.cacheKey())
                  % HexString<uint>(pe ? pe->mode : QIcon::Normal)
                  % HexString<quint64>(QGuiApplication::palette().cacheKey())
                  % HexString<uint>(actualSize.width())
                  % HexString<uint>(actualSize.height());

    if (mode == QIcon::Active) {
        if (QPixmapCache::find(key % HexString<uint>(mode), &pm))
            return pm; // horray
        if (QPixmapCache::find(key % HexString<uint>(QIcon::Normal), &pm)) {
            QPixmap active = pm;
            if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
                active = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(QIcon::Active, pm);
            if (pm.cacheKey() == active.cacheKey())
                return pm;
        }
    }

    if (!QPixmapCache::find(key % HexString<uint>(mode), &pm)) {
        if (pm.size() != actualSize)
            pm = pm.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if (pe->mode != mode && mode != QIcon::Normal) {
            QPixmap generated = pm;
            if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
                generated = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, pm);
            if (!generated.isNull())
                pm = generated;
        }
        QPixmapCache::insert(key % HexString<uint>(mode), pm);
    }
    return pm;
}

QSize QPixmapIconEngine::actualSize(const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    QSize actualSize;
    if (QPixmapIconEngineEntry *pe = bestMatch(size, mode, state, true))
        actualSize = pe->size;

    if (actualSize.isNull())
        return actualSize;

    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);
    return actualSize;
}

void QPixmapIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    if (!pixmap.isNull()) {
        QPixmapIconEngineEntry *pe = tryMatch(pixmap.size(), mode, state);
        if(pe && pe->size == pixmap.size()) {
            pe->pixmap = pixmap;
            pe->fileName.clear();
        } else {
            pixmaps += QPixmapIconEngineEntry(pixmap, mode, state);
        }
    }
}

// Read out original image depth as set by ICOReader
static inline int origIcoDepth(const QImage &image)
{
    const QString s = image.text(QStringLiteral("_q_icoOrigDepth"));
    return s.isEmpty() ? 32 : s.toInt();
}

static inline int findBySize(const QVector<QImage> &images, const QSize &size)
{
    for (int i = 0; i < images.size(); ++i) {
        if (images.at(i).size() == size)
            return i;
    }
    return -1;
}

// Convenience class providing a bool read() function.
namespace {
class ImageReader
{
public:
    ImageReader(const QString &fileName) : m_reader(fileName), m_atEnd(false) {}

    QByteArray format() const { return m_reader.format(); }

    bool read(QImage *image)
    {
        if (m_atEnd)
            return false;
        *image = m_reader.read();
        if (!image->size().isValid()) {
            m_atEnd = true;
            return false;
        }
        m_atEnd = !m_reader.jumpToNextImage();
        return true;
    }

private:
    QImageReader m_reader;
    bool m_atEnd;
};
} // namespace

void QPixmapIconEngine::addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (fileName.isEmpty())
        return;
    const QString abs = fileName.startsWith(QLatin1Char(':')) ? fileName : QFileInfo(fileName).absoluteFilePath();
    const bool ignoreSize = !size.isValid();
    ImageReader imageReader(abs);
    const QByteArray format = imageReader.format();
    if (format.isEmpty()) // Device failed to open or unsupported format.
        return;
    QImage image;
    if (format != "ico") {
        if (ignoreSize) { // No size specified: Add all images.
            while (imageReader.read(&image))
                pixmaps += QPixmapIconEngineEntry(abs, image, mode, state);
        } else {
            // Try to match size. If that fails, add a placeholder with the filename and empty pixmap for the size.
            while (imageReader.read(&image) && image.size() != size) {}
            pixmaps += image.size() == size ?
                QPixmapIconEngineEntry(abs, image, mode, state) : QPixmapIconEngineEntry(abs, size, mode, state);
        }
        return;
    }
    // Special case for reading Windows ".ico" files. Historically (QTBUG-39287),
    // these files may contain low-resolution images. As this information is lost,
    // ICOReader sets the original format as an image text key value. Read all matching
    // images into a list trying to find the highest quality per size.
    QVector<QImage> icoImages;
    while (imageReader.read(&image)) {
        if (ignoreSize || image.size() == size) {
            const int position = findBySize(icoImages, image.size());
            if (position >= 0) { // Higher quality available? -> replace.
                if (origIcoDepth(image) > origIcoDepth(icoImages.at(position)))
                    icoImages[position] = image;
            } else {
                icoImages.append(image);
            }
        }
    }
    for (const QImage &i : qAsConst(icoImages))
        pixmaps += QPixmapIconEngineEntry(abs, i, mode, state);
    if (icoImages.isEmpty() && !ignoreSize) // Add placeholder with the filename and empty pixmap for the size.
        pixmaps += QPixmapIconEngineEntry(abs, size, mode, state);
}

QString QPixmapIconEngine::key() const
{
    return QLatin1String("QPixmapIconEngine");
}

QIconEngine *QPixmapIconEngine::clone() const
{
    return new QPixmapIconEngine(*this);
}

bool QPixmapIconEngine::read(QDataStream &in)
{
    int num_entries;
    QPixmap pm;
    QString fileName;
    QSize sz;
    uint mode;
    uint state;

    in >> num_entries;
    for (int i=0; i < num_entries; ++i) {
        if (in.atEnd()) {
            pixmaps.clear();
            return false;
        }
        in >> pm;
        in >> fileName;
        in >> sz;
        in >> mode;
        in >> state;
        if (pm.isNull()) {
            addFile(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
        } else {
            QPixmapIconEngineEntry pe(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
            pe.pixmap = pm;
            pixmaps += pe;
        }
    }
    return true;
}

bool QPixmapIconEngine::write(QDataStream &out) const
{
    int num_entries = pixmaps.size();
    out << num_entries;
    for (int i=0; i < num_entries; ++i) {
        if (pixmaps.at(i).pixmap.isNull())
            out << QPixmap(pixmaps.at(i).fileName);
        else
            out << pixmaps.at(i).pixmap;
        out << pixmaps.at(i).fileName;
        out << pixmaps.at(i).size;
        out << (uint) pixmaps.at(i).mode;
        out << (uint) pixmaps.at(i).state;
    }
    return true;
}

void QPixmapIconEngine::virtual_hook(int id, void *data)
{
    switch (id) {
    case QIconEngine::AvailableSizesHook: {
        QIconEngine::AvailableSizesArgument &arg =
            *reinterpret_cast<QIconEngine::AvailableSizesArgument*>(data);
        arg.sizes.clear();
        for (int i = 0; i < pixmaps.size(); ++i) {
            QPixmapIconEngineEntry &pe = pixmaps[i];
            if (pe.size == QSize() && pe.pixmap.isNull()) {
                pe.pixmap = QPixmap(pe.fileName);
                pe.size = pe.pixmap.size();
            }
            if (pe.mode == arg.mode && pe.state == arg.state && !pe.size.isEmpty())
                arg.sizes.push_back(pe.size);
        }
        break;
    }
    default:
        QIconEngine::virtual_hook(id, data);
    }
}

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, loader,
    (QIconEngineFactoryInterface_iid, QLatin1String("/iconengines"), Qt::CaseInsensitive))

QFactoryLoader *qt_iconEngineFactoryLoader()
{
    return loader();
}


/*!
  \class QIcon

  \brief The QIcon class provides scalable icons in different modes
  and states.

  \ingroup painting
  \ingroup shared
  \inmodule QtGui

  A QIcon can generate smaller, larger, active, and disabled pixmaps
  from the set of pixmaps it is given. Such pixmaps are used by Qt
  widgets to show an icon representing a particular action.

  The simplest use of QIcon is to create one from a QPixmap file or
  resource, and then use it, allowing Qt to work out all the required
  icon styles and sizes. For example:

  \snippet code/src_gui_image_qicon.cpp 0

  To undo a QIcon, simply set a null icon in its place:

  \snippet code/src_gui_image_qicon.cpp 1

  Use the QImageReader::supportedImageFormats() and
  QImageWriter::supportedImageFormats() functions to retrieve a
  complete list of the supported file formats.

  When you retrieve a pixmap using pixmap(QSize, Mode, State), and no
  pixmap for this given size, mode and state has been added with
  addFile() or addPixmap(), then QIcon will generate one on the
  fly. This pixmap generation happens in a QIconEngine. The default
  engine scales pixmaps down if required, but never up, and it uses
  the current style to calculate a disabled appearance. By using
  custom icon engines, you can customize every aspect of generated
  icons. With QIconEnginePlugin it is possible to register different
  icon engines for different file suffixes, making it possible for
  third parties to provide additional icon engines to those included
  with Qt.

  \note Since Qt 4.2, an icon engine that supports SVG is included.

  \section1 Making Classes that Use QIcon

  If you write your own widgets that have an option to set a small
  pixmap, consider allowing a QIcon to be set for that pixmap.  The
  Qt class QToolButton is an example of such a widget.

  Provide a method to set a QIcon, and when you draw the icon, choose
  whichever pixmap is appropriate for the current state of your widget.
  For example:
  \snippet code/src_gui_image_qicon.cpp 2

  You might also make use of the \c Active mode, perhaps making your
  widget \c Active when the mouse is over the widget (see \l
  QWidget::enterEvent()), while the mouse is pressed pending the
  release that will activate the function, or when it is the currently
  selected item. If the widget can be toggled, the "On" mode might be
  used to draw a different icon.

  \image icon.png QIcon

  \note QIcon needs a QGuiApplication instance before the icon is created.

  \section1 High DPI Icons

  There are two ways that QIcon supports \l {High DPI Displays}{high DPI}
  icons: via \l addFile() and \l fromTheme().

  \l addFile() is useful if you have your own custom directory structure and do
  not need to use the \l {Icon Theme Specification}{freedesktop.org Icon Theme
  Specification}. Icons created via this approach use Qt's \l {High Resolution
  Versions of Images}{"@nx" high DPI syntax}.

  Using \l fromTheme() is necessary if you plan on following the Icon Theme
  Specification. To make QIcon use the high DPI version of an image, add an
  additional entry to the appropriate \c index.theme file:

  \badcode
  [Icon Theme]
  Name=Test
  Comment=Test Theme

  Directories=32x32/actions,32x32@2/actions

  [32x32/actions]
  Size=32
  Context=Actions
  Type=Fixed

  # High DPI version of the entry above.
  [32x32@2/actions]
  Size=32
  Scale=2
  Type=Fixed
  \endcode

  Your icon theme directory would then look something like this:

  \badcode
    ├── 32x32
    │   └── actions
    │       └── appointment-new.png
    ├── 32x32@2
    │   └── actions
    │       └── appointment-new.png
    └── index.theme
  \endcode

  \sa {fowler}{GUI Design Handbook: Iconic Label}, {Icons Example}
*/


/*!
  Constructs a null icon.
*/
QIcon::QIcon() noexcept
    : d(nullptr)
{
}

/*!
  Constructs an icon from a \a pixmap.
 */
QIcon::QIcon(const QPixmap &pixmap)
    :d(nullptr)
{
    addPixmap(pixmap);
}

/*!
  Constructs a copy of \a other. This is very fast.
*/
QIcon::QIcon(const QIcon &other)
    :d(other.d)
{
    if (d)
        d->ref.ref();
}

/*!
  \fn QIcon::QIcon(QIcon &&other)

  Move-constructs a QIcon instance, making it point to the same object
  that \a other was pointing to.
*/

/*!
    Constructs an icon from the file with the given \a fileName. The
    file will be loaded on demand.

    If \a fileName contains a relative path (e.g. the filename only)
    the relevant file must be found relative to the runtime working
    directory.

    The file name can refer to an actual file on disk or to
    one of the application's embedded resources.  See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    Use the QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormats() functions to retrieve a
    complete list of the supported file formats.
*/
QIcon::QIcon(const QString &fileName)
    : d(nullptr)
{
    addFile(fileName);
}


/*!
    Creates an icon with a specific icon \a engine. The icon takes
    ownership of the engine.
*/
QIcon::QIcon(QIconEngine *engine)
    :d(new QIconPrivate(engine))
{
}

/*!
    Destroys the icon.
*/
QIcon::~QIcon()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Assigns the \a other icon to this icon and returns a reference to
    this icon.
*/
QIcon &QIcon::operator=(const QIcon &other)
{
    if (other.d)
        other.d->ref.ref();
    if (d && !d->ref.deref())
        delete d;
    d = other.d;
    return *this;
}

/*!
    \fn QIcon &QIcon::operator=(QIcon &&other)

    Move-assigns \a other to this QIcon instance.

    \since 5.2
*/

/*!
    \fn void QIcon::swap(QIcon &other)
    \since 4.8

    Swaps icon \a other with this icon. This operation is very
    fast and never fails.
*/

/*!
   Returns the icon as a QVariant.
*/
QIcon::operator QVariant() const
{
    return QVariant(QMetaType::QIcon, this);
}

/*! \fn int QIcon::serialNumber() const
    \obsolete

    Returns a number that identifies the contents of this
    QIcon object. Distinct QIcon objects can have
    the same serial number if they refer to the same contents
    (but they don't have to). Also, the serial number of
    a QIcon object may change during its lifetime.

    Use cacheKey() instead.

    A null icon always has a serial number of 0.

    Serial numbers are mostly useful in conjunction with caching.

    \sa QPixmap::serialNumber()
*/

/*!
    Returns a number that identifies the contents of this QIcon
    object. Distinct QIcon objects can have the same key if
    they refer to the same contents.
    \since 4.3

    The cacheKey() will change when the icon is altered via
    addPixmap() or addFile().

    Cache keys are mostly useful in conjunction with caching.

    \sa QPixmap::cacheKey()
*/
qint64 QIcon::cacheKey() const
{
    if (!d)
        return 0;
    return (((qint64) d->serialNum) << 32) | ((qint64) (d->detach_no));
}

/*!
  Returns a pixmap with the requested \a size, \a mode, and \a
  state, generating one if necessary. The pixmap might be smaller than
  requested, but never larger.

  Setting the Qt::AA_UseHighDpiPixmaps application attribute enables this
  function to return pixmaps that are larger than the requested size. Such
  images will have a devicePixelRatio larger than 1.

  \sa actualSize(), paint()
*/
QPixmap QIcon::pixmap(const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QPixmap();
    return pixmap(nullptr, size, mode, state);
}

/*!
    \fn QPixmap QIcon::pixmap(int w, int h, Mode mode = Normal, State state = Off) const

    \overload

    Returns a pixmap of size QSize(\a w, \a h). The pixmap might be smaller than
    requested, but never larger.

    Setting the Qt::AA_UseHighDpiPixmaps application attribute enables this
    function to return pixmaps that are larger than the requested size. Such
    images will have a devicePixelRatio larger than 1.
*/

/*!
    \fn QPixmap QIcon::pixmap(int extent, Mode mode = Normal, State state = Off) const

    \overload

    Returns a pixmap of size QSize(\a extent, \a extent). The pixmap might be smaller
    than requested, but never larger.

    Setting the Qt::AA_UseHighDpiPixmaps application attribute enables this
    function to return pixmaps that are larger than the requested size. Such
    images will have a devicePixelRatio larger than 1.
*/

/*!  Returns the actual size of the icon for the requested \a size, \a
  mode, and \a state. The result might be smaller than requested, but
  never larger. The returned size is in device-independent pixels (This
  is relevant for high-dpi pixmaps.)

  \sa pixmap(), paint()
*/
QSize QIcon::actualSize(const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QSize();
    return actualSize(nullptr, size, mode, state);
}

/*!
  \since 5.1

  Returns a pixmap with the requested \a window \a size, \a mode, and \a
  state, generating one if necessary.

  The pixmap can be smaller than the requested size. If \a window is on
  a high-dpi display the pixmap can be larger. In that case it will have
  a devicePixelRatio larger than 1.

  \sa  actualSize(), paint()
*/
QPixmap QIcon::pixmap(QWindow *window, const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QPixmap();

    qreal devicePixelRatio = qt_effective_device_pixel_ratio(window);

    // Handle the simple normal-dpi case:
    if (!(devicePixelRatio > 1.0)) {
        QPixmap pixmap = d->engine->pixmap(size, mode, state);
        pixmap.setDevicePixelRatio(1.0);
        return pixmap;
    }

    // Try get a pixmap that is big enough to be displayed at device pixel resolution.
    QIconEngine::ScaledPixmapArgument scalePixmapArg = { size * devicePixelRatio, mode, state, devicePixelRatio, QPixmap() };
    d->engine->virtual_hook(QIconEngine::ScaledPixmapHook, reinterpret_cast<void*>(&scalePixmapArg));
    scalePixmapArg.pixmap.setDevicePixelRatio(d->pixmapDevicePixelRatio(devicePixelRatio, size, scalePixmapArg.pixmap.size()));
    return scalePixmapArg.pixmap;
}

/*!
  \since 5.1

  Returns the actual size of the icon for the requested \a window  \a size, \a
  mode, and \a state.

  The pixmap can be smaller than the requested size. The returned size
  is in device-independent pixels (This is relevant for high-dpi pixmaps.)

  \sa actualSize(), pixmap(), paint()
*/
QSize QIcon::actualSize(QWindow *window, const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QSize();

    qreal devicePixelRatio = qt_effective_device_pixel_ratio(window);

    // Handle the simple normal-dpi case:
    if (!(devicePixelRatio > 1.0))
        return d->engine->actualSize(size, mode, state);

    QSize actualSize = d->engine->actualSize(size * devicePixelRatio, mode, state);
    return actualSize / d->pixmapDevicePixelRatio(devicePixelRatio, size, actualSize);
}

/*!
    Uses the \a painter to paint the icon with specified \a alignment,
    required \a mode, and \a state into the rectangle \a rect.

    \sa actualSize(), pixmap()
*/
void QIcon::paint(QPainter *painter, const QRect &rect, Qt::Alignment alignment, Mode mode, State state) const
{
    if (!d || !painter)
        return;

    // Copy of QStyle::alignedRect
    const QSize size = d->engine->actualSize(rect.size(), mode, state);
    alignment = QGuiApplicationPrivate::visualAlignment(painter->layoutDirection(), alignment);
    int x = rect.x();
    int y = rect.y();
    int w = size.width();
    int h = size.height();
    if ((alignment & Qt::AlignVCenter) == Qt::AlignVCenter)
        y += rect.size().height()/2 - h/2;
    else if ((alignment & Qt::AlignBottom) == Qt::AlignBottom)
        y += rect.size().height() - h;
    if ((alignment & Qt::AlignRight) == Qt::AlignRight)
        x += rect.size().width() - w;
    else if ((alignment & Qt::AlignHCenter) == Qt::AlignHCenter)
        x += rect.size().width()/2 - w/2;
    QRect alignedRect(x, y, w, h);

    d->engine->paint(painter, alignedRect, mode, state);
}

/*!
    \fn void QIcon::paint(QPainter *painter, int x, int y, int w, int h, Qt::Alignment alignment,
                          Mode mode, State state) const

    \overload

    Paints the icon into the rectangle QRect(\a x, \a y, \a w, \a h).
*/

/*!
    Returns \c true if the icon is empty; otherwise returns \c false.

    An icon is empty if it has neither a pixmap nor a filename.

    Note: Even a non-null icon might not be able to create valid
    pixmaps, eg. if the file does not exist or cannot be read.
*/
bool QIcon::isNull() const
{
    return !d || d->engine->isNull();
}

/*!\internal
 */
bool QIcon::isDetached() const
{
    return !d || d->ref.loadRelaxed() == 1;
}

/*! \internal
 */
void QIcon::detach()
{
    if (d) {
        if (d->engine->isNull()) {
            if (!d->ref.deref())
                delete d;
            d = nullptr;
            return;
        } else if (d->ref.loadRelaxed() != 1) {
            QIconPrivate *x = new QIconPrivate(d->engine->clone());
            if (!d->ref.deref())
                delete d;
            d = x;
        }
        ++d->detach_no;
    }
}

/*!
    Adds \a pixmap to the icon, as a specialization for \a mode and
    \a state.

    Custom icon engines are free to ignore additionally added
    pixmaps.

    \sa addFile()
*/
void QIcon::addPixmap(const QPixmap &pixmap, Mode mode, State state)
{
    if (pixmap.isNull())
        return;
    detach();
    if (!d)
        d = new QIconPrivate(new QPixmapIconEngine);
    d->engine->addPixmap(pixmap, mode, state);
}

static QIconEngine *iconEngineFromSuffix(const QString &fileName, const QString &suffix)
{
    if (!suffix.isEmpty()) {
        const int index = loader()->indexOf(suffix);
        if (index != -1) {
            if (QIconEnginePlugin *factory = qobject_cast<QIconEnginePlugin*>(loader()->instance(index))) {
                return factory->create(fileName);
            }
        }
    }
    return nullptr;
}

/*!  Adds an image from the file with the given \a fileName to the
     icon, as a specialization for \a size, \a mode and \a state. The
     file will be loaded on demand. Note: custom icon engines are free
     to ignore additionally added pixmaps.

     If \a fileName contains a relative path (e.g. the filename only)
     the relevant file must be found relative to the runtime working
     directory.

    The file name can refer to an actual file on disk or to
    one of the application's embedded resources. See the
    \l{resources.html}{Resource System} overview for details on how to
    embed images and other resource files in the application's
    executable.

    Use the QImageReader::supportedImageFormats() and
    QImageWriter::supportedImageFormats() functions to retrieve a
    complete list of the supported file formats.

    If a high resolution version of the image exists (identified by
    the suffix \c @2x on the base name), it is automatically loaded
    and added with the \e{device pixel ratio} set to a value of 2.
    This can be disabled by setting the environment variable
    \c QT_HIGHDPI_DISABLE_2X_IMAGE_LOADING (see QImageReader).

    \note When you add a non-empty filename to a QIcon, the icon becomes
    non-null, even if the file doesn't exist or points to a corrupt file.

    \sa addPixmap(), QPixmap::devicePixelRatio()
 */
void QIcon::addFile(const QString &fileName, const QSize &size, Mode mode, State state)
{
    if (fileName.isEmpty())
        return;
    detach();
    if (!d) {

        QFileInfo info(fileName);
        QString suffix = info.suffix();
#if QT_CONFIG(mimetype)
        if (suffix.isEmpty())
            suffix = QMimeDatabase().mimeTypeForFile(info).preferredSuffix(); // determination from contents
#endif // mimetype
        QIconEngine *engine = iconEngineFromSuffix(fileName, suffix);
        d = new QIconPrivate(engine ? engine : new QPixmapIconEngine);
    }

    d->engine->addFile(fileName, size, mode, state);

    // Check if a "@Nx" file exists and add it.
    QString atNxFileName = qt_findAtNxFile(fileName, qApp->devicePixelRatio());
    if (atNxFileName != fileName)
        d->engine->addFile(atNxFileName, size, mode, state);
}

/*!
    \since 4.5

    Returns a list of available icon sizes for the specified \a mode and
    \a state.
*/
QList<QSize> QIcon::availableSizes(Mode mode, State state) const
{
    if (!d || !d->engine)
        return QList<QSize>();
    return d->engine->availableSizes(mode, state);
}

/*!
    \since 4.7

    Returns the name used to create the icon, if available.

    Depending on the way the icon was created, it may have an associated
    name. This is the case for icons created with fromTheme() or icons
    using a QIconEngine which supports the QIconEngine::IconNameHook.

    \sa fromTheme(), QIconEngine
*/
QString QIcon::name() const
{
    if (!d || !d->engine)
        return QString();
    return d->engine->iconName();
}

/*!
    \since 4.6

    Sets the search paths for icon themes to \a paths.
    \sa themeSearchPaths(), fromTheme(), setThemeName()
*/
void QIcon::setThemeSearchPaths(const QStringList &paths)
{
    QIconLoader::instance()->setThemeSearchPath(paths);
}

/*!
  \since 4.6

  Returns the search paths for icon themes.

  The default value will depend on the platform:

  On X11, the search path will use the XDG_DATA_DIRS environment
  variable if available.

  By default all platforms will have the resource directory
  \c{:\icons} as a fallback. You can use "rcc -project" to generate a
  resource file from your icon theme.

  \sa setThemeSearchPaths(), fromTheme(), setThemeName()
*/
QStringList QIcon::themeSearchPaths()
{
    return QIconLoader::instance()->themeSearchPaths();
}

/*!
    \since 5.11

    Returns the fallback search paths for icons.

    The default value will depend on the platform.

    \sa setFallbackSearchPaths(), themeSearchPaths()
*/
QStringList QIcon::fallbackSearchPaths()
{
    return QIconLoader::instance()->fallbackSearchPaths();
}

/*!
    \since 5.11

    Sets the fallback search paths for icons to \a paths.

    \note To add some path without replacing existing ones:

    \snippet code/src_gui_image_qicon.cpp 5

    \sa fallbackSearchPaths(), setThemeSearchPaths()
*/
void QIcon::setFallbackSearchPaths(const QStringList &paths)
{
    QIconLoader::instance()->setFallbackSearchPaths(paths);
}

/*!
    \since 4.6

    Sets the current icon theme to \a name.

    The \a name should correspond to a directory name in the
    themeSearchPath() containing an index.theme
    file describing its contents.

    \sa themeSearchPaths(), themeName()
*/
void QIcon::setThemeName(const QString &name)
{
    QIconLoader::instance()->setThemeName(name);
}

/*!
    \since 4.6

    Returns the name of the current icon theme.

    On X11, the current icon theme depends on your desktop
    settings. On other platforms it is not set by default.

    \sa setThemeName(), themeSearchPaths(), fromTheme(),
    hasThemeIcon()
*/
QString QIcon::themeName()
{
    return QIconLoader::instance()->themeName();
}

/*!
    \since 5.12

    Returns the name of the fallback icon theme.

    On X11, if not set, the fallback icon theme depends on your desktop
    settings. On other platforms it is not set by default.

    \sa setFallbackThemeName(), themeName()
*/
QString QIcon::fallbackThemeName()
{
    return QIconLoader::instance()->fallbackThemeName();
}

/*!
    \since 5.12

    Sets the fallback icon theme to \a name.

    The \a name should correspond to a directory name in the
    themeSearchPath() containing an index.theme
    file describing its contents.

    \note This should be done before creating \l QGuiApplication, to ensure
    correct initialization.

    \sa fallbackThemeName(), themeSearchPaths(), themeName()
*/
void QIcon::setFallbackThemeName(const QString &name)
{
    QIconLoader::instance()->setFallbackThemeName(name);
}

/*!
    \since 4.6

    Returns the QIcon corresponding to \a name in the current
    icon theme.

    The latest version of the freedesktop icon specification and naming
    specification can be obtained here:

    \list
    \li \l{http://standards.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html}
    \li \l{http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html}
    \endlist

    To fetch an icon from the current icon theme:

    \snippet code/src_gui_image_qicon.cpp 3

    \note By default, only X11 will support themed icons. In order to
    use themed icons on Mac and Windows, you will have to bundle a
    compliant theme in one of your themeSearchPaths() and set the
    appropriate themeName().

    \note Qt will make use of GTK's icon-theme.cache if present to speed up
    the lookup. These caches can be generated using gtk-update-icon-cache:
    \l{https://developer.gnome.org/gtk3/stable/gtk-update-icon-cache.html}.

    \note If an icon can't be found in the current theme, then it will be
    searched in fallbackSearchPaths() as an unthemed icon.

    \sa themeName(), setThemeName(), themeSearchPaths(), fallbackSearchPaths()
*/
QIcon QIcon::fromTheme(const QString &name)
{
    QIcon icon;

    if (qtIconCache()->contains(name)) {
        icon = *qtIconCache()->object(name);
    } else if (QDir::isAbsolutePath(name)) {
        return QIcon(name);
    } else {
        QPlatformTheme * const platformTheme = QGuiApplicationPrivate::platformTheme();
        bool hasUserTheme = QIconLoader::instance()->hasUserTheme();
        QIconEngine * const engine = (platformTheme && !hasUserTheme) ? platformTheme->createIconEngine(name)
                                                   : new QIconLoaderEngine(name);
        QIcon *cachedIcon  = new QIcon(engine);
        icon = *cachedIcon;
        qtIconCache()->insert(name, cachedIcon);
    }

    return icon;
}

/*!
    \overload

    Returns the QIcon corresponding to \a name in the current
    icon theme. If no such icon is found in the current theme
    \a fallback is returned instead.

    If you want to provide a guaranteed fallback for platforms that
    do not support theme icons, you can use the second argument:

    \snippet code/src_gui_image_qicon.cpp 4
*/
QIcon QIcon::fromTheme(const QString &name, const QIcon &fallback)
{
    QIcon icon = fromTheme(name);

    if (icon.isNull() || icon.availableSizes().isEmpty())
        return fallback;

    return icon;
}

/*!
    \since 4.6

    Returns \c true if there is an icon available for \a name in the
    current icon theme, otherwise returns \c false.

    \sa themeSearchPaths(), fromTheme(), setThemeName()
*/
bool QIcon::hasThemeIcon(const QString &name)
{
    QIcon icon = fromTheme(name);

    return icon.name() == name;
}

/*!
    \since 5.6

    Indicate that this icon is a mask image(boolean \a isMask), and hence can
    potentially be modified based on where it's displayed.
    \sa isMask()
*/
void QIcon::setIsMask(bool isMask)
{
    if (!d)
        d = new QIconPrivate(new QPixmapIconEngine);
    else
        detach();
    d->is_mask = isMask;
}

/*!
    \since 5.6

    Returns \c true if this icon has been marked as a mask image.
    Certain platforms render mask icons differently (for example,
    menu icons on \macos).

    \sa setIsMask()
*/
bool QIcon::isMask() const
{
    if (!d)
        return false;
    return d->is_mask;
}

/*****************************************************************************
  QIcon stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM)
/*!
    \fn QDataStream &operator<<(QDataStream &stream, const QIcon &icon)
    \relates QIcon
    \since 4.2

    Writes the given \a icon to the given \a stream as a PNG
    image. If the icon contains more than one image, all images will
    be written to the stream. Note that writing the stream to a file
    will not produce a valid image file.
*/

QDataStream &operator<<(QDataStream &s, const QIcon &icon)
{
    if (s.version() >= QDataStream::Qt_4_3) {
        if (icon.isNull()) {
            s << QString();
        } else {
            s << icon.d->engine->key();
            icon.d->engine->write(s);
        }
    } else if (s.version() == QDataStream::Qt_4_2) {
        if (icon.isNull()) {
            s << 0;
        } else {
            QPixmapIconEngine *engine = static_cast<QPixmapIconEngine *>(icon.d->engine);
            int num_entries = engine->pixmaps.size();
            s << num_entries;
            for (int i=0; i < num_entries; ++i) {
                s << engine->pixmaps.at(i).pixmap;
                s << engine->pixmaps.at(i).fileName;
                s << engine->pixmaps.at(i).size;
                s << (uint) engine->pixmaps.at(i).mode;
                s << (uint) engine->pixmaps.at(i).state;
            }
        }
    } else {
        s << QPixmap(icon.pixmap(22,22));
    }
    return s;
}

/*!
    \fn QDataStream &operator>>(QDataStream &stream, QIcon &icon)
    \relates QIcon
    \since 4.2

    Reads an image, or a set of images, from the given \a stream into
    the given \a icon.
*/

QDataStream &operator>>(QDataStream &s, QIcon &icon)
{
    if (s.version() >= QDataStream::Qt_4_3) {
        icon = QIcon();
        QString key;
        s >> key;
        if (key == QLatin1String("QPixmapIconEngine")) {
            icon.d = new QIconPrivate(new QPixmapIconEngine);
            icon.d->engine->read(s);
        } else if (key == QLatin1String("QIconLoaderEngine")) {
            icon.d = new QIconPrivate(new QIconLoaderEngine());
            icon.d->engine->read(s);
        } else {
            const int index = loader()->indexOf(key);
            if (index != -1) {
                if (QIconEnginePlugin *factory = qobject_cast<QIconEnginePlugin*>(loader()->instance(index))) {
                    if (QIconEngine *engine= factory->create()) {
                        icon.d = new QIconPrivate(engine);
                        engine->read(s);
                    } // factory
                } // instance
            } // index
        }
    } else if (s.version() == QDataStream::Qt_4_2) {
        icon = QIcon();
        int num_entries;
        QPixmap pm;
        QString fileName;
        QSize sz;
        uint mode;
        uint state;

        s >> num_entries;
        for (int i=0; i < num_entries; ++i) {
            s >> pm;
            s >> fileName;
            s >> sz;
            s >> mode;
            s >> state;
            if (pm.isNull())
                icon.addFile(fileName, sz, QIcon::Mode(mode), QIcon::State(state));
            else
                icon.addPixmap(pm, QIcon::Mode(mode), QIcon::State(state));
        }
    } else {
        QPixmap pm;
        s >> pm;
        icon.addPixmap(pm);
    }
    return s;
}

#endif //QT_NO_DATASTREAM

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QIcon &i)
{
    QDebugStateSaver saver(dbg);
    dbg.resetFormat();
    dbg.nospace();
    dbg << "QIcon(";
    if (i.isNull()) {
        dbg << "null";
    } else {
        if (!i.name().isEmpty())
            dbg << i.name() << ',';
        dbg << "availableSizes[normal,Off]=" << i.availableSizes()
            << ",cacheKey=" << Qt::showbase << Qt::hex << i.cacheKey() << Qt::dec << Qt::noshowbase;
    }
    dbg << ')';
    return dbg;
}
#endif

/*!
    \fn DataPtr &QIcon::data_ptr()
    \internal
*/

/*!
    \typedef QIcon::DataPtr
    \internal
*/

/*!
    \internal
    \since 5.6
    Attempts to find a suitable @Nx file for the given \a targetDevicePixelRatio
    Returns the \a baseFileName if no such file was found.

    Given base foo.png and a target dpr of 2.5, this function will look for
    foo@3x.png, then foo@2x, then fall back to foo.png if not found.

    \a sourceDevicePixelRatio will be set to the value of N if the argument is
    not \nullptr
*/
QString qt_findAtNxFile(const QString &baseFileName, qreal targetDevicePixelRatio,
                        qreal *sourceDevicePixelRatio)
{
    if (targetDevicePixelRatio <= 1.0)
        return baseFileName;

    static bool disableNxImageLoading = !qEnvironmentVariableIsEmpty("QT_HIGHDPI_DISABLE_2X_IMAGE_LOADING");
    if (disableNxImageLoading)
        return baseFileName;

    int dotIndex = baseFileName.lastIndexOf(QLatin1Char('.'));
    if (dotIndex == -1) { /* no dot */
        dotIndex = baseFileName.size(); /* append */
    } else if (dotIndex >= 2 && baseFileName[dotIndex - 1] == QLatin1Char('9')
        && baseFileName[dotIndex - 2] == QLatin1Char('.')) {
        // If the file has a .9.* (9-patch image) extension, we must ensure that the @nx goes before it.
        dotIndex -= 2;
    }

    QString atNxfileName = baseFileName;
    atNxfileName.insert(dotIndex, QLatin1String("@2x"));
    // Check for @Nx, ..., @3x, @2x file versions,
    for (int n = qMin(qCeil(targetDevicePixelRatio), 9); n > 1; --n) {
        atNxfileName[dotIndex + 1] = QLatin1Char('0' + n);
        if (QFile::exists(atNxfileName)) {
            if (sourceDevicePixelRatio)
                *sourceDevicePixelRatio = n;
            return atNxfileName;
        }
    }

    return baseFileName;
}

QT_END_NAMESPACE
#endif //QT_NO_ICON
