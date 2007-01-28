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

$RCSfile: ral_relvarobj.c,v $
$Revision: 1.28 $
$Date: 2007/01/28 02:21:11 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <assert.h>
#include <string.h>
#include "ral_relvarobj.h"
#include "ral_relvar.h"
#include "ral_tupleobj.h"
#include "ral_relationobj.h"
#include "ral_utils.h"

/*
 * We use Tcl_GetCurrentNamespace()
 * Before 8.5, they not part of the supported external interface.
 */
#if TCL_MINOR_VERSION <= 4
#   include "tclInt.h"
#endif

/*
MACRO DEFINITIONS
*/

/*
 * Enumeration of the flags used for tracing.
 * N.B. the careful encoding by powers of 2.
 */
#define	TRACEOP_DELETE_FLAG 0x01
#define	TRACEOP_INSERT_FLAG 0x02
#define	TRACEOP_SET_FLAG 0x04
#define	TRACEOP_UNSET_FLAG 0x08
#define	TRACEOP_UPDATE_FLAG 0x10
#define	TRACEOP_EVAL_FLAG 0x20

/*
TYPE DEFINITIONS
*/

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static char *relvarTraceProc(ClientData, Tcl_Interp *, const char *,
    const char *, int) ;
static const char *relvarResolveName(Tcl_Interp *, const char *,
    Tcl_DString *) ;
static int relvarNameIsAbsName(const char *) ;
static int relvarGetNamespaceName(Tcl_Interp *, const char *, Tcl_DString *) ;
static Tcl_Obj *relvarConstraintAttrNames(Tcl_Interp *, Ral_Relvar,
    Ral_JoinMap, int) ;
static Tcl_Obj *relvarAssocSpec(int, int) ;
static Ral_Constraint Ral_RelvarObjFindConstraint(Tcl_Interp *, Ral_RelvarInfo,
    const char *, char **) ;
static int Ral_RelvarObjDecodeTraceOps(Tcl_Interp *, Tcl_Obj *, int *) ;
static int Ral_RelvarObjEncodeTraceFlag(Tcl_Interp *, int, Tcl_Obj **) ;
static Tcl_Obj *Ral_RelvarObjExecTraces(Tcl_Interp *, Ral_Relvar, Tcl_ObjType *,
    Tcl_Obj *, Tcl_Obj *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static struct {
    const char *specString ;
    int conditionality ;
    int multiplicity ;
} specTable[] = {
    {"1", 0, 0},
    {"+", 0, 1},
    {"?", 1, 0},
    {"*", 1, 1},
    {NULL, 0, 0}
} ;

static const struct traceOpsMap {
    const char *opName ;
    int opFlag ;
} opsTable[] = {
    {"delete", TRACEOP_DELETE_FLAG},
    {"insert", TRACEOP_INSERT_FLAG},
    {"set", TRACEOP_SET_FLAG},
    {"unset", TRACEOP_UNSET_FLAG},
    {"update", TRACEOP_UPDATE_FLAG},
    {NULL, 0}
} ;
static const char specErrMsg[] = "multiplicity specification" ;
static int relvarTraceFlags = TCL_NAMESPACE_ONLY | TCL_TRACE_WRITES ;
static const char rcsid[] = "@(#) $RCSfile: ral_relvarobj.c,v $ $Revision: 1.28 $" ;

/*
FUNCTION DEFINITIONS
*/

int
Ral_RelvarObjNew(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    const char *name,
    Ral_RelationHeading heading)
{
    Tcl_DString resolve ;
    const char *resolvedName ;
    Ral_Relvar relvar ;
    int status ;

    /*
     * Creating a relvar is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptCreate,
	    RAL_ERR_BAD_TRANS_OP, "create") ;
	return TCL_ERROR ;
    }
    /*
     * Create the relvar.
     */
    resolvedName = relvarResolveName(interp, name, &resolve) ;
    relvar = Ral_RelvarNew(info, resolvedName, heading) ;
    if (relvar == NULL) {
	/*
	 * Duplicate name.
	 */
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptCreate,
	    RAL_ERR_DUP_NAME, resolvedName) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    /*
     * Create a variable by the same name.
     */
    if (Tcl_SetVar2Ex(interp, resolvedName, NULL, relvar->relObj,
	TCL_LEAVE_ERR_MSG) == NULL) {
	Tcl_DStringFree(&resolve) ;
	Ral_RelvarDelete(info, resolvedName) ;
	return TCL_ERROR ;
    }
    /*
     * Set up a trace to make the Tcl variable read only.
     */
    status = Tcl_TraceVar(interp, resolvedName, relvarTraceFlags,
	relvarTraceProc, info) ;
    assert(status == TCL_OK) ;

    Tcl_DStringFree(&resolve) ;
    Tcl_SetObjResult(interp, relvar->relObj) ;
    return TCL_OK ;
}

int
Ral_RelvarObjDelete(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    Tcl_Obj *nameObj)
{
    char *fullName ;
    Ral_Relvar relvar ;
    /*
     * Deleting a relvar is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptDelete,
	    RAL_ERR_BAD_TRANS_OP, "delete") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(nameObj),
	&fullName) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	ckfree(fullName) ;
	return TCL_ERROR ;
    }
    /*
     * Make sure that all the constraints associated with the
     * relvar are gone. Can't have dangling references to the relvar
     * via a constraint.
     */
    if (Ral_PtrVectorSize(relvar->constraints) != 0) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptDelete,
	    RAL_ERR_CONSTRAINTS_PRESENT, fullName) ;
	ckfree(fullName) ;
	return TCL_ERROR ;
    }
    /*
     * Run the unset traces.
     */
    Ral_RelvarObjExecUnsetTraces(interp, relvar) ;
    /*
     * Remove the trace.
     */
    Tcl_UntraceVar(interp, fullName, relvarTraceFlags, relvarTraceProc,
	info) ;
    /*
     * Remove the variable.
     */
    if (Tcl_UnsetVar(interp, fullName, TCL_LEAVE_ERR_MSG) != TCL_OK) {
	ckfree(fullName) ;
	return TCL_ERROR ;
    }

    Ral_RelvarDelete(info, fullName) ;

    ckfree(fullName) ;
    Tcl_ResetResult(interp) ;
    return TCL_OK ;
}

/*
 * (1) If name is fully resolved, then try to match it directly.
 * (2) else construct a fully resolved name for the current namespace and
 *     try to find that.
 * (3) If that fails, construct a fully resolved name for the global
 *     namespace.
 * (4) If that fails, then it is an unknown relvar.
 */
Ral_Relvar
Ral_RelvarObjFindRelvar(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    const char *name,
    char **fullName)
{
    Ral_Relvar relvar ;

    if (relvarNameIsAbsName(name)) {
	/*
	 * Absolute name.
	 */
	relvar = Ral_RelvarFind(info, name) ;
	if (relvar && fullName) {
	    *fullName = ckalloc(strlen(name) + 1) ;
	    strcpy(*fullName, name) ;
	}
    } else {
	/*
	 * Relative name. First try the current namespace.
	 */
	Tcl_DString resolve ;
	int globalName ;
	const char *resolvedName ;

	globalName = relvarGetNamespaceName(interp, name, &resolve) ;
	resolvedName = Tcl_DStringValue(&resolve) ;
	relvar = Ral_RelvarFind(info, resolvedName) ;
	/*
	 * Check if we found the relvar by the namespace name. If not and
	 * we were not in the global namespace, then we have to try the
	 * global one. This matches the normal rules of Tcl name resolution.
	 */
	if (relvar == NULL && !globalName) {
	    Tcl_DStringFree(&resolve) ;
	    Tcl_DStringInit(&resolve) ;
	    Tcl_DStringAppend(&resolve, "::", -1) ;
	    Tcl_DStringAppend(&resolve, name, -1) ;
	    resolvedName = Tcl_DStringValue(&resolve) ;
	    relvar = Ral_RelvarFind(info, resolvedName) ;
	}

	if (relvar && fullName) {
	    *fullName = ckalloc(strlen(resolvedName) + 1) ;
	    strcpy(*fullName, resolvedName) ;
	}
	Tcl_DStringFree(&resolve) ;
    }

    if (relvar == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptNone,
	    RAL_ERR_UNKNOWN_NAME, name) ;
    }
    return relvar ;
}

int
Ral_RelvarObjInsertTuple(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Tcl_Obj *nameValueObj,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation relation ;
    Ral_Tuple tuple ;
    int result = TCL_OK ;
    Tcl_Obj *resultObj ;

    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    /*
     * Make the new tuple refer to the heading contained in the relation.
     */
    tuple = Ral_TupleNew(relation->heading->tupleHeading) ;
    /*
     * Set the values of the attributes from the list of attribute / value
     * pairs.
     */
    if (Ral_TupleSetFromObj(tuple, interp, nameValueObj, errInfo) != TCL_OK) {
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }
    /*
     * Run the traces and get the result back.
     */
    resultObj = Ral_RelvarObjExecInsertTraces(interp, relvar,
	Ral_TupleObjNew(tuple)) ;
    if (resultObj) {
	/*
	 * Objects returned have already been converted to tuples.
	 * Insert the tuple into the relation.
	 */
	tuple = resultObj->internalRep.otherValuePtr ;
	if (!Ral_RelationPushBack(relation, tuple, NULL)) {
	    Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_DUPLICATE_TUPLE,
		resultObj) ;
	    Ral_InterpSetError(interp, errInfo) ;
	    result = TCL_ERROR ;
	}
    } else {
	result = TCL_ERROR ;
    }

    return result ;
}

int
Ral_RelvarObjUpdateTuple(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Ral_Relation relation,
    Ral_RelationIter tupleIter,
    Tcl_Obj *scriptObj,
    Tcl_Obj *tupleVarNameObj,
    Ral_ErrorInfo *errInfo)
{
    int result ;
    Tcl_Obj *newTupleObj ;
    Tcl_Obj *oldTupleObj ;
    Tcl_Obj *resultTupleObj ;
    /*
     * Evaluate the script.
     */
    result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
    if (result == TCL_ERROR) {
	static const char msgfmt[] =
	    "\n    (\"in ::ral::%s %s\" body line %d)" ;
	char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE + 50] ;

	sprintf(msg, msgfmt, Ral_ErrorInfoGetCommand(errInfo),
	    Ral_ErrorInfoGetOption(errInfo), interp->errorLine) ;
	Tcl_AddObjErrorInfo(interp, msg, -1) ;
	return TCL_ERROR ;
    } else if (result > TCL_CONTINUE) {
	return result ;
    }
    /*
     * Fetch the value of the tuple variable. It could be different now that
     * the script has been executed. Once we get the new tuple value, we can
     * use it to update the relvar.
     */
    newTupleObj = Tcl_ObjGetVar2(interp, tupleVarNameObj, NULL,
	TCL_LEAVE_ERR_MSG) ;
    if (newTupleObj == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, newTupleObj, &Ral_TupleObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    assert(newTupleObj->typePtr == &Ral_TupleObjType) ;
    Tcl_IncrRefCount(newTupleObj) ;
    /*
     * Create an object out of the old tuple value.
     */
    oldTupleObj = Ral_TupleObjNew(*tupleIter) ;
    Tcl_IncrRefCount(oldTupleObj) ;
    /*
     * Call all the traces handing the old tuple value and the new
     * tuple value.
     */
    resultTupleObj = Ral_RelvarObjExecUpdateTraces(interp, relvar, oldTupleObj,
	newTupleObj) ;
    if (resultTupleObj == NULL) {
	return TCL_ERROR ;
    }
    result = Ral_RelationUpdateTupleObj(relation, tupleIter, interp,
	    resultTupleObj, errInfo) != TCL_OK ? TCL_ERROR : result ;

    Tcl_DecrRefCount(oldTupleObj) ;
    Tcl_DecrRefCount(newTupleObj) ;
    return result ;
}

int
Ral_RelvarObjCreateAssoc(
    Tcl_Interp *interp,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo info)
{
    /*
     * name relvar1 attr-list1 spec1 relvar2 attr-list2 spec2
     */

    Ral_Relvar relvar1 ;
    Ral_Relation r1 ;
    int elemc1 ;
    Tcl_Obj **elemv1 ;
    int specIndex1 ;
    Ral_Relvar relvar2 ;
    Ral_Relation r2 ;
    int elemc2 ;
    Tcl_Obj **elemv2 ;
    int specIndex2 ;
    const char *name ;
    Tcl_DString resolve ;
    Ral_JoinMap refMap ;
    Ral_TupleHeading th1 ;
    Ral_TupleHeading th2 ;
    Ral_IntVector refAttrs ;
    int isNotId ;
    Ral_Constraint constraint ;
    Ral_AssociationConstraint assoc ;
    int result = TCL_OK ;
    Tcl_DString errMsg ;

    /*
     * Creating an association is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptAssociation,
	    RAL_ERR_BAD_TRANS_OP, "association") ;
	return TCL_ERROR ;
    }
    /*
     * Look up the relvars and make sure that the values are truly a
     * relation.
     */
    relvar1 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[1]),
	NULL) ;
    if (relvar1 == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar1->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = relvar1->relObj->internalRep.otherValuePtr ;
    /*
     * Get the elements from the attribute list.
     */
    if (Tcl_ListObjGetElements(interp, objv[2], &elemc1, &elemv1) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Check the association specifier.
     */
    if (Tcl_GetIndexFromObjStruct(interp, objv[3], specTable,
	sizeof(specTable[0]), specErrMsg, 0, &specIndex1) != TCL_OK) {
	return TCL_ERROR ;
    }

    relvar2 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[4]),
	NULL) ;
    if (relvar2 == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar2->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = relvar2->relObj->internalRep.otherValuePtr ;
    if (Tcl_ListObjGetElements(interp, objv[5], &elemc2, &elemv2) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[6], specTable,
	sizeof(specTable[0]), specErrMsg, 0, &specIndex2) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * The referred to multiplicity is cannot be many.
     */
    if (specTable[specIndex2].multiplicity) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptAssociation,
	    RAL_ERR_BAD_MULT, objv[6]) ;
	return TCL_ERROR ;
    }

    /*
     * The same number of attributes must be specified on each side of the
     * association.
     */
    if (elemc1 != elemc2) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptAssociation,
	    RAL_ERR_REFATTR_MISMATCH, objv[5]) ;
	return TCL_ERROR ;
    }

    /*
     * Construct the mapping of attributes in the referring relation
     * to those in the referred to relation.
     */
    refMap = Ral_JoinMapNew(0, 0) ;
    th1 = r1->heading->tupleHeading ;
    th2 = r2->heading->tupleHeading ;
    while (elemc1-- > 0) {
	int attrIndex1 = Ral_TupleHeadingIndexOf(th1, Tcl_GetString(*elemv1)) ;
	int attrIndex2 = Ral_TupleHeadingIndexOf(th2, Tcl_GetString(*elemv2)) ;

	if (attrIndex1 < 0) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptAssociation,
		RAL_ERR_UNKNOWN_ATTR, *elemv1) ;
	    Ral_JoinMapDelete(refMap) ;
	    return TCL_ERROR ;
	}
	if (attrIndex2 < 0) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptAssociation,
		RAL_ERR_UNKNOWN_ATTR, *elemv2) ;
	    Ral_JoinMapDelete(refMap) ;
	    return TCL_ERROR ;
	}
	Ral_JoinMapAddAttrMapping(refMap, attrIndex1, attrIndex2) ;
	++elemv1 ;
	++elemv2 ;
    }
    /*
     * The referred to attributes in the second relvar must constitute an
     * identifier of the relation.
     */
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    isNotId = Ral_RelationHeadingFindIdentifier(r2->heading, refAttrs) < 0 ;
    Ral_IntVectorDelete(refAttrs) ;
    if (isNotId) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptAssociation,
	    RAL_ERR_NOT_AN_IDENTIFIER, objv[5]) ;
	Ral_JoinMapDelete(refMap) ;
	return TCL_ERROR ;
    }
    /*
     * Sort the join map attributes to match the identifier order.
     */
    Ral_JoinMapSortAttr(refMap, 1) ;

    name = relvarResolveName(interp, Tcl_GetString(objv[0]), &resolve) ;
    constraint = Ral_ConstraintAssocCreate(name, info) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptAssociation,
	    RAL_ERR_DUP_CONSTRAINT, name) ;
	Ral_JoinMapDelete(refMap) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    assoc = constraint->association ;
    assoc->referringRelvar = relvar1 ;
    assoc->referringCond = specTable[specIndex1].conditionality ;
    assoc->referringMult = specTable[specIndex1].multiplicity ;
    assoc->referredToRelvar = relvar2 ;
    assoc->referredToCond = specTable[specIndex2].conditionality ;
    assoc->referredToMult = specTable[specIndex2].multiplicity ;
    assoc->referenceMap = refMap ;
    /*
     * Record which constraints apply to a given relvar. This is what allows
     * us to find the constraints that apply to a relvar when it is modified.
     */
    Ral_PtrVectorPushBack(relvar1->constraints, constraint) ;
    Ral_PtrVectorPushBack(relvar2->constraints, constraint) ;
    /*
     * Evaluate the newly created constraint to make sure that the
     * current values of the relvar pass the constraint evaluation.
     * If not, we delete the constraint because the state of the
     * relvar values is not correct.
     */
    Tcl_DStringInit(&errMsg) ;
    if (!Ral_RelvarConstraintEval(constraint, &errMsg)) {
	int status ;

	Tcl_DStringResult(interp, &errMsg) ;
	status = Ral_ConstraintDeleteByName(name, info) ;
	assert(status != 0) ;

	result = TCL_ERROR ;
    }

    Tcl_DStringFree(&resolve) ;
    Tcl_DStringFree(&errMsg) ;
    return result ;
}

int
Ral_RelvarObjCreatePartition(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo info)
{
    /*
     * name super super-attrs sub1 sub1-attrs
     * ?sub2 sub2-attrs sub3 sub3-attrs ...?
     */
    char const *superName ;
    Ral_Relvar super ;
    Ral_Relation superRel ;
    int supElemc ;
    Tcl_Obj **supElemv ;
    Ral_IntVector superAttrs ;
    int nSupAttrs ;
    Ral_TupleHeading supth ;
    const char *partName ;
    Tcl_DString resolve ;
    Tcl_DString errMsg ;
    Ral_Constraint constraint ;
    Ral_PartitionConstraint partition ;
    Ral_PtrVector subList ;	/* set of sub type names */
    int result = TCL_OK ;

    /*
     * Creating a partition is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptPartition,
	    RAL_ERR_BAD_TRANS_OP, "partition") ;
	return TCL_ERROR ;
    }
    /*
     * Look up the supertype and make sure that the value is truly a relation.
     */
    superName = Tcl_GetString(objv[1]) ;
    super = Ral_RelvarObjFindRelvar(interp, info, superName, NULL) ;
    if (super == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, super->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    superRel = super->relObj->internalRep.otherValuePtr ;
    /*
     * Get the elements from the super type attribute list.
     */
    if (Tcl_ListObjGetElements(interp, objv[2], &supElemc, &supElemv)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Gather the attribute indices for the super type into a vector that we
     * can use later.  Verify that the supertype attributes constitute an
     * identifier.
     */
    superAttrs = Ral_IntVectorNewEmpty(supElemc) ;
    supth = superRel->heading->tupleHeading ;
    while (supElemc-- > 0) {
	const char *attrName = Tcl_GetString(*supElemv++) ;
	int attrIndex = Ral_TupleHeadingIndexOf(supth, attrName) ;
	int status ;

	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptPartition,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(superAttrs) ;
	    return TCL_ERROR ;
	}
	status = Ral_IntVectorSetAdd(superAttrs, attrIndex) ;
	if (!status) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptPartition,
		RAL_ERR_DUP_ATTR_IN_ID, attrName) ;
	    Ral_IntVectorDelete(superAttrs) ;
	    return TCL_ERROR ;
	}
    }
    if (Ral_RelationHeadingFindIdentifier(superRel->heading, superAttrs) < 0) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptPartition,
	    RAL_ERR_NOT_AN_IDENTIFIER, objv[2]) ;
	Ral_IntVectorDelete(superAttrs) ;
	return TCL_ERROR ;
    }
    nSupAttrs = Ral_IntVectorSize(superAttrs) ;
    /*
     * Create the partition constraint.
     */
    partName = relvarResolveName(interp, Tcl_GetString(objv[0]), &resolve) ;
    constraint = Ral_ConstraintPartitionCreate(partName, info) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptPartition,
	    RAL_ERR_DUP_CONSTRAINT, objv[0]) ;
	Ral_IntVectorDelete(superAttrs) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    partition = constraint->partition ;
    partition->referredToRelvar = super ;

    Ral_PtrVectorPushBack(super->constraints, constraint) ;
    /*
     * Loop over the subtype relations and attributes.
     */
    subList = Ral_PtrVectorNew(1) ;
    assert ((objc - 3) % 2 == 0) ;
    for (objc -= 3, objv += 3 ; objc > 0 ; objc -= 2, objv += 2) {
	char *subName = Tcl_GetString(objv[0]) ;
	Ral_Relvar sub ;
	Ral_Relation subRel ;
	int subElemc ;
	Tcl_Obj **subElemv ;
	Ral_TupleHeading subth ;
	Ral_SubsetReference subRef ;
	Ral_JoinMap refMap ;
	Ral_IntVectorIter supAttrIter ;
	/*
	 * Make sure the sub type is not the super type.
	 */
	if (strcmp(superName, subName) == 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptPartition,
		RAL_ERR_SUPER_NAME, superName) ;
	    goto errorOut ;
	}
	/*
	 * Get the subtype relvar and the attributes.
	 */
	if (!Ral_PtrVectorSetAdd(subList, subName)) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptPartition,
		RAL_ERR_DUP_NAME, subName) ;
	    goto errorOut ;
	}
	sub = Ral_RelvarObjFindRelvar(interp, info, subName, NULL) ;
	if (sub == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ConvertToType(interp, sub->relObj, &Ral_RelationObjType)
	    != TCL_OK) {
	    goto errorOut ;
	}
	subRel = sub->relObj->internalRep.otherValuePtr ;
	/*
	 * Get the elements from the sub type attribute list.
	 */
	if (Tcl_ListObjGetElements(interp, objv[1], &subElemc, &subElemv)
	    != TCL_OK) {
	    goto errorOut ;
	}
	if (nSupAttrs != subElemc) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptPartition,
		RAL_ERR_REFATTR_MISMATCH, objv[1]) ;
	    goto errorOut ;
	}
	/*
	 * Construct the mapping of attributes in the super type to
	 * to those in the subtype relation. In this case,
	 * the subtype is the "referring" relation and the supertype is
	 * the "referred to" relation.
	 */
	subRef = (Ral_SubsetReference)ckalloc(sizeof(*subRef)) ;
	subRef->relvar = sub ;
	subRef->subsetMap = refMap = Ral_JoinMapNew(0, 0) ;
	Ral_PtrVectorPushBack(partition->subsetReferences, subRef) ;
	subth = subRel->heading->tupleHeading ;
	supAttrIter = Ral_IntVectorBegin(superAttrs) ;
	while (subElemc-- > 0) {
	    const char *attrName = Tcl_GetString(*subElemv++) ;
	    int attrIndex = Ral_TupleHeadingIndexOf(subth, attrName) ;

	    if (attrIndex < 0) {
		Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptPartition,
		    RAL_ERR_UNKNOWN_ATTR, attrName) ;
		goto errorOut ;
	    }
	    Ral_JoinMapAddAttrMapping(refMap, attrIndex, *supAttrIter++) ;
	}
	/*
	 * Sort the join map based on the super type identifier.
	 */
	Ral_JoinMapSortAttr(refMap, 1) ;
	Ral_PtrVectorPushBack(sub->constraints, constraint) ;
    }

    Tcl_DStringInit(&errMsg) ;
    if (!Ral_RelvarConstraintEval(constraint, &errMsg)) {
	int status ;

	Tcl_DStringResult(interp, &errMsg) ;
	status = Ral_ConstraintDeleteByName(partName, info) ;
	assert(status != 0) ;

	result = TCL_ERROR ;
    }

    Ral_IntVectorDelete(superAttrs) ;
    Ral_PtrVectorDelete(subList) ;
    Tcl_DStringFree(&errMsg) ;
    Tcl_DStringFree(&resolve) ;
    return result ;

errorOut:
    Ral_IntVectorDelete(superAttrs) ;
    Ral_PtrVectorDelete(subList) ;
    Ral_ConstraintDeleteByName(partName, info) ;
    Tcl_DStringFree(&resolve) ;
    return TCL_ERROR ;
}

int
Ral_RelvarObjCreateCorrelation(
    Tcl_Interp *interp,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo info)
{
    /*
     * ?-complete? name corrRelvar corr-attr1-list spec1 relvar1 attr-list1
     *                             corr-attr2-list spec2 relvar2 attr-list2
     */

    int complete = 0 ;
    Ral_Relvar relvarC ;
    Ral_Relation rC ;
    int elemcC1 ;
    Tcl_Obj **elemvC1 ;
    int elemcC2 ;
    Tcl_Obj **elemvC2 ;

    Ral_Relvar relvar1 ;
    Ral_Relation r1 ;
    int elemc1 ;
    Tcl_Obj **elemv1 ;
    int specIndex1 ;

    Ral_Relvar relvar2 ;
    Ral_Relation r2 ;
    int elemc2 ;
    Tcl_Obj **elemv2 ;
    int specIndex2 ;

    const char *name ;
    Tcl_DString resolve ;
    Ral_JoinMap refMap ;
    Ral_TupleHeading thC ;
    Ral_TupleHeading th1 ;
    Ral_TupleHeading th2 ;
    Ral_IntVector refAttrs ;
    int isNotId ;
    Ral_Constraint constraint ;
    Ral_CorrelationConstraint correl ;
    int result = TCL_OK ;
    Tcl_DString errMsg ;

    /*
     * Creating a correlation is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_BAD_TRANS_OP, "correlation") ;
	return TCL_ERROR ;
    }
    /*
     * Check if we got the "-complete" option.
     */
    if (strcmp(Tcl_GetString(objv[0]), "-complete") == 0) {
	complete = 1 ;
	++objv ;
    }
    /*
     * Look up the correlation relvar.
     */
    relvarC = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[1]),
	NULL) ;
    if (relvarC == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvarC->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    rC = relvarC->relObj->internalRep.otherValuePtr ;
    /*
     * Get the correlation attribute lists.
     */
    if (Tcl_ListObjGetElements(interp, objv[2], &elemcC1, &elemvC1) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_ListObjGetElements(interp, objv[6], &elemcC2, &elemvC2) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Check the correlation specifiers.
     */
    if (Tcl_GetIndexFromObjStruct(interp, objv[3], specTable,
	sizeof(specTable[0]), specErrMsg, 0, &specIndex1) != TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Check the spec against "complete". If "complete" was specified
     * then the correlation spec must be "+".
     */
    if (complete && (specTable[specIndex1].conditionality == 0 ||
	specTable[specIndex1].multiplicity == 0)) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_INCOMPLETE_SPEC, objv[3]) ;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[7], specTable,
	sizeof(specTable[0]), specErrMsg, 0, &specIndex2) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (complete && (specTable[specIndex2].conditionality == 0 ||
	specTable[specIndex2].multiplicity == 0)) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_INCOMPLETE_SPEC, objv[7]) ;
    }
    /*
     * Look up relvar 1.
     */
    relvar1 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[4]),
	NULL) ;
    if (relvar1 == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar1->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = relvar1->relObj->internalRep.otherValuePtr ;
    /*
     * Get the elements from the attribute list.
     */
    if (Tcl_ListObjGetElements(interp, objv[5], &elemc1, &elemv1) != TCL_OK) {
	return TCL_ERROR ;
    }

    relvar2 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[8]),
	NULL) ;
    if (relvar2 == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar2->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = relvar2->relObj->internalRep.otherValuePtr ;
    if (Tcl_ListObjGetElements(interp, objv[9], &elemc2, &elemv2) != TCL_OK) {
	return TCL_ERROR ;
    }

    /*
     * The same number of attributes must be specified on each side of the
     * association.
     */
    if (elemc1 != elemcC1) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_REFATTR_MISMATCH, objv[5]) ;
	return TCL_ERROR ;
    }
    if (elemc2 != elemcC2) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_REFATTR_MISMATCH, objv[9]) ;
	return TCL_ERROR ;
    }

    /*
     * Create the correlation constraint data structure and fill in
     * what we know.
     */
    name = relvarResolveName(interp, Tcl_GetString(objv[0]), &resolve) ;
    constraint = Ral_ConstraintCorrelationCreate(name, info) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_DUP_CONSTRAINT, name) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    correl = constraint->correlation ;
    correl->referringRelvar = relvarC ;
    correl->aRefToRelvar = relvar1 ;
    correl->aCond = specTable[specIndex1].conditionality ;
    correl->aMult = specTable[specIndex1].multiplicity ;
    correl->aReferenceMap = Ral_JoinMapNew(0, 0) ;
    correl->bRefToRelvar = relvar2 ;
    correl->bCond = specTable[specIndex2].conditionality ;
    correl->bMult = specTable[specIndex2].multiplicity ;
    correl->bReferenceMap = Ral_JoinMapNew(0, 0) ;
    correl->complete = complete ;
    /*
     * Now we construct two mappings. First from the correlation relvar to
     * relvar 1 and then from the correlation relvar to relvar 2.
     * The attributes in relvar 1 and relvar 2 must be identifiers.
     * The attributes in the correlation relvar are the referring attributes.
     */
    thC = rC->heading->tupleHeading ;
    th1 = r1->heading->tupleHeading ;
    refMap = correl->aReferenceMap ;
    while (elemc1-- > 0) {
	int attrIndexC = Ral_TupleHeadingIndexOf(thC, Tcl_GetString(*elemvC1)) ;
	int attrIndex1 = Ral_TupleHeadingIndexOf(th1, Tcl_GetString(*elemv1)) ;

	if (attrIndexC < 0) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
		RAL_ERR_UNKNOWN_ATTR, *elemvC1) ;
	    goto errorOut ;
	}
	if (attrIndex1 < 0) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
		RAL_ERR_UNKNOWN_ATTR, *elemv1) ;
	    goto errorOut ;
	}
	Ral_JoinMapAddAttrMapping(refMap, attrIndexC, attrIndex1) ;
	++elemvC1 ;
	++elemv1 ;
    }
    /*
     * The referred to attributes in the second relvar must constitute an
     * identifier of the relation.
     */
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    isNotId = Ral_RelationHeadingFindIdentifier(r1->heading, refAttrs) < 0 ;
    Ral_IntVectorDelete(refAttrs) ;
    if (isNotId) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_NOT_AN_IDENTIFIER, objv[5]) ;
	goto errorOut ;
    }
    /*
     * Sort the join map attributes to match the identifier order.
     */
    Ral_JoinMapSortAttr(refMap, 1) ;

    refMap = correl->bReferenceMap ;
    th2 = r2->heading->tupleHeading ;
    while (elemc2-- > 0) {
	int attrIndexC = Ral_TupleHeadingIndexOf(thC, Tcl_GetString(*elemvC2)) ;
	int attrIndex2 = Ral_TupleHeadingIndexOf(th2, Tcl_GetString(*elemv2)) ;

	if (attrIndexC < 0) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
		RAL_ERR_UNKNOWN_ATTR, *elemvC2) ;
	    goto errorOut ;
	}
	if (attrIndex2 < 0) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
		RAL_ERR_UNKNOWN_ATTR, *elemv2) ;
	    goto errorOut ;
	}
	Ral_JoinMapAddAttrMapping(refMap, attrIndexC, attrIndex2) ;
	++elemvC2 ;
	++elemv2 ;
    }
    /*
     * The referred to attributes in the second relvar must constitute an
     * identifier of the relation.
     */
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    isNotId = Ral_RelationHeadingFindIdentifier(r2->heading, refAttrs) < 0 ;
    Ral_IntVectorDelete(refAttrs) ;
    if (isNotId) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_NOT_AN_IDENTIFIER, objv[9]) ;
	goto errorOut ;
    }
    /*
     * Sort the join map attributes to match the identifier order.
     */
    Ral_JoinMapSortAttr(refMap, 1) ;

    /*
     * Record which constraints apply to a given relvar. This is what allows
     * us to find the constraints that apply to a relvar when it is modified.
     */
    Ral_PtrVectorPushBack(relvar1->constraints, constraint) ;
    Ral_PtrVectorPushBack(relvar2->constraints, constraint) ;
    Ral_PtrVectorPushBack(relvarC->constraints, constraint) ;
    /*
     * Evaluate the newly created constraint to make sure that the
     * current values of the relvar pass the constraint evaluation.
     * If not, we delete the constraint because the state of the
     * relvar values is not correct.
     */
    Tcl_DStringInit(&errMsg) ;
    if (!Ral_RelvarConstraintEval(constraint, &errMsg)) {
	int status ;

	Tcl_DStringResult(interp, &errMsg) ;
	status = Ral_ConstraintDeleteByName(name, info) ;
	assert(status != 0) ;

	result = TCL_ERROR ;
    }

    Tcl_DStringFree(&resolve) ;
    Tcl_DStringFree(&errMsg) ;
    return result ;

errorOut:
    Ral_ConstraintDeleteByName(name, info) ;
    Tcl_DStringFree(&resolve) ;
    return TCL_ERROR ;
}

int
Ral_RelvarObjConstraintDelete(
    Tcl_Interp *interp,
    const char *name,
    Ral_RelvarInfo info)
{
    Ral_Constraint constraint ;
    char *fullName ;
    int status ;

    constraint = Ral_RelvarObjFindConstraint(interp, info, name, &fullName) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptConstraint,
	    RAL_ERR_UNKNOWN_CONSTRAINT, name) ;
	return TCL_ERROR ;
    }
    status = Ral_ConstraintDeleteByName(fullName, info) ;
    assert(status != 0) ;
    ckfree(fullName) ;
    return TCL_OK ;
}

int
Ral_RelvarObjConstraintInfo(
    Tcl_Interp *interp,
    Tcl_Obj * const nameObj,
    Ral_RelvarInfo info)
{
    Ral_Constraint constraint ;
    char *fullName ;
    Tcl_Obj *resultObj ;
    Tcl_Obj *attrList ;

    /*
     * Look up the constraint by it name.
     */
    constraint = Ral_RelvarObjFindConstraint(interp, info,
	Tcl_GetString(nameObj), &fullName) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptConstraint,
	    RAL_ERR_UNKNOWN_CONSTRAINT, nameObj) ;
	return TCL_ERROR ;
    }

    resultObj = Tcl_NewListObj(0, 0) ;

    switch (constraint->type) {
    case ConstraintAssociation:
    {
	/* association name relvar1 attr-list1 spec1
	 * relvar2 attr-list2 spec2
	 */
	Ral_AssociationConstraint assoc = constraint->association ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("association", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(fullName, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	/*
	 * The referring side.
	 */
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(assoc->referringRelvar->name, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	attrList = relvarConstraintAttrNames(interp, assoc->referringRelvar,
	    assoc->referenceMap, 0) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    relvarAssocSpec(assoc->referringCond, assoc->referringMult))
	    != TCL_OK) {
	    goto errorOut ;
	}

	/*
	 * The referred to side.
	 */
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(assoc->referredToRelvar->name, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	attrList = relvarConstraintAttrNames(interp, assoc->referredToRelvar,
	    assoc->referenceMap, 1) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    relvarAssocSpec(assoc->referredToCond, assoc->referredToMult))
	    != TCL_OK) {
	    goto errorOut ;
	}
    }
	break ;

    case ConstraintPartition:
    {
	/*
	 * partition name super super-attrs sub1 sub1-attrs
	 * ?sub2 sub2-attrs sub3 sub3-attrs ...?
	 */
	Ral_PartitionConstraint partition = constraint->partition ;
	Ral_PtrVectorIter refEnd =
	    Ral_PtrVectorEnd(partition->subsetReferences) ;
	Ral_PtrVectorIter refIter ;
	Ral_SubsetReference subRef ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("partition", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(fullName, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	/*
	 * Super type side.
	 * We need to reach in and get one of the subtype join maps.
	 * The supertype attributes are the same for each subtype reference
	 * and are at offset 1 (since they are the referred to attributes).
	 */
	assert(Ral_PtrVectorSize(partition->subsetReferences) > 0) ;
	refIter = Ral_PtrVectorBegin(partition->subsetReferences) ;
	subRef = *refIter ;
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(partition->referredToRelvar->name, -1))
	    != TCL_OK) {
	    goto errorOut ;
	}
	attrList = relvarConstraintAttrNames(interp,
	    partition->referredToRelvar, subRef->subsetMap, 1) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
	/*
	 * Loop over the sub types and add them to the result.
	 */
	for ( ; refIter != refEnd ; ++refIter) {
	    subRef = *refIter ;
	    if (Tcl_ListObjAppendElement(interp, resultObj,
		Tcl_NewStringObj(subRef->relvar->name, -1)) != TCL_OK) {
		goto errorOut ;
	    }
	    attrList = relvarConstraintAttrNames(interp, subRef->relvar,
		subRef->subsetMap, 0) ;
	    if (attrList == NULL) {
		goto errorOut ;
	    }
	    if (Tcl_ListObjAppendElement(interp, resultObj, attrList)
		!= TCL_OK) {
		goto errorOut ;
	    }
	}
    }
	break ;

    case ConstraintCorrelation:
    {
	/* correlation ?-complete? name relvarC
	 *	attr-listC1 spec1 relvar1 attr-list1
	 *	attr-listC2 spec2 relvar2 attr-list2
	 */
	Ral_CorrelationConstraint correl = constraint->correlation ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("correlation", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (correl->complete && Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("-complete", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(fullName, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	/*
	 * Correlation relvar name
	 */
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(correl->referringRelvar->name, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	/*
	 * The "A" side.
	 */
	attrList = relvarConstraintAttrNames(interp, correl->referringRelvar,
	    correl->aReferenceMap, 0) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    relvarAssocSpec(correl->aCond, correl->aMult)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(correl->aRefToRelvar->name, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	attrList = relvarConstraintAttrNames(interp, correl->aRefToRelvar,
	    correl->aReferenceMap, 1) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
	/*
	 * The "B" side.
	 */
	attrList = relvarConstraintAttrNames(interp, correl->referringRelvar,
	    correl->bReferenceMap, 0) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    relvarAssocSpec(correl->bCond, correl->bMult)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(correl->bRefToRelvar->name, -1)) != TCL_OK) {
	    goto errorOut ;
	}
	attrList = relvarConstraintAttrNames(interp, correl->bRefToRelvar,
	    correl->bReferenceMap, 1) ;
	if (attrList == NULL) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj, attrList) != TCL_OK) {
	    goto errorOut ;
	}
    }
	break ;

    default:
	Tcl_Panic("Ral_RelvarObjConstraintInfo: unknown constraint type, %d",
	    constraint->type) ;
    }

    ckfree(fullName) ;
    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(resultObj) ;
    ckfree(fullName) ;
    return TCL_ERROR ;
}

int
Ral_RelvarObjConstraintNames(
    Tcl_Interp *interp,
    const char *pattern,
    Ral_RelvarInfo info)
{
    Tcl_Obj *nameList ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;

    nameList = Tcl_NewListObj(0, NULL) ;
    for (entry = Tcl_FirstHashEntry(&info->constraints, &search) ; entry ;
	entry = Tcl_NextHashEntry(&search)) {
	const char *name = (const char *)Tcl_GetHashKey(&info->constraints,
	    entry) ;

	if (Tcl_StringMatch(name, pattern) &&
	    Tcl_ListObjAppendElement(interp, nameList,
		Tcl_NewStringObj(name, -1)) != TCL_OK) {
	    Tcl_DecrRefCount(nameList) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, nameList) ;
    return TCL_OK ;
}

/*
 * returns an interpreter result that is the list of constraints
 * that a relvar is a member of.
 */
int
Ral_RelvarObjConstraintMember(
    Tcl_Interp *interp,
    Tcl_Obj * const relvarName,
    Ral_RelvarInfo info)
{
    Ral_Relvar relvar ;
    Tcl_Obj *resultObj ;
    Ral_PtrVectorIter iter ;
    Ral_PtrVectorIter end ;

    relvar = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(relvarName),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }

    resultObj = Tcl_NewListObj(0, NULL) ;
    end = Ral_PtrVectorEnd(relvar->constraints) ;
    for (iter = Ral_PtrVectorBegin(relvar->constraints) ; iter != end ;
	++iter) {
	Ral_Constraint c = *iter ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(c->name, -1)) != TCL_OK) {
	    Tcl_DecrRefCount(resultObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

int
Ral_RelvarObjConstraintPath(
    Tcl_Interp *interp,
    Tcl_Obj * const constraintName,
    Ral_RelvarInfo info)
{
    Ral_Constraint constraint ;
    char *name = Tcl_GetString(constraintName) ;
    char *fullName ;

    constraint = Ral_RelvarObjFindConstraint(interp, info, name, &fullName) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptConstraint,
	    RAL_ERR_UNKNOWN_CONSTRAINT, name) ;
	return TCL_ERROR ;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(fullName, -1)) ;
    ckfree(fullName) ;
    return TCL_OK ;
}

int
Ral_RelvarObjEndTrans(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    int failed)
{
    Tcl_DString errMsg ;
    int success ;

    Tcl_DStringInit(&errMsg) ;
    success = Ral_RelvarEndTransaction(info, failed, &errMsg) ;
    if (!(failed || success)) {
	Tcl_DStringResult(interp, &errMsg) ;
    }
    Tcl_DStringFree(&errMsg) ;

    return success ? TCL_OK : TCL_ERROR ;
}

int
Ral_RelvarObjEndCmd(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    int failed)
{
    Tcl_DString errMsg ;
    int success ;

    Tcl_DStringInit(&errMsg) ;
    success = Ral_RelvarEndCommand(info, failed, &errMsg) ;
    if (!(failed || success)) {
	Tcl_DStringResult(interp, &errMsg) ;
    }
    Tcl_DStringFree(&errMsg) ;

    return success ? TCL_OK : TCL_ERROR ;
}

int
Ral_RelvarObjTraceVarAdd(
    Tcl_Interp * interp,
    Ral_Relvar relvar,
    Tcl_Obj *const traceOps,
    Tcl_Obj *const command)
{
    int flags ;

    if (Ral_RelvarObjDecodeTraceOps(interp, traceOps, &flags) != TCL_OK) {
	return TCL_ERROR ;
    }
    Ral_RelvarTraceAdd(relvar, flags, command) ;
    return TCL_OK ;
}

int
Ral_RelvarObjTraceVarRemove(
    Tcl_Interp * interp,
    Ral_Relvar relvar,
    Tcl_Obj *const traceOps,
    Tcl_Obj *const command)
{
    int flags ;

    if (Ral_RelvarObjDecodeTraceOps(interp, traceOps, &flags) != TCL_OK) {
	return TCL_ERROR ;
    }
    Ral_RelvarTraceRemove(relvar, flags, command) ;
    return TCL_OK ;
}

int
Ral_RelvarObjTraceVarInfo(
    Tcl_Interp *interp,
    Ral_Relvar relvar)
{
    Tcl_Obj *resultObj ;
    Ral_TraceInfo trace ;

    resultObj = Tcl_NewListObj(0, 0) ;

    for (trace = relvar->traces ; trace ; trace = trace->next) {
	/*
	 * Each info element in the result list is in turn a list
	 * of two elements, the flags and the command prefix.
	 */
	Tcl_Obj *flagObj ;
	Tcl_Obj *infoObj[2] ;

	if (Ral_RelvarObjEncodeTraceFlag(interp, trace->flags, &flagObj)
	    != TCL_OK) {
	    Tcl_DecrRefCount(resultObj) ;
	    return TCL_ERROR ;
	}
	infoObj[0] = flagObj ;
	infoObj[1] = trace->command ;
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewListObj(2, infoObj)) != TCL_OK) {
	    Tcl_DecrRefCount(resultObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

int
Ral_RelvarObjTraceEvalAdd(
    Tcl_Interp *interp,
    Ral_RelvarInfo rInfo,
    Tcl_Obj *const cmdPrefix)
{
    Ral_TraceInfo info = (Ral_TraceInfo)ckalloc(sizeof *info) ;

    info->flags = TRACEOP_EVAL_FLAG ;
    Tcl_IncrRefCount(info->command = cmdPrefix) ;
    /*
     * Chain the new trace onto the beginning of the list.
     */
    info->next = rInfo->traces ;
    rInfo->traces = info ;
    return TCL_OK ;
}

int
Ral_RelvarObjTraceEvalRemove(
    Tcl_Interp *interp,
    Ral_RelvarInfo rInfo,
    Tcl_Obj *const cmdPrefix)
{
    Ral_TraceInfo prev = NULL ;
    Ral_TraceInfo trace = rInfo->traces ;
    const char *cmdString = Tcl_GetString(cmdPrefix) ;
    int flags = TRACEOP_EVAL_FLAG ;

    /*
     * The traces are in a linked list so we must traverse the list
     * to find the trace. Since it is singly linked we must keep a trailing
     * pointer to use during relinking.
     */
    while (trace) {
	if (trace->flags == flags &&
	    strcmp(Tcl_GetString(trace->command), cmdString) == 0) {
	    /*
	     * Found a match. Remember the one to delete.
	     */
	    Ral_TraceInfo del = trace ;
	    if (prev) {
		/*
		 * Unlinking in the middle of the list.
		 */
		prev->next = trace->next ;
	    } else {
		/*
		 * Unlinking the first one in the list.
		 */
		rInfo->traces = trace->next ;
	    }
	    /*
	     * Point to the next list item. Do this before freeing
	     * the item itself since after the cleanup the "trace"
	     * pointer is invalid.
	     */
	    trace = trace->next ;
	    Tcl_DecrRefCount(del->command) ;
	    ckfree((char *)del) ;
	} else {
	    /*
	     * No match, advance the pointers along the list.
	     */
	    prev = trace ;
	    trace = trace->next ;
	}
    }
    return TCL_OK ;
}

int
Ral_RelvarObjTraceEvalInfo(
    Tcl_Interp *interp,
    Ral_RelvarInfo rInfo)
{
    Tcl_Obj *resultObj ;
    Ral_TraceInfo trace ;

    resultObj = Tcl_NewListObj(0, 0) ;

    for (trace = rInfo->traces ; trace ; trace = trace->next) {
	/*
	 * Each info element in the result list is just the command prefix.
	 */
	if (Tcl_ListObjAppendElement(interp, resultObj, trace->command)
	    != TCL_OK) {
	    Tcl_DecrRefCount(resultObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

int
Ral_RelvarObjExecDeleteTraces(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Tcl_Obj *delTuple)
{
    int result = TCL_OK ;
    if (relvar->traces && relvar->traceFlags == 0) {
	relvar->traceFlags = TRACEOP_DELETE_FLAG ;
	result = Ral_RelvarObjExecTraces(interp, relvar, NULL, delTuple, NULL) ?
	    TCL_OK : TCL_ERROR ;
	relvar->traceFlags = 0 ;
    }
    return result ;
}

Tcl_Obj *
Ral_RelvarObjExecInsertTraces(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Tcl_Obj *insertTuple)
{
    if (relvar->traces && relvar->traceFlags == 0) {
	relvar->traceFlags = TRACEOP_INSERT_FLAG ;
	insertTuple = Ral_RelvarObjExecTraces(interp, relvar, &Ral_TupleObjType,
	    insertTuple, NULL) ;
	relvar->traceFlags = 0 ;
    }
    return insertTuple ;
}

Tcl_Obj *
Ral_RelvarObjExecUpdateTraces(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Tcl_Obj *oldTuple,
    Tcl_Obj *newTuple)
{
    if (relvar->traces && relvar->traceFlags == 0) {
	relvar->traceFlags = TRACEOP_UPDATE_FLAG ;
	newTuple = Ral_RelvarObjExecTraces(interp, relvar, &Ral_TupleObjType,
	    oldTuple, newTuple) ;
	relvar->traceFlags = 0 ;
    }
    return newTuple ;
}

Tcl_Obj *
Ral_RelvarObjExecSetTraces(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Tcl_Obj *relationObj,
    Ral_ErrorInfo *errInfo)
{
    if (relvar->traces && relvar->traceFlags == 0) {
	relvar->traceFlags = TRACEOP_SET_FLAG ;
	relationObj = Ral_RelvarObjExecTraces(interp, relvar,
	    &Ral_RelationObjType, relationObj, NULL) ;
	relvar->traceFlags = 0 ;

	/*
	 * For set traces, the result heading must match the heading
	 * for the relvar.
	 */
	if (relationObj) {
	    Ral_Relation resultRel ;
	    Ral_Relation origRel ;
	    int result ;
	    /*
	     * Result is converted to the proper type by the trace function.
	     */
	    assert(relationObj->typePtr == &Ral_RelationObjType) ;
	    resultRel = relationObj->internalRep.otherValuePtr ;
	    /*
	     * Make sure the relvar value didn't simmer during the trace.
	     */
	    result = Tcl_ConvertToType(interp, relvar->relObj,
		&Ral_RelationObjType) ;
	    if (result != TCL_OK) {
		Tcl_DecrRefCount(relationObj) ;
		return NULL ;
	    }
	    origRel = relvar->relObj->internalRep.otherValuePtr ;
	    if (!Ral_RelationHeadingEqual(resultRel->heading,
		    origRel->heading)) {
		char *headingStr =
		    Ral_RelationHeadingStringOf(resultRel->heading) ;
		Ral_ErrorInfoSetError(errInfo, RAL_ERR_HEADING_NOT_EQUAL,
		    headingStr) ;
		Ral_InterpSetError(interp, errInfo) ;
		ckfree(headingStr) ;
		Tcl_DecrRefCount(relationObj) ;
		return NULL ;
	    }
	}
    }
    return relationObj ;
}

void
Ral_RelvarObjExecUnsetTraces(
    Tcl_Interp *interp,
    Ral_Relvar relvar)
{
    if (relvar->traces) {
	relvar->traceFlags = TRACEOP_UNSET_FLAG ;
	Ral_RelvarObjExecTraces(interp, relvar, NULL, NULL, NULL) ;
	relvar->traceFlags = 0 ;
    }
}

void
Ral_RelvarObjExecEvalTraces(
    Tcl_Interp *interp,
    Ral_RelvarInfo rInfo,
    int isBegin,
    int level)
{
    if (rInfo->traces) {
	Tcl_Obj *evalObj = Tcl_NewStringObj("eval", -1) ;
	Tcl_Obj *beginObj = isBegin ?
	    Tcl_NewStringObj("begin", -1) : Tcl_NewStringObj("end", -1) ;
	Tcl_Obj *levelObj = Tcl_NewIntObj(level) ;
	Ral_TraceInfo trace ;
	Tcl_Obj *resultObj ;

	Tcl_IncrRefCount(evalObj) ;
	Tcl_IncrRefCount(beginObj) ;
	Tcl_IncrRefCount(levelObj) ;
	/*
	 * Since we ignore any errors from eval traces, we preserve
	 * the current value of the interpreter result while executing
	 * the trace commands.
	 */
	resultObj = Tcl_GetObjResult(interp) ;
	Tcl_IncrRefCount(resultObj) ;

	for (trace = rInfo->traces ; trace ; trace = trace->next) {
	    Tcl_Obj *cmd = Tcl_NewListObj(0, NULL) ;

	    /*
	     * Compose the command from the prefix, begin keyword and level.
	     */
	    if (Tcl_ListObjAppendList(interp, cmd, trace->command) == TCL_OK &&
		Tcl_ListObjAppendElement(interp, cmd, evalObj) == TCL_OK &&
		Tcl_ListObjAppendElement(interp, cmd, beginObj) == TCL_OK &&
		Tcl_ListObjAppendElement(interp, cmd, levelObj) == TCL_OK) {
		int cmdc ;
		Tcl_Obj **cmdv ;
		/*
		 * Break out the list as a array of argments and evaluate it.
		 */
		if (Tcl_ListObjGetElements(interp, cmd, &cmdc, &cmdv)
		    == TCL_OK) {
		    Tcl_EvalObjv(interp, cmdc, cmdv, TCL_EVAL_DIRECT) ;
		}
		Tcl_DecrRefCount(cmd) ;
	    }
	}

	Tcl_DecrRefCount(evalObj) ;
	Tcl_DecrRefCount(beginObj) ;
	Tcl_DecrRefCount(levelObj) ;
	Tcl_SetObjResult(interp, resultObj) ;
	Tcl_DecrRefCount(resultObj) ;
    }
}

/*
 * PRIVATE FUNCTIONS
 */

static char *
relvarTraceProc(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *name1,
    const char *name2,
    int flags)
{
    char *result = NULL ;

    /*
     * For write tracing, the value has been changed, so we must
     * restore it and spit back an error message.
     */
    if (flags & TCL_TRACE_WRITES) {
	Ral_RelvarInfo info = (Ral_RelvarInfo) clientData ;
	Tcl_DString resolve ;
	const char *resolvedName =
	    relvarResolveName(interp, name1, &resolve) ;
	Ral_Relvar relvar = Ral_RelvarFind(info, resolvedName) ;
	Tcl_Obj *newValue ;

	assert(relvar != NULL) ;
	newValue = Tcl_SetVar2Ex(interp, resolvedName, NULL, relvar->relObj,
	    flags) ;
	Tcl_DStringFree(&resolve) ;
	/*
	 * Should not be modified because tracing is suspended while tracing.
	 */
	assert(newValue == relvar->relObj) ;
	result = "relvar may only be modified using \"::ral::relvar\" command" ;
    } else {
	Tcl_Panic("relvarTraceProc: trace on non-write, flags = %#x\n", flags) ;
    }

    return result ;
}

static const char *
relvarResolveName(
    Tcl_Interp *interp,
    const char *name,
    Tcl_DString *resolvedName)
{
    if (relvarNameIsAbsName(name)) {
	/*
	 * absolute reference.
	 */
	Tcl_DStringInit(resolvedName) ;
	Tcl_DStringAppend(resolvedName, name, -1) ;
    } else if(interp) {
	relvarGetNamespaceName(interp, name, resolvedName) ;
    }

    return Tcl_DStringValue(resolvedName) ;
}

static int
relvarNameIsAbsName(
    const char *name)
{
    return strlen(name) >= 2 && name[0] == ':' && name[1] == ':' ;
}

/*
 * Construct an absolute name from the current namespace.
 * Returns whether or not the current namespace is the global namespace.
 */
static int
relvarGetNamespaceName(
    Tcl_Interp *interp,
    const char *name,
    Tcl_DString *nsVarName)
{
    int isGlobal = 1 ;
    Tcl_Namespace *curr = Tcl_GetCurrentNamespace(interp) ;

    Tcl_DStringInit(nsVarName) ;
    if (curr->parentPtr) {
	Tcl_DStringAppend(nsVarName, curr->fullName, -1) ;
	isGlobal = 0 ;
    }
    Tcl_DStringAppend(nsVarName, "::", -1) ;
    Tcl_DStringAppend(nsVarName, name, -1) ;

    return isGlobal ;
}

/*
 * Return a list of attribute names that are part of the join map
 * at the given offset.
 */
static Tcl_Obj *
relvarConstraintAttrNames(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Ral_JoinMap map,
    int offset)
{
    Ral_Relation relation ;
    Ral_TupleHeading th ;
    Ral_IntVector attrIndices = Ral_JoinMapGetAttr(map, offset) ;
    Ral_IntVectorIter aEnd = Ral_IntVectorEnd(attrIndices) ;
    Ral_IntVectorIter aIter ;
    Tcl_Obj *nameList = Tcl_NewListObj(0, NULL) ;

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relation = relvar->relObj->internalRep.otherValuePtr ;
    th = relation->heading->tupleHeading ;

    for (aIter = Ral_IntVectorBegin(attrIndices) ; aIter != aEnd ; ++aIter) {
	Ral_Attribute attr = Ral_TupleHeadingFetch(th, *aIter) ;
	if (Tcl_ListObjAppendElement(interp, nameList,
	    Tcl_NewStringObj(attr->name, -1)) != TCL_OK) {
	    Tcl_DecrRefCount(nameList) ;
	    return NULL ;
	}
    }

    return nameList ;
}

static Tcl_Obj *
relvarAssocSpec(
    int cond,
    int mult)
{
    static char const * const condMultStrings[2][2] = {
	{"1", "+"},
	{"?", "*"}
    } ;
    assert (cond < 2) ;
    assert (mult < 2) ;
    return Tcl_NewStringObj(condMultStrings[cond][mult], -1) ;
}

/*
 * Just like RelvarObjFindRelvar, except for constraint names.
 * (1) If name is fully resolved, then try to match it directly.
 * (2) else construct a fully resolved name for the current namespace and
 *     try to find that.
 * (3) If that fails, construct a fully resolved name for the global
 *     namespace.
 * (4) If that fails, then it is an unknown relvar.
 */
static Ral_Constraint
Ral_RelvarObjFindConstraint(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    const char *name,
    char **fullName)
{
    Ral_Constraint constraint ;

    if (relvarNameIsAbsName(name)) {
	/*
	 * Absolute name.
	 */
	constraint = Ral_ConstraintFindByName(name, info) ;
	if (constraint && fullName) {
	    *fullName = ckalloc(strlen(name) + 1) ;
	    strcpy(*fullName, name) ;
	}
    } else {
	/*
	 * Relative name. First try the current namespace.
	 */
	Tcl_DString resolve ;
	int globalName ;
	const char *resolvedName ;

	globalName = relvarGetNamespaceName(interp, name, &resolve) ;
	resolvedName = Tcl_DStringValue(&resolve) ;
	constraint = Ral_ConstraintFindByName(resolvedName, info) ;
	/*
	 * Check if we found the constraint by the namespace name. If not and
	 * we were not in the global namespace, then we have to try the
	 * global one. This matches the normal rules of Tcl name resolution.
	 */
	if (constraint == NULL && !globalName) {
	    Tcl_DStringFree(&resolve) ;
	    Tcl_DStringInit(&resolve) ;
	    Tcl_DStringAppend(&resolve, "::", -1) ;
	    Tcl_DStringAppend(&resolve, name, -1) ;
	    resolvedName = Tcl_DStringValue(&resolve) ;
	    constraint = Ral_ConstraintFindByName(resolvedName, info) ;
	}

	if (constraint && fullName) {
	    *fullName = ckalloc(strlen(resolvedName) + 1) ;
	    strcpy(*fullName, resolvedName) ;
	}
	Tcl_DStringFree(&resolve) ;
    }

    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptNone,
	    RAL_ERR_UNKNOWN_NAME, name) ;
    }
    return constraint ;
}

static int
Ral_RelvarObjDecodeTraceOps(
    Tcl_Interp *interp,
    Tcl_Obj *opsList,
    int *flagPtr)
{
    int opsCount ;
    Tcl_Obj **opsVect ;
    int flags = 0 ;
    int index ;

    if (Tcl_ListObjGetElements(interp, opsList, &opsCount, &opsVect)
	!= TCL_OK) {
	return TCL_ERROR ;
    }

    /*
     * Must have at least one trace operation.
     */
    if (opsCount == 0) {
	Tcl_SetResult(interp, "bad operation list: must be one or more of "
	    "delete, insert, update, set or unset", TCL_STATIC) ;
	return TCL_ERROR ;
    }

    for ( ; opsCount > 0 ; --opsCount, ++opsVect) {
	if (Tcl_GetIndexFromObjStruct(interp, *opsVect, opsTable,
	    sizeof(struct traceOpsMap), "traceOp", 0, &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	flags |= opsTable[index].opFlag ;
    }
    *flagPtr = flags ;
    return TCL_OK ;
}

static int
Ral_RelvarObjEncodeTraceFlag(
    Tcl_Interp *interp,
    int flags,
    Tcl_Obj **opsListPtr)
{
    Tcl_Obj *opsList = Tcl_NewListObj(0, NULL) ;
    struct traceOpsMap const * mp ;

    for (mp = opsTable ; mp->opFlag != 0 ; ++mp) {
	if (flags & mp->opFlag &&
	    Tcl_ListObjAppendElement(interp, opsList,
		Tcl_NewStringObj(mp->opName, -1)) != TCL_OK) {
	    Tcl_DecrRefCount(opsList) ;
	    return TCL_ERROR ;
	}
    }
    *opsListPtr = opsList ;
    return TCL_OK ;
}

/*
 * Execute the traces on a given relvar.
 */
static Tcl_Obj *	    /* returns the object returned by the trace proc.
			     * return NULL on error or if "type" is NULL. */
Ral_RelvarObjExecTraces(
    Tcl_Interp *interp,	    /* the interpreter */
    Ral_Relvar relvar,	    /* the relvar we are tracing. "flags" will be set
			     * with the single operation on which the traces
			     * are being invoked. */
    Tcl_ObjType *type,	    /* pointer to the type of the trace proc return
			     * result. NULL if we don't expect the trace proc
			     * to return a result. */
    Tcl_Obj *arg1,	    /* First argument passed to trace proc. NULL if
			     * there is no first argument. */
    Tcl_Obj *arg2)	    /* Second argument passed to trace proc. NULL if
			     * there is no second argument. */
{
    int result = TCL_OK ;
    Tcl_Obj *nameObj ;
    Tcl_Obj *flagObj ;
    Tcl_Obj *traceObj = arg2 ? arg2 : (arg1 ? arg1 : NULL) ;
    Ral_TraceInfo trace ;
    Tcl_Obj *cmd ;
    /*
     * Get the relvar into an object and hold on to it.
     */
    nameObj = Tcl_NewStringObj(relvar->name, -1) ;
    Tcl_IncrRefCount(nameObj) ;
    /*
     * Translate the trace flags into a string list. In this case there
     * will be only one flag bit set. Again we need to hold on to it.
     */
    if (Ral_RelvarObjEncodeTraceFlag(interp, relvar->traceFlags, &flagObj)
	!= TCL_OK) {
	Tcl_DecrRefCount(nameObj) ;
	return NULL ;
    }
    Tcl_IncrRefCount(flagObj) ;
    /*
     * Iterate through the traces. We allow the "command" to be a command
     * prefix and append the necessary argument as given. The strategy is
     * to accumulate all of this in a list, break out the list and evaluate
     * the result as pre-parsed command. This should reduce some of the
     * shimmering.
     */
    for (trace = relvar->traces ; trace ; trace = trace->next) {
	if (trace->flags & relvar->traceFlags) {
	    int cmdc ;
	    Tcl_Obj **cmdv ;

	    /*
	     * Compose the command from the prefix, relvar name and flags.
	     * All trace procs get these arguments.
	     */
	    cmd = Tcl_NewListObj(0, NULL) ;
	    if (Tcl_ListObjAppendList(interp, cmd, trace->command) == TCL_OK &&
		Tcl_ListObjAppendElement(interp, cmd, flagObj) == TCL_OK &&
		Tcl_ListObjAppendElement(interp, cmd, nameObj) == TCL_OK) {

		/*
		 * Add in the other arguments as given.
		 */
		if (arg1 != NULL &&
		    Tcl_ListObjAppendElement(interp, cmd, arg1) != TCL_OK) {
		    goto cmdError ;
		}
		if (arg2 != NULL &&
		    Tcl_ListObjAppendElement(interp, cmd, arg2) != TCL_OK) {
		    goto cmdError ;
		}

		/*
		 * Break out the list as a array of argments and evaluate it.
		 */
		if (Tcl_ListObjGetElements(interp, cmd, &cmdc, &cmdv)
			!= TCL_OK ||
		    Tcl_EvalObjv(interp, cmdc, cmdv, TCL_EVAL_DIRECT)
			!= TCL_OK) {
		    goto cmdError ;
		}
		if (type != NULL) {

		    /*
		     * When we care about the return object of the trace
		     * proc we fish it out of the interpreter and make
		     * sure that it is of the proper type.
		     */
		    traceObj = Tcl_GetObjResult(interp) ;
		    result = Tcl_ConvertToType(interp, traceObj, type) ;
		    /*
		     * If we got the correct object type, then we pass it
		     * along to the next trace proc. That always corresponds
		     * to the last of the arguments given (if any).
		     */
		    if (result == TCL_OK) {
			if (arg2) {
			    arg2 = traceObj ;
			} else if (arg1) {
			    arg1 = traceObj ;
			}
		    } else {
			goto cmdError ;
		    }
		}
	    } else {
		goto cmdError ;
	    }
	    Tcl_DecrRefCount(cmd) ;
	}
    }

    /*
     * Let go of the trace proc arguments that we created here.
     */
    Tcl_DecrRefCount(flagObj) ;
    Tcl_DecrRefCount(nameObj) ;

    /*
     * Return the last return value from the chain of trace procs.
     */
    return traceObj ;

cmdError:
    Tcl_DecrRefCount(cmd) ;
    return NULL ;
}
