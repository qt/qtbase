// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qcore_ohos_p.h"
#include <QtCore/qmetaobject.h>
#include <QtCore/private/qohoslogger_p.h>
#include <deque>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <napi.h>
#include <tuple>
#include <utility>

QT_BEGIN_NAMESPACE

QOhosJsState::~QOhosJsState() = default;

QOhosJsState::QOhosJsState() = default;

QOhosJsThreadOps::~QOhosJsThreadOps() = default;

QOhosJsThreadOps::QOhosJsThreadOps() = default;

static QOhosJsThreadOps *qOhosJsThreadOpsInstance = nullptr;

void QOhosJsThreadOps::registerInstance(QOhosJsThreadOps *ops)
{
    qOhosJsThreadOpsInstance = ops;
}

QOhosJsThreadOps &QOhosJsThreadOps::instance()
{
    Q_ASSERT(qOhosJsThreadOpsInstance != nullptr);
    return *qOhosJsThreadOpsInstance;
}

void QOhosJsThreadGateway::invoke(std::function<void(QOhosJsState &)> task)
{
    QOhosJsThreadOps::instance().invoke(std::move(task));
}

void QOhosJsThreadGateway::invokeAndWaitForContinue(
    std::function<void(QOhosJsState &, std::function<void()>)> &&task)
{
    QOhosJsThreadOps::instance().invokeAndWaitForContinue(std::move(task));
}

void QOhosJsThreadGateway::runAndWait(const std::function<void(QOhosJsState &)> &task)
{
    QOhosJsThreadOps::instance().runAndWait(task);
}

namespace QtOhos {

namespace {

template<typename Task>
class PreQueuingTasksExecutor
{
public:
    void setUnderlyingExecutor(
        std::function<void(Task)> executor,
        std::function<void(Task)> optSyncFlushExecutor = nullptr);
    void invokeTask(Task task);

private:
    std::recursive_mutex m_executorMutex;
    std::function<void(Task)> m_optUnderlyingExecutor;
    std::deque<Task> m_pendingTasks;
};

template<typename Task>
void PreQueuingTasksExecutor<Task>::setUnderlyingExecutor(
    std::function<void(Task)> executor, std::function<void(Task)> optSyncFlushExecutor)
{
    std::lock_guard<std::recursive_mutex> executorLock(m_executorMutex);
    auto &flushExecutor = optSyncFlushExecutor ? optSyncFlushExecutor : executor;
    while (!m_pendingTasks.empty()) {
        auto task = std::move(m_pendingTasks.front());
        m_pendingTasks.pop_front();
        flushExecutor(std::move(task));
    }
    m_optUnderlyingExecutor = std::move(executor);
}

template<typename Task>
void PreQueuingTasksExecutor<Task>::invokeTask(Task task)
{
    std::lock_guard<std::recursive_mutex> executorLock(m_executorMutex);
    if (m_optUnderlyingExecutor)
        m_optUnderlyingExecutor(std::move(task));
    else
        m_pendingTasks.push_back(std::move(task));
}

QOhosConsumer<std::function<void()>> makeQtThreadTasksExecutor()
{
    auto qobj = std::make_shared<QObject>();
    return [qobj](std::function<void()> task) {
        auto sharedTask = moveToSharedPtr(std::move(task));
        QMetaObject::invokeMethod(
            qobj.get(),
            [sharedTask]() {
                (*sharedTask)();
            },
            Qt::QueuedConnection);
    };
}

class QtStateImpl : public QtState
{
public:
    QtStateImpl() = default;

    void initInQtThread();

    bool isQtThread() const override;

    void invokeTask(std::function<void()> &&task) override;

private:
    PreQueuingTasksExecutor<std::function<void()>> m_tasksExecutor;

    QOhosMutexProtectedValue<pthread_t> m_qtThread;
};

void QtStateImpl::initInQtThread()
{
    m_qtThread.processValue(
        [](auto &qtThread) {
            qtThread = pthread_self();
        });

    m_tasksExecutor.setUnderlyingExecutor(
        makeQtThreadTasksExecutor(),
        [](auto task) {
            task();
        });
}

bool QtStateImpl::isQtThread() const
{
    return m_qtThread.evalWithValue(
        [](const auto &qtThread) {
            return qtThread == pthread_self();
        });
}

void QtStateImpl::invokeTask(std::function<void()> &&task)
{
    m_tasksExecutor.invokeTask(std::move(task));
}

QtStateImpl &getQtStateImpl()
{
    static QtStateImpl qtStateImpl;
    return qtStateImpl;
}

}

Q_GLOBAL_STATIC(QObjectThreadSafeRef::ObjectRefsMap, refsMap)

QObjectThreadSafeRef::QObjectThreadSafeRef()
    : QObjectThreadSafeRef(nullptr)
{
}

QObjectThreadSafeRef::QObjectThreadSafeRef(QPointer<QObject> obj)
    : m_creatorThread(::pthread_self())
{
    using namespace std::string_literals;

    if (obj.isNull())
        return;

    QMutexLocker refsMapLock{&refsMapMutex};

    auto refIter = refsMap->find(obj);
    if (refIter == refsMap->end()) {
        auto refName =
            std::to_string(reinterpret_cast<std::size_t>(obj.data()))
            + "/"s + std::to_string(refsMapInsertCounter);
        ++refsMapInsertCounter;

        QObjectRef ref = {
            .obj = obj,
            .refName = refName,
        };
        std::tie(refIter, std::ignore) = refsMap->emplace(obj, std::make_shared<QObjectRef>(ref));

        QObject::connect(
            obj, &QObject::destroyed, obj,
            [](QObject *obj) {
                if (refsMap.isDestroyed())
                    return;
                QMutexLocker refsMapLock{&refsMapMutex};
                refsMap->erase(obj);
            });
    }

    m_refName = refIter->second->refName;
    m_weakObjRef = refIter->second;
}

QObjectThreadSafeRef::QObjectThreadSafeRef(const QObjectThreadSafeRef &other) = default;

QObjectThreadSafeRef &QObjectThreadSafeRef::operator=(const QObjectThreadSafeRef &other) = default;

bool QObjectThreadSafeRef::operator==(const QObjectThreadSafeRef &other) const
{
    return m_refName == other.m_refName;
}

bool QObjectThreadSafeRef::operator!=(const QObjectThreadSafeRef &other) const
{
    return !(*this == other);
}

std::string QObjectThreadSafeRef::refName() const
{
    return m_refName;
}

QPointer<QObject> QObjectThreadSafeRef::data() const
{
    if (m_refName.empty())
        return nullptr;

    if (::pthread_equal(::pthread_self(), m_creatorThread) == 0) {
        qOhosPrintfError("QObjectThreadSafeRef: accessing pointer from wrong thread");
        return nullptr;
    }

    auto objRef = m_weakObjRef.lock();
    return objRef ? objRef->obj : nullptr;
}

void QObjectThreadSafeRef::visitInQtThreadIfAlive(std::function<void(QObject &)> visitFunc) const
{
    invokeInQtThread(
        [objRef = *this, visitFunc = std::move(visitFunc)]() {
            QPointer<QObject> obj = objRef.data();
            if (!obj.isNull())
                visitFunc(*obj);
        });
}

QtState::QtState() = default;

QtState::~QtState() = default;

void initQtThreadState()
{
    getQtStateImpl().initInQtThread();
}

QtState &getQtState()
{
    return getQtStateImpl();
}

void invokeInQtThread(std::function<void()> task)
{
    getQtStateImpl().invokeTask(std::move(task));
}

void logJsCallbackError(const QOhosCallbackInfo &cbInfo, const char *errorMessagePrefix)
{
    static const std::pair<const char *, bool (QNapi::Value::*)() const> errorPropsDefs[] = {
        {"name", &QNapi::Value::IsString},
        {"message", &QNapi::Value::IsString},
        {"code", &QNapi::Value::IsNumber},
    };

    Napi::HandleScope funcScope(cbInfo.Env());

    auto optCbArg = cbInfo.Length() != 0
        ? cbInfo.getFirstArg<QNapi::Value>(Q_FUNC_INFO)
        : cbInfo.Env().Undefined();
    auto error = optCbArg.IsObject()
        ? QNapi::checkedCast<QNapi::Object>(optCbArg)
        : QNapi::Object::New(cbInfo.Env());

    std::string errorDetailsStr;
    for (const auto &propDef : errorPropsDefs) {
        const auto *propName = propDef.first;
        const auto &typeCheckMemFun = propDef.second;

        auto optProp = QNapi::getPropOrUndefined(error, propName);
        if ((optProp.*typeCheckMemFun)()) {
            std::string propStr = optProp.ToString();
            if (!errorDetailsStr.empty())
                errorDetailsStr += ", ";
            errorDetailsStr += propName;
            errorDetailsStr += "='";
            errorDetailsStr += propStr;
            errorDetailsStr += "'";
        }
    }

    std::string errorStr = errorMessagePrefix;
    if (!errorDetailsStr.empty()) {
        errorStr += ": ";
        errorStr += errorDetailsStr;
    }

    qOhosPrintfError("%s", errorStr.c_str());
}

std::function<void(const QOhosCallbackInfo &)> makeErrorLoggingJsCallback(std::string callContext)
{
    using namespace std::string_literals;

    return [callContext = std::move(callContext)](const QOhosCallbackInfo &cbInfo) {
        auto errorMessagePrefix = "Got error from '"s + callContext + "'"s;
        QtOhos::logJsCallbackError(cbInfo, errorMessagePrefix.c_str());
    };
}

}

QOhosJsState &QOhosCallbackInfo::jsState() const
{
    return QOhosJsThreadOps::instance().jsState();
}

QT_END_NAMESPACE
