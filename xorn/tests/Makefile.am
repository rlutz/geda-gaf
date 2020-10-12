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

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = -Wall -W -Werror -Wno-empty-body -Wno-parentheses
LDADD = $(top_builddir)/src/storage/libxornstorage.la

TEST_EXTENSIONS = .py

AM_TESTS_ENVIRONMENT = \
	PYTHONPATH='$(top_builddir)/built-packages'; \
	export PYTHONPATH;
PY_LOG_COMPILER = $(PYTHON)

check_PROGRAMS = \
	storage/snippets/example \
	storage/snippets/functions \
	storage/snippets/motivation \
	storage/copy_attached \
	storage/copy_object \
	storage/copy_objects \
	storage/delete_sel \
	storage/exist \
	storage/get_attribute \
	storage/get_location \
	storage/get_obdata \
	storage/get_objects \
	storage/get_obtype \
	storage/invalid_obtype \
	storage/is_selected \
	storage/multiple_assignments \
	storage/normalize \
	storage/null \
	storage/pointer \
	storage/query_attached \
	storage/reloc_attach \
	storage/reloc_order \
	storage/select_by_attribute \
	storage/selection \
	storage/set_attribute \
	storage/set_obdata \
	storage/stat \
	storage/string \
	storage/transient \
	storage/validate

pythontests = \
	cpython/snippets/storage_funcs.py \
	cpython/snippets/guile.py \
	cpython/storage/copy_attached.py \
	cpython/storage/copy_object.py \
	cpython/storage/copy_objects.py \
	cpython/storage/data_ctors.py \
	cpython/storage/delete_sel.py \
	cpython/storage/exist.py \
	cpython/storage/get_loc_attach.py \
	cpython/storage/get_loc_order.py \
	cpython/storage/get_obdata.py \
	cpython/storage/get_objects.py \
	cpython/storage/get_obtype.py \
	cpython/storage/is_selected.py \
	cpython/storage/module.py \
	cpython/storage/normalize.py \
	cpython/storage/ob_equality.py \
	cpython/storage/pointer.py \
	cpython/storage/query_attached.py \
	cpython/storage/reloc_attach.py \
	cpython/storage/reloc_order.py \
	cpython/storage/selection.py \
	cpython/storage/set_obdata.py \
	cpython/storage/stat.py \
	cpython/storage/string.py \
	cpython/storage/transient.py \
	cpython/storage/validate.py \
	cpython/guile.py \
	python/snippets/xml_writer.py \
	python/base64.py \
	python/fixednum.py \
	python/hybridnum.py \
	python/proxy.py \
	python/xml_writer.py \
	gaf/attrib.py \
	gaf/complex.py \
	gaf/parse_attrib.py \
	gaf/pixmap.py \
	gaf/plain_transform.py \
	gaf/plainread_log.py \
	gaf/ripperdir.py \
	gaf/unhide_attrib.py \
	gaf/xmlread.py \
	gaf/xmlread_log.py \
	gaf/xmlwrite.py

testdata = \
	gaf/pixmap.data \
	gaf/plain_transform.sch \
	gaf/plain_transform.sch.xml \
	gaf/ripperdir.sch \
	gaf/xmlread.sch \
	gaf/xmlread.sch.xml \
	gaf/xmlwrite.sch.xml

MOSTLYCLEANFILES = \
	gaf/xmlread.out \
	gaf/xmlwrite.out

TESTS = $(check_PROGRAMS) $(pythontests)

EXTRA_DIST = \
	storage/Setup.h \
	cpython/storage/Setup.py \
	$(pythontests) $(testdata)
