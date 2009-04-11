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
$Revision: 1.42 $
$Date: 2009/04/11 18:18:54 $
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
static char *relvarTraceProc(ClientData, Tcl_Interp *, char const *,
    char const *, int) ;
static char const *relvarResolveName(Tcl_Interp *, char const *,
    Tcl_DString *) ;
static int relvarNameIsAbsName(char const *) ;
static int relvarGetNamespaceName(Tcl_Interp *, char const *, Tcl_DString *) ;
static Tcl_Obj *relvarConstraintAttrNames(Tcl_Interp *, Ral_Relvar,
    Ral_JoinMap, int) ;
static Tcl_Obj *relvarAssocSpec(int, int) ;
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
    char const *specString ;
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
    char const *opName ;
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

/*
FUNCTION DEFINITIONS
*/

int
Ral_RelvarObjNew(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    char const *name,
    Ral_TupleHeading heading,
    int idCnt,
    Tcl_Obj *const*idAttrs,
    Ral_ErrorInfo *errInfo)
{
    Tcl_DString resolve ;
    char const *resolvedName ;
    Ral_Relvar relvar ;
    int status ;
    struct relvarId *idIter ;
    Ral_IntVector id ;

    /*
     * Creating a relvar is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
        Ral_ErrorInfoSetError(errInfo, RAL_ERR_BAD_TRANS_OP,
            "relvar creation not allowed during a transaction") ;
        Ral_InterpSetError(interp, errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Create the relvar.
     */
    resolvedName = relvarResolveName(interp, name, &resolve) ;
    relvar = Ral_RelvarNew(info, resolvedName, heading, idCnt) ;
    if (relvar == NULL) {
	/*
	 * Duplicate name.
	 */
        Ral_ErrorInfoSetError(errInfo, RAL_ERR_DUP_NAME, resolvedName) ;
        Ral_InterpSetError(interp, errInfo) ;
        Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    Tcl_DStringFree(&resolve) ;
    /*
     * Deal with the identifiers. At this point the relvar structure
     * has the space allocated for identifier information, but we need
     * to look at the attribute lists to make sure they are well formed.
     */
    idIter = relvar->identifiers ;
    while (idCnt-- != 0) {
        int elemc ;
        Tcl_Obj **elemv ;
        struct relvarId *ckIter ;

        if (Tcl_ListObjGetElements(interp, *idAttrs, &elemc, &elemv)
                != TCL_OK) {
            Ral_RelvarDelete(info, relvar) ;
            return TCL_ERROR ;
        }
        if (elemc < 1) {
            Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_IDENTIFIER_FORMAT,
                    *idAttrs) ;
            Ral_InterpSetError(interp, errInfo) ;
            Ral_RelvarDelete(info, relvar) ;
            return TCL_ERROR ;
        }
        /*
         * Vector to hold the attribute indices that constitute the
         * identifier.
         */
        id = Ral_IntVectorNewEmpty(elemc) ;
        /*
         * Find the attribute in the tuple heading and build up a vector of
         * attributes indices that will form the identifier.
         */
        while (elemc-- > 0) {
            const char *attrName = Tcl_GetString(*elemv++) ;
            int index = Ral_TupleHeadingIndexOf(heading, attrName) ;
            if (index < 0) {
                Ral_ErrorInfoSetError(errInfo, RAL_ERR_UNKNOWN_ATTR, attrName) ;
                goto errorOut ;
            }
            /*
             * The indices in an identifier must form a set, i.e. you cannot
             * have a duplicate attribute in a list of attributes that is
             * intended to be an identifier of a relation.
             */
            if (!Ral_IntVectorSetAdd(id, index)) {
                Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_DUP_ATTR_IN_ID,
                        *idAttrs) ;
                goto errorOut ;
            }
        }
        /*
         * Add the set of attribute indices as an identifier.
         * Check if the identifier is not a subset of an existing one.
         * Make sure the identifier is in canonical order.
         */
        Ral_IntVectorSort(id) ;
        for (ckIter = relvar->identifiers ; ckIter != idIter ; ++ckIter) {
            if (Ral_IntVectorSubsetOf(id, ckIter->idAttrs) ||
                    Ral_IntVectorSubsetOf(ckIter->idAttrs, id)) {
                Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_IDENTIFIER_SUBSET,
                        *idAttrs) ;
                goto errorOut ;
            }
        }
        /*
         * Finally add the attribute vector to the relvar structure and
         * initialize the hash table that will be used to enforce
         * the identity constraints.
         */
        idIter->idAttrs = id ;
        Tcl_InitCustomHashTable(&idIter->idIndex, TCL_CUSTOM_PTR_KEYS,
                &tupleAttrHashType) ;
        ++idAttrs ;
        ++idIter ;
    }
    /*
     * Create a variable by the same name.
     */
    if (Tcl_SetVar2Ex(interp, relvar->name, NULL, relvar->relObj,
            TCL_LEAVE_ERR_MSG) == NULL) {
        Ral_RelvarDelete(info, relvar) ;
	return TCL_ERROR ;
    }
    /*
     * Set up a trace to make the Tcl variable read only.
     */
    status = Tcl_TraceVar(interp, relvar->name, relvarTraceFlags,
	relvarTraceProc, info) ;
    assert(status == TCL_OK) ;

    Tcl_SetObjResult(interp, relvar->relObj) ;
    return TCL_OK ;

errorOut:
    Ral_IntVectorDelete(id) ;
    Ral_RelvarDelete(info, relvar) ;
    Ral_InterpSetError(interp, errInfo) ;
    return TCL_ERROR ;
    
}

int
Ral_RelvarObjDelete(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    Tcl_Obj *nameObj)
{
    Ral_Relvar relvar ;
    /*
     * Unsetting a relvar is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptUnset,
	    RAL_ERR_BAD_TRANS_OP, "unset") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(nameObj)) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Make sure that all the constraints associated with the
     * relvar are gone. Can't have dangling references to the relvar
     * via a constraint.
     */
    if (Ral_PtrVectorSize(relvar->constraints) != 0) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptDelete,
	    RAL_ERR_CONSTRAINTS_PRESENT, relvar->name) ;
	return TCL_ERROR ;
    }
    /*
     * Run the unset traces.
     */
    Ral_RelvarObjExecUnsetTraces(interp, relvar) ;
    /*
     * Remove the trace.
     */
    Tcl_UntraceVar(interp, relvar->name, relvarTraceFlags, relvarTraceProc,
	info) ;
    /*
     * Remove the variable.
     */
    if (Tcl_UnsetVar(interp, relvar->name, TCL_LEAVE_ERR_MSG) != TCL_OK) {
	return TCL_ERROR ;
    }

    Ral_RelvarDelete(info, relvar) ;

    Tcl_ResetResult(interp) ;
    return TCL_OK ;
}

/*
 * Determine if the relation value stored in the relvar is being shared and
 * if so enforce the copy on write rules. This function must be called before
 * modifying the relation value stored in a relvar. It may change the
 * value of the Tcl_Obj * that is the relation value stored in the relvar.
 * Return TCL_OK if the relation value is not shared or was copied without
 * error and TCL_ERROR otherwise, leaving an error message in the interpreter.
 */
int
Ral_RelvarObjCopyOnShared(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    Ral_Relvar relvar)
{
    int result = TCL_OK ;

    Tcl_Obj *relObj = relvar->relObj ;
    assert(relObj->typePtr == &Ral_RelationObjType) ;

    /*
     * For relation values stored in relvars, we expect the reference count
     * to be at least 2, since we have one reference in the relvar and
     * another in the Tcl variable that shadows the relvar. Sharing is
     * determined then when the reference count is greater than 2.
     */
    assert(relObj->refCount > 1) ;
    if (relObj->refCount > 2) {
	int status ;
	/*
	 * If we are sharing, then we duplicate the relation value and store it
	 * back into the relvar. We also need to change the Tcl_Obj that the
	 * variable is referencing. This is slightly more complicated by virtue
	 * of the fact that we have to remove and then replace the write trace
	 * on that variable. Recall that relvars have a corresponding Tcl
	 * variable and we put a write trace on it to make the Tcl variable
	 * read only since all relvar modification should come via the
	 * relvar commands.
	 */
	Tcl_IncrRefCount(relvar->relObj = Tcl_DuplicateObj(relObj)) ;
	assert(relObj->typePtr == &Ral_RelationObjType) ;
	Tcl_DecrRefCount(relObj) ;
	Tcl_UntraceVar(interp, relvar->name, relvarTraceFlags, relvarTraceProc,
	    info) ;
	if (Tcl_SetVar2Ex(interp, relvar->name, NULL, relvar->relObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR ;
	}
	status = Tcl_TraceVar(interp, relvar->name, relvarTraceFlags,
	    relvarTraceProc, info) ;
	assert(status == TCL_OK) ;
    }
    assert(relvar->relObj->refCount == 2) ;

    return result ;
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
    char const *name)
{
    Ral_Relvar relvar ;

    if (relvarNameIsAbsName(name)) {
	/*
	 * Absolute name.
	 */
	relvar = Ral_RelvarFind(info, name) ;
    } else {
	/*
	 * Relative name. First try the current namespace.
	 */
	Tcl_DString resolve ;
	int globalName ;
	char const *resolvedName ;

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

	Tcl_DStringFree(&resolve) ;
    }

    if (relvar == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptNone,
	    RAL_ERR_UNKNOWN_NAME, name) ;
    }
    return relvar ;
}

/*
 * Returns a tuple object. This tuple object will be the final result
 * of any trace operations.
 */
Tcl_Obj *
Ral_RelvarObjInsertTuple(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Tcl_Obj *nameValueObj,
    Ral_IntVector *orderMap,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation relation ;
    Tcl_Obj *tupleObj ;
    Tcl_Obj *resultObj ;

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relation = relvar->relObj->internalRep.otherValuePtr ;

    /*
     * Create a new tuple object from the list of attribute / value pairs. It
     * is allowed that there be fewer attributes than the tuple heading so that
     * variable traces may extend the tuple.
     */
    tupleObj = Ral_TuplePartialSetFromObj(relation->heading, interp,
            nameValueObj, errInfo) ;
    if (tupleObj == NULL) {
	return NULL ;
    }
    Tcl_IncrRefCount(tupleObj) ;
    /*
     * Run the traces and get the result back. "resultObj" will have
     * a reference count of at least one and will need to be decremented.
     */
    resultObj = Ral_RelvarObjExecInsertTraces(interp, relvar, tupleObj) ;
    Tcl_DecrRefCount(tupleObj) ;
    if (resultObj) {
        Ral_Tuple tuple ;
	/*
	 * Objects returned have already been converted to tuples.
	 * Insert the tuple into the relation.
	 */
	assert(resultObj->typePtr == &Ral_TupleObjType) ;
	tuple = resultObj->internalRep.otherValuePtr ;
        /*
         * Check that the relvar traces did not return a heading
         * that no longer matches that of the relation value.
         */
        if (Ral_TupleHeadingEqual(relation->heading, tuple->heading)) {
            /*
             * Since the traces may return a tuple heading of any order
             * we must make sure to reorder things if necessary. This vector
             * is also returned by reference, so the caller must delete it.
             */
            *orderMap = Ral_TupleHeadingNewOrderMap(relation->heading,
                    tuple->heading) ;
            if (!Ral_RelvarInsertTuple(relvar, tuple, *orderMap, errInfo)) {
                goto errorOut ;
            }
        } else {
            Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_HEADING_NOT_EQUAL,
                    resultObj) ;
            goto errorOut ;
        }
    }

    return resultObj ;

errorOut:
    Ral_InterpSetError(interp, errInfo) ;
    Tcl_DecrRefCount(resultObj) ;
    return NULL ;
}

/*
 * Returns a tuple that has values set for the given attributes.
 * The attributes are examined to make sure that they form an identifier of the
 * relvar so that the resulting tuple can be used as a key to look up a
 * particular tuple in the relation of the relvar.
 * Caller must delete the returned tuple.
 */
Ral_Tuple
Ral_RelvarObjKeyTuple(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    int objc,
    Tcl_Obj *const*objv,
    int *idRef,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation relation ;
    Ral_IntVector id ;
    Ral_Tuple key ;
    int idNum ;

    if (objc % 2 != 0) {
	Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_PAIRS_LIST, *objv) ;
	Ral_InterpSetError(interp, errInfo) ;
	return NULL ;
    }
    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relation = relvar->relObj->internalRep.otherValuePtr ;
    /*
     * Iterate through the name/value list and construct an identifier
     * vector from the attribute names and a key tuple from the corresponding
     * values.
     */
    id = Ral_IntVectorNewEmpty(objc / 2) ;
    key = Ral_TupleNew(relation->heading) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	const char *attrName = Tcl_GetString(*objv) ;
	int attrIndex = Ral_TupleHeadingIndexOf(relation->heading, attrName) ;

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
    idNum = Ral_RelvarFindIdentifier(relvar, id) ;
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

int
Ral_RelvarObjUpdateTuple(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Ral_RelationIter tupleIter,
    Tcl_Obj *scriptObj,
    Tcl_Obj *tupleObj,
    Ral_Relation updated,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation relation ;
    int result ;
    Tcl_Obj *newTupleObj ;

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relation = relvar->relObj->internalRep.otherValuePtr ;
    /*
     * Evaluate the script.
     */
    result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
    if (result == TCL_ERROR) {
	static const char msgfmt[] =
	    "\n    (\"in ::ral::%s %s\" body line %d)" ;
	char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE + 50] ;

#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 6
	sprintf(msg, msgfmt, Ral_ErrorInfoGetCommand(errInfo),
	    Ral_ErrorInfoGetOption(errInfo), Tcl_GetErrorLine(interp)) ;
#else
	sprintf(msg, msgfmt, Ral_ErrorInfoGetCommand(errInfo),
	    Ral_ErrorInfoGetOption(errInfo), interp->errorLine) ;
#endif
	Tcl_AddObjErrorInfo(interp, msg, -1) ;
	return TCL_ERROR ;
    } else if (!(result == TCL_OK || result == TCL_RETURN
            || result == TCL_CONTINUE)) {
	return result ;
    }
    /*
     * Fetch the return value of the script.  Once we get the new tuple value,
     * we can use it to update the relvar.
     */
    newTupleObj = Tcl_GetObjResult(interp) ;
    return Ral_RelvarObjTraceUpdate(interp, relvar, tupleIter, newTupleObj,
            updated, errInfo) ;
}

int
Ral_RelvarObjTraceUpdate(
    Tcl_Interp *interp,
    Ral_Relvar relvar,
    Ral_RelationIter tupleIter,
    Tcl_Obj *newTupleObj,
    Ral_Relation updated,
    Ral_ErrorInfo *errInfo)
{
    Ral_Relation relation ;
    int result ;
    Tcl_Obj *oldTupleObj ;
    Tcl_Obj *resultTupleObj ;
    Ral_Tuple resultTuple ;

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relation = relvar->relObj->internalRep.otherValuePtr ;
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
    Tcl_DecrRefCount(oldTupleObj) ;
    Tcl_DecrRefCount(newTupleObj) ;
    if (resultTupleObj == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, resultTupleObj, &Ral_TupleObjType)
            != TCL_OK) {
	return TCL_ERROR ;
    }
    resultTuple = resultTupleObj->internalRep.otherValuePtr ;
    if (!Ral_TupleHeadingEqual(relation->heading, resultTuple->heading)) {
        Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_HEADING_NOT_EQUAL,
                resultTupleObj) ;
        Ral_InterpSetError(interp, errInfo) ;
        Tcl_DecrRefCount(resultTupleObj) ;
	return TCL_ERROR ;
    }
    /*
     * Remove the old identifier index values.
     */
    Ral_RelvarIdUnindexTuple(relvar, *tupleIter) ;
    /*
     * Update the tuple values in place.
     */
    result = Ral_RelationUpdateTupleObj(relation, tupleIter, interp,
	    resultTupleObj, errInfo) ;
    if (result == TCL_OK) {
        int status ;

	assert(resultTupleObj->typePtr == &Ral_TupleObjType) ;
	result = Ral_RelationInsertTupleObj(updated, interp, resultTupleObj,
	    errInfo) ;
        assert(result == TCL_OK) ;
        /*
         * Re-index the identifiers for the new tuple value. Here is where
         * we detect if the updated tuple value satisfies the identification
         * constraints on the relvar.
         */
        status = Ral_RelvarIdIndexTuple(relvar, *tupleIter,
                tupleIter - Ral_RelationBegin(relation), NULL) ;
        if (status == 0) {
            Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_IDENTITY_CONSTRAINT,
                    resultTupleObj) ;
            Ral_InterpSetError(interp, errInfo) ;
            result = TCL_ERROR ;
        }
    }
    Tcl_DecrRefCount(resultTupleObj) ;

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
    char const *name ;
    Tcl_DString resolve ;
    Ral_JoinMap refMap ;
    Ral_TupleHeading th1 ;
    Ral_TupleHeading th2 ;
    Ral_IntVector refAttrs ;
    int refToId ;
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
    relvar1 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[1])) ;
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

    relvar2 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[4])) ;
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
    th1 = r1->heading ;
    th2 = r2->heading ;
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
     * Sort the join map attributes relative to the referred to identifier.
     * Remember that identifier attribute vectors are kept in sorted order
     * and we must make sure that the referring attributes are kept in
     * the same relative order.
     */
    Ral_JoinMapSortAttr(refMap, 1) ;
    /*
     * The referred to attributes in the second relvar must constitute an
     * identifier of the relation.
     */
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    refToId = Ral_RelvarFindIdentifier(relvar2, refAttrs) ;
    Ral_IntVectorDelete(refAttrs) ;
    if (refToId < 0) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptAssociation,
	    RAL_ERR_NOT_AN_IDENTIFIER, objv[5]) ;
	Ral_JoinMapDelete(refMap) ;
	return TCL_ERROR ;
    }
    assert(refToId < relvar2->idCount) ;
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
    assoc = constraint->constraint.association ;
    assoc->referringRelvar = relvar1 ;
    assoc->referringCond = specTable[specIndex1].conditionality ;
    assoc->referringMult = specTable[specIndex1].multiplicity ;
    assoc->referredToRelvar = relvar2 ;
    assoc->referredToCond = specTable[specIndex2].conditionality ;
    assoc->referredToMult = specTable[specIndex2].multiplicity ;
    assoc->referredToIdentifier = refToId ;
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
    int supId ;
    int nSupAttrs ;
    Ral_TupleHeading supth ;
    char const *partName ;
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
    super = Ral_RelvarObjFindRelvar(interp, info, superName) ;
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
    supth = superRel->heading ;
    while (supElemc-- > 0) {
	char const *attrName = Tcl_GetString(*supElemv++) ;
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
    supId = Ral_RelvarFindIdentifier(super, superAttrs) ;
    if (supId < 0) {
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
    partition = constraint->constraint.partition ;
    partition->referredToRelvar = super ;
    partition->referredToIdentifier = supId ;

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
	sub = Ral_RelvarObjFindRelvar(interp, info, subName) ;
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
	subth = subRel->heading ;
	supAttrIter = Ral_IntVectorBegin(superAttrs) ;
	while (subElemc-- > 0) {
	    char const *attrName = Tcl_GetString(*subElemv++) ;
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

    char const *name ;
    Tcl_DString resolve ;
    Ral_JoinMap refMap ;
    Ral_TupleHeading thC ;
    Ral_TupleHeading th1 ;
    Ral_TupleHeading th2 ;
    Ral_IntVector refAttrs ;
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
    relvarC = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[1])) ;
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
    relvar1 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[4])) ;
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

    relvar2 = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[8])) ;
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
    correl = constraint->constraint.correlation ;
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
    thC = rC->heading ;
    th1 = r1->heading ;
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
    Ral_JoinMapSortAttr(refMap, 1) ;
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    correl->aIdentifier = Ral_RelvarFindIdentifier(relvar1, refAttrs) ;
    Ral_IntVectorDelete(refAttrs) ;
    if (correl->aIdentifier < 0) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelvar, Ral_OptCorrelation,
	    RAL_ERR_NOT_AN_IDENTIFIER, objv[5]) ;
	goto errorOut ;
    }
    /*
     * Sort the join map attributes to match the identifier order.
     */
    Ral_JoinMapSortAttr(refMap, 1) ;

    refMap = correl->bReferenceMap ;
    th2 = r2->heading ;
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
    Ral_JoinMapSortAttr(refMap, 1) ;
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    correl->bIdentifier = Ral_RelvarFindIdentifier(relvar2, refAttrs) ;
    Ral_IntVectorDelete(refAttrs) ;
    if (correl->bIdentifier < 0) {
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
    char const *name,
    Ral_RelvarInfo info)
{
    Ral_Constraint constraint ;
    int status ;

    constraint = Ral_RelvarObjFindConstraint(interp, info, name) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptConstraint,
	    RAL_ERR_UNKNOWN_CONSTRAINT, name) ;
	return TCL_ERROR ;
    }
    status = Ral_ConstraintDeleteByName(constraint->name, info) ;
    assert(status != 0) ;
    return TCL_OK ;
}

int
Ral_RelvarObjConstraintInfo(
    Tcl_Interp *interp,
    Tcl_Obj * const nameObj,
    Ral_RelvarInfo info)
{
    Ral_Constraint constraint ;
    Tcl_Obj *resultObj ;
    Tcl_Obj *attrList ;

    /*
     * Look up the constraint by it name.
     */
    constraint = Ral_RelvarObjFindConstraint(interp, info,
	    Tcl_GetString(nameObj)) ;
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
	Ral_AssociationConstraint assoc = constraint->constraint.association ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("association", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(constraint->name, -1)) != TCL_OK) {
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
	Ral_PartitionConstraint partition = constraint->constraint.partition ;
	Ral_PtrVectorIter refEnd =
	    Ral_PtrVectorEnd(partition->subsetReferences) ;
	Ral_PtrVectorIter refIter ;
	Ral_SubsetReference subRef ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("partition", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(constraint->name, -1)) != TCL_OK) {
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
	Ral_CorrelationConstraint correl = constraint->constraint.correlation ;

	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("correlation", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (correl->complete && Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj("-complete", -1)) != TCL_OK) {
	    goto errorOut ;
	}
	if (Tcl_ListObjAppendElement(interp, resultObj,
	    Tcl_NewStringObj(constraint->name, -1)) != TCL_OK) {
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

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(resultObj) ;
    return TCL_ERROR ;
}

int
Ral_RelvarObjConstraintNames(
    Tcl_Interp *interp,
    char const *pattern,
    Ral_RelvarInfo info)
{
    Tcl_Obj *nameList ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;

    nameList = Tcl_NewListObj(0, NULL) ;
    for (entry = Tcl_FirstHashEntry(&info->constraints, &search) ; entry ;
	entry = Tcl_NextHashEntry(&search)) {
	char const *name = (char const *)Tcl_GetHashKey(&info->constraints,
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

    relvar = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(relvarName)) ;
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

    constraint = Ral_RelvarObjFindConstraint(interp, info, name) ;
    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptConstraint,
	    RAL_ERR_UNKNOWN_CONSTRAINT, name) ;
	return TCL_ERROR ;
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(constraint->name, -1)) ;
    return TCL_OK ;
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
Ral_Constraint
Ral_RelvarObjFindConstraint(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    char const *name)
{
    Ral_Constraint constraint ;

    if (relvarNameIsAbsName(name)) {
	/*
	 * Absolute name.
	 */
	constraint = Ral_ConstraintFindByName(name, info) ;
    } else {
	/*
	 * Relative name. First try the current namespace.
	 */
	Tcl_DString resolve ;
	int globalName ;
	char const *resolvedName ;

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

	Tcl_DStringFree(&resolve) ;
    }

    if (constraint == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelvar, Ral_OptNone,
	    RAL_ERR_UNKNOWN_NAME, name) ;
    }
    return constraint ;
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
    /*
     * Guard against calling "transaction end" when there was not
     * "transaction begin".
     */
    if (Ral_PtrVectorSize(info->transactions) > 0) {
        success = Ral_RelvarEndTransaction(info, failed, &errMsg) ;
        if (!(failed || success)) {
            Tcl_DStringResult(interp, &errMsg) ;
        }
    } else {
	Tcl_DStringAppend(&errMsg, "end transaction with no beginning", -1) ;
        Tcl_DStringResult(interp, &errMsg) ;
        success = 0 ;
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
    char const *cmdString = Tcl_GetString(cmdPrefix) ;
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
    /*
     * Hold on to the insert tuple. This function returns an object that
     * has a reference count of at least one. The caller must decrement
     * the reference count.
     */
    Tcl_IncrRefCount(insertTuple) ;
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
    Tcl_IncrRefCount(newTuple) ;
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
    Tcl_IncrRefCount(relationObj) ;
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
	    if (!Ral_TupleHeadingEqual(resultRel->heading, origRel->heading)) {
		char *headingStr =
		    Ral_TupleHeadingStringOf(resultRel->heading) ;
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
	Tcl_Obj *evalObj = Tcl_NewStringObj("transaction", -1) ;
	Tcl_Obj *beginObj = isBegin ?
	    Tcl_NewStringObj("begin", -1) : Tcl_NewStringObj("end", -1) ;
	Tcl_Obj *levelObj = Tcl_NewIntObj(level) ;
	Ral_TraceInfo trace ;
	Tcl_Obj *resultObj ;

	Tcl_IncrRefCount(evalObj) ;
	Tcl_IncrRefCount(beginObj) ;
	Tcl_IncrRefCount(levelObj) ;
	/*
	 * Since we ignore any errors from transaction traces, we preserve
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
    char const *name1,
    char const *name2,
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
	char const *resolvedName =
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

static char const *
relvarResolveName(
    Tcl_Interp *interp,
    char const *name,
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
    char const *name)
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
    char const *name,
    Tcl_DString *nsVarName)
{
    int isGlobal = 1 ;
    Tcl_DStringInit(nsVarName) ;

#	ifdef Tcl_GetCurrentNamespace_TCL_DECLARED
    Tcl_Namespace *curr = Tcl_GetCurrentNamespace(interp) ;

    if (curr->parentPtr) {
	Tcl_DStringAppend(nsVarName, curr->fullName, -1) ;
	isGlobal = 0 ;
    }
#	else
    if (Tcl_Eval(interp, "namespace qualifiers [namespace current]")
	    == TCL_OK) {
	char const *result = Tcl_GetStringResult(interp) ;
	if (strlen(result)) {
	    Tcl_DStringAppend(nsVarName, result, -1) ;
	    isGlobal = 0 ;
	}
    }
#	endif
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
    th = relation->heading ;

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
                        != TCL_OK) {
		    goto cmdError ;
                }
                if (Tcl_EvalObjv(interp, cmdc, cmdv, TCL_EVAL_DIRECT)
                        != TCL_OK) {
		    goto cmdError ;
		}
		if (type != NULL) {

		    /*
		     * When we care about the return object of the trace
		     * proc we fish it out of the interpreter and make
		     * sure that it is of the proper type.
		     */
                    if (traceObj) {
                        Tcl_DecrRefCount(traceObj) ;
                    }
		    traceObj = Tcl_GetObjResult(interp) ;
                    Tcl_IncrRefCount(traceObj) ;
                    Tcl_ResetResult(interp) ;
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
    Tcl_DecrRefCount(flagObj) ;
    Tcl_DecrRefCount(nameObj) ;
    return NULL ;
}
