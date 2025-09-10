// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qpalette_p.h"
#include "qguiapplication_p.h"
#include "qdatastream.h"
#include "qvariant.h"
#include "qdebug.h"

#include <QtCore/qmetaobject.h>

QT_BEGIN_NAMESPACE

static_assert(QPalettePrivate::bitPosition(QPalette::ColorGroup(QPalette::NColorGroups - 1),
                              QPalette::ColorRole(QPalette::NColorRoles - 1))
                  < sizeof(QPalette::ResolveMask) * CHAR_BIT,
                  "The resolve mask type is not wide enough to fit the entire bit mask.");

static QColor qt_mix_colors(QColor a, QColor b)
{
    return QColor((a.red() + b.red()) / 2, (a.green() + b.green()) / 2,
                  (a.blue() + b.blue()) / 2, (a.alpha() + b.alpha()) / 2);
}

/*!
    \internal

    Derive undefined \l PlaceholderText colors from \l Text colors.
    Unless already set, PlaceholderText colors will be derived from their Text pendents.
    Colors of existing PlaceHolderText brushes will not be replaced.

    \a alpha represents the dim factor as a percentage. By default, a PlaceHolderText color
    becomes a 50% more transparent version of the corresponding Text color.
*/
static void qt_placeholder_from_text(QPalette &pal, int alpha = 50)
{
    if (alpha < 0 or alpha > 100)
        return;

    for (int cg = 0; cg < int(QPalette::NColorGroups); ++cg) {
        const QPalette::ColorGroup group = QPalette::ColorGroup(cg);

        // skip if the brush has been set already
        if (!pal.isBrushSet(group, QPalette::PlaceholderText)) {
            QColor c = pal.color(group, QPalette::Text);
            const int a = (c.alpha() * alpha) / 100;
            c.setAlpha(a);
            pal.setColor(group, QPalette::PlaceholderText, c);
        }
    }
}

static void qt_ensure_default_accent_color(QPalette &pal)
{
    // have a lighter/darker factor handy, depending on dark/light heuristics
    const int lighter = pal.base().color().lightness() > pal.text().color().lightness() ? 130 : 70;

    // Act only for color groups where no accent color is set
    for (int i = 0; i < QPalette::NColorGroups; ++i) {
        const QPalette::ColorGroup group = static_cast<QPalette::ColorGroup>(i);
        if (!pal.isBrushSet(group, QPalette::Accent)) {
            // Default to highlight if available, otherwise use a shade of base
            const QBrush accentBrush = pal.isBrushSet(group, QPalette::Highlight)
                                     ? pal.brush(group, QPalette::Highlight)
                                     : pal.brush(group, QPalette::Base).color().lighter(lighter);
            pal.setBrush(group, QPalette::Accent, accentBrush);
        }
    }
}

static void qt_palette_from_color(QPalette &pal, const QColor &button)
{
    int h, s, v;
    button.getHsv(&h, &s, &v);
    // inactive and active are the same..
    const QBrush whiteBrush = QBrush(Qt::white);
    const QBrush blackBrush = QBrush(Qt::black);
    const QBrush baseBrush = v > 128 ? whiteBrush : blackBrush;
    const QBrush foregroundBrush = v > 128 ? blackBrush : whiteBrush;
    const QBrush buttonBrush = QBrush(button);
    const QBrush buttonBrushDark = QBrush(button.darker());
    const QBrush buttonBrushDark150 = QBrush(button.darker(150));
    const QBrush buttonBrushLight150 = QBrush(button.lighter(150));
    pal.setColorGroup(QPalette::Active, foregroundBrush, buttonBrush, buttonBrushLight150,
                      buttonBrushDark, buttonBrushDark150, foregroundBrush, whiteBrush,
                      baseBrush, buttonBrush);
    pal.setColorGroup(QPalette::Inactive, foregroundBrush, buttonBrush, buttonBrushLight150,
                      buttonBrushDark, buttonBrushDark150, foregroundBrush, whiteBrush,
                      baseBrush, buttonBrush);
    pal.setColorGroup(QPalette::Disabled, buttonBrushDark, buttonBrush, buttonBrushLight150,
                      buttonBrushDark, buttonBrushDark150, buttonBrushDark,
                      whiteBrush, buttonBrush, buttonBrush);

    qt_placeholder_from_text(pal);
    qt_ensure_default_accent_color(pal);
}

/*!
    \fn QPalette &QPalette::operator=(QPalette &&other)

    Move-assigns \a other to this QPalette instance.

    \since 5.2
*/

/*!
   \fn const QColor &QPalette::color(ColorRole role) const

   \overload

    Returns the color that has been set for the given color \a role in
    the current ColorGroup.

    \sa brush(), ColorRole
 */

/*!
    \fn const QBrush &QPalette::brush(ColorRole role) const

    \overload

    Returns the brush that has been set for the given color \a role in
    the current ColorGroup.

    \sa color(), setBrush(), ColorRole
*/

/*!
    \fn void QPalette::setColor(ColorRole role, const QColor &color)

    \overload

    Sets the color used for the given color \a role, in all color
    groups, to the specified solid \a color.

    \sa brush(), setColor(), ColorRole
*/

/*!
    \fn void QPalette::setBrush(ColorRole role, const QBrush &brush)

    Sets the brush for the given color \a role to the specified \a
    brush for all groups in the palette.

    \sa brush(), setColor(), ColorRole
*/

/*!
    \fn const QBrush & QPalette::windowText() const

    Returns the window text (general foreground) brush of the
    current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::button() const

    Returns the button brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::light() const

    Returns the light brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush& QPalette::midlight() const

    Returns the midlight brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::dark() const

    Returns the dark brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::mid() const

    Returns the mid brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::text() const

    Returns the text foreground brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::brightText() const

    Returns the bright text foreground brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::buttonText() const

    Returns the button text foreground brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::base() const

    Returns the base brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::alternateBase() const

    Returns the alternate base brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::toolTipBase() const
    \since 4.4

    Returns the tool tip base brush of the current color group. This brush is
    used by QToolTip and QWhatsThis.

    \note Tool tips use the Inactive color group of QPalette, because tool
    tips are not active windows.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::toolTipText() const
    \since 4.4

    Returns the tool tip text brush of the current color group. This brush is
    used by QToolTip and QWhatsThis.

    \note Tool tips use the Inactive color group of QPalette, because tool
    tips are not active windows.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::window() const

    Returns the window (general background) brush of the current
    color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::shadow() const

    Returns the shadow brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::highlight() const

    Returns the highlight brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::highlightedText() const

    Returns the highlighted text brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::accent() const
    \since 6.6

    Returns the accent brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::link() const

    Returns the unvisited link text brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::linkVisited() const

    Returns the visited link text brush of the current color group.

    \sa ColorRole, brush()
*/

/*!
    \fn const QBrush & QPalette::placeholderText() const
    \since 5.12

    Returns the placeholder text brush of the current color group.

    \note Before Qt 5.12, the placeholder text color was hard-coded as QPalette::text().color()
    with an alpha of 128 applied. In Qt 6, it is an independent color.

    \sa ColorRole, brush()
*/

/*!
    \fn ColorGroup QPalette::currentColorGroup() const

    Returns the palette's current color group.
*/

/*!
    \fn void QPalette::setCurrentColorGroup(ColorGroup cg)

    Set the palette's current color group to \a cg.
*/

/*!
    \class QPalette

    \brief The QPalette class contains color groups for each widget state.

    \inmodule QtGui
    \ingroup appearance
    \ingroup shared

    A palette consists of three color groups: \e Active, \e Disabled,
    and \e Inactive. All widgets in Qt contain a palette and
    use their palette to draw themselves. This makes the user
    interface easily configurable and easier to keep consistent.


    If you create a new widget we strongly recommend that you use the
    colors in the palette rather than hard-coding specific colors.

    The color groups:
    \list
    \li The Active group is used for the window that has keyboard focus.
    \li The Inactive group is used for other windows.
    \li The Disabled group is used for widgets (not windows) that are
    disabled for some reason.
    \endlist

    Both active and inactive windows can contain disabled widgets.
    (Disabled widgets are often called \e inaccessible or \e{grayed
    out}.)

    In most styles, Active and Inactive look the same.

    Colors and brushes can be set for particular roles in any of a palette's
    color groups with setColor() and setBrush().  A color group contains a
    group of colors used by widgets for drawing themselves. We recommend that
    widgets use color group roles from the palette such as "foreground" and
    "base" rather than literal colors like "red" or "turquoise". The color
    roles are enumerated and defined in the \l ColorRole documentation.

    We strongly recommend that you use the default palette of the
    current style (returned by QGuiApplication::palette()) and
    modify that as necessary. This is done by Qt's widgets when they
    are drawn.

    To modify a color group you call the functions
    setColor() and setBrush(), depending on whether you want a pure
    color or a pixmap pattern.

    There are also corresponding color() and brush() getters, and a
    commonly used convenience function to get the ColorRole for the current ColorGroup:
    window(), windowText(), base(), etc.


    You can copy a palette using the copy constructor and test to see
    if two palettes are \e identical using isCopyOf().

    QPalette is optimized by the use of \l{implicit sharing},
    so it is very efficient to pass QPalette objects as arguments.

    \warning Some styles do not use the palette for all drawing, for
    instance, if they make use of native theme engines. This is the
    case for both the Windows Vista and the \macos
    styles.

    \sa QApplication::setPalette(), QWidget::setPalette(), QColor
*/

/*!
    \enum QPalette::ColorGroup

    \value Disabled
    \value Active
    \value Inactive
    \value Normal synonym for Active

    \omitvalue All
    \omitvalue NColorGroups
    \omitvalue Current
*/

/*!
    \enum QPalette::ColorRole

    \image palette.png Color Roles

    The ColorRole enum defines the different symbolic color roles used
    in current GUIs.

    The central roles are:

    \value Window  A general background color.

    \value WindowText  A general foreground color.

    \value Base  Used mostly as the background color for text entry widgets,
                 but can also be used for other painting - such as the
                 background of combobox drop down lists and toolbar handles.
                 It is usually white or another light color.

    \value AlternateBase  Used as the alternate background color in views with
                          alternating row colors (see
                          QAbstractItemView::setAlternatingRowColors()).

    \value ToolTipBase Used as the background color for QToolTip and
                          QWhatsThis. Tool tips use the Inactive color group
                          of QPalette, because tool tips are not active
                          windows.

    \value ToolTipText Used as the foreground color for QToolTip and
                          QWhatsThis. Tool tips use the Inactive color group
                          of QPalette, because tool tips are not active
                          windows.

    \value PlaceholderText Used as the placeholder color for various text input widgets.
           This enum value has been introduced in Qt 5.12

    \value Text  The foreground color used with \c Base. This is usually
                 the same as the \c WindowText, in which case it must provide
                 good contrast with \c Window and \c Base.

    \value Button The general button background color. This background can be different from
                  \c Window as some styles require a different background color for buttons.

    \value ButtonText  A foreground color used with the \c Button color.

    \value BrightText  A text color that is very different from
                       \c WindowText, and contrasts well with e.g. \c
                       Dark. Typically used for text that needs to be
                       drawn where \c Text or \c WindowText would give
                       poor contrast, such as on pressed push buttons.
                       Note that text colors can be used for things
                       other than just words; text colors are \e
                       usually used for text, but it's quite common to
                       use the text color roles for lines, icons, etc.


    There are some color roles used mostly for 3D bevel and shadow effects.
    All of these are normally derived from \c Window, and used in ways that
    depend on that relationship. For example, buttons depend on it to make the
    bevels look attractive, and Motif scroll bars depend on \c Mid to be
    slightly different from \c Window.

    \value Light  Lighter than \c Button color.

    \value Midlight  Between \c Button and \c Light.

    \value Dark  Darker than \c Button.

    \value Mid  Between \c Button and \c Dark.

    \value Shadow  A very dark color. By default, the shadow color is
                   Qt::black.


    Selected (marked) items have two roles:

    \value Highlight   A color to indicate a selected item or the current
                       item. By default, the highlight color is
                       Qt::darkBlue.

    \value [since 6.6] Accent
                       A color that typically contrasts or complements
                       Base, Window and Button colors. It usually represents
                       the users' choice of desktop personalisation.
                       Styling of interactive components is a typical use case.
                       Unless explicitly set, it defaults to Highlight.

    \value HighlightedText  A text color that contrasts with \c Highlight.
                            By default, the highlighted text color is Qt::white.

    There are two color roles related to hyperlinks:

    \value Link  A text color used for unvisited hyperlinks.
                 By default, the link color is Qt::blue.

    \value LinkVisited  A text color used for already visited hyperlinks.
                        By default, the linkvisited color is Qt::magenta.

    Note that we do not use the \c Link and \c LinkVisited roles when
    rendering rich text in Qt, and that we recommend that you use CSS
    and the QTextDocument::setDefaultStyleSheet() function to alter
    the appearance of links. For example:

    \snippet textdocument-css/main.cpp 0

    \value NoRole No role; this special role is often used to indicate that a
    role has not been assigned.

    \omitvalue NColorRoles
*/

/*!
    Constructs an empty palette object with no color roles set.

    When used as the palette of a QWidget the colors are resolved
    as described by QWidget::setPalette().

    \sa QApplication::setPalette(), QApplication::palette()
*/
QPalette::QPalette()
    : d(nullptr)
{
    // Initialize to application palette if present, else default to black.
    // This makes it possible to instantiate QPalette outside QGuiApplication,
    // for example in the platform plugins.
    if (QGuiApplicationPrivate::app_pal) {
        d = QGuiApplicationPrivate::app_pal->d;
        d->ref.ref();
        setResolveMask(0);
    } else {
        init();
        qt_palette_from_color(*this, Qt::black);
        d->resolveMask = 0;
    }
}

/*!
  Constructs a palette from the \a button color. The other colors are
  automatically calculated, based on this color. \c Window will be
  the button color as well.
*/
QPalette::QPalette(const QColor &button)
{
    init();
    qt_palette_from_color(*this, button);
}

/*!
  Constructs a palette from the \a button color. The other colors are
  automatically calculated, based on this color. \c Window will be
  the button color as well.
*/
QPalette::QPalette(Qt::GlobalColor button)
{
    init();
    qt_palette_from_color(*this, button);
}

/*!
    Constructs a palette. You can pass either brushes, pixmaps or
    plain colors for \a windowText, \a button, \a light, \a dark, \a
    mid, \a text, \a bright_text, \a base and \a window.

    \sa QBrush
*/
QPalette::QPalette(const QBrush &windowText, const QBrush &button,
                   const QBrush &light, const QBrush &dark,
                   const QBrush &mid, const QBrush &text,
                   const QBrush &bright_text, const QBrush &base,
                   const QBrush &window)
{
    init();
    setColorGroup(All, windowText, button, light, dark, mid, text, bright_text,
                  base, window);

    qt_placeholder_from_text(*this);
    qt_ensure_default_accent_color(*this);
}


/*!
  \deprecated

  Constructs a palette with the specified \a windowText, \a
  window, \a light, \a dark, \a mid, \a text, and \a base colors.
  The button color will be set to the window color.
*/
QPalette::QPalette(const QColor &windowText, const QColor &window,
                   const QColor &light, const QColor &dark, const QColor &mid,
                   const QColor &text, const QColor &base)
{
    init();
    const QBrush windowBrush(window);
    const QBrush lightBrush(light);
    setColorGroup(All, QBrush(windowText), windowBrush, lightBrush,
                  QBrush(dark), QBrush(mid), QBrush(text), lightBrush,
                  QBrush(base), windowBrush);
}

/*!
    Constructs a palette from a \a button color and a \a window.
    The other colors are automatically calculated, based on these
    colors.
*/
QPalette::QPalette(const QColor &button, const QColor &window)
{
    init();
    int h, s, v;
    window.getHsv(&h, &s, &v);

    const QBrush windowBrush = QBrush(window);
    const QBrush whiteBrush = QBrush(Qt::white);
    const QBrush blackBrush = QBrush(Qt::black);
    const QBrush baseBrush = v > 128 ? whiteBrush : blackBrush;
    const QBrush foregroundBrush = v > 128 ? blackBrush : whiteBrush;
    const QBrush disabledForeground = QBrush(Qt::darkGray);

    const QBrush buttonBrush = QBrush(button);
    const QBrush buttonBrushDark = QBrush(button.darker());
    const QBrush buttonBrushDark150 = QBrush(button.darker(150));
    const QBrush buttonBrushLight150 = QBrush(button.lighter(150));

    //inactive and active are identical
    setColorGroup(Inactive, foregroundBrush, buttonBrush, buttonBrushLight150, buttonBrushDark,
                  buttonBrushDark150, foregroundBrush, whiteBrush, baseBrush,
                  windowBrush);
    setColorGroup(Active, foregroundBrush, buttonBrush, buttonBrushLight150, buttonBrushDark,
                  buttonBrushDark150, foregroundBrush, whiteBrush, baseBrush,
                  windowBrush);
    setColorGroup(Disabled, disabledForeground, buttonBrush, buttonBrushLight150,
                  buttonBrushDark, buttonBrushDark150, disabledForeground,
                  whiteBrush, baseBrush, windowBrush);

    qt_placeholder_from_text(*this);
    qt_ensure_default_accent_color(*this);
}

/*!
    Constructs a copy of \a p.

    This constructor is fast thanks to \l{implicit sharing}.
*/
QPalette::QPalette(const QPalette &p)
    : d(p.d), currentGroup(p.currentGroup)
{
    d->ref.ref();
}

/*!
    \fn QPalette::QPalette(QPalette &&other)
    \since 5.4

    Move-constructs a QPalette instance, making it point at the same
    object that \a other was pointing to.

    After being moved from, you can only assign to or destroy \a other.
    Any other operation will result in undefined behavior.
*/

/*!
    Destroys the palette.
*/
QPalette::~QPalette()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!\internal*/
void QPalette::init()
{
    d = new QPalettePrivate;
}

/*!
    Assigns \a p to this palette and returns a reference to this
    palette.

    This operation is fast thanks to \l{implicit sharing}.
*/
QPalette &QPalette::operator=(const QPalette &p)
{
    p.d->ref.ref();
    currentGroup = p.currentGroup;
    if (d && !d->ref.deref())
        delete d;
    d = p.d;
    return *this;
}

/*!
    \fn void QPalette::swap(QPalette &other)
    \since 5.0
    \memberswap{palette instance}
*/

/*!
   Returns the palette as a QVariant
*/
QPalette::operator QVariant() const
{
    return QVariant::fromValue(*this);
}

/*!
    \fn const QColor &QPalette::color(ColorGroup group, ColorRole role) const

    Returns the color in the specified color \a group, used for the
    given color \a role.

    \sa brush(), setColor(), ColorRole
*/

/*!
    \fn const QBrush &QPalette::brush(ColorGroup group, ColorRole role) const

    Returns the brush in the specified color \a group, used for the
    given color \a role.

    \sa color(), setBrush(), ColorRole
*/
const QBrush &QPalette::brush(ColorGroup gr, ColorRole cr) const
{
    Q_ASSERT(cr < NColorRoles);
    if (gr >= (int)NColorGroups) {
        if (gr == Current) {
            gr = currentGroup;
        } else {
            qWarning("QPalette::brush: Unknown ColorGroup: %d", (int)gr);
            gr = Active;
        }
    }
    return d->data->br[gr][cr];
}

/*!
    \fn void QPalette::setColor(ColorGroup group, ColorRole role, const QColor &color)

    Sets the color in the specified color \a group, used for the given
    color \a role, to the specified solid \a color.

    \sa setBrush(), color(), ColorRole
*/

/*!
    \fn void QPalette::setBrush(ColorGroup group, ColorRole role, const QBrush &brush)
    \overload

    Sets the brush in the specified color \a group, used for the given
    color \a role, to \a brush.

    \sa brush(), setColor(), ColorRole
*/
void QPalette::setBrush(ColorGroup cg, ColorRole cr, const QBrush &b)
{
    Q_ASSERT(cr < NColorRoles);

    if (cg == All) {
        for (uint i = 0; i < NColorGroups; i++)
            setBrush(ColorGroup(i), cr, b);
        return;
    }

    if (cg == Current) {
        cg = currentGroup;
    } else if (cg >= NColorGroups) {
        qWarning("QPalette::setBrush: Unknown ColorGroup: %d", cg);
        cg = Active;
    }

    const auto newResolveMask = d->resolveMask | ResolveMask(1) << QPalettePrivate::bitPosition(cg, cr);
    const auto valueChanged = d->data->br[cg][cr] != b;

    if (valueChanged) {
        detach();
        d->data.detach();
        d->data->br[cg][cr] = b;
    } else if (d->resolveMask != newResolveMask) {
        detach();
    }

    d->resolveMask = newResolveMask;
}

/*!
    \since 4.2

    Returns \c true if the ColorGroup \a cg and ColorRole \a cr has been
    set previously on this palette; otherwise returns \c false.

    The ColorGroup \a cg should be less than QPalette::NColorGroups,
    but you can use QPalette::Current. In this case, the previously
    set current color group will be used.

    The ColorRole \a cr should be less than QPalette::NColorRoles.

    \sa setBrush(), currentColorGroup()
*/
bool QPalette::isBrushSet(ColorGroup cg, ColorRole cr) const
{
    // NoRole has no resolve mask and should never be set anyway
    if (cr == NoRole)
        return false;

    if (cg == Current)
        cg = currentGroup;

    if (cg >= NColorGroups) {
        qWarning() << "Wrong color group:" << cg;
        return false;
    }

    if (cr >= NColorRoles) {
        qWarning() << "Wrong color role:" << cr;
        return false;
    }

    return d->resolveMask & (ResolveMask(1) << QPalettePrivate::bitPosition(cg, cr));
}

/*!
    \internal
*/
void QPalette::detach()
{
    if (d->ref.loadRelaxed() != 1) {
        QPalettePrivate *x = new QPalettePrivate(d->data);
        x->resolveMask = d->resolveMask;
        if (!d->ref.deref())
            delete d;
        d = x;
    } else {
        d->detach_no = ++QPalettePrivate::qt_palette_private_count;
    }
}

/*!
    \fn bool QPalette::operator!=(const QPalette &p) const

    Returns \c true (slowly) if this palette is different from \a p;
    otherwise returns \c false (usually quickly).

    \note The current ColorGroup is not taken into account when
    comparing palettes

    \sa operator==()
*/

/*!
    Returns \c true (usually quickly) if this palette is equal to \a p;
    otherwise returns \c false (slowly).

    \note The following is not taken into account when comparing palettes:
    \list
    \li the \c current ColorGroup
    \li ColorRole NoRole \since 6.6
    \endlist

    \sa operator!=()
*/
bool QPalette::operator==(const QPalette &p) const
{
    if (isCopyOf(p) || d->data == p.d->data)
        return true;
    for(int grp = 0; grp < (int)NColorGroups; grp++) {
        for(int role = 0; role < (int)NColorRoles; role++) {
            // Dont't verify NoRole, because it has no resolve bit
            if (role == NoRole)
                continue;
            if (d->data->br[grp][role] != p.d->data->br[grp][role])
                return false;
        }
    }
    return true;
}

/*!
    \fn bool QPalette::isEqual(ColorGroup cg1, ColorGroup cg2) const

    Returns \c true (usually quickly) if color group \a cg1 is equal to
    \a cg2; otherwise returns \c false.
*/
bool QPalette::isEqual(QPalette::ColorGroup group1, QPalette::ColorGroup group2) const
{
    if (group1 >= (int)NColorGroups) {
        if (group1 == Current) {
            group1 = currentGroup;
        } else {
            qWarning("QPalette::brush: Unknown ColorGroup(1): %d", (int)group1);
            group1 = Active;
        }
    }
    if (group2 >= (int)NColorGroups) {
        if (group2 == Current) {
            group2 = currentGroup;
        } else {
            qWarning("QPalette::brush: Unknown ColorGroup(2): %d", (int)group2);
            group2 = Active;
        }
    }
    if (group1 == group2)
        return true;
    for(int role = 0; role < (int)NColorRoles; role++) {
        if (d->data->br[group1][role] != d->data->br[group2][role])
                return false;
    }
    return true;
}

/*!
    Returns a number that identifies the contents of this QPalette
    object. Distinct QPalette objects can have the same key if
    they refer to the same contents.

    The cacheKey() will change when the palette is altered.
*/
qint64 QPalette::cacheKey() const
{
    return (((qint64) d->data->ser_no) << 32) | ((qint64) (d->detach_no));
}

static constexpr QPalette::ResolveMask allResolveMask()
{
    QPalette::ResolveMask mask = {0};
    for (int role = 0; role < int(QPalette::NColorRoles); ++role) {
        for (int grp = 0; grp < int(QPalette::NColorGroups); ++grp) {
            mask |= (QPalette::ResolveMask(1) << QPalettePrivate::bitPosition(QPalette::ColorGroup(grp), QPalette::ColorRole(role)));
        }
    }
    return mask;
}

/*!
    Returns a new QPalette that is a union of this instance and \a other.
    Color roles set in this instance take precedence. Roles that are not
    set in this instance will be taken from \a other.

    \sa isBrushSet
*/
QPalette QPalette::resolve(const QPalette &other) const
{
    if ((*this == other && d->resolveMask == other.d->resolveMask)
        || d->resolveMask == 0) {
        QPalette o = other;
        o.setResolveMask(d->resolveMask);
        return o;
    }

    if (d->resolveMask == allResolveMask())
        return *this;

    QPalette palette(*this);
    palette.detach();

    for (int role = 0; role < int(NColorRoles); ++role) {
        // Don't resolve NoRole, its bits are needed for Accent (see bitPosition)
        if (role == NoRole)
            continue;

        for (int grp = 0; grp < int(NColorGroups); ++grp) {
            if (!(d->resolveMask & (ResolveMask(1) << QPalettePrivate::bitPosition(ColorGroup(grp), ColorRole(role))))) {
                palette.d->data.detach();
                palette.d->data->br[grp][role] = other.d->data->br[grp][role];
            }
        }
    }

    palette.d->resolveMask |= other.d->resolveMask;

    return palette;
}

/*!
    \internal
*/
QPalette::ResolveMask QPalette::resolveMask() const
{
    return d->resolveMask;
}

/*!
    \internal
*/
void QPalette::setResolveMask(QPalette::ResolveMask mask)
{
    if (mask == d->resolveMask)
        return;

    detach();
    d->resolveMask = mask;
}

/*!
    \typedef ResolveMask
    \internal

    A bit mask that stores which colors the palette instance explicitly defines,
    and which ones are inherited from a parent.
*/

/*****************************************************************************
  QPalette stream functions
 *****************************************************************************/

#ifndef QT_NO_DATASTREAM

static const int NumOldRoles = 7;
static const int oldRoles[7] = { QPalette::WindowText, QPalette::Window, QPalette::Light,
                                 QPalette::Dark, QPalette::Mid, QPalette::Text, QPalette::Base };

/*!
    \relates QPalette

    Writes the palette, \a p to the stream \a s and returns a
    reference to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

QDataStream &operator<<(QDataStream &s, const QPalette &p)
{
    for (int grp = 0; grp < (int)QPalette::NColorGroups; grp++) {
        if (s.version() == 1) {
            // Qt 1.x
            for (int i = 0; i < NumOldRoles; ++i)
                s << p.d->data->br[grp][oldRoles[i]].color();
        } else {
            int max = (int)QPalette::NColorRoles;
            if (s.version() <= QDataStream::Qt_2_1)
                max = QPalette::HighlightedText + 1;
            else if (s.version() <= QDataStream::Qt_4_3)
                max = QPalette::AlternateBase + 1;
            else if (s.version() <= QDataStream::Qt_5_11)
                max = QPalette::ToolTipText + 1;
            else if (s.version() <= QDataStream::Qt_6_5)
                max = QPalette::PlaceholderText + 1;

            for (int r = 0; r < max; r++)
                s << p.d->data->br[grp][r];
        }
    }
    return s;
}

static void readV1ColorGroup(QDataStream &s, QPalette &pal, QPalette::ColorGroup grp)
{
    for (int i = 0; i < NumOldRoles; ++i) {
        QColor col;
        s >> col;
        pal.setColor(grp, (QPalette::ColorRole)oldRoles[i], col);
    }
}

/*!
    \relates QPalette

    Reads a palette from the stream, \a s into the palette \a p, and
    returns a reference to the stream.

    \sa{Serializing Qt Data Types}{Format of the QDataStream operators}
*/

QDataStream &operator>>(QDataStream &s, QPalette &p)
{
    if (s.version() == 1) {
        p = QPalette();
        readV1ColorGroup(s, p, QPalette::Active);
        readV1ColorGroup(s, p, QPalette::Disabled);
        readV1ColorGroup(s, p, QPalette::Inactive);
    } else {
        int max = QPalette::NColorRoles;
        if (s.version() <= QDataStream::Qt_2_1) {
            p = QPalette();
            max = QPalette::HighlightedText + 1;
        } else if (s.version() <= QDataStream::Qt_4_3) {
            p = QPalette();
            max = QPalette::AlternateBase + 1;
        } else if (s.version() <= QDataStream::Qt_5_11) {
            p = QPalette();
            max = QPalette::ToolTipText + 1;
        } else if (s.version() <= QDataStream::Qt_6_5) {
            p = QPalette();
            max = QPalette::PlaceholderText + 1;
        }


        QBrush tmp;
        for(int grp = 0; grp < (int)QPalette::NColorGroups; ++grp) {
            const QPalette::ColorGroup group = static_cast<QPalette::ColorGroup>(grp);
            for(int role = 0; role < max; ++role) {
                s >> tmp;
                p.setBrush(group, (QPalette::ColorRole)role, tmp);
            }

            // Accent defaults to Highlight for stream versions that don't have it.
            if (s.version() < QDataStream::Qt_6_6)
                p.setBrush(group, QPalette::Accent, p.brush(group, QPalette::Highlight));
        }

    }
    return s;
}
#endif //QT_NO_DATASTREAM

/*!
    Returns \c true if this palette and \a p are copies of each other,
    i.e. one of them was created as a copy of the other and neither
    was subsequently modified; otherwise returns \c false. This is much
    stricter than equality.

    \sa operator=(), operator==()
*/

bool QPalette::isCopyOf(const QPalette &p) const
{
    return d == p.d;
}

/*!

    Sets a the group at \a cg. You can pass either brushes, pixmaps or
    plain colors for \a windowText, \a button, \a light, \a dark, \a
    mid, \a text, \a bright_text, \a base and \a window.

    \sa QBrush
*/
void QPalette::setColorGroup(ColorGroup cg, const QBrush &windowText, const QBrush &button,
                             const QBrush &light, const QBrush &dark, const QBrush &mid,
                             const QBrush &text, const QBrush &bright_text, const QBrush &base,
                             const QBrush &window)
{
    QBrush alt_base = QBrush(qt_mix_colors(base.color(), button.color()));
    QBrush mid_light = QBrush(qt_mix_colors(button.color(), light.color()));
    QColor toolTipBase(255, 255, 220);
    QColor toolTipText(0, 0, 0);

    setColorGroup(cg, windowText, button, light, dark, mid, text, bright_text, base,
                  alt_base, window, mid_light, text,
                  QBrush(Qt::black), QBrush(Qt::darkBlue), QBrush(Qt::white),
                  QBrush(Qt::blue), QBrush(Qt::magenta), QBrush(toolTipBase),
                  QBrush(toolTipText));

    for (int cr = Highlight; cr <= LinkVisited; ++cr) {
        if (cg == All) {
            for (int group = Active; group < NColorGroups; ++group) {
                d->resolveMask &= ~(ResolveMask(1) << QPalettePrivate::bitPosition(ColorGroup(group), ColorRole(cr)));
            }
        } else {
            d->resolveMask &= ~(ResolveMask(1) << QPalettePrivate::bitPosition(ColorGroup(cg), ColorRole(cr)));
        }
    }
}


/*!\internal*/
void
QPalette::setColorGroup(ColorGroup cg, const QBrush &foreground, const QBrush &button,
                        const QBrush &light, const QBrush &dark, const QBrush &mid,
                        const QBrush &text, const QBrush &bright_text,
                        const QBrush &base, const QBrush &alternate_base,
                        const QBrush &background, const QBrush &midlight,
                        const QBrush &button_text, const QBrush &shadow,
                        const QBrush &highlight, const QBrush &highlighted_text,
                        const QBrush &link, const QBrush &link_visited)
{
    setColorGroup(cg, foreground, button, light, dark, mid,
                  text, bright_text, base, alternate_base, background,
                  midlight, button_text, shadow, highlight, highlighted_text,
                  link, link_visited, background, foreground);
}

/*!\internal*/
void QPalette::setColorGroup(ColorGroup cg, const QBrush &foreground, const QBrush &button,
                             const QBrush &light, const QBrush &dark, const QBrush &mid,
                             const QBrush &text, const QBrush &bright_text,
                             const QBrush &base, const QBrush &alternate_base,
                             const QBrush &background, const QBrush &midlight,
                             const QBrush &button_text, const QBrush &shadow,
                             const QBrush &highlight, const QBrush &highlighted_text,
                             const QBrush &link, const QBrush &link_visited,
                             const QBrush &toolTipBase, const QBrush &toolTipText)
{
    setBrush(cg, WindowText, foreground);
    setBrush(cg, Button, button);
    setBrush(cg, Light, light);
    setBrush(cg, Dark, dark);
    setBrush(cg, Mid, mid);
    setBrush(cg, Text, text);
    setBrush(cg, BrightText, bright_text);
    setBrush(cg, Base, base);
    setBrush(cg, AlternateBase, alternate_base);
    setBrush(cg, Window, background);
    setBrush(cg, Midlight, midlight);
    setBrush(cg, ButtonText, button_text);
    setBrush(cg, Shadow, shadow);
    setBrush(cg, Highlight, highlight);
    setBrush(cg, HighlightedText, highlighted_text);
    setBrush(cg, Link, link);
    setBrush(cg, LinkVisited, link_visited);
    setBrush(cg, ToolTipBase, toolTipBase);
    setBrush(cg, ToolTipText, toolTipText);
}

#ifndef QT_NO_DEBUG_STREAM
static QString groupsToString(const QPalette &p, QPalette::ColorRole cr)
{
    const auto groupEnum = QMetaEnum::fromType<QPalette::ColorGroup>();

    QString groupString;
    for (int group = 0; group < QPalette::NColorGroups; ++group) {
        const auto cg = QPalette::ColorGroup(group);

        if (p.isBrushSet(cg, cr)) {
            const auto &color = p.color(cg, cr);
            groupString += QString::fromUtf8(groupEnum.valueToKey(cg)) + u':' +
                           color.name(QColor::HexArgb) + u',';
        }
    }
    groupString.chop(1);

    return groupString;
}

static QString rolesToString(const QPalette &p)
{
    const auto roleEnum = QMetaEnum::fromType<QPalette::ColorRole>();

    QString roleString;
    for (int role = 0; role < QPalette::NColorRoles; ++role) {
        const auto cr = QPalette::ColorRole(role);

        auto groupString = groupsToString(p, cr);
        if (!groupString.isEmpty())
            roleString += QString::fromUtf8(roleEnum.valueToKey(cr)) + QStringLiteral(":[") +
                          groupString + QStringLiteral("],");
    }
    roleString.chop(1);

    return roleString;
}

QDebug operator<<(QDebug dbg, const QPalette &p)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace();

    dbg << "QPalette(resolve=" << Qt::hex << Qt::showbase << p.resolveMask();

    auto roleString = rolesToString(p);
    if (!roleString.isEmpty())
        dbg << ',' << roleString;

    dbg << ')';

    return dbg;
 }
#endif

QT_END_NAMESPACE

#include "moc_qpalette.cpp"
