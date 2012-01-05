/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "v8test.h"

using namespace v8;

#define BEGINTEST() bool _testPassed = true;
#define ENDTEST() return _testPassed;

#define VERIFY(expr) { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL: %s:%d %s\n", __FILE__, __LINE__, # expr); \
        _testPassed = false; \
        goto cleanup; \
    }  \
}

struct MyStringResource : public String::ExternalAsciiStringResource
{
    static bool wasDestroyed;
    virtual ~MyStringResource() { wasDestroyed = true; }
    virtual const char* data() const { return "v8test"; }
    virtual size_t length() const { return 6; }
};
bool MyStringResource::wasDestroyed = false;

struct MyResource : public Object::ExternalResource
{
    static bool wasDestroyed;
    virtual ~MyResource() { wasDestroyed = true; }
};
bool MyResource::wasDestroyed = false;

bool v8test_externalteardown()
{
    BEGINTEST();

    Isolate *isolate = v8::Isolate::New();
    isolate->Enter();

    {
        HandleScope handle_scope;
        Persistent<Context> context = Context::New();
        Context::Scope context_scope(context);

        String::NewExternal(new MyStringResource);

        Local<FunctionTemplate> ft = FunctionTemplate::New();
        ft->InstanceTemplate()->SetHasExternalResource(true);

        Local<Object> obj = ft->GetFunction()->NewInstance();
        obj->SetExternalResource(new MyResource);

        context.Dispose();
    }

    // while (!v8::V8::IdleNotification()) ;
    isolate->Exit();
    isolate->Dispose();

    // ExternalString resources aren't guaranteed to be freed by v8 at this
    // point. Uncommenting the IdleNotification() line above helps.
//    VERIFY(MyStringResource::wasDestroyed);

    VERIFY(MyResource::wasDestroyed);

cleanup:

    ENDTEST();
}

bool v8test_eval()
{
    BEGINTEST();

    HandleScope handle_scope;
    Persistent<Context> context = Context::New();
    Context::Scope context_scope(context);

    Local<Object> qmlglobal = Object::New();
    qmlglobal->Set(String::New("a"), Integer::New(1922));

    Local<Script> script = Script::Compile(String::New("eval(\"a\")"), NULL, NULL,
                                           Handle<String>(), Script::QmlMode);

    TryCatch tc;
    Local<Value> result = script->Run(qmlglobal);

    VERIFY(!tc.HasCaught());
    VERIFY(result->Int32Value() == 1922);

cleanup:
    context.Dispose();

    ENDTEST();
}

bool v8test_globalcall()
{
    BEGINTEST();

    HandleScope handle_scope;
    Persistent<Context> context = Context::New();
    Context::Scope context_scope(context);

    Local<Object> qmlglobal = Object::New();

#define SOURCE "function func1() { return 1; }\n" \
               "function func2() { var sum = 0; for (var ii = 0; ii < 10000000; ++ii) { sum += func1(); } return sum; }\n" \
               "func2();"

    Local<Script> script = Script::Compile(String::New(SOURCE), NULL, NULL,
                                           Handle<String>(), Script::QmlMode);
    Local<Value> result = script->Run(qmlglobal);
    VERIFY(!result.IsEmpty());
    VERIFY(result->IsInt32());
    VERIFY(result->Int32Value() == 10000000);

#undef SOURCE

cleanup:
    context.Dispose();

    ENDTEST();
}

bool v8test_evalwithinwith()
{
    BEGINTEST();

    HandleScope handle_scope;
    Persistent<Context> context = Context::New();
    Context::Scope context_scope(context);

    Local<Object> qmlglobal = Object::New();
    qmlglobal->Set(String::New("a"), Integer::New(1922));
    // There was a bug that the "eval" lookup would incorrectly resolve
    // to the QML global object
    qmlglobal->Set(String::New("eval"), Integer::New(1922));

#define SOURCE \
    "(function() { " \
    "    var b = { c: 10 }; " \
    "    with (b) { " \
    "        return eval(\"a\"); " \
    "    } " \
    "})"
    Local<Script> script = Script::Compile(String::New(SOURCE), NULL, NULL, 
                                           Handle<String>(), Script::QmlMode);
#undef SOURCE

    TryCatch tc;
    Local<Value> result = script->Run(qmlglobal);

    VERIFY(!tc.HasCaught());
    VERIFY(result->IsFunction());

    {
    Local<Value> fresult = Handle<Function>::Cast(result)->Call(context->Global(), 0, 0);
    VERIFY(!tc.HasCaught());
    VERIFY(fresult->Int32Value() == 1922);
    }

cleanup:
    context.Dispose();

    ENDTEST();
}

static int userObjectComparisonCalled = 0;
static bool userObjectComparisonReturn = false;
static Local<Object> expectedLhs;
static Local<Object> expectedRhs;
static bool expectedObjectsCompared = false;

#define SET_EXPECTED(lhs, rhs) { \
    expectedObjectsCompared = false; \
    expectedLhs = lhs; \
    expectedRhs = rhs; \
}

static bool UserObjectComparison(Local<Object> lhs, Local<Object> rhs)
{
    userObjectComparisonCalled++;

    expectedObjectsCompared = (lhs == expectedLhs && rhs == expectedRhs);

    return userObjectComparisonReturn;
}

inline bool runscript(const char *source) {
    Local<Script> script = Script::Compile(String::New(source));
    Local<Value> result = script->Run();
    return result->BooleanValue();
}

bool v8test_userobjectcompare()
{
    BEGINTEST();

    HandleScope handle_scope;
    Persistent<Context> context = Context::New();
    Context::Scope context_scope(context);

    V8::SetUserObjectComparisonCallbackFunction(UserObjectComparison);

    Local<ObjectTemplate> ot = ObjectTemplate::New();
    ot->MarkAsUseUserObjectComparison();

    Local<Object> uoc1 = ot->NewInstance();
    Local<Object> uoc2 = ot->NewInstance();
    context->Global()->Set(String::New("uoc1a"), uoc1);
    context->Global()->Set(String::New("uoc1b"), uoc1);
    context->Global()->Set(String::New("uoc2"), uoc2);
    Local<Object> obj1 = Object::New();
    context->Global()->Set(String::New("obj1a"), obj1);
    context->Global()->Set(String::New("obj1b"), obj1);
    context->Global()->Set(String::New("obj2"), Object::New());
    Local<String> string1 = String::New("Hello World");
    context->Global()->Set(String::New("string1a"), string1);
    context->Global()->Set(String::New("string1b"), string1);
    context->Global()->Set(String::New("string2"), v8::String::New("Goodbye World"));

    // XXX Opportunity for optimization - don't invoke user callback if objects are
    // equal.
#if 0
    userObjectComparisonCalled = 0; userObjectComparisonReturn = false;
    VERIFY(true == runscript("uoc1a == uoc1b"));
    VERIFY(userObjectComparisonCalled == 0);
#endif

    // Comparing two uoc objects invokes uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    VERIFY(false == runscript("uoc1a == uoc2"));
    VERIFY(userObjectComparisonCalled == 1);

    VERIFY(false == runscript("uoc2 == uoc1a"));
    VERIFY(userObjectComparisonCalled == 2);
    userObjectComparisonReturn = true;
    VERIFY(true == runscript("uoc1a == uoc2"));
    VERIFY(userObjectComparisonCalled == 3);
    VERIFY(true == runscript("uoc2 == uoc1a"));
    VERIFY(userObjectComparisonCalled == 4);

    // != on two uoc object invokes uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    VERIFY(true == runscript("uoc1a != uoc2"));
    VERIFY(userObjectComparisonCalled == 1);
    VERIFY(true == runscript("uoc2 != uoc1a"));
    VERIFY(userObjectComparisonCalled == 2);
    userObjectComparisonReturn = true;
    VERIFY(false == runscript("uoc1a != uoc2"));
    VERIFY(userObjectComparisonCalled == 3);
    VERIFY(false == runscript("uoc2 != uoc1a"));
    VERIFY(userObjectComparisonCalled == 4);

    // Comparison against a non-object doesn't invoke uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    VERIFY(false == runscript("uoc1a == string1a"));
    VERIFY(userObjectComparisonCalled == 0);
    VERIFY(false == runscript("string1a == uoc1a"));
    VERIFY(userObjectComparisonCalled == 0);
    VERIFY(false == runscript("2 == uoc1a"));
    VERIFY(userObjectComparisonCalled == 0);
    VERIFY(true == runscript("uoc1a != string1a"));
    VERIFY(userObjectComparisonCalled == 0);
    VERIFY(true == runscript("string1a != uoc1a"));
    VERIFY(userObjectComparisonCalled == 0);
    VERIFY(true == runscript("2 != uoc1a"));
    VERIFY(userObjectComparisonCalled == 0);

    // Comparison against a non-uoc-object still invokes uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    VERIFY(false == runscript("uoc1a == obj1a"));
    VERIFY(userObjectComparisonCalled == 1);
    VERIFY(false == runscript("obj1a == uoc1a"));
    VERIFY(userObjectComparisonCalled == 2);
    userObjectComparisonReturn = true;
    VERIFY(true == runscript("uoc1a == obj1a"));
    VERIFY(userObjectComparisonCalled == 3);
    VERIFY(true == runscript("obj1a == uoc1a"));
    VERIFY(userObjectComparisonCalled == 4);

    // != comparison against a non-uoc-object still invokes uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    VERIFY(true == runscript("uoc1a != obj1a"));
    VERIFY(userObjectComparisonCalled == 1);
    VERIFY(true == runscript("obj1a != uoc1a"));
    VERIFY(userObjectComparisonCalled == 2);
    userObjectComparisonReturn = true;
    VERIFY(false == runscript("uoc1a != obj1a"));
    VERIFY(userObjectComparisonCalled == 3);
    VERIFY(false == runscript("obj1a != uoc1a"));
    VERIFY(userObjectComparisonCalled == 4);

    // Comparing two non-uoc objects does not invoke uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    VERIFY(true == runscript("obj1a == obj1a"));
    VERIFY(true == runscript("obj1a == obj1b"));
    VERIFY(false == runscript("obj1a == obj2"));
    VERIFY(false == runscript("obj1a == string1a"));
    VERIFY(true == runscript("string1a == string1a"));
    VERIFY(true == runscript("string1a == string1b"));
    VERIFY(false == runscript("string1a == string2"));
    VERIFY(userObjectComparisonCalled == 0);

    // Correct lhs and rhs passed to uoc
    userObjectComparisonCalled = 0;
    userObjectComparisonReturn = false;
    SET_EXPECTED(uoc1, uoc2);
    VERIFY(false == runscript("uoc1a == uoc2"));
    VERIFY(true == expectedObjectsCompared);
    SET_EXPECTED(uoc2, uoc1);
    VERIFY(false == runscript("uoc2 == uoc1a"));
    VERIFY(true == expectedObjectsCompared);
    SET_EXPECTED(uoc1, uoc2);
    VERIFY(true == runscript("uoc1a != uoc2"));
    VERIFY(true == expectedObjectsCompared);
    SET_EXPECTED(uoc2, uoc1);
    VERIFY(true == runscript("uoc2 != uoc1a"));
    VERIFY(true == expectedObjectsCompared);
    SET_EXPECTED(uoc1, obj1);
    VERIFY(false == runscript("uoc1a == obj1a"));
    VERIFY(true == expectedObjectsCompared);
    SET_EXPECTED(obj1, uoc1);
    VERIFY(false == runscript("obj1a == uoc1a"));
    VERIFY(true == expectedObjectsCompared);

cleanup:
    V8::SetUserObjectComparisonCallbackFunction(0);
    context.Dispose();

    ENDTEST();
}
