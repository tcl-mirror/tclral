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
$Revision: 1.18 $
$Date: 2004/08/15 22:55:21 $
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
#define	max(a, b)    ((a) > (b) ? (a) : (b))
#define	min(a, b)    ((a) < (b) ? (a) : (b))
#define	countof(a)  (sizeof(a) / sizeof(a[0]))

/*
TYPE DEFINITIONS
*/
/*
 * Used in the various command functions to map subcommand names
 * to the corresponding functions that implement the subcommands.
 */
struct cmdMap {
    const char *cmdName ;
    int (*const cmdFunc)(Tcl_Interp *, int, Tcl_Obj *const*) ;
} ;

/*
 * There are a number of places where fixed sized integer vectors
 * are needed.
 */
typedef struct Ral_FixedIntVector {
    int count ;
    int *vector ;
} Ral_FixedIntVector ;

/*
 * There are also a number of places where a mapping between indices
 * is needed. These are held as a vector of an array of two integers.
 */
typedef struct Ral_VarIntMap {
    int allocated ;
    int count ;
    struct intmap {
	int index1 ;
	int index2 ;
    } *vector ;
} Ral_VarIntMap ;

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
 * In this case the attributes must be reordered to match the order of the
 * target relation heading.
 */

typedef struct Ral_AttrOrderMap {
    int isInOrder ;	/* if the order happens to match, this is non-zero */
    Ral_FixedIntVector ordering ;
} Ral_AttrOrderMap ;

/*
 * A relation join map is a temporary data structure used to accumulate the
 * information for performing a join. This consists of two parts.  First, a
 * mapping of the attributes across which the join is performed and second a
 * map showing the tuples that match across the join attributes.
 */
typedef struct Ral_RelationJoinMap {
    Ral_VarIntMap attrMap ;	/* Attribute map that gives
				 * the attribute indices in the two relations
				 * across which the join is performed. */
    Ral_VarIntMap tupleMap ;	/* the tuple map giving the index in one
				 * relation that matches that in another
				 * relation */
} Ral_RelationJoinMap ;

/*
 * A Tuple Heading is a set of Heading Attributes. The attributes are stored in
 * an array. A hash table is used to map attribute names to an index in the
 * attribute array. The "nameMap" hash table has as a key the attribute name
 * and as a value the index into the attribute array.
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
 * The keyword distinguishes the string as a Tuple.  The heading is as
 * described above.  The heading consists of a list Attribute Name and Data
 * Type pairs.  The value definition is also a list consisting of Attribute
 * Name / Attribute Value pairs.
 * E.G.
 *	{Tuple {Name string Street int Wage double}\
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
 * A Relation Heading is a Tuple Heading with the addition of identifiers.
 * Identifiers are sub sets of attributes for which the tuples may not have
 * duplicated values.
 */

typedef struct Ral_RelationHeading {
    int refCount ;
    Ral_TupleHeading *tupleHeading ;
    int idCount ;
    Ral_FixedIntVector *idVector ;
} Ral_RelationHeading ;

/*
 * A Relation type consists of a heading with a body.  A body consists
 * of zero or more tuples.

 * The external string representation of a "Relation" is a specially
 * formatted list. The list consists of four elements.
 *
 * 1. The keyword "Relation".
 *
 * 2. A tuple heading. A tuple heading is a list of Name / Type pairs as
 *    described above.
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
 * be used for indices into the values of a Relation.
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
static int tupleHeadingFindIndex(Tcl_Interp *, Ral_TupleHeading *, Tcl_Obj *) ;

static int relationHeadingEqual(Ral_RelationHeading *, Ral_RelationHeading *) ;
static void relationHeadingReference(struct Ral_RelationHeading *) ;
static void relationHeadingUnreference(struct Ral_RelationHeading *) ;
static Ral_RelationHeading *relationHeadingNewFromObjs(Tcl_Interp *,
    Tcl_Obj *, Tcl_Obj *) ;
static Tcl_Obj *relationHeadingObjNew(Tcl_Interp *, Ral_RelationHeading *) ;

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
static const char ral_version[] = "0.6" ;
static const char ral_rcsid[] =
    "$Id: ral.c,v 1.18 2004/08/15 22:55:21 mangoa01 Exp $" ;
static const char ral_copyright[] =
    "This software is copyrighted 2004 by G. Andrew Mangogna."
    "Terms and conditions for use are distributed with the source code." ;

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
    return memcmp(s1, s2, l1) == 0 ;
}

/*
 * compare two integers indirectly. Used by "qsort".
 */
static int
int_ind_compare(
    const void *m1,
    const void *m2)
{
    const int *i1 = m1 ;
    const int *i2 = m2 ;

    return *i1 - *i2 ;
}

/*
 * ======================================================================
 * Fixed Length Integer Vectors Functions
 * ======================================================================
 */

static void
fixedIntVectorCtor(
    Ral_FixedIntVector *vect,
    int count)
{
    vect->count = count ;
    vect->vector = count == 0 ? NULL :
	(int *)ckalloc(count * sizeof(*vect->vector)) ;
}

static void
fixedIntVectorDtor(
    Ral_FixedIntVector *vect)
{
    if (vect->vector) {
	ckfree((char *)vect->vector) ;
    }
}

static void
fixedIntVectorCopy(
    Ral_FixedIntVector *src,
    Ral_FixedIntVector *dst)
{
    fixedIntVectorCtor(dst, src->count) ;
    memcpy(dst->vector, src->vector, dst->count * sizeof(*dst->vector)) ;
}

static void
fixedIntVectorSort(
    Ral_FixedIntVector *vect)
{
    qsort(vect->vector, vect->count, sizeof(*vect->vector), int_ind_compare) ;
}

static int
fixedIntVectorEqual(
    Ral_FixedIntVector *v1,
    Ral_FixedIntVector *v2)
{
    if (v1 == v2) {
	return 1 ;
    }
    if (v1->count != v2->count) {
	return 0 ;
    }
    /*
     * This works only if the integer vectors are in sorted order, as is the
     * case when the vector contains a set of indices to identifying
     * attributes.
     */
    return memcmp(v1->vector, v2->vector, v1->count * sizeof(*v1->vector))
	== 0 ;
}

/*
 * Is v1 a subset (perhaps improper subset) of v2.
 */
static int
fixedIntVectorIsSubsetOf(
    Ral_FixedIntVector *v1,
    Ral_FixedIntVector *v2)
{
    if (v1 == v2) {
	return 1 ;
    }
    if (v1->count > v2->count) {
	return 0 ;
    }
    /*
     * Index vector is sorted.
     */
    return memcmp(v1->vector, v2->vector, v1->count * sizeof(*v1->vector))
	== 0 ;
}

/*
 * Create a vector that holds the identity mapping.
 */
static void
fixedIntVectorIdentityMap(
    Ral_FixedIntVector *fiv,
    int degree)
{
    int index ;

    fixedIntVectorCtor(fiv, degree) ;
    for (index = 0 ; index < degree ; ++index) {
	fiv->vector[index] = index ;
    }
}

/*
 * ======================================================================
 * Variable Length Integer Map Functions
 * ======================================================================
 */
static void
varIntMapCtor(
    Ral_VarIntMap *map,
    int maxNum)
{
    map->allocated = maxNum ;
    map->count = 0 ;
    map->vector = maxNum == 0 ? NULL :
	(struct intmap *)ckalloc(maxNum * sizeof(*map->vector)) ;
}

static void
varIntMapDtor(
    Ral_VarIntMap *map)
{
    if (map->vector) {
	ckfree((char *)map->vector) ;
    }
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
attributeTypeEqual(
    Ral_Attribute *a1,
    Ral_Attribute *a2)
{
    int result ;

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

static int
attributeEqual(
    Ral_Attribute *a1,
    Ral_Attribute *a2)
{
    if (a1 == a2) {
	return 1 ;
    }
    if (!(ObjsEqual(a1->name, a2->name) && a1->attrType == a2->attrType)) {
	return 0 ;
    }

    return attributeTypeEqual(a1, a2) ;
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
    int typec ;
    Tcl_Obj **typev ;
    const char *typeName ;

    if (Tcl_ListObjGetElements(interp, dataType, &typec, &typev)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    typeName = Tcl_GetString(*typev) ;
    if (typec == 1) {
	Tcl_ObjType *tclType = Tcl_GetObjType(typeName) ;

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
	if (strcmp("Tuple", typeName) == 0) {
	    Ral_TupleHeading *heading =
		tupleHeadingNewFromString(interp, *(typev + 1)) ;

	    if (heading) {
		attributeCtorTupleType(attr, attrName, heading) ;
	    } else {
		return TCL_ERROR ;
	    }
	} else {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "expected Tuple type, but got, \"", typeName, "\"",
		    NULL) ;
	    }
	    return TCL_ERROR ;
	}
    } else if (typec == 3) {
	if (strcmp("Relation", typeName) == 0) {
	    Ral_RelationHeading *heading =
		relationHeadingNewFromObjs(interp, *(typev + 1),
		*(typev +2)) ;

	    if (heading) {
		attributeCtorRelationType(attr, attrName, heading) ;
	    } else {
		return TCL_ERROR ;
	    }
	} else {
	    if (interp) {
		Tcl_ResetResult(interp) ;
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "expected Relation type, but got, \"", typeName, "\"",
		    NULL) ;
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

static void
attributeAppendIdAttrs(
    Ral_Attribute *tupleAttrs,
    Ral_FixedIntVector *relId,
    Tcl_Obj *resultObj)
{
    int i ;

    for (i = 0 ; i < relId->count ; ++i) {
	if (i != 0) {
	    Tcl_AppendStringsToObj(resultObj, ", ", NULL) ;
	}
	Tcl_AppendStringsToObj(resultObj,
	    Tcl_GetString(tupleAttrs[relId->vector[i]].name), NULL) ;
    }
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

    map = (Ral_AttrOrderMap *)ckalloc(sizeof(*map)) ;
    map->isInOrder = 0 ;
    fixedIntVectorCtor(&map->ordering, attrCount) ;

    return map ;
}

static void
attrOrderMapDelete(
    Ral_AttrOrderMap *map)
{
    fixedIntVectorDtor(&map->ordering) ;
    ckfree((char *)map) ;
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
    order = map->ordering.vector ;

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
 * Relation Join Map Functions
 * ======================================================================
 */


static Ral_RelationJoinMap *
relationJoinMapNew(
    int nAttrs,
    int nTuples)
{
    Ral_RelationJoinMap *map ;

    map = (Ral_RelationJoinMap *)ckalloc(sizeof(*map)) ;
    varIntMapCtor(&map->attrMap, nAttrs) ;
    varIntMapCtor(&map->tupleMap, nTuples) ;

    return map ;
}

static void
relationJoinMapDelete(
    Ral_RelationJoinMap *map)
{
    varIntMapDtor(&map->attrMap) ;
    varIntMapDtor(&map->tupleMap) ;
    ckfree((char *)map) ;
}

static void
relationJoinMapAddAttrs(
    Ral_RelationJoinMap *map,
    int i1,
    int i2)
{
    assert(map->attrMap.count < map->attrMap.allocated) ;
    map->attrMap.vector[map->attrMap.count].index1 = i1 ;
    map->attrMap.vector[map->attrMap.count].index2 = i2 ;
    ++map->attrMap.count ;
}

static void
relationJoinMapAddTuples(
    Ral_RelationJoinMap *map,
    int i1,
    int i2)
{
    assert(map->tupleMap.count < map->tupleMap.allocated) ;
    map->tupleMap.vector[map->tupleMap.count].index1 = i1 ;
    map->tupleMap.vector[map->tupleMap.count].index2 = i2 ;
    ++map->tupleMap.count ;
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
    Ral_TupleHeading *src,
    int first,
    int last,
    Ral_TupleHeading *dest,
    int start)
{
    Ral_Attribute *srcAttr ;
    Ral_Attribute *destAttr ;

    assert(first <= src->degree) ;
    assert(last <= src->degree) ;
    assert(first <= last) ;
    assert(first + (last - first) <= src->degree) ;
    assert(start <= dest->degree) ;
    assert(start + (last - first) <= dest->degree) ;

    srcAttr = src->attrVector + first ;
    destAttr = dest->attrVector + start ;
    for ( ; first != last ; ++first, ++srcAttr, ++destAttr) {
	if (tupleHeadingInsertName(interp, dest, srcAttr->name, start++)
	    != TCL_OK) {
	    return TCL_ERROR ;
	}
	attributeCopy(srcAttr, destAttr) ;
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
    if (tupleHeadingCopy(NULL, srcHeading, 0, srcHeading->degree, dupHeading, 0)
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

    if (tupleHeadingCopy(interp, h1, 0, h1->degree, newHeading, 0) != TCL_OK ||
	tupleHeadingCopy(interp, h2, 0, h2->degree, newHeading, h1->degree)
	!= TCL_OK) {
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
    int where ;

    /*
     * Since the string representation of a Heading is a list of pairs,
     * we can use the list object functions to do the heavy lifting here.
     */
    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
	return NULL ;
    }
    if (objc % 2 != 0) {
	if (interp) {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"bad heading format, \"", Tcl_GetString(objPtr), "\"", NULL) ;
	}
	return NULL ;
    }
    heading = tupleHeadingNew(objc / 2) ;
    /*
     * Iterate through the list adding each element as an attribute to
     * a newly created Heading.
     */
    for (where = 0 ; objc > 0 ; objc -= 2, objv += 2) {
	if (tupleHeadingInsertAttribute(interp, heading, *objv, *(objv + 1),
	    where++) != TCL_OK) {
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
	if (Tcl_ListObjAppendElement(interp, headList, attr->name) != TCL_OK) {
	    goto errorOut ;
	}
	switch (attr->attrType) {
	case Tcl_Type:
	    if (Tcl_ListObjAppendElement(interp, headList,
		Tcl_NewStringObj(attr->tclType->name, -1)) != TCL_OK) {
		goto errorOut ;
	    }
	    break ;

	case Tuple_Type:
	    if (Tcl_ListObjAppendElement(interp, headList,
		tupleHeadingObjNew(interp, attr->tupleHeading))
		!= TCL_OK) {
		goto errorOut ;
	    }
	    break ;

	case Relation_Type:
	    if (Tcl_ListObjAppendElement(interp, headList,
		relationHeadingObjNew(interp, attr->relationHeading))
		!= TCL_OK) {
		goto errorOut ;
	    }
	    break ;

	default:
	    Tcl_Panic("unknown attribute data type") ;
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

static void
tupleHeadingCommonAttrs(
    Ral_TupleHeading *th1,
    Ral_TupleHeading *th2,
    Ral_RelationJoinMap *map)
{
    int index1 ;
    Ral_Attribute *attr1 ;

    for (index1 = 0, attr1 = th1->attrVector ; index1 < th1->degree ;
	++index1, ++attr1) {
	Ral_Attribute *attr2 ;
	int index2 ;

	attr2 = tupleHeadingFindAttribute(NULL, th2, attr1->name, &index2) ;
	if (attr2 && attributeTypeEqual(attr1, attr2)) {
	    relationJoinMapAddAttrs(map, index1, index2) ;
	}
    }
}

static int
tupleHeadingJoinAttrs(
    Tcl_Interp *interp,
    Ral_TupleHeading *th1,
    Ral_TupleHeading *th2,
    int nAttrs,
    Tcl_Obj *const*attrPairs,
    Ral_RelationJoinMap *map)
{
    while (nAttrs-- > 0) {
	int elemc ;
	Tcl_Obj **elemv ;
	Tcl_Obj *name1 ;
	Tcl_Obj *name2 ;
	Ral_Attribute *attr1 ;
	Ral_Attribute *attr2 ;
	int index1 ;
	int index2 ;

	if (Tcl_ListObjGetElements(interp, *attrPairs, &elemc, &elemv)
	    != TCL_OK) {
	    return TCL_ERROR ;
	}

	if (elemc == 1) {
	    name1 = name2 = *elemv ;
	} else if (elemc == 2) {
	    name1 = *elemv ;
	    name2 = *(elemv + 1) ;
	} else {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"badly formatted join attribute list, {",
		Tcl_GetString(*attrPairs), "}", NULL) ;
	    return TCL_ERROR ;
	}

	attr1 = tupleHeadingFindAttribute(interp, th1, name1, &index1) ;
	if (attr1 == NULL) {
	    return TCL_ERROR ;
	}
	attr2 = tupleHeadingFindAttribute(interp, th2, name2, &index2) ;
	if (attr2 == NULL) {
	    return TCL_ERROR ;
	}
	if (attributeTypeEqual(attr1, attr2)) {
	    relationJoinMapAddAttrs(map, index1, index2) ;
	} else {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"type of attribute, \"", Tcl_GetString(name1),
		"\", does not match that of attribute, \"",
		Tcl_GetString(name2), "\"", NULL) ;
	    return TCL_ERROR ;
	}
    }

    return TCL_OK ;
}

static int
tupleHeadingSetIdFromObj(
    Tcl_Interp *interp,
    Ral_TupleHeading *tupleHeading,
    Ral_FixedIntVector *relId,
    Tcl_Obj *objPtr)
{
    int elemc ;
    Tcl_Obj **elemv ;
    int c ;

    if (Tcl_ListObjGetElements(interp, objPtr, &elemc, &elemv)) {
	return TCL_ERROR ;
    }
    fixedIntVectorCtor(relId, elemc) ;

    for (c = 0 ; c < elemc ; ++c) {
	int index ;

	index = tupleHeadingFindIndex(interp, tupleHeading, *elemv++) ;
	if (index < 0) {
	    return TCL_ERROR ;
	}
	relId->vector[c] = index ;
    }
    fixedIntVectorSort(relId) ;

    return TCL_OK ;
}

static Tcl_Obj *
tupleHeadingIdToList(
    Tcl_Interp *interp,
    Ral_TupleHeading *tupleHeading,
    Ral_FixedIntVector *relId)
{
    Tcl_Obj *attrList ;
    int c ;

    attrList = Tcl_NewListObj(0, NULL) ;
    for (c = 0 ; c < relId->count ; ++c) {
	assert(relId->vector[c] < tupleHeading->degree) ;
	if (Tcl_ListObjAppendElement(interp, attrList,
	    tupleHeading->attrVector[relId->vector[c]].name) != TCL_OK) {
	    Tcl_DecrRefCount(attrList) ;
	    return NULL ;
	}
    }

    return attrList ;
}

/*
 * Recompute a relId based on the names from one heading indexed into
 * another heading.
 * Assumes the tuple headings are equal!
 */
static void
tupleHeadingMapIds(
    Ral_TupleHeading *h1,
    Ral_FixedIntVector *relId1,
    Ral_TupleHeading *h2,
    Ral_FixedIntVector *relId2)
{
    int c ;

    assert(tupleHeadingEqual(h1, h2)) ;

    for (c = 0 ; c < relId1->count ; ++c) {
	int index ;

	index = tupleHeadingFindIndex(NULL, h2, h1->attrVector[c].name) ;
	assert(index >= 0) ;
	relId2->vector[c] = index ;
    }
    fixedIntVectorSort(relId2) ;
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
    assert(last >= first) ;
    assert(last - first <= srcTuple->heading->degree) ;
    assert(start <= destTuple->heading->degree) ;
    assert(start + (last - first) <= destTuple->heading->degree) ;

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
    Ral_TupleHeading *destHeading = dest->heading ;
    int srcIndex ;

    srcIndex = tupleHeadingFindIndex(interp, srcHeading, attrName) ;
    if (srcIndex < 0) {
	return TCL_ERROR ;
    }
    if (tupleHeadingCopy(interp, srcHeading, srcIndex, srcIndex + 1,
	destHeading, destIndex) != TCL_OK) {
	return TCL_ERROR ;
    }
    Tcl_IncrRefCount(dest->values[destIndex] = src->values[srcIndex]) ;

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

static void
tupleGetIdKey(
    Ral_Tuple *tuple,
    Ral_FixedIntVector *relId,
    Tcl_DString *idKey)
{
    int i ;

    Tcl_DStringInit(idKey) ;
    for (i = 0 ; i < relId->count ; ++i) {
	Tcl_DStringAppend(idKey,
	    Tcl_GetString(tuple->values[relId->vector[i]]), -1) ;
    }
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
    Ral_FixedIntVector *idVector ;

    assert(nIds > 0) ;

    nBytes = sizeof(*heading) + nIds * sizeof(*heading->idVector) ;
    heading = (Ral_RelationHeading *)ckalloc(nBytes) ;
    memset(heading, 0, nBytes) ;

    heading->refCount = 0 ;
    tupleHeadingReference(heading->tupleHeading = tupleHeading) ;
    heading->idCount = nIds ;
    idVector = heading->idVector = (Ral_FixedIntVector *)(heading + 1) ;

    for ( ; nIds > 0 ; --nIds, ++idVector) {
	fixedIntVectorCtor(idVector, 0) ;
    }

    return heading ;
}

/*
 * Create a new relation heading by duplicating an existing heading
 * if all the attributes of that heading are mentioned in the retained list.
 */
static Ral_RelationHeading *
relationHeadingNewRetained(
    Ral_RelationHeading *heading,
    Ral_TupleHeading *tupleHeading,
    int *retainMap)
{
    int degree = heading->tupleHeading->degree ;
    Ral_FixedIntVector *srcIdVector = heading->idVector ;
    char *retainedIds ;
    int retainedCount ;
    int i ;
    Ral_RelationHeading *newHeading ;

    retainedIds = ckalloc(heading->idCount * sizeof(*retainedIds)) ;
    memset(retainedIds, 0, heading->idCount * sizeof(*retainedIds)) ;
    /*
     * Traverse the identifers of the heading and determine if all
     * the attributes are set in the retain map.
     */
    retainedCount = 0 ;
    for (i = 0 ; i < heading->idCount ; ++i) {
	Ral_FixedIntVector *v = srcIdVector + i ;
	int found = 0 ;
	int c  ;

	for (c = 0 ; c < v->count ; ++c) {
	    if (retainMap[v->vector[c]] >= 0) {
		++found ;
	    }
	}
	/*
	 * If every attribute of this identifier was found in the retain map,
	 * then we keep the identifier.
	 */
	if (found == v->count) {
	    retainedIds[i] = 1 ;
	    ++retainedCount ;
	}
    }

    if (retainedCount) {
	Ral_FixedIntVector *destIdVector ;

	newHeading = relationHeadingNew(tupleHeading, retainedCount) ;
	destIdVector = newHeading->idVector ;

	/*
	 * Construct the new identifer vectors.
	 */
	for (i = 0 ; i < retainedCount ; ++i) {
	    if (retainedIds[i]) {
		int j ;

		fixedIntVectorCtor(destIdVector, srcIdVector->count) ;
		/*
		 * When copying the identifier index from the source
		 * to the destination, it must be re-mapped to the
		 * destination order.
		 */
		for (j = 0 ; j < srcIdVector->count ; ++j) {
		    int srcIndex = srcIdVector->vector[j] ;
		    assert(retainMap[srcIndex] >= 0) ;
		    destIdVector->vector[j] = retainMap[srcIndex] ;
		}
		++destIdVector ;
	    }
	    ++srcIdVector ;
	}
    } else {
	newHeading = relationHeadingNew(tupleHeading, 1) ;
	fixedIntVectorIdentityMap(newHeading->idVector, tupleHeading->degree) ;
    }

    ckfree(retainedIds) ;
    return newHeading ;
}

/*
 * Create a new relation heading that is the union of two other headings.
 */
static Ral_RelationHeading *
relationHeadingNewUnion(
    Tcl_Interp *interp,
    Ral_RelationHeading *h1,
    Ral_RelationHeading *h2)
{
    Ral_TupleHeading *tupleHeading ;
    Ral_RelationHeading *unionHeading ;
    int h1IdCount ;
    int h2IdCount ;
    Ral_FixedIntVector *unionIdVect ;
    Ral_FixedIntVector *h1IdVect ;
    Ral_FixedIntVector *h2IdVect ;
    int c1 ;
    int c2 ;
    int h1degree ;

    /*
     * Union the tuple heading.
     */
    tupleHeading = tupleHeadingUnion(interp, h1->tupleHeading,
	h2->tupleHeading) ;
    if (tupleHeading == NULL) {
	return NULL ;
    }

    /*
     * The identifiers are the cross project of the identifiers
     * of the two headings.
     */
    h1IdCount = h1->idCount ;
    h2IdCount = h1->idCount ;
    unionHeading = relationHeadingNew(tupleHeading, h1IdCount * h2IdCount) ;

    unionIdVect = unionHeading->idVector ;
    h1IdVect = h1->idVector ;
    h1degree = h1->tupleHeading->degree ;
    /*
     * Loop through copying the identifier indices.
     */
    for (c1 = 0 ; c1 < h1IdCount ; ++c1) {
	h2IdVect = h2->idVector ;
	for (c2 = 0 ; c2 < h2IdCount ; ++c2) {
	    int i ;

	    fixedIntVectorCtor(unionIdVect, h1IdVect->count + h2IdVect->count) ;
	    memcpy(unionIdVect->vector, h1IdVect->vector,
		h1IdVect->count * sizeof(*unionIdVect->vector)) ;
	    for (i = 0 ; i < h2IdVect->count ; ++i) {
		/*
		 * Note that the identifier indices are offset by the
		 * degree of the first heading. This adjusts them to
		 * the proper value for the new heading.
		 */
		unionIdVect->vector[h1IdVect->count + i] =
		    h2IdVect->vector[i] + h1degree ;
	    }
	    ++h2IdVect ;
	    ++unionIdVect ;
	}
	++h1IdVect ;
    }

    return unionHeading ;
}

static void
relationHeadingDelete(
    Ral_RelationHeading *heading)
{
    Ral_FixedIntVector *idVector = heading->idVector ;
    int nIds ;

    assert(heading->refCount <= 0) ;

    for (nIds = heading->idCount ; nIds > 0 ; --nIds, ++idVector) {
	fixedIntVectorDtor(idVector) ;
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
    Ral_FixedIntVector *srcIdVector = srcHeading->idVector ;
    Ral_FixedIntVector *destIdVector ;

    tupleHeading = tupleHeadingDup(srcHeading->tupleHeading, addAttrs) ;
    if (!tupleHeading) {
	return NULL ;
    }

    heading = relationHeadingNew(tupleHeading, idCount) ;
    for (destIdVector = heading->idVector ; idCount > 0 ;
	--idCount, ++srcIdVector, ++destIdVector) {
	fixedIntVectorCtor(destIdVector, srcIdVector->count) ;
	memcpy(destIdVector->vector, srcIdVector->vector,
	    srcIdVector->count * sizeof(*srcIdVector->vector)) ;
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

static int
relationHeadingFindId(
    Ral_RelationHeading *heading,
    Ral_FixedIntVector *idKey)
{
    int idNum ;
    Ral_FixedIntVector *idVector ;

    for (idNum = 0, idVector = heading->idVector ;
	idNum < heading->idCount ; ++idNum, ++idVector) {
	if (fixedIntVectorEqual(idVector, idKey)) {
	    return idNum ;
	}
    }

    return -1 ;
}

/*
 * Returns 0 if the "key" is a subset of any of the id's between
 * [start, last).
 */
static int
relationHeadingCheckId(
    Ral_FixedIntVector *start,
    Ral_FixedIntVector *last,
    Ral_FixedIntVector *key)
{
    for ( ; start != last ; ++start) {
	if (fixedIntVectorIsSubsetOf(key, start) ||
	    fixedIntVectorIsSubsetOf(start, key)) {
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
    Ral_FixedIntVector *idVector = h1->idVector ;
    Ral_TupleHeading *th2 = h2->tupleHeading ;
    Ral_FixedIntVector keyId ;

    if (h1->idCount != h2->idCount) {
	return 0 ;
    }
    /*
     * Strategy is to compose a vector of indices of the identifying attributes
     * using the names of h1 and find the corresponding indices from h2. Then
     * we can search the Id's of h2 to see if we can find a match. If an
     * identifier in h1 has a correspondence in h2, then we have a match.
     */
    memset(&keyId, 0, sizeof(keyId)) ;
    for ( ; idCount > 0 ; --idCount, ++idVector) {
	int found ;

	fixedIntVectorCtor(&keyId, idVector->count) ;

	tupleHeadingMapIds(th1, idVector, th2, &keyId) ;
	found = relationHeadingFindId(h2, &keyId) ;

	fixedIntVectorDtor(&keyId) ;

	if (found < 0) {
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
    Ral_FixedIntVector *idVector ;

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
	if (tupleHeadingSetIdFromObj(interp, tupleHeading, idVector, *idv)
	    != TCL_OK) {
	    relationHeadingDelete(heading) ;
	    return NULL ;
	}
	if (!relationHeadingCheckId(heading->idVector, idVector, idVector)) {
	    if (interp) {
		Tcl_Obj *resultObj ;

		Tcl_ResetResult(interp) ;
		resultObj = Tcl_GetObjResult(interp) ;
		Tcl_AppendStringsToObj(resultObj, "identifier, \"", NULL) ;
		attributeAppendIdAttrs(tupleHeading->attrVector, idVector,
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

static Tcl_Obj *
relationHeadingIdsToList(
    Tcl_Interp *interp,
    Ral_RelationHeading *heading)
{
    Tcl_Obj *idObj ;
    int idCount ;
    Ral_FixedIntVector *idVector ;

    idObj = Tcl_NewListObj(0, NULL) ;
    for (idCount = heading->idCount, idVector = heading->idVector ;
	idCount > 0 ; --idCount, ++idVector) {
	Tcl_Obj *idAttrList ;

	idAttrList = tupleHeadingIdToList(interp, heading->tupleHeading,
	    idVector) ;
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
    Tcl_Obj *headElems[3] ;

    headElems[1] = tupleHeadingAttrsToList(interp, heading->tupleHeading) ;
    if (!headElems[1]) {
	return NULL ;
    }
    headElems[2] = relationHeadingIdsToList(interp, heading) ;
    if (!headElems[2]) {
	Tcl_DecrRefCount(headElems[1]) ;
	return NULL ;
    }

    headElems[0] = Tcl_NewStringObj(Ral_RelationType.name, -1) ;

    return Tcl_NewListObj(3, headElems) ;
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

static void
relationHeadingJoinIdentifiers(
    Ral_RelationHeading *jh,
    Ral_RelationHeading *h1,
    Ral_RelationHeading *h2,
    Ral_FixedIntVector *r2AttrMap)
{
    Ral_FixedIntVector *r1Ids = h1->idVector ;
    Ral_FixedIntVector *r1Last = h1->idVector + h1->idCount ;
    Ral_FixedIntVector *r2Ids = h2->idVector ;
    Ral_FixedIntVector *r2Last = h2->idVector + h2->idCount ;
    Ral_FixedIntVector *jIds = jh->idVector ;

    for ( ; r1Ids != r1Last ; ++r1Ids) {
	for ( ; r2Ids != r2Last ; ++r2Ids) {
	    /*
	     * We need to determine the number of attributes that are part
	     * of this identifier. That will be the number in r1 plus
	     * the number in r2 minus any in r2 that are eliminated. So
	     * we need to count how many are eliminated.
	     */
	    int eCount = 0 ;
	    int r2Index ;
	    int where ;
	    for (r2Index = 0 ; r2Index < r2Ids->count ; ++r2Index) {
		if (r2AttrMap->vector[r2Ids->vector[r2Index]] == -1) {
		    ++eCount ;
		}
	    }
	    fixedIntVectorCtor(jIds, r1Ids->count + r2Ids->count - eCount) ;
	    /*
	     * Copy all the identifiers from r1 to the join identifier.
	     */
	    memcpy(jIds->vector, r1Ids->vector,
		r1Ids->count * sizeof(*r1Ids->vector)) ;
	    /*
	     * Copy all the identifiers from r2, sans the eliminated join
	     * identifiers. Use the attribute map to adjust the attribute
	     * index to be that of the joined relation.
	     */
	    where = r1Ids->count ;
	    for (r2Index = 0 ; r2Index < r2Ids->count ; ++r2Index) {
		int jAttrIndex = r2AttrMap->vector[r2Ids->vector[r2Index]] ;
		if (jAttrIndex != -1) {
		    jIds->vector[where++] = jAttrIndex ;
		}
	    }
	    ++jIds ;
	}
    }

    return ;
}

static void
relationHeadingUngroupIdentifiers(
    Ral_RelationHeading *ugh,
    Ral_RelationHeading *relh,
    Ral_RelationHeading *attrh,
    int ungrpIndex)
{
    Ral_FixedIntVector *relIds = relh->idVector ;
    Ral_FixedIntVector *relLast = relh->idVector + relh->idCount ;
    Ral_FixedIntVector *attrIds = attrh->idVector ;
    Ral_FixedIntVector *attrLast = attrh->idVector + attrh->idCount ;
    Ral_FixedIntVector *ughIds = ugh->idVector ;
    int attrIdOffset = relh->tupleHeading->degree - 1 ;

    for ( ; relIds != relLast ; ++relIds) {
	for ( ; attrIds != attrLast ; ++attrIds) {
	    /*
	     * We need to determine if the ungrouped attribute is part of the
	     * identifier for the relation. If so, then it must be excluded.
	     */
	    int foundUngrp = 0 ;
	    int relIndex ;
	    int attrIndex ;
	    int where = 0 ;
	    for (relIndex = 0 ; relIndex < relIds->count ; ++relIndex) {
		if (relIds->vector[relIndex] == ungrpIndex) {
		    ++foundUngrp ;
		}
	    }
	    assert(foundUngrp <= 1) ;
	    fixedIntVectorCtor(ughIds, relIds->count + attrIds->count
		- foundUngrp) ;
	    /*
	     * Copy all the identifiers from the relation to the ungrouped
	     * heading except any that match the attribute index of that
	     * attribute being ungrouped.
	     */
	    for (relIndex = 0 ; relIndex < relIds->count ; ++relIndex) {
		if (relIds->vector[relIndex] != ungrpIndex) {
		    ughIds->vector[where++] = relIds->vector[relIndex] ;
		}
	    }
	    /*
	     * Copy all the identifiers from ungrouped attribute. Each has
	     * to be offset properly.
	     */
	    for (attrIndex = 0 ; attrIndex < attrIds->count ; ++attrIndex) {
		ughIds->vector[where++] = attrIds->vector[attrIndex]
		    + attrIdOffset ;
	    }
	    ++ughIds ;
	}
    }

    return ;
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

    if (relation->allocated - relation->cardinality >= nTuples) {
	return ;
    }

    relation->allocated = relation->allocated +
	max(relation->allocated / 2, nTuples) ;
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
    Tcl_Obj *nvList ;
    Ral_Tuple **tupleVector ;
    Ral_Tuple **last ;

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
    Ral_FixedIntVector *relId ;
    Tcl_HashTable *hashTable ;
    Tcl_DString idKey ;
    Tcl_HashEntry *entry ;
    int newPtr ;

    assert(idIndex < relation->heading->idCount) ;

    relId = relation->heading->idVector + idIndex ;
    hashTable = relation->indexVector + idIndex ;

    tupleGetIdKey(tuple, relId, &idKey) ;
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
	    attributeAppendIdAttrs(tuple->heading->attrVector, relId,
		resultObj) ;
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
    Tcl_DString idKey ;
    Tcl_HashEntry *entry ;

    assert(idIndex < relation->heading->idCount) ;

    tupleGetIdKey(tuple, &relation->heading->idVector[idIndex], &idKey) ;
    entry = Tcl_FindHashEntry(&relation->indexVector[idIndex],
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
	Ral_FixedIntVector *relId = relation->heading->idVector + idIndex ;
	Tcl_Obj **values = tuple->values ;
	int *attrIndices = relId->vector ;
	int *attrOrder = order->ordering.vector ;
	Tcl_HashTable *indexTable = relation->indexVector + idIndex ;
	int i ;
	Tcl_HashEntry *entry ;

	assert(relation->heading->tupleHeading->degree ==
	    order->ordering.count) ;

	Tcl_DStringInit(&idKey) ;
	for (i = relId->count ; i > 0 ; --i) {
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
	int c ;

	newTuple = tupleNew(relation->heading->tupleHeading) ;

	assert(newTuple->heading->degree == orderMap->ordering.count) ;
	for (c = 0 ; c < orderMap->ordering.count ; ++c) {
	    Tcl_IncrRefCount(newTuple->values[c] =
		tuple->values[orderMap->ordering.vector[c]]) ;
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
 * Big opportunity here to use the hash tables that index tuples
 * based on identifiers to find the matches.
 */
static void
relationFindJoinTuples(
    Ral_Relation *r1,
    Ral_Relation *r2,
    Ral_RelationJoinMap *map)
{
    int card1 ;
    Ral_Tuple **tv1 ;

    for (card1 = 0, tv1 = r1->tupleVector ; card1 < r1->cardinality ;
	++card1, ++tv1) {
	Ral_Tuple *tuple1 ;
	int card2 ;
	Ral_Tuple **tv2 ;

	tuple1 = *tv1 ;
	for (card2 = 0, tv2 = r2->tupleVector ; card2 < r2->cardinality ;
	    ++card2, ++tv2) {
	    Ral_Tuple *tuple2 ;
	    int attrIndex ;
	    int matches = 0 ;

	    tuple2 = *tv2 ;
	    for (attrIndex = 0 ; attrIndex < map->attrMap.count ; ++attrIndex) {
		assert(map->attrMap.vector[attrIndex].index1 <
		    tuple1->heading->degree) ;
		assert(map->attrMap.vector[attrIndex].index2 <
		    tuple2->heading->degree) ;

		if (!ObjsEqual(
		    tuple1->values[map->attrMap.vector[attrIndex].index1],
		    tuple2->values[map->attrMap.vector[attrIndex].index2])) {
		    break ;
		}
		++matches ;
	    }

	    if (matches == map->attrMap.count) {
		relationJoinMapAddTuples(map, card1, card2) ;
	    }
	}
    }
}

static int
relationFindSortAttrs(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Tcl_Obj *attrList,
    int *count,
    int **indices)
{
    int elemc ;
    Tcl_Obj **elemv ;
    int *sortAttrs ;
    int i ;

    if (Tcl_ListObjGetElements(interp, attrList, &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    *indices = sortAttrs = (int *)ckalloc(elemc * sizeof(*sortAttrs)) ;
    memset(sortAttrs, 0 , elemc * sizeof(*sortAttrs)) ;
    *count = elemc ;

    for (i = 0 ; i < elemc ; ++i) {
	int index ;

	index = tupleHeadingFindIndex(interp, relation->heading->tupleHeading,
	    *elemv++) ;
	if (index < 0) {
	    ckfree((char *)sortAttrs) ;
	    return TCL_ERROR ;
	}
	sortAttrs[i] = index ;
    }

    return TCL_OK ;
}

static int tupleSortCount ;
static int *tupleSortAttrs ;

static void
tupleSortKey(
    const Ral_Tuple *tuple,
    Tcl_DString *idKey)
{
    Tcl_Obj **values = tuple->values ;
    int *attrIndices = tupleSortAttrs ;
    int i ;

    Tcl_DStringInit(idKey) ;
    for (i = tupleSortCount ; i > 0 ; --i) {
	Tcl_DStringAppend(idKey, Tcl_GetString(values[*attrIndices++]), -1) ;
    }
}

static int
tupleCompareAscending(
    const void *v1,
    const void *v2)
{
    Ral_Tuple *const*t1 = v1 ;
    Ral_Tuple *const*t2 = v2 ;
    Tcl_DString sortKey1 ;
    Tcl_DString sortKey2 ;
    int result ;

    tupleSortKey(*t1, &sortKey1) ;
    tupleSortKey(*t2, &sortKey2) ;

    result = strcmp(Tcl_DStringValue(&sortKey1), Tcl_DStringValue(&sortKey2)) ;

    Tcl_DStringFree(&sortKey1) ;
    Tcl_DStringFree(&sortKey2) ;

    return result ;
}

static int
tupleCompareDescending(
    const void *v1,
    const void *v2)
{
    Ral_Tuple *const*t1 = v1 ;
    Ral_Tuple *const*t2 = v2 ;
    Tcl_DString sortKey1 ;
    Tcl_DString sortKey2 ;
    int result ;

    tupleSortKey(*t1, &sortKey1) ;
    tupleSortKey(*t2, &sortKey2) ;

    /*
     * N.B. inverted order of first and second tuple to obtain
     * the proper result for descending order.
     */
    result = strcmp(Tcl_DStringValue(&sortKey2), Tcl_DStringValue(&sortKey1)) ;

    Tcl_DStringFree(&sortKey1) ;
    Tcl_DStringFree(&sortKey2) ;

    return result ;
}

static Ral_Tuple **
relationSortAscending(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Tcl_Obj *attrList)
{
    Ral_Tuple **tv ;

    tv = (Ral_Tuple **)ckalloc(relation->cardinality * sizeof(*tv)) ;
    memcpy(tv, relation->tupleVector, relation->cardinality * sizeof(*tv)) ;
    if (relationFindSortAttrs(interp, relation, attrList, &tupleSortCount,
	&tupleSortAttrs) != TCL_OK) {
	return NULL ;
    }

    qsort(tv, relation->cardinality, sizeof(*tv), tupleCompareAscending) ;
    ckfree((char *)tupleSortAttrs) ;

    return tv ;
}

static Ral_Tuple **
relationSortDescending(
    Tcl_Interp *interp,
    Ral_Relation *relation,
    Tcl_Obj *attrList)
{
    Ral_Tuple **tv ;

    tv = (Ral_Tuple **)ckalloc(relation->cardinality * sizeof(*tv)) ;
    memcpy(tv, relation->tupleVector, relation->cardinality * sizeof(*tv)) ;
    if (relationFindSortAttrs(interp, relation, attrList, &tupleSortCount,
	&tupleSortAttrs) != TCL_OK) {
	return NULL ;
    }

    qsort(tv, relation->cardinality, sizeof(*tv), tupleCompareDescending) ;
    ckfree((char *)tupleSortAttrs) ;

    return tv ;
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
    if (objc != 4) {
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

    heading = relationHeadingNewFromObjs(interp, *(objv + 1), *(objv + 2)) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    relation = relationNew(heading) ;
    if (relationSetValuesFromString(interp, relation, *(objv + 3)) != TCL_OK) {
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
    char *elimMap ;
    int elimCount ;
    int i ;
    Tcl_Obj **values ;
    Ral_TupleHeading *newHeading ;
    int attrIndex ;
    Ral_Tuple *newTuple ;

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

    objc -= 3 ;
    if (objc <= 0) {
	Tcl_SetObjResult(interp, tupleObj) ;
	return TCL_OK ;
    }
    objv += 3 ;
    /*
     * Check that attributes to eliminate actually belong to the tuple.
     * Build a mapping structure that determines if we delete the attribute.
     */
    elimMap = ckalloc(heading->degree) ;
    memset(elimMap, 0, heading->degree) ;
    for (i = 0 ; i < objc ; ++i) {
	attrIndex = tupleHeadingFindIndex(interp, heading, objv[i]) ;

	if (attrIndex < 0) {
	    ckfree(elimMap) ;
	    return TCL_ERROR ;
	} else {
	    assert(attrIndex < heading->degree) ;
	    elimMap[attrIndex] = 1 ;
	}
    }
    /*
     * Do this as a separate step, just in case the same attribute
     * is mentioned multiple times to be eliminated.
     */
    elimCount = 0 ;
    for (i = 0 ; i < heading->degree ; ++i) {
	elimCount += elimMap[i] != 0 ;
    }
    if (elimCount > heading->degree) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	"attempted to eliminate more attributes than exist in the relation",
	    -1) ;
	ckfree((char *)elimMap) ;
	return TCL_ERROR ;
    }
    /*
     * Build a new heading. It will have as many fewer attributes
     * as are listed with the command.
     */
    newHeading = tupleHeadingNew(heading->degree - elimCount) ;
    newTuple = tupleNew(newHeading) ;
    i = 0 ;
    for (attrIndex = 0 ; attrIndex < heading->degree; ++attrIndex) {
	/*
	 * check if this attribute is to be included
	 */
	if (!elimMap[attrIndex]) {
	    /*
	     * Add the name to the heading of the new tuple.
	     */
	    if (tupleHeadingCopy(interp, heading, attrIndex, attrIndex + 1,
		newHeading, i) != TCL_OK) {
		ckfree(elimMap) ;
		tupleDelete(newTuple) ;
		return TCL_ERROR ;
	    }
	    /*
	     * Add the value to the new tuple.
	     */
	    tupleCopyValues(tuple, attrIndex, attrIndex + 1, newTuple, i) ;
	    ++i ;
	}
    }

    ckfree(elimMap) ;
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
TupleHeadingCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *tupleObj ;
    Ral_Tuple *tuple ;

    /* tuple heading tupleValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "tupleValue") ;
	return TCL_ERROR ;
    }

    tupleObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, tupleObj, &Ral_TupleType) != TCL_OK)
	return TCL_ERROR ;
    tuple = tupleObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, tupleHeadingAttrsToList(interp, tuple->heading)) ;
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
    int tupleAttr ;
    int tupleAttrIndex ;
    Tcl_Obj *tupleAttrValue ;
    Ral_Tuple *unTuple ;
    Ral_TupleHeading *newHeading ;
    Ral_Tuple *newTuple ;
    int attrIndex ;
    int newIndex ;

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
    tupleAttr = tupleHeadingFindIndex(interp, heading, tupleAttrObj) ;
    if (tupleAttr < 0) {
	return TCL_ERROR ;
    }
    if (heading->attrVector[tupleAttr].attrType != Tuple_Type) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "attribute, \"",
	    Tcl_GetString(tupleAttrObj), "\", is not of type Tuple", NULL) ;
	return TCL_ERROR ;
    }
    tupleAttrValue = tuple->values[tupleAttr] ;
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

    newIndex = 0 ;
    for (attrIndex = 0 ; attrIndex < heading->degree ; ++attrIndex) {
	    /*
	     * Check if this attribute needs to be unwrapped.
	     * Since only one one attribute will be found that needs
	     * unwrapping, avoid the Tcl_GetString() if we have already
	     * done the unwrapping.
	     */
	if (attrIndex == tupleAttr) {
	    Ral_TupleHeading *unHeading ;
	    int attrIndex ;
	    /*
	     * Found attribute that matches the one to be unwrapped.
	     * Add all the wrapped attributes to the unwrapped heading.
	     */
	    unHeading = unTuple->heading ;
	    if (tupleHeadingCopy(interp, unHeading, 0, unHeading->degree,
		newHeading, newIndex) != TCL_OK) {
		goto errorOut ;
	    }
	    tupleCopyValues(unTuple, 0, unHeading->degree, newTuple, newIndex) ;
	    newIndex += unHeading->degree ;
	} else {
	    /*
	     * Otherwise just add to the new tuple
	     */
	    if (tupleHeadingCopy(interp, heading, attrIndex, attrIndex + 1,
		newHeading, newIndex) != TCL_OK) {
		goto errorOut ;
	    }
	    tupleCopyValues(tuple, attrIndex, attrIndex + 1, newTuple,
		newIndex) ;
	    ++newIndex ;
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
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_Tuple *wrapTuple ;
    Ral_TupleHeading *wrapHeading ;
    char *wrapAttrMap ;
    int attrIndex ;
    Ral_TupleHeading *newHeading ;
    Ral_Tuple *newTuple ;
    int i ;
    Tcl_Obj *wrapTupleObj ;

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
    wrapAttrMap = ckalloc(heading->degree * sizeof(*wrapAttrMap)) ;
    memset(wrapAttrMap, 0, heading->degree) ;
    wrapHeading = tupleHeadingNew(elemc) ;
    wrapTuple = tupleNew(wrapHeading) ;
    for (i = 0 ; i < elemc ; ++i) {
	attrIndex = tupleHeadingFindIndex(interp, heading, *elemv++) ;
	if (attrIndex < 0) {
	    tupleDelete(wrapTuple) ;
	    goto errorOut ;
	}
	assert(attrIndex < heading->degree) ;
	wrapAttrMap[attrIndex] = 1 ;
	if (tupleHeadingCopy(interp, heading, attrIndex, attrIndex + 1,
	    wrapHeading, i) != TCL_OK) {
	    tupleDelete(wrapTuple) ;
	    goto errorOut ;
	}
	tupleCopyValues(tuple, attrIndex, attrIndex + 1, wrapTuple, i) ;
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
    for (attrIndex = 0 ; attrIndex < heading->degree ; ++attrIndex) {
	/*
	 * Only add the ones that are NOT in the old attribute list.
	 */
	if (!wrapAttrMap[attrIndex]) {
	    if (tupleHeadingCopy(interp, heading, attrIndex, attrIndex + 1,
		newHeading, i) != TCL_OK) {
		goto errorOut2 ;
	    }
	    tupleCopyValues(tuple, attrIndex, attrIndex + 1, newTuple, i) ;
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

    ckfree(wrapAttrMap) ;
    Tcl_SetObjResult(interp, tupleObjNew(newTuple)) ;
    return TCL_OK ;

errorOut2:
    tupleDelete(newTuple) ;
    Tcl_DecrRefCount(wrapTupleObj) ;
errorOut:
    ckfree(wrapAttrMap) ;
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
    static const struct cmdMap cmdTable[] = {
	{"assign", TupleAssignCmd},
	{"create", TupleCreateCmd},
	{"degree", TupleDegreeCmd},
	{"eliminate", TupleEliminateCmd},
	{"equal", TupleEqualCmd},
	{"extend", TupleExtendCmd},
	{"extract", TupleExtractCmd},
	{"get", TupleGetCmd},
	{"heading", TupleHeadingCmd},
	{"project", TupleProjectCmd},
	{"rename", TupleRenameCmd},
	{"unwrap", TupleUnwrapCmd},
	{"update", TupleUpdateCmd},
	{"wrap", TupleWrapCmd},
	{NULL, NULL},
    } ;

    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObjStruct(interp, *(objv + 1), cmdTable,
	sizeof(struct cmdMap), "subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    return cmdTable[index].cmdFunc(interp, objc, objv) ;
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
    /*
     * No return from "relvar create". Must use "relvar set" to access
     * the value.
     */
    Tcl_ResetResult(interp) ;
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

    Tcl_SetObjResult(interp, Tcl_NewIntObj(deleted)) ;
    return TCL_OK ;
}

static int
RelvarDestroyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *nameObj ;

    /* relvar destroy ?relvarName ... ? */
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 2, objv, "?relvarName ...?") ;
	return TCL_ERROR ;
    }

    nameObj = objv[2] ;
    objc -= 2 ;
    objv += 2 ;

    while (objc-- > 0) {
	Tcl_HashEntry *entry ;
	Tcl_Obj *relvarObj ;

	entry = Tcl_FindHashEntry(&relvarMap, (char *)*objv++) ;
	if (entry) {
	    relvarObj = Tcl_GetHashValue(entry) ;
	    assert(relvarObj != NULL) ;
	    Tcl_DecrRefCount(relvarObj) ;
	    Tcl_DeleteHashEntry(entry) ;
	} else {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"relvar, \"", nameObj, "\" does not exist",
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

    scriptObj = Tcl_NewStringObj("package require ral ", -1) ;
    Tcl_AppendStringsToObj(scriptObj, ral_version, "\n", NULL) ;

    /*
     * First emit a set of "relvar create" statements.
     */
    if (dumpType & schemaDump) {
	for (entry = Tcl_FirstHashEntry(&relvarMap, &search) ; entry ;
	    entry = Tcl_NextHashEntry(&search)) {
	    const char *relvarName ;
	    Tcl_Obj *relObj ;
	    Ral_Relation *relation ;
	    Ral_RelationHeading *heading ;
	    Tcl_Obj *headObj ;
	    Tcl_Obj *idObj ;

	    relvarName = Tcl_GetString(
		(Tcl_Obj *)Tcl_GetHashKey(&relvarMap, entry)) ;
	    if (patternObj &&
		!Tcl_StringMatch(relvarName, Tcl_GetString(patternObj))) {
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
		"::ral::relvar create ", relvarName,
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
	    const char *relvarName ;
	    Tcl_Obj *relObj ;
	    Ral_Relation *relation ;
	    Ral_Tuple **tupleVector ;
	    Ral_Tuple **lastVector ;

	    relvarName = Tcl_GetString(
		(Tcl_Obj *)Tcl_GetHashKey(&relvarMap, entry)) ;
	    if (patternObj &&
		!Tcl_StringMatch(relvarName, Tcl_GetString(patternObj))) {
		continue ;
	    }

	    relObj = Tcl_GetHashValue(entry) ;
	    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType)
		!= TCL_OK) {
		goto errorOut ;
	    }
	    relation = relObj->internalRep.otherValuePtr ;

	    tupleVector = relation->tupleVector ;
	    lastVector = tupleVector + relation->cardinality ;
	    for ( ; tupleVector != lastVector ; ++tupleVector) {
		Tcl_Obj *tupleNVList ;

		tupleNVList = tupleNameValueList(interp, *tupleVector) ;
		if (tupleNVList) {
		    Tcl_AppendStringsToObj(scriptObj,
			"::ral::relvar insert ", relvarName,
			" {", Tcl_GetString(tupleNVList), "}\n", NULL) ;
		    Tcl_DecrRefCount(tupleNVList) ;
		} else {
		    goto errorOut ;
		}
	    }
	}
    }

    Tcl_SetObjResult(interp, scriptObj) ;
    return TCL_OK ;

errorOut:
    Tcl_DecrRefCount(scriptObj) ;
    return TCL_ERROR ;
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

    /*
     * No return value. Use "relvar set"
     */
    Tcl_ResetResult(interp) ;
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
RelvarSetCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_HashEntry *entry ;
    Tcl_Obj *relVarObj ;
    Tcl_Obj *relValueObj ;

    /* relvar set relvarName ?relationValue? */
    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?relationValue?") ;
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

    if (objc == 4) {
	Ral_Relation *relvarRelation ;
	Ral_Relation *relvalRelation ;

	relValueObj = objv[3] ;

	if (Tcl_ConvertToType(interp, relVarObj, &Ral_RelationType) != TCL_OK ||
	    Tcl_ConvertToType(interp, relValueObj, &Ral_RelationType)
	    != TCL_OK) {
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
	relVarObj = relValueObj ;
    }

    Tcl_SetObjResult(interp, relVarObj) ;
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
	if (boolValue) {
	    if (relationUpdateTuple(interp, relation, index, objv[5])
		== TCL_OK) {
		++updated ;
	    } else {
		goto errorOut ;
	    }
	}
    }
    Tcl_UnsetVar(interp, Tcl_GetString(tupleNameObj), 0) ;

    Tcl_DecrRefCount(tupleNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    if (updated) {
	Tcl_InvalidateStringRep(relObj) ;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(updated)) ;
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
    static const struct cmdMap cmdTable[] = {
	{"create", RelvarCreateCmd},
	{"delete", RelvarDeleteCmd},
	{"destroy", RelvarDestroyCmd},
	{"dump", RelvarDumpCmd},
	{"insert", RelvarInsertCmd},
	{"names", RelvarNamesCmd},
	{"set", RelvarSetCmd},
	{"update", RelvarUpdateCmd},
	{NULL, NULL},
    } ;
    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObjStruct(interp, *(objv + 1), cmdTable,
	sizeof(struct cmdMap), "subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    return cmdTable[index].cmdFunc(interp, objc, objv) ;
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
    int idIndex ;
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
     * divisor tuples are contained in the mediator.  If they are, then that
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
	    "mediator heading must be the union of dividend heading and"
	    " divisor heading", -1) ;
	return TCL_ERROR ;
    }
    /*
     * create quotient
     */
    quot = relationNew(dend->heading) ;
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
    Ral_RelationHeading *heading ;
    Ral_TupleHeading *tupleHeading ;
    int c ;
    int *retainMap ;
    int retainCount ;
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
    heading = relation->heading ;
    tupleHeading = heading->tupleHeading ;

    objc -= 3 ;
    objv += 3 ;

    retainMap = (int *)ckalloc(tupleHeading->degree * sizeof(*retainMap)) ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	retainMap[c] = 1 ;
    }
    while (objc-- > 0) {
	int index ;

	index = tupleHeadingFindIndex(interp, tupleHeading, *objv++) ;
	if (index < 0) {
	    ckfree((char *)retainMap) ;
	    return TCL_ERROR ;
	}
	retainMap[index] = -1 ;
    }
    /*
     * Do this as a separate step, just in case the same attribute
     * is mentioned multiple times to be eliminated.
     */
    retainCount = 0 ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	if (retainMap[c] >= 0) {
	    retainMap[c] = retainCount++ ;
	}
    }

    elimTupleHeading = tupleHeadingNew(retainCount) ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	if (retainMap[c] >= 0 && tupleHeadingCopy(interp, tupleHeading,
	    c, c + 1, elimTupleHeading, retainMap[c]) != TCL_OK) {
	    ckfree((char *)retainMap) ;
	    tupleHeadingDelete(elimTupleHeading) ;
	    return TCL_ERROR ;
	}
    }

    elimHeading = relationHeadingNewRetained(heading, elimTupleHeading,
	retainMap) ;
    elimRelation = relationNew(elimHeading) ;
    relationReserve(elimRelation, relation->cardinality) ;

    for (card = relation->cardinality, tupleVector = relation->tupleVector ;
	card > 0 ; --card, ++tupleVector) {
	Ral_Tuple *srcTuple ;
	int degree ;

	srcTuple = *tupleVector ;
	elimTuple = tupleNew(elimTupleHeading) ;
	for (degree = 0 ; degree < tupleHeading->degree ; ++degree) {
	    if (retainMap[degree] >= 0) {
		Tcl_IncrRefCount(elimTuple->values[retainMap[degree]] =
		    srcTuple->values[degree]) ;
	    }
	}
	if (relationAppendTuple(NULL, elimRelation, elimTuple) != TCL_OK) {
	    tupleDelete(elimTuple) ;
	}
    }

    ckfree((char *)retainMap) ;
    Tcl_SetObjResult(interp, relationObjNew(elimRelation)) ;
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

    /*
     * relation extend relationValue tupleVarName
     *	    ?attr1 type1 expr1...attrN typeN exprN?
     */
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
    if (objc % 3 != 0) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "attribute / type / expression arguments must be given in triples",
	    -1) ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(varNameObj) ;
    /*
     * Make the new relation adding the extended attributes
     */
    extRelation = relationDup(relation, objc / 3) ;
    extTupleHeading = extRelation->heading->tupleHeading ;

    index = relation->heading->tupleHeading->degree ;
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	if (tupleHeadingInsertAttribute(interp, extTupleHeading, *v, *(v + 1),
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
	for (c = objc, v = objv + 2 ; c > 0 ; c -= 3, v += 3) {
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
    enum Ordering {
	SORT_ASCENDING,
	SORT_DESCENDING
    } ;
    static const char *orderKeyWords[] = {
	"ascending",
	"descending",
	NULL
    } ;

    Tcl_Obj *varNameObj ;
    Tcl_Obj *relObj ;
    Tcl_Obj *scriptObj ;
    Ral_Relation *relation ;
    int index ;
    int card ;
    Ral_Tuple **tupleVector ;
    Ral_Tuple **allocated = NULL ;
    int result = TCL_OK ;

    /* relation foreach tupleVarName relationValue ?ascending | descending?
     *	?attr-list? script */
    if (objc < 5 || objc > 7) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleVarName relationValue ?ascending | descending? ?attr-list?"
	    "script") ;
	return TCL_ERROR ;
    }

    varNameObj = objv[2] ;
    relObj = objv[3] ;

    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    if (objc == 5) {
	tupleVector = relation->tupleVector ;
	scriptObj = objv[4] ;
    } else if (objc == 6) {
	allocated = tupleVector = relationSortAscending(interp, relation,
	    objv[4]) ;
	if (allocated == NULL) {
	    return TCL_ERROR ;
	}
	scriptObj = objv[5] ;
    } else /* objc == 7 */ {
	if (Tcl_GetIndexFromObj(interp, objv[4], orderKeyWords, "ordering", 0,
	    &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	allocated = tupleVector = index == SORT_ASCENDING ?
	    relationSortAscending(interp, relation, objv[5]) :
	    relationSortDescending(interp, relation, objv[5]) ;
	if (allocated == NULL) {
	    return TCL_ERROR ;
	}
	scriptObj = objv[6] ;
    }

    /*
     * Hang onto these objects in case something strange happens in
     * the script execution.
     */
    Tcl_IncrRefCount(varNameObj) ;
    Tcl_IncrRefCount(relObj) ;
    Tcl_IncrRefCount(scriptObj) ;

    Tcl_ResetResult(interp) ;

    for (card = relation->cardinality ; card > 0 ; --card, ++tupleVector) {
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
    if (allocated) {
	ckfree((char *)allocated) ;
    }
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_DecrRefCount(relObj) ;
    Tcl_DecrRefCount(scriptObj) ;

    return result ;
}

static int
RelationGroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *rel ;
    Ral_TupleHeading *tupleHeading ;
    Tcl_Obj *newAttrObj ;
    Ral_TupleHeading *grpAttrTupleHeading ;
    Ral_RelationHeading *grpAttrRelationHeading ;
    char *relAttrMap ;
    int c ;
    int d ;
    int attrIndex ;
    int where ;
    Ral_TupleHeading *grpTupleHeading ;
    Ral_RelationHeading *grpHeading ;
    Ral_Relation *group ;
    Tcl_HashTable groupHash ;
    Ral_Tuple *grpAttrTuple ;

    /* relation group relation newattribute ?attr1 attr2 ...? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation newattribute ?attr1 attr2 ...?") ;
	return TCL_ERROR ;
    }
    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    rel = relObj->internalRep.otherValuePtr ;
    tupleHeading = rel->heading->tupleHeading ;
    newAttrObj = objv[3] ;
    objc -= 4 ;
    objv += 4 ;

    if (objc > tupleHeading->degree) {
	Tcl_SetStringObj(Tcl_GetObjResult(interp), 
	    "attempt to group more attributes than exist in the relation", -1) ;
    }

    /*
     * Examine the attribute arguments to determine if the attributes exist and
     * build a map to use later to determine which attributes will be in the
     * new relation valued attribute and which will remain in the tuple.
     * Construct the tuple heading for the new relation valued attribute.
     */
    relAttrMap = ckalloc(tupleHeading->degree) ;
    memset(relAttrMap, 0 , tupleHeading->degree) ;
    grpAttrTupleHeading = tupleHeadingNew(objc) ;
    for (d = 0 ; d < objc ; ++d) {
	attrIndex = tupleHeadingFindIndex(interp, tupleHeading, *objv++) ;
	if (attrIndex < 0 || tupleHeadingCopy(interp, tupleHeading,
	    attrIndex, attrIndex + 1, grpAttrTupleHeading, d) != TCL_OK) {
	    tupleHeadingDelete(grpAttrTupleHeading) ;
	    ckfree(relAttrMap) ;
	    return TCL_ERROR ;
	} else {
	    relAttrMap[attrIndex] = 1 ;
	}
    }
    grpAttrRelationHeading = relationHeadingNew(grpAttrTupleHeading, 1) ;
    /*
     * HERE - do something about the identifiers.
     */
    fixedIntVectorIdentityMap(grpAttrRelationHeading->idVector,
	grpAttrTupleHeading->degree) ;

    /*
     * The grouped relation has a heading that contains all the attributes
     * minus those that go into the grouping relation plus one for the
     * new relation valued attribute.
     */
    grpTupleHeading = tupleHeadingNew(tupleHeading->degree - objc + 1) ;
    where = 0 ;
    for (d = 0 ; d < tupleHeading->degree ; ++d) {
	if (!relAttrMap[d]) {
	    tupleHeadingCopy(interp, tupleHeading, d, d + 1, grpTupleHeading,
		where++) ;
	}
    }
    if (tupleHeadingInsertName(interp, grpTupleHeading, newAttrObj, where)
	!= TCL_OK) {
	ckfree(relAttrMap) ;
	relationHeadingDelete(grpAttrRelationHeading) ;
	tupleHeadingDelete(grpTupleHeading) ;
	return TCL_ERROR ;
    }
    attributeCtorRelationType(grpTupleHeading->attrVector + where, newAttrObj,
	grpAttrRelationHeading) ;

    grpHeading = relationHeadingNew(grpTupleHeading, 1) ;
    /*
     * HERE - do something about the identifiers.
     */
    fixedIntVectorIdentityMap(grpHeading->idVector, grpTupleHeading->degree) ;
    group = relationNew(grpHeading) ;

    /*
     * Build up the tuples for the new grouped relation.
     */
    Tcl_InitHashTable(&groupHash, TCL_STRING_KEYS) ;
    for (c = 0 ; c < rel->cardinality ; ++c) {
	Ral_Tuple *tuple = rel->tupleVector[c] ;
	Tcl_DString relKey ;
	int place = 0 ;
	Tcl_HashEntry *entry ;
	int newPtr ;

	grpAttrTuple = tupleNew(grpAttrTupleHeading) ;
	Tcl_DStringInit(&relKey) ;
	for (d = 0 ; d < tupleHeading->degree ; ++d) {
	    if (relAttrMap[d]) {
		tupleCopyValues(tuple, d, d + 1, grpAttrTuple, place++) ;
	    } else {
		Tcl_DStringAppend(&relKey, Tcl_GetString(tuple->values[d]),
		    -1) ;
	    }
	}

	entry = Tcl_CreateHashEntry(&groupHash, Tcl_DStringValue(&relKey),
	    &newPtr) ;
	Tcl_DStringFree(&relKey) ;
	if (newPtr) {
	    /*
	     * Build a new tuple in the grouped relation.
	     */
	    Ral_Relation *grpAttrRel = relationNew(grpAttrRelationHeading) ;
	    Ral_Tuple *grpTuple = tupleNew(grpTupleHeading) ;
	    int place = 0 ;

	    Tcl_SetHashValue(entry, group->cardinality) ;

	    relationReserve(grpAttrRel, 1) ;
	    if (relationAppendTuple(interp, grpAttrRel, grpAttrTuple)
		!= TCL_OK) {
		relationDelete(grpAttrRel) ;
		tupleDelete(grpTuple) ;
		goto errorOut ;
	    }

	    grpTuple = tupleNew(grpTupleHeading) ;
	    for (d = 0 ; d < tupleHeading->degree ; ++d) {
		if (!relAttrMap[d]) {
		    tupleCopyValues(tuple, d, d + 1, grpTuple, place++) ;
		}
	    }
	    Tcl_IncrRefCount(grpTuple->values[grpTupleHeading->degree - 1] =
		relationObjNew(grpAttrRel)) ;
	    relationReserve(group, 1) ;
	    if (relationAppendTuple(interp, group, grpTuple) != TCL_OK) {
		relationDelete(grpAttrRel) ;
		tupleDelete(grpTuple) ;
		goto errorOut ;
	    }
	} else {
	    /*
	     * Added a tuple to the relation valued attribute.
	     */
	    int index = (int)Tcl_GetHashValue(entry) ;
	    assert(index < group->cardinality) ;
	    Ral_Tuple *grpTuple = group->tupleVector[index] ;
	    Tcl_Obj *grpAttrRelObj =
		grpTuple->values[grpTupleHeading->degree - 1] ;
	    Ral_Relation *grpAttrRel =
		grpAttrRelObj->internalRep.otherValuePtr ;
	    relationReserve(grpAttrRel, 1) ;
	    if (relationAppendTuple(interp, grpAttrRel, grpAttrTuple)
		!= TCL_OK) {
		goto errorOut ;
	    }
	    /*
	     * Make sure to clear out the string rep since various
	     * comparisons can cause it to be recomputed between the
	     * time that the relation object is first put into the
	     * tuple and here.
	     */
	    Tcl_InvalidateStringRep(grpAttrRelObj) ;
	}
    }

    ckfree(relAttrMap) ;
    Tcl_DeleteHashTable(&groupHash) ;

    Tcl_SetObjResult(interp, relationObjNew(group)) ;
    return TCL_OK ;

errorOut:
    ckfree(relAttrMap) ;
    Tcl_DeleteHashTable(&groupHash) ;
    tupleDelete(grpAttrTuple) ;
    relationDelete(group) ;
    return TCL_ERROR ;
}

static int
RelationHeadingCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    /* relation heading relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp,
	tupleHeadingAttrsToList(interp, relation->heading->tupleHeading)) ;
    return TCL_OK ;
}

static int
RelationIdentifiersCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation *relation ;

    /* relation identifiers relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp,
	relationHeadingIdsToList(interp, relation->heading)) ;
    return TCL_OK ;
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

	attrOrderMapDelete(order) ;
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
RelationIsemptyCmd(
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

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(relation->cardinality == 0)) ;
    return TCL_OK ;
}

static int
RelationIsnotemptyCmd(
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

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(relation->cardinality != 0)) ;
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
    int r1degree ;
    int r2degree ;
    Ral_RelationJoinMap *jmap ;
    Ral_TupleHeading *joinTupleHeading ;
    Ral_FixedIntVector r2AttrMap ;
    int i ;
    int where ;
    int card2 ;
    Ral_RelationHeading *joinHeading ;
    Ral_Relation *join ;

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
    r1degree = r1->heading->tupleHeading->degree ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;
    r2degree = r2->heading->tupleHeading->degree ;

    objc -= 4 ;
    objv += 4 ;

    if (objc == 0) {
	jmap = relationJoinMapNew(min(r1degree, r2degree),
	    r1->cardinality * r2->cardinality) ;
	tupleHeadingCommonAttrs(r1->heading->tupleHeading,
	    r2->heading->tupleHeading, jmap) ;
    } else {
	jmap = relationJoinMapNew(objc, r1->cardinality * r2->cardinality) ;
	if (tupleHeadingJoinAttrs(interp, r1->heading->tupleHeading,
	    r2->heading->tupleHeading, objc, objv, jmap) != TCL_OK) {
	    relationJoinMapDelete(jmap) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Find the tuples that match.
     */
    relationFindJoinTuples(r1, r2, jmap) ;
    /*
     * Create a mapping that specifies how the attributes of the second
     * relation are to be mapped into the resulting join. The map is
     * "r2degree" in length and is indexed by the attribute order in
     * the second relation. The value stored in the map is the attribute
     * index in the joined relation or -1 if the attribute is not to be
     * included in the joined relation (i.e. if the attribute is one of
     * the eliminated attributes of the join).
     */
    fixedIntVectorCtor(&r2AttrMap, r2degree) ;
    /*
     * First, the join map contains the attribute index of the join
     * attributes that are to be eliminated from the result.
     */
    memset(r2AttrMap.vector, 0, r2AttrMap.count * sizeof(*r2AttrMap.vector)) ;
    for (i = 0 ; i < jmap->attrMap.count ; ++i) {
	r2AttrMap.vector[jmap->attrMap.vector[i].index2] = -1 ;
    }
    /*
     * Compose the matching tuples into the join relation.
     * All the attributes from the first relation are always part of the join.
     */
    joinTupleHeading = tupleHeadingNew(r1degree + r2degree -
	jmap->attrMap.count) ;
    tupleHeadingCopy(interp, r1->heading->tupleHeading, 0, r1degree,
	joinTupleHeading, 0) ;
    /*
     * The second relation is added on to the end, less those attributes
     * that are eliminated.
     */
    where = r1degree ;
    for (card2 = 0 ; card2 < r2degree ; ++card2) {
	if (r2AttrMap.vector[card2] != -1) {
	    if (tupleHeadingCopy(interp, r2->heading->tupleHeading,
		card2, card2 + 1, joinTupleHeading, where) == TCL_OK) {
		r2AttrMap.vector[card2] = where++ ;
	    } else {
		tupleHeadingDelete(joinTupleHeading) ;
		fixedIntVectorDtor(&r2AttrMap) ;
		relationJoinMapDelete(jmap) ;
		return TCL_ERROR ;
	    }
	}
    }
    /*
     * The identifiers for the resulting join are the cross product of
     * the identifiers for the two participating relations minus any
     * join attributes from r2 that are eliminated from the join.
     */
    joinHeading = relationHeadingNew(joinTupleHeading,
	r1->heading->idCount * r2->heading->idCount) ;
    relationHeadingJoinIdentifiers(joinHeading, r1->heading, r2->heading,
	&r2AttrMap) ;
    join = relationNew(joinHeading) ;
    relationReserve(join, jmap->tupleMap.count) ;
    /*
     * Step through the matches found in the join map and compose the
     * indicated tuples.
     */
    for (i = 0 ; i < jmap->tupleMap.count ; ++i) {
	Ral_Tuple *r1Tuple ;
	Ral_Tuple *r2Tuple ;
	Ral_Tuple *joinTuple ;
	int j ;

	r1Tuple = r1->tupleVector[jmap->tupleMap.vector[i].index1] ;
	r2Tuple = r2->tupleVector[jmap->tupleMap.vector[i].index2] ;
	joinTuple = tupleNew(joinTupleHeading) ;
	/*
	 * Take all the values from the first relation's tuple.
	 */
	tupleCopyValues(r1Tuple, 0, r1degree, joinTuple, 0) ;
	/*
	 * Take the values from the second relation's tuple, eliminating
	 * those that are part of the natural join.
	 */
	for (j = 0 ; j < r2degree ; ++j) {
	    int r2Pos = r2AttrMap.vector[j] ;
	    if (r2Pos != -1) {
		tupleCopyValues(r2Tuple, j, j + 1, joinTuple, r2Pos) ;
	    }
	}

	if (relationAppendTuple(interp, join, joinTuple) != TCL_OK) {
	    tupleDelete(joinTuple) ;
	    goto errorOut ;
	}
    }

    fixedIntVectorDtor(&r2AttrMap) ;
    relationJoinMapDelete(jmap) ;
    Tcl_SetObjResult(interp, relationObjNew(join)) ;
    return TCL_OK ;

errorOut:
    fixedIntVectorDtor(&r2AttrMap) ;
    relationJoinMapDelete(jmap) ;
    relationDelete(join) ;
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
    int *retainMap ;
    int retainCount ;
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

    retainMap = (int *)ckalloc(tupleHeading->degree * sizeof(*retainMap)) ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	retainMap[c] = -1 ;
    }
    while (objc-- > 0) {
	int index ;

	index = tupleHeadingFindIndex(interp, tupleHeading, *objv++) ;
	if (index < 0) {
	    ckfree((char *)retainMap) ;
	    return TCL_ERROR ;
	}
	retainMap[index] = 1 ;
    }
    /*
     * Do this as a separate step, just in case the same attribute
     * is mentioned multiple times to be projected.
     */
    retainCount = 0 ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	if (retainMap[c] >= 0) {
	    retainMap[c] = retainCount++ ;
	}
    }

    projTupleHeading = tupleHeadingNew(retainCount) ;
    for (c = 0 ; c < tupleHeading->degree ; ++c) {
	if (retainMap[c] >= 0 && tupleHeadingCopy(interp, tupleHeading,
	    c, c + 1, projTupleHeading, retainMap[c]) != TCL_OK) {
	    ckfree((char *)retainMap) ;
	    tupleHeadingDelete(projTupleHeading) ;
	    return TCL_ERROR ;
	}
    }

    projHeading = relationHeadingNewRetained(heading, projTupleHeading,
	retainMap) ;
    projRelation = relationNew(projHeading) ;
    relationReserve(projRelation, relation->cardinality) ;

    for (card = relation->cardinality, tupleVector = relation->tupleVector ;
	card > 0 ; --card, ++tupleVector) {
	Ral_Tuple *srcTuple ;
	int degree ;

	srcTuple = *tupleVector ;
	projTuple = tupleNew(projTupleHeading) ;
	for (degree = 0 ; degree < tupleHeading->degree ; ++degree) {
	    if (retainMap[degree] >= 0) {
		Tcl_IncrRefCount(projTuple->values[retainMap[degree]] =
		    srcTuple->values[degree]) ;
	    }
	}
	if (relationAppendTuple(NULL, projRelation, projTuple) != TCL_OK) {
	    tupleDelete(projTuple) ;
	}
    }

    ckfree((char *)retainMap) ;
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

static int
RelationSemijoinCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation *r2 ;
    int r1degree ;
    int r2degree ;
    Ral_RelationJoinMap *jmap ;
    int i ;
    Ral_Relation *semijoin ;

    /* relation semijoin relation1 relation2 ?joinAttrs1 joinAttrs2 ... ? */
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
    r1degree = r1->heading->tupleHeading->degree ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;
    r2degree = r2->heading->tupleHeading->degree ;

    objc -= 4 ;
    objv += 4 ;

    if (objc == 0) {
	jmap = relationJoinMapNew(min(r1degree, r2degree),
	    r1->cardinality * r2->cardinality) ;
	tupleHeadingCommonAttrs(r1->heading->tupleHeading,
	    r2->heading->tupleHeading, jmap) ;
    } else {
	jmap = relationJoinMapNew(objc, r1->cardinality * r2->cardinality) ;
	if (tupleHeadingJoinAttrs(interp, r1->heading->tupleHeading,
	    r2->heading->tupleHeading, objc, objv, jmap) != TCL_OK) {
	    relationJoinMapDelete(jmap) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Find the tuples that match.
     */
    relationFindJoinTuples(r1, r2, jmap) ;
    /*
     * The semijoin has the same relation heading as the first relation.
     */
    semijoin = relationNew(r1->heading) ;
    relationReserve(semijoin, jmap->tupleMap.count) ;
    /*
     * Step through the matches found in the join map and compose the
     * indicated tuples. We are only interesting in the tuple from the
     * first relation.
     */
    for (i = 0 ; i < jmap->tupleMap.count ; ++i) {
	Ral_Tuple *r1Tuple ;

	r1Tuple = r1->tupleVector[jmap->tupleMap.vector[i].index1] ;
	relationAppendTuple(NULL, semijoin, r1Tuple) ;
    }

    relationJoinMapDelete(jmap) ;
    Tcl_SetObjResult(interp, relationObjNew(semijoin)) ;
    return TCL_OK ;
}

static int
RelationSemiminusCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation *r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation *r2 ;
    int r1degree ;
    int r2degree ;
    Ral_RelationJoinMap *jmap ;
    int i ;
    Ral_Relation *semiminus ;
    char *includeTupleMap ;

    /* relation semiminus relation1 relation2 ?joinAttrs1 joinAttrs2 ... ? */
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
    r1degree = r1->heading->tupleHeading->degree ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;
    r2degree = r2->heading->tupleHeading->degree ;

    objc -= 4 ;
    objv += 4 ;

    if (objc == 0) {
	jmap = relationJoinMapNew(min(r1degree, r2degree),
	    r1->cardinality * r2->cardinality) ;
	tupleHeadingCommonAttrs(r1->heading->tupleHeading,
	    r2->heading->tupleHeading, jmap) ;
    } else {
	jmap = relationJoinMapNew(objc, r1->cardinality * r2->cardinality) ;
	if (tupleHeadingJoinAttrs(interp, r1->heading->tupleHeading,
	    r2->heading->tupleHeading, objc, objv, jmap) != TCL_OK) {
	    relationJoinMapDelete(jmap) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Find the tuples that match.
     */
    relationFindJoinTuples(r1, r2, jmap) ;
    /*
     * The semiminus has the same relation heading as the first relation.
     */
    semiminus = relationNew(r1->heading) ;
    /*
     * So we put together a mapping to know which tuple to include in the
     * semiminus relation.
     */
    includeTupleMap = (char *)ckalloc(r1->cardinality *
	sizeof(*includeTupleMap)) ;
    memset(includeTupleMap, 1, r1->cardinality * sizeof(*includeTupleMap)) ;
    for (i = 0 ; i < jmap->tupleMap.count ; ++i) {
	assert(jmap->tupleMap.vector[i].index1 < r1->cardinality) ;
	includeTupleMap[jmap->tupleMap.vector[i].index1] = 0 ;
    }
    relationJoinMapDelete(jmap) ;
    /*
     * Step through the tuples of the first relation and add only those
     * that did not match the join operation.
     */
    for (i = 0 ; i < r1->cardinality ; ++i) {
	if (includeTupleMap[i]) {
	    relationReserve(semiminus, 1) ;
	    if (relationAppendTuple(interp, semiminus, r1->tupleVector[i])
		!= TCL_OK) {
		goto errorOut ;
	    }
	}
    }

    ckfree(includeTupleMap) ;
    Tcl_SetObjResult(interp, relationObjNew(semiminus)) ;
    return TCL_OK ;

errorOut:
    ckfree(includeTupleMap) ;
    relationDelete(semiminus) ;
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
    Tcl_Obj *typeObj ;
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

    /* relation summarize relationValue perRelation attr type cmd initValue */
    if (objc != 8) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue perRelation attr type cmd initValue") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    perObj = *(objv + 3) ;
    attrObj = *(objv + 4) ;
    typeObj = *(objv + 5) ;
    cmdObj = *(objv + 6) ;
    initObj = *(objv + 7) ;

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
	    "the \"per\" relation heading must be a subset of the summarized"
	    " relation", -1) ;
	return TCL_ERROR ;
    }

    /*
     * Construct the heading for the result. It the heading of the
     * "per" relation plus the summary attribute.
     * We much check if we are summarizing over the "DEE" relation.
     * In that case the only attribute of the result will be the
     * summarizing attribute and it then must be the only attribute of
     * the single identifier of the result.
     */
    if (perTupleHeading->degree) {
	sumHeading = relationHeadingDup(perRelation->heading, 1) ;
	sumTupleHeading = sumHeading->tupleHeading ;
	if (tupleHeadingInsertAttribute(interp, sumTupleHeading, attrObj,
	    typeObj, perTupleHeading->degree) != TCL_OK) {
	    relationHeadingDelete(sumHeading) ;
	    return TCL_ERROR ;
	}
    } else {
	sumTupleHeading = tupleHeadingNew(1) ;
	if (tupleHeadingInsertAttribute(interp, sumTupleHeading, attrObj,
	    typeObj, 0) != TCL_OK) {
	    tupleHeadingDelete(sumTupleHeading) ;
	    return TCL_ERROR ;
	}
	sumHeading = relationHeadingNew(sumTupleHeading, 1) ;
	fixedIntVectorIdentityMap(sumHeading->idVector, 1) ;
    }

    sumAttribute = sumTupleHeading->attrVector + perTupleHeading->degree ;
    if (attributeConvertValue(interp, sumAttribute, initObj) != TCL_OK) {
	relationHeadingDelete(sumHeading) ;
	return TCL_ERROR ;
    }

    sumRelation = relationNew(sumHeading) ;
    relationReserve(sumRelation, perRelation->cardinality) ;

    /*
     * Create a vector containing a copy of the initialization object. This
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
    Ral_RelationHeading *prodHeading ;
    Ral_Relation *product ;

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

    prodHeading = relationHeadingNewUnion(interp, r1->heading, r2->heading) ;
    if (prodHeading == NULL) {
	return TCL_ERROR ;
    }

    product = relationNew(prodHeading) ;
    relationReserve(product, r1->cardinality * r2->cardinality) ;

    if (relationMultTuples(interp, product, r1, r2) != TCL_OK) {
	goto errorOut ;
    }

    objc -= 4 ;
    objv += 4 ;
    while (objc-- > 0) {
	r1 = product ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationType) != TCL_OK) {
	    goto errorOut ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	prodHeading = relationHeadingNewUnion(interp, r1->heading,
	    r2->heading) ;
	if (prodHeading == NULL) {
	    return TCL_ERROR ;
	}

	product = relationNew(prodHeading) ;
	relationReserve(product, r1->cardinality * r2->cardinality) ;

	if (relationMultTuples(interp, product, r1, r2) != TCL_OK) {
	    goto errorOut ;
	}

	relationDelete(r1) ;
    }

    Tcl_SetObjResult(interp, relationObjNew(product)) ;
    return TCL_OK ;

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
RelationUngroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation *rel ;
    Ral_TupleHeading *tupleHeading ;
    Tcl_Obj *attrObj ;
    int ungrpIndex ;
    Ral_RelationHeading *attrHeading ;
    Ral_TupleHeading *ungrpTupleHeading ;
    Ral_RelationHeading *ungrpHeading ;
    Ral_Relation *ungrp ;
    int card ;
    Ral_Tuple **tupleVector ;

    /* relation ungroup relation attribute */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation attribute") ;
	return TCL_ERROR ;
    }
    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationType) != TCL_OK) {
	return TCL_ERROR ;
    }
    rel = relObj->internalRep.otherValuePtr ;
    tupleHeading = rel->heading->tupleHeading ;
    attrObj = *(objv + 3) ;

    /*
     * Check that the attribute exists and is a relation type attribute
     */
    ungrpIndex = tupleHeadingFindIndex(interp, tupleHeading, attrObj) ;
    if (ungrpIndex < 0) {
	return TCL_ERROR ;
    }
    if (tupleHeading->attrVector[ungrpIndex].attrType != Relation_Type) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "attribute, \"", Tcl_GetString(attrObj),
	    "\", is not a Relation type", NULL) ;
	return TCL_ERROR ;
    }
    attrHeading = tupleHeading->attrVector[ungrpIndex].relationHeading ;
    /*
     * The ungrouped relation has a heading with all the attributes
     * of the original plus that of the ungrouped attribute minus one.
     */
    ungrpTupleHeading = tupleHeadingNew(tupleHeading->degree +
	attrHeading->tupleHeading->degree - 1) ;
	/* copy up to the attribute to be ungrouped */
    if (tupleHeadingCopy(interp, tupleHeading, 0, ungrpIndex,
	    ungrpTupleHeading, 0) != TCL_OK ||
	/* copy the attributes after the one to be ungrouped */
	tupleHeadingCopy(interp, tupleHeading, ungrpIndex + 1,
	    tupleHeading->degree, ungrpTupleHeading, ungrpIndex)
	    != TCL_OK ||
	/* copy the heading from the ungrouped attribute itself */
	tupleHeadingCopy(interp, attrHeading->tupleHeading, 0,
	    attrHeading->tupleHeading->degree, ungrpTupleHeading,
	    tupleHeading->degree - 1) != TCL_OK) {
	tupleHeadingDelete(ungrpTupleHeading) ;
	return TCL_ERROR ;
    }
    /*
     * The number of identifiers for the new relation is the product
     * of the number of the original relation and that of the ungrouped
     * attribute.
     */
    ungrpHeading = relationHeadingNew(ungrpTupleHeading,
	rel->heading->idCount * attrHeading->idCount) ;
    relationHeadingUngroupIdentifiers(ungrpHeading, rel->heading, attrHeading,
	ungrpIndex) ;
    ungrp = relationNew(ungrpHeading) ;
    /*
     * Now put together the tuples.
     */
    for (card = rel->cardinality, tupleVector = rel->tupleVector ; card > 0 ;
	--card, ++tupleVector) {
	Ral_Tuple *tuple = *tupleVector ;
	Tcl_Obj *ungrpObj = tuple->values[ungrpIndex] ;
	Ral_Relation *ungrpValue ;
	int c ;
	Ral_Tuple **tv ;

	if (Tcl_ConvertToType(interp, ungrpObj, &Ral_RelationType) != TCL_OK) {
	    relationDelete(ungrp) ;
	    return TCL_ERROR ;
	}
	ungrpValue = ungrpObj->internalRep.otherValuePtr ;
	relationReserve(ungrp, ungrpValue->cardinality) ;

	for (c = ungrpValue->cardinality, tv = ungrpValue->tupleVector ; c > 0 ;
	    --c, ++tv) {
	    Ral_Tuple *ungrpTuple = tupleNew(ungrpTupleHeading) ;

	    tupleCopyValues(tuple, 0, ungrpIndex, ungrpTuple, 0) ;
	    tupleCopyValues(tuple, ungrpIndex + 1, tupleHeading->degree,
		ungrpTuple, ungrpIndex) ;
	    tupleCopyValues(*tv, 0, attrHeading->tupleHeading->degree,
		ungrpTuple, tupleHeading->degree - 1) ;
	    if (relationAppendTuple(interp, ungrp, ungrpTuple) != TCL_OK) {
		tupleDelete(ungrpTuple) ;
	    }
	}
    }

    Tcl_SetObjResult(interp, relationObjNew(ungrp)) ;
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
    static const struct cmdMap cmdTable[] = {
	{"cardinality", RelationCardinalityCmd},
	{"degree", RelationDegreeCmd},
	{"divide", RelationDivideCmd},
	{"eliminate", RelationEliminateCmd},
	{"emptyof", RelationEmptyofCmd},
	{"extend", RelationExtendCmd},
	{"foreach", RelationForeachCmd},
	{"group", RelationGroupCmd},
	{"heading", RelationHeadingCmd},
	{"identifiers", RelationIdentifiersCmd},
	{"intersect", RelationIntersectCmd},
	{"is", RelationIsCmd},
	{"isempty", RelationIsemptyCmd},
	{"isnotempty", RelationIsnotemptyCmd},
	{"join", RelationJoinCmd},
	{"list", RelationListCmd},
	{"minus", RelationMinusCmd},
	{"project", RelationProjectCmd},
	{"rename", RelationRenameCmd},
	{"restrict", RelationRestrictCmd},
	{"semijoin", RelationSemijoinCmd},
	{"semiminus", RelationSemiminusCmd},
	{"summarize", RelationSummarizeCmd},
	{"times", RelationTimesCmd},
	{"tuple", RelationTupleCmd},
	{"ungroup", RelationUngroupCmd},
	{"union", RelationUnionCmd},
	{NULL, NULL},
    } ;

    int index ;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "subcommand ?arg? ...") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObjStruct(interp, *(objv + 1), cmdTable,
	sizeof(struct cmdMap), "subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    return cmdTable[index].cmdFunc(interp, objc, objv) ;
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

    Tcl_PkgProvide(interp, "ral", ral_version) ;

    return TCL_OK ;
}
