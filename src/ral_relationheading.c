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
$Revision: 1.5 $
$Date: 2006/03/01 02:28:40 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relationheading.c,v $
Revision 1.5  2006/03/01 02:28:40  mangoa01
Added new relation commands and test cases. Cleaned up Makefiles.

Revision 1.4  2006/02/26 04:57:53  mangoa01
Reworked the conversion from internal form to a string yet again.
This design is better and more recursive in nature.
Added additional code to the "relation" commands.
Now in a position to finish off the remaining relation commands.

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
#include "ral_relationobj.h"
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
static const char rcsid[] = "@(#) $RCSfile: ral_relationheading.c,v $ $Revision: 1.5 $" ;

static const char relationKeyword[] = "Relation" ;
static const char openList = '{' ;
static const char closeList = '}' ;

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
Ral_RelationHeadingDegree(
    Ral_RelationHeading heading)
{
    return Ral_TupleHeadingSize(heading->tupleHeading) ;
}

int
Ral_RelationHeadingEqual(
    Ral_RelationHeading h1,
    Ral_RelationHeading h2)
{
    int id1Count ;
    Ral_IntVector *id1Array ;

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
    id1Array = h1->identifiers ;
    for (id1Count = h1->idCount ; id1Count > 0 ; --id1Count) {
	Ral_IntVector id1 ;
	Ral_IntVector id2 ;
	Ral_IntVectorIter end1Iter ;
	Ral_IntVectorIter id1Iter ;
	int id2Index = 0 ;
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
	/*
	 * Iterate through all the attribute indices of the identifier and
	 * construct the vector to have a set of attribute indices that
	 * are relative to the ordering of attributes in the second heading.
	 */
	for (id1Iter = Ral_IntVectorBegin(id1) ; id1Iter != end1Iter ;
	    ++id1Iter) {
	    /*
	     * Get the attribute.
	     */
	    Ral_Attribute attr1 = Ral_TupleHeadingFetch(h1->tupleHeading,
		*id1Iter) ;
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
	    Ral_IntVectorStore(id2, id2Index++, attr2Index) ;
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
    Ral_AttributeTypeScanFlags *flags)
{
    int nBytes ;
    int idCount = h->idCount ;
    Ral_IntVector *ids = h->identifiers ;
    int needIdList = Ral_IntVectorSize(*ids) != 1 ;
    int length = strlen(Ral_RelationObjType.name) + 1 ; /* +1 for space */

    assert(flags->attrType == Relation_Type) ;
    assert(flags->compoundFlags.flags == NULL) ;
    /*
     * Allocate space for the type flags for each attribute.
     */
    flags->compoundFlags.count = Ral_RelationHeadingDegree(h) ;
    nBytes = flags->compoundFlags.count * sizeof(*flags->compoundFlags.flags) ;
    flags->compoundFlags.flags = (Ral_AttributeTypeScanFlags *)ckalloc(nBytes) ;
    memset(flags->compoundFlags.flags, 0, nBytes) ;
    /*
     * The attribute / type list of the heading.
     */
    length += Ral_TupleHeadingScanAttr(h->tupleHeading, flags) ;
    /*
     * Scan the identifiers
     */
    length += 1 ; /* separating space */
    if (needIdList) {
	length += sizeof(openList) ;
    }
    while (idCount-- > 0) {
	Ral_IntVector id = *ids++ ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
	int nIdAttrs = Ral_IntVectorSize(id) ;
	Ral_IntVectorIter iter ;

	if (nIdAttrs != 1) {
	    length += sizeof(openList) ;
	}
	for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	    /*
	     * Reuse the length stored when the attribute name was scanned.
	     */
	    assert(*iter < flags->compoundFlags.count) ;
	    /*
	     * +1 for the separating space
	     */
	    length += flags->compoundFlags.flags[*iter].nameLength + 1 ;
	}
	if (Ral_IntVectorSize(id) != 0) {
	    length -= 1 ;
	}
	if (nIdAttrs != 1) {
	    length += sizeof(closeList) ;
	    length += 1 ; /* separating space */
	}
	length += 1 ; /* separating space */
    }
    if (h->idCount > 0) {
	length -= 1 ; /* remove trailing space */
    }
    if (needIdList) {
	length += sizeof(closeList) ;
    }

    return length ;
}

int
Ral_RelationHeadingConvert(
    Ral_RelationHeading h,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    char *p = dst ;
    int idCount = h->idCount ;
    Ral_IntVector *ids = h->identifiers ;
    int needIdList = Ral_IntVectorSize(*ids) != 1 ;
    Ral_TupleHeading tupleHeading = h->tupleHeading ;

    assert(flags->attrType == Relation_Type) ;
    assert(flags->compoundFlags.count == Ral_RelationHeadingDegree(h)) ;

    /*
     * Copy in the "Relation" keyword.
     */
    strcpy(p, relationKeyword) ;
    p += sizeof(relationKeyword) - 1 ;
    /*
     * Add the heading.
     */
    *p++ = ' ' ;
    p += Ral_TupleHeadingConvertAttr(tupleHeading, p, flags) ;
    /*
     * Add the identifiers.
     */
    *p++ = ' ' ;
    if (needIdList) {
	*p++ = openList ;
    }
    while (idCount-- > 0) {
	Ral_IntVector id = *ids++ ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(id) ;
	int nIdAttrs = Ral_IntVectorSize(id) ;
	Ral_IntVectorIter iter ;

	if (nIdAttrs != 1) {
	    *p++ = openList ;
	}
	for (iter = Ral_IntVectorBegin(id) ; iter != end ; ++iter) {
	    Ral_Attribute attr = Ral_TupleHeadingFetch(tupleHeading, *iter) ;
	    /*
	     * Reuse the flags gathered during the scanning of the header.
	     */
	    assert(*iter < flags->compoundFlags.count) ;
	    p += Tcl_ConvertElement(attr->name, p,
		flags->compoundFlags.flags[*iter].nameFlags) ;
	    *p++ = ' ' ;
	}
	if (Ral_IntVectorSize(id) != 0) {
	    --p ;
	}
	if (nIdAttrs != 1) {
	    *p++ = closeList ;
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
    if (needIdList) {
	*p++ = closeList ;
    }

    return p - dst ;
}

void
Ral_RelationHeadingPrint(
    Ral_RelationHeading h,
    const char *format,
    FILE *f)
{
    char *str = Ral_RelationHeadingStringOf(h) ;

    fprintf(f, format, str) ;
    ckfree(str) ;
}

char *
Ral_RelationHeadingStringOf(
    Ral_RelationHeading h)
{
    Ral_AttributeTypeScanFlags flags ;
    char *str ;
    int length ;

    memset(&flags, 0, sizeof(flags)) ;
    flags.attrType = Relation_Type ;
    /*
     * +1 for the NUL terminator
     */
    str = ckalloc(Ral_RelationHeadingScan(h, &flags) + 1) ;
    length = Ral_RelationHeadingConvert(h, str, &flags) ;
    str[length] = '\0' ;
    Ral_AttributeTypeScanFlagsFree(&flags) ;

    return str ;
}

const char *
Ral_RelationHeadingVersion(void)
{
    return rcsid ;
}
