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
static Tcl_Obj *relvarConstraintAttrNames(Tcl_Interp *, Ral_Relvar,
    Ral_JoinMap, int) ;
static Tcl_Obj *relvarAssocSpec(int, int) ;

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
static const char rcsid[] = "@(#) $RCSfile: ral_relvarobj.c,v $ $Revision: 1.6 $" ;

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
    Tcl_Obj *varName ;
    int status ;

    /*
     * Creating a relvar is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_RelvarObjSetError(interp, RELVAR_BAD_TRANS_OP, "create") ;
	return TCL_ERROR ;
    }
    /*
     * Create the relvar.
     */
    resolvedName = Ral_RelvarObjResolveName(interp, name, &resolve) ;
    relvar = Ral_RelvarNew(info, resolvedName, heading) ;
    if (relvar == NULL) {
	/*
	 * Duplicate name.
	 */
	Ral_RelvarObjSetError(interp, RELVAR_DUP_NAME, resolvedName) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    /*
     * Create a variable by the same name.
     */
    varName = Tcl_NewStringObj(resolvedName, -1) ;
    if (Tcl_ObjSetVar2(interp, varName, NULL, relvar->relObj,
	TCL_LEAVE_ERR_MSG) == NULL) {
	Tcl_DStringFree(&resolve) ;
	Ral_RelvarObjDelete(interp, info, varName) ;
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
	Ral_RelvarObjSetError(interp, RELVAR_BAD_TRANS_OP, "delete") ;
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
	Ral_RelvarObjSetError(interp, RELVAR_CONSTRAINTS_PRESENT, fullName) ;
	ckfree(fullName) ;
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
 * HERE
 * This is wrong!
 * It should be:
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
    Tcl_DString resolve ;
    const char *resolvedName =
	Ral_RelvarObjResolveName(interp, name, &resolve) ;
    Ral_Relvar relvar = Ral_RelvarFind(info, resolvedName) ;

    if (relvar && fullName) {
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
    static const char specErrMsg[] = "multiplicity specification" ;

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

    /*
     * Creating an association is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_RelvarObjSetError(interp, RELVAR_BAD_TRANS_OP, "association") ;
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
	Ral_RelvarObjSetError(interp, RELVAR_BAD_MULT, Tcl_GetString(objv[6])) ;
	return TCL_ERROR ;
    }

    /*
     * The same number of attributes must be specified on each side of the
     * association.
     */
    if (elemc1 != elemc2) {
	Ral_RelvarObjSetError(interp, RELVAR_REFATTR_MISMATCH,
	    Tcl_GetString(objv[5])) ;
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
	Ral_RelvarObjSetError(interp, RELVAR_NOT_ID, Tcl_GetString(objv[5])) ;
	Ral_JoinMapDelete(refMap) ;
	return TCL_ERROR ;
    }

    constraint = Ral_ConstraintAssocCreate(name, info) ;
    if (constraint == NULL) {
	Ral_RelvarObjSetError(interp, RELVAR_DUP_CONSTRAINT, name) ;
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
    /*
     * Record which constraints apply to a given relvar. This is what allows
     * us to find the constraints that apply to a relvar when it is modified.
     * Treat the creation of the association as modifying the participating
     * relvars.
     */
    Ral_PtrVectorPushBack(relvar1->constraints, constraint) ;
    Ral_PtrVectorPushBack(relvar2->constraints, constraint) ;
    /*
     * Mark the relvars involved in the constraint as modified so that
     * the constraint will be checked.
     */
    Ral_RelvarStartTransaction(info, 0) ;
    Ral_RelvarStartCommand(info, relvar1) ;
    Ral_RelvarStartCommand(info, relvar2) ;
    return Ral_RelvarObjEndTrans(interp, info, 0) ;
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
    Ral_Relvar super ;
    Ral_Relation superRel ;
    int supElemc ;
    Tcl_Obj **supElemv ;
    Ral_IntVector superAttrs ;
    int nSupAttrs ;
    Ral_TupleHeading supth ;
    const char *partName ;
    Ral_Constraint constraint ;
    Ral_PartitionConstraint partition ;
    Ral_PtrVector subList ;	/* set of sub type names */

    /*
     * Creating a partition is not allowed during an "eval" script.
     */
    if (Ral_RelvarIsTransOnGoing(info)) {
	Ral_RelvarObjSetError(interp, RELVAR_BAD_TRANS_OP, "partition") ;
	return TCL_ERROR ;
    }
    /*
     * Look up the supertype and make sure that the value is truly a relation.
     */
    super = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(objv[1]),
	NULL) ;
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
	    Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(superAttrs) ;
	    return TCL_ERROR ;
	}
	status = Ral_IntVectorSetAdd(superAttrs, attrIndex) ;
	if (!status) {
	    Ral_RelationObjSetError(interp, REL_DUP_ATTR_IN_ID, attrName) ;
	    Ral_IntVectorDelete(superAttrs) ;
	    return TCL_ERROR ;
	}
	++supElemv ;
    }
    if (Ral_RelationHeadingFindIdentifier(superRel->heading, superAttrs) < 0) {
	Ral_RelvarObjSetError(interp, RELVAR_NOT_ID, Tcl_GetString(objv[2])) ;
	Ral_IntVectorDelete(superAttrs) ;
	return TCL_ERROR ;
    }
    nSupAttrs = Ral_IntVectorSize(superAttrs) ;
    /*
     * Create the partition constraint.
     */
    partName = Tcl_GetString(objv[0]) ;
    constraint = Ral_ConstraintPartitionCreate(partName, info) ;
    if (constraint == NULL) {
	Ral_RelvarObjSetError(interp, RELVAR_DUP_CONSTRAINT, partName) ;
	Ral_IntVectorDelete(superAttrs) ;
	return TCL_ERROR ;
    }
    partition = constraint->partition ;
    partition->referredToRelvar = super ;

    Ral_RelvarStartTransaction(info, 0) ;
    Ral_RelvarStartCommand(info, super) ;
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
	 * Get the subtype relvar and the attributes.
	 */
	if (!Ral_PtrVectorSetAdd(subList, subName)) {
	    Ral_RelvarObjSetError(interp, RELVAR_DUP_NAME, subName) ;
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
	    Ral_RelvarObjSetError(interp, RELVAR_REFATTR_MISMATCH,
		Tcl_GetString(objv[1])) ;
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
	    int attrIndex =
		Ral_TupleHeadingIndexOf(subth, Tcl_GetString(*subElemv)) ;

	    if (attrIndex < 0) {
		Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR,
		    Tcl_GetString(*subElemv)) ;
		goto errorOut ;
	    }
	    Ral_JoinMapAddAttrMapping(refMap, attrIndex, *supAttrIter++) ;
	    ++subElemv ;
	}
	Ral_RelvarStartCommand(info, sub) ;
	Ral_PtrVectorPushBack(sub->constraints, constraint) ;
    }

    Ral_IntVectorDelete(superAttrs) ;
    Ral_PtrVectorDelete(subList) ;
    return Ral_RelvarObjEndTrans(interp, info, 0) ;

errorOut:
    Ral_IntVectorDelete(superAttrs) ;
    Ral_PtrVectorDelete(subList) ;
    Ral_RelvarObjConstraintDelete(interp, partName, info) ;
    return Ral_RelvarObjEndTrans(interp, info, 1) ;
}

int
Ral_RelvarObjConstraintDelete(
    Tcl_Interp *interp,
    const char *name,
    Ral_RelvarInfo info)
{
    if (!Ral_ConstraintDeleteByName(name, info)) {
	Ral_RelvarObjSetError(interp, RELVAR_UNKNOWN_CONSTRAINT, name) ;
	return TCL_ERROR ;
    }
    return TCL_OK ;
}

int
Ral_RelvarObjConstraintInfo(
    Tcl_Interp *interp,
    Tcl_Obj * const nameObj,
    Ral_RelvarInfo info)
{
    const char *name = Tcl_GetString(nameObj) ;
    Ral_Constraint constraint ;
    Tcl_Obj *resultObj ;
    Tcl_Obj *attrList ;

    /*
     * Look up the constraint by it name.
     */
    constraint = Ral_ConstraintFindByName(name, info) ;
    if (constraint == NULL) {
	Ral_RelvarObjSetError(interp, RELVAR_UNKNOWN_CONSTRAINT, name) ;
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
	if (Tcl_ListObjAppendElement(interp, resultObj, nameObj) != TCL_OK) {
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
	if (Tcl_ListObjAppendElement(interp, resultObj, nameObj) != TCL_OK) {
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
	"referred to attributes do not form an identifier",
	"unknown constraint name",
	"relvar has constraints in place",
	"referred to identifiers can not have non-singular multiplicities",
	"operation is not allowed during \"eval\" command",
	"bad list of pairs",
    } ;
    static const char *errorStrings[] = {
	"OK",
	"DUP_NAME",
	"UNKNOWN_NAME",
	"HEADING_MISMATCH",
	"REFATTR_MISMATCH",
	"DUP_CONSTRAINT",
	"NOT_ID",
	"UNKNOWN_CONSTRAINT",
	"CONSTRAINTS_PRESENT",
	"BAD_MULT",
	"BAD_TRANS_OP",
	"BAD_PAIRS_LIST",
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
