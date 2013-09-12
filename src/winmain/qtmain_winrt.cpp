/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Windows main function of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

/*
  This file contains the code in the qtmain library for WinRT.
  qtmain contains the WinRT startup code and is required for
  linking to the Qt DLL.

  When a Windows application starts, the WinMain function is
  invoked. WinMain creates the WinRT application which in turn
  calls the main entry point.
*/

extern "C" int main(int, char **);

#include <qbytearray.h>
#include <qstring.h>
#include <qlist.h>
#include <qvector.h>

#include <wrl.h>
#include <Windows.ApplicationModel.core.h>

using namespace ABI::Windows::ApplicationModel;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;

#define qHString(x) Wrappers::HString::MakeReference(x).Get()
#define CoreApplicationClass RuntimeClass_Windows_ApplicationModel_Core_CoreApplication
typedef ITypedEventHandler<Core::CoreApplicationView *, Activation::IActivatedEventArgs *> ActivatedHandler;

class AppContainer : public Microsoft::WRL::RuntimeClass<Core::IFrameworkView>
{
public:
    AppContainer(int argc, wchar_t **argv) : m_argc(argc)
    {
        m_argv.reserve(argc);
        for (int i = 0; i < argc; ++i) {
            QByteArray arg = QString::fromWCharArray(argv[i]).toLocal8Bit();
            m_argv.append(qstrdup(arg.constData()));
        }
    }

    ~AppContainer()
    {
        foreach (const char *arg, m_argv)
            delete[] arg;
    }

    // IFrameworkView Methods
    HRESULT __stdcall Initialize(Core::ICoreApplicationView *view)
    {
        view->add_Activated(Callback<ActivatedHandler>(this, &AppContainer::onActivated).Get(),
                            &m_activationToken);
        return S_OK;
    }
    HRESULT __stdcall SetWindow(ABI::Windows::UI::Core::ICoreWindow *) { return S_OK; }
    HRESULT __stdcall Load(HSTRING) { return S_OK; }
    HRESULT __stdcall Run()
    {
        return main(m_argv.count(), m_argv.data());
    }
    HRESULT __stdcall Uninitialize() { return S_OK; }

private:
    // Activation handler
    HRESULT onActivated(Core::ICoreApplicationView *, Activation::IActivatedEventArgs *args)
    {
        Activation::ILaunchActivatedEventArgs *launchArgs;
        if (SUCCEEDED(args->QueryInterface(&launchArgs))) {
            for (int i = m_argc; i < m_argv.size(); ++i)
                delete[] m_argv[i];
            m_argv.resize(m_argc);
            HSTRING arguments;
            launchArgs->get_Arguments(&arguments);
            foreach (const QByteArray &arg, QString::fromWCharArray(
                         WindowsGetStringRawBuffer(arguments, nullptr)).toLocal8Bit().split(' ')) {
                m_argv.append(qstrdup(arg.constData()));
            }
        }
        return S_OK;
    }

    int m_argc;
    QVector<char *> m_argv;
    EventRegistrationToken m_activationToken;
};

class AppViewSource : public Microsoft::WRL::RuntimeClass<Core::IFrameworkViewSource>
{
public:
    AppViewSource(int argc, wchar_t *argv[]) : argc(argc), argv(argv) { }
    HRESULT __stdcall CreateView(Core::IFrameworkView **frameworkView)
    {
        return (*frameworkView = Make<AppContainer>(argc, argv).Detach()) ? S_OK : E_OUTOFMEMORY;
    }
private:
    int argc;
    wchar_t **argv;
};

// Main entry point for Appx containers
int wmain(int argc, wchar_t *argv[])
{
    if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
        return 1;

    Core::ICoreApplication *appFactory;
    if (FAILED(RoGetActivationFactory(qHString(CoreApplicationClass), IID_PPV_ARGS(&appFactory))))
        return 2;

    return appFactory->Run(Make<AppViewSource>(argc, argv).Get());
}
