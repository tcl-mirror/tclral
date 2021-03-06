/*
This software is copyrighted 2005 - 2011 by G. Andrew Mangogna.  The following
terms apply to all files associated with the software unless explicitly
disclaimed in individual files.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors and
need not follow the licensing terms described here, provided that the
new terms are clearly indicated on the first page of each file where
they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES
THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
OR MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the
U.S. government, the Government shall have only "Restricted Rights"
in the software and related documentation as defined in the Federal
Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
are acquiring the software on behalf of the Department of Defense,
the software shall be classified as "Commercial Computer Software"
and the Government shall have only "Restricted Rights" as defined in
Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing,
the authors grant the U.S. Government and others acting in its behalf
permission to use and distribute the software in accordance with the
terms specified in this license.
*/
/*
 *++
MODULE:

ABSTRACT:

$RCSfile: ral_vector.c,v $
$Revision: 1.16 $
$Date: 2011/06/05 18:01:10 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_vector.h"
#include "tcl.h"
#include <stdlib.h>
#include <string.h>

/*
MACRO DEFINITIONS
*/
/*
 * Holding my nose so tightly that I can hardly breathe.
 */
#ifdef _MSC_VER
#define snprintf    _snprintf
#endif /* _MSC_VER */

/*
TYPE DEFINITIONS
*/

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static int int_ind_compare(const void *, const void *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/

/*
FUNCTION DEFINITIONS
*/

/*
 * Allocate a new vector that is "size" entries and fill those entries
 * with "value".
 */
Ral_IntVector
Ral_IntVectorNew(
    int size,
    Ral_IntVectorValueType value)
{
    Ral_IntVector v ;

    v = (Ral_IntVector)ckalloc(sizeof(*v)) ;
    v->start = (Ral_IntVectorIter)ckalloc(
	size * sizeof(Ral_IntVectorValueType)) ;
    v->finish = v->endStorage = v->start + size ;
    Ral_IntVectorFill(v, value) ;

    return v ;
}

/*
 * Allocate a new vector that is "size" entries, but the vector contains
 * no values.
 */
Ral_IntVector
Ral_IntVectorNewEmpty(
    int size)
{
    Ral_IntVector v ;

    v = (Ral_IntVector)ckalloc(sizeof(*v)) ;
    v->finish = v->start = (Ral_IntVectorIter)ckalloc(
	size * sizeof(Ral_IntVectorValueType)) ;
    v->endStorage = v->start + size ;

    return v ;
}

Ral_IntVector
Ral_IntVectorDup(
    Ral_IntVector v)
{
    int size = Ral_IntVectorSize(v) ;
    int bytes = size * sizeof(Ral_IntVectorValueType) ;
    Ral_IntVector dupV ;

    dupV = (Ral_IntVector)ckalloc(sizeof(*dupV)) ;
    dupV->start = (Ral_IntVectorIter)ckalloc(bytes) ;
    dupV->finish = dupV->endStorage = dupV->start + size ;
    memcpy(dupV->start, v->start, bytes) ;
    return dupV ;
}

void
Ral_IntVectorDelete(
    Ral_IntVector v)
{
    if (v) {
	ckfree((char *)v->start) ;
	ckfree((char *)v) ;
    }
}

void
Ral_IntVectorReserve(
    Ral_IntVector v,
    int size)
{
    if (Ral_IntVectorCapacity(v) < size) {
	int oldSize = Ral_IntVectorSize(v) ;
	v->start = (Ral_IntVectorIter)ckrealloc((char *)v->start, 
	    size * sizeof(Ral_IntVectorValueType)) ;
	v->finish = v->start + oldSize ;
	v->endStorage = v->start + size ;
    }
}

void
Ral_IntVectorFill(
    Ral_IntVector v,
    Ral_IntVectorValueType value)
{
    Ral_IntVectorIter i = v->start ;

    for (i = v->start ; i < v->finish ; ++i) {
	*i = value ;
    }
}

/*
 * Fill a vector with consecutive integers begining at "start".
 */
void
Ral_IntVectorFillConsecutive(
    Ral_IntVector v,
    Ral_IntVectorValueType start)
{
    Ral_IntVectorIter i = v->start ;

    for (i = v->start ; i < v->finish ; ++i) {
	*i = start++ ;
    }
}

Ral_IntVectorValueType
Ral_IntVectorFetch(
    Ral_IntVector v,
    int at)
{
    if (v->start + at >= v->finish) {
	Tcl_Panic("Ral_IntVectorFetch: out of bounds access at offset \"%d\"",
	    at) ;
    }
    return *(v->start + at) ;
}

void
Ral_IntVectorStore(
    Ral_IntVector v,
    int at,
    Ral_IntVectorValueType value)
{
    if (v->start + at >= v->finish) {
	Tcl_Panic("Ral_IntVectorStore: out of bounds access at offset \"%d\"",
	    at) ;
    }
    *(v->start + at) = value ;
}

Ral_IntVectorValueType
Ral_IntVectorFront(
    Ral_IntVector v)
{
    if (Ral_IntVectorEmpty(v)) {
	Tcl_Panic("Ral_IntVectorFront: access to empty vector") ;
    }
    return *v->start ;
}

Ral_IntVectorValueType
Ral_IntVectorBack(
    Ral_IntVector v)
{
    if (Ral_IntVectorEmpty(v)) {
	Tcl_Panic("Ral_IntVectorFront: access to empty vector") ;
    }
    return *(v->finish - 1) ;
}

void
Ral_IntVectorPushBack(
    Ral_IntVector v,
    Ral_IntVectorValueType value)
{
    if (v->finish >= v->endStorage) {
	int oldCapacity = Ral_IntVectorCapacity(v) ;
	/*
	 * Increase the capacity by half again. +1 to make sure
	 * we allocate at least one slot if this is the first time
	 * we are pushing onto an empty vector.
	 */
	Ral_IntVectorReserve(v, oldCapacity + oldCapacity / 2 + 1) ;
    }
    *v->finish++ = value ;
}

void
Ral_IntVectorPopBack(
    Ral_IntVector v)
{
    if (Ral_IntVectorEmpty(v)) {
	Tcl_Panic("Ral_IntVectorPopBack: access to empty vector") ;
    }
    --v->finish ;
}

Ral_IntVectorIter
Ral_IntVectorInsert(
    Ral_IntVector v,
    Ral_IntVectorIter pos,
    int n,
    Ral_IntVectorValueType value)
{
    int size ;
    int capacity ;
    int offset ;

    if (pos > v->finish) {
	Tcl_Panic("Ral_IntVectorInsert: out of bounds access at offset \"%d\"",
	    (int)(pos - v->start)) ;
    }

    size = Ral_IntVectorSize(v) ;
    capacity = Ral_IntVectorCapacity(v) ;
    offset = pos - v->start ;
    if (capacity < size + n) {
	/*
	 * We must recompute the insert position since "Ral_IntVectorReserve"
	 * may reallocate the memory.
	 */
	Ral_IntVectorReserve(v, size + n) ;
	pos = v->start + offset ;
    }
    /*
     * We only have to move the vector data if we are not inserting at the end.
     */
    if (pos < v->finish) {
	memmove(pos + n, pos, (v->finish - pos) * sizeof(*pos)) ;
    }
    v->finish += n ;
    while (n--) {
	*pos++ = value ;
    }

    return v->start + offset ;
}

Ral_IntVectorIter
Ral_IntVectorErase(
    Ral_IntVector v,
    Ral_IntVectorIter first,
    Ral_IntVectorIter last)
{
    if (first > v->finish) {
	Tcl_Panic("Ral_IntVectorErase: out of bounds access at offset \"%d\"",
	    (int)(first - v->start)) ;
    }
    if (last > v->finish) {
	Tcl_Panic("Ral_IntVectorErase: out of bounds access at offset \"%d\"",
	    (int)(last - v->start)) ;
    }
    memmove(first, last, (v->finish - last) * sizeof(*first)) ;
    v->finish -= last - first ;

    return first ;
}

void
Ral_IntVectorExchange(
    Ral_IntVector v,
    int a,
    int b)
{
    int tmp ;
    if (v->start + a >= v->finish) {
	Tcl_Panic("Ral_IntVectorFetch: out of bounds access at offset \"%d\"",
	    a) ;
    }
    if (v->start + b >= v->finish) {
	Tcl_Panic("Ral_IntVectorFetch: out of bounds access at offset \"%d\"",
	    b) ;
    }

    tmp = v->start[a] ;
    v->start[a] = v->start[b] ;
    v->start[b] = tmp ;
}

int
Ral_IntVectorSetAdd(
    Ral_IntVector v,
    Ral_IntVectorValueType value)
{
    return Ral_IntVectorFind(v, value) == v->finish ?
	(Ral_IntVectorPushBack(v, value), 1) : 0 ;
}

/*
 * Returns a vector that is the complement of the set of
 * integers contained in the input vector. The input is assumed to
 * be a subset of the integer range 0 - "max". This function returns
 * a vector that contains all the integers in the range 0 - "max"
 * except those contained in the input vector. In other words the
 * returned set is {0 - max} - "v".
 */
Ral_IntVector
Ral_IntVectorSetComplement(
    Ral_IntVector v,
    int max)
{
    Ral_IntVector cmpl = Ral_IntVectorNew(max - Ral_IntVectorSize(v), -1) ;
    Ral_IntVectorIter iter ;
    Ral_IntVectorIter cmplIter = Ral_IntVectorBegin(cmpl) ;
    int value = 0 ;
    /*
     * Sort the input so that we can fill in the ranges properly.
     */
    Ral_IntVectorSort(v) ;
    /*
     * Iterate through the input.
     */
    for (iter = Ral_IntVectorBegin(v) ; iter != Ral_IntVectorEnd(v) ; ++iter) {
	/*
	 * Insert all the values less than value found in the input.
	 */
	while (value < *iter) {
	    *cmplIter++ = value++ ;
	}
	/*
	 * Skip the input value.
	 */
	value = *iter + 1 ;
    }
    /*
     * Insert all the values at the end of the range.
     */
    while (value < max) {
	*cmplIter++ = value++ ;
    }

    return cmpl ;
}

/*
 * Return a vector "size" large. Each slot contains either 0 if the
 * corresponding index does not appear in "v" and 1 if it does.
 * It follows that the maximum value contained in "v" must be less than "size".
 */
Ral_IntVector
Ral_IntVectorBooleanMap(
    Ral_IntVector v,
    int size)
{
    Ral_IntVectorIter end = Ral_IntVectorEnd(v) ;
    Ral_IntVectorIter iter ;
    Ral_IntVector mapV = Ral_IntVectorNew(size, 0) ;

    for (iter = Ral_IntVectorBegin(v) ; iter != end ; ++iter) {
	Ral_IntVectorStore(mapV, *iter, 1) ;
    }

    return mapV ;
}

void
Ral_IntVectorSort(
    Ral_IntVector v)
{
    qsort(v->start, v->finish - v->start, sizeof(Ral_IntVectorValueType),
	int_ind_compare) ;
}

Ral_IntVectorIter
Ral_IntVectorFind(
    Ral_IntVector v,
    Ral_IntVectorValueType value)
{
    Ral_IntVectorIter i ;

    for (i = v->start ; i < v->finish ; ++i) {
	if (*i == value) {
	    return i ;
	}
    }
    return v->finish ;
}

int
Ral_IntVectorIndexOf(
    Ral_IntVector v,
    Ral_IntVectorValueType value)
{
    Ral_IntVectorIter i ;

    for (i = v->start ; i < v->finish ; ++i) {
	if (*i == value) {
	    return i - v->start ;
	}
    }
    return -1 ;
}

int
Ral_IntVectorOffsetOf(
    Ral_IntVector v,
    Ral_IntVectorIter iter)
{
    if (iter < v->start || iter >= v->finish) {
	Tcl_Panic("Ral_IntVectorOffsetOf: out of bounds access") ;
    }
    return iter - v->start ;
}

Ral_IntVector
Ral_IntVectorIntersect(
    Ral_IntVector v1,
    Ral_IntVector v2)
{
    Ral_IntVectorIter v1Iter ;
    Ral_IntVectorIter v1End = Ral_IntVectorEnd(v1) ;
    Ral_IntVectorIter v2End = Ral_IntVectorEnd(v2) ;
    Ral_IntVector intersect ;

    intersect = Ral_IntVectorNewEmpty(1) ;
    for (v1Iter = Ral_IntVectorBegin(v1) ; v1Iter != v1End ; ++v1Iter) {
	Ral_IntVectorValueType v1Value = *v1Iter ;
	if (Ral_IntVectorFind(v2, v1Value) != v2End) {
	    Ral_IntVectorPushBack(intersect, v1Value) ;
	}
    }

    return intersect ;
}

Ral_IntVector
Ral_IntVectorMinus(
    Ral_IntVector v1,
    Ral_IntVector v2)
{
    Ral_IntVectorIter v1Iter ;
    Ral_IntVectorIter v1End = Ral_IntVectorEnd(v1) ;
    Ral_IntVectorIter v2End = Ral_IntVectorEnd(v2) ;
    Ral_IntVector diff ;

    diff = Ral_IntVectorNewEmpty(1) ;
    for (v1Iter = Ral_IntVectorBegin(v1) ; v1Iter != v1End ; ++v1Iter) {
	Ral_IntVectorValueType v1Value = *v1Iter ;
	if (Ral_IntVectorFind(v2, v1Value) == v2End) {
	    Ral_IntVectorPushBack(diff, v1Value) ;
	}
    }

    return diff ;
}

int
Ral_IntVectorEqual(
    Ral_IntVector v1,
    Ral_IntVector v2)
{
    int equals = 0 ;
    int v1Size = Ral_IntVectorSize(v1) ;
    if (v1Size == Ral_IntVectorSize(v2) &&
	memcmp(v1->start, v2->start, v1Size * sizeof(Ral_IntVectorValueType))
	    == 0) {
	equals = 1 ;
    }
    return equals ;
}

/*
 * Determine if "v1" is a subset (possibly improper) of "v2".
 */
int
Ral_IntVectorSubsetOf(
    Ral_IntVector v1,
    Ral_IntVector v2)
{
    int found = 0 ;
    Ral_IntVectorIter end1 = Ral_IntVectorEnd(v1) ;
    Ral_IntVectorIter end2 = Ral_IntVectorEnd(v2) ;
    Ral_IntVectorIter iter1 ;

    for (iter1 = Ral_IntVectorBegin(v1) ; iter1 != end1 ; ++iter1) {
	Ral_IntVectorIter iter2 = Ral_IntVectorFind(v2, *iter1) ;
	found += iter2 != end2 ;
    }

    return found == Ral_IntVectorSize(v1) ;
}

/*
 * Return a boolean to indicate if any values in v2 are found in v1,
 * i.e. return true if the set intersection is not empty.
 */
int
Ral_IntVectorContainsAny(
    Ral_IntVector v1,
    Ral_IntVector v2)
{
    Ral_IntVectorIter end1 = Ral_IntVectorEnd(v1) ;
    Ral_IntVectorIter end2 = Ral_IntVectorEnd(v2) ;
    Ral_IntVectorIter iter2 ;

    for (iter2 = Ral_IntVectorBegin(v2) ; iter2 != end2 ; ++iter2) {
	if (Ral_IntVectorFind(v1, *iter2) != end1) {
	    return 1 ;
	}
    }
    return 0 ;
}

Ral_IntVectorIter
Ral_IntVectorCopy(
    Ral_IntVector src,
    Ral_IntVectorIter first,
    Ral_IntVectorIter last,
    Ral_IntVector dst,
    Ral_IntVectorIter pos)
{
    int n ;

    if (first > src->finish) {
	Tcl_Panic(
	    "Ral_IntVectorCopy: out of bounds access at source offset \"%d\"",
	    (int)(first - src->start)) ;
    }
    if (last > src->finish) {
	Tcl_Panic(
	    "Ral_IntVectorCopy: out of bounds access at source offset \"%d\"",
	    (int)(last - src->start)) ;
    }
    if (pos > dst->finish) {
	Tcl_Panic(
	    "Ral_IntVectorCopy: out of bounds access at dest offset \"%d\"",
	    (int)(pos - dst->start)) ;
    }

    n = last - first ;
    pos = Ral_IntVectorInsert(dst, pos, n, 0) ;
    memcpy(pos, first, n * sizeof(*pos)) ;

    return pos ;
}

const char *
Ral_IntVectorPrint(
    Ral_IntVector v,
    Ral_IntVectorIter first)
{
    static char buf[BUFSIZ] ;

    char *s = buf ;
    int n ;

    for (n = first - v->start ; first != v->finish ; ++first) {
	s += snprintf(s, sizeof(buf) - (s - buf), "%d: %d\n", n++, *first) ;
    }

    if (s != buf) {
	--s ;
    }
    *s++ = '\0' ;

    return buf ;
}

/*
 * Vectors of pointers.
 */

Ral_PtrVector
Ral_PtrVectorNew(
    int size)
{
    Ral_PtrVector v ;

    v = (Ral_PtrVector)ckalloc(sizeof(*v)) ;
    v->start = v->finish = (Ral_PtrVectorIter)ckalloc(
	size * sizeof(Ral_PtrVectorValueType)) ;
    memset(v->start, 0, size * sizeof(Ral_PtrVectorValueType)) ;
    v->endStorage = v->start + size ;

    return v ;
}

Ral_PtrVector
Ral_PtrVectorDup(
    Ral_PtrVector v)
{
    int size = Ral_PtrVectorSize(v) ;
    int bytes = size * sizeof(Ral_PtrVectorValueType) ;
    Ral_PtrVector dupV ;

    dupV = (Ral_PtrVector)ckalloc(sizeof(*dupV)) ;
    dupV->start = (Ral_PtrVectorIter)ckalloc(bytes) ;
    dupV->finish = dupV->endStorage = dupV->start + size ;
    memcpy(dupV->start, v->start, bytes) ;
    return dupV ;
}

void
Ral_PtrVectorDelete(
    Ral_PtrVector v)
{
    if (v) {
	ckfree((char *)v->start) ;
	ckfree((char *)v) ;
    }
}

void
Ral_PtrVectorReserve(
    Ral_PtrVector v,
    int size)
{
    if (Ral_PtrVectorCapacity(v) < size) {
	int oldSize = Ral_PtrVectorSize(v) ;
	v->start = (Ral_PtrVectorIter)ckrealloc((char *)v->start, 
	    size * sizeof(Ral_PtrVectorValueType)) ;
	v->finish = v->start + oldSize ;
	v->endStorage = v->start + size ;
    }
}

void
Ral_PtrVectorFill(
    Ral_PtrVector v,
    Ral_PtrVectorValueType value)
{
    Ral_PtrVectorIter i = v->start ;

    for (i = v->start ; i < v->finish ; ++i) {
	*i = value ;
    }
}

Ral_PtrVectorValueType
Ral_PtrVectorFetch(
    Ral_PtrVector v,
    int at)
{
    if (v->start + at >= v->finish) {
	Tcl_Panic("Ral_PtrVectorFetch: out of bounds access at offset \"%d\"",
	    at) ;
    }
    return *(v->start + at) ;
}

void
Ral_PtrVectorStore(
    Ral_PtrVector v,
    int at,
    Ral_PtrVectorValueType value)
{
    if (v->start + at >= v->finish) {
	Tcl_Panic("Ral_PtrVectorStore: out of bounds access at offset \"%d\"",
	    at) ;
    }
    *(v->start + at) = value ;
}

Ral_PtrVectorValueType
Ral_PtrVectorFront(
    Ral_PtrVector v)
{
    if (Ral_PtrVectorEmpty(v)) {
	Tcl_Panic("Ral_PtrVectorFront: access to empty vector") ;
    }
    return *v->start ;
}

Ral_PtrVectorValueType
Ral_PtrVectorBack(
    Ral_PtrVector v)
{
    if (Ral_PtrVectorEmpty(v)) {
	Tcl_Panic("Ral_PtrVectorFront: access to empty vector") ;
    }
    return *(v->finish - 1) ;
}

void
Ral_PtrVectorPushBack(
    Ral_PtrVector v,
    Ral_PtrVectorValueType value)
{
    if (v->finish >= v->endStorage) {
	int oldCapacity = Ral_PtrVectorCapacity(v) ;
	/*
	 * Increase the capacity by half again. +1 to make sure
	 * we allocate at least one slot if this is the first time
	 * we are pushing onto an empty vector.
	 */
	Ral_PtrVectorReserve(v, oldCapacity + oldCapacity / 2 + 1) ;
    }
    *v->finish++ = value ;
}

void
Ral_PtrVectorPopBack(
    Ral_PtrVector v)
{
    if (Ral_PtrVectorEmpty(v)) {
	Tcl_Panic("Ral_PtrVectorPopBack: access to empty vector") ;
    }
    --v->finish ;
}

Ral_PtrVectorIter
Ral_PtrVectorInsert(
    Ral_PtrVector v,
    Ral_PtrVectorIter pos,
    int n,
    Ral_PtrVectorValueType value)
{
    int size ;
    int capacity ;
    int offset ;

    if (pos > v->finish) {
	Tcl_Panic("Ral_PtrVectorInsert: out of bounds access at offset \"%d\"",
	    (int)(pos - v->start)) ;
    }

    size = Ral_PtrVectorSize(v) ;
    capacity = Ral_PtrVectorCapacity(v) ;
    offset = pos - v->start ;
    if (capacity < size + n) {
	/*
	 * We must recompute the insert position since "Ral_PtrVectorReserve"
	 * may reallocate the memory.
	 */
	Ral_PtrVectorReserve(v, size + n) ;
	pos = v->start + offset ;
    }
    /*
     * We only have to move the vector data if we are not inserting at the end.
     */
    if (pos < v->finish) {
	memmove(pos + n, pos, (v->finish - pos) * sizeof(*pos)) ;
    }
    v->finish += n ;
    while (n--) {
	*pos++ = value ;
    }

    return v->start + offset ;
}

Ral_PtrVectorIter
Ral_PtrVectorErase(
    Ral_PtrVector v,
    Ral_PtrVectorIter first,
    Ral_PtrVectorIter last)
{
    if (first > v->finish) {
	Tcl_Panic("Ral_PtrVectorErase: out of bounds access at offset \"%d\"",
	    (int)(first - v->start)) ;
    }
    if (last > v->finish) {
	Tcl_Panic("Ral_PtrVectorErase: out of bounds access at offset \"%d\"",
	    (int)(last - v->start)) ;
    }
    memmove(first, last, (v->finish - last) * sizeof(*first)) ;
    v->finish -= last - first ;

    return first ;
}

static int
ptr_ind_compare(
    const void *m1,
    const void *m2)
{
    Ral_PtrVectorValueType const *i1 = m1 ;
    Ral_PtrVectorValueType const *i2 = m2 ;

    /*
     * Strictly speaking "*i1" is "void *" and since pointer arithmetic is
     * scaled by the size of the entity pointed to, you can't subtract two
     * "void *". GCC is very forgiving here, but native compilers are more
     * strict. It is sufficient to treat things as "char *".
     */
    return (char const *)*i1 - (char const *)*i2 ;
}

int
Ral_PtrVectorSetAdd(
    Ral_PtrVector v,
    Ral_PtrVectorValueType value)
{
    return Ral_PtrVectorFind(v, value) == v->finish ?
	(Ral_PtrVectorPushBack(v, value), 1) : 0 ;
}

void
Ral_PtrVectorSort(
    Ral_PtrVector v)
{
    qsort(v->start, v->finish - v->start, sizeof(Ral_PtrVectorValueType),
	ptr_ind_compare) ;
}

Ral_PtrVectorIter
Ral_PtrVectorFind(
    Ral_PtrVector v,
    Ral_PtrVectorValueType value)
{
    Ral_PtrVectorIter i ;

    for (i = v->start ; i < v->finish ; ++i) {
	if (*i == value) {
	    return i ;
	}
    }
    return v->finish ;
}

int
Ral_PtrVectorEqual(
    Ral_PtrVector v1,
    Ral_PtrVector v2)
{
    int equals = 0 ;
    int v1Size = Ral_PtrVectorSize(v1) ;
    if (v1Size == Ral_PtrVectorSize(v2) &&
	memcmp(v1->start, v2->start, v1Size * sizeof(Ral_PtrVectorValueType))
	    == 0) {
	equals = 1 ;
    }
    return equals ;
}

/*
 * Determine if "v1" is a subset (possibly improper) of "v2".
 */
int
Ral_PtrVectorSubsetOf(
    Ral_PtrVector v1,
    Ral_PtrVector v2)
{
    int found = 0 ;
    Ral_PtrVectorIter end1 = Ral_PtrVectorEnd(v1) ;
    Ral_PtrVectorIter end2 = Ral_PtrVectorEnd(v2) ;
    Ral_PtrVectorIter iter1 ;

    for (iter1 = Ral_PtrVectorBegin(v1) ; iter1 != end1 ; ++iter1) {
	Ral_PtrVectorIter iter2 = Ral_PtrVectorFind(v2, *iter1) ;
	found += iter2 != end2 ;
    }

    return found == Ral_PtrVectorSize(v1) ;
}

Ral_PtrVectorIter
Ral_PtrVectorCopy(
    Ral_PtrVector src,
    Ral_PtrVectorIter first,
    Ral_PtrVectorIter last,
    Ral_PtrVector dst,
    Ral_PtrVectorIter pos)
{
    int n ;

    if (first > src->finish) {
	Tcl_Panic(
	    "Ral_PtrVectorCopy: out of bounds access at source offset \"%d\"",
	    (int)(first - src->start)) ;
    }
    if (last > src->finish) {
	Tcl_Panic(
	    "Ral_PtrVectorCopy: out of bounds access at source offset \"%d\"",
	    (int)(last - src->start)) ;
    }
    if (pos > dst->finish) {
	Tcl_Panic(
	    "Ral_PtrVectorCopy: out of bounds access at dest offset \"%d\"",
	    (int)(pos - dst->start)) ;
    }

    n = last - first ;
    pos = Ral_PtrVectorInsert(dst, pos, n, 0) ;
    memcpy(pos, first, n * sizeof(*pos)) ;

    return pos ;
}

const char *
Ral_PtrVectorPrint(
    Ral_PtrVector v,
    Ral_PtrVectorIter first)
{
    static char buf[BUFSIZ] ;

    char *s = buf ;
    int n ;

    for (n = first - v->start ; first != v->finish ; ++first) {
	s += snprintf(s, sizeof(buf) - (s - buf), "%d: %X\n", n++,
	    (unsigned int)*first) ;
    }

    if (s != buf) {
	--s ;
    }
    *s++ = '\0' ;

    return buf ;
}

/*
 * PRIVATE FUNCTIONS
 */

static int
int_ind_compare(
    const void *m1,
    const void *m2)
{
    const Ral_IntVectorValueType *i1 = m1 ;
    const Ral_IntVectorValueType *i2 = m2 ;

    return *i1 - *i2 ;
}
