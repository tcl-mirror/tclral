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
$Revision: 1.6 $
$Date: 2006/03/19 19:48:31 $
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
static const char rcsid[] = "@(#) $RCSfile: ral_relationobj.c,v $ $Revision: 1.6 $" ;

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
	"relation must have cardinality of one",
	"bad list of pairs",
	"bad list of triples",
	"attributes do not constitute an identifier",
	"attribute must be of a Relation type",

	"bad relation heading format",
	"bad value type for value",
	"wrong number of attributes specified",
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
	"CARDINALITY_ONE",
	"BAD_PAIRS_LIST",
	"BAD_TRIPLE_LIST",
	"NOT_AN_IDENTIFIER",
	"NOT_A_RELATION",

	"HEADING_ERR",
	"BAD_VALUE",
	"WRONG_NUM_ATTRS",
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
