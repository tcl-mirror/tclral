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

ABSTRACT:

$RCSfile: ral_tupleheading.c,v $
$Revision: 1.12 $
$Date: 2006/11/05 00:15:59 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_tupleheading.h"
#include "ral_tupleobj.h"
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
static const char rcsid[] = "@(#) $RCSfile: ral_tupleheading.c,v $ $Revision: 1.12 $" ;

static const char openList = '{' ;
static const char closeList = '}' ;

/*
FUNCTION DEFINITIONS
*/

Ral_TupleHeading
Ral_TupleHeadingNew(
    int size)
{
    Ral_TupleHeading h ;
    int memSize ;

    memSize = sizeof(*h) + size * sizeof(Ral_Attribute) ;
    h = (Ral_TupleHeading)ckalloc(memSize) ;
    memset(h, 0, memSize) ; /* set reference count to 0 */
    h->finish = h->start = (Ral_TupleHeadingIter)(h + 1) ;
    h->endStorage = h->start + size ;
    Tcl_InitHashTable(&h->nameMap, TCL_STRING_KEYS) ;
    return h ;
}

/*
 * Build a new tuple heading based on the old heading
 * using only the attributes in the attribute set.
 */
Ral_TupleHeading
Ral_TupleHeadingSubset(
    Ral_TupleHeading heading,
    Ral_IntVector attrSet)
{
    Ral_TupleHeading newHeading =
	Ral_TupleHeadingNew(Ral_IntVectorSize(attrSet)) ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(attrSet) ;
    Ral_IntVectorIter iter ;

    for (iter = Ral_IntVectorBegin(attrSet) ; iter != end ; ++iter) {
	Ral_Attribute attr = Ral_TupleHeadingFetch(heading, *iter) ;
	Ral_Attribute newAttr = Ral_AttributeDup(attr) ;
	Ral_TupleHeadingIter hIter = Ral_TupleHeadingPushBack(newHeading,
	    newAttr) ;
	assert(hIter != Ral_TupleHeadingEnd(newHeading)) ;
    }

    return newHeading ;
}

Ral_TupleHeading
Ral_TupleHeadingDup(
    Ral_TupleHeading srcHeading)
{
    Ral_TupleHeading dupHeading ;
    int appended ;

    dupHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(srcHeading)) ;
    appended = Ral_TupleHeadingAppend(srcHeading,
	Ral_TupleHeadingBegin(srcHeading),
	Ral_TupleHeadingEnd(srcHeading), dupHeading) ;
    assert(appended != 0) ;

    return dupHeading ;
}

Ral_TupleHeading
Ral_TupleHeadingExtend(
    Ral_TupleHeading heading,
    int addAttrs)
{
    Ral_TupleHeading extendHeading ;
    int appended ;

    extendHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(heading) +
	addAttrs) ;
    appended = Ral_TupleHeadingAppend(heading, Ral_TupleHeadingBegin(heading),
	Ral_TupleHeadingEnd(heading), extendHeading) ;
    assert(appended != 0) ;

    return extendHeading ;
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
	Ral_Attribute a = Ral_AttributeDup(*first++) ;
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

int
Ral_TupleHeadingSize(
    Ral_TupleHeading h)
{
    return h->finish - h->start ;
}

int
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
	int oldIndex ;
	int currIndex ;

	oldEntry = Tcl_FindHashEntry(&h->nameMap, oldAttr->name) ;
	if (oldEntry == NULL) {
	    Tcl_Panic(
    "Ral_TupleHeadingStore: cannot find attribute name, \"%s\", in hash table",
		oldAttr->name) ;
	}
	oldIndex = (int)Tcl_GetHashValue(oldEntry) ;
	currIndex = (int)(i - Ral_TupleHeadingBegin(h)) ;
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
    Ral_TupleHeadingIter iter = h->finish ;

    if (h->finish >= h->endStorage) {
	Tcl_Panic("Ral_TupleHeadingPushBack: overflow") ;
    }

    /*
     * Check if the new attribute is already in place. Attribute names
     * must be unique.
     */
    e = Tcl_CreateHashEntry(&h->nameMap, attr->name, &isNewEntry) ;
    if (isNewEntry) {
	*h->finish++ = attr  ;
	Tcl_SetHashValue(e, iter - h->start) ;
    }

    return iter ;
}

Ral_TupleHeadingIter
Ral_TupleHeadingFind(
    Ral_TupleHeading h,
    const char *name)
{
    Tcl_HashEntry *e ;

    e = Tcl_FindHashEntry(&h->nameMap, name) ;
    return e ?  h->start + (int)Tcl_GetHashValue(e) : h->finish ;
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
	    Ral_TupleHeadingPushBack(unionHeading, Ral_AttributeDup(*h2Iter)) ;
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
    int h1Size = Ral_TupleHeadingSize(h1) ;
    int h2Size = Ral_TupleHeadingSize(h2) ;
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
		Ral_AttributeDup(*found)) ;

	    if (place == Ral_TupleHeadingEnd(intersectHeading)) {
		Ral_TupleHeadingDelete(intersectHeading) ;
		return NULL ;
	    }
	}
    }

    return intersectHeading ;
}

/*
 * Compute ordering to map attributes in "h1" to the corresponding attributes
 * in "h2".
 * If the ordering is the same, i.e. the mapping is the identity mapping, then
 * NULL is returned to indicate that no mapping is necessary.  Otherwise, the
 * returned vector is in the order of attributes in "h1" with the values giving
 * the index into "h2" where the same named attribute can be found.
 * Caller must delete the returned vector.
 */
Ral_IntVector
Ral_TupleHeadingNewOrderMap(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2)
{
    Ral_IntVector map = Ral_IntVectorNew(Ral_TupleHeadingSize(h1), -1) ;
    Ral_TupleHeadingIter end1 = Ral_TupleHeadingEnd(h1) ;
    Ral_TupleHeadingIter iter1 ;
    int index1 = 0 ;
    int found = 0 ;

    assert(Ral_TupleHeadingEqual(h1, h2)) ;

    for (iter1 = Ral_TupleHeadingBegin(h1) ; iter1 != end1 ; ++iter1) {
	int index2 = Ral_TupleHeadingIndexOf(h2, (*iter1)->name) ;
	/*
	 * Should always find the attribute in the second heading.
	 */
	assert(index2 >= 0) ;
	found += index1 == index2 ;
	Ral_IntVectorStore(map, index1++, index2) ;
    }

    return found == Ral_TupleHeadingSize(h1) ?
	Ral_IntVectorDelete(map), NULL : map ;
}

/*
 * Find the common attributes between two headings.
 * Return the number of common attributes found.
 * If "map" is non-NULL then the mapping is also recorded there.
 */
int
Ral_TupleHeadingCommonAttributes(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2,
    Ral_JoinMap map)
{
    int count = 0 ;
    Ral_TupleHeadingIter begin1 = Ral_TupleHeadingBegin(h1) ;
    Ral_TupleHeadingIter end1 = Ral_TupleHeadingEnd(h1) ;
    Ral_TupleHeadingIter begin2 = Ral_TupleHeadingBegin(h2) ;
    Ral_TupleHeadingIter end2 = Ral_TupleHeadingEnd(h2) ;
    Ral_TupleHeadingIter iter1 ;

    for (iter1 = begin1 ; iter1 != end1 ; ++iter1) {
	Ral_Attribute attr1 = *iter1 ;
	Ral_TupleHeadingIter iter2 = Ral_TupleHeadingFind(h2, attr1->name) ;
	/*
	 * Ral_AttributeEqual() will make sure that the types match.
	 * It is not sufficient that only the names match.
	 */
	if (iter2 != end2 && Ral_AttributeEqual(attr1, *iter2)) {
	    if (map) {
		Ral_JoinMapAddAttrMapping(map, iter1 - begin1, iter2 - begin2) ;
	    }
	    ++count ;
	}
    }

    return count ;
}

/*
 * Remap attribute indices.
 * "v" is a set of attribute indices for "h1".
 * Modify "v" in place so that the indices refer to the
 * same named attributes in "h2".
 */
void
Ral_TupleHeadingMapIndices(
    Ral_TupleHeading h1,
    Ral_IntVector v,
    Ral_TupleHeading h2)
{
    Ral_TupleHeadingIter h1Begin = Ral_TupleHeadingBegin(h1) ;
    Ral_IntVectorIter vEnd = Ral_IntVectorEnd(v) ;
    Ral_IntVectorIter vIter ;

    for (vIter = Ral_IntVectorBegin(v) ; vIter != vEnd ; ++vIter) {
	Ral_Attribute attr = *(h1Begin + *vIter) ;
	int newIndex = Ral_TupleHeadingIndexOf(h2, attr->name) ;
	assert(newIndex != -1) ;
	*vIter = newIndex ;
    }
}

int
Ral_TupleHeadingScan(
    Ral_TupleHeading h,
    Ral_AttributeTypeScanFlags *flags)
{
    int nBytes ;
    int length = strlen(Ral_TupleObjType.name) + 1 ; /* +1 for space */

    assert(flags->attrType == Tuple_Type) ;
    assert(flags->compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the type flags for each attribute.
     */
    flags->compoundFlags.count = Ral_TupleHeadingSize(h) ;
    nBytes = flags->compoundFlags.count * sizeof(*flags->compoundFlags.flags) ;
    flags->compoundFlags.flags = (Ral_AttributeTypeScanFlags *)ckalloc(nBytes) ;
    memset(flags->compoundFlags.flags, 0, nBytes) ;

    length += Ral_TupleHeadingScanAttr(h, flags) ;

    return length ;
}

int
Ral_TupleHeadingScanAttr(
    Ral_TupleHeading h,
    Ral_AttributeTypeScanFlags *flags)
{
    Ral_AttributeTypeScanFlags *typeFlag = flags->compoundFlags.flags ;
    Ral_TupleHeadingIter i ;
    int length = sizeof(openList) ;

    assert(flags->compoundFlags.count == Ral_TupleHeadingSize(h)) ;
    assert(flags->compoundFlags.flags != NULL) ;

    for (i = h->start ; i < h->finish ; ++i) {
	Ral_Attribute a = *i ;
	/*
	 * +1 to account for the space that needs to separate elements
	 * in the resulting list.
	 */
	length += Ral_AttributeScanName(a, typeFlag) + 1 ;
	length += Ral_AttributeScanType(a, typeFlag) + 1 ;
	++typeFlag ;
    }
    if (!Ral_TupleHeadingEmpty(h)) {
	--length ;
    }
    length += sizeof(closeList) ;

    return length ;
}

int
Ral_TupleHeadingConvert(
    Ral_TupleHeading h,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    char *p = dst ;

    assert(flags->attrType == Tuple_Type) ;
    assert(flags->compoundFlags.count == Ral_TupleHeadingSize(h)) ;

    strcpy(p, tupleKeyword) ;
    p += strlen(tupleKeyword) ;
    *p++ = ' ' ;

    p += Ral_TupleHeadingConvertAttr(h, p, flags) ;

    return p - dst ;
}

int
Ral_TupleHeadingConvertAttr(
    Ral_TupleHeading h,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    char *p = dst ;
    Ral_AttributeTypeScanFlags *typeFlag = flags->compoundFlags.flags ;
    Ral_TupleHeadingIter i ;

    assert(flags->compoundFlags.count == Ral_TupleHeadingSize(h)) ;

    *p++ = openList ;

    for (i = h->start ; i < h->finish ; ++i) {
	Ral_Attribute a = *i ;

	p += Ral_AttributeConvertName(a, p, typeFlag) ;
	*p++ = ' ' ;
	p += Ral_AttributeConvertType(a, p, typeFlag) ;
	*p++ = ' ' ;
	++typeFlag ;
    }
    /*
     * Overwrite any trailing blank. There will be no trailing blank if the
     * heading didn't have any attributes.
     */
    if (!Ral_TupleHeadingEmpty(h)) {
	--p ;
    }
    *p++ = closeList ;

    return p - dst ;
}

void
Ral_TupleHeadingPrint(
    Ral_TupleHeading h,
    const char *format,
    FILE *f)
{
    char *str = Ral_TupleHeadingStringOf(h) ;
    fprintf(f, format, str) ;
    ckfree(str) ;
}

char *
Ral_TupleHeadingStringOf(
    Ral_TupleHeading h)
{
    Ral_AttributeTypeScanFlags flags ;
    char *str ;
    int length ;

    memset(&flags, 0, sizeof(flags)) ;
    flags.attrType = Tuple_Type ;
    /*
     * +1 for the NUL terminator
     */
    str = ckalloc(Ral_TupleHeadingScan(h, &flags) + 1) ;
    length = Ral_TupleHeadingConvert(h, str, &flags) ;
    str[length] = '\0' ;
    Ral_AttributeTypeScanFlagsFree(&flags) ;

    return str ;
}

const char *
Ral_TupleHeadingVersion(void)
{
    return rcsid ;
}
