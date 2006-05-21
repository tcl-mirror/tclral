/*
This software is copyrighted 2006 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_relvarcmd.c,v $
$Revision: 1.6 $
$Date: 2006/05/21 04:22:00 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <string.h>
#include "tcl.h"
#include "ral_relvarcmd.h"
#include "ral_relvarobj.h"
#include "ral_relvar.h"
#include "ral_relationobj.h"
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
static int RelvarAssociationCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarConstraintCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarCreateCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarDeleteCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarDeleteOneCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarDestroyCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarEvalCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarInsertCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarNamesCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarPartitionCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarSetCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUpdateCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUpdateOneCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relvarcmd.c,v $ $Revision: 1.6 $" ;

/*
FUNCTION DEFINITIONS
*/

int
relvarCmd(
    ClientData clientData,  /* Contains an interpreter id */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    static const struct cmdMap {
	const char *cmdName ;
	int (*cmdFunc)(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
    } cmdTable[] = {
	{"association", RelvarAssociationCmd},
	{"constraint", RelvarConstraintCmd},
	{"create", RelvarCreateCmd},
	{"delete", RelvarDeleteCmd},
	{"deleteone", RelvarDeleteOneCmd},
	{"destroy", RelvarDestroyCmd},
	{"eval", RelvarEvalCmd},
	{"insert", RelvarInsertCmd},
	{"names", RelvarNamesCmd},
	{"partition", RelvarPartitionCmd},
	{"set", RelvarSetCmd},
	{"update", RelvarUpdateCmd},
	{"updateone", RelvarUpdateOneCmd},
	{NULL, NULL},
    } ;
    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObjStruct(interp, *(objv + 1), cmdTable,
	sizeof(struct cmdMap), "subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    return cmdTable[index].cmdFunc(interp, objc, objv,
	(Ral_RelvarInfo)clientData) ;
}

const char *
Ral_RelvarCmdVersion(void)
{
    return rcsid ;
}
/*
 * ======================================================================
 * Relvar Sub-Command Functions
 * ======================================================================
 */

static int
RelvarAssociationCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    /* relvar association name relvar1 attr-list1 spec1
     * relvar2 attr-list2 spec2 */
    if (objc != 9) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "name relvar1 attr-list1 spec1 relvar2 attr-list2 spec2") ;
	return TCL_ERROR ;
    }
    Tcl_ResetResult(interp) ;
    /*
     * Creating an association is an implicit transaction as each
     * relvar participating in the association is treated as modified.
     */
    return Ral_RelvarObjCreateAssoc(interp, objv + 2, rInfo) ;
}

static int
RelvarConstraintCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    enum ConstraintCmdType {
	ConstraintDelete,
	ConstraintInfo,
	ConstraintNames,
    } ;
    static const char *constraintCmds[] = {
	"delete",
	"info",
	"names",
	NULL
    } ;
    int result = TCL_ERROR ;
    int index ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "create | delete | info | names ?args?") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObj(interp, (Tcl_Obj *)objv[2], constraintCmds,
	"constraint subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    switch ((enum ConstraintCmdType)index) {
    /* relvar constraint delete ?name1 name2 ...? */
    case ConstraintDelete:
	objc -= 3 ;
	objv += 3 ;
	while (objc-- > 0) {
	    result = Ral_RelvarObjConstraintDelete(interp,
		Tcl_GetString(*objv++), rInfo) ;
	    if (result != TCL_OK) {
		break ;
	    }
	}
	return result ;

    /* relvar constraint info name */
    case ConstraintInfo:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "names") ;
	    return TCL_ERROR ;
	}
	result = Ral_RelvarObjConstraintInfo(interp, objv[3], rInfo) ;
	break ;

    /* relvar constraint names ?pattern? */
    case ConstraintNames:
    {
	const char *pattern ;

	if (objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "names ?pattern?") ;
	    return TCL_ERROR ;
	}
	pattern = objc == 3 ? "*" : Tcl_GetString(objv[3]) ;
	result = Ral_RelvarObjConstraintNames(interp, pattern, rInfo) ;
    }
	break ;

    default:
	Tcl_Panic("Unknown association command type, %d", index) ;
    }

    return result ;
}

static int
RelvarCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_RelationHeading heading ;

    /* relvar create relvarName heading */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName heading") ;
	return TCL_ERROR ;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc != 3) {
	Ral_RelationObjSetError(interp, REL_FORMAT_ERR,
	    Tcl_GetString(objv[3])) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_RelationObjType.name, Tcl_GetString(*elemv)) != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_KEYWORD,
	    Tcl_GetString(*elemv)) ;
	return TCL_ERROR ;
    }
    /*
     * Create the heading from the external representation.
     */
    heading = Ral_RelationHeadingNewFromObjs(interp, elemv[1], elemv[2]) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    return Ral_RelvarObjNew(interp, rInfo, Tcl_GetString(objv[2]), heading) ;
}

static int
RelvarDeleteCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    Tcl_Obj *tupleNameObj ;
    Tcl_Obj *exprObj ;
    Ral_RelationIter rIter ;
    int deleted = 0 ;
    int result = TCL_OK ;

    /* relvar delete relvarName tupleVarName expr */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName tupleVarName expr") ;
	return TCL_ERROR ;
    }
    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

    Ral_RelvarStartCommand(rInfo, relvar) ;
    for (rIter = Ral_RelationBegin(relation) ;
	    rIter != Ral_RelationEnd(relation) ;) {
	int boolValue ;
	Tcl_Obj *tupleObj ;

	tupleObj = Ral_TupleObjNew(*rIter) ;
	if (Tcl_ObjSetVar2(interp, tupleNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
	    result = TCL_ERROR ;
	    break ;
	}
	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
	if (boolValue) {
	    rIter = Ral_RelationErase(relation, rIter, rIter + 1) ;
	    ++deleted ;
	} else {
	    ++rIter ;
	}
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;
    if (deleted) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    result = Ral_RelvarObjEndCmd(interp, rInfo, 0) ;
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(deleted)) ;
    }
    return result ;
}

static int
RelvarDeleteOneCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    Ral_Tuple key ;
    int idNum ;
    int deleted ;
    int result ;

    /* relvar deleteone relvarName ?attrName1 value1 attrName2 value2 ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName relvarName "
	    "?attrName1 value1 attrName2 value2 ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    key = Ral_RelationObjKeyTuple(interp, relation, objc, objv, &idNum) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }
    Ral_RelvarStartCommand(rInfo, relvar) ;
    deleted = Ral_RelationEraseTuple(relation, idNum, key, NULL) ;
    Ral_TupleDelete(key) ;

    if (deleted) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    result = Ral_RelvarObjEndCmd(interp, rInfo, 0) ;
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(deleted)) ;
    }
    return result ;
}

static int
RelvarDestroyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    int result ;

    /* relvar destroy ?relvar1 relvar2 ...? */
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "?relvar1 relvar2 ...?") ;
	return TCL_ERROR ;
    }

    objc -= 2 ;
    objv += 2 ;

    while (objc-- > 0) {
	result = Ral_RelvarObjDelete(interp, rInfo, *objv++) ;
	if (result != TCL_OK) {
	    break ;
	}
    }

    return result ;
}

static int
RelvarEvalCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Tcl_Obj *scriptObj ;
    int result ;

    /* relvar eval script */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "script") ;
	return TCL_ERROR ;
    }
    scriptObj = objv[2] ;

    Ral_RelvarStartTransaction(rInfo, 0) ;

    result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
    if (result == TCL_ERROR) {
	static const char msgfmt[] =
	    "\n    (\"relvar eval\" body line %d)" ;
	char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE] ;
	sprintf(msg, msgfmt, interp->errorLine) ;
	Tcl_AddObjErrorInfo(interp, msg, -1) ;
    }

    return Ral_RelvarObjEndTrans(interp, rInfo, result == TCL_ERROR) == TCL_OK ?
	result : TCL_ERROR ;
}

static int
RelvarInsertCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    int inserted = 0 ;
    int result ;

    /* relvar insert relvarName ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    Ral_RelvarStartCommand(rInfo, relvar) ;
    Ral_RelationReserve(relation, objc) ;
    while (objc-- > 0) {
	if (Ral_RelationInsertTupleObj(relation, interp, *objv++) != TCL_OK) {
	    return Ral_RelvarObjEndCmd(interp, rInfo, 1) ;
	}
	++inserted ;
    }

    if (inserted) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    result = Ral_RelvarObjEndCmd(interp, rInfo, 0) ;
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, relvar->relObj) ;
    }
    return result ;
}

static int
RelvarNamesCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    const char *pattern ;
    Tcl_Obj *nameList ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;
    Tcl_HashTable *relvarMap = &rInfo->relvars ;

    /* relvar names ?pattern? */
    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "?pattern?") ;
	return TCL_ERROR ;
    }

    pattern = objc == 3 ? Tcl_GetString(objv[2]) : NULL ;
    nameList = Tcl_NewListObj(0, NULL) ;

    for (entry = Tcl_FirstHashEntry(relvarMap, &search) ; entry ;
	entry = Tcl_NextHashEntry(&search)) {
	const char *relvarName ;

	relvarName = (const char *)Tcl_GetHashKey(relvarMap, entry) ;
	if (pattern && !Tcl_StringMatch(relvarName, pattern)) {
	    continue ;
	}
	if (Tcl_ListObjAppendElement(interp, nameList,
	    Tcl_NewStringObj(relvarName, -1)) != TCL_OK) {
	    Tcl_DecrRefCount(nameList) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, nameList) ;
    return TCL_OK ;
}

static int
RelvarPartitionCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    /*
     * relvar partition name super super-attrs sub1 sub1-attrs
     * ?sub2 sub2-attrs sub3 sub3-attrs ...?
     */
    if (objc < 7 || (objc % 2) == 0) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "name super super-attrs sub1 sub1-attrs"
	     " ?sub2 sub2-attrs sub3 sub3-attrs ...?") ;
	return TCL_ERROR ;
    }
    Tcl_ResetResult(interp) ;
    /*
     * Creating a partition is an implicit transaction as each
     * relvar participating in the association is treated as modified.
     */
    return Ral_RelvarObjCreatePartition(interp, objc - 2, objv + 2, rInfo) ;
}

static int
RelvarSetCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relvalue ;
    int result = TCL_OK ;

    /* relvar set relvar relationValue */
    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvar ?relationValue?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relvalue = relvar->relObj->internalRep.otherValuePtr ;

    if (objc > 3) {
	Tcl_Obj *valueObj ;
	Ral_Relation relation ;

	valueObj = objv[3] ;
	if (Tcl_ConvertToType(interp, valueObj, &Ral_RelationObjType)
	    != TCL_OK) {
	    return TCL_ERROR ;
	}
	relation = valueObj->internalRep.otherValuePtr ;

	if (!Ral_RelationHeadingEqual(relvalue->heading, relation->heading)) {
	    char *headingStr = Ral_RelationHeadingStringOf(relation->heading) ;
	    Ral_RelvarObjSetError(interp, RELVAR_HEADING_MISMATCH, headingStr) ;
	    ckfree(headingStr) ;
	    return TCL_ERROR ;
	}

	Ral_RelvarStartCommand(rInfo, relvar) ;
	Ral_RelvarSetRelation(relvar, relation) ;
	Tcl_InvalidateStringRep(relvar->relObj) ;
	result = Ral_RelvarObjEndCmd(interp, rInfo, 0) ;
    }

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, relvar->relObj) ;
    }
    return result ;
}

static int
RelvarUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    Tcl_Obj *tupleVarNameObj ;
    Tcl_Obj *exprObj ;
    Tcl_Obj *scriptObj ;
    int updated = 0 ;
    Ral_RelationIter rEnd ;
    Ral_RelationIter rIter ;
    int result = TCL_OK ;

    /* relvar update relvarName tupleVarName expr script */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName tupleVarName expr script") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    Tcl_IncrRefCount(tupleVarNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;
    Tcl_IncrRefCount(scriptObj = objv[5]) ;

    Ral_RelvarStartCommand(rInfo, relvar) ;

    rEnd = Ral_RelationEnd(relation) ;
    for (rIter = Ral_RelationBegin(relation) ; rIter != rEnd ; ++rIter) {
	Tcl_Obj *tupleObj ;
	int boolValue ;
	Ral_Tuple tuple ;

	/*
	 * Clone the tuple into a Tcl object and store it into the
	 * given variable.
	 */
	tupleObj = Ral_TupleObjNew(Ral_TupleDup(*rIter)) ;
	if (Tcl_ObjSetVar2(interp, tupleVarNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
	    result = TCL_ERROR ;
	    break ;
	}
	/*
	 * Evaluate the expression to see if this is a tuple that is
	 * to be updated.
	 */
	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
	if (!boolValue) {
	    continue ;
	}
	/*
	 * Evaluate the script.
	 */
	result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
	if (result != TCL_OK) {
	    if (result == TCL_CONTINUE) {
		result = TCL_OK ;
	    } else if (result == TCL_BREAK) {
		result = TCL_OK ;
		break ;
	    }else if (result == TCL_ERROR) {
		static const char msgfmt[] =
		    "\n    (\"relvar eval\" body line %d)" ;
		char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE] ;
		sprintf(msg, msgfmt, interp->errorLine) ;
		Tcl_AddObjErrorInfo(interp, msg, -1) ;
		break ;
	    } else {
		break ;
	    }
	}
	++updated ;
	/*
	 * Fetch the value of the variable. It could be different now
	 * that the update has been performed. Once we get the new
	 * tuple value, we can use it to update the relvar.
	 */
	tupleObj = Tcl_ObjGetVar2(interp, tupleVarNameObj, NULL,
	    TCL_LEAVE_ERR_MSG) ;
	if (tupleObj == NULL) {
	    result = TCL_ERROR ;
	    break ;
	}
	result = Tcl_ConvertToType(interp, tupleObj, &Ral_TupleObjType) ;
	if (result != TCL_OK) {
	    break ;
	}
	tuple = tupleObj->internalRep.otherValuePtr ;
	if (!Ral_RelationUpdate(relation, rIter, tuple, NULL)) {
	    char *tupleStr = Ral_TupleValueStringOf(tuple) ;
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_TUPLE, tupleStr) ;
	    ckfree(tupleStr) ;
	    result = TCL_ERROR ;
	    break ;
	}
    }

    if (updated) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;

    Tcl_UnsetVar(interp, Tcl_GetString(tupleVarNameObj), 0) ;
    Tcl_DecrRefCount(scriptObj) ;
    Tcl_DecrRefCount(tupleVarNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(updated)) ;
    }

    return result ;
}

static int
RelvarUpdateOneCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_Tuple key ;
    int idNum ;
    Ral_RelationIter found ;
    int result = TCL_OK ;
    int updated = 0 ;

    /* relvar updateone relvarName idValueList nameValueList */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName idValueList nameValueList") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    if (Tcl_ListObjGetElements(interp, objv[3], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }

    key = Ral_RelationObjKeyTuple(interp, relation, elemc, elemv, &idNum) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }
    found = Ral_RelationFindKey(relation, idNum, key, NULL) ;
    Ral_TupleDelete(key) ;

    if (found != Ral_RelationEnd(relation)) {
	int failed = 0 ;
	Ral_RelvarStartCommand(rInfo, relvar) ;

	if (Ral_RelationObjUpdateTuple(interp, objv[4], relation, found)
	    != TCL_OK) {
	    failed = 1 ;
	} else {
	    Tcl_InvalidateStringRep(relvar->relObj) ;
	}
	result = Ral_RelvarObjEndCmd(interp, rInfo, failed) ;
	updated = 1 ;
    }
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(updated)) ;
    }
    return result ;
}
