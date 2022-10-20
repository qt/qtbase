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

#ifndef QTEXTOBJECT_H
#define QTEXTOBJECT_H

#include <QtGui/qtguiglobal.h>
#include <QtCore/qobject.h>
#include <QtGui/qtextformat.h>
#include <QtGui/qtextlayout.h>
#include <QtGui/qglyphrun.h>

QT_BEGIN_NAMESPACE


class QTextObjectPrivate;
class QTextDocument;
class QTextDocumentPrivate;
class QTextCursor;
class QTextBlock;
class QTextFragment;
class QTextList;

class Q_GUI_EXPORT QTextObject : public QObject
{
    Q_OBJECT

protected:
    explicit QTextObject(QTextDocument *doc);
    ~QTextObject();

    void setFormat(const QTextFormat &format);

public:
    QTextFormat format() const;
    int formatIndex() const;

    QTextDocument *document() const;

    int objectIndex() const;

protected:
    QTextObject(QTextObjectPrivate &p, QTextDocument *doc);

private:
    Q_DECLARE_PRIVATE(QTextObject)
    Q_DISABLE_COPY(QTextObject)
    friend class QTextDocumentPrivate;
};

class QTextBlockGroupPrivate;
class Q_GUI_EXPORT QTextBlockGroup : public QTextObject
{
    Q_OBJECT

protected:
    explicit QTextBlockGroup(QTextDocument *doc);
    ~QTextBlockGroup();

    virtual void blockInserted(const QTextBlock &block);
    virtual void blockRemoved(const QTextBlock &block);
    virtual void blockFormatChanged(const QTextBlock &block);

    QList<QTextBlock> blockList() const;

protected:
    QTextBlockGroup(QTextBlockGroupPrivate &p, QTextDocument *doc);
private:
    Q_DECLARE_PRIVATE(QTextBlockGroup)
    Q_DISABLE_COPY(QTextBlockGroup)
    friend class QTextDocumentPrivate;
};

class Q_GUI_EXPORT QTextFrameLayoutData {
public:
    virtual ~QTextFrameLayoutData();
};

class QTextFramePrivate;
class Q_GUI_EXPORT QTextFrame : public QTextObject
{
    Q_OBJECT

public:
    explicit QTextFrame(QTextDocument *doc);
    ~QTextFrame();

    inline void setFrameFormat(const QTextFrameFormat &format);
    QTextFrameFormat frameFormat() const { return QTextObject::format().toFrameFormat(); }

    QTextCursor firstCursorPosition() const;
    QTextCursor lastCursorPosition() const;
    int firstPosition() const;
    int lastPosition() const;

    QTextFrameLayoutData *layoutData() const;
    void setLayoutData(QTextFrameLayoutData *data);

    QList<QTextFrame *> childFrames() const;
    QTextFrame *parentFrame() const;

    class iterator {
        QTextFrame *f = nullptr;
        int b = 0;
        int e = 0;
        QTextFrame *cf = nullptr;
        int cb = 0;

        friend class QTextFrame;
        friend class QTextTableCell;
        friend class QTextDocumentLayoutPrivate;
        inline iterator(QTextFrame *frame, int block, int begin, int end)
            : f(frame), b(begin), e(end), cb(block)
        {}
    public:
        constexpr iterator() noexcept = default;
        QTextFrame *parentFrame() const { return f; }

        QTextFrame *currentFrame() const { return cf; }
        Q_GUI_EXPORT QTextBlock currentBlock() const;

        bool atEnd() const { return !cf && cb == e; }

        inline bool operator==(const iterator &o) const { return f == o.f && cf == o.cf && cb == o.cb; }
        inline bool operator!=(const iterator &o) const { return f != o.f || cf != o.cf || cb != o.cb; }
        Q_GUI_EXPORT iterator &operator++();
        inline iterator operator++(int) { iterator tmp = *this; operator++(); return tmp; }
        Q_GUI_EXPORT iterator &operator--();
        inline iterator operator--(int) { iterator tmp = *this; operator--(); return tmp; }
    };

    friend class iterator;
    // more Qt
    typedef iterator Iterator;

    iterator begin() const;
    iterator end() const;

protected:
    QTextFrame(QTextFramePrivate &p, QTextDocument *doc);
private:
    friend class QTextDocumentPrivate;
    Q_DECLARE_PRIVATE(QTextFrame)
    Q_DISABLE_COPY(QTextFrame)
};
Q_DECLARE_TYPEINFO(QTextFrame::iterator, Q_RELOCATABLE_TYPE);

inline void QTextFrame::setFrameFormat(const QTextFrameFormat &aformat)
{ QTextObject::setFormat(aformat); }

class Q_GUI_EXPORT QTextBlockUserData {
public:
    virtual ~QTextBlockUserData();
};

class Q_GUI_EXPORT QTextBlock
{
    friend class QSyntaxHighlighter;
public:
    inline QTextBlock(QTextDocumentPrivate *priv, int b) : p(priv), n(b) {}
    inline QTextBlock() : p(nullptr), n(0) {}
    inline QTextBlock(const QTextBlock &o) : p(o.p), n(o.n) {}
    inline QTextBlock &operator=(const QTextBlock &o) { p = o.p; n = o.n; return *this; }

    bool isValid() const;

    inline bool operator==(const QTextBlock &o) const { return p == o.p && n == o.n; }
    inline bool operator!=(const QTextBlock &o) const { return p != o.p || n != o.n; }
    inline bool operator<(const QTextBlock &o) const { return position() < o.position(); }

    int position() const;
    int length() const;
    bool contains(int position) const;

    QTextLayout *layout() const;
    void clearLayout();
    QTextBlockFormat blockFormat() const;
    int blockFormatIndex() const;
    QTextCharFormat charFormat() const;
    int charFormatIndex() const;

    Qt::LayoutDirection textDirection() const;

    QString text() const;

    QList<QTextLayout::FormatRange> textFormats() const;

    const QTextDocument *document() const;

    QTextList *textList() const;

    QTextBlockUserData *userData() const;
    void setUserData(QTextBlockUserData *data);

    int userState() const;
    void setUserState(int state);

    int revision() const;
    void setRevision(int rev);

    bool isVisible() const;
    void setVisible(bool visible);

    int blockNumber() const;
    int firstLineNumber() const;

    void setLineCount(int count);
    int lineCount() const;

    class iterator {
        const QTextDocumentPrivate *p = nullptr;
        int b = 0;
        int e = 0;
        int n = 0;
        friend class QTextBlock;
        iterator(const QTextDocumentPrivate *priv, int begin, int end, int f)
            : p(priv), b(begin), e(end), n(f) {}
    public:
        constexpr iterator() = default;

        Q_GUI_EXPORT QTextFragment fragment() const;

        bool atEnd() const { return n == e; }

        inline bool operator==(const iterator &o) const { return p == o.p && n == o.n; }
        inline bool operator!=(const iterator &o) const { return p != o.p || n != o.n; }
        Q_GUI_EXPORT iterator &operator++();
        inline iterator operator++(int) { iterator tmp = *this; operator++(); return tmp; }
        Q_GUI_EXPORT iterator &operator--();
        inline iterator operator--(int) { iterator tmp = *this; operator--(); return tmp; }
    };

    // more Qt
    typedef iterator Iterator;

    iterator begin() const;
    iterator end() const;

    QTextBlock next() const;
    QTextBlock previous() const;

    inline int fragmentIndex() const { return n; }

private:
    QTextDocumentPrivate *p;
    int n;
    friend class QTextDocumentPrivate;
    friend class QTextLayout;
};

Q_DECLARE_TYPEINFO(QTextBlock, Q_RELOCATABLE_TYPE);
Q_DECLARE_TYPEINFO(QTextBlock::iterator, Q_RELOCATABLE_TYPE);


class Q_GUI_EXPORT QTextFragment
{
public:
    inline QTextFragment(const QTextDocumentPrivate *priv, int f, int fe) : p(priv), n(f), ne(fe) {}
    inline QTextFragment() : p(nullptr), n(0), ne(0) {}
    inline QTextFragment(const QTextFragment &o) : p(o.p), n(o.n), ne(o.ne) {}
    inline QTextFragment &operator=(const QTextFragment &o) { p = o.p; n = o.n; ne = o.ne; return *this; }

    inline bool isValid() const { return p && n; }

    inline bool operator==(const QTextFragment &o) const { return p == o.p && n == o.n; }
    inline bool operator!=(const QTextFragment &o) const { return p != o.p || n != o.n; }
    inline bool operator<(const QTextFragment &o) const { return position() < o.position(); }

    int position() const;
    int length() const;
    bool contains(int position) const;

    QTextCharFormat charFormat() const;
    int charFormatIndex() const;
    QString text() const;

#if !defined(QT_NO_RAWFONT)
    QList<QGlyphRun> glyphRuns(int from = -1, int length = -1) const;
#endif

private:
    const QTextDocumentPrivate *p;
    int n;
    int ne;
};

Q_DECLARE_TYPEINFO(QTextFragment, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QTEXTOBJECT_H
