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

$RCSfile: ral_relationheading.c,v $
$Revision: 1.2 $
$Date: 2006/02/06 05:02:45 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relationheading.c,v $
Revision 1.2  2006/02/06 05:02:45  mangoa01
Started on relation heading and other code refactoring.
This is a checkpoint after a number of added files and changes
to tuple heading code.

Revision 1.1  2005/12/27 23:17:19  mangoa01
Update to the new spilt out file structure.

 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_relationheading.h"
#include <assert.h>
#include <string.h>

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

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationheading.c,v $ $Revision: 1.2 $" ;

/*
FUNCTION DEFINITIONS
*/

Ral_RelationHeading
Ral_RelationHeadingNew(
    Ral_TupleHeading tupleHeading,
    int idCount)
{
    Ral_RelationHeading heading ;
    int nBytes ;

    assert(idCount > 0) ;

    nBytes = sizeof(*heading) + idCount * sizeof(*heading->identifiers) ;
    heading = (Ral_RelationHeading)ckalloc(nBytes) ;
    memset(heading, 0, nBytes) ;

    heading->refCount = 0 ;
    Ral_TupleHeadingReference(heading->tupleHeading = tupleHeading) ;
    heading->idCount = idCount ;
    heading->identifiers = (Ral_IntVector *)(heading + 1) ;

    /*
     * Note at this point the vector pointers in the "identifiers" member
     * are all NULL.
     */

    return heading ;
}

void
Ral_RelationHeadingDelete(
    Ral_RelationHeading heading)
{
    Ral_IntVector *idArray = heading->identifiers ;
    int idCount = heading->idCount ;

    assert(heading->refCount <= 0) ;

    while (idCount-- > 0) {
	Ral_IntVector id = *idArray++ ;
	if (id) {
	    Ral_IntVectorDelete(id) ;
	}
    }
    Ral_TupleHeadingUnreference(heading->tupleHeading) ;
    ckfree((char *)heading) ;
}


void
Ral_RelationHeadingReference(
    Ral_RelationHeading heading)
{
    ++heading->refCount ;
}

void
Ral_RelationHeadingUnreference(
    Ral_RelationHeading heading)
{
    if (heading && --heading->refCount <= 0) {
	Ral_RelationHeadingDelete(heading) ;
    }
}

int
Ral_RelationHeadingEqual(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2)
{
    int id1Count ;
    Ral_IntVector *id1Array ;
    Ral_TupleHeadingIter h1Tuples ;

    /*
     * The easy case is when we are pointing to the same heading.
     */
    if (h1 == h2) {
	return 1 ;
    }
    /*
     * The Tuple Headings must be the same.
     */
    if (!Ral_TupleHeadingEqual(h1->tupleHeading, h2->tupleHeading)) {
	return 0 ;
    }
    /*
     * The identifiers must be the same.
     *
     * We start by insuring that we have the same number of identifiers.
     */
    if (h1->idCount != h2->idCount) {
	return 0 ;
    }
    /*
     * The strategy to compare the identifiers is to use the names of the
     * identifying attributes from the first heading to construct an identifier
     * index set using the ordering of the tuple heading in the second heading.
     * Remember, attribute ordering does not matter.  Given the vector of
     * attribute indices relative to the ordering of the second heading, we
     * then have to search the heading to find the identfier.
     *
     * Start with a loop iterating over the identifiers of the first heading.
     */
    h1Tuples = Ral_TupleHeadingBegin(h1->tupleHeading) ;
    id1Array = h1->identifiers ;
    for (id1Count = h1->idCount ; id1Count > 0 ; --id1Count) {
	Ral_IntVector id1 ;
	Ral_IntVector id2 ;
	Ral_IntVectorIter end1Iter ;
	Ral_IntVectorIter id1Iter ;
	Ral_IntVectorIter id2Iter ;
	int id2Count ;
	Ral_IntVector *id2Array ;
	int found ;

	id1 = *id1Array++ ;
	end1Iter = Ral_IntVectorEnd(id1) ;
	/*
	 * Construct a new vector of attribute indices using the
	 * names from the first heading to find its corresponding
	 * index in the second heading.
	 */
	id2 = Ral_IntVectorNew(Ral_IntVectorSize(id1), -1) ;
	id2Iter = Ral_IntVectorBegin(id2) ;
	/*
	 * Iterate through all the attribute indices of the identifier and
	 * construct the vector to have a set of attribute indices that
	 * are relative to the ordering of attributes in the second heading.
	 */
	for (id1Iter = Ral_IntVectorBegin(id1) ; id1Iter != end1Iter ;
	    ++id1Iter) {
	    int attr1Index = *id1Iter ;
	    /*
	     * Get the attribute.
	     */
	    Ral_Attribute attr1 = *(h1Tuples + attr1Index) ;
	    /*
	     * Find the index of that attribute name in the second heading.
	     */
	    int attr2Index = Ral_TupleHeadingIndexOf(h2->tupleHeading,
		attr1->name) ;
	    /*
	     * The attribute must exist in the second heading since we
	     * have already determined that the two tuple headings are equal.
	     */
	    assert (attr2Index >= 0) ;
	    *id2Iter ++ = attr2Index ;
	}
	/*
	 * Sort the identifier indices into canonical order.
	 */
	Ral_IntVectorSort(id2) ;
	/*
	 * Now we search the identifiers of the second heading to find
	 * a match to the vector we just constructed.
	 */
	id2Array = h2->identifiers ;
	found = 0 ;
	for (id2Count = h2->idCount ; id2Count > 0 ; --id2Count) {
	    Ral_IntVector heading2Id = *id2Array++ ;

	    /*
	     * All of the identifiers should have been added.
	     */
	    assert(heading2Id != NULL) ;
	    if (Ral_IntVectorEqual(id2, heading2Id)) {
		found = 1 ;
		break ;
	    }
	}
	/*
	 * Discard the vector.
	 */
	Ral_IntVectorDelete(id2) ;
	if (!found) {
	    return 0 ;
	}
    }

    return 1 ;
}

int
Ral_RelationHeadingAddIdentifier(
    Ral_RelationHeading heading,
    int idNum,
    Ral_IntVector id)
{
    int idCount ;
    Ral_IntVector *idArray ;
    int found ;

    assert (idNum < heading->idCount) ;
    assert (heading->identifiers[idNum] == NULL) ;
    /*
     * Make sure the identifier is in canonical order.
     */
    Ral_IntVectorSort(id) ;
    /*
     * Only add the identifier if it is not a subset of an existing
     * identifier. So we iterate through the identifiers of the heading
     * and make sure that the new one is not equal to or a subset of
     * an existing identifier.
     */
    idArray = heading->identifiers ;
    found = 0 ;
    for (idCount = heading->idCount ; idCount > 0 ; --idCount) {
	Ral_IntVector headingId = *idArray++ ;

	if (headingId && Ral_IntVectorSubsetOf(id, headingId)) {
	    return 0 ;
	}
    }

    /*
     * Take possession of the identifier vector.
     */
    heading->identifiers[idNum] = id ;
    return 1 ;
}

/*
 * Create a new relation heading that is the union of two other headings.
 */
Ral_RelationHeading
Ral_RelationHeadingUnion(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2)
{
    unsigned h1Size = Ral_TupleHeadingSize(h1->tupleHeading) ;
    Ral_TupleHeading unionTupleHeading ;
    Ral_RelationHeading unionRelationHeading ;
    int c1 ;
    int idNum = 0 ;

    /*
     * Union the tuple heading.
     */
    unionTupleHeading = Ral_TupleHeadingUnion(h1->tupleHeading,
	h2->tupleHeading) ;
    if (unionTupleHeading == NULL) {
	return NULL ;
    }

    /*
     * The identifiers for the union are the cross product of the identifiers
     * of the two component headings.
     */
    unionRelationHeading = Ral_RelationHeadingNew(unionTupleHeading,
	h1Size * Ral_TupleHeadingSize(h2->tupleHeading)) ;

    /*
     * Loop through the identifiers for the two heading components
     * and compose new identifiers for the union.
     */
    for (c1 = 0 ; c1 < h1->idCount ; ++c1) {
	Ral_IntVector h1Id = h1->identifiers[c1] ;
	int c2 ;

	for (c2 = 0 ; c2 < h2->idCount ; ++c2) {
	    Ral_IntVector h2Id = h2->identifiers[c2] ;
	    Ral_IntVectorIter h2IdIter ;
	    Ral_IntVectorIter h2IdEnd = Ral_IntVectorEnd(h2Id) ;
	    Ral_IntVector unionId = Ral_IntVectorNewEmpty(
		Ral_IntVectorSize(h1Id) + Ral_IntVectorSize(h2Id)) ;
	    int added ;

	    /*
	     * Copy the identifier indices from the first header.
	     * These indices are correct for the union header also.
	     */
	    Ral_IntVectorCopy(h1Id, Ral_IntVectorBegin(h1Id),
		Ral_IntVectorEnd(h1Id), unionId, Ral_IntVectorBegin(unionId)) ;

	    /*
	     * The identifier indices from the second header must
	     * be offset by the number of attributes in the first header.
	     * Union tuple headers are constructed as simple concatenations.
	     */
	    for (h2IdIter = Ral_IntVectorBegin(h2Id) ; h2IdIter != h2IdEnd ;
		++h2IdIter) {
		Ral_IntVectorPushBack(unionId, *h2IdIter + h1Size) ;
	    }

	    /*
	     * Add the newly formed identifier.
	     * It should always add.
	     */
	    added = Ral_RelationHeadingAddIdentifier(unionRelationHeading,
		idNum++, unionId) ;
	    assert(added == 1) ;
	}
    }

    return unionRelationHeading ;
}

const char *
Ral_RelationHeadingVersion(void)
{
    return rcsid ;
}
