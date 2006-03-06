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

$RCSfile: ral_relation.c,v $
$Revision: 1.5 $
$Date: 2006/03/06 01:07:37 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relation.c,v $
Revision 1.5  2006/03/06 01:07:37  mangoa01
More relation commands done. Cleaned up error reporting.

Revision 1.4  2006/03/01 02:28:40  mangoa01
Added new relation commands and test cases. Cleaned up Makefiles.

Revision 1.3  2006/02/26 04:57:53  mangoa01
Reworked the conversion from internal form to a string yet again.
This design is better and more recursive in nature.
Added additional code to the "relation" commands.
Now in a position to finish off the remaining relation commands.

Revision 1.2  2006/02/20 20:15:07  mangoa01
Now able to convert strings to relations and vice versa including
tuple and relation valued attributes.

Revision 1.1  2006/02/06 05:02:45  mangoa01
Started on relation heading and other code refactoring.
This is a checkpoint after a number of added files and changes
to tuple heading code.


 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_relation.h"
#include "ral_relationheading.h"
#include "ral_tupleheading.h"
#include <stdlib.h>
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
static Tcl_DString *Ral_RelationGetIdKey(Ral_Tuple, Ral_IntVector,
    Ral_IntVector) ;
static Tcl_HashEntry *Ral_RelationFindIndexEntry(Ral_Relation, int, Ral_Tuple,
    Ral_IntVector) ;
static int Ral_RelationFindTupleIndex(Ral_Relation, int, Ral_Tuple,
    Ral_IntVector) ;
static void Ral_RelationRemoveIndex(Ral_Relation, int, Ral_Tuple) ;
static int Ral_RelationIndexIdentifier(Ral_Relation, int, Ral_Tuple,
    Ral_RelationIter) ;
static int Ral_RelationIndexTuple(Ral_Relation, Ral_Tuple, Ral_RelationIter) ;
static void Ral_RelationRemoveTupleIndex(Ral_Relation, Ral_Tuple) ;
static void Ral_RelationTupleSortKey(const Ral_Tuple, Tcl_DString *) ;
static int Ral_RelationTupleCompareAscending(const void *, const void *) ;
static int Ral_RelationTupleCompareDescending(const void *, const void *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/
Ral_RelationError Ral_RelationLastError = REL_OK ;

/*
STATIC DATA ALLOCATION
*/
static Ral_IntVector sortAttrs ;
static const char openList = '{' ;
static const char closeList = '}' ;
static const char rcsid[] = "@(#) $RCSfile: ral_relation.c,v $ $Revision: 1.5 $" ;

/*
FUNCTION DEFINITIONS
*/

Ral_Relation
Ral_RelationNew(
    Ral_RelationHeading heading)
{
    int nBytes ;
    Ral_Relation relation ;
    int c ;
    Tcl_HashTable *indexVector ;

    nBytes = sizeof(*relation) +
	heading->idCount * sizeof(*relation->indexVector) ;
    relation = (Ral_Relation)ckalloc(nBytes) ;
    memset(relation, 0, nBytes) ;

    Ral_RelationHeadingReference(relation->heading = heading) ;
    relation->indexVector = (Tcl_HashTable *)(relation + 1) ;
    for (c = heading->idCount, indexVector = relation->indexVector ;
	c > 0 ; --c, ++indexVector) {
	Tcl_InitHashTable(indexVector, TCL_STRING_KEYS) ;
    }

    return relation ;
}

Ral_Relation
Ral_RelationDup(
    Ral_Relation srcRelation)
{
    int degree = Ral_RelationDegree(srcRelation) ;
    Ral_RelationHeading srcHeading = srcRelation->heading ;
    Ral_RelationHeading dupHeading ;
    Ral_Relation dupRelation ;
    Ral_TupleHeading dupTupleHeading ;
    Ral_RelationIter srcIter ;
    Ral_RelationIter srcEnd = srcRelation->finish ;

    dupHeading = Ral_RelationHeadingDup(srcHeading) ;
    if (!dupHeading) {
	return NULL ;
    }
    dupRelation = Ral_RelationNew(dupHeading) ;
    Ral_RelationReserve(dupRelation, Ral_RelationCardinality(srcRelation)) ;

    dupTupleHeading = dupHeading->tupleHeading ;
    for (srcIter = srcRelation->start ; srcIter != srcEnd ; ++srcIter) {
	Ral_Tuple srcTuple = *srcIter ;
	Ral_Tuple dupTuple ;
	int appended ;

	dupTuple = Ral_TupleNew(dupTupleHeading) ;
	Ral_TupleCopyValues(srcTuple->values, srcTuple->values + degree,
	    dupTuple->values) ;

	appended = Ral_RelationPushBack(dupRelation, dupTuple, NULL) ;
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

int
Ral_RelationPushBack(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    Ral_Tuple insertTuple ;
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

    insertTuple = orderMap ?  Ral_TupleDupOrdered(tuple,
	relation->heading->tupleHeading, orderMap) : tuple ;

    if (Ral_RelationIndexTuple(relation, insertTuple, relation->finish)) {
	Ral_TupleReference(*relation->finish++ = insertTuple) ;
	return 1 ;
    }
    if (orderMap) {
	Ral_TupleDelete(insertTuple) ;
    }

    return 0 ;
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
	 * install the new one into place.
	 */
	Ral_TupleUnreference(*pos) ;
	Ral_TupleReference(*pos = newTuple) ;
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

Ral_Relation
Ral_RelationUnion(
    Ral_Relation r1,
    Ral_Relation r2)
{
    Ral_Relation unionRel ;
    int copyStatus ;
    Ral_RelationIter iter2 ;
    Ral_RelationIter end2 = Ral_RelationEnd(r2) ;
    Ral_IntVector orderMap ;
    /*
     * Headings must be equal to perform a union.
     */
    if (!Ral_RelationHeadingEqual(r1->heading, r2->heading)) {
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
     * Iterate through the tuples of the second relation and attempt to insert
     * them into the union.  Just push it in and ignore any return status. If
     * there is already a matching tuple it will not be inserted.
     */
    for (iter2 = Ral_RelationBegin(r2) ; iter2 != end2 ; ++iter2) {
	Ral_RelationPushBack(unionRel, *iter2, orderMap) ;
    }

    Ral_IntVectorDelete(orderMap) ;

    return unionRel ;
}

Ral_Relation
Ral_RelationIntersect(
    Ral_Relation r1,
    Ral_Relation r2)
{
    Ral_Relation intersectRel ;
    Ral_IntVector orderMap ;
    Ral_RelationIter iter1 ;
    Ral_RelationIter end1 = Ral_RelationEnd(r1) ;
    Ral_RelationIter end2 = Ral_RelationEnd(r2) ;
    /*
     * Headings must be equal to perform a union.
     */
    if (!Ral_RelationHeadingEqual(r1->heading, r2->heading)) {
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
    Ral_Relation r2)
{
    Ral_Relation diffRel ;
    Ral_IntVector orderMap ;
    Ral_RelationIter iter1 ;
    Ral_RelationIter end1 = Ral_RelationEnd(r1) ;
    Ral_RelationIter end2 = Ral_RelationEnd(r2) ;
    /*
     * Headings must be equal to perform a union.
     */
    if (!Ral_RelationHeadingEqual(r1->heading, r2->heading)) {
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
Ral_RelationSortAscending(
    Ral_Relation relation,
    Ral_IntVector attrs)
{
    Ral_Relation sortedRel = Ral_RelationDup(relation) ;
    sortAttrs = attrs ;

    qsort(Ral_RelationBegin(sortedRel), Ral_RelationCardinality(sortedRel),
	sizeof(Ral_RelationIter), Ral_RelationTupleCompareAscending) ;
    sortAttrs = NULL ;

    return sortedRel ;
}

Ral_Relation
Ral_RelationSortDescending(
    Ral_Relation relation,
    Ral_IntVector attrs)
{
    Ral_Relation sortedRel = Ral_RelationDup(relation) ;
    sortAttrs = attrs ;

    qsort(Ral_RelationBegin(sortedRel), Ral_RelationCardinality(sortedRel),
	sizeof(Ral_RelationIter), Ral_RelationTupleCompareDescending) ;
    sortAttrs = NULL ;

    return sortedRel ;
}


/*
 * Find a tuple in a relation.
 * If "map" is NULL, then "tuple" is assumed to be ordered the same as the
 * tuple heading in "relation".  Othewise, "map" provides the reordering
 * mapping.
 */
Ral_RelationIter
Ral_RelationFind(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_IntVector map)
{
    /*
     * We can index the tuple based on any identifier. We choose
     * identifier 0 since it always exists.
     */
    int tupleIndex = Ral_RelationFindTupleIndex(relation, 0, tuple, map) ;

    assert(tupleIndex < Ral_RelationCardinality(relation)) ;
    /*
     * Check if a tuple with the identifier was found
     * and that the values of the tuple are equal.
     */
    return tupleIndex >= 0 &&
	Ral_TupleEqualValues(tuple, relation->start[tupleIndex]) ?
	    relation->start + tupleIndex : relation->finish ;
}

Ral_Tuple
Ral_RelationFetch(
    Ral_Relation relation,
    Ral_RelationIter pos)
{
    return *relation->start ;
}

void
Ral_RelationErase(
    Ral_Relation relation,
    Ral_RelationIter start,
    Ral_RelationIter finish)
{
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
    const char *newName)
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
	Ral_RelationLastError = REL_UNKNOWN_ATTR ;
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
	Ral_RelationLastError = REL_DUPLICATE_ATTR ;
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


const char *
Ral_RelationVersion(void)
{
    return rcsid ;
}

/*
 * PRIVATE FUNCTIONS
 */

/*
 * Generate a key string by concatenating the string representation of
 * the attributes that are part of an identifier. If "orderMap" is not
 * NULL it is used to reorder to attribute access in "tuple". This allows
 * "tuple" to be of a different order than the tuple heading to which "idMap"
 * applies.
 */
static Tcl_DString *
Ral_RelationGetIdKey(
    Ral_Tuple tuple,
    Ral_IntVector idMap,
    Ral_IntVector orderMap)
{
    static Tcl_DString idKey ;

    Ral_IntVectorIter iter ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(idMap) ;

    Tcl_DStringInit(&idKey) ;
    for (iter = Ral_IntVectorBegin(idMap) ; iter != end ; ++iter) {
	int attrIndex = orderMap ? Ral_IntVectorFetch(orderMap, *iter) : *iter ;
	assert(attrIndex < Ral_TupleDegree(tuple)) ;
	Tcl_DStringAppend(&idKey, Tcl_GetString(tuple->values[attrIndex]), -1) ;
    }

    return &idKey ;
}

static Tcl_HashEntry *
Ral_RelationFindIndexEntry(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple,
    Ral_IntVector orderMap)
{
    Tcl_DString *idKey ;
    Tcl_HashEntry *entry ;

    assert(idIndex < relation->heading->idCount) ;

    idKey = Ral_RelationGetIdKey(tuple, relation->heading->identifiers[idIndex],
	orderMap) ;
    entry = Tcl_FindHashEntry(relation->indexVector + idIndex,
	Tcl_DStringValue(idKey)) ;
    Tcl_DStringFree(idKey) ;

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
    Tcl_DString *idKey ;
    Tcl_HashEntry *entry ;
    int newPtr ;

    assert(idIndex < relation->heading->idCount) ;

    idMap = relation->heading->identifiers[idIndex] ;
    index = relation->indexVector + idIndex ;

    idKey = Ral_RelationGetIdKey(tuple, idMap, NULL) ;
    entry = Tcl_CreateHashEntry(index, Tcl_DStringValue(idKey), &newPtr) ;
    Tcl_DStringFree(idKey) ;
    /*
     * Check that there are no duplicate tuples.
     */
    if (newPtr == 0) {
	Ral_RelationLastError = REL_DUPLICATE_TUPLE ;
	return 0 ;
    }

    Tcl_SetHashValue(entry, where - relation->start) ;
    return 1 ;
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
Ral_RelationTupleSortKey(
    const Ral_Tuple tuple,
    Tcl_DString *idKey)
{
    Ral_IntVectorIter end = Ral_IntVectorEnd(sortAttrs) ;
    Ral_IntVectorIter iter ;
    Ral_TupleIter values = tuple->values ;

    Tcl_DStringInit(idKey) ;
    for (iter = Ral_IntVectorBegin(sortAttrs) ; iter != end ; ++iter) {
	Tcl_DStringAppend(idKey, Tcl_GetString(values[*iter]), -1) ;
    }
}

static int
Ral_RelationTupleCompareAscending(
    const void *v1,
    const void *v2)
{
    Tcl_DString sortKey1 ;
    Tcl_DString sortKey2 ;
    int result ;

    Ral_RelationTupleSortKey(*(Ral_RelationIter)v1, &sortKey1) ;
    Ral_RelationTupleSortKey(*(Ral_RelationIter)v2, &sortKey2) ;

    result = strcmp(Tcl_DStringValue(&sortKey1), Tcl_DStringValue(&sortKey2)) ;

    Tcl_DStringFree(&sortKey1) ;
    Tcl_DStringFree(&sortKey2) ;

    return result ;
}

static int
Ral_RelationTupleCompareDescending(
    const void *v1,
    const void *v2)
{
    Tcl_DString sortKey1 ;
    Tcl_DString sortKey2 ;
    int result ;

    Ral_RelationTupleSortKey(*(Ral_RelationIter)v1, &sortKey1) ;
    Ral_RelationTupleSortKey(*(Ral_RelationIter)v2, &sortKey2) ;

    /*
     * N.B. inverted order of first and second tuple to obtain
     * the proper result for descending order.
     */
    result = strcmp(Tcl_DStringValue(&sortKey2), Tcl_DStringValue(&sortKey1)) ;

    Tcl_DStringFree(&sortKey1) ;
    Tcl_DStringFree(&sortKey2) ;

    return result ;
}
