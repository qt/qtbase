#ifndef LIB_H
#define LIB_H

// App funcitons typedef and register functions. The application
// calls this function to register its init and exit function.
typedef void (*QAppInitFunction)(int argv, char **argc);
typedef int (*QAppExitFunction)(void);
void qCoreRegisterAppFunctions(QAppInitFunction appInitFunction, QAppExitFunction appExitFunction);
void qGuiRegisterAppFunctions(QAppInitFunction appInitFunction, QAppExitFunction appExitFunction);
void qWidgetsRegisterAppFunctions(QAppInitFunction appInitFunction, QAppExitFunction appExitFunction);

// Convenience macro for registerting the app init and exit
// functions. We need to run code before main is called. 
// Accomplish this by using a global constructor
#define QT_CORE_REGISTER_APP_FUNCTIONS(qtAppInitFunction, qtAppExitFunction) \
class QAppFunctionRegistrar \
{ \
public:  \
    QAppFunctionRegistrar() { qCoreRegisterAppFunctions(qtAppInitFunction,  qtAppExitFunction); } \
}; \
QAppFunctionRegistrar qtAppFunctionRegistar;

#define QT_GUI_REGISTER_APP_FUNCTIONS(qtAppInitFunction, qtAppExitFunction) \
class QAppFunctionRegistrar \
{ \
public:  \
    QAppFunctionRegistrar() { qGuiRegisterAppFunctions(qtAppInitFunction,  qtAppExitFunction); } \
}; \
QAppFunctionRegistrar qtAppFunctionRegistar;


#define QT_WIDGETS_REGISTER_APP_FUNCTIONS(qtAppInitFunction, qtAppExitFunction) \
class QAppFunctionRegistrar \
{ \
public:  \
    QAppFunctionRegistrar() { qWidgetsRegisterAppFunctions(qtAppInitFunction,  qtAppExitFunction); } \
}; \
QAppFunctionRegistrar qtAppFunctionRegistar;



#endif
