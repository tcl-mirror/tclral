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

$RCSfile: ral_tuplecmd.c,v $
$Revision: 1.14 $
$Date: 2006/09/10 18:22:59 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <assert.h>
#include "tcl.h"
#include "ral_vector.h"
#include "ral_tuplecmd.h"
#include "ral_tupleobj.h"

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
static int TupleAssignCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleCreateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleDegreeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleEliminateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleEqualCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleExtendCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleExtractCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleGetCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleHeadingCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleProjectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleRenameCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleUnwrapCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleUpdateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int TupleWrapCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_tuplecmd.c,v $ $Revision: 1.14 $" ;

/*
FUNCTION DEFINITIONS
*/

/*
 * ======================================================================
 * Tuple Ensemble Command Function
 * ======================================================================
 */
int
tupleCmd(
    ClientData clientData,  /* Not used */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    static const struct cmdMap {
	const char *cmdName ;
	int (*const cmdFunc)(Tcl_Interp *, int, Tcl_Obj *const*) ;
    } cmdTable[] = {
	{"assign", TupleAssignCmd},
	{"create", TupleCreateCmd},
	{"degree", TupleDegreeCmd},
	{"eliminate", TupleEliminateCmd},
	{"equal", TupleEqualCmd},
	{"extend", TupleExtendCmd},
	{"extract", TupleExtractCmd},
	{"get", TupleGetCmd},
	{"heading", TupleHeadingCmd},
	{"project", TupleProjectCmd},
	{"rename", TupleRenameCmd},
	{"unwrap", TupleUnwrapCmd},
	{"update", TupleUpdateCmd},
	{"wrap", TupleWrapCmd},
	{NULL, NULL},
    } ;

    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObjStruct(interp, *(objv + 1), cmdTable,
	sizeof(cmdTable[0]), "subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    return cmdTable[index].cmdFunc(interp, objc, objv) ;
}

const char *
Ral_TupleCmdVersion(void)
{
    return rcsid ;
}

/*
 * ======================================================================
 * Tuple Sub-Command Functions
 * ======================================================================
 */

/*
 * tuple assign tupleValue ?attrName | attr-var-pair ... ?
 *
 * Assign the values of the tuple attributes to Tcl variables that are
 * either the same name as the attribute names or assign the given
 * attributes to the given variable names.
 *
 * Returns the degree of the tuple which is the number of variable assignment
 * made.
 */
static int
TupleAssignCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj = objv[2] ;
    Ral_Tuple tuple ;
    Ral_ErrorInfo errInfo ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleValue ?attrName | attr-var-pair ... ?") ;
	return TCL_ERROR ;
    }

    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdTuple, Ral_OptAssign) ;

    return Ral_TupleAssignToVars(tuple, interp, objc - 3, objv + 3, &errInfo) ;
}

/*
 * tuple create heading name-value-list
 *
 * Create a tuple given the heading and an alternating list of
 * attribute name / value pairs. The heading must be an alternating
 * list of attribute name / type pairs.
 *
 * Returns the tuple.
 */
static int
TupleCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_TupleHeading heading ;
    Ral_Tuple tuple ;
    Ral_ErrorInfo errInfo ;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "heading name-value-list") ;
	return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdTuple, Ral_OptCreate) ;
    heading = Ral_TupleHeadingNewFromObj(interp, *(objv + 2), &errInfo) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    tuple = Ral_TupleNew(heading) ;
    if (Ral_TupleSetFromObj(tuple, interp, *(objv + 3), &errInfo) != TCL_OK) {
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_TupleObjNew(tuple)) ;
    return TCL_OK ;
}

/*
 * tuple degree tupleValue
 */
static int
TupleDegreeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(Ral_TupleDegree(tuple))) ;
    return TCL_OK ;
}

/*
 * tuple eliminate tupleValue ?attr? ...
 */
static int
TupleEliminateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    Ral_IntVector elimList ;
    Ral_IntVector attrList ;
    Ral_TupleHeading elimHeading ;
    Ral_Tuple elimTuple ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue ?attr? ...") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    /*
     * Check just in case there are no attribute to eliminate. In that case
     * we just return the original argument, unmodified.
     */
    if (objc <= 0) {
	Tcl_SetObjResult(interp, tupleObj) ;
	return TCL_OK ;
    }
    objv += 3 ;
    /*
     * Check that attributes to eliminate actually belong to the tuple.
     * Build a mapping structure that determines if we delete the attribute.
     */
    elimList = Ral_IntVectorNewEmpty(objc) ;
    while (objc-- > 0) {
	const char *attrName = Tcl_GetString(*objv++) ;
	int attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;

	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptEliminate,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(elimList) ;
	    return TCL_ERROR ;
	}
	Ral_IntVectorSetAdd(elimList, attrIndex) ;
    }
    /*
     * Create the complement map which contains the attributes to retain.
     */
    attrList = Ral_IntVectorSetComplement(elimList,
	Ral_TupleHeadingSize(heading)) ;
    Ral_IntVectorDelete(elimList) ;

    elimHeading = Ral_TupleHeadingSubset(heading, attrList) ;
    elimTuple = Ral_TupleSubset(tuple, elimHeading, attrList) ;
    Ral_IntVectorDelete(attrList) ;

    Tcl_SetObjResult(interp, Ral_TupleObjNew(elimTuple)) ;
    return TCL_OK ;
}

/* tuple equal tuple1 tuple2 */
static int
TupleEqualCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *t1Obj = objv[2] ;
    Tcl_Obj *t2Obj = objv[3] ;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tuple1 tuple2") ;
	return TCL_ERROR ;
    }

    if (Tcl_ConvertToType(interp, t1Obj, &Ral_TupleObjType) != TCL_OK)
	return TCL_ERROR ;
    if (Tcl_ConvertToType(interp, t2Obj, &Ral_TupleObjType) != TCL_OK)
	return TCL_ERROR ;
    Tcl_SetObjResult(interp,
	Tcl_NewBooleanObj(Ral_TupleEqual(t1Obj->internalRep.otherValuePtr,
	t2Obj->internalRep.otherValuePtr))) ;

    return TCL_OK ;
}

/* tuple extend tupleValue ?name type value ... ? */
static int
TupleExtendCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_Tuple newTuple ;
    Ral_TupleHeading newHeading ;
    Ral_TupleIter newValues ;
    Ral_ErrorInfo errInfo ;

    if (objc < 3 || objc % 3 != 0) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue ?name type value ... ?") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }

    objc -= 3 ;
    /*
     * Test to make sure that we are actually going to extend the tuple.
     * If not, then return the original tuple object.
     */
    if (objc <= 0) {
	Tcl_SetObjResult(interp, tupleObj) ;
	return TCL_OK ;
    }
    objv += 3 ;
    tuple = tupleObj->internalRep.otherValuePtr ;
    /*
     * The heading for the new tuple is larger by the number of new
     * attributes given in the command. This is the number of arguments
     * divided by three.
     */
    newHeading = Ral_TupleHeadingExtend(tuple->heading, objc / 3) ;
    newTuple = Ral_TupleExtend(tuple, newHeading) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdTuple, Ral_OptExtend) ;
    /*
     * Add the new attributes to the new tuple.  The new attributes are tacked
     * on at the end of the attributes that came from the original tuple.
     */
    for (newValues = Ral_TupleEnd(newTuple) ; objc > 0 ; objc -= 3, objv += 3) {
	Ral_Attribute attr ;
	Ral_TupleHeadingIter hiter ;

	attr = Ral_AttributeNewFromObjs(interp, objv[0], objv[1], &errInfo) ;
	if (attr == NULL) {
	    goto errorOut ;
	}
	hiter = Ral_TupleHeadingPushBack(newHeading, attr) ;
	if (hiter == Ral_TupleHeadingEnd(newHeading)) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR,
		objv[0]) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    goto errorOut ;
	}

	if (Ral_AttributeConvertValueToType(interp, attr, objv[2], &errInfo)
	    != TCL_OK) {
	    goto errorOut ;
	}
	Tcl_IncrRefCount(*newValues++ = objv[2]) ;
    }

    Tcl_SetObjResult(interp, Ral_TupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut:
    Ral_TupleDelete(newTuple) ;
    return TCL_ERROR ;
}

static int
TupleExtractCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    const char *attrName ;
    int attrIndex ;
    Tcl_Obj *resultObj ;

    /* tuple extract tupleValue attr ?...? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue attr ?...?") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    objv += 3 ;
    if (objc < 2) {
	attrName = Tcl_GetString(*objv) ;
	attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptExtract,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
	resultObj = tuple->values[attrIndex] ;
    } else {
	resultObj = Tcl_NewListObj(0, NULL) ;
	while (objc-- > 0) {
	    attrName = Tcl_GetString(*objv++) ;
	    attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	    if (attrIndex < 0) {
		Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptExtract,
		    RAL_ERR_UNKNOWN_ATTR, attrName) ;
		goto errorOut ;
	    }
	    if (Tcl_ListObjAppendElement(interp, resultObj,
		    tuple->values[attrIndex]) != TCL_OK) {
		goto errorOut ;
	    }
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(resultObj) ;
    return TCL_ERROR ;
}

/* tuple get tupleValue */
static int
TupleGetCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter hiter ;
    Ral_TupleHeadingIter hend ;
    Tcl_Obj **values ;
    Tcl_Obj *resultObj ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;
    values = tuple->values ;
    resultObj = Tcl_NewListObj(0, NULL) ;

    hend = Ral_TupleHeadingEnd(heading) ;
    for (hiter = Ral_TupleHeadingBegin(heading) ; hiter != hend ; ++hiter) {
	Ral_Attribute attr = *hiter ;
	if (Tcl_ListObjAppendElement(interp, resultObj,
		Tcl_NewStringObj(attr->name, -1)) != TCL_OK ||
		Tcl_ListObjAppendElement(interp, resultObj, *values++)
		!= TCL_OK) {
	    Tcl_DecrRefCount(resultObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

/* tuple heading tupleValue */
static int
TupleHeadingCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    char *strRep ;
    Tcl_Obj *resultObj ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;

    strRep = Ral_TupleHeadingStringOf(tuple->heading) ;
    resultObj = Tcl_NewStringObj(strRep, -1) ;
    ckfree(strRep) ;

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

/* tuple project tupleValue ?attr? ... */
static int
TupleProjectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    Ral_IntVector attrList ;
    Ral_TupleHeading projHeading ;
    Ral_Tuple projTuple ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue ?attr? ...") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    objv += 3 ;

    attrList = Ral_IntVectorNewEmpty(objc) ;
    while (objc-- > 0) {
	const char *attrName = Tcl_GetString(*objv++) ;
	int attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;

	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptProject,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(attrList) ;
	    return TCL_ERROR ;
	}
	Ral_IntVectorSetAdd(attrList, attrIndex) ;
    }

    projHeading = Ral_TupleHeadingSubset(heading, attrList) ;
    projTuple = Ral_TupleSubset(tuple, projHeading, attrList) ;
    Ral_IntVectorDelete(attrList) ;

    Tcl_SetObjResult(interp, Ral_TupleObjNew(projTuple)) ;
    return TCL_OK ;
}

/* tuple rename tupleValue ?oldname newname ... ? */
static int
TupleRenameCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_Tuple newTuple ;
    Ral_TupleHeading newHeading ;
    Ral_TupleHeadingIter hend ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleValue ?oldname newname ... ?") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptRename,
	    RAL_ERR_BAD_PAIRS_LIST, "for oldname / newname arguments") ;
	return TCL_ERROR ;
    }

    newTuple = Ral_TupleDup(tuple) ;
    newHeading = newTuple->heading ;
    hend = Ral_TupleHeadingEnd(newHeading) ;
    for ( ; objc > 0 ; objc -= 2) {
	const char *oldAttrName = Tcl_GetString(*objv++) ;
	const char *newAttrName = Tcl_GetString(*objv++) ;
	Ral_TupleHeadingIter hiter ;

	hiter =Ral_TupleHeadingFind(newHeading, oldAttrName) ;
	if (hiter == hend) {
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptRename,
		RAL_ERR_UNKNOWN_ATTR, oldAttrName) ;
	    goto errorOut ;
	}

	hiter = Ral_TupleHeadingStore(newHeading, hiter,
	    Ral_AttributeRename(*hiter, newAttrName)) ;
	if (hiter == hend) {
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptRename,
		RAL_ERR_DUPLICATE_ATTR, newAttrName) ;
	    goto errorOut ;
	}
    }

    Tcl_SetObjResult(interp, Ral_TupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut:
    Ral_TupleDelete(newTuple) ;
    return TCL_ERROR ;
}


/* tuple unwrap tupleValue tupleAttribute */
static int
TupleUnwrapCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    Tcl_Obj **values ;
    const char *tupleAttrName ;
    Ral_TupleHeadingIter tupleAttrIter ;
    Ral_Attribute tupleAttr ;
    int tupleAttrIndex ;
    Tcl_Obj *tupleAttrValue ;
    Ral_Tuple unTuple ;
    Ral_TupleHeading newHeading ;
    Ral_Tuple newTuple ;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue tupleAttribute") ;
	return TCL_ERROR ;
    }

    /*
     * Obtain the tuple that is to be unwrapped.
     */
    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;
    values = tuple->values ;

    /*
     * Obtain the name of the attribute that is to be unwrapped.
     */
    tupleAttrName = Tcl_GetString(objv[3]) ;
    tupleAttrIter = Ral_TupleHeadingFind(heading, tupleAttrName) ;
    if (tupleAttrIter == Ral_TupleHeadingEnd(heading)) {
	Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptUnwrap,
	    RAL_ERR_UNKNOWN_ATTR, tupleAttrName) ;
	return TCL_ERROR ;
    }
    /*
     * This attribute must be of Tuple type.
     */
    tupleAttr = *tupleAttrIter ;
    if (tupleAttr->attrType != Tuple_Type) {
	Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptUnwrap,
	    RAL_ERR_NOT_A_TUPLE, tupleAttrName) ;
	return TCL_ERROR ;
    }
    /*
     * Convert the value and obtain the internal representation of
     * the tuple that is to be unwrapped.
     */
    tupleAttrIndex = tupleAttrIter - Ral_TupleHeadingBegin(heading) ;
    tupleAttrValue = tuple->values[tupleAttrIndex] ;
    if (Tcl_ConvertToType(interp, tupleAttrValue, &Ral_TupleObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    unTuple = tupleAttrValue->internalRep.otherValuePtr ;
    /*
     * The new tuple contain all the attributes of the old one minus the
     * attribute that is being unwrapped plus all the attributes contained in
     * the tuple to be unwrapped.
     */
    newHeading = Ral_TupleHeadingNew(
	Ral_TupleDegree(tuple) - 1 + Ral_TupleDegree(unTuple)) ;
    newTuple = Ral_TupleNew(newHeading) ;

    /*
     * We create the new unwrapped tuple in such a way that the attribute that
     * is being unwrapped is substituted with all of its components.  We don't
     * really have to do it this way since order does not matter, but it makes
     * it easier to look at and a bit less "surprising" when printed out.
     *
     * So the strategy is to copy all the attributes up to the one that is
     * unwrapped, add in all the attributes of the tuple valued attributes that
     * is unwrapped and then copy in all the remaining attributes.
     */
    if (!(Ral_TupleCopy(tuple, Ral_TupleHeadingBegin(heading), tupleAttrIter,
	    newTuple) &&
	Ral_TupleCopy(unTuple, Ral_TupleHeadingBegin(unTuple->heading),
	    Ral_TupleHeadingEnd(unTuple->heading), newTuple) &&
	Ral_TupleCopy(tuple, tupleAttrIter + 1, Ral_TupleHeadingEnd(heading),
	    newTuple))) {
	Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptUnwrap,
	    RAL_ERR_DUPLICATE_ATTR, "while unwrapping tuple") ;
	Ral_TupleDelete(newTuple) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_TupleObjNew(newTuple)) ;
    return TCL_OK ;
}

static int
TupleUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple tuple ;
    Ral_ErrorInfo errInfo ;

    /* tuple update tupleVar ?attr1 value1 attr2 value2 ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleValue ?attr1 value1 attr2 value2?") ;
	return TCL_ERROR ;
    }

    tupleObj = Tcl_ObjGetVar2(interp, objv[2], NULL, TCL_LEAVE_ERR_MSG) ;
    if (tupleObj == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_IsShared(tupleObj)) {
	Tcl_Obj *dupObj ;

	dupObj = Tcl_DuplicateObj(tupleObj) ;
	tupleObj = Tcl_ObjSetVar2(interp, objv[2], NULL, dupObj,
	    TCL_LEAVE_ERR_MSG) ;
	if (tupleObj == NULL) {
	    Tcl_DecrRefCount(dupObj) ;
	    return TCL_ERROR ;
	}
    }
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    assert(tuple->refCount == 1) ;

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptUpdate,
	    RAL_ERR_BAD_PAIRS_LIST,
	    "for attribute name / attribute value arguments") ;
	return TCL_ERROR ;
    }

    /*
     * Go through the attribute / value pairs updating the attribute values.
     */
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdTuple, Ral_OptUpdate) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	const char *attrName = Tcl_GetString(objv[0]) ;

	if (!Ral_TupleUpdateAttrValue(tuple, attrName, objv[1], &errInfo)) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, tupleObj) ;
    return TCL_OK ;
}

static int
TupleWrapCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Tcl_Obj *newAttrNameObj ;
    Tcl_Obj *oldAttrList ;
    Ral_Tuple tuple ;
    Ral_TupleHeading heading ;
    int degree ;
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_Tuple wrapTuple ;
    Ral_TupleHeading wrapHeading ;
    Ral_TupleHeadingIter hend ;
    Ral_TupleHeadingIter attrIter ;
    Ral_TupleHeading newHeading ;
    Ral_Tuple newTuple ;
    int i ;
    Tcl_Obj *wrapTupleObj ;
    const char *newAttrName ;
    Ral_Attribute newAttr ;
    Ral_TupleHeadingIter newAttrIter ;
    Ral_ErrorInfo errInfo ;

    /* tuple wrap tupleValue newAttr oldAttrList */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue newAttr oldAttrList") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    newAttrNameObj = *(objv + 3) ;
    oldAttrList = *(objv + 4) ;

    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_ListObjGetElements(interp, oldAttrList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;
    degree = Ral_TupleDegree(tuple) ;
    if (elemc > degree) {
	Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptWrap,
	    RAL_ERR_WRONG_NUM_ATTRS,
	    "attempt to wrap more attributes than exist in the tuple") ;
	return TCL_ERROR ;
    }

    /*
     * The new tuple valued attribute will have the same number of
     * attributes as elements in the "oldAttrList" argument.
     */
    wrapHeading = Ral_TupleHeadingNew(elemc) ;
    wrapTuple = Ral_TupleNew(wrapHeading) ;
    hend = Ral_TupleHeadingEnd(heading) ;
    for (i = 0 ; i < elemc ; ++i) {
	const char *attrName = Tcl_GetString(*elemv++) ;

	attrIter = Ral_TupleHeadingFind(heading, attrName) ;
	if (attrIter == hend) {
	    Ral_TupleDelete(wrapTuple) ;
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptUnwrap,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
	if (!Ral_TupleCopy(tuple, attrIter, attrIter + 1, wrapTuple)) {
	    Ral_InterpErrorInfo(interp, Ral_CmdTuple, Ral_OptUnwrap,
		RAL_ERR_DUPLICATE_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Compose the subtuple as an object.
     * Later it is added to the newly created tuple.
     */
    wrapTupleObj = Ral_TupleObjNew(wrapTuple) ;
    /*
     * The newly created tuple has the same number of attributes as the
     * old tuple minus the number that are to be wrapped plus one for
     * the new tuple attribute.
     */
    newHeading = Ral_TupleHeadingNew(degree - elemc + 1) ;
    newTuple = Ral_TupleNew(newHeading) ;

    for (attrIter = Ral_TupleHeadingBegin(heading) ; attrIter != hend ;
	++attrIter) {
	Ral_Attribute attr = *attrIter ;
	/*
	 * Only add the ones that are NOT in the old attribute list.
	 */
	if (Ral_TupleHeadingIndexOf(wrapHeading, attr->name) < 0 &&
	    !Ral_TupleCopy(tuple, attrIter, attrIter + 1, newTuple)) {
	    /*
	     * We should never fail here. After all, if all the attribute
	     * names were unique in the original tuple we should be able
	     * to copy them into an empty one.
	     */
	    Tcl_Panic("failed to copy attribute, \"%s\"", attr->name) ;
	}
    }
    /*
     * Now add the wrapped tuple. First add a new attribute and then store the
     * value.
     */
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdTuple, Ral_OptWrap) ;
    newAttrName = Tcl_GetString(newAttrNameObj) ;
    newAttr = Ral_AttributeNewTupleType(newAttrName, wrapHeading) ;
    newAttrIter = Ral_TupleHeadingPushBack(newHeading, newAttr) ;
    if (newAttrIter == Ral_TupleHeadingEnd(newHeading)) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_DUPLICATE_ATTR, newAttrName) ;
	goto errorOut ;
    }

    if (!Ral_TupleUpdateAttrValue(newTuple, newAttrName, wrapTupleObj,
	&errInfo)) {
	goto errorOut ;
    }
	
    Tcl_SetObjResult(interp, Ral_TupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut:
    Ral_TupleDelete(newTuple) ;
    Tcl_DecrRefCount(wrapTupleObj) ;
    Ral_InterpSetError(interp, &errInfo) ;
    return TCL_ERROR ;
}
