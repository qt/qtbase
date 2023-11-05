// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

using namespace Qt::StringLiterals;

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
    Q_CONSTINIT static QBasicAtomicInt serial = Q_BASIC_ATOMIC_INITIALIZER(0);
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

QIconPrivate::QIconPrivate(QIconEngine *e)
    : engine(e), ref(1),
      serialNum(nextSerialNumCounter()),
    detach_no(0),
    is_mask(false)
{
}

void QIconPrivate::clearIconCache()
{
    qt_cleanup_icon_cache();
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
    auto paintDevice = painter->device();
    qreal dpr = paintDevice ? paintDevice->devicePixelRatio() : qApp->devicePixelRatio();
    const QSize pixmapSize = rect.size() * dpr;
    QPixmap px = scaledPixmap(pixmapSize, mode, state, dpr);
    painter->drawPixmap(rect, px);
}

static inline int area(const QSize &s) { return s.width() * s.height(); }

// Returns the smallest of the two that is still larger than or equal to size.
// Pixmaps at the correct scale are preferred, pixmaps at lower scale are
// used as fallbacks. We assume that the pixmap set is complete, in the sense
// that no 2x pixmap is going to be a better match than a 3x pixmap for the the
// target scale of 3 (It's OK if 3x pixmaps are missing - we'll fall back to
// the 2x pixmaps then.)
static QPixmapIconEngineEntry *bestSizeScaleMatch(const QSize &size, qreal scale, QPixmapIconEngineEntry *pa, QPixmapIconEngineEntry *pb)
{

    // scale: we can only differentiate on scale if the scale differs
    if (pa->scale != pb->scale) {

        // Score the pixmaps: 0 is an exact scale match, positive
        // scores have more detail than requested, negative scores
        // have less detail than requested.
        qreal ascore = pa->scale - scale;
        qreal bscore = pb->scale - scale;

        // Take the one closest to 0
        return (qAbs(ascore) < qAbs(bscore)) ? pa : pb;
    }

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

QPixmapIconEngineEntry *QPixmapIconEngine::tryMatch(const QSize &size, qreal scale, QIcon::Mode mode, QIcon::State state)
{
    QPixmapIconEngineEntry *pe = nullptr;
    for (int i = 0; i < pixmaps.size(); ++i)
        if (pixmaps.at(i).mode == mode && pixmaps.at(i).state == state) {
            if (pe)
                pe = bestSizeScaleMatch(size, scale, &pixmaps[i], pe);
            else
                pe = &pixmaps[i];
        }
    return pe;
}


QPixmapIconEngineEntry *QPixmapIconEngine::bestMatch(const QSize &size, qreal scale, QIcon::Mode mode, QIcon::State state, bool sizeOnly)
{
    QPixmapIconEngineEntry *pe = tryMatch(size, scale, mode, state);
    while (!pe){
        QIcon::State oppositeState = (state == QIcon::On) ? QIcon::Off : QIcon::On;
        if (mode == QIcon::Disabled || mode == QIcon::Selected) {
            QIcon::Mode oppositeMode = (mode == QIcon::Disabled) ? QIcon::Selected : QIcon::Disabled;
            if ((pe = tryMatch(size, scale, QIcon::Normal, state)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Active, state)))
                break;
            if ((pe = tryMatch(size, scale, mode, oppositeState)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Normal, oppositeState)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Active, oppositeState)))
                break;
            if ((pe = tryMatch(size, scale, oppositeMode, state)))
                break;
            if ((pe = tryMatch(size, scale, oppositeMode, oppositeState)))
                break;
        } else {
            QIcon::Mode oppositeMode = (mode == QIcon::Normal) ? QIcon::Active : QIcon::Normal;
            if ((pe = tryMatch(size, scale, oppositeMode, state)))
                break;
            if ((pe = tryMatch(size, scale, mode, oppositeState)))
                break;
            if ((pe = tryMatch(size, scale, oppositeMode, oppositeState)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Disabled, state)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Selected, state)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Disabled, oppositeState)))
                break;
            if ((pe = tryMatch(size, scale, QIcon::Selected, oppositeState)))
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
    return scaledPixmap(size, mode, state, 1.0);
}

QPixmap QPixmapIconEngine::scaledPixmap(const QSize &size, QIcon::Mode mode, QIcon::State state, qreal scale)
{

    QPixmap pm;
    QPixmapIconEngineEntry *pe = bestMatch(size, scale, mode, state, false);
    if (pe)
        pm = pe->pixmap;

    if (pm.isNull()) {
        int idx = pixmaps.size();
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

    QString key = "qt_"_L1
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

    // The returned actual size is the size in device independent pixels,
    // so we limit the search to scale 1 and assume that e.g. @2x versions
    // does not proviode extra actual sizes not also provided by the 1x versions.
    qreal scale = 1;

    if (QPixmapIconEngineEntry *pe = bestMatch(size, scale, mode, state, true))
        actualSize = pe->size;

    if (actualSize.isNull())
        return actualSize;

    if (!actualSize.isNull() && (actualSize.width() > size.width() || actualSize.height() > size.height()))
        actualSize.scale(size, Qt::KeepAspectRatio);
    return actualSize;
}

QList<QSize> QPixmapIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state)
{
    QList<QSize> sizes;
    for (int i = 0; i < pixmaps.size(); ++i) {
        QPixmapIconEngineEntry &pe = pixmaps[i];
        if (pe.size == QSize() && pe.pixmap.isNull()) {
            pe.pixmap = QPixmap(pe.fileName);
            pe.size = pe.pixmap.size();
        }
        if (pe.mode == mode && pe.state == state && !pe.size.isEmpty())
            sizes.push_back(pe.size);
     }
    return sizes;
}

void QPixmapIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    if (!pixmap.isNull()) {
        QPixmapIconEngineEntry *pe = tryMatch(pixmap.size(), pixmap.devicePixelRatio(), mode, state);
        if (pe && pe->size == pixmap.size() && pe->scale == pixmap.devicePixelRatio()) {
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

static inline int findBySize(const QList<QImage> &images, const QSize &size)
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
    const QString abs = fileName.startsWith(u':') ? fileName : QFileInfo(fileName).absoluteFilePath();
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
    QList<QImage> icoImages;
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
    for (const QImage &i : std::as_const(icoImages))
        pixmaps += QPixmapIconEngineEntry(abs, i, mode, state);
    if (icoImages.isEmpty() && !ignoreSize) // Add placeholder with the filename and empty pixmap for the size.
        pixmaps += QPixmapIconEngineEntry(abs, size, mode, state);
}

bool QPixmapIconEngine::isNull()
{
    return pixmaps.isEmpty();
}

QString QPixmapIconEngine::key() const
{
    return "QPixmapIconEngine"_L1;
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

Q_GLOBAL_STATIC_WITH_ARGS(QFactoryLoader, iceLoader,
    (QIconEngineFactoryInterface_iid, "/iconengines"_L1, Qt::CaseInsensitive))

QFactoryLoader *qt_iconEngineFactoryLoader()
{
    return iceLoader();
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

  There are two ways that QIcon supports \l {High DPI}{high DPI}
  icons: via \l addFile() and \l fromTheme().

  \l addFile() is useful if you have your own custom directory structure and do
  not need to use the \l {Freedesktop Icon Theme Specification}. Icons
  created via this approach use Qt's \l {High Resolution Versions of Images}
  {"@nx" high DPI syntax}.

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
    return QVariant::fromValue(*this);
}

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
  requested, but never larger, unless the device-pixel ratio of the returned
  pixmap is larger than 1.

  \sa actualSize(), paint()
*/
QPixmap QIcon::pixmap(const QSize &size, Mode mode, State state) const
{
    if (!d)
        return QPixmap();
    const qreal dpr = -1; // don't know target dpr
    return pixmap(size, dpr, mode, state);
}

/*!
    \fn QPixmap QIcon::pixmap(int w, int h, Mode mode = Normal, State state = Off) const

    \overload

    Returns a pixmap of size QSize(\a w, \a h). The pixmap might be smaller than
    requested, but never larger, unless the device-pixel ratio of the returned
    pixmap is larger than 1.
*/

/*!
    \fn QPixmap QIcon::pixmap(int extent, Mode mode = Normal, State state = Off) const

    \overload

    Returns a pixmap of size QSize(\a extent, \a extent). The pixmap might be smaller
    than requested, but never larger, unless the device-pixel ratio of the returned
    pixmap is larger than 1.
*/

/*!
  \overload
  \since 6.0

  Returns a pixmap with the requested \a size, \a devicePixelRatio, \a mode, and \a
  state, generating one if necessary.

  \sa  actualSize(), paint()
*/
QPixmap QIcon::pixmap(const QSize &size, qreal devicePixelRatio, Mode mode, State state) const
{
    if (!d)
        return QPixmap();

    // Use the global devicePixelRatio if the caller does not know the target dpr
    if (devicePixelRatio == -1)
        devicePixelRatio = qApp->devicePixelRatio();

    // Handle the simple normal-dpi case
    if (!(devicePixelRatio > 1.0)) {
        QPixmap pixmap = d->engine->pixmap(size, mode, state);
        pixmap.setDevicePixelRatio(1.0);
        return pixmap;
    }

    // Try get a pixmap that is big enough to be displayed at device pixel resolution.
    QPixmap pixmap = d->engine->scaledPixmap(size * devicePixelRatio, mode, state, devicePixelRatio);
    pixmap.setDevicePixelRatio(d->pixmapDevicePixelRatio(devicePixelRatio, size, pixmap.size()));
    return pixmap;
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
  \since 5.1
  \deprecated [6.0] Use pixmap(size, devicePixelRatio) instead.

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

    qreal devicePixelRatio = window ? window->devicePixelRatio() : qApp->devicePixelRatio();
    return pixmap(size, devicePixelRatio, mode, state);
}
#endif


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

    const qreal devicePixelRatio = qApp->devicePixelRatio();

    // Handle the simple normal-dpi case:
    if (!(devicePixelRatio > 1.0))
        return d->engine->actualSize(size, mode, state);

    const QSize actualSize = d->engine->actualSize(size * devicePixelRatio, mode, state);
    return actualSize / d->pixmapDevicePixelRatio(devicePixelRatio, size, actualSize);
}

#if QT_DEPRECATED_SINCE(6, 0)
/*!
  \since 5.1
  \deprecated [6.0] Use actualSize(size) instead.

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

    qreal devicePixelRatio = window ? window->devicePixelRatio() : qApp->devicePixelRatio();

    // Handle the simple normal-dpi case:
    if (!(devicePixelRatio > 1.0))
        return d->engine->actualSize(size, mode, state);

    QSize actualSize = d->engine->actualSize(size * devicePixelRatio, mode, state);
    return actualSize / d->pixmapDevicePixelRatio(devicePixelRatio, size, actualSize);
}
#endif

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
        const int index = iceLoader()->indexOf(suffix);
        if (index != -1) {
            if (QIconEnginePlugin *factory = qobject_cast<QIconEnginePlugin*>(iceLoader()->instance(index))) {
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
    name. This is the case for icons created with fromTheme().

    \sa fromTheme(), QIconEngine::iconName()
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

    The content of \a paths should follow the theme format
    documented by setThemeName().

    \sa themeSearchPaths(), fromTheme(), setThemeName()
*/
void QIcon::setThemeSearchPaths(const QStringList &paths)
{
    QIconLoader::instance()->setThemeSearchPath(paths);
}

/*!
    \since 4.6

    Returns the search paths for icon themes.

    The default search paths will be defined by the platform.
    All platforms will also have the resource directory \c{:\icons} as a fallback.

    \sa setThemeSearchPaths(), fromTheme(), setThemeName()
*/
QStringList QIcon::themeSearchPaths()
{
    return QIconLoader::instance()->themeSearchPaths();
}

/*!
    \since 5.11

    Returns the fallback search paths for icons.

    The fallback search paths are consulted for standalone
    icon files if the \l{themeName()}{current icon theme}
    or \l{fallbackThemeName()}{fallback icon theme} do
    not provide results for an icon lookup.

    If not set, the fallback search paths will be defined
    by the platform.

    \sa setFallbackSearchPaths(), themeSearchPaths()
*/
QStringList QIcon::fallbackSearchPaths()
{
    return QIconLoader::instance()->fallbackSearchPaths();
}

/*!
    \since 5.11

    Sets the fallback search paths for icons to \a paths.

    The fallback search paths are consulted for standalone
    icon files if the \l{themeName()}{current icon theme}
    or \l{fallbackThemeName()}{fallback icon theme} do
    not provide results for an icon lookup.

    For example:

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

    The theme will be will be looked up in themeSearchPaths().

    At the moment the only supported icon theme format is the
    \l{Freedesktop Icon Theme Specification}. The \a name should
    correspond to a directory name in the themeSearchPath()
    containing an \c index.theme file describing its contents.

    \sa themeSearchPaths(), themeName(),
    {Freedesktop Icon Theme Specification}
*/
void QIcon::setThemeName(const QString &name)
{
    QIconLoader::instance()->setThemeName(name);
}

/*!
    \since 4.6

    Returns the name of the current icon theme.

    If not set, the current icon theme will be defined by the
    platform.

    \note Platform icon themes are only implemented on
    \l{Freedesktop} based systems at the moment, and the
    icon theme depends on your desktop settings.

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

    If not set, the fallback icon theme will be defined by the
    platform.

    \note Platform fallback icon themes are only implemented on
    \l{Freedesktop} based systems at the moment, and the
    icon theme depends on your desktop settings.

    \sa setFallbackThemeName(), themeName()
*/
QString QIcon::fallbackThemeName()
{
    return QIconLoader::instance()->fallbackThemeName();
}

/*!
    \since 5.12

    Sets the fallback icon theme to \a name.

    The fallback icon theme is consulted for icons not provided by
    the \l{themeName()}{current icon theme}, or if the \l{themeName()}
    {current icon theme} does not exist.

    The \a name should correspond to theme in the same format
    as documented by setThemeName(), and will be looked up
    in themeSearchPaths().

    \note Fallback icon themes should be set before creating
    QGuiApplication, to ensure correct initialization.

    \sa fallbackThemeName(), themeSearchPaths(), themeName()
*/
void QIcon::setFallbackThemeName(const QString &name)
{
    QIconLoader::instance()->setFallbackThemeName(name);
}

/*!
    \since 4.6

    Returns the QIcon corresponding to \a name in the
    \l{themeName()}{current icon theme}.

    If the current theme does not provide an icon for \a name,
    the \l{fallbackThemeName()}{fallback icon theme} is consulted,
    before falling back to looking up standalone icon files in the
    \l{QIcon::fallbackSearchPaths()}{fallback icon search path}.

    To fetch an icon from the current icon theme:

    \snippet code/src_gui_image_qicon.cpp 3

    If an \l{themeName()}{icon theme} has not been explicitly
    set via setThemeName() a platform defined icon theme will
    be used.

    \note Platform icon themes is only implemented on
    \l{Freedesktop} based systems at the moment,
    following the \l{Freedesktop Icon Naming Specification}.
    In order to use themed icons on other platforms, you will have
    to bundle a \l{setThemeName()}{compliant theme} in one of your
    themeSearchPaths(), and set the appropriate themeName().

    \sa themeName(), fallbackThemeName(), setThemeName(), themeSearchPaths(), fallbackSearchPaths(),
        {Freedesktop Icon Naming Specification}
*/
QIcon QIcon::fromTheme(const QString &name)
{

    if (QIcon *cachedIcon = qtIconCache()->object(name))
        return *cachedIcon;

    if (QDir::isAbsolutePath(name))
        return QIcon(name);

    QIcon icon(new QThemeIconEngine(name));
    qtIconCache()->insert(name, new QIcon(icon));
    return icon;
}

/*!
    \overload

    Returns the QIcon corresponding to \a name in the
    \l{themeName()}{current icon theme}.

    If the current theme does not provide an icon for \a name,
    the \l{fallbackThemeName()}{fallback icon theme} is consulted,
    before falling back to looking up standalone icon files in the
    \l{QIcon::fallbackSearchPaths()}{fallback icon search path}.

    If no icon is found \a fallback is returned.

    This is useful to provide a guaranteed fallback, regardless of
    whether the current set of icon themes and fallbacks paths
    support the requested icon.

    For example:

    \snippet code/src_gui_image_qicon.cpp 4

    \sa fallbackThemeName(), fallbackSearchPaths()
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
    current icon theme or any of the fallbacks, as described by
    fromTheme(), otherwise returns \c false.

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
    detach();
    if (!d)
        d = new QIconPrivate(new QPixmapIconEngine);
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
        if (key == "QPixmapIconEngine"_L1) {
            icon.d = new QIconPrivate(new QPixmapIconEngine);
            icon.d->engine->read(s);
        } else if (key == "QIconLoaderEngine"_L1 || key == "QThemeIconEngine"_L1) {
            icon.d = new QIconPrivate(new QThemeIconEngine);
            icon.d->engine->read(s);
        } else {
            const int index = iceLoader()->indexOf(key);
            if (index != -1) {
                if (QIconEnginePlugin *factory = qobject_cast<QIconEnginePlugin*>(iceLoader()->instance(index))) {
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

    int dotIndex = baseFileName.lastIndexOf(u'.');
    if (dotIndex == -1) { /* no dot */
        dotIndex = baseFileName.size(); /* append */
    } else if (dotIndex >= 2 && baseFileName[dotIndex - 1] == u'9'
        && baseFileName[dotIndex - 2] == u'.') {
        // If the file has a .9.* (9-patch image) extension, we must ensure that the @nx goes before it.
        dotIndex -= 2;
    }

    QString atNxfileName = baseFileName;
    atNxfileName.insert(dotIndex, "@2x"_L1);
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
