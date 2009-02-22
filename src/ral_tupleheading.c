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
$Revision: 1.15.2.4 $
$Date: 2009/02/22 21:13:30 $
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
	Ral_TupleHeadingIter hIter ;

	hIter = Ral_TupleHeadingPushBack(newHeading, newAttr) ;
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

    for (i = Ral_TupleHeadingBegin(h) ; i != Ral_TupleHeadingEnd(h) ; ++i) {
	Ral_AttributeDelete(*i) ;
    }
    Tcl_DeleteHashTable(&h->nameMap) ;
    ckfree((char *)h) ;
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

    for (i1 = Ral_TupleHeadingBegin(h1) ; i1 < Ral_TupleHeadingEnd(h1) ; ++i1) {
	Ral_Attribute a = *i1 ;
	Ral_TupleHeadingIter i2 = Ral_TupleHeadingFind(h2, a->name) ;

	if (i2 == Ral_TupleHeadingEnd(h2) || !Ral_AttributeEqual(a, *i2)) {
	    return 0 ;
	}
    }

    return 1 ;
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

    if (i >= Ral_TupleHeadingEnd(h)) {
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
    Ral_TupleHeadingIter iter = Ral_TupleHeadingEnd(h) ;

    if (Ral_TupleHeadingSize(h) >= Ral_TupleHeadingCapacity(h)) {
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
    for (h2Iter = Ral_TupleHeadingBegin(h2) ; h2Iter != Ral_TupleHeadingEnd(h2) ;
            ++h2Iter) {
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
    Ral_TupleHeadingIter h1Iter ;

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
    for (h1Iter = Ral_TupleHeadingBegin(h1) ; h1Iter != Ral_TupleHeadingEnd(h1) ;
            ++h1Iter) {
	Ral_Attribute h1Attr = *h1Iter ;
	Ral_TupleHeadingIter found = Ral_TupleHeadingFind(h2, h1Attr->name) ;
	/*
         * N.B. we wish the names to match and the attributes to be equal
         * meaning that the data types must match.
	 */
	if (found != Ral_TupleHeadingEnd(h2) &&
                Ral_AttributeEqual(*found, h1Attr)) {
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
 * Create a new tuple heading that is the join of two other headings.  "map"
 * contains the attribute mapping of what is to be joined.  Returns the new
 * tuple heading and "attrMap" which is a vector that is the same size as
 * the degree of h2.  Each element contains either -1 if that attribute of h2
 * is not included in the join, or the index into the join heading where that
 * attribute of h2 is to be placed in the joined relation.
 * Caller must delete the returned "attrMap" vector.
 */
Ral_TupleHeading
Ral_TupleHeadingJoin(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2,
    Ral_JoinMap map,
    Ral_IntVector *attrMap,
    Ral_ErrorInfo *errInfo)
{
    int status ;
    Ral_TupleHeading joinHeading ;
    Ral_IntVector h2JoinAttrs ;
    Ral_IntVectorIter h2AttrMapIter ;
    Ral_TupleHeadingIter h2TupleHeadingIter ;
    Ral_TupleHeadingIter h2TupleHeadingEnd = Ral_TupleHeadingEnd(h2) ;
    int joinIndex = Ral_TupleHeadingSize(h1) ;
    /*
     * Construct the tuple heading. The size of the heading is the
     * sum of the degrees of the two relations being joined minus
     * the number of attributes participating in the join.
     */
    joinHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(h1) +
	Ral_TupleHeadingSize(h2) - Ral_JoinMapAttrSize(map)) ;
    /*
     * The tuple heading contains all the attributes of the first
     * relation.
     */
    status = Ral_TupleHeadingAppend(h1, Ral_TupleHeadingBegin(h1),
	Ral_TupleHeadingEnd(h1), joinHeading) ;
    assert(status != 0) ;
    /*
     * The tuple heading contains all the attributes of the second relation
     * except those that are used in the join. So create a vector that
     * contains a boolean that indicates whether or not a given attribute
     * index is contained in the join map and use that determine which
     * attributes in r2 are placed in the join. As we iterate through the
     * vector, we modify it in place to be the offset in the join tuple
     * heading where this attribute will be placed.
     */
    *attrMap = h2JoinAttrs = Ral_JoinMapAttrMap(map, 1,
	Ral_TupleHeadingSize(h2)) ;
    h2AttrMapIter = Ral_IntVectorBegin(h2JoinAttrs) ;
    for (h2TupleHeadingIter = Ral_TupleHeadingBegin(h2) ;
	h2TupleHeadingIter != h2TupleHeadingEnd ; ++h2TupleHeadingIter) {
	if (*h2AttrMapIter) {
	    status = Ral_TupleHeadingAppend(h2,
		h2TupleHeadingIter, h2TupleHeadingIter + 1, joinHeading) ;
	    if (status == 0) {
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
		    (*h2TupleHeadingIter)->name) ;
		Ral_TupleHeadingDelete(joinHeading) ;
		Ral_IntVectorDelete(h2JoinAttrs) ;
		*attrMap = NULL ;
		return NULL ;
	    }
	    *h2AttrMapIter++ = joinIndex++ ;
	} else {
	    /*
	     * Attributes that don't appear in the result are given a final
	     * index of -1
	     */
	    *h2AttrMapIter++ = -1 ;
	}
    }

    return joinHeading ;
}

/*
 * Create a new tuple heading that is the compose of two other headings.
 * "map" contains the attribute mapping of what is to be joined.  Returns the
 * new tuple heading and two attribute maps.  An attribute map is a vector
 * that is the same size as the degree of the corresponding heading.  Each
 * element contains either -1 if that attribute of the heading is not included
 * in the compose, or the index into the compose heading where that attribute
 * of the heading is to be placed in the composed relation.  Caller must delete
 * the returned attribute map vectors.
 */
Ral_TupleHeading
Ral_TupleHeadingCompose(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2,
    Ral_JoinMap map,
    Ral_IntVector *attrMap1,
    Ral_IntVector *attrMap2,
    Ral_ErrorInfo *errInfo)
{
    int status ;
    Ral_TupleHeading composeHeading ;
    Ral_IntVector h1JoinAttrs ;
    Ral_IntVector h2JoinAttrs ;
    Ral_IntVectorIter attrMapIter ;
    Ral_TupleHeadingIter tupleHeadingIter ;
    Ral_TupleHeadingIter tupleHeadingEnd ;
    int composeIndex = 0 ;

    /*
     * Construct the tuple heading. The size of the heading is the
     * sum of the degrees of the two relations being composeed minus
     * twice the number of attributes participating in the compose.
     * Twice because we are eliminating all the compose attributes.
     */
    composeHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(h1) +
	Ral_TupleHeadingSize(h2) - 2 * Ral_JoinMapAttrSize(map)) ;
    /*
     * The tuple heading contains all the attributes of the first
     * relation except those used in the join and all the attribute of the
     * second heading except again all those used in the join.
     * So create a vector that
     * contains a boolean that indicates whether or not a given attribute
     * index is contained in the compose map and use that determine which
     * attributes are placed in the compose. As we iterate through the
     * vector, we modify it in place to be the offset in the compose tuple
     * heading where this attribute will be placed.
     */
    *attrMap1 = h1JoinAttrs = Ral_JoinMapAttrMap(map, 0,
	Ral_TupleHeadingSize(h1)) ;
    attrMapIter = Ral_IntVectorBegin(h1JoinAttrs) ;
    tupleHeadingEnd = Ral_TupleHeadingEnd(h1) ;

    for (tupleHeadingIter = Ral_TupleHeadingBegin(h1) ;
	tupleHeadingIter != tupleHeadingEnd ; ++tupleHeadingIter) {
	if (*attrMapIter) {
	    status = Ral_TupleHeadingAppend(h1,
		tupleHeadingIter, tupleHeadingIter + 1, composeHeading) ;
	    if (status == 0) {
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
		    (*tupleHeadingIter)->name) ;
		Ral_TupleHeadingDelete(composeHeading) ;
		Ral_IntVectorDelete(h1JoinAttrs) ;
		*attrMap1 = NULL ;
		return NULL ;
	    }
	    *attrMapIter++ = composeIndex++ ;
	} else {
	    /*
	     * Attributes that don't appear in the result are given a final
	     * index of -1
	     */
	    *attrMapIter++ = -1 ;
	}
    }
    /*
     * Now for the second relation.
     */
    *attrMap2 = h2JoinAttrs = Ral_JoinMapAttrMap(map, 1,
            Ral_TupleHeadingSize(h2)) ;
    attrMapIter = Ral_IntVectorBegin(h2JoinAttrs) ;
    tupleHeadingEnd = Ral_TupleHeadingEnd(h2) ;

    for (tupleHeadingIter = Ral_TupleHeadingBegin(h2) ;
	tupleHeadingIter != tupleHeadingEnd ; ++tupleHeadingIter) {
	if (*attrMapIter) {
	    status = Ral_TupleHeadingAppend(h2,
		tupleHeadingIter, tupleHeadingIter + 1, composeHeading) ;
	    if (status == 0) {
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
		    (*tupleHeadingIter)->name) ;
		Ral_TupleHeadingDelete(composeHeading) ;
		Ral_IntVectorDelete(h2JoinAttrs) ;
		*attrMap2 = NULL ;
		return NULL ;
	    }
	    *attrMapIter++ = composeIndex++ ;
	} else {
	    /*
	     * Attributes that don't appear in the result are given a final
	     * index of -1
	     */
	    *attrMapIter++ = -1 ;
	}
    }

    return composeHeading ;
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
    Ral_TupleHeadingIter iter1 ;
    int index1 = 0 ;
    int sameIndex = 0 ;

    assert(Ral_TupleHeadingEqual(h1, h2)) ;

    for (iter1 = Ral_TupleHeadingBegin(h1) ; iter1 != Ral_TupleHeadingEnd(h1) ;
            ++iter1) {
	int index2 = Ral_TupleHeadingIndexOf(h2, (*iter1)->name) ;
	/*
	 * Should always find the attribute in the second heading.
	 */
	assert(index2 >= 0) ;
        /*
         * Count the number of times that the two indices turn out to be
         * the same. This is used below to determine if the mapping
         * is the identity mapping.
         */
	sameIndex += index1 == index2 ;
	Ral_IntVectorStore(map, index1++, index2) ;
    }

    /*
     * Check if the mapping is the identity mapping and return NULL if it is.
     * We have already allocated the map, so if NULL is returned the map must
     * be deleted.
     */
    if (sameIndex == Ral_TupleHeadingSize(h1)) {
	Ral_IntVectorDelete(map) ;
	map = NULL ;
    }
    return map ;
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
    Ral_TupleHeadingIter iter1 ;

    for (iter1 = Ral_TupleHeadingBegin(h1) ; iter1 != Ral_TupleHeadingEnd(h1) ;
            ++iter1) {
	Ral_Attribute attr1 = *iter1 ;
	Ral_TupleHeadingIter iter2 = Ral_TupleHeadingFind(h2, attr1->name) ;
	/*
	 * Ral_AttributeEqual() will make sure that the types match.
	 * It is not sufficient that only the names match.
	 */
	if (iter2 != Ral_TupleHeadingEnd(h2) &&
                Ral_AttributeEqual(attr1, *iter2)) {
	    if (map) {
		Ral_JoinMapAddAttrMapping(map,
                        iter1 - Ral_TupleHeadingBegin(h1),
                        iter2 - Ral_TupleHeadingBegin(h2)) ;
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

/*
 * Scan the heading to determine how much space is needed for a string
 * to hold it.
 * N.B. this does NOT include any braces to surround the heading list.
 */
int
Ral_TupleHeadingScan(
    Ral_TupleHeading h,
    Ral_AttributeTypeScanFlags *flags)
{
    int nBytes ;
    Ral_AttributeTypeScanFlags *typeFlag ;
    Ral_TupleHeadingIter i ;
    int length ;

    assert(flags->flags.compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the type flags for each attribute.
     */
    flags->flags.compoundFlags.count = Ral_TupleHeadingSize(h) ;
    nBytes = flags->flags.compoundFlags.count *
	    sizeof(*flags->flags.compoundFlags.flags) ;
    flags->flags.compoundFlags.flags =
	    (Ral_AttributeTypeScanFlags *)ckalloc(nBytes) ;
    memset(flags->flags.compoundFlags.flags, 0, nBytes) ;

    typeFlag = flags->flags.compoundFlags.flags ;

    length = 0 ;
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

    return length ;
}

int
Ral_TupleHeadingConvert(
    Ral_TupleHeading h,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    char *p = dst ;
    Ral_AttributeTypeScanFlags *typeFlag = flags->flags.compoundFlags.flags ;
    Ral_TupleHeadingIter i ;

    assert(flags->flags.compoundFlags.count == Ral_TupleHeadingSize(h)) ;

    for (i = h->start ; i < h->finish ; ++i) {
	Ral_Attribute a = *i ;

	p += Ral_AttributeConvertName(a, p, typeFlag) ;
	*p++ = ' ' ;
	p += Ral_AttributeConvertType(a, p, typeFlag) ;
	*p++ = ' ' ;
	++typeFlag ;
    }
    /*
     * Remove any trailing blank.
     */
    if (*(p - 1) == ' ') {
	--p ;
    }

    return p - dst ;
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
    *(str + length) = '\0' ;
    Ral_AttributeTypeScanFlagsFree(&flags) ;

    return str ;
}
