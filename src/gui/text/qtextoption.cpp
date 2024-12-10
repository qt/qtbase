// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextoption.h"
#include "qguiapplication.h"
#include "qlist.h"

QT_BEGIN_NAMESPACE

QT_IMPL_METATYPE_EXTERN_TAGGED(QTextOption::Tab, QTextOption_Tab)

struct QTextOptionPrivate
{
    QList<QTextOption::Tab> tabStops;
};

/*!
    Constructs a text option with default properties for text.
    The text alignment property is set to Qt::AlignLeft. The
    word wrap property is set to QTextOption::WordWrap. The
    using of design metrics flag is set to false.
*/
QTextOption::QTextOption()
    : QTextOption(Qt::AlignLeft)
{
    direction = Qt::LayoutDirectionAuto;
}

/*!
    Constructs a text option with the given \a alignment for text.
    The word wrap property is set to QTextOption::WordWrap. The using
    of design metrics flag is set to false.
*/
QTextOption::QTextOption(Qt::Alignment alignment)
    : align(alignment),
      wordWrap(QTextOption::WordWrap),
      design(false),
      unused(0),
      f(0),
      tab(-1),
      d(nullptr)
{
    direction = QGuiApplication::layoutDirection();
}

/*!
    Destroys the text option.
*/
QTextOption::~QTextOption()
{
    delete d;
}

/*!
    \fn QTextOption::QTextOption(const QTextOption &other)

    Construct a copy of the \a other text option.
*/
QTextOption::QTextOption(const QTextOption &o)
    : align(o.align),
      wordWrap(o.wordWrap),
      design(o.design),
      direction(o.direction),
      unused(o.unused),
      f(o.f),
      tab(o.tab),
      d(nullptr)
{
    if (o.d)
        d = new QTextOptionPrivate(*o.d);
}

/*!
    \fn QTextOption &QTextOption::operator=(const QTextOption &other)

    Returns \c true if the text option is the same as the \a other text option;
    otherwise returns \c false.
*/
QTextOption &QTextOption::operator=(const QTextOption &o)
{
    if (this == &o)
        return *this;

    QTextOptionPrivate* dNew = nullptr;
    if (o.d)
        dNew = new QTextOptionPrivate(*o.d);
    delete d;
    d = dNew;

    align = o.align;
    wordWrap = o.wordWrap;
    design = o.design;
    direction = o.direction;
    unused = o.unused;
    f = o.f;
    tab = o.tab;
    return *this;
}

/*!
    Sets the tab positions for the text layout to those specified by
    \a tabStops.

    \sa tabArray(), setTabStopDistance(), setTabs()
*/
void QTextOption::setTabArray(const QList<qreal> &tabStops)
{
    if (!d)
        d = new QTextOptionPrivate;
    QList<QTextOption::Tab> tabs;
    QTextOption::Tab tab;
    tabs.reserve(tabStops.size());
    for (qreal pos : tabStops) {
        tab.position = pos;
        tabs.append(tab);
    }
    d->tabStops = tabs;
}

/*!
    \since 4.4
    Sets the tab positions for the text layout to those specified by
    \a tabStops.

    \sa tabStopDistance()
*/
void QTextOption::setTabs(const QList<QTextOption::Tab> &tabStops)
{
    if (!d)
        d = new QTextOptionPrivate;
    d->tabStops = tabStops;
}

/*!
    Returns a list of tab positions defined for the text layout.

    \sa setTabArray(), tabStopDistance()
*/
QList<qreal> QTextOption::tabArray() const
{
    QList<qreal> answer;
    if (!d)
        return answer;

    answer.reserve(d->tabStops.size());
    QList<QTextOption::Tab>::ConstIterator iter = d->tabStops.constBegin();
    while(iter != d->tabStops.constEnd()) {
        answer.append( (*iter).position);
        ++iter;
    }
    return answer;
}


QList<QTextOption::Tab> QTextOption::tabs() const
{
    if (!d)
        return QList<QTextOption::Tab>();
    return d->tabStops;
}

/*!
    \class QTextOption
    \reentrant

    \brief The QTextOption class provides a description of general rich text
    properties.
    \inmodule QtGui

    \ingroup richtext-processing

    QTextOption is used to encapsulate common rich text properties in a single
    object. It contains information about text alignment, layout direction,
    word wrapping, and other standard properties associated with text rendering
    and layout.

    \sa QTextEdit, QTextDocument, QTextCursor
*/

/*!
    \enum QTextOption::WrapMode

    This enum describes how text is wrapped in a document.

    \value NoWrap       Text is not wrapped at all.
    \value WordWrap     Text is wrapped at word boundaries.
    \value ManualWrap   Same as QTextOption::NoWrap
    \value WrapAnywhere Text can be wrapped at any point on a line, even if
                        it occurs in the middle of a word.
    \value WrapAtWordBoundaryOrAnywhere If possible, wrapping occurs at a word
                        boundary; otherwise it will occur at the appropriate
                        point on the line, even in the middle of a word.
*/

/*!
  \fn void QTextOption::setUseDesignMetrics(bool enable)

    If \a enable is true then the layout will use design metrics;
    otherwise it will use the metrics of the paint device (which is
    the default behavior).

    \sa useDesignMetrics()
*/

/*!
  \fn bool QTextOption::useDesignMetrics() const

    Returns \c true if the layout uses design rather than device metrics;
    otherwise returns \c false.

    \sa setUseDesignMetrics()
*/

/*!
  \fn Qt::Alignment QTextOption::alignment() const

  Returns the text alignment defined by the option.

  \sa setAlignment()
*/

/*!
  \fn void QTextOption::setAlignment(Qt::Alignment alignment);

  Sets the option's text alignment to the specified \a alignment.

  \sa alignment()
*/

/*!
  \fn Qt::LayoutDirection QTextOption::textDirection() const

  Returns the direction of the text layout defined by the option.

  \sa setTextDirection()
*/

/*!
  \fn void QTextOption::setTextDirection(Qt::LayoutDirection direction)

  Sets the direction of the text layout defined by the option to the
  given \a direction.

  \sa textDirection()
*/

/*!
  \fn WrapMode QTextOption::wrapMode() const

  Returns the text wrap mode defined by the option.

  \sa setWrapMode()
*/

/*!
  \fn void QTextOption::setWrapMode(WrapMode mode)

  Sets the option's text wrap mode to the given \a mode.
*/

/*!
  \enum QTextOption::Flag

  \value IncludeTrailingSpaces When this option is set, QTextLine::naturalTextWidth() and naturalTextRect() will
                               return a value that includes the width of trailing spaces in the text; otherwise
                               this width is excluded.
  \value ShowTabsAndSpaces Visualize spaces with little dots, and tabs with little arrows. Non-breaking spaces are
            shown differently to breaking spaces.
  \value ShowLineAndParagraphSeparators Visualize line and paragraph separators with appropriate symbol characters.
  \value [since 5.7] ShowDocumentTerminator Visualize the end of the document with a section sign.
  \value [since 6.9] ShowDefaultIgnorables Render normally non-visual characters if supported by font.
  \value AddSpaceForLineAndParagraphSeparators While determining the line-break positions take into account the
            space added for drawing a separator character.
  \value SuppressColors Suppress all color changes in the character formats (except the main selection).
  \value [since 6.9] DisableEmojiParsing By default, Qt will detect emoji sequences in input strings
    and prioritize using color fonts to display them. This extra step can be disabled by setting the
    DisableEmojiParsing flag if it is known in advance that it will not be needed.
*/

/*!
  \fn Flags QTextOption::flags() const

  Returns the flags associated with the option.

  \sa setFlags()
*/

/*!
  \fn void QTextOption::setFlags(Flags flags)

  Sets the flags associated with the option to the given \a flags.

  \sa flags()
*/

/*!
  \fn qreal QTextOption::tabStopDistance() const
  \since 5.10

  Returns the distance in device units between tab stops.

  \sa setTabStopDistance(), tabArray(), setTabs(), tabs()
*/

/*!
  \fn void QTextOption::setTabStopDistance(qreal tabStopDistance)
  \since 5.10

  Sets the default distance in device units between tab stops to the value specified
  by \a tabStopDistance.

  \sa tabStopDistance(), setTabArray(), setTabs(), tabs()
*/

/*!
    \enum QTextOption::TabType
    \since 4.4

    This enum holds the different types of tabulator

    \value LeftTab      A left-tab
    \value RightTab     A right-tab
    \value CenterTab    A centered-tab
    \value DelimiterTab A tab stopping at a certain delimiter-character
*/

/*!
    \class QTextOption::Tab
    \since 4.4
    \inmodule QtGui
    Each tab definition is represented by this struct.
*/

/*!
    \variable QTextOption::Tab::position
    Distance from the start of the paragraph.
    The position of a tab is from the start of the paragraph which implies that when
    the alignment of the paragraph is set to centered, the tab is interpreted to be
    moved the same distance as the left edge of the paragraph does.
    In case the paragraph is set to have a layoutDirection() RightToLeft the position
    is interpreted to be from the right side of the paragraph with higher numbers moving
    the tab to the left.
*/

/*!
    \variable QTextOption::Tab::type
    Determine which type is used.
    In a paragraph that has layoutDirection() RightToLeft the type LeftTab will
    be interpreted to be a RightTab and vice versa.
*/

/*!
    \variable QTextOption::Tab::delimiter
    If type is DelimitorTab; tab until this char is found in the text.
*/

/*!
    \fn QTextOption::Tab::Tab()
    Creates a default left tab with position 80.
*/

/*!
    \fn QTextOption::Tab::Tab(qreal pos, TabType tabType, QChar delim = QChar())

    Creates a tab with the given position, tab type, and delimiter
    (\a pos, \a tabType, \a delim).

    \note \a delim is only used when \a tabType is DelimiterTab.

    \since 4.7
*/

/*!
    \fn bool QTextOption::Tab::operator==(const QTextOption::Tab &other) const

    Returns \c true if tab \a other is equal to this tab;
    otherwise returns \c false.
*/

/*!
    \fn bool QTextOption::Tab::operator!=(const QTextOption::Tab &other) const

    Returns \c true if tab \a other is not equal to this tab;
    otherwise returns \c false.
*/

/*!
  \since 4.4
  \fn QList<QTextOption::Tab> QTextOption::tabs() const
  Returns a list of tab positions defined for the text layout.

  \sa tabStopDistance(), setTabs(), setTabStopDistance()
*/


QT_END_NAMESPACE
