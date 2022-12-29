// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/QCoreApplication>
#include <QtCore/QEvent>
#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtGui/private/qwasmlocalfileaccess_p.h>

#include <qtwasmtestlib.h>
#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <string_view>

using namespace emscripten;

class FilesTest : public QObject
{
    Q_OBJECT

public:
    FilesTest() : m_window(val::global("window")), m_testSupport(val::object()) {}

    ~FilesTest() noexcept {
        for (auto& cleanup: m_cleanup) {
            cleanup();
        }
    }

private:
    void init() {
        EM_ASM({
            window.testSupport = {};

            window.showOpenFilePicker = sinon.stub();

            window.mockOpenFileDialog = (files) => {
                window.showOpenFilePicker.withArgs(sinon.match.any).callsFake(
                    (options) => Promise.resolve(files.map(file => {
                        const getFile = sinon.stub();
                        getFile.callsFake(() => Promise.resolve({
                            name: file.name,
                            size: file.content.length,
                            slice: () => new Blob([new TextEncoder().encode(file.content)]),
                        }));
                        return {
                            kind: 'file',
                            name: file.name,
                            getFile
                        };
                    }))
                );
            };

            window.showSaveFilePicker = sinon.stub();

            window.mockSaveFilePicker = (file) => {
                window.showSaveFilePicker.withArgs(sinon.match.any).callsFake(
                    (options) => {
                        const createWritable = sinon.stub();
                        createWritable.callsFake(() => {
                            const write = file.writeFn ?? (() => {
                                const write = sinon.stub();
                                write.callsFake((stuff) => {
                                    if (file.content !== new TextDecoder().decode(stuff)) {
                                        const message = `Bad file content ${file.content} !== ${new TextDecoder().decode(stuff)}`;
                                        Module.qtWasmFail(message);
                                        return Promise.reject(message);
                                    }

                                    return Promise.resolve();
                                });
                                return write;
                            })();

                            window.testSupport.write = write;

                            const close = file.closeFn ?? (() => {
                                const close = sinon.stub();
                                close.callsFake(() => Promise.resolve());
                                return close;
                            })();

                            window.testSupport.close = close;

                            return Promise.resolve({
                                write,
                                close
                            });
                        });
                        return Promise.resolve({
                            kind: 'file',
                            name: file.name,
                            createWritable
                        });
                    }
                );
            };
        });
    }

    template <class T>
    T* Own(T* plainPtr) {
        m_cleanup.emplace_back([plainPtr]() mutable {
            delete plainPtr;
        });
        return plainPtr;
    }

    val m_window;
    val m_testSupport;

    std::vector<std::function<void()>> m_cleanup;

private slots:
    void selectOneFileWithFileDialog();
    void selectMultipleFilesWithFileDialog();
    void cancelFileDialog();
    void rejectFile();
    void saveFileWithFileDialog();
};

class BarrierCallback {
public:
    BarrierCallback(int number, std::function<void()> onDone)
        : m_remaining(number), m_onDone(std::move(onDone)) {}

    void operator()() {
        if (!--m_remaining) {
            m_onDone();
        }
    }

private:
    int m_remaining;
    std::function<void()> m_onDone;
};


template <class Arg>
std::string argToString(std::add_lvalue_reference_t<std::add_const_t<Arg>> arg) {
    return std::to_string(arg);
}

template <>
std::string argToString<bool>(const bool& value) {
    return value ? "true" : "false";
}

template <>
std::string argToString<std::string>(const std::string& arg) {
    return arg;
}

template <>
std::string argToString<const std::string&>(const std::string& arg) {
    return arg;
}

template<class Type>
struct Matcher {
    virtual ~Matcher() = default;

    virtual bool matches(std::string* explanation, const Type& actual) const = 0;
};

template<class Type>
struct AnyMatcher : public Matcher<Type> {
    bool matches(std::string* explanation, const Type& actual) const final {
        Q_UNUSED(explanation);
        Q_UNUSED(actual);
        return true;
    }

    Type m_value;
};

template<class Type>
struct EqualsMatcher : public Matcher<Type> {
    EqualsMatcher(Type value) : m_value(std::forward<Type>(value)) {}

    bool matches(std::string* explanation, const Type& actual) const final {
        const bool ret = actual == m_value;
        if (!ret)
            *explanation += argToString<Type>(actual) + " != " + argToString<Type>(m_value);
        return actual == m_value;
    }

    // It is crucial to hold a copy, otherwise we lose const refs.
    std::remove_reference_t<Type> m_value;
};

template<class Type>
std::unique_ptr<EqualsMatcher<Type>> equals(Type value) {
    return std::make_unique<EqualsMatcher<Type>>(value);
}

template<class Type>
std::unique_ptr<AnyMatcher<Type>> any(Type value) {
    return std::make_unique<AnyMatcher<Type>>(value);
}

template <class ...Types>
struct Expectation {
    std::tuple<std::unique_ptr<Matcher<Types>>...> m_argMatchers;
    int m_callCount = 0;
    int m_expectedCalls = 1;

    template<std::size_t... Indices>
    bool match(std::string* explanation, const std::tuple<Types...>& tuple, std::index_sequence<Indices...>) const {
        return ( ... && (std::get<Indices>(m_argMatchers)->matches(explanation, std::get<Indices>(tuple))));
    }

    bool matches(std::string* explanation, Types... args) const {
        if (m_callCount >= m_expectedCalls) {
            *explanation += "Too many calls\n";
            return false;
        }
        return match(explanation, std::make_tuple(args...), std::make_index_sequence<std::tuple_size_v<std::tuple<Types...>>>());
    }
};

template <class R, class ...Types>
struct Behavior {
    std::function<R(Types...)> m_callback;

    void call(std::function<R(Types...)> callback) {
        m_callback = std::move(callback);
    }
};

template<class... Args>
std::string argsToString(Args... args) {
    return (... + (", " + argToString<Args>(args)));
}

template<>
std::string argsToString<>() {
    return "";
}

template<class R, class ...Types>
struct ExpectationToBehaviorMapping {
    Expectation<Types...> expectation;
    Behavior<R, Types...> behavior;
};

template<class R, class... Args>
class MockCallback {
public:
    std::function<R(Args...)> get() {
        return [this](Args... result) -> R {
            return processCall(std::forward<Args>(result)...);
        };
    }

    Behavior<R, Args...>& expectCallWith(std::unique_ptr<Matcher<Args>>... matcherArgs) {
        auto matchers = std::make_tuple(std::move(matcherArgs)...);
        m_behaviorByExpectation.push_back({Expectation<Args...> {std::move(matchers)}, Behavior<R, Args...> {}});
        return m_behaviorByExpectation.back().behavior;
    }

    Behavior<R, Args...>& expectRepeatedCallWith(int times, std::unique_ptr<Matcher<Args>>... matcherArgs) {
        auto matchers = std::make_tuple(std::move(matcherArgs)...);
        m_behaviorByExpectation.push_back({Expectation<Args...> {std::move(matchers), 0, times}, Behavior<R, Args...> {}});
        return m_behaviorByExpectation.back().behavior;
    }

private:
    R processCall(Args... args) {
        std::string argsAsString = argsToString(args...);
        std::string triedExpectations;
        auto it = std::find_if(m_behaviorByExpectation.begin(), m_behaviorByExpectation.end(),
            [&](const ExpectationToBehaviorMapping<R, Args...>& behavior) {
                return behavior.expectation.matches(&triedExpectations, std::forward<Args>(args)...);
            });
        if (it != m_behaviorByExpectation.end()) {
            ++it->expectation.m_callCount;
            return it->behavior.m_callback(args...);
        } else {
            QWASMFAIL("Unexpected call with " + argsAsString + ". Tried: " + triedExpectations);
        }
        return R();
    }

    std::vector<ExpectationToBehaviorMapping<R, Args...>> m_behaviorByExpectation;
};

void FilesTest::selectOneFileWithFileDialog()
{
    init();

    static constexpr std::string_view testFileContent = "This is a happy case.";

    EM_ASM({
        mockOpenFileDialog([{
            name: 'file1.jpg',
            content: UTF8ToString($0)
        }]);
    }, testFileContent.data());

    auto* fileSelectedCallback = Own(new MockCallback<void, bool>());
    fileSelectedCallback->expectCallWith(equals(true)).call([](bool) mutable {});

    auto* fileBuffer = Own(new QByteArray());

    auto* acceptFileCallback = Own(new MockCallback<char*, uint64_t, const std::string&>());
    acceptFileCallback->expectCallWith(equals<uint64_t>(testFileContent.size()), equals<const std::string&>("file1.jpg"))
        .call([fileBuffer](uint64_t, std::string) mutable -> char* {
            fileBuffer->resize(testFileContent.size());
            return fileBuffer->data();
        });

    auto* fileDataReadyCallback = Own(new MockCallback<void>());
    fileDataReadyCallback->expectCallWith().call([fileBuffer]() mutable {
        QWASMCOMPARE(fileBuffer->data(), std::string(testFileContent));
        QWASMSUCCESS();
    });

    QWasmLocalFileAccess::openFile("*", fileSelectedCallback->get(), acceptFileCallback->get(),
                                   fileDataReadyCallback->get());
}

void FilesTest::selectMultipleFilesWithFileDialog()
{
    static constexpr std::array<std::string_view, 3> testFileContent =
        { "Cont 1", "2s content", "What is hiding in 3?"};

    init();

    EM_ASM({
        mockOpenFileDialog([{
            name: 'file1.jpg',
            content: UTF8ToString($0)
        }, {
            name: 'file2.jpg',
            content: UTF8ToString($1)
        }, {
            name: 'file3.jpg',
            content: UTF8ToString($2)
        }]);
    }, testFileContent[0].data(), testFileContent[1].data(), testFileContent[2].data());

    auto* fileSelectedCallback = Own(new MockCallback<void, int>());
    fileSelectedCallback->expectCallWith(equals(3)).call([](int) mutable {});

    auto fileBuffer = std::make_shared<QByteArray>();

    auto* acceptFileCallback = Own(new MockCallback<char*, uint64_t, const std::string&>());
    acceptFileCallback->expectCallWith(equals<uint64_t>(testFileContent[0].size()), equals<const std::string&>("file1.jpg"))
        .call([fileBuffer](uint64_t, std::string) mutable -> char* {
            fileBuffer->resize(testFileContent[0].size());
            return fileBuffer->data();
        });
    acceptFileCallback->expectCallWith(equals<uint64_t>(testFileContent[1].size()), equals<const std::string&>("file2.jpg"))
        .call([fileBuffer](uint64_t, std::string) mutable -> char* {
            fileBuffer->resize(testFileContent[1].size());
            return fileBuffer->data();
        });
    acceptFileCallback->expectCallWith(equals<uint64_t>(testFileContent[2].size()), equals<const std::string&>("file3.jpg"))
        .call([fileBuffer](uint64_t, std::string) mutable -> char* {
            fileBuffer->resize(testFileContent[2].size());
            return fileBuffer->data();
        });

    auto* fileDataReadyCallback = Own(new MockCallback<void>());
    fileDataReadyCallback->expectRepeatedCallWith(3).call([fileBuffer]() mutable {
        static int callCount = 0;
        QWASMCOMPARE(fileBuffer->data(), std::string(testFileContent[callCount]));

        callCount++;
        if (callCount == 3) {
            QWASMSUCCESS();
        }
    });

    QWasmLocalFileAccess::openFiles("*", QWasmLocalFileAccess::FileSelectMode::MultipleFiles,
                                    fileSelectedCallback->get(), acceptFileCallback->get(),
                                    fileDataReadyCallback->get());
}

void FilesTest::cancelFileDialog()
{
    init();

    EM_ASM({
        window.showOpenFilePicker.withArgs(sinon.match.any).returns(Promise.reject("The user cancelled the dialog"));
    });

    auto* fileSelectedCallback = Own(new MockCallback<void, bool>());
    fileSelectedCallback->expectCallWith(equals(false)).call([](bool) mutable {
        QWASMSUCCESS();
    });

    auto* acceptFileCallback = Own(new MockCallback<char*, uint64_t, const std::string&>());
    auto* fileDataReadyCallback = Own(new MockCallback<void>());

    QWasmLocalFileAccess::openFile("*", fileSelectedCallback->get(), acceptFileCallback->get(),
                                   fileDataReadyCallback->get());
}

void FilesTest::rejectFile()
{
    init();

    static constexpr std::string_view testFileContent = "We don't want this file.";

    EM_ASM({
        mockOpenFileDialog([{
            name: 'dontwant.dat',
            content: UTF8ToString($0)
        }]);
    }, testFileContent.data());

    auto* fileSelectedCallback = Own(new MockCallback<void, bool>());
    fileSelectedCallback->expectCallWith(equals(true)).call([](bool) mutable {});

    auto* fileDataReadyCallback = Own(new MockCallback<void>());

    auto* acceptFileCallback = Own(new MockCallback<char*, uint64_t, const std::string&>());
    acceptFileCallback->expectCallWith(equals<uint64_t>(std::string_view(testFileContent).size()), equals<const std::string&>("dontwant.dat"))
        .call([](uint64_t, const std::string) {
            QTimer::singleShot(0, []() {
                // No calls to fileDataReadyCallback
                QWASMSUCCESS();
            });
            return nullptr;
        });

    QWasmLocalFileAccess::openFile("*", fileSelectedCallback->get(), acceptFileCallback->get(),
                                   fileDataReadyCallback->get());
}

void FilesTest::saveFileWithFileDialog()
{
    init();

    static constexpr std::string_view testFileContent = "Save this important content";

    EM_ASM({
        mockSaveFilePicker({
            name: 'somename',
            content: UTF8ToString($0),
            closeFn: (() => {
                const close = sinon.stub();
                close.callsFake(() =>
                    new Promise(resolve => {
                        resolve();
                        Module.qtWasmPass();
                    }));
                return close;
            })()
        });
    }, testFileContent.data());

    QByteArray data;
    data.prepend(testFileContent);
    QWasmLocalFileAccess::saveFile(data, "hintie");
}

int main(int argc, char **argv)
{
    auto testObject = std::make_shared<FilesTest>();
    QtWasmTest::initTestCase<QCoreApplication>(argc, argv, testObject);
    return 0;
}

#include "files_main.moc"
