// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2013 Ivan Komissarov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qkeysequenceedit.h"
#include "qkeysequenceedit_p.h"

#include "qboxlayout.h"
#include "qlineedit.h"
#include <private/qkeymapper_p.h>

QT_BEGIN_NAMESPACE

static_assert(QKeySequencePrivate::MaxKeyCount == 4); // assumed by the code around here

void QKeySequenceEditPrivate::init()
{
    Q_Q(QKeySequenceEdit);

    lineEdit = new QLineEdit(q);
    lineEdit->setObjectName(QStringLiteral("qt_keysequenceedit_lineedit"));
    lineEdit->setClearButtonEnabled(false);
    q->connect(lineEdit, &QLineEdit::textChanged, [q](const QString& text) {
        // Clear the shortcut if the user clicked on the clear icon
        if (text.isEmpty())
            q->clear();
    });

    keyNum = 0;
    prevKey = -1;
    releaseTimer = 0;
    finishingKeyCombinations = {Qt::Key_Tab, Qt::Key_Backtab};

    QVBoxLayout *layout = new QVBoxLayout(q);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(lineEdit);

    std::fill_n(key, QKeySequencePrivate::MaxKeyCount, QKeyCombination::fromCombined(0));

    lineEdit->setFocusProxy(q);
    lineEdit->installEventFilter(q);
    resetState();

    q->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    q->setFocusPolicy(Qt::StrongFocus);
    q->setAttribute(Qt::WA_MacShowFocusRect, true);
    q->setAttribute(Qt::WA_InputMethodEnabled, false);
}

int QKeySequenceEditPrivate::translateModifiers(Qt::KeyboardModifiers state, const QString &text)
{
    Q_UNUSED(text);
    int result = 0;
    if (state & Qt::ControlModifier)
        result |= Qt::CTRL;
    if (state & Qt::MetaModifier)
        result |= Qt::META;
    if (state & Qt::AltModifier)
        result |= Qt::ALT;
    return result;
}

void QKeySequenceEditPrivate::resetState()
{
    Q_Q(QKeySequenceEdit);

    if (releaseTimer) {
        q->killTimer(releaseTimer);
        releaseTimer = 0;
    }
    prevKey = -1;
    lineEdit->setText(keySequence.toString(QKeySequence::NativeText));
    lineEdit->setPlaceholderText(QKeySequenceEdit::tr("Press shortcut"));
}

void QKeySequenceEditPrivate::finishEditing()
{
    Q_Q(QKeySequenceEdit);

    resetState();
    emit q->keySequenceChanged(keySequence);
    emit q->editingFinished();
}

/*!
    \class QKeySequenceEdit
    \brief The QKeySequenceEdit widget allows to input a QKeySequence.

    \inmodule QtWidgets

    \since 5.2

    This widget lets the user choose a QKeySequence, which is usually used as
    a shortcut. The recording is initiated when the widget receives the focus
    and ends one second after the user releases the last key.

    \sa QKeySequenceEdit::keySequence
*/

/*!
    Constructs a QKeySequenceEdit widget with the given \a parent.
*/
QKeySequenceEdit::QKeySequenceEdit(QWidget *parent)
    : QKeySequenceEdit(*new QKeySequenceEditPrivate, parent, { })
{
}

/*!
    Constructs a QKeySequenceEdit widget with the given \a keySequence and \a parent.
*/
QKeySequenceEdit::QKeySequenceEdit(const QKeySequence &keySequence, QWidget *parent)
    : QKeySequenceEdit(parent)
{
    setKeySequence(keySequence);
}

/*!
    \internal
*/
QKeySequenceEdit::QKeySequenceEdit(QKeySequenceEditPrivate &dd, QWidget *parent, Qt::WindowFlags f) :
    QWidget(dd, parent, f)
{
    Q_D(QKeySequenceEdit);
    d->init();
}

/*!
    Destroys the QKeySequenceEdit object.
*/
QKeySequenceEdit::~QKeySequenceEdit()
{
}

/*!
    \property QKeySequenceEdit::keySequence

    \brief This property contains the currently chosen key sequence.

    The shortcut can be changed by the user or via setter function.

    \note If the QKeySequence is longer than the maximumSequenceLength
    property, the key sequence is truncated.
*/
QKeySequence QKeySequenceEdit::keySequence() const
{
    Q_D(const QKeySequenceEdit);

    return d->keySequence;
}

/*!
    \property QKeySequenceEdit::clearButtonEnabled
    \brief Whether the key sequence edit displays a clear button when it is not
    empty.

    If enabled, the key sequence edit displays a trailing \e clear button when
    it contains some text, otherwise the line edit does not show a clear button
    (the default).

    \since 6.4
*/
void QKeySequenceEdit::setClearButtonEnabled(bool enable)
{
    Q_D(QKeySequenceEdit);

    d->lineEdit->setClearButtonEnabled(enable);
}

bool QKeySequenceEdit::isClearButtonEnabled() const
{
    Q_D(const QKeySequenceEdit);

    return d->lineEdit->isClearButtonEnabled();
}

/*!
    \property QKeySequenceEdit::maximumSequenceLength
    \brief The maximum sequence length.

    The maximum number of key sequences a user can enter. The value needs to
    be between 1 and 4, with 4 being the default.

    \since 6.5
*/
qsizetype QKeySequenceEdit::maximumSequenceLength() const
{
    Q_D(const QKeySequenceEdit);
    return d->maximumSequenceLength;
}

void QKeySequenceEdit::setMaximumSequenceLength(qsizetype count)
{
    Q_D(QKeySequenceEdit);

    if (count < 1 || count > QKeySequencePrivate::MaxKeyCount) {
        qWarning("QKeySequenceEdit: maximumSequenceLength %lld is out of range (1..%d)",
                 qlonglong(count), QKeySequencePrivate::MaxKeyCount);
        return;
    }
    d->maximumSequenceLength = int(count);
    if (d->keyNum > count) {
        for (qsizetype i = d->keyNum; i < count; ++i)
            d->key[i] = QKeyCombination::fromCombined(0);
        d->keyNum = count;
        d->rebuildKeySequence();
    }
}

/*!
    \property QKeySequenceEdit::finishingKeyCombinations
    \brief The list of key combinations that finish editing the key sequences.

    Any combination in the list will finish the editing of key sequences.
    All other key combinations can be recorded as part of a key sequence. By
    default, Qt::Key_Tab and Qt::Key_Backtab will finish recording the key
    sequence.

    \since 6.5
*/
void QKeySequenceEdit::setFinishingKeyCombinations(const QList<QKeyCombination> &finishingKeyCombinations)
{
    Q_D(QKeySequenceEdit);

    d->finishingKeyCombinations = finishingKeyCombinations;
}

QList<QKeyCombination> QKeySequenceEdit::finishingKeyCombinations() const
{
    Q_D(const QKeySequenceEdit);

    return d->finishingKeyCombinations;
}

void QKeySequenceEdit::setKeySequence(const QKeySequence &keySequence)
{
    Q_D(QKeySequenceEdit);

    d->resetState();

    if (d->keySequence == keySequence)
        return;

    const auto desiredCount = keySequence.count();
    if (desiredCount > d->maximumSequenceLength) {
        qWarning("QKeySequenceEdit: setting a key sequence of length %d "
                 "when maximumSequenceLength is %d, truncating.",
                 desiredCount, d->maximumSequenceLength);
    }

    d->keyNum = std::min(desiredCount, d->maximumSequenceLength);
    for (int i = 0; i < d->keyNum; ++i)
        d->key[i] = keySequence[i];
    for (int i = d->keyNum; i < QKeySequencePrivate::MaxKeyCount; ++i)
        d->key[i] = QKeyCombination::fromCombined(0);

    d->rebuildKeySequence();

    d->lineEdit->setText(d->keySequence.toString(QKeySequence::NativeText));

    emit keySequenceChanged(d->keySequence);
}

/*!
    \fn void QKeySequenceEdit::editingFinished()

    This signal is emitted when the user finishes entering the shortcut.

    \note there is a one second delay before releasing the last key and
    emitting this signal.
*/

/*!
    \brief Clears the current key sequence.
*/
void QKeySequenceEdit::clear()
{
    setKeySequence(QKeySequence());
}

/*!
    \reimp
*/
bool QKeySequenceEdit::event(QEvent *e)
{
    Q_D(const QKeySequenceEdit);

    switch (e->type()) {
    case QEvent::Shortcut:
        return true;
    case QEvent::ShortcutOverride:
        e->accept();
        return true;
    case QEvent::KeyPress: {
            QKeyEvent *ke = static_cast<QKeyEvent *>(e);
            if (!d->finishingKeyCombinations.contains(ke->keyCombination())) {
                keyPressEvent(ke);
                return true;
            }
        }
        break;
    default:
        break;
    }

    return QWidget::event(e);
}

/*!
    \reimp
*/
void QKeySequenceEdit::keyPressEvent(QKeyEvent *e)
{
    Q_D(QKeySequenceEdit);

    if (d->finishingKeyCombinations.contains(e->keyCombination())) {
        d->finishEditing();
        return;
    }

    int nextKey = e->key();

    if (d->prevKey == -1) {
        clear();
        d->prevKey = nextKey;
    }

    d->lineEdit->setPlaceholderText(QString());
    if (nextKey == Qt::Key_Control
            || nextKey == Qt::Key_Shift
            || nextKey == Qt::Key_Meta
            || nextKey == Qt::Key_Alt
            || nextKey == Qt::Key_unknown) {
        return;
    }

    QString selectedText = d->lineEdit->selectedText();
    if (!selectedText.isEmpty() && selectedText == d->lineEdit->text()) {
        clear();
        if (nextKey == Qt::Key_Backspace)
            return;
    }

    if (d->keyNum >= d->maximumSequenceLength)
        return;

    if (e->modifiers() & Qt::ShiftModifier) {
        QList<int> possibleKeys = QKeyMapper::possibleKeys(e);
        int pkTotal = possibleKeys.size();
        if (!pkTotal)
            return;
        bool found = false;
        for (int i = 0; i < possibleKeys.size(); ++i) {
            if (possibleKeys.at(i) - nextKey == int(e->modifiers())
                || (possibleKeys.at(i) == nextKey && e->modifiers() == Qt::ShiftModifier)) {
                nextKey = possibleKeys.at(i);
                found = true;
                break;
            }
        }
        // Use as fallback
        if (!found)
            nextKey = possibleKeys.first();
    } else {
        nextKey |= d->translateModifiers(e->modifiers(), e->text());
    }


    d->key[d->keyNum] = QKeyCombination::fromCombined(nextKey);
    d->keyNum++;

    d->rebuildKeySequence();
    QString text = d->keySequence.toString(QKeySequence::NativeText);
    if (d->keyNum < d->maximumSequenceLength) {
        //: This text is an "unfinished" shortcut, expands like "Ctrl+A, ..."
        text = tr("%1, ...").arg(text);
    }
    d->lineEdit->setText(text);
    e->accept();
}

/*!
    \reimp
*/
void QKeySequenceEdit::keyReleaseEvent(QKeyEvent *e)
{
    Q_D(QKeySequenceEdit);

    if (d->prevKey == e->key()) {
        if (d->keyNum < d->maximumSequenceLength)
            d->releaseTimer = startTimer(1000);
        else
            d->finishEditing();
    }
    e->accept();
}

/*!
    \reimp
*/
void QKeySequenceEdit::timerEvent(QTimerEvent *e)
{
    Q_D(QKeySequenceEdit);
    if (e->timerId() == d->releaseTimer) {
        d->finishEditing();
        return;
    }

    QWidget::timerEvent(e);
}

/*!
    \reimp
*/
void QKeySequenceEdit::focusOutEvent(QFocusEvent *e)
{
    Q_D(QKeySequenceEdit);
    if (e->reason() != Qt::PopupFocusReason)
        d->finishEditing();
    QWidget::focusOutEvent(e);
}

QT_END_NAMESPACE

#include "moc_qkeysequenceedit.cpp"
