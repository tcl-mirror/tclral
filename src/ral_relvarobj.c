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
$Revision: 1.2 $
$Date: 2006/04/27 14:48:56 $
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
#include "ral_relationobj.h"
#include "ral_utils.h"

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
static char *relvarTraceProc(ClientData, Tcl_Interp *, const char *,
    const char *, int) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static int relvarTraceFlags = TCL_NAMESPACE_ONLY | TCL_TRACE_WRITES ;
static const char rcsid[] = "@(#) $RCSfile: ral_relvarobj.c,v $ $Revision: 1.2 $" ;

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
    const char *resolvedName =
	Ral_RelvarObjResolveName(interp, name, &resolve) ;
    Ral_Relvar relvar = Ral_RelvarNew(info, resolvedName, heading) ;
    int status ;

    if (relvar == NULL) {
	/*
	 * Duplicate name.
	 */
	Ral_RelvarObjSetError(interp, Ral_RelvarLastError, resolvedName) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    /*
     * Create a variable by the same name.
     */
    if (Tcl_ObjSetVar2(interp, Tcl_NewStringObj(resolvedName, -1), NULL,
	relvar->relObj, TCL_LEAVE_ERR_MSG) == NULL) {
	goto errorOut ;
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

errorOut:
    Ral_RelvarDelete(info, resolvedName) ;
    Tcl_DStringFree(&resolve) ;
    return TCL_ERROR ;
}

int
Ral_RelvarObjDelete(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    Tcl_Obj *nameObj)
{
    char *fullName ;
    Ral_Relvar relvar ;
    int status ;

    relvar = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(nameObj),
	&fullName) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    /*
     * Remove the trace.
     */
    Tcl_UntraceVar(interp, fullName, relvarTraceFlags, relvarTraceProc,
	info) ;
    /*
     * Remove the variable.
     */
    status = Tcl_UnsetVar(interp, fullName, TCL_LEAVE_ERR_MSG) ;
    assert(status == TCL_OK) ;

    Ral_RelvarStartCommand(info, relvar) ;
    Ral_RelvarDelete(info, fullName) ;
    Ral_RelvarEndCommand(info, relvar, 0) ;

    ckfree(fullName) ;
    Tcl_ResetResult(interp) ;
    return TCL_OK ;
}

Ral_Relvar
Ral_RelvarObjFindRelvar(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    const char *name,
    char **fullName)
{
    Tcl_DString resolve ;
    const char *resolvedName =
	Ral_RelvarObjResolveName(interp, name, &resolve) ;
    Ral_Relvar relvar = Ral_RelvarFind(info, resolvedName) ;

    if (fullName) {
	*fullName = ckalloc(strlen(resolvedName) + 1) ;
	strcpy(*fullName, resolvedName) ;
    }
    Tcl_DStringFree(&resolve) ;

    if (relvar == NULL) {
	Ral_RelvarObjSetError(interp, RELVAR_UNKNOWN_NAME, name) ;
    }
    return relvar ;
}

const char *
Ral_RelvarObjResolveName(
    Tcl_Interp *interp,
    const char *name,
    Tcl_DString *resolvedName)
{
    int nameLen = strlen(name) ;

    Tcl_DStringInit(resolvedName) ;

    if (nameLen >= 2 && name[0] == ':' && name[1] == ':') {
	/*
	 * absolute reference.
	 */
	Tcl_DStringAppend(resolvedName, name, -1) ;
    } else if(interp) {
	/*
	 * construct an absolute name from the current namespace.
	 */
	Tcl_Namespace *curr = Tcl_GetCurrentNamespace(interp) ;
	if (curr->parentPtr) {
	    Tcl_DStringAppend(resolvedName, curr->fullName, -1) ;
	}
	Tcl_DStringAppend(resolvedName, "::", -1) ;
	Tcl_DStringAppend(resolvedName, name, -1) ;
    }

    return Tcl_DStringValue(resolvedName) ;
}

int
Ral_RelvarObjCreateAssoc(
    Tcl_Interp *interp,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo info)
{
    /* name relvar1 attr-list1 spec1 relvar2 attr-list2 spec2 */

    static struct specMap {
	const char *specString ;
	int conditionality ;
	int multiplicity ;
    } specTable[] = {
	{"1", 0, 0},
	{"1..N", 0, 1},
	{"0..1", 1, 0},
	{"0..N", 1, 1},
    } ;

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
    const char *name = Tcl_GetString(objv[0]) ;
    Ral_JoinMap refMap ;
    Ral_TupleHeading th1 ;
    Ral_TupleHeading th2 ;
    Ral_IntVector refAttrs ;
    int isNotId ;
    Ral_Constraint constraint ;
    Ral_AssociationConstraint assoc ;

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
    if (Ral_RelationCardinality(r1) != 0) {
	Ral_RelvarObjSetError(interp, RELVAR_NOT_EMPTY,
	    Tcl_GetString(relvar1->relObj)) ;
    }
    if (Tcl_ListObjGetElements(interp, objv[2], &elemc1, &elemv1) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[3], specTable,
	sizeof(struct specMap), "association specification", 0, &specIndex1)
	!= TCL_OK) {
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
    if (Ral_RelationCardinality(r2) != 0) {
	Ral_RelvarObjSetError(interp, RELVAR_NOT_EMPTY,
	    Tcl_GetString(relvar2->relObj)) ;
    }
    if (Tcl_ListObjGetElements(interp, objv[5], &elemc2, &elemv2) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[6], specTable,
	sizeof(struct specMap), "association specification", 0, &specIndex2)
	!= TCL_OK) {
	return TCL_ERROR ;
    }

    if (elemc1 != elemc2) {
	Ral_RelvarObjSetError(interp, RELVAR_REFATTR_MISMATCH,
	    Tcl_GetString(objv[5])) ;
	return TCL_ERROR ;
    }

    refMap = Ral_JoinMapNew(0, 0) ;
    th1 = r1->heading->tupleHeading ;
    th2 = r2->heading->tupleHeading ;
    while (elemc1-- > 0) {
	int attrIndex1 = Ral_TupleHeadingIndexOf(th1, Tcl_GetString(*elemv1)) ;
	int attrIndex2 = Ral_TupleHeadingIndexOf(th2, Tcl_GetString(*elemv2)) ;

	if (attrIndex1 < 0) {
	    Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR,
		Tcl_GetString(*elemv1)) ;
	    Ral_JoinMapDelete(refMap) ;
	    return TCL_ERROR ;
	}
	if (attrIndex2 < 0) {
	    Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR,
		Tcl_GetString(*elemv2)) ;
	    Ral_JoinMapDelete(refMap) ;
	    return TCL_ERROR ;
	}
	Ral_JoinMapAddAttrMapping(refMap, attrIndex1, attrIndex2) ;
    }

    constraint = Ral_ConstraintAssocCreate(name, info) ;
    if (constraint == NULL) {
	Ral_RelvarObjSetError(interp, RELVAR_REFATTR_MISMATCH, name) ;
	Ral_JoinMapDelete(refMap) ;
	return TCL_ERROR ;
    }
    /*
     * The referred to attributes in the second relvar must constitute an
     * identifier of the relation.
     */
    refAttrs = Ral_JoinMapGetAttr(refMap, 1) ;
    isNotId = Ral_RelationHeadingFindIdentifier(r2->heading, refAttrs) < 0 ;
    Ral_IntVectorDelete(refAttrs) ;
    if (isNotId) {
	Ral_RelvarObjSetError(interp, RELVAR_NOT_ID, Tcl_GetString(objv[5])) ;
	Ral_JoinMapDelete(refMap) ;
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
    Ral_PtrVectorPushBack(relvar1->constraints, constraint) ;
    Ral_PtrVectorPushBack(relvar2->constraints, constraint) ;

    return TCL_OK ;
}

void
Ral_RelvarObjSetError(
    Tcl_Interp *interp,
    Ral_RelvarError error,
    const char *param)
{
    /*
     * These must be in the same order as the encoding of the Ral_RelationError
     * enumeration.
     */
    static const char *resultStrings[] = {
	"no error",
	"duplicate relvar name",
	"unknown relvar name",
	"relation heading mismatch",
	"mismatch between referential attributes",
	"duplicate constraint name",
	"association may only be applied to empty relvars",
    } ;
    static const char *errorStrings[] = {
	"OK",
	"DUP_NAME",
	"UNKNOWN_NAME",
	"HEADING_MISMATCH",
	"REFATTR_MISMATCH",
	"DUP_CONSTRAINT",
	"NOT_ID",
	"NOT_EMPTY",
    } ;

    Ral_ObjSetError(interp, "RELVAR", resultStrings[error],
	errorStrings[error], param) ;
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
	    Ral_RelvarObjResolveName(interp, name1, &resolve) ;
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
	fprintf(stderr, "relvarTraceProc: trace on non-write, flags = %#x\n",
	    flags) ;
    }

    return result ;
}

/*
 * PRIVATE FUNCTIONS
 */
