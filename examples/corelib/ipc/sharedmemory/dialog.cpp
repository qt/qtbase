/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
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

#include "dialog.h"
#include <QFileDialog>
#include <QBuffer>
#include <QtCore/QDebug>

/*!
  \class Dialog

  \brief This class is a simple example of how to use QSharedMemory.

  It is a simple dialog that presents a few buttons. To compile the
  example, run make in qt/examples/ipc. Then run the executable twice
  to create two processes running the dialog. In one of the processes,
  press the button to load an image into a shared memory segment, and
  then select an image file to load. Once the first process has loaded
  and displayed the image, in the second process, press the button to
  read the same image from shared memory. The second process displays
  the same image loaded from its new loaction in shared memory.
*/

/*!
  The class contains a data member \l {QSharedMemory} {sharedMemory},
  which is initialized with the key "QSharedMemoryExample" to force
  all instances of Dialog to access the same shared memory segment.
  The constructor also connects the clicked() signal from each of the
  three dialog buttons to the slot function appropriate for handling
  each button.
*/
//! [0]
Dialog::Dialog(QWidget *parent)
  : QDialog(parent), sharedMemory("QSharedMemoryExample")
{
    ui.setupUi(this);
    connect(ui.loadFromFileButton, SIGNAL(clicked()), SLOT(loadFromFile()));
    connect(ui.loadFromSharedMemoryButton,
            SIGNAL(clicked()),
            SLOT(loadFromMemory()));
    setWindowTitle(tr("SharedMemory Example"));
}
//! [0]

/*!
  This slot function is called when the \tt {Load Image From File...}
  button is pressed on the firs Dialog process. First, it tests
  whether the process is already connected to a shared memory segment
  and, if so, detaches from that segment. This ensures that we always
  start the example from the beginning if we run it multiple times
  with the same two Dialog processes. After detaching from an existing
  shared memory segment, the user is prompted to select an image file.
  The selected file is loaded into a QImage. The QImage is displayed
  in the Dialog and streamed into a QBuffer with a QDataStream.

  Next, it gets a new shared memory segment from the system big enough
  to hold the image data in the QBuffer, and it locks the segment to
  prevent the second Dialog process from accessing it. Then it copies
  the image from the QBuffer into the shared memory segment. Finally,
  it unlocks the shared memory segment so the second Dialog process
  can access it.

  After this function runs, the user is expected to press the \tt
  {Load Image from Shared Memory} button on the second Dialog process.

  \sa loadFromMemory()
 */
//! [1]
void Dialog::loadFromFile()
{
    if (sharedMemory.isAttached())
        detach();

    ui.label->setText(tr("Select an image file"));
    QString fileName = QFileDialog::getOpenFileName(0, QString(), QString(),
                                        tr("Images (*.png *.xpm *.jpg)"));
    QImage image;
    if (!image.load(fileName)) {
        ui.label->setText(tr("Selected file is not an image, please select another."));
        return;
    }
    ui.label->setPixmap(QPixmap::fromImage(image));
//! [1] //! [2]

    // load into shared memory
    QBuffer buffer;
    buffer.open(QBuffer::ReadWrite);
    QDataStream out(&buffer);
    out << image;
    int size = buffer.size();

    if (!sharedMemory.create(size)) {
        ui.label->setText(tr("Unable to create shared memory segment."));
        return;
    }
    sharedMemory.lock();
    char *to = (char*)sharedMemory.data();
    const char *from = buffer.data().data();
    memcpy(to, from, qMin(sharedMemory.size(), size));
    sharedMemory.unlock();
}
//! [2]

/*!
  This slot function is called in the second Dialog process, when the
  user presses the \tt {Load Image from Shared Memory} button. First,
  it attaches the process to the shared memory segment created by the
  first Dialog process. Then it locks the segment for exclusive
  access, copies the image data from the segment into a QBuffer, and
  streams the QBuffer into a QImage. Then it unlocks the shared memory
  segment, detaches from it, and finally displays the QImage in the
  Dialog.

  \sa loadFromFile()
 */
//! [3]
void Dialog::loadFromMemory()
{
    if (!sharedMemory.attach()) {
        ui.label->setText(tr("Unable to attach to shared memory segment.\n" \
                             "Load an image first."));
        return;
    }

    QBuffer buffer;
    QDataStream in(&buffer);
    QImage image;

    sharedMemory.lock();
    buffer.setData((char*)sharedMemory.constData(), sharedMemory.size());
    buffer.open(QBuffer::ReadOnly);
    in >> image;
    sharedMemory.unlock();

    sharedMemory.detach();
    ui.label->setPixmap(QPixmap::fromImage(image));
}
//! [3]

/*!
  This private function is called by the destructor to detach the
  process from its shared memory segment. When the last process
  detaches from a shared memory segment, the system releases the
  shared memory.
 */
void Dialog::detach()
{
    if (!sharedMemory.detach())
        ui.label->setText(tr("Unable to detach from shared memory."));
}

