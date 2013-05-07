/****************************************************************************
**
** Copyright (C) 2012 BogDan Vatra <bogdan@kde.org>
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <android/log.h>

#include "qandroidinputcontext.h"
#include "androidjnimain.h"
#include "androidjniinput.h"
#include <QDebug>
#include <qevent.h>
#include <qguiapplication.h>
#include <qsharedpointer.h>
#include <qthread.h>
#include <qinputmethod.h>
#include <qwindow.h>

#include <QTextCharFormat>

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

static jboolean commitText(JNIEnv *env, jobject /*thiz*/, jstring text, jint newCursorPosition)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    jboolean isCopy;
    const jchar *jstr = env->GetStringChars(text, &isCopy);
    QString str(reinterpret_cast<const QChar *>(jstr), env->GetStringLength(text));
    env->ReleaseStringChars(text, jstr);

    return m_androidInputContext->commitText(str, newCursorPosition);
}

static jboolean deleteSurroundingText(JNIEnv */*env*/, jobject /*thiz*/, jint leftLength, jint rightLength)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->deleteSurroundingText(leftLength, rightLength);
}

static jboolean finishComposingText(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

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
    return env->NewString(reinterpret_cast<const jchar *>(text.constData()), jsize(text.length()));
}

static jstring getTextAfterCursor(JNIEnv *env, jobject /*thiz*/, jint length, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    const QString &text = m_androidInputContext->getTextAfterCursor(length, flags);
    return env->NewString(reinterpret_cast<const jchar *>(text.constData()), jsize(text.length()));
}

static jstring getTextBeforeCursor(JNIEnv *env, jobject /*thiz*/, jint length, jint flags)
{
    if (!m_androidInputContext)
        return 0;

    const QString &text = m_androidInputContext->getTextBeforeCursor(length, flags);
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

    return m_androidInputContext->setComposingText(str, newCursorPosition);
}

static jboolean setSelection(JNIEnv */*env*/, jobject /*thiz*/, jint start, jint end)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->setSelection(start, end);
}

static jboolean selectAll(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->selectAll();
}

static jboolean cut(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->cut();
}

static jboolean copy(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->copy();
}

static jboolean copyURL(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->copyURL();
}

static jboolean paste(JNIEnv */*env*/, jobject /*thiz*/)
{
    if (!m_androidInputContext)
        return JNI_FALSE;

    return m_androidInputContext->paste();
}


static JNINativeMethod methods[] = {
    {"commitText", "(Ljava/lang/String;I)Z", (void *)commitText},
    {"deleteSurroundingText", "(II)Z", (void *)deleteSurroundingText},
    {"finishComposingText", "()Z", (void *)finishComposingText},
    {"getCursorCapsMode", "(I)I", (void *)getCursorCapsMode},
    {"getExtractedText", "(III)Lorg/qtproject/qt5/android/QtExtractedText;", (void *)getExtractedText},
    {"getSelectedText", "(I)Ljava/lang/String;", (void *)getSelectedText},
    {"getTextAfterCursor", "(II)Ljava/lang/String;", (void *)getTextAfterCursor},
    {"getTextBeforeCursor", "(II)Ljava/lang/String;", (void *)getTextBeforeCursor},
    {"setComposingText", "(Ljava/lang/String;I)Z", (void *)setComposingText},
    {"setSelection", "(II)Z", (void *)setSelection},
    {"selectAll", "()Z", (void *)selectAll},
    {"cut", "()Z", (void *)cut},
    {"copy", "()Z", (void *)copy},
    {"copyURL", "()Z", (void *)copyURL},
    {"paste", "()Z", (void *)paste}
};


QAndroidInputContext::QAndroidInputContext():QPlatformInputContext()
{
    QtAndroid::AttachedJNIEnv env;
    if (!env.jniEnv)
        return;

    jclass clazz = QtAndroid::findClass(QtNativeInputConnectionClassName, env.jniEnv);
    if (clazz == NULL) {
        qCritical() << "Native registration unable to find class '"
                    << QtNativeInputConnectionClassName
                    << "'";
        return;
    }

    if (env.jniEnv->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) < 0) {
        qCritical() << "RegisterNatives failed for '"
                    << QtNativeInputConnectionClassName
                    << "'";
        return;
    }

    clazz = QtAndroid::findClass(QtExtractedTextClassName, env.jniEnv);
    if (clazz == NULL) {
        qCritical() << "Native registration unable to find class '"
                    << QtExtractedTextClassName
                    << "'";
        return;
    }

    m_extractedTextClass = static_cast<jclass>(env.jniEnv->NewGlobalRef(clazz));
    m_classConstructorMethodID = env.jniEnv->GetMethodID(m_extractedTextClass, "<init>", "()V");
    if (m_classConstructorMethodID == NULL) {
        qCritical() << "GetMethodID failed";
        return;
    }

    m_partialEndOffsetFieldID = env.jniEnv->GetFieldID(m_extractedTextClass, "partialEndOffset", "I");
    if (m_partialEndOffsetFieldID == NULL) {
        qCritical() << "Can't find field partialEndOffset";
        return;
    }

    m_partialStartOffsetFieldID = env.jniEnv->GetFieldID(m_extractedTextClass, "partialStartOffset", "I");
    if (m_partialStartOffsetFieldID == NULL) {
        qCritical() << "Can't find field partialStartOffset";
        return;
    }

    m_selectionEndFieldID = env.jniEnv->GetFieldID(m_extractedTextClass, "selectionEnd", "I");
    if (m_selectionEndFieldID == NULL) {
        qCritical() << "Can't find field selectionEnd";
        return;
    }

    m_selectionStartFieldID = env.jniEnv->GetFieldID(m_extractedTextClass, "selectionStart", "I");
    if (m_selectionStartFieldID == NULL) {
        qCritical() << "Can't find field selectionStart";
        return;
    }

    m_startOffsetFieldID = env.jniEnv->GetFieldID(m_extractedTextClass, "startOffset", "I");
    if (m_startOffsetFieldID == NULL) {
        qCritical() << "Can't find field startOffset";
        return;
    }

    m_textFieldID = env.jniEnv->GetFieldID(m_extractedTextClass, "text", "Ljava/lang/String;");
    if (m_textFieldID == NULL) {
        qCritical() << "Can't find field text";
        return;
    }
    qRegisterMetaType<QInputMethodEvent *>("QInputMethodEvent*");
    qRegisterMetaType<QInputMethodQueryEvent *>("QInputMethodQueryEvent*");
    m_androidInputContext = this;
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

void QAndroidInputContext::reset()
{
    clear();
    if (qGuiApp->focusObject())
        QtAndroidInput::resetSoftwareKeyboard();
    else
        QtAndroidInput::hideSoftwareKeyboard();
}

void QAndroidInputContext::commit()
{
    finishComposingText();
}

void QAndroidInputContext::updateCursorPosition()
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (!query.isNull()) {
        const int cursorPos = query->value(Qt::ImCursorPosition).toInt();
        QtAndroidInput::updateSelection(cursorPos, cursorPos, -1, -1); //selection empty and no pre-edit text
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

    if (action == QInputMethod::Click)
        commit();
}

QRectF QAndroidInputContext::keyboardRect() const
{
    return QPlatformInputContext::keyboardRect();
}

bool QAndroidInputContext::isAnimating() const
{
    return false;
}

void QAndroidInputContext::showInputPanel()
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return;

    disconnect(m_updateCursorPosConnection);
    if (qGuiApp->focusObject()->metaObject()->indexOfSignal("cursorPositionChanged(int,int)") >= 0) // QLineEdit breaks the pattern
        m_updateCursorPosConnection = connect(qGuiApp->focusObject(), SIGNAL(cursorPositionChanged(int,int)), this, SLOT(updateCursorPosition()));
    else
        m_updateCursorPosConnection = connect(qGuiApp->focusObject(), SIGNAL(cursorPositionChanged()), this, SLOT(updateCursorPosition()));
    QRectF itemRect = qGuiApp->inputMethod()->inputItemRectangle();
    QRect rect = qGuiApp->inputMethod()->inputItemTransform().mapRect(itemRect).toRect();
    QWindow *window = qGuiApp->focusWindow();
    if (window)
        rect = QRect(window->mapToGlobal(rect.topLeft()), rect.size());

    QtAndroidInput::showSoftwareKeyboard(rect.left(),
                                         rect.top(),
                                         rect.width(),
                                         rect.height(),
                                         query->value(Qt::ImHints).toUInt());
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
    m_extractedText.clear();
}

void QAndroidInputContext::sendEvent(QObject *receiver, QInputMethodEvent *event)
{
    QCoreApplication::sendEvent(receiver, event);
}

void QAndroidInputContext::sendEvent(QObject *receiver, QInputMethodQueryEvent *event)
{
    QCoreApplication::sendEvent(receiver, event);
}

jboolean QAndroidInputContext::commitText(const QString &text, jint /*newCursorPosition*/)
{
    m_composingText = text;
    return finishComposingText();
}

jboolean QAndroidInputContext::deleteSurroundingText(jint leftLength, jint rightLength)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return JNI_TRUE;

    m_composingText.clear();

    QInputMethodEvent event;
    event.setCommitString(QString(), -leftLength, leftLength+rightLength);
    sendInputMethodEvent(&event);
    clear();

    return JNI_TRUE;
}

jboolean QAndroidInputContext::finishComposingText()
{
    QInputMethodEvent event;
    event.setCommitString(m_composingText);
    sendInputMethodEvent(&event);
    clear();

    return JNI_TRUE;
}

jint QAndroidInputContext::getCursorCapsMode(jint /*reqModes*/)
{
    jint res = 0;
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return res;

    const uint qtInputMethodHints = query->value(Qt::ImHints).toUInt();

    if (qtInputMethodHints & Qt::ImhPreferUppercase)
        res = CAP_MODE_SENTENCES;

    if (qtInputMethodHints & Qt::ImhUppercaseOnly)
        res = CAP_MODE_CHARACTERS;

    return res;
}

const QAndroidInputContext::ExtractedText &QAndroidInputContext::getExtractedText(jint hintMaxChars, jint /*hintMaxLines*/, jint /*flags*/)
{
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return m_extractedText;

    if (hintMaxChars)
        m_extractedText.text = query->value(Qt::ImSurroundingText).toString().right(hintMaxChars);

    m_extractedText.startOffset = query->value(Qt::ImCursorPosition).toInt();
    const QString &selection = query->value(Qt::ImCurrentSelection).toString();
    const int selLen = selection.length();
    if (selLen) {
        m_extractedText.selectionStart = query->value(Qt::ImAnchorPosition).toInt();
        m_extractedText.selectionEnd = m_extractedText.startOffset;
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
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
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
    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (query.isNull())
        return QString();

    QString text = query->value(Qt::ImSurroundingText).toString();
    if (!text.length())
        return text;

    int cursorPos = query->value(Qt::ImCursorPosition).toInt();
    const int wordLeftPos = cursorPos - length;
    return text.mid(wordLeftPos > 0 ? wordLeftPos : 0, cursorPos);
}

jboolean QAndroidInputContext::setComposingText(const QString &text, jint newCursorPosition)
{
    if (newCursorPosition > 0)
        newCursorPosition += text.length() - 1;
    m_composingText = text;
    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Cursor,
                                                   newCursorPosition,
                                                   1,
                                                   QVariant()));
    // Show compose text underlined
    QTextCharFormat underlined;
    underlined.setFontUnderline(true);
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::TextFormat,0, text.length(),
                                                   QVariant(underlined)));

    QInputMethodEvent event(m_composingText, attributes);
    sendInputMethodEvent(&event);

    QSharedPointer<QInputMethodQueryEvent> query = focusObjectInputMethodQuery();
    if (!query.isNull()) {
        int cursorPos = query->value(Qt::ImCursorPosition).toInt();
        int preeditLength = text.length();
        QtAndroidInput::updateSelection(cursorPos+preeditLength, cursorPos+preeditLength, cursorPos, cursorPos+preeditLength);
    }

    return JNI_TRUE;
}

jboolean QAndroidInputContext::setSelection(jint start, jint end)
{
    QList<QInputMethodEvent::Attribute> attributes;
    attributes.append(QInputMethodEvent::Attribute(QInputMethodEvent::Selection,
                                                   start,
                                                   end - start,
                                                   QVariant()));

    QInputMethodEvent event(QString(), attributes);
    sendInputMethodEvent(&event);
    return JNI_TRUE;
}

jboolean QAndroidInputContext::selectAll()
{
#warning TODO
    return JNI_FALSE;
}

jboolean QAndroidInputContext::cut()
{
#warning TODO
    return JNI_FALSE;
}

jboolean QAndroidInputContext::copy()
{
#warning TODO
    return JNI_FALSE;
}

jboolean QAndroidInputContext::copyURL()
{
#warning TODO
    return JNI_FALSE;
}

jboolean QAndroidInputContext::paste()
{
#warning TODO
    return JNI_FALSE;
}

QSharedPointer<QInputMethodQueryEvent> QAndroidInputContext::focusObjectInputMethodQuery(Qt::InputMethodQueries queries)
{
#warning TODO make qGuiApp->focusObject() thread safe !!!
    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return QSharedPointer<QInputMethodQueryEvent>();

    QSharedPointer<QInputMethodQueryEvent> ret = QSharedPointer<QInputMethodQueryEvent>(new QInputMethodQueryEvent(queries));
    if (qGuiApp->thread()==QThread::currentThread()) {
        QCoreApplication::sendEvent(focusObject, ret.data());
    } else {
        QMetaObject::invokeMethod(this,
                                  "sendEvent",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QObject*, focusObject),
                                  Q_ARG(QInputMethodQueryEvent*, ret.data()));
    }

    return ret;
}

void QAndroidInputContext::sendInputMethodEvent(QInputMethodEvent *event)
{
#warning TODO make qGuiApp->focusObject() thread safe !!!
    QObject *focusObject = qGuiApp->focusObject();
    if (!focusObject)
        return;

    if (qGuiApp->thread() == QThread::currentThread()) {
        QCoreApplication::sendEvent(focusObject, event);
    } else {
        QMetaObject::invokeMethod(this,
                                  "sendEvent",
                                  Qt::BlockingQueuedConnection,
                                  Q_ARG(QObject*, focusObject),
                                  Q_ARG(QInputMethodEvent*, event));
    }
}

QT_END_NAMESPACE
