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
static const char rcsid[] = "@(#) $RCSfile: ral_relvar.c,v $ $Revision: 1.2 $" ;

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
    Ral_Relation relation ;
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
     * Remove all the tuples.
     */
    assert(relvar->relObj->typePtr == &Ral_RelationObjType) ;
    relation = relvar->relObj->internalRep.otherValuePtr ;
    Ral_RelationErase(relation, Ral_RelationBegin(relation),
	Ral_RelationEnd(relation)) ;
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

void
Ral_RelvarEndTransaction(
    Ral_RelvarInfo info,
    int failed)
{
    Ral_RelvarTransaction trans ;
    Ral_PtrVectorIter modIter ;
    Ral_PtrVectorIter modEnd ;

    trans = Ral_PtrVectorBack(info->transactions) ;
    assert(trans->isSingleCmd == 0) ;
    Ral_PtrVectorPopBack(info->transactions) ;

    modEnd = Ral_PtrVectorEnd(trans->modified) ;
    if (!failed) {
	/*
	 * Iterate across the transaction evaluating the constraints.
	 * HERE
	 * Build a set of constraints that the modified relvars
	 * participate in and then evaluate all the  constraints found.
	 */
	for (modIter = Ral_PtrVectorBegin(trans->modified) ; modIter != modEnd ;
	    ++modIter) {
	    failed += !Ral_RelvarConstraints(*modIter) ;
	}
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

void
Ral_RelvarEndCommand(
    Ral_RelvarInfo info,
    Ral_Relvar relvar,
    int failed)
{
    Ral_RelvarTransaction currTrans ;
    assert(Ral_PtrVectorSize(info->transactions) > 0) ;

    currTrans = Ral_PtrVectorBack(info->transactions) ;
    if (currTrans->isSingleCmd) {
	assert(Ral_PtrVectorSize(info->transactions) == 1) ;
	assert(Ral_PtrVectorSize(relvar->transStack) == 1) ;
	/*
	 * Evaluate constraint on this relvar.
	 * If passed, discard the pushed relation value.
	 * If failed, discard the current value and restore.
	 */
	if (!failed && Ral_RelvarConstraints(relvar)) {
	    Ral_RelvarDiscardPrev(relvar) ;
	} else {
	    Ral_RelvarRestorePrev(relvar) ;
	}
	Ral_PtrVectorPopBack(info->transactions) ;
	Ral_RelvarDeleteTransaction(currTrans) ;
    }
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
Ral_RelvarConstraints(
    Ral_Relvar relvar)
{
    /*
     * For now, everything succeeds.
     */
    return 1 ;
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
    Tcl_DecrRefCount(relvar->relObj) ;

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
