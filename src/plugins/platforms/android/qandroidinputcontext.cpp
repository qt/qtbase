/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include <android/log.h>

#include "qandroidinputcontext.h"
#include "androidjnimain.h"
#include "androidjniinput.h"
#include "qandroideventdispatcher.h"
#include "androiddeadlockprotector.h"
#include "qandroidplatformintegration.h"
#include <QDebug>
#include <qevent.h>
#include <qguiapplication.h>
#include <qsharedpointer.h>
#include <qthread.h>
#include <qinputmethod.h>
#include <qwindow.h>
#include <QtCore/private/qjni_p.h>
#include <private/qhighdpiscaling_p.h>

#include <QTextCharFormat>
#include <QTextBoundaryFinder>

#include <QDebug>

QT_BEGIN_NAMESPACE

static QAndroidInputContext *m_androidInputContext = 0;
static char const *const QtNativeInputConnectionClassName = "org/qtproject/qt5/android/QtNativeInputConnection";
static char const *const QtExtractedTextClassName = "org/qtproject/qt5/android/QtExtractedText";
static jclass m_extractedTextClass = 0;
static jmethodID m_classConstructorMethodID = 0;
static jfieldID m_partialEndOffsetFieldID = 0;
static jfieldID m_partialStartOffsetFieldID = 0;
static jfieldID m_selectionEndFieldID = 0;
static jfieldID m_selectionStartFieldID = 0;
static jfieldID m_startOffsetFieldID = 0;
static jfieldID m_textFieldID = 0;

static jboolean beginBatchEdit(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ BEGINBATCH");
#endif

    return m_androidInputContext->beginBatchEdit();

    return JNI_TRUE;
}

static jboolean endBatchEdit(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ ENDBATCH");
#endif

    return m_androidInputContext->endBatchEdit();

    return JNI_TRUE;
}


static jboolean commitText(JNIEnv *env, jobject /*thiz*/, jstring text, jint newCursorPosition)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    jboolean isCopy;
    const jchar *jstr = env->GetStringChars(text, &isCopy);
    QString str(reinterpret_cast<const QChar *>(jstr), env->GetStringLength(text));
    env->ReleaseStringChars(text, jstr);

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ COMMIT" << str << newCursorPosition;
#endif
    return m_androidInputContext->commitText(str, newCursorPosition);
}

static jboolean deleteSurroundingText(JNIEnv */*env*/, jobject /*thiz*/, jint leftLength, jint rightLength)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ DELETE" << leftLength << rightLength;
#endif
    return m_androidInputContext->deleteSurroundingText(leftLength, rightLength);
}

static jboolean finishComposingText(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ FINISH");
#endif
    return m_androidInputContext->finishComposingText();
}

static jint getCursorCapsMode(JNIEnv */*env*/, jobject /*thiz*/, jint reqModes)
{
    if (!m_androidInputContext)
        return 0;

    return m_androidInputContext->getCursorCapsMode(reqModes);
}

static jobject getExtractedText(JNIEnv *env, jobject /*thiz*/, int hintMaxChars, int hintMaxLines, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    const QAndroidInputContext::ExtractedText &extractedText =
            m_androidInputContext->getExtractedText(hintMaxChars, hintMaxLines, flags);

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ GETEX" << hintMaxChars << hintMaxLines << QString::fromLatin1("0x") + QString::number(flags,16) << extractedText.text << "partOff:" << extractedText.partialStartOffset << extractedText.partialEndOffset << "sel:" << extractedText.selectionStart << extractedText.selectionEnd << "offset:" << extractedText.startOffset;
#endif

    jobject object = env->NewObject(m_extractedTextClass, m_classConstructorMethodID);
    env->SetIntField(object, m_partialStartOffsetFieldID, extractedText.partialStartOffset);
    env->SetIntField(object, m_partialEndOffsetFieldID, extractedText.partialEndOffset);
    env->SetIntField(object, m_selectionStartFieldID, extractedText.selectionStart);
    env->SetIntField(object, m_selectionEndFieldID, extractedText.selectionEnd);
    env->SetIntField(object, m_startOffsetFieldID, extractedText.startOffset);
    env->SetObjectField(object,
                        m_textFieldID,
                        env->NewString(reinterpret_cast<const jchar *>(extractedText.text.constData()),
                                       jsize(extractedText.text.length())));

    return object;
}

static jstring getSelectedText(JNIEnv *env, jobject /*thiz*/, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    const QString &text = m_androidInputContext->getSelectedText(flags);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ GETSEL" << text;
#endif
    if (text.isEmpty())
        return 0;
    return env->NewString(reinterpret_cast<const jchar *>(text.constData()), jsize(text.length()));
}

static jstring getTextAfterCursor(JNIEnv *env, jobject /*thiz*/, jint length, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    const QString &text = m_androidInputContext->getTextAfterCursor(length, flags);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ GETA" << length << text;
#endif
    return env->NewString(reinterpret_cast<const jchar *>(text.constData()), jsize(text.length()));
}

static jstring getTextBeforeCursor(JNIEnv *env, jobject /*thiz*/, jint length, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    const QString &text = m_androidInputContext->getTextBeforeCursor(length, flags);
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ GETB" << length << text;
#endif
    return env->NewString(reinterpret_cast<const jchar *>(text.constData()), jsize(text.length()));
}

static jboolean setComposingText(JNIEnv *env, jobject /*thiz*/, jstring text, jint newCursorPosition)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    jboolean isCopy;
    const jchar *jstr = env->GetStringChars(text, &isCopy);
    QString str(reinterpret_cast<const QChar *>(jstr), env->GetStringLength(text));
    env->ReleaseStringChars(text, jstr);

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ SET" << str << newCursorPosition;
#endif
    return m_androidInputContext->setComposingText(str, newCursorPosition);
}

static jboolean setComposingRegion(JNIEnv */*env*/, jobject /*thiz*/, jint start, jint end)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ SETR" << start << end;
#endif
    return m_androidInputContext->setComposingRegion(start, end);
}


static jboolean setSelection(JNIEnv */*env*/, jobject /*thiz*/, jint start, jint end)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ SETSEL" << start << end;
#endif
    return m_androidInputContext->setSelection(start, end);
}

static jboolean selectAll(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ SELALL");
#endif
    return m_androidInputContext->selectAll();
}

static jboolean cut(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@");
#endif
    return m_androidInputContext->cut();
}

static jboolean copy(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@");
#endif
    return m_androidInputContext->copy();
}

static jboolean copyURL(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@");
#endif
    return m_androidInputContext->copyURL();
}

static jboolean paste(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ PASTE");
#endif
    return m_androidInputContext->paste();
}

static jboolean updateCursorPosition(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ UPDATECURSORPOS");
#endif
    m_androidInputContext->updateCursorPosition();
    return true;
}


static JNINativeMethod methods[] = {
    {"beginBatchEdit", "()Z", (void *)beginBatchEdit},
    {"endBatchEdit", "()Z", (void *)endBatchEdit},
    {"commitText", "(Ljava/lang/String;I)Z", (void *)commitText},
    {"deleteSurroundingText", "(II)Z", (void *)deleteSurroundingText},
    {"finishComposingText", "()Z", (void *)finishComposingText},
    {"getCursorCapsMode", "(I)I", (void *)getCursorCapsMode},
    {"getExtractedText", "(III)Lorg/qtproject/qt5/android/QtExtractedText;", (void *)getExtractedText},
    {"getSelectedText", "(I)Ljava/lang/String;", (void *)getSelectedText},
    {"getTextAfterCursor", "(II)Ljava/lang/String;", (void *)getTextAfterCursor},
    {"getTextBeforeCursor", "(II)Ljava/lang/String;", (void *)getTextBeforeCursor},
    {"setComposingText", "(Ljava/lang/String;I)Z", (void *)setComposingText},
    {"setComposingRegion", "(II)Z", (void *)setComposingRegion},
    {"setSelection", "(II)Z", (void *)setSelection},
    {"selectAll", "()Z", (void *)selectAll},
    {"cut", "()Z", (void *)cut},
    {"copy", "()Z", (void *)copy},
    {"copyURL", "()Z", (void *)copyURL},
    {"paste", "()Z", (void *)paste},
    {"updateCursorPosition", "()Z", (void *)updateCursorPosition}
};

static QRect inputItemRectangle()
{
    QRectF itemRect = qGuiApp->inputMethod()->inputItemRectangle();
    QRect rect = qGuiApp->inputMethod()->inputItemTransform().mapRect(itemRect).toRect();
    QWindow *window = qGuiApp->focusWindow();
    if (window)
        rect = QRect(window->mapToGlobal(rect.topLeft()), rect.size());
    double pixelDensity = window
        ? QHighDpiScaling::factor(window)
        : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());
    if (pixelDensity != 1.0) {
        rect.setRect(rect.x() * pixelDensity,
                     rect.y() * pixelDensity,
                     rect.width() * pixelDensity,
                     rect.height() * pixelDensity);
    }
    return rect;
}

QAndroidInputContext::QAndroidInputContext()
    : QPlatformInputContext(), m_composingTextStart(-1), m_blockUpdateSelection(false),
    m_cursorHandleShown(CursorHandleNotShown), m_batchEditNestingLevel(0), m_focusObject(0)
{
    jclass clazz = QJNIEnvironmentPrivate::findClass(QtNativeInputConnectionClassName);
    if (Q_UNLIKELY(!clazz)) {
        qCritical() << "Native registration unable to find class '"
                    << QtNativeInputConnectionClassName
                    << '\'';
        return;
    }

    QJNIEnvironmentPrivate env;
    if (Q_UNLIKELY(env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0)) {
        qCritical() << "RegisterNatives failed for '"
                    << QtNativeInputConnectionClassName
                    << '\'';
        return;
    }

    clazz = QJNIEnvironmentPrivate::findClass(QtExtractedTextClassName);
    if (Q_UNLIKELY(!clazz)) {
        qCritical() << "Native registration unable to find class '"
                    << QtExtractedTextClassName
                    << '\'';
        return;
    }

    m_extractedTextClass = static_cast<jclass>(env->NewGlobalRef(clazz));
    m_classConstructorMethodID = env->GetMethodID(m_extractedTextClass, "<init>", "()V");
    if (Q_UNLIKELY(!m_classConstructorMethodID)) {
        qCritical("GetMethodID failed");
        return;
    }

    m_partialEndOffsetFieldID = env->GetFieldID(m_extractedTextClass, "partialEndOffset", "I");
    if (Q_UNLIKELY(!m_partialEndOffsetFieldID)) {
        qCritical("Can't find field partialEndOffset");
        return;
    }

    m_partialStartOffsetFieldID = env->GetFieldID(m_extractedTextClass, "partialStartOffset", "I");
    if (Q_UNLIKELY(!m_partialStartOffsetFieldID)) {
        qCritical("Can't find field partialStartOffset");
        return;
    }

    m_selectionEndFieldID = env->GetFieldID(m_extractedTextClass, "selectionEnd", "I");
    if (Q_UNLIKELY(!m_selectionEndFieldID)) {
        qCritical("Can't find field selectionEnd");
        return;
    }

    m_selectionStartFieldID = env->GetFieldID(m_extractedTextClass, "selectionStart", "I");
    if (Q_UNLIKELY(!m_selectionStartFieldID)) {
        qCritical("Can't find field selectionStart");
        return;
    }

    m_startOffsetFieldID = env->GetFieldID(m_extractedTextClass, "startOffset", "I");
    if (Q_UNLIKELY(!m_startOffsetFieldID)) {
        qCritical("Can't find field startOffset");
        return;
    }

    m_textFieldID = env->GetFieldID(m_extractedTextClass, "text", "Ljava/lang/String;");
    if (Q_UNLIKELY(!m_textFieldID)) {
        qCritical("Can't find field text");
        return;
    }
    qRegisterMetaType<QInputMethodEvent *>("QInputMethodEvent*");
    qRegisterMetaType<QInputMethodQueryEvent *>("QInputMethodQueryEvent*");
    m_androidInputContext = this;

    QObject::connect(QGuiApplication::inputMethod(), &QInputMethod::cursorRectangleChanged,
                     this, &QAndroidInputContext::updateSelectionHandles);
    QObject::connect(QGuiApplication::inputMethod(), &QInputMethod::anchorRectangleChanged,
                     this, &QAndroidInputContext::updateSelectionHandles);
    QObject::connect(QGuiApplication::inputMethod(), &QInputMethod::inputItemClipRectangleChanged, this, [this]{
        auto im = qGuiApp->inputMethod();
        if (!im->inputItemClipRectangle().contains(im->anchorRectangle()) ||
                !im->inputItemClipRectangle().contains(im->cursorRectangle())) {
            m_cursorHandleShown = CursorHandleNotShown;
            updateSelectionHandles();
        }
    });
}

QAndroidInputContext::~QAndroidInputContext()
{
    m_androidInputContext = 0;
    m_extractedTextClass = 0;
    m_partialEndOffsetFieldID = 0;
    m_partialStartOffsetFieldID = 0;
    m_selectionEndFieldID = 0;
    m_selectionStartFieldID = 0;
    m_startOffsetFieldID = 0;
    m_textFieldID = 0;
}

QAndroidInputContext *QAndroidInputContext::androidInputContext()
{
    return m_androidInputContext;
}

// cursor position getter that also works with editors that have not been updated to the new API
static inline int getAbsoluteCursorPosition(const QSharedPointer<QInputMethodQueryEvent> &query)
{
    QVariant absolutePos = query->value(Qt::ImAbsolutePosition);
    return absolutePos.isValid() ? absolutePos.toInt() : query->value(Qt::ImCursorPosition).toInt();
}

// position of the start of the current block
static inline int getBlockPosition(const QSharedPointer<QInputMethodQueryEvent> &query)
{
    QVariant absolutePos = query->value(Qt::ImAbsolutePosition);
    return  absolutePos.isValid() ? absolutePos.toInt() - query->value(Qt::ImCursorPosition).toInt() : 0;
}

void QAndroidInputContext::reset()
{
    clear();
    m_batchEditNestingLevel = 0;
    m_cursorHandleShown = QAndroidInputContext::CursorHandleNotShown;
    if (qGuiApp->focusObject()) {
        QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe(Qt::ImEnabled);
        if (!query.isNull() && query->value(Qt::ImEnabled).toBool()) {
            QtAndroidInput::resetSoftwareKeyboard();
            return;
        }
    }
    QtAndroidInput::hideSoftwareKeyboard();
}

void QAndroidInputContext::commit()
{
    finishComposingText();
}

void QAndroidInputContext::updateCursorPosition()
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (!query.isNull() && !m_blockUpdateSelection && !m_batchEditNestingLevel) {
        const int cursorPos = getAbsoluteCursorPosition(query);
        const int composeLength = m_composingText.length();

        //Q_ASSERT(m_composingText.isEmpty() == (m_composingTextStart == -1));
        if (m_composingText.isEmpty() != (m_composingTextStart == -1))
            qWarning() << "Input method out of sync" << m_composingText << m_composingTextStart;

        int realCursorPosition = cursorPos;
        int realAnchorPosition = cursorPos;

        int cpos = query->value(Qt::ImCursorPosition).toInt();
        int anchor = query->value(Qt::ImAnchorPosition).toInt();
        if (cpos != anchor) {
            if (!m_composingText.isEmpty()) {
                qWarning("Selecting text while preediting may give unpredictable results.");
                finishComposingText();
            }
            int blockPos = getBlockPosition(query);
            realCursorPosition = blockPos + cpos;
            realAnchorPosition = blockPos + anchor;
        }
        // Qt's idea of the cursor position is the start of the preedit area, so we maintain our own preedit cursor pos
        if (!m_composingText.isEmpty())
            realCursorPosition = realAnchorPosition = m_composingCursor;
        QtAndroidInput::updateSelection(realCursorPosition, realAnchorPosition,
                                        m_composingTextStart, m_composingTextStart + composeLength); // pre-edit text
    }
}

void QAndroidInputContext::updateSelectionHandles()
{
    static bool noHandles = qEnvironmentVariableIntValue("QT_QPA_NO_TEXT_HANDLES");
    if (noHandles)
        return;

    auto im = qGuiApp->inputMethod();
    if (!m_focusObject || (m_cursorHandleShown == CursorHandleNotShown)) {
        // Hide the handles
        QtAndroidInput::updateHandles(0);
        return;
    }
    QWindow *window = qGuiApp->focusWindow();
    double pixelDensity = window
        ? QHighDpiScaling::factor(window)
        : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImAnchorPosition | Qt::ImEnabled | Qt::ImCurrentSelection);
    QCoreApplication::sendEvent(m_focusObject, &query);
    int cpos = query.value(Qt::ImCursorPosition).toInt();
    int anchor = query.value(Qt::ImAnchorPosition).toInt();

    if (cpos == anchor || im->anchorRectangle().isNull()) {
        if (!query.value(Qt::ImEnabled).toBool()) {
            QtAndroidInput::updateHandles(0);
            return;
        }

        auto curRect = im->cursorRectangle();
        QPoint cursorPoint(curRect.center().x(), curRect.bottom());
        QPoint editMenuPoint(curRect.center().x(), curRect.top());
        QtAndroidInput::updateHandles(m_cursorHandleShown, cursorPoint * pixelDensity,
                                      editMenuPoint * pixelDensity);
        return;
    }

    auto leftRect = im->cursorRectangle();
    auto rightRect = im->anchorRectangle();
    if (cpos > anchor)
        std::swap(leftRect, rightRect);

    QPoint leftPoint(leftRect.bottomLeft().toPoint() * pixelDensity);
    QPoint righPoint(rightRect.bottomRight().toPoint() * pixelDensity);
    QtAndroidInput::updateHandles(CursorHandleShowSelection, leftPoint, righPoint,
                                  query.value(Qt::ImCurrentSelection).toString().isRightToLeft());

    if (m_cursorHandleShown == CursorHandleShowPopup) {
        // make sure the popup does not reappear when the selection menu closes
        m_cursorHandleShown = QAndroidInputContext::CursorHandleNotShown;
    }
}

/*
   Called from Java when a cursor/selection handle was dragged to a new position

   handleId of 1 means the cursor handle,  2 means the left handle, 3 means the right handle
 */
void QAndroidInputContext::handleLocationChanged(int handleId, int x, int y)
{
    if (m_batchEditNestingLevel.load() || m_blockUpdateSelection)
        return;

    finishComposingText();

    auto im = qGuiApp->inputMethod();
    auto leftRect = im->cursorRectangle();
    // The handle is down of the cursor, but we want the position in the middle.
    QWindow *window = qGuiApp->focusWindow();
    double pixelDensity = window
        ? QHighDpiScaling::factor(window)
        : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());
    QPointF point(x / pixelDensity, y / pixelDensity);
    point.setY(point.y() - leftRect.width() / 2);
    if (handleId == 1) {
        setSelectionOnFocusObject(point, point);
        return;
    }

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImAnchorPosition | Qt::ImCurrentSelection);
    QCoreApplication::sendEvent(m_focusObject, &query);
    int cpos = query.value(Qt::ImCursorPosition).toInt();
    int anchor = query.value(Qt::ImAnchorPosition).toInt();
    bool rtl = query.value(Qt::ImCurrentSelection).toString().isRightToLeft();
    auto rightRect = im->anchorRectangle();
    if (cpos > anchor)
        std::swap(leftRect, rightRect);

    auto checkLeftHandle = [&rightRect](QPointF &handlePos) {
        if (handlePos.y() > rightRect.center().y())
            handlePos.setY(rightRect.center().y()); // adjust Y handle pos
        if (handlePos.y() >= rightRect.y() && handlePos.y() <= rightRect.bottom() && handlePos.x() >= rightRect.x())
            return false; // same line and wrong X pos ?
        return true;
    };

    auto checkRtlRightHandle = [&rightRect](QPointF &handlePos) {
        if (handlePos.y() > rightRect.center().y())
            handlePos.setY(rightRect.center().y()); // adjust Y handle pos
        if (handlePos.y() >= rightRect.y() && handlePos.y() <= rightRect.bottom() && rightRect.x() >= handlePos.x())
            return false; // same line and wrong X pos ?
        return true;
    };

    auto checkRightHandle = [&leftRect](QPointF &handlePos) {
        if (handlePos.y() < leftRect.center().y())
            handlePos.setY(leftRect.center().y()); // adjust Y handle pos
        if (handlePos.y() >= leftRect.y() && handlePos.y() <= leftRect.bottom() && leftRect.x() >= handlePos.x())
            return false; // same line and wrong X pos ?
        return true;
    };

    auto checkRtlLeftHandle = [&leftRect](QPointF &handlePos) {
        if (handlePos.y() < leftRect.center().y())
            handlePos.setY(leftRect.center().y()); // adjust Y handle pos
        if (handlePos.y() >= leftRect.y() && handlePos.y() <= leftRect.bottom() && handlePos.x() >= leftRect.x())
            return false; // same line and wrong X pos ?
        return true;
    };

    if (handleId == 2) {
        QPointF rightPoint(rightRect.center());
        if ((!rtl && !checkLeftHandle(point)) || (rtl && !checkRtlRightHandle(point)))
            return;
        setSelectionOnFocusObject(point, rightPoint);
    } else if (handleId == 3) {
        QPointF leftPoint(leftRect.center());
        if ((!rtl && !checkRightHandle(point)) || (rtl && !checkRtlLeftHandle(point)))
            return;
        setSelectionOnFocusObject(leftPoint, point);
    }
}

void QAndroidInputContext::touchDown(int x, int y)
{
    if (m_focusObject && inputItemRectangle().contains(x, y) && !m_cursorHandleShown) {
        // If the user touch the input rectangle, we can show the cursor handle
        m_cursorHandleShown = QAndroidInputContext::CursorHandleShowNormal;
        updateSelectionHandles();
    }
}

void QAndroidInputContext::longPress(int x, int y)
{
    if (m_focusObject && inputItemRectangle().contains(x, y)) {
        // Show the paste menu if there is something to paste.
        m_cursorHandleShown = QAndroidInputContext::CursorHandleShowPopup;
        updateSelectionHandles();
    }
}

void QAndroidInputContext::keyDown()
{
    if (m_cursorHandleShown) {
        // When the user enter text on the keyboard, we hide the cursor handle
        m_cursorHandleShown = QAndroidInputContext::CursorHandleNotShown;
        updateSelectionHandles();
    }
}

void QAndroidInputContext::update(Qt::InputMethodQueries queries)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe(queries);
    if (query.isNull())
        return;
#warning TODO extract the needed data from query
}

void QAndroidInputContext::invokeAction(QInputMethod::Action action, int cursorPosition)
{
#warning TODO Handle at least QInputMethod::ContextMenu action
    Q_UNUSED(action)
    Q_UNUSED(cursorPosition)
    //### click should be passed to the IM, but in the meantime it's better to ignore it than to do something wrong
    // if (action == QInputMethod::Click)
    //     commit();
}

QRectF QAndroidInputContext::keyboardRect() const
{
    return QtAndroidInput::softwareKeyboardRect();
}

bool QAndroidInputContext::isAnimating() const
{
    return false;
}

void QAndroidInputContext::showInputPanel()
{
    if (QGuiApplication::applicationState() != Qt::ApplicationActive) {
        connect(qGuiApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(showInputPanelLater(Qt::ApplicationState)));
        return;
    }
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return;

    disconnect(m_updateCursorPosConnection);
    if (qGuiApp->focusObject()->metaObject()->indexOfSignal("cursorPositionChanged(int,int)") >= 0) // QLineEdit breaks the pattern
        m_updateCursorPosConnection = connect(qGuiApp->focusObject(), SIGNAL(cursorPositionChanged(int,int)), this, SLOT(updateCursorPosition()));
    else
        m_updateCursorPosConnection = connect(qGuiApp->focusObject(), SIGNAL(cursorPositionChanged()), this, SLOT(updateCursorPosition()));

    QRect rect = inputItemRectangle();
    QtAndroidInput::showSoftwareKeyboard(rect.left(), rect.top(), rect.width(), rect.height(),
                                         query->value(Qt::ImHints).toUInt(),
                                         query->value(Qt::ImEnterKeyType).toUInt());
}

void QAndroidInputContext::showInputPanelLater(Qt::ApplicationState state)
{
    if (state != Qt::ApplicationActive)
        return;
    disconnect(qGuiApp, SIGNAL(applicationStateChanged(Qt::ApplicationState)), this, SLOT(showInputPanelLater(Qt::ApplicationState)));
    showInputPanel();
}

void QAndroidInputContext::hideInputPanel()
{
    QtAndroidInput::hideSoftwareKeyboard();
}

bool QAndroidInputContext::isInputPanelVisible() const
{
    return QtAndroidInput::isSoftwareKeyboardVisible();
}

bool QAndroidInputContext::isComposing() const
{
    return m_composingText.length();
}

void QAndroidInputContext::clear()
{
    m_composingText.clear();
    m_composingTextStart  = -1;
    m_extractedText.clear();
}


void QAndroidInputContext::setFocusObject(QObject *object)
{
    if (object != m_focusObject) {
        m_focusObject = object;
        if (!m_composingText.isEmpty())
            finishComposingText();
        reset();
    }
    QPlatformInputContext::setFocusObject(object);
    updateSelectionHandles();
}

jboolean QAndroidInputContext::beginBatchEdit()
{
    ++m_batchEditNestingLevel;
    return JNI_TRUE;
}

jboolean QAndroidInputContext::endBatchEdit()
{
    if (--m_batchEditNestingLevel == 0 && !m_blockUpdateSelection) //ending batch edit mode
        updateCursorPosition();
    return JNI_TRUE;
}

/*
  Android docs say: If composing, replace compose text with \a text.
  Otherwise insert \a text at current cursor position.

  The cursor should then be moved to newCursorPosition. If > 0, this is
  relative to the end of the text - 1; if <= 0, this is relative to the start
  of the text. updateSelection() needs to be called.
*/
jboolean QAndroidInputContext::commitText(const QString &text, jint newCursorPosition)
{
    bool updateSelectionWasBlocked = m_blockUpdateSelection;
    m_blockUpdateSelection = true;

    QInputMethodEvent event;
    event.setCommitString(text);
    sendInputMethodEventThreadSafe(&event);
    clear();

    // Qt has now put the cursor at the end of the text, corresponding to newCursorPosition == 1
    if (newCursorPosition != 1) {
        QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
        if (!query.isNull()) {
            QList<QInputMethodEvent::Attribute> attributes;
            const int localPos = query->value(Qt::ImCursorPosition).toInt();
            const int newLocalPos = newCursorPosition > 0
                                    ? localPos + newCursorPosition - 1
                                    : localPos - text.length() + newCursorPosition;
            //move the cursor
            attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection,
                                                           newLocalPos, 0));
        }
    }
    m_blockUpdateSelection = updateSelectionWasBlocked;

    updateCursorPosition();
    return JNI_TRUE;
}

jboolean QAndroidInputContext::deleteSurroundingText(jint leftLength, jint rightLength)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return JNI_TRUE;

    m_composingText.clear();
    m_composingTextStart = -1;

    if (leftLength < 0) {
        rightLength += -leftLength;
        leftLength = 0;
    }

    QInputMethodEvent event;
    event.setCommitString(QString(), -leftLength, leftLength+rightLength);
    sendInputMethodEventThreadSafe(&event);
    clear();

    return JNI_TRUE;
}

// Android docs say the cursor must not move
jboolean QAndroidInputContext::finishComposingText()
{
    if (m_composingText.isEmpty())
        return JNI_TRUE; // not composing

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return JNI_FALSE;

    const int blockPos = getBlockPosition(query);
    const int localCursorPos = m_composingCursor - blockPos;

    // Moving Qt's cursor to where the preedit cursor used to be
    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection, localCursorPos, 0));

    QInputMethodEvent event(QString(), attributes);
    event.setCommitString(m_composingText);
    sendInputMethodEventThreadSafe(&event);
    clear();

    return JNI_TRUE;
}

jint QAndroidInputContext::getCursorCapsMode(jint /*reqModes*/)
{
    jint res = 0;
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return res;

    const uint qtInputMethodHints = query->value(Qt::ImHints).toUInt();
    const int localPos = query->value(Qt::ImCursorPosition).toInt();

    bool atWordBoundary = (localPos == 0);
    if (!atWordBoundary) {
        QString surroundingText = query->value(Qt::ImSurroundingText).toString();
        surroundingText.truncate(localPos);
        // Add a character to see if it is at the end of the sentence or not
        QTextBoundaryFinder finder(QTextBoundaryFinder::Sentence, surroundingText + QLatin1Char('A'));
        finder.setPosition(localPos);
        if (finder.isAtBoundary())
            atWordBoundary = finder.isAtBoundary();
    }
    if (atWordBoundary && !(qtInputMethodHints & Qt::ImhLowercaseOnly) && !(qtInputMethodHints & Qt::ImhNoAutoUppercase))
        res |= CAP_MODE_SENTENCES;

    if (qtInputMethodHints & Qt::ImhUppercaseOnly)
        res |= CAP_MODE_CHARACTERS;

    return res;
}



const QAndroidInputContext::ExtractedText &QAndroidInputContext::getExtractedText(jint /*hintMaxChars*/, jint /*hintMaxLines*/, jint /*flags*/)
{
    // Note to self: "if the GET_EXTRACTED_TEXT_MONITOR flag is set, you should be calling
    // updateExtractedText(View, int, ExtractedText) whenever you call
    // updateSelection(View, int, int, int, int)."  QTBUG-37980

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return m_extractedText;

    int localPos = query->value(Qt::ImCursorPosition).toInt(); //position before pre-edit text relative to the current block
    int blockPos = getBlockPosition(query);
    QString blockText = query->value(Qt::ImSurroundingText).toString();
    int composeLength = m_composingText.length();

    if (composeLength > 0) {
        //Qt doesn't give us the preedit text, so we have to insert it at the correct position
        int localComposePos = m_composingTextStart - blockPos;
        blockText = blockText.leftRef(localComposePos) + m_composingText + blockText.midRef(localComposePos);
    }

    int cpos = localPos + composeLength; //actual cursor pos relative to the current block

    int localOffset = 0; // start of extracted text relative to the current block

    // It is documented that we should try to return hintMaxChars
    // characters, but that's not what the standard Android controls do, and
    // there are input methods out there that (surprise) seem to depend on
    // what happens in reality rather than what's documented.

    m_extractedText.text = blockText;
    m_extractedText.startOffset = blockPos + localOffset;

    const QString &selection = query->value(Qt::ImCurrentSelection).toString();
    const int selLen = selection.length();
    if (selLen) {
        m_extractedText.selectionStart = query->value(Qt::ImAnchorPosition).toInt() - localOffset;
        m_extractedText.selectionEnd = m_extractedText.selectionStart + selLen;
    } else if (composeLength > 0) {
        m_extractedText.selectionStart = m_composingCursor - m_extractedText.startOffset;
        m_extractedText.selectionEnd = m_composingCursor - m_extractedText.startOffset;
    } else  {
        m_extractedText.selectionStart = cpos - localOffset;
        m_extractedText.selectionEnd = cpos - localOffset;
    }

    return m_extractedText;
}

QString QAndroidInputContext::getSelectedText(jint /*flags*/)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return QString();

    return query->value(Qt::ImCurrentSelection).toString();
}

QString QAndroidInputContext::getTextAfterCursor(jint length, jint /*flags*/)
{
    //### the preedit text could theoretically be after the cursor
    QVariant textAfter = queryFocusObjectThreadSafe(Qt::ImTextAfterCursor, QVariant(length));
    if (textAfter.isValid()) {
        return textAfter.toString().left(length);
    }

    //compatibility code for old controls that do not implement the new API
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return QString();

    QString text = query->value(Qt::ImSurroundingText).toString();
    if (!text.length())
        return text;

    int cursorPos = query->value(Qt::ImCursorPosition).toInt();
    return text.mid(cursorPos, length);
}

QString QAndroidInputContext::getTextBeforeCursor(jint length, jint /*flags*/)
{
    QVariant textBefore = queryFocusObjectThreadSafe(Qt::ImTextBeforeCursor, QVariant(length));
    if (textBefore.isValid())
        return textBefore.toString().rightRef(length) + m_composingText;

    //compatibility code for old controls that do not implement the new API
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return QString();

    int cursorPos = query->value(Qt::ImCursorPosition).toInt();
    QString text = query->value(Qt::ImSurroundingText).toString();
    if (!text.length())
        return text;

    //### the preedit text does not need to be immediately before the cursor
    if (cursorPos <= length)
        return text.leftRef(cursorPos) + m_composingText;
    else
        return text.midRef(cursorPos - length, length) + m_composingText;
}

/*
  Android docs say that this function should remove the current preedit text
  if any, and replace it with the given text. Any selected text should be
  removed. The cursor is then moved to newCursorPosition. If > 0, this is
  relative to the end of the text - 1; if <= 0, this is relative to the start
  of the text.
 */

jboolean QAndroidInputContext::setComposingText(const QString &text, jint newCursorPosition)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return JNI_FALSE;

    const int cursorPos = getAbsoluteCursorPosition(query);
    if (newCursorPosition > 0)
        newCursorPosition += text.length() - 1;

    m_composingText = text;
    m_composingTextStart = text.isEmpty() ? -1 : cursorPos;
    m_composingCursor = cursorPos + newCursorPosition;
    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   newCursorPosition,
                                                   1));
    // Show compose text underlined
    QTextCharFormat underlined;
    underlined.setFontUnderline(true);
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,0, text.length(),
                                                   QVariant(underlined)));

    QInputMethodEvent event(m_composingText, attributes);
    sendInputMethodEventThreadSafe(&event);

    QMetaObject::invokeMethod(this, "keyDown");

    updateCursorPosition();

    return JNI_TRUE;
}

// Android docs say:
// * start may be after end, same meaning as if swapped
// * this function should not trigger updateSelection
// * if start == end then we should stop composing
jboolean QAndroidInputContext::setComposingRegion(jint start, jint end)
{
    // Qt will not include the current preedit text in the query results, and interprets all
    // parameters relative to the text excluding the preedit. The simplest solution is therefore to
    // tell Qt that we commit the text before we set the new region. This may cause a little flicker, but is
    // much more robust than trying to keep the two different world views in sync

    bool wasComposing = !m_composingText.isEmpty();
    if (wasComposing)
        finishComposingText();

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return JNI_FALSE;

    if (start > end)
        qSwap(start, end);

    /*
      start and end are  cursor positions, not character positions,
      i.e. selecting the first character is done by start == 0 and end == 1,
      and start == end means no character selected

      Therefore, the length of the region is end - start
     */

    int length = end - start;
    int localPos = query->value(Qt::ImCursorPosition).toInt();
    int blockPosition = getBlockPosition(query);
    int localStart = start - blockPosition; // Qt uses position inside block
    int currentCursor = wasComposing ? m_composingCursor : blockPosition + localPos;

    bool updateSelectionWasBlocked = m_blockUpdateSelection;
    m_blockUpdateSelection = true;

    QString text = query->value(Qt::ImSurroundingText).toString();

    m_composingText = text.mid(localStart, length);
    m_composingTextStart = start;
    m_composingCursor = currentCursor;

    //in the Qt text controls, the preedit is defined relative to the cursor position
    int relativeStart = localStart - localPos;

    QList<QInputMethodEvent::Attribute> attributes;

    // Show compose text underlined
    QTextCharFormat underlined;
    underlined.setFontUnderline(true);
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,0, length,
                                                   QVariant(underlined)));

    // Keep the cursor position unchanged (don't move to end of preedit)
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, currentCursor - start, 1));

    QInputMethodEvent event(m_composingText, attributes);
    event.setCommitString(QString(), relativeStart, length);
    sendInputMethodEventThreadSafe(&event);

    m_blockUpdateSelection = updateSelectionWasBlocked;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
     QSharedPointer<QInputMethodQueryEvent> query2 = focusObjectInputMethodQueryThreadSafe();
     if (!query2.isNull()) {
         qDebug() << "Setting. Prev local cpos:" << localPos << "block pos:" <<blockPosition << "comp.start:" << m_composingTextStart << "rel.start:" << relativeStart << "len:" << length << "cpos attr:" << localPos - localStart;
         qDebug() << "New cursor pos" << getAbsoluteCursorPosition(query2);
     }
#endif

    return JNI_TRUE;
}

jboolean QAndroidInputContext::setSelection(jint start, jint end)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQueryThreadSafe();
    if (query.isNull())
        return JNI_FALSE;

    int blockPosition = getBlockPosition(query);
    int localCursorPos = start - blockPosition;

    QList<QInputMethodEvent::Attribute> attributes;
    if (!m_composingText.isEmpty() && start == end) {
        // not actually changing the selection; just moving the
        // preedit cursor
        int localOldPos = query->value(Qt::ImCursorPosition).toInt();
        int pos = localCursorPos - localOldPos;
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, pos, 1));

        //but we have to tell Qt about the compose text all over again

        // Show compose text underlined
        QTextCharFormat underlined;
        underlined.setFontUnderline(true);
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,0, m_composingText.length(),
                                                   QVariant(underlined)));
        m_composingCursor = start;

    } else {
        // actually changing the selection
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection,
                                                       localCursorPos,
                                                       end - start));
    }
    QInputMethodEvent event(m_composingText, attributes);
    sendInputMethodEventThreadSafe(&event);
    updateCursorPosition();
    return JNI_TRUE;
}

jboolean QAndroidInputContext::selectAll()
{
    sendShortcut(QKeySequence::SelectAll);
    return JNI_TRUE;
}

jboolean QAndroidInputContext::cut()
{
    m_cursorHandleShown = CursorHandleNotShown;
    sendShortcut(QKeySequence::Cut);
    return JNI_TRUE;
}

jboolean QAndroidInputContext::copy()
{
    sendShortcut(QKeySequence::Copy);
    return JNI_TRUE;
}

jboolean QAndroidInputContext::copyURL()
{
#warning TODO
    return JNI_FALSE;
}

jboolean QAndroidInputContext::paste()
{
    finishComposingText();
    m_cursorHandleShown = CursorHandleNotShown;
    sendShortcut(QKeySequence::Paste);
    return JNI_TRUE;
}

void QAndroidInputContext::sendShortcut(const QKeySequence &sequence)
{
    for (int i = 0; i < sequence.count(); ++i) {
        const int keys = sequence[i];
        Qt::Key key = Qt::Key(keys & ~Qt::KeyboardModifierMask);
        Qt::KeyboardModifiers mod = Qt::KeyboardModifiers(keys & Qt::KeyboardModifierMask);
        QGuiApplication::postEvent(m_focusObject, new QKeyEvent(QEvent::KeyPress, key, mod));
        QGuiApplication::postEvent(m_focusObject, new QKeyEvent(QEvent::KeyRelease, key, mod));
    }
}

Q_INVOKABLE QVariant QAndroidInputContext::queryFocusObjectUnsafe(Qt::InputMethodQuery query, QVariant argument)
{
    return QInputMethod::queryFocusObject(query, argument);
}

QVariant QAndroidInputContext::queryFocusObjectThreadSafe(Qt::InputMethodQuery query, QVariant argument)
{
    QVariant retval;
    if (!qGuiApp)
        return retval;
    const bool inMainThread = qGuiApp->thread() == QThread::currentThread();
    if (QAndroidEventDispatcherStopper::stopped() && !inMainThread)
        return retval;
    AndroidDeadlockProtector protector;
    if (!inMainThread && !protector.acquire())
        return retval;

    QMetaObject::invokeMethod(this, "queryFocusObjectUnsafe",
                              inMainThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QVariant, retval),
                              Q_ARG(Qt::InputMethodQuery, query),
                              Q_ARG(QVariant, argument));

    return retval;
}

QSharedPointer<QInputMethodQueryEvent> QAndroidInputContext::focusObjectInputMethodQueryThreadSafe(Qt::InputMethodQueries queries) {
    QSharedPointer<QInputMethodQueryEvent> retval;
    if (!qGuiApp)
        return QSharedPointer<QInputMethodQueryEvent>();
    const bool inMainThread = qGuiApp->thread() == QThread::currentThread();
    if (QAndroidEventDispatcherStopper::stopped() && !inMainThread)
        return QSharedPointer<QInputMethodQueryEvent>();
    AndroidDeadlockProtector protector;
    if (!inMainThread && !protector.acquire())
        return QSharedPointer<QInputMethodQueryEvent>();

    QInputMethodQueryEvent *queryEvent = 0;
    QMetaObject::invokeMethod(this, "focusObjectInputMethodQueryUnsafe",
                              inMainThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QInputMethodQueryEvent*, queryEvent),
                              Q_ARG(Qt::InputMethodQueries, queries));

    return QSharedPointer<QInputMethodQueryEvent>(queryEvent);
}

QInputMethodQueryEvent *QAndroidInputContext::focusObjectInputMethodQueryUnsafe(Qt::InputMethodQueries queries)
{
    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return 0;

    QInputMethodQueryEvent *ret = new QInputMethodQueryEvent(queries);
    QCoreApplication::sendEvent(focusObject, ret);
    return ret;
}

void QAndroidInputContext::sendInputMethodEventUnsafe(QInputMethodEvent *event)
{
    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return;

    QCoreApplication::sendEvent(focusObject, event);
}

void QAndroidInputContext::sendInputMethodEventThreadSafe(QInputMethodEvent *event)
{
    if (!qGuiApp)
        return;
    const bool inMainThread = qGuiApp->thread() == QThread::currentThread();
    if (QAndroidEventDispatcherStopper::stopped() && !inMainThread)
        return;
    AndroidDeadlockProtector protector;
    if (!inMainThread && !protector.acquire())
        return;
    QMetaObject::invokeMethod(this, "sendInputMethodEventUnsafe",
                              inMainThread ? Qt::DirectConnection : Qt::BlockingQueuedConnection,
                              Q_ARG(QInputMethodEvent*, event));
}

QT_END_NAMESPACE
