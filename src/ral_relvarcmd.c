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
$Revision: 1.32 $
$Date: 2008/04/15 01:10:53 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <string.h>
#include <assert.h>
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
static int RelvarCorrelationCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarCreateCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarDeleteCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarDeleteOneCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarEvalCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarInsertCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarIntersectCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarMinusCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarNamesCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarPartitionCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarPathCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarSetCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarTraceCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarTransactionCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarUnionCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUnsetCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
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
static const char rcsid[] = "@(#) $RCSfile: ral_relvarcmd.c,v $ $Revision: 1.32 $" ;

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
	char const *cmdName ;
	int (*cmdFunc)(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
    } cmdTable[] = {
	{"association", RelvarAssociationCmd},
	{"constraint", RelvarConstraintCmd},
	{"correlation", RelvarCorrelationCmd},
	{"create", RelvarCreateCmd},
	{"delete", RelvarDeleteCmd},
	{"deleteone", RelvarDeleteOneCmd},
	{"eval", RelvarEvalCmd},
	{"insert", RelvarInsertCmd},
	{"intersect", RelvarIntersectCmd},
	{"minus", RelvarMinusCmd},
	{"names", RelvarNamesCmd},
	{"partition", RelvarPartitionCmd},
	{"path", RelvarPathCmd},
	{"set", RelvarSetCmd},
	{"trace", RelvarTraceCmd},
	{"transaction", RelvarTransactionCmd},
	{"union", RelvarUnionCmd},
	{"unset", RelvarUnsetCmd},
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

char const *
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
	    "name refrngRelvar refrngAttrList refToSpec "
	    "refToRelvar refToAttrList refrngSpec") ;
	return TCL_ERROR ;
    }
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
	ConstraintMember,
	ConstraintPath,
    } ;
    static char const *constraintCmds[] = {
	"delete",
	"info",
	"names",
	"member",
	"path",
	NULL
    } ;
    int result = TCL_OK ;
    int index ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "delete | info | names ?args? | member <relvar> | path <name>") ;
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
		return result ;
	    }
	}
	break ;

    /* relvar constraint info name */
    case ConstraintInfo:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name") ;
	    return TCL_ERROR ;
	}
	result = Ral_RelvarObjConstraintInfo(interp, objv[3], rInfo) ;
	break ;

    /* relvar constraint names ?pattern? */
    case ConstraintNames:
    {
	char const *pattern ;

	if (objc > 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "?pattern?") ;
	    return TCL_ERROR ;
	}
	pattern = objc == 3 ? "*" : Tcl_GetString(objv[3]) ;
	result = Ral_RelvarObjConstraintNames(interp, pattern, rInfo) ;
    }
	break ;

    /* relvar constraint member relvarName */
    case ConstraintMember:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "relvarName") ;
	    return TCL_ERROR ;
	}
	result = Ral_RelvarObjConstraintMember(interp, objv[3], rInfo) ;
	break ;

    /* relvar constraint path name */
    case ConstraintPath:
	result = Ral_RelvarObjConstraintPath(interp, objv[3], rInfo) ;
	break ;

    default:
	Tcl_Panic("Unknown association command type, %d", index) ;
    }

    return result ;
}

static int RelvarCorrelationCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    /* relvar correlation ?-complete? name corrRelvar
     *      corr-attr-list1 relvar1 attr-list1 spec1
     *      corr-attr-list2 relvar2 attr-list2 spec2
     */
    if (objc < 12 || objc > 13) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "?-complete? name corrRelvar "
	    "corr-attr-list1 relvar1 attr-list1 spec1 "
	    "corr-attr-list2 relvar2 attr-list2 spec2") ;
	return TCL_ERROR ;
    }
    return Ral_RelvarObjCreateCorrelation(interp, objv + 2, rInfo) ;
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
    Ral_ErrorInfo errInfo ;

    /* relvar create relvarName heading */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName heading") ;
	return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptCreate) ;

    if (Tcl_ListObjGetElements(interp, objv[3], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc != 3) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_FORMAT_ERR, objv[3]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_RelationObjType.name, Tcl_GetString(*elemv)) != 0) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_BAD_KEYWORD, *elemv) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Create the heading from the external representation.
     */
    heading = Ral_RelationHeadingNewFromObjs(interp, elemv[1], elemv[2],
	&errInfo) ;
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
    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptDelete,
	    RAL_ERR_ONGOING_CMD, objv[2]) ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

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
	result = Tcl_ExprBooleanObj(interp, exprObj, &boolValue) ;
	if (result != TCL_OK) {
	    break ;
	}
	if (boolValue) {
	    result = Ral_RelvarObjExecDeleteTraces(interp, relvar, tupleObj) ;
	    if (result != TCL_OK) {
		break ;
	    }
	    rIter = Ral_RelationErase(relation, rIter, rIter + 1) ;
	    ++deleted ;
	} else {
	    ++rIter ;
	}
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;
    if (result == TCL_OK) {
	if (deleted) {
	    Tcl_InvalidateStringRep(relvar->relObj) ;
	}
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
    int deleted = 0 ;
    int result ;
    Ral_ErrorInfo errInfo ;
    Ral_RelationIter found ;

    /* relvar deleteone relvarName ?attrName1 value1 attrName2 value2 ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName relvarName "
	    "?attrName1 value1 attrName2 value2 ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptDeleteone) ;

    key = Ral_RelationObjKeyTuple(interp, relation, objc, objv, &idNum,
	&errInfo) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptDeleteone,
	    RAL_ERR_ONGOING_CMD, objv[2]) ;
	return TCL_ERROR ;
    }

    found = Ral_RelationFindKey(relation, idNum, key, NULL) ;
    if (found != Ral_RelationEnd(relation)) {
	Tcl_Obj *tupleObj ;

	Tcl_IncrRefCount(tupleObj = Ral_TupleObjNew(*found)) ;
	if (Ral_RelvarObjExecDeleteTraces(interp, relvar, tupleObj) == TCL_OK) {
	    Ral_RelationErase(relation, found, found + 1) ;
	    deleted = 1 ;
	    Tcl_InvalidateStringRep(relvar->relObj) ;
	}
	Tcl_DecrRefCount(tupleObj) ;
    }

    Ral_TupleDelete(key) ;

    result = Ral_RelvarObjEndCmd(interp, rInfo, 0) ;
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(deleted)) ;
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
    Tcl_Obj *objPtr ;
    int result ;

    /* relvar eval arg ?arg ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "arg ?arg ...?") ;
	return TCL_ERROR ;
    }

    Ral_RelvarStartTransaction(rInfo, 0) ;

    Ral_RelvarObjExecEvalTraces(interp, rInfo, 1,
	Ral_PtrVectorSize(rInfo->transactions)) ;

    /*
     * Do not need to worry about deleting the return from Tcl_ConcatObj().
     * Tcl_EvalObjEx will do that after evaluating it.
     */
    objPtr = objc == 3 ? objv[2] : Tcl_ConcatObj(objc - 2, objv + 2) ;
    result = Tcl_EvalObjEx(interp, objPtr, 0) ;
    if (result == TCL_ERROR) {
	static const char msgfmt[] =
	    "\n    (\"in ::ral::relvar eval\" body line %d)" ;
	char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE] ;
	sprintf(msg, msgfmt, interp->errorLine) ;
	Tcl_AddObjErrorInfo(interp, msg, -1) ;
    }

    Ral_RelvarObjExecEvalTraces(interp, rInfo, 0,
	Ral_PtrVectorSize(rInfo->transactions)) ;

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
    int inserted = 0 ;
    int result = TCL_OK ;
    Ral_ErrorInfo errInfo ;
    Ral_Relation relation ;
    Ral_Relation resultRel ;

    /* relvar insert relvarName ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptInsert,
	    RAL_ERR_ONGOING_CMD, objv[2]) ;
	return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptInsert) ;
    objc -= 3 ;
    objv += 3 ;

    relation = relvar->relObj->internalRep.otherValuePtr ;
    resultRel = Ral_RelationNew(relation->heading) ;

    while (objc-- > 0) {
	Tcl_Obj *insertedTuple =
	    Ral_RelvarObjInsertTuple(interp, relvar, *objv++, &errInfo) ;
	if (insertedTuple) {
	    Ral_Tuple tuple ;

	    Tcl_IncrRefCount(insertedTuple) ;
	    assert(insertedTuple->typePtr == &Ral_TupleObjType) ;
	    tuple = insertedTuple->internalRep.otherValuePtr ;
	    if (!Ral_RelationPushBack(resultRel, tuple, NULL)) {
		Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_TUPLE,
		    insertedTuple) ;
		Ral_InterpSetError(interp, &errInfo) ;
		result = TCL_ERROR ;
		break ;
	    }
	    Tcl_DecrRefCount(insertedTuple) ;
	} else {
	    result = TCL_ERROR ;
	    break ;
	}
	++inserted ;
    }

    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;
    if (result == TCL_OK) {
	if (inserted) {
	    Tcl_InvalidateStringRep(relvar->relObj) ;
	}
	Tcl_SetObjResult(interp, Ral_RelationObjNew(resultRel)) ;
    } else {
	Ral_RelationDelete(resultRel) ;
    }
    return result ;
}

static int
RelvarIntersectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relvalue ;
    Ral_Relation intersectRel ;
    int result = TCL_OK ;
    Ral_ErrorInfo errInfo ;

    /* relvar intersect relvarName ?relationValue ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?relationValue ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    intersectRel = relvalue = relvar->relObj->internalRep.otherValuePtr ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptIntersect) ;

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    /*
     * Index to the first relation value.
     */
    objc -= 3 ;
    objv += 3 ;
    while (objc-- > 0) {
	Tcl_Obj *relObj ;
	Ral_Relation r1 ;
	Ral_Relation r2 ;

	r1 = intersectRel ;

	relObj = *objv++ ;
	if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType)
		!= TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
	r2 = relObj->internalRep.otherValuePtr ;

	intersectRel = Ral_RelationIntersect(r1, r2, &errInfo) ;
	if (r1 != relvalue) {
	    Ral_RelationDelete(r1) ;
	}
	if (intersectRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    result = TCL_ERROR ;
	    break ;
	}
    }
    if (result == TCL_OK) {
	Tcl_Obj *resultObj ;
	Tcl_Obj *intersectObj = Ral_RelationObjNew(intersectRel) ;

	Tcl_IncrRefCount(intersectObj) ;
	resultObj = Ral_RelvarObjExecSetTraces(interp, relvar, intersectObj,
		&errInfo) ;
	if (resultObj) {
	    Ral_RelvarSetRelation(relvar, resultObj) ;
	} else {
	    result = TCL_ERROR ;
	}
	Tcl_DecrRefCount(intersectObj) ;
    }
    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, relvar->relObj) ;
    }
    return result ;
}

static int
RelvarMinusCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relvalue ;
    Tcl_Obj *subObj ;
    Ral_Relation subvalue ;
    Ral_Relation diffvalue ;
    int result = TCL_OK ;
    Ral_ErrorInfo errInfo ;

    /* relvar intersect relvarName relationValue */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName relationValue") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    relvalue = relvar->relObj->internalRep.otherValuePtr ;

    subObj = objv[3] ;
    if (Tcl_ConvertToType(interp, subObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    subvalue = subObj->internalRep.otherValuePtr ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptMinus) ;

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    diffvalue = Ral_RelationMinus(relvalue, subvalue, &errInfo) ;
    if (diffvalue) {
	Tcl_Obj *diffObj = Ral_RelationObjNew(diffvalue) ;
	Tcl_Obj *resultObj ;
	/*
	 * Run the traces on the difference relation.
	 */
	Tcl_IncrRefCount(diffObj) ;
	resultObj = Ral_RelvarObjExecSetTraces(interp, relvar, diffObj,
	    &errInfo) ;
	/*
	 * Set the resulting value back into the relvar.
	 */
	if (resultObj) {
	    Ral_RelvarSetRelation(relvar, resultObj) ;
	} else {
	    result = TCL_ERROR ;
	}
	Tcl_DecrRefCount(diffObj) ;
    } else {
	Ral_InterpSetError(interp, &errInfo) ;
	result = TCL_ERROR ;
    }
    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;

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
    char const *pattern ;
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
	char const *relvarName ;

	relvarName = (char const *)Tcl_GetHashKey(relvarMap, entry) ;
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
RelvarPathCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;

    /* relvar path relvarName */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName") ;
	return TCL_ERROR ;
    }
    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(relvar->name, -1)) ;
    /*
     * Creating a partition is an implicit transaction as each
     * relvar participating in the association is treated as modified.
     */
    return TCL_OK ;
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
    Tcl_Obj *resultObj ;
    int result = TCL_OK ;
    Ral_ErrorInfo errInfo ;

    /* relvar set relvar relationValue */
    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvar ?relationValue?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    relvalue = relvar->relObj->internalRep.otherValuePtr ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptSet) ;

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
	    Ral_ErrorInfoSetError(&errInfo, RAL_ERR_HEADING_NOT_EQUAL,
		headingStr) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    ckfree(headingStr) ;
	    return TCL_ERROR ;
	}

	if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}

	Tcl_IncrRefCount(valueObj) ;
	resultObj = Ral_RelvarObjExecSetTraces(interp, relvar, valueObj,
	    &errInfo) ;
	if (resultObj) {
	    Ral_RelvarSetRelation(relvar, resultObj) ;
	} else {
	    result = TCL_ERROR ;
	}
	Tcl_DecrRefCount(valueObj) ;

	result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;
    }

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, relvar->relObj) ;
    }
    return result ;
}

/*
 * relvar trace add variable relvarName ops cmdPrefix
 * relvar trace remove variable relvarName ops cmdPrefix
 * relvar trace info variable relvarname
 *
 * relvar trace add eval cmdPrefix
 * relvar trace remove eval cmdPrefix
 * relvar trace info eval
 */
static int
RelvarTraceCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    enum TraceOption {
	TraceAdd,
	TraceRemove,
	TraceInfo,
    } ;
    static char const *traceOptions[] = {
	"add",
	"remove",
	"info",
	NULL
    } ;
    enum TraceType {
	TraceVariable,
	TraceEval,
    } ;
    static char const *traceTypes[] = {
	"variable",
	"eval",
	NULL
    } ;
    int option ;
    int type ;
    Ral_Relvar relvar ;
    int result = TCL_ERROR ;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "option type ?arg arg ...?") ;
	return TCL_ERROR ;
    }

    /*
     * Look up the option that indicates the operation to be performed.
     */
    if (Tcl_GetIndexFromObj(interp, objv[2], traceOptions, "trace option", 0,
	&option) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Look up the type of the trace.
     */
    if (Tcl_GetIndexFromObj(interp, objv[3], traceTypes, "trace type", 0,
	&type) != TCL_OK) {
	return TCL_ERROR ;
    }

    /*
     * Deal with variable tracing first.
     */
    if ((enum TraceType)type == TraceVariable) {
	relvar = Ral_RelvarObjFindRelvar(interp, rInfo,
		Tcl_GetString(objv[4])) ;
	if (relvar == NULL) {
	    return TCL_ERROR ;
	}

	switch ((enum TraceOption)option) {
	case TraceAdd:
	    /* relvar trace add variable relvarName ops cmdPrefix */
	    if (objc != 7) {
		Tcl_WrongNumArgs(interp, 2, objv,
		    "add variable relvarName ops cmdPrefix") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceVarAdd(interp, relvar, objv[5],
		objv[6]) ;
	    break ;

	case TraceRemove:
	    /* relvar trace remove variable relvarName ops cmdPrefix */
	    if (objc != 7) {
		Tcl_WrongNumArgs(interp, 2, objv,
		    "remove variable relvarName ops cmdPrefix") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceVarRemove(interp, relvar, objv[5],
		objv[6]) ;
	    break ;

	case TraceInfo:
	    /* relvar trace info variable relvarName */
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "info variable relvarName") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceVarInfo(interp, relvar) ;
	    break ;

	default:
	    Tcl_Panic("Unknown trace option, %d", option) ;
	}
    }
    /*
     * Deal with eval tracing.
     */
    else if ((enum TraceType)type == TraceEval) {
	switch ((enum TraceOption)option) {
	case TraceAdd:
	    /* relvar trace add eval cmdPrefix */
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "add eval cmdPrefix") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceEvalAdd(interp, rInfo, objv[4]) ;
	    break ;

	case TraceRemove:
	    /* relvar trace remove eval cmdPrefix */
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "remove eval cmdPrefix") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceEvalRemove(interp, rInfo, objv[4]) ;
	    break ;

	case TraceInfo:
	    /* relvar trace info eval*/
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "info eval") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceEvalInfo(interp, rInfo) ;
	    break ;

	default:
	    Tcl_Panic("Unknown trace option, %d", option) ;
	}
    } else {
	Tcl_Panic("Unknown trace type, %d", type) ;
    }

    return result ;
}

static int
RelvarTransactionCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    enum TransactionOption {
	TransactionBegin,
	TransactionEnd,
	TransactionRollback,
    } ;
    static char const *transactionOptions[] = {
	"begin",
	"end",
	"rollback",
	NULL
    } ;

    int option ;
    int result = TCL_OK ;

    /* relvar transaction begin | end | rollback */

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "transaction option") ;
	return TCL_ERROR ;
    }

    /*
     * Look up the option that indicates the transaction operation.
     */
    if (Tcl_GetIndexFromObj(interp, objv[2], transactionOptions,
	"transaction option", 0, &option) != TCL_OK) {
	return TCL_ERROR ;
    }
    switch ((enum TransactionOption)option) {
    case TransactionBegin:
	Ral_RelvarStartTransaction(rInfo, 0) ;

	Ral_RelvarObjExecEvalTraces(interp, rInfo, 1,
	    Ral_PtrVectorSize(rInfo->transactions)) ;
	break ;

    case TransactionEnd:
	Ral_RelvarObjExecEvalTraces(interp, rInfo, 0,
	    Ral_PtrVectorSize(rInfo->transactions)) ;
	result =  Ral_RelvarObjEndTrans(interp, rInfo, 0) ;
	break ;

    case TransactionRollback:
	Ral_RelvarObjExecEvalTraces(interp, rInfo, 0,
	    Ral_PtrVectorSize(rInfo->transactions)) ;
	Ral_RelvarObjEndTrans(interp, rInfo, 1) ;
	break ;

    default:
	Tcl_Panic("Unknown transaction option, %d", option) ;
    }

    return result ;
}

static int
RelvarUnionCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relvalue ;
    Ral_Relation unionRel ;
    int result = TCL_OK ;
    Ral_ErrorInfo errInfo ;

    /* relvar union relvarName ?relationValue ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?relationValue ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    unionRel = relvalue = relvar->relObj->internalRep.otherValuePtr ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptUnion) ;

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    /*
     * Index to the first relation value.
     */
    objc -= 3 ;
    objv += 3 ;
    while (objc-- > 0) {
	Tcl_Obj *relObj ;
	Ral_Relation r1 ;
	Ral_Relation r2 ;

	r1 = unionRel ;

	relObj = *objv++ ;
	if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType)
	    != TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
	r2 = relObj->internalRep.otherValuePtr ;

	unionRel = Ral_RelationUnion(r1, r2, &errInfo) ;
	if (r1 != relvalue) {
	    Ral_RelationDelete(r1) ;
	}
	if (unionRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    result = TCL_ERROR ;
	    break ;
	}
    }
    if (result == TCL_OK) {
	Tcl_Obj *unionObj = Ral_RelationObjNew(unionRel) ;
	Tcl_Obj *resultObj ;

	Tcl_IncrRefCount(unionObj) ;
	resultObj = Ral_RelvarObjExecSetTraces(interp, relvar, unionObj,
	    &errInfo) ;
	if (resultObj) {
	    Ral_RelvarSetRelation(relvar, resultObj) ;
	} else {
	    result = TCL_ERROR ;
	}
	Tcl_DecrRefCount(unionObj) ;
    }
    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, relvar->relObj) ;
    }
    return result ;
}

static int
RelvarUnsetCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    int result = TCL_OK ;

    /* relvar unset ?relvar1 relvar2 ...? */
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
    Ral_Relation updatedTuples ;
    Ral_RelationIter rEnd ;
    Ral_RelationIter rIter ;
    Ral_ErrorInfo errInfo ;
    int result = TCL_OK ;


    /* relvar update relvarName tupleVarName expr script */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName tupleVarName expr script") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptUpdate) ;

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(tupleVarNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;
    Tcl_IncrRefCount(scriptObj = objv[5]) ;

    updatedTuples = Ral_RelationNew(relation->heading) ;

    rEnd = Ral_RelationEnd(relation) ;
    for (rIter = Ral_RelationBegin(relation) ; rIter != rEnd ; ++rIter) {
	Tcl_Obj *tupleObj ;
	int boolValue ;

	/*
	 * Clone the tuple into a Tcl object and store it into the
	 * given variable. Note that we duplicate the tuple here
	 * so to avoid the possiblity that it might be modified
	 * in place by the expression evaluation.
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
	 * Evaluate the script, run the traces and update the relation.
	 */
	result = Ral_RelvarObjUpdateTuple(interp, relvar, relation, rIter,
	    scriptObj, tupleVarNameObj, updatedTuples, &errInfo) ;

	if (result != TCL_OK) {
	    if (result == TCL_CONTINUE) {
		result = TCL_OK ;
	    } else if (result == TCL_BREAK) {
		result = TCL_OK ;
		break ;
	    } else {
		break ;
	    }
	}
    }

    result = Ral_RelvarObjEndCmd(interp, rInfo, result == TCL_ERROR) == TCL_OK ?
	result : TCL_ERROR ;

    if (Ral_RelationCardinality(updatedTuples)) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    Tcl_UnsetVar(interp, Tcl_GetString(tupleVarNameObj), 0) ;
    Tcl_DecrRefCount(scriptObj) ;
    Tcl_DecrRefCount(tupleVarNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Ral_RelationObjNew(updatedTuples)) ;
    } else {
	Ral_RelationDelete(updatedTuples) ;
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
    Ral_Relation updatedTuples ;
    Ral_ErrorInfo errInfo ;

    /* relvar updateone relvarName tupleVarName idValueList script */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName tupleVarName idValueList script") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK ||
	    Ral_RelvarObjCopyOnShared(interp, rInfo, relvar) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptUpdateone) ;

    if (Tcl_ListObjGetElements(interp, objv[4], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }


    /*
     * Find the tuple whose identifying attribute values are given.
     */
    key = Ral_RelationObjKeyTuple(interp, relation, elemc, elemv, &idNum,
	&errInfo) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }
    found = Ral_RelationFindKey(relation, idNum, key, NULL) ;
    Ral_TupleDelete(key) ;
    updatedTuples = Ral_RelationNew(relation->heading) ;

    if (found != Ral_RelationEnd(relation)) {
	Tcl_Obj *tupleVarNameObj = objv[3] ;
	Tcl_Obj *tupleObj ;

	if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptUpdateone,
		RAL_ERR_ONGOING_CMD, objv[2]) ;
	    Ral_RelationDelete(updatedTuples) ;
	    return TCL_ERROR ;
	}

	/*
	 * Clone the tuple into a Tcl object and store it into the
	 * given variable.
	 */
	tupleObj = Ral_TupleObjNew(Ral_TupleDup(*found)) ;
	if (Tcl_ObjSetVar2(interp, tupleVarNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
	    Ral_RelationDelete(updatedTuples) ;
	    return TCL_ERROR ;
	}
	/*
	 * Evaluate the script, run the traces and update the relation.
	 */
	result = Ral_RelvarObjUpdateTuple(interp, relvar, relation, found,
	    objv[5], tupleVarNameObj, updatedTuples, &errInfo) ;
	if (result == TCL_OK) {
	    Tcl_InvalidateStringRep(relvar->relObj) ;
	}

	result = Ral_RelvarObjEndCmd(interp, rInfo, result == TCL_ERROR)
	    == TCL_OK ? result : TCL_ERROR ;
	/*
	 * Delete the variable.
	 */
	Tcl_UnsetVar(interp, Tcl_GetString(tupleVarNameObj), 0) ;
    }

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Ral_RelationObjNew(updatedTuples)) ;
    } else {
	Ral_RelationDelete(updatedTuples) ;
    }
    return result ;
}
