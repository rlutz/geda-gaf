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

import os, sys
import gaf.read
import gaf.write

OUTPUT_FILE = 'gaf/xmlread.out'

rev = gaf.read.read(sys.argv[0][:-3] + '.sch.xml',
                    gaf.fileformat.FORMAT_SCH_XML)

gaf.write.write(rev, OUTPUT_FILE, gaf.fileformat.FORMAT_SCH,
                write_kwds = { 'backup': False, 'fsync': False })

os.execvp('diff', ['diff', '-u', sys.argv[0][:-3] + '.sch', OUTPUT_FILE])
