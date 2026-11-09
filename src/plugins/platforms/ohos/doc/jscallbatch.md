# JsCallBatch

## Problem

Gateway classes such as `QOhosWindowProxy` and `QNativeNode` give the Qt thread access to objects that live on the JS thread. Their member functions do their work there, most of them through one or more blocking calls to `QOhosJsThreadGateway::runAndWait()` or `invokeAndWaitForContinue()`. A caller that makes several such calls in a row pays one round trip per JS-thread call, and each round trip can wait behind unrelated work on the JS thread.

`QtOhos::JsCallBatch` lets the caller group these calls into one round trip. The caller keeps calling the gateway's member functions and doesn't need to know which JS-thread calls they make.

A sync point is a call from the Qt thread to the JS thread that isn't queued, including a member function called without a batch. It runs the queued calls before its own work. A JS-thread task is one run of work from the JS thread's event loop. Other JS-thread work, such as an ArkUI layout, can run between two JS-thread tasks.

## Using a batch

```cpp
if (m_fooProxy && needsUpdate) {
    QtOhos::JsCallBatch jsCallBatch(Q_FUNC_INFO);
    m_fooProxy->setTitle(title, jsCallBatch);
    m_fooProxy->setTouchable(touchable, jsCallBatch);
}
```

Calls submitted to the batch are queued and run at the latest when the batch goes out of scope. A member function called without a batch gets `JsCallBatch::immediate()`, which runs its JS-thread calls right away. A batch created on another thread does the same. The guarantees below apply to a batch the caller creates on the Qt thread:

- The JS-thread calls run in the order of the member function calls, also relative to the other calls the Qt thread makes to the JS thread while the batch exists. A blocking sync point runs the queued calls in the same round trip as its own work, and `QOhosJsThreadGateway::invoke()` runs them in a separate round trip before it posts its task. Calls from other threads are not ordered against the queued calls and can run before them. All batches share one queue, so they can't reorder each other, and the end of any batch also runs the calls an enclosing batch has queued.
- The Qt thread waits until every queued call has settled, at the end of the batch or at the first sync point before it. Even `invoke()`, which otherwise doesn't wait, waits there. A sync point can hide in code that doesn't look like a JS call, for example a getter that uses `eval()`, the destruction of a `JsScopeDataOwner`, or the release of a handle made with `makeProxyWithJsThreadDeleter()` or `makeProxyWithAsyncJsThreadDeleter()`. Calls submitted after that are queued again and waited for at the next sync point or at the end of the batch.
- Each queued call starts once the previous one has settled. A run of adjacent calls that are done when they return, which includes every call submitted without a promise, runs in one JS-thread task, so no other JS-thread work, such as an ArkUI layout, runs between them. A call that settles its promise later lets other JS-thread work run until it does, and the calls after it run in the callback that settles it. Code that relies on two calls running with no layout in between submits both to the same batch, next to each other, and the first one must be done when it returns.

A batch pays off when at least two calls to the JS thread would otherwise wait one after another, including a setter followed by a getter. It gains nothing for a single call that nothing follows. Calls made after a sync point in the scope are grouped only with the calls after them.

## Rules for callers

1. **Smallest scope.** The batch lives in the smallest block that holds exactly the calls it groups. A dedicated `{ }` block is needed only when the enclosing block goes on past the batched calls. A getter that follows the batched calls can be in the scope too. It runs them in the same round trip as its own work and then reads the state they have left.
2. **Qt-side code in the scope may run first.** Code in the batch's scope may run before the queued calls take effect, so it may read only Qt-side state that they don't change. Code that can run application code stays outside the batch, because the application can destroy the called objects or run a nested event loop that keeps the calls queued. This includes emitting a signal, sending an event, showing a window and setting up a window proxy, which can show its parent window. Posting an event is fine. Running the event loop inside a batch, for example in a nested event loop or `QCoreApplication::processEvents()`, logs a warning that names the batch, once per batch. Other application code in the scope goes undetected, such as a slot connected to a signal emitted there or events delivered by `QCoreApplication::sendPostedEvents()`.
3. **Called objects outlive the batch.** Destroying an object while calls to it are queued is a misuse that nothing reliably detects. The queued calls run at the first call to the JS thread during the destruction, against an object that is being destroyed. If the destruction makes no such call, for example because the object's `JsScopeDataOwner` never created its data, they run later on freed memory.
4. **Pass the batch on.** A function that takes a batch passes it to every call it makes that takes one. No call spells out `JsCallBatch::immediate()` as an argument.
5. **Naming.** A batch variable or parameter is named `jsCallBatch`. Each batch is created with `Q_FUNC_INFO`, so that messages about it can say which function it belongs to.

## Making a gateway member function batchable

A batchable member function hands its JS-thread work to `submit()` as a task:

```cpp
class QOhosFooProxy
{
public:
    using JsCallBatch = QtOhos::JsCallBatch;

    void setTitle(const QString &title, JsCallBatch &jsCallBatch = JsCallBatch::immediate());
    QString title() const;
};

void QOhosFooProxy::setTitle(const QString &title, JsCallBatch &jsCallBatch)
{
    qCDebug(QtForOhos, "%s", Q_FUNC_INFO);
    jsCallBatch.submit(
        [this, title](QOhosJsState &, QOhosTaskPromise<> taskPromise) {
            if (m_jsScopeData->isClosing()) {
                taskPromise();
                return;
            }

            m_jsScopeData->jsFoo.evalToPromiseOrRejectOnThrow("setTitle(*)", {title.toStdString()})
            .onCatch(QtOhos::makeErrorLoggingJsCallback(Q_FUNC_INFO))
            .onFinally(std::move(taskPromise).makeChained(Q_FUNC_INFO));
        },
        Q_FUNC_INFO);
}
```

1. **Only void member functions whose Qt side doesn't need the JS result.** Getters, such as `title()` above, member functions that return a value and member functions that store a handle after their round trip take no batch. Passing a batch to them doesn't compile, and those that call the JS thread act as sync points.
2. **Qt-side work before the submit.** The body may log, prepare arguments, check state that the queued calls don't change and do Qt-side bookkeeping that doesn't depend on the JS result, all before submitting. Nothing after a `submit()` may depend on its effect, because with a batch the task hasn't run yet. The body makes no other call to the JS thread. JS state that the task needs, such as the current window size, is read inside the task, where it also reflects the calls queued before it.
3. **One task.** All JS-thread work of the member function goes in one task. Without a batch, each `submit()` is a round trip of its own.
4. **Capture owned values.** A queued task outlives the member function call. It captures `this` and values it owns, such as copies of the arguments or a `std::shared_ptr`, and never references or raw pointers to arguments or to other objects. `QNativeNode::setParent(std::shared_ptr<QXComponentNode>)`, for example, shares ownership of the new parent. A member function whose task can't do without a reference or raw pointer to another object takes no batch, because caller rule 3 covers only the called object. A Qt-side member that the task reads is read when the task runs, not when it is submitted. Capture its value instead, unless it can't change after construction.
5. **Captures may be destroyed on the JS thread.** A queued task and its captured copies are destroyed on the JS thread, so capture only values that may be destroyed there. For a Qt type that isn't safe there, such as `QCursor`, capture the data the task needs, such as the cursor shape. Without a batch, a task without a promise is destroyed on the Qt thread, so a wrong capture can pass every unbatched run.
6. **Choose the overload by completion.** A task that may complete asynchronously takes a `QOhosTaskPromise<>` and settles it once on every path, including early returns, on the JS thread. Only queued calls check the thread, so a task that settles elsewhere passes without a batch and aborts with one. A task that is done when it returns uses the overload without a promise. A queued task that throws or drops its promise aborts, and the message names the `callerContextName` passed to `submit()`.
7. **Convert one to one.** A member function that used `runAndWait()` submits a task without a promise, and one that used `invokeAndWaitForContinue()` submits a task with one. With the default `JsCallBatch::immediate()`, `submit()` calls exactly these functions, so callers that pass no batch see no change. A member function built on `eval()` or `evalWithPromise()` returns a result and stays as it is.
8. **Trailing defaulted parameter.** The batch is the last parameter, `JsCallBatch &jsCallBatch = JsCallBatch::immediate()`. A class outside `QtOhos` declares `using JsCallBatch = QtOhos::JsCallBatch;` in its public section.

## Passing a batch through other classes

A class that implements its own behavior with gateways, such as `QOhosView`, decides on batching internally:

1. **Private only.** Only gateway classes take a batch in public member functions. Batching is the class's own decision, and a batch parameter in its public API would expose the JS thread to callers that don't deal with it. A public function that needs batching delegates to a private helper that takes the batch.
2. **Alias in the private section.** The class declares `using JsCallBatch = QtOhos::JsCallBatch;` at the start of its private section.
3. **Default only while needed.** A helper's batch parameter defaults to `JsCallBatch::immediate()` only while some caller passes no batch. When every caller passes one, the parameter is required.

## Why an explicit batch

- **No hidden deferral.** A call is deferred only when the call site passes a batch, so reading the code shows which calls are queued.
- **No member list to maintain.** A gateway member function opts in through its own parameter, so no separate builder repeats the gateway's interface.
- **No reordering on the JS thread.** Every call from the Qt thread to the JS thread that takes no batch is a sync point, so mixing batched and unbatched calls keeps the order of the Qt thread's calls to the JS thread. Only Qt-side code in the batch's scope can run earlier than it would without the batch, which caller rule 2 covers.
