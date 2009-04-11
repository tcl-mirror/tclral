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

ABSTRACT:

$RCSfile: ral_tupleobj.c,v $
$Revision: 1.15 $
$Date: 2009/04/11 18:18:54 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_attribute.h"
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
    ral_tupleTypeName,
    FreeTupleInternalRep,
    DupTupleInternalRep,
    UpdateStringOfTuple,
    SetTupleFromAny
} ;

/*
STATIC DATA ALLOCATION
*/

/*
FUNCTION DEFINITIONS
*/

Tcl_Obj *
Ral_TupleObjNew(
    Ral_Tuple tuple)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->typePtr = &Ral_TupleObjType ;
    objPtr->internalRep.otherValuePtr = tuple ;
    Ral_TupleReference(tuple) ;
    objPtr->bytes = NULL ;
    objPtr->length = 0 ;
    return objPtr ;
}

int
Ral_TupleObjConvert(
    Ral_TupleHeading heading,
    Tcl_Interp *interp,
    Tcl_Obj *value,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    Ral_Tuple tuple = Ral_TupleNew(heading) ;

    if (Ral_TupleSetFromObj(tuple, interp, value, errInfo) != TCL_OK) {
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
     * Install the new internal representation.
     */
    objPtr->typePtr = &Ral_TupleObjType ;
    objPtr->internalRep.otherValuePtr = tuple ;
    Ral_TupleReference(tuple) ;

    return TCL_OK ;
}

Ral_TupleHeading
Ral_TupleHeadingNewFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
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
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, objPtr) ;
	Ral_InterpSetError(interp, errInfo) ;
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

	attr = Ral_AttributeNewFromObjs(interp, *objv, *(objv + 1), errInfo) ;
	if (attr == NULL) {
	    Ral_TupleHeadingDelete(heading) ;
	    return NULL ;
	}
	iter = Ral_TupleHeadingPushBack(heading, attr) ;
	if (iter == Ral_TupleHeadingEnd(heading)) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR, attr->name) ;
	    Ral_InterpSetError(interp, errInfo) ;

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
	char const *attrName = Tcl_GetString(*objv++) ;
	int attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	if (attrIndex >= 0) {
	    /*
	     * N.B. Attributes must always form a set.
	     */
	    Ral_IntVectorSetAdd(attrVector, attrIndex) ;
	} else {
	    Ral_InterpErrorInfo(interp, Ral_CmdUnknown, Ral_OptNone,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(attrVector) ;
            attrVector = NULL ;
            break ;
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
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_IntVector attrStatus ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * We must have attribute name / value pairs.
     */
    if (elemc % 2 != 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, objPtr) ;
	Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Make sure we get the correct number of attributes.
     */
    if (elemc / 2 != Ral_TupleDegree(tuple)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_WRONG_NUM_ATTRS, objPtr) ;
	Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Go through the attribute / value pairs making sure that that each
     * attribute is known and mentioned no more than once.  We use a vector to
     * keep track of this.
     */
    attrStatus = Ral_IntVectorNew(Ral_TupleDegree(tuple), 0) ;
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	const char *attrName = Tcl_GetString(*elemv) ;
	int hindex = Ral_TupleHeadingIndexOf(tuple->heading, attrName) ;

	if (hindex < 0) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    goto errorOut ;
	} else if (Ral_IntVectorFetch(attrStatus, hindex)) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR, attrName) ;
	    goto errorOut ;
	}
	Ral_IntVectorStore(attrStatus, hindex, 1) ;
    }
    /*
     * At this point we know that all the attributes are accounted for because
     * we insisted that the number of attributes match the number in the tuple
     * heading, that all of them are found in the tuple heading and that no
     * duplicates were among them.
     */
    Ral_IntVectorDelete(attrStatus) ;

    /*
     * Once we've established that all the attributes are in the list,
     * then the normal update function assigns the values to the attributes.
     */
    return Ral_TupleUpdateFromObj(tuple, interp, objPtr, errInfo) ;

errorOut:
    Ral_IntVectorDelete(attrStatus) ;
    Ral_InterpSetError(interp, errInfo) ;
    return TCL_ERROR ;
}

/*
 * The Tcl object must be a list of attribute values in the same
 * order as the tuple heading. A value must be given for every attribute.
 */
int
Ral_TupleSetFromValueList(
    Ral_Tuple tuple,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_TupleIter tIter ;
    Ral_TupleHeadingIter hIter ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Make sure we get the correct number of attributes.
     */
    if (elemc != Ral_TupleDegree(tuple)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_WRONG_NUM_ATTRS, objPtr) ;
	Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Go through the attributes in the tuple and update the value to that
     * given in the value list.
     */
    hIter = Ral_TupleHeadingBegin(tuple->heading) ;
    for (tIter = Ral_TupleBegin(tuple) ; tIter != Ral_TupleEnd(tuple) ;
            ++tIter) {
        Tcl_Obj *cvtValue ;
        Tcl_Obj *oldValue ;

        cvtValue = Ral_AttributeConvertValueToType(interp, *hIter++, *elemv++,
            errInfo) ;
        if (cvtValue == NULL) {
            Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_VALUE, *(elemv - 1)) ;
            return TCL_ERROR ;
        }
        oldValue = *tIter ;
        if (oldValue) {
            Tcl_DecrRefCount(oldValue) ;
        }
        Tcl_IncrRefCount(cvtValue) ;
        *tIter = cvtValue ;
    }

    return TCL_OK ;
}

/*
 * Construct a tuple from a name / value list. Obtain the data type information
 * from the "tuple" argument and insist that there are no unknown attribute
 * names. But allow the tuple to be "short", i.e. some of the attributes may
 * not be present. Return the newly created tuple object.
 */
Tcl_Obj *
Ral_TuplePartialSetFromObj(
    Ral_TupleHeading heading,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_TupleHeading newHeading ;
    Ral_Tuple newTuple ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return NULL ;
    }
    /*
     * We must have attribute name / value pairs.
     */
    if (elemc % 2 != 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, objPtr) ;
	Ral_InterpSetError(interp, errInfo) ;
	return NULL ;
    }
    newHeading = Ral_TupleHeadingNew(elemc / 2) ;
    newTuple = Ral_TupleNew(newHeading) ;
    /*
     * Go through the attribute / value pairs making sure that that each
     * attribute is known in the tuple. If so then add the attribute
     * to the heading and also add its value to the tuple.
     */
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	const char *attrName ;
        Tcl_Obj *value ;
        Tcl_Obj *cvtValue ;
	Ral_TupleHeadingIter found ;
        int status ;
	Ral_TupleHeadingIter aIter ;

	attrName = Tcl_GetString(*elemv) ;
        value = *(elemv + 1) ;

	found = Ral_TupleHeadingFind(heading, attrName) ;
	if (found == Ral_TupleHeadingEnd(heading)) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
            Ral_InterpSetError(interp, errInfo) ;
            goto errorOut ;
	}
        status = Ral_TupleHeadingAppend(heading, found, found + 1, newHeading) ;
        if (status == 0) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUPLICATE_ATTR, attrName) ;
            Ral_InterpSetError(interp, errInfo) ;
            goto errorOut ;
        }
        /*
         * Convert the value to the type of the attribute.
         */
        aIter = Ral_TupleHeadingEnd(newHeading) - 1 ;
        cvtValue = Ral_AttributeConvertValueToType(interp, *aIter, value,
                errInfo) ;
        if (cvtValue == NULL) {
            goto errorOut ;
        }
        /*
         * Update the tuple to the new value.
         */
        newTuple->values[aIter - Ral_TupleHeadingBegin(newHeading)] = cvtValue ;
        Tcl_IncrRefCount(cvtValue) ;
    }

    return Ral_TupleObjNew(newTuple) ;

errorOut:
    Ral_TupleDelete(newTuple) ;
    return NULL ;
}

/*
 * For update purposes, we do not insist that every attribute be set.
 */
int
Ral_TupleUpdateFromObj(
    Ral_Tuple tuple,
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc % 2 != 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, objPtr) ;
	Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Go through the attribute / value pairs updating the attribute values.
     */
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	const char *attrName = Tcl_GetString(*elemv) ;

	if (!Ral_TupleUpdateAttrValue(tuple, attrName, elemv[1], errInfo)) {
	    Ral_InterpSetError(interp, errInfo) ;
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

/*
 * Assign tuple attributes to Tcl variables. May assign all or part
 * of the attributes to variable of either the same name as the attribute
 * or a specified name.
 * The interpreter value is set to the number of assignment made.
 */
int
Ral_TupleAssignToVars(
    Ral_Tuple tuple,	    /* tuple to assign */
    Tcl_Interp *interp,	    /* ubiquitous interpreter */
    int varc,		    /* count of attribute / variable name objects */
    Tcl_Obj *const*varv,    /* vector of attribute / variable name objects
			     * Attribute / Variable name objects may be one or
			     * two element lists. If one, it is the name of an
			     * attribute and a variable of the same name is
			     * created. If two, the first is an attribute name
			     * and the second is the name of the variable to
			     * which the attribute value is assigned. */
    Ral_ErrorInfo *errInfo) /* error information to identify the command */
{
    Ral_TupleHeading heading = tuple->heading ;
    Tcl_Obj **values ;
    int assigned = 0 ;

    /*
     * If there are no at attribute / variable names given, than assign
     * all attributes to variables that are the same name as the attribute.
     */
    if (varc == 0) {
	Ral_TupleHeadingIter hiter ;
	Ral_TupleHeadingIter hend = Ral_TupleHeadingEnd(heading) ;

	/*
	 * Iterate through the tuple heading and create the variables
	 * by the same name.
	 */
	values = tuple->values ;
	for (hiter = Ral_TupleHeadingBegin(heading) ; hiter != hend ;
	    ++hiter, ++assigned) {
	    Ral_Attribute attr = *hiter ;
	    if (Tcl_SetVar2Ex(interp, attr->name, NULL, *values++,
		TCL_LEAVE_ERR_MSG) == NULL) {
		return TCL_ERROR ;
	    }
	}
    } else {
	/*
	 * If attribute variable name lists are given, then iterate
	 * through them, finding the attribute mentions and creating
	 * variables.
	 */
	for ( ; varc-- > 0 ; ++varv, ++assigned) {
	    int elemc ;
	    Tcl_Obj **elemv ;

	    /*
	     * Split into a list so we can see if there are one or
	     * two elements.
	     */
	    if (Tcl_ListObjGetElements(interp, *varv, &elemc, &elemv)
		!= TCL_OK) {
		return TCL_ERROR ;
	    }
	    if (elemc == 1 || elemc == 2) {
		const char *attrName = Tcl_GetString(*elemv) ;
		/*
		 * Look up the attribute in the tuple heading.
		 */
		int hindex = Ral_TupleHeadingIndexOf(tuple->heading, attrName) ;
		/*
		 * Determine the name of the variable. A little too cute.
		 */
		Tcl_Obj *varName = *(elemv + elemc - 1) ;

		if (hindex < 0) {
		    Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR,
			attrName) ;
		    Ral_InterpSetError(interp, errInfo) ;
		    return TCL_ERROR ;
		} else if (Tcl_ObjSetVar2(interp, varName, NULL,
		    tuple->values[hindex], TCL_LEAVE_ERR_MSG) == NULL) {
		    return TCL_ERROR ;
		}
	    } else {
		Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST,
		    *varv) ;
		Ral_InterpSetError(interp, errInfo) ;
		return TCL_ERROR ;
	    }
	}
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(assigned)) ;
    return TCL_OK ;
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
    /*
     * Removed the assertion that objPtr->typePtr == &Ral_TupleObjType.
     * When cleaning up the literal table, Tcl calls this function after
     * purposely setting the typePtr to NULL. Not sure why, but it does.
     */
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
    dupPtr->internalRep.otherValuePtr = dupTuple ;
    Ral_TupleReference(dupTuple) ;
    dupPtr->typePtr = &Ral_TupleObjType ;
}

static void
UpdateStringOfTuple(
    Tcl_Obj *objPtr)
{
    /*
     * N.B. that "Ral_TupleStringOf" allocates the string and that
     * pointer is simply transferred to the object.
     */
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
    Ral_ErrorInfo errInfo ;

    /*
     * The string representation of a Tuple is a two element list.
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdSetFromAny, Ral_OptNone) ;
    if (objc != 2) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_FORMAT_ERR, objPtr) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    heading = Ral_TupleHeadingNewFromObj(interp, objv[0], &errInfo) ;
    if (!heading) {
	return TCL_ERROR ;
    }
    return Ral_TupleObjConvert(heading, interp, objv[1], objPtr, &errInfo) ;
}
