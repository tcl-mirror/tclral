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
$Revision: 1.2 $
$Date: 2006/02/20 20:15:07 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relation.c,v $
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
static Tcl_DString *Ral_RelationGetIdKey(Ral_Tuple, Ral_IntVector) ;
static Tcl_HashEntry *Ral_RelationFindIndexEntry(Ral_Relation, int, Ral_Tuple) ;
static void Ral_RelationRemoveIndex(Ral_Relation, int, Ral_Tuple) ;
static int Ral_RelationIndexIdentifier(Ral_Relation, int, Ral_Tuple,
    Ral_RelationIter) ;
static int Ral_RelationIndexTuple(Ral_Relation, Ral_Tuple, Ral_RelationIter) ;
static void Ral_RelationRemoveTupleIndex(Ral_Relation, Ral_Tuple) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char openList[] = "{" ;
static const char closeList[] = "}" ;
static const char rcsid[] = "@(#) $RCSfile: ral_relation.c,v $ $Revision: 1.2 $" ;

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
    Ral_RelationHeading srcHeading = srcRelation->heading ;
    Ral_RelationHeading dupHeading ;
    Ral_Relation dupRelation ;
    Ral_TupleHeading dupTupleHeading ;
    Ral_RelationIter srcIter ;
    Ral_RelationIter srcEnd = srcRelation->finish ;
    Ral_RelationIter dupIter ;

    dupHeading = Ral_RelationHeadingDup(srcHeading) ;
    if (!dupHeading) {
	return NULL ;
    }
    dupRelation = Ral_RelationNew(dupHeading) ;
    Ral_RelationReserve(dupRelation, Ral_RelationCardinality(srcRelation)) ;

    dupTupleHeading = dupHeading->tupleHeading ;
    dupIter = dupRelation->start ;
    for (srcIter = srcRelation->start ; srcIter != srcEnd ; ++srcIter) {
	Ral_Tuple srcTuple = *srcIter ;
	Ral_Tuple dupTuple ;
	int copied ;
	int appended ;

	dupTuple = Ral_TupleNew(dupTupleHeading) ;
	copied = Ral_TupleCopy(srcTuple,
	    Ral_TupleHeadingBegin(srcTuple->heading),
	    Ral_TupleHeadingEnd(srcTuple->heading), dupTuple) ;
	assert(copied != 0) ;

	appended = Ral_RelationPushBack(dupRelation, dupTuple) ;
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
Ral_RelationDegree(
    Ral_Relation r)
{
    return Ral_TupleHeadingSize(r->heading->tupleHeading) ;
}

int
Ral_RelationPushBack(
    Ral_Relation relation,
    Ral_Tuple tuple)
{
    assert(relation->heading->tupleHeading == tuple->heading) ;

    if (relation->finish >= relation->endStorage) {
	int oldCapacity = Ral_RelationCapacity(relation) ;
	/*
	 * Increase the capacity by half again. +1 to make sure
	 * we allocate at least one slot if this is the first time
	 * we are pushing onto an empty vector.
	 */
	Ral_RelationReserve(relation, oldCapacity + oldCapacity / 2 + 1) ;
    }
    if (Ral_RelationIndexTuple(relation, tuple, relation->finish)) {
	Ral_TupleReference(*relation->finish++ = tuple) ;
	return 1 ;
    }

    return 0 ;
}

int
Ral_RelationScan(
    Ral_Relation relation,
    Ral_RelationScanFlags *flags)
{
    Ral_RelationHeading heading = relation->heading ;
    Ral_RelationScanFlags scanFlags ;

    *flags = scanFlags = Ral_RelationScanFlagsAlloc(heading,
	Ral_RelationCardinality(relation)) ;

    return
	Ral_RelationHeadingScan(heading, scanFlags) +
	Ral_RelationScanValue(relation, scanFlags) +
	1 + /* +1 for the separating space */
	1 ; /* +1 for NUL terminator */
}

int
Ral_RelationConvert(
    Ral_Relation relation,
    char *dst,
    Ral_RelationScanFlags scanFlags)
{
    char *p = dst ;

    p += Ral_RelationHeadingConvert(relation->heading, p, scanFlags) ;
    *p++ = ' ' ;
    p += Ral_RelationConvertValue(relation, p, scanFlags) ;
    Ral_RelationScanFlagsFree(scanFlags) ;

    return p - dst ;
}

int
Ral_RelationScanValue(
    Ral_Relation relation,
    Ral_RelationScanFlags flags)
{
    int length ;
    Ral_AttributeScanFlags valueFlag = flags->valueFlags ;
    Ral_RelationIter rend = Ral_RelationEnd(relation) ;
    Ral_RelationIter riter ;
    Ral_TupleHeading tupleHeading = relation->heading->tupleHeading ;
    Ral_TupleHeadingIter hEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter hIter ;

    /*
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    length = sizeof(openList) - 1 ;

    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;

	length = sizeof(openList) - 1 ;
	for (hIter = Ral_TupleHeadingBegin(tupleHeading) ;
	    hIter != hEnd ; ++hIter) {
	    Ral_Attribute a = *hIter ;
	    Tcl_Obj *v = *values++ ;

	    assert(v != NULL) ;
	    /* +1 for space */
	    length += Ral_AttributeScanName(a, valueFlag) + 1 ;
	    length += Ral_AttributeScanValue(a, v, valueFlag) + 1 ;
	    ++valueFlag ;
	}
	length += sizeof(closeList) - 1 ;
	++length ; /* space between list elements */
    }
    length += sizeof(closeList) - 1 ;

    return length ;
}

int
Ral_RelationConvertValue(
    Ral_Relation relation,
    char *dst,
    Ral_RelationScanFlags flags)
{
    char *p = dst ;
    Ral_AttributeScanFlags valueFlag = flags->valueFlags ;
    Ral_RelationIter rend = Ral_RelationEnd(relation) ;
    Ral_RelationIter riter ;
    Ral_TupleHeading tupleHeading = relation->heading->tupleHeading ;
    int nonEmptyHeading = !Ral_TupleHeadingEmpty(tupleHeading) ;
    Ral_TupleHeadingIter hEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    Ral_TupleHeadingIter hIter ;

    /*
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    strcpy(p, openList) ;
    p += sizeof(openList) - 1 ;
    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_Tuple tuple = *riter ;
	Tcl_Obj **values = tuple->values ;

	strcpy(p, openList) ;
	p += sizeof(openList) - 1 ;

	for (hIter = Ral_TupleHeadingBegin(tupleHeading) ;
	    hIter != hEnd ; ++hIter) {
	    Ral_Attribute a = *hIter ;
	    Tcl_Obj *v = *values++ ;

	    assert(v != NULL) ;
	    p += Ral_AttributeConvertName(a, p, valueFlag) ;
	    *p++ = ' ' ;
	    p += Ral_AttributeConvertValue(a, v, p, valueFlag) ;
	    *p++ = ' ' ;

	    ++valueFlag ;
	}
	if (nonEmptyHeading) {
	    --p ;
	}
	strcpy(p, closeList) ;
	p += sizeof(closeList) - 1 ;
	*p++ = ' ' ;
    }
    /*
     * Remove the trailing space. Check that the relation actually had
     * some values!
     */
    if (Ral_RelationCardinality(relation)) {
	--p ;
    }
    strcpy(p, closeList) ;
    p += sizeof(closeList) - 1 ;

    return p - dst ;
}

void
Ral_RelationPrint(
    Ral_Relation relation,
    const char *format,
    FILE *f)
{
    Ral_RelationScanFlags flags ;
    char *str ;

    str = ckalloc(Ral_RelationScan(relation, &flags)) ;
    Ral_RelationConvert(relation, str, flags) ;
    fprintf(f, format, str) ;
    ckfree(str) ;
}


const char *
Ral_RelationVersion(void)
{
    return rcsid ;
}

/*
 * PRIVATE FUNCTIONS
 */

static Tcl_DString *
Ral_RelationGetIdKey(
    Ral_Tuple tuple,
    Ral_IntVector idMap)
{
    static Tcl_DString idKey ;

    Ral_IntVectorIter iter ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(idMap) ;

    Tcl_DStringInit(&idKey) ;
    for (iter = Ral_IntVectorBegin(idMap) ; iter != end ; ++iter) {
	Tcl_DStringAppend(&idKey, Tcl_GetString(tuple->values[*iter]), -1) ;
    }

    return &idKey ;
}

static Tcl_HashEntry *
Ral_RelationFindIndexEntry(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple)
{
    Tcl_DString *idKey ;
    Tcl_HashEntry *entry ;

    assert(idIndex < relation->heading->idCount) ;

    idKey = Ral_RelationGetIdKey(tuple,
	relation->heading->identifiers[idIndex]) ;
    entry = Tcl_FindHashEntry(relation->indexVector + idIndex,
	Tcl_DStringValue(idKey)) ;
    Tcl_DStringFree(idKey) ;

    return entry ;
}

static void
Ral_RelationRemoveIndex(
    Ral_Relation relation,
    int idIndex,
    Ral_Tuple tuple)
{
    Tcl_HashEntry *entry ;

    entry = Ral_RelationFindIndexEntry(relation, idIndex, tuple) ;
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

    idKey = Ral_RelationGetIdKey(tuple, idMap) ;
    entry = Tcl_CreateHashEntry(index, Tcl_DStringValue(idKey), &newPtr) ;
    Tcl_DStringFree(idKey) ;
    /*
     * Check that there are no duplicate tuples.
     */
    if (newPtr == 0) {
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
