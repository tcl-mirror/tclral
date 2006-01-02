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
$Revision: 1.2 $
$Date: 2006/01/02 01:39:29 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_tuple.c,v $
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
static const char openList[] = "{" ;
static const char closeList[] = "}" ;
static const char rcsid[] = "@(#) $RCSfile: ral_tuple.c,v $ $Revision: 1.2 $" ;

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

unsigned
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
    Ral_TupleHeading srcHeading = src->heading ;
    Ral_TupleHeading dstHeading = dst->heading ;
    int dstOffset ;
    int srcOffset ;
    int srcCount ;
    Tcl_Obj **srcValues ;
    Tcl_Obj **dstValues ;

    /*
     * Record where the copy of the values will begin.
     */
    dstOffset = Ral_TupleHeadingSize(dstHeading) ;
    /*
     * Copy the attributes into the destination heading.
     */
    if (!Ral_TupleHeadingAppend(srcHeading, start, finish, dstHeading)) {
	return 0 ;
    }
    /*
     * Compute the offsets into the value vectors where the copy will begin.
     */
    srcOffset = start - Ral_TupleHeadingBegin(srcHeading) ;
    srcValues = src->values + srcOffset ;
    dstValues = dst->values + dstOffset ;
    /*
     * Copy the values making sure the increment the reference count.
     */
    for (srcCount = finish - start ; srcCount > 0 ; --srcCount) {
	Tcl_IncrRefCount(*dstValues++ = *srcValues++) ;
    }

    return 1 ;
}

Ral_Tuple
Ral_TupleDuplicate(
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
Ral_TupleScanValue(
    Ral_Tuple tuple,
    Ral_AttributeScanFlags flags)
{
    int length ;
    Ral_TupleHeading heading = tuple->heading ;
    Tcl_Obj **values = tuple->values ;
    Ral_TupleHeadingIter hIter ;
    Ral_TupleHeadingIter hEnd ;

    /*
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    length = sizeof(openList) - 1 ;
    hEnd = Ral_TupleHeadingEnd(heading) ;
    for (hIter = Ral_TupleHeadingBegin(heading) ; hIter != hEnd ; ++hIter) {
	Ral_Attribute a = *hIter ;
	Tcl_Obj *v = *values++ ;

	length += Ral_AttributeScanName(a, flags) + 1 ; /* +1 for space */
	length += Ral_AttributeScanValue(a, v, flags) + 1 ;
	++flags ;
    }
    length += sizeof(closeList) - 1 ;

    return length ;
}

int
Ral_TupleConvertValue(
    Ral_Tuple tuple,
    char *dst,
    Ral_AttributeScanFlags flags)
{
    char *p = dst ;
    Ral_TupleHeading heading = tuple->heading ;
    Tcl_Obj **values = tuple->values ;
    Ral_TupleHeadingIter hIter ;
    Ral_TupleHeadingIter hEnd ;

    /*
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    strcpy(p, openList) ;
    p += sizeof(openList) - 1 ;
    hEnd = Ral_TupleHeadingEnd(heading) ;
    for (hIter = Ral_TupleHeadingBegin(heading) ; hIter != hEnd ; ++hIter) {
	Ral_Attribute a = *hIter ;
	Tcl_Obj *v = *values++ ;

	p += Ral_AttributeConvertName(a, p, flags) ;
	*p++ = ' ' ;
	p += Ral_AttributeConvertValue(a, v, p, flags) ;
	*p++ = ' ' ;

	++flags ;
    }
    /*
     * Remove the trailing space. Check that the tuple actually had
     * some attributes!
     */
    if (Ral_TupleDegree(tuple)) {
	--p ;
    }
    strcpy(p, closeList) ;
    p += sizeof(closeList) - 1 ;

    return p - dst ;
}

int
Ral_TupleScan(
    Ral_Tuple tuple,
    Ral_AttributeScanFlags *flags)
{
    Ral_TupleHeading heading = tuple->heading ;
    Ral_AttributeScanFlags scanFlags ;
    int length ;

    /*
     * To scan the tuple, we need an array of attribute scan flags
     * that is the same size as the tuple.
     */
    *flags = scanFlags =
	(Ral_AttributeScanFlags)ckalloc(Ral_TupleHeadingSize(heading) *
	    sizeof(*scanFlags)) ;

    /*
     * The tuple header has a function that does the work of gathering
     * the information on the attribute names and type name strings.
     */
    length = Ral_TupleHeadingScan(heading, scanFlags) ;
    length += 1 ; /* +1 for the separating space */
    /*
     * Now we need to scan all the values, computing the length and
     * recording the scan flags for the values.
     */
    length += Ral_TupleScanValue(tuple, scanFlags) ;

    return length ;
}

int
Ral_TupleConvert(
    Ral_Tuple tuple,
    char *dst,
    Ral_AttributeScanFlags scanFlags)
{
    char *p = dst ;

    /*
     * Convert the heading.
     */
    p += Ral_TupleHeadingConvert(tuple->heading, p, scanFlags) ;
    /*
     * Separate the heading from the body by space.
     */
    *p++ = ' ' ;
    /*
     * Convert the body.
     */
    p += Ral_TupleConvertValue(tuple, p, scanFlags) ;

    return p - dst ;
}

void
Ral_TuplePrint(
    Ral_Tuple tuple,
    FILE *f)
{
    Ral_TupleHeading h = tuple->heading ;
    Ral_TupleHeadingIter i ;
    Ral_TupleHeadingIter end = Ral_TupleHeadingEnd(h) ;
    Tcl_Obj **v = tuple->values ;
    int valueIndex = 0 ;

    for (i = Ral_TupleHeadingBegin(h) ; i != end ; ++i) {
	Ral_Attribute a = *i ;
	Tcl_Obj *value = v[valueIndex] ;

	fprintf(f, "%d: %s ==> %s\n", valueIndex, a->name,
	    value ?  Tcl_GetString(v[valueIndex]) : "none") ;

	++valueIndex ;
    }
}

const char *
Ral_TupleVersion(void)
{
    return rcsid ;
}
