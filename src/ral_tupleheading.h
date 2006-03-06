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

$RCSfile: ral_tupleheading.h,v $
$Revision: 1.7 $
$Date: 2006/03/06 01:07:37 $
 *--
 */
#ifndef _ral_tupleheading_h_
#define _ral_tupleheading_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_attribute.h"
#include "ral_vector.h"
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
 * A Tuple Heading is a set of Heading Attributes. The attributes are stored in
 * an attribute vector.  It is useful to define a vector of attributes. In this
 * case the vector is not dynamic in size, but it is useful to have the ability
 * to append attributes and iterate through the set. Notice also the use of the
 * hash table to provide indexing by name.
 *
 * The external string representation of a "heading" is a list of alternating
 * pairs of attribute name / attribute type, i.e. it looks like a dictionary.
 *
 *	{Name string Street int Wage double}
 *
 * The attribute name can be an arbitrary string. Attribute names must be
 * unique within a given heading. The attribute type must be a valid registered
 * Tcl type such that Tcl_GetObjType() will return a non-NULL result or a Tuple
 * or Relation type.
 *
 * Notice that Tuple Headings are reference counted. Many tuples (e.g. in
 * a relation) will reference the same heading.
 */


typedef Ral_Attribute *Ral_TupleHeadingIter ;
typedef struct Ral_TupleHeading {
    int refCount ;			/* Reference Count */
    Ral_TupleHeadingIter start ;	/* Pointer to beginning of storage */
    Ral_TupleHeadingIter finish ;	/* Pointer to one past valid elements */
    Ral_TupleHeadingIter endStorage ;	/* Pointer to one past end of memory */
    Tcl_HashTable nameMap ;		/* Hash table used for indexing by
					 * name*/
} *Ral_TupleHeading ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_TupleHeading Ral_TupleHeadingNew(int) ;
extern Ral_TupleHeading Ral_TupleHeadingSubset(Ral_TupleHeading,
    Ral_IntVector) ;
extern Ral_TupleHeading Ral_TupleHeadingDup(Ral_TupleHeading) ;
extern void Ral_TupleHeadingDelete(Ral_TupleHeading) ;
extern void Ral_TupleHeadingReference(Ral_TupleHeading) ;
extern void Ral_TupleHeadingUnreference(Ral_TupleHeading) ;
extern int Ral_TupleHeadingAppend(Ral_TupleHeading, Ral_TupleHeadingIter,
    Ral_TupleHeadingIter, Ral_TupleHeading) ;
extern int Ral_TupleHeadingSize(Ral_TupleHeading) ;
extern int Ral_TupleHeadingCapacity(Ral_TupleHeading) ;
extern int Ral_TupleHeadingEmpty(Ral_TupleHeading) ;
extern int Ral_TupleHeadingEqual(Ral_TupleHeading, Ral_TupleHeading) ;
extern Ral_TupleHeadingIter Ral_TupleHeadingBegin(Ral_TupleHeading) ;
extern Ral_TupleHeadingIter Ral_TupleHeadingEnd(Ral_TupleHeading) ;
extern Ral_TupleHeadingIter Ral_TupleHeadingStore(Ral_TupleHeading,
    Ral_TupleHeadingIter, Ral_Attribute) ;
extern Ral_Attribute Ral_TupleHeadingFetch(Ral_TupleHeading, int) ;
extern Ral_TupleHeadingIter Ral_TupleHeadingPushBack(Ral_TupleHeading,
    Ral_Attribute) ;
extern Ral_TupleHeadingIter Ral_TupleHeadingFind(Ral_TupleHeading,
    const char *) ;
extern int Ral_TupleHeadingIndexOf(Ral_TupleHeading, const char *) ;
extern Ral_TupleHeading Ral_TupleHeadingUnion(Ral_TupleHeading,
    Ral_TupleHeading) ;
extern Ral_TupleHeading Ral_TupleHeadingIntersect(Ral_TupleHeading,
    Ral_TupleHeading) ;
extern Ral_IntVector Ral_TupleHeadingNewOrderMap(Ral_TupleHeading,
    Ral_TupleHeading) ;

extern int Ral_TupleHeadingScan(Ral_TupleHeading,
    Ral_AttributeTypeScanFlags *) ;
extern int Ral_TupleHeadingScanAttr(Ral_TupleHeading,
    Ral_AttributeTypeScanFlags *) ;
extern int Ral_TupleHeadingConvert(Ral_TupleHeading, char *,
    Ral_AttributeTypeScanFlags *) ;
extern int Ral_TupleHeadingConvertAttr(Ral_TupleHeading, char *,
    Ral_AttributeTypeScanFlags *) ;
extern void Ral_TupleHeadingPrint(Ral_TupleHeading, const char *, FILE *) ;
extern char *Ral_TupleHeadingStringOf(Ral_TupleHeading) ;
extern const char *Ral_TupleHeadingVersion(void) ;

#endif /* _ral_tupleheading_h_ */
