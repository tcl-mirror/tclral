/*
This software is copyrighted 2004, 2005, 2006 by G. Andrew Mangogna.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files.

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

$RCSfile: ral_relvar.c,v $
$Revision: 1.6 $
$Date: 2006/07/01 23:56:31 $
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
#include "ral_relvar.h"
#include "ral_relationobj.h"
#include "ral_relation.h"

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
static void relvarCleanup(Ral_Relvar) ;
static void relvarSetIntRep(Ral_Relvar, Ral_Relation) ;
static int relvarAssocConstraintEval(const char *, Ral_AssociationConstraint,
    Tcl_DString *) ;
static int relvarPartitionConstraintEval(const char *,
    Ral_PartitionConstraint, Tcl_DString *) ;
static int relvarEvalAssocTupleCounts(Ral_IntVector, int, int, Ral_IntVector,
    Ral_IntVector) ;
static void relvarAssocConstraintErrorMsg(Tcl_DString *, const char *,
    Ral_AssociationConstraint, Ral_Relvar, Ral_IntVector, const char *) ;
static void relvarPartitionConstraintErrorMsg(Tcl_DString *, const char *,
    Ral_PartitionConstraint, Ral_Relvar, Ral_IntVector, const char *) ;
static void relvarConstraintErrorMsg(Tcl_DString *, const char *, Ral_Relation,
    Ral_IntVector, const char *) ;
static void relvarAssocConstraintToString(const char *,
    Ral_AssociationConstraint, Tcl_DString *) ;
static void relvarPartitionConstraintToString(const char *,
    Ral_PartitionConstraint, Tcl_DString *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/
Ral_RelvarError Ral_RelvarLastError = RELVAR_OK ;

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relvar.c,v $ $Revision: 1.6 $" ;

/*
FUNCTION DEFINITIONS
*/

Ral_Relvar
Ral_RelvarNew(
    Ral_RelvarInfo info,
    const char *name,
    Ral_RelationHeading heading)
{
    int newPtr ;
    Tcl_HashEntry *entry ;
    Ral_Relvar relvar = NULL ;

    entry = Tcl_CreateHashEntry(&info->relvars, name, &newPtr) ;
    if (newPtr) {
	    /* +1 for NUL terminator */
	relvar = (Ral_Relvar)ckalloc(sizeof(*relvar) + strlen(name) + 1) ;
	relvar->name = (char *)(relvar + 1) ;
	strcpy(relvar->name, name) ;
	Tcl_IncrRefCount(
	    relvar->relObj = Ral_RelationObjNew(Ral_RelationNew(heading))) ;
	relvar->transStack = Ral_PtrVectorNew(1) ;
	relvar->constraints = Ral_PtrVectorNew(0) ;
	Tcl_SetHashValue(entry, relvar) ;
    } else {
	/*
	 * Duplicate name.
	 */
	Ral_RelvarLastError = RELVAR_DUP_NAME ;
    }

    return relvar ;
}

void
Ral_RelvarDelete(
    Ral_RelvarInfo info,
    const char *name)
{
    Tcl_HashEntry *entry ;
    Ral_Relvar relvar ;

    /*
     * Clean out the variable name from the hash table so that subsequent
     * commands will not see it.
     */
    entry = Tcl_FindHashEntry(&info->relvars, name) ;
    assert(entry != NULL) ;
    relvar = (Ral_Relvar)Tcl_GetHashValue(entry) ;
    Tcl_DeleteHashEntry(entry) ;
    /*
     * Remove the object.
     */
    relvarCleanup(relvar) ;
}

Ral_Relvar
Ral_RelvarFind(
    Ral_RelvarInfo info,
    const char *name)
{
    Tcl_HashEntry *entry ;
    Ral_Relvar relvar = NULL ;

    entry = Tcl_FindHashEntry(&info->relvars, name) ;
    if (entry) {
	relvar = Tcl_GetHashValue(entry) ;
    }

    return relvar ;
}

ClientData
Ral_RelvarNewInfo(
    const char *name,
    Tcl_Interp *interp)
{
    Ral_RelvarInfo info = (Ral_RelvarInfo)ckalloc(sizeof(*info)) ;
    memset(info, 0, sizeof(*info)) ;
    info->transactions = Ral_PtrVectorNew(1) ;
    Tcl_InitHashTable(&info->relvars, TCL_STRING_KEYS) ;
    Tcl_InitHashTable(&info->constraints, TCL_STRING_KEYS) ;
    Tcl_SetAssocData(interp, name, Ral_RelvarDeleteInfo, info) ;

    return (ClientData)info ;
}

void
Ral_RelvarDeleteInfo(
    ClientData cd,
    Tcl_Interp *interp)
{
    Ral_RelvarInfo info = (Ral_RelvarInfo)cd ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;

    /*
     * We should only be deleting the relvar info outside of a transaction.
     */
    assert(Ral_PtrVectorSize(info->transactions) == 0) ;
    Ral_PtrVectorDelete(info->transactions) ;

    for (entry = Tcl_FirstHashEntry(&info->relvars, &search) ;
	entry ; entry = Tcl_NextHashEntry(&search)) {
	relvarCleanup(Tcl_GetHashValue(entry)) ;
    }
    Tcl_DeleteHashTable(&info->relvars) ;
    for (entry = Tcl_FirstHashEntry(&info->constraints, &search) ;
	entry ; entry = Tcl_NextHashEntry(&search)) {
	 Ral_ConstraintDelete(Tcl_GetHashValue(entry)) ;
    }
    Tcl_DeleteHashTable(&info->relvars) ;
    ckfree((char *)info) ;
}

void
Ral_RelvarStartTransaction(
    Ral_RelvarInfo info,
    int single)
{
    Ral_RelvarTransaction newTrans = Ral_RelvarNewTransaction() ;
    newTrans->isSingleCmd = single ;
    Ral_PtrVectorPushBack(info->transactions, newTrans) ;
}

int
Ral_RelvarEndTransaction(
    Ral_RelvarInfo info,
    int failed,
    Tcl_DString *errMsg)
{
    Ral_RelvarTransaction trans = Ral_PtrVectorBack(info->transactions) ;
    Ral_PtrVectorIter pIter ;
    Ral_PtrVectorIter pEnd ;

    if (!failed) {
	Ral_PtrVector constraints = Ral_PtrVectorNew(0) ;
	Ral_PtrVectorIter cEnd ;
	Ral_PtrVectorIter cIter ;
	/*
	 * Iterate across the transaction evaluating the constraints.
	 * Build a set of constraints that the modified relvars
	 * participate in and then evaluate all the constraints in that set.
	 * "trans->modified" is the set of relvars that were modified
	 * during the transaction.
	 */
	pEnd = Ral_PtrVectorEnd(trans->modifiedRelvars) ;
	for (pIter = Ral_PtrVectorBegin(trans->modifiedRelvars) ;
	    pIter != pEnd ; ++pIter) {
	    Ral_Relvar modRelvar = *pIter ;
	    Ral_PtrVectorIter rcEnd = Ral_PtrVectorEnd(modRelvar->constraints) ;
	    Ral_PtrVectorIter rcIter ;
	    for (rcIter = Ral_PtrVectorBegin(modRelvar->constraints) ;
		rcIter != rcEnd ; ++rcIter) {
		Ral_PtrVectorSetAdd(constraints, *rcIter) ;
	    }
	}
	cEnd = Ral_PtrVectorEnd(constraints) ;
	for (cIter = Ral_PtrVectorBegin(constraints) ; cIter != cEnd ;
	    ++cIter) {
	    failed += !Ral_RelvarConstraintEval(*cIter, errMsg) ;
	}
	Ral_PtrVectorDelete(constraints) ;
    }
    pEnd = Ral_PtrVectorEnd(trans->modifiedRelvars) ;
    if (failed) {
	/*
	 * If any constaint fails, pop and restore the saved values of
	 * the relvars.
	 */
	for (pIter = Ral_PtrVectorBegin(trans->modifiedRelvars) ;
	    pIter != pEnd ; ++pIter) {
	    Ral_RelvarRestorePrev(*pIter) ;
	}
    } else {
	/*
	 * Pop and discard the saved values.
	 */
	for (pIter = Ral_PtrVectorBegin(trans->modifiedRelvars) ;
	    pIter != pEnd ; ++pIter) {
	    Ral_RelvarDiscardPrev(*pIter) ;
	}
    }
    /*
     * Delete the transaction.
     */
    Ral_RelvarDeleteTransaction(trans) ;
    Ral_PtrVectorPopBack(info->transactions) ;

    return !failed ;
}

int
Ral_RelvarIsTransOnGoing(
    Ral_RelvarInfo info)
{
    return Ral_PtrVectorSize(info->transactions) > 0 ;
}

void
Ral_RelvarStartCommand(
    Ral_RelvarInfo info,
    Ral_Relvar relvar)
{
    Ral_RelvarTransaction currTrans ;
    if (Ral_PtrVectorSize(info->transactions) == 0) {
	Ral_RelvarStartTransaction(info, 1) ;
    }
    assert(Ral_PtrVectorSize(info->transactions) > 0) ;
    currTrans = Ral_PtrVectorBack(info->transactions) ;
    int added = Ral_RelvarTransModifiedRelvar(info, relvar) ;
    if (added) {
	Ral_Relation rel = relvar->relObj->internalRep.otherValuePtr ;
	Ral_PtrVectorPushBack(relvar->transStack, Ral_RelationDup(rel)) ;
    }
}

int
Ral_RelvarEndCommand(
    Ral_RelvarInfo info,
    int failed,
    Tcl_DString *errMsg)
{
    Ral_RelvarTransaction trans ;
    int success = !failed ;

    assert(Ral_PtrVectorSize(info->transactions) > 0) ;
    trans = Ral_PtrVectorBack(info->transactions) ;
    if (trans->isSingleCmd) {
	assert(Ral_PtrVectorSize(info->transactions) == 1) ;
	/*
	 * End the single command transaction. 
	 */
	success = Ral_RelvarEndTransaction(info, failed, errMsg) ;
    }

    return success ;
}

int
Ral_RelvarTransModifiedRelvar(
    Ral_RelvarInfo info,
    Ral_Relvar relvar)
{
    Ral_RelvarTransaction trans ;

    assert(Ral_PtrVectorSize(info->transactions) > 0) ;
    trans = Ral_PtrVectorBack(info->transactions) ;
    return Ral_PtrVectorSetAdd(trans->modifiedRelvars, relvar) ;
}

Ral_RelvarTransaction
Ral_RelvarNewTransaction(void)
{
    Ral_RelvarTransaction trans ;

    trans = (Ral_RelvarTransaction)ckalloc(sizeof(*trans)) ;
	/*
	 * 1, so that we don't reallocate immediately
	 */
    trans->modifiedRelvars = Ral_PtrVectorNew(1) ;
    return trans ;
}

void
Ral_RelvarDeleteTransaction(
    Ral_RelvarTransaction trans)
{
    Ral_PtrVectorDelete(trans->modifiedRelvars) ;
    ckfree((char *)trans) ;
}

Ral_Constraint
Ral_ConstraintAssocCreate(
    const char *name,
    Ral_RelvarInfo info)
{
    int newPtr ;
    Tcl_HashEntry *entry ;
    Ral_Constraint constraint = NULL ;

    entry = Tcl_CreateHashEntry(&info->constraints, name, &newPtr) ;
    if (newPtr) {
	constraint = Ral_ConstraintNewAssociation(name) ;
	Tcl_SetHashValue(entry, constraint) ;
    }

    return constraint ;
}

Ral_Constraint
Ral_ConstraintPartitionCreate(
    const char *name,
    Ral_RelvarInfo info)
{
    int newPtr ;
    Tcl_HashEntry *entry ;
    Ral_Constraint constraint = NULL ;

    entry = Tcl_CreateHashEntry(&info->constraints, name, &newPtr) ;
    if (newPtr) {
	constraint = Ral_ConstraintNewPartition(name) ;
	Tcl_SetHashValue(entry, constraint) ;
    }

    return constraint ;
}

int
Ral_ConstraintDeleteByName(
    const char *name,
    Ral_RelvarInfo info)
{
    int deleted = 0 ;
    Tcl_HashEntry *entry ;

    entry = Tcl_FindHashEntry(&info->constraints, name) ;
    if (entry) {
	Ral_Constraint constraint = (Ral_Constraint)Tcl_GetHashValue(entry) ;
	Ral_ConstraintDelete(constraint) ;
	Tcl_DeleteHashEntry(entry) ;
	deleted = 1 ;
    }
    return deleted ;
}

Ral_Constraint
Ral_ConstraintFindByName(
    const char *name,
    Ral_RelvarInfo info)
{
    Tcl_HashEntry *entry ;
    Ral_Constraint constraint = NULL ;

    entry = Tcl_FindHashEntry(&info->constraints, name) ;
    if (entry) {
	constraint = (Ral_Constraint)Tcl_GetHashValue(entry) ;
    }

    return constraint ;
}

static Ral_Constraint
Ral_ConstraintNew(
    const char *name)
{
    Ral_Constraint constraint ;

    constraint = (Ral_Constraint)ckalloc(sizeof(*constraint) + strlen(name)
	+ 1) ; /* +1 for NUL terminator */
    constraint->name = (char *)(constraint + 1) ;
    strcpy(constraint->name, name) ;

    return constraint ;
}

Ral_Constraint
Ral_ConstraintNewAssociation(
    const char *name)
{
    Ral_Constraint constraint = Ral_ConstraintNew(name) ;

    constraint->type = ConstraintAssociation ;
    constraint->association = (Ral_AssociationConstraint)ckalloc(
	sizeof(struct Ral_AssociationConstraint)) ;

    return constraint ;
}

Ral_Constraint
Ral_ConstraintNewPartition(
    const char *name)
{
    Ral_Constraint constraint = Ral_ConstraintNew(name) ;

    constraint->type = ConstraintPartition ;
    constraint->partition = (Ral_PartitionConstraint)ckalloc(
	sizeof(struct Ral_PartitionConstraint)) ;
    constraint->partition->subsetReferences = Ral_PtrVectorNew(2) ;

    return constraint ;
}

void
Ral_ConstraintDelete(
    Ral_Constraint constraint)
{
    switch (constraint->type) {
    case ConstraintAssociation:
    {
	Ral_AssociationConstraint assoc = constraint->association ;
	Ral_Relvar referring = assoc->referringRelvar ;
	Ral_Relvar referred = assoc->referredToRelvar ;
	Ral_PtrVectorIter found ;

	/*
	 * Find the constraint in the list of constraints associated
	 * with each relvar that participates in the association.
	 * We have to remove these references before deleting the
	 * constraint data structure.
	 */
	found = Ral_PtrVectorFind(referring->constraints, constraint) ;
	/*
	 * We allow the constraint to not be found in case this
	 * is called as a clean up when the constraint is only
	 * partially formed.
	 */
	if (found != Ral_PtrVectorEnd(referring->constraints)) {
	    Ral_PtrVectorErase(referring->constraints, found, found + 1) ;
	}

	found = Ral_PtrVectorFind(referred->constraints, constraint) ;
	if (found != Ral_PtrVectorEnd(referred->constraints)) {
	    Ral_PtrVectorErase(referred->constraints, found, found + 1) ;
	}
	if (assoc->referenceMap) {
	    Ral_JoinMapDelete(assoc->referenceMap) ;
	}
	ckfree((char *)assoc) ;
    }
	break ;

    case ConstraintPartition:
    {
	Ral_PartitionConstraint partition = constraint->partition ;
	Ral_Relvar super = partition->referredToRelvar ;
	Ral_PtrVectorIter found ;
	Ral_PtrVectorIter sEnd ;
	Ral_PtrVectorIter sIter ;

	/*
	 * Find the pointer to the constraint in the super type and remove it.
	 */
	found = Ral_PtrVectorFind(super->constraints, constraint) ;
	if (found != Ral_PtrVectorEnd(super->constraints)) {
	    Ral_PtrVectorErase(super->constraints, found, found + 1) ;
	}
	/*
	 * Now iterate through the subtypes and remove the constraint
	 * references found there.
	 */
	sEnd = Ral_PtrVectorEnd(partition->subsetReferences) ;
	for (sIter = Ral_PtrVectorBegin(partition->subsetReferences) ;
	    sIter != sEnd ; ++sIter) {
	    Ral_SubsetReference subRef = *sIter ;
	    Ral_Relvar sub = subRef->relvar ;

	    found = Ral_PtrVectorFind(sub->constraints, constraint) ;
	    if (found != Ral_PtrVectorEnd(sub->constraints)) {
		Ral_PtrVectorErase(sub->constraints, found, found + 1) ;
	    }
	}

	sEnd = Ral_PtrVectorEnd(partition->subsetReferences) ;
	for (sIter = Ral_PtrVectorBegin(partition->subsetReferences) ;
	    sIter != sEnd ; ++sIter) {
	    Ral_SubsetReference subRef = *sIter ;
	    if (subRef->subsetMap) {
		Ral_JoinMapDelete(subRef->subsetMap) ;
	    }
	}
	Ral_PtrVectorDelete(partition->subsetReferences) ;
	ckfree((char *)constraint->partition) ;
    }
	break ;

    default:
	Tcl_Panic("Ral_ConstraintDelete: unknown constraint type, %d",
	    constraint->type) ;
    }

    ckfree((char *)constraint) ;
}

int
Ral_RelvarConstraintEval(
    Ral_Constraint constraint,
    Tcl_DString *errMsg)
{
    switch (constraint->type) {
    case ConstraintAssociation:
	return relvarAssocConstraintEval(constraint->name,
	    constraint->association, errMsg) ;
	break ;

    case ConstraintPartition:
	return relvarPartitionConstraintEval(constraint->name,
	    constraint->partition, errMsg) ;
	break ;

    default:
	Tcl_Panic("unknown constraint type, %d", constraint->type) ;
	return 0 ;
    }
    /* NOT REACHED */
}

void
Ral_RelvarSetRelation(
    Ral_Relvar relvar,
    Ral_Relation newRel)
{
    Ral_Relation oldRel ;
    Ral_Relation copyRel ;
    Ral_IntVector orderMap ;
    int copied ;

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    oldRel = relvar->relObj->internalRep.otherValuePtr ;
    /*
     * Copy the new relation into a heading that matches the old relation.
     * This is necessary since the constraints and other relvar info
     * contain attribute indices that correspond to the heading of the
     * old relation.
     */
    copyRel = Ral_RelationNew(oldRel->heading) ;
    Ral_RelationReserve(copyRel, Ral_RelationCardinality(newRel)) ;
    orderMap = Ral_TupleHeadingNewOrderMap(copyRel->heading->tupleHeading,
	newRel->heading->tupleHeading) ;
    copied = Ral_RelationCopy(newRel, Ral_RelationBegin(newRel),
	Ral_RelationEnd(newRel), copyRel, orderMap) ;
    assert(copied == 1) ;
    Ral_IntVectorDelete(orderMap) ;
    /*
     * Free up the old relation and install the copy.
     */
    relvarSetIntRep(relvar, copyRel) ;
}

/*
 * When restoring the value saved during the transaction, we
 * have to set the underlying relation back to the old value.
 */
void
Ral_RelvarRestorePrev(
    Ral_Relvar relvar)
{
    Ral_Relation oldRel = Ral_PtrVectorBack(relvar->transStack) ;
    Ral_PtrVectorPopBack(relvar->transStack) ;
    relvarSetIntRep(relvar, oldRel) ;
}

void
Ral_RelvarDiscardPrev(
    Ral_Relvar relvar)
{
    Ral_RelationDelete(Ral_PtrVectorBack(relvar->transStack)) ;
    Ral_PtrVectorPopBack(relvar->transStack) ;
}

/*
 * PRIVATE FUNCTIONS
 */

static void
relvarCleanup(
    Ral_Relvar relvar)
{
    if (relvar->relObj) {
	assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
	Tcl_DecrRefCount(relvar->relObj) ;
    }

    for ( ; Ral_PtrVectorSize(relvar->transStack) != 0 ;
	Ral_PtrVectorPopBack(relvar->transStack)) {
	Ral_RelationDelete(Ral_PtrVectorBack(relvar->transStack)) ;
    }
    Ral_PtrVectorDelete(relvar->transStack) ;
    Ral_PtrVectorDelete(relvar->constraints) ;

    ckfree((char *)relvar) ;
}

static void
relvarSetIntRep(
    Ral_Relvar relvar,
    Ral_Relation relation)
{
    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relvar->relObj->typePtr->freeIntRepProc(relvar->relObj) ;
    relvar->relObj->internalRep.otherValuePtr = relation ;
    relvar->relObj->typePtr = &Ral_RelationObjType ;
    Tcl_InvalidateStringRep(relvar->relObj) ;
    relvar->relObj->length = 0 ;
}

/*
 * Evaluate an association type constraint.
 * Return 1 if the constraint is satisfied,  0 otherwise.
 * On error, "errMsg" if it is non-NULL, contains text to identify the error.
 * Assumes that, "errMsg" is properly initialized on entry.
 */
static int
relvarAssocConstraintEval(
    const char *name,			    /* name of constraint */
    Ral_AssociationConstraint association,  /* association constraint */
    Tcl_DString *errMsg)		    /* error message left here */
{
    Ral_Relvar referringRelvar = association->referringRelvar ;
    Ral_Relvar referredToRelvar = association->referredToRelvar ;
    Ral_Relation referringRel ;
    Ral_Relation referredToRel ;
    Ral_IntVector cnts ;
    Ral_IntVector multViolations ;
    Ral_IntVector condViolations ;
    int ref_result ;
    int refTo_result ;

    assert(referringRelvar->relObj->typePtr == &Ral_RelationObjType) ;
    assert(referredToRelvar->relObj->typePtr == &Ral_RelationObjType) ;
    referringRel = referringRelvar->relObj->internalRep.otherValuePtr ;
    referredToRel = referredToRelvar->relObj->internalRep.otherValuePtr ;
    /*
     * Map the tuples as if they were to be joined.
     */
    Ral_RelationFindJoinTuples(referringRel, referredToRel,
	association->referenceMap) ;
    /*
     * Now we count the tuples in the mapping and verify that the join
     * would match the conditionality and multiplicity of the association.
     * So if we count the number of times each tuple in the referredTo relation
     * is found among the referring tuples, then if any of those counts is > 1
     * then the referrring multiplicity must be true and if any == 0 then the
     * referring conditionality must be true.
     */
    cnts = Ral_IntVectorNew(Ral_RelationCardinality(referredToRel), 0) ;
    Ral_JoinMapTupleCounts(association->referenceMap, 1, cnts) ;

    multViolations = Ral_IntVectorNewEmpty(0) ;
    condViolations = Ral_IntVectorNewEmpty(0) ;
    ref_result = relvarEvalAssocTupleCounts(cnts, association->referringMult,
	association->referringCond, multViolations, condViolations) ;
    Ral_IntVectorDelete(cnts) ;
    /*
     * Leave error messages.
     */
    if (!ref_result) {
	relvarAssocConstraintErrorMsg(errMsg, name, association,
	    referredToRelvar, multViolations,
	    "is referenced by multiple tuples") ;
	relvarAssocConstraintErrorMsg(errMsg, name, association,
	    referredToRelvar, condViolations,
	    "is not referenced by any tuple") ;
    }
    Ral_IntVectorDelete(multViolations) ;
    Ral_IntVectorDelete(condViolations) ;
    /*
     * Now do the same for the referred to direction.
     */
    cnts = Ral_IntVectorNew(Ral_RelationCardinality(referringRel), 0) ;
    Ral_JoinMapTupleCounts(association->referenceMap, 0, cnts) ;

    multViolations = Ral_IntVectorNewEmpty(0) ;
    condViolations = Ral_IntVectorNewEmpty(0) ;
    refTo_result = relvarEvalAssocTupleCounts(cnts, association->referredToMult,
	association->referredToCond, multViolations, condViolations) ;
    Ral_IntVectorDelete(cnts) ;
    if (!refTo_result) {
	relvarAssocConstraintErrorMsg(errMsg, name, association,
	    referringRelvar, multViolations, "references multiple tuples") ;
	relvarAssocConstraintErrorMsg(errMsg, name, association,
	    referringRelvar, condViolations, "references no tuple") ;
    }
    Ral_IntVectorDelete(multViolations) ;
    Ral_IntVectorDelete(condViolations) ;
    /*
     * Clean out the tuple matches from the join map.
     */
    Ral_JoinMapTupleEmpty(association->referenceMap) ;
    /*
     * Get rid of the trailing newline.
     */
    if (errMsg &&
	*(Tcl_DStringValue(errMsg) + Tcl_DStringLength(errMsg) - 1) == '\n') {
	Tcl_DStringSetLength(errMsg, Tcl_DStringLength(errMsg) - 1) ;
    }

    return ref_result && refTo_result ;
}

/*
 * Evaluate a partition type constraint.
 * Return 1 if the constraint is satisfied,  0 otherwise.
 * On error, "errMsg" if it is non-NULL, contains text to identify the error.
 * Assumes that, "errMsg" is properly initialized on entry.
 *
 * Partition constraints insist that each subtype reference exactly
 * one tuple in the supertype and that every tuple in the supertype
 * is referred to by exactly one tuple from among all the subtypes.
 */
static int
relvarPartitionConstraintEval(
    const char *name,
    Ral_PartitionConstraint partition,
    Tcl_DString *errMsg)
{
    int result = 1 ;
    Ral_Relvar super = partition->referredToRelvar ;
    Ral_Relation superRel ;
    Ral_IntVector superMatches ;
    Ral_PtrVectorIter subEnd = Ral_PtrVectorEnd(partition->subsetReferences) ;
    Ral_PtrVectorIter subIter ;
    Ral_IntVectorIter mEnd ;
    Ral_IntVectorIter mIter ;
    Ral_IntVector multViolations ;
    Ral_IntVector condViolations ;

    assert(super->relObj->typePtr == &Ral_RelationObjType) ;
    superRel = super->relObj->internalRep.otherValuePtr ;
    superMatches = Ral_IntVectorNew(Ral_RelationCardinality(superRel), 0) ;

    for (subIter = Ral_PtrVectorBegin(partition->subsetReferences) ;
	subIter != subEnd ; ++subIter) {
	Ral_SubsetReference subRef = *subIter ;
	Ral_Relvar sub = subRef->relvar ;
	Ral_Relation subRel ;
	Ral_JoinMap subMap = subRef->subsetMap ;
	int subMatches ;

	assert(sub->relObj->typePtr == &Ral_RelationObjType) ;
	subRel = sub->relObj->internalRep.otherValuePtr ;
	/*
	 * Find the join from the subtype to the supertype.
	 */
	Ral_RelationFindJoinTuples(subRel, superRel, subMap) ;
	/*
	 * The number of matching tuple entries must be the same as
	 * the cardinality of the subtype.
	 */
	subMatches = Ral_JoinMapTupleCounts(subMap, 1, superMatches) ;
	if (subMatches != Ral_RelationCardinality(subRel)) {
	    Ral_IntVector violations = Ral_IntVectorNewEmpty(1) ;
	    Ral_IntVector matchMap = Ral_JoinMapTupleMap(subMap, 0,
		Ral_RelationCardinality(subRel)) ;
	    Ral_IntVectorIter mEnd = Ral_IntVectorEnd(matchMap) ;
	    Ral_IntVectorIter mIter ;

	    /*
	     * We must find the tuple that did not match anything in
	     * the supertype. We can do this by getting the map
	     * and examining it.
	     */
	    for (mIter = Ral_IntVectorBegin(matchMap) ; mIter != mEnd ;
		++mIter) {
		if (*mIter) {
		    Ral_IntVectorPushBack(violations,
			Ral_IntVectorOffsetOf(matchMap, mIter)) ;
		}
	    }
	    Ral_IntVectorDelete(matchMap) ;

	    relvarPartitionConstraintErrorMsg(errMsg, name, partition,
		sub, violations, "references no tuple") ;
	    Ral_IntVectorDelete(violations) ;

	    result = 0 ;
	}
	/*
	 * Clear out the matches for the next time the constraint is evaluated.
	 */
	Ral_JoinMapTupleEmpty(subMap) ;
    }

    /*
     * At this point, the "superMatches" vector has accumulated the number
     * of times a given tuple in the super type has been matched.
     * This must be exactly one or it is a violation of the partition
     * constraint.
     */
    multViolations = Ral_IntVectorNewEmpty(0) ;
    condViolations = Ral_IntVectorNewEmpty(0) ;
    mEnd = Ral_IntVectorEnd(superMatches) ;
    for (mIter = Ral_IntVectorBegin(superMatches) ; mIter != mEnd ; ++mIter) {
	int matchCount = *mIter ;

	if (matchCount != 1) {
	    if (matchCount > 1) {
		Ral_IntVectorPushBack(multViolations,
		    Ral_IntVectorOffsetOf(superMatches, mIter)) ;
	    } else /* matchCount == 0 */ {
		Ral_IntVectorPushBack(condViolations,
		    Ral_IntVectorOffsetOf(superMatches, mIter)) ;
	    }
	    result = 0 ;
	}
    }
    relvarPartitionConstraintErrorMsg(errMsg, name, partition, super,
	multViolations, "is referred to by multiple tuples") ;
    relvarPartitionConstraintErrorMsg(errMsg, name, partition, super,
	condViolations, "is not referred to by any tuple") ;

    Ral_IntVectorDelete(multViolations) ;
    Ral_IntVectorDelete(condViolations) ;
    Ral_IntVectorDelete(superMatches) ;
    /*
     * Get rid of the trailing newline.
     */
    if (errMsg &&
	*(Tcl_DStringValue(errMsg) + Tcl_DStringLength(errMsg) - 1) == '\n') {
	Tcl_DStringSetLength(errMsg, Tcl_DStringLength(errMsg) - 1) ;
    }
    return result ;
}

static int
relvarEvalAssocTupleCounts(
    Ral_IntVector counts,
    int multiplicity,
    int conditionality,
    Ral_IntVector multViolate,
    Ral_IntVector condViolate)
{
    int result = 1 ;
    Ral_IntVectorIter cEnd ;
    Ral_IntVectorIter cIter ;

    cEnd = Ral_IntVectorEnd(counts) ;
    for (cIter = Ral_IntVectorBegin(counts) ; cIter != cEnd ; ++cIter) {
	Ral_IntVectorValueType count = *cIter ;
	if (count != 1) {
	    if (count > 1 && multiplicity == 0) {
		/*
		 * multiplicity violation
		 */
		result = 0 ;
		Ral_IntVectorSetAdd(multViolate,
		    Ral_IntVectorOffsetOf(counts, cIter)) ;
	    } else if (count == 0 && conditionality == 0) {
		/*
		 * conditionality violation.
		 */
		result = 0 ;
		Ral_IntVectorSetAdd(condViolate,
		    Ral_IntVectorOffsetOf(counts, cIter)) ;
	    }
	}
    }

    return result ;
}

static void
relvarAssocConstraintErrorMsg(
    Tcl_DString *msg,		    /* error text appended here */
    const char *name,		    /* constraint name */
    Ral_AssociationConstraint assoc,/* association contraint in error */
    Ral_Relvar relvar,		    /* the relvar in error */
    Ral_IntVector violations,	    /* a list of tuple indices in error */
    const char *detail)		    /* text detail for the error */
{
    Ral_Relation rel ;

    if (msg == NULL || Ral_IntVectorSize(violations) == 0) {
	return ;
    }

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    rel = relvar->relObj->internalRep.otherValuePtr ;

    Tcl_DStringAppend(msg, "for association ", -1) ;
    relvarAssocConstraintToString(name, assoc, msg) ;
    relvarConstraintErrorMsg(msg, relvar->name, rel, violations, detail) ;
}

static void
relvarPartitionConstraintErrorMsg(
    Tcl_DString *msg,		    /* error text appended here */
    const char *name,		    /* constraint name */
    Ral_PartitionConstraint partition,/* partition contraint in error */
    Ral_Relvar relvar,		    /* the relvar in error */
    Ral_IntVector violations,	    /* a list of tuple indices in error */
    const char *detail)		    /* text detail for the error */
{
    Ral_Relation rel ;

    if (msg == NULL || Ral_IntVectorSize(violations) == 0) {
	return ;
    }

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    rel = relvar->relObj->internalRep.otherValuePtr ;

    Tcl_DStringAppend(msg, "for partition ", -1) ;
    relvarPartitionConstraintToString(name, partition, msg) ;
    relvarConstraintErrorMsg(msg, relvar->name, rel, violations, detail) ;
}

static void
relvarConstraintErrorMsg(
    Tcl_DString *msg,
    const char *relvarName,
    Ral_Relation rel,
    Ral_IntVector violations,
    const char *detail)
{
    Ral_IntVectorIter vEnd = Ral_IntVectorEnd(violations) ;
    Ral_IntVectorIter vIter ;

    Tcl_DStringAppend(msg, ", in relvar ", -1) ;
    Tcl_DStringAppend(msg, relvarName, -1) ;
    Tcl_DStringAppend(msg, "\n", -1) ;

    for (vIter = Ral_IntVectorBegin(violations) ; vIter != vEnd ; ++vIter) {
	Ral_Tuple errTuple = Ral_RelationTupleAt(rel, *vIter) ;
	char *tupleString = Ral_TupleValueStringOf(errTuple) ;

	Tcl_DStringAppend(msg, "tuple ", -1) ;
	Tcl_DStringAppend(msg, tupleString, -1) ;
	ckfree(tupleString) ;
	Tcl_DStringAppend(msg, " ", -1) ;
	Tcl_DStringAppend(msg, detail, -1) ;
	Tcl_DStringAppend(msg, "\n", -1) ;
    }
}

static void
relvarAssocConstraintToString(
    const char *name,
    Ral_AssociationConstraint assoc,
    Tcl_DString *result)
{
    static char const * const condMultStrings[2][2] = {
	{"1", "+"},
	{"?", "*"}
    } ;

    Ral_Relvar referringRelvar = assoc->referringRelvar ;
    Ral_Relvar referredToRelvar = assoc->referredToRelvar ;

    Tcl_DStringAppend(result, name, -1) ;
    Tcl_DStringAppend(result, "(", -1) ;
    Tcl_DStringAppend(result, referringRelvar->name, -1) ;
    Tcl_DStringAppend(result, " [", -1) ;
    Tcl_DStringAppend(result,
	condMultStrings[assoc->referringCond][assoc->referringMult], -1) ;
    Tcl_DStringAppend(result, "] ==> [", -1) ;
    Tcl_DStringAppend(result,
	condMultStrings[assoc->referredToCond][assoc->referredToMult], -1) ;
    Tcl_DStringAppend(result, "] ", -1) ;
    Tcl_DStringAppend(result, referredToRelvar->name, -1) ;
    Tcl_DStringAppend(result, ")", -1) ;
}

static void
relvarPartitionConstraintToString(
    const char *name,
    Ral_PartitionConstraint partition,
    Tcl_DString *result)
{
    Ral_Relvar super = partition->referredToRelvar ;
    Ral_PtrVector subRefs = partition->subsetReferences ;
    Ral_PtrVectorIter rEnd = Ral_PtrVectorEnd(subRefs) ;
    Ral_PtrVectorIter rIter ;

    Tcl_DStringAppend(result, name, -1) ;
    Tcl_DStringAppend(result, "(", -1) ;
    Tcl_DStringAppend(result, super->name, -1) ;
    Tcl_DStringAppend(result, " is partitioned [", -1) ;

    for (rIter = Ral_PtrVectorBegin(subRefs) ; rIter != rEnd ; ++rIter) {
	Ral_SubsetReference subRef = *rIter ;
	Ral_Relvar sub = subRef->relvar ;
	Tcl_DStringAppend(result, sub->name, -1) ;
	Tcl_DStringAppend(result, " | ", -1) ;
    }
    /*
     * Remove the trailing space, vertical bar and space.
     */
    Tcl_DStringSetLength(result, Tcl_DStringLength(result) - 3) ;

    Tcl_DStringAppend(result, "])", -1) ;
}
