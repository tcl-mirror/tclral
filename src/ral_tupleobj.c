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

$RCSfile: ral_tupleobj.c,v $
$Revision: 1.1 $
$Date: 2005/12/27 23:17:19 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_tupleobj.c,v $
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
#include "tcl.h"
#include "ral_vector.h"
#include "ral_tupleobj.h"
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
 * Functions that implement the generic object operations for a Tuple.
 */
static void FreeTupleInternalRep(Tcl_Obj *) ;
static void DupTupleInternalRep(Tcl_Obj *, Tcl_Obj *) ;
static void UpdateStringOfTuple(Tcl_Obj *) ;
static int SetTupleFromAny(Tcl_Interp *, Tcl_Obj *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/
Tcl_ObjType Ral_TupleObjType = {
    "Tuple",
    FreeTupleInternalRep,
    DupTupleInternalRep,
    UpdateStringOfTuple,
    SetTupleFromAny
} ;

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_tupleobj.c,v $ $Revision: 1.1 $" ;

/*
FUNCTION DEFINITIONS
*/

Tcl_Obj *
Ral_TupleNewObj(
    Ral_Tuple tuple)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->typePtr = &Ral_TupleObjType ;
    Ral_TupleReference(objPtr->internalRep.otherValuePtr = tuple) ;
    Tcl_InvalidateStringRep(objPtr) ;
    return objPtr ;
}

/*
 * The Tcl object must be a list of attribute value pairs and a value
 * must be given for every attribute.
 */
int
Ral_TupleSetValuesFromObj(
    Ral_Tuple tuple,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_IntVector attrStatus ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc % 2 != 0) {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"list must have an even number of elements", NULL) ;
	}
	return TCL_ERROR ;
    }
    if (elemc / 2 != Ral_TupleDegree(tuple)) {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"wrong number of attributes specified", NULL) ;
	}
	return TCL_ERROR ;
    }
    /*
     * Go through the attribute / value pairs updating the tuple values.
     * We must make sure that each attribute is assigned to exactly once.
     * We use a vector to keep track of this.
     */
    attrStatus = Ral_IntVectorNew(Ral_TupleDegree(tuple), 0) ;
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	const char *attrName = Tcl_GetString(*elemv) ;
	int hindex = Ral_TupleHeadingIndexOf(tuple->heading, attrName) ;
	Ral_TupleUpdateStatus status ;

	if (hindex < 0) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"unknown attribute name, \"", attrName, "\"", NULL) ;
	    goto errorOut ;
	} else if (Ral_IntVectorFetch(attrStatus, hindex)) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"attempted to update the value of attribute, \"",
		attrName, "\" more than once", NULL) ;
	    goto errorOut ;
	}
	Ral_IntVectorStore(attrStatus, hindex, 1) ;

	status = Ral_TupleUpdateAttrValue(tuple, attrName, *(elemv + 1)) ;
	if (status != AttributeUpdated) {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		if (status == NoSuchAttribute) {
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"unknown attribute name, \"", attrName, "\"", NULL) ;
		} else if (status == BadValueType) {
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"bad value type for value, \"",
			Tcl_GetString(*(elemv + 1)), "\"", NULL) ;
		} else {
		    Tcl_Panic("unknown tuple update status, \"%d\"", status) ;
		}
	    }
	    goto errorOut ;
	}
    }

    Ral_IntVectorDelete(attrStatus) ;
    return TCL_OK ;

errorOut:
    Ral_IntVectorDelete(attrStatus) ;
    return TCL_ERROR ;
}

const char *
Ral_TupleObjVersion(void)
{
    return rcsid ;
}

/*
 * ======================================================================
 * Functions to Support the Tcl type
 * ======================================================================
 */

static void
FreeTupleInternalRep(
    Tcl_Obj *objPtr)
{
    assert(objPtr->typePtr == &Ral_TupleObjType) ;
    Ral_TupleUnreference(objPtr->internalRep.otherValuePtr) ;
    objPtr->typePtr = objPtr->internalRep.otherValuePtr = NULL ;
}

static void
DupTupleInternalRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    Ral_Tuple srcTuple ;
    Ral_Tuple dupTuple ;

    assert(srcPtr->typePtr == &Ral_TupleObjType) ;
    srcTuple = srcPtr->internalRep.otherValuePtr ;

    dupTuple = Ral_TupleDuplicate(srcTuple) ;
    Ral_TupleReference(dupPtr->internalRep.otherValuePtr = dupTuple) ;
    dupPtr->typePtr = &Ral_TupleObjType ;
}

/*
 * The string representation of a "Tuple" is a specially formed list.  The list
 * consists of three elements:
 *
 * 1. A heading definition.
 *
 * 2. A value definition.
 *
 * The heading consists of a two element list, the first element being the
 * keyword "Tuple" to distinguish the type and the second element being a list
 * Attribute Name and Data Type pairs.  The value definition is also a list
 * consisting of Attribute Name / Attribute Value pairs. A tuple value is then
 * the concatenation of these two lists to yield a list of three elements.
 * e.g.
 *	{Tuple {Name string Street int Wage double}\
 *	{Name Andrew Street Blackwood Wage 5.74}}
 * So to convert the internal representation to a string the strategy is
 * to use Tcl_ScanElement and Tcl_ConvertElement to generate the proper list
 * representation. There are Tuple Header functions to do deal with the
 * header part of the tuple.
 */
static void
UpdateStringOfTuple(
    Tcl_Obj *objPtr)
{
    static const char openList[] = " {" ;
    static const char closeList[] = "}" ;

    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    int length ;
    Ral_TupleHeadingScanFlags headingFlags ;
    unsigned size ;
    int *flagVect ;
    int *flagPtr ;
    Ral_TupleHeadingIter hIter ;
    Ral_TupleHeadingIter hEnd ;
    Tcl_Obj **v ;
    char *dst ;

    tuple = objPtr->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    /*
     * First phase is to scan the tuple header and values to compute the
     * amount of space required to hold the string representation. Compare
     * here with the way the internal "list" type accomplishes this.
     */

    /*
     * The tuple header has a function that does the work.
     */
    length = Ral_TupleHeadingScan(heading, &headingFlags) ;

    /*
     * The tuple values part of the list is alternating attribute
     * names and values. Here we set up to use Tcl_ScanElement().
     */
    size = Ral_TupleHeadingSize(heading) ;
    /* Need flags for attribute names and values */
    flagPtr = flagVect = (int *)ckalloc(sizeof(*flagVect) * size * 2) ;

    hEnd = Ral_TupleHeadingEnd(heading) ;
    v = tuple->values ;

    /*
     * Iterate through the values and compute the space needed for
     * the attribute names and values.
     */
    /*
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    length += sizeof(openList) - 1 ;
    for (hIter = Ral_TupleHeadingBegin(heading) ; hIter != hEnd ; ++hIter) {
	Ral_Attribute a = *hIter ;

	/* +1 for the space between list elements */
	length += Ral_AttributeScanName(a, flagPtr++) + 1 ;
	length += Tcl_ScanElement(Tcl_GetString(*v++), flagPtr++) + 1 ;
    }
    length += sizeof(closeList) - 1 ;

    /*
     * Second phase is to allocate the memory and generate the string rep
     * into the object structure.
     */

    objPtr->bytes = dst = ckalloc(length + 1) ; /* +1 for the NUL terminator */
    /*
     * Copy in the Tuple Heading.
     */
    dst += Ral_TupleHeadingConvert(heading, dst, headingFlags) ;
    /*
     * The next element is the attribute / value pairs list.
     */
    strcpy(dst, openList) ;
    dst += sizeof(openList) - 1 ;
    v = tuple->values ;
    flagPtr = flagVect ;
    for (hIter = Ral_TupleHeadingBegin(heading) ; hIter != hEnd ; ++hIter) {
	Ral_Attribute a = *hIter ;

	dst += Ral_AttributeConvertName(a, dst, *flagPtr++) ;
	*dst++ = ' ' ;
	dst += Tcl_ConvertElement(Tcl_GetString(*v++), dst, *flagPtr++) ;
	*dst++ = ' ' ;
    }
    /* Done with the scan flags. */
    ckfree((char *)flagVect) ;
    /* Get rid of trailing blank, but only for non-empty headings */
    if (Ral_TupleHeadingSize(heading)) {
	--dst ;
    }
    /*
     * Note that copying in the closing brace also NUL terminates the result.
     */
    strcpy(dst, closeList) ;
    dst += sizeof(closeList) - 1 ;
    objPtr->length = dst - objPtr->bytes ;
}

static int
SetTupleFromAny(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_TupleHeading heading ;
    Ral_Tuple tuple ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc != 3) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "badly formatted tuple, \"", Tcl_GetString(objPtr), "\"", NULL) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_TupleObjType.name, Tcl_GetString(*objv)) != 0) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "bad tuple type keyword: expected, \"", Ral_TupleObjType.name,
	    "\", but got, \"", Tcl_GetString(*objv), "\"", NULL) ;
	return TCL_ERROR ;
    }

    heading = Ral_TupleHeadingNewFromObj(interp, *(objv + 1)) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    tuple = Ral_TupleNew(heading) ;
    if (Ral_TupleSetValuesFromObj(tuple, interp, *(objv + 2)) != TCL_OK) {
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }
    /*
     * Discard the old internal representation.
     */
    if (objPtr->typePtr && objPtr->typePtr->freeIntRepProc) {
	objPtr->typePtr->freeIntRepProc(objPtr) ;
    }
    /*
     * Invalidate the string representation.  There are several string reps
     * that will map to the same tuple and we want to force a new string rep to
     * be generated in order to obtain the canonical string form.
     */
    Tcl_InvalidateStringRep(objPtr) ;
    /*
     * Install the new internal representation.
     */
    objPtr->typePtr = &Ral_TupleObjType ;
    Ral_TupleReference(objPtr->internalRep.otherValuePtr = tuple) ;

    return TCL_OK ;
}
