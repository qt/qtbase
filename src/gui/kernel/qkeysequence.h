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

#ifndef QKEYSEQUENCE_H
#define QKEYSEQUENCE_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qstring.h>
#include <QtCore/qobjectdefs.h>

QT_REQUIRE_CONFIG(shortcut);

QT_BEGIN_NAMESPACE

class QKeySequence;

/*****************************************************************************
  QKeySequence stream functions
 *****************************************************************************/
#if !defined(QT_NO_DATASTREAM) || defined(Q_CLANG_QDOC)
Q_GUI_EXPORT QDataStream &operator<<(QDataStream &in, const QKeySequence &ks);
Q_GUI_EXPORT QDataStream &operator>>(QDataStream &out, QKeySequence &ks);
#endif

#if defined(Q_CLANG_QDOC)
void qt_set_sequence_auto_mnemonic(bool b);
#endif

class QVariant;
class QKeySequencePrivate;

Q_GUI_EXPORT Q_DECL_PURE_FUNCTION size_t qHash(const QKeySequence &key, size_t seed = 0) noexcept;

class Q_GUI_EXPORT QKeySequence
{
    Q_GADGET

public:
    enum StandardKey {
        UnknownKey,
        HelpContents,
        WhatsThis,
        Open,
        Close,
        Save,
        New,
        Delete,
        Cut,
        Copy,
        Paste,
        Undo,
        Redo,
        Back,
        Forward,
        Refresh,
        ZoomIn,
        ZoomOut,
        Print,
        AddTab,
        NextChild,
        PreviousChild,
        Find,
        FindNext,
        FindPrevious,
        Replace,
        SelectAll,
        Bold,
        Italic,
        Underline,
        MoveToNextChar,
        MoveToPreviousChar,
        MoveToNextWord,
        MoveToPreviousWord,
        MoveToNextLine,
        MoveToPreviousLine,
        MoveToNextPage,
        MoveToPreviousPage,
        MoveToStartOfLine,
        MoveToEndOfLine,
        MoveToStartOfBlock,
        MoveToEndOfBlock,
        MoveToStartOfDocument,
        MoveToEndOfDocument,
        SelectNextChar,
        SelectPreviousChar,
        SelectNextWord,
        SelectPreviousWord,
        SelectNextLine,
        SelectPreviousLine,
        SelectNextPage,
        SelectPreviousPage,
        SelectStartOfLine,
        SelectEndOfLine,
        SelectStartOfBlock,
        SelectEndOfBlock,
        SelectStartOfDocument,
        SelectEndOfDocument,
        DeleteStartOfWord,
        DeleteEndOfWord,
        DeleteEndOfLine,
        InsertParagraphSeparator,
        InsertLineSeparator,
        SaveAs,
        Preferences,
        Quit,
        FullScreen,
        Deselect,
        DeleteCompleteLine,
        Backspace,
        Cancel
     };
     Q_ENUM(StandardKey)

    enum SequenceFormat {
        NativeText,
        PortableText
    };

    QKeySequence();
    QKeySequence(const QString &key, SequenceFormat format = NativeText);
    QKeySequence(int k1, int k2 = 0, int k3 = 0, int k4 = 0);
    QKeySequence(QKeyCombination k1,
                 QKeyCombination k2 = QKeyCombination::fromCombined(0),
                 QKeyCombination k3 = QKeyCombination::fromCombined(0),
                 QKeyCombination k4 = QKeyCombination::fromCombined(0));
    QKeySequence(const QKeySequence &ks);
    QKeySequence(StandardKey key);
    ~QKeySequence();

    int count() const;
    bool isEmpty() const;

    enum SequenceMatch {
        NoMatch,
        PartialMatch,
        ExactMatch
    };

    QString toString(SequenceFormat format = PortableText) const;
    static QKeySequence fromString(const QString &str, SequenceFormat format = PortableText);

    static QList<QKeySequence> listFromString(const QString &str, SequenceFormat format = PortableText);
    static QString listToString(const QList<QKeySequence> &list, SequenceFormat format = PortableText);

    SequenceMatch matches(const QKeySequence &seq) const;
    static QKeySequence mnemonic(const QString &text);
    static QList<QKeySequence> keyBindings(StandardKey key);

    operator QVariant() const;
    QKeyCombination operator[](uint i) const;
    QKeySequence &operator=(const QKeySequence &other);
    QT_MOVE_ASSIGNMENT_OPERATOR_IMPL_VIA_PURE_SWAP(QKeySequence)
    void swap(QKeySequence &other) noexcept { qSwap(d, other.d); }

    bool operator==(const QKeySequence &other) const;
    inline bool operator!= (const QKeySequence &other) const
    { return !(*this == other); }
    bool operator< (const QKeySequence &ks) const;
    inline bool operator> (const QKeySequence &other) const
    { return other < *this; }
    inline bool operator<= (const QKeySequence &other) const
    { return !(other < *this); }
    inline bool operator>= (const QKeySequence &other) const
    { return !(*this < other); }

    bool isDetached() const;
private:
    static int decodeString(const QString &ks);
    static QString encodeString(int key);
    int assign(const QString &str);
    int assign(const QString &str, SequenceFormat format);
    void setKey(QKeyCombination key, int index);

    QKeySequencePrivate *d;

    friend Q_GUI_EXPORT QDataStream &operator<<(QDataStream &in, const QKeySequence &ks);
    friend Q_GUI_EXPORT QDataStream &operator>>(QDataStream &in, QKeySequence &ks);
    friend Q_GUI_EXPORT size_t qHash(const QKeySequence &key, size_t seed) noexcept;
    friend class QShortcutMap;
    friend class QShortcut;

public:
    typedef QKeySequencePrivate * DataPtr;
    inline DataPtr &data_ptr() { return d; }
};

Q_DECLARE_SHARED(QKeySequence)

#if !defined(QT_NO_DEBUG_STREAM) || defined(Q_CLANG_QDOC)
Q_GUI_EXPORT QDebug operator<<(QDebug, const QKeySequence &);
#endif

QT_END_NAMESPACE

#endif // QKEYSEQUENCE_H
