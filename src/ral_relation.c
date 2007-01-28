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

$RCSfile: ral_relation.c,v $
$Revision: 1.25 $
$Date: 2007/01/28 02:21:11 $
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
#include "ral_relationheading.h"
#include "ral_tupleheading.h"
#include "ral_relationobj.h"
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
static Tcl_HashEntry *Ral_RelationFindIndexEntry(Ral_Relation, int, Ral_Tuple,
    Ral_IntVector) ;
static int Ral_RelationFindTupleIndex(Ral_Relation, int, Ral_Tuple,
    Ral_IntVector) ;
static int Ral_RelationFindTupleReference(Ral_Relation, int, Ral_Tuple,
    Ral_IntVector) ;
static void Ral_RelationRemoveIndex(Ral_Relation, int, Ral_Tuple) ;
static int Ral_RelationIndexIdentifier(Ral_Relation, int, Ral_Tuple,
    Ral_RelationIter) ;
static int Ral_RelationIndexTuple(Ral_Relation, Ral_Tuple, Ral_RelationIter) ;
static void Ral_RelationRemoveTupleIndex(Ral_Relation, Ral_Tuple) ;
static void Ral_RelationGetJoinMapKey(Ral_Tuple, Ral_JoinMap, int,
    Tcl_DString *) ;
static int Ral_RelationFindJoinId(Ral_Relation, Ral_JoinMap, int) ;

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
static const char rcsid[] = "@(#) $RCSfile: ral_relation.c,v $ $Revision: 1.25 $" ;

/*
FUNCTION DEFINITIONS
*/

Ral_Relation
Ral_RelationNew(
    Ral_RelationHeading heading)
{
    int nBytes ;
    Ral_Relation relation ;
    int idCount ;
    Tcl_HashTable *indexVector ;

    nBytes = sizeof(*relation) +
	heading->idCount * sizeof(*relation->indexVector) ;
    relation = (Ral_Relation)ckalloc(nBytes) ;
    memset(relation, 0, nBytes) ;

    Ral_RelationHeadingReference(relation->heading = heading) ;
    relation->indexVector = (Tcl_HashTable *)(relation + 1) ;
    for (idCount = heading->idCount, indexVector = relation->indexVector ;
	idCount > 0 ; --idCount, ++indexVector) {
	Tcl_InitHashTable(indexVector, TCL_STRING_KEYS) ;
    }

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
    Ral_RelationHeading srcHeading = srcRelation->heading ;
    Ral_RelationHeading dupHeading ;
    Ral_Relation dupRelation ;
    Ral_TupleHeading dupTupleHeading ;
    Ral_RelationIter srcIter ;
    Ral_RelationIter srcEnd = srcRelation->finish ;

    dupHeading = Ral_RelationHeadingDup(srcHeading) ;
    dupRelation = Ral_RelationNew(dupHeading) ;
    Ral_RelationReserve(dupRelation, Ral_RelationCardinality(srcRelation)) ;

    dupTupleHeading = dupHeading->tupleHeading ;
    for (srcIter = srcRelation->start ; srcIter != srcEnd ; ++srcIter) {
	Ral_Tuple srcTuple = *srcIter ;
	Ral_Tuple dupTuple ;
	int appended ;

	dupTuple = Ral_TupleNew(dupTupleHeading) ;
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

/*
 * This function makes a new relation that includes a new tuple heading.
 * The tuple values are just reference counted. The copy is then suitable
 * for modifying the heading in place (e.g. a relation rename).
 */
Ral_Relation
Ral_RelationMediumCopy(
    Ral_Relation srcRelation)
{
    Ral_RelationHeading srcHeading = srcRelation->heading ;
    Ral_RelationHeading dupHeading ;
    Ral_Relation dupRelation ;
    Ral_RelationIter srcIter ;
    Ral_RelationIter srcEnd = srcRelation->finish ;

    dupHeading = Ral_RelationHeadingDup(srcHeading) ;
    dupRelation = Ral_RelationNew(dupHeading) ;
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
    int idCount = relation->heading->idCount ;
    Tcl_HashTable *indexVector = relation->indexVector ;

    for (iter = relation->start ; iter != relation->finish ; ++iter) {
	Ral_TupleUnreference(*iter) ;
    }
    while (idCount-- > 0) {
	Tcl_DeleteHashTable(indexVector++) ;
    }

    if (relation->start) {
	ckfree((char *)relation->start) ;
    }
    Ral_RelationHeadingUnreference(relation->heading) ;
    ckfree((char *)relation) ;
}

Ral_RelationIter
Ral_RelationBegin(
    Ral_Relation relation)
{
    return relation->start ;
}

Ral_RelationIter
Ral_RelationEnd(
    Ral_Relation relation)
{
    return relation->finish ;
}

int
Ral_RelationDegree(
    Ral_Relation r)
{
    return Ral_TupleHeadingSize(r->heading->tupleHeading) ;
}

int
Ral_RelationCardinality(
    Ral_Relation relation)
{
    return relation->finish - relation->start ;
}

int
Ral_RelationCapacity(
    Ral_Relation relation)
{
    return relation->endStorage - relation->start ;
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
 * Insert a tuple at the end of relation. Keeps the indices up to date.
 */
int
Ral_RelationPushBack(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    Ral_Tuple insertTuple ;
    int status = 0 ;

    assert(Ral_TupleHeadingEqual(relation->heading->tupleHeading,
	tuple->heading)) ;

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
	Ral_TupleDupOrdered(tuple, relation->heading->tupleHeading, orderMap) :
	tuple ;
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
    Ral_RelationRemoveTupleIndex(relation, *pos) ;
    /*
     * If reordering is required, we create a new tuple with the correct
     * attribute ordering to match the relation.
     */
    newTuple = orderMap ?
	Ral_TupleDupOrdered(tuple, relation->heading->tupleHeading, orderMap) :
	tuple ;
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
    if (!Ral_RelationHeadingMatch(r1->heading, r2->heading, errInfo)) {
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
    orderMap = Ral_TupleHeadingNewOrderMap(r1->heading->tupleHeading,
	r2->heading->tupleHeading) ;
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
    if (!Ral_RelationHeadingMatch(r1->heading, r2->heading, errInfo)) {
	return NULL ;
    }
    /*
     * Reordering may be necessary. Since we will search
     * "r2" to find matching tuples from "r1", we must compute
     * the map onto "r2" from "r1".
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r2->heading->tupleHeading,
	r1->heading->tupleHeading) ;

    intersectRel = Ral_RelationNew(r1->heading) ;
    /*
     * Iterate through the tuples in the first relation and search
     * the second one for a match. Only tuples contained in both relations
     * are added to the intersection.
     */
    for (iter1 = Ral_RelationBegin(r1) ; iter1 != end1 ; ++iter1) {
	if (Ral_RelationFind(r2, 0, *iter1, orderMap) != end2) {
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
    if (!Ral_RelationHeadingMatch(r1->heading, r2->heading, errInfo)) {
	return NULL ;
    }
    /*
     * Reordering may be necessary. Since we will search
     * "r2" to find matching tuples from "r1", we must compute
     * the map onto "r2" from "r1".
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r2->heading->tupleHeading,
	r1->heading->tupleHeading) ;

    diffRel = Ral_RelationNew(r1->heading) ;
    /*
     * Iterate through the tuples in the first relation and search the second
     * one for a match. Only tuples not contained in the second relation are
     * added to the difference.
     */
    for (iter1 = Ral_RelationBegin(r1) ; iter1 != end1 ; ++iter1) {
	if (Ral_RelationFind(r2, 0, *iter1, orderMap) == end2) {
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
    Ral_RelationHeading prodHeading ;
    Ral_Relation product ;
    Ral_RelationIter mcandIter ;
    Ral_RelationIter mcandEnd = Ral_RelationEnd(multiplicand) ;
    Ral_RelationIter mlierIter ;
    Ral_RelationIter mlierEnd = Ral_RelationEnd(multiplier) ;
    Ral_TupleHeading prodTupleHeading ;
    int mlierOffset = Ral_RelationDegree(multiplicand) ;

    prodHeading = Ral_RelationHeadingUnion(multiplicand->heading,
	multiplier->heading) ;
    if (!prodHeading) {
	return NULL ;
    }
    prodTupleHeading = prodHeading->tupleHeading ;

    product = Ral_RelationNew(prodHeading) ;
    Ral_RelationReserve(product, Ral_RelationCardinality(multiplicand) *
	Ral_RelationCardinality(multiplier)) ;

    for (mcandIter = Ral_RelationBegin(multiplicand) ; mcandIter != mcandEnd ;
	++mcandIter) {
	for (mlierIter = Ral_RelationBegin(multiplier) ; mlierIter != mlierEnd ;
	    ++mlierIter) {
	    Ral_Tuple mcandTuple = *mcandIter ;
	    Ral_Tuple mlierTuple = *mlierIter ;
	    Ral_Tuple prodTuple = Ral_TupleNew(prodTupleHeading) ;
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
    Ral_RelationHeading projHeading =
	Ral_RelationHeadingSubset(relation->heading, attrSet) ;
    Ral_TupleHeading projTupleHeading = projHeading->tupleHeading ;
    Ral_Relation projRel = Ral_RelationNew(projHeading) ;
    Ral_RelationIter end = Ral_RelationEnd(relation) ;
    Ral_RelationIter iter ;

    Ral_RelationReserve(projRel, Ral_RelationCardinality(relation)) ;
    for (iter = Ral_RelationBegin(relation) ; iter != end ; ++iter) {
	Ral_Tuple projTuple = Ral_TupleSubset(*iter, projTupleHeading,
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
    Ral_RelationHeading heading = relation->heading ;
    Ral_TupleHeading tupleHeading = heading->tupleHeading ;
    Ral_TupleHeading attrTupleHeading ;
    Ral_RelationHeading attrHeading ;
    Ral_RelationIdIter relIdIter ;
    Ral_RelationIdIter relIdEnd = Ral_RelationHeadingIdEnd(heading) ;
    Ral_RelationIdIter relIdBegin = Ral_RelationHeadingIdBegin(heading) ;
    Ral_IntVector idSet ;
    int idCount ;
    Ral_TupleHeadingIter thIter ;
    int nAttrs = Ral_TupleHeadingSize(tupleHeading) ;
    int nGrpAttrs = Ral_IntVectorSize(grpAttrs) ;
    Ral_TupleHeading grpTupleHeading ;
    Ral_RelationHeading grpHeading ;
    Ral_Relation group ;
    Ral_IntVector attrMap ;
    Ral_IntVectorIter attrMapIter ;
    Ral_IntVectorIter attrMapEnd ;
    Ral_TupleHeadingIter grpAttrIter ;
    Ral_RelationIter relIter ;
    Ral_RelationIter relEnd = Ral_RelationEnd(relation) ;
    Ral_IntVector idMap ;
    Ral_IntVector grpId ;
    Ral_IntVectorIter grpAttrsBegin = Ral_IntVectorBegin(grpAttrs) ;
    Ral_IntVectorIter grpAttrsEnd = Ral_IntVectorEnd(grpAttrs) ;

    /*
     * Create the relation heading for the grouped attribute.  Examine each
     * identifier of the old Heading to determine the set of attributes that
     * are both in the attribute map and in the identifier.  If that set is
     * non-empty, then look up the name of the attribute in the old heading and
     * map it to a new index in the tupleHeading.
     */
    attrTupleHeading = Ral_TupleHeadingSubset(tupleHeading, grpAttrs) ;
    idSet = Ral_IntVectorNewEmpty(heading->idCount) ;
    for (relIdIter = relIdBegin ; relIdIter != relIdEnd ; ++relIdIter) {
	Ral_IntVector id = *relIdIter ;
	if (Ral_IntVectorContainsAny(id, grpAttrs)) {
	    Ral_IntVectorPushBack(idSet, relIdIter - relIdBegin) ;
	}
    }
    idCount = Ral_IntVectorSize(idSet) ;
    if (idCount) {
	Ral_IntVectorIter idIter ;
	Ral_IntVectorIter idEnd = Ral_IntVectorEnd(idSet) ;

	attrHeading = Ral_RelationHeadingNew(attrTupleHeading, idCount) ;

	/*
	 * Go through the identifiers that contain at least on of the
	 * grouped attributes and form an identifier from those attributes
	 * that appear in the relation identifier. We must of course remap
	 * the attribute indices to the new order of the grouped attribute
	 * tuple heading.
	 */
	idCount = 0 ;
	for (idIter = Ral_IntVectorBegin(idSet) ; idIter != idEnd ; ++idIter) {
	    Ral_IntVector commonAttrs ;
	    int status ;

	    relIdIter = relIdBegin + *idIter ;
	    assert(relIdIter < relIdEnd) ;
	    commonAttrs = Ral_IntVectorIntersect(*relIdIter, grpAttrs) ;
	    assert(Ral_IntVectorSize(commonAttrs) != 0) ;
	    /*
	     * Remap the attribute indices.
	     */
	    Ral_TupleHeadingMapIndices(tupleHeading, commonAttrs,
		attrTupleHeading) ;
	    /*
	     * Install the new identifier.
	     */
	    status = Ral_RelationHeadingAddIdentifier(attrHeading, idCount++,
		commonAttrs) ;
	    assert(status != 0) ;
	}
    } else {
	/*
	 * Didn't find any grouped attributes that were part of an identifier.
	 * So we make the default identifier.
	 */
	attrHeading = Ral_RelationHeadingNewDefaultId(attrTupleHeading) ;
    }
    Ral_IntVectorDelete(idSet) ;

    /*
     * The grouped relation has a heading that contains all the attributes
     * minus those that go into the relation valued attribute plus one for the
     * new relation valued attribute itself.
     */
    grpTupleHeading = Ral_TupleHeadingNew(nAttrs - nGrpAttrs + 1) ;
    /*
     * Copy in the attributes from the relation. Use a map to make this easier.
     */
    thIter = Ral_TupleHeadingBegin(tupleHeading) ;
    attrMap = Ral_IntVectorBooleanMap(grpAttrs, nAttrs) ;
    attrMapEnd = Ral_IntVectorEnd(attrMap) ;
    for (attrMapIter = Ral_IntVectorBegin(attrMap) ; attrMapIter != attrMapEnd ;
	++attrMapIter) {
	if (!*attrMapIter) {
	    int status ;

	    status = Ral_TupleHeadingAppend(tupleHeading, thIter, thIter + 1,
		grpTupleHeading) ;
	    assert(status != 0) ;
	}
	++thIter ;
    }
    /*
     * Add the new relation valued attribute.
     */
    grpAttrIter = Ral_TupleHeadingPushBack(grpTupleHeading,
	Ral_AttributeNewRelationType(newAttrName, attrHeading)) ;
    assert(grpAttrIter != Ral_TupleHeadingEnd(grpTupleHeading)) ;
    /*
     * The grouped relation has identifiers that are the same as the
     * original relation minus any attributes contained in the new
     * relational attribute. If that leaves an identifier to be the
     * empty set then it must be eliminated. And of course if that means
     * that all the identifiers are empty, then we must make all the
     * attributes of the new grouped relation the single identifier.
     * N.B. that the new relation valued attribute is never an identifier.
     */
    idCount = 0 ;
    for (relIdIter = relIdBegin ; relIdIter != relIdEnd ; ++relIdIter) {
	Ral_IntVector id = *relIdIter ;
	if (!Ral_IntVectorSubsetOf(id, grpAttrs)) {
	    ++idCount ;
	}
    }
    if (idCount) {
	grpHeading = Ral_RelationHeadingNew(grpTupleHeading, idCount) ;
	idCount = 0 ;
	for (relIdIter = relIdBegin ; relIdIter != relIdEnd ; ++relIdIter) {
	    Ral_IntVector id = *relIdIter ;
	    Ral_IntVector grpId = Ral_IntVectorMinus(id, grpAttrs) ;

	    if (Ral_IntVectorSize(grpId) != 0) {
		int status ;
		/*
		 * Remap the attribute indices.
		 */
		Ral_TupleHeadingMapIndices(tupleHeading, grpId,
		    grpTupleHeading) ;
		status = Ral_RelationHeadingAddIdentifier(grpHeading, idCount++,
		    grpId) ;
		assert(status != 0) ;
	    } else {
		Ral_IntVectorDelete(grpId) ;
	    }
	}
    } else {
	/*
	 * All the identifiers were subsets of the grouped attributes.
	 * Therefore create a single identifier that consists of all
	 * the attributes except the relation valued attribute.
	 */
	Ral_IntVector allId =
	    Ral_IntVectorNew(Ral_TupleHeadingSize(grpTupleHeading) - 1, 0) ;
	grpHeading = Ral_RelationHeadingNew(grpTupleHeading, 1) ;
	int status ;

	Ral_IntVectorFillConsecutive(allId, 0) ;
	status = Ral_RelationHeadingAddIdentifier(grpHeading, 0, allId) ;
	assert(status != 0) ;
    }

    group = Ral_RelationNew(grpHeading) ;
    /*
     * Now add the tuples to the new relation.
     * First we must generate a map between an identifier in the
     * grouped relation and the corresponding indices of the original
     * relation.
     */
    grpId = *Ral_RelationHeadingIdBegin(grpHeading) ;
    idMap = Ral_IntVectorDup(grpId) ;
    Ral_TupleHeadingMapIndices(grpTupleHeading, idMap, tupleHeading) ;
    /*
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
	int grpIndex = Ral_RelationFindTupleReference(group, 0, tuple, idMap) ;
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
	    grpTuple = Ral_TupleNew(grpTupleHeading) ;
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
	attrTuple = Ral_TupleNew(attrTupleHeading) ;
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
    Ral_RelationHeading heading = relation->heading ;
    Ral_TupleHeading tupleHeading = heading->tupleHeading ;
    Ral_TupleHeadingIter ungrpAttrIter ;
    Ral_Attribute ungrpAttr ;
    Ral_RelationHeading attrHeading ;
    Ral_TupleHeading attrTupleHeading ;
    Ral_TupleHeading resultTupleHeading ;
    int status ;
    Ral_RelationHeading resultHeading ;
    Ral_Relation resultRel ;
    Ral_RelationIter relIter ;
    Ral_RelationIter relEnd ;
    int ungrpIndex ;
    int indexOffset ;
    Ral_RelationIdIter relIdIter ;
    Ral_RelationIdIter relIdEnd ;
    Ral_RelationIdIter attrIdIter ;
    Ral_RelationIdIter attrIdEnd ;
    int idNum = 0 ;

    /*
     * Check that the attribute exists and is a relation type attribute
     */
    ungrpAttrIter = Ral_TupleHeadingFind(tupleHeading, attrName) ;
    if (ungrpAttrIter == Ral_TupleHeadingEnd(tupleHeading)) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
	return NULL ;
    }
    ungrpAttr = *ungrpAttrIter ;
    if (ungrpAttr->attrType != Relation_Type) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_NOT_A_RELATION, attrName) ;
	return NULL ;
    }
    attrHeading = ungrpAttr->relationHeading ;
    attrTupleHeading = attrHeading->tupleHeading ;
    /*
     * Create a new tuple heading for the ungrouped result.  The ungrouped
     * relation has a heading with all the attributes of the original plus that
     * of the ungrouped attribute minus one (the attribute being ungrouped).
     */
    resultTupleHeading = Ral_TupleHeadingNew(Ral_RelationDegree(relation) +
	Ral_RelationHeadingDegree(attrHeading) - 1) ;
    /*
     * Copy up to the attribute to be ungrouped. We should always be able
     * to copy the old heading parts into an empty heading.
     */
    status = Ral_TupleHeadingAppend(tupleHeading,
	Ral_TupleHeadingBegin(tupleHeading), ungrpAttrIter,
	resultTupleHeading) ;
    assert(status != 0) ;
    /*
     * Copy the attributes after the one to be ungrouped to the end.
     */
    status = Ral_TupleHeadingAppend(tupleHeading, ungrpAttrIter + 1,
	Ral_TupleHeadingEnd(tupleHeading), resultTupleHeading) ;
    assert(status != 0) ;
    /*
     * Copy the heading from the ungrouped attribute itself.
     * Now we have to deal with an attribute in the ungrouped relation
     * that is the same as another attribute already in the heading.
     */
    status = Ral_TupleHeadingAppend(attrTupleHeading,
	Ral_TupleHeadingBegin(attrTupleHeading),
	Ral_TupleHeadingEnd(attrTupleHeading), resultTupleHeading) ;
    if (status == 0) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR,
	    "while ungrouping relation") ;
	Ral_TupleHeadingDelete(resultTupleHeading) ;
	return NULL ;
    }
    /*
     * The number of identifiers for the new relation is the product
     * of the number of the original relation and that of the ungrouped
     * attribute.
     */
    resultHeading = Ral_RelationHeadingNew(resultTupleHeading,
	relation->heading->idCount * attrHeading->idCount) ;
    /*
     * Need to create the identifiers for the new ungrouped relation.  The
     * identifiers are the cross product of the identifiers in the original
     * relation and those of the relation in the ungrouped attribute.  Two
     * complications: (1) If the ungrouped attribute appears in an identifier
     * of the original relation, it must be removed. (2) The indices in the
     * ungrouped attribute relation must be offset properly to match the new
     * relation heading.
     */
    ungrpIndex = ungrpAttrIter - Ral_TupleHeadingBegin(tupleHeading) ;
    assert(ungrpIndex >= 0) ;
    indexOffset = Ral_RelationHeadingDegree(heading) - 1 ;
    assert(indexOffset >= 0) ;
    relIdEnd = Ral_RelationHeadingIdEnd(heading) ;
    attrIdEnd = Ral_RelationHeadingIdEnd(attrHeading) ;
    for (relIdIter = Ral_RelationHeadingIdBegin(heading);
	relIdIter != relIdEnd ; ++relIdIter) {
	Ral_IntVector relId = *relIdIter ;
	Ral_IntVectorIter rEnd = Ral_IntVectorEnd(relId) ;
	int rSize = Ral_IntVectorSize(relId) ;

	for (attrIdIter = Ral_RelationHeadingIdBegin(attrHeading) ;
	    attrIdIter != attrIdEnd ; ++attrIdIter) {
	    Ral_IntVectorIter rIter ;
	    Ral_IntVector attrId = *attrIdIter ;
	    Ral_IntVectorIter aEnd = Ral_IntVectorEnd(attrId) ;
	    Ral_IntVectorIter aIter ;
	    int status ;
	    /*
	     * The new identifier size is that of the sum of the sizes
	     * of the identifier in the original relation plus that of the
	     * ungrouped attribute relation but less 1 if the ungrouped
	     * attribute was part of the identifier.
	     */
	    Ral_IntVector newId = Ral_IntVectorNew(rSize
		+ Ral_IntVectorSize(attrId) -
		(Ral_IntVectorFind(relId, ungrpIndex) != rEnd), -1) ;
	    Ral_IntVectorIter newIter = Ral_IntVectorBegin(newId) ;
	    /*
	     * Iterate through the attribute indices of the identifier from
	     * the original relation.
	     */
	    for (rIter = Ral_IntVectorBegin(relId) ; rIter != rEnd ; ++rIter) {
		Ral_IntVectorValueType i = *rIter ;
		if (i != ungrpIndex) {
		    *newIter++ = i > ungrpIndex ? i - 1 : i ;
		}
		/* else i == ungrpIndex, and it is eliminated */
	    }
	    /*
	     * Iterate through the attribute indices of the ungrouped attribute
	     * identifier. They must be offset by the number of attributes
	     * that already precede them in the result heading.
	     */
	    for (aIter = Ral_IntVectorBegin(attrId) ; aIter != aEnd ; ++aIter) {
		*newIter++ = *aIter + indexOffset ;
	    }
	    /*
	     * It should be the case that we covered all slots in the identifer
	     * vector, so we should not be able to find a "-1" in the vector.
	     */
	    assert(Ral_IntVectorFind(newId, -1) == Ral_IntVectorEnd(newId)) ;
	    /*
	     * Add the identifer to the new relation heading.
	     */
	    status = Ral_RelationHeadingAddIdentifier(resultHeading, idNum++,
		newId) ;
	    /*
	     * The cross product of two existing identifier sets should never
	     * yield an identifier that is a subset of another one.
	     */
	    assert(status != 0) ;
	}
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
	Ral_TupleIter ungrpAttr = tupleBegin + ungrpIndex ;
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
	    Ral_Tuple ungrpTuple = Ral_TupleNew(resultTupleHeading) ;
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
    Ral_RelationHeading r1Heading = r1->heading ;
    Ral_RelationHeading r2Heading = r2->heading ;
    Ral_RelationHeading joinHeading ;
    Ral_TupleHeading joinTupleHeading ;
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
    joinHeading = Ral_RelationHeadingJoin(r1Heading, r2Heading, map,
	&r2JoinAttrs, errInfo) ;
    if (joinHeading == NULL) {
	return NULL ;
    }
    joinTupleHeading = joinHeading->tupleHeading ;
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

	joinTuple = Ral_TupleNew(joinTupleHeading) ;
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
	    return NULL ;
	}
    }

    Ral_IntVectorDelete(r2JoinAttrs) ;

    return joinRel ;
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
    Ral_RelationHeading dendHeading = dend->heading ;
    Ral_TupleHeading dendTupleHeading = dendHeading->tupleHeading ;
    Ral_RelationHeading dsorHeading = dsor->heading ;
    Ral_TupleHeading dsorTupleHeading = dsorHeading->tupleHeading ;
    int dsorCard = Ral_RelationCardinality(dsor) ;
    Ral_RelationHeading medHeading = med->heading ;
    Ral_TupleHeading medTupleHeading = medHeading->tupleHeading ;
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
    if (Ral_TupleHeadingCommonAttributes(dendTupleHeading, dsorTupleHeading,
	NULL) != 0) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_NOT_DISJOINT,
	    "while computing quotient") ;
	return NULL ;
    }
    if (Ral_TupleHeadingCommonAttributes(dendTupleHeading, medTupleHeading,
	NULL) != Ral_TupleHeadingSize(dendTupleHeading) ||
	Ral_TupleHeadingCommonAttributes(dsorTupleHeading, medTupleHeading,
	NULL) != Ral_TupleHeadingSize(dsorTupleHeading)) {
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
    trialTupleHeading = Ral_TupleHeadingUnion(dendTupleHeading,
	dsorTupleHeading) ;
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
	    matches += Ral_RelationFind(med, 0, trialTuple, trialOrder)
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

    assert(Ral_RelationHeadingEqual(src->heading, dst->heading)) ;

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
	    int r1Index = Ral_RelationFindTupleReference(r1, idNum, *r2Iter,
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
		int r2Index = Ral_RelationFindTupleReference(r2, idNum, *r1Iter,
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
    Ral_RelationHeading heading = relation->heading ;
    Ral_TupleHeading tupleHeading = heading->tupleHeading ;
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
		Ral_Tuple tuple = Ral_TupleNew(tupleHeading) ;
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
    Ral_TupleHeading heading = props->relation->heading->tupleHeading ;
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

Ral_RelationIter
Ral_RelationFindKey(
    Ral_Relation relation,
    int idNum,
    Ral_Tuple tuple,
    Ral_IntVector map)
{
    int tupleIndex = Ral_RelationFindTupleIndex(relation, idNum, tuple, map) ;

    assert(tupleIndex < Ral_RelationCardinality(relation)) ;
    /*
     * Check if a tuple with the identifier was found
     * N.B. that we don't look at any attributes other than the ones
     * in the identifier.
     */
    return tupleIndex >= 0 ?  relation->start + tupleIndex : relation->finish ;
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
    int idNum,
    Ral_Tuple tuple,
    Ral_IntVector map)
{
    Ral_RelationIter found = Ral_RelationFindKey(relation, idNum, tuple, map) ;
    /*
     * Check if a tuple with the identifier was found
     * and that the values of the tuple are equal.
     */
    return found != relation->finish && Ral_TupleEqualValues(tuple, *found) ?
	    found : relation->finish ;
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
	Ral_RelationRemoveTupleIndex(relation, *iter) ;
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
	int status = Ral_RelationIndexTuple(relation, *iter, iter) ;
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
    if (!Ral_RelationHeadingEqual(r1->heading, r2->heading)) {
	return -1 ;
    }
    /*
     * Reordering may be necessary. Since we will search
     * "r2" to find matching tuples from "r1", we must compute
     * the map onto "r2" from "r1".
     */
    orderMap = Ral_TupleHeadingNewOrderMap(r2->heading->tupleHeading,
	r1->heading->tupleHeading) ;
    /*
     * Iterate through the tuples in the first relation and search
     * the second one for a match. Count the number of matches.
     */
    for (iter1 = Ral_RelationBegin(r1) ; iter1 != end1 ; ++iter1) {
	if (Ral_RelationFind(r2, 0, *iter1, orderMap) != end2) {
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
    Ral_TupleHeading tupleHeading = relation->heading->tupleHeading ;
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
    Ral_RelationHeading heading = relation->heading ;
    int length ;
    /*
     * Scan the header.
     * +1 for space
     */
    length = Ral_RelationHeadingScan(heading, typeFlags) + 1 ;
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
     * Convert the heading.
     */
    p += Ral_RelationHeadingConvert(relation->heading, p, typeFlags) ;
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
    Ral_TupleHeading tupleHeading = relation->heading->tupleHeading ;
    int nonEmptyHeading = !Ral_TupleHeadingEmpty(tupleHeading) ;
    Ral_TupleHeadingIter hEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter hIter ;

    assert(typeFlags->attrType == Relation_Type) ;
    assert(typeFlags->compoundFlags.count == Ral_RelationDegree(relation)) ;
    assert(valueFlags->attrType == Relation_Type) ;
    assert(valueFlags->compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the value flags.
     * For a relation we need a flag structure for each value in each tuple.
     */
    valueFlags->compoundFlags.count =
	typeFlags->compoundFlags.count * Ral_RelationCardinality(relation) ;
    nBytes = valueFlags->compoundFlags.count *
	sizeof(*valueFlags->compoundFlags.flags) ;
    valueFlags->compoundFlags.flags =
	(Ral_AttributeValueScanFlags *)ckalloc(nBytes) ;
    memset(valueFlags->compoundFlags.flags, 0, nBytes) ;

    length = sizeof(openList) ;

    valueFlag = valueFlags->compoundFlags.flags ;
    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;
	typeFlag = typeFlags->compoundFlags.flags ;

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
    Ral_TupleHeading tupleHeading = relation->heading->tupleHeading ;
    int nonEmptyHeading = !Ral_TupleHeadingEmpty(tupleHeading) ;
    Ral_TupleHeadingIter hEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter hIter ;

    assert(typeFlags->attrType == Relation_Type) ;
    assert(typeFlags->compoundFlags.count == Ral_RelationDegree(relation)) ;
    assert(valueFlags->attrType == Relation_Type) ;
    assert(valueFlags->compoundFlags.count ==
	Ral_RelationDegree(relation) * Ral_RelationCardinality(relation)) ;

    valueFlag = valueFlags->compoundFlags.flags ;
    *p++ = openList ;
    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;
	typeFlag = typeFlags->compoundFlags.flags ;

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
	if (nonEmptyHeading) {
	    --p ;
	}
	*p++ = closeList ;
	*p++ = ' ' ;
    }
    /*
     * Remove the trailing space. Check that the relation actually had
     * some values!
     */
    if (Ral_RelationCardinality(relation)) {
	--p ;
    }
    *p++ = closeList ;

    return p - dst ;
}

void
Ral_RelationPrint(
    Ral_Relation relation,
    const char *format,
    FILE *f)
{
    char *str = Ral_RelationStringOf(relation) ;
    fprintf(f, format, str) ;
    ckfree(str) ;
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
    Ral_RelationHeading heading = relation->heading ;
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
    Ral_RelationHeadingScan(heading, &typeFlags) ;
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
 * Generate a key string by concatenating the string representation of
 * the attributes that are part of an identifier. If "orderMap" is not
 * NULL it is used to reorder to attribute access in "tuple". This allows
 * "tuple" to be of a different order than the tuple heading to which "idMap"
 * applies.
 */
const char *
Ral_RelationGetIdKey(
    Ral_Tuple tuple,
    Ral_IntVector idMap,
    Ral_IntVector orderMap,
    Tcl_DString *idKey)
{
    Ral_IntVectorIter iter ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(idMap) ;

    Tcl_DStringInit(idKey) ;
    for (iter = Ral_IntVectorBegin(idMap) ; iter != end ; ++iter) {
	int attrIndex = orderMap ? Ral_IntVectorFetch(orderMap, *iter) : *iter ;
	assert(attrIndex < Ral_TupleDegree(tuple)) ;
	Tcl_DStringAppend(idKey, Tcl_GetString(tuple->values[attrIndex]), -1) ;
    }

    return Tcl_DStringValue(idKey) ;
}


const char *
Ral_RelationVersion(void)
{
    return rcsid ;
}

/*
 * PRIVATE FUNCTIONS
 */

static Tcl_HashEntry *
Ral_RelationFindIndexEntry(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    Tcl_DString idKey ;
    const char *id ;
    Tcl_HashEntry *entry ;

    assert(idIndex < relation->heading->idCount) ;

    id = Ral_RelationGetIdKey(tuple, relation->heading->identifiers[idIndex],
	orderMap, &idKey) ;
    entry = Tcl_FindHashEntry(relation->indexVector + idIndex, id) ;
    Tcl_DStringFree(&idKey) ;

    return entry ;
}

static int
Ral_RelationFindTupleIndex(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    Tcl_HashEntry *entry = Ral_RelationFindIndexEntry(relation, idIndex,
	tuple, orderMap) ;
    return entry ? (int)Tcl_GetHashValue(entry) : -1 ;
}

/*
 * Find a tuple in "relation" based on the values in "tuple". The attributes
 * to be used are given in the "attrSet". These attributes refer to the
 * same attributes as the identifier given by "idIndex".
 */
static int
Ral_RelationFindTupleReference(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple,
    Ral_IntVector attrSet)
{
    Tcl_DString idKey ;
    const char *id ;
    Tcl_HashEntry *entry ;

    assert(idIndex < relation->heading->idCount) ;
    assert(Ral_IntVectorSize(relation->heading->identifiers[idIndex]) ==
	Ral_IntVectorSize(attrSet)) ;

    id = Ral_RelationGetIdKey(tuple, attrSet, NULL, &idKey) ;
    entry = Tcl_FindHashEntry(relation->indexVector + idIndex, id) ;
    Tcl_DStringFree(&idKey) ;

    return entry ? (int)Tcl_GetHashValue(entry) : -1 ;
}

static void
Ral_RelationRemoveIndex(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple)
{
    Tcl_HashEntry *entry ;

    entry = Ral_RelationFindIndexEntry(relation, idIndex, tuple, NULL) ;
    assert (entry != NULL) ;
    Tcl_DeleteHashEntry(entry) ;
}

static int
Ral_RelationIndexIdentifier(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple,
    Ral_RelationIter where)
{
    Ral_IntVector idMap ;
    Tcl_HashTable *index ;
    Tcl_DString idKey ;
    const char *id ;
    Tcl_HashEntry *entry ;
    int newPtr ;

    assert(idIndex < relation->heading->idCount) ;

    idMap = relation->heading->identifiers[idIndex] ;
    index = relation->indexVector + idIndex ;

    id = Ral_RelationGetIdKey(tuple, idMap, NULL, &idKey) ;
    entry = Tcl_CreateHashEntry(index, id, &newPtr) ;
    Tcl_DStringFree(&idKey) ;
    /*
     * Check that an entry was actually created and if so, set its value.
     */
    if (newPtr) {
	Tcl_SetHashValue(entry, where - relation->start) ;
    }

    return newPtr ;
}

static int
Ral_RelationIndexTuple(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_RelationIter where)
{
    int i ;
    Ral_RelationHeading heading = relation->heading ;

    assert(heading->idCount > 0) ;

    for (i = 0 ; i < heading->idCount ; ++i) {
	if (!Ral_RelationIndexIdentifier(relation, i, tuple, where)) {
	    /*
	     * Need to back out any index that was successfully done.
	     */
	    int j ;
	    for (j = 0 ; j < i ; ++j) {
		Ral_RelationRemoveIndex(relation, j, tuple) ;
	    }
	    return 0 ;
	}
    }

    return 1 ;
}

static void
Ral_RelationRemoveTupleIndex(
    Ral_Relation relation,
    Ral_Tuple tuple)
{
    int i ;

    for (i = 0 ; i < relation->heading->idCount ; ++i) {
	Ral_RelationRemoveIndex(relation, i, tuple) ;
    }
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
    Ral_IntVector mapAttrs = Ral_JoinMapGetAttr(map, offset) ;
    int idNum = Ral_RelationHeadingFindIdentifier(rel->heading, mapAttrs) ;
    Ral_IntVectorDelete(mapAttrs) ;
    return idNum ;
}
