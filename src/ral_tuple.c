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

$RCSfile: ral_tuple.c,v $
$Revision: 1.5 $
$Date: 2006/02/26 04:57:53 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_tuple.c,v $
Revision 1.5  2006/02/26 04:57:53  mangoa01
Reworked the conversion from internal form to a string yet again.
This design is better and more recursive in nature.
Added additional code to the "relation" commands.
Now in a position to finish off the remaining relation commands.

Revision 1.4  2006/02/20 20:15:07  mangoa01
Now able to convert strings to relations and vice versa including
tuple and relation valued attributes.

Revision 1.3  2006/02/06 05:02:45  mangoa01
Started on relation heading and other code refactoring.
This is a checkpoint after a number of added files and changes
to tuple heading code.

Revision 1.2  2006/01/02 01:39:29  mangoa01
Tuple commands now operate properly. Fixed problems of constructing the string representation when there were tuple valued attributes.

Revision 1.1  2005/12/27 23:17:19  mangoa01
Update to the new spilt out file structure.

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
static const char rcsid[] = "@(#) $RCSfile: ral_tuple.c,v $ $Revision: 1.5 $" ;

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

void
Ral_TupleDelete(
    Ral_Tuple tuple)
{
    if (tuple) {
	Tcl_Obj **objPtr ;
	Tcl_Obj **last ;

	assert(tuple->refCount <= 0) ;
	for (objPtr = tuple->values,
	    last = objPtr + Ral_TupleHeadingSize(tuple->heading) ;
	    objPtr != last ; ++objPtr) {
	    if (*objPtr) {
		Tcl_DecrRefCount(*objPtr) ;
	    }
	}
	Ral_TupleHeadingUnreference(tuple->heading) ;

	ckfree((char *)tuple) ;
    }
}

void
Ral_TupleReference(
    Ral_Tuple tuple)
{
    if (tuple) {
	++tuple->refCount ;
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
Ral_TupleDegree(
    Ral_Tuple tuple)
{
    return Ral_TupleHeadingSize(tuple->heading) ;
}

int
Ral_TupleEqual(
    Ral_Tuple tuple1,
    Ral_Tuple tuple2)
{
    Ral_TupleHeading h1 ;
    Ral_TupleHeading h2 ;
    Tcl_Obj **v1 ;
    Tcl_Obj **v2 ;
    Ral_TupleHeadingIter b1 ;
    Ral_TupleHeadingIter e1 ;

    /*
     * Tuples at the same address are equal.
     */
    if (tuple1 == tuple2) {
	return 1 ;
    }
    /*
     * For tuples to be equal they must have equal headings.
     */
    h1 = tuple1->heading ;
    h2 = tuple2->heading ;
    if (!Ral_TupleHeadingEqual(h1, h2)) {
	return 0 ;
    }
    /*
     * Match the values. We iterate on the first tuple, using the
     * attribute name to find the proper index into the second tuple.
     * Thus the comparison is independent of storage order as desired.
     */
    v1 = tuple1->values ;
    v2 = tuple2->values ;
    e1 = Ral_TupleHeadingEnd(h1) ;
    for (b1 = Ral_TupleHeadingBegin(h1) ; b1 != e1 ; ++b1) {
	int i2 = Ral_TupleHeadingIndexOf(h2, (*b1)->name) ;
	/*
	 * Since the two headings are equal, we must be able to find
	 * each attribute.
	 */
	assert(i2 >= 0) ;
	/*
	 * And all the object values must be equal.
	 */
	if (!Ral_ObjEqual(*v1++, v2[i2])) {
	    return 0 ;
	}
    }
    return 1 ;
}

Ral_TupleUpdateStatus
Ral_TupleUpdateAttrValue(
    Ral_Tuple tuple,
    const char *attrName,
    Tcl_Obj *value)
{
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter i_attr ;
    Ral_Attribute attribute ;
    int valueIndex ;
    Tcl_Obj *oldValue ;

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
	return NoSuchAttribute ;
    }
    attribute = *i_attr ;
    /*
     * Convert the value to the type of the attribute.
     */
    if (Ral_AttributeConvertValueToType(NULL, attribute, value) != TCL_OK) {
	return BadValueType ;
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
    Tcl_IncrRefCount(tuple->values[valueIndex] = value) ;
    return AttributeUpdated ;
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
void
Ral_TupleCopyValues(
    Ral_TupleIter start,
    Ral_TupleIter finish,
    Ral_TupleIter dst)
{
    /*
     * Copy the values making sure the increment the reference count.
     */
    while (start != finish) {
	Tcl_IncrRefCount(*dst++ = *start++) ;
    }
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
     * +1 for space
     */
    length = Ral_TupleHeadingScan(heading, typeFlags) + 1 ;
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
     * Convert the heading.
     */
    p += Ral_TupleHeadingConvert(tuple->heading, p, typeFlags) ;
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
    Ral_TupleHeadingIter hEnd ;

    assert(typeFlags->attrType == Tuple_Type) ;
    assert(typeFlags->compoundFlags.count == Ral_TupleDegree(tuple)) ;
    assert(valueFlags->attrType == Tuple_Type) ;
    assert(valueFlags->compoundFlags.flags == NULL) ;

    /*
     * Allocate space for the value flags.
     */
    valueFlags->compoundFlags.count = typeFlags->compoundFlags.count ;
    nBytes = valueFlags->compoundFlags.count *
	sizeof(*valueFlags->compoundFlags.flags) ;
    valueFlags->compoundFlags.flags =
	(Ral_AttributeValueScanFlags *)ckalloc(nBytes) ;
    memset(valueFlags->compoundFlags.flags, 0, nBytes) ;

    /*
     * Iterate through the heading and scan the corresponding values.
     */
    typeFlag = typeFlags->compoundFlags.flags ;
    valueFlag = valueFlags->compoundFlags.flags ;
    hEnd = Ral_TupleHeadingEnd(heading) ;

    length = sizeof(openList) ;
    for (hIter = Ral_TupleHeadingBegin(heading) ; hIter != hEnd ; ++hIter) {
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
    Ral_TupleHeadingIter hEnd ;

    assert(typeFlags->attrType == Tuple_Type) ;
    assert(typeFlags->compoundFlags.count == Ral_TupleDegree(tuple)) ;
    assert(valueFlags->attrType == Tuple_Type) ;
    assert(valueFlags->compoundFlags.count == Ral_TupleDegree(tuple)) ;

    typeFlag = typeFlags->compoundFlags.flags ;
    valueFlag = valueFlags->compoundFlags.flags ;
    hEnd = Ral_TupleHeadingEnd(heading) ;

    *p++ = openList ;
    for (hIter = Ral_TupleHeadingBegin(heading) ; hIter != hEnd ; ++hIter) {
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
    if (Ral_TupleDegree(tuple)) {
	--p ;
    }
    *p++ = closeList ;

    return p - dst ;
}

void
Ral_TuplePrint(
    Ral_Tuple tuple,
    const char *format,
    FILE *f)
{
    char *str = Ral_TupleStringOf(tuple) ;
    fprintf(f, format, str) ;
    ckfree(str) ;
}

char *
Ral_TupleStringOf(
    Ral_Tuple tuple)
{
    Ral_AttributeTypeScanFlags typeFlags ;
    Ral_AttributeValueScanFlags valueFlags ;
    char *str ;
    int length ;

    memset(&typeFlags, 0, sizeof(typeFlags)) ;
    typeFlags.attrType = Tuple_Type ;
    memset(&valueFlags, 0, sizeof(valueFlags)) ;
    valueFlags.attrType = Tuple_Type ;

    /* +1 for NUL terminator */
    str = ckalloc(Ral_TupleScan(tuple, &typeFlags, &valueFlags) + 1) ;
    length = Ral_TupleConvert(tuple, str, &typeFlags, &valueFlags) ;
    str[length] = '\0' ;

    return str ;
}

const char *
Ral_TupleVersion(void)
{
    return rcsid ;
}
