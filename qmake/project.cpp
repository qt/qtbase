/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the qmake application of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "project.h"
#include "property.h"
#include "option.h"
#include "cachekeys.h"
#include "generators/metamakefile.h"

#include <qdatetime.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qregexp.h>
#include <qtextstream.h>
#include <qstack.h>
#include <qdebug.h>
#ifdef Q_OS_UNIX
#include <time.h>
#include <utime.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#elif defined(Q_OS_WIN32)
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>

#ifdef Q_OS_WIN32
#define QT_POPEN _popen
#define QT_PCLOSE _pclose
#else
#define QT_POPEN popen
#define QT_PCLOSE pclose
#endif

QT_BEGIN_NAMESPACE

//expand functions
enum ExpandFunc { E_MEMBER=1, E_FIRST, E_LAST, E_CAT, E_FROMFILE, E_EVAL, E_LIST,
                  E_SPRINTF, E_FORMAT_NUMBER, E_JOIN, E_SPLIT, E_BASENAME, E_DIRNAME, E_SECTION,
                  E_FIND, E_SYSTEM, E_UNIQUE, E_REVERSE, E_QUOTE, E_ESCAPE_EXPAND,
                  E_UPPER, E_LOWER, E_FILES, E_PROMPT, E_RE_ESCAPE, E_VAL_ESCAPE, E_REPLACE,
                  E_SIZE, E_SORT_DEPENDS, E_RESOLVE_DEPENDS, E_ENUMERATE_VARS,
                  E_SHADOWED, E_ABSOLUTE_PATH, E_RELATIVE_PATH, E_CLEAN_PATH,
                  E_SYSTEM_PATH, E_SHELL_PATH, E_SYSTEM_QUOTE, E_SHELL_QUOTE };
QHash<QString, ExpandFunc> qmake_expandFunctions()
{
    static QHash<QString, ExpandFunc> *qmake_expand_functions = 0;
    if(!qmake_expand_functions) {
        qmake_expand_functions = new QHash<QString, ExpandFunc>;
        qmakeAddCacheClear(qmakeDeleteCacheClear<QHash<QString, ExpandFunc> >, (void**)&qmake_expand_functions);
        qmake_expand_functions->insert("member", E_MEMBER);
        qmake_expand_functions->insert("first", E_FIRST);
        qmake_expand_functions->insert("last", E_LAST);
        qmake_expand_functions->insert("cat", E_CAT);
        qmake_expand_functions->insert("fromfile", E_FROMFILE);
        qmake_expand_functions->insert("eval", E_EVAL);
        qmake_expand_functions->insert("list", E_LIST);
        qmake_expand_functions->insert("sprintf", E_SPRINTF);
        qmake_expand_functions->insert("format_number", E_FORMAT_NUMBER);
        qmake_expand_functions->insert("join", E_JOIN);
        qmake_expand_functions->insert("split", E_SPLIT);
        qmake_expand_functions->insert("basename", E_BASENAME);
        qmake_expand_functions->insert("dirname", E_DIRNAME);
        qmake_expand_functions->insert("section", E_SECTION);
        qmake_expand_functions->insert("find", E_FIND);
        qmake_expand_functions->insert("system", E_SYSTEM);
        qmake_expand_functions->insert("unique", E_UNIQUE);
        qmake_expand_functions->insert("reverse", E_REVERSE);
        qmake_expand_functions->insert("quote", E_QUOTE);
        qmake_expand_functions->insert("escape_expand", E_ESCAPE_EXPAND);
        qmake_expand_functions->insert("upper", E_UPPER);
        qmake_expand_functions->insert("lower", E_LOWER);
        qmake_expand_functions->insert("re_escape", E_RE_ESCAPE);
        qmake_expand_functions->insert("val_escape", E_VAL_ESCAPE);
        qmake_expand_functions->insert("files", E_FILES);
        qmake_expand_functions->insert("prompt", E_PROMPT);
        qmake_expand_functions->insert("replace", E_REPLACE);
        qmake_expand_functions->insert("size", E_SIZE);
        qmake_expand_functions->insert("sort_depends", E_SORT_DEPENDS);
        qmake_expand_functions->insert("resolve_depends", E_RESOLVE_DEPENDS);
        qmake_expand_functions->insert("enumerate_vars", E_ENUMERATE_VARS);
        qmake_expand_functions->insert("shadowed", E_SHADOWED);
        qmake_expand_functions->insert("absolute_path", E_ABSOLUTE_PATH);
        qmake_expand_functions->insert("relative_path", E_RELATIVE_PATH);
        qmake_expand_functions->insert("clean_path", E_CLEAN_PATH);
        qmake_expand_functions->insert("system_path", E_SYSTEM_PATH);
        qmake_expand_functions->insert("shell_path", E_SHELL_PATH);
        qmake_expand_functions->insert("system_quote", E_SYSTEM_QUOTE);
        qmake_expand_functions->insert("shell_quote", E_SHELL_QUOTE);
    }
    return *qmake_expand_functions;
}
//replace functions
enum TestFunc { T_REQUIRES=1, T_GREATERTHAN, T_LESSTHAN, T_EQUALS,
                T_EXISTS, T_EXPORT, T_CLEAR, T_UNSET, T_EVAL, T_CONFIG, T_SYSTEM,
                T_RETURN, T_BREAK, T_NEXT, T_DEFINED, T_CONTAINS, T_INFILE,
                T_COUNT, T_ISEMPTY, T_INCLUDE, T_LOAD,
                T_DEBUG, T_ERROR, T_MESSAGE, T_WARNING, T_LOG,
                T_IF, T_OPTION, T_CACHE, T_MKPATH, T_WRITE_FILE, T_TOUCH };
QHash<QString, TestFunc> qmake_testFunctions()
{
    static QHash<QString, TestFunc> *qmake_test_functions = 0;
    if(!qmake_test_functions) {
        qmake_test_functions = new QHash<QString, TestFunc>;
        qmake_test_functions->insert("requires", T_REQUIRES);
        qmake_test_functions->insert("greaterThan", T_GREATERTHAN);
        qmake_test_functions->insert("lessThan", T_LESSTHAN);
        qmake_test_functions->insert("equals", T_EQUALS);
        qmake_test_functions->insert("isEqual", T_EQUALS);
        qmake_test_functions->insert("exists", T_EXISTS);
        qmake_test_functions->insert("export", T_EXPORT);
        qmake_test_functions->insert("clear", T_CLEAR);
        qmake_test_functions->insert("unset", T_UNSET);
        qmake_test_functions->insert("eval", T_EVAL);
        qmake_test_functions->insert("CONFIG", T_CONFIG);
        qmake_test_functions->insert("if", T_IF);
        qmake_test_functions->insert("isActiveConfig", T_CONFIG);
        qmake_test_functions->insert("system", T_SYSTEM);
        qmake_test_functions->insert("return", T_RETURN);
        qmake_test_functions->insert("break", T_BREAK);
        qmake_test_functions->insert("next", T_NEXT);
        qmake_test_functions->insert("defined", T_DEFINED);
        qmake_test_functions->insert("contains", T_CONTAINS);
        qmake_test_functions->insert("infile", T_INFILE);
        qmake_test_functions->insert("count", T_COUNT);
        qmake_test_functions->insert("isEmpty", T_ISEMPTY);
        qmake_test_functions->insert("include", T_INCLUDE);
        qmake_test_functions->insert("load", T_LOAD);
        qmake_test_functions->insert("debug", T_DEBUG);
        qmake_test_functions->insert("error", T_ERROR);
        qmake_test_functions->insert("message", T_MESSAGE);
        qmake_test_functions->insert("warning", T_WARNING);
        qmake_test_functions->insert("log", T_LOG);
        qmake_test_functions->insert("option", T_OPTION);
        qmake_test_functions->insert("cache", T_CACHE);
        qmake_test_functions->insert("mkpath", T_MKPATH);
        qmake_test_functions->insert("write_file", T_WRITE_FILE);
        qmake_test_functions->insert("touch", T_TOUCH);
    }
    return *qmake_test_functions;
}

struct parser_info {
    QString file;
    int line_no;
    bool from_file;
} parser;

static QString cached_source_root;
static QString cached_build_root;
static QStringList cached_qmakepath;
static QStringList cached_qmakefeatures;

static QStringList *all_feature_roots[2] = { 0, 0 };

static void
invalidateFeatureRoots()
{
    for (int i = 0; i < 2; i++)
        if (all_feature_roots[i])
            all_feature_roots[i]->clear();
}

static QString remove_quotes(const QString &arg)
{
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';

    const QChar *arg_data = arg.data();
    const ushort first = arg_data->unicode();
    const int arg_len = arg.length();
    if(first == SINGLEQUOTE || first == DOUBLEQUOTE) {
        const ushort last = (arg_data+arg_len-1)->unicode();
        if(last == first)
            return arg.mid(1, arg_len-2);
    }
    return arg;
}

static QString varMap(const QString &x)
{
    QString ret(x);
    if(ret == "INTERFACES")
        ret = "FORMS";
    else if(ret == "QMAKE_POST_BUILD")
        ret = "QMAKE_POST_LINK";
    else if(ret == "TARGETDEPS")
        ret = "POST_TARGETDEPS";
    else if(ret == "LIBPATH")
        ret = "QMAKE_LIBDIR";
    else if(ret == "QMAKE_EXT_MOC")
        ret = "QMAKE_EXT_CPP_MOC";
    else if(ret == "QMAKE_MOD_MOC")
        ret = "QMAKE_H_MOD_MOC";
    else if(ret == "QMAKE_LFLAGS_SHAPP")
        ret = "QMAKE_LFLAGS_APP";
    else if(ret == "PRECOMPH")
        ret = "PRECOMPILED_HEADER";
    else if(ret == "PRECOMPCPP")
        ret = "PRECOMPILED_SOURCE";
    else if(ret == "INCPATH")
        ret = "INCLUDEPATH";
    else if(ret == "QMAKE_EXTRA_WIN_COMPILERS" || ret == "QMAKE_EXTRA_UNIX_COMPILERS")
        ret = "QMAKE_EXTRA_COMPILERS";
    else if(ret == "QMAKE_EXTRA_WIN_TARGETS" || ret == "QMAKE_EXTRA_UNIX_TARGETS")
        ret = "QMAKE_EXTRA_TARGETS";
    else if(ret == "QMAKE_EXTRA_UNIX_INCLUDES")
        ret = "QMAKE_EXTRA_INCLUDES";
    else if(ret == "QMAKE_EXTRA_UNIX_VARIABLES")
        ret = "QMAKE_EXTRA_VARIABLES";
    else if(ret == "QMAKE_RPATH")
        ret = "QMAKE_LFLAGS_RPATH";
    else if(ret == "QMAKE_FRAMEWORKDIR")
        ret = "QMAKE_FRAMEWORKPATH";
    else if(ret == "QMAKE_FRAMEWORKDIR_FLAGS")
        ret = "QMAKE_FRAMEWORKPATH_FLAGS";
    else if(ret == "IN_PWD")
        ret = "PWD";
    else
        return ret;
    warn_msg(WarnDeprecated, "%s:%d: Variable %s is deprecated; use %s instead.",
             parser.file.toLatin1().constData(), parser.line_no,
             x.toLatin1().constData(), ret.toLatin1().constData());
    return ret;
}

static QStringList split_arg_list(const QString &params)
{
    int quote = 0;
    QStringList args;

    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';
    const ushort BACKSLASH = '\\';
    const ushort COMMA = ',';
    const ushort SPACE = ' ';
    //const ushort TAB = '\t';

    const QChar *params_data = params.data();
    const int params_len = params.length();
    for(int last = 0; ;) {
        while(last < params_len && (params_data[last].unicode() == SPACE
                                    /*|| params_data[last].unicode() == TAB*/))
            ++last;
        for(int x = last, parens = 0; ; x++) {
            if(x == params_len) {
                while(x > last && params_data[x-1].unicode() == SPACE)
                    --x;
                args << params.mid(last, x - last);
                // Could do a check for unmatched parens here, but split_value_list()
                // is called on all our output, so mistakes will be caught anyway.
                return args;
            }
            ushort unicode = params_data[x].unicode();
            if(x != (int)params_len-1 && unicode == BACKSLASH &&
                (params_data[x+1].unicode() == SINGLEQUOTE || params_data[x+1].unicode() == DOUBLEQUOTE)) {
                x++; //get that 'escape'
            } else if(quote && unicode == quote) {
                quote = 0;
            } else if(!quote && (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE)) {
                quote = unicode;
            } else if(unicode == RPAREN) {
                --parens;
            } else if(unicode == LPAREN) {
                ++parens;
            }
            if(!parens && !quote && unicode == COMMA) {
                int prev = last;
                last = x+1;
                while(x > prev && params_data[x-1].unicode() == SPACE)
                    --x;
                args << params.mid(prev, x - prev);
                break;
            }
        }
    }
}

static QStringList split_value_list(const QString &vals)
{
    QString build;
    QStringList ret;
    ushort quote = 0;
    int parens = 0;

    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';
    const ushort BACKSLASH = '\\';

    ushort unicode;
    const QChar *vals_data = vals.data();
    const int vals_len = vals.length();
    for(int x = 0; x < vals_len; x++) {
        unicode = vals_data[x].unicode();
        if(x != (int)vals_len-1 && unicode == BACKSLASH &&
            (vals_data[x+1].unicode() == SINGLEQUOTE || vals_data[x+1].unicode() == DOUBLEQUOTE)) {
            build += vals_data[x++]; //get that 'escape'
        } else if(quote && unicode == quote) {
            quote = 0;
        } else if(!quote && (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE)) {
            quote = unicode;
        } else if(unicode == RPAREN) {
            --parens;
        } else if(unicode == LPAREN) {
            ++parens;
        }

        if(!parens && !quote && (vals_data[x] == Option::field_sep)) {
            ret << build;
            build.clear();
        } else {
            build += vals_data[x];
        }
    }
    if(!build.isEmpty())
        ret << build;
    if (parens)
        warn_msg(WarnDeprecated, "%s:%d: Unmatched parentheses are deprecated.",
                 parser.file.toLatin1().constData(), parser.line_no);
    // Could do a check for unmatched quotes here, but doVariableReplaceExpand()
    // is called on all our output, so mistakes will be caught anyway.
    return ret;
}

//just a parsable entity
struct ParsableBlock
{
    ParsableBlock() : ref_cnt(1) { }
    virtual ~ParsableBlock() { }

    struct Parse {
        QString text;
        parser_info pi;
        Parse(const QString &t) : text(t){ pi = parser; }
    };
    QList<Parse> parselist;

    inline int ref() { return ++ref_cnt; }
    inline int deref() { return --ref_cnt; }

protected:
    int ref_cnt;
    virtual bool continueBlock() = 0;
    bool eval(QMakeProject *p, QHash<QString, QStringList> &place);
};

bool ParsableBlock::eval(QMakeProject *p, QHash<QString, QStringList> &place)
{
    //save state
    parser_info pi = parser;
    const int block_count = p->scope_blocks.count();

    //execute
    bool ret = true;
    for(int i = 0; i < parselist.count(); i++) {
        parser = parselist.at(i).pi;
        if(!(ret = p->parse(parselist.at(i).text, place)) || !continueBlock())
            break;
    }

    //restore state
    parser = pi;
    while(p->scope_blocks.count() > block_count)
        p->scope_blocks.pop();
    return ret;
}

//defined functions
struct FunctionBlock : public ParsableBlock
{
    FunctionBlock() : calling_place(0), scope_level(1), cause_return(false) { }

    QHash<QString, QStringList> vars;
    QHash<QString, QStringList> *calling_place;
    QStringList return_value;
    int scope_level;
    bool cause_return;

    bool exec(const QList<QStringList> &args,
              QMakeProject *p, QHash<QString, QStringList> &place, QStringList &functionReturn);
    virtual bool continueBlock() { return !cause_return; }
};

bool FunctionBlock::exec(const QList<QStringList> &args,
                         QMakeProject *proj, QHash<QString, QStringList> &place,
                         QStringList &functionReturn)
{
    //save state
#if 1
    calling_place = &place;
#else
    calling_place = &proj->variables();
#endif
    return_value.clear();
    cause_return = false;

    //execute
#if 0
    vars = proj->variables(); // should be place so that local variables can be inherited
#else
    vars = place;
#endif
    vars["ARGS"].clear();
    for(int i = 0; i < args.count(); i++) {
        vars["ARGS"] += args[i];
        vars[QString::number(i+1)] = args[i];
    }
    bool ret = ParsableBlock::eval(proj, vars);
    functionReturn = return_value;

    //restore state
    calling_place = 0;
    return_value.clear();
    vars.clear();
    return ret;
}

//loops
struct IteratorBlock : public ParsableBlock
{
    IteratorBlock() : scope_level(1), loop_forever(false), cause_break(false), cause_next(false) { }

    int scope_level;

    struct Test {
        QString func;
        QStringList args;
        bool invert;
        parser_info pi;
        Test(const QString &f, QStringList &a, bool i) : func(f), args(a), invert(i) { pi = parser; }
    };
    QList<Test> test;

    QString variable;

    bool loop_forever, cause_break, cause_next;
    QStringList list;

    bool exec(QMakeProject *p, QHash<QString, QStringList> &place);
    virtual bool continueBlock() { return !cause_next && !cause_break; }
};
bool IteratorBlock::exec(QMakeProject *p, QHash<QString, QStringList> &place)
{
    bool ret = true;
    QStringList::Iterator it;
    if(!loop_forever)
        it = list.begin();
    int iterate_count = 0;
    //save state
    IteratorBlock *saved_iterator = p->iterator;
    p->iterator = this;

    //do the loop
    while(loop_forever || it != list.end()) {
        cause_next = cause_break = false;
        if(!loop_forever && (*it).isEmpty()) { //ignore empty items
            ++it;
            continue;
        }

        //set up the loop variable
        QStringList va;
        if(!variable.isEmpty()) {
            va = place[variable];
            if(loop_forever)
                place[variable] = QStringList(QString::number(iterate_count));
            else
                place[variable] = QStringList(*it);
        }
        //do the iterations
        bool succeed = true;
        for(QList<Test>::Iterator test_it = test.begin(); test_it != test.end(); ++test_it) {
            parser = (*test_it).pi;
            succeed = p->doProjectTest((*test_it).func, (*test_it).args, place);
            if((*test_it).invert)
                succeed = !succeed;
            if(!succeed)
                break;
        }
        if(succeed)
            ret = ParsableBlock::eval(p, place);
        //restore the variable in the map
        if(!variable.isEmpty())
            place[variable] = va;
        //loop counters
        if(!loop_forever)
            ++it;
        iterate_count++;
        if(!ret || cause_break)
            break;
    }

    //restore state
    p->iterator = saved_iterator;
    return ret;
}

QMakeProject::ScopeBlock::~ScopeBlock()
{
#if 0
    if(iterate)
        delete iterate;
#endif
}

static void qmake_error_msg(const QString &msg)
{
    fprintf(stderr, "%s:%d: %s\n", parser.file.toLatin1().constData(), parser.line_no,
            msg.toLatin1().constData());
}

/*
   1) environment variable QMAKEFEATURES (as separated by colons)
   2) property variable QMAKEFEATURES (as separated by colons)
   3) <project_root> (where .qmake.cache lives) + FEATURES_DIR
   4) environment variable QMAKEPATH (as separated by colons) + /mkspecs/FEATURES_DIR
   5) your QMAKESPEC/features dir
   6) your data_install/mkspecs/FEATURES_DIR
   7) your QMAKESPEC/../FEATURES_DIR dir

   FEATURES_DIR is defined as:

   1) features/(unix|win32|macx)/
   2) features/
*/
QStringList QMakeProject::qmakeFeaturePaths()
{
    const QString mkspecs_concat = QLatin1String("/mkspecs");
    const QString base_concat = QLatin1String("/features");
    QStringList concat;
    foreach (const QString &sfx, values("QMAKE_PLATFORM"))
        concat << base_concat + QLatin1Char('/') + sfx;
    concat << base_concat;

    QStringList feature_roots = splitPathList(QString::fromLocal8Bit(qgetenv("QMAKEFEATURES")));
    feature_roots += cached_qmakefeatures;
    if(prop)
        feature_roots += splitPathList(prop->value("QMAKEFEATURES"));
    QStringList feature_bases;
    if (!cached_build_root.isEmpty())
        feature_bases << cached_build_root;
    if (!cached_source_root.isEmpty())
        feature_bases << cached_source_root;
    QStringList qmakepath = splitPathList(QString::fromLocal8Bit(qgetenv("QMAKEPATH")));
    qmakepath += cached_qmakepath;
    foreach (const QString &path, qmakepath)
        feature_bases << (path + mkspecs_concat);
    if (!real_spec.isEmpty()) {
        // The spec is already platform-dependent, so no subdirs here.
        feature_roots << real_spec + base_concat;

        // Also check directly under the root directory of the mkspecs collection
        QFileInfo specfi(real_spec);
        QDir specrootdir(specfi.absolutePath());
        while (!specrootdir.isRoot()) {
            const QString specrootpath = specrootdir.path();
            if (specrootpath.endsWith(mkspecs_concat)) {
                if (QFile::exists(specrootpath + base_concat))
                    feature_bases << specrootpath;
                break;
            }
            specrootdir.cdUp();
        }
    }
    feature_bases << (QLibraryInfo::rawLocation(QLibraryInfo::HostDataPath,
                                                QLibraryInfo::EffectivePaths) + mkspecs_concat);
    foreach (const QString &fb, feature_bases)
        foreach (const QString &cc, concat)
            feature_roots << (fb + cc);
    feature_roots.removeDuplicates();

    QStringList ret;
    foreach (const QString &root, feature_roots)
        if (QFileInfo(root).exists())
            ret << root;
    return ret;
}

QStringList qmake_mkspec_paths()
{
    QStringList ret;
    const QString concat = QLatin1String("/mkspecs");

    QStringList qmakepath = splitPathList(QString::fromLocal8Bit(qgetenv("QMAKEPATH")));
    qmakepath += cached_qmakepath;
    foreach (const QString &path, qmakepath)
        ret << (path + concat);
    if (!cached_build_root.isEmpty())
        ret << cached_build_root + concat;
    if (!cached_source_root.isEmpty())
        ret << cached_source_root + concat;
    ret << QLibraryInfo::rawLocation(QLibraryInfo::HostDataPath, QLibraryInfo::EffectivePaths) + concat;
    ret.removeDuplicates();

    return ret;
}

static void
setTemplate(QStringList &varlist)
{
    if (!Option::user_template.isEmpty()) { // Don't permit override
        varlist = QStringList(Option::user_template);
    } else {
        if (varlist.isEmpty())
            varlist << "app";
        else
            varlist.erase(varlist.begin() + 1, varlist.end());
    }
    if (!Option::user_template_prefix.isEmpty()
        && !varlist.first().startsWith(Option::user_template_prefix)) {
        varlist.first().prepend(Option::user_template_prefix);
    }
}

QMakeProject::~QMakeProject()
{
    if(own_prop)
        delete prop;
    cleanup();
}


void
QMakeProject::init(QMakeProperty *p)
{
    if(!p) {
        prop = new QMakeProperty;
        own_prop = true;
    } else {
        prop = p;
        own_prop = false;
    }
    host_build = false;
    reset();
}

void
QMakeProject::cleanup()
{
    for (QHash<QString, FunctionBlock*>::iterator it = replaceFunctions.begin(); it != replaceFunctions.end(); ++it)
        if (!it.value()->deref())
            delete it.value();
    replaceFunctions.clear();
    for (QHash<QString, FunctionBlock*>::iterator it = testFunctions.begin(); it != testFunctions.end(); ++it)
        if (!it.value()->deref())
            delete it.value();
    testFunctions.clear();
}

// Duplicate project. It is *not* allowed to call the complex read() functions on the copy.
QMakeProject::QMakeProject(QMakeProject *p, const QHash<QString, QStringList> *_vars)
{
    init(p->properties());
    vars = _vars ? *_vars : p->variables();
    host_build = p->host_build;
    for(QHash<QString, FunctionBlock*>::iterator it = p->replaceFunctions.begin(); it != p->replaceFunctions.end(); ++it) {
        it.value()->ref();
        replaceFunctions.insert(it.key(), it.value());
    }
    for(QHash<QString, FunctionBlock*>::iterator it = p->testFunctions.begin(); it != p->testFunctions.end(); ++it) {
        it.value()->ref();
        testFunctions.insert(it.key(), it.value());
    }
}

void
QMakeProject::reset()
{
    // scope_blocks starts with one non-ignoring entity
    scope_blocks.clear();
    scope_blocks.push(ScopeBlock());
    iterator = 0;
    function = 0;
    backslashWarned = false;
    need_restart = false;
}

bool
QMakeProject::parse(const QString &t, QHash<QString, QStringList> &place, int numLines)
{
    // To preserve the integrity of any UTF-8 characters in .pro file, temporarily replace the
    // non-breaking space (0xA0) characters with another non-space character, so that
    // QString::simplified() call will not replace it with space.
    // Note: There won't be any two byte characters in .pro files, so 0x10A0 should be a safe
    // replacement character.
    static QChar nbsp(0xA0);
    static QChar nbspFix(0x01A0);
    QString s;
    if (t.indexOf(nbsp) != -1) {
        s = t;
        s.replace(nbsp, nbspFix);
        s = s.simplified();
        s.replace(nbspFix, nbsp);
    } else {
        s = t.simplified();
    }

    int hash_mark = s.indexOf("#");
    if(hash_mark != -1) //good bye comments
        s = s.left(hash_mark);
    if(s.isEmpty()) // blank_line
        return true;

    if(scope_blocks.top().ignore) {
        bool continue_parsing = false;
        // adjust scope for each block which appears on a single line
        for(int i = 0; i < s.length(); i++) {
            if(s[i] == '{') {
                scope_blocks.push(ScopeBlock(true));
            } else if(s[i] == '}') {
                if(scope_blocks.count() == 1) {
                    fprintf(stderr, "Braces mismatch %s:%d\n", parser.file.toLatin1().constData(), parser.line_no);
                    return false;
                }
                ScopeBlock sb = scope_blocks.pop();
                if(sb.iterate) {
                    sb.iterate->exec(this, place);
                    delete sb.iterate;
                    sb.iterate = 0;
                }
                if(!scope_blocks.top().ignore) {
                    debug_msg(1, "Project Parser: %s:%d : Leaving block %d", parser.file.toLatin1().constData(),
                              parser.line_no, scope_blocks.count()+1);
                    s = s.mid(i+1).trimmed();
                    continue_parsing = !s.isEmpty();
                    break;
                }
            }
        }
        if(!continue_parsing) {
            debug_msg(1, "Project Parser: %s:%d : Ignored due to block being false.",
                      parser.file.toLatin1().constData(), parser.line_no);
            return true;
        }
    }

    if(function) {
        QString append;
        int d_off = 0;
        const QChar *d = s.unicode();
        bool function_finished = false;
        while(d_off < s.length()) {
            if(*(d+d_off) == QLatin1Char('}')) {
                function->scope_level--;
                if(!function->scope_level) {
                    function_finished = true;
                    break;
                }
            } else if(*(d+d_off) == QLatin1Char('{')) {
                function->scope_level++;
            }
            append += *(d+d_off);
            ++d_off;
        }
        if(!append.isEmpty())
            function->parselist.append(IteratorBlock::Parse(append));
        if(function_finished) {
            function = 0;
            s = QString(d+d_off, s.length()-d_off);
        } else {
            return true;
        }
    } else if(IteratorBlock *it = scope_blocks.top().iterate) {
        QString append;
        int d_off = 0;
        const QChar *d = s.unicode();
        bool iterate_finished = false;
        while(d_off < s.length()) {
            if(*(d+d_off) == QLatin1Char('}')) {
                it->scope_level--;
                if(!it->scope_level) {
                    iterate_finished = true;
                    break;
                }
            } else if(*(d+d_off) == QLatin1Char('{')) {
                it->scope_level++;
            }
            append += *(d+d_off);
            ++d_off;
        }
        if(!append.isEmpty())
            scope_blocks.top().iterate->parselist.append(IteratorBlock::Parse(append));
        if(iterate_finished) {
            scope_blocks.top().iterate = 0;
            bool ret = it->exec(this, place);
            delete it;
            if(!ret)
                return false;
            s = s.mid(d_off);
        } else {
            return true;
        }
    }

    QString scope, var, op;
    QStringList val;
#define SKIP_WS(d, o, l) while(o < l && (*(d+o) == QLatin1Char(' ') || *(d+o) == QLatin1Char('\t'))) ++o
    const QChar *d = s.unicode();
    int d_off = 0;
    SKIP_WS(d, d_off, s.length());
    IteratorBlock *iterator = 0;
    bool scope_failed = false, else_line = false, or_op=false;
    QChar quote = 0;
    int parens = 0, scope_count=0, start_block = 0;
    while(d_off < s.length()) {
        if(!parens) {
            if(*(d+d_off) == QLatin1Char('='))
                break;
            if(*(d+d_off) == QLatin1Char('+') || *(d+d_off) == QLatin1Char('-') ||
               *(d+d_off) == QLatin1Char('*') || *(d+d_off) == QLatin1Char('~')) {
                if(*(d+d_off+1) == QLatin1Char('=')) {
                    break;
                } else if(*(d+d_off+1) == QLatin1Char(' ')) {
                    const QChar *k = d+d_off+1;
                    int k_off = 0;
                    SKIP_WS(k, k_off, s.length()-d_off);
                    if(*(k+k_off) == QLatin1Char('=')) {
                        QString msg;
                        qmake_error_msg(QString(d+d_off, 1) + "must be followed immediately by =");
                        return false;
                    }
                }
            }
        }

        if(!quote.isNull()) {
            if(*(d+d_off) == quote)
                quote = QChar();
        } else if(*(d+d_off) == '(') {
            ++parens;
        } else if(*(d+d_off) == ')') {
            --parens;
        } else if(*(d+d_off) == '"' /*|| *(d+d_off) == '\''*/) {
            quote = *(d+d_off);
        }

        if(!parens && quote.isNull() &&
           (*(d+d_off) == QLatin1Char(':') || *(d+d_off) == QLatin1Char('{') ||
            *(d+d_off) == QLatin1Char(')') || *(d+d_off) == QLatin1Char('|'))) {
            scope_count++;
            scope = var.trimmed();
            if(*(d+d_off) == QLatin1Char(')'))
                scope += *(d+d_off); // need this
            var = "";

            bool test = scope_failed;
            if(scope.isEmpty()) {
                test = true;
            } else if(scope.toLower() == "else") { //else is a builtin scope here as it modifies state
                if(scope_count != 1 || scope_blocks.top().else_status == ScopeBlock::TestNone) {
                    qmake_error_msg(("Unexpected " + scope + " ('" + s + "')").toLatin1());
                    return false;
                }
                else_line = true;
                test = (scope_blocks.top().else_status == ScopeBlock::TestSeek);
                debug_msg(1, "Project Parser: %s:%d : Else%s %s.", parser.file.toLatin1().constData(), parser.line_no,
                          scope == "else" ? "" : QString(" (" + scope + ")").toLatin1().constData(),
                          test ? "considered" : "excluded");
            } else {
                QString comp_scope = scope;
                bool invert_test = (comp_scope.at(0) == QLatin1Char('!'));
                if(invert_test)
                    comp_scope = comp_scope.mid(1);
                int lparen = comp_scope.indexOf('(');
                if(or_op == scope_failed) {
                    if(lparen != -1) { // if there is an lparen in the scope, it IS a function
                        int rparen = comp_scope.lastIndexOf(')');
                        if(rparen == -1) {
                            qmake_error_msg("Function missing right paren: " + comp_scope);
                            return false;
                        }
                        QString func = comp_scope.left(lparen);
                        QStringList args = split_arg_list(comp_scope.mid(lparen+1, rparen - lparen - 1));
                        if(function) {
                            fprintf(stderr, "%s:%d: No tests can come after a function definition!\n",
                                    parser.file.toLatin1().constData(), parser.line_no);
                            return false;
                        } else if(func == "for") { //for is a builtin function here, as it modifies state
                            if(args.count() > 2 || args.count() < 1) {
                                fprintf(stderr, "%s:%d: for(iterate, list) requires two arguments.\n",
                                        parser.file.toLatin1().constData(), parser.line_no);
                                return false;
                            } else if(iterator) {
                                fprintf(stderr, "%s:%d unexpected nested for()\n",
                                        parser.file.toLatin1().constData(), parser.line_no);
                                return false;
                            }

                            iterator = new IteratorBlock;
                            QString it_list;
                            if(args.count() == 1) {
                                doVariableReplace(args[0], place);
                                it_list = args[0];
                                if(args[0] != "ever") {
                                    delete iterator;
                                    iterator = 0;
                                    fprintf(stderr, "%s:%d: for(iterate, list) requires two arguments.\n",
                                            parser.file.toLatin1().constData(), parser.line_no);
                                    return false;
                                }
                                it_list = "forever";
                            } else if(args.count() == 2) {
                                iterator->variable = args[0];
                                doVariableReplace(args[1], place);
                                it_list = args[1];
                            }
                            QStringList list = place[it_list];
                            if(list.isEmpty()) {
                                if(it_list == "forever") {
                                    iterator->loop_forever = true;
                                } else {
                                    int dotdot = it_list.indexOf("..");
                                    if(dotdot != -1) {
                                        bool ok;
                                        int start = it_list.left(dotdot).toInt(&ok);
                                        if(ok) {
                                            int end = it_list.mid(dotdot+2).toInt(&ok);
                                            if(ok) {
                                                if(start < end) {
                                                    for(int i = start; i <= end; i++)
                                                        list << QString::number(i);
                                                } else {
                                                    for(int i = start; i >= end; i--)
                                                        list << QString::number(i);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            iterator->list = list;
                            test = !invert_test;
                        } else if(iterator) {
                            iterator->test.append(IteratorBlock::Test(func, args, invert_test));
                            test = !invert_test;
                        } else if(func == "defineTest" || func == "defineReplace") {
                            if(!function_blocks.isEmpty()) {
                                fprintf(stderr,
                                        "%s:%d: cannot define a function within another definition.\n",
                                        parser.file.toLatin1().constData(), parser.line_no);
                                return false;
                            }
                            if(args.count() != 1) {
                                fprintf(stderr, "%s:%d: %s(function_name) requires one argument.\n",
                                        parser.file.toLatin1().constData(), parser.line_no, func.toLatin1().constData());
                                return false;
                            }
                            QHash<QString, FunctionBlock*> *map = 0;
                            if(func == "defineTest")
                                map = &testFunctions;
                            else
                                map = &replaceFunctions;
#if 0
                            if(!map || map->contains(args[0])) {
                                fprintf(stderr, "%s:%d: Function[%s] multiply defined.\n",
                                        parser.file.toLatin1().constData(), parser.line_no, args[0].toLatin1().constData());
                                return false;
                            }
#endif
                            function = new FunctionBlock;
                            map->insert(args[0], function);
                            test = true;
                        } else {
                            test = doProjectTest(func, args, place);
                            if(*(d+d_off) == QLatin1Char(')') && d_off == s.length()-1) {
                                if(invert_test)
                                    test = !test;
                                scope_blocks.top().else_status =
                                    (test ? ScopeBlock::TestFound : ScopeBlock::TestSeek);
                                return true;  // assume we are done
                            }
                        }
                    } else {
                        QString cscope = comp_scope.trimmed();
                        doVariableReplace(cscope, place);
                        test = isActiveConfig(cscope.trimmed(), true, &place);
                    }
                    if(invert_test)
                        test = !test;
                }
            }
            if(!test && !scope_failed)
                debug_msg(1, "Project Parser: %s:%d : Test (%s) failed.", parser.file.toLatin1().constData(),
                          parser.line_no, scope.toLatin1().constData());
            if(test == or_op)
                scope_failed = !test;
            or_op = (*(d+d_off) == QLatin1Char('|'));

            if(*(d+d_off) == QLatin1Char('{')) { // scoping block
                start_block++;
                if(iterator) {
                    for(int off = 0, braces = 0; true; ++off) {
                        if(*(d+d_off+off) == QLatin1Char('{'))
                            ++braces;
                        else if(*(d+d_off+off) == QLatin1Char('}') && braces)
                            --braces;
                        if(!braces || d_off+off == s.length()) {
                            iterator->parselist.append(s.mid(d_off, off-1));
                            if(braces > 1)
                                iterator->scope_level += braces-1;
                            d_off += off-1;
                            break;
                        }
                    }
                }
            }
        } else if(!parens && *(d+d_off) == QLatin1Char('}')) {
            if(start_block) {
                --start_block;
            } else if(!scope_blocks.count()) {
                warn_msg(WarnParser, "Possible braces mismatch %s:%d", parser.file.toLatin1().constData(), parser.line_no);
            } else {
                if(scope_blocks.count() == 1) {
                    fprintf(stderr, "Braces mismatch %s:%d\n", parser.file.toLatin1().constData(), parser.line_no);
                    return false;
                }
                debug_msg(1, "Project Parser: %s:%d : Leaving block %d", parser.file.toLatin1().constData(),
                          parser.line_no, scope_blocks.count());
                ScopeBlock sb = scope_blocks.pop();
                if(sb.iterate)
                    sb.iterate->exec(this, place);
            }
        } else {
            var += *(d+d_off);
        }
        ++d_off;
    }
    var = var.trimmed();

    if(!else_line || (else_line && !scope_failed))
        scope_blocks.top().else_status = (!scope_failed ? ScopeBlock::TestFound : ScopeBlock::TestSeek);
    if(start_block) {
        ScopeBlock next_block(scope_failed);
        next_block.iterate = iterator;
        if(iterator)
            next_block.else_status = ScopeBlock::TestNone;
        else if(scope_failed)
            next_block.else_status = ScopeBlock::TestSeek;
        else
            next_block.else_status = ScopeBlock::TestFound;
        scope_blocks.push(next_block);
        debug_msg(1, "Project Parser: %s:%d : Entering block %d (%d). [%s]", parser.file.toLatin1().constData(),
                  parser.line_no, scope_blocks.count(), scope_failed, s.toLatin1().constData());
    } else if(iterator) {
        iterator->parselist.append(QString(var+s.mid(d_off)));
        bool ret = iterator->exec(this, place);
        delete iterator;
        return ret;
    }

    if((!scope_count && !var.isEmpty()) || (scope_count == 1 && else_line))
        scope_blocks.top().else_status = ScopeBlock::TestNone;
    if(d_off == s.length()) {
        if(!var.trimmed().isEmpty())
            qmake_error_msg(("Parse Error ('" + s + "')").toLatin1());
        return var.isEmpty(); // allow just a scope
    }

    SKIP_WS(d, d_off, s.length());
    for(; d_off < s.length() && op.indexOf('=') == -1; op += *(d+(d_off++)))
        ;
    op.replace(QRegExp("\\s"), "");

    SKIP_WS(d, d_off, s.length());
    QString vals = s.mid(d_off); // vals now contains the space separated list of values
    int rbraces = vals.count('}'), lbraces = vals.count('{');
    if(scope_blocks.count() > 1 && rbraces - lbraces == 1 && vals.endsWith('}')) {
        debug_msg(1, "Project Parser: %s:%d : Leaving block %d", parser.file.toLatin1().constData(),
                  parser.line_no, scope_blocks.count());
        ScopeBlock sb = scope_blocks.pop();
        if(sb.iterate)
            sb.iterate->exec(this, place);
        vals.truncate(vals.length()-1);
    } else if(rbraces != lbraces) {
        warn_msg(WarnParser, "Possible braces mismatch {%s} %s:%d",
                 vals.toLatin1().constData(), parser.file.toLatin1().constData(), parser.line_no);
    }
    if(scope_failed)
        return true; // oh well
#undef SKIP_WS

    doVariableReplace(var, place);
    var = varMap(var); //backwards compatibility

    if(vals.contains('=') && numLines > 1)
        warn_msg(WarnParser, "Possible accidental line continuation: {%s} at %s:%d",
                 var.toLatin1().constData(), parser.file.toLatin1().constData(), parser.line_no);

    QStringList &varlist = place[var]; // varlist is the list in the symbol table

    if(Option::debug_level >= 1) {
        QString tmp_vals = vals;
        doVariableReplace(tmp_vals, place);
        debug_msg(1, "Project Parser: %s:%d :%s: :%s: (%s)", parser.file.toLatin1().constData(), parser.line_no,
                  var.toLatin1().constData(), op.toLatin1().constData(), tmp_vals.toLatin1().constData());
    }

    // now do the operation
    if(op == "~=") {
        doVariableReplace(vals, place);
        if(vals.length() < 4 || vals.at(0) != 's') {
            qmake_error_msg(("~= operator only can handle s/// function ('" +
                            s + "')").toLatin1());
            return false;
        }
        QChar sep = vals.at(1);
        QStringList func = vals.split(sep);
        if(func.count() < 3 || func.count() > 4) {
            qmake_error_msg(("~= operator only can handle s/// function ('" +
                s + "')").toLatin1());
            return false;
        }
        bool global = false, case_sense = true, quote = false;
        if(func.count() == 4) {
            global = func[3].indexOf('g') != -1;
            case_sense = func[3].indexOf('i') == -1;
            quote = func[3].indexOf('q') != -1;
        }
        QString from = func[1], to = func[2];
        if(quote)
            from = QRegExp::escape(from);
        QRegExp regexp(from, case_sense ? Qt::CaseSensitive : Qt::CaseInsensitive);
        for(QStringList::Iterator varit = varlist.begin(); varit != varlist.end();) {
            if((*varit).contains(regexp)) {
                (*varit) = (*varit).replace(regexp, to);
                if ((*varit).isEmpty())
                    varit = varlist.erase(varit);
                else
                    ++varit;
                if(!global)
                    break;
            } else
                ++varit;
        }
    } else {
        QStringList vallist;
        {
            //doVariableReplace(vals, place);
            QStringList tmp = split_value_list(vals);
            for(int i = 0; i < tmp.size(); ++i)
                vallist += doVariableReplaceExpand(tmp[i], place);
        }

        if(op == "=") {
#if 0 // This is way too noisy
            if(!varlist.isEmpty()) {
                bool send_warning = false;
                if(var != "TEMPLATE" && var != "TARGET") {
                    QSet<QString> incoming_vals = vallist.toSet();
                    for(int i = 0; i < varlist.size(); ++i) {
                        const QString var = varlist.at(i).trimmed();
                        if(!var.isEmpty() && !incoming_vals.contains(var)) {
                            send_warning = true;
                            break;
                        }
                    }
                }
                if(send_warning)
                    warn_msg(WarnParser, "Operator=(%s) clears variables previously set: %s:%d",
                             var.toLatin1().constData(), parser.file.toLatin1().constData(), parser.line_no);
            }
#endif
            varlist.clear();
        }
        for(QStringList::ConstIterator valit = vallist.begin();
            valit != vallist.end(); ++valit) {
            if((*valit).isEmpty())
                continue;
            if((op == "*=" && !varlist.contains((*valit))) ||
               op == "=" || op == "+=")
                varlist.append((*valit));
            else if(op == "-=")
                varlist.removeAll((*valit));
        }
        if(var == "REQUIRES") // special case to get communicated to backends!
            doProjectCheckReqs(vallist, place);
        else if (var == QLatin1String("TEMPLATE"))
            setTemplate(varlist);
    }
    return true;
}

bool
QMakeProject::read(QTextStream &file, QHash<QString, QStringList> &place)
{
    int numLines = 0;
    bool ret = true;
    QString s;
    while(!file.atEnd()) {
        parser.line_no++;
        QString line = file.readLine().trimmed();
        int prelen = line.length();

        int hash_mark = line.indexOf("#");
        if(hash_mark != -1) //good bye comments
            line = line.left(hash_mark).trimmed();
        if(!line.isEmpty() && line.right(1) == "\\") {
            if(!line.startsWith("#")) {
                line.truncate(line.length() - 1);
                s += line + Option::field_sep;
                ++numLines;
            }
        } else if(!line.isEmpty() || (line.isEmpty() && !prelen)) {
            if(s.isEmpty() && line.isEmpty())
                continue;
            if(!line.isEmpty()) {
                s += line;
                ++numLines;
            }
            if(!s.isEmpty()) {
                if(!(ret = parse(s, place, numLines))) {
                    s = "";
                    numLines = 0;
                    break;
                }
                s = "";
                numLines = 0;
                if (need_restart)
                    break;
            }
        }
    }
    if (!s.isEmpty())
        ret = parse(s, place, numLines);
    return ret;
}

bool
QMakeProject::read(const QString &file, QHash<QString, QStringList> &place)
{
    parser_info pi = parser;
    reset();

    const QString oldpwd = qmake_getpwd();
    QString filename = Option::normalizePath(file, false);
    bool ret = false, using_stdin = false;
    QFile qfile;
    if(filename == QLatin1String("-")) {
        qfile.setFileName("");
        ret = qfile.open(stdin, QIODevice::ReadOnly);
        using_stdin = true;
    } else if(QFileInfo(file).isDir()) {
        return false;
    } else {
        qfile.setFileName(filename);
        ret = qfile.open(QIODevice::ReadOnly);
        qmake_setpwd(QFileInfo(filename).absolutePath());
    }
    if(ret) {
        place["PWD"] = QStringList(qmake_getpwd());
        parser_info pi = parser;
        parser.from_file = true;
        parser.file = filename;
        parser.line_no = 0;
        if (qfile.peek(3) == QByteArray("\xef\xbb\xbf")) {
            //UTF-8 BOM will cause subtle errors
            qmake_error_msg("Unexpected UTF-8 BOM found");
            ret = false;
        } else {
            QTextStream t(&qfile);
            ret = read(t, place);
        }
        if(!using_stdin)
            qfile.close();
    }
    if (!need_restart && scope_blocks.count() != 1) {
        qmake_error_msg("Unterminated conditional block at end of file");
        ret = false;
    }
    parser = pi;
    qmake_setpwd(oldpwd);
    return ret;
}

bool
QMakeProject::read(const QString &project, uchar cmd)
{
    pfile = QFileInfo(project).absoluteFilePath();
    return read(cmd);
}

bool
QMakeProject::read(uchar cmd)
{
  again:
    if (init_vars.isEmpty()) {
        loadDefaults();
        init_vars = vars;
    } else {
        vars = init_vars;
    }
    if (cmd & ReadSetup) {
      if (base_vars.isEmpty()) {
        QString superdir;
        QString project_root;
        QStringList qmakepath;
        QStringList qmakefeatures;
        project_build_root.clear();
        if (Option::mkfile::do_cache) {        // parse the cache
            QHash<QString, QStringList> cache;
            QString rdir = Option::output_dir;
            forever {
                QFileInfo qfi(rdir, QLatin1String(".qmake.super"));
                if (qfi.exists()) {
                    superfile = qfi.filePath();
                    if (!read(superfile, cache))
                        return false;
                    superdir = rdir;
                    break;
                }
                QFileInfo qdfi(rdir);
                if (qdfi.isRoot())
                    break;
                rdir = qdfi.path();
            }
            if (Option::mkfile::cachefile.isEmpty())  { //find it as it has not been specified
                QString sdir = qmake_getpwd();
                QString dir = Option::output_dir;
                forever {
                    QFileInfo qsfi(sdir, QLatin1String(".qmake.conf"));
                    if (qsfi.exists()) {
                        conffile = qsfi.filePath();
                        if (!read(conffile, cache))
                            return false;
                    }
                    QFileInfo qfi(dir, QLatin1String(".qmake.cache"));
                    if (qfi.exists()) {
                        cachefile = qfi.filePath();
                        if (!read(cachefile, cache))
                            return false;
                    }
                    if (!conffile.isEmpty() || !cachefile.isEmpty()) {
                        project_root = sdir;
                        project_build_root = dir;
                        break;
                    }
                    if (dir == superdir)
                        break;
                    QFileInfo qsdfi(sdir);
                    QFileInfo qdfi(dir);
                    if (qsdfi.isRoot() || qdfi.isRoot())
                        break;
                    sdir = qsdfi.path();
                    dir = qdfi.path();
                }
            } else {
                QFileInfo fi(Option::mkfile::cachefile);
                cachefile = QDir::cleanPath(fi.absoluteFilePath());
                if (!read(cachefile, cache))
                    return false;
                project_build_root = QDir::cleanPath(fi.absolutePath());
                // This intentionally bypasses finding a source root,
                // as the result would be more or less arbitrary.
            }

            if (Option::mkfile::xqmakespec.isEmpty() && !cache["XQMAKESPEC"].isEmpty())
                Option::mkfile::xqmakespec = cache["XQMAKESPEC"].first();
            if (Option::mkfile::qmakespec.isEmpty() && !cache["QMAKESPEC"].isEmpty()) {
                Option::mkfile::qmakespec = cache["QMAKESPEC"].first();
                if (Option::mkfile::xqmakespec.isEmpty())
                    Option::mkfile::xqmakespec = Option::mkfile::qmakespec;
            }
            qmakepath = cache.value(QLatin1String("QMAKEPATH"));
            qmakefeatures = cache.value(QLatin1String("QMAKEFEATURES"));

            if (!superfile.isEmpty())
                vars["_QMAKE_SUPER_CACHE_"] << superfile;
            if (!cachefile.isEmpty())
                vars["_QMAKE_CACHE_"] << cachefile;
        }

        // Look for mkspecs/ in source and build. First to win determines the root.
        QString sdir = qmake_getpwd();
        QString dir = Option::output_dir;
        while (dir != project_build_root) {
            if ((dir != sdir && QFileInfo(sdir, QLatin1String("mkspecs")).isDir())
                    || QFileInfo(dir, QLatin1String("mkspecs")).isDir()) {
                if (dir != sdir)
                    project_root = sdir;
                project_build_root = dir;
                break;
            }
            if (dir == superdir)
                break;
            QFileInfo qsdfi(sdir);
            QFileInfo qdfi(dir);
            if (qsdfi.isRoot() || qdfi.isRoot())
                break;
            sdir = qsdfi.path();
            dir = qdfi.path();
        }

        if (qmakepath != cached_qmakepath || qmakefeatures != cached_qmakefeatures
            || project_build_root != cached_build_root) { // No need to check source dir, as it goes in sync
            cached_source_root = project_root;
            cached_build_root = project_build_root;
            cached_qmakepath = qmakepath;
            cached_qmakefeatures = qmakefeatures;
            invalidateFeatureRoots();
        }

        {             // parse mkspec
            QString *specp = host_build ? &Option::mkfile::qmakespec : &Option::mkfile::xqmakespec;
            QString qmakespec = *specp;
            if (qmakespec.isEmpty())
                qmakespec = host_build ? "default-host" : "default";
            if (QDir::isRelativePath(qmakespec)) {
                    QStringList mkspec_roots = qmake_mkspec_paths();
                    debug_msg(2, "Looking for mkspec %s in (%s)", qmakespec.toLatin1().constData(),
                              mkspec_roots.join("::").toLatin1().constData());
                    bool found_mkspec = false;
                    for (QStringList::ConstIterator it = mkspec_roots.begin(); it != mkspec_roots.end(); ++it) {
                        QString mkspec = (*it) + QLatin1Char('/') + qmakespec;
                        if (QFile::exists(mkspec)) {
                            found_mkspec = true;
                            *specp = qmakespec = mkspec;
                            break;
                        }
                    }
                    if (!found_mkspec) {
                        fprintf(stderr, "Could not find mkspecs for your QMAKESPEC(%s) after trying:\n\t%s\n",
                                qmakespec.toLatin1().constData(), mkspec_roots.join("\n\t").toLatin1().constData());
                        return false;
                    }
            }

            // We do this before reading the spec, so it can use module and feature paths from
            // here without resorting to tricks. This is the only planned use case anyway.
            if (!superfile.isEmpty()) {
                debug_msg(1, "Project super cache file: reading %s", superfile.toLatin1().constData());
                read(superfile, vars);
            }

            // parse qmake configuration
            doProjectInclude("spec_pre", IncludeFlagFeature, vars);
            while(qmakespec.endsWith(QLatin1Char('/')))
                qmakespec.truncate(qmakespec.length()-1);
            QString spec = qmakespec + QLatin1String("/qmake.conf");
            debug_msg(1, "QMAKESPEC conf: reading %s", spec.toLatin1().constData());
            if (!read(spec, vars)) {
                fprintf(stderr, "Failure to read QMAKESPEC conf file %s.\n", spec.toLatin1().constData());
                return false;
            }
#ifdef Q_OS_UNIX
            real_spec = QFileInfo(qmakespec).canonicalFilePath();
#else
            // We can't resolve symlinks as they do on Unix, so configure.exe puts the source of the
            // qmake.conf at the end of the default/qmake.conf in the QMAKESPEC_ORG variable.
            QString orig_spec = first(QLatin1String("QMAKESPEC_ORIGINAL"));
            real_spec = orig_spec.isEmpty() ? qmakespec : orig_spec;
#endif
            vars["QMAKESPEC"] << real_spec;
            short_spec = QFileInfo(real_spec).fileName();
            doProjectInclude("spec_post", IncludeFlagFeature, vars);
            // The spec extends the feature search path, so invalidate the cache.
            invalidateFeatureRoots();
            // The MinGW and x-build specs may change the separator; $$shell_{path,quote}() need it
            Option::dir_sep = first(QLatin1String("QMAKE_DIR_SEP"));

            if (!conffile.isEmpty()) {
                debug_msg(1, "Project config file: reading %s", conffile.toLatin1().constData());
                read(conffile, vars);
            }
            if (!cachefile.isEmpty()) {
                debug_msg(1, "QMAKECACHE file: reading %s", cachefile.toLatin1().constData());
                read(cachefile, vars);
            }
        }

        base_vars = vars;
      } else {
        vars = base_vars; // start with the base
      }
        setupProject();
    }

    for (QHash<QString, QStringList>::ConstIterator it = extra_vars.constBegin();
         it != extra_vars.constEnd(); ++it)
        vars.insert(it.key(), it.value());

    if(cmd & ReadFeatures) {
        debug_msg(1, "Processing default_pre: %s", vars["CONFIG"].join("::").toLatin1().constData());
        doProjectInclude("default_pre", IncludeFlagFeature, vars);
    }

    //before commandline
    if (cmd & ReadSetup) {
        parser.file = "(internal)";
        parser.from_file = false;
        parser.line_no = 1; //really arg count now.. duh
        reset();
        for(QStringList::ConstIterator it = Option::before_user_vars.begin();
            it != Option::before_user_vars.end(); ++it) {
            if(!parse((*it), vars)) {
                fprintf(stderr, "Argument failed to parse: %s\n", (*it).toLatin1().constData());
                return false;
            }
            parser.line_no++;
        }
    }

    // After user configs, to override them
    if (!extra_configs.isEmpty()) {
        parser.file = "(extra configs)";
        parser.from_file = false;
        parser.line_no = 1; //really arg count now.. duh
        parse("CONFIG += " + extra_configs.join(" "), vars);
    }

    if(cmd & ReadProFile) { // parse project file
        debug_msg(1, "Project file: reading %s", pfile.toLatin1().constData());
        if(pfile != "-" && !QFile::exists(pfile) && !pfile.endsWith(Option::pro_ext))
            pfile += Option::pro_ext;
        if(!read(pfile, vars))
            return false;
        if (need_restart) {
            base_vars.clear();
            cleanup();
            goto again;
        }
    }

    if (cmd & ReadSetup) {
        parser.file = "(internal)";
        parser.from_file = false;
        parser.line_no = 1; //really arg count now.. duh
        reset();
        for(QStringList::ConstIterator it = Option::after_user_vars.begin();
            it != Option::after_user_vars.end(); ++it) {
            if(!parse((*it), vars)) {
                fprintf(stderr, "Argument failed to parse: %s\n", (*it).toLatin1().constData());
                return false;
            }
            parser.line_no++;
        }
    }

    // Again, to ensure the project does not mess with us.
    // Specifically, do not allow a project to override debug/release within a
    // debug_and_release build pass - it's too late for that at this point anyway.
    if (!extra_configs.isEmpty()) {
        parser.file = "(extra configs)";
        parser.from_file = false;
        parser.line_no = 1; //really arg count now.. duh
        parse("CONFIG += " + extra_configs.join(" "), vars);
    }

    if(cmd & ReadFeatures) {
        debug_msg(1, "Processing default_post: %s", vars["CONFIG"].join("::").toLatin1().constData());
        doProjectInclude("default_post", IncludeFlagFeature, vars);

        QHash<QString, bool> processed;
        const QStringList &configs = vars["CONFIG"];
        debug_msg(1, "Processing CONFIG features: %s", configs.join("::").toLatin1().constData());
        while(1) {
            bool finished = true;
            for(int i = configs.size()-1; i >= 0; --i) {
		const QString config = configs[i].toLower();
                if(!processed.contains(config)) {
                    processed.insert(config, true);
                    if(doProjectInclude(config, IncludeFlagFeature, vars) == IncludeSuccess) {
                        finished = false;
                        break;
                    }
                }
            }
            if(finished)
                break;
        }
    }
    return true;
}

void
QMakeProject::setupProject()
{
    setTemplate(vars["TEMPLATE"]);
    if (pfile != "-")
        vars["TARGET"] << QFileInfo(pfile).baseName();
    vars["_PRO_FILE_"] << pfile;
    vars["_PRO_FILE_PWD_"] << (pfile.isEmpty() ? qmake_getpwd() : QFileInfo(pfile).absolutePath());
    vars["OUT_PWD"] << Option::output_dir;
}

void
QMakeProject::loadDefaults()
{
    vars["LITERAL_WHITESPACE"] << QLatin1String("\t");
    vars["LITERAL_DOLLAR"] << QLatin1String("$");
    vars["LITERAL_HASH"] << QLatin1String("#");
    vars["DIR_SEPARATOR"] << Option::dir_sep;
    vars["DIRLIST_SEPARATOR"] << Option::dirlist_sep;
    vars["QMAKE_QMAKE"] << Option::qmake_abslocation;
    vars["_DATE_"] << QDateTime::currentDateTime().toString();
#if defined(Q_OS_WIN32)
    vars["QMAKE_HOST.os"] << QString::fromLatin1("Windows");

    DWORD name_length = 1024;
    wchar_t name[1024];
    if (GetComputerName(name, &name_length))
        vars["QMAKE_HOST.name"] << QString::fromWCharArray(name);

    QSysInfo::WinVersion ver = QSysInfo::WindowsVersion;
    vars["QMAKE_HOST.version"] << QString::number(ver);
    QString verStr;
    switch (ver) {
    case QSysInfo::WV_Me: verStr = QLatin1String("WinMe"); break;
    case QSysInfo::WV_95: verStr = QLatin1String("Win95"); break;
    case QSysInfo::WV_98: verStr = QLatin1String("Win98"); break;
    case QSysInfo::WV_NT: verStr = QLatin1String("WinNT"); break;
    case QSysInfo::WV_2000: verStr = QLatin1String("Win2000"); break;
    case QSysInfo::WV_2003: verStr = QLatin1String("Win2003"); break;
    case QSysInfo::WV_XP: verStr = QLatin1String("WinXP"); break;
    case QSysInfo::WV_VISTA: verStr = QLatin1String("WinVista"); break;
    default: verStr = QLatin1String("Unknown"); break;
    }
    vars["QMAKE_HOST.version_string"] << verStr;

    SYSTEM_INFO info;
    GetSystemInfo(&info);
    QString archStr;
    switch (info.wProcessorArchitecture) {
# ifdef PROCESSOR_ARCHITECTURE_AMD64
    case PROCESSOR_ARCHITECTURE_AMD64:
        archStr = QLatin1String("x86_64");
        break;
# endif
    case PROCESSOR_ARCHITECTURE_INTEL:
        archStr = QLatin1String("x86");
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
# ifdef PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
    case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
# endif
        archStr = QLatin1String("IA64");
        break;
    default:
        archStr = QLatin1String("Unknown");
        break;
    }
    vars["QMAKE_HOST.arch"] << archStr;

# if defined(Q_CC_MSVC)
    QString paths = QString::fromLocal8Bit(qgetenv("PATH"));
    QString vcBin64 = QString::fromLocal8Bit(qgetenv("VCINSTALLDIR"));
    if (!vcBin64.endsWith('\\'))
        vcBin64.append('\\');
    vcBin64.append("bin\\amd64");
    QString vcBinX86_64 = QString::fromLocal8Bit(qgetenv("VCINSTALLDIR"));
    if (!vcBinX86_64.endsWith('\\'))
        vcBinX86_64.append('\\');
    vcBinX86_64.append("bin\\x86_amd64");
    if (paths.contains(vcBin64,Qt::CaseInsensitive) || paths.contains(vcBinX86_64,Qt::CaseInsensitive))
        vars["QMAKE_TARGET.arch"] << QString::fromLatin1("x86_64");
    else
        vars["QMAKE_TARGET.arch"] << QString::fromLatin1("x86");
# endif
#elif defined(Q_OS_UNIX)
    struct utsname name;
    if (!uname(&name)) {
        vars["QMAKE_HOST.os"] << QString::fromLocal8Bit(name.sysname);
        vars["QMAKE_HOST.name"] << QString::fromLocal8Bit(name.nodename);
        vars["QMAKE_HOST.version"] << QString::fromLocal8Bit(name.release);
        vars["QMAKE_HOST.version_string"] << QString::fromLocal8Bit(name.version);
        vars["QMAKE_HOST.arch"] << QString::fromLocal8Bit(name.machine);
    }
#endif
}

bool
QMakeProject::isActiveConfig(const QString &x, bool regex, QHash<QString, QStringList> *place)
{
    if(x.isEmpty())
        return true;

    //magic types for easy flipping
    if(x == "true")
        return true;
    else if(x == "false")
        return false;

    if (x == "host_build")
        return host_build;

    //mkspecs
    QRegExp re(x, Qt::CaseSensitive, QRegExp::Wildcard);
    if ((regex && re.exactMatch(short_spec)) || (!regex && short_spec == x))
        return true;

    //simple matching
    const QStringList &configs = (place ? (*place)["CONFIG"] : vars["CONFIG"]);
    for(QStringList::ConstIterator it = configs.begin(); it != configs.end(); ++it) {
        if(((regex && re.exactMatch((*it))) || (!regex && (*it) == x)) && re.exactMatch((*it)))
            return true;
    }
    return false;
}

bool
QMakeProject::doProjectTest(QString str, QHash<QString, QStringList> &place)
{
    QString chk = remove_quotes(str);
    if(chk.isEmpty())
        return true;
    bool invert_test = (chk.left(1) == "!");
    if(invert_test)
        chk = chk.mid(1);

    bool test=false;
    int lparen = chk.indexOf('(');
    if(lparen != -1) { // if there is an lparen in the chk, it IS a function
        int rparen = chk.indexOf(')', lparen);
        if(rparen == -1) {
            qmake_error_msg("Function missing right paren: " + chk);
        } else {
            QString func = chk.left(lparen);
            test = doProjectTest(func, chk.mid(lparen+1, rparen - lparen - 1), place);
        }
    } else {
        test = isActiveConfig(chk, true, &place);
    }
    if(invert_test)
        return !test;
    return test;
}

bool
QMakeProject::doProjectTest(QString func, const QString &params,
                            QHash<QString, QStringList> &place)
{
    return doProjectTest(func, split_arg_list(params), place);
}

QMakeProject::IncludeStatus
QMakeProject::doProjectInclude(QString file, uchar flags, QHash<QString, QStringList> &place)
{
    if(flags & IncludeFlagFeature) {
        if(!file.endsWith(Option::prf_ext))
            file += Option::prf_ext;
        {
            QStringList *&feature_roots = all_feature_roots[host_build];
            if(!feature_roots) {
                feature_roots = new QStringList;
                qmakeAddCacheClear(qmakeDeleteCacheClear<QStringList>, (void**)&feature_roots);
            }
            if (feature_roots->isEmpty())
                *feature_roots = qmakeFeaturePaths();
            debug_msg(2, "Looking for feature '%s' in (%s)", file.toLatin1().constData(),
			feature_roots->join("::").toLatin1().constData());
            int start_root = 0;
            if(parser.from_file) {
                QFileInfo currFile(parser.file), prfFile(file);
                if(currFile.fileName() == prfFile.fileName()) {
                    currFile = QFileInfo(currFile.canonicalFilePath());
                    for(int root = 0; root < feature_roots->size(); ++root) {
                        prfFile = QFileInfo(feature_roots->at(root) +
                                            QLatin1Char('/') + file).canonicalFilePath();
                        if(prfFile == currFile) {
                            start_root = root+1;
                            break;
                        }
                    }
                }
            }
            for(int root = start_root; root < feature_roots->size(); ++root) {
                QString prf(feature_roots->at(root) + QLatin1Char('/') + file);
                if (QFile::exists(prf)) {
                    file = prf;
                    goto foundf;
                }
            }
            return IncludeNoExist;
          foundf: ;
        }
        if(place["QMAKE_INTERNAL_INCLUDED_FEATURES"].indexOf(file) != -1)
            return IncludeFeatureAlreadyLoaded;
        place["QMAKE_INTERNAL_INCLUDED_FEATURES"].append(file);
    } else if (QDir::isRelativePath(file)) {
        QStringList include_roots;
        if(Option::output_dir != qmake_getpwd())
            include_roots << qmake_getpwd();
        include_roots << Option::output_dir;
        for(int root = 0; root < include_roots.size(); ++root) {
            QString testName = QDir::fromNativeSeparators(include_roots[root]);
            if (!testName.endsWith(QLatin1Char('/')))
                testName += QLatin1Char('/');
            testName += file;
            if(QFile::exists(testName)) {
                file = testName;
                goto foundi;
            }
        }
        return IncludeNoExist;
      foundi: ;
    } else if (!QFile::exists(file)) {
        return IncludeNoExist;
    }
    debug_msg(1, "Project Parser: %s'ing file %s.", (flags & IncludeFlagFeature) ? "load" : "include",
              file.toLatin1().constData());

    QString orig_file = file;
    int di = file.lastIndexOf(QLatin1Char('/'));
    QString oldpwd = qmake_getpwd();
    if(di != -1) {
        if(!qmake_setpwd(file.left(file.lastIndexOf(QLatin1Char('/'))))) {
            fprintf(stderr, "Cannot find directory: %s\n", file.left(di).toLatin1().constData());
            return IncludeFailure;
        }
    }
    bool parsed = false;
    parser_info pi = parser;
    {
        if(flags & (IncludeFlagNewProject|IncludeFlagNewParser)) {
            // The "project's variables" are used in other places (eg. export()) so it's not
            // possible to use "place" everywhere. Instead just set variables and grab them later
            QMakeProject proj(prop);
            if(flags & IncludeFlagNewParser) {
                parsed = proj.read(file, proj.variables()); // parse just that file (fromfile, infile)
            } else {
                parsed = proj.read(file); // parse all aux files (load/include into)
            }
            place = proj.variables();
        } else {
            QStack<ScopeBlock> sc = scope_blocks;
            IteratorBlock *it = iterator;
            FunctionBlock *fu = function;
            parsed = read(file, place);
            iterator = it;
            function = fu;
            scope_blocks = sc;
        }
    }
    if(parsed) {
        if(place["QMAKE_INTERNAL_INCLUDED_FILES"].indexOf(orig_file) == -1)
            place["QMAKE_INTERNAL_INCLUDED_FILES"].append(orig_file);
    } else {
        warn_msg(WarnParser, "%s:%d: Failure to include file %s.",
                 pi.file.toLatin1().constData(), pi.line_no, orig_file.toLatin1().constData());
    }
    parser = pi;
    qmake_setpwd(oldpwd);
    place["PWD"] = QStringList(qmake_getpwd());
    if(!parsed)
        return IncludeParseFailure;
    return IncludeSuccess;
}

static void
subAll(QStringList *val, const QStringList &diffval)
{
    foreach (const QString &dv, diffval)
        val->removeAll(dv);
}

inline static
bool hasSpecialChars(const QString &arg, const uchar (&iqm)[16])
{
    for (int x = arg.length() - 1; x >= 0; --x) {
        ushort c = arg.unicode()[x].unicode();
        if ((c < sizeof(iqm) * 8) && (iqm[c / 8] & (1 << (c & 7))))
            return true;
    }
    return false;
}

static QString
shellQuoteUnix(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0xdf, 0x07, 0x00, 0xd8,
        0x00, 0x00, 0x00, 0x38, 0x01, 0x00, 0x00, 0x78
    }; // 0-32 \'"$`<>|;&(){}*?#!~[]

    if (!arg.length())
        return QString::fromLatin1("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        ret.replace(QLatin1Char('\''), QLatin1String("'\\''"));
        ret.prepend(QLatin1Char('\''));
        ret.append(QLatin1Char('\''));
    }
    return ret;
}

static QString
shellQuoteWin(const QString &arg)
{
    // Chars that should be quoted (TM). This includes:
    // - control chars & space
    // - the shell meta chars "&()<>^|
    // - the potential separators ,;=
    static const uchar iqm[] = {
        0xff, 0xff, 0xff, 0xff, 0x45, 0x13, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x10
    };

    if (!arg.length())
        return QString::fromLatin1("\"\"");

    QString ret(arg);
    if (hasSpecialChars(ret, iqm)) {
        // Quotes are escaped and their preceding backslashes are doubled.
        // It's impossible to escape anything inside a quoted string on cmd
        // level, so the outer quoting must be "suspended".
        ret.replace(QRegExp(QLatin1String("(\\\\*)\"")), QLatin1String("\"\\1\\1\\^\"\""));
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

static QString
quoteValue(const QString &val)
{
    QString ret;
    ret.reserve(val.length());
    bool quote = val.isEmpty();
    bool escaping = false;
    for (int i = 0, l = val.length(); i < l; i++) {
        QChar c = val.unicode()[i];
        ushort uc = c.unicode();
        if (uc < 32) {
            if (!escaping) {
                escaping = true;
                ret += QLatin1String("$$escape_expand(");
            }
            switch (uc) {
            case '\r':
                ret += QLatin1String("\\\\r");
                break;
            case '\n':
                ret += QLatin1String("\\\\n");
                break;
            case '\t':
                ret += QLatin1String("\\\\t");
                break;
            default:
                ret += QString::fromLatin1("\\\\x%1").arg(uc, 2, 16, QLatin1Char('0'));
                break;
            }
        } else {
            if (escaping) {
                escaping = false;
                ret += QLatin1Char(')');
            }
            switch (uc) {
            case '\\':
                ret += QLatin1String("\\\\");
                break;
            case '"':
                ret += QLatin1String("\\\"");
                break;
            case '\'':
                ret += QLatin1String("\\'");
                break;
            case '$':
                ret += QLatin1String("\\$");
                break;
            case '#':
                ret += QLatin1String("$${LITERAL_HASH}");
                break;
            case 32:
                quote = true;
                // fallthrough
            default:
                ret += c;
                break;
            }
        }
    }
    if (escaping)
        ret += QLatin1Char(')');
    if (quote) {
        ret.prepend(QLatin1Char('"'));
        ret.append(QLatin1Char('"'));
    }
    return ret;
}

static bool
writeFile(const QString &name, QIODevice::OpenMode mode, const QString &contents, QString *errStr)
{
    QByteArray bytes = contents.toLocal8Bit();
    QFile cfile(name);
    if (!(mode & QIODevice::Append) && cfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (cfile.readAll() == bytes)
            return true;
        cfile.close();
    }
    if (!cfile.open(mode | QIODevice::WriteOnly | QIODevice::Text)) {
        *errStr = cfile.errorString();
        return false;
    }
    cfile.write(bytes);
    cfile.close();
    if (cfile.error() != QFile::NoError) {
        *errStr = cfile.errorString();
        return false;
    }
    return true;
}

static QByteArray
getCommandOutput(const QString &args)
{
    QByteArray out;
    if (FILE *proc = QT_POPEN(args.toLatin1().constData(), "r")) {
        while (!feof(proc)) {
            char buff[10 * 1024];
            int read_in = int(fread(buff, 1, sizeof(buff), proc));
            if (!read_in)
                break;
            out += QByteArray(buff, read_in);
        }
        QT_PCLOSE(proc);
    }
    return out;
}

#ifdef Q_OS_WIN
static QString windowsErrorCode()
{
    wchar_t *string = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL,
                  GetLastError(),
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPWSTR)&string,
                  0,
                  NULL);
    QString ret = QString::fromWCharArray(string);
    LocalFree((HLOCAL)string);
    return ret;
}
#endif

QStringList
QMakeProject::doProjectExpand(QString func, const QString &params,
                              QHash<QString, QStringList> &place)
{
    return doProjectExpand(func, split_arg_list(params), place);
}

QStringList
QMakeProject::doProjectExpand(QString func, QStringList args,
                              QHash<QString, QStringList> &place)
{
    QList<QStringList> args_list;
    for(int i = 0; i < args.size(); ++i) {
        QStringList arg = split_value_list(args[i]), tmp;
        for(int i = 0; i < arg.size(); ++i)
            tmp += doVariableReplaceExpand(arg[i], place);;
        args_list += tmp;
    }
    return doProjectExpand(func, args_list, place);
}

static void
populateDeps(const QStringList &deps, const QString &prefix,
             QHash<QString, QSet<QString> > &dependencies, QHash<QString, QStringList> &dependees,
             QStringList &rootSet, QHash<QString, QStringList> &place)
{
    foreach (const QString &item, deps)
        if (!dependencies.contains(item)) {
            QSet<QString> &dset = dependencies[item]; // Always create entry
            QStringList depends = place.value(prefix + item + ".depends");
            if (depends.isEmpty()) {
                rootSet << item;
            } else {
                foreach (const QString &dep, depends) {
                    dset.insert(dep);
                    dependees[dep] << item;
                }
                populateDeps(depends, prefix, dependencies, dependees, rootSet, place);
            }
        }
}

QStringList
QMakeProject::doProjectExpand(QString func, QList<QStringList> args_list,
                              QHash<QString, QStringList> &place)
{
    func = func.trimmed();
    if(replaceFunctions.contains(func)) {
        FunctionBlock *defined = replaceFunctions[func];
        function_blocks.push(defined);
        QStringList ret;
        defined->exec(args_list, this, place, ret);
        bool correct = function_blocks.pop() == defined;
        Q_ASSERT(correct); Q_UNUSED(correct);
        return ret;
    }

    QStringList args; //why don't the builtin functions just use args_list? --Sam
    for(int i = 0; i < args_list.size(); ++i)
        args += args_list[i].join(QString(Option::field_sep));

    ExpandFunc func_t = qmake_expandFunctions().value(func);
    if (!func_t && (func_t = qmake_expandFunctions().value(func.toLower())))
        warn_msg(WarnDeprecated, "%s:%d: Using uppercased builtin functions is deprecated.",
                 parser.file.toLatin1().constData(), parser.line_no);
    debug_msg(1, "Running project expand: %s(%s) [%d]",
              func.toLatin1().constData(), args.join("::").toLatin1().constData(), func_t);

    QStringList ret;
    switch(func_t) {
    case E_MEMBER: {
        if(args.count() < 1 || args.count() > 3) {
            fprintf(stderr, "%s:%d: member(var, start, end) requires three arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            bool ok = true;
            const QStringList &var = values(args.first(), place);
            int start = 0, end = 0;
            if(args.count() >= 2) {
                QString start_str = args[1];
                start = start_str.toInt(&ok);
                if(!ok) {
                    if(args.count() == 2) {
                        int dotdot = start_str.indexOf("..");
                        if(dotdot != -1) {
                            start = start_str.left(dotdot).toInt(&ok);
                            if(ok)
                                end = start_str.mid(dotdot+2).toInt(&ok);
                        }
                    }
                    if(!ok)
                        fprintf(stderr, "%s:%d: member() argument 2 (start) '%s' invalid.\n",
                                parser.file.toLatin1().constData(), parser.line_no,
                                start_str.toLatin1().constData());
                } else {
                    end = start;
                    if(args.count() == 3)
                        end = args[2].toInt(&ok);
                    if(!ok)
                        fprintf(stderr, "%s:%d: member() argument 3 (end) '%s' invalid.\n",
                                parser.file.toLatin1().constData(), parser.line_no,
                                args[2].toLatin1().constData());
                }
            }
            if(ok) {
                if(start < 0)
                    start += var.count();
                if(end < 0)
                    end += var.count();
                if(start < 0 || start >= var.count() || end < 0 || end >= var.count()) {
                    //nothing
                } else if(start < end) {
                    for(int i = start; i <= end && (int)var.count() >= i; i++)
                        ret += var[i];
                } else {
                    for(int i = start; i >= end && (int)var.count() >= i && i >= 0; i--)
                        ret += var[i];
                }
            }
        }
        break; }
    case E_FIRST:
    case E_LAST: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: %s(var) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no, func.toLatin1().constData());
        } else {
            const QStringList &var = values(args.first(), place);
            if(!var.isEmpty()) {
                if(func_t == E_FIRST)
                    ret = QStringList(var[0]);
                else
                    ret = QStringList(var[var.size()-1]);
            }
        }
        break; }
    case E_CAT: {
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d: cat(file) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QString file = Option::normalizePath(args[0]);

            bool blob = false;
            bool lines = false;
            bool singleLine = true;
            if (args.count() > 1) {
                if (!args.at(1).compare(QLatin1String("false"), Qt::CaseInsensitive))
                    singleLine = false;
                else if (!args.at(1).compare(QLatin1String("blob"), Qt::CaseInsensitive))
                    blob = true;
                else if (!args.at(1).compare(QLatin1String("lines"), Qt::CaseInsensitive))
                    lines = true;
            }
            QFile qfile(file);
            if(qfile.open(QIODevice::ReadOnly)) {
                QTextStream stream(&qfile);
                if (blob) {
                    ret += stream.readAll();
                } else {
                    while (!stream.atEnd()) {
                        if (lines) {
                            ret += stream.readLine();
                        } else {
                            ret += split_value_list(stream.readLine().trimmed());
                            if (!singleLine)
                                ret += "\n";
                        }
                    }
                }
            }
        }
        break; }
    case E_FROMFILE: {
        if(args.count() != 2) {
            fprintf(stderr, "%s:%d: fromfile(file, variable) requires two arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QString seek_var = args[1], file = Option::normalizePath(args[0]);

            QHash<QString, QStringList> tmp;
            if(doProjectInclude(file, IncludeFlagNewParser, tmp) == IncludeSuccess) {
                if(tmp.contains("QMAKE_INTERNAL_INCLUDED_FILES")) {
                    QStringList &out = place["QMAKE_INTERNAL_INCLUDED_FILES"];
                    const QStringList &in = tmp["QMAKE_INTERNAL_INCLUDED_FILES"];
                    for(int i = 0; i < in.size(); ++i) {
                        if(out.indexOf(in[i]) == -1)
                            out += in[i];
                    }
                }
                ret = tmp[seek_var];
            }
        }
        break; }
    case E_EVAL: {
        if (args.count() != 1) {
            fprintf(stderr, "%s:%d: eval(variable) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);

        } else {
            ret += place.value(args.at(0));
        }
        break; }
    case E_LIST: {
        static int x = 0;
        QString tmp;
        tmp.sprintf(".QMAKE_INTERNAL_TMP_VAR_%d", x++);
        ret = QStringList(tmp);
        QStringList &lst = (*((QHash<QString, QStringList>*)&place))[tmp];
        lst.clear();
        for(QStringList::ConstIterator arg_it = args.begin();
            arg_it != args.end(); ++arg_it)
            lst += split_value_list((*arg_it));
        break; }
    case E_SPRINTF: {
        if(args.count() < 1) {
            fprintf(stderr, "%s:%d: sprintf(format, ...) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QString tmp = args.at(0);
            for(int i = 1; i < args.count(); ++i)
                tmp = tmp.arg(args.at(i));
            ret = split_value_list(tmp);
        }
        break; }
    case E_FORMAT_NUMBER:
        if (args.count() > 2) {
            fprintf(stderr, "%s:%d: format_number(number[, options...]) requires one or two arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            int ibase = 10;
            int obase = 10;
            int width = 0;
            bool zeropad = false;
            bool leftalign = false;
            enum { DefaultSign, PadSign, AlwaysSign } sign = DefaultSign;
            if (args.count() >= 2) {
                foreach (const QString &opt, split_value_list(args.at(1))) {
                    if (opt.startsWith(QLatin1String("ibase="))) {
                        ibase = opt.mid(6).toInt();
                    } else if (opt.startsWith(QLatin1String("obase="))) {
                        obase = opt.mid(6).toInt();
                    } else if (opt.startsWith(QLatin1String("width="))) {
                        width = opt.mid(6).toInt();
                    } else if (opt == QLatin1String("zeropad")) {
                        zeropad = true;
                    } else if (opt == QLatin1String("padsign")) {
                        sign = PadSign;
                    } else if (opt == QLatin1String("alwayssign")) {
                        sign = AlwaysSign;
                    } else if (opt == QLatin1String("leftalign")) {
                        leftalign = true;
                    } else {
                        fprintf(stderr, "%s:%d: format_number(): invalid format option %s.\n",
                                parser.file.toLatin1().constData(), parser.line_no,
                                opt.toLatin1().constData());
                        goto formfail;
                    }
                }
            }
            if (args.at(0).contains(QLatin1Char('.'))) {
                fprintf(stderr, "%s:%d: format_number(): floats are currently not supported.\n",
                        parser.file.toLatin1().constData(), parser.line_no);
                break;
            }
            bool ok;
            qlonglong num = args.at(0).toLongLong(&ok, ibase);
            if (!ok) {
                fprintf(stderr, "%s:%d: format_number(): malformed number %s for base %d.\n",
                        parser.file.toLatin1().constData(), parser.line_no,
                        args.at(0).toLatin1().constData(), ibase);
                break;
            }
            QString outstr;
            if (num < 0) {
                num = -num;
                outstr = QLatin1Char('-');
            } else if (sign == AlwaysSign) {
                outstr = QLatin1Char('+');
            } else if (sign == PadSign) {
                outstr = QLatin1Char(' ');
            }
            QString numstr = QString::number(num, obase);
            int space = width - outstr.length() - numstr.length();
            if (space <= 0) {
                outstr += numstr;
            } else if (leftalign) {
                outstr += numstr + QString(space, QLatin1Char(' '));
            } else if (zeropad) {
                outstr += QString(space, QLatin1Char('0')) + numstr;
            } else {
                outstr.prepend(QString(space, QLatin1Char(' ')));
                outstr += numstr;
            }
            ret += outstr;
        }
      formfail:
        break;
    case E_JOIN: {
        if(args.count() < 1 || args.count() > 4) {
            fprintf(stderr, "%s:%d: join(var, glue, before, after) requires four"
                    "arguments.\n", parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QString glue, before, after;
            if(args.count() >= 2)
                glue = args[1];
            if(args.count() >= 3)
                before = args[2];
            if(args.count() == 4)
                after = args[3];
            const QStringList &var = values(args.first(), place);
            if(!var.isEmpty())
                ret = split_value_list(before + var.join(glue) + after);
        }
        break; }
    case E_SPLIT: {
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d split(var, sep) requires one or two arguments\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QString sep = QString(Option::field_sep);
            if(args.count() >= 2)
                sep = args[1];
            QStringList var = values(args.first(), place);
            for(QStringList::ConstIterator vit = var.begin(); vit != var.end(); ++vit) {
                QStringList lst = (*vit).split(sep);
                for(QStringList::ConstIterator spltit = lst.begin(); spltit != lst.end(); ++spltit)
                    ret += (*spltit);
            }
        }
        break; }
    case E_BASENAME:
    case E_DIRNAME:
    case E_SECTION: {
        bool regexp = false;
        QString sep, var;
        int beg=0, end=-1;
        if(func_t == E_SECTION) {
            if(args.count() != 3 && args.count() != 4) {
                fprintf(stderr, "%s:%d section(var, sep, begin, end) requires three argument\n",
                        parser.file.toLatin1().constData(), parser.line_no);
            } else {
                var = args[0];
                sep = args[1];
                beg = args[2].toInt();
                if(args.count() == 4)
                    end = args[3].toInt();
            }
        } else {
            if(args.count() != 1) {
                fprintf(stderr, "%s:%d %s(var) requires one argument.\n",
                        parser.file.toLatin1().constData(), parser.line_no, func.toLatin1().constData());
            } else {
                var = args[0];
                regexp = true;
                sep = "[" + QRegExp::escape(Option::dir_sep) + "/]";
                if(func_t == E_DIRNAME)
                    end = -2;
                else
                    beg = -1;
            }
        }
        if(!var.isNull()) {
            const QStringList &l = values(var, place);
            for(QStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
                QString separator = sep;
                if(regexp)
                    ret += (*it).section(QRegExp(separator), beg, end);
                else
                    ret += (*it).section(separator, beg, end);
            }
        }
        break; }
    case E_FIND: {
        if(args.count() != 2) {
            fprintf(stderr, "%s:%d find(var, str) requires two arguments\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QRegExp regx(args[1]);
            const QStringList &var = values(args.first(), place);
            for(QStringList::ConstIterator vit = var.begin();
                vit != var.end(); ++vit) {
                if(regx.indexIn(*vit) != -1)
                    ret += (*vit);
            }
        }
        break;  }
    case E_SYSTEM: {
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d system(execut) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            bool blob = false;
            bool lines = false;
            bool singleLine = true;
            if (args.count() > 1) {
                if (!args.at(1).compare(QLatin1String("false"), Qt::CaseInsensitive))
                    singleLine = false;
                else if (!args.at(1).compare(QLatin1String("blob"), Qt::CaseInsensitive))
                    blob = true;
                else if (!args.at(1).compare(QLatin1String("lines"), Qt::CaseInsensitive))
                    lines = true;
            }
            QByteArray bytes = getCommandOutput(args.at(0));
            if (lines) {
                QTextStream stream(bytes);
                while (!stream.atEnd())
                    ret += stream.readLine();
            } else {
                QString output = QString::fromLocal8Bit(bytes);
                if (blob) {
                    ret += output;
                } else {
                    output.replace(QLatin1Char('\t'), QLatin1Char(' '));
                    if (singleLine)
                        output.replace(QLatin1Char('\n'), QLatin1Char(' '));
                    ret += split_value_list(output);
                }
            }
        }
        break; }
    case E_UNIQUE: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d unique(var) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            const QStringList &var = values(args.first(), place);
            for(int i = 0; i < var.count(); i++) {
                if(!ret.contains(var[i]))
                    ret.append(var[i]);
            }
        }
        break; }
    case E_REVERSE:
        if (args.count() != 1) {
            fprintf(stderr, "%s:%d reverse(var) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QStringList var = values(args.first(), place);
            for (int i = 0; i < var.size() / 2; i++)
                var.swap(i, var.size() - i - 1);
            ret += var;
        }
        break;
    case E_QUOTE:
        ret = args;
        break;
    case E_ESCAPE_EXPAND: {
        for(int i = 0; i < args.size(); ++i) {
            QChar *i_data = args[i].data();
            int i_len = args[i].length();
            for(int x = 0; x < i_len; ++x) {
                if(*(i_data+x) == '\\' && x < i_len-1) {
                    if(*(i_data+x+1) == '\\') {
                        ++x;
                    } else {
                        struct {
                            char in, out;
                        } mapped_quotes[] = {
                            { 'n', '\n' },
                            { 't', '\t' },
                            { 'r', '\r' },
                            { 0, 0 }
                        };
                        for(int i = 0; mapped_quotes[i].in; ++i) {
                            if(*(i_data+x+1) == mapped_quotes[i].in) {
                                *(i_data+x) = mapped_quotes[i].out;
                                if(x < i_len-2)
                                    memmove(i_data+x+1, i_data+x+2, (i_len-x-2)*sizeof(QChar));
                                --i_len;
                                break;
                            }
                        }
                    }
                }
            }
            ret.append(QString(i_data, i_len));
        }
        break; }
    case E_RE_ESCAPE: {
        for(int i = 0; i < args.size(); ++i)
            ret += QRegExp::escape(args[i]);
        break; }
    case E_VAL_ESCAPE:
        if (args.count() != 1) {
            fprintf(stderr, "%s:%d val_escape(var) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QStringList vals = values(args.at(0), place);
            ret.reserve(vals.length());
            foreach (const QString &str, vals)
                ret += quoteValue(str);
        }
        break;
    case E_UPPER:
    case E_LOWER: {
        for(int i = 0; i < args.size(); ++i) {
            if(func_t == E_UPPER)
                ret += args[i].toUpper();
            else
                ret += args[i].toLower();
        }
        break; }
    case E_FILES: {
        if(args.count() != 1 && args.count() != 2) {
            fprintf(stderr, "%s:%d files(pattern) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            bool recursive = false;
            if(args.count() == 2)
                recursive = (args[1].toLower() == "true" || args[1].toInt());
            QStringList dirs;
            QString r = Option::normalizePath(args[0]);
            int slash = r.lastIndexOf(QLatin1Char('/'));
            if(slash != -1) {
                dirs.append(r.left(slash));
                r = r.mid(slash+1);
            } else {
                dirs.append("");
            }

            QRegExp regex(r, Qt::CaseSensitive, QRegExp::Wildcard);
            for(int d = 0; d < dirs.count(); d++) {
                QString dir = dirs[d];
                if (!dir.isEmpty() && !dir.endsWith(QLatin1Char('/')))
                    dir += "/";

                QDir qdir(dir);
                for(int i = 0; i < (int)qdir.count(); ++i) {
                    if(qdir[i] == "." || qdir[i] == "..")
                        continue;
                    QString fname = dir + qdir[i];
                    if(QFileInfo(fname).isDir()) {
                        if(recursive)
                            dirs.append(fname);
                    }
                    if(regex.exactMatch(qdir[i]))
                        ret += fname;
                }
            }
        }
        break; }
    case E_PROMPT: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d prompt(question) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else if(pfile == "-") {
            fprintf(stderr, "%s:%d prompt(question) cannot be used when '-o -' is used.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            QString msg = fixEnvVariables(args.first());
            if(!msg.endsWith("?"))
                msg += "?";
            fprintf(stderr, "Project %s: %s ", func.toUpper().toLatin1().constData(),
                    msg.toLatin1().constData());

            QFile qfile;
            if(qfile.open(stdin, QIODevice::ReadOnly)) {
                QTextStream t(&qfile);
                ret = split_value_list(t.readLine());
            }
        }
        break; }
    case E_REPLACE: {
        if(args.count() != 3 ) {
            fprintf(stderr, "%s:%d replace(var, before, after) requires three arguments\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            const QRegExp before( args[1] );
            const QString after( args[2] );
            QStringList var = values(args.first(), place);
            for(QStringList::Iterator it = var.begin(); it != var.end(); ++it)
                ret += it->replace(before, after);
        }
        break; }
    case E_SIZE: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: size(var) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            int size = values(args[0], place).size();
            ret += QString::number(size);
        }
        break; }
    case E_SORT_DEPENDS:
    case E_RESOLVE_DEPENDS: {
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d: %s(var, prefix) requires one or two arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no, func.toLatin1().constData());
        } else {
            QHash<QString, QSet<QString> > dependencies;
            QHash<QString, QStringList> dependees;
            QStringList rootSet;

            QStringList orgList = values(args[0], place);
            populateDeps(orgList, (args.count() != 2 ? QString() : args[1]),
                         dependencies, dependees, rootSet, place);

            for (int i = 0; i < rootSet.size(); ++i) {
                const QString &item = rootSet.at(i);
                if ((func_t == E_RESOLVE_DEPENDS) || orgList.contains(item))
                    ret.prepend(item);
                foreach (const QString &dep, dependees[item]) {
                    QSet<QString> &dset = dependencies[dep];
                    dset.remove(rootSet.at(i)); // *Don't* use 'item' - rootSet may have changed!
                    if (dset.isEmpty())
                        rootSet << dep;
                }
            }
        }
        break; }
    case E_ENUMERATE_VARS:
        ret += place.keys();
        break;
    case E_SHADOWED: {
        QString val = QDir::cleanPath(QFileInfo(args.at(0)).absoluteFilePath());
        if (Option::mkfile::source_root.isEmpty()) {
            ret += val;
        } else if (val.startsWith(Option::mkfile::source_root)
                   && (val.length() == Option::mkfile::source_root.length()
                       || val.at(Option::mkfile::source_root.length()) == QLatin1Char('/'))) {
            ret += Option::mkfile::build_root + val.mid(Option::mkfile::source_root.length());
        }
        break; }
    case E_ABSOLUTE_PATH:
        if (args.count() > 2)
            fprintf(stderr, "%s:%d absolute_path(path[, base]) requires one or two arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else
            ret += QDir::cleanPath(QDir(args.count() > 1 ? args.at(1) : QString())
                                   .absoluteFilePath(args.at(0)));
        break;
    case E_RELATIVE_PATH:
        if (args.count() > 2)
            fprintf(stderr, "%s:%d relative_path(path[, base]) requires one or two arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else
            ret += QDir::cleanPath(QDir(args.count() > 1 ? args.at(1) : QString())
                                   .relativeFilePath(args.at(0)));
        break;
    case E_CLEAN_PATH:
        if (args.count() != 1)
            fprintf(stderr, "%s:%d clean_path(path) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else
            ret += QDir::cleanPath(args.at(0));
        break;
    case E_SYSTEM_PATH:
        if (args.count() != 1)
            fprintf(stderr, "%s:%d system_path(path) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else
            ret += Option::fixPathToLocalOS(args.at(0), false);
        break;
    case E_SHELL_PATH:
        if (args.count() != 1)
            fprintf(stderr, "%s:%d shell_path(path) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else
            ret += Option::fixPathToTargetOS(args.at(0), false);
        break;
    case E_SYSTEM_QUOTE:
        if (args.count() != 1)
            fprintf(stderr, "%s:%d system_quote(args) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else
#ifdef Q_OS_WIN
            ret += shellQuoteWin(args.at(0));
#else
            ret += shellQuoteUnix(args.at(0));
#endif
        break;
    case E_SHELL_QUOTE:
        if (args.count() != 1)
            fprintf(stderr, "%s:%d shell_quote(args) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        else if (Option::dir_sep.at(0) != QLatin1Char('/'))
            ret += shellQuoteWin(args.at(0));
        else
            ret += shellQuoteUnix(args.at(0));
        break;
    default: {
        fprintf(stderr, "%s:%d: Unknown replace function: %s\n",
                parser.file.toLatin1().constData(), parser.line_no,
                func.toLatin1().constData());
        break; }
    }
    return ret;
}

bool
QMakeProject::doProjectTest(QString func, QStringList args, QHash<QString, QStringList> &place)
{
    QList<QStringList> args_list;
    for(int i = 0; i < args.size(); ++i) {
        QStringList arg = split_value_list(args[i]), tmp;
        for(int i = 0; i < arg.size(); ++i)
            tmp += doVariableReplaceExpand(arg[i], place);
        args_list += tmp;
    }
    return doProjectTest(func, args_list, place);
}

bool
QMakeProject::doProjectTest(QString func, QList<QStringList> args_list, QHash<QString, QStringList> &place)
{
    func = func.trimmed();

    if(testFunctions.contains(func)) {
        FunctionBlock *defined = testFunctions[func];
        QStringList ret;
        function_blocks.push(defined);
        defined->exec(args_list, this, place, ret);
        bool correct = function_blocks.pop() == defined;
        Q_ASSERT(correct); Q_UNUSED(correct);

        if(ret.isEmpty()) {
            return true;
        } else {
            if(ret.first() == "true") {
                return true;
            } else if(ret.first() == "false") {
                return false;
            } else {
                bool ok;
                int val = ret.first().toInt(&ok);
                if(ok)
                    return val;
                fprintf(stderr, "%s:%d Unexpected return value from test %s [%s].\n",
                        parser.file.toLatin1().constData(),
                        parser.line_no, func.toLatin1().constData(),
                        ret.join("::").toLatin1().constData());
            }
            return false;
        }
        return false;
    }

    QStringList args; //why don't the builtin functions just use args_list? --Sam
    for(int i = 0; i < args_list.size(); ++i)
        args += args_list[i].join(QString(Option::field_sep));

    TestFunc func_t = qmake_testFunctions().value(func);
    debug_msg(1, "Running project test: %s(%s) [%d]",
              func.toLatin1().constData(), args.join("::").toLatin1().constData(), func_t);

    switch(func_t) {
    case T_REQUIRES:
        return doProjectCheckReqs(args, place);
    case T_LESSTHAN:
    case T_GREATERTHAN: {
        if(args.count() != 2) {
            fprintf(stderr, "%s:%d: %s(variable, value) requires two arguments.\n", parser.file.toLatin1().constData(),
                    parser.line_no, func.toLatin1().constData());
            return false;
        }
        QString rhs(args[1]), lhs(values(args[0], place).join(QString(Option::field_sep)));
        bool ok;
        int rhs_int = rhs.toInt(&ok);
        if(ok) { // do integer compare
            int lhs_int = lhs.toInt(&ok);
            if(ok) {
                if(func_t == T_GREATERTHAN)
                    return lhs_int > rhs_int;
                return lhs_int < rhs_int;
            }
        }
        if(func_t == T_GREATERTHAN)
            return lhs > rhs;
        return lhs < rhs; }
    case T_IF: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: if(condition) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        const QString cond = args.first();
        const QChar *d = cond.unicode();
        QChar quote = 0;
        bool ret = true, or_op = false;
        QString test;
        for(int d_off = 0, parens = 0, d_len = cond.size(); d_off < d_len; ++d_off) {
            if(!quote.isNull()) {
                if(*(d+d_off) == quote)
                    quote = QChar();
            } else if(*(d+d_off) == '(') {
                ++parens;
            } else if(*(d+d_off) == ')') {
                --parens;
            } else if(*(d+d_off) == '"' /*|| *(d+d_off) == '\''*/) {
                quote = *(d+d_off);
            }
            if(!parens && quote.isNull() && (*(d+d_off) == QLatin1Char(':') || *(d+d_off) == QLatin1Char('|') || d_off == d_len-1)) {
                if(d_off == d_len-1)
                    test += *(d+d_off);
                if(!test.isEmpty()) {
                    if (or_op != ret)
                        ret = doProjectTest(test, place);
                    test.clear();
                }
                if(*(d+d_off) == QLatin1Char(':')) {
                    or_op = false;
                } else if(*(d+d_off) == QLatin1Char('|')) {
                    or_op = true;
                }
            } else {
                test += *(d+d_off);
            }
        }
        return ret; }
    case T_EQUALS:
        if(args.count() != 2) {
            fprintf(stderr, "%s:%d: %s(variable, value) requires two arguments.\n", parser.file.toLatin1().constData(),
                    parser.line_no, func.toLatin1().constData());
            return false;
        }
        return values(args[0], place).join(QString(Option::field_sep)) == args[1];
    case T_EXISTS: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: exists(file) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        QString file = Option::normalizePath(args.first());

        if(QFile::exists(file))
            return true;
        //regular expression I guess
        QString dirstr = qmake_getpwd();
        int slsh = file.lastIndexOf(QLatin1Char('/'));
        if(slsh != -1) {
            dirstr = file.left(slsh+1);
            file = file.right(file.length() - slsh - 1);
        }
        return QDir(dirstr).entryList(QStringList(file)).count(); }
    case T_EXPORT:
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: export(variable) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        for(int i = 0; i < function_blocks.size(); ++i) {
            FunctionBlock *f = function_blocks.at(i);
            f->vars[args[0]] = values(args[0], place);
            if(!i && f->calling_place)
                (*f->calling_place)[args[0]] = values(args[0], place);
        }
        return true;
    case T_CLEAR:
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: clear(variable) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        if(!place.contains(args[0]))
            return false;
        place[args[0]].clear();
        return true;
    case T_UNSET:
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: unset(variable) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        if(!place.contains(args[0]))
            return false;
        place.remove(args[0]);
        return true;
    case T_EVAL: {
        if(args.count() < 1 && 0) {
            fprintf(stderr, "%s:%d: eval(project) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        QString project = args.join(" ");
        parser_info pi = parser;
        parser.from_file = false;
        parser.file = "(eval)";
        parser.line_no = 0;
        QTextStream t(&project, QIODevice::ReadOnly);
        bool ret = read(t, place);
        parser = pi;
        return ret; }
    case T_CONFIG: {
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d: CONFIG(config) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        if(args.count() == 1)
            return isActiveConfig(args[0]);
        const QStringList mutuals = args[1].split('|');
        const QStringList &configs = values("CONFIG", place);
        for(int i = configs.size()-1; i >= 0; i--) {
            for(int mut = 0; mut < mutuals.count(); mut++) {
                if(configs[i] == mutuals[mut].trimmed())
                    return (configs[i] == args[0]);
            }
        }
        return false; }
    case T_SYSTEM:
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d: system(exec) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        if(args.count() == 2) {
            const QString sarg = args[1];
            if (sarg.toLower() == "true" || sarg.toInt())
                warn_msg(WarnParser, "%s:%d: system()'s second argument is now hard-wired to false.\n",
                         parser.file.toLatin1().constData(), parser.line_no);
        }
        return system(args[0].toLatin1().constData()) == 0;
    case T_RETURN:
        if(function_blocks.isEmpty()) {
            fprintf(stderr, "%s:%d unexpected return()\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
            FunctionBlock *f = function_blocks.top();
            f->cause_return = true;
            if(args_list.count() >= 1)
                f->return_value += args_list[0];
        }
        return true;
    case T_BREAK:
        if(iterator)
            iterator->cause_break = true;
        else
            fprintf(stderr, "%s:%d unexpected break()\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        return true;
    case T_NEXT:
        if(iterator)
            iterator->cause_next = true;
        else
            fprintf(stderr, "%s:%d unexpected next()\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        return true;
    case T_DEFINED:
        if(args.count() < 1 || args.count() > 2) {
            fprintf(stderr, "%s:%d: defined(function) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
        } else {
           if(args.count() > 1) {
               if(args[1] == "test")
                   return testFunctions.contains(args[0]);
               else if(args[1] == "replace")
                   return replaceFunctions.contains(args[0]);
               else if(args[1] == "var")
                   return place.contains(args[0]);
               fprintf(stderr, "%s:%d: defined(function, type): unexpected type [%s].\n",
                       parser.file.toLatin1().constData(), parser.line_no,
                       args[1].toLatin1().constData());
            } else {
                if(replaceFunctions.contains(args[0]) || testFunctions.contains(args[0]))
                    return true;
            }
        }
        return false;
    case T_CONTAINS: {
        if(args.count() < 2 || args.count() > 3) {
            fprintf(stderr, "%s:%d: contains(var, val) requires at lesat 2 arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }
        QRegExp regx(args[1]);
        const QStringList &l = values(args[0], place);
        if(args.count() == 2) {
            for(int i = 0; i < l.size(); ++i) {
                const QString val = l[i];
                if(regx.exactMatch(val) || val == args[1])
                    return true;
            }
        } else {
            const QStringList mutuals = args[2].split('|');
            for(int i = l.size()-1; i >= 0; i--) {
                const QString val = l[i];
                for(int mut = 0; mut < mutuals.count(); mut++) {
                    if(val == mutuals[mut].trimmed())
                        return (regx.exactMatch(val) || val == args[1]);
                }
            }
        }
        return false; }
    case T_INFILE: {
        if(args.count() < 2 || args.count() > 3) {
            fprintf(stderr, "%s:%d: infile(file, var, val) requires at least 2 arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }

        bool ret = false;
        QHash<QString, QStringList> tmp;
        if(doProjectInclude(Option::normalizePath(args[0]), IncludeFlagNewParser, tmp) == IncludeSuccess) {
            if(tmp.contains("QMAKE_INTERNAL_INCLUDED_FILES")) {
                QStringList &out = place["QMAKE_INTERNAL_INCLUDED_FILES"];
                const QStringList &in = tmp["QMAKE_INTERNAL_INCLUDED_FILES"];
                for(int i = 0; i < in.size(); ++i) {
                    if(out.indexOf(in[i]) == -1)
                        out += in[i];
                }
            }
            if(args.count() == 2) {
                ret = tmp.contains(args[1]);
            } else {
                QRegExp regx(args[2]);
                const QStringList &l = tmp[args[1]];
                for(QStringList::ConstIterator it = l.begin(); it != l.end(); ++it) {
                    if(regx.exactMatch((*it)) || (*it) == args[2]) {
                        ret = true;
                        break;
                    }
                }
            }
        }
        return ret; }
    case T_COUNT:
        if(args.count() != 2 && args.count() != 3) {
            fprintf(stderr, "%s:%d: count(var, count) requires two arguments.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        if(args.count() == 3) {
            QString comp = args[2];
            if(comp == ">" || comp == "greaterThan")
                return values(args[0], place).count() > args[1].toInt();
            if(comp == ">=")
                return values(args[0], place).count() >= args[1].toInt();
            if(comp == "<" || comp == "lessThan")
                return values(args[0], place).count() < args[1].toInt();
            if(comp == "<=")
                return values(args[0], place).count() <= args[1].toInt();
            if(comp == "equals" || comp == "isEqual" || comp == "=" || comp == "==")
                return values(args[0], place).count() == args[1].toInt();
            fprintf(stderr, "%s:%d: unexpected modifier to count(%s)\n", parser.file.toLatin1().constData(),
                    parser.line_no, comp.toLatin1().constData());
            return false;
        }
        return values(args[0], place).count() == args[1].toInt();
    case T_ISEMPTY:
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: isEmpty(var) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        return values(args[0], place).isEmpty();
    case T_INCLUDE:
    case T_LOAD: {
        QString parseInto;
        const bool include_statement = (func_t == T_INCLUDE);
        bool ignore_error = false;
        if(args.count() >= 2) {
            if(func_t == T_INCLUDE) {
                parseInto = args[1];
                if (args.count() == 3){
                    QString sarg = args[2];
                    if (sarg.toLower() == "true" || sarg.toInt())
                        ignore_error = true;
                }
            } else {
                QString sarg = args[1];
                ignore_error = (sarg.toLower() == "true" || sarg.toInt());
            }
        } else if(args.count() != 1) {
            QString func_desc = "load(feature)";
            if(include_statement)
                func_desc = "include(file)";
            fprintf(stderr, "%s:%d: %s requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no, func_desc.toLatin1().constData());
            return false;
        }
        QString file = Option::normalizePath(args.first());
        uchar flags = IncludeFlagNone;
        if(!include_statement)
            flags |= IncludeFlagFeature;
        IncludeStatus stat = IncludeFailure;
        if(!parseInto.isEmpty()) {
            QHash<QString, QStringList> symbols;
            stat = doProjectInclude(file, flags|IncludeFlagNewProject, symbols);
            if(stat == IncludeSuccess) {
                QHash<QString, QStringList> out_place;
                for(QHash<QString, QStringList>::ConstIterator it = place.begin(); it != place.end(); ++it) {
                    const QString var = it.key();
                    if(var != parseInto && !var.startsWith(parseInto + "."))
                        out_place.insert(var, it.value());
                }
                for(QHash<QString, QStringList>::ConstIterator it = symbols.begin(); it != symbols.end(); ++it) {
                    const QString var = it.key();
                    if(!var.startsWith("."))
                        out_place.insert(parseInto + "." + it.key(), it.value());
                }
                place = out_place;
            }
        } else {
            stat = doProjectInclude(file, flags, place);
        }
        if(stat == IncludeFeatureAlreadyLoaded) {
            warn_msg(WarnParser, "%s:%d: Duplicate of loaded feature %s",
                     parser.file.toLatin1().constData(), parser.line_no, file.toLatin1().constData());
        } else if(stat == IncludeNoExist && !ignore_error) {
            warn_msg(WarnAll, "%s:%d: Unable to find file for inclusion %s",
                     parser.file.toLatin1().constData(), parser.line_no, file.toLatin1().constData());
            return false;
        } else if(stat >= IncludeFailure) {
            if(!ignore_error) {
                printf("Project LOAD(): Feature %s cannot be found.\n", file.toLatin1().constData());
                if (!ignore_error)
#if defined(QT_BUILD_QMAKE_LIBRARY)
                    return false;
#else
                    exit(3);
#endif
            }
            return false;
        }
        return true; }
    case T_DEBUG: {
        if(args.count() != 2) {
            fprintf(stderr, "%s:%d: debug(level, message) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no);
            return false;
        }
        QString msg = fixEnvVariables(args[1]);
        debug_msg(args[0].toInt(), "Project DEBUG: %s", msg.toLatin1().constData());
        return true; }
    case T_LOG:
    case T_ERROR:
    case T_MESSAGE:
    case T_WARNING: {
        if(args.count() != 1) {
            fprintf(stderr, "%s:%d: %s(message) requires one argument.\n", parser.file.toLatin1().constData(),
                    parser.line_no, func.toLatin1().constData());
            return false;
        }
        QString msg = fixEnvVariables(args.first());
        if (func_t == T_LOG) {
            fputs(msg.toLatin1().constData(), stderr);
        } else {
            fprintf(stderr, "Project %s: %s\n", func.toUpper().toLatin1().constData(), msg.toLatin1().constData());
            if (func == "error")
#if defined(QT_BUILD_QMAKE_LIBRARY)
                return false;
#else
                exit(2);
#endif
        }
        return true; }
    case T_OPTION:
        if (args.count() != 1) {
            fprintf(stderr, "%s:%d: option() requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }
        if (args.first() == "host_build") {
            if (!host_build && isActiveConfig("cross_compile")) {
                host_build = true;
                need_restart = true;
            }
        } else {
            fprintf(stderr, "%s:%d: unrecognized option() argument '%s'.\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    args.first().toLatin1().constData());
            return false;
        }
        return true;
    case T_CACHE: {
        if (args.count() > 3) {
            fprintf(stderr, "%s:%d: cache(var, [set|add|sub] [transient] [super], [srcvar]) requires one to three arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }
        bool persist = true;
        bool super = false;
        enum { CacheSet, CacheAdd, CacheSub } mode = CacheSet;
        QString srcvar;
        if (args.count() >= 2) {
            foreach (const QString &opt, split_value_list(args.at(1))) {
                if (opt == QLatin1String("transient")) {
                    persist = false;
                } else if (opt == QLatin1String("super")) {
                    super = true;
                } else if (opt == QLatin1String("set")) {
                    mode = CacheSet;
                } else if (opt == QLatin1String("add")) {
                    mode = CacheAdd;
                } else if (opt == QLatin1String("sub")) {
                    mode = CacheSub;
                } else {
                    fprintf(stderr, "%s:%d: cache(): invalid flag %s.\n",
                            parser.file.toLatin1().constData(), parser.line_no,
                            opt.toLatin1().constData());
                    return false;
                }
            }
            if (args.count() >= 3) {
                srcvar = args.at(2);
            } else if (mode != CacheSet) {
                fprintf(stderr, "%s:%d: cache(): modes other than 'set' require a source variable.\n",
                        parser.file.toLatin1().constData(), parser.line_no);
                return false;
            }
        }
        QString varstr;
        QString dstvar = args.at(0);
        if (!dstvar.isEmpty()) {
            if (srcvar.isEmpty())
                srcvar = dstvar;
            if (!place.contains(srcvar)) {
                fprintf(stderr, "%s:%d: variable %s is not defined.\n",
                        parser.file.toLatin1().constData(), parser.line_no,
                        srcvar.toLatin1().constData());
                return false;
            }
            // The current ("native") value can differ from the cached value, e.g., the current
            // CONFIG will typically have more values than the cached one. Therefore we deal with
            // them separately.
            const QStringList diffval = values(srcvar, place);
            const QStringList oldval = base_vars.value(dstvar);
            QStringList newval;
            if (mode == CacheSet) {
                newval = diffval;
            } else {
                newval = oldval;
                if (mode == CacheAdd)
                    newval += diffval;
                else
                    subAll(&newval, diffval);
            }
            // We assume that whatever got the cached value to be what it is now will do so
            // the next time as well, so it is OK that the early exit here will skip the
            // persisting as well.
            if (oldval == newval)
                return true;
            base_vars[dstvar] = newval;
            do {
                if (dstvar == "QMAKEPATH")
                    cached_qmakepath = newval;
                else if (dstvar == "QMAKEFEATURES")
                    cached_qmakefeatures = newval;
                else
                    break;
                invalidateFeatureRoots();
            } while (false);
            if (!persist)
                return true;
            varstr = dstvar;
            if (mode == CacheAdd)
                varstr += QLatin1String(" +=");
            else if (mode == CacheSub)
                varstr += QLatin1String(" -=");
            else
                varstr += QLatin1String(" =");
            if (diffval.count() == 1) {
                varstr += QLatin1Char(' ');
                varstr += quoteValue(diffval.at(0));
            } else if (!diffval.isEmpty()) {
                foreach (const QString &vval, diffval) {
                    varstr += QLatin1String(" \\\n    ");
                    varstr += quoteValue(vval);
                }
            }
            varstr += QLatin1Char('\n');
        }
        QString fn;
        if (super) {
            if (superfile.isEmpty()) {
                superfile = Option::output_dir + QLatin1String("/.qmake.super");
                printf("Info: creating super cache file %s\n", superfile.toLatin1().constData());
                vars["_QMAKE_SUPER_CACHE_"] << superfile;
            }
            fn = superfile;
        } else {
            if (cachefile.isEmpty()) {
                cachefile = Option::output_dir + QLatin1String("/.qmake.cache");
                printf("Info: creating cache file %s\n", cachefile.toLatin1().constData());
                vars["_QMAKE_CACHE_"] << cachefile;
                if (cached_build_root.isEmpty()) {
                    cached_build_root = Option::output_dir;
                    cached_source_root = values("_PRO_FILE_PWD_", place).first();
                    if (cached_source_root == cached_build_root)
                        cached_source_root.clear();
                    invalidateFeatureRoots();
                }
            }
            fn = cachefile;
        }
        QFileInfo qfi(fn);
        if (!QDir::current().mkpath(qfi.path())) {
            fprintf(stderr, "%s:%d: ERROR creating cache directory %s\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    qfi.path().toLatin1().constData());
            return false;
        }
        QString errStr;
        if (!writeFile(fn, QIODevice::Append, varstr, &errStr)) {
            fprintf(stderr, "ERROR writing cache file %s: %s\n",
                    fn.toLatin1().constData(), errStr.toLatin1().constData());
#if defined(QT_BUILD_QMAKE_LIBRARY)
            return false;
#else
            exit(2);
#endif
        }
        return true; }
    case T_MKPATH:
        if (args.count() != 1) {
            fprintf(stderr, "%s:%d: mkpath(name) requires one argument.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }
        if (!QDir::current().mkpath(args.at(0))) {
            fprintf(stderr, "%s:%d: ERROR creating directory %s\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    QDir::toNativeSeparators(args.at(0)).toLatin1().constData());
            return false;
        }
        return true;
    case T_WRITE_FILE: {
        if (args.count() > 3) {
            fprintf(stderr, "%s:%d: write_file(name, [content var, [append]]) requires one to three arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }
        QIODevice::OpenMode mode = QIODevice::Truncate;
        QString contents;
        if (args.count() >= 2) {
            QStringList vals = values(args.at(1), place);
            if (!vals.isEmpty())
                contents = vals.join(QLatin1String("\n")) + QLatin1Char('\n');
            if (args.count() >= 3)
                if (!args.at(2).compare(QLatin1String("append"), Qt::CaseInsensitive))
                    mode = QIODevice::Append;
        }
        QFileInfo qfi(args.at(0));
        if (!QDir::current().mkpath(qfi.path())) {
            fprintf(stderr, "%s:%d: ERROR creating directory %s\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    qfi.path().toLatin1().constData());
            return false;
        }
        QString errStr;
        if (!writeFile(args.at(0), mode, contents, &errStr)) {
            fprintf(stderr, "%s:%d ERROR writing %s: %s\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    args.at(0).toLatin1().constData(), errStr.toLatin1().constData());
            return false;
        }
        return true; }
    case T_TOUCH: {
        if (args.count() != 2) {
            fprintf(stderr, "%s:%d: touch(file, reffile) requires two arguments.\n",
                    parser.file.toLatin1().constData(), parser.line_no);
            return false;
        }
#ifdef Q_OS_UNIX
        struct stat st;
        if (stat(args.at(1).toLocal8Bit().constData(), &st)) {
            fprintf(stderr, "%s:%d: ERROR: cannot stat() reference file %s: %s.\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    args.at(1).toLatin1().constData(), strerror(errno));
            return false;
        }
        struct utimbuf utb;
        utb.actime = time(0);
        utb.modtime = st.st_mtime;
        if (utime(args.at(0).toLocal8Bit().constData(), &utb)) {
            fprintf(stderr, "%s:%d: ERROR: cannot touch %s: %s.\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    args.at(0).toLatin1().constData(), strerror(errno));
            return false;
        }
#else
        HANDLE rHand = CreateFile((wchar_t*)args.at(1).utf16(),
                                  GENERIC_READ, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (rHand == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "%s:%d: ERROR: cannot open() reference file %s: %s.\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    args.at(1).toLatin1().constData(),
                    windowsErrorCode().toLatin1().constData());
            return false;
        }
        FILETIME ft;
        GetFileTime(rHand, 0, 0, &ft);
        CloseHandle(rHand);
        HANDLE wHand = CreateFile((wchar_t*)args.at(0).utf16(),
                                  GENERIC_WRITE, FILE_SHARE_READ,
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (wHand == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "%s:%d: ERROR: cannot open %s: %s.\n",
                    parser.file.toLatin1().constData(), parser.line_no,
                    args.at(0).toLatin1().constData(),
                    windowsErrorCode().toLatin1().constData());
            return false;
        }
        SetFileTime(wHand, 0, 0, &ft);
        CloseHandle(wHand);
#endif
        break; }
    default:
        fprintf(stderr, "%s:%d: Unknown test function: %s\n", parser.file.toLatin1().constData(), parser.line_no,
                func.toLatin1().constData());
    }
    return false;
}

bool
QMakeProject::doProjectCheckReqs(const QStringList &deps, QHash<QString, QStringList> &place)
{
    bool ret = false;
    for(QStringList::ConstIterator it = deps.begin(); it != deps.end(); ++it) {
        bool test = doProjectTest((*it), place);
        if(!test) {
            debug_msg(1, "Project Parser: %s:%d Failed test: REQUIRES = %s",
                      parser.file.toLatin1().constData(), parser.line_no,
                      (*it).toLatin1().constData());
            place["QMAKE_FAILED_REQUIREMENTS"].append((*it));
            ret = false;
        }
    }
    return ret;
}

bool
QMakeProject::test(const QString &v)
{
    QHash<QString, QStringList> tmp = vars;
    return doProjectTest(v, tmp);
}

bool
QMakeProject::test(const QString &func, const QList<QStringList> &args)
{
    QHash<QString, QStringList> tmp = vars;
    return doProjectTest(func, args, tmp);
}

QStringList
QMakeProject::expand(const QString &str)
{
    bool ok;
    QHash<QString, QStringList> tmp = vars;
    const QStringList ret = doVariableReplaceExpand(str, tmp, &ok);
    if(ok)
        return ret;
    return QStringList();
}

QString
QMakeProject::expand(const QString &str, const QString &file, int line)
{
    bool ok;
    parser_info pi = parser;
    parser.file = file;
    parser.line_no = line;
    parser.from_file = false;
    QHash<QString, QStringList> tmp = vars;
    const QStringList ret = doVariableReplaceExpand(str, tmp, &ok);
    parser = pi;
    return ok ? ret.join(QString(Option::field_sep)) : QString();
}

QStringList
QMakeProject::expand(const QString &func, const QList<QStringList> &args)
{
    QHash<QString, QStringList> tmp = vars;
    return doProjectExpand(func, args, tmp);
}

bool
QMakeProject::doVariableReplace(QString &str, QHash<QString, QStringList> &place)
{
    bool ret;
    str = doVariableReplaceExpand(str, place, &ret).join(QString(Option::field_sep));
    return ret;
}

QStringList
QMakeProject::doVariableReplaceExpand(const QString &str, QHash<QString, QStringList> &place, bool *ok)
{
    QStringList ret;
    if(ok)
        *ok = true;
    if(str.isEmpty())
        return ret;

    const ushort LSQUARE = '[';
    const ushort RSQUARE = ']';
    const ushort LCURLY = '{';
    const ushort RCURLY = '}';
    const ushort LPAREN = '(';
    const ushort RPAREN = ')';
    const ushort DOLLAR = '$';
    const ushort SLASH = '\\';
    const ushort UNDERSCORE = '_';
    const ushort DOT = '.';
    const ushort SPACE = ' ';
    const ushort TAB = '\t';
    const ushort SINGLEQUOTE = '\'';
    const ushort DOUBLEQUOTE = '"';

    ushort unicode, quote = 0;
    const QChar *str_data = str.data();
    const int str_len = str.length();

    ushort term;
    QString var, args;

    int replaced = 0;
    QString current;
    for(int i = 0; i < str_len; ++i) {
        unicode = str_data[i].unicode();
        const int start_var = i;
        if(unicode == DOLLAR && str_len > i+2) {
            unicode = str_data[++i].unicode();
            if(unicode == DOLLAR) {
                term = 0;
                var.clear();
                args.clear();
                enum { VAR, ENVIRON, FUNCTION, PROPERTY } var_type = VAR;
                unicode = str_data[++i].unicode();
                if(unicode == LSQUARE) {
                    unicode = str_data[++i].unicode();
                    term = RSQUARE;
                    var_type = PROPERTY;
                } else if(unicode == LCURLY) {
                    unicode = str_data[++i].unicode();
                    var_type = VAR;
                    term = RCURLY;
                } else if(unicode == LPAREN) {
                    unicode = str_data[++i].unicode();
                    var_type = ENVIRON;
                    term = RPAREN;
                }
                while(1) {
                    if(!(unicode & (0xFF<<8)) &&
                       unicode != DOT && unicode != UNDERSCORE &&
                       //unicode != SINGLEQUOTE && unicode != DOUBLEQUOTE &&
                       (unicode < 'a' || unicode > 'z') && (unicode < 'A' || unicode > 'Z') &&
                       (unicode < '0' || unicode > '9') && (!term || unicode != '/'))
                        break;
                    var.append(QChar(unicode));
                    if(++i == str_len)
                        break;
                    unicode = str_data[i].unicode();
                    // at this point, i points to either the 'term' or 'next' character (which is in unicode)
                }
                if(var_type == VAR && unicode == LPAREN) {
                    var_type = FUNCTION;
                    int depth = 0;
                    while(1) {
                        if(++i == str_len)
                            break;
                        unicode = str_data[i].unicode();
                        if(unicode == LPAREN) {
                            depth++;
                        } else if(unicode == RPAREN) {
                            if(!depth)
                                break;
                            --depth;
                        }
                        args.append(QChar(unicode));
                    }
                    if(++i < str_len)
                        unicode = str_data[i].unicode();
                    else
                        unicode = 0;
                    // at this point i is pointing to the 'next' character (which is in unicode)
                    // this might actually be a term character since you can do $${func()}
                }
                if(term) {
                    if(unicode != term) {
                        qmake_error_msg("Missing " + QString(term) + " terminator [found " + (unicode?QString(unicode):QString("end-of-line")) + "]");
                        if(ok)
                            *ok = false;
                        return QStringList();
                    }
                } else {
                    // move the 'cursor' back to the last char of the thing we were looking at
                    --i;
                }
                // since i never points to the 'next' character, there is no reason for this to be set
                unicode = 0;

                QStringList replacement;
                if(var_type == ENVIRON) {
                    replacement = split_value_list(QString::fromLocal8Bit(qgetenv(var.toLatin1().constData())));
                } else if(var_type == PROPERTY) {
                    if(prop)
                        replacement = split_value_list(prop->value(var));
                } else if(var_type == FUNCTION) {
                    replacement = doProjectExpand(var, args, place);
                } else if(var_type == VAR) {
                    replacement = magicValues(var, place);
                }
                if(!(replaced++) && start_var)
                    current = str.left(start_var);
                if(!replacement.isEmpty()) {
                    if(quote) {
                        current += replacement.join(QString(Option::field_sep));
                    } else {
                        current += replacement.takeFirst();
                        if(!replacement.isEmpty()) {
                            if(!current.isEmpty())
                                ret.append(current);
                            current = replacement.takeLast();
                            if(!replacement.isEmpty())
                                ret += replacement;
                        }
                    }
                }
                debug_msg(2, "Project Parser [var replace]: %s -> %s",
                          str.toLatin1().constData(), var.toLatin1().constData(),
                          replacement.join("::").toLatin1().constData());
            } else {
                if(replaced)
                    current.append("$");
            }
        }
        if(quote && unicode == quote) {
            unicode = 0;
            quote = 0;
        } else if(unicode == SLASH) {
            bool escape = false;
            const char *symbols = "[]{}()$\\'\"";
            for(const char *s = symbols; *s; ++s) {
                if(str_data[i+1].unicode() == (ushort)*s) {
                    i++;
                    escape = true;
                    if(!(replaced++))
                        current = str.left(start_var);
                    current.append(str.at(i));
                    break;
                }
            }
            if(!escape && !backslashWarned) {
                backslashWarned = true;
                warn_msg(WarnDeprecated, "%s:%d: Unescaped backslashes are deprecated.",
                         parser.file.toLatin1().constData(), parser.line_no);
            }
            if(escape || !replaced)
                unicode =0;
        } else if(!quote && (unicode == SINGLEQUOTE || unicode == DOUBLEQUOTE)) {
            quote = unicode;
            unicode = 0;
            if(!(replaced++) && i)
                current = str.left(i);
        } else if(!quote && (unicode == SPACE || unicode == TAB)) {
            unicode = 0;
            if(!current.isEmpty()) {
                ret.append(current);
                current.clear();
            }
        }
        if(replaced && unicode)
            current.append(QChar(unicode));
    }
    if(!replaced)
        ret = QStringList(str);
    else if(!current.isEmpty())
        ret.append(current);
    //qDebug() << "REPLACE" << str << ret;
    if (quote)
        warn_msg(WarnDeprecated, "%s:%d: Unmatched quotes are deprecated.",
                 parser.file.toLatin1().constData(), parser.line_no);
    return ret;
}

QStringList QMakeProject::magicValues(const QString &_var, const QHash<QString, QStringList> &place) const
{
    QString var = varMap(_var);
    if (var == QLatin1String("_LINE_")) { //parser line number
        return QStringList(QString::number(parser.line_no));
    } else if(var == QLatin1String("_FILE_")) { //parser file
        return QStringList(parser.file);
    }
    return place[var];
}

QStringList &QMakeProject::values(const QString &_var, QHash<QString, QStringList> &place)
{
    QString var = varMap(_var);
    return place[var];
}

bool QMakeProject::isEmpty(const QString &v) const
{
    QHash<QString, QStringList>::ConstIterator it = vars.constFind(v);
    return it == vars.constEnd() || it->isEmpty();
}

void
QMakeProject::dump() const
{
    QStringList out;
    for (QHash<QString, QStringList>::ConstIterator it = vars.begin(); it != vars.end(); ++it) {
        if (!it.key().startsWith('.')) {
            QString str = it.key() + " =";
            foreach (const QString &v, it.value())
                str += ' ' + quoteValue(v);
            out << str;
        }
    }
    out.sort();
    foreach (const QString &v, out)
        puts(qPrintable(v));
}

QT_END_NAMESPACE
