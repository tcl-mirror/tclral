/*
This software is copyrighted 2006 - 2011 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_joinmap.c,v $
$Revision: 1.8 $
$Date: 2011/06/05 18:01:10 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_joinmap.h"
#include <stdlib.h>
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

Ral_JoinMap
Ral_JoinMapNew(
    int attrCount,
    int tupleCount)
{
    Ral_JoinMap map ;

    map = (Ral_JoinMap)ckalloc(sizeof(*map)) ;
    map->attrStart = map->attrFinish = (Ral_JoinMapIter)ckalloc(
	attrCount * sizeof(*map->attrStart)) ;
    map->attrEndStorage = map->attrStart + attrCount ;
    map->tupleStart = map->tupleFinish = (Ral_JoinMapIter)ckalloc(
	tupleCount * sizeof(*map->tupleStart)) ;
    map->tupleEndStorage = map->tupleStart + tupleCount ;

    return map ;
}

void
Ral_JoinMapDelete(
    Ral_JoinMap map)
{
    if (map->attrStart) {
	ckfree((char *)map->attrStart) ;
    }
    if (map->tupleStart) {
	ckfree((char *)map->tupleStart) ;
    }
    ckfree((char *)map) ;
}

void
Ral_JoinMapAttrReserve(
    Ral_JoinMap map,
    int size)
{
    if (Ral_JoinMapAttrCapacity(map) < size) {
	int oldSize = Ral_JoinMapAttrSize(map) ;
	map->attrStart = (Ral_JoinMapIter)ckrealloc((char *)map->attrStart, 
	    size * sizeof(Ral_JoinMapValueType)) ;
	map->attrFinish = map->attrStart + oldSize ;
	map->attrEndStorage = map->attrStart + size ;
    }
}

void
Ral_JoinMapTupleReserve(
    Ral_JoinMap map,
    int size)
{
    if (Ral_JoinMapTupleCapacity(map) < size) {
	int oldSize = Ral_JoinMapTupleSize(map) ;
	map->tupleStart = (Ral_JoinMapIter)ckrealloc((char *)map->tupleStart, 
	    size * sizeof(Ral_JoinMapValueType)) ;
	map->tupleFinish = map->tupleStart + oldSize ;
	map->tupleEndStorage = map->tupleStart + size ;
    }
}

/*
 * Add attribute indices to the join map. Checks that there are not duplicates.
 * Return 0 upon success, 1 if attribute #1 is a duplicate, 2 if attribute #2
 * is a duplicate.
 */
int
Ral_JoinMapAddAttrMapping(
    Ral_JoinMap map,
    Ral_JoinMapComponentType attr1Index,
    Ral_JoinMapComponentType attr2Index)
{
    Ral_JoinMapIter iter ;

    if (map->attrFinish >= map->attrEndStorage) {
	int oldCapacity = Ral_JoinMapAttrCapacity(map) ;
	/*
	 * Increase the capacity by half again. +1 to make sure
	 * we allocate at least one slot if this is the first time
	 * we are pushing onto an empty vector.
	 */
	Ral_JoinMapAttrReserve(map, oldCapacity + oldCapacity / 2 + 1) ;
    }
    /*
     * We need to make sure that the indices do not already occur in the join
     * map, i.e. the attribute indices on each side of the join should be sets.
     * So we iterate through the map comparing indices. If we get to the end
     * and don't find a match, then we can just insert out new indices.
     */
    for (iter = Ral_JoinMapAttrBegin(map) ; iter != Ral_JoinMapAttrEnd(map) ;
            ++iter) {
        if (iter->m[0] == attr1Index) {
            return 1 ;
        } else if (iter->m[1] == attr2Index) {
            return 2 ;
        }
    }
    assert(iter == Ral_JoinMapAttrEnd(map)) ;
    iter->m[0] = attr1Index ;
    iter->m[1] = attr2Index ;
    ++map->attrFinish ;
    return 0 ;
}

void
Ral_JoinMapAddTupleMapping(
    Ral_JoinMap map,
    int tuple1Index,
    int tuple2Index)
{
    if (map->tupleFinish >= map->tupleEndStorage) {
	int oldCapacity = Ral_JoinMapTupleCapacity(map) ;
	/*
	 * Increase the capacity by half again. +1 to make sure
	 * we allocate at least one slot if this is the first time
	 * we are pushing onto an empty vector.
	 */
	Ral_JoinMapTupleReserve(map, oldCapacity + oldCapacity / 2 + 1) ;
    }
    map->tupleFinish->m[0] = tuple1Index ;
    map->tupleFinish->m[1] = tuple2Index ;
    ++map->tupleFinish ;
}

/*
 * Return an integer vector containing the set of attribute indices
 * in "map" at the given "offset".
 * Caller must delete the returned vector.
 */
Ral_IntVector
Ral_JoinMapGetAttr(
    Ral_JoinMap map,
    int offset)
{
    Ral_IntVector v = Ral_IntVectorNewEmpty(Ral_JoinMapAttrSize(map)) ;
    Ral_JoinMapIter end = Ral_JoinMapAttrEnd(map) ;
    Ral_JoinMapIter iter ;

    assert(offset < MAP_ELEMENT_COUNT) ;
    for (iter = Ral_JoinMapAttrBegin(map) ; iter != end ; ++iter) {
	Ral_IntVectorPushBack(v, iter->m[offset]) ;
    }

    return v ;
}

/*
 * Return a vector "size" large. Each slot contains either 1 if the
 * corresponding attribute index does not appear in "map" at offset "offset"
 * and 0 if it does.  It follows that the maximum value contained in "map" must
 * be less than "size".
 * Caller must delete the returned vector.
 */
Ral_IntVector
Ral_JoinMapAttrMap(
    Ral_JoinMap map,
    int offset,
    int size)
{
    Ral_IntVector v = Ral_IntVectorNew(size, 1) ;
    Ral_JoinMapIter end = Ral_JoinMapAttrEnd(map) ;
    Ral_JoinMapIter iter ;

    assert(offset < MAP_ELEMENT_COUNT) ;
    for (iter = Ral_JoinMapAttrBegin(map) ; iter != end ; ++iter) {
	Ral_IntVectorStore(v, iter->m[offset], 0) ;
    }

    return v ;
}

/*
 * Search the map for a matching attribute index. Return the corresponding
 * attribute index. Return -1 if not found.
 */
int
Ral_JoinMapFindAttr(
    Ral_JoinMap map,
    int offset,
    int attrIndex)
{
    Ral_JoinMapIter end = Ral_JoinMapAttrEnd(map) ;
    Ral_JoinMapIter iter ;

    assert(offset < MAP_ELEMENT_COUNT) ;
    for (iter = Ral_JoinMapAttrBegin(map) ; iter != end ; ++iter) {
	if (iter->m[offset] == attrIndex) {
	    return iter->m[(offset + 1) % MAP_ELEMENT_COUNT] ;
	}
    }

    return -1 ;
}

static int
attr_0_cmp(
    void const *m1,
    void const *m2)
{
    const Ral_JoinMapValueType *a1 = m1 ;
    const Ral_JoinMapValueType *a2 = m2 ;

    return a1->m[0] - a2->m[0] ;
}

static int
attr_1_cmp(
    void const *m1,
    void const *m2)
{
    Ral_JoinMapValueType const *a1 = m1 ;
    Ral_JoinMapValueType const *a2 = m2 ;

    return a1->m[1] - a2->m[1] ;
}

void
Ral_JoinMapSortAttr(
    Ral_JoinMap map,
    int offset)
{
    static int (* const cmpFuncs[])(void const *, void const *) =
    {
	attr_0_cmp,
	attr_1_cmp,
    } ;
    assert(offset < MAP_ELEMENT_COUNT) ;
    qsort(map->attrStart, map->attrFinish - map->attrStart,
	sizeof(Ral_JoinMapValueType), cmpFuncs[offset]) ;
}

/*
 * Return a vector "size" large. Each slot contains either 1 if the
 * corresponding tuple index does not appear in "map" at offset "offset"
 * and 0 if it does.  It follows that the maximum value contained in "map" must
 * be less than "size".
 * Caller must delete the returned vector.
 */
Ral_IntVector
Ral_JoinMapTupleMap(
    Ral_JoinMap map,
    int offset,
    int size)
{
    Ral_IntVector v = Ral_IntVectorNew(size, 1) ;
    Ral_JoinMapIter end = Ral_JoinMapTupleEnd(map) ;
    Ral_JoinMapIter iter ;

    assert(offset < MAP_ELEMENT_COUNT) ;

    for (iter = Ral_JoinMapTupleBegin(map) ; iter != end ; ++iter) {
	Ral_IntVectorStore(v, iter->m[offset], 0) ;
    }

    return v ;
}

/*
 * Fill in a vector with the number of times that tuple index appears in the
 * "map" at "offset". The vector is assumed to be the same size as the join
 * map.  The maximum value contained in "map" will be less than that size.
 */
int
Ral_JoinMapTupleCounts(
    Ral_JoinMap map,
    int offset,
    Ral_IntVector v)
{
    int cnt = 0 ;
    Ral_IntVectorIter vBegin = Ral_IntVectorBegin(v) ;
    Ral_JoinMapIter end = Ral_JoinMapTupleEnd(map) ;
    Ral_JoinMapIter iter ;

    assert(offset < MAP_ELEMENT_COUNT) ;

    for (iter = Ral_JoinMapTupleBegin(map) ; iter != end ; ++iter) {
	assert(iter->m[offset] < Ral_IntVectorSize(v)) ;
	*(vBegin + iter->m[offset]) += 1 ;
	++cnt ;
    }

    return cnt ;
}

/*
 * Return a set of tuple indices that match the given tuple "index".
 * Caller must delete the returned vector.
 */
Ral_IntVector
Ral_JoinMapMatchingTupleSet(
    Ral_JoinMap map,
    int offset,
    int index)
{
    Ral_IntVector matchSet = Ral_IntVectorNewEmpty(5) ; /* just a guess */
    Ral_JoinMapIter end = Ral_JoinMapTupleEnd(map) ;
    Ral_JoinMapIter iter ;
    int otherOffset = (offset + 1) % MAP_ELEMENT_COUNT ;

    for (iter = Ral_JoinMapTupleBegin(map) ; iter != end ; ++iter) {
	if (iter->m[offset] == index) {
	    Ral_IntVectorSetAdd(matchSet, iter->m[otherOffset]) ;
	}
    }

    return matchSet ;
}
