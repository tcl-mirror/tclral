/*
This software is copyrighted 2005 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_tupleheading.c,v $
$Revision: 1.3 $
$Date: 2006/02/06 05:02:45 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_tupleheading.c,v $
Revision 1.3  2006/02/06 05:02:45  mangoa01
Started on relation heading and other code refactoring.
This is a checkpoint after a number of added files and changes
to tuple heading code.

Revision 1.2  2006/01/02 01:39:29  mangoa01
Tuple commands now operate properly. Fixed problems of constructing the string representation when there were tuple valued attributes.

Revision 1.1  2005/12/27 23:17:19  mangoa01
Update to the new spilt out file structure.

 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_tupleheading.h"
#include "ral_attribute.h"
#include "tcl.h"
#include <string.h>
#include <assert.h>

/*
MACRO DEFINITIONS
*/

/*
TYPE DEFINITIONS
*/

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_tupleheading.c,v $ $Revision: 1.3 $" ;

static const char tupleKeyword[] = "Tuple {" ;
static const char closeList[] = "}" ;

/*
FUNCTION DEFINITIONS
*/

Ral_TupleHeading
Ral_TupleHeadingNew(
    unsigned size)
{
    Ral_TupleHeading h ;
    unsigned memSize ;

    memSize = sizeof(*h) + size * sizeof(Ral_Attribute) ;
    h = (Ral_TupleHeading)ckalloc(memSize) ;
    memset(h, memSize, 0) ; /* set reference count to 0 */
    h->finish = h->start = (Ral_TupleHeadingIter)(h + 1) ;
    h->endStorage = h->start + size ;
    Tcl_InitHashTable(&h->nameMap, TCL_STRING_KEYS) ;
    return h ;
}

void
Ral_TupleHeadingDelete(
    Ral_TupleHeading h)
{
    Ral_TupleHeadingIter i ;

    for (i = h->start ; i != h->finish ; ++i) {
	Ral_AttributeDelete(*i) ;
    }
    Tcl_DeleteHashTable(&h->nameMap) ;
    ckfree((char *)h) ;
}

void
Ral_TupleHeadingReference(
    Ral_TupleHeading heading)
{
    ++heading->refCount ;
}

void
Ral_TupleHeadingUnreference(
    Ral_TupleHeading heading)
{
    if (heading && --heading->refCount <= 0) {
	Ral_TupleHeadingDelete(heading) ;
    }
}

int
Ral_TupleHeadingAppend(
    Ral_TupleHeading src,
    Ral_TupleHeadingIter first,
    Ral_TupleHeadingIter last,
    Ral_TupleHeading dst)
{
    int n ;

    if (first > src->finish) {
	Tcl_Panic(
	"Ral_TupleHeadingAppend: out of bounds access at source offset \"%d\"",
	    first - src->start) ;
    }
    if (last > src->finish) {
	Tcl_Panic(
	"Ral_TupleHeadingAppend: out of bounds access at source offset \"%d\"",
	    last - src->start) ;
    }
    n = last - first ;
    if (dst->finish + n > dst->endStorage) {
	Tcl_Panic("Ral_TupleHeadingAppend: overflow of destination") ;
    }

    for ( ; n ; --n) {
	Ral_Attribute a = Ral_AttributeCopy(*first++) ;
	Ral_TupleHeadingIter i = Ral_TupleHeadingPushBack(dst, a) ;
	if (i == dst->finish) {
	    /* push back failed */
	    Ral_AttributeDelete(a) ;
	    return 0 ;
	}
    }

    return 1 ;
}

int
Ral_TupleHeadingEqual(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2)
{
    Ral_TupleHeadingIter i1 ;

    if (Ral_TupleHeadingSize(h1) != Ral_TupleHeadingSize(h2)) {
	return 0 ;
    }

    for (i1 = h1->start ; i1 < h1->finish ; ++i1) {
	Ral_Attribute a = *i1 ;
	Ral_TupleHeadingIter i2 = Ral_TupleHeadingFind(h2, a->name) ;

	if (i2 == h2->finish || !Ral_AttributeEqual(a, *i2)) {
	    return 0 ;
	}
    }

    return 1 ;
}

unsigned
Ral_TupleHeadingSize(
    Ral_TupleHeading h)
{
    return h->finish - h->start ;
}

unsigned
Ral_TupleHeadingCapacity(
    Ral_TupleHeading h)
{
    return h->endStorage - h->start ;
}

int
Ral_TupleHeadingEmpty(
    Ral_TupleHeading h)
{
    return h->finish == h->start ;
}

Ral_TupleHeadingIter
Ral_TupleHeadingBegin(
    Ral_TupleHeading h)
{
    return h->start ;
}

Ral_TupleHeadingIter
Ral_TupleHeadingEnd(
    Ral_TupleHeading h)
{
    return h->finish ;
}

Ral_Attribute
Ral_TupleHeadingFetch(
    Ral_TupleHeading h,
    int index)
{
    Ral_TupleHeadingIter i = h->start + index ;

    if (i >= h->finish) {
	Tcl_Panic(
	    "Ral_TupleHeadingFetch: out of bounds access at offset \"%d\"",
	    index) ;
    }
    return *i ;
}

Ral_TupleHeadingIter
Ral_TupleHeadingStore(
    Ral_TupleHeading h,
    Ral_TupleHeadingIter i,
    Ral_Attribute attr)
{
    Tcl_HashEntry *e ;
    int isNewEntry ;

    if (i >= h->finish) {
	Tcl_Panic(
	    "Ral_TupleHeadingStore: out of bounds access at offset \"%d\"",
	    i - h->start) ;
    }

    /*
     * Check if the new attribute is already in place. Attribute names
     * must be unique.
     */
    e = Tcl_CreateHashEntry(&h->nameMap, attr->name, &isNewEntry) ;
    if (isNewEntry) {
	Ral_Attribute oldAttr = *i ;
	Tcl_HashEntry *oldEntry ;
	unsigned oldIndex ;
	unsigned currIndex ;

	oldEntry = Tcl_FindHashEntry(&h->nameMap, oldAttr->name) ;
	if (oldEntry == NULL) {
	    Tcl_Panic(
    "Ral_TupleHeadingStore: cannot find attribute name, \"%s\", in hash table",
		oldAttr->name) ;
	}
	oldIndex = (unsigned)Tcl_GetHashValue(oldEntry) ;
	currIndex = (unsigned)(i - Ral_TupleHeadingBegin(h)) ;
	if (oldIndex != currIndex) {
	    Tcl_Panic(
	    "Ral_TupleHeadingStore: inconsistent index, expected %u, got %u",
		oldIndex, currIndex) ;
	}
	Ral_AttributeDelete(oldAttr) ;
	Tcl_DeleteHashEntry(oldEntry) ;
	*i = attr ;
	Tcl_SetHashValue(e, currIndex) ;
    } else {
	Tcl_DeleteHashEntry(e) ;
	i = Ral_TupleHeadingEnd(h) ;
    }

    return i ;
}

Ral_TupleHeadingIter
Ral_TupleHeadingPushBack(
    Ral_TupleHeading h,
    Ral_Attribute attr)
{
    Tcl_HashEntry *e ;
    int isNewEntry ;
    Ral_TupleHeadingIter i ;

    if (h->finish >= h->endStorage) {
	Tcl_Panic("Ral_TupleHeadingPushBack: overflow") ;
    }

    /*
     * Check if the new attribute is already in place. Attribute names
     * must be unique.
     */
    e = Tcl_CreateHashEntry(&h->nameMap, attr->name, &isNewEntry) ;
    if (isNewEntry) {
	i = h->finish ;
	*i = attr ;
	++h->finish ;
	Tcl_SetHashValue(e, i - Ral_TupleHeadingBegin(h)) ;
    } else {
	i = Ral_TupleHeadingEnd(h) ;
    }

    return i ;
}

Ral_TupleHeadingIter
Ral_TupleHeadingFind(
    Ral_TupleHeading h,
    const char *name)
{
    Tcl_HashEntry *e ;

    e = Tcl_FindHashEntry(&h->nameMap, name) ;
    return e ?  h->start + (unsigned)Tcl_GetHashValue(e) : h->finish ;
}

int
Ral_TupleHeadingIndexOf(
    Ral_TupleHeading h,
    const char *name)
{
    Tcl_HashEntry *e ;

    e = Tcl_FindHashEntry(&h->nameMap, name) ;
    return e ?  (int)Tcl_GetHashValue(e) : -1 ;
}

/*
 * Create a new tuple heading that is the union of two headings.
 */
Ral_TupleHeading
Ral_TupleHeadingUnion(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2)
{
    Ral_TupleHeading unionHeading ;
    Ral_TupleHeadingIter h2End ;
    Ral_TupleHeadingIter h2Iter ;

    /*
     * The maximum size of the union is the sum of the sizes of the
     * two components.
     */
    unionHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(h1) +
	Ral_TupleHeadingSize(h2)) ;

    /*
     * Copy in the first heading.
     */
    if (!Ral_TupleHeadingAppend(h1, Ral_TupleHeadingBegin(h1),
	    Ral_TupleHeadingEnd(h1), unionHeading)) {
	Ral_TupleHeadingDelete(unionHeading) ;
	return NULL ;
    }
    /*
     * Iterate through the second heading and push on the attributes.
     * Unlike a normal set union, it is important that there be no
     * common attributes between the two headings that are components
     * of the union.
     */
    h2End = Ral_TupleHeadingEnd(h2) ;
    for (h2Iter = Ral_TupleHeadingBegin(h2) ; h2Iter != h2End ; ++h2Iter) {
	Ral_TupleHeadingIter i =
	    Ral_TupleHeadingPushBack(unionHeading, Ral_AttributeCopy(*h2Iter)) ;
	if (i == Ral_TupleHeadingEnd(unionHeading)) {
	    Ral_TupleHeadingDelete(unionHeading) ;
	    return NULL ;
	}
    }

    return unionHeading ;
}

/*
 * Create a new tuple heading that is the intersection of two headings.
 */
Ral_TupleHeading
Ral_TupleHeadingIntersect(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2)
{
    Ral_TupleHeading intersectHeading ;
    unsigned h1Size = Ral_TupleHeadingSize(h1) ;
    unsigned h2Size = Ral_TupleHeadingSize(h2) ;
    Ral_TupleHeadingIter h1End ;
    Ral_TupleHeadingIter h1Iter ;
    Ral_TupleHeadingIter h2End ;

    /*
     * The maximum size of the intersection is the smaller of the sum of the
     * sizes of the two components.
     */
    intersectHeading = Ral_TupleHeadingNew(h1Size < h2Size ? h1Size : h2Size) ;

    /*
     * Iterate through the first heading finding those attributes that also
     * exist in the second heading. If the attribute is contained in both tuple
     * headings, then it is placed in the intersection.
     */
    h1End = Ral_TupleHeadingEnd(h1) ;
    h2End = Ral_TupleHeadingEnd(h2) ;
    for (h1Iter = Ral_TupleHeadingBegin(h1) ; h1Iter != h1End ; ++h1Iter) {
	Ral_Attribute h1Attr = *h1Iter ;
	Ral_TupleHeadingIter found = Ral_TupleHeadingFind(h2, h1Attr->name) ;

	/*
	 * N.B. we wish the names to match and the attributes to be
	 * otherwise equal.
	 */
	if (found != h2End && Ral_AttributeEqual(*found, h1Attr)) {
	    Ral_TupleHeadingIter place =
		Ral_TupleHeadingPushBack(intersectHeading,
		Ral_AttributeCopy(*found)) ;

	    if (place == Ral_TupleHeadingEnd(intersectHeading)) {
		Ral_TupleHeadingDelete(intersectHeading) ;
		return NULL ;
	    }
	}
    }

    return intersectHeading ;
}

int
Ral_TupleHeadingScan(
    Ral_TupleHeading h,
    Ral_AttributeScanFlags flags)
{
    Ral_TupleHeadingIter i ;
    int length = 1 ; /* for the NUL terminator */

    /*
     * The keyword part of the heading.
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    length += sizeof(tupleKeyword) - 1 ;

    for (i = h->start ; i < h->finish ; ++i) {
	Ral_Attribute a = *i ;

	/*
	 * +1 to account for the space that needs to separate elements
	 * in the resulting list.
	 */
	length += Ral_AttributeScanName(a, flags) + 1 ;
	length += Ral_AttributeScanType(a, flags) + 1 ;
	++flags ;
    }
    length += sizeof(closeList) - 1 ;

    return length ;
}

int
Ral_TupleHeadingConvert(
    Ral_TupleHeading h,
    char *dst,
    Ral_AttributeScanFlags flags)
{
    char *p = dst ;
    Ral_TupleHeadingIter i ;

    /*
     * Copy in the "Tuple" keyword.
     */
    strcpy(p, tupleKeyword) ;
    p += sizeof(tupleKeyword) - 1 ;

    for (i = h->start ; i < h->finish ; ++i) {
	Ral_Attribute a = *i ;

	p += Ral_AttributeConvertName(a, p, flags) ;
	*p++ = ' ' ;
	p += Ral_AttributeConvertType(a, p, flags) ;
	*p++ = ' ' ;
	++flags ;
    }
    /*
     * Overwrite any trailing blank. There will be no trailing blank if the
     * heading didn't have any attributes.
     */
    if (!Ral_TupleHeadingEmpty(h)) {
	--p ;
    }
    strcpy(p, closeList) ; /* NUL terminates the result */
    p += sizeof(closeList) - 1 ;

    return p - dst ;
}

/*
 * Returned string must be freed by the caller.
 */
char *
Ral_TupleHeadingToString(
    Ral_TupleHeading h)
{
    unsigned size = Ral_TupleHeadingSize(h) ;
    Ral_AttributeScanFlags flags ;
    char *str ;

    flags = (Ral_AttributeScanFlags)ckalloc(size * sizeof(*flags)) ;

    str = ckalloc(Ral_TupleHeadingScan(h, flags)) ;
    Ral_TupleHeadingConvert(h, str, flags) ;

    Ral_AttributeScanFlagsFree(size, flags) ;

    return str ;
}

const char *
Ral_TupleHeadingVersion(void)
{
    return rcsid ;
}
