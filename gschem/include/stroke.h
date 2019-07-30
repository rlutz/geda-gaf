/*
   libstroke - a stroke interface library
   Copyright (c) 1996,1997,1998,1999,2000  Mark F. Willey, ETLA Technical

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software, including the rights to use, copy,
   modify, merge, publish, and distribute copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   See the file "LICENSE" for a copy of the GNU GPL terms.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall
   name be used in advertising or otherwise to promote the sale, use or
   other dealings in this Software without prior written authorization
   from the author.

   Non-GPL commercial use licenses are available - contact copyright holder.

   Author: Mark F. Willey  --  willey@etla.net
   http://www.etla.net/
*/

/* largest number of points allowed to be sampled */
#define STROKE_MAX_POINTS 10000

/* number of sample points required to have a valid stroke */
#define STROKE_MIN_POINTS 50

/* maximum number of numbers in stroke */
#define STROKE_MAX_SEQUENCE 20

/* threshold of size of smaller axis needed for it to define its own
   bin size */
#define STROKE_SCALE_RATIO 4

/* minimum percentage of points in bin needed to add to sequence */
#define STROKE_BIN_COUNT_PERCENT 0.07

/* translate stroke to sequence */
int stroke_trans (char *sequence);

/* record point in stroke */
void stroke_record (int x, int y);

/* initialize stroke functions */
void stroke_init (void);
