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
$Revision: 1.3 $
$Date: 2006/02/20 20:15:07 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relationheading.c,v $
Revision 1.3  2006/02/20 20:15:07  mangoa01
Now able to convert strings to relations and vice versa including
tuple and relation valued attributes.

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
static const char rcsid[] = "@(#) $RCSfile: ral_relationheading.c,v $ $Revision: 1.3 $" ;

static const char relationKeyword[] = "Relation" ;
static const char openList[] = "{" ;
static const char closeList[] = "}" ;

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

Ral_RelationHeading
Ral_RelationHeadingDup(
    Ral_RelationHeading srcHeading)
{
    Ral_TupleHeading tupleHeading ;
    Ral_RelationHeading dupHeading ;
    int idCount = srcHeading->idCount ;
    Ral_IntVector *srcIds = srcHeading->identifiers ;
    Ral_IntVector *dstIds ;

    tupleHeading = Ral_TupleHeadingDup(srcHeading->tupleHeading) ;
    if (!tupleHeading) {
	return NULL ;
    }

    dupHeading = Ral_RelationHeadingNew(tupleHeading, idCount) ;
    dstIds = dupHeading->identifiers ;
    while (idCount-- > 0) {
	*dstIds++ = Ral_IntVectorDup(*srcIds++) ;
    }

    return dupHeading ;
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
    int idCount = heading->idCount ;
    Ral_IntVector *idArray = heading->identifiers ;

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
    while (idCount-- > 0) {
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
    int h1Size = Ral_TupleHeadingSize(h1->tupleHeading) ;
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

int
Ral_RelationHeadingScan(
    Ral_RelationHeading h,
    Ral_RelationScanFlags flags)
{
    Ral_TupleHeading tupleHeading = h->tupleHeading ;
    Ral_TupleHeadingIter thIter ;
    Ral_TupleHeadingIter thEnd ;
    Ral_AttributeScanFlags attrFlag = flags->attrFlags ;
    int idCount = h->idCount ;
    Ral_AttributeScanFlags idFlag = flags->idFlags ;
    Ral_IntVector *ids = h->identifiers ;
    int length = 1 ; /* for the NUL terminator */
    /*
     * The keyword part of the heading.
     * N.B. here and below all the "-1"'s remove counting the NUL terminator
     * on the statically allocated character strings.
     */
    length += sizeof(relationKeyword) - 1 ;
    length += 1 ; /* separating space */
    /*
     * Next scan the heading.
     */
    assert(flags->degree == Ral_TupleHeadingSize(tupleHeading)) ;
    length += sizeof(openList) - 1 ;
    thEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    for (thIter = Ral_TupleHeadingBegin(tupleHeading) ; thIter != thEnd ;
	++thIter) {
	Ral_Attribute attr = *thIter ;
	/*
	 * +1 to account for the space that needs to separate elements
	 * in the resulting list.
	 */
	length += Ral_AttributeScanName(attr, attrFlag) + 1 ;
	length += Ral_AttributeScanType(attr, attrFlag) + 1 ;
	++attrFlag ;
    }
    length += sizeof(closeList) - 1 ;
    /*
     * Scan the identifiers
     */
    length += 1 ; /* separating space */
    length += sizeof(openList) - 1 ;
    while (idCount-- > 0) {
	Ral_IntVector id = *ids++ ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
	int nIdAttrs = Ral_IntVectorSize(id) ;
	Ral_IntVectorIter iter ;

	if (nIdAttrs != 1) {
	    length += sizeof(openList) - 1 ;
	}
	for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	    Ral_Attribute attr = Ral_TupleHeadingFetch(tupleHeading, *iter) ;
	    length += Ral_AttributeScanName(attr, idFlag) + 1 ;
	    idFlag->type = Tcl_Type ; /* attribute names are always simple */
	    ++idFlag ;
	}
	if (Ral_IntVectorSize(id) != 0) {
	    length -= 1 ;
	}
	if (nIdAttrs != 1) {
	    length += sizeof(closeList) - 1 ;
	    length += 1 ; /* separating space */
	}
	length += 1 ; /* separating space */
    }
    if (h->idCount > 0) {
	length -= 1 ; /* remove trailing space */
    }
    length += sizeof(closeList) - 1 ;

    return length ;
}

int
Ral_RelationHeadingConvert(
    Ral_RelationHeading h,
    char *dst,
    Ral_RelationScanFlags flags)
{
    char *p = dst ;
    Ral_TupleHeading tupleHeading = h->tupleHeading ;
    Ral_TupleHeadingIter thIter ;
    Ral_TupleHeadingIter thEnd ;
    Ral_AttributeScanFlags attrFlag = flags->attrFlags ;
    int idCount = h->idCount ;
    Ral_AttributeScanFlags idFlag = flags->idFlags ;
    Ral_IntVector *ids = h->identifiers ;

    /*
     * Copy in the "Relation" keyword.
     */
    strcpy(p, relationKeyword) ;
    p += sizeof(relationKeyword) - 1 ;
    /*
     * Add the heading.
     */
    assert(flags->degree == Ral_TupleHeadingSize(tupleHeading)) ;
    *p++ = ' ' ;
    strcpy(p, openList) ;
    p += sizeof(openList) - 1 ;
    thEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    for (thIter = Ral_TupleHeadingBegin(tupleHeading) ; thIter != thEnd ;
	++thIter) {
	Ral_Attribute attr = *thIter ;
	p += Ral_AttributeConvertName(attr, p, attrFlag) ;
	*p++ = ' ' ;
	p += Ral_AttributeConvertType(attr, p, attrFlag) ;
	*p++ = ' ' ;
	++attrFlag ;
    }
    /*
     * Overwrite any trailing blank. There will be no trailing blank if the
     * heading didn't have any attributes.
     */
    if (!Ral_TupleHeadingEmpty(tupleHeading)) {
	--p ;
    }
    strcpy(p, closeList) ;
    p += sizeof(closeList) - 1 ;
    /*
     * Add the identifiers.
     */
    *p++ = ' ' ;
    strcpy(p, openList) ;
    p += sizeof(openList) - 1 ;
    while (idCount-- > 0) {
	Ral_IntVector id = *ids++ ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
	int nIdAttrs = Ral_IntVectorSize(id) ;
	Ral_IntVectorIter iter ;

	if (nIdAttrs != 1) {
	    strcpy(p, openList) ;
	    p += sizeof(openList) - 1 ;
	}
	for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	    Ral_Attribute attr = Ral_TupleHeadingFetch(tupleHeading, *iter) ;

	    p += Ral_AttributeConvertName(attr, p, idFlag++) ;
	    *p++ = ' ' ;
	}
	if (Ral_IntVectorSize(id) != 0) {
	    --p ;
	}
	if (nIdAttrs != 1) {
	    strcpy(p, closeList) ;
	    p += sizeof(closeList) - 1 ;
	}
	*p++ = ' ' ;
    }
    /*
     * Overwrite any trailing blank. There will be no trailing blank if the
     * heading didn't have any identifiers.
     */
    if (h->idCount > 0) {
	--p ;
    }
    strcpy(p, closeList) ; /* NUL terminates the result */
    p += sizeof(closeList) - 1 ;

    return p - dst ;
}

void
Ral_RelationHeadingPrint(
    Ral_RelationHeading h,
    const char *format,
    FILE *f)
{
    Ral_RelationScanFlags flags = Ral_RelationScanFlagsAlloc(h, 0) ;
    char *str = ckalloc(Ral_RelationHeadingScan(h, flags)) ;

    Ral_RelationHeadingConvert(h, str, flags) ;
    fprintf(f, format, str) ;
    ckfree(str) ;
    Ral_RelationScanFlagsFree(flags) ;
}

Ral_RelationScanFlags
Ral_RelationScanFlagsAlloc(
    Ral_RelationHeading h,
    int cardinality)
{
    int degree = Ral_TupleHeadingSize(h->tupleHeading) ;
    int nBytes ;
    Ral_RelationScanFlags flags ;
    Ral_IntVector *ids = h->identifiers ;
    int idCount = h->idCount ;
    int nIds = 0 ;

    flags = (Ral_RelationScanFlags)ckalloc(sizeof(*flags)) ;

    flags->degree = degree ;
    nBytes = flags->degree * sizeof(*flags->attrFlags) ;
    flags->attrFlags = (Ral_AttributeScanFlags)ckalloc(nBytes) ;
    memset(flags->attrFlags, 0, nBytes) ;

    /*
     * Count the number of distinct attributes used as identifiers.
     */
    while (idCount-- > 0) {
	nIds += Ral_IntVectorSize(*ids++) ;
    }
    flags->idCount = nIds ;
    nBytes = nIds * sizeof(*flags->idFlags) ;
    flags->idFlags = (Ral_AttributeScanFlags)ckalloc(nBytes) ;
    memset(flags->idFlags, 0, nBytes) ;

    flags->cardinality = cardinality ;
    nBytes = flags->degree * cardinality * sizeof(*flags->valueFlags) ;
    flags->valueFlags = (Ral_AttributeScanFlags)ckalloc(nBytes) ;
    memset(flags->valueFlags, 0, nBytes) ;

    return flags ;
}

void
Ral_RelationScanFlagsFree(
    Ral_RelationScanFlags flags)
{
    int degree = flags->degree ;
    Ral_AttributeScanFlags attrFlags = flags->attrFlags ;
    int idCount = flags->idCount ;
    Ral_AttributeScanFlags idFlags = flags->idFlags ;
    int nValues = degree * flags->cardinality ;
    Ral_AttributeScanFlags valueFlags = flags->valueFlags ;

    assert(attrFlags != NULL) ;
    while (degree-- > 0) {
	Ral_AttributeScanFlagsFree(attrFlags++) ;
    }
    ckfree((char *)flags->attrFlags) ;

    assert(flags->idFlags != NULL) ;
    while (idCount-- > 0) {
	Ral_AttributeScanFlagsFree(idFlags++) ;
    }
    ckfree((char *)flags->idFlags) ;

    assert(flags->valueFlags != NULL) ;
    while (nValues-- > 0) {
	Ral_AttributeScanFlagsFree(valueFlags++) ;
    }
    ckfree((char *)flags->valueFlags) ;

    ckfree((char *)flags) ;
}

const char *
Ral_RelationHeadingVersion(void)
{
    return rcsid ;
}
