/*
This software is copyrighted 2005 - 2011 by G. Andrew Mangogna.
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

$RCSfile: ral_relation.h,v $
$Revision: 1.23 $
$Date: 2011/06/05 18:01:10 $
 *--
 */
#ifndef _ral_relation_h_
#define _ral_relation_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_utils.h"
#include "ral_tupleheading.h"
#include "ral_tuple.h"
#include "ral_attribute.h"
#include "ral_vector.h"
#include "ral_joinmap.h"

/*
MACRO DEFINITIONS
*/

/*
FORWARD CLASS REFERENCES
*/

/*
TYPE DECLARATIONS
*/

/*
 * A Relation type consists of a heading with a body.  A body consists
 * of zero or more tuples.

 * The external string representation of a "Relation" is a specially
 * formatted list. The list consists of two elements.
 *
 * 1. A tuple heading. A tuple heading is a list of Name / Type pairs as
 *    described for the Tuple type.
 *
 * 2. A list of tuple values. Each element of the list is a set of
 *    Attribute Name / Attribute Value pairs.
 * E.G.
 *    {{Name string Number int Wage double} {
 *     {Name Andrew Number 200 Wage 5.75}
 *     {Name George Number 174 Wage 10.25}}
 *    }
 * is a Relation consisting of a body which has two tuples.
 *
 * All tuples in a relation must have the same heading and all tuples in a
 * relation must be unique.  We build hash tables for the identifiers that can
 * be used as indices into the tuple storage of a Relation.
 */

typedef Ral_Tuple *Ral_RelationIter ;
typedef struct Ral_Relation {
    Ral_TupleHeading heading ;      /* The heading  of a relation is the
                                     * same as for a tuple. */
    Ral_RelationIter start ;        /* Tuple values are stored in a dynamic
                                     * vector of tuple pointers. We use the
                                     * usual start, finish, end pointers to
                                     * manipulate the vector.
                                     * Cf. Ral_IntVector and Ral_TupleHeading */
    Ral_RelationIter finish ;
    Ral_RelationIter endStorage ;
    Tcl_HashTable index ;           /* A hash table that maps the values of
                                     * the attributes of the tuples to the
                                     * index in the tuple vector where the
                                     * corresponding tuple is found. */
} *Ral_Relation ;

/*
 * MEMBER ACCESS MACROS
 * N.B. many of these macros access their arguments multiple times.
 * No side effects for the arguments!
 */
#define Ral_RelationBegin(r)    ((r)->start)
#define Ral_RelationEnd(r)      ((r)->finish)
#define Ral_RelationDegree(r)   (Ral_TupleHeadingSize((r)->heading))
#define Ral_RelationCardinality(r)  ((r)->finish - (r)->start)
#define Ral_RelationCapacity(r)  ((r)->endStorage - (r)->start)

/*
 * Data structure used in a custom hash table for finding tuples
 * in a relation based on the values of the tuple attributes.
 */
typedef struct Ral_TupleAttrHashKey {
    Ral_Tuple tuple ;       /* reference to tuple that is the hash key */
    Ral_IntVector attrs ;   /* vector holds the attribute indices that are
                             * to be used to form the key and for comparison
                             * purposes.
                             */
} *Ral_TupleAttrHashKey ;

/*
DATA DECLARATIONS
*/
extern Tcl_HashKeyType tupleAttrHashType ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_Relation Ral_RelationNew(Ral_TupleHeading) ;
extern Ral_Relation Ral_RelationDup(Ral_Relation) ;
extern Ral_Relation Ral_RelationShallowCopy(Ral_Relation) ;
extern void Ral_RelationDelete(Ral_Relation) ;

extern void Ral_RelationReserve(Ral_Relation, int) ;

extern int Ral_RelationPushBack(Ral_Relation, Ral_Tuple, Ral_IntVector) ;
extern int Ral_RelationUpdate(Ral_Relation, Ral_RelationIter, Ral_Tuple,
    Ral_IntVector) ;

extern Ral_RelationIter Ral_RelationFind(Ral_Relation, Ral_Tuple,
    Ral_IntVector) ;
extern Ral_Relation Ral_RelationExtract(Ral_Relation, Ral_IntVector) ;
extern Ral_RelationIter Ral_RelationErase(Ral_Relation, Ral_RelationIter,
    Ral_RelationIter) ;

extern int Ral_RelationCompare(Ral_Relation, Ral_Relation) ;
extern int Ral_RelationEqual(Ral_Relation, Ral_Relation) ;
extern int Ral_RelationNotEqual(Ral_Relation, Ral_Relation) ;
extern int Ral_RelationSubsetOf(Ral_Relation, Ral_Relation) ;
extern int Ral_RelationProperSubsetOf(Ral_Relation, Ral_Relation) ;
extern int Ral_RelationSupersetOf(Ral_Relation, Ral_Relation) ;
extern int Ral_RelationProperSupersetOf(Ral_Relation, Ral_Relation) ;

extern int Ral_RelationRenameAttribute(Ral_Relation, char const *, char const *,
        Ral_ErrorInfo *) ;

extern Ral_Relation Ral_RelationUnion(Ral_Relation, Ral_Relation, int,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationIntersect(Ral_Relation, Ral_Relation,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationMinus(Ral_Relation, Ral_Relation,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationTimes(Ral_Relation, Ral_Relation) ;
extern Ral_Relation Ral_RelationProject(Ral_Relation, Ral_IntVector) ;
extern Ral_Relation Ral_RelationGroup(Ral_Relation, char const *,
        Ral_IntVector) ;
extern Ral_Relation Ral_RelationUngroup(Ral_Relation, char const *,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationJoin(Ral_Relation, Ral_Relation, Ral_JoinMap,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationCompose(Ral_Relation, Ral_Relation, Ral_JoinMap,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationSemiJoin(Ral_Relation, Ral_Relation,
        Ral_JoinMap) ;
extern Ral_Relation Ral_RelationSemiMinus(Ral_Relation, Ral_Relation,
        Ral_JoinMap) ;
extern Ral_Relation Ral_RelationDivide(Ral_Relation, Ral_Relation, Ral_Relation,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationTag(Ral_Relation, char const *, Ral_IntVector,
        int, Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationTagWithin(Ral_Relation, char const *,
        Ral_IntVector, Ral_IntVector, int, Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationUnwrap(Ral_Relation, char const *,
        Ral_ErrorInfo *) ;
extern Ral_Relation Ral_RelationTclose(Ral_Relation) ;

extern Ral_IntVector Ral_RelationSort(Ral_Relation, Ral_IntVector, int) ;
extern Ral_RelationIter Ral_RelationDisjointCopy(Ral_RelationIter,
        Ral_RelationIter, Ral_Relation, Ral_IntVector) ;
extern void Ral_RelationUnionCopy(Ral_RelationIter,
        Ral_RelationIter, Ral_Relation, Ral_IntVector) ;
extern void Ral_RelationFindJoinTuples(Ral_Relation, Ral_Relation,
        Ral_JoinMap) ;

extern int Ral_RelationScan(Ral_Relation, Ral_AttributeTypeScanFlags *,
        Ral_AttributeValueScanFlags *) ;
extern int Ral_RelationConvert(Ral_Relation, char *,
        Ral_AttributeTypeScanFlags *, Ral_AttributeValueScanFlags *) ;
extern int Ral_RelationScanValue(Ral_Relation,
        Ral_AttributeTypeScanFlags *, Ral_AttributeValueScanFlags *) ;
extern int Ral_RelationConvertValue(Ral_Relation, char *,
        Ral_AttributeTypeScanFlags *, Ral_AttributeValueScanFlags *) ;
extern char *Ral_RelationStringOf(Ral_Relation) ;
extern char *Ral_RelationValueStringOf(Ral_Relation) ;

char const *Ral_RelationGetIdKey(Ral_Tuple, Ral_IntVector,
        Ral_IntVector, Tcl_DString *) ;

#endif /* _ral_relation_h_ */

/* vim:set ts=4 sw=4 sts=4 expandtab: */
