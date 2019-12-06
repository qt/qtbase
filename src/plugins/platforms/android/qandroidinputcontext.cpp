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

namespace {

class BatchEditLock
{
public:

    explicit BatchEditLock(QAndroidInputContext *context)
        : m_context(context)
    {
        m_context->beginBatchEdit();
    }

    ~BatchEditLock()
    {
        m_context->endBatchEdit();
    }

    BatchEditLock(const BatchEditLock &) = delete;
    BatchEditLock &operator=(const BatchEditLock &) = delete;

private:

    QAndroidInputContext *m_context;
};

} // namespace anonymous

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

static void runOnQtThread(const std::function<void()> &func)
{
    AndroidDeadlockProtector protector;
    if (!protector.acquire())
        return;
    QMetaObject::invokeMethod(m_androidInputContext, "safeCall", Qt::BlockingQueuedConnection, Q_ARG(std::function<void()>, func));
}

static jboolean beginBatchEdit(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ BEGINBATCH");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&res]{res = m_androidInputContext->beginBatchEdit();});
    return res;
}

static jboolean endBatchEdit(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ ENDBATCH");
#endif

    jboolean res = JNI_FALSE;
    runOnQtThread([&res]{res = m_androidInputContext->endBatchEdit();});
    return res;
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
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->commitText(str, newCursorPosition);});
    return res;
}

static jboolean deleteSurroundingText(JNIEnv */*env*/, jobject /*thiz*/, jint leftLength, jint rightLength)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ DELETE" << leftLength << rightLength;
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->deleteSurroundingText(leftLength, rightLength);});
    return res;
}

static jboolean finishComposingText(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ FINISH");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->finishComposingText();});
    return res;
}

static jint getCursorCapsMode(JNIEnv */*env*/, jobject /*thiz*/, jint reqModes)
{
    if (!m_androidInputContext)
        return 0;

    jint res = 0;
    runOnQtThread([&]{res = m_androidInputContext->getCursorCapsMode(reqModes);});
    return res;
}

static jobject getExtractedText(JNIEnv *env, jobject /*thiz*/, int hintMaxChars, int hintMaxLines, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    QAndroidInputContext::ExtractedText extractedText;
    runOnQtThread([&]{extractedText = m_androidInputContext->getExtractedText(hintMaxChars, hintMaxLines, flags);});

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

    QString text;
    runOnQtThread([&]{text = m_androidInputContext->getSelectedText(flags);});
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

    QString text;
    runOnQtThread([&]{text = m_androidInputContext->getTextAfterCursor(length, flags);});
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ GETA" << length << text;
#endif
    return env->NewString(reinterpret_cast<const jchar *>(text.constData()), jsize(text.length()));
}

static jstring getTextBeforeCursor(JNIEnv *env, jobject /*thiz*/, jint length, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    QString text;
    runOnQtThread([&]{text = m_androidInputContext->getTextBeforeCursor(length, flags);});
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
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->setComposingText(str, newCursorPosition);});
    return res;
}

static jboolean setComposingRegion(JNIEnv */*env*/, jobject /*thiz*/, jint start, jint end)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ SETR" << start << end;
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->setComposingRegion(start, end);});
    return res;
}


static jboolean setSelection(JNIEnv */*env*/, jobject /*thiz*/, jint start, jint end)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug() << "@@@ SETSEL" << start << end;
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->setSelection(start, end);});
    return res;

}

static jboolean selectAll(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ SELALL");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->selectAll();});
    return res;
}

static jboolean cut(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->cut();});
    return res;
}

static jboolean copy(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->copy();});
    return res;
}

static jboolean copyURL(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->copyURL();});
    return res;
}

static jboolean paste(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ PASTE");
#endif
    jboolean res = JNI_FALSE;
    runOnQtThread([&]{res = m_androidInputContext->paste();});
    return res;
}

static jboolean updateCursorPosition(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
    qDebug("@@@ UPDATECURSORPOS");
#endif

    runOnQtThread([&]{m_androidInputContext->updateCursorPosition();});
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
    : QPlatformInputContext()
    , m_composingTextStart(-1)
    , m_composingCursor(-1)
    , m_handleMode(Hidden)
    , m_batchEditNestingLevel(0)
    , m_focusObject(0)
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
            m_handleMode = Hidden;
            updateSelectionHandles();
        }
    });
    m_hideCursorHandleTimer.setInterval(4000);
    m_hideCursorHandleTimer.setSingleShot(true);
    m_hideCursorHandleTimer.setTimerType(Qt::VeryCoarseTimer);
    connect(&m_hideCursorHandleTimer, &QTimer::timeout, this, [this]{
        m_handleMode = Hidden;
        updateSelectionHandles();
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
    focusObjectStopComposing();
    clear();
    m_batchEditNestingLevel = 0;
    m_handleMode = Hidden;
    if (qGuiApp->focusObject()) {
        QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery(Qt::ImEnabled);
        if (!query.isNull() && query->value(Qt::ImEnabled).toBool()) {
            QtAndroidInput::resetSoftwareKeyboard();
            return;
        }
    }
    QtAndroidInput::hideSoftwareKeyboard();
}

void QAndroidInputContext::commit()
{
    focusObjectStopComposing();
}

void QAndroidInputContext::updateCursorPosition()
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (!query.isNull() && m_batchEditNestingLevel == 0) {
        const int cursorPos = getAbsoluteCursorPosition(query);
        const int composeLength = m_composingText.length();

        //Q_ASSERT(m_composingText.isEmpty() == (m_composingTextStart == -1));
        if (m_composingText.isEmpty() != (m_composingTextStart == -1))
            qWarning() << "Input method out of sync" << m_composingText << m_composingTextStart;

        int realSelectionStart = cursorPos;
        int realSelectionEnd = cursorPos;

        int cpos = query->value(Qt::ImCursorPosition).toInt();
        int anchor = query->value(Qt::ImAnchorPosition).toInt();
        if (cpos != anchor) {
            if (!m_composingText.isEmpty()) {
                qWarning("Selecting text while preediting may give unpredictable results.");
                focusObjectStopComposing();
            }
            int blockPos = getBlockPosition(query);
            realSelectionStart = blockPos + cpos;
            realSelectionEnd = blockPos + anchor;
        }
        // Qt's idea of the cursor position is the start of the preedit area, so we maintain our own preedit cursor pos
        if (focusObjectIsComposing())
            realSelectionStart = realSelectionEnd = m_composingCursor;

        // Some keyboards misbahave when selStart > selEnd
        if (realSelectionStart > realSelectionEnd)
            std::swap(realSelectionStart, realSelectionEnd);

        QtAndroidInput::updateSelection(realSelectionStart, realSelectionEnd,
                                        m_composingTextStart, m_composingTextStart + composeLength); // pre-edit text
    }
}

void QAndroidInputContext::updateSelectionHandles()
{
    static bool noHandles = qEnvironmentVariableIntValue("QT_QPA_NO_TEXT_HANDLES");
    if (noHandles)
        return;

    auto im = qGuiApp->inputMethod();
    if (!m_focusObject || ((m_handleMode & 0xff) == Hidden)) {
        // Hide the handles
        QtAndroidInput::updateHandles(Hidden);
        return;
    }
    QWindow *window = qGuiApp->focusWindow();
    double pixelDensity = window
        ? QHighDpiScaling::factor(window)
        : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImAnchorPosition | Qt::ImEnabled | Qt::ImCurrentSelection | Qt::ImHints | Qt::ImSurroundingText);
    QCoreApplication::sendEvent(m_focusObject, &query);

    int cpos = query.value(Qt::ImCursorPosition).toInt();
    int anchor = query.value(Qt::ImAnchorPosition).toInt();

    if (cpos == anchor || im->anchorRectangle().isNull()) {
        if (!query.value(Qt::ImEnabled).toBool()) {
            QtAndroidInput::updateHandles(Hidden);
            return;
        }

        auto curRect = im->cursorRectangle();
        QPoint cursorPoint(curRect.center().x(), curRect.bottom());
        QPoint editMenuPoint(curRect.x(), curRect.y());
        m_handleMode &= ShowEditPopup;
        m_handleMode |= ShowCursor;
        uint32_t buttons = EditContext::PasteButton;
        if (!query.value(Qt::ImSurroundingText).toString().isEmpty())
            buttons |= EditContext::SelectAllButton;
        QtAndroidInput::updateHandles(m_handleMode, editMenuPoint * pixelDensity, buttons, cursorPoint * pixelDensity);
        // The VK is hidden, reset the timer
        if (m_hideCursorHandleTimer.isActive())
            m_hideCursorHandleTimer.start();
        return;
    }

    m_handleMode = ShowSelection | ShowEditPopup ;
    auto leftRect = im->cursorRectangle();
    auto rightRect = im->anchorRectangle();
    if (cpos > anchor)
        std::swap(leftRect, rightRect);

    QPoint leftPoint(leftRect.bottomLeft().toPoint() * pixelDensity);
    QPoint righPoint(rightRect.bottomRight().toPoint() * pixelDensity);
    QPoint editPoint(leftRect.united(rightRect).topLeft().toPoint() * pixelDensity);
    QtAndroidInput::updateHandles(m_handleMode, editPoint, EditContext::AllButtons, leftPoint, righPoint,
                                  query.value(Qt::ImCurrentSelection).toString().isRightToLeft());
    m_hideCursorHandleTimer.stop();
}

/*
   Called from Java when a cursor/selection handle was dragged to a new position

   handleId of 1 means the cursor handle,  2 means the left handle, 3 means the right handle
 */
void QAndroidInputContext::handleLocationChanged(int handleId, int x, int y)
{
    if (m_batchEditNestingLevel != 0) {
        qWarning() << "QAndroidInputContext::handleLocationChanged returned";
        return;
    }

    auto im = qGuiApp->inputMethod();
    auto leftRect = im->cursorRectangle();
    // The handle is down of the cursor, but we want the position in the middle.
    QWindow *window = qGuiApp->focusWindow();
    double pixelDensity = window
        ? QHighDpiScaling::factor(window)
        : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());
    QPointF point(x / pixelDensity, y / pixelDensity);
    point.setY(point.y() - leftRect.width() / 2);

    QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImAnchorPosition
                                 | Qt::ImAbsolutePosition | Qt::ImCurrentSelection);
    QCoreApplication::sendEvent(m_focusObject, &query);
    int cpos = query.value(Qt::ImCursorPosition).toInt();
    int anchor = query.value(Qt::ImAnchorPosition).toInt();
    auto rightRect = im->anchorRectangle();
    if (cpos > anchor)
        std::swap(leftRect, rightRect);

    // Do not allow dragging left handle below right handle, or right handle above left handle
    if (handleId == 2 && point.y() > rightRect.center().y()) {
        point.setY(rightRect.center().y());
    } else if (handleId == 3 && point.y() < leftRect.center().y()) {
        point.setY(leftRect.center().y());
    }

    const QPointF pointLocal = im->inputItemTransform().inverted().map(point);
    bool ok;
    const int handlePos =
            QInputMethod::queryFocusObject(Qt::ImCursorPosition, pointLocal).toInt(&ok);
    if (!ok)
        return;

    int newCpos = cpos;
    int newAnchor = anchor;
    if (newAnchor > newCpos)
        std::swap(newAnchor, newCpos);

    if (handleId == 1) {
        newCpos = handlePos;
        newAnchor = handlePos;
    } else if (handleId == 2) {
        newAnchor = handlePos;
    } else if (handleId == 3) {
        newCpos = handlePos;
    }

    /*
      Do not allow clearing selection by dragging selection handles and do not allow swapping
      selection handles for consistency with Android's native text editing controls. Ensure that at
      least one symbol remains selected.
     */
    if ((handleId == 2 || handleId == 3) && newCpos <= newAnchor) {
        QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme,
                                   query.value(Qt::ImCurrentSelection).toString());

        const int oldSelectionStartPos = qMin(cpos, anchor);

        if (handleId == 2) {
            finder.toEnd();
            finder.toPreviousBoundary();
            newAnchor = finder.position() + oldSelectionStartPos;
        } else {
            finder.toStart();
            finder.toNextBoundary();
            newCpos = finder.position() + oldSelectionStartPos;
        }
    }

    // Check if handle has been dragged far enough
    if (!focusObjectIsComposing() && newCpos == cpos && newAnchor == anchor)
        return;

    /*
      If the editor is currently in composing state, we have to compare newCpos with
      m_composingCursor instead of cpos. And since there is nothing to compare with newAnchor, we
      perform the check only when user drags the cursor handle.
     */
    if (focusObjectIsComposing() && handleId == 1) {
        int absoluteCpos = query.value(Qt::ImAbsolutePosition).toInt(&ok);
        if (!ok)
            absoluteCpos = cpos;
        const int blockPos = absoluteCpos - cpos;

        if (blockPos + newCpos == m_composingCursor)
            return;
    }

    BatchEditLock batchEditLock(this);

    focusObjectStopComposing();

    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append({ QInputMethodEvent::Selection, newAnchor, newCpos - newAnchor });
    if (newCpos != newAnchor)
        attributes.append({ QInputMethodEvent::Cursor, 0, 0 });

    QInputMethodEvent event(QString(), attributes);
    QGuiApplication::sendEvent(m_focusObject, &event);
}

void QAndroidInputContext::touchDown(int x, int y)
{
    if (m_focusObject && inputItemRectangle().contains(x, y)) {
        // If the user touch the input rectangle, we can show the cursor handle
        m_handleMode = ShowCursor;
        // The VK will appear in a moment, stop the timer
        m_hideCursorHandleTimer.stop();

        if (focusObjectIsComposing()) {
            const double pixelDensity =
                    QGuiApplication::focusWindow()
                    ? QHighDpiScaling::factor(QGuiApplication::focusWindow())
                    : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());

            const QPointF touchPointLocal =
                    QGuiApplication::inputMethod()->inputItemTransform().inverted().map(
                            QPointF(x / pixelDensity, y / pixelDensity));

            const int curBlockPos = getBlockPosition(
                    focusObjectInputMethodQuery(Qt::ImCursorPosition | Qt::ImAbsolutePosition));
            const int touchPosition = curBlockPos
                    + QInputMethod::queryFocusObject(Qt::ImCursorPosition, touchPointLocal).toInt();
            if (touchPosition != m_composingCursor)
                focusObjectStopComposing();
        }

        updateSelectionHandles();
    }
}

void QAndroidInputContext::longPress(int x, int y)
{
    static bool noHandles = qEnvironmentVariableIntValue("QT_QPA_NO_TEXT_HANDLES");
    if (noHandles)
        return;

    if (m_focusObject && inputItemRectangle().contains(x, y)) {
        BatchEditLock batchEditLock(this);

        focusObjectStopComposing();

        const double pixelDensity =
                QGuiApplication::focusWindow()
                ? QHighDpiScaling::factor(QGuiApplication::focusWindow())
                : QHighDpiScaling::factor(QtAndroid::androidPlatformIntegration()->screen());
        const QPointF touchPoint(x / pixelDensity, y / pixelDensity);
        setSelectionOnFocusObject(touchPoint, touchPoint);

        QInputMethodQueryEvent query(Qt::ImCursorPosition | Qt::ImAnchorPosition | Qt::ImTextBeforeCursor | Qt::ImTextAfterCursor);
        QCoreApplication::sendEvent(m_focusObject, &query);
        int cursor = query.value(Qt::ImCursorPosition).toInt();
        int anchor = cursor;
        QString before = query.value(Qt::ImTextBeforeCursor).toString();
        QString after = query.value(Qt::ImTextAfterCursor).toString();
        for (const auto &ch : after) {
            if (!ch.isLetterOrNumber())
                break;
            ++anchor;
        }

        for (auto itch = before.rbegin(); itch != after.rend(); ++itch) {
            if (!itch->isLetterOrNumber())
                break;
            --cursor;
        }
        if (cursor == anchor || cursor < 0 || cursor - anchor > 500) {
            m_handleMode = ShowCursor | ShowEditPopup;
            updateSelectionHandles();
            return;
        }
        QList<QInputMethodEvent::Attribute> imAttributes;
        imAttributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, 0, 0, QVariant()));
        imAttributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection, anchor, cursor - anchor, QVariant()));
        QInputMethodEvent event(QString(), imAttributes);
        QGuiApplication::sendEvent(m_focusObject, &event);

        m_handleMode = ShowSelection | ShowEditPopup;
        updateSelectionHandles();
    }
}

void QAndroidInputContext::keyDown()
{
    if (m_handleMode) {
        // When the user enter text on the keyboard, we hide the cursor handle
        m_handleMode = Hidden;
        updateSelectionHandles();
    }
}

void QAndroidInputContext::hideSelectionHandles()
{
    if (m_handleMode & ShowSelection) {
        m_handleMode = Hidden;
        updateSelectionHandles();
    } else {
        m_hideCursorHandleTimer.start();
    }
}

void QAndroidInputContext::update(Qt::InputMethodQueries queries)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery(queries);
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
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
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

void QAndroidInputContext::safeCall(const std::function<void()> &func, Qt::ConnectionType conType)
{
    if (qGuiApp->thread() == QThread::currentThread())
        func();
    else
        QMetaObject::invokeMethod(this, "safeCall", conType, Q_ARG(std::function<void()>, func));
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
    m_composingCursor = -1;
    m_extractedText.clear();
}


void QAndroidInputContext::setFocusObject(QObject *object)
{
    if (object != m_focusObject) {
        focusObjectStopComposing();
        m_focusObject = object;
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
    if (--m_batchEditNestingLevel == 0) { //ending batch edit mode
        focusObjectStartComposing();
        updateCursorPosition();
    }
    return JNI_TRUE;
}

/*
  Android docs say: This behaves like calling setComposingText(text, newCursorPosition) then
  finishComposingText().
*/
jboolean QAndroidInputContext::commitText(const QString &text, jint newCursorPosition)
{
    BatchEditLock batchEditLock(this);
    return setComposingText(text, newCursorPosition) && finishComposingText();
}

jboolean QAndroidInputContext::deleteSurroundingText(jint leftLength, jint rightLength)
{
    BatchEditLock batchEditLock(this);

    focusObjectStopComposing();

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return JNI_TRUE;

    if (leftLength < 0) {
        rightLength += -leftLength;
        leftLength = 0;
    }

    const int initialBlockPos = getBlockPosition(query);
    const int initialCursorPos = getAbsoluteCursorPosition(query);
    const int initialAnchorPos = initialBlockPos + query->value(Qt::ImAnchorPosition).toInt();

    /*
      According to documentation, we should delete leftLength characters before current selection
      and rightLength characters after current selection (without affecting selection). But that is
      absolutely not what Android's native EditText does. It deletes leftLength characters before
      min(selection start, composing region start) and rightLength characters after max(selection
      end, composing region end). There are no known keyboards that depend on this behavior, but
      it is better to be consistent with EditText behavior, because there definetly should be no
      keyboards that depend on documented behavior.
     */
    const int leftEnd =
            m_composingText.isEmpty()
            ? qMin(initialCursorPos, initialAnchorPos)
            : qMin(qMin(initialCursorPos, initialAnchorPos), m_composingTextStart);

    const int rightBegin =
            m_composingText.isEmpty()
            ? qMax(initialCursorPos, initialAnchorPos)
            : qMax(qMax(initialCursorPos, initialAnchorPos),
                   m_composingTextStart + m_composingText.length());

    int textBeforeCursorLen;
    int textAfterCursorLen;

    QVariant textBeforeCursor = query->value(Qt::ImTextBeforeCursor);
    QVariant textAfterCursor = query->value(Qt::ImTextAfterCursor);
    if (textBeforeCursor.isValid() && textAfterCursor.isValid()) {
        textBeforeCursorLen = textBeforeCursor.toString().length();
        textAfterCursorLen = textAfterCursor.toString().length();
    } else {
        textBeforeCursorLen = initialCursorPos - initialBlockPos;
        textAfterCursorLen =
                query->value(Qt::ImSurroundingText).toString().length() - textBeforeCursorLen;
    }

    leftLength = qMin(qMax(0, textBeforeCursorLen - (initialCursorPos - leftEnd)), leftLength);
    rightLength = qMin(qMax(0, textAfterCursorLen - (rightBegin - initialCursorPos)), rightLength);

    if (leftLength == 0 && rightLength == 0)
        return JNI_TRUE;

    if (leftEnd == rightBegin) {
        // We have no selection and no composing region; we can do everything using one event
        QInputMethodEvent event;
        event.setCommitString({}, -leftLength, leftLength + rightLength);
        QGuiApplication::sendEvent(m_focusObject, &event);
    } else {
        if (initialCursorPos != initialAnchorPos) {
            QInputMethodEvent event({}, {
                { QInputMethodEvent::Selection, initialCursorPos - initialBlockPos, 0 }
            });

            QGuiApplication::sendEvent(m_focusObject, &event);
        }

        int currentCursorPos = initialCursorPos;

        if (rightLength > 0) {
            QInputMethodEvent event;
            event.setCommitString({}, rightBegin - currentCursorPos, rightLength);
            QGuiApplication::sendEvent(m_focusObject, &event);

            currentCursorPos = rightBegin;
        }

        if (leftLength > 0) {
            const int leftBegin = leftEnd - leftLength;

            QInputMethodEvent event;
            event.setCommitString({}, leftBegin - currentCursorPos, leftLength);
            QGuiApplication::sendEvent(m_focusObject, &event);

            currentCursorPos = leftBegin;

            if (!m_composingText.isEmpty())
                m_composingTextStart -= leftLength;
        }

        // Restore cursor position or selection
        if (currentCursorPos != initialCursorPos - leftLength
                || initialCursorPos != initialAnchorPos) {
            // If we have deleted a newline character, we are now in a new block
            const int currentBlockPos = getBlockPosition(
                    focusObjectInputMethodQuery(Qt::ImAbsolutePosition | Qt::ImCursorPosition));

            QInputMethodEvent event({}, {
                { QInputMethodEvent::Selection, initialCursorPos - leftLength - currentBlockPos,
                  initialAnchorPos - initialCursorPos },
                { QInputMethodEvent::Cursor, 0, 0 }
            });

            QGuiApplication::sendEvent(m_focusObject, &event);
        }
    }

    return JNI_TRUE;
}

// Android docs say the cursor must not move
jboolean QAndroidInputContext::finishComposingText()
{
    BatchEditLock batchEditLock(this);

    if (!focusObjectStopComposing())
        return JNI_FALSE;

    clear();
    return JNI_TRUE;
}

bool QAndroidInputContext::focusObjectIsComposing() const
{
    return m_composingCursor != -1;
}

void QAndroidInputContext::focusObjectStartComposing()
{
    if (focusObjectIsComposing() || m_composingText.isEmpty())
        return;

    // Composing strings containing newline characters are rare and may cause problems
    if (m_composingText.contains(QLatin1Char('\n')))
        return;

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (!query)
        return;

    if (query->value(Qt::ImCursorPosition).toInt() != query->value(Qt::ImAnchorPosition).toInt())
        return;

    const int absoluteCursorPos = getAbsoluteCursorPosition(query);
    if (absoluteCursorPos < m_composingTextStart
            || absoluteCursorPos > m_composingTextStart + m_composingText.length())
        return;

    m_composingCursor = absoluteCursorPos;

    QTextCharFormat underlined;
    underlined.setFontUnderline(true);

    QInputMethodEvent event(m_composingText, {
        { QInputMethodEvent::Cursor, absoluteCursorPos - m_composingTextStart, 1 },
        { QInputMethodEvent::TextFormat, 0, m_composingText.length(), underlined }
    });

    event.setCommitString({}, m_composingTextStart - absoluteCursorPos, m_composingText.length());

    QGuiApplication::sendEvent(m_focusObject, &event);
}

bool QAndroidInputContext::focusObjectStopComposing()
{
    if (!focusObjectIsComposing())
        return true; // not composing

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return false;

    const int blockPos = getBlockPosition(query);
    const int localCursorPos = m_composingCursor - blockPos;

    m_composingCursor = -1;

    // Moving Qt's cursor to where the preedit cursor used to be
    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection, localCursorPos, 0));

    QInputMethodEvent event(QString(), attributes);
    event.setCommitString(m_composingText);
    sendInputMethodEvent(&event);

    return true;
}

jint QAndroidInputContext::getCursorCapsMode(jint /*reqModes*/)
{
    jint res = 0;
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return res;

    const uint qtInputMethodHints = query->value(Qt::ImHints).toUInt();
    const int localPos = query->value(Qt::ImCursorPosition).toInt();

    bool atWordBoundary =
            localPos == 0
            && (!focusObjectIsComposing() || m_composingCursor == m_composingTextStart);

    if (!atWordBoundary) {
        QString surroundingText = query->value(Qt::ImSurroundingText).toString();
        surroundingText.truncate(localPos);
        if (focusObjectIsComposing())
            surroundingText += m_composingText.leftRef(m_composingCursor - m_composingTextStart);
        // Add a character to see if it is at the end of the sentence or not
        QTextBoundaryFinder finder(QTextBoundaryFinder::Sentence, surroundingText + QLatin1Char('A'));
        finder.setPosition(surroundingText.length());
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

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery(
            Qt::ImCursorPosition | Qt::ImAbsolutePosition | Qt::ImAnchorPosition);
    if (query.isNull())
        return m_extractedText;

    const int cursorPos = getAbsoluteCursorPosition(query);
    const int blockPos = getBlockPosition(query);

    // It is documented that we should try to return hintMaxChars
    // characters, but standard Android controls always return all text, and
    // there are input methods out there that (surprise) seem to depend on
    // what happens in reality rather than what's documented.

    QVariant textBeforeCursor = QInputMethod::queryFocusObject(Qt::ImTextBeforeCursor, INT_MAX);
    QVariant textAfterCursor = QInputMethod::queryFocusObject(Qt::ImTextAfterCursor, INT_MAX);
    if (textBeforeCursor.isValid() && textAfterCursor.isValid()) {
        if (focusObjectIsComposing()) {
            m_extractedText.text =
                    textBeforeCursor.toString() + m_composingText + textAfterCursor.toString();
        } else {
            m_extractedText.text = textBeforeCursor.toString() + textAfterCursor.toString();
        }

        m_extractedText.startOffset = qMax(0, cursorPos - textBeforeCursor.toString().length());
    } else {
        m_extractedText.text = focusObjectInputMethodQuery(Qt::ImSurroundingText)
                ->value(Qt::ImSurroundingText).toString();

        if (focusObjectIsComposing())
            m_extractedText.text.insert(cursorPos - blockPos, m_composingText);

        m_extractedText.startOffset = blockPos;
    }

    if (focusObjectIsComposing()) {
        m_extractedText.selectionStart = m_composingCursor - m_extractedText.startOffset;
        m_extractedText.selectionEnd = m_extractedText.selectionStart;
    } else {
        m_extractedText.selectionStart = cursorPos - m_extractedText.startOffset;
        m_extractedText.selectionEnd =
                blockPos + query->value(Qt::ImAnchorPosition).toInt() - m_extractedText.startOffset;

        // Some keyboards misbehave when selectionStart > selectionEnd
        if (m_extractedText.selectionStart > m_extractedText.selectionEnd)
            std::swap(m_extractedText.selectionStart, m_extractedText.selectionEnd);
    }

    return m_extractedText;
}

QString QAndroidInputContext::getSelectedText(jint /*flags*/)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return QString();

    return query->value(Qt::ImCurrentSelection).toString();
}

QString QAndroidInputContext::getTextAfterCursor(jint length, jint /*flags*/)
{
    if (length <= 0)
        return QString();

    QString text;

    QVariant reportedTextAfter = QInputMethod::queryFocusObject(Qt::ImTextAfterCursor, length);
    if (reportedTextAfter.isValid()) {
        text = reportedTextAfter.toString();
    } else {
        // Compatibility code for old controls that do not implement the new API
        QSharedPointer<QInputMethodQueryEvent> query =
                focusObjectInputMethodQuery(Qt::ImCursorPosition | Qt::ImSurroundingText);
        if (query) {
            const int cursorPos = query->value(Qt::ImCursorPosition).toInt();
            text = query->value(Qt::ImSurroundingText).toString().mid(cursorPos);
        }
    }

    if (focusObjectIsComposing()) {
        // Controls do not report preedit text, so we have to add it
        const int cursorPosInsidePreedit = m_composingCursor - m_composingTextStart;
        text = m_composingText.midRef(cursorPosInsidePreedit) + text;
    } else {
        // We must not return selected text if there is any
        QSharedPointer<QInputMethodQueryEvent> query =
                focusObjectInputMethodQuery(Qt::ImCursorPosition | Qt::ImAnchorPosition);
        if (query) {
            const int cursorPos = query->value(Qt::ImCursorPosition).toInt();
            const int anchorPos = query->value(Qt::ImAnchorPosition).toInt();
            if (anchorPos > cursorPos)
                text.remove(0, anchorPos - cursorPos);
        }
    }

    text.truncate(length);
    return text;
}

QString QAndroidInputContext::getTextBeforeCursor(jint length, jint /*flags*/)
{
    if (length <= 0)
        return QString();

    QString text;

    QVariant reportedTextBefore = QInputMethod::queryFocusObject(Qt::ImTextBeforeCursor, length);
    if (reportedTextBefore.isValid()) {
        text = reportedTextBefore.toString();
    } else {
        // Compatibility code for old controls that do not implement the new API
        QSharedPointer<QInputMethodQueryEvent> query =
                focusObjectInputMethodQuery(Qt::ImCursorPosition | Qt::ImSurroundingText);
        if (query) {
            const int cursorPos = query->value(Qt::ImCursorPosition).toInt();
            text = query->value(Qt::ImSurroundingText).toString().left(cursorPos);
        }
    }

    if (focusObjectIsComposing()) {
        // Controls do not report preedit text, so we have to add it
        const int cursorPosInsidePreedit = m_composingCursor - m_composingTextStart;
        text += m_composingText.leftRef(cursorPosInsidePreedit);
    } else {
        // We must not return selected text if there is any
        QSharedPointer<QInputMethodQueryEvent> query =
                focusObjectInputMethodQuery(Qt::ImCursorPosition | Qt::ImAnchorPosition);
        if (query) {
            const int cursorPos = query->value(Qt::ImCursorPosition).toInt();
            const int anchorPos = query->value(Qt::ImAnchorPosition).toInt();
            if (anchorPos < cursorPos)
                text.chop(cursorPos - anchorPos);
        }
    }

    if (text.length() > length)
        text = text.right(length);
    return text;
}

/*
  Android docs say that this function should:
  - remove the current composing text, if there is any
  - otherwise remove currently selected text, if there is any
  - insert new text in place of old composing text or, if there was none, at current cursor position
  - mark the inserted text as composing
  - move cursor as specified by newCursorPosition: if > 0, it is relative to the end of inserted
    text - 1; if <= 0, it is relative to the start of inserted text
 */

jboolean QAndroidInputContext::setComposingText(const QString &text, jint newCursorPosition)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return JNI_FALSE;

    BatchEditLock batchEditLock(this);

    const int absoluteCursorPos = getAbsoluteCursorPosition(query);
    int absoluteAnchorPos = getBlockPosition(query) + query->value(Qt::ImAnchorPosition).toInt();

    // If we have composing region and selection (and therefore focusObjectIsComposing() == false),
    // we must clear selection so that we won't delete it when we will be replacing composing text
    if (!m_composingText.isEmpty() && absoluteCursorPos != absoluteAnchorPos) {
        const int cursorPos = query->value(Qt::ImCursorPosition).toInt();
        QInputMethodEvent event({}, { { QInputMethodEvent::Selection, cursorPos, 0 } });
        QGuiApplication::sendEvent(m_focusObject, &event);

        absoluteAnchorPos = absoluteCursorPos;
    }

    // If we had no composing region, pretend that we had a zero-length composing region at current
    // cursor position to simplify code. Also account for that we must delete selected text if there
    // (still) is any.
    const int effectiveAbsoluteCursorPos = qMin(absoluteCursorPos, absoluteAnchorPos);
    if (m_composingTextStart == -1)
        m_composingTextStart = effectiveAbsoluteCursorPos;

    const int oldComposingTextLen = m_composingText.length();
    m_composingText = text;

    const int newAbsoluteCursorPos =
            newCursorPosition <= 0
            ? m_composingTextStart + newCursorPosition
            : m_composingTextStart + m_composingText.length() + newCursorPosition - 1;

    const bool focusObjectWasComposing = focusObjectIsComposing();

    // Same checks as in focusObjectStartComposing()
    if (!m_composingText.isEmpty() && !m_composingText.contains(QLatin1Char('\n'))
            && newAbsoluteCursorPos >= m_composingTextStart
            && newAbsoluteCursorPos <= m_composingTextStart + m_composingText.length())
        m_composingCursor = newAbsoluteCursorPos;
    else
        m_composingCursor = -1;

    QInputMethodEvent event;
    if (focusObjectIsComposing()) {
        QTextCharFormat underlined;
        underlined.setFontUnderline(true);

        event = QInputMethodEvent(m_composingText, {
            { QInputMethodEvent::TextFormat, 0, m_composingText.length(), underlined },
            { QInputMethodEvent::Cursor, m_composingCursor - m_composingTextStart, 1 }
        });

        if (oldComposingTextLen > 0 && !focusObjectWasComposing) {
            event.setCommitString({}, m_composingTextStart - effectiveAbsoluteCursorPos,
                                  oldComposingTextLen);
        }
    } else {
        event = QInputMethodEvent({}, {});

        if (focusObjectWasComposing) {
            event.setCommitString(m_composingText);
        } else {
            event.setCommitString(m_composingText,
                                  m_composingTextStart - effectiveAbsoluteCursorPos,
                                  oldComposingTextLen);
        }
    }

    if (m_composingText.isEmpty())
        clear();

    QGuiApplication::sendEvent(m_focusObject, &event);

    if (!focusObjectIsComposing() && newCursorPosition != 1) {
        // Move cursor using a separate event because if we have inserted or deleted a newline
        // character, then we are now inside an another block

        const int newBlockPos = getBlockPosition(
                focusObjectInputMethodQuery(Qt::ImCursorPosition | Qt::ImAbsolutePosition));

        event = QInputMethodEvent({}, {
            { QInputMethodEvent::Selection, newAbsoluteCursorPos - newBlockPos, 0 }
        });

        QGuiApplication::sendEvent(m_focusObject, &event);
    }

    keyDown();

    return JNI_TRUE;
}

// Android docs say:
// * start may be after end, same meaning as if swapped
// * this function should not trigger updateSelection, but Android's native EditText does trigger it
// * if start == end then we should stop composing
jboolean QAndroidInputContext::setComposingRegion(jint start, jint end)
{
    BatchEditLock batchEditLock(this);

    // Qt will not include the current preedit text in the query results, and interprets all
    // parameters relative to the text excluding the preedit. The simplest solution is therefore to
    // tell Qt that we commit the text before we set the new region. This may cause a little flicker, but is
    // much more robust than trying to keep the two different world views in sync

    finishComposingText();

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return JNI_FALSE;

    if (start == end)
        return JNI_TRUE;
    if (start > end)
        qSwap(start, end);

    QString text = query->value(Qt::ImSurroundingText).toString();
    int textOffset = getBlockPosition(query);

    if (start < textOffset || end > textOffset + text.length()) {
        const int cursorPos = query->value(Qt::ImCursorPosition).toInt();

        if (end - textOffset > text.length()) {
            const QString after = query->value(Qt::ImTextAfterCursor).toString();
            const int additionalSuffixLen = after.length() - (text.length() - cursorPos);

            if (additionalSuffixLen > 0)
                text += after.rightRef(additionalSuffixLen);
        }

        if (start < textOffset) {
            QString before = query->value(Qt::ImTextBeforeCursor).toString();
            before.chop(cursorPos);

            if (!before.isEmpty()) {
                text = before + text;
                textOffset -= before.length();
            }
        }

        if (start < textOffset || end - textOffset > text.length()) {
#ifdef QT_DEBUG_ANDROID_IM_PROTOCOL
            qWarning("setComposingRegion: failed to retrieve text from composing region");
#endif

            return JNI_TRUE;
        }
    }

    m_composingText = text.mid(start - textOffset, end - start);
    m_composingTextStart = start;

    return JNI_TRUE;
}

jboolean QAndroidInputContext::setSelection(jint start, jint end)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return JNI_FALSE;

    BatchEditLock batchEditLock(this);

    int blockPosition = getBlockPosition(query);
    int localCursorPos = start - blockPosition;

    if (focusObjectIsComposing() && start == end && start >= m_composingTextStart
            && start <= m_composingTextStart + m_composingText.length()) {
        // not actually changing the selection; just moving the
        // preedit cursor
        int localOldPos = query->value(Qt::ImCursorPosition).toInt();
        int pos = localCursorPos - localOldPos;
        QList<QInputMethodEvent::Attribute> attributes;
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, pos, 1));

        //but we have to tell Qt about the compose text all over again

        // Show compose text underlined
        QTextCharFormat underlined;
        underlined.setFontUnderline(true);
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,0, m_composingText.length(),
                                                   QVariant(underlined)));
        m_composingCursor = start;

        QInputMethodEvent event(m_composingText, attributes);
        QGuiApplication::sendEvent(m_focusObject, &event);
    } else {
        // actually changing the selection
        focusObjectStopComposing();
        QList<QInputMethodEvent::Attribute> attributes;
        attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection,
                                                       localCursorPos,
                                                       end - start));
        QInputMethodEvent event({}, attributes);
        QGuiApplication::sendEvent(m_focusObject, &event);
    }
    return JNI_TRUE;
}

jboolean QAndroidInputContext::selectAll()
{
    BatchEditLock batchEditLock(this);

    focusObjectStopComposing();
    m_handleMode = ShowCursor;
    sendShortcut(QKeySequence::SelectAll);
    return JNI_TRUE;
}

jboolean QAndroidInputContext::cut()
{
    BatchEditLock batchEditLock(this);

    // This is probably not what native EditText would do, but normally if there is selection, then
    // there will be no composing region
    finishComposingText();

    m_handleMode = ShowCursor;
    sendShortcut(QKeySequence::Cut);
    return JNI_TRUE;
}

jboolean QAndroidInputContext::copy()
{
    BatchEditLock batchEditLock(this);

    focusObjectStopComposing();
    m_handleMode = ShowCursor;
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
    BatchEditLock batchEditLock(this);

    // TODO: This is not what native EditText does
    finishComposingText();

    m_handleMode = ShowCursor;
    sendShortcut(QKeySequence::Paste);
    return JNI_TRUE;
}

void QAndroidInputContext::sendShortcut(const QKeySequence &sequence)
{
    for (int i = 0; i < sequence.count(); ++i) {
        const int keys = sequence[i];
        Qt::Key key = Qt::Key(keys & ~Qt::KeyboardModifierMask);
        Qt::KeyboardModifiers mod = Qt::KeyboardModifiers(keys & Qt::KeyboardModifierMask);

        QKeyEvent pressEvent(QEvent::KeyPress, key, mod);
        QKeyEvent releaseEvent(QEvent::KeyRelease, key, mod);

        QGuiApplication::sendEvent(m_focusObject, &pressEvent);
        QGuiApplication::sendEvent(m_focusObject, &releaseEvent);
    }
}

QSharedPointer<QInputMethodQueryEvent> QAndroidInputContext::focusObjectInputMethodQuery(Qt::InputMethodQueries queries) {
    if (!qGuiApp)
        return {};

    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return {};

    QInputMethodQueryEvent *ret = new QInputMethodQueryEvent(queries);
    QCoreApplication::sendEvent(focusObject, ret);
    return QSharedPointer<QInputMethodQueryEvent>(ret);
}

void QAndroidInputContext::sendInputMethodEvent(QInputMethodEvent *event)
{
    if (!qGuiApp)
        return;

    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return;

    QCoreApplication::sendEvent(focusObject, event);
}

QT_END_NAMESPACE
