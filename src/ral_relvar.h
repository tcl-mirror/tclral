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
$Revision: 1.3 $
$Date: 2006/05/07 03:53:28 $
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
} Ral_ConstraintType ;

typedef struct Ral_AssociationConstraint {
    struct Ral_Relvar *referringRelvar ;
    int referringCond ;
    int referringMult ;
    struct Ral_Relvar *referredToRelvar ;
    int referredToCond ;
    int referredToMult ;
    Ral_JoinMap referenceMap ;
} *Ral_AssociationConstraint ;

typedef struct Ral_SubsetReference {
    struct Ral_Relvar *relvar ;
    Ral_JoinMap subsetReference ;
} *Ral_SubsetReference ;

typedef struct Ral_PartitionConstraint {
    struct Ral_Relvar *referredToRelvar ;
    Ral_PtrVector subsetMap ;	/* list of subtype references */
} *Ral_PartitionConstraint ;

typedef struct Ral_Constraint {
    Ral_ConstraintType type ;
    char *name ;
    union {
	Ral_AssociationConstraint association ;
	Ral_PartitionConstraint partition ;
    } ;
} *Ral_Constraint ;

typedef struct Ral_Relvar {
    char *name ;		/* fully resolved name */
    Tcl_Obj *relObj ;		/* relation valued object */
    Ral_PtrVector transStack ;	/* a stack of Ral_Relation */
    Ral_PtrVector constraints ;	/* a list of Ral_Constraint  */
} *Ral_Relvar ;

typedef struct Ral_RelvarTransaction {
    int isSingleCmd ;		/* transaction for a single command only */
    Ral_PtrVector modified ;	/* a set of Ral_Relvar */
    Ral_PtrVector pendingDelete ;   /* a set of Ral_Relvar */
} *Ral_RelvarTransaction ;

typedef struct Ral_RelvarInfo {
    Ral_PtrVector transactions ;    /* a stack of Ral_RelvarTransactions */
    Tcl_HashTable relvars ;	    /* mapping name ==> Ral_Relvar */
    Tcl_HashTable constraints ;	    /* mapping name ==> Ral_Constraint */
} *Ral_RelvarInfo ;

/*
 * Order here is important!
 * These enumerators are used as an array index in Ral_RelationObjSetError()
 */
typedef enum Ral_RelvarError {
    RELVAR_OK = 0,
    RELVAR_DUP_NAME,
    RELVAR_UNKNOWN_NAME,
    RELVAR_HEADING_MISMATCH,
    RELVAR_REFATTR_MISMATCH,
    RELVAR_DUP_CONSTRAINT,
    RELVAR_NOT_ID,
    RELVAR_NOT_EMPTY,
} Ral_RelvarError ;

/*
DATA DECLARATIONS
*/
extern Ral_RelvarError Ral_RelvarLastError ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_Relvar Ral_RelvarNew(Ral_RelvarInfo, const char *,
    Ral_RelationHeading) ;
extern void Ral_RelvarDelete(Ral_RelvarInfo, const char *) ;
extern Ral_Relvar Ral_RelvarFind(Ral_RelvarInfo, const char *) ;

extern ClientData Ral_RelvarNewInfo(const char *, Tcl_Interp *) ;
extern void Ral_RelvarDeleteInfo(ClientData, Tcl_Interp *) ;
extern Ral_Relvar Ral_RelvarLookupRelvar(Tcl_Interp *, Ral_RelvarInfo,
    Tcl_Obj *) ;

extern void Ral_RelvarStartTransaction(Ral_RelvarInfo, int) ;
extern int Ral_RelvarEndTransaction(Ral_RelvarInfo, int, Tcl_DString *) ;
extern void Ral_RelvarStartCommand(Ral_RelvarInfo, Ral_Relvar) ;
extern int Ral_RelvarEndCommand(Ral_RelvarInfo, Ral_Relvar, int,
    Tcl_DString *) ;

extern Ral_RelvarTransaction Ral_RelvarNewTransaction(void) ;
extern void Ral_RelvarDeleteTransaction(Ral_RelvarTransaction) ;

extern Ral_Constraint Ral_ConstraintAssocCreate(const char *, Ral_RelvarInfo) ;
extern Ral_Constraint Ral_ConstraintNewAssociation(const char *) ;
extern Ral_Constraint Ral_ConstraintNewPartition(const char *) ;
extern void Ral_ConstraintDelete(Ral_Constraint) ;
extern int Ral_RelvarConstraintEval(Ral_Constraint, Tcl_DString *) ;

extern void Ral_RelvarSetRelation(Ral_Relvar, Ral_Relation) ;
extern void Ral_RelvarRestorePrev(Ral_Relvar) ;
extern void Ral_RelvarDiscardPrev(Ral_Relvar) ;

#endif /* _ral_relvar_h_ */
