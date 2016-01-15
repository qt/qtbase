/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

/****************************************************************************
**
** In addition, as a special exception, the copyright holders listed above give
** permission to link the code of its release of Qt with the OpenSSL project's
** "OpenSSL" library (or modified versions of the "OpenSSL" library that use the
** same license as the original version), and distribute the linked executables.
**
** You must comply with the GNU General Public License version 2 in all
** respects for all of the code used other than the "OpenSSL" code.  If you
** modify this file, you may extend this exception to your version of the file,
** but you are not obligated to do so.  If you do not wish to do so, delete
** this exception statement from your version of this file.
**
****************************************************************************/

#include "qsslsocket_openssl_p.h"
#include <QtCore/private/qjni_p.h>

QT_BEGIN_NAMESPACE

QList<QByteArray> QSslSocketPrivate::fetchSslCertificateData()
{
    QList<QByteArray> certificateData;

    QJNIObjectPrivate certificates = QJNIObjectPrivate::callStaticObjectMethod("org/qtproject/qt5/android/QtNative",
                                                                               "getSSLCertificates",
                                                                               "()[[B");
    if (!certificates.isValid())
        return certificateData;

    QJNIEnvironmentPrivate env;
    jobjectArray jcertificates = static_cast<jobjectArray>(certificates.object());
    const jint nCertificates = env->GetArrayLength(jcertificates);

    for (int i = 0; i < nCertificates; ++i) {
        jbyteArray jCert = static_cast<jbyteArray>(env->GetObjectArrayElement(jcertificates, i));
        const uint sz = env->GetArrayLength(jCert);
        jbyte *buffer = env->GetByteArrayElements(jCert, 0);
        certificateData.append(QByteArray(reinterpret_cast<char*>(buffer), sz));

        env->ReleaseByteArrayElements(jCert, buffer, JNI_ABORT); // don't copy back the elements
        env->DeleteLocalRef(jCert);
    }

    return certificateData;
}

QT_END_NAMESPACE
