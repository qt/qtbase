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
#include "private/qoffsetstringarray_p.h"
#include "qpa/qplatformtheme.h"

#ifndef QT_NO_ICON
QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
// Convenience class providing a bool read() function.
namespace {
class ImageReader
{
public:
    ImageReader(const QString &fileName, QSize size)
        : m_reader(fileName)
        , m_atEnd(false)
    {
        if (m_reader.supportsOption(QImageIOHandler::ScaledSize))
            m_reader.setScaledSize(size);
    }

    QByteArray format() const { return m_reader.format(); }
    bool supportsReadSize() const { return m_reader.supportsOption(QImageIOHandler::Size); }
    QSize size() const { return m_reader.size(); }
    bool jumpToNextImage() { return m_reader.jumpToNextImage(); }
    void jumpToImage(int index) { m_reader.jumpToImage(index); }

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

  \value On  Display the pixmap when the widget is in an "on" state
  \value Off  Display the pixmap when the widget is in an "off" state
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

    If displayDevicePixelRatio is 1.0 the returned value is 1.0, always.

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
                         qreal(actualSize.height()) / qreal(targetSize.height()));
    qreal dpr = qMax(qreal(1.0), displayDevicePixelRatio * scale);
    return qRound(dpr * 100) / 100.0;
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
    QPixmap px = scaledPixmap(rect.size(), mode, state, dpr);
    painter->drawPixmap(rect, px);
}

static inline qint64 area(const QSize &s) { return qint64(s.width()) * s.height(); }

// Returns the smallest of the two that is still larger than or equal to size.
// Pixmaps at the correct scale are preferred, pixmaps at lower scale are
// used as fallbacks. We assume that the pixmap set is complete, in the sense
// that no 2x pixmap is going to be a better match than a 3x pixmap for the the
// target scale of 3 (It's OK if 3x pixmaps are missing - we'll fall back to
// the 2x pixmaps then.)
static QPixmapIconEngineEntry *bestSizeScaleMatch(const QSize &size, qreal scale, QPixmapIconEngineEntry *pa, QPixmapIconEngineEntry *pb)
{
    const auto scaleA = pa->pixmap.devicePixelRatio();
    const auto scaleB = pb->pixmap.devicePixelRatio();
    // scale: we can only differentiate on scale if the scale differs
    if (scaleA != scaleB) {

        // Score the pixmaps: 0 is an exact scale match, positive
        // scores have more detail than requested, negative scores
        // have less detail than requested.
        qreal ascore = scaleA - scale;
        qreal bscore = scaleB - scale;

        // always prefer positive scores to prevent upscaling
        if ((ascore < 0) != (bscore < 0))
            return bscore < 0 ? pa : pb;
        // Take the one closest to 0
        return (qAbs(ascore) < qAbs(bscore)) ? pa : pb;
    }

    qint64 s = area(size * scale);
    if (pa->size == QSize() && pa->pixmap.isNull()) {
        pa->pixmap = QPixmap(pa->fileName);
        pa->size = pa->pixmap.size();
    }
    qint64 a = area(pa->size);
    if (pb->size == QSize() && pb->pixmap.isNull()) {
        pb->pixmap = QPixmap(pb->fileName);
        pb->size = pb->pixmap.size();
    }
    qint64 b = area(pb->size);
    qint64 res = a;
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
    for (auto &entry : pixmaps) {
        if (entry.mode == mode && entry.state == state) {
            if (pe)
                pe = bestSizeScaleMatch(size, scale, &entry, pe);
            else
                pe = &entry;
        }
    }
    return pe;
}


QPixmapIconEngineEntry *QPixmapIconEngine::bestMatch(const QSize &size, qreal scale, QIcon::Mode mode, QIcon::State state)
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

    if (pe->pixmap.isNull()) {
        // delay-load the image
        QImage image, prevImage;
        const QSize realSize = size * scale;
        ImageReader imageReader(pe->fileName, realSize);
        bool fittingImageFound = false;
        if (imageReader.supportsReadSize()) {
            // find the image with the best size without loading the entire image
            do {
                fittingImageFound = imageReader.size() == realSize;
            } while (!fittingImageFound && imageReader.jumpToNextImage());
        }
        if (!fittingImageFound) {
            imageReader.jumpToImage(0);
            while (imageReader.read(&image) && image.size() != realSize)
                prevImage = image;
            if (image.isNull())
                image = prevImage;
        } else {
            imageReader.read(&image);
        }
        if (!image.isNull()) {
            pe->pixmap.convertFromImage(image);
            if (!pe->pixmap.isNull()) {
                pe->size = pe->pixmap.size();
                pe->pixmap.setDevicePixelRatio(scale);
            }
        }
        if (!pe->size.isValid()) {
            removePixmapEntry(pe);
            pe = nullptr;
        }
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
    QPixmapIconEngineEntry *pe = bestMatch(size, scale, mode, state);
    if (pe)
        pm = pe->pixmap;
    else
        return pm;

    if (pm.isNull()) {
        removePixmapEntry(pe);
        if (pixmaps.isEmpty())
            return pm;
        return scaledPixmap(size, mode, state, scale);
    }

    const auto actualSize = adjustSize(size * scale, pm.size());
    const auto calculatedDpr = QIconPrivate::pixmapDevicePixelRatio(scale, size, actualSize);
    QString key = "qt_"_L1
                  % HexString<quint64>(pm.cacheKey())
                  % HexString<quint8>(pe->mode)
                  % HexString<quint64>(QGuiApplication::palette().cacheKey())
                  % HexString<uint>(actualSize.width())
                  % HexString<uint>(actualSize.height())
                  % HexString<quint16>(qRound(calculatedDpr * 1000));

    if (mode == QIcon::Active) {
        if (QPixmapCache::find(key % HexString<quint8>(mode), &pm))
            return pm; // horray
        if (QPixmapCache::find(key % HexString<quint8>(QIcon::Normal), &pm)) {
            QPixmap active = pm;
            if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
                active = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(QIcon::Active, pm);
            if (pm.cacheKey() == active.cacheKey())
                return pm;
        }
    }

    if (!QPixmapCache::find(key % HexString<quint8>(mode), &pm)) {
        if (pm.size() != actualSize)
            pm = pm.scaled(actualSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        if (pe->mode != mode && mode != QIcon::Normal) {
            QPixmap generated = pm;
            if (QGuiApplication *guiApp = qobject_cast<QGuiApplication *>(qApp))
                generated = static_cast<QGuiApplicationPrivate*>(QObjectPrivate::get(guiApp))->applyQIconStyleHelper(mode, pm);
            if (!generated.isNull())
                pm = generated;
        }
        pm.setDevicePixelRatio(calculatedDpr);
        QPixmapCache::insert(key % HexString<quint8>(mode), pm);
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

    if (QPixmapIconEngineEntry *pe = bestMatch(size, scale, mode, state))
        actualSize = pe->size;

    return adjustSize(size, actualSize);
}

QList<QSize> QPixmapIconEngine::availableSizes(QIcon::Mode mode, QIcon::State state)
{
    QList<QSize> sizes;
    for (QPixmapIconEngineEntry &pe : pixmaps) {
        if (pe.mode != mode || pe.state != state)
            continue;
        if (pe.size.isEmpty() && pe.pixmap.isNull()) {
            pe.pixmap = QPixmap(pe.fileName);
            pe.size = pe.pixmap.size();
        }
        if (!pe.size.isEmpty() && !sizes.contains(pe.size))
            sizes.push_back(pe.size);
    }
    return sizes;
}

void QPixmapIconEngine::addPixmap(const QPixmap &pixmap, QIcon::Mode mode, QIcon::State state)
{
    if (!pixmap.isNull()) {
        QPixmapIconEngineEntry *pe = tryMatch(pixmap.size() / pixmap.devicePixelRatio(),
                                              pixmap.devicePixelRatio(), mode, state);
        if (pe && pe->size == pixmap.size() && pe->pixmap.devicePixelRatio() == pixmap.devicePixelRatio()) {
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
    for (qsizetype i = 0; i < images.size(); ++i) {
        if (images.at(i).size() == size)
            return i;
    }
    return -1;
}

void QPixmapIconEngine::addFile(const QString &fileName, const QSize &size, QIcon::Mode mode, QIcon::State state)
{
    if (fileName.isEmpty())
        return;
    const QString abs = fileName.startsWith(u':') ? fileName : QFileInfo(fileName).absoluteFilePath();
    const bool ignoreSize = !size.isValid();
    ImageReader imageReader(abs, size);
    const QByteArray format = imageReader.format();
    if (format.isEmpty()) // Device failed to open or unsupported format.
        return;
    QImage image;
    if (format != "ico") {
        if (ignoreSize) { // No size specified: Add all images.
            if (imageReader.supportsReadSize()) {
                do {
                    pixmaps += QPixmapIconEngineEntry(abs, imageReader.size(), mode, state);
                } while (imageReader.jumpToNextImage());
            } else {
                while (imageReader.read(&image))
                    pixmaps += QPixmapIconEngineEntry(abs, image, mode, state);
            }
        } else {
            pixmaps += QPixmapIconEngineEntry(abs, size, mode, state);
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
  UI components to show an icon representing a particular action.

  \section1 Creating an icon from image files

  The simplest way to construct a QIcon is to create one from one or
  several image files or resources. For example:

  \snippet code/src_gui_image_qicon.cpp 0

  QIcon can store several images for different states, and Qt will
  select the image that is the closest match for the action's current
  state.

  \snippet code/src_gui_image_qicon.cpp addFile

  Qt will generate the required icon styles and sizes when needed,
  e.g. the pixmap for the QIcon::Disabled state might be generated by
  graying out one of the provided pixmaps.

  To clear the icon, simply set a null icon in its place:

  \snippet code/src_gui_image_qicon.cpp 1

  Use the QImageReader::supportedImageFormats() and
  QImageWriter::supportedImageFormats() functions to retrieve a
  complete list of the supported file formats.

  \note If using an SVG image file, make sure to add it before any non-SVG files,
  so that the correct \l{Icon Engines}{icon engine} gets selected.

  \section1 Creating an icon from a theme or icon library

  The most convenient way to construct an icon is by using the
  \l{QIcon::}{fromTheme()} factory function. Qt implements access to
  the native icon library on platforms that support the
  \l {Freedesktop Icon Theme Specification}.

  Applications can use the same theming specification to provide
  their own icon library. See below for an example theme description
  and the corresponding directory structure for the image files.

  Since Qt 6.7, Qt also provides access to the native icon library on macOS,
  iOS, and Windows 10 and 11. On Android, Qt can access icons from the Material
  design system as long as the
  \l{https://github.com/google/material-design-icons/tree/master/font}
  {MaterialIcons-Regular} font is available on the system, or bundled
  as a resource at \c{:/qt-project.org/icons/MaterialIcons-Regular.ttf}
  with the application.

  \snippet code/src_gui_image_qicon.cpp fromTheme

  Since Qt 6.9, Qt can generate icons from named glyphs in an available icon
  font. Set the \l{QIcon::themeName()}{theme name} to the family name of the
  font, and use \l{QIcon::}{fromTheme()} with the name of the glyph.

  \snippet code/src_gui_image_qicon.cpp iconFont

  The icon font can be installed on the system, or bundled as an
  \l{QFontDatabase::addApplicationFont()}{application font}.

  \section1 Icon Engines

  Internally, QIcon instantiates an \l {QIconEngine} {icon engine}
  backend to handle and render the icon images. The type of icon
  engine is determined by the first file or pixmap or theme added to a
  QIcon object. Additional files or pixmaps will then be handled by
  the same engine.

  Icon engines differ in the way they handle and render icons. The
  default pixmap-based engine only deals with fixed images, while the
  QtSvg module provides an icon engine that can re-render the provided
  vector graphics files at the requested size for better quality. The
  theme icon engines will typically only provide images from native
  platform icon library, and ignore any added files or pixmaps.

  In addition, it is possible to provide custom icon engines. This
  allows applications to customize every aspect of generated
  icons. With QIconEnginePlugin it is possible to register different
  icon engines for different file suffixes, making it possible for
  third parties to provide additional icon engines to those included
  with Qt.

  \section1 Using QIcon in the User Interface

  If you write your own widgets that have an option to set a small
  pixmap, consider allowing a QIcon to be set for that pixmap.  The
  Qt class QToolButton is an example of such a widget.

  Provide a method to set a QIcon, and paint the QIcon with
  \l{QIcon::}{paint}, choosing the appropriate parameters based
  on the current state of your widget. For example:

  \snippet code/src_gui_image_qicon.cpp 2

  When you retrieve a pixmap using pixmap(QSize, Mode, State), and no
  pixmap for this given size, mode and state has been added with
  addFile() or addPixmap(), then QIcon will generate one on the
  fly. This pixmap generation happens in a QIconEngine. The default
  engine scales pixmaps down if required, but never up, and it uses
  the current style to calculate a disabled appearance.

  You might also make use of the \c Active mode, perhaps making your
  widget \c Active when the mouse is over the widget (see \l
  QWidget::enterEvent()), while the mouse is pressed pending the
  release that will activate the function, or when it is the currently
  selected item. If the widget can be toggled, the "On" mode might be
  used to draw a different icon.

  QIcons generated from the native icon library, or from an icon font, use the
  same glyph for both the \c On and \c Off states of the icon. Applications can
  change the icon depending on the state of the respective UI control or action.
  In a Qt Quick application, this can be done with a binding.

  \snippet code/src_gui_image_qicon.qml iconFont

  \image icon.png QIcon

  \note QIcon needs a QGuiApplication instance before the icon is created.

  \section1 High DPI Icons

  Icons that are provided by the native icon library, or generated from the
  glyph in an icon font, are usually based on vector graphics, and will
  automatically be rendered in the appropriate resolution.

  When providing your own image files via \l addFile(), then QIcon will
  use Qt's \l {High Resolution Versions of Images}{"@nx" high DPI syntax}.
  This is useful if you have your own custom directory structure and do not
  use follow \l {Freedesktop Icon Theme Specification}.

  When providing an application theme, then you need to follow the Icon Theme
  Specification to specify which files to use for different resolutions.
  To make QIcon use the high DPI version of an image, add an additional entry
  to the appropriate \c index.theme file:

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
    \memberswap{icon}
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
  state, generating one with the given \a mode and \a state if necessary. The pixmap
  might be smaller than requested, but never larger, unless the device-pixel ratio
  of the returned pixmap is larger than 1.

  \note The requested devicePixelRatio might not match the returned one. This delays the
  scaling of the QPixmap until it is drawn later on.
  \note Prior to Qt 6.8 this function wronlgy passed the device dependent pixmap size to
  QIconEngine::scaledPixmap(), since Qt 6.8 it's the device independent size (not scaled
  with the \a devicePixelRatio).

  \sa  actualSize(), paint()
*/
QPixmap QIcon::pixmap(const QSize &size, qreal devicePixelRatio, Mode mode, State state) const
{
    if (!d)
        return QPixmap();

    // Use the global devicePixelRatio if the caller does not know the target dpr
    if (devicePixelRatio == -1)
        devicePixelRatio = qApp->devicePixelRatio();

    QPixmap pixmap = d->engine->scaledPixmap(size, mode, state, devicePixelRatio);
    // even though scaledPixmap() should return a pixmap with an appropriate size, we
    // can not rely on it. Therefore we simply set the dpr to a correct size to make
    // sure the pixmap is not larger than requested.
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
    bool alreadyAdded = false;
    if (!d) {

        QFileInfo info(fileName);
        QString suffix = info.suffix();
#if QT_CONFIG(mimetype)
        if (suffix.isEmpty())
            suffix = QMimeDatabase().mimeTypeForFile(info).preferredSuffix(); // determination from contents
#endif // mimetype
        QIconEngine *engine = iconEngineFromSuffix(fileName, suffix);
        if (engine)
            alreadyAdded = !engine->isNull();
        d = new QIconPrivate(engine ? engine : new QPixmapIconEngine);
    }
    if (!alreadyAdded)
        d->engine->addFile(fileName, size, mode, state);

    if (d->engine->key() == "svg"_L1)   // not needed and also not supported
        return;

    // Check if a "@Nx" file exists and add it.
    QVarLengthArray<int, 4> devicePixelRatios;
    const auto screens = qApp->screens();
    for (const auto *screen : screens) {
        const auto dpr = qCeil(screen->devicePixelRatio()); // qt_findAtNxFile only supports integer values
        if (dpr >= 1 && !devicePixelRatios.contains(dpr))
            devicePixelRatios.push_back(dpr);
    }
    std::sort(devicePixelRatios.begin(), devicePixelRatios.end(), std::greater<int>());
    qreal sourceDevicePixelRatio = std::numeric_limits<qreal>::max();
    for (const auto dpr : std::as_const(devicePixelRatios)) {
        if (dpr >= sourceDevicePixelRatio)
            continue;
        const QString atNxFileName = qt_findAtNxFile(fileName, dpr, &sourceDevicePixelRatio);
        if (atNxFileName != fileName)
            d->engine->addFile(atNxFileName, size, mode, state);
    }
}

/*!
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
    Sets the current icon theme to \a name.

    If the theme matches the name of an installed font that provides named
    glyphs, then QIcon::fromTheme calls that match one of the glyphs will
    produce an icon for that glyph.

    Otherwise, the theme will be looked up in themeSearchPaths(). At the moment
    the only supported icon theme format is the \l{Freedesktop Icon Theme
    Specification}. The \a name should correspond to a directory name in the
    themeSearchPath() containing an \c index.theme file describing its
    contents.

    \sa themeSearchPaths(), themeName(),
    {Freedesktop Icon Theme Specification}
*/
void QIcon::setThemeName(const QString &name)
{
    QIconLoader::instance()->setThemeName(name);
}

/*!
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
    Returns the QIcon corresponding to \a name in the
    \l{themeName()}{current icon theme}.

    If the current theme does not provide an icon for \a name,
    the \l{fallbackThemeName()}{fallback icon theme} is consulted,
    before falling back to looking up standalone icon files in the
    \l{QIcon::fallbackSearchPaths()}{fallback icon search path}.
    Finally, the platform's native icon library is consulted.

    To fetch an icon from the current icon theme:

    \snippet code/src_gui_image_qicon.cpp fromTheme

    If an \l{themeName()}{icon theme} has not been explicitly
    set via setThemeName() a platform defined icon theme will
    be used.

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
    Finally, the platform's native icon library is consulted.

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

static constexpr auto themeIconMapping = qOffsetStringArray(
    "address-book-new",
    "application-exit",
    "appointment-new",
    "call-start",
    "call-stop",
    "contact-new",
    "document-new",
    "document-open",
    "document-open-recent",
    "document-page-setup",
    "document-print",
    "document-print-preview",
    "document-properties",
    "document-revert",
    "document-save",
    "document-save-as",
    "document-send",
    "edit-clear",
    "edit-copy",
    "edit-cut",
    "edit-delete",
    "edit-find",
    "edit-paste",
    "edit-redo",
    "edit-select-all",
    "edit-undo",
    "folder-new",
    "format-indent-less",
    "format-indent-more",
    "format-justify-center",
    "format-justify-fill",
    "format-justify-left",
    "format-justify-right",
    "format-text-direction-ltr",
    "format-text-direction-rtl",
    "format-text-bold",
    "format-text-italic",
    "format-text-underline",
    "format-text-strikethrough",
    "go-down",
    "go-home",
    "go-next",
    "go-previous",
    "go-up",
    "help-about",
    "help-faq",
    "insert-image",
    "insert-link",
    "insert-text",
    "list-add",
    "list-remove",
    "mail-forward",
    "mail-mark-important",
    "mail-mark-read",
    "mail-mark-unread",
    "mail-message-new",
    "mail-reply-all",
    "mail-reply-sender",
    "mail-send",
    "media-eject",
    "media-playback-pause",
    "media-playback-start",
    "media-playback-stop",
    "media-record",
    "media-seek-backward",
    "media-seek-forward",
    "media-skip-backward",
    "media-skip-forward",
    "object-rotate-left",
    "object-rotate-right",
    "process-stop",
    "system-lock-screen",
    "system-log-out",
    "system-search",
    "system-reboot",
    "system-shutdown",
    "tools-check-spelling",
    "view-fullscreen",
    "view-refresh",
    "view-restore",
    "window-close",
    "window-new",
    "zoom-fit-best",
    "zoom-in",
    "zoom-out",

    "audio-card",
    "audio-input-microphone",
    "battery",
    "camera-photo",
    "camera-video",
    "camera-web",
    "computer",
    "drive-harddisk",
    "drive-optical",
    "input-gaming",
    "input-keyboard",
    "input-mouse",
    "input-tablet",
    "media-flash",
    "media-optical",
    "media-tape",
    "multimedia-player",
    "network-wired",
    "network-wireless",
    "phone",
    "printer",
    "scanner",
    "video-display",

    "appointment-missed",
    "appointment-soon",
    "audio-volume-high",
    "audio-volume-low",
    "audio-volume-medium",
    "audio-volume-muted",
    "battery-caution",
    "battery-low",
    "dialog-error",
    "dialog-information",
    "dialog-password",
    "dialog-question",
    "dialog-warning",
    "folder-drag-accept",
    "folder-open",
    "folder-visiting",
    "image-loading",
    "image-missing",
    "mail-attachment",
    "mail-unread",
    "mail-read",
    "mail-replied",
    "media-playlist-repeat",
    "media-playlist-shuffle",
    "network-offline",
    "printer-printing",
    "security-high",
    "security-low",
    "software-update-available",
    "software-update-urgent",
    "sync-error",
    "sync-synchronizing",
    "user-available",
    "user-offline",
    "weather-clear",
    "weather-clear-night",
    "weather-few-clouds",
    "weather-few-clouds-night",
    "weather-fog",
    "weather-showers",
    "weather-snow",
    "weather-storm"
);
static_assert(QIcon::ThemeIcon::NThemeIcons == QIcon::ThemeIcon(themeIconMapping.count()));

static constexpr QLatin1StringView themeIconName(QIcon::ThemeIcon icon)
{
    using ThemeIconIndex = std::underlying_type_t<QIcon::ThemeIcon>;
    const auto index = static_cast<ThemeIconIndex>(icon);
    Q_ASSERT(index < themeIconMapping.count());
    return QLatin1StringView(themeIconMapping.viewAt(index));
}

/*!
    \enum QIcon::ThemeIcon
    \since 6.7

    This enum provides access to icons that are provided by most
    icon theme implementations.

    \value AddressBookNew       The icon for the action to create a new address book.
    \value ApplicationExit      The icon for exiting an application.
    \value AppointmentNew       The icon for the action to create a new appointment.
    \value CallStart            The icon for initiating or accepting a call.
    \value CallStop             The icon for stopping a current call.
    \value ContactNew           The icon for the action to create a new contact.
    \value DocumentNew          The icon for the action to create a new document.
    \value DocumentOpen         The icon for the action to open a document.
    \value DocumentOpenRecent   The icon for the action to open a document that was recently opened.
    \value DocumentPageSetup    The icon for the \e{page setup} action.
    \value DocumentPrint        The icon for the \e{print} action.
    \value DocumentPrintPreview The icon for the \e{print preview} action.
    \value DocumentProperties   The icon for the action to view the properties of a document.
    \value DocumentRevert       The icon for the action of reverting to a previous version of a document.
    \value DocumentSave         The icon for the \e{save} action.
    \value DocumentSaveAs       The icon for the \e{save as} action.
    \value DocumentSend         The icon for the \e{send} action.
    \value EditClear            The icon for the \e{clear} action.
    \value EditCopy             The icon for the \e{copy} action.
    \value EditCut              The icon for the \e{cut} action.
    \value EditDelete           The icon for the \e{delete} action.
    \value EditFind             The icon for the \e{find} action.
    \value EditPaste            The icon for the \e{paste} action.
    \value EditRedo             The icon for the \e{redo} action.
    \value EditSelectAll        The icon for the \e{select all} action.
    \value EditUndo             The icon for the \e{undo} action.
    \value FolderNew            The icon for creating a new folder.
    \value FormatIndentLess     The icon for the \e{decrease indent formatting} action.
    \value FormatIndentMore     The icon for the \e{increase indent formatting} action.
    \value FormatJustifyCenter  The icon for the \e{center justification formatting} action.
    \value FormatJustifyFill    The icon for the \e{fill justification formatting} action.
    \value FormatJustifyLeft    The icon for the \e{left justification formatting} action.
    \value FormatJustifyRight   The icon for the \e{right justification} action.
    \value FormatTextDirectionLtr   The icon for the \e{left-to-right text formatting} action.
    \value FormatTextDirectionRtl   The icon for the \e{right-to-left formatting} action.
    \value FormatTextBold       The icon for the \e{bold text formatting} action.
    \value FormatTextItalic     The icon for the \e{italic text formatting} action.
    \value FormatTextUnderline  The icon for the \e{underlined text formatting} action.
    \value FormatTextStrikethrough  The icon for the \e{strikethrough text formatting} action.
    \value GoDown               The icon for the \e{go down in a list} action.
    \value GoHome               The icon for the \e{go to home location} action.
    \value GoNext               The icon for the \e{go to the next item in a list} action.
    \value GoPrevious           The icon for the \e{go to the previous item in a list} action.
    \value GoUp                 The icon for the \e{go up in a list} action.
    \value HelpAbout            The icon for the \e{About} item in the Help menu.
    \value HelpFaq              The icon for the \e{FAQ} item in the Help menu.
    \value InsertImage          The icon for the \e{insert image} action of an application.
    \value InsertLink           The icon for the \e{insert link} action of an application.
    \value InsertText           The icon for the \e{insert text} action of an application.
    \value ListAdd              The icon for the \e{add to list} action.
    \value ListRemove           The icon for the \e{remove from list} action.
    \value MailForward          The icon for the \e{forward} action.
    \value MailMarkImportant    The icon for the \e{mark as important} action.
    \value MailMarkRead         The icon for the \e{mark as read} action.
    \value MailMarkUnread       The icon for the \e{mark as unread} action.
    \value MailMessageNew       The icon for the \e{compose new mail} action.
    \value MailReplyAll         The icon for the \e{reply to all} action.
    \value MailReplySender      The icon for the \e{reply to sender} action.
    \value MailSend             The icon for the \e{send} action.
    \value MediaEject           The icon for the \e{eject} action of a media player or file manager.
    \value MediaPlaybackPause   The icon for the \e{pause} action of a media player.
    \value MediaPlaybackStart   The icon for the \e{start playback} action of a media player.
    \value MediaPlaybackStop    The icon for the \e{stop} action of a media player.
    \value MediaRecord          The icon for the \e{record} action of a media application.
    \value MediaSeekBackward    The icon for the \e{seek backward} action of a media player.
    \value MediaSeekForward     The icon for the \e{seek forward} action of a media player.
    \value MediaSkipBackward    The icon for the \e{skip backward} action of a media player.
    \value MediaSkipForward     The icon for the \e{skip forward} action of a media player.
    \value ObjectRotateLeft     The icon for the \e{rotate left} action performed on an object.
    \value ObjectRotateRight    The icon for the \e{rotate right} action performed on an object.
    \value ProcessStop          The icon for the \e{stop action in applications with} actions that
                                may take a while to process, such as web page loading in a browser.
    \value SystemLockScreen     The icon for the \e{lock screen} action.
    \value SystemLogOut         The icon for the \e{log out} action.
    \value SystemSearch         The icon for the \e{search} action.
    \value SystemReboot         The icon for the \e{reboot} action.
    \value SystemShutdown       The icon for the \e{shutdown} action.
    \value ToolsCheckSpelling   The icon for the \e{check spelling} action.
    \value ViewFullscreen       The icon for the \e{fullscreen} action.
    \value ViewRefresh          The icon for the \e{refresh} action.
    \value ViewRestore          The icon for leaving the fullscreen view.
    \value WindowClose          The icon for the \e{close window} action.
    \value WindowNew            The icon for the \e{new window} action.
    \value ZoomFitBest          The icon for the \e{best fit} action.
    \value ZoomIn               The icon for the \e{zoom in} action.
    \value ZoomOut              The icon for the \e{zoom out} action.

    \value AudioCard            The icon for the audio rendering device.
    \value AudioInputMicrophone The icon for the microphone audio input device.
    \value Battery              The icon for the system battery device.
    \value CameraPhoto          The icon for a digital still camera devices.
    \value CameraVideo          The icon for a video camera device.
    \value CameraWeb            The icon for a web camera device.
    \value Computer             The icon for the computing device as a whole.
    \value DriveHarddisk        The icon for hard disk drives.
    \value DriveOptical         The icon for optical media drives such as CD and DVD.
    \value InputGaming          The icon for the gaming input device.
    \value InputKeyboard        The icon for the keyboard input device.
    \value InputMouse           The icon for the mousing input device.
    \value InputTablet          The icon for graphics tablet input devices.
    \value MediaFlash           The icon for flash media, such as a memory stick.
    \value MediaOptical         The icon for physical optical media such as CD and DVD.
    \value MediaTape            The icon for generic physical tape media.
    \value MultimediaPlayer     The icon for generic multimedia playing devices.
    \value NetworkWired         The icon for wired network connections.
    \value NetworkWireless      The icon for wireless network connections.
    \value Phone                The icon for phone devices.
    \value Printer              The icon for a printer device.
    \value Scanner              The icon for a scanner device.
    \value VideoDisplay         The icon for the monitor that video gets displayed on.

    \value AppointmentMissed    The icon for when an appointment was missed.
    \value AppointmentSoon      The icon for when an appointment will occur soon.
    \value AudioVolumeHigh      The icon used to indicate high audio volume.
    \value AudioVolumeLow       The icon used to indicate low audio volume.
    \value AudioVolumeMedium    The icon used to indicate medium audio volume.
    \value AudioVolumeMuted     The icon used to indicate the muted state for audio playback.
    \value BatteryCaution       The icon used when the battery is below 40%.
    \value BatteryLow           The icon used when the battery is below 20%.
    \value DialogError          The icon used when a dialog is opened to explain an error
                                condition to the user.
    \value DialogInformation    The icon used when a dialog is opened to give information to the
                                user that may be pertinent to the requested action.
    \value DialogPassword       The icon used when a dialog requesting the authentication
                                credentials for a user is opened.
    \value DialogQuestion       The icon used when a dialog is opened to ask a simple question
                                to the user.
    \value DialogWarning        The icon used when a dialog is opened to warn the user of
                                impending issues with the requested action.
    \value FolderDragAccept     The icon used for a folder while an acceptable object is being
                                dragged onto it.
    \value FolderOpen           The icon used for folders, while their contents are being displayed
                                within the same window.
    \value FolderVisiting       The icon used for folders, while their contents are being displayed
                                in another window.
    \value ImageLoading         The icon used while another image is being loaded.
    \value ImageMissing         The icon used when another image could not be loaded.
    \value MailAttachment       The icon for a message that contains attachments.
    \value MailUnread           The icon for an unread message.
    \value MailRead             The icon for a read message.
    \value MailReplied          The icon for a message that has been replied to.
    \value MediaPlaylistRepeat  The icon for the repeat mode of a media player.
    \value MediaPlaylistShuffle The icon for the shuffle mode of a media player.
    \value NetworkOffline       The icon used to indicate that the device is not connected to the
                                network.
    \value PrinterPrinting      The icon used while a print job is successfully being spooled to a
                                printing device.
    \value SecurityHigh         The icon used to indicate that the security level of an item is
                                known to be high.
    \value SecurityLow          The icon used to indicate that the security level of an item is
                                known to be low.
    \value SoftwareUpdateAvailable  The icon used to indicate that an update is available.
    \value SoftwareUpdateUrgent The icon used to indicate that an urgent update is available.
    \value SyncError            The icon used when an error occurs while attempting to synchronize
                                data across devices.
    \value SyncSynchronizing    The icon used while data is successfully synchronizing across
                                devices.
    \value UserAvailable        The icon used to indicate that a user is available.
    \value UserOffline          The icon used to indicate that a user is not available.
    \value WeatherClear         The icon used to indicate that the sky is clear.
    \value WeatherClearNight    The icon used to indicate that the sky is clear
                                during the night.
    \value WeatherFewClouds     The icon used to indicate that the sky is partly cloudy.
    \value WeatherFewCloudsNight    The icon used to indicate that the sky is partly cloudy
                                during the night.
    \value WeatherFog           The icon used to indicate that the weather is foggy.
    \value WeatherShowers       The icon used to indicate that rain showers are occurring.
    \value WeatherSnow          The icon used to indicate that snow is falling.
    \value WeatherStorm         The icon used to indicate that the weather is stormy.

    \omitvalue NThemeIcons

    \sa {Creating an icon from a theme or icon library},
        fromTheme()
*/

/*!
    \since 6.7
    \overload

    Returns \c true if there is an icon available for \a icon in the
    current icon theme or any of the fallbacks, as described by
    fromTheme(), otherwise returns \c false.

    \sa fromTheme()
*/
bool QIcon::hasThemeIcon(QIcon::ThemeIcon icon)
{
    return hasThemeIcon(themeIconName(icon));
}

/*!
    \fn QIcon QIcon::fromTheme(QIcon::ThemeIcon icon)
    \fn QIcon QIcon::fromTheme(QIcon::ThemeIcon icon, const QIcon &fallback)
    \since 6.7
    \overload

    Returns the QIcon corresponding to \a icon in the
    \l{themeName()}{current icon theme}.

    If the current theme does not provide an icon for \a icon,
    the \l{fallbackThemeName()}{fallback icon theme} is consulted,
    before falling back to looking up standalone icon files in the
    \l{QIcon::fallbackSearchPaths()}{fallback icon search path}.
    Finally, the platform's native icon library is consulted.

    If no icon is found and a \a fallback is provided, \a fallback is
    returned. This is useful to provide a guaranteed fallback, regardless
    of whether the current set of icon themes and fallbacks paths
    support the requested icon.

    If no icon is found and no \a fallback is provided, a default
    constructed, empty QIcon is returned.
*/
QIcon QIcon::fromTheme(QIcon::ThemeIcon icon)
{
    return fromTheme(themeIconName(icon));
}

QIcon QIcon::fromTheme(QIcon::ThemeIcon icon, const QIcon &fallback)
{
    return fromTheme(themeIconName(icon), fallback);
}

/*!
    \since 5.6

    Indicate that this icon is a mask image(boolean \a isMask), and hence can
    potentially be modified based on where it's displayed.
    \sa isMask()
*/
void QIcon::setIsMask(bool isMask)
{
    if (isMask == (d && d->is_mask))
        return;

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
    if (sourceDevicePixelRatio)
        *sourceDevicePixelRatio = 1;
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
