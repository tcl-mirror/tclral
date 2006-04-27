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
$Revision: 1.12 $
$Date: 2006/04/27 14:48:56 $
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

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationheading.c,v $ $Revision: 1.12 $" ;

static const char relationKeyword[] = "Relation" ;
static const char openList = '{' ;
static const char closeList = '}' ;

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
    if (!tupleHeading) {
	return NULL ;
    }

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
    Ral_TupleHeading extTupleHeading)
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
    extHeading = Ral_RelationHeadingNew(extTupleHeading, heading->idCount) ;
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
	int id2Index = 0 ;
	int id2Count ;
	Ral_IntVector *id2Array ;
	int found ;

	id1 = *id1Array++ ;
	end1Iter = Ral_IntVectorEnd(id1) ;
	/*
	 * Construct a new vector of attribute indices using the
	 * names from the first heading to find its corresponding
	 * index in the second heading.
	 */
	id2 = Ral_IntVectorNew(Ral_IntVectorSize(id1), -1) ;
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
	    Ral_IntVectorStore(id2, id2Index++, attr2Index) ;
	}
	/*
	 * Sort the identifier indices into canonical order.
	 */
	Ral_IntVectorSort(id2) ;
	/*
	 * Now we search the identifiers of the second heading to find
	 * a match to the vector we just constructed.
	 */
	id2Array = h2->identifiers ;
	found = 0 ;
	for (id2Count = h2->idCount ; id2Count > 0 ; --id2Count) {
	    Ral_IntVector heading2Id = *id2Array++ ;
	    /*
	     * All of the identifiers should have been added.
	     */
	    assert(heading2Id != NULL) ;
	    if (Ral_IntVectorEqual(id2, heading2Id)) {
		found = 1 ;
		break ;
	    }
	}
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

int
Ral_RelationHeadingAddIdentifier(
    Ral_RelationHeading heading,
    int idNum,
    Ral_IntVector id)
{
    int idCount = heading->idCount ;
    Ral_IntVector *idArray = heading->identifiers ;

    assert (idNum < heading->idCount) ;
    assert (heading->identifiers[idNum] == NULL) ;
    /*
     * Make sure the identifier is in canonical order.
     */
    Ral_IntVectorSort(id) ;
    /*
     * Only add the identifier if it is not a subset of an existing
     * identifier. So we iterate through the identifiers of the heading
     * and make sure that the new one is not equal to or a subset of
     * an existing identifier.
     */
    while (idCount-- > 0) {
	Ral_IntVector headingId = *idArray++ ;

	if (headingId && Ral_IntVectorSubsetOf(id, headingId)) {
	    return 0 ;
	}
    }
    /*
     * Take possession of the identifier vector.
     */
    heading->identifiers[idNum] = id ;
    return 1 ;
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

    /*
     * Make sure the candidate identifier is in canonical order.
     */
    Ral_IntVectorSort(id) ;
    /*
     * Iterate through the identifies in the heading looking for a match.
     */
    assert(idCount >= 1) ;
    for (idArray = heading->identifiers ; idCount > 0 ; --idCount, ++idArray) {
	Ral_IntVector headingId = *idArray ;

	if (headingId && Ral_IntVectorEqual(id, headingId)) {
	    return idArray - heading->identifiers ;
	}
    }
    return -1 ;
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
    Ral_IntVector *attrMap)
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
     * The tuple heading contains all the attributes of the second relations
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
		Ral_RelationLastError = REL_DUPLICATE_ATTR ;
		Ral_TupleHeadingDelete(joinTupleHeading) ;
		Ral_IntVectorDelete(h2JoinAttrs) ;
		*attrMap = NULL ;
		return NULL ;
	    }
	    *h2AttrMapIter++ = joinIndex++ ;
	} else {
	    /*
	     * Attributes that don't appear in the results are given
	     * a final index of -1
	     */
	    *h2AttrMapIter++ = -1 ;
	}
    }
    /*
     * Construct the relation heading -- infer the identifiers.
     * The identifiers for the join are the cross product of the identifiers
     * of the two relations, minus any join attributes of r2.
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
	    Ral_IntVectorIter h2IdIter ;
	    Ral_IntVectorIter h2IdEnd = Ral_IntVectorEnd(h2Id) ;
	    Ral_IntVector joinId = Ral_IntVectorNewEmpty(
		Ral_IntVectorSize(h1Id) + Ral_IntVectorSize(h2Id)) ;
	    int added ;
	    /*
	     * Copy the identifier indices from the first header.
	     * These indices are correct for the join header also.
	     */
	    Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
		Ral_IntVectorEnd(h1Id), joinId, Ral_IntVectorBegin(joinId)) ;
	    /*
	     * The identifier indices from the second header must
	     * be corrected for the place where the attributes will end up.
	     * If the identifier is to be eliminated, the we have to find
	     * the corresponding attribute in the first relation.
	     */
	    for (h2IdIter = Ral_IntVectorBegin(h2Id) ; h2IdIter != h2IdEnd ;
		++h2IdIter) {
		Ral_IntVectorValueType index = Ral_IntVectorFetch(h2JoinAttrs,
		    *h2IdIter) ;
		if (index >= 0) {
		    Ral_IntVectorPushBack(joinId, index) ;
		} else {
		    /*
		     * Identifier in the second relation is to be eliminated.
		     * Find its corresponding attribute in the first and
		     * it will become an identifier (if it is not already).
		     */
		    int attrInr1 = Ral_JoinMapFindAttr(map, 1, *h2IdIter) ;
		    assert(attrInr1 != -1) ;
		    /*
		     * Add the corresponding attribute in r1 in a set fashion
		     * in case it is already part of this identifier.
		     */
		    Ral_IntVectorSetAdd(joinId, attrInr1) ;
		}
	    }
	    /*
	     * Add the newly formed identifier.
	     * It should always add.
	     */
	    added = Ral_RelationHeadingAddIdentifier(joinHeading, idNum++,
		joinId) ;
	    assert(added == 1) ;
	}
    }

    return joinHeading ;
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
    assert(flags->compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the type flags for each attribute.
     */
    flags->compoundFlags.count = Ral_RelationHeadingDegree(h) ;
    nBytes = flags->compoundFlags.count * sizeof(*flags->compoundFlags.flags) ;
    flags->compoundFlags.flags = (Ral_AttributeTypeScanFlags *)ckalloc(nBytes) ;
    memset(flags->compoundFlags.flags, 0, nBytes) ;
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
	    assert(*iter < flags->compoundFlags.count) ;
	    /*
	     * +1 for the separating space
	     */
	    length += flags->compoundFlags.flags[*iter].nameLength + 1 ;
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
    assert(flags->compoundFlags.count == Ral_RelationHeadingDegree(h)) ;

    /*
     * Copy in the "Relation" keyword.
     */
    strcpy(p, relationKeyword) ;
    p += sizeof(relationKeyword) - 1 ;
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
	    assert(*iter < flags->compoundFlags.count) ;
	    p += Tcl_ConvertElement(attr->name, p,
		flags->compoundFlags.flags[*iter].nameFlags) ;
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
 * Call must free the returned memory.
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
