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
$Revision: 1.36.2.5 $
$Date: 2009/02/08 19:04:44 $
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
static int Ral_RelationIndexTuple(Ral_Relation, Ral_Tuple, Ral_RelationIter) ;
static int Ral_RelationUpdateTupleIndex(Ral_Relation, Ral_Tuple,
        Ral_RelationIter) ;
static void Ral_RelationIndexByAttrs(Tcl_HashTable *, Ral_Relation,
        Ral_IntVector) ;

static unsigned int tupleHashGenKey(Tcl_HashTable *, void *) ;
static int tupleHashCompareKeys(void *, Tcl_HashEntry *) ;
static Tcl_HashEntry *tupleHashEntryAlloc(Tcl_HashTable *, void *) ;
static void tupleHashEntryFree(Tcl_HashEntry *) ;

static unsigned int tupleAttrHashGenKey(Tcl_HashTable *, void *) ;
static int tupleAttrHashCompareKeys(void *, Tcl_HashEntry *) ;
static Tcl_HashEntry *tupleAttrHashEntryAlloc(Tcl_HashTable *, void *) ;
static void tupleAttrHashEntryFree(Tcl_HashEntry *) ;

static Tcl_HashEntry *tupleAttrHashMultiEntryAlloc(Tcl_HashTable *, void *) ;
static void tupleAttrHashMultiEntryFree(Tcl_HashEntry *) ;

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

/*
 * We use three custom hash tables with relation values.
 * The first one is used to hash an entire Tuple. All the attributes are hashed
 * and this type of table is kept as part of the relation data structure in
 * order to be able to determine if a given Tuple may be inserted into the
 * relation. The key is the Tuple reference itself and the hash value is the
 * index into the relation where the tuple is stored. This is used to enforce
 * the constraint that relations are sets of tuples with no duplicates.
 */
static Tcl_HashKeyType tupleHashType = {
    TCL_HASH_KEY_TYPE_VERSION,  /* version */
    TCL_HASH_KEY_RANDOMIZE_HASH,/* flags */
    tupleHashGenKey,            /* hashKeyProc */
    tupleHashCompareKeys,       /* compareKeysProc */
    tupleHashEntryAlloc,        /* allocEntryProc */
    tupleHashEntryFree,         /* freeEntryProc */
} ;

/*
 * The second type of custom hash table is used to index identifiers for
 * relvars. The key is a reference to a tuple plus a vector of attribute
 * indices. The attribute index vector gives the offset into the tuple heading
 * for the subset of attributes that are to be hashed. The client data is the
 * offset into the relation where the tuple that contains the attributes is
 * located. This is used to enforce identifier constraints in relvars.
 */
Tcl_HashKeyType tupleAttrHashType = {
    TCL_HASH_KEY_TYPE_VERSION,  /* version */
    TCL_HASH_KEY_RANDOMIZE_HASH,/* flags */
    tupleAttrHashGenKey,        /* hashKeyProc */
    tupleAttrHashCompareKeys,   /* compareKeysProc */
    tupleAttrHashEntryAlloc,    /* allocEntryProc */
    tupleAttrHashEntryFree,     /* freeEntryProc */
} ;

/*
 * The third type of custom hash table is used when we need to create an index
 * on a subset of attributes but the possiblity exists that the attributes do
 * not form an identifier. That is it is possible for many tuples in a relation
 * to have the same attribute values. In this case the key is the same as the
 * hash table above, but the clientData is a vector of tuple offsets into the
 * relation. This type of hash table is used in join and all its derivatives to
 * find all the tuples that match values from a given tuple.
 */
static Tcl_HashKeyType tupleAttrHashMultiType = {
    TCL_HASH_KEY_TYPE_VERSION,      /* version */
    TCL_HASH_KEY_RANDOMIZE_HASH,    /* flags */
    tupleAttrHashGenKey,            /* hashKeyProc */
    tupleAttrHashCompareKeys,       /* compareKeysProc */
    tupleAttrHashMultiEntryAlloc,   /* allocEntryProc */
    tupleAttrHashMultiEntryFree,    /* freeEntryProc */
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

    for (srcIter = Ral_RelationBegin(srcRelation) ;
            srcIter != Ral_RelationEnd(srcRelation) ; ++srcIter) {
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

    dupRelation = Ral_RelationNew(srcRelation->heading) ;
    Ral_RelationReserve(dupRelation, Ral_RelationCardinality(srcRelation)) ;

    for (srcIter = Ral_RelationBegin(srcRelation) ;
            srcIter != Ral_RelationEnd(srcRelation) ; ++srcIter) {
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

    for (iter = Ral_RelationBegin(relation) ;
            iter != Ral_RelationEnd(relation) ; ++iter) {
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

    status = Ral_RelationIndexTuple(relation, insertTuple, relation->finish) ;
    if (status) {
	Ral_TupleReference(*relation->finish++ = insertTuple) ;
    }

    Ral_TupleUnreference(insertTuple) ;
    Ral_TupleUnreference(tuple) ;

    return status ;
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
    int result = 0 ;
    Ral_Tuple newTuple ;
    Ral_RelationIter found ;

    if (pos >= relation->finish) {
	Tcl_Panic("Ral_RelationUpdate: attempt to update non-existant tuple") ;
    }

    assert(*pos != NULL) ;
    assert(pos < Ral_RelationEnd(relation)) ;

    /*
     * If reordering is required, we create a new tuple with the correct
     * attribute ordering to match the relation.
     * Again, careful management of the tuple reference counts.
     */
    Ral_TupleReference(tuple) ;
    newTuple = orderMap ?
	Ral_TupleDupOrdered(tuple, relation->heading, orderMap) : tuple ;
    Ral_TupleReference(newTuple) ;
    /*
     * Hash the new tuple to determine if it is unique.
     */
    found = Ral_RelationFind(relation, newTuple, NULL) ;
    if (found != Ral_RelationEnd(relation)) {
        int status ;
	/*
         * If the new tuple is unique, then discard the old one and install the
         * new one into the same place. Note that we increment the reference
         * count of the new tuple before decrementing the reference count of
         * the old one, just in case that they are the same tuple (i.e. we are
         * updating the same physical location).
	 */
        status = Ral_RelationIndexTuple(relation, newTuple, pos) ;
        assert(status != 0) ;
	Ral_TupleReference(newTuple) ;
	Ral_TupleUnreference(*pos) ;
	*pos = newTuple ;
        result = 1 ;
    }
    Ral_TupleUnreference(newTuple) ;
    Ral_TupleUnreference(tuple) ;

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
    Ral_RelationIter mlierIter ;

    prodHeading = Ral_TupleHeadingUnion(multiplicand->heading,
	multiplier->heading) ;
    if (!prodHeading) {
	return NULL ;
    }

    product = Ral_RelationNew(prodHeading) ;
    Ral_RelationReserve(product, Ral_RelationCardinality(multiplicand) *
	Ral_RelationCardinality(multiplier)) ;

    for (mcandIter = Ral_RelationBegin(multiplicand) ;
            mcandIter != Ral_RelationEnd(multiplicand) ; ++mcandIter) {
	for (mlierIter = Ral_RelationBegin(multiplier) ;
                mlierIter != Ral_RelationEnd(multiplier) ; ++mlierIter) {
	    Ral_Tuple mcandTuple = *mcandIter ;
	    Ral_Tuple mlierTuple = *mlierIter ;
	    Ral_Tuple prodTuple = Ral_TupleNew(prodHeading) ;
            int status ;

	    Ral_TupleCopyValues(Ral_TupleBegin(mcandTuple),
                    Ral_TupleEnd(mcandTuple), Ral_TupleBegin(prodTuple)) ;
	    Ral_TupleCopyValues(Ral_TupleBegin(mlierTuple),
                    Ral_TupleEnd(mlierTuple), Ral_TupleBegin(prodTuple) +
                    Ral_RelationDegree(multiplicand)) ;

            status = Ral_RelationPushBack(product, prodTuple, NULL) ;
            assert(status != 0) ;
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
    Ral_RelationIter iter ;

    Ral_RelationReserve(projRel, Ral_RelationCardinality(relation)) ;
    for (iter = Ral_RelationBegin(relation) ;
            iter != Ral_RelationEnd(relation) ; ++iter) {
	Ral_Tuple projTuple = Ral_TupleSubset(*iter, projHeading, attrSet) ;
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
    char const *newAttrName,
    Ral_IntVector grpAttrs)
{
    Ral_TupleHeading heading = relation->heading ;
    Ral_TupleHeading attrHeading ;
    Ral_IntVector nongrpAttrs ;
    Ral_IntVectorIter ivIter ;
    Ral_TupleHeading grpHeading ;
    Ral_TupleHeadingIter grpAttrIter ;
    Ral_Relation group ;
    Tcl_HashTable nongrpIndex ;
    Tcl_HashSearch search ;
    Tcl_HashEntry *entry ;
    int status ;
    /*
     * Compute the complement of the set of grouped attributes.  This is the
     * set of attributes that will remain in the heading of the resulting
     * grouped relation and it is the values of these attributes that we must
     * find in the input relation.
     *
     * N.B. Do this first because computing the set compliment will sort the
     * source vector into increasing numerical order. Do this sort before we
     * use it later.
     */
    nongrpAttrs = Ral_IntVectorSetComplement(grpAttrs,
            Ral_TupleHeadingSize(heading)) ;
    /*
     * Create the tuple heading for the grouped attribute.
     */
    attrHeading = Ral_TupleHeadingSubset(heading, grpAttrs) ;
    /*
     * The grouped relation has a heading that contains all the original
     * attributes minus those that go into the relation valued attribute plus
     * one for the new relation valued attribute itself.
     */
    grpHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(heading) -
        Ral_IntVectorSize(grpAttrs) + 1) ;
    /*
     * Copy in the attributes from the input relation that are not part of the
     * grouped attribute.
     */
    for (ivIter = Ral_IntVectorBegin(nongrpAttrs) ;
            ivIter != Ral_IntVectorEnd(nongrpAttrs) ; ++ivIter) {
        Ral_TupleHeadingIter place ;

        assert(*ivIter < Ral_TupleHeadingSize(heading)) ;
        place = Ral_TupleHeadingBegin(heading) + *ivIter ;
        status = Ral_TupleHeadingAppend(heading, place, place + 1,
                grpHeading) ;
        /*
         * Since we are taking attributes from an existing relation,
         * we should not have any duplicated attribute names.
         */
        assert(status != 0) ;
    }
    /*
     * Add the new relation valued attribute to the heading.
     */
    grpAttrIter = Ral_TupleHeadingPushBack(grpHeading,
	Ral_AttributeNewRelationType(newAttrName, attrHeading)) ;
    assert(grpAttrIter != Ral_TupleHeadingEnd(grpHeading)) ;
    /*
     * Now that the heading is complete, we construct the new relation from
     * that heading.
     */
    group = Ral_RelationNew(grpHeading) ;
    /*
     * Index the input relation across the attributes that are not part
     * of the grouping. The resulting hash table has values of all the
     * tuples that have the same values for all the values of the ungrouped
     * attributes.
     */
    Ral_RelationIndexByAttrs(&nongrpIndex, relation, nongrpAttrs) ;
    /*
     * Now iterate through the hash table. The hash key references a
     * tuple from where we want to pull the values of the ungrouped attributes
     * and the hash value is a vector of tuple indices from where we can
     * get the values to build up the relation valued attribute.
     */
    for (entry = Tcl_FirstHashEntry(&nongrpIndex, &search) ; entry ;
            entry = Tcl_NextHashEntry(&search)) {
        Ral_TupleAttrHashKey key ;
        Ral_Tuple keyTuple ;
	Ral_Tuple grpTuple ;
        Ral_TupleIter grpIter ;
        Ral_IntVector tupindex ;
        Ral_Relation attrRel ;
        Tcl_Obj *attrObj ;

        key = (Ral_TupleAttrHashKey)Tcl_GetHashKey(&nongrpIndex, entry) ;
        /*
         * Copy the attribute values from the key tuple into a new tuple
         * that has the heading of the result relation. Only copy those
         * attributes that are not part of the grouped attribute.
         */
        keyTuple = key->tuple ;
        grpTuple = Ral_TupleNew(grpHeading) ;
        grpIter = Ral_TupleBegin(grpTuple) ;
        for (ivIter = Ral_IntVectorBegin(nongrpAttrs) ;
                ivIter != Ral_IntVectorEnd(nongrpAttrs) ; ++ivIter) {
            Ral_TupleIter src = Ral_TupleBegin(keyTuple) + *ivIter ;
            Ral_TupleCopyValues(src, src + 1, grpIter++) ;
        }
        /*
         * Now build up the relation valued attribute.
         */
        attrRel = Ral_RelationNew(attrHeading) ;
        attrObj = Ral_RelationObjNew(attrRel) ;
        Tcl_IncrRefCount(*grpIter = attrObj) ;
        /*
         * The hash value is a vector of tuple offsets that all have the same
         * values of the ungrouped attributes.
         */
        tupindex = (Ral_IntVector)Tcl_GetHashValue(entry) ;
        assert(Ral_IntVectorSize(tupindex) != 0) ;
        /*
         * We know exactly how many tuples will be in the relation valued
         * attribute.
         */
        Ral_RelationReserve(attrRel, Ral_IntVectorSize(tupindex)) ;
        /*
         * Iterate through the set of tuple indices to construct the tuples
         * for the valued attribute.
         */
        for (ivIter = Ral_IntVectorBegin(tupindex) ;
                ivIter != Ral_IntVectorEnd(tupindex) ; ++ivIter) {
            Ral_Tuple srcTuple ;
            Ral_Tuple attrTuple ;
            Ral_TupleIter attrTupleIter ;
            Ral_IntVectorIter gaIter ;
            /*
             * Copy values for the grouped attributes from the tuple in
             * the input relation to a new tuple that we will insert into
             * the relation valued attribute.
             */
            assert(*ivIter < Ral_RelationCardinality(relation)) ;
            srcTuple = *(Ral_RelationBegin(relation) + *ivIter) ;
            attrTuple = Ral_TupleNew(attrHeading) ;
            attrTupleIter = Ral_TupleBegin(attrTuple) ;
            for (gaIter = Ral_IntVectorBegin(grpAttrs) ;
                    gaIter != Ral_IntVectorEnd(grpAttrs) ; ++gaIter) {
                assert(*gaIter < Ral_TupleDegree(srcTuple)) ;
                Ral_TupleIter srcIter = Ral_TupleBegin(srcTuple) + *gaIter ;
                Ral_TupleCopyValues(srcIter, srcIter + 1, attrTupleIter++) ;
            }
            /*
             * Insert the tuple into the tuple valued attribute.
             * Ignore duplicates.
             */
            Ral_RelationPushBack(attrRel, attrTuple, NULL) ;
        }
        /*
         * Insert the newly constructed tuple into the result.
         */
        status = Ral_RelationPushBack(group, grpTuple, NULL) ;
        /*
         * Since we have partitioned the attributes we should always insert
         * the new tuple.
         */
        assert(status != 0) ;
    }
    Ral_IntVectorDelete(nongrpAttrs) ;
    Tcl_DeleteHashTable(&nongrpIndex) ;

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
    for (tupleMapIter = Ral_JoinMapTupleBegin(map) ;
            tupleMapIter != Ral_JoinMapTupleEnd(map) ; ++tupleMapIter) {
	Ral_Tuple r1Tuple = *(r1Begin + tupleMapIter->m[0]) ;
	Ral_Tuple r2Tuple = *(r2Begin + tupleMapIter->m[1]) ;
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
                r2TupleIter != Ral_TupleEnd(r2Tuple) ; ++r2TupleIter) {
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
 * Compose of two relations. Like join, except all the join attributes
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
    for (tupleMapIter = Ral_JoinMapTupleBegin(map) ;
            tupleMapIter != Ral_JoinMapTupleEnd(map) ; ++tupleMapIter) {
	Ral_Tuple r1Tuple = *(r1Begin + tupleMapIter->m[0]) ;
	Ral_Tuple r2Tuple = *(r2Begin + tupleMapIter->m[1]) ;
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
	attrMapIter = Ral_IntVectorBegin(r1JoinAttrs) ;
	for (tupleIter = Ral_TupleBegin(r1Tuple) ;
                tupleIter != Ral_TupleEnd(r1Tuple) ; ++tupleIter) {
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
	attrMapIter = Ral_IntVectorBegin(r2JoinAttrs) ;
	for (tupleIter = Ral_TupleBegin(r2Tuple) ;
                tupleIter != Ral_TupleEnd(r2Tuple) ; ++tupleIter) {
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
    for (tupleMapIter = Ral_JoinMapTupleBegin(map) ;
            tupleMapIter != Ral_JoinMapTupleEnd(map) ; ++tupleMapIter) {
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
    for (r2Iter = Ral_RelationBegin(r2) ; r2Iter != Ral_RelationEnd(r2) ;
            ++r2Iter) {
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
    Ral_RelationIter dendIter ;

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
    for (dendIter = Ral_RelationBegin(dend) ;
            dendIter != Ral_RelationEnd(dend) ; ++dendIter) {
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
	for (dsorIter = Ral_RelationBegin(dsor) ;
                dsorIter != Ral_RelationEnd(dsor) ; ++dsorIter) {
	    Ral_Tuple dsorTuple = *dsorIter ;
	    /*
	     * Copy in the tuple values from the divisor.
	     */
	    Ral_TupleCopyValues(Ral_TupleBegin(dsorTuple),
		Ral_TupleEnd(dsorTuple), trialIter) ;
	    /*
	     * Tally if we can find this tuple in the mediator.
	     */
	    matches += Ral_RelationFind(med, trialTuple, trialOrder) !=
                    Ral_RelationEnd(med) ;
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

Ral_Relation
Ral_RelationTag(
    Ral_Relation relation,
    char const *attrName,
    Ral_IntVector sortMap,
    Ral_ErrorInfo *errInfo)
{
    Ral_Attribute tagAttr ;
    Ral_TupleHeading tagHeading ;
    Ral_Relation tagRelation ;
    Ral_IntVectorIter mIter ;
    int tagValue = 0 ;
    /*
     * Make a new relation, adding the tag attribute.
     */
    tagAttr = Ral_AttributeNewTclType(attrName, "int") ;
    assert(tagAttr != NULL) ;
    tagHeading = Ral_TupleHeadingExtend(relation->heading, 1) ;
    if (Ral_TupleHeadingPushBack(tagHeading, tagAttr) ==
            Ral_TupleHeadingEnd(tagHeading)) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR, attrName) ;
	Ral_TupleHeadingDelete(tagHeading) ;
	return NULL ;
    }
    tagRelation = Ral_RelationNew(tagHeading) ;
    /*
     * Iterate through the relation in the order of the sort map adding
     * the tag attribute value.
     */
    for (mIter = Ral_IntVectorBegin(sortMap) ;
            mIter != Ral_IntVectorEnd(sortMap) ; ++mIter) {
        Ral_Tuple tuple = *(Ral_RelationBegin(relation) + *mIter) ;
        Ral_Tuple tagTuple = Ral_TupleNew(tagHeading) ;
        Ral_TupleIter tagIter = Ral_TupleBegin(tagTuple) ;
        int status ;

        tagIter += Ral_TupleCopyValues(Ral_TupleBegin(tuple),
                Ral_TupleEnd(tuple), tagIter) ;
        *tagIter = Tcl_NewIntObj(tagValue++) ;
        Tcl_IncrRefCount(*tagIter) ;
        /*
         * Should always be able to insert the tagged tuple since
         * the tuples values of the original relation were already unique.
         */
        status = Ral_RelationPushBack(tagRelation, tagTuple, NULL) ;
        assert(status != 0) ;
    }

    return tagRelation ;
}

/*
 * Like Ral_RelationTag(), but tag values are minimally unique within
 * a set of attributes.
 */
Ral_Relation
Ral_RelationTagWithin(
    Ral_Relation relation,
    char const *attrName,
    Ral_IntVector sortMap,
    Ral_IntVector withinAttrs,
    Ral_ErrorInfo *errInfo)
{
    Ral_Attribute tagAttr ;
    Ral_TupleHeading tagHeading ;
    Ral_Relation tagRelation ;
    Ral_IntVectorIter mIter ;
    Tcl_HashTable index ;
    /*
     * Make a new relation, adding the tag attribute.
     */
    tagAttr = Ral_AttributeNewTclType(attrName, "int") ;
    assert(tagAttr != NULL) ;
    tagHeading = Ral_TupleHeadingExtend(relation->heading, 1) ;
    if (Ral_TupleHeadingPushBack(tagHeading, tagAttr) ==
            Ral_TupleHeadingEnd(tagHeading)) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR, attrName) ;
	Ral_TupleHeadingDelete(tagHeading) ;
	return NULL ;
    }
    tagRelation = Ral_RelationNew(tagHeading) ;
    /*
     * Iterate through the relation in the order of the sort map adding the tag
     * attribute value. In order to find the groups of tuples that have the
     * same values for the given attribute set, we use a hash table.  The hash
     * entry value will be the current value of the tag attribute.
     */
    Tcl_InitCustomHashTable(&index, TCL_CUSTOM_PTR_KEYS, &tupleAttrHashType) ;
    for (mIter = Ral_IntVectorBegin(sortMap) ;
            mIter != Ral_IntVectorEnd(sortMap) ; ++mIter) {
        struct Ral_TupleAttrHashKey key ;
        Ral_Tuple tuple = *(Ral_RelationBegin(relation) + *mIter) ;
	Tcl_HashEntry *entry ;
	int newPtr ;
        int tagValue ;
        Ral_Tuple tagTuple = Ral_TupleNew(tagHeading) ;
        Ral_TupleIter tagIter = Ral_TupleBegin(tagTuple) ;
        int status ;

        /*
         * Put the tuple into the hash table based on the attribute set
         * we are interested in. We don't really care if a new entry is
         * created or not. If a new is created, the hash value will be
         * zero as that is what it is set to by the custom entry allocation
         * function.
         */
        key.tuple = tuple ;
        key.attrs = withinAttrs ;
        entry = Tcl_CreateHashEntry(&index, (char const *)&key, &newPtr) ;
        tagValue = (int)Tcl_GetHashValue(entry) ;
        /*
         * Copy the old tuple values over, adding the tag attrbute value.
         */
        tagIter += Ral_TupleCopyValues(Ral_TupleBegin(tuple),
                Ral_TupleEnd(tuple), tagIter) ;
        *tagIter = Tcl_NewIntObj(tagValue++) ;
        Tcl_IncrRefCount(*tagIter) ;
        /*
         * Update the tag value back into the hash entry.
         */
        Tcl_SetHashValue(entry, (ClientData)tagValue) ;
        /*
         * Should always be able to insert the tagged tuple since
         * the tuples values of the original relation were already unique.
         */
        status = Ral_RelationPushBack(tagRelation, tagTuple, NULL) ;
        assert(status != 0) ;
    }

    Tcl_DeleteHashTable(&index) ;
    return tagRelation ;
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
 * The strategy is to compute a hash of one relation that indexes the
 * join attributes for that relation and then iterate through the
 * other relation searching the hash for tuples that have the same
 * value of the attributes.
 *
 * We use the custom hash table "tupleAttrHashMultiType" to do this.
 */
void
Ral_RelationFindJoinTuples(
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap map)
{
    Tcl_HashTable r2Index ;
    Ral_IntVector r2Attrs ;
    Ral_IntVector r1Attrs ;
    Ral_RelationIter r1Iter ;
    /*
     * Index the "r2" relation by the join attributes in the join map.
     */
    r2Attrs = Ral_JoinMapGetAttr(map, 1) ;
    Ral_RelationIndexByAttrs(&r2Index, r2, r2Attrs) ;
    r1Attrs = Ral_JoinMapGetAttr(map, 0) ;
    /*
     * Foreach tuple in "r1", find the corresponding tuples in "r2" that
     * have the same values for the r1 join attributes. Record the matches
     * back into the join map.
     */
    for (r1Iter = Ral_RelationBegin(r1) ; r1Iter != Ral_RelationEnd(r1) ;
            ++r1Iter) {
        struct Ral_TupleAttrHashKey key ;
        Tcl_HashEntry *entry ;

        key.tuple =  *r1Iter ;
        key.attrs = r1Attrs ;
        entry = Tcl_FindHashEntry(&r2Index, (char const *)&key) ;
        if (entry) {
            Ral_IntVector r2TupIndices ;
            Ral_IntVectorIter r2TupIter ;
            /*
             * If we find an entry, then the hash value is a vector of
             * tuple indices in "r2" whose attribute values match those of
             * the key. Add the indices to the join map.
             */
            r2TupIndices = (Ral_IntVector)Tcl_GetHashValue(entry) ;
            for (r2TupIter = Ral_IntVectorBegin(r2TupIndices) ;
                    r2TupIter != Ral_IntVectorEnd(r2TupIndices) ; ++r2TupIter) {
                Ral_JoinMapAddTupleMapping(map, r1Iter - Ral_RelationBegin(r1),
                        *r2TupIter) ;
            }
        }
    }
    /*
     * Done. Clean up.
     */
    Tcl_DeleteHashTable(&r2Index) ;
    Ral_IntVectorDelete(r1Attrs) ;
    Ral_IntVectorDelete(r2Attrs) ;
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

    if (map) {
        Ral_Tuple newTuple ;

        newTuple = Ral_TupleDupOrdered(tuple, relation->heading, map) ;
        entry = Tcl_FindHashEntry(&relation->index, (char const *)newTuple) ;
        Ral_TupleDelete(newTuple) ;
    } else {
        entry = Tcl_FindHashEntry(&relation->index, (char const *)tuple) ;
    }

    return entry ? Ral_RelationBegin(relation) + (int)Tcl_GetHashValue(entry) :
            Ral_RelationEnd(relation) ;
}

/*
 * Create a new relation that contains a given set of tuples.
 */
Ral_Relation
Ral_RelationExtract(
    Ral_Relation relation,
    Ral_IntVector tupleSet)
{
    Ral_Relation subRel = Ral_RelationNew(relation->heading) ;
    Ral_IntVectorIter iter ;

    Ral_RelationReserve(subRel, Ral_IntVectorSize(tupleSet)) ;

    for (iter = Ral_IntVectorBegin(tupleSet) ;
            iter != Ral_IntVectorEnd(tupleSet) ; ++iter) {
	int status ;

	status = Ral_RelationPushBack(subRel,
                *(Ral_RelationBegin(relation) + *iter), NULL) ;
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
     * Remove a reference from the tuples from first to last.
     */
    for (iter = first ; iter != last ; ++iter) {
        Tcl_HashEntry *entry ;
        /*
         * Clean up the index as we are deleting a tuple from the relation.
         */
        entry = Tcl_FindHashEntry(&relation->index, (char const *)*iter) ;
        assert(entry != NULL) ;
        Tcl_DeleteHashEntry(entry) ;
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

	status = Ral_RelationUpdateTupleIndex(relation, *iter, iter) ;
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
    Ral_TupleHeadingIter found ;
    Ral_Attribute newAttr ;

    /*
     * Find the old name.
     */
    found = Ral_TupleHeadingFind(tupleHeading, oldName) ;
    if (found == Ral_TupleHeadingEnd(tupleHeading)) {
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
    if (found == Ral_TupleHeadingEnd(tupleHeading)) {
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
    Ral_RelationIter riter ;
    Ral_TupleHeading tupleHeading = relation->heading ;
    Ral_TupleHeadingIter hiter ;

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
    for (riter = Ral_RelationBegin(relation) ;
            riter != Ral_RelationEnd(relation) ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;
	typeFlag = typeFlags->flags.compoundFlags.flags ;

	length += sizeof(openList) ;
	for (hiter = Ral_TupleHeadingBegin(tupleHeading) ;
                hiter != Ral_TupleHeadingEnd(tupleHeading) ; ++hiter) {
	    Ral_Attribute a = *hiter ;
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
	if (!Ral_TupleHeadingEmpty(tupleHeading)) {
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
    Ral_RelationIter riter ;
    Ral_TupleHeading tupleHeading = relation->heading ;
    Ral_TupleHeadingIter hiter ;

    assert(typeFlags->attrType == Relation_Type) ;
    assert(typeFlags->flags.compoundFlags.count
	    == Ral_RelationDegree(relation)) ;
    assert(valueFlags->attrType == Relation_Type) ;
    assert(valueFlags->flags.compoundFlags.count ==
	Ral_RelationDegree(relation) * Ral_RelationCardinality(relation)) ;

    valueFlag = valueFlags->flags.compoundFlags.flags ;
    *p++ = openList ;
    for (riter = Ral_RelationBegin(relation) ;
            riter != Ral_RelationEnd(relation) ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;
	typeFlag = typeFlags->flags.compoundFlags.flags ;

	*p++ = openList ;
	for (hiter = Ral_TupleHeadingBegin(tupleHeading) ;
                hiter != Ral_TupleHeadingEnd(tupleHeading) ; ++hiter) {
	    Ral_Attribute a = *hiter ;
	    Tcl_Obj *v = *values++ ;

	    p += Ral_AttributeConvertName(a, p, typeFlag) ;
	    *p++ = ' ' ;
	    assert(v != NULL) ;
	    p += Ral_AttributeConvertValue(a, v, p, typeFlag++, valueFlag++) ;
	    *p++ = ' ' ;
	}
        /*
         * Remove any trailing space.
         */
        if (*(p - 1) == ' ') {
	    --p ;
	}
	*p++ = closeList ;
	*p++ = ' ' ;
    }
    /*
     * Remove any trailing space.
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
	Tcl_SetHashValue(entry, where - Ral_RelationBegin(relation)) ;
    }
    return newPtr ;
}

/*
 * In this case we assume the tuple already exists and we want to modify
 * the hash value.
 */
static int
Ral_RelationUpdateTupleIndex(
    Ral_Relation relation,
    Ral_Tuple tuple,
    Ral_RelationIter where)
{
    Tcl_HashEntry *entry ;

    entry = Tcl_FindHashEntry(&relation->index, (char const *)tuple) ;
    /*
     * Check that an entry was actually created and if so, set its value.
     */
    if (entry) {
	Tcl_SetHashValue(entry, where - Ral_RelationBegin(relation)) ;
    }
    return entry != NULL ;
}

/*
 * Create a hash table that hashes a subset of attributes of the tuples in a
 * relation. The subset to hash is given by a vector that contains the indices
 * of the attributes in the tuple heading.  Storage for the hash table is
 * managed by the caller.  In this case we use the custom hash table that holds
 * a vector of tuple indices.
 */
static void
Ral_RelationIndexByAttrs(
    Tcl_HashTable *index,
    Ral_Relation relation,
    Ral_IntVector attrs)
{
    Ral_RelationIter iter ;
    struct Ral_TupleAttrHashKey key ;

    Tcl_InitCustomHashTable(index, TCL_CUSTOM_PTR_KEYS,
            &tupleAttrHashMultiType) ;

    for (iter = Ral_RelationBegin(relation) ;
            iter != Ral_RelationEnd(relation) ; ++iter) {
        Tcl_HashEntry *entry ;
        int newPtr ;
        Ral_IntVector tupindices ;

        key.tuple = *iter ;
        key.attrs = attrs ;

        entry = Tcl_CreateHashEntry(index, (char const *)&key, &newPtr) ;
        /*
         * N.B. that we don't care about whether or not a new entry
         * was created. The vector of indices held in the entry can record
         * multiple tuples that have the same attribute values.
         */
        tupindices = Tcl_GetHashValue(entry) ;
        Ral_IntVectorPushBack(tupindices, iter - Ral_RelationBegin(relation)) ;
    }
}

/*
 * Functions for custom hash table to deal with hashing tuples
 * for quick access in relations.
 */
static unsigned int
tupleHashGenKey(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    return Ral_TupleHash((Ral_Tuple)keyPtr) ;
}

static int
tupleHashCompareKeys(
    void *keyPtr,
    Tcl_HashEntry *hPtr)
{
    Ral_Tuple tuple1 = (Ral_Tuple)keyPtr ;
    Ral_Tuple tuple2 = (Ral_Tuple)hPtr->key.oneWordValue ;

    assert(Ral_TupleHeadingEqual(tuple1->heading, tuple2->heading)) ;
    return Ral_TupleEqualValues(tuple1, tuple2) ;
}

static Tcl_HashEntry *
tupleHashEntryAlloc(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    Tcl_HashEntry *hPtr ;

    hPtr = (Tcl_HashEntry *)ckalloc(sizeof(*hPtr)) ;
    hPtr->key.oneWordValue = (char *)keyPtr ;
    Ral_TupleReference((Ral_Tuple)keyPtr) ;
    hPtr->clientData = NULL ;

    return hPtr ;
}

static void
tupleHashEntryFree(
    Tcl_HashEntry *hPtr)
{
    Ral_Tuple tuple = (Ral_Tuple)hPtr->key.oneWordValue ;
    Ral_TupleUnreference(tuple) ;
    ckfree((char *)hPtr) ;
}

/*
 * Functions for custom hash table to deal with hashing tuples attribute
 * values. This type of hash table used as indices into a relation when tuples
 * need to be found based on values supplied from outside of the relation.
 */

static unsigned int
tupleAttrHashGenKey(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    Ral_TupleAttrHashKey tupleAttrKey = (Ral_TupleAttrHashKey)keyPtr ;
    return Ral_TupleHashAttr(tupleAttrKey->tuple, tupleAttrKey->attrs) ;
}

static int
tupleAttrHashCompareKeys(
    void *keyPtr,
    Tcl_HashEntry *hPtr)
{
    Ral_TupleAttrHashKey tupleAttrKey1 = (Ral_TupleAttrHashKey)keyPtr ;
    Ral_TupleAttrHashKey tupleAttrKey2 =
            (Ral_TupleAttrHashKey)hPtr->key.oneWordValue ;

    return Ral_TupleAttrEqual(tupleAttrKey1->tuple, tupleAttrKey1->attrs,
            tupleAttrKey2->tuple, tupleAttrKey2->attrs) ;
}

static Tcl_HashEntry *
tupleAttrHashEntryAlloc(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    Tcl_HashEntry *hPtr ;
    Ral_TupleAttrHashKey newKey ;
    Ral_TupleAttrHashKey oldKey ;
    /*
     * Allocate hash entry and key in one block.
     */
    hPtr = (Tcl_HashEntry *)ckalloc(sizeof(*hPtr) + sizeof(*newKey)) ;
    newKey = (Ral_TupleAttrHashKey)(hPtr + 1) ;
    hPtr->key.oneWordValue = (char *)newKey ;
    hPtr->clientData = NULL ;
    /*
     * Initialize the new key from the old key.  Tuples are reference counted
     * so we just copy the pointer and increment the reference count.
     *
     * N.B. we just copy the pointer to the attributes vector. This implies
     * that the lifetime of the vector must be longer that that of the hash
     * table.
     */
    oldKey = (Ral_TupleAttrHashKey)keyPtr ;
    newKey->tuple = oldKey->tuple ;
    Ral_TupleReference(newKey->tuple) ;
    newKey->attrs = oldKey->attrs ;

    return hPtr ;
}

static void
tupleAttrHashEntryFree(
    Tcl_HashEntry *hPtr)
{
    Ral_TupleAttrHashKey key ;
    /*
     * Clean up the key references. We must decrement the reference count
     * on the tuple and delete the attribute index vector. The entry
     * itself was allocated in one block.
     */
    key = (Ral_TupleAttrHashKey)hPtr->key.oneWordValue ;
    Ral_TupleUnreference(key->tuple) ;
    ckfree((char *)hPtr) ;
}

/*
 * Compare to tupleAttrHashEntryAlloc(). The only difference is that
 * the clientData is set to an empty vector.
 */
static Tcl_HashEntry *
tupleAttrHashMultiEntryAlloc(
    Tcl_HashTable *tablePtr,
    void *keyPtr)
{
    Tcl_HashEntry *hPtr ;
    Ral_TupleAttrHashKey newKey ;
    Ral_TupleAttrHashKey oldKey ;
    /*
     * Allocate hash entry and key in one block.
     */
    hPtr = (Tcl_HashEntry *)ckalloc(sizeof(*hPtr) + sizeof(*newKey)) ;
    newKey = (Ral_TupleAttrHashKey)(hPtr + 1) ;
    hPtr->key.oneWordValue = (char *)newKey ;
    /*
     * We have to guess here what we think is the most probable case of tuples
     * that hash to the same place.  We guess 3. This is just to try to prevent
     * expanding the vector at run time and yet still not waste too much memory.
     */
    hPtr->clientData = Ral_IntVectorNewEmpty(3) ;
    /*
     * Initialize the new key from the old key.  Tuples are reference counted
     * so we just copy the pointer and increment the reference count.
     *
     * N.B. we just copy the pointer to the attributes vector. This implies
     * that the lifetime of the vector must be longer that that of the hash
     * table.
     */
    oldKey = (Ral_TupleAttrHashKey)keyPtr ;
    newKey->tuple = oldKey->tuple ;
    Ral_TupleReference(newKey->tuple) ;
    newKey->attrs = oldKey->attrs ;

    return hPtr ;
}

static void
tupleAttrHashMultiEntryFree(
    Tcl_HashEntry *hPtr)
{
    Ral_TupleAttrHashKey key ;
    /*
     * Clean up the key references. We must decrement the reference count on
     * the tuple and delete the attribute index vector. Also since we have a
     * vector as the clientData it must also be freed.  The entry itself was
     * allocated in one block.
     */
    key = (Ral_TupleAttrHashKey)hPtr->key.oneWordValue ;
    Ral_TupleUnreference(key->tuple) ;
    Ral_IntVectorDelete(hPtr->clientData) ;
    ckfree((char *)hPtr) ;
}
