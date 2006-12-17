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

$RCSfile: ral_relationobj.c,v $
$Revision: 1.20 $
$Date: 2006/12/17 00:46:58 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_utils.h"
#include "ral_attribute.h"
#include "ral_relationobj.h"
#include "ral_relation.h"
#include "ral_tupleobj.h"
#include "ral_joinmap.h"

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
static int Ral_RelationFindJoinAttrs(Tcl_Interp *, Ral_Relation, Ral_Relation,
    Tcl_Obj *, Ral_JoinMap, Ral_ErrorInfo *) ;
static void FreeRelationInternalRep(Tcl_Obj *) ;
static void DupRelationInternalRep(Tcl_Obj *, Tcl_Obj *) ;
static void UpdateStringOfRelation(Tcl_Obj *) ;
static int SetRelationFromAny(Tcl_Interp *, Tcl_Obj *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

Tcl_ObjType Ral_RelationObjType = {
    relationKeyword,
    FreeRelationInternalRep,
    DupRelationInternalRep,
    UpdateStringOfRelation,
    SetRelationFromAny
} ;

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationobj.c,v $ $Revision: 1.20 $" ;

/*
FUNCTION DEFINITIONS
*/

Tcl_Obj *
Ral_RelationObjNew(
    Ral_Relation relation)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->internalRep.otherValuePtr = relation ;
    objPtr->typePtr = &Ral_RelationObjType ;
    objPtr->bytes = NULL ;
    objPtr->length = 0 ;
    return objPtr ;
}

int
Ral_RelationObjConvert(
    Ral_RelationHeading heading,
    Tcl_Interp *interp,
    Tcl_Obj *value,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    /*
     * Create the relation and set the values of the tuples from the object.
     * The object must be a list of tuple values.
     */
    Ral_Relation relation = Ral_RelationNew(heading) ;
    /*
     * Split the value into tuples.
     */
    if (Tcl_ListObjGetElements(interp, value, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Reserve the required storage so as not to reallocate during the
     * insertions.
     */
    Ral_RelationReserve(relation, elemc) ;

    while (elemc-- > 0) {
	if (Ral_RelationInsertTupleObj(relation, interp, *elemv++, errInfo)
	    != TCL_OK) {
	    Ral_RelationDelete(relation) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Discard the old internal representation.
     */
    if (objPtr->typePtr && objPtr->typePtr->freeIntRepProc) {
	objPtr->typePtr->freeIntRepProc(objPtr) ;
    }
    /*
     * Invalidate the string representation.  There are several string reps
     * that will map to the same relation and we want to force a new string rep
     * to be generated in order to obtain the canonical string form.
     */
    Tcl_InvalidateStringRep(objPtr) ;
    objPtr->length = 0 ;
    /*
     * Install the new internal representation.
     */
    objPtr->typePtr = &Ral_RelationObjType ;
    objPtr->internalRep.otherValuePtr = relation ;

    return TCL_OK ;
}


Ral_RelationHeading
Ral_RelationHeadingNewFromObjs(
    Tcl_Interp *interp,
    Tcl_Obj *headingObj,
    Tcl_Obj *identObj,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading tupleHeading ;
    Ral_RelationHeading heading ;
    int idc ;
    Tcl_Obj **idv ;
    int idNum ;

    /*
     * Create the tuple heading.
     */
    tupleHeading = Ral_TupleHeadingNewFromObj(interp, headingObj, errInfo) ;
    if (!tupleHeading) {
	return NULL ;
    }
    /*
     * The identifiers are also in a list.
     */
    if (Tcl_ListObjGetElements(interp, identObj, &idc, &idv) != TCL_OK) {
	Ral_TupleHeadingDelete(tupleHeading) ;
	return NULL ;
    }
    /*
     * If a relation has any attributes, then it must also have an identifier.
     */
    if (idc == 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_NO_IDENTIFIER, identObj) ;
	Ral_InterpSetError(interp, errInfo) ;
	Ral_TupleHeadingDelete(tupleHeading) ;
	return NULL ;
    }

    /*
     * We now know the number of identifers and can create the relation
     * heading.
     */
    heading = Ral_RelationHeadingNew(tupleHeading, idc) ;
    /*
     * Iterate over the identifier lists and insert them into the heading.
     */
    for (idNum = 0 ; idNum < idc ; ++idNum, ++idv) {
	if (Ral_RelationHeadingNewIdFromObj(interp, heading, idNum, *idv,
	    errInfo) != TCL_OK) {
	    Ral_RelationHeadingDelete(heading) ;
	    return NULL ;
	}
    }

    return heading ;
}

int
Ral_RelationHeadingNewIdFromObj(
    Tcl_Interp *interp,
    Ral_RelationHeading heading,
    int idNum,
    Tcl_Obj *identObj,
    Ral_ErrorInfo *errInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_IntVector id ;

    if (Tcl_ListObjGetElements(interp, identObj, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Vector to hold the attribute indices that constitute the
     * identifier.
     */
    id = Ral_IntVectorNewEmpty(elemc) ;
    /*
     * Find the attribute in the tuple heading and build up a
     * vector to install in the relation heading.
     */
    while (elemc-- > 0) {
	const char *attrName = Tcl_GetString(*elemv++) ;
	int index = Ral_TupleHeadingIndexOf(heading->tupleHeading, attrName) ;

	if (index < 0) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    Ral_InterpSetError(interp, errInfo) ;
	    Ral_IntVectorDelete(id) ;
	    return TCL_ERROR ;
	}
	/*
	 * The indices in an identifier must form a set, i.e. you
	 * cannot have a duplicate attribute in a list of attributes
	 * that is intended to be an identifier of a relation.
	 */
	if (!Ral_IntVectorSetAdd(id, index)) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUP_ATTR_IN_ID,
		attrName) ;
	    Ral_InterpSetError(interp, errInfo) ;
	    Ral_IntVectorDelete(id) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Add the set of attribute indices as an identifier. Adding
     * the vector checks that we do not have a subset dependency.
     */
    if (!Ral_RelationHeadingAddIdentifier(heading, idNum, id)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_IDENTIFIER_SUBSET, identObj) ;
	Ral_InterpSetError(interp, errInfo) ;
	Ral_IntVectorDelete(id) ;
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

int
Ral_RelationInsertTupleObj(
    Ral_Relation relation,
    Tcl_Interp *interp,
    Tcl_Obj *tupleObj,
    Ral_ErrorInfo *errInfo)
{
    Ral_Tuple tuple ;

    /*
     * Make the new tuple refer to the heading contained in the relation.
     */
    tuple = Ral_TupleNew(relation->heading->tupleHeading) ;
    /*
     * Set the values of the attributes from the list of attribute / value
     * pairs.
     */
    if (Ral_TupleSetFromObj(tuple, interp, tupleObj, errInfo) != TCL_OK) {
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }
    /*
     * Insert the tuple into the relation.
     */
    if (!Ral_RelationPushBack(relation, tuple, NULL)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_DUPLICATE_TUPLE, tupleObj) ;
	Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

int
Ral_RelationObjParseJoinArgs(
    Tcl_Interp *interp,
    int *objcPtr,
    Tcl_Obj *const**objvPtr,
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap joinMap,
    Ral_ErrorInfo *errInfo)
{
    int findCommon = 1 ;
    int objc = *objcPtr ;
    Tcl_Obj *const*objv = *objvPtr ;

    if (objc) {
	const char *nextArg = Tcl_GetString(*objv) ;
	if (strcmp(nextArg, "-using") == 0) {
	    findCommon = 0 ;
	    /*
	     * Join arguments specified.
	     */
	    if (Ral_RelationFindJoinAttrs(interp, r1, r2, *(objv + 1), joinMap,
		errInfo) != TCL_OK) {
		return TCL_ERROR ;
	    }
	    *objcPtr -= 2 ;
	    *objvPtr += 2 ;
	}
    }
    if (findCommon) {
	/*
	 * Join on common attributes.
	 */
	Ral_TupleHeadingCommonAttributes(
	    r1->heading->tupleHeading, r2->heading->tupleHeading, joinMap) ;
    }

    return TCL_OK ;
}

/*
 * Returns a tuple that has values set for the given attributes.
 * The attributes are examined to make sure that they form an identifier
 * so that the resulting tuple can be used as a key to look up a particular
 * tuple in the relation.
 * Caller must delete the returned tuple.
 */
Ral_Tuple
Ral_RelationObjKeyTuple(
    Tcl_Interp *interp,
    Ral_Relation relation,
    int objc,
    Tcl_Obj *const*objv,
    int *idRef,
    Ral_ErrorInfo *errInfo)
{
    Ral_RelationHeading heading = relation->heading ;
    Ral_TupleHeading tupleHeading = heading->tupleHeading ;
    Ral_IntVector id ;
    Ral_Tuple key ;
    int idNum ;

    if (objc % 2 != 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, *objv) ;
	Ral_InterpSetError(interp, errInfo) ;
	return NULL ;
    }
    /*
     * Iterate through the name/value list and construct an identifier
     * vector from the attribute names and a key tuple from the corresponding
     * values.
     */
    id = Ral_IntVectorNewEmpty(objc / 2) ;
    key = Ral_TupleNew(tupleHeading) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	const char *attrName = Tcl_GetString(*objv) ;
	int attrIndex = Ral_TupleHeadingIndexOf(tupleHeading, attrName) ;

	if (attrIndex < 0) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    goto error_out ;
	}
	Ral_IntVectorPushBack(id, attrIndex) ;

	if (!Ral_TupleUpdateAttrValue(key, attrName, *(objv + 1), errInfo)) {
	    goto error_out ;
	}
    }

    /*
     * Check if the attributes given do constitute an identifier.
     */
    idNum = Ral_RelationHeadingFindIdentifier(heading, id) ;
    if (idNum < 0) {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_NOT_AN_IDENTIFIER,
	    "during identifier construction operation") ;
	goto error_out ;
    } else if (idRef) {
	*idRef = idNum ;
    }
    Ral_IntVectorDelete(id) ;

    return key ;

error_out:
    Ral_InterpSetError(interp, errInfo) ;
    Ral_IntVectorDelete(id) ;
    Ral_TupleDelete(key) ;
    return NULL ;
}

/*
 * Update a tuple in a relationship by running a script and
 * retrieving the updated value of a tuple.
 */
int
Ral_RelationObjUpdateTuple(
    Tcl_Interp *interp,		/* interpreter */
    Tcl_Obj *tupleVarNameObj,	/* name of the tuple variable */
    Tcl_Obj *scriptObj,		/* script to run */
    Ral_Relation relation,	/* relation to update */
    Ral_RelationIter tupleIter, /* tuple withing the relation to update */
    Ral_CmdOption cmdOpt)	/* which command is calling -- for errors */
{
    int result ;
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;

    /*
     * Evaluate the script.
     */
    result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
    if (result == TCL_ERROR) {
	return result ;
    }
    /*
     * Fetch the value of the variable. It could be different now
     * that the update has been performed. Once we get the new
     * tuple value, we can use it to update the relvar.
     */
    tupleObj = Tcl_ObjGetVar2(interp, tupleVarNameObj, NULL,
	TCL_LEAVE_ERR_MSG) ;
    if (tupleObj == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    assert(tupleObj->typePtr == &Ral_TupleObjType) ;
    tuple = tupleObj->internalRep.otherValuePtr ;
    if (!Ral_RelationUpdate(relation, tupleIter, tuple, NULL)) {
	char *tupleStr = Ral_TupleValueStringOf(tuple) ;
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, cmdOpt,
	    RAL_ERR_DUPLICATE_TUPLE, tupleStr) ;
	ckfree(tupleStr) ;
	result = TCL_ERROR ;
    }
    return result ;
}

const char *
Ral_RelationObjVersion(void)
{
    return rcsid ;
}
/*
 * ======================================================================
 * PRIVATE FUNCTIONS
 * ======================================================================
 */

static int
Ral_RelationFindJoinAttrs(
    Tcl_Interp *interp,
    Ral_Relation r1,
    Ral_Relation r2,
    Tcl_Obj *attrList,
    Ral_JoinMap joinMap,
    Ral_ErrorInfo *errInfo)
{
    Ral_TupleHeading r1TupleHeading = r1->heading->tupleHeading ;
    Ral_TupleHeading r2TupleHeading = r2->heading->tupleHeading ;
    int objc ;
    Tcl_Obj **objv ;

    if (Tcl_ListObjGetElements(interp, attrList, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc % 2 != 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, attrList) ;
	goto errorOut ;
    }
    Ral_JoinMapAttrReserve(joinMap, objc / 2) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	int r1Index = Ral_TupleHeadingIndexOf(r1TupleHeading,
	    Tcl_GetString(*objv)) ;
	int r2Index = Ral_TupleHeadingIndexOf(r2TupleHeading,
	    Tcl_GetString(*(objv + 1))) ;
	if (r1Index < 0) {
	    Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_UNKNOWN_ATTR, *objv) ;
	    goto errorOut ;
	}
	if (r2Index < 0) {
	    Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_UNKNOWN_ATTR,
		*(objv + 1)) ;
	    goto errorOut ;
	}
	Ral_JoinMapAddAttrMapping(joinMap, r1Index, r2Index) ;
    }

    return TCL_OK ;

errorOut:
    Ral_InterpSetError(interp, errInfo) ;
    return TCL_ERROR ;
}

/*
 * ======================================================================
 * Functions to Support the Relation type
 * ======================================================================
 */

static void
FreeRelationInternalRep(
    Tcl_Obj *objPtr)
{
    assert(objPtr->typePtr == &Ral_RelationObjType) ;
    Ral_RelationDelete(objPtr->internalRep.otherValuePtr) ;
    objPtr->typePtr = objPtr->internalRep.otherValuePtr = NULL ;
}

static void
DupRelationInternalRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    Ral_Relation srcRelation ;
    Ral_Relation dupRelation ;

    assert(srcPtr->typePtr == &Ral_RelationObjType) ;
    srcRelation = srcPtr->internalRep.otherValuePtr ;

    dupRelation = Ral_RelationDup(srcRelation) ;
    if (dupRelation) {
	dupPtr->internalRep.otherValuePtr = dupRelation ;
	dupPtr->typePtr = &Ral_RelationObjType ;
    }
}

static void
UpdateStringOfRelation(
    Tcl_Obj *objPtr)
{
    objPtr->bytes = Ral_RelationStringOf(objPtr->internalRep.otherValuePtr) ;
    objPtr->length = strlen(objPtr->bytes) ;
}

static int
SetRelationFromAny(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_RelationHeading heading ;
    Ral_ErrorInfo errInfo ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdSetFromAny, Ral_OptNone) ;
    if (objc != 4) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_FORMAT_ERR, objPtr) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_RelationObjType.name, Tcl_GetString(*objv)) != 0) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_BAD_KEYWORD, *objv) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Create the heading from the external representation.
     */
    heading = Ral_RelationHeadingNewFromObjs(interp, objv[1], objv[2],
	&errInfo) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    return Ral_RelationObjConvert(heading, interp, objv[3], objPtr, &errInfo) ;
}
