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

$RCSfile: ral_relationobj.c,v $
$Revision: 1.14 $
$Date: 2006/05/29 21:07:42 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_utils.h"
#include "ral_relationobj.h"
#include "ral_relation.h"
#include "ral_tupleobj.h"
#include "ral_joinmap.h"

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
static void FreeRelationInternalRep(Tcl_Obj *) ;
static void DupRelationInternalRep(Tcl_Obj *, Tcl_Obj *) ;
static void UpdateStringOfRelation(Tcl_Obj *) ;
static int SetRelationFromAny(Tcl_Interp *, Tcl_Obj *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

Tcl_ObjType Ral_RelationObjType = {
    "Relation",
    FreeRelationInternalRep,
    DupRelationInternalRep,
    UpdateStringOfRelation,
    SetRelationFromAny
} ;

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationobj.c,v $ $Revision: 1.14 $" ;

/*
FUNCTION DEFINITIONS
*/

Tcl_Obj *
Ral_RelationObjNew(
    Ral_Relation relation)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->typePtr = &Ral_RelationObjType ;
    objPtr->internalRep.otherValuePtr = relation ;
    Tcl_InvalidateStringRep(objPtr) ;
    return objPtr ;
}

int
Ral_RelationObjConvert(
    Ral_RelationHeading heading,
    Tcl_Interp *interp,
    Tcl_Obj *value,
    Tcl_Obj *objPtr)
{
    /*
     * Create the relation and set the values of the tuples from the object.
     * The object must be a list of tuple values.
     */
    Ral_Relation relation = Ral_RelationNew(heading) ;

    if (Ral_RelationSetFromObj(relation, interp, value) != TCL_OK) {
	Ral_RelationDelete(relation) ;
	return TCL_ERROR ;
    }
    /*
     * Discard the old internal representation.
     */
    if (objPtr->typePtr && objPtr->typePtr->freeIntRepProc) {
	objPtr->typePtr->freeIntRepProc(objPtr) ;
    }
    /*
     * Invalidate the string representation.  There are several string reps
     * that will map to the same relation and we want to force a new string rep
     * to be generated in order to obtain the canonical string form.
     */
    Tcl_InvalidateStringRep(objPtr) ;
    /*
     * Install the new internal representation.
     */
    objPtr->typePtr = &Ral_RelationObjType ;
    objPtr->internalRep.otherValuePtr = relation ;

    return TCL_OK ;
}


Ral_RelationHeading
Ral_RelationHeadingNewFromObjs(
    Tcl_Interp *interp,
    Tcl_Obj *headingObj,
    Tcl_Obj *identObj)
{
    Ral_TupleHeading tupleHeading ;
    Ral_RelationHeading heading ;
    int idc ;
    Tcl_Obj **idv ;
    int idNum ;

    /*
     * Create the tuple heading.
     */
    tupleHeading = Ral_TupleHeadingNewFromObj(interp, headingObj) ;
    if (!tupleHeading) {
	return NULL ;
    }
    /*
     * The identifiers are also in a list.
     */
    if (Tcl_ListObjGetElements(interp, identObj, &idc, &idv) != TCL_OK) {
	Ral_TupleHeadingDelete(tupleHeading) ;
	return NULL ;
    }
    /*
     * If a relation has any attributes, then it must also have
     * an identifier.
     */
    if (idc == 0) {
	Ral_RelationObjSetError(interp, REL_NO_IDENTIFIER, NULL) ;
	Ral_TupleHeadingDelete(tupleHeading) ;
	return NULL ;
    }

    /*
     * We now know the number of identifers and can create the
     * relation heading.
     */
    heading = Ral_RelationHeadingNew(tupleHeading, idc) ;
    /*
     * Iterate over the identifier elements and insert them
     * into the heading.
     */
    for (idNum = 0 ; idNum < idc ; ++idNum) {
	/*
	 * Iterate over the members of each identifier.
	 */
	int elemc ;
	Tcl_Obj **elemv ;
	Ral_IntVector id ;

	if (Tcl_ListObjGetElements(interp, *idv++, &elemc, &elemv) != TCL_OK) {
	    Ral_RelationHeadingDelete(heading) ;
	    return NULL ;
	}
	/*
	 * Vector to hold the attribute indices that constitute the
	 * identifier.
	 */
	id = Ral_IntVectorNewEmpty(elemc) ;
	/*
	 * Find the attribute in the tuple heading and build up a
	 * vector to install in the relation heading.
	 */
	while (elemc-- > 0) {
	    const char *attrName = Tcl_GetString(*elemv++) ;
	    int index = Ral_TupleHeadingIndexOf(tupleHeading, attrName) ;

	    if (index < 0) {
		Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR, attrName) ;
		Ral_RelationHeadingDelete(heading) ;
		return NULL ;
	    }
	    /*
	     * The indices in an identifier must form a set, i.e. you
	     * cannot have a duplicate attribute in a list of attributes
	     * that is intended to be an identifier of a relation.
	     */
	    if (!Ral_IntVectorSetAdd(id, index)) {
		Ral_RelationObjSetError(interp, REL_DUP_ATTR_IN_ID, attrName) ;
		Ral_RelationHeadingDelete(heading) ;
		return NULL ;
	    }
	}
	/*
	 * Add the set of attribute indices as an identifier. Adding
	 * the vector checks that we do not have a subset dependency.
	 */
	if (!Ral_RelationHeadingAddIdentifier(heading, idNum, id)) {
	    Ral_RelationObjSetError(interp, REL_IDENTIFIER_SUBSET, NULL) ;
	    Ral_RelationHeadingDelete(heading) ;
	    return NULL ;
	}
    }

    return heading ;
}

int
Ral_RelationSetFromObj(
    Ral_Relation relation,
    Tcl_Interp *interp,
    Tcl_Obj *tupleList)
{
    int elemc ;
    Tcl_Obj **elemv ;

    if (Tcl_ListObjGetElements(interp, tupleList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }

    /*
     * Reserve the required storage so as not to reallocate during the
     * insertions.
     */
    Ral_RelationReserve(relation, elemc) ;

    while (elemc-- > 0) {
	if (Ral_RelationInsertTupleObj(relation, interp, *elemv++) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

int
Ral_RelationInsertTupleObj(
    Ral_Relation relation,
    Tcl_Interp *interp,
    Tcl_Obj *tupleObj)
{
    Ral_Tuple tuple ;

    /*
     * Make the new tuple refer to the heading contained in the relation.
     */
    tuple = Ral_TupleNew(relation->heading->tupleHeading) ;
    /*
     * Set the values of the attributes from the list of attribute / value
     * pairs.
     */
    if (Ral_TupleSetFromObj(tuple, interp, tupleObj) != TCL_OK) {
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }
    /*
     * Insert the tuple into the relation.
     */
    if (!Ral_RelationPushBack(relation, tuple, NULL)) {
	Ral_RelationObjSetError(interp, REL_DUPLICATE_TUPLE,
	    Tcl_GetString(tupleObj)) ;
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

int Ral_RelationObjParseJoinArgs(
    Tcl_Interp *interp,
    int *objcPtr,
    Tcl_Obj *const**objvPtr,
    Ral_Relation r1,
    Ral_Relation r2,
    Ral_JoinMap joinMap)
{
    int findCommon = 1 ;
    int objc = *objcPtr ;
    Tcl_Obj *const*objv = *objvPtr ;

    if (objc) {
	const char *nextArg = Tcl_GetString(*objv) ;
	if (strcmp(nextArg, "-using") == 0) {
	    findCommon = 0 ;
	    /*
	     * Join arguments specified.
	     */
	    if (Ral_RelationFindJoinAttrs(interp, r1, r2, *(objv + 1), joinMap)
		!= TCL_OK) {
		return TCL_ERROR ;
	    }
	    *objcPtr -= 2 ;
	    *objvPtr += 2 ;
	}
    }
    if (findCommon) {
	/*
	 * Join on common attributes.
	 */
	Ral_TupleHeadingCommonAttributes(
	    r1->heading->tupleHeading, r2->heading->tupleHeading, joinMap) ;
    }

    return TCL_OK ;
}

int
Ral_RelationFindJoinAttrs(
    Tcl_Interp *interp,
    Ral_Relation r1,
    Ral_Relation r2,
    Tcl_Obj *attrList,
    Ral_JoinMap joinMap)
{
    Ral_TupleHeading r1TupleHeading = r1->heading->tupleHeading ;
    Ral_TupleHeading r2TupleHeading = r2->heading->tupleHeading ;
    int objc ;
    Tcl_Obj **objv ;

    if (Tcl_ListObjGetElements(interp, attrList, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc % 2 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_PAIRS_LIST,
	    Tcl_GetString(attrList)) ;
	return TCL_ERROR ;
    }
    Ral_JoinMapAttrReserve(joinMap, objc / 2) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	int r1Index = Ral_TupleHeadingIndexOf(r1TupleHeading,
	    Tcl_GetString(*objv)) ;
	int r2Index = Ral_TupleHeadingIndexOf(r2TupleHeading,
	    Tcl_GetString(*(objv + 1))) ;
	if (r1Index < 0) {
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
		Tcl_GetString(*objv)) ;
	    return TCL_ERROR ;
	}
	if (r2Index < 0) {
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
		Tcl_GetString(*(objv + 1))) ;
	    return TCL_ERROR ;
	}
	Ral_JoinMapAddAttrMapping(joinMap, r1Index, r2Index) ;
    }

    return TCL_OK ;
}

/*
 * Returns a tuple that has values set for the given attributes.
 * The attributes are examined to make sure that they form an identifier
 * so that the resulting tuple can be used as a key to look up a particular
 * tuple in the relation.
 * Caller must delete the returned tuple.
 */
Ral_Tuple
Ral_RelationObjKeyTuple(
    Tcl_Interp *interp,
    Ral_Relation relation,
    int objc,
    Tcl_Obj *const*objv,
    int *idRef)
{
    Ral_RelationHeading heading = relation->heading ;
    Ral_TupleHeading tupleHeading = heading->tupleHeading ;
    Ral_IntVector id ;
    Ral_Tuple key ;
    int idNum ;

    if (objc % 2 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_PAIRS_LIST, "key tuple") ;
	return NULL ;
    }
    /*
     * Iterate through the name/value list and construct an identifier
     * vector from the attribute names and a key tuple from the corresponding
     * values.
     */
    id = Ral_IntVectorNewEmpty(objc / 2) ;
    key = Ral_TupleNew(tupleHeading) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	const char *attrName = Tcl_GetString(*objv) ;
	int attrIndex = Ral_TupleHeadingIndexOf(tupleHeading, attrName) ;
	int updated ;

	if (attrIndex < 0) {
	    Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR, attrName) ;
	    goto error_out ;
	}
	Ral_IntVectorPushBack(id, attrIndex) ;

	updated = Ral_TupleUpdateAttrValue(key, attrName, *(objv + 1)) ;
	if (!updated) {
	    Ral_TupleObjSetError(interp, Ral_TupleLastError,
		Tcl_GetString(*(objv + 1))) ;
	    goto error_out ;
	}
    }

    /*
     * Check if the attributes given do constitute an identifier.
     */
    idNum = Ral_RelationHeadingFindIdentifier(heading, id) ;
    if (idNum < 0) {
	Ral_RelationObjSetError(interp, REL_NOT_AN_IDENTIFIER,
	    "during find operation") ;
	goto error_out ;
    } else if (idRef) {
	*idRef = idNum ;
    }
    Ral_IntVectorDelete(id) ;

    return key ;

error_out:
    Ral_IntVectorDelete(id) ;
    Ral_TupleDelete(key) ;
    return NULL ;
}

int
Ral_RelationObjUpdateTuple(
    Tcl_Interp *interp,
    Tcl_Obj *updateList,
    Ral_Relation relation,
    Ral_RelationIter tupleIter)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_Tuple tuple ;
    int updated ;

    if (Tcl_ListObjGetElements(interp, updateList, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc % 2 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_PAIRS_LIST,
	    Tcl_GetString(updateList)) ;
	return TCL_ERROR ;
    }
    /*
     * Clone the tuple. This is the only way that we can handle the potential
     * for errors with unknown attributes, etc.
     */
    tuple = Ral_TupleDup(*tupleIter) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	const char *attrName = Tcl_GetString(objv[0]) ;
	updated = Ral_TupleUpdateAttrValue(tuple, attrName, objv[1]) ;

	if (!updated) {
	    Ral_TupleObjSetError(interp, Ral_TupleLastError, attrName) ;
	    Ral_TupleDelete(tuple) ;
	    return TCL_ERROR ;
	}
    }
    if (!Ral_RelationUpdate(relation, tupleIter, tuple, NULL)) {
	char *tupleStr = Ral_TupleValueStringOf(tuple) ;
	Ral_RelationObjSetError(interp, REL_DUPLICATE_TUPLE, tupleStr) ;
	ckfree(tupleStr) ;
	Ral_TupleDelete(tuple) ;
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

const char *
Ral_RelationObjVersion(void)
{
    return rcsid ;
}

void
Ral_RelationObjSetError(
    Tcl_Interp *interp,
    Ral_RelationError error,
    const char *param)
{
    /*
     * These must be in the same order as the encoding of the Ral_RelationError
     * enumeration.
     */
    static const char *resultStrings[] = {
	"no error",
	"bad relation value format",
	"bad relation type keyword",
	"relations of non-zero degree must have at least one identifier",
	"identifiers must have at least one attribute",
	"identifiers must not be subsets of other identifiers",
	"duplicate attribute name in identifier attribute set",
	"unknown attribute name",
	"duplicate tuple",
	"headings not equal",
	"duplicate attribute name",
	"relation must have degree of one",
	"relation must have degree of two",
	"relation must have cardinality of one",
	"bad list of pairs",
	"bad list of triples",
	"attributes do not constitute an identifier",
	"attribute must be of a Relation type",
	"relation is not a projection of the summarized relation",
	"divisor heading must be disjoint from the dividend heading",
	"mediator heading must be a union of the dividend and divisor headings",
	"too many attributes specified",
	"attributes must have the same type",
	"only a single identifier may be specified",
	"identifier must have only a single attribute",
	"\"-within\" option attributes are not the subset of any identifier",
	"attribute is not a valid type for rank operation",
    } ;
    static const char *errorStrings[] = {
	"OK",
	"FORMAT_ERR",
	"BAD_KEYWORD",
	"NO_IDENTIFIER",
	"IDENTIFIER_FORMAT",
	"IDENTIFIER_SUBSET",
	"DUP_ATTR_IN_ID",
	"UNKNOWN_ATTR",
	"DUPLICATE_TUPLE",
	"HEADING_NOT_EQUAL",
	"DUPLICATE_ATTR",
	"DEGREE_ONE",
	"DEGREE_TWO",
	"CARDINALITY_ONE",
	"BAD_PAIRS_LIST",
	"BAD_TRIPLE_LIST",
	"NOT_AN_IDENTIFIER",
	"NOT_A_RELATION",
	"NOT_A_PROJECTION",
	"NOT_DISJOINT",
	"NOT_UNION",
	"TOO_MANY_ATTRS",
	"TYPE_MISMATCH",
	"SINGLE_IDENTIFIER",
	"SINGLE_ATTRIBUTE",
	"WITHIN_NOT_SUBSET",
	"BAD_RANK_TYPE",
    } ;

    Ral_ObjSetError(interp, "RELATION", resultStrings[error],
	errorStrings[error], param) ;
}

/*
 * ======================================================================
 * Functions to Support the Relation type
 * ======================================================================
 */

static void
FreeRelationInternalRep(
    Tcl_Obj *objPtr)
{
    assert(objPtr->typePtr == &Ral_RelationObjType) ;
    Ral_RelationDelete(objPtr->internalRep.otherValuePtr) ;
    objPtr->typePtr = objPtr->internalRep.otherValuePtr = NULL ;
}

static void
DupRelationInternalRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    Ral_Relation srcRelation ;
    Ral_Relation dupRelation ;

    assert(srcPtr->typePtr == &Ral_RelationObjType) ;
    srcRelation = srcPtr->internalRep.otherValuePtr ;

    dupRelation = Ral_RelationDup(srcRelation) ;
    if (dupRelation) {
	dupPtr->internalRep.otherValuePtr = dupRelation ;
	dupPtr->typePtr = &Ral_RelationObjType ;
    }
}

static void
UpdateStringOfRelation(
    Tcl_Obj *objPtr)
{
    objPtr->bytes = Ral_RelationStringOf(objPtr->internalRep.otherValuePtr) ;
    objPtr->length = strlen(objPtr->bytes) ;
}

static int
SetRelationFromAny(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_RelationHeading heading ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc != 4) {
	Ral_RelationObjSetError(interp, REL_FORMAT_ERR, Tcl_GetString(objPtr)) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_RelationObjType.name, Tcl_GetString(*objv)) != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_KEYWORD, Tcl_GetString(*objv)) ;
	return TCL_ERROR ;
    }
    /*
     * Create the heading from the external representation.
     */
    heading = Ral_RelationHeadingNewFromObjs(interp, objv[1], objv[2]) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    return Ral_RelationObjConvert(heading, interp, objv[3], objPtr) ;
}
