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
$Revision: 1.3 $
$Date: 2006/02/26 04:57:53 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relation.c,v $
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
static const char openList = '{' ;
static const char closeList = '}' ;
static const char rcsid[] = "@(#) $RCSfile: ral_relation.c,v $ $Revision: 1.3 $" ;

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
