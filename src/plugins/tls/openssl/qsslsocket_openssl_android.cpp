// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include <QtCore/QJniEnvironment>
#include <QtCore/QJniObject>
#include <QtCore/QList>
#include <QtCore/QByteArray>

QT_BEGIN_NAMESPACE

namespace QTlsPrivate {

QList<QByteArray> fetchSslCertificateData()
{
    QList<QByteArray> certificateData;

    QJniObject certificates = QJniObject::callStaticObjectMethod("org/qtproject/qt/android/QtNative",
                                                                 "getSSLCertificates",
                                                                 "()[[B");
    if (!certificates.isValid())
        return certificateData;

    QJniEnvironment env;
    jobjectArray jcertificates = certificates.object<jobjectArray>();
    const jint nCertificates = env->GetArrayLength(jcertificates);
    certificateData.reserve(static_cast<int>(nCertificates));

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

} // namespace QTlsPrivate

QT_END_NAMESPACE
