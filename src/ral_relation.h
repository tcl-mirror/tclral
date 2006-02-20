/*
This software is copyrighted 2005 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_relation.h,v $
$Revision: 1.2 $
$Date: 2006/02/20 20:15:07 $
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
#include "ral_relationheading.h"
#include "ral_tuple.h"
#include "ral_attribute.h"

#include <stdio.h>

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
 * formatted list. The list consists of four elements.
 *
 * 1. The keyword "Relation".
 *
 * 2. A tuple heading. A tuple heading is a list of Name / Type pairs as
 *    described for the Tuple type.
 *
 * 3. A list of identifiers. Each identifier is in turn a list of attribute
 *    names that constitute the identifier
 *
 * 4. A list of tuple values. Each element of the list is a set of
 *    Attribute Name / Attribute Value pairs.
 * E.G.
 *    {Relation {{{Name string} {Number int} {Wage double}} {{Name}}}\
 *    {{Name Andrew Number 200 Wage 5.75}\
 *     {Name George Number 174 Wage 10.25}}}
 * is a Relation consisting of a body which has two tuples.
 *
 * All tuples in a relation must have the same heading and all tuples in a
 * relation must be unique.  We build hash tables for the identifiers that can
 * be used as indices into the tuple storage of a Relation.
 */

typedef Ral_Tuple *Ral_RelationIter ;
typedef struct Ral_Relation {
    Ral_RelationHeading heading ;   /* The relation heading */
    Ral_RelationIter start ;	    /* Tuple values are stored in a dynamic
				     * vector of tuple pointers. We use the
				     * usual start, finish, end pointers to
				     * manipulate the vector.
				     * Cf. Ral_IntVector and Ral_TupleHeading */
    Ral_RelationIter finish ;
    Ral_RelationIter endStorage ;
    Tcl_HashTable *indexVector ;    /* A vector of hash tables that map the
				     * the values of identifying attributes
				     * to the index in the tuple vector where
				     * the corresponding tuple is found.
				     * There is one hash table for each
				     * identifier, i.e. "indexVector" points
				     * to an array of "heading->idCount" hash
				     * tables. */
} *Ral_Relation ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_Relation Ral_RelationNew(Ral_RelationHeading) ;
extern Ral_Relation Ral_RelationDup(Ral_Relation) ;
extern void Ral_RelationDelete(Ral_Relation) ;
extern Ral_RelationIter Ral_RelationBegin(Ral_Relation) ;
extern Ral_RelationIter Ral_RelationEnd(Ral_Relation) ;
extern int Ral_RelationCardinality(Ral_Relation) ;
extern int Ral_RelationCapacity(Ral_Relation) ;
extern void Ral_RelationReserve(Ral_Relation, int) ;
extern int Ral_RelationDegree(Ral_Relation) ;
extern int Ral_RelationPushBack(Ral_Relation, Ral_Tuple) ;

extern int Ral_RelationScan(Ral_Relation, Ral_RelationScanFlags *) ;
extern int Ral_RelationConvert(Ral_Relation, char *, Ral_RelationScanFlags) ;
extern int Ral_RelationScanValue(Ral_Relation, Ral_RelationScanFlags) ;
extern int Ral_RelationConvertValue(Ral_Relation, char *,
    Ral_RelationScanFlags) ;
extern void Ral_RelationPrint(Ral_Relation, const char *, FILE *) ;

extern const char * Ral_RelationVersion(void) ;

#endif /* _ral_relation_h_ */
