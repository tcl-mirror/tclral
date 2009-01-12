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
$Revision: 1.24.2.1 $
$Date: 2009/01/12 00:45:36 $
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
    ral_relationTypeName,
    FreeRelationInternalRep,
    DupRelationInternalRep,
    UpdateStringOfRelation,
    SetRelationFromAny
} ;

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationobj.c,v $ $Revision: 1.24.2.1 $" ;

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
    Ral_TupleHeading heading,
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
	if (Ral_RelationInsertTupleValue(relation, interp, *elemv++, errInfo)
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
     * Install the new internal representation.
     */
    objPtr->typePtr = &Ral_RelationObjType ;
    objPtr->internalRep.otherValuePtr = relation ;

    return TCL_OK ;
}

/*
 * Insert a tuple value body consisting of a list of attribute name / attribute
 * value into a relation.
 */
int
Ral_RelationInsertTupleValue(
    Ral_Relation relation,
    Tcl_Interp *interp,
    Tcl_Obj *tupleValue,
    Ral_ErrorInfo *errInfo)
{
    Ral_Tuple tuple ;

    /*
     * Make the new tuple refer to the heading contained in the relation.
     */
    tuple = Ral_TupleNew(relation->heading) ;
    /*
     * Set the values of the attributes from the list of attribute / value
     * pairs.
     */
    if (Ral_TupleSetFromObj(tuple, interp, tupleValue, errInfo) != TCL_OK) {
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }
    /*
     * Insert the tuple into the relation.
     */
    if (!Ral_RelationPushBack(relation, tuple, NULL)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_DUPLICATE_TUPLE, tupleValue) ;
	Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

/*
 * Insert a tuple contained in a Tcl object into a relation.
 */
int
Ral_RelationInsertTupleObj(
    Ral_Relation relation,
    Tcl_Interp *interp,
    Tcl_Obj *tupleObj,
    Ral_ErrorInfo *errInfo)
{
    Ral_Tuple tuple ;
    Ral_IntVector orderMap ;
    int result = TCL_OK ;

    /*
     * Convert to a tuple type.
     */
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    assert(tupleObj->typePtr == &Ral_TupleObjType) ;
    tuple = tupleObj->internalRep.otherValuePtr ;
    orderMap = Ral_TupleHeadingNewOrderMap(relation->heading, tuple->heading) ;
    if (!Ral_RelationPushBack(relation, tuple, orderMap)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_DUPLICATE_TUPLE, tupleObj) ;
	Ral_InterpSetError(interp, errInfo) ;
	result = TCL_ERROR ;
    }
    Ral_IntVectorDelete(orderMap) ;

    return result ;
}

/*
 * Update a tuple value contained in a Tcl object into a relation.
 */
int
Ral_RelationUpdateTupleObj(
    Ral_Relation relation,
    Ral_RelationIter where,
    Tcl_Interp *interp,
    Tcl_Obj *tupleObj,
    Ral_ErrorInfo *errInfo)
{
    Ral_Tuple tuple ;
    Ral_IntVector orderMap ;
    int result = TCL_OK ;

    /*
     * Convert to a tuple type.
     */
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    assert(tupleObj->typePtr == &Ral_TupleObjType) ;
    tuple = tupleObj->internalRep.otherValuePtr ;
    orderMap = Ral_TupleHeadingNewOrderMap(relation->heading, tuple->heading) ;
    if (!Ral_RelationUpdate(relation, where, tuple, orderMap)) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_DUPLICATE_TUPLE, tupleObj) ;
	Ral_InterpSetError(interp, errInfo) ;
	result = TCL_ERROR ;
    }
    Ral_IntVectorDelete(orderMap) ;

    return result ;
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
	Ral_TupleHeadingCommonAttributes(r1->heading, r2->heading, joinMap) ;
    }

    return TCL_OK ;
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
    Ral_TupleHeading r1TupleHeading = r1->heading ;
    Ral_TupleHeading r2TupleHeading = r2->heading ;
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
    /*
     * Removed the assertion that objPtr->typePtr == &Ral_RelationObjType.
     * When cleaning up the literal table, Tcl calls this function after
     * purposely setting the typePtr to NULL. Not sure why, but it does.
     */
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
    Ral_TupleHeading heading ;
    Ral_ErrorInfo errInfo ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdSetFromAny, Ral_OptNone) ;
    if (objc != 2) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_FORMAT_ERR, objPtr) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Create the heading from the external representation.
     */
    heading = Ral_TupleHeadingNewFromObj(interp, objv[0], &errInfo) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    return Ral_RelationObjConvert(heading, interp, objv[1], objPtr, &errInfo) ;
}
