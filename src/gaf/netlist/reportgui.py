# gaf.netlist - gEDA Netlist Extraction and Generation
# Copyright (C) 1998-2010 Ales Hvezda
# Copyright (C) 1998-2010 gEDA Contributors (see ChangeLog for details)
# Copyright (C) 2013-2020 Roland Lutz
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

## \namespace gaf.netlist.reportgui
## Report errors, warnings, and exceptions in a GUI dialog.

import collections, cStringIO, gtk, pango, re, traceback
from gettext import gettext as _
import xorn.config

COLUMN_TYPE, \
COLUMN_FILENAME, \
COLUMN_WHAT, \
COLUMN_MESSAGE = xrange(4)

Issue = collections.namedtuple('Issue', [
    'stock_id', 'filename', 'what', 'message'])


def build_window(message_type, stock_id, message,
                 widget, top_label = None, bottom_label = None):
    image = gtk.Image()
    image.set_from_stock(stock_id, gtk.ICON_SIZE_LARGE_TOOLBAR)
    image.show()

    label = gtk.Label()
    label.set_markup('<b>%s</b>' % message)
    label.set_alignment(0., .5)
    label.show()

    info_bar = gtk.InfoBar()
    info_bar.set_message_type(message_type)

    content_area = info_bar.get_content_area()
    content_area.pack_start(image, False, False, 0)
    content_area.pack_start(label, True, True, 0)

    info_bar.show()

    scrolled_window = gtk.ScrolledWindow()
    scrolled_window.add(widget)
    scrolled_window.set_shadow_type(gtk.SHADOW_IN)
    scrolled_window.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
    scrolled_window.show()

    close_button = gtk.Button(gtk.STOCK_CLOSE)
    close_button.set_use_stock(True)
    close_button.set_can_default(True)
    close_button.connect('clicked', gtk.main_quit)
    close_button.show()

    button_hbox = gtk.HBox(False, 4)
    button_hbox.pack_end(close_button, False, False, 0)
    button_hbox.show()

    dialog_vbox = gtk.VBox(False, 8)
    dialog_vbox.set_border_width(8)
    if top_label is not None:
        dialog_vbox.pack_start(top_label, False, False, 0)
    dialog_vbox.pack_start(scrolled_window, True, True, 0)
    if bottom_label is not None:
        dialog_vbox.pack_start(bottom_label, False, False, 0)
    dialog_vbox.pack_start(button_hbox, False, False, 0)
    dialog_vbox.show()

    top_vbox = gtk.VBox(False, 0)
    top_vbox.pack_start(info_bar, False, False, 0)
    top_vbox.pack_start(dialog_vbox, True, True, 0)
    top_vbox.show()

    def key_press_event(window, event):
        if event.keyval == gtk.keysyms.Escape:
            gtk.main_quit()

    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    window.set_title('gnetlist')
    window.set_default_size(600, 400)
    window.set_position(gtk.WIN_POS_CENTER)
    window.connect('key-press-event', key_press_event)
    window.connect('destroy', gtk.main_quit)
    window.add(top_vbox)
    window.show()


def report_messages(exit_code, log):
    lines = log.split('\n')
    if lines and lines[-1] == '':
        del lines[-1]

    loading_re = re.compile('Loading schematic \[.*\]$')
    message1_re = re.compile('(.*): (warning|error): (.*)')
    message2_re = re.compile('(.*):(.*): (warning|error): (.*)')

    issues = []

    for line in lines:
        match = message2_re.match(line)
        if match is not None:
            if match.group(3) == 'warning':
                stock_id = gtk.STOCK_DIALOG_WARNING
            else:
                stock_id = gtk.STOCK_DIALOG_ERROR
            issues.append(Issue(stock_id = stock_id,
                                filename = match.group(1),
                                what = match.group(2),
                                message = match.group(4)))
            continue

        match = message1_re.match(line)
        if match is not None:
            if match.group(2) == 'warning':
                stock_id = gtk.STOCK_DIALOG_WARNING
            else:
                stock_id = gtk.STOCK_DIALOG_ERROR
            issues.append(Issue(stock_id = stock_id,
                                filename = match.group(1),
                                what = '',
                                message = match.group(3)))
            continue

        match = loading_re.match(line)
        if match is not None:
            continue

        issues.append(Issue(stock_id = '',
                            filename = '',
                            what = '',
                            message = line))

    if exit_code == 0 and not issues:
        return

    store = gtk.ListStore(str, str, str, str)
    for issue in issues:
        it = store.append()
        store.set(it, COLUMN_TYPE,     issue.stock_id,
                      COLUMN_FILENAME, issue.filename,
                      COLUMN_WHAT,     issue.what,
                      COLUMN_MESSAGE,  issue.message)

    tree_view = gtk.TreeView(store)
    tree_view.show()

    # icon column
    column = gtk.TreeViewColumn()
    tree_view.append_column(column)

    renderer = gtk.CellRendererPixbuf()
    column.pack_start(renderer, True)
    column.add_attribute(renderer, "stock-id", COLUMN_TYPE)

    # filename column
    column = gtk.TreeViewColumn()
    column.set_resizable(True)
    column.set_title(_("Filename"))
    tree_view.append_column(column)

    renderer = gtk.CellRendererText()
    column.pack_start(renderer, True)
    column.add_attribute(renderer, "text", COLUMN_FILENAME)

    # what column
    column = gtk.TreeViewColumn()
    column.set_resizable(True)
    column.set_title(_("Refdes"))
    tree_view.append_column(column)

    renderer = gtk.CellRendererText()
    column.pack_start(renderer, True)
    column.add_attribute(renderer, "text", COLUMN_WHAT)

    # text column
    column = gtk.TreeViewColumn()
    column.set_resizable(True)
    column.set_title(_("Text"))
    tree_view.append_column(column)

    renderer = gtk.CellRendererText()
    column.pack_start(renderer, True)
    column.add_attribute(renderer, "text", COLUMN_MESSAGE)

    if exit_code == 0:
        message_type = gtk.MESSAGE_INFO
        stock_id = gtk.STOCK_DIALOG_INFO
        message = _("There may be some issues with the netlist.")
    else:
        message_type = gtk.MESSAGE_WARNING
        stock_id = gtk.STOCK_DIALOG_WARNING
        message = _("There were errors while generating the netlist.")

    build_window(message_type, stock_id, message, tree_view)

    gtk.main()
    return


def report_crash():
    s = cStringIO.StringIO()
    traceback.print_exc(file = s)

    text_view = gtk.TextView()
    text_view.set_editable(False)
    text_view.set_cursor_visible(False)
    text_view.get_buffer().set_text(s.getvalue())
    text_view.show()

    top_label = gtk.Label(_("Exception details:"))
    top_label.set_alignment(0., .5)
    top_label.show()

    bottom_label = gtk.Label()
    bottom_label.set_markup(_("You can help improving gEDA by reporting this "
                              "to <a href=\"%s\">%s</a> or\nsending an e-mail "
                              "to <a href=\"mailto:%s\">%s</a>.")
                            % ('https://bugs.launchpad.net/geda',
                               'https://bugs.launchpad.net/geda',
                               xorn.config.PACKAGE_BUGREPORT,
                               xorn.config.PACKAGE_BUGREPORT))
    bottom_label.set_alignment(0., .5)
    bottom_label.show()

    build_window(gtk.MESSAGE_ERROR,
                 gtk.STOCK_DIALOG_ERROR,
                 _("The netlister encountered an internal error."),
                 text_view, top_label, bottom_label)

    gtk.main()
    return
