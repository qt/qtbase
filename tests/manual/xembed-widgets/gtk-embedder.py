#!/usr/bin/python
#############################################################################
##
## Copyright (C) 2013 Canonical Ltd.
## Contact: http://www.qt-project.org/legal
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and Digia.  For licensing terms and
## conditions see http://qt.digia.com/licensing.  For further information
## use the contact form at http://qt.digia.com/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 2.1 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU Lesser General Public License version 2.1 requirements
## will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
##
## In addition, as a special exception, Digia gives you certain additional
## rights.  These rights are described in the Digia Qt LGPL Exception
## version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3.0 as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL included in the
## packaging of this file.  Please review the following information to
## ensure the GNU General Public License version 3.0 requirements will be
## met: http://www.gnu.org/copyleft/gpl.html.
##
##
## $QT_END_LICENSE$
##
#############################################################################

from gi.repository import Gtk
from subprocess import Popen

window = Gtk.Window()

box = Gtk.VBox(False, 0)
window.add(box)

child = None
def on_button_clicked(button, socket):
    global child
    child = Popen(['./lineedits', str(socket.get_id())])

button = Gtk.Button("Press me to embed a Qt client")
box.pack_start(button, False, False, 0)

entry = Gtk.Entry()
box.pack_start(entry, False, False, 0)

socket = Gtk.Socket()
socket.set_size_request(200, 200)
box.add(socket)

button.connect("clicked", on_button_clicked, socket)
window.connect("destroy", Gtk.main_quit)

def plugged_event(widget):
    print("A window was embedded!")

socket.connect("plug-added", plugged_event)

window.show_all()
Gtk.main()
if child:
    child.terminate()
