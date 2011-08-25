/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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



#include <qscriptengine.h>

#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QTest>

#include <qstringlist.h>
#include <stdlib.h>
#include <qsharedmemory.h>
#include <qsystemsemaphore.h>
#include <qsystemlock.h>

class ScriptSystemSemaphore : public QObject
{
    Q_OBJECT

public:
    ScriptSystemSemaphore(QObject *parent = 0) : QObject(parent), ss(QString())
    {
    }

public slots:
    bool acquire()
    {
        return ss.acquire();
    };

    bool release(int n = 1)
    {
        return ss.release(n);
    };

    void setKey(const QString &key, int n = 0)
    {
        ss.setKey(key, n);
    };

    QString key() const
    {
        return ss.key();
    }

private:
    QSystemSemaphore ss;
};

class ScriptSystemLock : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString key WRITE setKey READ key)

public:
    ScriptSystemLock(QObject *parent = 0) : QObject(parent), sl(QString())
    {
    }

public slots:

    bool lockReadOnly()
    {
        return sl.lock(QSystemLock::ReadOnly);
    }

    bool lock()
    {
        return sl.lock();
    };

    bool unlock()
    {
        return sl.unlock();
    };

    void setKey(const QString &key)
    {
        sl.setKey(key);
    };

    QString key() const
    {
        return sl.key();
    }

private:
    QSystemLock sl;
};

class ScriptSharedMemory : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool attached READ isAttached)
    Q_PROPERTY(QString key WRITE setKey READ key)

public:
    enum SharedMemoryError
    {
        NoError = 0,
        PermissionDenied = 1,
        InvalidSize = 2,
        KeyError = 3,
        AlreadyExists = 4,
        NotFound = 5,
        LockError = 6,
        OutOfResources = 7,
        UnknownError = 8
    };

    ScriptSharedMemory(QObject *parent = 0) : QObject(parent)
    {
    }

public slots:
    void sleep(int x) const
    {
        QTest::qSleep(x);
    }

    bool create(int size)
    {
        return sm.create(size);
    };

    bool createReadOnly(int size)
    {
        return sm.create(size, QSharedMemory::ReadOnly);
    };

    int size() const
    {
        return sm.size();
    };

    bool attach()
    {
        return sm.attach();
    };

    bool attachReadOnly()
    {
        return sm.attach(QSharedMemory::ReadOnly);
    };

    bool isAttached() const
    {
        return sm.isAttached();
    };

    bool detach()
    {
        return sm.detach();
    };

    int error() const
    {
        return (int)sm.error();
    };

    QString errorString() const
    {
        return sm.errorString();
    };

    void set(int i, QChar value)
    {
        ((char*)sm.data())[i] = value.toLatin1();
    }

    QString get(int i)
    {
        return QChar::fromLatin1(((char*)sm.data())[i]);
    }

    char *data() const
    {
        return (char*)sm.data();
    };

    void setKey(const QString &key)
    {
        sm.setKey(key);
    };

    QString key() const
    {
        return sm.key();
    }

    bool lock()
    {
        return sm.lock();
    }

    bool unlock()
    {
        return sm.unlock();
    }

private:
    QSharedMemory sm;
};

QT_BEGIN_NAMESPACE
Q_SCRIPT_DECLARE_QMETAOBJECT(ScriptSharedMemory, QObject*);
Q_SCRIPT_DECLARE_QMETAOBJECT(ScriptSystemLock, QObject*);
Q_SCRIPT_DECLARE_QMETAOBJECT(ScriptSystemSemaphore, QObject*);
QT_END_NAMESPACE

static void interactive(QScriptEngine &eng)
{
#ifdef Q_OS_WINCE
    fprintf(stderr, "Interactive mode not supported on Windows CE\n");
    return;
#endif
    QTextStream qin(stdin, QFile::ReadOnly);

    const char *qscript_prompt = "qs> ";
    const char *dot_prompt = ".... ";
    const char *prompt = qscript_prompt;

    QString code;

    forever {
        QString line;

        printf("%s", prompt);
        fflush(stdout);

        line = qin.readLine();
        if (line.isNull())
        break;

        code += line;
        code += QLatin1Char('\n');

        if (line.trimmed().isEmpty()) {
            continue;

        } else if (! eng.canEvaluate(code)) {
            prompt = dot_prompt;

        } else {
            QScriptValue result = eng.evaluate(code);
            code.clear();
            prompt = qscript_prompt;
            if (!result.isUndefined())
                fprintf(stderr, "%s\n", qPrintable(result.toString()));
        }
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QScriptEngine eng;
    QScriptValue globalObject = eng.globalObject();

    QScriptValue sm = qScriptValueFromQMetaObject<ScriptSharedMemory>(&eng);
    eng.globalObject().setProperty("ScriptSharedMemory", sm);

    QScriptValue sl = qScriptValueFromQMetaObject<ScriptSystemLock>(&eng);
    eng.globalObject().setProperty("ScriptSystemLock", sl);

    QScriptValue ss = qScriptValueFromQMetaObject<ScriptSystemSemaphore>(&eng);
    eng.globalObject().setProperty("ScriptSystemSemaphore", ss);


    if (! *++argv) {
        interactive(eng);
        return EXIT_SUCCESS;
    }

    QStringList arguments = app.arguments();
    arguments.takeFirst();

    while (!arguments.isEmpty()) {
        QString fn = arguments.takeFirst();

        if (fn == QLatin1String("-i")) {
            interactive(eng);
            break;
        }

        QString contents;

        if (fn == QLatin1String("-")) {
            QTextStream stream(stdin, QFile::ReadOnly);
            contents = stream.readAll();
        } else {
            QFile file(fn);
	    if (!file.exists()) {
                fprintf(stderr, "%s doesn't exists\n", qPrintable(fn));
	        return EXIT_FAILURE;
	    }
            if (file.open(QFile::ReadOnly)) {
                QTextStream stream(&file);
                contents = stream.readAll();
                file.close();
            }
        }

        if (contents.isEmpty())
            continue;

        if (contents[0] == '#') {
            contents.prepend("//");
            QScriptValue args = eng.newArray();
            args.setProperty("0", QScriptValue(&eng, fn));
            int i = 1;
            while (!arguments.isEmpty())
                args.setProperty(i++, QScriptValue(&eng, arguments.takeFirst()));
            eng.currentContext()->activationObject().setProperty("args", args);
        }
        QScriptValue r = eng.evaluate(contents);
        if (eng.hasUncaughtException()) {
            int line = eng.uncaughtExceptionLineNumber();
            fprintf(stderr, "%d: %s\n\t%s\n\n", line, qPrintable(fn), qPrintable(r.toString()));
            return EXIT_FAILURE;
        }
        if (r.isNumber())
            return r.toInt32();
    }

    return EXIT_SUCCESS;
}

#include "main.moc"
