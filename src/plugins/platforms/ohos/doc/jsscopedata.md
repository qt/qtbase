# JsScopeData

## Problem

This pattern targets Qt-thread classes whose interface is implemented internally on the JS thread. Such a class spans two threads:

- The **Qt thread** owns the C++ object. Qt creates it, calls its public interface, and destroys it here.
- The **JS thread** is where all NAPI/ArkTS APIs run. NAPI references, JS proxy objects, and registered listeners have **thread affinity**: they must be created *and destroyed* on the JS thread.

These conflict: the object lives and dies on the Qt thread, but holds resources that may only be touched on the JS thread. Destroying such a resource from the Qt-thread destructor is undefined behavior.

## Pattern

Bundle all JS-thread-affine state into a private nested `struct JsScopeData` and hold it in a `QtOhos::JsScopeDataOwner<JsScopeData>` member:

```cpp
// lives on the Qt thread
class QOhosFoo : public SomeQtClass
{
    // ... Qt-thread public interface ...

private:
    struct JsScopeData
    {
        QNapi::Reference<QNapi::Object> jsObject;
        std::shared_ptr<void> someListenerHandle;
        // ...only JS-thread state...
    };

    // Qt-thread members live directly in the class, NOT in JsScopeData:
    QIcon m_icon;
    QPointer<QObject> m_focusObject;

    QtOhos::JsScopeDataOwner<JsScopeData> m_jsScopeData;
};
```

The owner is a non-movable Qt-thread member. It creates the bundle on the JS thread the first time it is used (`m_jsScopeData->`, `*m_jsScopeData` or `addAsyncCleanup()`), and releases it on the JS thread when the owner is destroyed. `JsScopeData` must be default-constructible: its members start empty and are populated from JS-thread code, usually in the constructor of the owning object:

```cpp
QOhosFoo::QOhosFoo()
{
    QOhosJsThreadGateway::runAndWait(
        [&](QOhosJsState &jsState) {
            m_jsScopeData->jsObject = makeJsObject(jsState);
            m_jsScopeData->someListenerHandle = registerListener(jsState, /* ... */);
        },
        Q_FUNC_INFO);
}
```

A bundle whose members all come from one JS-thread visit is assigned as a whole instead: `*m_jsScopeData = JsScopeData { ... };`. `JsScopeData` must then be movable: a bundle that declares a destructor has no implicit move operations and must declare them, or the assignment silently copies.

A class whose bundle exists only part of the time (e.g. while a tray icon is shown) holds `std::optional<QtOhos::JsScopeDataOwner<JsScopeData>> m_optJsScopeData`, calls `emplace()` on the Qt thread before populating and `reset()` there to release.

You do not write any teardown code in the destructor: when `m_jsScopeData` or the owning object is destroyed (on the Qt thread, or any thread other than the JS thread), the contents are released on the JS thread automatically. The owning object's destructor can stay trivial. If populating fails, release whatever was populated the same way, by destroying the owner, or resetting the `std::optional` holding it, on the Qt thread.

An object that is itself created or destroyed on the JS thread is outside this pattern; it keeps its JS-thread state in a `std::shared_ptr` wrapped with `QtOhos::makeProxyWithJsThreadDeleter()`.

## Asynchronous cleanup

The bundle's member destructors are synchronous. When releasing a resource takes asynchronous JS calls (a destroy function returning a promise, an `off()` that must complete before the next step), register the release as a cleanup, on the JS thread, where the resource is acquired:

```cpp
m_jsScopeData->jsObject = makeJsObject(jsState);
m_jsScopeData.addAsyncCleanup(
    [](QOhosJsState &, JsScopeData &data, QOhosTaskPromise<> donePromise) {
        data.jsObject.evalToPromiseOrRejectOnThrow("destroy()")
        .onCatch(QtOhos::makeErrorLoggingJsCallback("destroy()"))
        .onFinally(std::move(donePromise).makeChained(Q_FUNC_INFO));
    });
```

- Cleanups run on the JS thread in reverse registration order, before the members are destroyed; the bundle stays valid for the whole chain. The owner's destructor waits until the last one has settled.
- A cleanup settles its promise once its JS calls have completed, whether they succeeded or failed. One that throws or drops the promise is logged and the chain continues.
- Several asynchronous steps are either one cleanup each, or one cleanup chaining them.

## Rules

1. **Strict partition.** Only JS-thread-affine state goes in `JsScopeData`; only Qt-thread state goes in the other members. Accessing the bundle from Qt-thread code, through `m_jsScopeData` or a dereference of `m_optJsScopeData`, is strictly forbidden. Accessing Qt-thread state (other members) from JS-thread code is allowed (unless forbidden for other reasons) if proper synchronization is ensured (e.g. inside `QOhosJsThreadGateway::runAndWait()` blocks).

2. **Access syntax tags thread affinity.** With the partition held, `m_jsScopeData` or a dereference of `m_optJsScopeData` (`*m_optJsScopeData`, `m_optJsScopeData->`) means "JS-thread state", and `m_def` means "Qt-thread state" at every use site. `m_optJsScopeData` itself is Qt-thread state like any other `m_...` member. This makes wrong-thread access syntactically visible:
   - `m_jsScopeData`, or a dereference of `m_optJsScopeData`, *outside* a `QOhosJsThreadGateway::runAndWait`, `QOhosJsThreadGateway::eval`, etc. closure is a red flag.
   - Touching a plain `m_...` member *inside* a JS-thread closure requires extra attention.

3. **Create and touch `JsScopeData` contents only inside JS-thread closures** (`QOhosJsThreadGateway::runAndWait`, `QOhosJsThreadGateway::eval`, etc.). The Qt thread only ever constructs or destroys the owner, never dereferences it for JS work.

## Why bundle instead of per-resource handles

Each resource could carry its own JS-thread-hopping deleter, but bundling gives:

- **One thread hop on teardown** instead of N blocking cross-thread round-trips.
- **Atomic lifetime** - everything created in one JS-thread visit, released in one teardown, even when that teardown spans asynchronous cleanups.
- **Visible thread affinity at call sites** - every JS-state access reads `m_jsScopeData->...`, distinct from Qt-state members, so wrong-thread access stands out (see Rule 2).
