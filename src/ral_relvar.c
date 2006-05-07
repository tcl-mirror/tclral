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
$Revision: 1.3 $
$Date: 2006/05/07 03:53:28 $
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
static int relvarEvalTupleCounts(Ral_IntVector, int, int, Ral_IntVector,
    Ral_IntVector) ;
static void relvarAssocConstraintErrorMsg(Tcl_DString *, const char *,
    Ral_AssociationConstraint, Ral_Relvar, Ral_IntVector, const char *) ;
static void relvarAssocConstraintToString(const char *,
    Ral_AssociationConstraint, Tcl_DString *) ;

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
static const char rcsid[] = "@(#) $RCSfile: ral_relvar.c,v $ $Revision: 1.3 $" ;

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
    Ral_RelvarTransaction currTrans ;
    int added ;

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
    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    Tcl_DecrRefCount(relvar->relObj) ;
    relvar->relObj = NULL ; /* just to be tidy */
    /*
     * Insert the relvar into the pending delete set.  We must not clean it out
     * all of its data structure until the end of the transaction so that
     * constraint evaluation can take place.
     */
    assert(Ral_PtrVectorSize(info->transactions) > 0) ;
    currTrans = Ral_PtrVectorBack(info->transactions) ;
    added = Ral_PtrVectorSetAdd(currTrans->pendingDelete, relvar) ;
    assert(added != 0) ;
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
    Ral_RelvarTransaction trans ;
    Ral_PtrVectorIter modIter ;
    Ral_PtrVectorIter modEnd ;

    trans = Ral_PtrVectorBack(info->transactions) ;
    assert(trans->isSingleCmd == 0) ;
    Ral_PtrVectorPopBack(info->transactions) ;

    modEnd = Ral_PtrVectorEnd(trans->modified) ;
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
	for (modIter = Ral_PtrVectorBegin(trans->modified) ; modIter != modEnd ;
	    ++modIter) {
	    Ral_Relvar modRelvar = *modIter ;
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
    if (failed) {
	/*
	 * If any constaint fails, pop and restore the saved values of
	 * the relvars.
	 */
	for (modIter = Ral_PtrVectorBegin(trans->modified) ; modIter != modEnd ;
	    ++modIter) {
	    Ral_RelvarRestorePrev(*modIter) ;
	}
    } else {
	/*
	 * If all constraints pass, pop and discard the saved values.
	 */
	for (modIter = Ral_PtrVectorBegin(trans->modified) ; modIter != modEnd ;
	    ++modIter) {
	    Ral_RelvarDiscardPrev(*modIter) ;
	}
	/*
	 * Get rid of any relvars marked as pending delete.
	 */
	modEnd = Ral_PtrVectorEnd(trans->pendingDelete) ;
	for (modIter = Ral_PtrVectorBegin(trans->pendingDelete) ;
	    modIter != modEnd ; ++modIter) {
	    relvarCleanup(*modIter) ;
	}
    }
    /*
     * Delete the transaction.
     */
    Ral_RelvarDeleteTransaction(trans) ;

    return !failed ;
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
    int added = Ral_PtrVectorSetAdd(currTrans->modified, relvar) ;
    if (added) {
	Ral_Relation rel = relvar->relObj->internalRep.otherValuePtr ;
	Ral_PtrVectorPushBack(relvar->transStack, Ral_RelationDup(rel)) ;
    }
}

int
Ral_RelvarEndCommand(
    Ral_RelvarInfo info,
    Ral_Relvar relvar,
    int failed,
    Tcl_DString *errMsg)
{
    Ral_RelvarTransaction currTrans ;
    assert(Ral_PtrVectorSize(info->transactions) > 0) ;

    currTrans = Ral_PtrVectorBack(info->transactions) ;
    if (currTrans->isSingleCmd) {
	assert(Ral_PtrVectorSize(info->transactions) == 1) ;
	assert(Ral_PtrVectorSize(relvar->transStack) == 1) ;
	/*
	 * Evaluate constraints on this relvar.
	 * If passed, discard the pushed relation value.
	 * If failed, discard the current value and restore.
	 */
	if (!failed) {
	    Ral_PtrVectorIter cEnd = Ral_PtrVectorEnd(relvar->constraints) ;
	    Ral_PtrVectorIter cIter ;
	    for (cIter = Ral_PtrVectorBegin(relvar->constraints) ;
		cIter != cEnd ; ++cIter) {
		failed += !Ral_RelvarConstraintEval(*cIter, errMsg) ;
	    }
	}
	if (failed) {
	    Ral_RelvarRestorePrev(relvar) ;
	} else {
	    Ral_RelvarDiscardPrev(relvar) ;
	}
	Ral_PtrVectorPopBack(info->transactions) ;
	Ral_RelvarDeleteTransaction(currTrans) ;
    }

    return !failed ;
}

Ral_RelvarTransaction
Ral_RelvarNewTransaction(void)
{
    Ral_RelvarTransaction trans ;

    trans = (Ral_RelvarTransaction)ckalloc(sizeof(*trans)) ;
    trans->modified = Ral_PtrVectorNew(1) ; /* 1, so that we don't reallocate
					     * immediately
					     */
    trans->pendingDelete = Ral_PtrVectorNew(0) ;
    return trans ;
}

void
Ral_RelvarDeleteTransaction(
    Ral_RelvarTransaction trans)
{
    Ral_PtrVectorDelete(trans->modified) ;
    Ral_PtrVectorDelete(trans->pendingDelete) ;
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
	    if (assoc->referenceMap) {
		Ral_JoinMapDelete(assoc->referenceMap) ;
	    }
	    ckfree((char *)assoc) ;
	}
	break ;

    case ConstraintPartition:
	ckfree((char *)constraint->partition) ;
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
 * Utilities
 */
static void
relvarCleanup(
    Ral_Relvar relvar)
{
    if (relvar->relObj) {
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
}

static int
relvarAssocConstraintEval(
    const char *name,
    Ral_AssociationConstraint association,
    Tcl_DString *errMsg)
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
    multViolations = Ral_IntVectorNewEmpty(0) ;
    condViolations = Ral_IntVectorNewEmpty(0) ;
    cnts = Ral_JoinMapTupleCounts(association->referenceMap, 1,
	Ral_RelationCardinality(referredToRel)) ;
    ref_result = relvarEvalTupleCounts(cnts, association->referringMult,
	association->referringCond, multViolations, condViolations) ;
    Ral_IntVectorDelete(cnts) ;
    /*
     * Leave error messages if requested.
     */
    if (!ref_result && errMsg) {
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
    multViolations = Ral_IntVectorNewEmpty(0) ;
    condViolations = Ral_IntVectorNewEmpty(0) ;
    cnts = Ral_JoinMapTupleCounts(association->referenceMap, 0,
	Ral_RelationCardinality(referringRel)) ;
    refTo_result = relvarEvalTupleCounts(cnts, association->referredToMult,
	association->referredToCond, multViolations, condViolations) ;
    Ral_IntVectorDelete(cnts) ;
    if (!refTo_result && errMsg) {
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

    return ref_result && refTo_result ;
}

static int
relvarPartitionConstraintEval(
    const char *name,
    Ral_PartitionConstraint partition,
    Tcl_DString *errMsg)
{
    return 1 ;
}

static int
relvarEvalTupleCounts(
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
    Tcl_DString *msg,
    const char *name,
    Ral_AssociationConstraint assoc,
    Ral_Relvar relvar,
    Ral_IntVector violations,
    const char *detail)
{
    Ral_Relation rel ;
    Ral_IntVectorIter vEnd = Ral_IntVectorEnd(violations) ;
    Ral_IntVectorIter vIter ;

    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    rel = relvar->relObj->internalRep.otherValuePtr ;

    if (Ral_IntVectorSize(violations) == 0) {
	return ;
    }

    Tcl_DStringAppend(msg, "for association ", -1) ;
    relvarAssocConstraintToString(name, assoc, msg) ;
    Tcl_DStringAppend(msg, ", in relvar ", -1) ;
    Tcl_DStringAppend(msg, relvar->name, -1) ;
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
    /*
     * Get rid of the trailing newline.
     */
    Tcl_DStringSetLength(msg, Tcl_DStringLength(msg) - 1) ;
}

static void
relvarAssocConstraintToString(
    const char *name,
    Ral_AssociationConstraint assoc,
    Tcl_DString *result)
{
    static char const * const condMultStrings[2][2] = {
	{"1", "1..N"},
	{"0..1", "0..N"}
    } ;

    Ral_Relvar referringRelvar = assoc->referringRelvar ;
    Ral_Relvar referredToRelvar = assoc->referredToRelvar ;

    Tcl_DStringAppend(result, name, -1) ;
    Tcl_DStringAppend(result, "(", -1) ;
    Tcl_DStringAppend(result, referringRelvar->name, -1) ;
    Tcl_DStringAppend(result, " [", -1) ;
    Tcl_DStringAppend(result,
	condMultStrings[assoc->referringCond][assoc->referringMult], -1) ;
    Tcl_DStringAppend(result, "] <==> [", -1) ;
    Tcl_DStringAppend(result,
	condMultStrings[assoc->referredToCond][assoc->referredToMult], -1) ;
    Tcl_DStringAppend(result, "] ", -1) ;
    Tcl_DStringAppend(result, referredToRelvar->name, -1) ;
    Tcl_DStringAppend(result, ")", -1) ;
}
