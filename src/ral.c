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
$Revision: 1.7 $
$Date: 2004/05/21 04:20:34 $
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
#define	max(a,b)    ((a) > (b) ? (a) : (b))


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
 * An attribute order map is a data structure that allows the attributes of a
 * tuple to be reordered to match that of another tuple. Since attribute order
 * is arbitrary, it is possible to combine tuples from different relations that
 * have equal headings but for which the attributes are ordered differently.
 * In this case the attributes must be reordered to match the order stored a
 * relation heading.
 */

typedef struct Ral_AttrOrderMap {
    int isInOrder ;	/* if the order happens to match, this is non-zero */
    int attrCount ;
    int *attrOrder ;
} Ral_AttrOrderMap ;

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
 *
 * Internally tuples are reference counted. The same tuple is sometimes
 * referenced by many different relations.
 */

typedef struct Ral_Tuple {
    int refCount ;		/* Reference Count */
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
    int *attrIndices ;	/* Pointer to an array of attribute indices that
			 * constitute the identifier. The attribute indices
			 * index into the "attrVector" of the corresponding
			 * tuple heading */
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
    Ral_Tuple **tupleVector ;	    /* A vector of tuples that constitute the
				     * body of the relation. The vector is
				     * "allocated" long and the first
				     * "cardinality" entries are in use. */
    Tcl_HashTable *indexVector ;    /* A vector of hash tables that map the
				     * the values of identifying attributes
				     * to the index in the tuple vector where
				     * the corresponding tuple is found.
				     * There is one hash table for each
				     * identifier */
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
static char rcsid[] = "@(#) $RCSfile: ral.c,v $ $Revision: 1.7 $" ;

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

/*
 * compare two integers. Used by "qsort".
 */
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
    assert(degree >= 0) ;
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
    Ral_TupleHeading *heading)
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
tupleHeadingInsertName(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading,
    Tcl_Obj *attrName,
    int where)
{
    Tcl_HashEntry *entry ;
    int newPtr ;

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
    assert(where < heading->degree) ;
    Tcl_SetHashValue(entry, where) ;

    return TCL_OK ;
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
	if (tupleHeadingInsertName(interp, dest, first->name, start++)
	    != TCL_OK) {
	    return TCL_ERROR ;
	}
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
	srcHeading->attrVector + srcHeading->degree, dupHeading, 0)
	!= TCL_OK) {
	tupleHeadingDelete(dupHeading) ;
	return NULL ;
    }
    return dupHeading ;
}

/*
 * Create a new tuple heading that is the union of two headings.
 */
static Ral_TupleHeading *
tupleHeadingUnion(
    Tcl_Interp *interp,
    Ral_TupleHeading *h1,
    Ral_TupleHeading *h2)
{
    Ral_TupleHeading *newHeading ;

    newHeading = tupleHeadingNew(h1->degree + h2->degree) ;

    if (tupleHeadingCopy(interp, h1->attrVector, h1->attrVector + h1->degree,
	    newHeading, 0) != TCL_OK ||
	tupleHeadingCopy(interp, h2->attrVector, h2->attrVector + h2->degree,
	    newHeading, h1->degree) != TCL_OK) {
	tupleHeadingDelete(newHeading) ;
	return NULL ;
    }

    return newHeading ;
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
tupleHeadingFindIndex(
    Tcl_Interp *interp,
    Ral_TupleHeading *heading,
    Tcl_Obj *attrName)
{
    Tcl_HashEntry *entry ;
    int index = -1 ;

    entry = Tcl_FindHashEntry(&heading->nameMap, (char *)attrName) ;
    if (entry) {
	index = (int)Tcl_GetHashValue(entry) ;
	assert(index < heading->degree) ;
    } else if (interp) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "unknown attribute name, \"", Tcl_GetString(attrName), "\"",
	    NULL) ;
    }

    return index ;
}

/*
 * Determine the number of attributes in "h1" that have a match in "h2".
 */
static int
tupleHeadingCompare(
    Ral_TupleHeading *h1,
    Ral_TupleHeading *h2)
{
    Ral_Attribute *a1 ;
    Ral_Attribute *last ;
    int matches = 0 ;

    /*
     * Heading order does not matter.  So iterate through the vector of the
     * first heading an look up the corresponding attribute names in the other
     * heading.
     */
    last = h1->attrVector + h1->degree ;
    for (a1 = h1->attrVector ; a1 != last ; ++a1) {
	Ral_Attribute *a2 ;

	a2 = tupleHeadingFindAttribute(NULL, h2, a1->name, NULL) ;
	if (a2 && attributeEqual(a1, a2)) {
	    ++matches ;
	}
    }

    return matches ;
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

    return tupleHeadingCompare(h1, h2) == h1->degree ;
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
 * Attribute Order Map Functions
 * ======================================================================
 */

static Ral_AttrOrderMap *
attrOrderMapNew(
    int attrCount)
{
    Ral_AttrOrderMap *map ;
    int nbytes ;

    nbytes = sizeof(*map) + attrCount * sizeof(*map->attrOrder) ;
    map = (Ral_AttrOrderMap *)ckalloc(nbytes) ;
    memset(map, 0, nbytes) ;

    map->isInOrder = 0 ;
    map->attrCount = attrCount ;
    map->attrOrder = (int *)(map + 1) ;

    return map ;
}

/*
 * Compute ordering to map values from "th2" to the same order as "th1".
 */
static Ral_AttrOrderMap *
attrOrderMapGenerateOrdering(
    Ral_TupleHeading *th1,
    Ral_TupleHeading *th2)
{
    Ral_AttrOrderMap *map ;
    int index1 ;
    Ral_Attribute *attr1 ;
    int *order ;
    int nOutOfOrder = 0 ;

    assert(tupleHeadingEqual(th1, th2)) ;
    map = attrOrderMapNew(th1->degree) ;
    order = map->attrOrder ;

    attr1 = th1->attrVector ;
    for (index1 = 0 ; index1 < th1->degree ; ++index1, ++attr1) {
	int index2 ;

	index2 = tupleHeadingFindIndex(NULL, th2, attr1->name) ;
	assert(index2 >= 0) ;

	*order++ = index2 ;
	nOutOfOrder += index1 != index2 ;
    }

    map->isInOrder = nOutOfOrder == 0 ;
    return map ;
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

    tuple->refCount = 0 ;
    tupleHeadingReference(tuple->heading = heading) ;
    tuple->values = (Tcl_Obj **)(tuple + 1) ;

    return tuple ;
}

static void
tupleDelete(
    Ral_Tuple *tuple)
{
    if (tuple) {
	Tcl_Obj **objPtr ;
	Tcl_Obj **last ;

	assert(tuple->refCount <= 0) ;
	for (objPtr = tuple->values, last = objPtr + tuple->heading->degree ;
	    objPtr != last ; ++objPtr) {
	    if (*objPtr) {
		Tcl_DecrRefCount(*objPtr) ;
	    }
	}
	tupleHeadingUnreference(tuple->heading) ;

	ckfree((char *)tuple) ;
    }
}

static void
tupleReference(
    Ral_Tuple *tuple)
{
    ++tuple->refCount ;
}

static void
tupleUnreference(
    Ral_Tuple *tuple)
{
    if (tuple && --tuple->refCount < 0) {
	tupleDelete(tuple) ;
    }
}

static void
tupleCopyValues(
    Ral_Tuple *srcTuple,
    int first,
    int last,
    Ral_Tuple *destTuple,
    int start)
{
    Tcl_Obj **srcValues ;
    Tcl_Obj **lastValue ;
    Tcl_Obj **destValues ;

    assert(first <= srcTuple->heading->degree) ;
    assert(last <= srcTuple->heading->degree) ;
    assert(first + last <= srcTuple->heading->degree) ;
    assert(start <= destTuple->heading->degree) ;
    assert(start + last <= destTuple->heading->degree) ;

    srcValues = srcTuple->values + first ;
    lastValue = srcTuple->values + last ;
    destValues = destTuple->values + start ;

    while (srcValues != lastValue) {
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
    tupleCopyValues(srcTuple, 0, srcTuple->heading->degree, dupTuple, 0) ;

    return dupTuple ;
}

/*
 * A shallow tuple duplication just references the tuple heading instead
 * of also making a copy.
 */
static Ral_Tuple *
tupleDupShallow(
    Ral_Tuple *srcTuple)
{
    Ral_TupleHeading *srcHeading = srcTuple->heading ;
    Ral_Tuple *dupTuple ;

    dupTuple = tupleNew(srcHeading) ;
    /*
     * Copy the values to the new tuple.
     */
    tupleCopyValues(srcTuple, 0, srcHeading->degree, dupTuple, 0) ;

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
    tupleReference(objPtr->internalRep.otherValuePtr = tuple) ;
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
tupleUpdateValues(
    Tcl_Interp *interp,
    Ral_Tuple *tuple,
    Tcl_Obj *nvList)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_TupleHeading *heading ;

    if (Tcl_ListObjGetElements(interp, nvList, &elemc, &elemv) != TCL_OK)
	return TCL_ERROR ;
    if (elemc % 2 != 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp),
	    "list must have an even number of elements", -1) ;
	return TCL_ERROR ;
    }

    if (tuple->refCount > 1) {
	Tcl_Panic("tupleUpdateValues called with a shared tuple") ;
    }
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
    tupleUnreference(objPtr->internalRep.otherValuePtr) ;
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
	tupleReference(dupPtr->internalRep.otherValuePtr = dupTuple) ;
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
    tupleReference(objPtr->internalRep.otherValuePtr = tuple) ;

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
    if (relId->attrIndices) {
	ckfree((char *)relId->attrIndices) ;
    }
    relId->attrIndices = NULL ;
    relId->attrCount = 0 ;
}

static void
relIdCtor(
    Ral_RelId *relId,
    int nAttrs)
{
    relIdDtor(relId) ;

    if (nAttrs > 0) {
	relId->attrCount = nAttrs ;
	relId->attrIndices = (int *)ckalloc(nAttrs *
	    sizeof(*relId->attrIndices)) ;
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
    return memcmp(id1->attrIndices, id2->attrIndices,
	id1->attrCount * sizeof(*id1->attrIndices)) == 0 ;
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
    return memcmp(id1->attrIndices, id2->attrIndices,
	id1->attrCount * sizeof(*id1->attrIndices)) == 0 ;
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
    int *attrIndices ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv)) {
	return TCL_ERROR ;
    }
    relIdCtor(relId, elemc) ;

    for (attrIndices = relId->attrIndices ; elemc > 0 ;
	--elemc, ++elemv, ++attrIndices) {
	int index ;

	index = tupleHeadingFindIndex(interp, tupleHeading, *elemv) ;
	if (index < 0) {
	    return TCL_ERROR ;
	}
	*attrIndices = index ;
    }
    qsort(relId->attrIndices, relId->attrCount, sizeof(*relId->attrIndices),
	int_compare) ;

    return TCL_OK ;
}

/*
 * Create an identifier that matches all the attributes of the heading.
 */
static void
relIdSetAllAttributes(
    Ral_RelId *relId,
    int degree)
{
    int index ;
    int *attrIndices ;

    relIdCtor(relId, degree) ;
    for (index = 0, attrIndices = relId->attrIndices ; index < degree ;
	++index, ++attrIndices) {
	*attrIndices = index ;
    }
}

static Tcl_Obj *
relIdAttrsToList(
    Tcl_Interp *interp,
    Ral_RelId *relId,
    Ral_TupleHeading *tupleHeading)
{
    Tcl_Obj *attrList ;
    int attrCount ;
    int *attrIndices ;
    Ral_Attribute *attrSet ;

    attrList = Tcl_NewListObj(0, NULL) ;
    attrSet = tupleHeading->attrVector ;
    for (attrCount = relId->attrCount, attrIndices = relId->attrIndices ;
	attrCount > 0 ; --attrCount, ++attrIndices) {
	if (Tcl_ListObjAppendElement(interp, attrList,
	    attrSet[*attrIndices].name) != TCL_OK) {
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
    int *attrIndices ;
    int i ;

    attrIndices = relId->attrIndices ;
    for (i = 0 ; i < relId->attrCount ; ++i) {
	if (i != 0) {
	    Tcl_AppendStringsToObj(resultObj, ", ", NULL) ;
	}
	Tcl_AppendStringsToObj(resultObj,
	    Tcl_GetString(tupleAttrs[*attrIndices++].name), NULL) ;
    }
}

static void
relIdKey(
    Ral_RelId *relId,
    Ral_Tuple *tuple,
    Tcl_DString *idKey)
{
    Tcl_Obj **values = tuple->values ;
    int *attrIndices = relId->attrIndices ;
    int i ;

    Tcl_DStringInit(idKey) ;
    for (i = relId->attrCount ; i > 0 ; --i) {
	Tcl_DStringAppend(idKey, Tcl_GetString(values[*attrIndices++]), -1) ;
    }
}

/*
 * Recompute a relId based on the names from one heading indexed into
 * another heading.
 * Assumes the tuple headings are equal!
 */
static void
relIdMapToHeading(
    Ral_RelId *relId1,
    Ral_TupleHeading *h1,
    Ral_RelId *relId2,
    Ral_TupleHeading *h2)
{
    int c1 = relId1->attrCount ;
    int *v1 = relId1->attrIndices ;
    int *v2 = relId2->attrIndices ;

    assert(h1->degree == h2->degree) ;

    for ( ; c1 > 0 ; --c1, ++v1, ++v2) {
	Ral_Attribute *attr1 = h1->attrVector + *v1 ;
	Ral_Attribute *attr2 ;
	int index ;

	index = tupleHeadingFindIndex(NULL, h2, attr1->name) ;
	assert(index >= 0) ;
	*v2 = index ;
    }
    qsort(relId2->attrIndices, relId2->attrCount, sizeof(*relId2->attrIndices),
	int_compare) ;
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

    assert(nIds > 0) ;

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
    if (addAttrs == 0) {
	relationHeadingReference(srcHeading) ;
	return srcHeading ;
    } else {
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
	    memcpy(destIdVector->attrIndices, srcIdVector->attrIndices,
		srcIdVector->attrCount * sizeof(*srcIdVector->attrIndices)) ;
	}
	return heading ;
    }
    /* NOTREACHED */
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

/*
 * Determine if  the identifiers of two headings are the same.
 * Takes into account that the order of attributes in the two headings
 * might be different.
 */
static int
relationHeadingIdsEqual(
    Ral_RelationHeading *h1,
    Ral_RelationHeading *h2)
{
    Ral_TupleHeading *th1 = h1->tupleHeading ;
    int idCount = h1->idCount ;
    Ral_RelId *idVector = h1->idVector ;
    Ral_TupleHeading *th2 = h2->tupleHeading ;
    Ral_RelId keyId ;

    if (h1->idCount != h2->idCount) {
	return 0 ;
    }
    /*
     * Strategy is to compose a "relId" using the names of h1 and find
     * the corresponding indices from h2. Then we can search the Id's
     * of h2 to see if we can find a match. If identifier in h1 has
     * a correspondence in h2, then we have a match.
     */
    memset(&keyId, 0, sizeof(keyId)) ;
    for ( ; idCount > 0 ; --idCount, ++idVector) {
	Ral_RelId *found ;

	relIdCtor(&keyId, idVector->attrCount) ;

	relIdMapToHeading(idVector, th1, &keyId, th2) ;
	found = relationHeadingFindId(h2, &keyId) ;

	relIdDtor(&keyId) ;

	if (found == NULL) {
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
    if (h1 == h2) {
	return 1 ;
    }
    if (!tupleHeadingEqual(h1->tupleHeading, h2->tupleHeading)) {
	return 0 ;
    }
    return relationHeadingIdsEqual(h1, h2) ;
}

static Ral_RelationHeading *
relationHeadingNewFromObjs(
    Tcl_Interp *interp,
    Tcl_Obj *headingObj,
    Tcl_Obj *identObj)
{
    Ral_TupleHeading *tupleHeading ;
    Ral_RelationHeading *heading ;
    Tcl_Obj *allocated = NULL ;
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
    if (idc == 0) {
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
	    relationHeadingDelete(heading) ;
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
relationHeadingCmpTypes(
    Tcl_Interp *interp,
    Ral_RelationHeading *h1,
    Ral_RelationHeading *h2)
{
    if (!relationHeadingEqual(h1, h2)) {
	Tcl_Obj *h1Obj = relationHeadingObjNew(NULL, h1) ;
	Tcl_Obj *h2Obj = relationHeadingObjNew(NULL, h2) ;

	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "type mismatch: heading, \"",
	    h1Obj ? Tcl_GetString(h1Obj) : "<unknown union heading>",
	    "\", does match, \"",
	    h2Obj ? Tcl_GetString(h2Obj) : "<unknown relation heading>",
	    "\"", NULL) ;

	if (h1Obj) {
	    Tcl_DecrRefCount(h1Obj) ;
	}
	if (h2Obj) {
	    Tcl_DecrRefCount(h2Obj) ;
	}
	return TCL_ERROR ;
    }

    return TCL_OK ;
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
    int nBytes ;
    Ral_Relation *relation ;
    int c ;
    Tcl_HashTable *indexVector ;

    nBytes = sizeof(*relation) +
	heading->idCount * sizeof(*relation->indexVector) ;
    relation = (Ral_Relation *)ckalloc(nBytes) ;
    memset(relation, 0, nBytes) ;

    relationHeadingReference(relation->heading = heading) ;
    relation->indexVector = (Tcl_HashTable *)(relation + 1) ;
    for (c = heading->idCount, indexVector = relation->indexVector ;
	c > 0 ; --c, ++indexVector) {
	Tcl_InitHashTable(indexVector, TCL_STRING_KEYS) ;
    }

    return relation ;
}

static void
relationDelete(
    Ral_Relation *relation)
{
    int c ;
    Ral_Tuple **t ;
    Tcl_HashTable *indexVector ;

    for (c = relation->cardinality, t = relation->tupleVector ; c > 0 ;
	--c, ++t) {
	if (*t) {
	    tupleUnreference(*t) ;
	}
    }
    for (c = relation->heading->idCount, indexVector = relation->indexVector ;
	c > 0 ; --c, ++indexVector) {
	Tcl_DeleteHashTable(indexVector) ;
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
    relation->tupleVector = (Ral_Tuple **)ckrealloc(
	(char *)relation->tupleVector,
	relation->allocated * sizeof(*relation->tupleVector)) ;
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

static Tcl_Obj *
relationNameValueList(
    Tcl_Interp *interp,
    Ral_Relation *relation)
{
    Ral_RelationHeading *heading ;
    Tcl_Obj *nvList ;
    Ral_Tuple **tupleVector ;
    Ral_Tuple **last ;

    heading = relation->heading ;
    nvList = Tcl_NewListObj(0, NULL) ;

    for (tupleVector = relation->tupleVector,
	last = tupleVector + relation->cardinality ; tupleVector != last ;
	++tupleVector) {
	Ral_Tuple *tuple ;
	Tcl_Obj *tupleNVList ;

	tuple = *tupleVector ;
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
relationIndexIdentifier(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    int idIndex,
    Ral_Tuple *tuple,
    int where)
{
    Ral_RelId *relId ;
    Tcl_HashTable *hashTable ;
    Tcl_DString idKey ;
    Tcl_HashEntry *entry ;
    int newPtr ;

    assert(idIndex < relation->heading->idCount) ;

    relId = relation->heading->idVector + idIndex ;
    hashTable = relation->indexVector + idIndex ;

    relIdKey(relId, tuple, &idKey) ;
    entry = Tcl_CreateHashEntry(hashTable, Tcl_DStringValue(&idKey), &newPtr) ;
    Tcl_DStringFree(&idKey) ;
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
	return TCL_ERROR ;
    }

    Tcl_SetHashValue(entry, where) ;
    return TCL_OK ;
}

static Tcl_HashEntry *
relationFindIndexEntry(
    Ral_Relation *relation,
    int idIndex,
    Ral_Tuple *tuple)
{
    Ral_RelId *relId ;
    Tcl_DString idKey ;
    Tcl_HashEntry *entry ;

    assert(idIndex < relation->heading->idCount) ;
    relId = relation->heading->idVector + idIndex ;

    relIdKey(relId, tuple, &idKey) ;
    entry = Tcl_FindHashEntry(relation->indexVector + idIndex,
	Tcl_DStringValue(&idKey)) ;
    Tcl_DStringFree(&idKey) ;

    return entry ;
}

static void
relationRemoveIndex(
    Ral_Relation *relation,
    int idIndex,
    Ral_Tuple *tuple)
{
    Tcl_HashEntry *entry ;

    entry = relationFindIndexEntry(relation, idIndex, tuple) ;
    assert (entry != NULL) ;
    Tcl_DeleteHashEntry(entry) ;
}

static int
relationFindTupleIndex(
    Ral_Relation *relation,
    int idIndex,
    Ral_Tuple *tuple)
{
    Tcl_HashEntry *entry ;

    entry = relationFindIndexEntry(relation, idIndex, tuple) ;
    return entry ? (int)Tcl_GetHashValue(entry) : -1 ;
}

/*
 * Find the index into the tuples of a relation that match the given tuple.
 * "idIndex" is the identifier index, "order" maps the value order in "tuple"
 * to that of the given relation (which might or might not be the same as the
 * relation to which "tuple" belongs; if it is the same relation then the
 * mapping is the identity).
 */
static int
relationFindIndexInOrder(
    Ral_Relation *relation,
    int idIndex,
    Ral_AttrOrderMap *order,
    Ral_Tuple *tuple)
{
    int tupleIndex ;

    assert (idIndex < relation->heading->idCount) ;

    if (order->isInOrder) {
	tupleIndex = relationFindTupleIndex(relation, idIndex, tuple) ;
    } else {
	Tcl_DString idKey ;
	Ral_RelId *relId = relation->heading->idVector + idIndex ;
	Tcl_Obj **values = tuple->values ;
	int *attrIndices = relId->attrIndices ;
	int *attrOrder = order->attrOrder ;
	Tcl_HashTable *indexTable = relation->indexVector + idIndex ;
	int i ;
	Tcl_HashEntry *entry ;

	assert(relation->heading->tupleHeading->degree == order->attrCount) ;

	Tcl_DStringInit(&idKey) ;
	for (i = relId->attrCount ; i > 0 ; --i) {
	    Tcl_DStringAppend(&idKey,
		Tcl_GetString(values[attrOrder[*attrIndices++]]), -1) ;
	}
	entry = Tcl_FindHashEntry(indexTable, Tcl_DStringValue(&idKey)) ;
	Tcl_DStringFree(&idKey) ;
	tupleIndex = entry ? (int)Tcl_GetHashValue(entry) : -1 ;
    }

    return tupleIndex ;
}

static int
relationIndexTuple(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Ral_Tuple *tuple,
    int where)
{
    int i ;
    Ral_RelationHeading *heading = relation->heading ;

    assert(heading->idCount > 0) ;

    for (i = 0 ; i < heading->idCount ; ++i) {
	if (relationIndexIdentifier(interp, relation, i, tuple, where)
	    != TCL_OK) {
	    /*
	     * Need to back out any index that was successfully done.
	     */
	    int j ;
	    for (j = 0 ; j < i ; ++j) {
		relationRemoveIndex(relation, j, tuple) ;
	    }
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

static void
relationRemoveTupleIndex(
    Ral_Relation *relation,
    Ral_Tuple *tuple)
{
    int i ;
    Ral_RelationHeading *heading = relation->heading ;

    for (i = 0 ; i < heading->idCount ; ++i) {
	relationRemoveIndex(relation, i, tuple) ;
    }
}

static void
relationReindexTuple(
    Ral_Relation *relation,
    Ral_Tuple *tuple,
    int where)
{
    relationRemoveTupleIndex(relation, tuple) ;
    relationIndexTuple(NULL, relation, tuple, where) ;
}

/*
 * Create a hash table based on the values of all the attributes.
 */
static void
relationIndexAllAttributes(
    Ral_Relation *relation,
    Tcl_HashTable *hashTable)
{
    Ral_Tuple **tupleVector = relation->tupleVector ;
    int card ;
    Ral_TupleHeading *tupleHeading = relation->heading->tupleHeading ;

    for (card = 0 ; card < relation->cardinality ; ++card, ++tupleVector) {
	Ral_Tuple *tuple = *tupleVector ;
	int degree ;
	Tcl_DString idKey ;
	Tcl_HashEntry *entry ;
	int newPtr ;

	Tcl_DStringInit(&idKey) ;

	for (degree = 0 ; degree < tupleHeading->degree ; ++degree) {
	    Tcl_DStringAppend(&idKey, Tcl_GetString(tuple->values[degree]),
		-1) ;
	}

	entry = Tcl_CreateHashEntry(hashTable, Tcl_DStringValue(&idKey),
	    &newPtr) ;
	Tcl_DStringFree(&idKey) ;
	/*
	 * there are not supposed to be duplicate tuples in a relation.
	 */
	assert(newPtr == 1) ;
	assert(entry != NULL) ;
	Tcl_SetHashValue(entry, card) ;
    }
}

/*
 * Find the index in a hash table were the values of tuple match.
 * Only those parts of the tuple that correpond to the given tuple heading
 * are considered.
 */
static int
relationFindCorresponding(
    Ral_Tuple *tuple,
    Ral_TupleHeading *hashTupleHeading,
    Tcl_HashTable *hashTable)
{
    Tcl_DString idKey ;
    int degree ;
    Ral_Attribute *attrVector ;
    Tcl_HashEntry *entry ;

    Tcl_DStringInit(&idKey) ;

    for (degree = hashTupleHeading->degree,
	attrVector = hashTupleHeading->attrVector ; degree > 0 ;
	--degree, ++attrVector) {
	int index ;
	index = tupleHeadingFindIndex(NULL, tuple->heading, attrVector->name) ;
	assert(index >= 0) ;
	Tcl_DStringAppend(&idKey, Tcl_GetString(tuple->values[index]), -1) ;
    }
    entry = Tcl_FindHashEntry(hashTable, Tcl_DStringValue(&idKey)) ;
    Tcl_DStringFree(&idKey) ;
    return entry ? (int)Tcl_GetHashValue(entry) : -1 ;
}

static int
relationAppendTuple(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Ral_Tuple *tuple)
{
    assert(relation->cardinality < relation->allocated) ;
    if (relationIndexTuple(interp, relation, tuple, relation->cardinality)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    tupleReference(relation->tupleVector[relation->cardinality++] = tuple) ;
    return TCL_OK ;
}
    
static int
relationAppendValue(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Tcl_Obj *nvList)
{
    Ral_Tuple *tuple ;

    assert(relation->cardinality < relation->allocated) ;

    tuple = tupleNew(relation->heading->tupleHeading) ;
    if (tupleSetValuesFromString(interp, tuple, nvList) != TCL_OK ||
	relationAppendTuple(interp, relation, tuple) != TCL_OK) {
	tupleDelete(tuple) ;
	return TCL_ERROR ;
    }

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

    if (Tcl_ListObjGetElements(interp, tupleList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    relationReserve(relation, elemc) ;
    assert(relation->allocated >= relation->cardinality + elemc) ;

    while (elemc-- > 0) {
	if (relationAppendValue(interp, relation, *elemv++) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

/*
 * Insert a tuple into a relation. This function presumes that all the
 * checks necessary to insure that there are no duplicates have been
 * made. If the value ordering in the tuple matches the relation, the
 * the tuple is referenced counted and stored. Otherwise a new tuple
 * is constructed to match the relation heading ordering and that tuple
 * is inserted.
 */
static int
relationAppendReorderedTuple(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Ral_Tuple *tuple,
    Ral_AttrOrderMap *orderMap)
{
    /*
     * Reorder the values in the tuple if necessary. If they happen to
     * be in order, then we can just reference to tuple object. Otherwise
     * we have to create a new tuple and add the values in the order
     * to match the relation.
     */
    if (orderMap->isInOrder) {
	return relationAppendTuple(interp, relation , tuple) ;
    } else {
	Ral_Tuple *newTuple ;
	int attrCount ;
	int *order ;
	Tcl_Obj **values ;
	Tcl_Obj **newValues ;

	newTuple = tupleNew(relation->heading->tupleHeading) ;

	order = orderMap->attrOrder ;
	values = tuple->values ;
	newValues = newTuple->values ;
	assert(newTuple->heading->degree == orderMap->attrCount) ;
	for (attrCount = orderMap->attrCount ; attrCount > 0 ; --attrCount) {
	    Tcl_IncrRefCount(*newValues++ = values[*order++]) ;
	}

	if (relationAppendTuple(interp, relation , newTuple) != TCL_OK) {
	    tupleDelete(newTuple) ;
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

static int
relationDeleteTuple(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    int where)
{
    Ral_Tuple **tupleVector ;
    Ral_Tuple *tuple ;

    assert(where < relation->cardinality) ;
    tupleVector = relation->tupleVector + where ;

    tuple = *tupleVector ;
    relationRemoveTupleIndex(relation, tuple) ;
    tupleUnreference(tuple) ;
    --relation->cardinality ;

    for ( ; where < relation->cardinality ; ++where, ++tupleVector) {
	tuple = *tupleVector = *(tupleVector + 1) ;
	relationReindexTuple(relation, tuple, where) ;
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
    Ral_Tuple **tupleVector ;
    Ral_Tuple *tuple ;
    Ral_Tuple *dupTuple ;

    assert(where < relation->cardinality) ;
    tupleVector = relation->tupleVector + where ;

    tuple = *tupleVector ;
    dupTuple = tupleDup(tuple, 0) ;

    if (tupleUpdateValues(interp, dupTuple, nvList) != TCL_OK) {
	tupleDelete(dupTuple) ;
	return TCL_ERROR ;
    }

    relationRemoveTupleIndex(relation, tuple) ;
    if (relationIndexTuple(interp, relation, dupTuple, where) != TCL_OK) {
	relationIndexTuple(interp, relation, tuple, where) ;
	tupleDelete(dupTuple) ;
	return TCL_ERROR ;
    }

    tupleUnreference(tuple) ;
    tupleReference(*tupleVector = dupTuple) ;

    return TCL_OK ;
}

static Ral_Relation *
relationDup(
    Ral_Relation *srcRelation,
    int addAttrs)
{
    Ral_RelationHeading *srcHeading = srcRelation->heading ;
    int srcDegree = srcHeading->tupleHeading->degree ;
    Ral_RelationHeading *dupHeading ;
    Ral_Relation *dupRelation ;
    Ral_TupleHeading *dupTupleHeading ;
    Ral_Tuple **srcVector ;
    Ral_Tuple **last ;

    dupHeading = relationHeadingDup(srcHeading, addAttrs) ;
    if (!dupHeading) {
	return NULL ;
    }
    dupRelation = relationNew(dupHeading) ;
    relationReserve(dupRelation, srcRelation->cardinality) ;

    dupTupleHeading = dupHeading->tupleHeading ;
    for (srcVector = srcRelation->tupleVector,
	last = srcVector + srcRelation->cardinality ; srcVector != last ;
	++srcVector) {
	Ral_Tuple *dupTuple ;

	dupTuple = tupleNew(dupTupleHeading) ;
	tupleCopyValues(*srcVector, 0, srcDegree, dupTuple, 0) ;
	if (relationAppendTuple(NULL, dupRelation, dupTuple) != TCL_OK) {
	    tupleDelete(dupTuple) ;
	    relationDelete(dupRelation) ;
	    return NULL ;
	}
    }

    return dupRelation ;
}

static int
relationMultTuples(
    Tcl_Interp *interp,
    Ral_Relation *product,
    Ral_Relation *multiplicand,
    Ral_Relation *multiplier)
{
    Ral_Tuple **mcand ;
    int card1 ;

    for (card1 = multiplicand->cardinality, mcand = multiplicand->tupleVector ;
	card1 > 0 ; --card1, ++mcand) {
	Ral_Tuple *t1 = *mcand ;
	int degree1 = t1->heading->degree ;
	int card2 ;
	Ral_Tuple **mier ;


	for (card2 = multiplier->cardinality, mier = multiplier->tupleVector ;
	    card2 > 0 ; --card2, ++mier) {
	    Ral_Tuple *t2 = *mier ;
	    Ral_Tuple *prodTuple = tupleNew(product->heading->tupleHeading) ;

	    tupleCopyValues(t1, 0, degree1, prodTuple, 0) ;
	    tupleCopyValues(t2, 0, t2->heading->degree, prodTuple, degree1) ;
	    if (relationAppendTuple(interp, product, prodTuple) != TCL_OK) {
		return TCL_ERROR ;
	    }
	}
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
	if (tupleHeadingFindIndex(interp, heading, objv[i]) < 0) {
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
	attrIndex = tupleHeadingFindIndex(interp, heading, *objv) ;
	if (attrIndex < 0) {
	    return TCL_ERROR ;
	}
	resultObj = tuple->values[attrIndex] ;
    } else {
	resultObj = Tcl_NewListObj(0, NULL) ;
	while (objc-- > 0) {
	    attrIndex = tupleHeadingFindIndex(interp, heading, *objv++) ;
	    if (attrIndex < 0 ||
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
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK) {
	return TCL_ERROR ;
    }
    tuple = tupleObj->internalRep.otherValuePtr ;
    assert(tuple->refCount == 1) ;

    if (tupleUpdateValues(interp, tuple, objv[3]) != TCL_OK) {
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
    if (elemc > heading->degree) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "attempt to wrap more attributes than exist in the tuple", -1) ;
    }

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
	Tcl_Panic("tuple: unexpected tuple subcommand value") ;
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

static Tcl_Obj *
relvarFind(
    Tcl_Interp *interp,
    Tcl_Obj *relvarName)
{
    Tcl_HashEntry *entry ;
    Tcl_Obj *relObj = NULL ;

    entry = Tcl_FindHashEntry(&relvarMap, (char *)relvarName) ;
    if (entry) {
	relObj = (Tcl_Obj *)Tcl_GetHashValue(entry) ;
	if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	    return NULL ;
	}
    } else {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"unknown relvar, \"", Tcl_GetString(relvarName), "\"", NULL) ;
	}
    }

    return relObj ;
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
    while (objc-- > 0 ) {
	if (relationAppendValue(interp, relation, *objv++) != TCL_OK) {
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
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    Tcl_Obj *tupleNameObj ;
    Tcl_Obj *exprObj ;
    int index ;
    int deleted = 0 ;
    int result = TCL_OK ;

    /* relvar delete relvarName tupleVarName expr */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName tupleVarName expr") ;
	return TCL_ERROR ;
    }

    relObj = relvarFind(interp, objv[2]) ;
    if (!relObj) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

    for (index = 0 ; index < relation->cardinality ; ) {
	int boolValue ;
	Tcl_Obj *tupleObj ;

	tupleObj = tupleObjNew(relation->tupleVector[index]) ;
	if (Tcl_ObjSetVar2(interp, tupleNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
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
	    ++deleted ;
	} else {
	    ++index ;
	}
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;
    if (deleted) {
	Tcl_InvalidateStringRep(relObj) ;
    }

    Tcl_SetObjResult(interp, relObj) ;
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

    /* relvar destroy ?relvarName ... ? */
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "?relvarName ...?") ;
	return TCL_ERROR ;
    }

    objc -= 2 ;
    objv += 2 ;

    while (objc-- > 0) {
	entry = Tcl_FindHashEntry(&relvarMap, (char *)*objv++) ;
	if (entry) {
	    relvarObj = Tcl_GetHashValue(entry) ;
	    assert(relvarObj != NULL) ;
	    Tcl_DecrRefCount(relvarObj) ;
	    Tcl_DeleteHashEntry(entry) ;
	} else {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"relvar, \"", Tcl_GetString(objv[2]), "\" does not exist",
		NULL) ;
	    return TCL_ERROR ;
	}
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
	    Ral_Tuple **tupleVector ;

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
		Tcl_Obj *tupleNVList ;

		tupleNVList = tupleNameValueList(interp, *tupleVector) ;
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
    Tcl_Obj *relObj ;

    /* relvar get relvarName */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName") ;
	return TCL_ERROR ;
    }
    relObj = relvarFind(interp, objv[2]) ;
    if (!relObj) {
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, relObj) ;
    return TCL_OK ;
}

static int
RelvarInsertCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    int inserted = 0 ;

    /* relvar insert relvarName ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relObj = relvarFind(interp, objv[2]) ;
    if (!relObj) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    relationReserve(relation, objc) ;
    while (objc-- > 0) {
	if (relationAppendValue(interp, relation, *objv++) != TCL_OK) {
	    return TCL_ERROR ;
	}
	++inserted ;
    }

    if (inserted) {
	Tcl_InvalidateStringRep(relObj) ;
    }

    Tcl_SetObjResult(interp, relObj) ;
    return TCL_OK ;
}

static int
RelvarNamesCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *patternObj ;
    Tcl_Obj *nameList ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;

    /* relvar names ?pattern? */
    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "?pattern?") ;
	return TCL_ERROR ;
    }

    patternObj = objc == 3 ? objv[2] : NULL ;
    nameList = Tcl_NewListObj(0, NULL) ;

    for (entry = Tcl_FirstHashEntry(&relvarMap, &search) ; entry ;
	entry = Tcl_NextHashEntry(&search)) {
	Tcl_Obj *relvarName ;

	relvarName = (Tcl_Obj *)Tcl_GetHashKey(&relvarMap, entry) ;
	if (patternObj && !Tcl_StringMatch(Tcl_GetString(relvarName),
		Tcl_GetString(patternObj))) {
	    continue ;
	}
	if (Tcl_ListObjAppendElement(interp, nameList, relvarName) != TCL_OK) {
	    Tcl_DecrRefCount(nameList) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, nameList) ;
    return TCL_OK ;
}

static int
RelvarUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    Tcl_Obj *tupleNameObj ;
    Tcl_Obj *exprObj ;
    int index ;
    int updated = 0 ;

    /* relvar update relvarName tupleVarName expr nameValueList */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relvarName tupleVarName expr nameValueList") ;
	return TCL_ERROR ;
    }

    relObj = relvarFind(interp, objv[2]) ;
    if (!relObj) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    Tcl_IncrRefCount(tupleNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;

    for (index = 0 ; index < relation->cardinality ; ++index) {
	int boolValue ;
	Tcl_Obj *tupleObj ;

	tupleObj = tupleObjNew(relation->tupleVector[index]) ;
	if (Tcl_ObjSetVar2(interp, tupleNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
	    goto errorOut ;
	}
	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    goto errorOut ;
	}
	if (boolValue &&
	    relationUpdateTuple(interp, relation, index, objv[5]) != TCL_OK) {
	    goto errorOut ;
	}
	++updated ;
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    if (updated) {
	Tcl_InvalidateStringRep(relObj) ;
    }

    Tcl_SetObjResult(interp, relObj) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;
    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;
    if (updated) {
	Tcl_InvalidateStringRep(relObj) ;
    }
    return TCL_ERROR ;
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
	RELVAR_NAMES,
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
	"names",
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

    case RELVAR_NAMES:
	return RelvarNamesCmd(interp, objc, objv) ;

    case RELVAR_UPDATE:
	return RelvarUpdateCmd(interp, objc, objv) ;

    default:
	Tcl_Panic("relvar: unexpected relvar subcommand value") ;
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
RelationDivideCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *dendObj ;
    Ral_Relation *dend ;
    Ral_TupleHeading *dendTupleHeading ;
    Tcl_Obj *dsorObj ;
    Ral_Relation *dsor ;
    Ral_TupleHeading *dsorTupleHeading ;
    Tcl_Obj *medObj ;
    Ral_Relation *med ;
    Ral_TupleHeading *medTupleHeading ;
    Tcl_HashTable medHashTable ;
    Ral_Relation *quot ;
    Ral_TupleHeading *trialTupleHeading ;
    Ral_Tuple *trialTuple ;
    int dendCard ;
    Ral_Tuple **dendTupleVector ;

    /* relation divide dividend divisor mediator */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "dividend divisor mediator") ;
	return TCL_ERROR ;
    }

    dendObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, dendObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    dend = dendObj->internalRep.otherValuePtr ;
    dendTupleHeading = dend->heading->tupleHeading ;

    dsorObj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, dsorObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    dsor = dsorObj->internalRep.otherValuePtr ;
    dsorTupleHeading = dsor->heading->tupleHeading ;

    medObj = *(objv + 4) ;
    if (Tcl_ConvertToType(interp, medObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    med = medObj->internalRep.otherValuePtr ;
    medTupleHeading = med->heading->tupleHeading ;

    /*
     * The heading of the dividend must be disjoint from the heading of the
     * divisor.
     *
     * The heading of the mediator must be the union of the dividend and
     * divisor headings.
     *
     * The quotient, which has the same heading as the dividend, is then
     * computed by iterating over the dividend and for each tuple, determining
     * if all the tuples composed by combining a dividend tuple with all the
     * divisor tuples are contained in the mediator.  If they are, the that
     * dividend tuple is a tuple of the quotient.
     */

    if (tupleHeadingCompare(dendTupleHeading, dsorTupleHeading) != 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "divisor heading must be disjoint from the dividend heading", -1) ;
	return TCL_ERROR ;
    }

    if (tupleHeadingCompare(dendTupleHeading, medTupleHeading) +
	tupleHeadingCompare(dsorTupleHeading, medTupleHeading) !=
	medTupleHeading->degree) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
"mediator heading must be the union of dividend heading and divisor heading",
	    -1) ;
	return TCL_ERROR ;
    }
    /*
     * create quotient
     */
    quot = relationNew(relationHeadingNew(dendTupleHeading, 1)) ;
    relIdCtor(quot->heading->idVector, dend->heading->idVector->attrCount) ;
    memcpy(quot->heading->idVector->attrIndices,
	dend->heading->idVector->attrIndices,
	quot->heading->idVector->attrCount *
	    sizeof(*quot->heading->idVector->attrIndices)) ;
    relationReserve(quot, dend->cardinality) ;
    /*
     * Create an index for the mediator relation that will be used to
     * find the matching tuples for the quotient.
     */
    Tcl_InitHashTable(&medHashTable, TCL_STRING_KEYS) ;
    relationIndexAllAttributes(med, &medHashTable) ;
    /*
     * The trial tuple has a heading that is the union of
     * the dividend and divisor. That heading is the same as the
     * mediator heading, except for the possible difference in
     * attribute order. That's okay, because as we construct the
     * trial tuple from the dividend and divisor in their value order,
     * the comparison against the indexed mediator will happen in the
     * correct order when "relationFindCorresponding" is called.
     * Otherwise we would have to figure out how to construct the
     * trial tuple in mediator order.
     */
    trialTupleHeading = tupleHeadingUnion(interp, dendTupleHeading,
	dsorTupleHeading) ;
    if (trialTupleHeading == NULL) {
	goto errorOut ;
    }
    /*
     * Loop over the dividend.
     */
    for (dendCard = dend->cardinality, dendTupleVector = dend->tupleVector ;
	dendCard > 0 ; --dendCard, ++dendTupleVector) {
	Ral_Tuple *dendTuple ;
	int matches ;
	int dsorCard ;
	Ral_Tuple **dsorTupleVector ;

	dendTuple = *dendTupleVector ;
	matches = 0 ;
	/*
	 * Loop over the divsor.
	 */
	for (dsorCard = dsor->cardinality, dsorTupleVector = dsor->tupleVector ;
	    dsorCard > 0 ; --dsorCard, ++dsorTupleVector) {
	    Ral_Tuple *dsorTuple ;
	    int index ;

	    dsorTuple = *dsorTupleVector ;
	    /*
	     * Create the trial tuple.
	     */
	    trialTuple = tupleNew(trialTupleHeading) ;
	    /*
	     * Copy the dividend and divsor values into the trial tuple.
	     */
	    tupleCopyValues(dendTuple, 0, dendTupleHeading->degree,
		trialTuple, 0) ;
	    tupleCopyValues(dsorTuple, 0, dsorTupleHeading->degree,
		trialTuple, dendTupleHeading->degree) ;
	    /*
	     * Find the trial tuple in the mediator.
	     */
	    index = relationFindCorresponding(trialTuple, medTupleHeading,
		&medHashTable) ;
	    if (index >= 0) {
		++matches ;
	    }
	    tupleDelete(trialTuple) ;
	}

	    /*
	     * If every trial tuple appeared in the mediator, then
	     * corresponding dividend tuple is inserted into the quotient.
	     */
	if (matches == dsor->cardinality &&
	    relationAppendTuple(interp, quot, dendTuple) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }

    tupleHeadingDelete(trialTupleHeading) ;
    Tcl_DeleteHashTable(&medHashTable) ;
    Tcl_SetObjResult(interp, relationObjNew(quot)) ;
    return TCL_OK ;

errorOut2:
    tupleHeadingDelete(trialTupleHeading) ;
errorOut:
    Tcl_DeleteHashTable(&medHashTable) ;
    relationDelete(quot) ;
    return TCL_ERROR ;
}

static int
RelationEliminateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    Ral_TupleHeading *tupleHeading ;
    int elimDegree ;
    int c ;
    int *elimMap ;
    int elimCount ;
    int where ;
    Ral_TupleHeading *elimTupleHeading ;
    Ral_RelationHeading *elimHeading ;
    Ral_Relation *elimRelation ;
    int card ;
    Ral_Tuple **tupleVector ;
    Ral_Tuple *elimTuple ;

    /* relation eliminate relationValue ?attr ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue ?attr ... ?") ;
	return TCL_ERROR ;
    }
    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    tupleHeading = relation->heading->tupleHeading ;

    objc -= 3 ;
    objv += 3 ;

    if (objc > tupleHeading->degree) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	"attempted to eliminate more attributes than exist in the relation",
	    -1) ;
	return TCL_ERROR ;
    }

    elimMap = (int *)ckalloc(tupleHeading->degree * sizeof(*elimMap)) ;
    memset(elimMap, 0, tupleHeading->degree * sizeof(*elimMap)) ;
    while (objc-- > 0) {
	int index ;

	index = tupleHeadingFindIndex(interp, tupleHeading, *objv++) ;
	if (index < 0) {
	    ckfree((char *)elimMap) ;
	    return TCL_ERROR ;
	}
	elimMap[index] = 1 ;
    }
    elimCount = 0 ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	if (elimMap[c]) {
	    ++elimCount ;
	}
    }
    elimDegree = tupleHeading->degree - elimCount ;
    elimTupleHeading = tupleHeadingNew(elimDegree) ;

    where = 0 ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	if (!elimMap[c]) {
	    Ral_Attribute *attr = tupleHeading->attrVector + c ;

	    if (tupleHeadingCopy(interp, attr, attr + 1, elimTupleHeading,
		where++) != TCL_OK) {
		ckfree((char *)elimMap) ;
		tupleHeadingDelete(elimTupleHeading) ;
		return TCL_ERROR ;
	    }
	}
    }

    /*
     * HERE
     * This is probably wrong! If we are not eliminating any
     * identifying attributes, then we should keep the identifiers
     * of the original relation. Only if we are eliminating an attribute
     * that is part of an identifier do we have to resort to just
     * making every attribute part of a identifer. Same goes for
     * "project" command.
     */
    elimHeading = relationHeadingNew(elimTupleHeading, 1) ;
    relIdSetAllAttributes(elimHeading->idVector, elimTupleHeading->degree) ;
    elimRelation = relationNew(elimHeading) ;
    relationReserve(elimRelation, relation->cardinality) ;

    for (card = relation->cardinality, tupleVector = relation->tupleVector ;
	card > 0 ; --card, ++tupleVector) {
	Ral_Tuple *srcTuple ;
	int degree ;

	srcTuple = *tupleVector ;
	elimTuple = tupleNew(elimTupleHeading) ;
	where = 0 ;
	for (degree = 0 ; degree < tupleHeading->degree ; ++degree) {
	    if (!elimMap[degree]) {
		Tcl_IncrRefCount(elimTuple->values[where++] =
		    srcTuple->values[degree]) ;
	    }
	}
	if (relationAppendTuple(NULL, elimRelation, elimTuple) != TCL_OK) {
	    tupleDelete(elimTuple) ;
	    goto errorOut ;
	}
    }

    ckfree((char *)elimMap) ;
    Tcl_SetObjResult(interp, relationObjNew(elimRelation)) ;
    return TCL_OK ;

errorOut:
    ckfree((char *)elimMap) ;
    relationDelete(elimRelation) ;
    return TCL_ERROR ;
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
RelationExtendCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    Tcl_Obj *tupleObj ;
    Tcl_Obj *varNameObj ;
    Ral_Relation *extRelation ;
    Ral_TupleHeading *extTupleHeading ;
    int c ;
    Tcl_Obj *const*v ;
    int index ;
    int tupleIndex ;
    Ral_Tuple **tupleVector ;

    /* relation extend relationValue tupleVarName ?attr1 expr1...attrN exprN? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue tupleVarName ?attr1 expr1 ... attrN exprN?") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    varNameObj = *(objv + 3) ;

    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 4 ;
    objv += 4 ;
    if (objc % 2 != 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "attribute / expression arguments must be given in pairs", -1) ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(varNameObj) ;
    /*
     * Make the new relation adding the extended attributes
     */
    extRelation = relationDup(relation, objc / 2) ;
    extTupleHeading = extRelation->heading->tupleHeading ;

    index = relation->heading->tupleHeading->degree ;
    for (c = objc, v = objv ; c > 0 ; c -= 2, v += 2) {
	if (tupleHeadingInsertAttributeFromPair(interp, extTupleHeading, *v,
	    index++) != TCL_OK) {
	    goto errorOut ;
	}
    }

    for (tupleIndex = 0, tupleVector = relation->tupleVector ;
	tupleIndex < relation->cardinality ; ++tupleIndex, ++tupleVector) {
	Ral_Tuple *tuple ;
	Tcl_Obj *tupleObj ;
	Ral_Tuple *extTuple ;
	Ral_Attribute *attr ;

	tuple = *tupleVector ;
	tupleObj = tupleObjNew(tuple) ;
	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
	    goto errorOut ;
	}

	extTuple = extRelation->tupleVector[tupleIndex] ;

	index = relation->heading->tupleHeading->degree ;
	attr = extTupleHeading->attrVector + index ;
	for (c = objc, v = objv + 1 ; c > 0 ; c -= 2, v += 2) {
	    Tcl_Obj *exprResult ;

	    if (Tcl_ExprObj(interp, *v, &exprResult) != TCL_OK) {
		goto errorOut ;
	    }
	    if (attributeConvertValue(interp, attr, exprResult) != TCL_OK) {
		Tcl_DecrRefCount(exprResult) ;
		goto errorOut ;
	    }
	    assert(index < extTupleHeading->degree) ;
	    Tcl_IncrRefCount(extTuple->values[index++] = exprResult) ;

	    Tcl_DecrRefCount(exprResult) ;
	    ++attr ;
	}
    }

    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_SetObjResult(interp, relationObjNew(extRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    relationDelete(extRelation) ;
    return TCL_ERROR ;
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
    Ral_Tuple **tupleVector ;
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
	Tcl_Obj *tupleObj ;

	tupleObj = tupleObjNew(*tupleVector) ;
	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
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
RelationIntersectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Ral_Relation *interRel ;

    /* relation intersect relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
	return TCL_ERROR ;
    }
    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;
    interRel = relationNew(r1->heading) ;

    objc -= 3 ;
    objv += 3 ;
    while (objc-- > 0) {
	Tcl_Obj *r2Obj ;
	Ral_Relation *r2 ;
	Ral_Tuple **tv ;
	Ral_Tuple **last ;
	Ral_AttrOrderMap *order ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	    relationDelete(interRel) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	if (relationHeadingCmpTypes(interp, interRel->heading, r2->heading)
	    != TCL_OK) {
	    relationDelete(interRel) ;
	    return TCL_ERROR ;
	}

	order = attrOrderMapGenerateOrdering(r1->heading->tupleHeading,
	    r2->heading->tupleHeading) ;

	relationReserve(interRel, max(r1->cardinality, r2->cardinality)) ;

	last = r2->tupleVector + r2->cardinality ;
	for (tv = r2->tupleVector ; tv != last ; ++tv) {
	    Ral_Tuple *tuple = *tv ;

	    if (relationFindIndexInOrder(r1, 0, order, tuple) >= 0) {
		relationAppendReorderedTuple(NULL, interRel, tuple, order) ;
	    }
	}

	ckfree((char *)order) ;
	r1 = interRel ;
    }

    Tcl_SetObjResult(interp, relationObjNew(interRel)) ;
    return TCL_OK ;
}

static int
RelationIsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    enum CmpOperators {
	CMPOP_EQUAL,
	CMPOP_NOTEQUAL,
	CMPOP_PROPERSUBSETOF,
	CMPOP_PROPERSUPERSETOF,
	CMPOP_SUBSETOF,
	CMPOP_SUPERSETOF,
    } ;
    static const char *operators[] = {
	"equal",
	"notequal",
	"propersubsetof",
	"propersupersetof",
	"subsetof",
	"supersetof",
	NULL
    } ;

    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    int opIndex ;
    enum CmpOperators op ;
    Ral_Relation *cmpSrcRel ;
    Ral_Relation *cmpToRel ;
    Ral_AttrOrderMap *orderMap ;
    int matches ;
    Ral_Tuple **tupleVector ;
    Ral_Tuple **last ;
    int result ;

    /* relation is relation1 compareop relation2 */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation1 compareop relation2") ;
	return TCL_ERROR ;
    }
    r1Obj = *(objv + 2) ;
    r2Obj = *(objv + 4) ;

    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObj(interp, *(objv + 3), operators, "compareop", 0,
	&opIndex) != TCL_OK) {
	return TCL_ERROR ;
    }

    op = (enum CmpOperators)opIndex ;
    switch (op) {
    case CMPOP_EQUAL:
    case CMPOP_NOTEQUAL:
    case CMPOP_PROPERSUPERSETOF:
    case CMPOP_SUPERSETOF:
	cmpSrcRel = r1Obj->internalRep.otherValuePtr ;
	cmpToRel = r2Obj->internalRep.otherValuePtr ;
	break ;

    case CMPOP_PROPERSUBSETOF:
    case CMPOP_SUBSETOF:
	cmpSrcRel = r2Obj->internalRep.otherValuePtr ;
	cmpToRel = r1Obj->internalRep.otherValuePtr ;
	break ;

    default:
	Tcl_Panic("relation is: unknown relational comparison operator") ;
    }

    if (relationHeadingCmpTypes(interp, cmpSrcRel->heading, cmpToRel->heading)
	!= TCL_OK) {
	return TCL_ERROR ;
    }

    orderMap = attrOrderMapGenerateOrdering(cmpSrcRel->heading->tupleHeading,
	cmpToRel->heading->tupleHeading) ;

    /*
     * Loop through the "to" relation and find the number of matching
     * tuples in the "src" relation.
     */
    matches = 0 ;
    for (tupleVector = cmpToRel->tupleVector,
	last = tupleVector + cmpToRel->cardinality ; tupleVector != last ;
	++tupleVector) {
	Ral_Tuple *toTuple = *tupleVector ;
	int srcIndex ;

	srcIndex = relationFindIndexInOrder(cmpSrcRel, 0, orderMap, toTuple) ;
	if (srcIndex >= 0) {
	    assert(srcIndex < cmpSrcRel->cardinality) ;

	    matches += tupleEqual(cmpSrcRel->tupleVector[srcIndex], toTuple) ;
	}
    }
    ckfree((char *)orderMap) ;

    result = matches == cmpToRel->cardinality ;
    switch (op) {
    case CMPOP_EQUAL:
	result = result && cmpSrcRel->cardinality == cmpToRel->cardinality ;
	break ;

    case CMPOP_NOTEQUAL:
	result = !(result && cmpSrcRel->cardinality == cmpToRel->cardinality) ;
	break ;

    case CMPOP_PROPERSUPERSETOF:
    case CMPOP_PROPERSUBSETOF:
	result = result && cmpSrcRel->cardinality > cmpToRel->cardinality ;
	break ;

    case CMPOP_SUPERSETOF:
    case CMPOP_SUBSETOF:
	result = result && cmpSrcRel->cardinality >= cmpToRel->cardinality ;
	break ;

    default:
	Tcl_Panic("relation is: unknown relational comparison operator") ;
    }

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(result)) ;
    return TCL_OK ;
}

static int
RelationJoinCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation *r2 ;
    Ral_Relation *joinRel ;
    int result = TCL_ERROR ;

    /* relation join relation1 relation2 ?joinAttrs1 joinAttrs2 ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?joinAttrs1 joinAttrs2 ... ?") ;
	return TCL_ERROR ;
    }
    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    objc -= 4 ;
    objv += 4 ;

#if 0
    if (objc == 0) {
	/*
	 * Join on the common attributes. Worst case count is the
	 * min of the attributes in the relations.
	 */
	nJoinAttrs = r1->heading.attrCount < r2->heading.attrCount ?
	    r1->heading.attrCount : r2->heading.attrCount ;
    } else {
	nJoinAttrs = objc ;
    }

    rj = relJoinNew(interp, r1, r2, nJoinAttrs) ;
    if (rj == NULL) {
	return TCL_ERROR ;
    }
    if (objc == 0) {
	relJoinFindCommonAttrs(rj) ;
    } else {
	if (relJoinMapJoinAttrs(interp, rj, objc, objv) != TCL_OK) {
	    goto errorOut ;
	}
    }

    if (relJoinFindMatches(interp, rj) != TCL_OK) {
	goto errorOut ;
    }

    joinRel = relJoinComposeMatches(interp, rj) ;
    ckfree((char *)rj) ;

    if (joinRel) {
	Tcl_SetObjResult(interp, relMakeObj(joinRel)) ;
	result = TCL_OK ;
    }

    return result ;

errorOut:
    ckfree((char *)rj) ;
#endif
    return TCL_ERROR ;
}

/* 
 * Create a Tcl List for a relation that has a single column.
 */
static int
RelationListCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation list relationValue */
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;
    Tcl_Obj *listObj ;
    Ral_Tuple **tv ;
    Ral_Tuple **last ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;
    if (relation->heading->tupleHeading->degree != 1) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp),
	    "relation must have degree of one", -1) ;
	return TCL_ERROR ;
    }

    listObj = Tcl_NewListObj(0, NULL) ;
    for (tv = relation->tupleVector, last = tv + relation->cardinality ;
	tv != last ; ++tv) {
	Ral_Tuple *tuple = *tv ;

	if (Tcl_ListObjAppendElement(interp, listObj, tuple->values[0])
	    != TCL_OK) {
	    goto errorOut ;
	}
    }

    Tcl_SetObjResult(interp, listObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(listObj) ;
    return TCL_ERROR ;
}

static int
RelationMinusCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation *r2 ;
    Ral_Relation *diffRel ;
    Ral_AttrOrderMap *order ;
    Ral_Tuple **tv ;
    Ral_Tuple **last ;

    /* relation minus relation1 relation2 */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation1 relation2") ;
	return TCL_ERROR ;
    }
    r1Obj = *(objv + 2) ;
    r2Obj = *(objv + 3) ;

    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;
    r2 = r2Obj->internalRep.otherValuePtr ;
    if (relationHeadingCmpTypes(interp, r1->heading, r2->heading) != TCL_OK) {
	return TCL_ERROR ;
    }

    diffRel = relationNew(r1->heading) ;
    relationReserve(diffRel, r1->cardinality) ; /* worst case */

    order = attrOrderMapGenerateOrdering(r2->heading->tupleHeading,
	diffRel->heading->tupleHeading) ;

    for (tv = r1->tupleVector, last = tv + r1->cardinality ; tv != last
	; ++tv) {
	Ral_Tuple *tuple = *tv ;

	if (relationFindIndexInOrder(r2, 0, order, tuple) < 0 &&
	    relationAppendTuple(interp, diffRel, tuple) != TCL_OK) {
	    return TCL_ERROR ;
	}
    }

    ckfree((char *)order) ;

    Tcl_SetObjResult(interp, relationObjNew(diffRel)) ;
    return TCL_OK ;
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
RelationProjectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    Ral_Relation *projRelation ;
    Ral_RelationHeading *heading ;
    Ral_TupleHeading *tupleHeading ;
    Ral_TupleHeading *projTupleHeading ;
    Ral_RelationHeading *projHeading ;
    int *valueMap ;
    int *vm ;
    int c ;
    Tcl_Obj *const*v ;
    int card ;
    Ral_Tuple **tupleVector ;
    Ral_Tuple *projTuple ;

    /* relation project relationValue ?attr ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue ?attr ... ?") ;
	return TCL_ERROR ;
    }
    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;
    tupleHeading = heading->tupleHeading ;

    objc -= 3 ;
    objv += 3 ;

    valueMap = (int *)ckalloc(objc * sizeof(*valueMap)) ;
    projTupleHeading = tupleHeadingNew(objc) ;

    vm = valueMap ;
    for (c = 0, v = objv ; c < objc ; ++c, ++v) {
	Ral_Attribute *attr ;
	int index ;

	attr = tupleHeadingFindAttribute(interp, tupleHeading, *v, &index) ;
	if (attr == NULL ||
	    tupleHeadingCopy(interp, attr, attr + 1, projTupleHeading, c)
		!= TCL_OK) {
	    ckfree((char *)valueMap) ;
	    tupleHeadingDelete(projTupleHeading) ;
	    return TCL_ERROR ;
	}
	*vm++ = index ;
    }

    /*
     * HERE
     * This is probably wrong! If we are not eliminating any
     * identifying attributes, then we should keep the identifiers
     * of the original relation. Only if we are eliminating an attribute
     * that is part of an identifier do we have to resort to just
     * making every attribute part of a identifer. Same goes for
     * "eliminate" command.
     */
    projHeading = relationHeadingNew(projTupleHeading, 1) ;
    relIdSetAllAttributes(projHeading->idVector, projTupleHeading->degree) ;
    projRelation = relationNew(projHeading) ;
    relationReserve(projRelation, relation->cardinality) ;

    for (card = relation->cardinality, tupleVector = relation->tupleVector ;
	card > 0 ; --card, ++tupleVector) {
	Ral_Tuple *srcTuple = *tupleVector ;
	int degree ;

	projTuple = tupleNew(projTupleHeading) ;
	for (degree = 0, vm = valueMap ; degree < projTupleHeading->degree ;
	    ++degree, ++vm) {
	    Tcl_IncrRefCount(projTuple->values[degree] =
		srcTuple->values[*vm]) ;
	}
	if (relationAppendTuple(NULL, projRelation, projTuple) != TCL_OK) {
	    tupleDelete(projTuple) ;
	}
    }

    ckfree((char *)valueMap) ;
    Tcl_SetObjResult(interp, relationObjNew(projRelation)) ;
    return TCL_OK ;
}

static int
RelationRenameCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;
    Ral_Relation *newRelation ;

    /* relation rename relationValue ?oldname newname ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue ?oldname newname ... ?") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relationObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	static const char errStr[] =
	    "oldname / newname arguments must be given in pairs" ;
	Tcl_SetStringObj(Tcl_GetObjResult(interp), errStr, sizeof(errStr) - 1) ;
	return TCL_ERROR ;
    }

    newRelation = relationDup(relation, 0) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	if (tupleHeadingRenameAttribute(interp,
	    newRelation->heading->tupleHeading, objv[0], objv[1]) != TCL_OK) {
	    relationDelete(newRelation) ;
	}
    }

    Tcl_SetObjResult(interp, relationObjNew(newRelation)) ;
    return TCL_OK ;
}

static int
RelationRestrictCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *relation ;
    Tcl_Obj *exprObj ;
    Tcl_Obj *varName ;
    Ral_Relation *newRelation ;
    int card ;
    Ral_Tuple **tupleVector ;

    /* relation restrict relValue tupleVarName expr */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue tupleVarName expr") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    varName = *(objv + 3) ;
    exprObj = *(objv + 4) ;

    newRelation = relationNew(relation->heading) ;
    relationReserve(newRelation, relation->cardinality) ;

    Tcl_IncrRefCount(exprObj) ;
    Tcl_IncrRefCount(varName) ;

    for (card = relation->cardinality, tupleVector = relation->tupleVector ;
	card > 0 ; --card, ++tupleVector) {
	Ral_Tuple *tuple = *tupleVector ;
	Tcl_Obj *tupleObj = tupleObjNew(tuple) ;
	int boolValue ;

	if (Tcl_ObjSetVar2(interp, varName, NULL, tupleObj, TCL_LEAVE_ERR_MSG)
	    == NULL) {
	    goto errorOut ;
	}

	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    goto errorOut ;
	}
	if (boolValue &&
	    relationAppendTuple(interp, newRelation, tuple) != TCL_OK) {
	    goto errorOut ;
	}
    }

    Tcl_UnsetVar(interp, Tcl_GetString(varName), 0) ;
    Tcl_DecrRefCount(exprObj) ;
    Tcl_DecrRefCount(varName) ;
    Tcl_SetObjResult(interp, relationObjNew(newRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varName), 0) ;
    Tcl_DecrRefCount(exprObj) ;
    Tcl_DecrRefCount(varName) ;
    relationDelete(newRelation) ;
    return TCL_ERROR ;
}

/*
 * HERE
 * Allow for multiple "summaries", i.e. multiple "attr-type cmd initValue"
 * argument sets.
 */
static int
RelationSummarizeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Tcl_Obj *perObj ;
    Tcl_Obj *attrObj ;
    Tcl_Obj *cmdObj ;
    int cmdLength ;
    Tcl_Obj *initObj ;
    Ral_Relation *relation ;
    Ral_TupleHeading *tupleHeading ;
    Ral_Relation *perRelation ;
    Ral_TupleHeading *perTupleHeading ;
    Ral_TupleHeading *sumTupleHeading ;
    Ral_RelationHeading *sumHeading ;
    Ral_Relation *sumRelation ;
    Ral_Attribute *sumAttribute ;
    Tcl_Obj **summaryVector ;
    Tcl_HashTable perHashTable ;
    int c ;
    int rCnt = 0 ;

    /* relation summarize relationValue perRelation attr-type cmd initValue */
    if (objc != 7) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue perRelation attr-type cmd initValue") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    perObj = *(objv + 3) ;
    attrObj = *(objv + 4) ;
    cmdObj = *(objv + 5) ;
    initObj = *(objv + 6) ;

    if (Tcl_ListObjLength(interp, cmdObj, &cmdLength) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    tupleHeading = relation->heading->tupleHeading ;

    if (Tcl_ConvertToType(interp, perObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    perRelation = perObj->internalRep.otherValuePtr ;
    perTupleHeading = perRelation->heading->tupleHeading ;

    /*
     * The "per" relation must be a subset of the summarized relation.
     */
    if (tupleHeadingCompare(perTupleHeading, tupleHeading) !=
	perTupleHeading->degree) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	"the \"per\" relation heading must be a subset of summarized relation",
	    -1) ;
	return TCL_ERROR ;
    }

    /*
     * Construct the heading for the result. It the heading of the
     * "per" relation plus the summary attribute.
     */
    sumTupleHeading = tupleHeadingNew(perTupleHeading->degree + 1) ;
    if (tupleHeadingCopy(interp, perTupleHeading->attrVector,
	    perTupleHeading->attrVector + perTupleHeading->degree,
	    sumTupleHeading, 0) != TCL_OK ||
	tupleHeadingInsertAttributeFromPair(interp, sumTupleHeading, attrObj,
	    perTupleHeading->degree) != TCL_OK) {
	tupleHeadingDelete(sumTupleHeading) ;
	return TCL_ERROR ;
    }

    sumAttribute = sumTupleHeading->attrVector + perTupleHeading->degree ;
    if (attributeConvertValue(interp, sumAttribute, initObj) != TCL_OK) {
	tupleHeadingDelete(sumTupleHeading) ;
	return TCL_ERROR ;
    }

    sumHeading = relationHeadingNew(sumTupleHeading, 1) ;
    relIdSetAllAttributes(sumHeading->idVector, sumTupleHeading->degree) ;
    sumRelation = relationNew(sumHeading) ;
    relationReserve(sumRelation, perRelation->cardinality) ;

    /*
     * Create a vector containing a copy of initialization object. This
     * will be used to accumulate the result of the command evaluation.
     */
    summaryVector = (Tcl_Obj **)ckalloc(perRelation->cardinality *
	sizeof(*summaryVector)) ;
    for (c = 0 ; c < perRelation->cardinality ; ++c) {
	Tcl_IncrRefCount(summaryVector[c] = initObj) ;
    }

    /*
     * Create an index for the "per" relation that will be used to
     * find the matching tuples in the summarized relation.
     */
    Tcl_InitHashTable(&perHashTable, TCL_STRING_KEYS) ;
    relationIndexAllAttributes(perRelation, &perHashTable) ;
    /*
     * Take a copy of the command so that we can hold on to it and
     * modify it by adding arguments.
     */
    cmdObj = Tcl_DuplicateObj(cmdObj) ;
    Tcl_IncrRefCount(cmdObj) ;
    /*
     * Loop through each tuple of the summarized relation and find
     * the match in the "per" relation. If there is a match, then
     * run the command and place the result back in the summary vector.
     */
    for (c = 0 ; c < relation->cardinality ; ++c) {
	Ral_Tuple *tuple ;
	int index ;
	Tcl_Obj *argVect[2] ;
	Tcl_Obj *resultObj ;

	tuple = relation->tupleVector[c] ;
	index = relationFindCorresponding(tuple, perTupleHeading,
	    &perHashTable) ;
	if (index < 0) {
	    continue ;
	}
	/*
	 * Append to arguments to the command:
	 * (1) The summary value
	 * (2) The tuple
	 */
	argVect[0] = summaryVector[index] ;
	argVect[1] = tupleObjNew(tuple) ;
	if (Tcl_ListObjReplace(interp, cmdObj, cmdLength, rCnt, 2, argVect)
	    != TCL_OK || Tcl_EvalObjEx(interp, cmdObj, 0)) {
	    goto errorOut2 ;
	}
	rCnt = 2 ;

	resultObj = Tcl_GetObjResult(interp) ;
	if (attributeConvertValue(interp, sumAttribute, resultObj) != TCL_OK) {
	    goto errorOut2 ;
	}

	Tcl_DecrRefCount(summaryVector[index]) ;
	Tcl_IncrRefCount(summaryVector[index] = resultObj) ;
    }

    /*
     * At this point the summary vector contains the values for the
     * additional attribute. Now we put together the summary relation
     * by extending the "per" relation with the summary values.
     */
    for (c = 0 ; c < perRelation->cardinality ; ++c) {
	Ral_Tuple *tuple ;
	Ral_Tuple *sumTuple ;
	int i ;

	tuple = perRelation->tupleVector[c] ;
	sumTuple = tupleNew(sumTupleHeading) ;
	for (i = 0 ; i < perTupleHeading->degree ; ++i) {
	    Tcl_IncrRefCount(sumTuple->values[i] = tuple->values[i]) ;
	}
	assert(i < sumTupleHeading->degree) ;
	Tcl_IncrRefCount(sumTuple->values[i] = summaryVector[c]) ;

	if (relationAppendTuple(interp, sumRelation, sumTuple) != TCL_OK) {
	    tupleDelete(sumTuple) ;
	    goto errorOut2 ;
	}
    }

    Tcl_DecrRefCount(cmdObj) ;
    Tcl_DeleteHashTable(&perHashTable) ;
    ckfree((char *)summaryVector) ;
    Tcl_SetObjResult(interp, relationObjNew(sumRelation)) ;
    return TCL_OK ;

errorOut2:
    Tcl_DecrRefCount(cmdObj) ;
errorOut:
    Tcl_DeleteHashTable(&perHashTable) ;
    for (c = 0 ; c < perRelation->cardinality ; ++c) {
	Tcl_DecrRefCount(summaryVector[c]) ;
    }
    ckfree((char *)summaryVector) ;
    relationDelete(sumRelation) ;
    return TCL_ERROR ;
}

static int
RelationTimesCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation *r2 ;
    Ral_TupleHeading *tupleHeading ;
    Ral_RelationHeading *relationHeading ;
    Ral_Relation *product ;
    int r1c ;
    Tcl_Obj **r1v ;
    int r2c ;
    Tcl_Obj **r2v ;

    /* relation times relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation1 relation2 ? ... ?") ;
	return TCL_ERROR ;
    }
    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    tupleHeading = tupleHeadingUnion(interp, r1->heading->tupleHeading,
	r2->heading->tupleHeading) ;
    if (tupleHeading == NULL) {
	return TCL_ERROR ;
    }

    relationHeading = relationHeadingNew(tupleHeading, 1) ;
    /*
     * One identifier consisting of all the attributes.
     */
    relIdSetAllAttributes(relationHeading->idVector, tupleHeading->degree) ;

    product = relationNew(relationHeading) ;
    relationReserve(product, r1->cardinality * r2->cardinality) ;

    if (relationMultTuples(interp, product, r1, r2) != TCL_OK) {
	goto errorOut ;
    }

    objc -= 4 ;
    objv += 4 ;
    while (objc-- > 0) {
	int i ;

	r1 = product ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	    goto errorOut ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	tupleHeading = tupleHeadingUnion(interp, r1->heading->tupleHeading,
	    r2->heading->tupleHeading) ;
	if (tupleHeading == NULL) {
	    return TCL_ERROR ;
	}

	/*
	 * One identifier consisting of all the attributes.
	 */
	relationHeading = relationHeadingNew(tupleHeading, 1) ;
	relIdCtor(relationHeading->idVector, tupleHeading->degree) ;
	for (i = 0 ; i < tupleHeading->degree ; ++i) {
	    relationHeading->idVector->attrIndices[i] = i ;
	}

	product = relationNew(relationHeading) ;
	relationReserve(product, r1->cardinality * r2->cardinality) ;

	if (relationMultTuples(interp, product, r1, r2) != TCL_OK) {
	    goto errorOut ;
	}

	relationDelete(r1) ;
    }

    Tcl_SetObjResult(interp, relationObjNew(product)) ;
    return TCL_OK ;

errorOut2:
    relationDelete(r1) ;
errorOut:
    relationDelete(product) ;
    return TCL_ERROR ;
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

    Tcl_SetObjResult(interp, tupleObjNew(*relation->tupleVector)) ;
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

static int
RelationUnionCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Ral_Relation *unionRel ;

    /* relation union relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
	return TCL_ERROR ;
    }
    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;
    unionRel = relationDup(r1, 0) ;

    objc -= 3 ;
    objv += 3 ;
    while (objc-- > 0) {
	Tcl_Obj *r2Obj ;
	Ral_Relation *r2 ;
	Ral_Tuple **tv ;
	Ral_Tuple **last ;
	Ral_AttrOrderMap *order ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	    relationDelete(unionRel) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	if (relationHeadingCmpTypes(interp, unionRel->heading, r2->heading)
	    != TCL_OK) {
	    relationDelete(unionRel) ;
	    return TCL_ERROR ;
	}

	order = attrOrderMapGenerateOrdering(unionRel->heading->tupleHeading,
	    r2->heading->tupleHeading) ;

	relationReserve(unionRel, r2->cardinality) ;

	for (tv = r2->tupleVector, last = tv + r2->cardinality ; tv != last ;
	    ++tv) {
	    /*
	     * Just do the insert. Use NULL so as not to clutter the
	     * interpreter result. Duplicates will not be reinserted.
	     */
	    relationAppendReorderedTuple(NULL, unionRel, *tv, order) ;
	}

	ckfree((char *)order) ;
    }

    Tcl_SetObjResult(interp, relationObjNew(unionRel)) ;
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
	RELATION_CARDINALITY,
	RELATION_DEGREE,
	RELATION_DIVIDE,
	RELATION_ELIMINATE,
	RELATION_EMPTY,
	RELATION_EMPTYOF,
	RELATION_EXTEND,
	RELATION_FOREACH,
	RELATION_GROUP,
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
	RELATION_SUMMARIZE,
	RELATION_TCLOSE,
	RELATION_TIMES,
	RELATION_TUPLE,
	RELATION_TYPEOF,
	RELATION_UNGROUP,
	RELATION_UNION,
    } ;
    static const char *subCmds[] = {
	"cardinality",
	"degree",
	"divide",
	"eliminate",
	"empty",
	"emptyof",
	"extend",
	"foreach",
	"group",
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
	"summarize",
	"tclose",
	"times",
	"tuple",
	"typeof",
	"ungroup",
	"union",
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

    switch ((enum RelationSubCmds)index) {
    case RELATION_CARDINALITY:
	return RelationCardinalityCmd(interp, objc, objv) ;

    case RELATION_DEGREE:
	return RelationDegreeCmd(interp, objc, objv) ;

    case RELATION_DIVIDE:
	return RelationDivideCmd(interp, objc, objv) ;

    case RELATION_ELIMINATE:
	return RelationEliminateCmd(interp, objc, objv) ;

    case RELATION_EMPTY:
	return RelationEmptyCmd(interp, objc, objv) ;

    case RELATION_EMPTYOF:
	return RelationEmptyofCmd(interp, objc, objv) ;

    case RELATION_EXTEND:
	return RelationExtendCmd(interp, objc, objv) ;

    case RELATION_FOREACH:
	return RelationForeachCmd(interp, objc, objv) ;

#if 0
    case RELATION_GROUP:
	return RelationGroupCmd(interp, objc, objv) ;
#endif

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

    case RELATION_NOTEMPTY:
	return RelationNotemptyCmd(interp, objc, objv) ;

    case RELATION_PROJECT:
	return RelationProjectCmd(interp, objc, objv) ;

    case RELATION_RENAME:
	return RelationRenameCmd(interp, objc, objv) ;

    case RELATION_RESTRICT:
	return RelationRestrictCmd(interp, objc, objv) ;

#if 0
    case RELATION_SEMIJOIN:
	return RelationSemijoinCmd(interp, objc, objv) ;

    case RELATION_SEMIMINUS:
	return RelationSemiminusCmd(interp, objc, objv) ;
#endif

    case RELATION_SUMMARIZE:
	return RelationSummarizeCmd(interp, objc, objv) ;

#if 0
    case RELATION_TCLOSE:
	return RelationTcloseCmd(interp, objc, objv) ;
#endif

    case RELATION_TIMES:
	return RelationTimesCmd(interp, objc, objv) ;

    case RELATION_TUPLE:
	return RelationTupleCmd(interp, objc, objv) ;

    case RELATION_TYPEOF:
	return RelationTypeofCmd(interp, objc, objv) ;

#if 0
    case RELATION_UNGROUP:
	return RelationUngroupCmd(interp, objc, objv) ;
#endif

    case RELATION_UNION:
	return RelationUnionCmd(interp, objc, objv) ;

    default:
	Tcl_Panic("relation: unexpected relation subcommand value") ;
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
