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
$Revision: 1.6 $
$Date: 2006/03/06 01:07:37 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_tupleobj.c,v $
Revision 1.6  2006/03/06 01:07:37  mangoa01
More relation commands done. Cleaned up error reporting.

Revision 1.5  2006/02/26 04:57:53  mangoa01
Reworked the conversion from internal form to a string yet again.
This design is better and more recursive in nature.
Added additional code to the "relation" commands.
Now in a position to finish off the remaining relation commands.

Revision 1.4  2006/02/20 20:15:10  mangoa01
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
#include "tcl.h"
#include "ral_utils.h"
#include "ral_vector.h"
#include "ral_tupleobj.h"
#include "ral_tuple.h"

#include <assert.h>
#include <string.h>

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
static const char rcsid[] = "@(#) $RCSfile: ral_tupleobj.c,v $ $Revision: 1.6 $" ;

/*
FUNCTION DEFINITIONS
*/

Tcl_Obj *
Ral_TupleObjNew(
    Ral_Tuple tuple)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->typePtr = &Ral_TupleObjType ;
    Ral_TupleReference(objPtr->internalRep.otherValuePtr = tuple) ;
    Tcl_InvalidateStringRep(objPtr) ;
    return objPtr ;
}

int
Ral_TupleObjConvert(
    Ral_TupleHeading heading,
    Tcl_Interp *interp,
    Tcl_Obj *value,
    Tcl_Obj *objPtr)
{
    Ral_Tuple tuple = Ral_TupleNew(heading) ;

    if (Ral_TupleSetFromObj(tuple, interp, value) != TCL_OK) {
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

Ral_TupleHeading
Ral_TupleHeadingNewFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_TupleHeading heading ;

    /*
     * Since the string representation of a Heading is a list of pairs,
     * we can use the list object functions to do the heavy lifting here.
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return NULL ;
    }
    if (objc % 2 != 0) {
	Ral_TupleObjSetError(interp, TUP_HEADING_ERR, Tcl_GetString(objPtr)) ;
	return NULL ;
    }
    heading = Ral_TupleHeadingNew(objc / 2) ;
    /*
     * Iterate through the list adding each element as an attribute to
     * a newly created Heading.
     */
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	Ral_Attribute attr ;
	Ral_TupleHeadingIter iter ;

	attr = Ral_AttributeNewFromObjs(interp, *objv, *(objv + 1)) ;
	if (attr == NULL) {
	    Ral_TupleHeadingDelete(heading) ;
	    return NULL ;
	}
	iter = Ral_TupleHeadingPushBack(heading, attr) ;
	if (iter == Ral_TupleHeadingEnd(heading)) {
	    Ral_TupleObjSetError(interp, TUP_DUPLICATE_ATTR, attr->name) ;
	    Ral_AttributeDelete(attr) ;
	    Ral_TupleHeadingDelete(heading) ;
	    return NULL ;
	}
    }

    return heading ;
}

/*
 * Scan the object and return the list of attributes.
 * The returned vector contains a set of attribute indices
 * that map into the heading for those attribute names mentioned
 * in the Tcl object.
 * Caller must delete the returned vector.
 */
Ral_IntVector
Ral_TupleHeadingAttrsFromObj(
    Ral_TupleHeading heading,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;

    /*
     * Convert the object into a list
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return NULL ;
    }
    return Ral_TupleHeadingAttrsFromVect(heading, interp, objc, objv) ;
}

Ral_IntVector
Ral_TupleHeadingAttrsFromVect(
    Ral_TupleHeading heading,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_IntVector attrVector = Ral_IntVectorNewEmpty(objc) ;
    /*
     * Iterate over the array looking up the attributes in the
     * tuple heading.
     */
    while (objc-- > 0) {
	const char *attrName = Tcl_GetString(*objv++) ;
	int attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	if (attrIndex >= 0) {
	    /*
	     * N.B. Attributes must always form a set.
	     */
	    Ral_IntVectorSetAdd(attrVector, attrIndex) ;
	} else {
	    Ral_TupleObjSetError(interp, TUP_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(attrVector) ;
	    return NULL ;
	}
    }

    return attrVector ;
}

/*
 * The Tcl object must be a list of attribute value pairs and a value
 * must be given for every attribute.
 */
int
Ral_TupleSetFromObj(
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
	Ral_TupleObjSetError(interp, TUP_FORMAT_ERR, Tcl_GetString(objPtr)) ;
	return TCL_ERROR ;
    }
    if (elemc / 2 != Ral_TupleDegree(tuple)) {
	Ral_TupleObjSetError(interp, TUP_WRONG_NUM_ATTRS,
	    Tcl_GetString(objPtr)) ;
	return TCL_ERROR ;
    }
    /*
     * Go through the attribute / value pairs making sure that that each
     * attribute is mentioned exactly once.  We use a vector to keep track of
     * this.
     */
    attrStatus = Ral_IntVectorNew(Ral_TupleDegree(tuple), 0) ;
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	const char *attrName = Tcl_GetString(*elemv) ;
	int hindex = Ral_TupleHeadingIndexOf(tuple->heading, attrName) ;

	if (hindex < 0) {
	    Ral_TupleObjSetError(interp, TUP_UNKNOWN_ATTR, attrName) ;
	    goto errorOut ;
	} else if (Ral_IntVectorFetch(attrStatus, hindex)) {
	    Ral_TupleObjSetError(interp, TUP_DUPLICATE_ATTR, attrName) ;
	    goto errorOut ;
	}
	Ral_IntVectorStore(attrStatus, hindex, 1) ;
    }
    Ral_IntVectorDelete(attrStatus) ;

    /*
     * Once we've established that all the attributes are in the list,
     * then the normal update function assigns the values to the attributes.
     */
    return Ral_TupleUpdateFromObj(tuple, interp, objPtr) ;

errorOut:
    Ral_IntVectorDelete(attrStatus) ;
    return TCL_ERROR ;
}

/*
 * For update purposes, we do not insist that every attribute be set.
 */
int
Ral_TupleUpdateFromObj(
    Ral_Tuple tuple,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int elemc ;
    Tcl_Obj **elemv ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc % 2 != 0) {
	Ral_TupleObjSetError(interp, TUP_FORMAT_ERR, Tcl_GetString(objPtr)) ;
	return TCL_ERROR ;
    }
    /*
     * Go through the attribute / value pairs updating the attribute values.
     */
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	const char *attrName = Tcl_GetString(*elemv) ;
	if (!Ral_TupleUpdateAttrValue(tuple, attrName, elemv[1])) {
	    if (Ral_TupleLastError == TUP_BAD_VALUE) {
		Ral_TupleObjSetError(interp, Ral_TupleLastError,
		    Tcl_GetString(elemv[1])) ;
	    } else {
		Ral_TupleObjSetError(interp, Ral_TupleLastError, attrName) ;
	    }
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

const char *
Ral_TupleObjVersion(void)
{
    return rcsid ;
}

void
Ral_TupleObjSetError(
    Tcl_Interp *interp,
    Ral_TupleError error,
    const char *param)
{
    /*
     * These must be in the same order as the encoding of the Ral_TupleError
     * enumeration.
     */
    static const char *resultStrings[] = {
	"no error",
	"unknown attribute name",
	"bad tuple heading format",
	"bad tuple value format",
	"duplicate attribute name",
	"bad value type for value",
	"bad tuple type keyword",
	"wrong number of attributes specified",
	"bad list of pairs"
    } ;
    static const char *errorStrings[] = {
	"OK",
	"UNKNOWN_ATTR",
	"HEADING_ERR",
	"FORMAT_ERR",
	"DUPLICATE_ATTR",
	"BAD_VALUE",
	"BAD_KEYWORD",
	"WRONG_NUM_ATTRS",
	"BAD_PAIRS_LIST",
    } ;

    Ral_ObjSetError(interp, "TUPLE", resultStrings[error], errorStrings[error],
	param) ;
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

    dupTuple = Ral_TupleDup(srcTuple) ;
    Ral_TupleReference(dupPtr->internalRep.otherValuePtr = dupTuple) ;
    dupPtr->typePtr = &Ral_TupleObjType ;
}

static void
UpdateStringOfTuple(
    Tcl_Obj *objPtr)
{
    objPtr->bytes = Ral_TupleStringOf(objPtr->internalRep.otherValuePtr) ;
    objPtr->length = strlen(objPtr->bytes) ;
}

static int
SetTupleFromAny(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_TupleHeading heading ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc != 3) {
	Ral_TupleObjSetError(interp, TUP_FORMAT_ERR, Tcl_GetString(objPtr)) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_TupleObjType.name, Tcl_GetString(*objv)) != 0) {
	Ral_TupleObjSetError(interp, TUP_BAD_KEYWORD, Tcl_GetString(*objv)) ;
	return TCL_ERROR ;
    }

    heading = Ral_TupleHeadingNewFromObj(interp, objv[1]) ;
    if (!heading) {
	return TCL_ERROR ;
    }
    return Ral_TupleObjConvert(heading, interp, objv[2], objPtr) ;
}
