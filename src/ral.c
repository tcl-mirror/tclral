/*
This software is copyrighted 2004 by G. Andrew Mangogna.  The following
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

$RCSfile: ral.c,v $
$Revision: 1.3 $
$Date: 2004/04/26 04:13:08 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "tcl.h"

/*
MACRO DEFINITIONS
*/


/*
TYPE DEFINITIONS
*/

/*
 * Attributes can be of either a base type supplied by Tcl or one of
 * the generated types of Tuple or Relation.
 */
typedef enum Ral_AttrDataType {
    Tcl_Type,
    Tuple_Type,
    Relation_Type
} Ral_AttrDataType ;

/*
 * A Heading Attribute is a component of a Heading.  A Heading is composed of
 * an Attribute Name and a data type.  In this case the "name" member is a Tcl
 * object containing the attribute name and the data type is described by a
 * pointer to a Tcl type structure.  For "Tuple" or "Relation" types,
 * additional information is needed to specify the heading.
 */

typedef struct Ral_Attribute {
    Tcl_Obj *name ;
    Ral_AttrDataType attrType ;
    union {
	Tcl_ObjType *tclType ;
	struct Ral_TupleHeading *tupleHeading ;
	struct Ral_RelationHeading *relationHeading ;
    } ;
} Ral_Attribute ;

/*
 * A Tuple Heading is a set of Heading Attributes. The attributes are stored in
 * an array. A hash table is used to map attribute names to an index in the
 * attribute array. The "nameMap" hash table has as a key the attribute name
 * and as a value the index into the attribute array.
 *
 * The external string representation of a "heading" is a list with each
 * element being a sub-list containing an attribute name / attribute type pair.
 * For example:
 *
 *	{{Name string} {Street int} {Wage double}}
 *
 * If the type of an attribute is Tuple or Relation, then the type sub-element
 * will be a list of two elements itself. The first element will be the keyword
 * "Tuple" or "Relation" and the second element will be the heading.  For
 * example:
 *
 *	{{Name string} {Address {Tuple {{Number int} {Street string}}}}}
 *
 * The attribute name can be an arbitrary string. Attribute names must be
 * unique within a given heading. The attribute type must be a valid registered
 * Tcl type such that Tcl_GetObjType() will return a non-NULL result or a Tuple
 * or Relation type.
 */

typedef struct Ral_TupleHeading {
    int refCount ;		/* Reference Count */
    int degree ;		/* Number of attributes */
    Ral_Attribute *attrVector ;	/* Pointer to an array of attributes */
    Tcl_HashTable nameMap ;	/* Map of attribute name to vector indices */
} Ral_TupleHeading ;

/*
 * A Tuple type is a heading combined with a list of values, one for each
 * attribute.
 *
 * The string representation of a "Tuple" is a specially formed list.  The list
 * consists of three elements:
 *
 * 1. The keyword "Tuple".
 *
 * 2. A heading definition.
 *
 * 3. A value definition.
 *
 * The keyword distinguishes the string as a Tuple.  The heading is as described
 * above.  The heading consists of a list of sublists which contains the
 * Attribute Names and Data Types.  The value definition is also a list
 * consisting of Attribute Name / Attribute Value pairs.
 * E.G.
 *	{Tuple {{Name string} {Street int} {Wage double}}\
 *	{Name Andrew Street Blackwood Wage 5.74}}
 */

typedef struct Ral_Tuple {
    Ral_TupleHeading *heading ;	/* Pointer to Tuple heading */
    Tcl_Obj **values ;		/* Pointer to an array of values */
} Ral_Tuple ;

/*
 * A Relation may also have identifiers defined for it. Identifiers are sub
 * sets of attributes for which the tuples may not have duplicated values. We
 * will also build hash tables for the identifiers that can be used for indices
 * into the values of a Relation.
 */

typedef struct Ral_RelId {
    int attrCount ;	/* Number of attributes in the identifier */
    int *attrVector ;	/* Pointer to an array of attribute indices that
			 * constitute the identifier */
    Tcl_HashTable attrMap ;	/* Hash table that maps the identifier values to
				 * the index of the tuple in the relation that
				 * matches the identifer */
} Ral_RelId ;

/*
 * A Relation Heading is a Tuple Heading with the addition of identifiers.
 */
typedef struct Ral_RelationHeading {
    int refCount ;
    Ral_TupleHeading *tupleHeading ;
    int idCount ;
    Ral_RelId *idVector ;
} Ral_RelationHeading ;

/*
 * A Relation type consists of a heading with a body.  A body consists
 * of zero or more tuples.

 * The external string representation of a "Relation" is a specially
 * formatted list. The list consists of four elements.
 *
 * 1. The keyword "Relation".
 *
 * 2. A relation heading. A relation heading is a two element list.
 *    The first sub-element is a tuple heading as described above.  The second
 *    sub-element is a list of identifiers (keys).  Each element is in turn a
 *    list of attribute names that constitute the identifier.
 *
 * 3. A list of tuple values. Each element of the list is a set of
 *    Attribute Name / Attribute Value pairs.
 * E.G.
 *    {Relation {{{Name string} {Number int} {Wage double}} {{Name}}}\
 *    {{Name Andrew Number 200 Wage 5.75}\
 *     {Name George Number 174 Wage 10.25}}}
 * is a Relation consisting of a body which has two tuples.
 * All tuples in a relation must have the same heading and all tuples
 * in a relation must be unique.
 */

typedef struct Ral_Relation {
    Ral_RelationHeading *heading ;
    int cardinality ;
    int allocated ;
    Tcl_Obj **tupleVector ;
} Ral_Relation ;

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static int tupleHeadingEqual(Ral_TupleHeading *, Ral_TupleHeading *) ;
static void tupleHeadingReference(struct Ral_TupleHeading *) ;
static void tupleHeadingUnreference(struct Ral_TupleHeading *) ;
static Ral_TupleHeading *tupleHeadingNewFromString(Tcl_Interp *, Tcl_Obj *) ;
static Tcl_Obj *tupleHeadingObjNew(Tcl_Interp *, Ral_TupleHeading *) ;

static int relationHeadingEqual(Ral_RelationHeading *, Ral_RelationHeading *) ;
static void relationHeadingReference(struct Ral_RelationHeading *) ;
static void relationHeadingUnreference(struct Ral_RelationHeading *) ;
static Ral_RelationHeading *relationHeadingNewFromString(Tcl_Interp *,
    Tcl_Obj *) ;
static Tcl_Obj *relationHeadingObjNew(Tcl_Interp *,
    Ral_RelationHeading *) ;

static Ral_Tuple *tupleNew(Ral_TupleHeading *) ;
static void tupleDelete(Ral_Tuple *) ;
static Tcl_Obj *tupleObjNew(Ral_Tuple *) ;
static int tupleSetValuesFromString(Tcl_Interp *, Ral_Tuple *, Tcl_Obj *) ;
static Tcl_Obj *tupleNameValueList(Tcl_Interp *, Ral_Tuple *) ;

static Ral_Relation *relationNew(Ral_RelationHeading *) ;
static void relationDelete(Ral_Relation *) ;
static Tcl_Obj *relationObjNew(Ral_Relation *) ;
static int relationSetValuesFromString(Tcl_Interp *, Ral_Relation *,
    Tcl_Obj *) ;
static Tcl_Obj *relationNameValueList(Tcl_Interp *, Ral_Relation *) ;

/*
 * Functions that implement the generic object operations for a Tuple.
 */
static void FreeTupleInternalRep(Tcl_Obj *) ;
static void DupTupleInternalRep(Tcl_Obj *, Tcl_Obj *) ;
static void UpdateStringOfTuple(Tcl_Obj *) ;
static int SetTupleFromAny(Tcl_Interp *, Tcl_Obj *) ;

/*
 * Functions that implement the generic object operations for a Relation.
 */
static void FreeRelationInternalRep(Tcl_Obj *) ;
static void DupRelationInternalRep(Tcl_Obj *, Tcl_Obj *) ;
static void UpdateStringOfRelation(Tcl_Obj *) ;
static int SetRelationFromAny(Tcl_Interp *, Tcl_Obj *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
STATIC DATA ALLOCATION
*/
static char rcsid[] = "@(#) $RCSfile: ral.c,v $ $Revision: 1.3 $" ;

static Tcl_ObjType Ral_TupleType = {
    "Tuple",
    FreeTupleInternalRep,
    DupTupleInternalRep,
    UpdateStringOfTuple,
    SetTupleFromAny
} ;


static Tcl_ObjType Ral_RelationType = {
    "Relation",
    FreeRelationInternalRep,
    DupRelationInternalRep,
    UpdateStringOfRelation,
    SetRelationFromAny
} ;

static Tcl_HashTable relvarMap ;

/*
FUNCTION DEFINITIONS
*/

/*
 * ======================================================================
 * Utility Functions
 * ======================================================================
 */

/*
 * ObjsEqual

 * Compare two objects. The comparison is based on comparing the string
 * representation of the objects.

 * Returns 1 if the objects are equal, 0 otherwise.
 */

/*
 * There's a problem here. Tuples and relations cannot compared based
 * on their string representations.
 */

static int
ObjsEqual(
    Tcl_Obj *o1,	/* first object pointer to compare */
    Tcl_Obj *o2)	/* second object pointer to compare */
{
    const char *s1 ;
    int l1 ;
    const char *s2 ;
    int l2 ;

    if (o1 == o2) {
	return 1 ;
    }
    s1 = Tcl_GetStringFromObj(o1, &l1) ;
    s2 = Tcl_GetStringFromObj(o2, &l2) ;
    if (l1 != l2) {
	return 0 ;
    }
    return (memcmp(s1, s2, l1) == 0) ;
}

static Tcl_Obj *
ObjFindInVect (
    int objc,		    /* count of objects in the search array */
    Tcl_Obj *const*objv,    /* pointer to the search array start */
    Tcl_Obj *key)	    /* object to compare against */
{
    for ( ; objc > 0 ; --objc, ++objv) {
	if (ObjsEqual(key, *objv)) {
	    return *objv ;
	}
    }

    return NULL ;
}

static int
int_compare(
    const void *m1,
    const void *m2)
{
    const int *i1 = m1 ;
    const int *i2 = m2 ;

    return *i1 - *i2 ;
}

/*
 * ======================================================================
 * Attribute Functions
 * ======================================================================
 */

static void
attributeCtorTclType(
    Ral_Attribute *attr,
    Tcl_Obj *attrName,
    Tcl_ObjType *attrType)
{
    Tcl_IncrRefCount(attr->name = attrName) ;
    attr->attrType = Tcl_Type ;
    attr->tclType = attrType ;
}

static void
attributeCtorTupleType(
    Ral_Attribute *attr,
    Tcl_Obj *attrName,
    struct Ral_TupleHeading *heading)
{
    Tcl_IncrRefCount(attr->name = attrName) ;
    attr->attrType = Tuple_Type ;
    tupleHeadingReference(attr->tupleHeading = heading) ;
}

static void
attributeCtorRelationType(
    Ral_Attribute *attr,
    Tcl_Obj *attrName,
    struct Ral_RelationHeading *heading)
{
    Tcl_IncrRefCount(attr->name = attrName) ;
    attr->attrType = Relation_Type ;
    relationHeadingReference(attr->relationHeading = heading) ;
}

static void
attributeDtor(
    Ral_Attribute *attr)
{
    if (attr->name) {
	Tcl_DecrRefCount(attr->name) ;
	switch (attr->attrType) {
	case Tuple_Type:
	    tupleHeadingUnreference(attr->tupleHeading) ;
	    break ;

	case Relation_Type:
	    relationHeadingUnreference(attr->relationHeading) ;
	    break ;
	}
	/*
	 * Nothing to do for Tcl_Type.
	 */
    }
}

static void
attributeCopy(
    Ral_Attribute *src,
    Ral_Attribute *dest)
{
    Tcl_IncrRefCount(dest->name = src->name) ;
    switch (dest->attrType = src->attrType) {
    case Tcl_Type:
	dest->tclType = src->tclType ;
	break ;

    case Tuple_Type:
	tupleHeadingReference(dest->tupleHeading = src->tupleHeading) ;
	break ;

    case Relation_Type:
	relationHeadingReference(dest->relationHeading = src->relationHeading) ;
	break ;

    default:
	Tcl_Panic("unknown attribute data type") ;
    }
}

static int
attributeEqual(
    Ral_Attribute *a1,
    Ral_Attribute *a2)
{
    int result ;

    if (a1 == a2) {
	return 1 ;
    }
    if (!(ObjsEqual(a1->name, a2->name) && a1->attrType == a2->attrType)) {
	return 0 ;
    }
    switch (a1->attrType) {
    case Tcl_Type:
	result = a1->tclType == a2->tclType ;
	break ;

    case Tuple_Type:
	result = tupleHeadingEqual(a1->tupleHeading, a2->tupleHeading) ;
	break ;

    case Relation_Type:
	result = relationHeadingEqual(a1->relationHeading,
	    a2->relationHeading) ;
	break ;

    default:
	Tcl_Panic("unknown attribute data type") ;
    }

    return result ;
}

/*
 * Assign to an attribute based on the name and data type.
 */
static int
attributeFromObjs(
    Tcl_Interp *interp,
    Ral_Attribute *attr,
    Tcl_Obj *attrName,
    Tcl_Obj *dataType)
{
    /*
     * By default, if the data type is missing, then assume "string"
     */
    if (dataType == NULL) {
	attributeCtorTclType(attr, attrName, Tcl_GetObjType("string")) ;
	assert(attr->tclType != NULL) ;
    } else {
	int typec ;
	Tcl_Obj **typev ;

	if (Tcl_ListObjGetElements(interp, dataType, &typec, &typev)
	    != TCL_OK) {
	    return TCL_ERROR ;
	}
	if (typec == 1) {
	    Tcl_ObjType *tclType = Tcl_GetObjType(Tcl_GetString(*typev)) ;

	    if (tclType == NULL) {
		if (interp) {
		    Tcl_ResetResult(interp) ;
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"unknown data type, \"", Tcl_GetString(*typev), "\"",
			NULL) ;
		}
		return TCL_ERROR ;
	    }
	    attributeCtorTclType(attr, attrName, tclType) ;
	} else if (typec == 2) {
	    const char *typeName = Tcl_GetString(*typev) ;
	    if (strcmp("Tuple", typeName) == 0) {
		Ral_TupleHeading *heading =
		    tupleHeadingNewFromString(interp, *(typev + 1)) ;

		if (heading) {
		    attributeCtorTupleType(attr, attrName, heading) ;
		} else {
		    return TCL_ERROR ;
		}
	    } else if (strcmp("Relation", typeName) == 0) {
		Ral_RelationHeading *heading =
		    relationHeadingNewFromString(interp, *(typev + 1)) ;

		if (heading) {
		    attributeCtorRelationType(attr, attrName, heading) ;
		} else {
		    return TCL_ERROR ;
		}
	    } else {
		if (interp) {
		    Tcl_ResetResult(interp) ;
		    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
			"expected Tuple or Relation type, but got, \"",
			typeName, "\"", NULL) ;
		}
		return TCL_ERROR ;
	    }
	} else {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "bad type specification, \"", Tcl_GetString(dataType),
		    "\"", NULL) ;
	    }
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

/*
 * Assign to an attribute based on a string.
 */
static int
attributeFromString(
    Tcl_Interp *interp,
    Ral_Attribute *attr,
    Tcl_Obj *attrSpec)
{
    int pairc ;
    Tcl_Obj **pairv ;

    if (Tcl_ListObjGetElements(interp, attrSpec, &pairc, &pairv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (pairc == 1) {
	attributeFromObjs(interp, attr, *pairv, NULL) ;
    } else if (pairc == 2) {
	attributeFromObjs(interp, attr, *pairv, *(pairv + 1)) ;
    } else {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"bad heading format, \"", Tcl_GetString(attrSpec), "\"", NULL) ;
	}
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

static int
attributeConvertValue(
    Tcl_Interp *interp,
    Ral_Attribute *attr,
    Tcl_Obj *objPtr)
{
    switch (attr->attrType) {
    case Tcl_Type:
	if (Tcl_ConvertToType(interp, objPtr, attr->tclType) != TCL_OK) {
	    return TCL_ERROR ;
	}
	if (strcmp(attr->tclType->name, "string") != 0) {
	    Tcl_InvalidateStringRep(objPtr) ;
	}
	break ;

    case Tuple_Type:
	if (objPtr->typePtr != &Ral_TupleType) {
	    Ral_Tuple *tuple = tupleNew(attr->tupleHeading) ;

	    if (tupleSetValuesFromString(interp, tuple, objPtr) != TCL_OK) {
		tupleDelete(tuple) ;
		return TCL_ERROR ;
	    }
	    /*
	     * Discard the old internal representation.
	     */
	    if (objPtr->typePtr && objPtr->typePtr->freeIntRepProc) {
		objPtr->typePtr->freeIntRepProc(objPtr) ;
	    }
	    /*
	     * Invalidate the string representation.
	     */
	    Tcl_InvalidateStringRep(objPtr) ;
	    /*
	     * Install the new internal representation.
	     */
	    objPtr->typePtr = &Ral_TupleType ;
	    objPtr->internalRep.otherValuePtr = tuple ;
	}
	break ;

    case Relation_Type:
	if (objPtr->typePtr != &Ral_RelationType) {
	    Ral_Relation *relation = relationNew(attr->relationHeading) ;

	    if (relationSetValuesFromString(interp, relation, objPtr)
		!= TCL_OK) {
		relationDelete(relation) ;
		return TCL_ERROR ;
	    }
	    /*
	     * Discard the old internal representation.
	     */
	    if (objPtr->typePtr && objPtr->typePtr->freeIntRepProc) {
		objPtr->typePtr->freeIntRepProc(objPtr) ;
	    }
	    /*
	     * Invalidate the string representation.
	     */
	    Tcl_InvalidateStringRep(objPtr) ;
	    /*
	     * Install the new internal representation.
	     */
	    objPtr->typePtr = &Ral_RelationType ;
	    objPtr->internalRep.otherValuePtr = relation ;
	}
	break ;

    default:
	Tcl_Panic("unknown attribute data type") ;
	break ;
    }

    return TCL_OK ;
}

static int
attributeValueToString(
    Tcl_Interp *interp,
    Ral_Attribute *attr,
    Tcl_Obj *valuePtr,
    Tcl_Obj **resultPtr)
{
    Tcl_Obj *result ;

    switch (attr->attrType) {
    case Tcl_Type:
	/*
	 * For ordinary objects, the string representation of the object is
	 * what we want.
	 */
	result = valuePtr ;
	break ;

    case Tuple_Type:
    {
	/*
	 * For tuple types, the value of the attribute is a list of
	 * attribute names and corresponding values.
	 */
	Ral_Tuple *tuple ;

	if (Tcl_ConvertToType(interp, valuePtr, &Ral_TupleType) != TCL_OK) {
	    return TCL_ERROR ;
	}
	tuple = valuePtr->internalRep.otherValuePtr ;
	result = tupleNameValueList(interp, tuple) ;
    }
	break ;

    case Relation_Type:
    {
	/*
	 * For relation types, the value of the attribute is a list of
	 * lists in which sub-element is like a tuple consisting of a list
	 * of attribute names and corresponding values.
	 */
	Ral_Relation *relation ;

	if (Tcl_ConvertToType(interp, valuePtr, &Ral_RelationType) != TCL_OK) {
	    return TCL_ERROR ;
	}
	relation = valuePtr->internalRep.otherValuePtr ;
	result = relationNameValueList(interp, relation) ;
    }
	break ;

    default:
	Tcl_Panic("unknown attribute data type") ;
	break ;
    }

    return (*resultPtr = result) ? TCL_OK : TCL_ERROR ;
}

/*
 * ======================================================================
 * Tuple Heading Functions
 * ======================================================================
 */

static Ral_TupleHeading *
tupleHeadingNew(
    int degree)
{
    int nBytes ;
    Ral_TupleHeading *heading ;

    /*
     * Allocate space for both the heading and the attribute vector as a
     * single chunk.
     */
    nBytes = sizeof(*heading) + degree * sizeof(*heading->attrVector) ;
    heading = (Ral_TupleHeading *)ckalloc(nBytes) ;
    memset(heading, 0, nBytes) ;
    heading->refCount = 0 ;
    heading->degree = degree ;
    heading->attrVector = (Ral_Attribute *)(heading + 1) ;
    Tcl_InitObjHashTable(&heading->nameMap) ;

    return heading ;
}

static void
tupleHeadingDelete(
    Ral_TupleHeading *heading)
{
    Ral_Attribute *attr ;
    Ral_Attribute *last ;

    assert(heading->refCount <= 0) ;

    last = heading->attrVector + heading->degree ;
    for (attr = heading->attrVector ; attr != last ; ++attr) {
	attributeDtor(attr) ;
    }

    Tcl_DeleteHashTable(&heading->nameMap) ;
    ckfree((char *)heading) ;
}

static void
tupleHeadingReference(
    Ral_TupleHeading * heading)
{
    ++heading->refCount ;
}

static void
tupleHeadingUnreference(
    Ral_TupleHeading *heading)
{
    if (heading && --heading->refCount < 0) {
	tupleHeadingDelete(heading) ;
    }
}

static int
tupleHeadingCopy(
    Tcl_Interp *interp,
    Ral_Attribute *first,
    Ral_Attribute *last,
    Ral_TupleHeading *dest,
    int start)
{
    Ral_Attribute *destAttr ;

    assert(first <= last) ;
    assert(start <= dest->degree) ;
    assert(start + (last - first) <= dest->degree) ;

    for (destAttr = dest->attrVector + start ; first != last ;
	++first, ++destAttr) {
	Tcl_HashEntry *entry ;
	int newPtr ;

	/*
	 * Add the new attribute name to the nameMap.
	 */
	entry = Tcl_CreateHashEntry(&dest->nameMap, (char *)first->name,
	    &newPtr) ;
	/*
	 * Check that there are no duplicate attribute names.
	 */
	if (newPtr == 0) {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "duplicate attribute name, \"",
		    Tcl_GetString(first->name), "\"", NULL) ;
	    }
	    return TCL_ERROR ;
	}
	/*
	 * Insert the new heading attribute and record the index into
	 * the nameMap.
	 */
	Tcl_SetHashValue(entry, start++) ;
	attributeCopy(first, destAttr) ;
    }

    return TCL_OK ;
}

static Ral_TupleHeading *
tupleHeadingDup(
    Ral_TupleHeading *srcHeading,
    int addAttrs)
{
    Ral_TupleHeading *dupHeading ;

    dupHeading = tupleHeadingNew(srcHeading->degree + addAttrs) ;
    if (tupleHeadingCopy(NULL, srcHeading->attrVector,
	srcHeading->attrVector + srcHeading->degree, dupHeading, 0) != TCL_OK) {
	tupleHeadingDelete(dupHeading) ;
	return NULL ;
    }

    return dupHeading ;
}

static Ral_Attribute *
tupleHeadingFindAttribute(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading,
    Tcl_Obj *attrName,
    int *indexPtr)
{
    Tcl_HashEntry *entry ;

    entry = Tcl_FindHashEntry(&heading->nameMap, (char *)attrName) ;
    if (entry) {
	int index ;

	index = (int)Tcl_GetHashValue(entry) ;
	if (indexPtr) {
	    *indexPtr = index ;
	}
	assert(index < heading->degree) ;
	return heading->attrVector + index ;
    } else if (interp) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "unknown attribute name, \"", Tcl_GetString(attrName), "\"",
	    NULL) ;
    }
    return NULL ;
}

static int
tupleHeadingEqual(
    Ral_TupleHeading *h1,
    Ral_TupleHeading *h2)
{
    Ral_Attribute *a1 ;
    Ral_Attribute *last ;

    if (h1 == h2) {
	return 1 ;
    }
    if (h1->degree != h2->degree) {
	return 0 ;
    }
    /*
     * Headings are equal if both the attribute names are equal and the
     * corresponding data types are the same. However, order does not
     * matter. Two headings can be equal even if the order in which the
     * attributes appear in the vector differs.  So iterate through the
     * vector of the first heading an look up the corresponding attribute
     * names in the other heading.
     */
    last = h1->attrVector + h1->degree ;
    for (a1 = h1->attrVector ; a1 != last ; ++a1) {
	Ral_Attribute *a2 ;

	a2 = tupleHeadingFindAttribute(NULL, h2, a1->name, NULL) ;
	if (!a2) {
	    return 0 ;
	}
	if (!attributeEqual(a1, a2)) {
	    return 0 ;
	}
    }

    return 1 ;
}

static int
tupleHeadingInsertName(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading,
    Tcl_Obj *attrName,
    int where)
{
    Tcl_HashEntry *entry ;
    int newPtr ;

    assert(where < heading->degree) ;
    entry = Tcl_CreateHashEntry(&heading->nameMap, (char *)attrName, &newPtr) ;
    /*
     * Check that there are no duplicate attribute names.
     */
    if (newPtr == 0) {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"duplicate attribute name, \"",
		Tcl_GetString(attrName), "\"", NULL) ;
	}
	return TCL_ERROR ;
    }
    Tcl_SetHashValue(entry, where) ;

    return TCL_OK ;
}

static int
tupleHeadingInsertAttribute(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading,
    Tcl_Obj *attrName,
    Tcl_Obj *dataType,
    int where)
{
    if (tupleHeadingInsertName(interp, heading, attrName, where) != TCL_OK ||
	attributeFromObjs(interp, heading->attrVector + where, attrName,
	dataType) != TCL_OK) {
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

static int
tupleHeadingInsertAttributeFromPair(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading,
    Tcl_Obj *pair,
    int where)
{
    int pairc ;
    Tcl_Obj **pairv ;
    int result ;

    if (Tcl_ListObjGetElements(interp, pair, &pairc, &pairv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (pairc == 1) {
	result = tupleHeadingInsertAttribute(interp, heading, *pairv,
	    NULL, where) ;
    } else if (pairc == 2) {
	result = tupleHeadingInsertAttribute(interp, heading, *pairv,
	    *(pairv + 1), where) ;
    } else {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"bad heading format, \"", Tcl_GetString(pair), "\"", NULL) ;
	}
	result = TCL_ERROR ;
    }

    return result ;
}

static int
tupleHeadingRenameAttribute(
    Tcl_Interp *interp,		/* interpreter for error reporting */
    Ral_TupleHeading *heading,  /* heading to which the attribute is added */
    Tcl_Obj *oldName,		/* current name of the attribute */
    Tcl_Obj *newName)		/* desired new name of the attribute */
{
    Tcl_HashEntry *oldEntry ;
    Tcl_HashEntry *newEntry ;
    Ral_Attribute *attr ;
    int newPtr ;
    int index ;

    /*
     * Find the old name.
     */
    oldEntry = Tcl_FindHashEntry(&heading->nameMap, (char *)oldName) ;
    if (oldEntry == NULL) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "unknown attribute name, \"", Tcl_GetString(oldName), "\"",
	    NULL) ;
	return TCL_ERROR ;
    }
    /*
     * Create an entry for the new name.
     */
    newEntry = Tcl_CreateHashEntry(&heading->nameMap, (char *)newName,
	&newPtr) ;
    if (newPtr == 0) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "duplicate attribute name, \"", Tcl_GetString(newName), "\"",
	    NULL) ;
	return TCL_ERROR ;
    }
    /*
     * Set the new hash entry to have an index value that is the same as the
     * old entry. Change the attribute heading to reflect the new name.
     */
    index = (int)Tcl_GetHashValue(oldEntry) ;
    assert(index < heading->degree) ;
    Tcl_DeleteHashEntry(oldEntry) ;
    Tcl_SetHashValue(newEntry, index) ;
    attr = heading->attrVector + index ;
    Tcl_DecrRefCount(attr->name) ;
    Tcl_IncrRefCount(attr->name = newName) ;

    return TCL_OK ;
}

static Ral_TupleHeading *
tupleHeadingNewFromString(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_TupleHeading *heading ;
    int i ;

    /*
     * Since the string representation of a Heading is a list of pairs,
     * we can use the list object functions to do the heavy lifting here.
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return NULL ;
    }
    heading = tupleHeadingNew(objc) ;
    /*
     * Iterate through the list adding each element as an attribute to
     * a newly created Heading.
     */
    for (i = 0 ; i < objc ; ++i) {
	if (tupleHeadingInsertAttributeFromPair(interp, heading, *objv++, i)
	    != TCL_OK) {
	    goto errorOut ;
	}
    }

    return heading ;

errorOut:
    tupleHeadingDelete(heading) ;
    return NULL ;
}

static Tcl_Obj *
tupleHeadingAttrsToList(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading)
{
    Tcl_Obj *headList ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;
    /*
     * Iterate through the Heading Attributes and add them to the
     * new list. We do this in index order so that the string that is
     * created does not appear to be random (i.e. in the order that
     * the hash table puts things in. Although ordering the external
     * representation in this way is not strictly necessary for correct
     * operation of a the header, it does represent a canonical form. The
     * object here is that if you convert a string into a Heading and
     * then back again, you will get the same string.  This is helpful
     * if the string representations are placed in files or if simple
     * string comparisons wish to be used.
     */
    headList = Tcl_NewListObj(0, NULL) ;
    last = heading->attrVector + heading->degree ;
    for (attr = heading->attrVector ; attr != last ; ++attr) {
	Tcl_Obj *pair ;
	/*
	 * Create a new list to contain the attribute name / data
	 * type pair.
	 */
	pair = Tcl_NewListObj(0, NULL) ;

	if (Tcl_ListObjAppendElement(interp, pair, attr->name) != TCL_OK) {
	    goto errorOut ;
	}
	switch (attr->attrType) {
	case Tcl_Type:
	    if (Tcl_ListObjAppendElement(interp, pair,
		Tcl_NewStringObj(attr->tclType->name, -1)) != TCL_OK) {
		goto errorOut ;
	    }
	    break ;

	case Tuple_Type:
	    if (Tcl_ListObjAppendElement(interp, pair,
		tupleHeadingObjNew(interp, attr->tupleHeading))
		!= TCL_OK) {
		goto errorOut ;
	    }
	    break ;

	case Relation_Type:
	    if (Tcl_ListObjAppendElement(interp, pair,
		relationHeadingObjNew(interp, attr->relationHeading))
		!= TCL_OK) {
		goto errorOut ;
	    }
	    break ;

	default:
	    Tcl_Panic("unknown attribute data type") ;
	}

	if (Tcl_ListObjAppendElement(interp, headList, pair) != TCL_OK) {
	    goto errorOut ;
	}
    }

    return headList ;

errorOut:
    Tcl_DecrRefCount(headList) ;
    return NULL ;
}

static Tcl_Obj *
tupleHeadingObjNew(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading)
{
    Tcl_Obj *pair[2] ;

    pair[1] = tupleHeadingAttrsToList(interp, heading) ;
    if (!pair[1]) {
	return NULL ;
    }
    pair[0] = Tcl_NewStringObj(Ral_TupleType.name, -1) ;

    return Tcl_NewListObj(2, pair) ;
}

/*
 * ======================================================================
 * Tuple Functions
 * ======================================================================
 */

static Ral_Tuple *
tupleNew(
    Ral_TupleHeading *heading)
{
    int nBytes ;
    Ral_Tuple *tuple ;

    nBytes = sizeof(*tuple) + heading->degree * sizeof(*tuple->values) ;
    tuple = (Ral_Tuple *)ckalloc(nBytes) ;
    memset(tuple, 0, nBytes) ;

    tupleHeadingReference(tuple->heading = heading) ;
    tuple->values = (Tcl_Obj **)(tuple + 1) ;

    return tuple ;
}

static void
tupleDelete(
    Ral_Tuple *tuple)
{
    if (tuple) {
	int degree ;
	Tcl_Obj **objPtr ;

	for (degree = tuple->heading->degree, objPtr = tuple->values ;
	    degree > 0 ; --degree, ++objPtr) {
	    if (*objPtr) {
		Tcl_DecrRefCount(*objPtr) ;
	    }
	}
	tupleHeadingUnreference(tuple->heading) ;

	ckfree((char *)tuple) ;
    }
}

static void
tupleCopyValues(
    Ral_Tuple *srcTuple,
    Ral_Tuple *destTuple)
{
    Tcl_Obj **srcValues = srcTuple->values ;
    Tcl_Obj **destValues = destTuple->values ;
    int degree = srcTuple->heading->degree ;

    for ( ; degree > 0 ; --degree) {
	Tcl_IncrRefCount(*destValues++ = *srcValues++) ;
    }
    return ;
}

static Ral_Tuple *
tupleDup(
    Ral_Tuple *srcTuple,
    int addAttrs)
{
    Ral_TupleHeading *srcHeading = srcTuple->heading ;
    Ral_Tuple *dupTuple ;
    Ral_TupleHeading *dupHeading ;

    /*
     * Duplicate the heading.
     */
    dupHeading = tupleHeadingDup(srcHeading, addAttrs) ;
    if (!dupHeading) {
	return NULL ;
    }
    dupTuple = tupleNew(dupHeading) ;
    /*
     * Copy the values to the new tuple.
     */
    tupleCopyValues(srcTuple, dupTuple) ;

    return dupTuple ;
}

static int
tupleEqual(
    Ral_Tuple *t1,
    Ral_Tuple *t2)
{
    Ral_TupleHeading *h1 = t1->heading ;
    Ral_TupleHeading *h2 = t2->heading ;
    Tcl_Obj **v1 = t1->values ;
    Tcl_Obj **v2 = t2->values ;
    int d1 = h1->degree ;
    int i1 ;
    int i2 ;
    Ral_Attribute *a1 ;
    Ral_Attribute *a2 ;

    if (t1 == t2) {
	return 1 ;
    }

    for (i1 = 0, a1 = h1->attrVector ; i1 < d1 ; ++i1, ++a1) {
	a2 = tupleHeadingFindAttribute(NULL, h2, a1->name, &i2) ;
	/*
	 * Compare the attributes and values.
	 */
	if (!(a2 && attributeEqual(a1, a2) && ObjsEqual(v1[i1], v2[i2]))) {
	    return 0 ;
	}
    }
    return 1 ;
}

static Tcl_Obj *
tupleObjNew(
    Ral_Tuple *tuple)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->typePtr = &Ral_TupleType ;
    objPtr->internalRep.otherValuePtr = tuple ;
    Tcl_InvalidateStringRep(objPtr) ;
    return objPtr ;
}

static int
tupleSetValuesFromString(
    Tcl_Interp *interp,
    Ral_Tuple *tuple,
    Tcl_Obj *nvList)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_TupleHeading *heading ;

    if (Tcl_ListObjGetElements(interp, nvList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc % 2 != 0) {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"list must have an even number of elements", NULL) ;
	}
	return TCL_ERROR ;
    }
    heading = tuple->heading ;
    if (elemc / 2 != heading->degree) {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"wrong number of attributes specified", NULL) ;
	}
	return TCL_ERROR ;
    }
    /*
     * Reorder the values to match the heading.
     */
    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	Tcl_HashEntry *entry ;
	int index ;
	Ral_Attribute *attr ;
	Tcl_Obj **v ;

	entry = Tcl_FindHashEntry(&heading->nameMap, (char *)*elemv) ;
	if (entry) {
	    index = (int)Tcl_GetHashValue(entry) ;
	    assert(index < heading->degree) ;
	} else {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "unknown attribute name, \"", Tcl_GetString(*elemv), "\"",
		    NULL) ;
	    }
	    return TCL_ERROR ;
	}

	v = tuple->values + index ;
	if (*v != NULL) {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "attribute, \"", Tcl_GetString(*elemv),
		    "\", assigned a value multiple times", NULL) ;
	    }
	    return TCL_ERROR ;
	}
	Tcl_IncrRefCount(*v = *(elemv + 1)) ;
	attr = heading->attrVector + index ;
	if (attributeConvertValue(interp, attr, *v) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }
    /*
     * At this point we assert that all the attributes are accounted for:
     * 1. There was an name / value pair for each attribute in the heading.
     * 2. No attribute is given a value more than once.
     * 3. All attribute names had to match something in the heading.
     */
    return TCL_OK ;
}

static Tcl_Obj *
tupleNameValueList(
    Tcl_Interp *interp,
    Ral_Tuple *tuple)
{
    Ral_TupleHeading *heading = tuple->heading ;
    Tcl_Obj **values = tuple->values ;
    Tcl_Obj *nvList ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;

    nvList = Tcl_NewListObj(0, NULL) ;
    last = heading->attrVector + heading->degree ;
    for (attr = heading->attrVector ; attr != last ; ++attr) {
	Tcl_Obj *convertResult ;

	if (Tcl_ListObjAppendElement(interp, nvList, attr->name) != TCL_OK ||
	    attributeValueToString(interp, attr, *values++, &convertResult)
		!= TCL_OK ||
	    Tcl_ListObjAppendElement(interp, nvList, convertResult) != TCL_OK) {
	    Tcl_DecrRefCount(nvList) ;
	    return NULL ;
	}
    }

    return nvList ;
}

static int
tupleCopyAttribute(
    Tcl_Interp *interp,
    Ral_Tuple *src,
    Tcl_Obj *attrName,
    Ral_Tuple *dest,
    int destIndex)
{
    Ral_TupleHeading *srcHeading = src->heading ;
    Tcl_Obj **srcValues = src->values ;
    Ral_TupleHeading *destHeading = dest->heading ;
    Tcl_Obj **destValues = dest->values ;
    Ral_Attribute *srcAttr ;
    int srcIndex ;

    srcAttr = tupleHeadingFindAttribute(interp, srcHeading, attrName,
	&srcIndex) ;
    if (!srcAttr) {
	return TCL_ERROR ;
    }
    if (tupleHeadingCopy(interp, srcAttr, srcAttr + 1, destHeading, destIndex)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    Tcl_IncrRefCount(destValues[destIndex] = srcValues[srcIndex]) ;

    return TCL_OK ;
}

static int
tupleObjUpdateValues(
    Tcl_Interp *interp,
    Tcl_Obj *tupleObj,
    Tcl_Obj *nvList)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;

    if (Tcl_ListObjGetElements(interp, nvList, &elemc, &elemv) != TCL_OK)
	return TCL_ERROR ;
    if (elemc % 2 != 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp),
	    "list must have an even number of elements", -1) ;
	return TCL_ERROR ;
    }

    if (Tcl_IsShared(tupleObj)) {
	Tcl_Panic("tupleObjUpdateValues called with a shared tuple") ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    for ( ; elemc > 0 ; elemc -= 2, elemv += 2) {
	Ral_Attribute *attr ;
	int attrIndex ;

	/*
	 * Find the entry name.
	 */
	attr = tupleHeadingFindAttribute(interp, heading, *elemv, &attrIndex) ;
	if (attr == NULL) {
	    return TCL_ERROR ;
	}
	/*
	 * Force the new value to be of the correct type.
	 */
	if (attributeConvertValue(interp, attr, elemv[1]) != TCL_OK) {
	    return TCL_ERROR ;
	}
	/*
	 * Replace the new value.
	 */
	Tcl_DecrRefCount(tuple->values[attrIndex]) ;
	Tcl_IncrRefCount(tuple->values[attrIndex] = elemv[1]) ;
    }

    Tcl_InvalidateStringRep(tupleObj) ;
    return TCL_OK ;
}

/*
 * ======================================================================
 * Functions to Support the Tcl type
 * ======================================================================
 */

static void
FreeTupleInternalRep(
    Tcl_Obj *objPtr)
{
    assert(objPtr->typePtr == &Ral_TupleType) ;
    tupleDelete(objPtr->internalRep.otherValuePtr) ;
    objPtr->typePtr = objPtr->internalRep.otherValuePtr = NULL ;
}

static void
DupTupleInternalRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    Ral_Tuple *srcTuple ;
    Ral_Tuple *dupTuple ;

    assert(srcPtr->typePtr == &Ral_TupleType) ;
    srcTuple = srcPtr->internalRep.otherValuePtr ;

    dupTuple = tupleDup(srcTuple, 0) ;
    if (dupTuple) {
	dupPtr->internalRep.otherValuePtr = dupTuple ;
	dupPtr->typePtr = &Ral_TupleType ;
    }
}

static void
UpdateStringOfTuple(
    Tcl_Obj *objPtr)
{
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Tcl_Obj *resultObj ;
    Tcl_Obj *valueList ;
    char *strRep ;
    int repLen ;

    tuple = objPtr->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    resultObj = tupleHeadingObjNew(NULL, heading) ;
    valueList = tupleNameValueList(NULL, tuple) ;
    Tcl_ListObjAppendElement(NULL, resultObj, valueList) ;

    strRep = Tcl_GetStringFromObj(resultObj, &repLen) ;
    objPtr->bytes = ckalloc(repLen + 1) ;
    objPtr->length = repLen ;
    memcpy(objPtr->bytes, strRep, repLen) ;
    *(objPtr->bytes + repLen) = '\0' ;

    Tcl_DecrRefCount(resultObj) ;
}

static int
SetTupleFromAny(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_TupleHeading *heading ;
    Ral_Tuple *tuple ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc != 3) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "badly formatted tuple, \"", Tcl_GetString(objPtr), "\"", NULL) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_TupleType.name, Tcl_GetString(*objv)) != 0) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "bad tuple type keyword: expected, \"", Ral_TupleType.name,
	    "\", but got, \"", Tcl_GetString(*objv), "\"", NULL) ;
	return TCL_ERROR ;
    }

    heading = tupleHeadingNewFromString(interp, *(objv + 1)) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    tuple = tupleNew(heading) ;
    if (tupleSetValuesFromString(interp, tuple, *(objv + 2)) != TCL_OK) {
	tupleDelete(tuple) ;
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
     * that will map to the same tuple and we want to force a new string rep to
     * be generated in order to obtain the canonical string form.
     */
    Tcl_InvalidateStringRep(objPtr) ;
    /*
     * Install the new internal representation.
     */
    objPtr->typePtr = &Ral_TupleType ;
    objPtr->internalRep.otherValuePtr = tuple ;

    return TCL_OK ;
}

/*
 * ======================================================================
 * Relation Identifier Functions
 * ======================================================================
 */

static void
relIdDtor(
    Ral_RelId *relId)
{
    if (relId->attrVector) {
	ckfree((char *)relId->attrVector) ;
	Tcl_DeleteHashTable(&relId->attrMap) ;
	relId->attrVector = NULL ;
	relId->attrCount = 0 ;
    }
}

static void
relIdCtor(
    Ral_RelId *relId,
    int nAttrs)
{
    relIdDtor(relId) ;

    if (nAttrs > 0) {
	relId->attrCount = nAttrs ;
	relId->attrVector = (int *)ckalloc(nAttrs *
	    sizeof(*relId->attrVector)) ;
	Tcl_InitHashTable(&relId->attrMap, TCL_STRING_KEYS) ;
    }
}

static int
relIdEqual(
    Ral_RelId *id1,
    Ral_RelId *id2)
{
    if (id1 == id2) {
	return 1 ;
    }
    if (id1->attrCount != id2->attrCount) {
	return 0 ;
    }
    /*
     * This works because the attribute index vectors are
     * always kept in sorted order.
     */
    return memcmp(id1->attrVector, id2->attrVector,
	id1->attrCount * sizeof(*id1->attrVector)) == 0 ;
}

/*
 * Is id1 a subset (perhaps improper subset) of id2.
 */
static int
relIdIsSubsetOf(
    Ral_RelId *id1,
    Ral_RelId *id2)
{
    if (id1 == id2) {
	return 1 ;
    }
    if (id1->attrCount > id2->attrCount) {
	return 0 ;
    }
    /*
     * Index vector is sorted.
     */
    return memcmp(id1->attrVector, id2->attrVector,
	id1->attrCount * sizeof(*id1->attrVector)) == 0 ;
}

static int
relIdSetFromObj(
    Tcl_Interp *interp,
    Ral_RelId *relId,
    Ral_TupleHeading *tupleHeading,
    Tcl_Obj *objPtr)
{
    int elemc ;
    Tcl_Obj **elemv ;
    int *attrVector ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv)) {
	return TCL_ERROR ;
    }
    if (elemc == 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp),
	    "identifier must have at least one attribute", -1) ;
	return TCL_ERROR ;
    }
    relIdCtor(relId, elemc) ;

    for (attrVector = relId->attrVector ; elemc > 0 ;
	--elemc, ++elemv, ++attrVector) {
	int index ;

	if (!tupleHeadingFindAttribute(interp, tupleHeading, *elemv, &index)) {
	    return TCL_ERROR ;
	}
	*attrVector = index ;
    }
    qsort(relId->attrVector, relId->attrCount, sizeof(*relId->attrVector),
	int_compare) ;

    return TCL_OK ;
}

static Tcl_Obj *
relIdAttrsToList(
    Tcl_Interp *interp,
    Ral_RelId *relId,
    Ral_TupleHeading *tupleHeading)
{
    Tcl_Obj *attrList ;
    int attrCount ;
    int *attrVector ;
    Ral_Attribute *attrSet ;

    attrList = Tcl_NewListObj(0, NULL) ;
    attrSet = tupleHeading->attrVector ;
    for (attrCount = relId->attrCount, attrVector = relId->attrVector ;
	attrCount > 0 ; --attrCount, ++attrVector) {
	if (Tcl_ListObjAppendElement(interp, attrList,
	    attrSet[*attrVector].name) != TCL_OK) {
	    Tcl_DecrRefCount(attrList) ;
	    return NULL ;
	}
    }

    return attrList ;
}

static void
relIdAppendAttrNames(
    Ral_RelId *relId,
    Ral_Attribute *tupleAttrs,
    Tcl_Obj *resultObj)
{
    int *attrVector ;
    int i ;

    attrVector = relId->attrVector ;
    for (i = 0 ; i < relId->attrCount ; ++i) {
	if (i != 0) {
	    Tcl_AppendStringsToObj(resultObj, ", ", NULL) ;
	}
	Tcl_AppendStringsToObj(resultObj,
	    Tcl_GetString(tupleAttrs[*attrVector++].name), NULL) ;
    }
}

static int
relIdIndexTuple(
    Tcl_Interp *interp,
    Ral_RelId *relId,
    Ral_Tuple *tuple,
    int where)
{
    Tcl_Obj **values = tuple->values ;
    int *attrVector = relId->attrVector ;
    int i = relId->attrCount ;
    const char *key ;
    Tcl_DString idKey ;
    Tcl_HashEntry *entry ;
    int newPtr ;
    int result = TCL_OK ;

    assert(relId->attrCount >= 1) ;
    /*
     * One identifying attribute is a common and easier to deal with
     * case to make the test worthwhile.
     */
    if (i == 1) {
	key = Tcl_GetString(values[*attrVector]) ;
    } else {
	Tcl_DStringInit(&idKey) ;
	for ( ; i > 0 ; --i) {
	    Tcl_DStringAppend(&idKey, Tcl_GetString(values[*attrVector++]),
		-1) ;
	}
	key = Tcl_DStringValue(&idKey) ;
    }
    entry = Tcl_CreateHashEntry(&relId->attrMap, key, &newPtr) ;
    /*
     * Check that there are no duplicate tuples.
     */
    if (newPtr == 0) {
	if (interp) {
	    Tcl_Obj *nvList = tupleNameValueList(NULL, tuple) ;
	    Tcl_Obj *resultObj ;

	    Tcl_ResetResult(interp) ;
	    resultObj = Tcl_GetObjResult(interp) ;
	    Tcl_AppendStringsToObj(resultObj, "tuple, \"",
		Tcl_GetString(nvList),
		"\", contains duplicate values for the identifier, \"", NULL) ;
	    relIdAppendAttrNames(relId, tuple->heading->attrVector, resultObj) ;
	    Tcl_AppendStringsToObj(resultObj, "\"", NULL) ;
	    Tcl_DecrRefCount(nvList) ;
	}
	result = TCL_ERROR ;
    }
    Tcl_SetHashValue(entry, where) ;

    if (relId->attrCount > 1) {
	Tcl_DStringFree(&idKey) ;
    }

    return result ;
}

static void
relIdRemoveIndex(
    Ral_RelId *relId,
    Ral_Tuple *tuple)
{
    Tcl_Obj **values = tuple->values ;
    int *attrVector = relId->attrVector ;
    int i = relId->attrCount ;
    const char *key ;
    Tcl_DString idKey ;
    Tcl_HashEntry *entry ;

    assert(relId->attrCount >= 1) ;
    /*
     * One identifying attribute is a common and easier to deal with
     * case to make the test worthwhile.
     */
    if (i == 1) {
	key = Tcl_GetString(values[*attrVector]) ;
    } else {
	Tcl_DStringInit(&idKey) ;
	for ( ; i > 0 ; --i) {
	    Tcl_DStringAppend(&idKey, Tcl_GetString(values[*attrVector++]),
		-1) ;
	}
	key = Tcl_DStringValue(&idKey) ;
    }
    entry = Tcl_FindHashEntry(&relId->attrMap, key) ;
    assert (entry != NULL) ;
    Tcl_DeleteHashEntry(entry) ;

    if (relId->attrCount > 1) {
	Tcl_DStringFree(&idKey) ;
    }
}

/*
 * ======================================================================
 * Relation Heading Functions
 * ======================================================================
 */

static Ral_RelationHeading *
relationHeadingNew(
    Ral_TupleHeading *tupleHeading,
    int nIds)
{
    Ral_RelationHeading *heading ;
    int nBytes ;
    Ral_RelId *idVector ;

    nBytes = sizeof(*heading) + nIds * sizeof(*heading->idVector) ;
    heading = (Ral_RelationHeading *)ckalloc(nBytes) ;
    memset(heading, 0, nBytes) ;

    heading->refCount = 0 ;
    tupleHeadingReference(heading->tupleHeading = tupleHeading) ;
    heading->idCount = nIds ;
    idVector = heading->idVector = (Ral_RelId *)(heading + 1) ;

    for ( ; nIds > 0 ; --nIds, ++idVector) {
	relIdCtor(idVector, 0) ;
    }

    return heading ;
}

static void
relationHeadingDelete(
    Ral_RelationHeading *heading)
{
    Ral_RelId *idVector = heading->idVector ;
    int nIds ;

    assert(heading->refCount <= 0) ;

    for (nIds = heading->idCount ; nIds > 0 ; --nIds, ++idVector) {
	relIdDtor(idVector) ;
    }
    tupleHeadingUnreference(heading->tupleHeading) ;
    ckfree((char *)heading) ;
}

static Ral_RelationHeading *
relationHeadingDup(
    Ral_RelationHeading *srcHeading,
    int addAttrs)
{
    Ral_TupleHeading *tupleHeading ;
    Ral_RelationHeading *heading ;
    int idCount = srcHeading->idCount ;
    Ral_RelId *srcIdVector = srcHeading->idVector ;
    Ral_RelId *destIdVector ;

    tupleHeading = tupleHeadingDup(srcHeading->tupleHeading, addAttrs) ;
    if (!tupleHeading) {
	return NULL ;
    }

    heading = relationHeadingNew(tupleHeading, idCount) ;
    for (destIdVector = heading->idVector ; idCount > 0 ;
	--idCount, ++srcIdVector, ++destIdVector) {
	relIdCtor(destIdVector, srcIdVector->attrCount) ;
	memcpy(destIdVector->attrVector, srcIdVector->attrVector,
	    srcIdVector->attrCount * sizeof(*srcIdVector->attrVector)) ;
    }

    return heading ;
}

static void
relationHeadingReference(
    Ral_RelationHeading *heading)
{
    ++heading->refCount ;
}

static void
relationHeadingUnreference(
    Ral_RelationHeading *heading)
{
    if (heading && --heading->refCount <= 0) {
	relationHeadingDelete(heading) ;
    }
}

static Ral_RelId *
relationHeadingFindId(
    Ral_RelationHeading *heading,
    Ral_RelId *id)
{
    int idCount ;
    Ral_RelId *idVector ;

    for (idCount = heading->idCount, idVector = heading->idVector ;
	idCount > 0 ; --idCount, ++idVector) {
	if (relIdEqual(idVector, id)) {
	    return idVector ;
	}
    }

    return NULL ;
}

/*
 * Returns 0 if the "key" is a subset of any of the id's between
 * [start, last).
 */
static int
relationHeadingCheckId(
    Ral_RelId *start,
    Ral_RelId *last,
    Ral_RelId *key)
{
    for ( ; start != last ; ++start) {
	if (relIdIsSubsetOf(key, start) || relIdIsSubsetOf(start, key)) {
	    return 0 ;
	}
    }
    return 1 ;
}

static int
relationHeadingEqual(
    Ral_RelationHeading *h1,
    Ral_RelationHeading *h2)
{
    int idCount ;
    Ral_RelId *idVector ;

    if (h1 == h2) {
	return 1 ;
    }
    if (!tupleHeadingEqual(h1->tupleHeading, h2->tupleHeading)) {
	return 0 ;
    }
    for (idCount = h1->idCount, idVector = h1->idVector ;
	idCount > 0 ; --idCount, ++idVector) {
	if (!relationHeadingFindId(h2, idVector)) {
	    return 0 ;
	}
    }

    return 1 ;
}

static Ral_RelationHeading *
relationHeadingNewFromObjs(
    Tcl_Interp *interp,
    Tcl_Obj *headingObj,
    Tcl_Obj *identObj)
{
    Ral_TupleHeading *tupleHeading ;
    Ral_RelationHeading *heading ;
    int idc ;
    Tcl_Obj **idv ;
    Ral_RelId *idVector ;

    tupleHeading = tupleHeadingNewFromString(interp, headingObj) ;
    if (!tupleHeading) {
	return NULL ;
    }
    if (Tcl_ListObjGetElements(interp, identObj, &idc, &idv) != TCL_OK) {
	tupleHeadingDelete(tupleHeading) ;
	return NULL ;
    }
    if (tupleHeading->degree > 0 && idc == 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp),
	    "relation must have at least one identifier", -1) ;
	tupleHeadingDelete(tupleHeading) ;
	return NULL ;
    }

    heading = relationHeadingNew(tupleHeading, idc) ;
    for (idVector = heading->idVector ; idc > 0 ; --idc, ++idv, ++idVector) {
	if (relIdSetFromObj(interp, idVector, tupleHeading, *idv) != TCL_OK) {
	    relationHeadingDelete(heading) ;
	    return NULL ;
	}
	if (!relationHeadingCheckId(heading->idVector, idVector, idVector)) {
	    if (interp) {
		Tcl_Obj *resultObj ;

		Tcl_ResetResult(interp) ;
		resultObj = Tcl_GetObjResult(interp) ;
		Tcl_AppendStringsToObj(resultObj, "identifier, \"", NULL) ;
		relIdAppendAttrNames(idVector, tupleHeading->attrVector,
		    resultObj) ;
		Tcl_AppendStringsToObj(resultObj,
	"\", is not irreducible with respect to least one other indentifier",
		NULL) ;
	    }
	    return NULL ;
	}
    }

    return heading ;
}

static Ral_RelationHeading *
relationHeadingNewFromString(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int elemc ;
    Tcl_Obj **elemv ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv) != TCL_OK) {
	return NULL ;
    }

    if (elemc != 2) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "bad relation heading format, \"", Tcl_GetString(objPtr),
	    "\"", NULL) ;
	return NULL ;
    }

    return relationHeadingNewFromObjs(interp, elemv[0], elemv[1]) ;
}

static Tcl_Obj *
relationHeadingIdsToList(
    Tcl_Interp *interp,
    Ral_RelationHeading *heading)
{
    Tcl_Obj *idObj ;
    int idCount ;
    Ral_RelId *idVector ;

    idObj = Tcl_NewListObj(0, NULL) ;
    for (idCount = heading->idCount, idVector = heading->idVector ;
	idCount > 0 ; --idCount, ++idVector) {
	Tcl_Obj *idAttrList ;

	idAttrList = relIdAttrsToList(interp, idVector, heading->tupleHeading) ;
	if (idAttrList == NULL || Tcl_ListObjAppendElement(interp, idObj,
	    idAttrList) != TCL_OK) {
	    Tcl_DecrRefCount(idObj) ;
	    return NULL ;
	}
    }

    return idObj ;
}

static Tcl_Obj *
relationHeadingObjNew(
    Tcl_Interp *interp,
    Ral_RelationHeading *heading)
{
    Tcl_Obj *headElems[2] ;
    Tcl_Obj *pair[2] ;
    int idCount ;
    Ral_RelId *idVector ;

    headElems[0] = tupleHeadingAttrsToList(interp, heading->tupleHeading) ;
    if (!headElems[0]) {
	return NULL ;
    }
    headElems[1] = relationHeadingIdsToList(interp, heading) ;
    if (!headElems[0]) {
	Tcl_DecrRefCount(headElems[0]) ;
	return NULL ;
    }

    pair[0] = Tcl_NewStringObj(Ral_RelationType.name, -1) ;
    pair[1] = Tcl_NewListObj(2, headElems) ;

    return Tcl_NewListObj(2, pair) ;
}

static int
relationHeadingIndexTuple(
    Tcl_Interp *interp,
    Ral_RelationHeading *heading,
    Ral_Tuple *tuple,
    int where)
{
    int i ;
    Ral_RelId *ids ;

    /*
     * Special test for the DUM and DEE relations.
     */
    if (tuple->heading->degree == 0 && where > 0) {
	if (interp) {
	    Tcl_Obj *nvList = tupleNameValueList(NULL, tuple) ;

	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"duplicate tuple, \"", Tcl_GetString(nvList), "\"", NULL) ;
	    Tcl_DecrRefCount(nvList) ;
	}
	return TCL_ERROR ;
    } else {
	for (i = 0, ids = heading->idVector ; i < heading->idCount ;
	    ++i, ++ids) {
	    if (relIdIndexTuple(interp, ids, tuple, where) != TCL_OK) {
		/*
		 * Need to back out any index that was successfully done.
		 */
		int j ;
		for (j = 0, ids = heading->idVector ; j < i ; ++j, ++ids) {
		    relIdRemoveIndex(ids, tuple) ;
		}
		return TCL_ERROR ;
	    }
	}
    }

    return TCL_OK ;
}

static void
relationHeadingRemoveTupleIndex(
    Ral_RelationHeading *heading,
    Ral_Tuple *tuple)
{
    Ral_RelId *ids ;
    Ral_RelId *last ;

    for (last = (ids = heading->idVector) + heading->idCount ; ids != last ;
	++ids) {
	relIdRemoveIndex(ids, tuple) ;
    }
}

static void
relationHeadingReindexTuple(
    Ral_RelationHeading *heading,
    Ral_Tuple *tuple,
    int where)
{
    Ral_RelId *ids ;
    Ral_RelId *last ;

    for (last = (ids = heading->idVector) + heading->idCount ; ids != last ;
	++ids) {
	relIdRemoveIndex(ids, tuple) ;
	relIdIndexTuple(NULL, ids, tuple, where) ;
    }
}

/*
 * ======================================================================
 * Relation Functions
 * ======================================================================
 */

static Ral_Relation *
relationNew(
    Ral_RelationHeading *heading)
{
    Ral_Relation *relation ;

    relation = (Ral_Relation *)ckalloc(sizeof(*relation)) ;
    memset(relation, 0, sizeof(*relation)) ;

    relationHeadingReference(relation->heading = heading) ;

    return relation ;
}

static void
relationDelete(
    Ral_Relation *relation)
{
    int c ;
    Tcl_Obj **t ;

    for (c = relation->cardinality, t = relation->tupleVector ; c > 0 ;
	--c, ++t) {
	Tcl_DecrRefCount(*t) ;
    }

    if (relation->tupleVector) {
	ckfree((char *)relation->tupleVector) ;
    }
    relationHeadingUnreference(relation->heading) ;
    ckfree((char *)relation) ;
}

static void
relationReserve(
    Ral_Relation *relation,
    int nTuples)
{
    int newAllocated ;
    Ral_Tuple **newVector ;

    if (relation->allocated - relation->cardinality > nTuples) {
	return ;
    }

    relation->allocated = relation->allocated + relation->allocated / 2 +
	nTuples ;
    relation->tupleVector = (Tcl_Obj **)ckrealloc(
	(char *)relation->tupleVector,
	relation->allocated * sizeof(*relation->tupleVector)) ;
}

static Ral_Relation *
relationDup(
    Ral_Relation *srcRelation,
    int addAttrs)
{
    Ral_RelationHeading *srcHeading = srcRelation->heading ;
    Ral_RelationHeading *dupHeading ;
    Ral_Relation *dupRelation ;
    Ral_TupleHeading *tupleHeading ;
    int c ;
    Tcl_Obj **s ;
    Tcl_Obj **d ;

    dupHeading = relationHeadingDup(srcHeading, addAttrs) ;
    if (!dupHeading) {
	return NULL ;
    }
    dupRelation = relationNew(dupHeading) ;
    relationReserve(dupRelation, srcRelation->cardinality) ;

    tupleHeading = dupHeading->tupleHeading ;
    d = dupRelation->tupleVector ;
    for (c = srcRelation->cardinality, s = srcRelation->tupleVector ; c > 0 ;
	--c, ++s) {
	Ral_Tuple *t ;

	if (Tcl_ConvertToType(NULL, *s, &Ral_TupleType) != TCL_OK) {
	    relationDelete(dupRelation) ;
	    return NULL ;
	}
	t = (*s)->internalRep.otherValuePtr ;
	if (relationHeadingIndexTuple(NULL, dupHeading, t,
	    dupRelation->cardinality) != TCL_OK) {
	    relationDelete(dupRelation) ;
	    return NULL ;
	}
	Tcl_IncrRefCount(*d++ = *s) ;
	++dupRelation->cardinality ;
    }

    return dupRelation ;
}

static Tcl_Obj *
relationObjNew(
    Ral_Relation *relation)
{
    Tcl_Obj *objPtr = Tcl_NewObj() ;
    objPtr->typePtr = &Ral_RelationType ;
    objPtr->internalRep.otherValuePtr = relation ;
    Tcl_InvalidateStringRep(objPtr) ;
    return objPtr ;
}

static int
relationInsertValue(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Tcl_Obj *nvList,
    int where)
{
    Ral_Tuple *tuple ;
    Tcl_Obj **tupleVector ;

    assert(where < relation->allocated) ;

    tuple = tupleNew(relation->heading->tupleHeading) ;
    if (tupleSetValuesFromString(interp, tuple, nvList) != TCL_OK ||
	relationHeadingIndexTuple(interp, relation->heading, tuple,
	    relation->cardinality) != TCL_OK) {
	tupleDelete(tuple) ;
	return TCL_ERROR ;
    }
    Tcl_IncrRefCount(relation->tupleVector[where] = tupleObjNew(tuple)) ;
    ++relation->cardinality ;

    return TCL_OK ;
}

static int
relationSetValuesFromString(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Tcl_Obj *tupleList)
{
    int elemc ;
    Tcl_Obj **elemv ;
    int i ;

    if (Tcl_ListObjGetElements(interp, tupleList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    relationReserve(relation, elemc) ;
    assert(relation->allocated >= relation->cardinality + elemc) ;

    for (i = 0 ; i < elemc ; ++i) {
	if (relationInsertValue(interp, relation, *elemv++, i) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

static Tcl_Obj *
relationNameValueList(
    Tcl_Interp *interp,
    Ral_Relation *relation)
{
    Ral_RelationHeading *heading ;
    Tcl_Obj **tupleVector ;
    Tcl_Obj **last ;
    Tcl_Obj *nvList ;

    heading = relation->heading ;

    nvList = Tcl_NewListObj(0, NULL) ;
    last = relation->tupleVector + relation->cardinality ;
    for (tupleVector = relation->tupleVector ; tupleVector != last ;
	++tupleVector) {
	Ral_Tuple *tuple ;
	Tcl_Obj *tupleNVList ;

	if (Tcl_ConvertToType(NULL, *tupleVector, &Ral_TupleType) != TCL_OK) {
	    Tcl_DecrRefCount(nvList) ;
	    return NULL ;
	}
	tuple = (*tupleVector)->internalRep.otherValuePtr ;

	tupleNVList = tupleNameValueList(interp, tuple) ;
	if (tupleNVList == NULL ||
	    Tcl_ListObjAppendElement(interp, nvList, tupleNVList) != TCL_OK) {
	    Tcl_DecrRefCount(nvList) ;
	    Tcl_DecrRefCount(tupleNVList) ;
	    return NULL ;
	}
    }

    return nvList ;
}

static int
relationDeleteTuple(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    int where)
{
    Tcl_Obj **tupleVector ;
    Ral_Tuple *tuple ;

    assert(where < relation->cardinality) ;
    tupleVector = relation->tupleVector + where ;

    if (Tcl_ConvertToType(interp, *tupleVector, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = (*tupleVector)->internalRep.otherValuePtr ;
    relationHeadingRemoveTupleIndex(relation->heading, tuple) ;
    Tcl_DecrRefCount(*tupleVector) ;
    --relation->cardinality ;

    for ( ; where < relation->cardinality ; ++where, ++tupleVector) {
	*tupleVector = *(tupleVector + 1) ;
	if (Tcl_ConvertToType(interp, *tupleVector, &Ral_TupleType) != TCL_OK) {
	    return TCL_ERROR ;
	}
	tuple = (*tupleVector)->internalRep.otherValuePtr ;
	relationHeadingReindexTuple(relation->heading, tuple, where) ;
    }

    return TCL_OK ;
}

static int
relationUpdateTuple(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    int where,
    Tcl_Obj *nvList)
{
    Tcl_Obj **tupleVector ;
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;

    assert(where < relation->cardinality) ;
    tupleVector = relation->tupleVector + where ;

    if (Tcl_ConvertToType(interp, *tupleVector, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }

    tupleObj = *tupleVector ;
    if (Tcl_IsShared(tupleObj)) {
	Tcl_Obj *dupObj ;

	dupObj = Tcl_DuplicateObj(tupleObj) ;
	Tcl_DecrRefCount(tupleObj) ;
	Tcl_IncrRefCount(*tupleVector = tupleObj = dupObj) ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    relationHeadingRemoveTupleIndex(relation->heading, tuple) ;
    if (tupleObjUpdateValues(interp, tupleObj, nvList) != TCL_OK ||
	relationHeadingIndexTuple(interp, relation->heading, tuple, where)
	    != TCL_OK) {
	return TCL_ERROR ;
    }

    return TCL_OK ;
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
    assert(objPtr->typePtr == &Ral_RelationType) ;
    relationDelete(objPtr->internalRep.otherValuePtr) ;
    objPtr->typePtr = objPtr->internalRep.otherValuePtr = NULL ;
}

static void
DupRelationInternalRep(
    Tcl_Obj *srcPtr,
    Tcl_Obj *dupPtr)
{
    Ral_Relation *srcRelation ;
    Ral_Relation *dupRelation ;

    assert(srcPtr->typePtr == &Ral_RelationType) ;
    srcRelation = srcPtr->internalRep.otherValuePtr ;

    dupRelation = relationDup(srcRelation, 0) ;
    if (dupRelation) {
	dupPtr->internalRep.otherValuePtr = dupRelation ;
	dupPtr->typePtr = &Ral_RelationType ;
    }
}

static void
UpdateStringOfRelation(
    Tcl_Obj *objPtr)
{
    Ral_Relation *relation ;
    Tcl_Obj *resultObj ;
    Tcl_Obj *valueList ;
    char *strRep ;
    int repLen ;

    relation = objPtr->internalRep.otherValuePtr ;

    resultObj = relationHeadingObjNew(NULL, relation->heading) ;
    valueList = relationNameValueList(NULL, relation) ;
    if (valueList == NULL) {
	return ;
    }

    Tcl_ListObjAppendElement(NULL, resultObj, valueList) ;

    strRep = Tcl_GetStringFromObj(resultObj, &repLen) ;
    objPtr->bytes = ckalloc(repLen + 1) ;
    objPtr->length = repLen ;
    memcpy(objPtr->bytes, strRep, repLen) ;
    *(objPtr->bytes + repLen) = '\0' ;

    Tcl_DecrRefCount(resultObj) ;
}

static int
SetRelationFromAny(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    int objc ;
    Tcl_Obj **objv ;
    Ral_RelationHeading *heading ;
    Ral_Relation *relation ;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (objc != 3) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "badly formatted relation, \"", Tcl_GetString(objPtr), "\"", NULL) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_RelationType.name, Tcl_GetString(*objv)) != 0) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "bad relation type keyword: expected, \"", Ral_TupleType.name,
	    "\", but got, \"", Tcl_GetString(*objv), "\"", NULL) ;
	return TCL_ERROR ;
    }

    heading = relationHeadingNewFromString(interp, *(objv + 1)) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    relation = relationNew(heading) ;
    if (relationSetValuesFromString(interp, relation, *(objv + 2)) != TCL_OK) {
	relationDelete(relation) ;
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
    objPtr->typePtr = &Ral_RelationType ;
    objPtr->internalRep.otherValuePtr = relation ;

    return TCL_OK ;
}

/*
 * ======================================================================
 * Tuple Sub-Command Functions
 * ======================================================================
 */
static int
TupleAssignCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Tcl_Obj **values ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    last = heading->attrVector + heading->degree ;
    for (attr = heading->attrVector, values = tuple->values ; attr != last ;
	++attr, ++values) {
	if (Tcl_ObjSetVar2(interp, attr->name, NULL, *values, TCL_LEAVE_ERR_MSG)
	    == NULL) {
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(heading->degree)) ;
    return TCL_OK ;
}

static int
TupleCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_TupleHeading *heading ;
    Ral_Tuple *tuple ;
    int result = TCL_ERROR ;

    /* tuple create heading name-value-list */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "heading name-value-list") ;
	return TCL_ERROR ;
    }

    heading = tupleHeadingNewFromString(interp, *(objv + 2)) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    tuple = tupleNew(heading) ;
    if (tupleSetValuesFromString(interp, tuple, *(objv + 3)) != TCL_OK) {
	tupleDelete(tuple) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, tupleObjNew(tuple)) ;
    return TCL_OK ;
}

static int
TupleDegreeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;

    /* tuple degree tupleValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    Tcl_SetObjResult(interp, Tcl_NewIntObj(tuple->heading->degree)) ;

    return TCL_OK ;
}

static int
TupleEliminateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Tcl_Obj **values ;
    int i ;
    Ral_TupleHeading *newHeading ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;
    Ral_Tuple *newTuple ;
    Tcl_Obj **newValues ;

    /* tuple eliminate tupleValue ?attr? ... */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue ?attr? ...") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;
    last = heading->attrVector + heading->degree ;

    objc -= 3 ;
    if (objc <= 0) {
	Tcl_SetObjResult(interp, tupleObj) ;
	return TCL_OK ;
    }
    objv += 3 ;
    /*
     * Check that attributes to eliminate actually belong to the tuple.
     */
    for (i = 0 ; i < objc ; ++i) {
	if (tupleHeadingFindAttribute(interp, heading, objv[i], NULL) == NULL) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"attribute, \"", Tcl_GetString(objv[i]),
		"\", is not a member of the tuple, \"",
		Tcl_GetString(tupleObj), "\"", NULL) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Build a new heading. It will have as many fewer attributes
     * as are listed with the command.
     */
    newHeading = tupleHeadingNew(heading->degree - objc) ;
    newTuple = tupleNew(newHeading) ;
    newValues = newTuple->values ;
    i = 0 ;
    for (attr = heading->attrVector, values = tuple->values ; attr != last ;
	++attr, ++values) {
	/*
	 * check if this attribute is to be included
	 */
	if (ObjFindInVect(objc, objv, attr->name) == NULL) {
	    /*
	     * Add the name to the heading of the new tuple.
	     */
	    assert(i < newHeading->degree) ;
	    if (tupleHeadingCopy(interp, attr, attr + 1, newHeading, i++)
		!= TCL_OK) {
		tupleDelete(newTuple) ;
		return TCL_ERROR ;
	    }
	    /*
	     * Add the value to the new tuple.
	     */
	    Tcl_IncrRefCount(*newValues++ = *values) ;
	}
    }

    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;
}

static int
TupleEqualCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *t1Obj ;
    Tcl_Obj *t2Obj ;

    /* tuple equal tuple1 tuple2 */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tuple1 tuple2") ;
	return TCL_ERROR ;
    }
    t1Obj = *(objv + 2) ;
    t2Obj = *(objv + 3) ;

    if (Tcl_ConvertToType(interp, t1Obj, &Ral_TupleType) != TCL_OK)
	return TCL_ERROR ;
    if (Tcl_ConvertToType(interp, t2Obj, &Ral_TupleType) != TCL_OK)
	return TCL_ERROR ;
    Tcl_SetObjResult(interp,
	Tcl_NewBooleanObj(tupleEqual(t1Obj->internalRep.otherValuePtr,
	t2Obj->internalRep.otherValuePtr))) ;

    return TCL_OK ;
}

static int
TupleExtendCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Tcl_Obj **values ;
    Ral_TupleHeading *newHeading ;
    Ral_Tuple *newTuple ;
    Tcl_Obj **newValues ;
    int i ;

    /* tuple extend tupleValue ?name-type-value ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleValue ?name-type-value ... ?") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    if (objc <= 0) {
	Tcl_SetObjResult(interp, tupleObj) ;
	return TCL_OK ;
    }
    objv += 3 ;
    /*
     * The heading for the new tuple is larger by the number of new
     * attributes given in the command.
     */
    newTuple = tupleDup(tuple, objc) ;
    if (newTuple == NULL) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "cannot duplicate tuple, \"", Tcl_GetString(tupleObj), "\"",
	    NULL) ;
	return TCL_ERROR ;
    }
    newHeading = newTuple->heading ;
    /*
     * Add the new attributes to the new tuple.  The new attributes are tacked
     * on at the end of the attributes that came from the original tuple.
     */
    i = heading->degree ;
    newValues = newTuple->values + i ;
    for ( ; objc > 0 ; --objc, ++objv, ++i) {
	int elemc ;
	Tcl_Obj **elemv ;

	if (Tcl_ListObjGetElements(interp, *objv, &elemc, &elemv) != TCL_OK) {
	    goto errorOut ;
	}
	if (elemc != 3) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"bad name-type-value format, \"", Tcl_GetString(*objv), "\"",
		NULL) ;
	    goto errorOut ;
	}
	if (tupleHeadingInsertAttribute(interp, newHeading, elemv[0], elemv[1],
	    i) != TCL_OK ||
	    attributeConvertValue(interp, newHeading->attrVector + i, elemv[2])
	    != TCL_OK) {
	    goto errorOut ;
	}
	Tcl_IncrRefCount(*newValues++ = elemv[2]) ;
    }

    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut:
    tupleDelete(newTuple) ;
    return TCL_ERROR ;
}

static int
TupleExtractCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Ral_Attribute *attr ;
    int attrIndex ;
    Tcl_Obj *resultObj ;

    /* tuple extract tupleValue attr ?...? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue attr ?...?") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    objv += 3 ;
    if (objc < 2) {
	if (tupleHeadingFindAttribute(interp, heading, *objv, &attrIndex)
	    == NULL) {
	    return TCL_ERROR ;
	}
	resultObj = tuple->values[attrIndex] ;
    } else {
	resultObj = Tcl_NewListObj(0, NULL) ;
	while (objc-- > 0) {
	    if (tupleHeadingFindAttribute(interp, heading, *objv++, &attrIndex)
		    == NULL ||
		Tcl_ListObjAppendElement(interp, resultObj,
		    tuple->values[attrIndex]) != TCL_OK) {
		goto errorOut ;
	    }
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(resultObj) ;
    return TCL_ERROR ;
}

static int
TupleGetCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;
    Tcl_Obj **values ;
    Tcl_Obj *resultObj ;

    /* tuple get tupleValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;
    last = heading->attrVector + heading->degree ;
    resultObj = Tcl_NewListObj(0, NULL) ;

    for (attr = heading->attrVector, values = tuple->values ; attr != last ;
	++attr, ++values) {
	if (Tcl_ListObjAppendElement(interp, resultObj, attr->name) != TCL_OK ||
	    Tcl_ListObjAppendElement(interp, resultObj, *values) != TCL_OK) {
	    Tcl_DecrRefCount(resultObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

static int
TupleProjectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Ral_Tuple *newTuple ;
    Ral_TupleHeading *newHeading ;
    int index ;

    /* tuple project tupleValue ?attr? ... */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue ?attr? ...") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    objv += 3 ;

    newHeading = tupleHeadingNew(objc) ;
    newTuple = tupleNew(newHeading) ;
    for (index = 0  ; index < objc ; ++index, ++objv) {
	if (tupleCopyAttribute(interp, tuple, *objv, newTuple, index)
	    != TCL_OK) {
	    goto errorOut ;
	}
    }

    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut:
    tupleDelete(newTuple) ;
    return TCL_ERROR ;
}

static int
TupleRenameCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Ral_Tuple *newTuple ;

    /* tuple rename tupleValue ?oldname newname ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleValue ?oldname newname ... ?") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "oldname / newname arguments must come in pairs", NULL) ;
	return TCL_ERROR ;
    }

    newTuple = tupleDup(tuple, 0) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	if (tupleHeadingRenameAttribute(interp, newTuple->heading,
	    objv[0], objv[1]) != TCL_OK) {
	    tupleDelete(newTuple) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;
}

static int
TupleTypeofCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;

    /* tuple typeof tupleValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK)
	return TCL_ERROR ;
    tuple = tupleObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, tupleHeadingObjNew(interp, tuple->heading)) ;
    return TCL_OK ;
}

static int
TupleUnwrapCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Tcl_Obj **values ;
    Tcl_Obj *tupleAttrObj ;
    Ral_Attribute *tupleAttr ;
    int tupleAttrIndex ;
    Tcl_Obj *tupleAttrValue ;
    Ral_Tuple *unTuple ;
    Ral_TupleHeading *newHeading ;
    Ral_Tuple *newTuple ;
    Tcl_Obj **newValues ;
    int newIndex ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;

    /* tuple unwrap tupleValue tupleAttribute */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue tupleAttribute") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;
    values = tuple->values ;

    tupleAttrObj = *(objv + 3) ;
    tupleAttr = tupleHeadingFindAttribute(interp, heading, tupleAttrObj,
	&tupleAttrIndex) ;
    if (tupleAttr == NULL) {
	return TCL_ERROR ;
    }
    if (tupleAttr->attrType != Tuple_Type) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "attribute, \"",
	    Tcl_GetString(tupleAttrObj), "\", is not of type Tuple", NULL) ;
	return TCL_ERROR ;
    }
    tupleAttrValue = values[tupleAttrIndex] ;
    if (Tcl_ConvertToType(interp, tupleAttrValue, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    unTuple = tupleAttrValue->internalRep.otherValuePtr ;
    /*
     * The new tuple contain all the attributes of the old one minus the
     * attribute that is being unwrapped plus all the attributes contained in
     * the tuple to be unwrapped.
     */
    newHeading = tupleHeadingNew(tuple->heading->degree - 1 +
	unTuple->heading->degree) ;
    newTuple = tupleNew(newHeading) ;
    newValues = newTuple->values ;

    newIndex = 0 ;
    last = heading->attrVector + heading->degree ;
    for (attr = heading->attrVector ; attr != last ; ++attr) {
	    /*
	     * Check if this attribute needs to be unwrapped.
	     * Since only one one attribute will be found that needs
	     * unwrapping, avoid the Tcl_GetString() if we have already
	     * done the unwrapping.
	     */
	if (attr == tupleAttr) {
	    Ral_TupleHeading *unHeading ;
	    Ral_Attribute *unAttr ;
	    Ral_Attribute *unLast ;
	    int i ;
	    Tcl_Obj **unValues ;
	    /*
	     * Found attribute that matches the one to be unwrapped.
	     * Add all the wrapped attributes to the unwrapped heading.
	     */
	    unHeading = unTuple->heading ;
	    unLast = unHeading->attrVector + unHeading->degree ;
	    unAttr = unHeading->attrVector ;
	    if (tupleHeadingCopy(interp, unAttr, unLast, newHeading, newIndex)
		!= TCL_OK) {
		goto errorOut ;
	    }
	    newIndex += unHeading->degree ;
	    /*
	     * Append the wrapped values to the unwrapped values.
	     */
	    unValues = unTuple->values ;
	    for (i = unHeading->degree ; i > 0 ; --i) {
		Tcl_IncrRefCount(*newValues++ = *unValues++) ;
	    }
	} else {
	    /*
	     * Otherwise just add to the new tuple
	     */
	    if (tupleHeadingCopy(interp, attr, attr + 1, newHeading, newIndex++)
		!= TCL_OK) {
		goto errorOut ;
	    }
	    Tcl_IncrRefCount(*newValues++ = *values++) ;
	}
    }

    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut:
    tupleDelete(newTuple) ;
    return TCL_ERROR ;
}

static int
TupleUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;

    /* tuple update tupleVar name-value-list */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue name-value-list") ;
	return TCL_ERROR ;
    }

    tupleObj = Tcl_ObjGetVar2(interp, objv[2], NULL, TCL_LEAVE_ERR_MSG) ;
    if (tupleObj == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }

    if (Tcl_IsShared(tupleObj)) {
	Tcl_Obj *dupObj ;

	dupObj = Tcl_DuplicateObj(tupleObj) ;
	tupleObj = Tcl_ObjSetVar2(interp, objv[2], NULL, dupObj,
	    TCL_LEAVE_ERR_MSG) ;
	if (tupleObj == NULL) {
	    Tcl_DecrRefCount(dupObj) ;
	    return TCL_ERROR ;
	}
    }

    if (tupleObjUpdateValues(interp, tupleObj, objv[3]) != TCL_OK) {
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, tupleObj) ;
    return TCL_OK ;
}

static int
TupleWrapCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Tcl_Obj *newAttr ;
    Tcl_Obj *oldAttrList ;
    Ral_Tuple *tuple ;
    Ral_TupleHeading *heading ;
    Tcl_Obj **values ;
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_Tuple *wrapTuple ;
    Ral_TupleHeading *wrapHeading ;
    Ral_Tuple *newTuple ;
    Ral_TupleHeading *newHeading ;
    int i ;
    Tcl_Obj **e ;
    Tcl_Obj *wrapTupleObj ;
    Ral_Attribute *attr ;
    Ral_Attribute *last ;

    /* tuple wrap tupleValue newAttr oldAttrList */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue newAttr oldAttrList") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    newAttr = *(objv + 3) ;
    oldAttrList = *(objv + 4) ;

    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_ListObjGetElements(interp, oldAttrList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }

    tuple = tupleObj->internalRep.otherValuePtr ;
    heading = tuple->heading ;

    /*
     * The wrapped tuple will have the number of attributes specified.
     */
    wrapHeading = tupleHeadingNew(elemc) ;
    wrapTuple = tupleNew(wrapHeading) ;
    for (i = 0, e = elemv ; i < elemc ; ++i, ++e) {
	if (tupleCopyAttribute(interp, tuple, *e, wrapTuple, i) != TCL_OK) {
	    goto errorOut ;
	}
    }
    /*
     * Compose the subtuple as an object.
     * Later it is added to the newly created tuple.
     */
    wrapTupleObj = tupleObjNew(wrapTuple) ;
    /*
     * The newly created tuple has the same number of attributes as the
     * old tuple minus the number that are to be wrapped plus one for
     * the new tuple attribute.
     */
    newHeading = tupleHeadingNew(heading->degree - elemc + 1) ;
    newTuple = tupleNew(newHeading) ;

    i = 0 ;
    last = heading->attrVector + heading->degree ;
    for (attr = heading->attrVector, values = tuple->values ; attr != last ;
	++attr, ++values) {
	/*
	 * Only add the ones that are NOT in the old attribute list.
	 */
	if (ObjFindInVect(elemc, elemv, attr->name) == NULL) {
	    if (tupleHeadingCopy(interp, attr, attr + 1, newHeading, i)
		!= TCL_OK) {
		goto errorOut2 ;
	    }
	    Tcl_IncrRefCount(newTuple->values[i] = *values) ;
	    ++i ;
	}
    }
    /*
     * Add the wrapped tuple. First check for a duplicated name.
     */
    if (tupleHeadingInsertName(interp, newHeading, newAttr, i) != TCL_OK) {
	goto errorOut2 ;
    }
    attributeCtorTupleType(newHeading->attrVector + i, newAttr, wrapHeading) ;
    Tcl_IncrRefCount(newTuple->values[i] = wrapTupleObj) ;

    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut2:
    tupleDelete(newTuple) ;
errorOut:
    Tcl_DecrRefCount(wrapTupleObj) ;
    return TCL_ERROR ;
}

/*
 * ======================================================================
 * Tuple Ensemble Command Function
 * ======================================================================
 */
static int
tupleCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    enum TupleSubCmds {
	TUPLE_ASSIGN,
	TUPLE_CREATE,
	TUPLE_DEGREE,
	TUPLE_ELIMINATE,
	TUPLE_EQUAL,
	TUPLE_EXTEND,
	TUPLE_EXTRACT,
	TUPLE_GET,
	TUPLE_PROJECT,
	TUPLE_RENAME,
	TUPLE_TYPEOF,
	TUPLE_UNWRAP,
	TUPLE_UPDATE,
	TUPLE_WRAP
    } ;
    static const char *subCmds[] = {
	"assign",
	"create",
	"degree",
	"eliminate",
	"equal",
	"extend",
	"extract",
	"get",
	"project",
	"rename",
	"typeof",
	"unwrap",
	"update",
	"wrap",
	NULL
    } ;

    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObj(interp, *(objv + 1), subCmds, "subcommand", 0,
	&index) != TCL_OK) {
	return TCL_ERROR ;
    }

    switch ((enum TupleSubCmds)index) {
    case TUPLE_ASSIGN:
	return TupleAssignCmd(interp, objc, objv) ;

    case TUPLE_CREATE:
	return TupleCreateCmd(interp, objc, objv) ;

    case TUPLE_DEGREE:
	return TupleDegreeCmd(interp, objc, objv) ;

    case TUPLE_ELIMINATE:
	return TupleEliminateCmd(interp, objc, objv) ;

    case TUPLE_EQUAL:
	return TupleEqualCmd(interp, objc, objv) ;

    case TUPLE_EXTEND:
	return TupleExtendCmd(interp, objc, objv) ;

    case TUPLE_EXTRACT:
	return TupleExtractCmd(interp, objc, objv) ;

    case TUPLE_GET:
	return TupleGetCmd(interp, objc, objv) ;

    case TUPLE_PROJECT:
	return TupleProjectCmd(interp, objc, objv) ;

    case TUPLE_RENAME:
	return TupleRenameCmd(interp, objc, objv) ;

    case TUPLE_TYPEOF:
	return TupleTypeofCmd(interp, objc, objv) ;

    case TUPLE_UNWRAP:
	return TupleUnwrapCmd(interp, objc, objv) ;

    case TUPLE_UPDATE:
	return TupleUpdateCmd(interp, objc, objv) ;

    case TUPLE_WRAP:
	return TupleWrapCmd(interp, objc, objv) ;

    default:
	Tcl_Panic("unexpected tuple subcommand value") ;
    }

    /*
     * NOT REACHED
     */
    return TCL_ERROR ;
}

/*
 * ======================================================================
 * Relvar Mapping Functions
 * ======================================================================
 */

static Ral_Relation *
relvarFind(
    Tcl_Interp *interp,
    Tcl_Obj *relvarName)
{
    Tcl_HashEntry *entry ;
    Ral_Relation *relation = NULL ;

    entry = Tcl_FindHashEntry(&relvarMap, (char *)relvarName) ;
    if (entry) {
	Tcl_Obj *relObj = (Tcl_Obj *)Tcl_GetHashValue(entry) ;
	if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) == TCL_OK) {
	    relation = relObj->internalRep.otherValuePtr ;
	}
    } else {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"unknown relvar, \"", Tcl_GetString(relvarName), "\"", NULL) ;
	}
    }

    return relation ;
}

/*
 * ======================================================================
 * Relvar Sub-Command Functions
 * ======================================================================
 */

static int
RelvarAssignCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_HashEntry *entry ;
    Tcl_Obj *relVarObj ;
    Tcl_Obj *relValueObj ;
    Ral_Relation *relvarRelation ;
    Ral_Relation *relvalRelation ;

    /* relvar assign relvarName relationValue */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName relationValue") ;
	return TCL_ERROR ;
    }

    entry = Tcl_FindHashEntry(&relvarMap, (char *)objv[2]) ;
    if (!entry) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "unknown relvar, \"", Tcl_GetString(objv[2]), "\"", NULL) ;
	return TCL_ERROR ;
    }
    relVarObj = Tcl_GetHashValue(entry) ;
    relValueObj = objv[3] ;

    if (Tcl_ConvertToType(interp, relVarObj, &Ral_RelationType) != TCL_OK ||
	Tcl_ConvertToType(interp, relValueObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }

    relvarRelation = relVarObj->internalRep.otherValuePtr ;
    relvalRelation = relValueObj->internalRep.otherValuePtr ;
    /*
     * Can only assign something of the same type.
     */
    if (!relationHeadingEqual(relvarRelation->heading,
	relvalRelation->heading)) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "value assigned to relvar must be of the same type", -1) ;
	return TCL_ERROR ;
    }

    Tcl_DecrRefCount(relVarObj) ;
    Tcl_IncrRefCount(relValueObj) ;
    Tcl_SetHashValue(entry, relValueObj) ;

    Tcl_SetObjResult(interp, relValueObj) ;
    return TCL_OK ;
}

static int
RelvarCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relvarName ;
    Tcl_HashEntry *entry ;
    int newPtr ;
    Ral_RelationHeading *heading ;
    Ral_Relation *relation ;
    int i ;
    Tcl_Obj *relObj ;

    /* relvar create relvarName heading identifiers ?name-value-list ...? */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName heading identifers ?name-value-list ..?") ;
	return TCL_ERROR ;
    }

    relvarName = objv[2] ;

    heading = relationHeadingNewFromObjs(interp, objv[3], objv[4]) ;
    if (!heading) {
	return TCL_ERROR ;
    }
    relation = relationNew(heading) ;

    objc -= 5 ;
    objv += 5 ;

    relationReserve(relation, objc) ;
    for (i = 0 ; i < objc ; ++i) {
	if (relationInsertValue(interp, relation, *objv++, i) != TCL_OK) {
	    relationDelete(relation) ;
	    return TCL_ERROR ;
	}
    }

    relObj = relationObjNew(relation) ;
    entry = Tcl_CreateHashEntry(&relvarMap, (char *)relvarName, &newPtr) ;
    if (newPtr == 0) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "relvar, \"", Tcl_GetString(relvarName), "\" already exists",
	    NULL) ;
	Tcl_DecrRefCount(relObj) ;
	return TCL_ERROR ;
    }
    /*
     * Increment the reference count since we are also storing a pointer
     * to the object in the relvar hash table.
     */
    Tcl_IncrRefCount(relObj) ;
    Tcl_SetHashValue(entry, relObj) ;

    Tcl_SetObjResult(interp, relObj) ;

    return TCL_OK ;
}

static int
RelvarDeleteCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_Relation *relation ;
    Tcl_Obj *tupleNameObj ;
    Tcl_Obj *exprObj ;
    int index ;
    int result = TCL_OK ;

    /* relvar delete relvarName tupleVarName expr */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName tupleVarName expr") ;
	return TCL_ERROR ;
    }

    relation = relvarFind(interp, objv[2]) ;
    if (!relation) {
	return TCL_ERROR ;
    }
    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

    for (index = 0 ; index < relation->cardinality ; ) {
	int boolValue ;

	if (Tcl_ObjSetVar2(interp, tupleNameObj, NULL,
	    relation->tupleVector[index], TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR ;
	    break ;
	}
	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
	if (boolValue) {
	    if (relationDeleteTuple(interp, relation, index) != TCL_OK) {
		result = TCL_ERROR ;
		break ;
	    }
	} else {
	    ++index ;
	}
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    return TCL_OK ;
}

static int
RelvarDestroyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_HashEntry *entry ;
    Tcl_Obj *relvarObj ;

    /* relvar destroy relvarName */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName") ;
	return TCL_ERROR ;
    }

    entry = Tcl_FindHashEntry(&relvarMap, (char *)objv[2]) ;
    if (entry) {
	relvarObj = Tcl_GetHashValue(entry) ;
	assert(relvarObj != NULL) ;
	Tcl_DecrRefCount(relvarObj) ;
	Tcl_DeleteHashEntry(entry) ;
	Tcl_UnsetVar(interp, Tcl_GetString(objv[2]), 0) ;
    } else {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "relvar, \"", Tcl_GetString(objv[2]), "\" does not exist",
	    NULL) ;
	return TCL_ERROR ;
    }

    return TCL_OK ;
}

static int
RelvarDumpCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    static const int bodyDump = 1 ;
    static const int schemaDump = 2 ;
    static const char *cmdArgs[] = {
	"-body",
	"-schema",
	NULL
    } ;

    int dumpType ;
    Tcl_Obj *patternObj ;
    Tcl_Obj *scriptObj ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;
    int index ;

    /* relvar dump ?-schema | -body? ?pattern? */
    if (objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "?-schema | -body? ?pattern?") ;
	return TCL_ERROR ;
    }

    if (objc == 2) {
	/*
	 * By default we dump everything for all relvars.
	 */
	dumpType = schemaDump | bodyDump ;
	patternObj = NULL ;
    } else if (objc == 3) {
	const char *arg2 = Tcl_GetString(objv[2]) ;

	if (*arg2 == '-') {
	    if (Tcl_GetIndexFromObj(interp, objv[2], cmdArgs,
		"dump type switch", 0, &index) != TCL_OK) {
		return TCL_ERROR ;
	    }
	    dumpType = 1 << index ;
	    patternObj = NULL ;
	} else {
	    dumpType = schemaDump | bodyDump ;
	    patternObj = objv[2] ;
	}
    } else {
	if (Tcl_GetIndexFromObj(interp, objv[2], cmdArgs, "dump type switch", 0,
	    &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	dumpType = 1 << index ;
	patternObj = objv[3] ;
    }

    scriptObj = Tcl_NewStringObj("package require ral\n", -1) ;

    /*
     * First emit a set of "relvar create" statements.
     */
    if (dumpType & schemaDump) {
	for (entry = Tcl_FirstHashEntry(&relvarMap, &search) ; entry ;
	    entry = Tcl_NextHashEntry(&search)) {
	    Tcl_Obj *relvarName ;
	    Tcl_Obj *relObj ;
	    Ral_Relation *relation ;
	    Ral_RelationHeading *heading ;
	    Tcl_Obj *headObj ;
	    Tcl_Obj *idObj ;

	    relvarName = (Tcl_Obj *)Tcl_GetHashKey(&relvarMap, entry) ;
	    if (patternObj &&
		!Tcl_StringMatch(Tcl_GetString(relvarName),
		    Tcl_GetString(patternObj))) {
		continue ;
	    }

	    relObj = Tcl_GetHashValue(entry) ;
	    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType)
		!= TCL_OK) {
		goto errorOut ;
	    }
	    relation = relObj->internalRep.otherValuePtr ;
	    heading = relation->heading ;

	    headObj = tupleHeadingAttrsToList(interp, heading->tupleHeading) ;
	    if (!headObj) {
		goto errorOut ;
	    }
	    idObj = relationHeadingIdsToList(interp, heading) ;
	    if (!idObj) {
		Tcl_DecrRefCount(headObj) ;
		goto errorOut ;
	    }

	    Tcl_AppendStringsToObj(scriptObj,
		"::ral::relvar create ",
		Tcl_GetString(relvarName),
		" {",
		Tcl_GetString(headObj),
		"} {",
		Tcl_GetString(idObj),
		"}\n",
		NULL) ;
	    Tcl_DecrRefCount(headObj) ;
	    Tcl_DecrRefCount(idObj) ;
	}
    }

    /*
     * Next emit the necessary "relvar insert" statements to populate
     * the schema.
     */
    if (dumpType & bodyDump) {
	for (entry = Tcl_FirstHashEntry(&relvarMap, &search) ; entry ;
	    entry = Tcl_NextHashEntry(&search)) {
	    Tcl_Obj *relvarName ;
	    Tcl_Obj *relObj ;
	    Ral_Relation *relation ;
	    int card ;
	    Tcl_Obj **tupleVector ;

	    relvarName = (Tcl_Obj *)Tcl_GetHashKey(&relvarMap, entry) ;
	    if (patternObj &&
		!Tcl_StringMatch(Tcl_GetString(relvarName),
		    Tcl_GetString(patternObj))) {
		continue ;
	    }

	    relObj = Tcl_GetHashValue(entry) ;
	    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType)
		!= TCL_OK) {
		goto errorOut ;
	    }
	    relation = relObj->internalRep.otherValuePtr ;
	    if (relation->cardinality == 0) {
		continue ;
	    }

	    Tcl_AppendStringsToObj(scriptObj,
		"::ral::relvar insert ",
		Tcl_GetString(relvarName),
		NULL) ;

	    for (card = relation->cardinality,
		tupleVector = relation->tupleVector ; card > 0 ;
		--card, ++tupleVector) {
		Ral_Tuple *tuple ;
		Tcl_Obj *tupleNVList ;

		if (Tcl_ConvertToType(interp, *tupleVector, &Ral_TupleType)
		    != TCL_OK) {
		    goto errorOut ;
		}
		tuple = (*tupleVector)->internalRep.otherValuePtr ;
		tupleNVList = tupleNameValueList(interp, tuple) ;
		if (tupleNVList) {
		    Tcl_AppendStringsToObj(scriptObj,
			" {", Tcl_GetString(tupleNVList), "}", NULL) ;
		    Tcl_DecrRefCount(tupleNVList) ;
		} else {
		    goto errorOut ;
		}
	    }
	    Tcl_AppendStringsToObj(scriptObj, "\n", NULL) ;
	}
    }

    Tcl_SetObjResult(interp, scriptObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(scriptObj) ;
    return TCL_ERROR ;
}

static int
RelvarGetCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_HashEntry *entry ;

    /* relvar get relvarName */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName") ;
	return TCL_ERROR ;
    }

    entry = Tcl_FindHashEntry(&relvarMap, (char *)objv[2]) ;
    if (!entry) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "unknown relvar, \"", Tcl_GetString(objv[2]), "\"", NULL) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Tcl_GetHashValue(entry)) ;
    return TCL_OK ;
}

static int
RelvarInsertCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_Relation *relation ;
    int where ;

    /* relvar insert relvarName ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relation = relvarFind(interp, objv[2]) ;
    if (!relation) {
	return TCL_ERROR ;
    }

    objc -= 3 ;
    objv += 3 ;

    relationReserve(relation, objc) ;
    for (where = relation->cardinality ; objc > 0 ; --objc, ++where, ++objv) {
	if (relationInsertValue(interp, relation, *objv, where) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

static int
RelvarUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_Relation *relation ;
    Tcl_Obj *tupleNameObj ;
    Tcl_Obj *exprObj ;
    int index ;
    int result = TCL_OK ;

    /* relvar update relvarName tupleVarName expr nameValueList */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName tupleVarName expr nameValueList") ;
	return TCL_ERROR ;
    }

    relation = relvarFind(interp, objv[2]) ;
    if (!relation) {
	return TCL_ERROR ;
    }
    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

    for (index = 0 ; index < relation->cardinality ; ++index) {
	int boolValue ;

	if (Tcl_ObjSetVar2(interp, tupleNameObj, NULL,
	    relation->tupleVector[index], TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR ;
	    break ;
	}
	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
	if (boolValue &&
	    relationUpdateTuple(interp, relation, index, objv[5]) != TCL_OK) {
	    result = TCL_ERROR ;
	    break ;
	}
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    return result ;
}

/*
 * ======================================================================
 * Relvar Ensemble Command Function
 * ======================================================================
 */

static int
relvarCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    enum RelvarSubCmds {
	RELVAR_ASSIGN,
	RELVAR_CREATE,
	RELVAR_DELETE,
	RELVAR_DESTROY,
	RELVAR_DUMP,
	RELVAR_GET,
	RELVAR_INSERT,
	RELVAR_UPDATE
    } ;
    static const char *subCmds[] = {
	"assign",
	"create",
	"delete",
	"destroy",
	"dump",
	"get",
	"insert",
	"update",
	NULL
    } ;

    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObj(interp, *(objv + 1), subCmds, "subcommand", 0,
	&index) != TCL_OK) {
	return TCL_ERROR ;
    }

    switch ((enum RelvarSubCmds)index) {
    case RELVAR_ASSIGN:
	return RelvarAssignCmd(interp, objc, objv) ;

    case RELVAR_CREATE:
	return RelvarCreateCmd(interp, objc, objv) ;

    case RELVAR_DELETE:
	return RelvarDeleteCmd(interp, objc, objv) ;

    case RELVAR_DESTROY:
	return RelvarDestroyCmd(interp, objc, objv) ;

    case RELVAR_DUMP:
	return RelvarDumpCmd(interp, objc, objv) ;

    case RELVAR_GET:
	return RelvarGetCmd(interp, objc, objv) ;

    case RELVAR_INSERT:
	return RelvarInsertCmd(interp, objc, objv) ;

    case RELVAR_UPDATE:
	return RelvarUpdateCmd(interp, objc, objv) ;

    default:
	Tcl_Panic("unexpected relvar subcommand value") ;
    }

    /*
     * NOT REACHED
     */
    return TCL_ERROR ;
}

/*
 * ======================================================================
 * Relation Sub-Command Functions
 * ======================================================================
 */

static int
RelationCardinalityCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;
    int nTuples ;

    /* relation cardinality relValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewIntObj(relation->cardinality)) ;
    return TCL_OK ;
}

static int
RelationDegreeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    /* relation degree relValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp,
	Tcl_NewIntObj(relation->heading->tupleHeading->degree)) ;

    return TCL_OK ;
}

static int
RelationEmptyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    /* relation empty relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewIntObj(relation->cardinality == 0)) ;
    return TCL_OK ;
}

static int
RelationEmptyofCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    /* relation emptyof relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, relationObjNew(relationNew(relation->heading))) ;
    return TCL_OK ;
}

static int
RelationForeachCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *varNameObj ;
    Tcl_Obj *relObj ;
    Tcl_Obj *scriptObj ;
    Ral_Relation *relation ;
    int card ;
    Tcl_Obj **tupleVector ;
    int result = TCL_OK ;

    /* relation foreach tupleVarName relationValue script */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleVarName relationValue script") ;
	return TCL_ERROR ;
    }

    varNameObj = objv[2] ;
    relObj = objv[3] ;
    scriptObj = objv[4] ;

    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    /*
     * Hang onto these objects in case something strange happens in
     * the script execution.
     */
    Tcl_IncrRefCount(varNameObj) ;
    Tcl_IncrRefCount(relObj) ;
    Tcl_IncrRefCount(scriptObj) ;

    Tcl_ResetResult(interp) ;

    for (card = relation->cardinality, tupleVector = relation->tupleVector ;
	card > 0 ; --card, ++tupleVector) {

	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, *tupleVector,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    result = TCL_ERROR ;
	    break; 
	}

	result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
	if (result != TCL_OK) {
	    if (result == TCL_CONTINUE) {
		result = TCL_OK ;
	    } else if (result == TCL_BREAK) {
		result = TCL_OK ;
		break ;
	    } else if (result == TCL_ERROR) {
		static const char msgfmt[] =
		    "\n    (\"relation foreach\" body line %d)" ;
		char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE] ;
		sprintf(msg, msgfmt, interp->errorLine) ;
		Tcl_AddObjErrorInfo(interp, msg, -1) ;
		break ;
	    } else {
		break ;
	    }
	}
    }

    if (relation->cardinality) {
	Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    }
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_DecrRefCount(relObj) ;
    Tcl_DecrRefCount(scriptObj) ;

    return result ;
}

static int
RelationNotemptyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;
    Ral_Relation *newRelation ;

    /* relation notempty relValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewIntObj(relation->cardinality != 0)) ;
    return TCL_OK ;
}

static int
RelationTupleCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation tuple relationValue */
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;
    if (relation->cardinality != 1) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "relation must have cardinality of one", -1) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, *relation->tupleVector) ;
    return TCL_OK ;
}

static int
RelationTypeofCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    /* relation typeof relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, relationHeadingObjNew(interp, relation->heading)) ;
    return TCL_OK ;
}

/*
 * ======================================================================
 * Relation Ensemble Command Function
 * ======================================================================
 */

static int relationCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    enum RelationSubCmds {
	RELATION_ACCUMULATE,
	RELATION_CARDINALITY,
	RELATION_DEGREE,
	RELATION_DIVIDE,
	RELATION_ELIMINATE,
	RELATION_EMPTY,
	RELATION_EMPTYOF,
	RELATION_EXTEND,
	RELATION_FOREACH,
	RELATION_INTERSECT,
	RELATION_IS,
	RELATION_JOIN,
	RELATION_LIST,
	RELATION_MINUS,
	RELATION_NOTEMPTY,
	RELATION_PROJECT,
	RELATION_RENAME,
	RELATION_RESTRICT,
	RELATION_SEMIJOIN,
	RELATION_SEMIMINUS,
	RELATION_TIMES,
	RELATION_TUPLE,
	RELATION_TYPEOF,
	RELATION_UNGROUP,
	RELATION_UNION,
    } ;
    static const char *subCmds[] = {
	"accumulate",
	"cardinality",
	"degree",
	"divide",
	"eliminate",
	"empty",
	"emptyof",
	"extend",
	"foreach",
	"intersect",
	"is",
	"join",
	"list",
	"minus",
	"notempty",
	"project",
	"rename",
	"restrict",
	"semijoin",
	"semiminus",
	"times",
	"tuple",
	"typeof",
	"ungroup",
	"union",
	NULL
    } ;
    /*
	summarize
	tclose
	group
     */

    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObj(interp, *(objv + 1), subCmds, "subcommand", 0,
	&index) != TCL_OK) {
	return TCL_ERROR ;
    }

    switch ((enum RelationSubCmds)index) {
#if 0
    case RELATION_ACCUMULATE:
	return RelationAccumulateCmd(interp, objc, objv) ;
#endif

    case RELATION_CARDINALITY:
	return RelationCardinalityCmd(interp, objc, objv) ;

    case RELATION_DEGREE:
	return RelationDegreeCmd(interp, objc, objv) ;

#if 0
    case RELATION_DIVIDE:
	return RelationDivideCmd(interp, objc, objv) ;

    case RELATION_ELIMINATE:
	return RelationEliminateCmd(interp, objc, objv) ;
#endif

    case RELATION_EMPTY:
	return RelationEmptyCmd(interp, objc, objv) ;

    case RELATION_EMPTYOF:
	return RelationEmptyofCmd(interp, objc, objv) ;

#if 0
    case RELATION_EXTEND:
	return RelationExtendCmd(interp, objc, objv) ;
#endif

    case RELATION_FOREACH:
	return RelationForeachCmd(interp, objc, objv) ;

#if 0
    case RELATION_INTERSECT:
	return RelationIntersectCmd(interp, objc, objv) ;

    case RELATION_IS:
	return RelationIsCmd(interp, objc, objv) ;

    case RELATION_JOIN:
	return RelationJoinCmd(interp, objc, objv) ;

    case RELATION_LIST:
	return RelationListCmd(interp, objc, objv) ;

    case RELATION_MINUS:
	return RelationMinusCmd(interp, objc, objv) ;
#endif

    case RELATION_NOTEMPTY:
	return RelationNotemptyCmd(interp, objc, objv) ;

#if 0
    case RELATION_PROJECT:
	return RelationProjectCmd(interp, objc, objv) ;

    case RELATION_RENAME:
	return RelationRenameCmd(interp, objc, objv) ;

    case RELATION_RESTRICT:
	return RelationRestrictCmd(interp, objc, objv) ;

    case RELATION_SEMIJOIN:
	return RelationSemijoinCmd(interp, objc, objv) ;

    case RELATION_SEMIMINUS:
	return RelationSemiminusCmd(interp, objc, objv) ;

    case RELATION_TIMES:
	return RelationTimesCmd(interp, objc, objv) ;
#endif

    case RELATION_TUPLE:
	return RelationTupleCmd(interp, objc, objv) ;

    case RELATION_TYPEOF:
	return RelationTypeofCmd(interp, objc, objv) ;

#if 0
    case RELATION_UNGROUP:
	return RelationUngroupCmd(interp, objc, objv) ;

    case RELATION_UNION:
	return RelationUnionCmd(interp, objc, objv) ;
#endif

    default:
	Tcl_Panic("unexpected relation subcommand value") ;
    }

    /*
     * NOT REACHED
     */
    return TCL_ERROR ;
}

/*
 * ======================================================================
 * Initialization Function
 * ======================================================================
 */

int
Ral_Init(
    Tcl_Interp * interp)
{
    Tcl_Namespace *ralNs ;

    Tcl_InitStubs(interp, TCL_VERSION, 0) ;

    Tcl_RegisterObjType(&Ral_TupleType) ;
    Tcl_RegisterObjType(&Ral_RelationType) ;

    ralNs = Tcl_CreateNamespace(interp, "::ral", NULL, NULL) ;

    Tcl_CreateObjCommand(interp, "::ral::tuple", tupleCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, "tuple", 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    Tcl_InitObjHashTable(&relvarMap) ;
    Tcl_CreateObjCommand(interp, "::ral::relvar", relvarCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, "relvar", 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    Tcl_CreateObjCommand(interp, "::ral::relation", relationCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, "relation", 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    Tcl_PkgProvide(interp, "ral", "0.6") ;

    return TCL_OK ;
}
