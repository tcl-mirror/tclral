/*
This software is copyrighted 2006 - 2011 by G. Andrew Mangogna.  The following
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
$Revision: 1.41 $
$Date: 2012/02/26 19:09:04 $
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
static int RelvarDunionCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarEvalCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarExistsCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarIdentifiersCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarInsertCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarIntersectCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarMinusCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarNamesCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarPartitionCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarPathCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarProceduralCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarRestrictOneCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarSetCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarTraceCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarTransactionCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarUinsertCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarUnionCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUnsetCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarUpdateCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUpdateOneCmd(Tcl_Interp *, int, Tcl_Obj *const*,
        Ral_RelvarInfo) ;
static int RelvarUpdatePerCmd(Tcl_Interp *, int, Tcl_Obj *const*,
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
        {"dunion", RelvarDunionCmd},
	{"eval", RelvarEvalCmd},
	{"exists", RelvarExistsCmd},
	{"identifiers", RelvarIdentifiersCmd},
	{"insert", RelvarInsertCmd},
	{"intersect", RelvarIntersectCmd},
	{"minus", RelvarMinusCmd},
	{"names", RelvarNamesCmd},
	{"partition", RelvarPartitionCmd},
	{"path", RelvarPathCmd},
	{"procedural", RelvarProceduralCmd},
	{"restrictone", RelvarRestrictOneCmd},
	{"set", RelvarSetCmd},
	{"trace", RelvarTraceCmd},
	{"transaction", RelvarTransactionCmd},
        {"uinsert", RelvarUinsertCmd},
	{"union", RelvarUnionCmd},
	{"unset", RelvarUnsetCmd},
	{"update", RelvarUpdateCmd},
	{"updateone", RelvarUpdateOneCmd},
	{"updateper", RelvarUpdatePerCmd},
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
	ConstraintExists,
	ConstraintInfo,
	ConstraintNames,
	ConstraintMember,
	ConstraintPath
    } ;
    static char const *constraintCmds[] = {
	"delete",
	"exists",
	"info",
	"names",
	"member",
	"path",
	NULL
    } ;
    int result = TCL_OK ;
    int index ;
    Ral_Constraint constraint ;

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

    /* relvar constraint exists name */
    case ConstraintExists:
	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 3, objv, "name") ;
	    return TCL_ERROR ;
	}
        constraint = Ral_RelvarObjFindConstraint(interp, rInfo,
                Tcl_GetString(objv[3])) ;
        Tcl_SetObjResult(interp, Tcl_NewBooleanObj(constraint != NULL)) ;
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
    Ral_TupleHeading heading ;
    Ral_ErrorInfo errInfo ;
    char const *name ;

    /* relvar create relvarName heading id1 ?id2 id3 ...? */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
                "relvarName heading id1 ?id2 id3 ...?") ;
	return TCL_ERROR ;
    }

    name = Tcl_GetString(objv[2]) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptCreate) ;

    /*
     * Create the heading from the external representation.
     */
    heading = Ral_TupleHeadingNewFromObj(interp, objv[3], &errInfo) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    objc -= 4 ;
    objv += 4 ;

    return Ral_RelvarObjNew(interp, rInfo, name, heading, objc, objv,
            &errInfo) ;
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
    if (relvar->stateFlags) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptDelete,
	    RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

    for (rIter = Ral_RelationBegin(relation) ;
	    rIter != Ral_RelationEnd(relation) ;) {
	int boolValue ;
	Tcl_Obj *tupleObj ;

	tupleObj = Ral_TupleObjNew(*rIter) ;
        Tcl_IncrRefCount(tupleObj) ;
	if (Tcl_ObjSetVar2(interp, tupleNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
            Tcl_DecrRefCount(tupleObj) ;
	    result = TCL_ERROR ;
	    break ;
	}
	result = Tcl_ExprBooleanObj(interp, exprObj, &boolValue) ;
	if (result != TCL_OK) {
            Tcl_DecrRefCount(tupleObj) ;
	    break ;
	}
	if (boolValue) {
	    result = Ral_RelvarObjExecDeleteTraces(interp, relvar, tupleObj) ;
	    if (result != TCL_OK) {
                Tcl_DecrRefCount(tupleObj) ;
		break ;
	    }
	    rIter = Ral_RelvarDeleteTuple(relvar, rIter) ;
	    ++deleted ;
	} else {
	    ++rIter ;
	}
        Tcl_DecrRefCount(tupleObj) ;
    }

    relvar->stateFlags = 0 ;
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
    int result = TCL_OK ;
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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptDeleteone) ;
    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

    objc -= 3 ;
    objv += 3 ;
    /*
     * Make a tuple to use as a key from the arguments.
     */
    key = Ral_RelvarObjKeyTuple(interp, relvar, objc, objv, &idNum,
            &errInfo) ;
    if (key != NULL) {
        found = Ral_RelvarFindById(relvar, idNum, key) ;
        Ral_TupleDelete(key) ;

        if (found != Ral_RelationEnd(relation)) {
            Tcl_Obj *tupleObj ;

            tupleObj = Ral_TupleObjNew(*found) ;
            Tcl_IncrRefCount(tupleObj) ;
            result = Ral_RelvarObjExecDeleteTraces(interp, relvar, tupleObj) ;
            if (result == TCL_OK) {
                Ral_RelvarDeleteTuple(relvar, found) ;
                deleted = 1 ;
                Tcl_InvalidateStringRep(relvar->relObj) ;
            }
            Tcl_DecrRefCount(tupleObj) ;
        }
    } else {
        result = TCL_ERROR ;
    }

    relvar->stateFlags = 0 ;
    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;
    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(deleted)) ;
    }
    return result ;
}

static int
RelvarDunionCmd(
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
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION,
                objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

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

	unionRel = Ral_RelationUnion(r1, r2, 1, &errInfo) ;
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
        if (!(resultObj && Ral_RelvarSetRelation(relvar, resultObj,
                &errInfo))) {
            Ral_InterpSetError(interp, &errInfo) ;
            result = TCL_ERROR ;
        }
	Tcl_DecrRefCount(unionObj) ;
    }
    relvar->stateFlags = 0 ;
    result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, relvar->relObj) ;
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
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
        Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
        "\n    (\"in ::ral::relvar eval\" body line %d)",
#       if TCL_MINOR_VERSION < 6
        interp->errorLine
#       else
        Tcl_GetErrorLine(interp)
#       endif
        )) ;
#endif
    }

    Ral_RelvarObjExecEvalTraces(interp, rInfo, 0,
	Ral_PtrVectorSize(rInfo->transactions)) ;

    return Ral_RelvarObjEndTrans(interp, rInfo, result == TCL_ERROR) == TCL_OK ?
	result : TCL_ERROR ;
}

static int
RelvarExistsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;

    /* relvar exists relvarName */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;

    Tcl_SetObjResult(interp,
            relvar ? Tcl_NewBooleanObj(1) : Tcl_NewBooleanObj(0)) ;
    return TCL_OK ;
}

static int
RelvarIdentifiersCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    Tcl_Obj *idListObj ;
    Tcl_Obj *idObj ;
    struct relvarId *idIter ;
    int cnt ;

    /* relvar identifiers relvarName */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    idListObj = Tcl_NewListObj(0, NULL) ;
    /*
     * Iterate through the identifiers and generate a list of attribute
     * names for each identifier. In general, each identifier is a list
     * of attribute names.
     */
    for (idIter = relvar->identifiers, cnt = relvar->idCount ; cnt != 0 ;
            ++idIter, --cnt) {
	Ral_IntVector idVect = idIter->idAttrs ;
	Ral_IntVectorIter iter ;

	idObj = Tcl_NewListObj(0, NULL) ;
	for (iter = Ral_IntVectorBegin(idVect) ;
                iter != Ral_IntVectorEnd(idVect) ; ++iter) {
	    Ral_Attribute attr ;
            
            attr = Ral_TupleHeadingFetch(relation->heading, *iter) ;
	    if (Tcl_ListObjAppendElement(interp, idObj,
                    Tcl_NewStringObj(attr->name, -1)) != TCL_OK) {
                goto errorOut ;
	    }
	}

	if (Tcl_ListObjAppendElement(interp, idListObj, idObj) != TCL_OK) {
            goto errorOut ;
	}
    }

    Tcl_SetObjResult(interp, idListObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(idObj) ;
    Tcl_DecrRefCount(idListObj) ;
    return TCL_ERROR ;
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
    Ral_IntVector orderMap = NULL ;

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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptInsert) ;
    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

    objc -= 3 ;
    objv += 3 ;

    /*
     * The result of relvar insert is a relation value that contains the
     * tuples that were inserted into the relvar. This allows access to
     * modifications that the relvar traces might have performed.
     */
    relation = relvar->relObj->internalRep.otherValuePtr ;
    resultRel = Ral_RelationNew(relation->heading) ;

    while (objc-- > 0) {
        Tcl_Obj *insertedTuple ;

        Ral_IntVectorDelete(orderMap) ;
        insertedTuple = Ral_RelvarObjInsertTuple(interp, relvar, *objv++,
                &orderMap, &errInfo) ;
        if (insertedTuple) {
            Ral_Tuple tuple ;

            assert(insertedTuple->typePtr == &Ral_TupleObjType) ;
            tuple = insertedTuple->internalRep.otherValuePtr ;
            if (!Ral_RelationPushBack(resultRel, tuple, orderMap)) {
                Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_TUPLE,
                        insertedTuple) ;
                Ral_InterpSetError(interp, &errInfo) ;
                result = TCL_ERROR ;
                break ;
            }
            /*
             * The inserted tuple object had a reference count of at least
             * one from the insertion processing. We now need to discard
             * the Tcl_Obj, even though the underlying tuple had
             * its reference count incremented by being inserted into
             * both the relvar and the result relation value.
             */
            Tcl_DecrRefCount(insertedTuple) ;
            ++inserted ;
	} else {
	    result = TCL_ERROR ;
	    break ;
	}
    }

    Ral_IntVectorDelete(orderMap) ;
    relvar->stateFlags = 0 ;
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
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

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
        if (!(resultObj && Ral_RelvarSetRelation(relvar, resultObj,
                &errInfo))) {
            Ral_InterpSetError(interp, &errInfo) ;
            result = TCL_ERROR ;
        }
	Tcl_DecrRefCount(intersectObj) ;
    }
    relvar->stateFlags = 0 ;
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

    /* relvar minus relvarName relationValue */
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
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

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
        if (!(resultObj && Ral_RelvarSetRelation(relvar, resultObj,
                &errInfo))) {
            Ral_InterpSetError(interp, &errInfo) ;
            result = TCL_ERROR ;
        }
	Tcl_DecrRefCount(diffObj) ;
    } else {
	Ral_InterpSetError(interp, &errInfo) ;
	result = TCL_ERROR ;
    }
    relvar->stateFlags = 0 ;
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
    return TCL_OK ;
}

static int
RelvarProceduralCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    /* relvar procedural name relvarName1 ... script */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
                "name relvarName1 ?relvarName2 ...? script") ;
	return TCL_ERROR ;
    }
    /*
     * -3 to skip the "relvar", "procedural" and "script" arguments.
     * +2 to point to the contraint name
     */
    return Ral_RelvarObjCreateProcedural(interp, objc - 3, objv + 2,
            *(objv + objc - 1), rInfo) ;
}

static int
RelvarRestrictOneCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    Ral_ErrorInfo errInfo ;
    Ral_Tuple key ;
    int idNum ;
    Ral_Relation newRelation ;
    Ral_RelationIter found ;

    /* relvar choose relvarName attr value ?attr2 value2 ...? */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarValue attr value ?attr2 value 2 ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2])) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
		!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptRestrictone) ;
    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_BAD_PAIRS_LIST,
	    "attribute / value arguments must be given in pairs") ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Make a tuple to use as a key from the arguments.
     */
    key = Ral_RelvarObjKeyTuple(interp, relvar, objc, objv, &idNum,
            &errInfo) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }
    /*
     * Create the result relation.
     */
    newRelation = Ral_RelationNew(relation->heading) ;
    /*
     * Check if we find the tuple. If so, put it into the new relation.
     */
    found = Ral_RelvarFindById(relvar, idNum, key) ;
    Ral_TupleDelete(key) ;
    if (found != Ral_RelationEnd(relation)) {
        int inserted ;

        inserted = Ral_RelationPushBack(newRelation, *found, NULL) ;
        /*
         * Should always be able to insert into an empty relation.
         */
        assert(inserted != 0) ;
        (void)inserted ;
    }
    /*
     * Either we we return a new relation value. This will necessarily
     * have a cardinality of 0 or 1.
     */
    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;

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
    int result = TCL_ERROR ;
    Ral_ErrorInfo errInfo ;

    /* relvar set relvar ?relationValue? */
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

	if (!Ral_TupleHeadingEqual(relvalue->heading, relation->heading)) {
	    char *headingStr = Ral_TupleHeadingStringOf(relation->heading) ;
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
        if (relvar->stateFlags) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
        }
        relvar->stateFlags = 1 ;

	resultObj = Ral_RelvarObjExecSetTraces(interp, relvar, valueObj,
	    &errInfo) ;
        if (resultObj) {
            if (Ral_RelvarSetRelation(relvar, resultObj, &errInfo)) {
                result = TCL_OK ;
            } else {
                Ral_InterpSetError(interp, &errInfo) ;
            }
            Tcl_DecrRefCount(resultObj) ;
        }

        relvar->stateFlags = 0 ;
	result = Ral_RelvarObjEndCmd(interp, rInfo, result != TCL_OK) ;
    } else {
        result = TCL_OK ;
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
 * relvar trace suspend variable relvarname script
 *
 * relvar trace add transaction cmdPrefix
 * relvar trace remove transaction cmdPrefix
 * relvar trace info transaction
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
	TraceSuspend
    } ;
    static char const *traceOptions[] = {
	"add",
	"remove",
	"info",
	"suspend",
	NULL
    } ;
    enum TraceType {
	TraceVariable,
	TraceEval
    } ;
    static char const *traceTypes[] = {
	"variable",
	"transaction",
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

        case TraceSuspend:
	    /* relvar trace suspend variable relvarName script */
	    if (objc != 6) {
		Tcl_WrongNumArgs(interp, 2, objv,
                        "suspend variable relvarName script") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceVarSuspend(interp, relvar, objv[5]) ;
            break ;

	default:
	    Tcl_Panic("Unknown trace option, %d", option) ;
	}
    }
    /*
     * Deal with transaction tracing.
     */
    else if ((enum TraceType)type == TraceEval) {
	switch ((enum TraceOption)option) {
	case TraceAdd:
	    /* relvar trace add transaction cmdPrefix */
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv, "add transaction cmdPrefix") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceEvalAdd(interp, rInfo, objv[4]) ;
	    break ;

	case TraceRemove:
	    /* relvar trace remove transaction cmdPrefix */
	    if (objc != 5) {
		Tcl_WrongNumArgs(interp, 2, objv,
                        "remove transaction cmdPrefix") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceEvalRemove(interp, rInfo, objv[4]) ;
	    break ;

	case TraceInfo:
	    /* relvar trace info transaction*/
	    if (objc != 4) {
		Tcl_WrongNumArgs(interp, 2, objv, "info transaction") ;
		return TCL_ERROR ;
	    }
	    result = Ral_RelvarObjTraceEvalInfo(interp, rInfo) ;
	    break ;

        case TraceSuspend:
            Tcl_SetObjResult(interp,
                    Tcl_NewStringObj("suspending eval traces not implemented",
                    -1)) ;
	    result = TCL_ERROR ;
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
	TransactionRollback
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
RelvarUinsertCmd(
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
    Ral_IntVector orderMap = NULL ;

    /* relvar uinsert relvarName ?name-value-list ...? */
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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptUinsert) ;
    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION,
                objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

    objc -= 3 ;
    objv += 3 ;

    /*
     * The result of relvar uinsert is a relation value that contains the
     * tuples that were inserted into the relvar. This allows access to
     * modifications that the relvar traces might have performed.
     */
    relation = relvar->relObj->internalRep.otherValuePtr ;
    resultRel = Ral_RelationNew(relation->heading) ;

    while (objc-- > 0) {
        Tcl_Obj *insertedTuple ;

        Ral_IntVectorDelete(orderMap) ;
        insertedTuple = Ral_RelvarObjInsertTuple(interp, relvar, *objv++,
                &orderMap, &errInfo) ;
        if (insertedTuple) {
            Ral_Tuple tuple ;

            assert(insertedTuple->typePtr == &Ral_TupleObjType) ;
            tuple = insertedTuple->internalRep.otherValuePtr ;
            Ral_RelationPushBack(resultRel, tuple, orderMap) ;
            /*
             * The inserted tuple object had a reference count of at least
             * one from the insertion processing. We now need to discard
             * the Tcl_Obj, even though the underlying tuple had
             * its reference count incremented by being inserted into
             * both the relvar and the result relation value.
             */
            Tcl_DecrRefCount(insertedTuple) ;
            ++inserted ;
	}
        /*
         * N.B. we ignore any attempt the insert a tuple that fails
         * the identification constraints.
         */
    }

    Ral_IntVectorDelete(orderMap) ;
    relvar->stateFlags = 0 ;
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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptDunion) ;
    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION,
                objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

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

	unionRel = Ral_RelationUnion(r1, r2, 0, &errInfo) ;
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
        if (!(resultObj && Ral_RelvarSetRelation(relvar, resultObj,
                &errInfo))) {
            Ral_InterpSetError(interp, &errInfo) ;
            result = TCL_ERROR ;
        }
	Tcl_DecrRefCount(unionObj) ;
    }
    relvar->stateFlags = 0 ;
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
    if (relvar->stateFlags) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

    Tcl_IncrRefCount(tupleVarNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;
    Tcl_IncrRefCount(scriptObj = objv[5]) ;

    updatedTuples = Ral_RelationNew(relation->heading) ;

    for (rIter = Ral_RelationBegin(relation) ;
            rIter != Ral_RelationEnd(relation) ; ++rIter) {
	Tcl_Obj *tupleObj ;
	int boolValue ;

	/*
	 * Place the tuple into a Tcl object and store it into the
	 * given variable.
	 */
	tupleObj = Ral_TupleObjNew(*rIter) ;
        Tcl_IncrRefCount(tupleObj) ;
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
            Tcl_DecrRefCount(tupleObj) ;
	    break ;
	}
	if (boolValue) {
            /*
             * Evaluate the script, run the traces and update the relation.
             */
            result = Ral_RelvarObjUpdateTuple(interp, relvar, rIter, scriptObj,
                    tupleObj, updatedTuples, &errInfo) ;
            if (result != TCL_OK) {
                Tcl_DecrRefCount(tupleObj) ;
                if (result == TCL_BREAK) {
                    result = TCL_OK ;
                }
                break ;
            }
        }
        Tcl_DecrRefCount(tupleObj) ;
    }

    relvar->stateFlags = 0 ;
    result = Ral_RelvarObjEndCmd(interp, rInfo, result == TCL_ERROR) == TCL_OK ?
	result : TCL_ERROR ;

    if (Ral_RelationCardinality(updatedTuples)) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    Tcl_UnsetVar(interp, Tcl_GetString(tupleVarNameObj), 0) ;
    Tcl_DecrRefCount(scriptObj) ;
    Tcl_DecrRefCount(tupleVarNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    if (result == TCL_ERROR) {
	Ral_RelationDelete(updatedTuples) ;
    } else {
	Tcl_SetObjResult(interp, Ral_RelationObjNew(updatedTuples)) ;
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
    /*
     * Make a tuple to use as a key from the attribute / value list.
     */
    if (Tcl_ListObjGetElements(interp, objv[4], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    key = Ral_RelvarObjKeyTuple(interp, relvar, elemc, elemv, &idNum,
            &errInfo) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }
    found = Ral_RelvarFindById(relvar, idNum, key) ;
    Ral_TupleDelete(key) ;

    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
        Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptUpdateone,
            RAL_ERR_ONGOING_CMD, objv[2]) ;
        return TCL_ERROR ;
    }
    if (relvar->stateFlags) {
        Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptUpdateone,
            RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
        return TCL_ERROR ;
    }
    relvar->stateFlags = 1 ;

    updatedTuples = Ral_RelationNew(relation->heading) ;
    if (found != Ral_RelationEnd(relation)) {
	Tcl_Obj *tupleVarNameObj = objv[3] ;
	Tcl_Obj *tupleObj ;

	/*
	 * Make the tuple into a Tcl object and store it into the
	 * given variable.
	 */
	tupleObj = Ral_TupleObjNew(*found) ;
        Tcl_IncrRefCount(tupleObj) ;
	if (Tcl_ObjSetVar2(interp, tupleVarNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
            result = TCL_ERROR ;
	    Tcl_DecrRefCount(tupleObj) ;
	} else {
            /*
             * Evaluate the script, run the traces and update the relation.
             */
            result = Ral_RelvarObjUpdateTuple(interp, relvar, found,
                objv[5], tupleObj, updatedTuples, &errInfo) ;
            if (result != TCL_ERROR) {
                Tcl_InvalidateStringRep(relvar->relObj) ;
            }
        }
	/*
	 * Delete the variable.
	 */
	Tcl_UnsetVar(interp, Tcl_GetString(tupleVarNameObj), 0) ;
        Tcl_DecrRefCount(tupleObj) ;
    }

    relvar->stateFlags = 0 ;
    result = Ral_RelvarObjEndCmd(interp, rInfo, result == TCL_ERROR) == TCL_OK ?
            result : TCL_ERROR ;
    if (result == TCL_ERROR) {
	Ral_RelationDelete(updatedTuples) ;
    } else if (result != TCL_RETURN) {
	Tcl_SetObjResult(interp, Ral_RelationObjNew(updatedTuples)) ;
    }
    return result ;
}

/*
 * relvar updateper relvarName relationValue
 * 1. "relationValue" is a projection of "relvarName"
 * 2. At least one identifier of "relvarName" must be present in "relationValue"
 * 3. For each tuple in "relationValue", find the tuple
 *    in "relvarName" that matches the identifying attributes and update
 *    the non-identifying attributes to match the those in "relationValue"
 *
 * This is one big whacking function for this command. It could be factored
 * into sub-functions, but they would all be called only once and so
 * there is really no common code to factor. There is a little common
 * code at the end shared with other update commands but this command is
 * quite unique in the way the logic turned out.
 */
static int
RelvarUpdatePerCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    Tcl_Obj *relValueObj ;
    Ral_Relation perRelation ;
    Ral_TupleHeadingIter hIter ;
    Ral_PtrVector idSet ;
    Ral_IntVector idNums ;
    int idCount ;
    struct relvarId *idIter ;
    Ral_IntVector idAttrSet ;
    Ral_PtrVectorIter sIter ;
    Ral_IntVector nonIdAttrSet ;
    Ral_Relation updatedTuples ;
    Ral_RelationIter perIter ;
    int result = TCL_OK ;
    Ral_ErrorInfo errInfo ;

    /* relvar updateper relvarName relationValue */
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
    relation = relvar->relObj->internalRep.otherValuePtr ;

    relValueObj = objv[3] ;
    if (Tcl_ConvertToType(interp, relValueObj, &Ral_RelationObjType)
        != TCL_OK) {
        return TCL_ERROR ;
    }
    perRelation = relValueObj->internalRep.otherValuePtr ;
    /*
     * First we insist the all the attibutes of the "perRelation" also
     * be attributes of the relvar relation.
     */
    for (hIter = Ral_TupleHeadingBegin(perRelation->heading) ;
            hIter != Ral_TupleHeadingEnd(perRelation->heading) ; ++hIter) {
        Ral_Attribute pattr = *hIter ;
        Ral_TupleHeadingIter riter ;

        riter = Ral_TupleHeadingFind(relation->heading, pattr->name) ;
        if (riter == Ral_TupleHeadingEnd(relation->heading)) {
            Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptUpdateper,
                    RAL_ERR_UNKNOWN_ATTR, pattr->name) ;
            return TCL_ERROR ;
        }
        if (!Ral_AttributeEqual(pattr, *riter)) {
            Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptUpdateper,
                    RAL_ERR_TYPE_MISMATCH, pattr->name) ;
            return TCL_ERROR ;
        }
    }
    /*
     * Iterate through the identifiers for the relvar and determine
     * if the "per" relation contains attributes that correspond to
     * one or more identifiers. Create a vector of int vectors that
     * contain the corresponding attribute indices.
     */
    idSet = Ral_PtrVectorNew(relvar->idCount) ;
    idNums = Ral_IntVectorNewEmpty(relvar->idCount) ;
    idCount = 0 ;
    /*
     * Iterate through the identifiers.
     */
    for (idIter = relvar->identifiers ;
            idIter != relvar->identifiers + relvar->idCount ;
            ++idIter, ++idCount) {
        Ral_IntVector idVect ;
        Ral_IntVectorIter idxIter ;
        int refIndex = -1 ;
        int refCount = 0 ;
        /*
         * Create a new vector that will be used to store the indices
         * of the attributes from "perRelation".
         */
        idVect = Ral_IntVectorNewEmpty(Ral_IntVectorSize(idIter->idAttrs)) ;
        /*
         * Iterate throught indices for a given identifier.
         */
        for (idxIter = Ral_IntVectorBegin(idIter->idAttrs) ;
                idxIter != Ral_IntVectorEnd(idIter->idAttrs) ; ++idxIter) {
            Ral_Attribute attr ;

            attr = Ral_TupleHeadingFetch(relation->heading, *idxIter) ;
            refIndex = Ral_TupleHeadingIndexOf(perRelation->heading,
                    attr->name) ;
            if (refIndex == -1 || !Ral_AttributeEqual(attr,
                    Ral_TupleHeadingFetch(perRelation->heading, refIndex))) {
                /*
                 * We insist that any attributes in "perRelation" that are part
                 * of an identifier must be a complete identifier.
                 */
                Ral_IntVectorDelete(idVect) ;
                if (refCount > 0) {
                    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar,
                            Ral_OptUpdateper, RAL_ERR_NOT_AN_IDENTIFIER,
                            objv[3]) ;
                    result = TCL_ERROR ;
                    goto cleanup ;
                } else {
                    /*
                     * In case we got here because the name matched but
                     * the data type didn't.
                     */
                    refIndex = -1 ;
                    break ;
                }
            }
            Ral_IntVectorPushBack(idVect, refIndex) ;
            ++refCount ;
        }
        if (refIndex >= 0) {
            Ral_PtrVectorPushBack(idSet, idVect) ;
            Ral_IntVectorPushBack(idNums, idCount) ;
        }
    }
    /*
     * We insist that the attibutes for at least one identifier show
     * up in the "perRelation".
     */
    if (Ral_PtrVectorSize(idSet) == 0) {
        Ral_PtrVectorDelete(idSet) ;
        Ral_IntVectorDelete(idNums) ;
        Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptUpdateper,
                RAL_ERR_NOT_AN_IDENTIFIER, objv[3]) ;
        return TCL_ERROR ;
    }
    /*
     * Now we need to find all the non-identifing attributes. We do that
     * by tallying all the identifying ones and then take the complement
     * of that set.
     */
    idAttrSet = Ral_IntVectorNewEmpty(
            Ral_TupleHeadingSize(perRelation->heading));
    for (sIter = Ral_PtrVectorBegin(idSet) ; sIter != Ral_PtrVectorEnd(idSet) ;
            ++sIter) {
        Ral_IntVector idVect = *sIter ;
        Ral_IntVectorIter idxIter ;

        for (idxIter = Ral_IntVectorBegin(idVect) ;
                idxIter != Ral_IntVectorEnd(idVect) ; ++idxIter) {
            Ral_IntVectorSetAdd(idAttrSet, *idxIter) ;
        }
    }
    /*
     * Now we can compute those attributes that are not part of
     * an identifier. These are the attributes that will be updated.
     */
    nonIdAttrSet = Ral_IntVectorSetComplement(idAttrSet,
            Ral_TupleHeadingSize(perRelation->heading)) ;
    Ral_IntVectorDelete(idAttrSet) ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelvar, Ral_OptUpdateper) ;
    if (!Ral_RelvarStartCommand(rInfo, relvar)) {
        Ral_IntVectorDelete(nonIdAttrSet) ;
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_CMD, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
        return TCL_ERROR ;
    }
    if (relvar->stateFlags) {
        Ral_IntVectorDelete(nonIdAttrSet) ;
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_ONGOING_MODIFICATION, objv[2]) ;
	Ral_InterpSetError(interp, &errInfo) ;
        return TCL_ERROR ;
    }
    updatedTuples = Ral_RelationNew(relation->heading) ;
    /*
     * Iterate through the tuples of the "perRelation" and for each tuple find
     * the corresponding tuple in the relvar. Update the non-identifying
     * attributes. Run the traces and add the resulting tuple to the result
     * value that is to be returned.
     */
    for (perIter = Ral_RelationBegin(perRelation) ;
            perIter != Ral_RelationEnd(perRelation) ; ++perIter) {
        Ral_Tuple perTuple ;
        Ral_PtrVectorIter idSetIter ;
        Ral_IntVectorIter numIter ;
        int tupleOffset = - 1 ;
        int prevTupleOffset = -1 ;
        int idCount = 0 ;

        /*
         * Iterate across the identifiers and make sure that we find a tuple
         * that matches all the attributes that were contained in the
         * perRelation heading.  We want to find that tuple in "relvar" that
         * matches the values of the corresponding identifying attributes in
         * "perTuple".  Make sure that all matches are to the same tuple.
         */
        perTuple = *perIter ;
        for (idSetIter = Ral_PtrVectorBegin(idSet),
                numIter = Ral_IntVectorBegin(idNums) ;
                idSetIter != Ral_PtrVectorEnd(idSet) ;
                ++idSetIter, ++numIter) {
            /*
             * Hash into the hash table for the identifier using the
             * attribute vector and values from the "perTuple".
             */
            struct Ral_TupleAttrHashKey key ;
            Tcl_HashEntry *entry ;

            key.tuple = perTuple ;
            key.attrs = *idSetIter ;
            entry = Tcl_FindHashEntry(&relvar->identifiers[*numIter].idIndex,
                    (char const *)&key) ;
            if (entry) {
                tupleOffset = (int)Tcl_GetHashValue(entry) ;
                if (!(prevTupleOffset == -1 ||
                        prevTupleOffset == tupleOffset)) {
                    break ;
                }
                prevTupleOffset = tupleOffset ;
                ++idCount ;
            }
        }
        if (idCount == Ral_PtrVectorSize(idSet)) {
            /*
             * Found a match. Update the values of the non-identifying
             * attributes.
             */
            Ral_RelationIter relIter ;
            Ral_Tuple matchTuple ;
            Ral_IntVectorIter nidIter ;

            assert(tupleOffset != -1) ;
            relIter = Ral_RelationBegin(relation) + tupleOffset ;
            matchTuple = *relIter ;
            /*
             * Make a copy if this tuple is shared. Use "shallow" duplication
             * since we want to share the tuple heading.
             */
            if (matchTuple->refCount > 1) {
                matchTuple = Ral_TupleDupShallow(*relIter) ;
            }
            /*
             * Iterate through the non-identifying attributes.  Update values
             * from the perTuple into the matching tuple.
             */
            for (nidIter = Ral_IntVectorBegin(nonIdAttrSet) ;
                    nidIter != Ral_IntVectorEnd(nonIdAttrSet) ; ++nidIter) {
                Ral_TupleIter tIter ;
                Ral_TupleHeadingIter thIter ;
                Ral_TupleHeadingIter aIter ;
                Tcl_Obj *newValue ;
                Tcl_Obj *oldValue ;
                int valueIndex ;

                tIter = Ral_TupleBegin(perTuple) + *nidIter ;
                thIter = Ral_TupleHeadingBegin(perTuple->heading) + *nidIter ;
                aIter = Ral_TupleHeadingFind(matchTuple->heading,
                        (*thIter)->name) ;
                assert(aIter != Ral_TupleHeadingEnd(matchTuple->heading)) ;
                newValue = Ral_AttributeConvertValueToType(interp, *aIter,
                        *tIter, &errInfo) ;
                assert(newValue != NULL) ;
                valueIndex = aIter -
                        Ral_TupleHeadingBegin(matchTuple->heading) ;
                oldValue = matchTuple->values[valueIndex] ;
                if (oldValue) {
                    Tcl_DecrRefCount(oldValue) ;
                }
                matchTuple->values[valueIndex] = newValue ;
                Tcl_IncrRefCount(newValue) ;
            }
            /*
             * Now we have updated the "matchTuple" with new values
             * from the matching "perRelation" tuple. Create an object,
             * run the traces and perform the update.
             */
            result = Ral_RelvarObjTraceUpdate(interp, relvar, relIter,
                    Ral_TupleObjNew(matchTuple), updatedTuples, &errInfo) ;
            if (result != TCL_OK) {
                break ;
            }
        }
    }
    Ral_IntVectorDelete(nonIdAttrSet) ;

    relvar->stateFlags = 0 ;
    result = Ral_RelvarObjEndCmd(interp, rInfo, result == TCL_ERROR) == TCL_OK ?
            result : TCL_ERROR ;
    if (result == TCL_ERROR) {
	Ral_RelationDelete(updatedTuples) ;
    } else {
	Tcl_SetObjResult(interp, Ral_RelationObjNew(updatedTuples)) ;
    }

cleanup:
    Ral_IntVectorDelete(idNums) ;
    for (sIter = Ral_PtrVectorBegin(idSet) ; sIter != Ral_PtrVectorEnd(idSet) ;
            ++sIter) {
        Ral_IntVectorDelete(*sIter) ;
    }
    Ral_PtrVectorDelete(idSet) ;
    return result ;
}
