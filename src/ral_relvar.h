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

$RCSfile: ral_relvar.h,v $
$Revision: 1.13 $
$Date: 2009/04/11 18:18:54 $
 *--
 */
#ifndef _ral_relvar_h_
#define _ral_relvar_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_vector.h"
#include "ral_relation.h"
#include "ral_joinmap.h"
#include "ral_utils.h"
#include "ral_vector.h"

/*
MACRO DEFINITIONS
*/

/*
FORWARD CLASS REFERENCES
*/

/*
TYPE DECLARATIONS
*/
typedef enum {
    ConstraintAssociation,
    ConstraintPartition,
    ConstraintCorrelation,
} Ral_ConstraintType ;

typedef struct Ral_AssociationConstraint {
    struct Ral_Relvar *referringRelvar ;
    int referringCond ;
    int referringMult ;
    struct Ral_Relvar *referredToRelvar ;
    int referredToCond ;
    int referredToMult ;
    int referredToIdentifier ;
    Ral_JoinMap referenceMap ;
} *Ral_AssociationConstraint ;

typedef struct Ral_SubsetReference {
    struct Ral_Relvar *relvar ;	    /* subtype relvar */
    Ral_JoinMap subsetMap ;	    /* join map from subtype to supertype */
} *Ral_SubsetReference ;

typedef struct Ral_PartitionConstraint {
    struct Ral_Relvar *referredToRelvar ;
    int referredToIdentifier ;
    Ral_PtrVector subsetReferences ;	/* list of Ral_SubsetReference */
} *Ral_PartitionConstraint ;

typedef struct Ral_CorrelationConstraint {
    struct Ral_Relvar *referringRelvar ;
    struct Ral_Relvar *aRefToRelvar ;
    int aCond ;
    int aMult ;
    int aIdentifier ;
    Ral_JoinMap aReferenceMap ;
    struct Ral_Relvar *bRefToRelvar ;
    int bCond ;
    int bMult ;
    int bIdentifier ;
    Ral_JoinMap bReferenceMap ;
    int complete ;
} *Ral_CorrelationConstraint ;

typedef struct Ral_Constraint {
    Ral_ConstraintType type ;
    char *name ;
    union {
	Ral_AssociationConstraint association ;
	Ral_PartitionConstraint partition ;
	Ral_CorrelationConstraint correlation ;
    } constraint ;
} *Ral_Constraint ;

/*
 * Currently there is not "C" level access to relvar tracing functionality.
 * These data structures are just the minimum to allow script level access.
 */
typedef struct Ral_TraceInfo {
    struct Ral_TraceInfo *next ;    /* use a linked list for trace info */
    int flags ;			    /* to which operations the trace applies */
    Tcl_Obj *command ;		    /* command prefix for the trace */
} *Ral_TraceInfo ;

typedef struct Ral_Relvar {
    char *name ;		/* fully resolved name */
    Tcl_Obj *relObj ;		/* relation valued object */
    Ral_PtrVector transStack ;	/* a stack of Ral_Relation */
    Ral_PtrVector constraints ;	/* a list of Ral_Constraint  */
    Ral_TraceInfo traces ;	/* linked list of Ral_TraceInfo */
    int traceFlags ;		/* state of tracing */
    unsigned idCount ;          /* Number of identifiers for the relvar.
                                 * This is the actual size of the array of
                                 * hash tables in the next member. */
    struct relvarId {
        Ral_IntVector idAttrs ; /* A set of attribute indices that form an
                                 * and identifier. */
        Tcl_HashTable idIndex ; /* Hash tables used to enforce identifier
                                 * constraints. The tuples are hashed based
                                 * in the identifying attributes. */
    } identifiers[1] ;          /* Every relvar must have at least one
                                 * identifier. This is allocated at the end of
                                 * the relvar structure so that it can be
                                 * of variable length and the memory for a
                                 * relvar can be allocated in one block. */
} *Ral_Relvar ;

typedef struct Ral_RelvarTransaction {
    int isSingleCmd ;		/* transaction for a single command only */
    Ral_PtrVector modifiedRelvars ;	/* a set of Ral_Relvar */
} *Ral_RelvarTransaction ;

typedef struct Ral_RelvarInfo {
    Ral_PtrVector transactions ;    /* a stack of Ral_RelvarTransactions */
    Tcl_HashTable relvars ;	    /* mapping name ==> Ral_Relvar */
    Tcl_HashTable constraints ;	    /* mapping name ==> Ral_Constraint */
    Ral_TraceInfo traces ;	    /* list of eval traces */
} *Ral_RelvarInfo ;

/*
DATA DECLARATIONS
*/

/*
FUNCTION DECLARATIONS
*/

extern Ral_Relvar Ral_RelvarNew(Ral_RelvarInfo, char const *,
    Ral_TupleHeading, int) ;
extern void Ral_RelvarDelete(Ral_RelvarInfo, Ral_Relvar) ;
extern Ral_Relvar Ral_RelvarFind(Ral_RelvarInfo, char const *) ;

extern ClientData Ral_RelvarNewInfo(char const *, Tcl_Interp *) ;
extern void Ral_RelvarDeleteInfo(ClientData, Tcl_Interp *) ;
extern Ral_Relvar Ral_RelvarLookupRelvar(Tcl_Interp *, Ral_RelvarInfo,
    Tcl_Obj *) ;

extern void Ral_RelvarStartTransaction(Ral_RelvarInfo, int) ;
extern int Ral_RelvarEndTransaction(Ral_RelvarInfo, int, Tcl_DString *) ;
extern int Ral_RelvarIsTransOnGoing(Ral_RelvarInfo) ;
extern int Ral_RelvarTransModifiedRelvar(Ral_RelvarInfo, Ral_Relvar) ;

extern int Ral_RelvarStartCommand(Ral_RelvarInfo, Ral_Relvar) ;
extern int Ral_RelvarEndCommand(Ral_RelvarInfo, int, Tcl_DString *) ;

extern Ral_RelvarTransaction Ral_RelvarNewTransaction(void) ;
extern void Ral_RelvarDeleteTransaction(Ral_RelvarTransaction) ;

extern Ral_Constraint Ral_ConstraintAssocCreate(char const *, Ral_RelvarInfo) ;
extern Ral_Constraint Ral_ConstraintPartitionCreate(char const *,
    Ral_RelvarInfo) ;
extern Ral_Constraint Ral_ConstraintCorrelationCreate(char const *,
    Ral_RelvarInfo) ;

extern int Ral_ConstraintDeleteByName(char const *, Ral_RelvarInfo) ;
extern Ral_Constraint Ral_ConstraintFindByName(char const *, Ral_RelvarInfo) ;
extern Ral_Constraint Ral_ConstraintNewAssociation(char const *) ;
extern Ral_Constraint Ral_ConstraintNewPartition(char const *) ;
extern Ral_Constraint Ral_ConstraintNewCorrelation(char const *) ;
extern void Ral_ConstraintDelete(Ral_Constraint) ;
extern int Ral_RelvarConstraintEval(Ral_Constraint, Tcl_DString *) ;

extern int Ral_RelvarSetRelation(Ral_Relvar, Tcl_Obj *, Ral_ErrorInfo *) ;
extern int Ral_RelvarInsertTuple(Ral_Relvar, Ral_Tuple, Ral_IntVector,
        Ral_ErrorInfo *) ;
extern Ral_RelationIter Ral_RelvarDeleteTuple(Ral_Relvar, Ral_RelationIter) ;
extern int Ral_RelvarFindIdentifier(Ral_Relvar, Ral_IntVector) ;
extern Ral_RelationIter Ral_RelvarFindById(Ral_Relvar, int, Ral_Tuple) ;
extern int Ral_RelvarIdIndexTuple(Ral_Relvar, Ral_Tuple, int, Ral_ErrorInfo *) ;
extern void Ral_RelvarIdUnindexTuple(Ral_Relvar relvar, Ral_Tuple tuple) ;
extern void Ral_RelvarRestorePrev(Ral_Relvar) ;
extern void Ral_RelvarDiscardPrev(Ral_Relvar) ;

extern void Ral_RelvarTraceAdd(Ral_Relvar, int, Tcl_Obj *const) ;
extern int Ral_RelvarTraceRemove(Ral_Relvar, int, Tcl_Obj *const) ;


#endif /* _ral_relvar_h_ */
