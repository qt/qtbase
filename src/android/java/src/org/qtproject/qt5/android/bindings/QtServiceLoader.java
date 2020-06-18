/*
    Copyright (c) 2016, BogDan Vatra <bogdan@kde.org>
    Contact: http://www.qt.io/licensing/

    Commercial License Usage
    Licensees holding valid commercial Qt licenses may use this file in
    accordance with the commercial license agreement provided with the
    Software or, alternatively, in accordance with the terms contained in
    a written agreement between you and The Qt Company. For licensing terms
    and conditions see http://www.qt.io/terms-conditions. For further
    information use the contact form at http://www.qt.io/contact-us.

    BSD License Usage
    Alternatively, this file may be used under the BSD license as follows:
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package org.qtproject.qt5.android.bindings;

import android.os.Bundle;
import android.content.ComponentName;
import android.content.pm.PackageManager;

public class QtServiceLoader extends QtLoader {
    QtService m_service;
    QtServiceLoader(QtService service) {
        super(service, QtService.class);
        m_service = service;
    }

    public void onCreate() {
        try {
            m_contextInfo = m_service.getPackageManager().getServiceInfo(new ComponentName(m_service, m_service.getClass()), PackageManager.GET_META_DATA);
        } catch (Exception e) {
            e.printStackTrace();
            m_service.stopSelf();
            return;
        }

        if (QtApplication.m_delegateObject != null && QtApplication.onCreate != null) {
            Bundle bundle = null;
            QtApplication.invokeDelegateMethod(QtApplication.onCreate, bundle);
        }
        startApp(true);
    }

    @Override
    protected void finish() {
        m_service.stopSelf();
    }

    @Override
    protected String loaderClassName() {
        return "org.qtproject.qt5.android.QtServiceDelegate";
    }

    @Override
    protected Class<?> contextClassName() {
        return android.app.Service.class;
    }
}
