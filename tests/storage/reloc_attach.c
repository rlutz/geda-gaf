/* Copyright (C) 2013-2020 Roland Lutz

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

#include <xornstorage.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define E_OK      ((xorn_error_t) -1)
#define E_NOTRANS xorn_error_revision_not_transient
#define E_NOEXIST xorn_error_object_doesnt_exist
#define E_IHIER   xorn_error_invalid_parent
#define E_PNOTSIB xorn_error_successor_not_sibling

#define _ NULL


static void check(xorn_revision_t rev, xorn_object_t ob,
		  xorn_object_t attach_to, xorn_object_t insert_before,
		  xorn_error_t expected_result,
		  xorn_object_t ob0, xorn_object_t ob1, xorn_object_t ob2)
{
	xorn_error_t err;
	xorn_object_t *objects;
	size_t count;

	err = E_OK;
	assert(xorn_relocate_object(rev, ob, attach_to, insert_before, &err)
		   == (expected_result == E_OK ? 0 : -1));
	assert(err == expected_result);

	assert(xorn_get_objects(rev, &objects, &count) == 0);
	assert(objects != NULL);
	assert(count == 3);
	assert(objects[0] == ob0);
	assert(objects[1] == ob1);
	assert(objects[2] == ob2);
	free(objects);
}

static void common_checks(
	xorn_revision_t rev,
	xorn_object_t N, xorn_object_t a, xorn_object_t b,
	xorn_object_t ob0, xorn_object_t ob1, xorn_object_t ob2,
	bool final, xorn_error_t hier_err)
{
	/* can't relocate NULL */

	check(rev, _, _, _, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, _, N, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, _, a, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, _, b, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);

	check(rev, _, N, _, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, N, N, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, N, a, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, N, b, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);

	check(rev, _, a, _, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, a, N, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, a, a, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, a, b, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);

	check(rev, _, b, _, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, b, N, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, b, a, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);
	check(rev, _, b, b, final ? E_NOTRANS : E_NOEXIST, ob0, ob1, ob2);

	/* can't embed N to itself */

	check(rev, N, N, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, N, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, N, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, N, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed N to a */

	check(rev, N, a, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, a, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, a, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, a, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed N to b */

	check(rev, N, b, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, b, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, b, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, N, b, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed a to itself */

	check(rev, a, a, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, a, a, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, a, a, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, a, a, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed a to b */

	check(rev, a, b, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, a, b, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, a, b, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, a, b, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed b to a */

	check(rev, b, a, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, b, a, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, b, a, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, b, a, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed b to itself */

	check(rev, b, b, _, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, b, b, N, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, b, b, a, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);
	check(rev, b, b, b, final ? E_NOTRANS : E_IHIER,   ob0, ob1, ob2);

	/* can't embed something to N before N */

	check(rev, a, N, N, final ? E_NOTRANS : hier_err,  ob0, ob1, ob2);
	check(rev, b, N, N, final ? E_NOTRANS : hier_err,  ob0, ob1, ob2);
}

static void check_delete_0(xorn_revision_t rev, xorn_object_t del)
{
	xorn_revision_t r;
	xorn_object_t *objects;
	size_t count;

	assert(r = xorn_new_revision(rev));
	assert(xorn_delete_object(r, del, NULL) == 0);

	assert(xorn_get_objects(r, &objects, &count) == 0);
	assert(count == 0);
	free(objects);

	xorn_free_revision(r);
}

static void check_delete_1(xorn_revision_t rev, xorn_object_t del,
			   xorn_object_t ob0)
{
	xorn_revision_t r;
	xorn_object_t *objects;
	size_t count;

	assert(r = xorn_new_revision(rev));
	assert(xorn_delete_object(r, del, NULL) == 0);

	assert(xorn_get_objects(r, &objects, &count) == 0);
	assert(objects != NULL);
	assert(count == 1);
	assert(objects[0] == ob0);
	free(objects);

	xorn_free_revision(r);
}

static void check_delete_2(xorn_revision_t rev, xorn_object_t del,
			   xorn_object_t ob0, xorn_object_t ob1)
{
	xorn_revision_t r;
	xorn_object_t *objects;
	size_t count;

	assert(r = xorn_new_revision(rev));
	assert(xorn_delete_object(r, del, NULL) == 0);

	assert(xorn_get_objects(r, &objects, &count) == 0);
	assert(objects != NULL);
	assert(count == 2);
	assert(objects[0] == ob0);
	assert(objects[1] == ob1);
	free(objects);

	xorn_free_revision(r);
}

static void check0(xorn_revision_t rev,
		   xorn_object_t N, xorn_object_t a, xorn_object_t b,
		   xorn_error_t hier_err)
{
	/* N() a b */
	common_checks(rev, N, a, b, N, a, b, false, hier_err);

	check(rev, N, _, _, E_OK,      a, b, N);
	check(rev, N, _, b, E_OK,      a, N, b);
	check(rev, N, _, a, E_OK,      N, a, b);

	check(rev, N, _, N, E_OK,      N, a, b);
	check(rev, a, _, a, E_OK,      N, a, b);
	check(rev, a, N, a, hier_err,  N, a, b);
	check(rev, b, _, b, E_OK,      N, a, b);
	check(rev, b, N, b, hier_err,  N, a, b);

	check_delete_2(rev, N, a, b);
}

static void check1(xorn_revision_t rev,
		   xorn_object_t N, xorn_object_t a, xorn_object_t b,
		   xorn_error_t hier_err)
{
	/* a N() b */
	common_checks(rev, N, a, b, a, N, b, false, hier_err);

	check(rev, N, _, _, E_OK,      a, b, N);
	check(rev, N, _, a, E_OK,      N, a, b);
	check(rev, N, _, b, E_OK,      a, N, b);

	check(rev, N, _, N, E_OK,      a, N, b);
	check(rev, a, _, a, E_OK,      a, N, b);
	check(rev, a, N, a, hier_err,  a, N, b);
	check(rev, b, _, b, E_OK,      a, N, b);
	check(rev, b, N, b, hier_err,  a, N, b);

	check_delete_2(rev, N, a, b);
}

static void check2(xorn_revision_t rev,
		   xorn_object_t N, xorn_object_t a, xorn_object_t b,
		   xorn_error_t hier_err)
{
	/* a b N() */
	common_checks(rev, N, a, b, a, b, N, false, hier_err);

	check(rev, N, _, b, E_OK,      a, N, b);
	check(rev, N, _, a, E_OK,      N, a, b);
	check(rev, N, _, _, E_OK,      a, b, N);

	check(rev, N, _, N, E_OK,      a, b, N);
	check(rev, a, _, a, E_OK,      a, b, N);
	check(rev, a, N, a, hier_err,  a, b, N);
	check(rev, b, _, b, E_OK,      a, b, N);
	check(rev, b, N, b, hier_err,  a, b, N);

	check_delete_2(rev, N, a, b);
}

static void check3(xorn_revision_t rev,
		   xorn_object_t N, xorn_object_t a, xorn_object_t b,
		   xorn_error_t hier_err)
{
	/* N(a) b */
	common_checks(rev, N, a, b, N, a, b, false, hier_err);

	check(rev, N, _, _, E_OK,      b, N, a);
	check(rev, N, _, a, E_PNOTSIB, b, N, a);
	check(rev, N, _, b, E_OK,      N, a, b);

	check(rev, N, _, N, E_OK,      N, a, b);
	check(rev, a, _, a, E_PNOTSIB, N, a, b);
	check(rev, a, N, a, E_OK,      N, a, b);
	check(rev, b, _, b, E_OK,      N, a, b);
	check(rev, b, N, b, hier_err,  N, a, b);

	check_delete_1(rev, N, b);
}

static void check4(xorn_revision_t rev,
		   xorn_object_t N, xorn_object_t a, xorn_object_t b,
		   xorn_error_t hier_err)
{
	/* a N(b) */
	common_checks(rev, N, a, b, a, N, b, false, hier_err);

	check(rev, N, _, a, E_OK,      N, b, a);
	check(rev, N, _, b, E_PNOTSIB, N, b, a);
	check(rev, N, _, _, E_OK,      a, N, b);

	check(rev, N, _, N, E_OK,      a, N, b);
	check(rev, a, _, a, E_OK,      a, N, b);
	check(rev, a, N, a, hier_err,  a, N, b);
	check(rev, b, _, b, E_PNOTSIB, a, N, b);
	check(rev, b, N, b, E_OK,      a, N, b);

	check_delete_1(rev, N, a);
}

static void check5(xorn_revision_t rev,
		   xorn_object_t N, xorn_object_t a, xorn_object_t b,
		   xorn_error_t hier_err)
{
	/* N(a b) */
	common_checks(rev, N, a, b, N, a, b, false, hier_err);

	check(rev, N, _, _, E_OK,      N, a, b);
	check(rev, N, _, a, E_PNOTSIB, N, a, b);
	check(rev, N, _, b, E_PNOTSIB, N, a, b);

	check(rev, N, _, N, E_OK,      N, a, b);
	check(rev, a, _, a, E_PNOTSIB, N, a, b);
	check(rev, a, N, a, E_OK,      N, a, b);
	check(rev, b, _, b, E_PNOTSIB, N, a, b);
	check(rev, b, N, b, E_OK,      N, a, b);

	check_delete_0(rev, N);
}

static void do_it(xorn_revision_t rev, xorn_object_t ob,
		  xorn_object_t attach_to, xorn_object_t insert_before,
		  int expected_result,
		  xorn_object_t ob0, xorn_object_t ob1, xorn_object_t ob2,
		  void (*fun)(xorn_revision_t rev, xorn_object_t N,
			      xorn_object_t a, xorn_object_t b,
			      xorn_error_t hier_err),
		  xorn_object_t fN, xorn_object_t fa, xorn_object_t fb,
		  xorn_error_t hier_err)
{
	check(rev, ob, attach_to, insert_before,
	      expected_result, ob0, ob1, ob2);
	(*fun)(rev, fN, fa, fb, hier_err);

	check(rev, ob, attach_to, insert_before,
	      expected_result, ob0, ob1, ob2);
	(*fun)(rev, fN, fa, fb, hier_err);
}

int main(void)
{
	xorn_revision_t rev;
	struct xornsch_net net_data;
	struct xornsch_text text_data;
	xorn_object_t N, a, b;

	xorn_revision_t rev1;
	struct xornsch_line line_data;
	struct xornsch_component component_data;

	assert(rev = xorn_new_revision(NULL));

	memset(&net_data, 0, sizeof net_data);
	assert(N = xornsch_add_net(rev, &net_data, NULL));

	memset(&text_data, 0, sizeof text_data);
	assert(a = xornsch_add_text(rev, &text_data, NULL));
	assert(b = xornsch_add_text(rev, &text_data, NULL));

	common_checks(rev, N, a, b, N, a, b, false, E_PNOTSIB);

	/* can move objects */

	do_it(rev, N, _, _, E_OK,      a, b, N, &check2, N, a, b, E_PNOTSIB);
	do_it(rev, N, _, a, E_OK,      N, a, b, &check0, N, a, b, E_PNOTSIB);
	do_it(rev, N, _, b, E_OK,      a, N, b, &check1, N, a, b, E_PNOTSIB);

	do_it(rev, a, _, _, E_OK,      N, b, a, &check0, N, b, a, E_PNOTSIB);
	do_it(rev, a, _, N, E_OK,      a, N, b, &check1, N, a, b, E_PNOTSIB);
	do_it(rev, a, _, b, E_OK,      N, a, b, &check0, N, a, b, E_PNOTSIB);

	do_it(rev, b, _, N, E_OK,      b, N, a, &check1, N, b, a, E_PNOTSIB);
	do_it(rev, b, _, a, E_OK,      N, b, a, &check0, N, b, a, E_PNOTSIB);
	do_it(rev, b, _, _, E_OK,      N, a, b, &check0, N, a, b, E_PNOTSIB);

	/* can embed a to N, but not before b */

	do_it(rev, a, N, _, E_OK,      N, a, b, &check3, N, a, b, E_PNOTSIB);
	do_it(rev, a, N, b, E_PNOTSIB, N, a, b, &check3, N, a, b, E_PNOTSIB);

	do_it(rev, b, _, N, E_OK,      b, N, a, &check4, N, b, a, E_PNOTSIB);
	do_it(rev, b, _, a, E_PNOTSIB, b, N, a, &check4, N, b, a, E_PNOTSIB);
	do_it(rev, b, _, _, E_OK,      N, a, b, &check3, N, a, b, E_PNOTSIB);

	do_it(rev, a, _, b, E_OK,      N, a, b, &check0, N, a, b, E_PNOTSIB);

	/* can embed b to N, but not before a */

	do_it(rev, b, N, _, E_OK,      N, b, a, &check3, N, b, a, E_PNOTSIB);
	do_it(rev, b, N, a, E_PNOTSIB, N, b, a, &check3, N, b, a, E_PNOTSIB);

	do_it(rev, a, _, N, E_OK,      a, N, b, &check4, N, a, b, E_PNOTSIB);
	do_it(rev, a, _, b, E_PNOTSIB, a, N, b, &check4, N, a, b, E_PNOTSIB);
	do_it(rev, a, _, _, E_OK,      N, b, a, &check3, N, b, a, E_PNOTSIB);

	do_it(rev, b, _, _, E_OK,      N, a, b, &check0, N, a, b, E_PNOTSIB);

	/* can embed both */

	do_it(rev, a, N, _, E_OK,      N, a, b, &check3, N, a, b, E_PNOTSIB);
	do_it(rev, b, N, _, E_OK,      N, a, b, &check5, N, a, b, E_PNOTSIB);

	do_it(rev, a, N, _, E_OK,      N, b, a, &check5, N, b, a, E_PNOTSIB);
	do_it(rev, a, N, b, E_OK,      N, a, b, &check5, N, a, b, E_PNOTSIB);
	do_it(rev, b, N, a, E_OK,      N, b, a, &check5, N, b, a, E_PNOTSIB);
	do_it(rev, b, N, _, E_OK,      N, a, b, &check5, N, a, b, E_PNOTSIB);

	do_it(rev, a, _, _, E_OK,      N, b, a, &check3, N, b, a, E_PNOTSIB);
	do_it(rev, b, _, _, E_OK,      N, a, b, &check0, N, a, b, E_PNOTSIB);

	xorn_finalize_revision(rev);

	common_checks(rev, N, a, b, N, a, b, true, E_PNOTSIB);

	check(rev, N, _, _, E_NOTRANS, N, a, b);
	check(rev, N, _, a, E_NOTRANS, N, a, b);
	check(rev, N, _, b, E_NOTRANS, N, a, b);

	check(rev, a, _, _, E_NOTRANS, N, a, b);
	check(rev, a, _, N, E_NOTRANS, N, a, b);
	check(rev, a, _, b, E_NOTRANS, N, a, b);
	check(rev, a, N, _, E_NOTRANS, N, a, b);

	check(rev, b, _, _, E_NOTRANS, N, a, b);
	check(rev, b, _, N, E_NOTRANS, N, a, b);
	check(rev, b, _, a, E_NOTRANS, N, a, b);
	check(rev, b, N, _, E_NOTRANS, N, a, b);

	assert(rev1 = xorn_new_revision(rev));

	/* can't attach text to line */

	memset(&line_data, 0, sizeof line_data);
	assert(xornsch_set_line_data(rev1, N, &line_data, NULL) == 0);
	do_it(rev1, a, N, _, E_IHIER,   N, a, b, &check0, N, a, b, E_IHIER);

	/* can attach text to component */

	memset(&component_data, 0, sizeof component_data);
	assert(xornsch_set_component_data(
		       rev1, N, &component_data, NULL) == 0);
	do_it(rev1, a, N, _, E_OK,      N, a, b, &check3, N, a, b, E_PNOTSIB);

	/* can't attach text to deleted object */

	xorn_error_t err;
	err = E_OK;
	assert(xorn_delete_object(rev1, N, &err) == 0);
	assert(err == E_OK);
	err = E_OK;
	assert(xorn_delete_object(rev1, N, &err) == -1);
	assert(err == xorn_error_object_doesnt_exist);
	err = E_OK;
	assert(xorn_relocate_object(rev1, b, N, NULL, &err) == -1);
	assert(err == xorn_error_parent_doesnt_exist);

	xorn_object_t *objects;
	size_t count;
	assert(xorn_get_objects(rev1, &objects, &count) == 0);
	assert(objects != NULL);
	assert(count == 1);
	assert(objects[0] == b);
	free(objects);

	xorn_free_revision(rev1);
	xorn_free_revision(rev);
	return 0;
}
