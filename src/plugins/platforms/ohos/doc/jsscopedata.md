# JsScopeData

## Problem

This pattern targets Qt-thread classes whose interface is implemented internally on the JS thread. Such a class spans two threads:

- The **Qt thread** owns the C++ object. Qt creates it, calls its public interface, and destroys it here.
- The **JS thread** is where all NAPI/ArkTS APIs run. NAPI references, JS proxy objects, and registered listeners have **thread affinity**: they must be created *and destroyed* on the JS thread.

These conflict: the object lives and dies on the Qt thread, but holds resources that may only be touched on the JS thread. Destroying such a resource from the Qt-thread destructor is undefined behavior.

## Pattern

Bundle all JS-thread-affine state into a private nested `struct JsScopeData`, held by a `shared_ptr` whose deleter performs destruction on the JS thread:

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

    std::shared_ptr<JsScopeData> m_jsScopeData;
};
```

`m_jsScopeData` always holds a `JsScopeData` wrapped with `QtOhos::makeProxyWithJsThreadDeleter()`, which makes the deleter hop to the JS thread. There are two ways to set it up; pick whichever fits.

**Option A** - create it empty on the Qt thread, then populate it on the JS thread. Usable only when every `JsScopeData` member is safe to default-construct off the JS thread (e.g. an empty `QNapi::Reference` or `std::shared_ptr`); otherwise use Option B.

```cpp
m_jsScopeData = QtOhos::makeProxyWithJsThreadDeleter(std::make_shared<JsScopeData>());

QtOhos::runInJsThreadAndWait(
    [&](QtOhos::JsState &jsState) {
        m_jsScopeData->jsObject = makeJsObject(jsState);
        m_jsScopeData->someListenerHandle = registerListener(jsState, /* ... */);
    },
    Q_FUNC_INFO);
```

**Option B** - build the fully populated bundle on the JS thread in one step (e.g. via `evalInJsThread()`):

```cpp
m_jsScopeData = QtOhos::evalInJsThread(
    [&](QtOhos::JsState &jsState) {
        return QtOhos::makeProxyWithJsThreadDeleter(
            QtOhos::moveToSharedPtr(
                JsScopeData{
                    .jsObject = ...,
                    ...
                }));
    },
    Q_FUNC_INFO);
```

Whichever you pick, you do not write any teardown code: when `m_jsScopeData` is reset or the owning object is destroyed (on any thread), the contents are released on the JS thread automatically. The owner's destructor can stay trivial.

After construction, read or modify `m_jsScopeData` contents only from code running in the JS thread, exactly as in Option A.

## Rules

1. **Strict partition.** Only JS-thread-affine state goes in `JsScopeData`; only Qt-thread state goes in the other members. Accessing `m_jsScopeData->...` from Qt-thread code is strictly forbidden. Accessing Qt-thread state (other members) from JS-thread code is allowed (unless forbidden for other reasons) if proper synchronization is ensured (e.g. inside `runInJsThreadAndWait()` blocks).

2. **Access syntax tags thread affinity.** With the partition held, `m_jsScopeData->abc` means "JS-thread state" and `m_def` means "Qt-thread state" at every use site. This makes wrong-thread access syntactically visible:
   - A bare `m_jsScopeData->...` *outside* a `runInJsThreadAndWait`, `evalInJsThread`, etc. closure is a red flag.
   - Touching a plain `m_...` member *inside* a JS-thread closure requires extra attention.

3. **Create and touch `JsScopeData` contents only inside JS-thread closures** (`runInJsThreadAndWait`, `evalInJsThread`, etc.). The Qt thread only ever moves/resets the `shared_ptr` itself, never dereferences it for JS work.

## Why bundle instead of per-resource handles

Each resource could carry its own JS-thread-hopping deleter, but bundling gives:

- **One thread hop on teardown** instead of N blocking cross-thread round-trips.
- **Atomic lifetime** - everything created in one JS-thread visit, released in one.
- **Visible thread affinity at call sites** - every JS-state access reads `m_jsScopeData->...`, distinct from Qt-state members, so wrong-thread access stands out (see Rule 2).
