/*
This software is copyrighted 2005 - 2011 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_tuple.c,v $
$Revision: 1.16 $
$Date: 2011/06/05 18:01:10 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_utils.h"
#include "ral_tuple.h"
#include "ral_vector.h"
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
static const char openList = '{' ;
static const char closeList = '}' ;

/*
FUNCTION DEFINITIONS
*/

Ral_Tuple
Ral_TupleNew(
    Ral_TupleHeading heading)
{
    int nBytes ;
    Ral_Tuple tuple ;

    nBytes = sizeof(*tuple) +
	Ral_TupleHeadingCapacity(heading) * sizeof(*tuple->values) ;
    tuple = (Ral_Tuple)ckalloc(nBytes) ;
    memset(tuple, 0, nBytes) ;

    tuple->refCount = 0 ;
    Ral_TupleHeadingReference(tuple->heading = heading) ;
    tuple->values = (Tcl_Obj **)(tuple + 1) ;

    return tuple ;
}

Ral_Tuple
Ral_TupleSubset(
    Ral_Tuple tuple,
    Ral_TupleHeading heading,
    Ral_IntVector attrSet)
{
    Ral_Tuple subTuple = Ral_TupleNew(heading) ;
    Ral_TupleIter subIter = Ral_TupleBegin(subTuple) ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(attrSet) ;
    Ral_IntVectorIter iter ;

    for (iter = Ral_IntVectorBegin(attrSet) ; iter != end ; ++iter) {
	Ral_IntVectorValueType attrIndex = *iter ;
	assert(attrIndex < Ral_TupleDegree(tuple)) ;
	Tcl_IncrRefCount(*subIter++ = tuple->values[attrIndex]) ;
    }

    return subTuple ;
}

Ral_Tuple
Ral_TupleExtend(
    Ral_Tuple tuple,
    Ral_TupleHeading heading)
{
    Ral_Tuple extendTuple = Ral_TupleNew(heading) ;

    Ral_TupleCopyValues(Ral_TupleBegin(tuple), Ral_TupleEnd(tuple),
	Ral_TupleBegin(extendTuple)) ;

    return extendTuple ;
}

void
Ral_TupleDelete(
    Ral_Tuple tuple)
{
    if (tuple) {
	Ral_TupleIter iter ;
	Ral_TupleIter end = Ral_TupleEnd(tuple) ;

	assert(tuple->refCount <= 0) ;
	for (iter = Ral_TupleBegin(tuple) ; iter != end ; ++iter) {
	    if (*iter) {
		Tcl_DecrRefCount(*iter) ;
	    }
	}
	Ral_TupleHeadingUnreference(tuple->heading) ;

	ckfree((char *)tuple) ;
    }
}

void
Ral_TupleUnreference(
    Ral_Tuple tuple)
{
    if (tuple && --tuple->refCount <= 0) {
	Ral_TupleDelete(tuple) ;
    }
}

int
Ral_TupleEqual(
    Ral_Tuple tuple1,
    Ral_Tuple tuple2)
{
    /*
     * Tuples at the same address are equal.
     */
    if (tuple1 == tuple2) {
	return 1 ;
    }
    /*
     * For tuples to be equal they must have equal headings.
     */
    if (!Ral_TupleHeadingEqual(tuple1->heading, tuple2->heading)) {
	return 0 ;
    }
    /*
     * Match the values.
     */
    return Ral_TupleEqualValues(tuple1, tuple2) ;
}

/*
 * Only compare the values in two tuples.
 * Assumes that the headers have already been verified to be equal.
 * If not, use Ral_TupleEqual().
 */
int
Ral_TupleEqualValues(
    Ral_Tuple tuple1,
    Ral_Tuple tuple2)
{
    Ral_TupleHeading h1 = tuple1->heading ;
    Ral_TupleHeading h2 = tuple2->heading ;
    Ral_TupleIter iter1 = tuple1->values ;
    Ral_TupleIter iter2 = tuple2->values ;
    Ral_TupleHeadingIter e1 = Ral_TupleHeadingEnd(h1) ;
    Ral_TupleHeadingIter b1 ;
    /*
     * Match the values. We iterate on the first tuple, using the
     * attribute name to find the proper index into the second tuple.
     * Thus the comparison is independent of storage order as desired.
     */
    for (b1 = Ral_TupleHeadingBegin(h1) ; b1 != e1 ; ++b1) {
	Ral_Attribute attr = *b1 ;
	int i2 = Ral_TupleHeadingIndexOf(h2, attr->name) ;
	/*
	 * Since the two headings are equal, we must be able to find
	 * each attribute.
	 */
	assert(i2 >= 0) ;
	/*
	 * And all the object values must be equal.
	 */
	if (!Ral_AttributeValueEqual(attr, *iter1++, *(iter2 + i2))) {
	    return 0 ;
	}
    }
    return 1 ;
}

/*
 * Compute a hash value for a tuple.
 */
unsigned int
Ral_TupleHash(
    Ral_Tuple tuple)
{
    Ral_TupleHeading heading = tuple->heading ;
    Ral_TupleIter viter = Ral_TupleBegin(tuple) ;
    Ral_TupleHeadingIter hiter = Ral_TupleHeadingBegin(heading) ;
    unsigned int hash = 0 ;

    while (hiter != Ral_TupleHeadingEnd(heading)) {
        hash += Ral_AttributeHashValue(*hiter++, *viter++) ;
    }

    return hash ;
}

/*
 * Compute the hash value for a tuple using only the specified
 * attributes.
 */
unsigned int
Ral_TupleHashAttr(
    Ral_Tuple keyTuple,
    Ral_IntVector keyAttrs)
{
    Ral_IntVectorIter indexIter ;
    unsigned int hash = 0 ;

    for (indexIter = Ral_IntVectorBegin(keyAttrs) ;
            indexIter != Ral_IntVectorEnd(keyAttrs) ; ++indexIter) {
        Ral_TupleHeadingIter hiter =
                Ral_TupleHeadingBegin(keyTuple->heading) + *indexIter ;
        Ral_TupleIter viter =
                Ral_TupleBegin(keyTuple) + *indexIter ;

        assert(*indexIter < Ral_TupleDegree(keyTuple)) ;
        hash += Ral_AttributeHashValue(*hiter, *viter) ;
    }

    return hash ;
}

/*
 * Compare the attributes of two tuples to determine if they are equal.
 * The attributes must have the same type, but not necessarily the same name.
 */
int
Ral_TupleAttrEqual(
    Ral_Tuple keyTuple1,
    Ral_IntVector keyAttrs1,
    Ral_Tuple keyTuple2,
    Ral_IntVector keyAttrs2)
{
    Ral_IntVectorIter indexIter1 ;
    Ral_IntVectorIter indexIter2 ;

    assert(Ral_IntVectorSize(keyAttrs1) == Ral_IntVectorSize(keyAttrs2)) ;

    for (   indexIter1 = Ral_IntVectorBegin(keyAttrs1),
            indexIter2 = Ral_IntVectorBegin(keyAttrs2) ;
            indexIter1 != Ral_IntVectorEnd(keyAttrs1) ;
            ++indexIter1, ++indexIter2) {
        Ral_TupleHeadingIter hiter1 =
                Ral_TupleHeadingBegin(keyTuple1->heading) + *indexIter1 ;
        Ral_TupleIter viter1 =
                Ral_TupleBegin(keyTuple1) + *indexIter1 ;
        Ral_TupleIter viter2 =
                Ral_TupleBegin(keyTuple2) + *indexIter2 ;

        assert(*indexIter1 < Ral_TupleDegree(keyTuple1)) ;
        assert(*indexIter2 < Ral_TupleDegree(keyTuple2)) ;
        assert(Ral_AttributeTypeEqual(*hiter1,
                *(Ral_TupleHeadingBegin(keyTuple2->heading) + *indexIter2))) ;

        if (!Ral_AttributeValueEqual(*hiter1, *viter1, *viter2)) {
            return 0 ;
        }
    }

    return 1 ;
}

int
Ral_TupleUpdateAttrValue(
    Ral_Tuple tuple,
    const char *attrName,
    Tcl_Obj *value,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter i_attr ;
    Ral_Attribute attribute ;
    int valueIndex ;
    Tcl_Obj *oldValue ;
    Tcl_Obj *cvtValue ;

    /*
     * Check that we are abiding by copy-on-write semantics.
     */
    if (tuple->refCount > 1) {
	Tcl_Panic("Ral_TupleUpdate: attempt to update a shared tuple") ;
    }
    /*
     * Find the attribute associated with the given name.
     */
    heading = tuple->heading ;
    i_attr = Ral_TupleHeadingFind(heading, attrName) ;
    if (i_attr == Ral_TupleHeadingEnd(heading)) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
	return 0 ;
    }
    attribute = *i_attr ;
    /*
     * Convert the value to the type of the attribute.
     */
    cvtValue = Ral_AttributeConvertValueToType(NULL, attribute, value,
            errInfo) ;
    if (cvtValue == NULL) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_VALUE, value) ;
	return 0 ;
    }
    /*
     * Compute where to update in the object array.
     */
    valueIndex = i_attr - Ral_TupleHeadingBegin(heading) ;
    /*
     * Dispose of the old value if there was one.
     */
    oldValue = tuple->values[valueIndex] ;
    if (oldValue) {
	Tcl_DecrRefCount(oldValue) ;
    }
    /*
     * Update the tuple to the new value.
     */
    Tcl_IncrRefCount(tuple->values[valueIndex] = cvtValue) ;
    return 1 ;
}

Tcl_Obj *
Ral_TupleGetAttrValue(
    Ral_Tuple tuple,
    const char *attrName)
{
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter i_attr ;
    Tcl_Obj *value = NULL ;

    /*
     * Find the attribute associated with the given name.
     */
    heading = tuple->heading ;
    i_attr = Ral_TupleHeadingFind(heading, attrName) ;
    if (i_attr != Ral_TupleHeadingEnd(heading)) {
	value = tuple->values[i_attr - Ral_TupleHeadingBegin(heading)] ;
    }

    return value ;
}

/*
 * Copy attributes and values from a source tuple to a destination tuple.
 */
int
Ral_TupleCopy(
    Ral_Tuple src,
    Ral_TupleHeadingIter start,
    Ral_TupleHeadingIter finish,
    Ral_Tuple dst)
{
    Ral_TupleHeadingIter srcBegin = Ral_TupleHeadingBegin(src->heading) ;
    Ral_TupleIter srcValuesBegin = src->values + (start - srcBegin) ;
    Ral_TupleIter srcValuesEnd = src->values + (finish - srcBegin) ;
    Ral_TupleIter dstPlace = dst->values + Ral_TupleDegree(dst) ;
    /*
     * Copy the attributes into the destination heading.
     */
    if (!Ral_TupleHeadingAppend(src->heading, start, finish, dst->heading)) {
	return 0 ;
    }
    /*
     * Copy the values into the destination.
     */
    Ral_TupleCopyValues(srcValuesBegin, srcValuesEnd, dstPlace) ;

    return 1 ;
}

/*
 * Assumes that the source tuple heading is the same order as the
 * destination tuple heading.
 */
int
Ral_TupleCopyValues(
    Ral_TupleIter start,
    Ral_TupleIter finish,
    Ral_TupleIter dst)
{
    int count = 0 ;
    /*
     * Copy the values making sure the increment the reference count.
     * Also be careful to decrement the reference count of any
     * value already stored in the tuple.
     */
    while (start != finish) {
	if (*dst) {
	    Tcl_DecrRefCount(*dst) ;
	}
	Tcl_IncrRefCount(*dst++ = *start++) ;
	++count ;
    }

    return count ;
}

Ral_Tuple
Ral_TupleDup(
    Ral_Tuple src)
{
    Ral_TupleHeading srcHeading = src->heading ;
    Ral_TupleHeading dstHeading = Ral_TupleHeadingNew(
	Ral_TupleHeadingSize(srcHeading)) ;
    Ral_Tuple dst = Ral_TupleNew(dstHeading) ;

    if (!Ral_TupleCopy(src, Ral_TupleHeadingBegin(srcHeading),
	Ral_TupleHeadingEnd(srcHeading), dst)) {
	Ral_TupleDelete(dst) ;
	dst = NULL ;
    }

    return dst ;
}

/*
 * duplicate a tuple, but reuse the heading.
 */
Ral_Tuple
Ral_TupleDupShallow(
    Ral_Tuple src)
{
    Ral_Tuple newTuple ;

    newTuple = Ral_TupleNew(src->heading) ;
    Ral_TupleCopyValues(Ral_TupleBegin(src), Ral_TupleEnd(src),
            Ral_TupleBegin(newTuple)) ;
    return newTuple ;
}

/*
 * Create a new tuple that contains the same values
 * of a given tuple but reordered to match the heading.
 * The "orderMap" is in the same attribute order as "heading" and
 * gives the indices into tuple for the corresponding attributes.
 */
Ral_Tuple
Ral_TupleDupOrdered(
    Ral_Tuple tuple,
    Ral_TupleHeading heading,
    Ral_IntVector orderMap)
{
    Ral_Tuple newTuple = Ral_TupleNew(heading) ;
    Ral_IntVectorIter end = Ral_IntVectorEnd(orderMap) ;
    Ral_IntVectorIter iter ;
    int newAttrIndex = 0 ;

    assert(Ral_IntVectorSize(orderMap) == Ral_TupleHeadingSize(heading)) ;

    for (iter = Ral_IntVectorBegin(orderMap) ; iter !=end ; ++iter) {
	int oldAttrIndex = *iter ;
	assert(oldAttrIndex < Ral_TupleDegree(tuple)) ;
	Tcl_IncrRefCount(newTuple->values[newAttrIndex++] =
	    tuple->values[oldAttrIndex]) ;
    }

    return newTuple ;
}

int
Ral_TupleScan(
    Ral_Tuple tuple,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    Ral_TupleHeading heading = tuple->heading ;
    int length ;
    /*
     * Scan the header.
     * +1 for "{", +1 for "}" and +1 for the separating space
     */
    length = Ral_TupleHeadingScan(heading, typeFlags) + 3 ;
    /*
     * Scan the values.
     */
    length += Ral_TupleScanValue(tuple, typeFlags, valueFlags) ;
    return length ;
}

int
Ral_TupleConvert(
    Ral_Tuple tuple,
    char *dst,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    char *p = dst ;

    /*
     * Convert the heading adding "{}" to make it a list.
     */
    *p++ = openList ;
    p += Ral_TupleHeadingConvert(tuple->heading, p, typeFlags) ;
    *p++ = closeList ;
    /*
     * Separate the heading from the body by space.
     */
    *p++ = ' ' ;
    /*
     * Convert the body.
     */
    p += Ral_TupleConvertValue(tuple, p, typeFlags, valueFlags) ;
    /*
     * Finished with the flags.
     */
    Ral_AttributeTypeScanFlagsFree(typeFlags) ;
    Ral_AttributeValueScanFlagsFree(valueFlags) ;

    return p - dst ;
}

int
Ral_TupleScanValue(
    Ral_Tuple tuple,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    int nBytes ;
    int length ;
    Ral_TupleHeading heading = tuple->heading ;
    Tcl_Obj **values = tuple->values ;
    Ral_AttributeTypeScanFlags *typeFlag ;
    Ral_AttributeValueScanFlags *valueFlag ;
    Ral_TupleHeadingIter hIter ;

    assert(typeFlags->attrType == Tuple_Type) ;
    assert(typeFlags->flags.compoundFlags.count == Ral_TupleDegree(tuple)) ;
    assert(valueFlags->attrType == Tuple_Type) ;
    assert(valueFlags->flags.compoundFlags.flags == NULL) ;

    /*
     * Allocate space for the value flags.
     */
    valueFlags->flags.compoundFlags.count =
	    typeFlags->flags.compoundFlags.count ;
    nBytes = valueFlags->flags.compoundFlags.count *
	sizeof(*valueFlags->flags.compoundFlags.flags) ;
    valueFlags->flags.compoundFlags.flags =
	(Ral_AttributeValueScanFlags *)ckalloc(nBytes) ;
    memset(valueFlags->flags.compoundFlags.flags, 0, nBytes) ;

    /*
     * Iterate through the heading and scan the corresponding values.
     */
    typeFlag = typeFlags->flags.compoundFlags.flags ;
    valueFlag = valueFlags->flags.compoundFlags.flags ;

    length = sizeof(openList) ;
    for (hIter = Ral_TupleHeadingBegin(heading) ;
            hIter != Ral_TupleHeadingEnd(heading) ; ++hIter) {
	Ral_Attribute a = *hIter ;
	Tcl_Obj *v = *values++ ;

	assert(v != NULL) ;
	length += typeFlag->nameLength + 1 ; /* +1 for space */
	length += Ral_AttributeScanValue(a, v, typeFlag++, valueFlag++) + 1 ;
    }
    length += sizeof(closeList) ;

    return length ;
}

int
Ral_TupleConvertValue(
    Ral_Tuple tuple,
    char *dst,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    char *p = dst ;
    Ral_TupleHeading heading = tuple->heading ;
    Tcl_Obj **values = tuple->values ;
    Ral_AttributeTypeScanFlags *typeFlag ;
    Ral_AttributeValueScanFlags *valueFlag ;
    Ral_TupleHeadingIter hIter ;

    assert(typeFlags->attrType == Tuple_Type) ;
    assert(typeFlags->flags.compoundFlags.count == Ral_TupleDegree(tuple)) ;
    assert(valueFlags->attrType == Tuple_Type) ;
    assert(valueFlags->flags.compoundFlags.count == Ral_TupleDegree(tuple)) ;

    typeFlag = typeFlags->flags.compoundFlags.flags ;
    valueFlag = valueFlags->flags.compoundFlags.flags ;

    *p++ = openList ;
    for (hIter = Ral_TupleHeadingBegin(heading) ;
            hIter != Ral_TupleHeadingEnd(heading) ; ++hIter) {
	Ral_Attribute a = *hIter ;
	Tcl_Obj *v = *values++ ;

	assert(v != NULL) ;
	p += Ral_AttributeConvertName(a, p, typeFlag) ;
	*p++ = ' ' ;
	p += Ral_AttributeConvertValue(a, v, p, typeFlag, valueFlag) ;
	*p++ = ' ' ;

	++typeFlag ;
	++valueFlag ;
    }
    /*
     * Remove the trailing space. Check that the tuple actually had
     * some attributes!
     */
    if (*(p - 1) == ' ') {
	--p ;
    }
    *p++ = closeList ;

    return p - dst ;
}

char *
Ral_TupleStringOf(
    Ral_Tuple tuple)
{
    Ral_AttributeTypeScanFlags typeFlags ;
    Ral_AttributeValueScanFlags valueFlags ;
    char *str ;
    int scanLen ;
    int length ;

    memset(&typeFlags, 0, sizeof(typeFlags)) ;
    typeFlags.attrType = Tuple_Type ;
    memset(&valueFlags, 0, sizeof(valueFlags)) ;
    valueFlags.attrType = Tuple_Type ;

    /* +1 for NUL terminator */
    scanLen = Ral_TupleScan(tuple, &typeFlags, &valueFlags) + 1 ;
    str = ckalloc(scanLen) ;
    length = Ral_TupleConvert(tuple, str, &typeFlags, &valueFlags) ;
    assert(length <= scanLen) ;
    str[length] = '\0' ;

    return str ;
}

char *
Ral_TupleValueStringOf(
    Ral_Tuple tuple)
{
    Ral_TupleHeading heading = tuple->heading ;
    Ral_AttributeTypeScanFlags typeFlags ;
    Ral_AttributeValueScanFlags valueFlags ;
    char *str ;
    int length ;

    memset(&typeFlags, 0, sizeof(typeFlags)) ;
    typeFlags.attrType = Tuple_Type ;
    memset(&valueFlags, 0, sizeof(valueFlags)) ;
    valueFlags.attrType = Tuple_Type ;

    /*
     * Scan the header just to get the typeFlags set properly.
     */
    Ral_TupleHeadingScan(heading, &typeFlags) ;
    /*
     * Scan and convert only the value portion of the tuple.
     * +1 for NUL terminator
     */
    str = ckalloc(Ral_TupleScanValue(tuple, &typeFlags, &valueFlags) + 1) ;
    length = Ral_TupleConvertValue(tuple, str, &typeFlags, &valueFlags) ;
    *(str + length) = '\0' ;

    return str ;
}
