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

$RCSfile: ral_relationheading.c,v $
$Revision: 1.22 $
$Date: 2008/01/19 19:16:45 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_relationheading.h"
#include "ral_relationobj.h"
#include <assert.h>
#include <string.h>

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
static int Ral_IsIdASubsetOf(Ral_RelationHeading, Ral_IntVector) ;
static int Ral_IsForeignIdASubsetOf(Ral_RelationHeading, Ral_RelationHeading,
    Ral_IntVector) ;
static void Ral_AddJoinId( Ral_IntVector, Ral_IntVector, Ral_JoinMap,
    Ral_IntVector) ;
static int Ral_IdContainsMappedAttr(Ral_IntVector, Ral_IntVector) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationheading.c,v $ $Revision: 1.22 $" ;

static char const openList = '{' ;
static char const closeList = '}' ;

/*
FUNCTION DEFINITIONS
*/

Ral_RelationHeading
Ral_RelationHeadingNew(
    Ral_TupleHeading tupleHeading,
    int idCount)
{
    Ral_RelationHeading heading ;
    int nBytes ;

    assert(idCount > 0) ;

    nBytes = sizeof(*heading) + idCount * sizeof(*heading->identifiers) ;
    heading = (Ral_RelationHeading)ckalloc(nBytes) ;
    memset(heading, 0, nBytes) ;

    heading->refCount = 0 ;
    Ral_TupleHeadingReference(heading->tupleHeading = tupleHeading) ;
    heading->idCount = idCount ;
    heading->identifiers = (Ral_IntVector *)(heading + 1) ;

    /*
     * Note at this point the vector pointers in the "identifiers" member
     * are all NULL.
     */

    return heading ;
}

/*
 * Create a new heading with a default id consisting of all the attributes.
 */
Ral_RelationHeading
Ral_RelationHeadingNewDefaultId(
    Ral_TupleHeading tupleHeading)
{
    Ral_RelationHeading heading =Ral_RelationHeadingNew(tupleHeading, 1) ;
    Ral_IntVector allId =
	Ral_IntVectorNew(Ral_TupleHeadingSize(tupleHeading), 0) ;
    int status ;

    Ral_IntVectorFillConsecutive(allId, 0) ;
    status = Ral_RelationHeadingAddIdentifier(heading, 0, allId) ;
    assert(status != 0) ;

    return heading ;
}

Ral_RelationHeading
Ral_RelationHeadingSubset(
    Ral_RelationHeading heading,
    Ral_IntVector attrList)
{
    Ral_TupleHeading tupleHeading =
	Ral_TupleHeadingSubset(heading->tupleHeading, attrList) ;
    Ral_IntVector foundIds = Ral_IntVectorNewEmpty(heading->idCount) ;
    Ral_RelationIdIter idIter ;
    Ral_RelationIdIter idBegin = Ral_RelationHeadingIdBegin(heading) ;
    Ral_RelationIdIter idEnd = Ral_RelationHeadingIdEnd(heading) ;
    Ral_RelationHeading subHeading ;
    /*
     * Scan the identifiers to determine if they are retained in the
     * subset.
     */
    for (idIter = idBegin ; idIter != idEnd ; ++idIter) {
	if (Ral_IntVectorSubsetOf(*idIter, attrList)) {
	    Ral_IntVectorPushBack(foundIds, idIter - idBegin) ;
	}
    }

    subHeading = Ral_RelationHeadingIdSubset(heading, tupleHeading, foundIds,
	attrList) ;
    Ral_IntVectorDelete(foundIds) ;
    return subHeading ;
}

/*
 * Create a new relation heading from "tupleHeading" using the
 * identifiers in "heading" that are contained in "idList". The identifiers
 * in the new heading have their numbers remapped according to the
 * contents of "attrList".
 */
Ral_RelationHeading
Ral_RelationHeadingIdSubset(
    Ral_RelationHeading heading,
    Ral_TupleHeading tupleHeading,
    Ral_IntVector idList,
    Ral_IntVector attrList)
{
    int idCount = Ral_IntVectorSize(idList) ;
    Ral_RelationIdIter idBegin = Ral_RelationHeadingIdBegin(heading) ;
    Ral_RelationHeading subHeading ;

    if (idCount) {
	Ral_IntVectorIter end = Ral_IntVectorEnd(idList) ;
	Ral_IntVectorIter iter ;
	int newIdCount = 0 ;
	/*
	 * Create a new relation heading to hold the identifiers
	 * and copy them in.
	 */
	subHeading = Ral_RelationHeadingNew(tupleHeading, idCount) ;
	for (iter = Ral_IntVectorBegin(idList) ; iter != end ; ++iter) {
	    Ral_IntVector newId = Ral_IntVectorDup(*(idBegin + *iter)) ;
	    Ral_IntVectorIter newIdEnd = Ral_IntVectorEnd(newId) ;
	    Ral_IntVectorIter newIdIter ;
	    int status ;
	    /*
	     * Remap new id
	     */
	    for (newIdIter = Ral_IntVectorBegin(newId) ; newIdIter != newIdEnd ;
		++newIdIter) {
		int found = Ral_IntVectorIndexOf(attrList, *newIdIter) ;
		assert(found != -1) ;
		*newIdIter = found ;
	    }
	    status = Ral_RelationHeadingAddIdentifier(subHeading, newIdCount++,
		newId) ;
	    assert(status != 0) ;
	}
    } else {
	/*
	 * None of the identifiers survived the subset, so make all
	 * the attributes the single identifier.
	 */
	subHeading = Ral_RelationHeadingNewDefaultId(tupleHeading) ;
    }

    return subHeading ;
}

Ral_RelationHeading
Ral_RelationHeadingDup(
    Ral_RelationHeading srcHeading)
{
    Ral_TupleHeading tupleHeading ;
    Ral_RelationHeading dupHeading ;
    int idCount = srcHeading->idCount ;
    Ral_IntVector *srcIds = srcHeading->identifiers ;
    Ral_IntVector *dstIds ;

    tupleHeading = Ral_TupleHeadingDup(srcHeading->tupleHeading) ;
    dupHeading = Ral_RelationHeadingNew(tupleHeading, idCount) ;
    dstIds = dupHeading->identifiers ;
    while (idCount-- > 0) {
	*dstIds++ = Ral_IntVectorDup(*srcIds++) ;
    }

    return dupHeading ;
}

/*
 * Create a new relation heading that is an extension of "heading".
 * The extended heading will have "extTupleHeading" as the tupleheading.
 */
Ral_RelationHeading
Ral_RelationHeadingExtend(
    Ral_RelationHeading heading,
    Ral_TupleHeading extTupleHeading,
    int addIds)
{
    Ral_RelationHeading extHeading ;
    Ral_RelationIdIter srcIdIter ;
    Ral_RelationIdIter srcIdEnd = Ral_RelationHeadingIdEnd(heading) ;
    Ral_RelationIdIter dstIdIter ;

    /*
     * Normally all the identifiers are obtained from the original heading.
     * That is extending a heading does not introduce new identifiers.
     * The exception is the "dum" and "dee" relations. Extending them
     * results in identifiers begin created from the extended attributes.
     */
    extHeading = Ral_RelationHeadingNew(extTupleHeading,
	heading->idCount + addIds) ;
    dstIdIter = Ral_RelationHeadingIdBegin(extHeading) ;
    if (Ral_RelationHeadingDegree(heading) == 0) {
	/*
	 * If we are extending a relation that has no attributes, then we take
	 * all the attributes of the extention as components of the single
	 * identifier for the extended relation.
	 */
	Ral_IntVector idVect = Ral_IntVectorNew(
	    Ral_TupleHeadingSize(extTupleHeading), 0) ;

	assert(extHeading->idCount == 1) ;
	Ral_IntVectorFillConsecutive(idVect, 0) ;
	*extHeading->identifiers = idVect ;
    } else {
	for (srcIdIter = Ral_RelationHeadingIdBegin(heading) ;
	    srcIdIter != srcIdEnd ; ++srcIdIter) {
	    *dstIdIter++ = Ral_IntVectorDup(*srcIdIter) ;
	}
    }

    return extHeading ;
}

void
Ral_RelationHeadingDelete(
    Ral_RelationHeading heading)
{
    Ral_IntVector *idArray = heading->identifiers ;
    int idCount = heading->idCount ;

    assert(heading->refCount <= 0) ;

    while (idCount-- > 0) {
	Ral_IntVector id = *idArray++ ;
	if (id) {
	    Ral_IntVectorDelete(id) ;
	}
    }
    Ral_TupleHeadingUnreference(heading->tupleHeading) ;
    ckfree((char *)heading) ;
}


void
Ral_RelationHeadingReference(
    Ral_RelationHeading heading)
{
    ++heading->refCount ;
}

void
Ral_RelationHeadingUnreference(
    Ral_RelationHeading heading)
{
    if (heading && --heading->refCount <= 0) {
	Ral_RelationHeadingDelete(heading) ;
    }
}

Ral_RelationIdIter
Ral_RelationHeadingIdBegin(
    Ral_RelationHeading heading)
{
    return heading->identifiers ;
}

Ral_RelationIdIter
Ral_RelationHeadingIdEnd(
    Ral_RelationHeading heading)
{
    return heading->identifiers + heading->idCount ;
}

int
Ral_RelationHeadingDegree(
    Ral_RelationHeading heading)
{
    return Ral_TupleHeadingSize(heading->tupleHeading) ;
}

int
Ral_RelationHeadingEqual(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2)
{
    int id1Count ;
    Ral_IntVector *id1Array ;

    /*
     * The easy case is when we are pointing to the same heading.
     */
    if (h1 == h2) {
	return 1 ;
    }
    /*
     * The Tuple Headings must be the same.
     */
    if (!Ral_TupleHeadingEqual(h1->tupleHeading, h2->tupleHeading)) {
	return 0 ;
    }
    /*
     * The identifiers must be the same.
     *
     * We start by insuring that we have the same number of identifiers.
     */
    if (h1->idCount != h2->idCount) {
	return 0 ;
    }
    /*
     * The strategy to compare the identifiers is to use the names of the
     * identifying attributes from the first heading to construct an identifier
     * index set using the ordering of the tuple heading in the second heading.
     * Remember, attribute ordering does not matter.  Given the vector of
     * attribute indices relative to the ordering of the second heading, we
     * then have to search the heading to find the identfier.
     *
     * Start with a loop iterating over the identifiers of the first heading.
     */
    id1Array = h1->identifiers ;
    for (id1Count = h1->idCount ; id1Count > 0 ; --id1Count) {
	Ral_IntVector id1 ;
	Ral_IntVector id2 ;
	Ral_IntVectorIter end1Iter ;
	Ral_IntVectorIter id1Iter ;
	int found ;

	id1 = *id1Array++ ;
	end1Iter = Ral_IntVectorEnd(id1) ;
	/*
	 * Construct a new vector of attribute indices using the
	 * names from the first heading to find its corresponding
	 * index in the second heading.
	 */
	id2 = Ral_IntVectorNewEmpty(Ral_IntVectorSize(id1)) ;
	/*
	 * Iterate through all the attribute indices of the identifier and
	 * construct the vector to have a set of attribute indices that
	 * are relative to the ordering of attributes in the second heading.
	 */
	for (id1Iter = Ral_IntVectorBegin(id1) ; id1Iter != end1Iter ;
	    ++id1Iter) {
	    /*
	     * Get the attribute.
	     */
	    Ral_Attribute attr1 = Ral_TupleHeadingFetch(h1->tupleHeading,
		*id1Iter) ;
	    /*
	     * Find the index of that attribute name in the second heading.
	     */
	    int attr2Index = Ral_TupleHeadingIndexOf(h2->tupleHeading,
		attr1->name) ;
	    /*
	     * The attribute must exist in the second heading since we
	     * have already determined that the two tuple headings are equal.
	     */
	    assert (attr2Index >= 0) ;
	    Ral_IntVectorPushBack(id2, attr2Index) ;
	}

	/*
	 * Now we search the identifiers of the second heading to find
	 * a match to the vector we just constructed.
	 */
	found = Ral_RelationHeadingFindIdentifier(h2, id2) != -1 ;
	/*
	 * Discard the vector.
	 */
	Ral_IntVectorDelete(id2) ;
	if (!found) {
	    return 0 ;
	}
    }

    return 1 ;
}

/*
 * Determine if two headings are equal and report the error information if not.
 */
int
Ral_RelationHeadingMatch(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2,
    Ral_ErrorInfo *errInfo)
{
    int status = Ral_RelationHeadingEqual(h1, h2) ;
    if (status == 0) {
	char *h2str = Ral_RelationHeadingStringOf(h2) ;
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_HEADING_NOT_EQUAL, h2str) ;
    }
    return status ;
}

int
Ral_RelationHeadingAddIdentifier(
    Ral_RelationHeading heading,
    int idNum,
    Ral_IntVector id)
{
    int result = 0 ;

    assert (idNum < heading->idCount) ;
    assert (heading->identifiers[idNum] == NULL) ;
    /*
     * Check if the identifier is not a subset of an existing one.
     */
    if (!Ral_IsIdASubsetOf(heading, id)) {
	/*
	 * Take possession of the identifier vector.
	 */
	if (heading->identifiers[idNum]) {
	    Ral_IntVectorDelete(heading->identifiers[idNum]) ;
	}
	heading->identifiers[idNum] = id ;
	result = 1 ;
    }
    return result ;
}

/*
 * Return the index into the identifer array where the identifier
 * matches. Returns -1 if there is no match.
 */
int
Ral_RelationHeadingFindIdentifier(
    Ral_RelationHeading heading,
    Ral_IntVector id)
{
    int idCount = heading->idCount ;
    Ral_IntVector *idArray = heading->identifiers ;
    int idSize = Ral_IntVectorSize(id) ;
    int result = -1 ;

    /*
     * Iterate through the identifies in the heading looking for a match.
     * Identifiers are sorted, however the "id" argument may not be.
     * We don't want to sort it in place, so the comparison for equality
     * means that we must go through the values in "id" and determine if
     * all of them are contained in some identifier of the "heading".
     */
    assert(idCount >= 1) ;
    for (idArray = heading->identifiers ; idCount > 0 ; --idCount, ++idArray) {
	Ral_IntVector headingId = *idArray ;

	if (headingId && idSize == Ral_IntVectorSize(headingId) &&
	    Ral_IntVectorSubsetOf(id, headingId)) {
	    result = idArray - heading->identifiers ;
	    break ;
	}
    }
    return result ;
}

/*
 * Create a new relation heading that is the union of two other headings.
 */
Ral_RelationHeading
Ral_RelationHeadingUnion(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2)
{
    int h1Size = Ral_TupleHeadingSize(h1->tupleHeading) ;
    Ral_TupleHeading unionTupleHeading ;
    Ral_RelationHeading unionRelationHeading ;
    Ral_RelationIdIter id1Iter ;
    Ral_RelationIdIter id1End = Ral_RelationHeadingIdEnd(h1) ;
    int idNum = 0 ;

    /*
     * Union the tuple heading.
     */
    unionTupleHeading = Ral_TupleHeadingUnion(h1->tupleHeading,
	h2->tupleHeading) ;
    if (unionTupleHeading == NULL) {
	return NULL ;
    }

    /*
     * The identifiers for the union are the cross product of the identifiers
     * of the two component headings.
     */
    unionRelationHeading = Ral_RelationHeadingNew(unionTupleHeading,
	h1->idCount * h2->idCount) ;

    /*
     * Loop through the identifiers for the two heading components
     * and compose new identifiers for the union.
     */
    for (id1Iter = Ral_RelationHeadingIdBegin(h1) ;
	id1Iter != id1End ; ++id1Iter) {
	Ral_IntVector h1Id = *id1Iter ;
	Ral_RelationIdIter id2Iter ;
	Ral_RelationIdIter id2End = Ral_RelationHeadingIdEnd(h2) ;

	for (id2Iter = Ral_RelationHeadingIdBegin(h2) ;
	    id2Iter != id2End ; ++id2Iter) {
	    Ral_IntVector h2Id = *id2Iter ;
	    Ral_IntVectorIter h2IdIter ;
	    Ral_IntVectorIter h2IdEnd = Ral_IntVectorEnd(h2Id) ;
	    Ral_IntVector unionId = Ral_IntVectorNewEmpty(
		Ral_IntVectorSize(h1Id) + Ral_IntVectorSize(h2Id)) ;
	    int added ;
	    /*
	     * Copy the identifier indices from the first header.
	     * These indices are correct for the union header also.
	     */
	    Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
		Ral_IntVectorEnd(h1Id), unionId, Ral_IntVectorBegin(unionId)) ;
	    /*
	     * The identifier indices from the second header must
	     * be offset by the number of attributes in the first header.
	     * Union tuple headers are constructed as simple concatenations.
	     */
	    for (h2IdIter = Ral_IntVectorBegin(h2Id) ; h2IdIter != h2IdEnd ;
		++h2IdIter) {
		Ral_IntVectorPushBack(unionId, *h2IdIter + h1Size) ;
	    }
	    /*
	     * Add the newly formed identifier.
	     * It should always add.
	     */
	    added = Ral_RelationHeadingAddIdentifier(unionRelationHeading,
		idNum++, unionId) ;
	    assert(added == 1) ;
	}
    }

    return unionRelationHeading ;
}

/*
 * Create a new relation heading that is the join of two other headings.  "map"
 * contains the attribute mapping of what is to be joined.  Returns the new
 * relation heading and "attrMap" which is a vector that is the same size as
 * the degree of h2.  Each element contains either -1 if that attribute of h2
 * is not included in the join, or the index into the join heading where that
 * attribute of h2 is to be placed in the joined relation.
 * Caller must delete the returned "attrMap" vector.
 */
Ral_RelationHeading
Ral_RelationHeadingJoin(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2,
    Ral_JoinMap map,
    Ral_IntVector *attrMap,
    Ral_ErrorInfo *errInfo)
{
    int status ;
    Ral_TupleHeading joinTupleHeading ;
    Ral_RelationHeading joinHeading ;
    Ral_TupleHeading h1TupleHeading = h1->tupleHeading ;
    Ral_TupleHeading h2TupleHeading = h2->tupleHeading ;
    Ral_IntVector h2JoinAttrs ;
    Ral_IntVectorIter h2AttrMapIter ;
    Ral_TupleHeadingIter h2TupleHeadingIter ;
    Ral_TupleHeadingIter h2TupleHeadingEnd =
	Ral_TupleHeadingEnd(h2TupleHeading) ;
    int joinIndex = Ral_TupleHeadingSize(h1TupleHeading) ;
    Ral_RelationIdIter id1Iter ;
    Ral_RelationIdIter id1End = Ral_RelationHeadingIdEnd(h1) ;
    Ral_RelationIdIter id2End = Ral_RelationHeadingIdEnd(h2) ;
    int idNum = 0 ;
    /*
     * Construct the tuple heading. The size of the heading is the
     * sum of the degrees of the two relations being joined minus
     * the number of attributes participating in the join.
     */
    joinTupleHeading = Ral_TupleHeadingNew(Ral_RelationHeadingDegree(h1) +
	Ral_RelationHeadingDegree(h2) - Ral_JoinMapAttrSize(map)) ;
    /*
     * The tuple heading contains all the attributes of the first
     * relation.
     */
    status = Ral_TupleHeadingAppend(h1TupleHeading,
	Ral_TupleHeadingBegin(h1TupleHeading),
	Ral_TupleHeadingEnd(h1TupleHeading), joinTupleHeading) ;
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
	Ral_TupleHeadingSize(h2TupleHeading)) ;
    h2AttrMapIter = Ral_IntVectorBegin(h2JoinAttrs) ;
    for (h2TupleHeadingIter = Ral_TupleHeadingBegin(h2TupleHeading) ;
	h2TupleHeadingIter != h2TupleHeadingEnd ; ++h2TupleHeadingIter) {
	if (*h2AttrMapIter) {
	    status = Ral_TupleHeadingAppend(h2TupleHeading,
		h2TupleHeadingIter, h2TupleHeadingIter + 1, joinTupleHeading) ;
	    if (status == 0) {
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
		    (*h2TupleHeadingIter)->name) ;
		Ral_TupleHeadingDelete(joinTupleHeading) ;
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
    /*
     * Construct the relation heading -- infer the identifiers.
     * The maximun number of identifiers is the product of the number
     * of identifiers in the two relations.
     * We adjust the "idCount" later to match what actually turned up.
     */
    joinHeading = Ral_RelationHeadingNew(joinTupleHeading,
	h1->idCount * h2->idCount) ;
    /*
     * Loop through the identifiers for the two heading components
     * and compose new identifiers for the join.
     */
    for (id1Iter = Ral_RelationHeadingIdBegin(h1) ;
	id1Iter != id1End ; ++id1Iter) {
	Ral_IntVector h1Id = *id1Iter ;
	Ral_RelationIdIter id2Iter ;

	for (id2Iter = Ral_RelationHeadingIdBegin(h2) ;
	    id2Iter != id2End ; ++id2Iter) {
	    Ral_IntVector h2Id = *id2Iter ;
	    Ral_IntVector joinId = Ral_IntVectorNewEmpty(
		Ral_IntVectorSize(h1Id) + Ral_IntVectorSize(h2Id)) ;
	    int added ;

	    /*
	     * In general the identifiers of the join are the cross product of
	     * the identifiers of the two relations being joined. However, if
	     * there is any intersection in those identifiers, those
	     * intersecting identifiers must be eliminated from that element of
	     * the cross product.
	     */
	    if (Ral_IsForeignIdASubsetOf(h1, h2, h2Id)) {
		Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
		    Ral_IntVectorEnd(h1Id), joinId,
		    Ral_IntVectorBegin(joinId)) ;
	    } else if (Ral_IsForeignIdASubsetOf(h2, h1, h1Id)) {
		Ral_AddJoinId(joinId, h2Id, map, h2JoinAttrs) ;
	    } else {
		Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
		    Ral_IntVectorEnd(h1Id), joinId,
		    Ral_IntVectorBegin(joinId)) ;
		Ral_AddJoinId(joinId, h2Id, map, h2JoinAttrs) ;
	    }
	    /*
	     * Add the newly formed identifier.
	     * It is possible that that identifier will not add, e.g. when
	     * the joined relations have an identifier subset in common.
	     */
	    added = Ral_RelationHeadingAddIdentifier(joinHeading, idNum,
		joinId) ;
	    if (added == 0) {
		/*
		 * If we didn't add, we need to clean up things.
		 */
		Ral_IntVectorDelete(joinId) ;
	    } else {
		++idNum ;
	    }
	}
    }
    /*
     * Patch up the actual number of identifiers generated.
     */
    joinHeading->idCount = idNum ;

    return joinHeading ;
}

/*
 * Create a new relation heading that is the compose of two other headings.
 * "map" contains the attribute mapping of what is to be joined.  Returns the
 * new relation heading and two attribute maps.  An attribute map is a vector
 * that is the same size as the degree of the corresponding heading.  Each
 * element contains either -1 if that attribute of the heading is not included
 * in the compose, or the index into the compose heading where that attribute
 * of the heading is to be placed in the composed relation.  Caller must delete
 * the returned attribute map vectors.
 */
Ral_RelationHeading
Ral_RelationHeadingCompose(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2,
    Ral_JoinMap map,
    Ral_IntVector *attrMap1,
    Ral_IntVector *attrMap2,
    Ral_ErrorInfo *errInfo)
{
    int status ;
    Ral_TupleHeading composeTupleHeading ;
    Ral_RelationHeading composeHeading ;
    Ral_TupleHeading h1TupleHeading = h1->tupleHeading ;
    Ral_TupleHeading h2TupleHeading = h2->tupleHeading ;
    Ral_IntVector h1JoinAttrs ;
    Ral_IntVector h2JoinAttrs ;
    Ral_IntVectorIter attrMapIter ;
    Ral_TupleHeadingIter tupleHeadingIter ;
    Ral_TupleHeadingIter tupleHeadingEnd ;
    int composeIndex = 0 ;

    Ral_RelationIdIter id1Iter ;
    Ral_RelationIdIter id1End = Ral_RelationHeadingIdEnd(h1) ;
    Ral_RelationIdIter id2Iter ;
    Ral_RelationIdIter id2End = Ral_RelationHeadingIdEnd(h2) ;
    int idNum = 0 ;
    /*
     * Construct the tuple heading. The size of the heading is the
     * sum of the degrees of the two relations being composeed minus
     * twice the number of attributes participating in the compose.
     * Twice because we are eliminating all the compose attributes.
     */
    composeTupleHeading = Ral_TupleHeadingNew(Ral_RelationHeadingDegree(h1) +
	Ral_RelationHeadingDegree(h2) - 2 * Ral_JoinMapAttrSize(map)) ;
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
	Ral_TupleHeadingSize(h1TupleHeading)) ;
    attrMapIter = Ral_IntVectorBegin(h1JoinAttrs) ;
    tupleHeadingEnd = Ral_TupleHeadingEnd(h1TupleHeading) ;

    for (tupleHeadingIter = Ral_TupleHeadingBegin(h1TupleHeading) ;
	tupleHeadingIter != tupleHeadingEnd ; ++tupleHeadingIter) {
	if (*attrMapIter) {
	    status = Ral_TupleHeadingAppend(h1TupleHeading,
		tupleHeadingIter, tupleHeadingIter + 1, composeTupleHeading) ;
	    if (status == 0) {
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
		    (*tupleHeadingIter)->name) ;
		Ral_TupleHeadingDelete(composeTupleHeading) ;
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
	Ral_TupleHeadingSize(h2TupleHeading)) ;
    attrMapIter = Ral_IntVectorBegin(h2JoinAttrs) ;
    tupleHeadingEnd = Ral_TupleHeadingEnd(h2TupleHeading) ;

    for (tupleHeadingIter = Ral_TupleHeadingBegin(h2TupleHeading) ;
	tupleHeadingIter != tupleHeadingEnd ; ++tupleHeadingIter) {
	if (*attrMapIter) {
	    status = Ral_TupleHeadingAppend(h2TupleHeading,
		tupleHeadingIter, tupleHeadingIter + 1, composeTupleHeading) ;
	    if (status == 0) {
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
		    (*tupleHeadingIter)->name) ;
		Ral_TupleHeadingDelete(composeTupleHeading) ;
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
    /*
     * Construct the relation heading -- infer the identifiers.
     * The maximun number of identifiers is the product of the number
     * of identifiers in the two relations.
     * We adjust the "idCount" later to match what actually turned up.
     */
    composeHeading = Ral_RelationHeadingNew(composeTupleHeading,
	h1->idCount * h2->idCount) ;
    /*
     * Loop through the identifiers for the two heading components
     * and infer new identifiers for the compose.
     * For compose we must eliminate any identifier that contains an attribute
     * that is one of the composed attributes.
     */
    for (id1Iter = Ral_RelationHeadingIdBegin(h1) ;
	id1Iter != id1End ; ++id1Iter) {
	Ral_IntVector h1Id = *id1Iter ;

	int id1Mapped = Ral_IdContainsMappedAttr(h1Id, h1JoinAttrs) ;

	for (id2Iter = Ral_RelationHeadingIdBegin(h2) ;
	    id2Iter != id2End ; ++id2Iter) {
	    Ral_IntVector h2Id = *id2Iter ;
	    Ral_IntVector composeId = Ral_IntVectorNewEmpty(
		Ral_IntVectorSize(h1Id) + Ral_IntVectorSize(h2Id)) ;
	    int added ;
	    int id2Mapped = Ral_IdContainsMappedAttr(h2Id, h2JoinAttrs) ;

	    /*
	     * In general the identifiers of the compose are the cross product
	     * of the identifiers of the two relations being composed.
	     * However, if there is any intersection in those identifiers,
	     * those intersecting identifiers must be eliminated from that
	     * element of the cross product.
	     */
	    if (id1Mapped && id2Mapped) {
		/*
		 * Both id's contain composed attributes.
		 */
		continue ;
	    } else if (id1Mapped) {
		Ral_AddJoinId(composeId, h2Id, map, h2JoinAttrs) ;
	    } else if (id2Mapped) {
		Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
		    Ral_IntVectorEnd(h1Id), composeId,
		    Ral_IntVectorBegin(composeId)) ;
	    } else {
		/*
		 * Neither id contains composed attributes.
		 */
		if (Ral_IsForeignIdASubsetOf(h1, h2, h2Id)) {
		    Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
			Ral_IntVectorEnd(h1Id), composeId,
			Ral_IntVectorBegin(composeId)) ;
		} else if (Ral_IdContainsMappedAttr(h1Id, h1JoinAttrs) ||
			    Ral_IsForeignIdASubsetOf(h2, h1, h1Id)) {
		    Ral_AddJoinId(composeId, h2Id, map, h2JoinAttrs) ;
		} else {
		    Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
			Ral_IntVectorEnd(h1Id), composeId,
			Ral_IntVectorBegin(composeId)) ;
		    Ral_AddJoinId(composeId, h2Id, map, h2JoinAttrs) ;
		}
	    }
	    /*
	     * Add the newly formed identifier.
	     * It is possible that that identifier will not add, e.g. when
	     * the composed relations have an identifier subset in common.
	     */
	    added = Ral_RelationHeadingAddIdentifier(composeHeading, idNum,
		composeId) ;
	    if (added == 0) {
		/*
		 * If we didn't add, we need to clean up things.
		 */
		Ral_IntVectorDelete(composeId) ;
	    } else {
		++idNum ;
	    }
	}
    }
    if (idNum == 0) {
	/*
	 * In this case we didn't find any suitable identifiers and we must
	 * create one that has all the attributes.
	 */
	Ral_IntVector allId = Ral_IntVectorNew(
	    Ral_RelationHeadingDegree(composeHeading), 0) ;
	Ral_IntVectorFillConsecutive(allId, 0) ;
#	ifndef NDEBUG
	int added = Ral_RelationHeadingAddIdentifier(composeHeading, idNum,
	    allId) ;
	assert(added != 0) ;
#	else
	Ral_RelationHeadingAddIdentifier(composeHeading, idNum, allId) ;
#	endif
	idNum = 1 ;
    }
    /*
     * Patch up the actual number of identifiers generated.
     */
    composeHeading->idCount = idNum ;

    return composeHeading ;
}

int
Ral_RelationHeadingScan(
    Ral_RelationHeading h,
    Ral_AttributeTypeScanFlags *flags)
{
    int nBytes ;
    Ral_RelationIdIter idIter = Ral_RelationHeadingIdBegin(h) ;
    Ral_RelationIdIter idEnd = Ral_RelationHeadingIdEnd(h) ;
    int needIdList = h->idCount > 1 || Ral_IntVectorSize(*idIter) != 1 ;
    int length = strlen(Ral_RelationObjType.name) + 1 ; /* +1 for space */

    assert(flags->attrType == Relation_Type) ;
    assert(flags->flags.compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the type flags for each attribute.
     */
    flags->flags.compoundFlags.count = Ral_RelationHeadingDegree(h) ;
    nBytes = flags->flags.compoundFlags.count
	    * sizeof(*flags->flags.compoundFlags.flags) ;
    flags->flags.compoundFlags.flags =
	    (Ral_AttributeTypeScanFlags *)ckalloc(nBytes) ;
    memset(flags->flags.compoundFlags.flags, 0, nBytes) ;
    /*
     * The attribute / type list of the heading.
     */
    length += Ral_TupleHeadingScanAttr(h->tupleHeading, flags) ;
    /*
     * Scan the identifiers
     */
    length += 1 ; /* separating space */
    if (needIdList) {
	length += sizeof(openList) ;
    }
    for (; idIter != idEnd ; ++idIter) {
	Ral_IntVector id = *idIter ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
	int nIdAttrs = Ral_IntVectorSize(id) ;
	Ral_IntVectorIter iter ;

	if (nIdAttrs != 1) {
	    length += sizeof(openList) ;
	}
	for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	    /*
	     * Reuse the length stored when the attribute name was scanned.
	     */
	    assert(*iter < flags->flags.compoundFlags.count) ;
	    /*
	     * +1 for the separating space
	     */
	    length += flags->flags.compoundFlags.flags[*iter].nameLength + 1 ;
	}
	if (Ral_IntVectorSize(id) != 0) {
	    length -= 1 ;
	}
	if (nIdAttrs != 1) {
	    length += sizeof(closeList) ;
	    length += 1 ; /* separating space */
	}
	length += 1 ; /* separating space */
    }
    if (h->idCount > 0) {
	length -= 1 ; /* remove trailing space */
    }
    if (needIdList) {
	length += sizeof(closeList) ;
    }

    return length ;
}

int
Ral_RelationHeadingConvert(
    Ral_RelationHeading h,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    char *p = dst ;
    Ral_RelationIdIter idIter = Ral_RelationHeadingIdBegin(h) ;
    Ral_RelationIdIter idEnd = Ral_RelationHeadingIdEnd(h) ;
    int needIdList = h->idCount > 1 || Ral_IntVectorSize(*idIter) != 1 ;
    Ral_TupleHeading tupleHeading = h->tupleHeading ;

    assert(flags->attrType == Relation_Type) ;
    assert(flags->flags.compoundFlags.count == Ral_RelationHeadingDegree(h)) ;

    /*
     * Copy in the "Relation" keyword.
     */
    strcpy(p, relationKeyword) ;
    p += strlen(relationKeyword) ;
    /*
     * Add the heading.
     */
    *p++ = ' ' ;
    p += Ral_TupleHeadingConvertAttr(tupleHeading, p, flags) ;
    /*
     * Add the identifiers.
     */
    *p++ = ' ' ;
    if (needIdList) {
	*p++ = openList ;
    }
    for (; idIter != idEnd ; ++idIter) {
	Ral_IntVector id = *idIter ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
	int nIdAttrs = Ral_IntVectorSize(id) ;
	Ral_IntVectorIter iter ;

	if (nIdAttrs != 1) {
	    *p++ = openList ;
	}
	for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	    Ral_Attribute attr = Ral_TupleHeadingFetch(tupleHeading, *iter) ;
	    /*
	     * Reuse the flags gathered during the scanning of the header.
	     */
	    assert(*iter < flags->flags.compoundFlags.count) ;
	    p += Tcl_ConvertElement(attr->name, p,
		flags->flags.compoundFlags.flags[*iter].nameFlags) ;
	    *p++ = ' ' ;
	}
	if (Ral_IntVectorSize(id) != 0) {
	    --p ;
	}
	if (nIdAttrs != 1) {
	    *p++ = closeList ;
	}
	*p++ = ' ' ;
    }
    /*
     * Overwrite any trailing blank. There will be no trailing blank if the
     * heading didn't have any identifiers.
     */
    if (h->idCount > 0) {
	--p ;
    }
    if (needIdList) {
	*p++ = closeList ;
    }

    return p - dst ;
}

void
Ral_RelationHeadingPrint(
    Ral_RelationHeading h,
    const char *format,
    FILE *f)
{
    char *str = Ral_RelationHeadingStringOf(h) ;

    fprintf(f, format, str) ;
    ckfree(str) ;
}

/*
 * Caller must free the returned memory.
 */
char *
Ral_RelationHeadingStringOf(
    Ral_RelationHeading h)
{
    Ral_AttributeTypeScanFlags flags ;
    char *str ;
    int length ;

    memset(&flags, 0, sizeof(flags)) ;
    flags.attrType = Relation_Type ;
    /*
     * +1 for the NUL terminator
     */
    str = ckalloc(Ral_RelationHeadingScan(h, &flags) + 1) ;
    length = Ral_RelationHeadingConvert(h, str, &flags) ;
    str[length] = '\0' ;
    Ral_AttributeTypeScanFlagsFree(&flags) ;

    return str ;
}

const char *
Ral_RelationHeadingVersion(void)
{
    return rcsid ;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * Determine if the given "id" is a subset of any identifier in "heading".
 */
static int
Ral_IsIdASubsetOf(
    Ral_RelationHeading heading,
    Ral_IntVector id)
{
    int idCount = heading->idCount ;
    Ral_IntVector *idArray = heading->identifiers ;

    /*
     * Make sure the identifier is in canonical order.
     */
    Ral_IntVectorSort(id) ;
    /*
     * Iterate through the identifiers of the heading and make sure that the
     * given one is not equal to or a subset of an existing identifier.
     */
    while (idCount-- > 0) {
	Ral_IntVector headingId = *idArray++ ;

	if (headingId &&
	    (Ral_IntVectorSubsetOf(id, headingId) ||
	     Ral_IntVectorSubsetOf(headingId, id))) {
	    return 1 ;
	}
    }
    return 0 ;
}

/*
 * Determine if the identifier from "h2" is a subset of any identifier
 * from "h1".
 */
static int
Ral_IsForeignIdASubsetOf(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2,
    Ral_IntVector h2Id)
{
    Ral_IntVectorIter id2Iter ;
    Ral_IntVectorIter id2End = Ral_IntVectorEnd(h2Id) ;
    Ral_IntVector h1Id = Ral_IntVectorNew(Ral_IntVectorSize(h2Id), -1) ;
    Ral_IntVectorIter h1Iter = Ral_IntVectorBegin(h1Id) ;
    Ral_TupleHeading th1 = h1->tupleHeading ;
    Ral_TupleHeading th2 = h2->tupleHeading ;
    int isSubset ;

    /*
     * Loop through the indices of the id, find the attribute name
     * and look up that attribute in h1. If the attribute is not found,
     * then the id cannot possibly be a subset of an id in h1.
     */
    for (id2Iter = Ral_IntVectorBegin(h2Id) ; id2Iter != id2End ; ++id2Iter) {
	Ral_Attribute h2Attr = Ral_TupleHeadingFetch(th2, *id2Iter) ;
	int h1Index = Ral_TupleHeadingIndexOf(th1, h2Attr->name) ;

	if (h1Index >= 0) {
	    *h1Iter++ = h1Index ;
	} else {
	    Ral_IntVectorDelete(h1Id) ;
	    return 0 ;
	}
    }
    /*
     * At this point, h1Id contains the attribute indices of the identifier
     * relative to "h1". Now we need to find out if this is a subset of
     * any of the identifiers of h1.
     */
    isSubset = Ral_IsIdASubsetOf(h1, h1Id) ;
    Ral_IntVectorDelete(h1Id) ;
    return isSubset ;
}

/*
 * Add identifier attributes to form a new identifier for join.
 * This is used to add attributes from the second relation of a join.
 */
static void
Ral_AddJoinId(
    Ral_IntVector joinId,	/* Identifier being built */
    Ral_IntVector id,		/* Identifier to add */
    Ral_JoinMap map,		/* Join map of the join */
    Ral_IntVector joinAttrs)	/* Map of what attribute are in the join */
{
    Ral_IntVectorIter idIter ;
    Ral_IntVectorIter idEnd = Ral_IntVectorEnd(id) ;
    /*
     * The identifier indices from the second header must
     * be corrected for the place where the attributes will end up.
     * If the identifier is to be eliminated, the we have to find
     * the corresponding attribute in the first relation.
     */
    for (idIter = Ral_IntVectorBegin(id) ; idIter != idEnd ; ++idIter) {
	Ral_IntVectorValueType index =
	    Ral_IntVectorFetch(joinAttrs, *idIter) ;
	if (index >= 0) {
	    Ral_IntVectorPushBack(joinId, index) ;
	} else {
	    /*
	     * Attribute in the second relation is to be eliminated.  Find its
	     * corresponding attribute in the first and it will become part of
	     * the identifier (if it is not already).
	     */
	    int attrInr1 = Ral_JoinMapFindAttr(map, 1, *idIter) ;
	    assert(attrInr1 != -1) ;
	    /*
	     * Add the corresponding attribute in r1 in a set fashion in case
	     * it is already part of this identifier.
	     */
	    Ral_IntVectorSetAdd(joinId, attrInr1) ;
	}
    }
}

static int
Ral_IdContainsMappedAttr(
    Ral_IntVector id,
    Ral_IntVector attrMap)
{
    Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
    Ral_IntVectorIter iter ;

    /*
     * Look through the id vector and see if any of the attribute
     * indices are part of the compose attributes. If so, then
     * this identifier cannot be used an part of an identifier for
     * the composition.
     */
    for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	if (Ral_IntVectorFetch(attrMap, *iter) != -1) {
	    return 0 ;
	}
    }

    return 1 ;
}
