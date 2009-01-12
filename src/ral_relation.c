/*
This software is copyrighted 2005, 2006, 2007, 2008, 2009 by G. Andrew Mangogna.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files.

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

$RCSfile: ral_relation.c,v $
$Revision: 1.36.2.1 $
$Date: 2009/01/12 00:45:36 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_relation.h"
#include "ral_tupleheading.h"
#include "ral_tuple.h"
#include "ral_relationobj.h"
#include "ral_tupleobj.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
MACRO DEFINITIONS
*/

/*
TYPE DEFINITIONS
*/
typedef int (*Ral_CmpFunc)(Tcl_Obj *, Tcl_Obj *) ;
typedef struct {
    Ral_Relation relation ;
    Ral_IntVector attrs ;
    Ral_IntVector sortVect ;
    int sortDirection ;	    /* 0 ==> ascending, 1 ==> descending */
} Ral_RelSortProps ;

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static int Ral_HeadingMatch(Ral_TupleHeading, Ral_TupleHeading,
        Ral_ErrorInfo *) ;
static int Ral_RelationFindTupleReference(Ral_Relation, Ral_Tuple,
        Ral_IntVector) ;
static int Ral_RelationIndexTuple(Ral_Relation, Ral_Tuple, Ral_RelationIter) ;
static void Ral_RelationGetJoinMapKey(Ral_Tuple, Ral_JoinMap, int,
        Tcl_DString *) ;
static int Ral_RelationFindJoinId(Ral_Relation, Ral_JoinMap, int) ;

static unsigned int tupleHashKeys(Tcl_HashTable *, void *) ;
static int tupleCompareKeys(void *, Tcl_HashEntry *) ;
static Tcl_HashEntry *tupleEntryAlloc(Tcl_HashTable *, void *) ;
static void tupleEntryFree(Tcl_HashEntry *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char openList = '{' ;
static const char closeList = '}' ;

static Tcl_HashKeyType const tupleHashType = {
    TCL_HASH_KEY_TYPE_VERSION,  /* version */
    TCL_HASH_KEY_RANDOMIZE_HASH,/* flags */
    tupleHashKeys,              /* hashKeyProc */
    tupleCompareKeys,           /* compareKeysProc */
    tupleEntryAlloc,            /* allocEntryProc */
    tupleEntryFree,             /* freeEntryProc */
} ;

/*
FUNCTION DEFINITIONS
*/

Ral_Relation
Ral_RelationNew(
    Ral_TupleHeading heading)
{
    Ral_Relation relation ;

    relation = (Ral_Relation)ckalloc(sizeof(*relation)) ;
    memset(relation, 0, sizeof(*relation)) ;

    Ral_TupleHeadingReference(relation->heading = heading) ;
    Tcl_InitCustomHashTable(&relation->index, TCL_CUSTOM_PTR_KEYS,
            &tupleHashType) ;

    return relation ;
}

/*
 * This function fully duplicates a relation. A new relation heading,
 * tuple heading and complete set of tuples is replicated.
 */
Ral_Relation
Ral_RelationDup(
    Ral_Relation srcRelation)
{
    Ral_TupleHeading dupHeading ;
    Ral_Relation dupRelation ;
    Ral_RelationIter srcIter ;

    dupHeading = Ral_TupleHeadingDup(srcRelation->heading) ;
    dupRelation = Ral_RelationNew(dupHeading) ;
    Ral_RelationReserve(dupRelation, Ral_RelationCardinality(srcRelation)) ;

    for (srcIter = srcRelation->start ; srcIter != srcRelation->finish ;
            ++srcIter) {
	Ral_Tuple srcTuple = *srcIter ;
	Ral_Tuple dupTuple ;
	int appended ;

	dupTuple = Ral_TupleNew(dupRelation->heading) ;
	Ral_TupleCopyValues(srcTuple->values,
	    srcTuple->values + Ral_RelationDegree(srcRelation),
	    dupTuple->values) ;

	appended = Ral_RelationPushBack(dupRelation, dupTuple, NULL) ;
	assert(appended != 0) ;
    }

    return dupRelation ;
}

/*
 * This function makes a new relation but reference counts as much
 * as possible. The resulting relation cannot be subsequently modified
 * in place.
 */
Ral_Relation
Ral_RelationShallowCopy(
    Ral_Relation srcRelation)
{
    Ral_Relation dupRelation ;
    Ral_RelationIter srcIter ;
    Ral_RelationIter srcEnd = srcRelation->finish ;

    dupRelation = Ral_RelationNew(srcRelation->heading) ;
    Ral_RelationReserve(dupRelation, Ral_RelationCardinality(srcRelation)) ;

    for (srcIter = srcRelation->start ; srcIter != srcEnd ; ++srcIter) {
	int appended ;

	appended = Ral_RelationPushBack(dupRelation, *srcIter, NULL) ;
	assert(appended != 0) ;
    }

    return dupRelation ;
}

void
Ral_RelationDelete(
    Ral_Relation relation)
{
    Ral_RelationIter iter ;

    for (iter = relation->start ; iter != relation->finish ; ++iter) {
	Ral_TupleUnreference(*iter) ;
    }
    Tcl_DeleteHashTable(&relation->index) ;

    if (relation->start) {
	ckfree((char *)relation->start) ;
    }
    Ral_TupleHeadingUnreference(relation->heading) ;
    ckfree((char *)relation) ;
}

void
Ral_RelationReserve(
    Ral_Relation relation,
    int size)
{
    if (Ral_RelationCapacity(relation) < size) {
	int oldSize = Ral_RelationCardinality(relation) ;
	relation->start = (Ral_RelationIter)ckrealloc((char *)relation->start, 
	    size * sizeof(*relation->start)) ;
	relation->finish = relation->start + oldSize ;
	relation->endStorage = relation->start + size ;
    }
}

/*
 * Insert a tuple at the end of relation. Keeps the index up to date.
 */
int
Ral_RelationPushBack(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    Ral_Tuple insertTuple ;
    int status = 0 ;

    assert(Ral_TupleHeadingEqual(relation->heading, tuple->heading)) ;

    if (relation->finish >= relation->endStorage) {
	int oldCapacity = Ral_RelationCapacity(relation) ;
	/*
	 * Increase the capacity by half again. +1 to make sure
	 * we allocate at least one slot if this is the first time
	 * we are pushing onto an empty vector.
	 */
	Ral_RelationReserve(relation, oldCapacity + oldCapacity / 2 + 1) ;
    }
    /*
     * Manage the references. Here we reference the tuple and then dereference
     * it after we decide if it need to be duplicated in a different order. The
     * net number of references is 0, but if the tuple has no other references
     * it will be discarded (i.e. this function assumes ownership of a newly
     * created tuple).  Further we reference the "insertTuple" and dereference
     * it after the insertion attempt.  If the insertion succeeds, then an
     * additional reference is acquired resulting in a net number of references
     * of 1. If the insertion fails, then the dereference will delete the
     * reordered tuple if "orderMap" is non-NULL.  Note that if tuple is
     * created with zero references and passed here, then failing to insert
     * "tuple" will result in it being deleted. Consequently, the caller need
     * not manage the lifetime of any tuple passed into this function.
     */
    Ral_TupleReference(tuple) ;
    insertTuple = orderMap ?
	Ral_TupleDupOrdered(tuple, relation->heading, orderMap) : tuple ;
    Ral_TupleReference(insertTuple) ;

    if (Ral_RelationIndexTuple(relation, insertTuple, relation->finish)) {
	Ral_TupleReference(*relation->finish++ = insertTuple) ;
	status = 1 ;
    }

    Ral_TupleUnreference(insertTuple) ;
    Ral_TupleUnreference(tuple) ;

    return status ;
}

Ral_Tuple
Ral_RelationTupleAt(
    Ral_Relation relation,
    int offset)
{
    if (relation->start + offset >= relation->finish) {
	Tcl_Panic("Ral_RelationTupleAt: attempt to access non-existant tuple "
	    "at offset %d", offset) ;
    }
    return *(relation->start + offset) ;
}

/*
 * Update the tuple at "pos"
 */
int
Ral_RelationUpdate(
    Ral_Relation relation,
    Ral_RelationIter pos,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    int result = 1 ;
    Ral_Tuple newTuple ;

    if (pos >= relation->finish) {
	Tcl_Panic("Ral_RelationUpdate: attempt to update non-existant tuple") ;
    }

    assert(*pos != NULL) ;
    assert(pos < Ral_RelationEnd(relation)) ;

    /*
     * Remove the index value for the identifiers for the tuple that
     * is already in place.
     */
    // Ral_RelationRemoveTupleIndex(relation, *pos) ;
    /*
     * If reordering is required, we create a new tuple with the correct
     * attribute ordering to match the relation.
     */
    newTuple = orderMap ?
	Ral_TupleDupOrdered(tuple, relation->heading, orderMap) : tuple ;
    /*
     * Compute the indices for each identifier.
     */
    if (Ral_RelationIndexTuple(relation, newTuple, pos)) {
	/*
	 * If the new tuple is unique, then discard the old one and
	 * install the new one into the same place. Note that we increment
	 * the reference count of the new tuple before decrementing the
	 * reference count of the old one, just in case that they are the
	 * same tuple (i.e. we are updating the same physical location).
	 */
	Ral_TupleReference(newTuple) ;
	Ral_TupleUnreference(*pos) ;
	*pos = newTuple ;
    } else {
	/*
	 * The indexing failed ==> that the new tuple is not unique in
	 * its identifying attribute values. Restore the old tuple.
	 * If we can't compute an index for the old tuple value, then
	 * there has been a tear in the space / time continuum.
	 */
	if (!Ral_RelationIndexTuple(relation, *pos, pos)) {
	    Tcl_Panic("Ral_RelationUpdate: recovery failure on tuple, \"%s\"",
		Ral_TupleStringOf(*pos)) ;
	}
	result = 0 ;
    }

    return result ;
}

Ral_Relation
Ral_RelationUnion(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation unionRel ;
    int copyStatus ;
    Ral_IntVector orderMap ;
    /*
     * Headings must be equal to perform a union.
     */
    if (!Ral_HeadingMatch(r1->heading, r2->heading, errInfo)) {
	return NULL ;
    }
    unionRel = Ral_RelationNew(r1->heading) ;
    /*
     * Everything in the first relation is in the union, so just copy it in.
     * No reordering necessary.
     */
    copyStatus = Ral_RelationCopy(r1, Ral_RelationBegin(r1),
	Ral_RelationEnd(r1), unionRel, NULL) ;
    assert(copyStatus == 1) ;
    /*
     * Reordering may be necessary.
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r1->heading, r2->heading) ;
    /*
     * Copy in the tuples from the second relation.  Ignore any return status.
     * If there is already a matching tuple it will not be inserted.
     */
    Ral_RelationCopy(r2, Ral_RelationBegin(r2), Ral_RelationEnd(r2), unionRel,
	orderMap) ;

    Ral_IntVectorDelete(orderMap) ;

    return unionRel ;
}

Ral_Relation
Ral_RelationIntersect(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation intersectRel ;
    Ral_IntVector orderMap ;
    Ral_RelationIter iter1 ;
    Ral_RelationIter end1 = Ral_RelationEnd(r1) ;
    Ral_RelationIter end2 = Ral_RelationEnd(r2) ;
    /*
     * Headings must be equal to perform a union.
     */
    if (!Ral_HeadingMatch(r1->heading, r2->heading, errInfo)) {
	return NULL ;
    }
    /*
     * Reordering may be necessary. Since we will search
     * "r2" to find matching tuples from "r1", we must compute
     * the map onto "r2" from "r1".
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r2->heading, r1->heading) ;

    intersectRel = Ral_RelationNew(r1->heading) ;
    /*
     * Iterate through the tuples in the first relation and search
     * the second one for a match. Only tuples contained in both relations
     * are added to the intersection.
     */
    for (iter1 = Ral_RelationBegin(r1) ; iter1 != end1 ; ++iter1) {
	if (Ral_RelationFind(r2, *iter1, orderMap) != end2) {
	    /*
	     * No reordering is necessary since we know the heading of the
	     * intersection is the same as r1.
	     */
	    Ral_RelationPushBack(intersectRel, *iter1, NULL) ;
	}
    }

    Ral_IntVectorDelete(orderMap) ;

    return intersectRel ;
}

Ral_Relation
Ral_RelationMinus(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation diffRel ;
    Ral_IntVector orderMap ;
    Ral_RelationIter iter1 ;
    Ral_RelationIter end1 = Ral_RelationEnd(r1) ;
    Ral_RelationIter end2 = Ral_RelationEnd(r2) ;
    /*
     * Headings must be equal to perform a union.
     */
    if (!Ral_HeadingMatch(r1->heading, r2->heading, errInfo)) {
	return NULL ;
    }
    /*
     * Reordering may be necessary. Since we will search
     * "r2" to find matching tuples from "r1", we must compute
     * the map onto "r2" from "r1".
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r2->heading, r1->heading) ;

    diffRel = Ral_RelationNew(r1->heading) ;
    /*
     * Iterate through the tuples in the first relation and search the second
     * one for a match. Only tuples not contained in the second relation are
     * added to the difference.
     */
    for (iter1 = Ral_RelationBegin(r1) ; iter1 != end1 ; ++iter1) {
	if (Ral_RelationFind(r2, *iter1, orderMap) == end2) {
	    /*
	     * No reordering is necessary since we know the heading of the
	     * difference is the same as r1.
	     */
	    Ral_RelationPushBack(diffRel, *iter1, NULL) ;
	}
    }

    Ral_IntVectorDelete(orderMap) ;

    return diffRel ;
}

Ral_Relation
Ral_RelationTimes(
    Ral_Relation multiplicand,
    Ral_Relation multiplier)
{
    Ral_TupleHeading prodHeading ;
    Ral_Relation product ;
    Ral_RelationIter mcandIter ;
    Ral_RelationIter mcandEnd = Ral_RelationEnd(multiplicand) ;
    Ral_RelationIter mlierIter ;
    Ral_RelationIter mlierEnd = Ral_RelationEnd(multiplier) ;
    int mlierOffset = Ral_RelationDegree(multiplicand) ;

    prodHeading = Ral_TupleHeadingUnion(multiplicand->heading,
	multiplier->heading) ;
    if (!prodHeading) {
	return NULL ;
    }

    product = Ral_RelationNew(prodHeading) ;
    Ral_RelationReserve(product, Ral_RelationCardinality(multiplicand) *
	Ral_RelationCardinality(multiplier)) ;

    for (mcandIter = Ral_RelationBegin(multiplicand) ; mcandIter != mcandEnd ;
	++mcandIter) {
	for (mlierIter = Ral_RelationBegin(multiplier) ; mlierIter != mlierEnd ;
	    ++mlierIter) {
	    Ral_Tuple mcandTuple = *mcandIter ;
	    Ral_Tuple mlierTuple = *mlierIter ;
	    Ral_Tuple prodTuple = Ral_TupleNew(prodHeading) ;
	    Ral_TupleIter prodTupleBegin = Ral_TupleBegin(prodTuple) ;

	    Ral_TupleCopyValues(Ral_TupleBegin(mcandTuple),
		Ral_TupleEnd(mcandTuple), prodTupleBegin) ;
	    Ral_TupleCopyValues(Ral_TupleBegin(mlierTuple),
		Ral_TupleEnd(mlierTuple), prodTupleBegin + mlierOffset) ;

	    if (!Ral_RelationPushBack(product, prodTuple, NULL)) {
		Ral_RelationDelete(product) ;
		return NULL ;
	    }
	}
    }

    return product ;
}

Ral_Relation
Ral_RelationProject(
    Ral_Relation relation,
    Ral_IntVector attrSet)
{
    Ral_TupleHeading projHeading =
	Ral_TupleHeadingSubset(relation->heading, attrSet) ;
    Ral_Relation projRel = Ral_RelationNew(projHeading) ;
    Ral_RelationIter end = Ral_RelationEnd(relation) ;
    Ral_RelationIter iter ;

    Ral_RelationReserve(projRel, Ral_RelationCardinality(relation)) ;
    for (iter = Ral_RelationBegin(relation) ; iter != end ; ++iter) {
	Ral_Tuple projTuple = Ral_TupleSubset(*iter, projHeading,
	    attrSet) ;
	/*
	 * Ignore the return. Projecting, in general, results in
	 * duplicated tuples.
	 */
	Ral_RelationPushBack(projRel, projTuple, NULL) ;
    }

    return projRel ;
}

Ral_Relation
Ral_RelationGroup(
    Ral_Relation relation,
    const char *newAttrName,
    Ral_IntVector grpAttrs)
{
    Ral_TupleHeading heading = relation->heading ;
    Ral_TupleHeading attrHeading ;
    Ral_TupleHeadingIter thIter ;
    int nAttrs = Ral_TupleHeadingSize(heading) ;
    int nGrpAttrs = Ral_IntVectorSize(grpAttrs) ;
    Ral_TupleHeading grpHeading ;
    Ral_Relation group ;
    Ral_IntVector attrMap ;
    Ral_IntVectorIter attrMapIter ;
    Ral_IntVectorIter attrMapEnd ;
    Ral_TupleHeadingIter grpAttrIter ;
    Ral_RelationIter relIter ;
    Ral_RelationIter relEnd = Ral_RelationEnd(relation) ;
    Ral_IntVector idMap ;
    Ral_IntVectorIter grpAttrsBegin = Ral_IntVectorBegin(grpAttrs) ;
    Ral_IntVectorIter grpAttrsEnd = Ral_IntVectorEnd(grpAttrs) ;

    /*
     * Create the relation heading for the grouped attribute.
     */
    attrHeading = Ral_TupleHeadingSubset(heading, grpAttrs) ;
    /*
     * The grouped relation has a heading that contains all the attributes
     * minus those that go into the relation valued attribute plus one for the
     * new relation valued attribute itself.
     */
    grpHeading = Ral_TupleHeadingNew(nAttrs - nGrpAttrs + 1) ;
    /*
     * Copy in the attributes from the relation. Use a map to make this easier.
     */
    thIter = Ral_TupleHeadingBegin(heading) ;
    attrMap = Ral_IntVectorBooleanMap(grpAttrs, nAttrs) ;
    attrMapEnd = Ral_IntVectorEnd(attrMap) ;
    for (attrMapIter = Ral_IntVectorBegin(attrMap) ; attrMapIter != attrMapEnd ;
	++attrMapIter) {
	if (!*attrMapIter) {
	    int status ;

	    status = Ral_TupleHeadingAppend(heading, thIter, thIter + 1,
                    grpHeading) ;
	    assert(status != 0) ;
	}
	++thIter ;
    }
    /*
     * Add the new relation valued attribute.
     */
    grpAttrIter = Ral_TupleHeadingPushBack(grpHeading,
	Ral_AttributeNewRelationType(newAttrName, attrHeading)) ;
    assert(grpAttrIter != Ral_TupleHeadingEnd(grpHeading)) ;

    group = Ral_RelationNew(grpHeading) ;
    /*
     * Now add the tuples to the new relation.
     * Iterate through the tuples of the relation and using the values
     * attempt to find the tuple in the new grouped relation. If it is
     * found, the we need to construct a tuple for the grouped attribute.
     * If not, then we need to add a new tuple that includes a relation
     * valued attribute.
     */
    for (relIter = Ral_RelationBegin(relation) ; relIter != relEnd ;
	++relIter) {
	Ral_Tuple tuple = *relIter ;
	Ral_TupleIter tupleBegin = Ral_TupleBegin(tuple) ;
	int grpIndex = Ral_RelationFindTupleReference(group, tuple, idMap) ;
	Ral_Tuple grpTuple ;
	Ral_IntVectorIter grpAttrsIter ;
	Ral_Tuple attrTuple ;
	Ral_TupleIter attrTupleBegin ;
	Ral_Relation attrRel ;
	Tcl_Obj *attrObj ;

	if (grpIndex == -1) {
	    int status ;
	    Ral_TupleIter grpIter ;
	    Ral_TupleIter tupleIter = tupleBegin ;
	    /*
	     * Tuple is not in the grouped relation.
	     * Use the values in the attribute map to find the
	     * values to place in the new grouped tuple.
	     */
	    grpTuple = Ral_TupleNew(grpHeading) ;
	    grpIter = Ral_TupleBegin(grpTuple) ;
	    for (attrMapIter = Ral_IntVectorBegin(attrMap) ;
		attrMapIter != attrMapEnd ; ++attrMapIter) {
		if (!*attrMapIter) {
		    Ral_TupleCopyValues(tupleIter, tupleIter + 1,
			grpIter++) ;
		}
		++tupleIter ;
	    }
	    attrRel = Ral_RelationNew(attrHeading) ;
	    attrObj = Ral_RelationObjNew(attrRel) ;
	    Tcl_IncrRefCount(*grpIter = attrObj) ;

	    status = Ral_RelationPushBack(group, grpTuple, NULL) ;
	    assert(status != 0) ;
	} else {
	    grpTuple = *(Ral_RelationBegin(group) + grpIndex) ;
	    attrObj = *(Ral_TupleEnd(grpTuple) - 1) ;
	    if (Tcl_ConvertToType(NULL, attrObj, &Ral_RelationObjType)
		!= TCL_OK) {
		Ral_IntVectorDelete(attrMap) ;
		return NULL ;
	    }
	    attrRel = attrObj->internalRep.otherValuePtr ;
	}
	/*
	 * Add values to the relation valued attribute.
	 */
	attrTuple = Ral_TupleNew(attrHeading) ;
	attrTupleBegin = Ral_TupleBegin(attrTuple) ;
	for (grpAttrsIter = grpAttrsBegin ; grpAttrsIter != grpAttrsEnd ;
	    ++grpAttrsIter) {
	    Ral_TupleIter tupleIter = tupleBegin + *grpAttrsIter ;
	    Ral_TupleCopyValues(tupleIter, tupleIter + 1,
		attrTupleBegin++) ;
	}
	/*
	 * Ignore duplicates.
	 */
	Ral_RelationPushBack(attrRel, attrTuple, NULL) ;
    }

    Ral_IntVectorDelete(attrMap) ;
    return group ;
}

/*
 * Create a new relation by unwinding a relation valued attribute.
 * The attribute named "attrName" must be a relation and the new
 * relation will have all the attributes of the old relation minus
 * "attrName" plus all the attributes of the relation given by "attrName".
 */
Ral_Relation
Ral_RelationUngroup(
    Ral_Relation relation,
    const char *attrName,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading heading = relation->heading ;
    Ral_TupleHeadingIter ungrpAttrIter ;
    Ral_Attribute ungrpAttr ;
    Ral_TupleHeading attrHeading ;
    Ral_TupleHeading resultHeading ;
    int status ;
    Ral_Relation resultRel ;
    Ral_RelationIter relIter ;
    Ral_RelationIter relEnd ;

    /*
     * Check that the attribute exists and is a relation type attribute
     */
    ungrpAttrIter = Ral_TupleHeadingFind(heading, attrName) ;
    if (ungrpAttrIter == Ral_TupleHeadingEnd(heading)) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
	return NULL ;
    }
    ungrpAttr = *ungrpAttrIter ;
    if (ungrpAttr->attrType != Relation_Type) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_NOT_A_RELATION, attrName) ;
	return NULL ;
    }
    attrHeading = ungrpAttr->heading ;
    /*
     * Create a new tuple heading for the ungrouped result.  The ungrouped
     * relation has a heading with all the attributes of the original plus that
     * of the ungrouped attribute minus one (the attribute being ungrouped).
     */
    resultHeading = Ral_TupleHeadingNew(Ral_RelationDegree(relation) +
	Ral_TupleHeadingSize(attrHeading) - 1) ;
    /*
     * Copy up to the attribute to be ungrouped. We should always be able
     * to copy the old heading parts into an empty heading.
     */
    status = Ral_TupleHeadingAppend(heading, Ral_TupleHeadingBegin(heading),
            ungrpAttrIter, resultHeading) ;
    assert(status != 0) ;
    /*
     * Copy the attributes after the one to be ungrouped to the end.
     */
    status = Ral_TupleHeadingAppend(heading, ungrpAttrIter + 1,
	Ral_TupleHeadingEnd(heading), resultHeading) ;
    assert(status != 0) ;
    /*
     * Copy the heading from the ungrouped attribute itself.
     * Now we have to deal with an attribute in the ungrouped relation
     * that is the same as another attribute already in the heading.
     */
    status = Ral_TupleHeadingAppend(attrHeading,
	Ral_TupleHeadingBegin(attrHeading),
	Ral_TupleHeadingEnd(attrHeading), resultHeading) ;
    if (status == 0) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
	    "while ungrouping relation") ;
	Ral_TupleHeadingDelete(resultHeading) ;
	return NULL ;
    }
    /*
     * Create the result relation.
     */
    resultRel = Ral_RelationNew(resultHeading) ;
    /*
     * Now put together the tuples.  Iterate through the relation and add
     * tuples to the result.  The body of the result consists of the cross
     * product of each tuple in the original relation with all of the tuples
     * from the ungrouped attribute.
     */
    relEnd = Ral_RelationEnd(relation) ;
    for (relIter = Ral_RelationBegin(relation) ; relIter != relEnd ;
	++relIter) {
	Ral_Tuple tuple = *relIter ;
	Ral_TupleIter tupleBegin = Ral_TupleBegin(tuple) ;
	Ral_TupleIter tupleEnd = Ral_TupleEnd(tuple) ;
	Ral_TupleIter ungrpAttr = tupleBegin +
                (ungrpAttrIter - Ral_TupleHeadingBegin(heading)) ;
	Tcl_Obj *ungrpObj = *ungrpAttr ;
	Ral_Relation ungrpRel ;
	Ral_RelationIter ungrpIter ;
	Ral_RelationIter ungrpEnd ;

	if (Tcl_ConvertToType(NULL, ungrpObj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_FORMAT_ERR, ungrpObj) ;
	    Ral_RelationDelete(resultRel) ;
	    return NULL ;
	}
	ungrpRel = ungrpObj->internalRep.otherValuePtr ;
	/*
	 * A new result tuple will be added for each tuple in the
	 * ungrouped relation valued attribute.
	 */
	Ral_RelationReserve(resultRel, Ral_RelationCardinality(ungrpRel)) ;

	ungrpEnd = Ral_RelationEnd(ungrpRel) ;
	for (ungrpIter = Ral_RelationBegin(ungrpRel) ; ungrpIter != ungrpEnd ;
	    ++ungrpIter) {
	    Ral_Tuple ungrpValue = *ungrpIter ;
	    Ral_Tuple ungrpTuple = Ral_TupleNew(resultHeading) ;
	    Ral_TupleIter dstLoc = Ral_TupleBegin(ungrpTuple) ;

	    dstLoc += Ral_TupleCopyValues(tupleBegin, ungrpAttr, dstLoc) ;
	    dstLoc += Ral_TupleCopyValues(ungrpAttr + 1, tupleEnd, dstLoc) ;
	    Ral_TupleCopyValues(Ral_TupleBegin(ungrpValue),
		Ral_TupleEnd(ungrpValue), dstLoc) ;

	    /*
	     * Ignore any duplicates. PushBack manages the lifetime of
	     * "ungrpTuple" properly if it is not inserted.
	     */
	    Ral_RelationPushBack(resultRel, ungrpTuple, NULL) ;
	}
    }
    return resultRel ;
}

/*
 * Natural join of two relations using the attributes in the join map.
 */
Ral_Relation
Ral_RelationJoin(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap map,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading r1Heading = r1->heading ;
    Ral_TupleHeading r2Heading = r2->heading ;
    Ral_TupleHeading joinHeading ;
    Ral_IntVector r2JoinAttrs ;
    Ral_Relation joinRel ;
    Ral_JoinMapIter tupleMapEnd ;
    Ral_JoinMapIter tupleMapIter ;
    Ral_RelationIter r1Begin = Ral_RelationBegin(r1) ;
    Ral_RelationIter r2Begin = Ral_RelationBegin(r2) ;

    /*
     * Finish building the join map by finding which tuples are
     * to be joined.
     */
    Ral_RelationFindJoinTuples(r1, r2, map) ;
    /*
     * Construct the heading for the joined relation.
     */
    joinHeading = Ral_TupleHeadingJoin(r1Heading, r2Heading, map,
	&r2JoinAttrs, errInfo) ;
    if (joinHeading == NULL) {
	return NULL ;
    }
    /*
     * Construct the joined relation.
     */
    joinRel = Ral_RelationNew(joinHeading) ;
    /*
     * Add in the tuples.
     */
    Ral_RelationReserve(joinRel, Ral_JoinMapTupleSize(map)) ;
    /*
     * Step through the matches found in the join map and compose the
     * indicated tuples.
     */
    tupleMapEnd = Ral_JoinMapTupleEnd(map) ;
    for (tupleMapIter = Ral_JoinMapTupleBegin(map) ;
	tupleMapIter != tupleMapEnd ; ++tupleMapIter) {
	Ral_Tuple r1Tuple = *(r1Begin + tupleMapIter->m[0]) ;
	Ral_Tuple r2Tuple = *(r2Begin + tupleMapIter->m[1]) ;
	Ral_TupleIter r2TupleEnd = Ral_TupleEnd(r2Tuple) ;
	Ral_TupleIter r2TupleIter ;
	Ral_Tuple joinTuple ;
	Ral_TupleIter jtIter ;
	Ral_IntVectorIter attrMapIter = Ral_IntVectorBegin(r2JoinAttrs) ;
	int status ;

	joinTuple = Ral_TupleNew(joinHeading) ;
	jtIter = Ral_TupleBegin(joinTuple) ;
	/*
	 * Take all the values from the first relation's tuple.
	 */
	jtIter += Ral_TupleCopyValues(Ral_TupleBegin(r1Tuple),
	    Ral_TupleEnd(r1Tuple), jtIter) ;
	/*
	 * Take the values from the second relation's tuple, eliminating
	 * those that are part of the natural join.
	 */
	for (r2TupleIter = Ral_TupleBegin(r2Tuple) ;
	    r2TupleIter != r2TupleEnd ; ++r2TupleIter) {
	    int attrIndex = *attrMapIter++ ;
	    if (attrIndex != -1) {
		jtIter += Ral_TupleCopyValues(r2TupleIter,
		    r2TupleIter + 1, jtIter) ;
	    }
	}

	status = Ral_RelationPushBack(joinRel, joinTuple, NULL) ;
	if (status == 0) {
	    Ral_RelationDelete(joinRel) ;
	    Ral_IntVectorDelete(r2JoinAttrs) ;
            Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_INTERNAL_ERROR,
                    Ral_TupleObjNew(joinTuple)) ;
	    return NULL ;
	}
    }

    Ral_IntVectorDelete(r2JoinAttrs) ;

    return joinRel ;
}

/*
 * Compose of two relations. Like compose, except all the compose attributes
 * are elminated.
 */
Ral_Relation
Ral_RelationCompose(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap map,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading r1Heading = r1->heading ;
    Ral_TupleHeading r2Heading = r2->heading ;
    Ral_TupleHeading composeHeading ;
    Ral_IntVector r1JoinAttrs ;
    Ral_IntVector r2JoinAttrs ;
    Ral_Relation composeRel ;
    Ral_JoinMapIter tupleMapEnd ;
    Ral_JoinMapIter tupleMapIter ;
    Ral_RelationIter r1Begin = Ral_RelationBegin(r1) ;
    Ral_RelationIter r2Begin = Ral_RelationBegin(r2) ;

    /*
     * Finish building the join map by finding which tuples are
     * to be composeed.
     */
    Ral_RelationFindJoinTuples(r1, r2, map) ;
    /*
     * Construct the heading for the composeed relation.
     */
    composeHeading = Ral_TupleHeadingCompose(r1Heading, r2Heading, map,
	&r1JoinAttrs, &r2JoinAttrs, errInfo) ;
    if (composeHeading == NULL) {
	return NULL ;
    }
    /*
     * Construct the composeed relation.
     */
    composeRel = Ral_RelationNew(composeHeading) ;
    /*
     * Add in the tuples.
     */
    Ral_RelationReserve(composeRel, Ral_JoinMapTupleSize(map)) ;
    /*
     * Step through the matches found in the compose map and compose the
     * indicated tuples.
     */
    tupleMapEnd = Ral_JoinMapTupleEnd(map) ;
    for (tupleMapIter = Ral_JoinMapTupleBegin(map) ;
	tupleMapIter != tupleMapEnd ; ++tupleMapIter) {
	Ral_Tuple r1Tuple = *(r1Begin + tupleMapIter->m[0]) ;
	Ral_Tuple r2Tuple = *(r2Begin + tupleMapIter->m[1]) ;
	Ral_TupleIter tupleEnd ;
	Ral_TupleIter tupleIter ;
	Ral_Tuple composeTuple ;
	Ral_TupleIter jtIter ;
	Ral_IntVectorIter attrMapIter ;

	composeTuple = Ral_TupleNew(composeHeading) ;
	jtIter = Ral_TupleBegin(composeTuple) ;
	/*
	 * Take the values from the first relation's tuple, eliminating
	 * those that are part of the join attributes.
	 */
	tupleEnd = Ral_TupleEnd(r1Tuple) ;
	attrMapIter = Ral_IntVectorBegin(r1JoinAttrs) ;
	for (tupleIter = Ral_TupleBegin(r1Tuple) ;
	    tupleIter != tupleEnd ; ++tupleIter) {
	    int attrIndex = *attrMapIter++ ;
	    if (attrIndex != -1) {
		jtIter += Ral_TupleCopyValues(tupleIter,
		    tupleIter + 1, jtIter) ;
	    }
	}
	/*
	 * Take the values from the second relation's tuple, eliminating
	 * those that are part of the join attributes.
	 */
	tupleEnd = Ral_TupleEnd(r2Tuple) ;
	attrMapIter = Ral_IntVectorBegin(r2JoinAttrs) ;
	for (tupleIter = Ral_TupleBegin(r2Tuple) ;
	    tupleIter != tupleEnd ; ++tupleIter) {
	    int attrIndex = *attrMapIter++ ;
	    if (attrIndex != -1) {
		jtIter += Ral_TupleCopyValues(tupleIter,
		    tupleIter + 1, jtIter) ;
	    }
	}

	/*
	 * Ignore any duplicates.
	 */
	Ral_RelationPushBack(composeRel, composeTuple, NULL) ;
    }

    Ral_IntVectorDelete(r1JoinAttrs) ;
    Ral_IntVectorDelete(r2JoinAttrs) ;

    return composeRel ;
}

/*
 * Semi-join of two relations using the attributes in the join map.
 * N.B. that we invert the sense of this relative to Date's definition,
 * so that is can be n-adic from left to right. So:
 *	semijoin A B ==> a relation that is the same type as B.
 * So the semijoin of A and B yields a relation containing those tuples
 * in B that have a correspondence in A.
 */
Ral_Relation
Ral_RelationSemiJoin(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap map)
{
    Ral_Relation semiJoinRel ;
    Ral_JoinMapIter tupleMapEnd ;
    Ral_JoinMapIter tupleMapIter ;
    Ral_RelationIter r2Begin = Ral_RelationBegin(r2) ;

    /*
     * Finish building the join map by finding which tuples are
     * to be joined.
     */
    Ral_RelationFindJoinTuples(r1, r2, map) ;
    /*
     * The relation heading for a semijoin is the same as the second
     * relation in the operation
     */
    semiJoinRel = Ral_RelationNew(r2->heading) ;
    /*
     * Add in the tuples.  Step through the matches found in the join map and
     * compose the indicated tuples. The tuples of the semijoin will simply be
     * those from the second relation that matched.
     */
    Ral_RelationReserve(semiJoinRel, Ral_JoinMapTupleSize(map)) ;
    tupleMapEnd = Ral_JoinMapTupleEnd(map) ;
    for (tupleMapIter = Ral_JoinMapTupleBegin(map) ;
	tupleMapIter != tupleMapEnd ; ++tupleMapIter) {
	Ral_Tuple r2Tuple = *(r2Begin + tupleMapIter->m[1]) ;
	/*
	 * Ignore any duplicates. It is quite possible that many tuples
	 * in r1 match the same tuple in r2 and this would show up as
	 * a match in the join map.
	 */
	Ral_RelationPushBack(semiJoinRel, r2Tuple, NULL) ;
    }

    return semiJoinRel ;
}

/*
 * Semi-minus of two relations using the attributes in the join map.
 * Like semi-join we invert the sense relative to Date's definition.
 */
Ral_Relation
Ral_RelationSemiMinus(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap map)
{
    Ral_Relation semiMinusRel ;
    Ral_RelationIter r2End = Ral_RelationEnd(r2) ;
    Ral_RelationIter r2Iter ;
    Ral_IntVector tupleMatches ;
    Ral_IntVectorIter nomatch ;

    /*
     * Finish building the join map by finding which tuples are
     * to be joined.
     */
    Ral_RelationFindJoinTuples(r1, r2, map) ;
    /*
     * The relation heading for a semiminus is the same as the second
     * relation in the operation
     */
    semiMinusRel = Ral_RelationNew(r2->heading) ;
    /*
     * Add in the tuples.  The tuples we want are the ones that did
     * NOT match. We get these from the join map.
     */
    tupleMatches = Ral_JoinMapTupleMap(map, 1, Ral_RelationCardinality(r2)) ;
    nomatch = Ral_IntVectorBegin(tupleMatches) ;
    for (r2Iter = Ral_RelationBegin(r2) ; r2Iter != r2End ; ++r2Iter) {
	if (*nomatch++) {
	    int status ;
	    /*
	     * There should not be any duplicates.
	     */
	    status = Ral_RelationPushBack(semiMinusRel, *r2Iter, NULL) ;
	    assert(status != 0) ;
	}
    }

    Ral_IntVectorDelete(tupleMatches) ;

    return semiMinusRel ;
}

Ral_Relation
Ral_RelationDivide(
    Ral_Relation dend,
    Ral_Relation dsor,
    Ral_Relation med,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading dendHeading = dend->heading ;
    int dendHeadingSize = Ral_TupleHeadingSize(dendHeading) ;
    Ral_TupleHeading dsorHeading = dsor->heading ;
    int dsorHeadingSize = Ral_TupleHeadingSize(dsorHeading) ;
    int dsorCard = Ral_RelationCardinality(dsor) ;
    Ral_TupleHeading medTupleHeading = med->heading ;
    int medHeadingSize = Ral_TupleHeadingSize(medTupleHeading) ;
    Ral_Relation quot ;
    Ral_TupleHeading trialTupleHeading ;
    Ral_Tuple trialTuple ;
    Ral_IntVector trialOrder ;
    Ral_RelationIter dendEnd ;
    Ral_RelationIter dendIter ;
    Ral_RelationIter dsorEnd ;
    Ral_RelationIter medEnd ;

    /*
     * The heading of the dividend must be disjoint from the heading of the
     * divisor.
     *
     * The heading of the mediator must be the union of the dividend and
     * divisor headings.
     *
     * The quotient, which has the same heading as the dividend, is then
     * computed by iterating over the dividend and for each tuple, determining
     * if all the tuples composed by combining a dividend tuple with all the
     * divisor tuples are contained in the mediator.  If they are, then that
     * dividend tuple is a tuple of the quotient.
     */
    if (Ral_TupleHeadingCommonAttributes(dendHeading, dsorHeading,
	NULL) != 0) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_NOT_DISJOINT,
	    "while computing quotient") ;
	return NULL ;
    }
    if (Ral_TupleHeadingCommonAttributes(dendHeading, medTupleHeading,
		NULL) != dendHeadingSize ||
	Ral_TupleHeadingCommonAttributes(dsorHeading, medTupleHeading,
		NULL) != dsorHeadingSize ||
	dendHeadingSize + dsorHeadingSize != medHeadingSize) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_NOT_UNION,
	    "while computing quotient") ;
	return NULL ;
    }
    /*
     * Create the quotient. It has the same heading as the dividend.
     */
    quot = Ral_RelationNew(dendHeading) ;
    /*
     * Create a tuple heading that is the union of divisor and dividend.
     * We union the two headings here to make it easier to copy the
     * values from the dividend and divsor into the trial tuple.
     */
    trialTupleHeading = Ral_TupleHeadingUnion(dendHeading, dsorHeading) ;
    trialTuple = Ral_TupleNew(trialTupleHeading) ;
    /*
     * Create an order map between the tuple heading and that of the mediator.
     * There is no guarantee that the mediator is in the same order as
     * the union of the dividend and divisor headings.
     */
    trialOrder = Ral_TupleHeadingNewOrderMap(medTupleHeading,
	trialTupleHeading) ;
    /*
     * Iterate over the dividend modifying a tuple with the divsor values
     * and then find it in the mediator.
     */
    dendEnd = Ral_RelationEnd(dend) ;
    dsorEnd = Ral_RelationEnd(dsor) ;
    medEnd = Ral_RelationEnd(med) ;
    for (dendIter = Ral_RelationBegin(dend) ; dendIter != dendEnd ;
	++dendIter) {
	Ral_Tuple dendTuple = *dendIter ;
	Ral_TupleIter trialIter = Ral_TupleBegin(trialTuple) ;
	Ral_RelationIter dsorIter ;
	int matches = 0 ;

	/*
	 * Copy in the values from the dividend. N.B. that the copy cleans
	 * up any values that might already be there.
	 */
	trialIter += Ral_TupleCopyValues(Ral_TupleBegin(dendTuple),
	    Ral_TupleEnd(dendTuple), trialIter) ;
	/*
	 * Iterate through each tuple in the divisor.
	 */
	for (dsorIter = Ral_RelationBegin(dsor) ; dsorIter != dsorEnd ;
	    ++dsorIter) {
	    Ral_Tuple dsorTuple = *dsorIter ;
	    /*
	     * Copy in the tuple values from the divisor.
	     */
	    Ral_TupleCopyValues(Ral_TupleBegin(dsorTuple),
		Ral_TupleEnd(dsorTuple), trialIter) ;
	    /*
	     * Tally if we can find this tuple in the mediator.
	     */
	    matches += Ral_RelationFind(med, trialTuple, trialOrder)
		!= medEnd ;
	}
	/*
	 * If the number of matches equals the cardinality of the divisor then
	 * that tuple of the dividend is added to the quotient.
	 */
	if (matches == dsorCard) {
	    int status ;
	    status = Ral_RelationPushBack(quot, dendTuple, NULL) ;
	    assert(status != 0) ;
	}
    }

    Ral_TupleDelete(trialTuple) ;
    Ral_IntVectorDelete(trialOrder) ;

    return quot ;
}

int
Ral_RelationCopy(
    Ral_Relation src,
    Ral_RelationIter start,
    Ral_RelationIter finish,
    Ral_Relation dst,
    Ral_IntVector orderMap)
{
    int allCopied = 1 ;

    assert(Ral_TupleHeadingEqual(src->heading, dst->heading)) ;

    Ral_RelationReserve(dst, finish - start) ;
    while (start != finish) {
	allCopied = Ral_RelationPushBack(dst, *start++, orderMap) && allCopied ;
    }

    return allCopied ;
}

/*
 * Find the tuples that match according to the attributes in the
 * join map. The result is recorded back into the join map.
 */
void
Ral_RelationFindJoinTuples(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap map)
{
    int idNum ;
    /*
     * First we want to determine if the join is across identifiers and
     * referring attributes. If so, then we can use the hash tables to
     * speed the process along. Otherwise we will have to iterate through
     * both relations finding the matching tuples.
     *
     * So first compare the identifiers in "r1" to the join map and see
     * if we can find a match. Then compare the identifiers in "r2" to the
     * join map, again looking for a match. Finally, we resort to doing it
     * the hard way.
     */
    idNum = Ral_RelationFindJoinId(r1, map, 0) ;
    if (idNum >= 0) {
	/*
	 * "r2" refers to "idNum" of "r1".
	 * For this case, the identifier given by "idNum" matches the 1st
	 * attribute vector in the join map and is an identifier of "r1".  Find
	 * the tuples in "r2" that match the attributes in the second attribute
	 * vector of the join map. The resulting map is placed back into "map".
	 */
	Ral_RelationIter r2Iter ;
	Ral_RelationIter r2Begin = Ral_RelationBegin(r2) ;
	Ral_RelationIter r2End = Ral_RelationEnd(r2) ;
	Ral_IntVector refAttrs ;
	/*
	 * Sort the attributes in the 1st attribute vector to match
	 * the same order as the identifer.
	 */
	Ral_JoinMapSortAttr(map, 0) ;
	refAttrs = Ral_JoinMapGetAttr(map, 1) ;
	/*
	 * Iterate through the tuples in r2, and find and find any corresponding
	 * match in r1, based on the ID given by idNum.
	 */
	for (r2Iter = r2Begin ; r2Iter != r2End ; ++r2Iter) {
	    int r1Index = Ral_RelationFindTupleReference(r1, *r2Iter,
		refAttrs) ;

	    if (r1Index >= 0) {
		Ral_JoinMapAddTupleMapping(map, r1Index, r2Iter - r2Begin) ;
	    }
	}
	Ral_IntVectorDelete(refAttrs) ;
    } else {
	idNum = Ral_RelationFindJoinId(r2, map, 1) ;
	if (idNum >= 0) {
	    /*
	     * "r1" refers to "idNum" of "r2"
	     * Here tuples in "r1" refer to an identifier in "r2". The
	     * identifier is given by "idNum". Find the tuples in "r1" whose
	     * attributes match the corresponding attributes in "r2".
	     */
	    Ral_RelationIter r1Iter ;
	    Ral_RelationIter r1Begin = Ral_RelationBegin(r1) ;
	    Ral_RelationIter r1End = Ral_RelationEnd(r1) ;
	    Ral_IntVector refAttrs ;
	    /*
	     * Sort the attributes in the 2nd attribute vector to match
	     * the same order as the identifer.
	     */
	    Ral_JoinMapSortAttr(map, 1) ;
	    refAttrs = Ral_JoinMapGetAttr(map, 0) ;
	    /*
	     * Iterate through the tuples in r1, and find any corresponding
	     * match in r2, based on the ID given by idNum.
	     */
	    for (r1Iter = r1Begin ; r1Iter != r1End ; ++r1Iter) {
		int r2Index = Ral_RelationFindTupleReference(r2, *r1Iter,
		    refAttrs) ;

		if (r2Index >= 0) {
		    Ral_JoinMapAddTupleMapping(map, r1Iter - r1Begin, r2Index) ;
		}
	    }
	    Ral_IntVectorDelete(refAttrs) ;
	} else {
	    /*
	     * Join attributes do not constitute an identifier in either
	     * relation. Must join by searching both relations.
	     */
	    Ral_RelationIter r1Iter ;
	    Ral_RelationIter r1Begin = Ral_RelationBegin(r1) ;
	    Ral_RelationIter r1End = Ral_RelationEnd(r1) ;
	    Ral_RelationIter r2Iter ;
	    Ral_RelationIter r2Begin = Ral_RelationBegin(r2) ;
	    Ral_RelationIter r2End = Ral_RelationEnd(r2) ;
	    Tcl_DString *r1Keys ;
	    Tcl_DString *key1 ;
	    Tcl_DString *r2Keys ;
	    Tcl_DString *key2 ;

	    /*
	     * First compute the keys for all the tuples in both relations.
	     */
	    r1Keys = (Tcl_DString*)ckalloc(
		sizeof(*r1Keys) * Ral_RelationCardinality(r1)) ;
	    r2Keys = (Tcl_DString *)ckalloc(
		sizeof(*r2Keys) * Ral_RelationCardinality(r2)) ;

	    key1 = r1Keys ;
	    for (r1Iter = r1Begin ; r1Iter != r1End ; ++r1Iter) {
		Ral_RelationGetJoinMapKey(*r1Iter, map, 0, key1++) ;
	    }

	    key2 = r2Keys ;
	    for (r2Iter = r2Begin ; r2Iter != r2End ; ++r2Iter) {
		Ral_RelationGetJoinMapKey(*r2Iter, map, 1, key2++) ;
	    }
	    /*
	     * Search through the tuples and find the corresponding matches.
	     */
	    key1 = r1Keys ;
	    for (r1Iter = r1Begin ; r1Iter != r1End ; ++r1Iter) {
		int r1Index = r1Iter - r1Begin ;
		key2 = r2Keys ;
		for (r2Iter = r2Begin ; r2Iter != r2End ; ++r2Iter) {
		    if (strcmp(Tcl_DStringValue(key1), Tcl_DStringValue(key2))
			== 0) {
			Ral_JoinMapAddTupleMapping(map, r1Index,
			    r2Iter - r2Begin) ;
		    }
		    ++key2 ;
		}
		++key1 ;
	    }
	    /*
	     * Clean up the key information.
	     */
	    key1 = r1Keys ;
	    for (r1Iter = r1Begin ; r1Iter != r1End ; ++r1Iter) {
		Tcl_DStringFree(key1++) ;
	    }
	    key2 = r2Keys ;
	    for (r2Iter = r2Begin ; r2Iter != r2End ; ++r2Iter) {
		Tcl_DStringFree(key2++) ;
	    }
	    ckfree((char *)r1Keys) ;
	    ckfree((char *)r2Keys) ;
	}
    }
}

/*
 * Compute the transitive closure of the relation.
 * Assume that there are two attributes and the are of the same type.
 * Return the transitive closure.
 */
Ral_Relation
Ral_RelationTclose(
    Ral_Relation relation)
{
    Ral_TupleHeading heading = relation->heading ;
    Ral_RelationIter rIter ;
    Ral_RelationIter rBegin = Ral_RelationBegin(relation) ;
    Ral_RelationIter rEnd = Ral_RelationEnd(relation) ;
    int index = 0 ;
    Tcl_HashTable vertices ;
    Ral_PtrVector valueMap ;
    char *Ckij ;
    int nVertices ;
    int size ;
    int i ;
    int j ;
    int k ;
    Ral_Relation tclose ;

    /*
     * We treat the two attributes as if they defined the edge in a graph. So
     * first we must come up with a list of vertices.  This amounts to
     * projecting each attribute and taking the resulting union. We will do
     * that by using a hash table and a vector. The hash table will be used to
     * look up attribute and determine if we have already seen it. The vector
     * will then used to associate consecutive indices with the values.  This
     * will also help us decode the result in the end.
     */
    Tcl_InitObjHashTable(&vertices) ;
    valueMap = Ral_PtrVectorNew(Ral_RelationCardinality(relation) * 2) ;
    for (rIter = rBegin ; rIter != rEnd ; ++rIter) {
	Ral_TupleIter tIter = Ral_TupleBegin(*rIter) ;
	Tcl_Obj *value ;
	Tcl_HashEntry *entry ;
	int newPtr ;

	value = *tIter++ ;
	entry = Tcl_CreateHashEntry(&vertices, (const char *)value, &newPtr) ;
	if (newPtr) {
	    Tcl_SetHashValue(entry, index++) ;
	    Ral_PtrVectorPushBack(valueMap, value) ;
	}
	value = *tIter ;
	entry = Tcl_CreateHashEntry(&vertices, (const char *)value, &newPtr) ;
	if (newPtr) {
	    Tcl_SetHashValue(entry, index++) ;
	    Ral_PtrVectorPushBack(valueMap, value) ;
	}
    }

    /*
     * The closure algorithm works in a single N x N array.  The values in the
     * array will be either 0 or 1.
     */
    nVertices = Ral_PtrVectorSize(valueMap) ;
    size = nVertices * nVertices ;
    Ckij = ckalloc(size) ;
    memset(Ckij, 0, size) ;

    /*
     * Look up in the relation for each tuple and seed the set of
     * edges defined there. This completes the calculation of C0ij, the
     * paths involving no vertices other than the endpoints.
     */
    for (rIter = rBegin ; rIter != rEnd ; ++rIter) {
	Ral_TupleIter tIter = Ral_TupleBegin(*rIter) ;
	Tcl_HashEntry *entry ;

	entry = Tcl_FindHashEntry(&vertices, (const char *)*tIter++) ;
	assert(entry != NULL) ;
	i = (int)Tcl_GetHashValue(entry) ;
	entry = Tcl_FindHashEntry(&vertices, (const char *)*tIter) ;
	assert(entry != NULL) ;
	j = (int)Tcl_GetHashValue(entry) ;
	Ckij[i * nVertices + j] = 1 ;
    }
    /*
     * Done with the hash table.
     */
    Tcl_DeleteHashTable(&vertices) ;

    /*
     * Now iterate over all the subsets of vertices and compute the
     * magic formula. See Aho, Hopcroft and Ullman, "The Design and
     * Analysis of Computer Algorithms", p. 199. Also see Aho, Hopcroft
     * and Ullman, "Data Structures and Algorithms", p. 212.
     */
    for (k = 0 ; k < nVertices ; ++k) {
	int k_row = k * nVertices ;
	for (i = 0 ; i < size ; i += nVertices) {
	    for (j = 0 ; j < nVertices ; ++j) {
		Ckij[i + j] |= Ckij[i + k] & Ckij[k_row + j] ;
	    }
	}
    }
    /*
     * At this point, "Ckij" has a 1 in every element where there is some path
     * from "i" to "j". Now construct the relation.  We know the new relation
     * will have at least as many tuples as the original relation (since it is
     * always a subset of the closure).  The we traverse the "Ckij" array and
     * every where that there is a one, we add that pair to the result.  We use
     * the vector generated earlier to look up the tuple values.
     */
    tclose = Ral_RelationNew(heading) ;
    Ral_RelationReserve(tclose, Ral_RelationCardinality(relation)) ;
    for (i = 0 ; i < nVertices ; ++i) {
	int i_row = i * nVertices ;
	for (j = 0 ; j < nVertices ; ++j) {
	    if (Ckij[i_row++]) {
		Ral_Tuple tuple = Ral_TupleNew(heading) ;
		Ral_TupleIter tIter = Ral_TupleBegin(tuple) ;
		Tcl_IncrRefCount(*tIter++ = Ral_PtrVectorFetch(valueMap, i)) ;
		Tcl_IncrRefCount(*tIter = Ral_PtrVectorFetch(valueMap, j)) ;
		/*
		 * Ignore duplicates.
		 */
		Ral_RelationPushBack(tclose, tuple, NULL) ;
	    }
	}
    }

    ckfree(Ckij) ;
    Ral_PtrVectorDelete(valueMap) ;
    return tclose ;
}

static int
Ral_TupleCompare(
    int l,
    int r,
    Ral_RelSortProps *props)
{
    Ral_TupleHeading heading = props->relation->heading ;
    Ral_RelationIter begin = Ral_RelationBegin(props->relation) ;
    Ral_Tuple t1 = *(begin + Ral_IntVectorFetch(props->sortVect, l)) ;
    Ral_Tuple t2 = *(begin + Ral_IntVectorFetch(props->sortVect, r)) ;
    Ral_IntVectorIter aEnd = Ral_IntVectorEnd(props->attrs) ;
    Ral_IntVectorIter aIter ;
    int result = 0 ;

    for (aIter = Ral_IntVectorBegin(props->attrs) ;
	result == 0 && aIter != aEnd ; ++aIter) {
	int attrIndex = *aIter ;
	Ral_Attribute sortAttr = Ral_TupleHeadingFetch(heading, attrIndex) ;
	Tcl_Obj *o1 = t1->values[attrIndex] ;
	Tcl_Obj *o2 = t2->values[attrIndex] ;

	result = Ral_AttributeValueCompare(sortAttr, o1, o2) ;
    }

    return props->sortDirection ? -result : result ;
}

static void
Ral_DownHeap(
    int v,
    int n,
    Ral_RelSortProps *props)
{
    int w = 2 * v + 1 ;

    while (w < n) {
	if (w + 1 < n) {
	    if (Ral_TupleCompare(w + 1, w, props) > 0) {
		w++ ;
	    }
	}
	if (Ral_TupleCompare(v, w, props) >= 0) {
	    return ;
	}

	Ral_IntVectorExchange(props->sortVect, v, w) ;

	v = w ;
	w = 2 * v + 1 ;
    }
}

static void
Ral_BuildHeap(
    Ral_RelSortProps *props)
{
    int v ;
    int n = Ral_IntVectorSize(props->sortVect) ;

    for (v = n / 2 - 1 ; v >= 0 ; --v) {
	Ral_DownHeap(v, n, props) ;
    }
}

/*
 * Return a vector that gives the permutation of the relation tuples
 * that will achieve the sort. So the value of the returned vector at
 * index 0 will be the index into the relation for the first tuple according
 * to the sorting. Sorting algorithm is heap sort.
 */
Ral_IntVector
Ral_RelationSort(
    Ral_Relation relation,  /* relation to sort */
    Ral_IntVector attrs,    /* vector of attribute indices to sort by */
    int direction)	    /* 0 ==> ascending, 1 ==> descending */
{
    Ral_RelSortProps props ;
    int n = Ral_RelationCardinality(relation) ;
    int nAttrs = Ral_IntVectorSize(attrs) ;

    props.sortVect = Ral_IntVectorNew(n, 0) ;
    Ral_IntVectorFillConsecutive(props.sortVect, 0) ;

    if (nAttrs > 0) {
	props.relation = relation ;
	props.attrs = attrs ;
	props.sortDirection = direction ;

	Ral_BuildHeap(&props) ;
	while (n > 1) {
	    --n ;
	    Ral_IntVectorExchange(props.sortVect, 0, n) ;
	    Ral_DownHeap(0, n, &props) ;
	}
    }

    return props.sortVect ;
}

/*
 * Find a tuple in a relation.
 * The tuple is located with respect to the identifier given in "idNum".
 * If "map" is NULL, then "tuple" is assumed to be ordered the same as the
 * tuple heading in "relation".  Otherwise, "map" provides the reordering
 * mapping.
 */
Ral_RelationIter
Ral_RelationFind(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_IntVector map)
{
    Tcl_HashEntry *entry ;
    Ral_RelationIter found = Ral_RelationEnd(relation) ;

    entry = Tcl_FindHashEntry(&relation->index, (char const *)tuple) ;

    if (entry) {
        found = Ral_RelationBegin(relation) + (int)Tcl_GetHashValue(entry) ;
    }
    return found  ;
}

Ral_Relation
Ral_RelationExtract(
    Ral_Relation relation,
    Ral_IntVector tupleSet)
{
    Ral_Relation subRel = Ral_RelationNew(relation->heading) ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(tupleSet) ;
    Ral_IntVectorIter iter ;
    Ral_RelationIter relBegin = Ral_RelationBegin(relation) ;

    Ral_RelationReserve(subRel, Ral_IntVectorSize(tupleSet)) ;

    for (iter = Ral_IntVectorBegin(tupleSet) ; iter != end ; ++iter) {
	int status ;

	status = Ral_RelationPushBack(subRel, *(relBegin + *iter), NULL) ;
	assert(status != 0) ;
    }

    return subRel ;
}

Ral_RelationIter
Ral_RelationErase(
    Ral_Relation relation,
    Ral_RelationIter first,
    Ral_RelationIter last)
{
    Ral_RelationIter iter ;

    if (first < relation->start || first > relation->finish) {
	Tcl_Panic("Ral_RelationErase: first iterator out of bounds") ;
    }
    if (last < relation->start || last > relation->finish) {
	Tcl_Panic("Ral_RelationErase: last iterator out of bounds") ;
    }
    if (first > last) {
	Tcl_Panic("Ral_RelationErase: first iterator greater than last") ;
    }

    /*
     * Remove the indices for all tuples from the first to the finish.
     */
    for (iter = first ; iter != relation->finish ; ++iter) {
	// Ral_RelationRemoveTupleIndex(relation, *iter) ;
    }
    /*
     * Remove a reference from the tuples from first to last.
     */
    for (iter = first ; iter != last ; ++iter) {
	Ral_TupleUnreference(*iter) ;
    }
    /*
     * Close up the array.
     */
    memmove(first, last, (relation->finish - last) * sizeof(*first)) ;
    relation->finish -= last - first ;
    /*
     * Reindex the new arrangement of tuples.
     */
    for (iter = first ; iter != relation->finish ; ++iter) {
	int status ;

	status = Ral_RelationIndexTuple(relation, *iter, iter) ;
	assert(status != 0) ;
    }
    return first ;
}

/*
 * Count the number of tuples in r1 that match in r2.
 * Returns -1 if the relations are not comparable.
 */
int
Ral_RelationCompare(
    Ral_Relation r1,
    Ral_Relation r2)
{
    Ral_IntVector orderMap ;
    Ral_RelationIter iter1 ;
    Ral_RelationIter end1 = Ral_RelationEnd(r1) ;
    Ral_RelationIter end2 = Ral_RelationEnd(r2) ;
    int found = 0 ;

    /*
     * For two relations to be compared, the headings must be equal.
     */
    if (!Ral_TupleHeadingEqual(r1->heading, r2->heading)) {
	return -1 ;
    }
    /*
     * Reordering may be necessary. Since we will search
     * "r2" to find matching tuples from "r1", we must compute
     * the map onto "r2" from "r1".
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r2->heading, r1->heading) ;
    /*
     * Iterate through the tuples in the first relation and search
     * the second one for a match. Count the number of matches.
     */
    for (iter1 = Ral_RelationBegin(r1) ; iter1 != end1 ; ++iter1) {
	if (Ral_RelationFind(r2, *iter1, orderMap) != end2) {
	    ++found ;
	}
    }

    Ral_IntVectorDelete(orderMap) ;

    return found ;
}

int
Ral_RelationEqual(
    Ral_Relation r1,
    Ral_Relation r2)
{
    int matches ;
    int r1Card ;
    int r2Card ;

    matches = Ral_RelationCompare(r1, r2) ;
    if (matches < 0) {
	return matches ;
    }

    r1Card = Ral_RelationCardinality(r1) ;
    r2Card = Ral_RelationCardinality(r2) ;

    return r1Card == r2Card && matches == r1Card ;
}

int
Ral_RelationNotEqual(
    Ral_Relation r1,
    Ral_Relation r2)
{
    int equals = Ral_RelationEqual(r1, r2) ;

    if (equals >= 0) {
	equals = !equals ;
    }

    return equals ;
}

/*
 * Is r1 a subset of r2?
 */
int
Ral_RelationSubsetOf(
    Ral_Relation r1,
    Ral_Relation r2)
{
    int matches ;
    int r1Card ;
    int r2Card ;

    matches = Ral_RelationCompare(r1, r2) ;
    if (matches < 0) {
	return matches ;
    }

    r1Card = Ral_RelationCardinality(r1) ;
    r2Card = Ral_RelationCardinality(r2) ;

    return r1Card <= r2Card && matches == r1Card ;
}

/*
 * Is r1 a proper subset of r2?
 */
int
Ral_RelationProperSubsetOf(
    Ral_Relation r1,
    Ral_Relation r2)
{
    int matches ;
    int r1Card ;
    int r2Card ;

    matches = Ral_RelationCompare(r1, r2) ;
    if (matches < 0) {
	return matches ;
    }

    r1Card = Ral_RelationCardinality(r1) ;
    r2Card = Ral_RelationCardinality(r2) ;

    return r1Card < r2Card && matches == r1Card ;
}

/*
 * Is r1 a superset of r2?
 */
int
Ral_RelationSupersetOf(
    Ral_Relation r1,
    Ral_Relation r2)
{
    int matches ;
    int r1Card ;
    int r2Card ;

    matches = Ral_RelationCompare(r1, r2) ;
    if (matches < 0) {
	return matches ;
    }

    r1Card = Ral_RelationCardinality(r1) ;
    r2Card = Ral_RelationCardinality(r2) ;

    return r1Card >= r2Card && matches == r2Card ;
}

/*
 * Is r1 a proper superset of r2?
 */
int
Ral_RelationProperSupersetOf(
    Ral_Relation r1,
    Ral_Relation r2)
{
    int matches ;
    int r1Card ;
    int r2Card ;

    matches = Ral_RelationCompare(r1, r2) ;
    if (matches < 0) {
	return matches ;
    }

    r1Card = Ral_RelationCardinality(r1) ;
    r2Card = Ral_RelationCardinality(r2) ;

    return r1Card > r2Card && matches == r2Card ;
}

int
Ral_RelationRenameAttribute(
    Ral_Relation relation,
    const char *oldName,
    const char *newName,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading tupleHeading = relation->heading ;
    Ral_TupleHeadingIter end = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter found ;
    Ral_Attribute newAttr ;

    /*
     * Find the old name.
     */
    found = Ral_TupleHeadingFind(tupleHeading, oldName) ;
    if (found == end) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, oldName) ;
	return 0 ;
    }
    /*
     * Create a new attribute with the new name.
     */
    newAttr = Ral_AttributeRename(*found, newName) ;
    /*
     * Store the new attribute into the heading.
     */
    found = Ral_TupleHeadingStore(tupleHeading, found, newAttr) ;
    if (found == end) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR, newName) ;
	return 0 ;
    }

    return 1 ;
}


int
Ral_RelationScan(
    Ral_Relation relation,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    Ral_TupleHeading heading = relation->heading ;
    int length ;
    /*
     * Scan the header.
     * +1 for "{", +1 for "}" and +1 for the separating space
     */
    length = Ral_TupleHeadingScan(heading, typeFlags) + 3 ;
    /*
     * Scan the values.
     */
    length += Ral_RelationScanValue(relation, typeFlags, valueFlags) ;
    return length ;
}

int
Ral_RelationConvert(
    Ral_Relation relation,
    char *dst,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    char *p = dst ;

    /*
     * Convert the heading adding "{}" to make it a list.
     */
    *p++ = openList ;
    p += Ral_TupleHeadingConvert(relation->heading, p, typeFlags) ;
    *p++ = closeList ;
    /*
     * Separate the heading from the body by space.
     */
    *p++ = ' ' ;
    /*
     * Convert the body.
     */
    p += Ral_RelationConvertValue(relation, p, typeFlags, valueFlags) ;
    /*
     * Finished with the flags.
     */
    Ral_AttributeTypeScanFlagsFree(typeFlags) ;
    Ral_AttributeValueScanFlagsFree(valueFlags) ;

    return p - dst ;
}

int
Ral_RelationScanValue(
    Ral_Relation relation,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    int nBytes ;
    int length ;
    Ral_AttributeTypeScanFlags *typeFlag ;
    Ral_AttributeValueScanFlags *valueFlag ;
    Ral_RelationIter rend = Ral_RelationEnd(relation) ;
    Ral_RelationIter riter ;
    Ral_TupleHeading tupleHeading = relation->heading ;
    int nonEmptyHeading = !Ral_TupleHeadingEmpty(tupleHeading) ;
    Ral_TupleHeadingIter hEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter hIter ;

    assert(typeFlags->attrType == Relation_Type) ;
    assert(typeFlags->flags.compoundFlags.count
	    == Ral_RelationDegree(relation)) ;
    assert(valueFlags->attrType == Relation_Type) ;
    assert(valueFlags->flags.compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the value flags.
     * For a relation we need a flag structure for each value in each tuple.
     */
    valueFlags->flags.compoundFlags.count = typeFlags->flags.compoundFlags.count
	    * Ral_RelationCardinality(relation) ;
    nBytes = valueFlags->flags.compoundFlags.count *
	sizeof(*valueFlags->flags.compoundFlags.flags) ;
    valueFlags->flags.compoundFlags.flags =
	(Ral_AttributeValueScanFlags *)ckalloc(nBytes) ;
    memset(valueFlags->flags.compoundFlags.flags, 0, nBytes) ;

    length = sizeof(openList) ;

    valueFlag = valueFlags->flags.compoundFlags.flags ;
    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;
	typeFlag = typeFlags->flags.compoundFlags.flags ;

	length += sizeof(openList) ;
	for (hIter = Ral_TupleHeadingBegin(tupleHeading) ;
	    hIter != hEnd ; ++hIter) {
	    Ral_Attribute a = *hIter ;
	    Tcl_Obj *v = *values++ ;

	    /*
	     * Reuse the scan info for the attribute name.
	     */
	    length += typeFlag->nameLength + 1 ; /* +1 for space */
	    assert(v != NULL) ;
	    length += Ral_AttributeScanValue(a, v, typeFlag++, valueFlag++)
		+ 1 ; /* +1 for the separating space */
	}
	/*
	 * Remove trailing space from tuple values if the tuple has a
	 * non-zero header.
	 */
	if (nonEmptyHeading) {
	    --length ;
	}
	length += sizeof(closeList) ;
	++length ; /* space between list elements */
    }
    /*
     * Remove the trailing space if the relation contained any tuples.
     */
    if (Ral_RelationCardinality(relation)) {
	--length ;
    }
    length += sizeof(closeList) ;

    return length ;
}

int
Ral_RelationConvertValue(
    Ral_Relation relation,
    char *dst,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    char *p = dst ;
    Ral_AttributeTypeScanFlags *typeFlag ;
    Ral_AttributeValueScanFlags *valueFlag ;
    Ral_RelationIter rend = Ral_RelationEnd(relation) ;
    Ral_RelationIter riter ;
    Ral_TupleHeading tupleHeading = relation->heading ;
    Ral_TupleHeadingIter hEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter hIter ;

    assert(typeFlags->attrType == Relation_Type) ;
    assert(typeFlags->flags.compoundFlags.count
	    == Ral_RelationDegree(relation)) ;
    assert(valueFlags->attrType == Relation_Type) ;
    assert(valueFlags->flags.compoundFlags.count ==
	Ral_RelationDegree(relation) * Ral_RelationCardinality(relation)) ;

    valueFlag = valueFlags->flags.compoundFlags.flags ;
    *p++ = openList ;
    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;
	typeFlag = typeFlags->flags.compoundFlags.flags ;

	*p++ = openList ;
	for (hIter = Ral_TupleHeadingBegin(tupleHeading) ;
	    hIter != hEnd ; ++hIter) {
	    Ral_Attribute a = *hIter ;
	    Tcl_Obj *v = *values++ ;

	    p += Ral_AttributeConvertName(a, p, typeFlag) ;
	    *p++ = ' ' ;
	    assert(v != NULL) ;
	    p += Ral_AttributeConvertValue(a, v, p, typeFlag++, valueFlag++) ;
	    *p++ = ' ' ;
	}
        /*
         * Remove the trailing space.
         */
        if (*(p - 1) == ' ') {
	    --p ;
	}
	*p++ = closeList ;
	*p++ = ' ' ;
    }
    /*
     * Remove the trailing space.
     */
    if (*(p - 1) == ' ') {
	--p ;
    }
    *p++ = closeList ;

    return p - dst ;
}

char *
Ral_RelationStringOf(
    Ral_Relation relation)
{
    Ral_AttributeTypeScanFlags typeFlags ;
    Ral_AttributeValueScanFlags valueFlags ;
    char *str ;
    int length ;

    memset(&typeFlags, 0, sizeof(typeFlags)) ;
    typeFlags.attrType = Relation_Type ;
    memset(&valueFlags, 0, sizeof(valueFlags)) ;
    valueFlags.attrType = Relation_Type ;

    /* +1 for NUL terminator */
    str = ckalloc(Ral_RelationScan(relation, &typeFlags, &valueFlags) + 1) ;
    length = Ral_RelationConvert(relation, str, &typeFlags, &valueFlags) ;
    str[length] = '\0' ;

    return str ;
}

char *
Ral_RelationValueStringOf(
    Ral_Relation relation)
{
    Ral_TupleHeading heading = relation->heading ;
    Ral_AttributeTypeScanFlags typeFlags ;
    Ral_AttributeValueScanFlags valueFlags ;
    char *str ;
    int length ;

    memset(&typeFlags, 0, sizeof(typeFlags)) ;
    typeFlags.attrType = Relation_Type ;
    memset(&valueFlags, 0, sizeof(valueFlags)) ;
    valueFlags.attrType = Relation_Type ;

    /*
     * Scan the header just to get the typeFlags set properly.
     */
    Ral_TupleHeadingScan(heading, &typeFlags) ;
    /*
     * Scan and convert only the value portion of the relation.
     * +1 for NUL terminator
     */
    str = ckalloc(Ral_RelationScanValue(relation, &typeFlags, &valueFlags)
	+ 1) ;
    length = Ral_RelationConvertValue(relation, str, &typeFlags, &valueFlags) ;
    str[length] = '\0' ;

    return str ;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * Determine if two headings are equal and report the error information if not.
 */
static int
Ral_HeadingMatch(
    Ral_TupleHeading h1,
    Ral_TupleHeading h2,
    Ral_ErrorInfo *errInfo)
{
    int status = Ral_TupleHeadingEqual(h1, h2) ;
    if (status == 0) {
	char *h2str = Ral_TupleHeadingStringOf(h2) ;
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_HEADING_NOT_EQUAL, h2str) ;
        ckfree(h2str) ;
    }
    return status ;
}

/*
 * Find a tuple in "relation" based on the values in "tuple". The attributes
 * to be used are given in the "attrSet".
 */
static int
Ral_RelationFindTupleReference(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_IntVector attrSet)
{
    /*
     * HERE
     */
    return -1 ;
}

static int
Ral_RelationIndexTuple(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_RelationIter where)
{
    Tcl_HashEntry *entry ;
    int newPtr ;

    entry = Tcl_CreateHashEntry(&relation->index, (char const *)tuple,
            &newPtr) ;
    /*
     * Check that an entry was actually created and if so, set its value.
     */
    if (newPtr) {
	Tcl_SetHashValue(entry, where - relation->start) ;
    }
    return newPtr ;
}

static void
Ral_RelationGetJoinMapKey(
    Ral_Tuple tuple,
    Ral_JoinMap map,
    int offset,
    Tcl_DString *idKey)
{
    Ral_JoinMapIter iter ;
    Ral_JoinMapIter end = Ral_JoinMapAttrEnd(map) ;

    Tcl_DStringInit(idKey) ;
    for (iter = Ral_JoinMapAttrBegin(map) ; iter != end ; ++iter) {
	int attrIndex = iter->m[offset] ;
	assert(attrIndex < Ral_TupleDegree(tuple)) ;
	Tcl_DStringAppend(idKey, Tcl_GetString(tuple->values[attrIndex]), -1) ;
    }

    return ;
}

static int
Ral_RelationFindJoinId(
    Ral_Relation rel,
    Ral_JoinMap map,
    int offset)
{
    /*
     * HERE
     */
    return 0 ;
}

static unsigned int
tupleHashKeys(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    return Ral_TupleHash((Ral_Tuple)keyPtr) ;
}

static int
tupleCompareKeys(
    void *keyPtr,
    Tcl_HashEntry *hPtr)
{
    return Ral_TupleEqual((Ral_Tuple)keyPtr,
            (Ral_Tuple)hPtr->key.oneWordValue) ;
}

static Tcl_HashEntry *
tupleEntryAlloc(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    Ral_Tuple tuple = (Ral_Tuple)keyPtr ;
    Tcl_HashEntry *hPtr ;

    hPtr = (Tcl_HashEntry *)ckalloc(sizeof(*hPtr)) ;
    hPtr->key.oneWordValue = (char *)tuple ;
    Ral_TupleReference(tuple) ;
    hPtr->clientData = NULL ;

    return hPtr ;
}

static void
tupleEntryFree(
    Tcl_HashEntry *hPtr)
{
    Ral_Tuple tuple = (Ral_Tuple)hPtr->key.oneWordValue ;
    Ral_TupleUnreference(tuple) ;
    ckfree((char *)hPtr) ;
}
