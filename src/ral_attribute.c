/*
This software is copyrighted 2005 -2011 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_attribute.c,v $
$Revision: 1.29 $
$Date: 2012/02/26 19:09:04 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ral_attribute.h"
#include "ral_utils.h"
#include "ral_tupleheading.h"
#include "ral_tupleobj.h"
#include "ral_relationobj.h"
#include "tclTomMath.h"

/*
MACRO DEFINITIONS
*/
#define	COUNTOF(a)  (sizeof(a) / sizeof(a[0]))

/*
TYPE DEFINITIONS
*/
/*
 * These declarations and the forward function references below form a "shadow"
 * typing system that is used by TclRAL. The idea here is to divorce TclRAL
 * from the internals of Tcl types. This is done by eliminating the use of
 * Tcl_ObjType and removing all the calls to Tcl_ConvertToType() for ordinary
 * Tcl types. Tcl_ConvertToType() is used for "Relation" and "Tuple" types, but
 * then we are priviledged to know the internals of those types, after all this
 * package creates those types. In order to remove the knowledge of Tcl type
 * internals, the Tcl_GetXXXFromObj() functions are used to retrieve values. It
 * turns out that TclRAL only uses three aspects of types, is a string
 * coercible to a type, are two values equal and what is the relative order of
 * two values (assuming the domain of the values is fully ordered).
 *
 * The type names chosen for the TclRAL types are the same as those used
 * for Tcl native types, just to avoid (or perhaps create) confusion. Not
 * all Tcl types are supported. The supported types are the important ones,
 * e.g. the numeric and string types. Lists and Dicts are supported but
 * ordering and equality are a bit less well defined for those types.
 * Each type supplies three functions to perform the require tests and all
 * of this is assembled into a table sorted by type name.
 */
typedef int (*IsATypeFunc)(Tcl_Interp *, Tcl_Obj *) ;
typedef int (*IsEqualFunc)(Tcl_Obj *, Tcl_Obj *) ;
typedef int (*CompareFunc)(Tcl_Obj *, Tcl_Obj *) ;
typedef unsigned (*HashFunc)(Tcl_Obj *) ;
struct ral_type {
    char const *typeName ;
    IsATypeFunc isa ;
    IsEqualFunc isequal ;
    CompareFunc compare ;
    HashFunc hash ;
} ;

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static struct ral_type *findRalType(char const *) ;

static int isABoolean(Tcl_Interp *, Tcl_Obj *) ;
static int booleanEqual(Tcl_Obj *, Tcl_Obj *) ;
static int booleanCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned booleanHash(Tcl_Obj *) ;

static int isAByteArray(Tcl_Interp *, Tcl_Obj *) ;
static int byteArrayEqual(Tcl_Obj *, Tcl_Obj *) ;
static int byteArrayCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned byteArrayHash(Tcl_Obj *) ;

static int isAnInt(Tcl_Interp *, Tcl_Obj *) ;
static int intEqual(Tcl_Obj *, Tcl_Obj *) ;
static int intCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned intHash(Tcl_Obj *) ;

static int isALong(Tcl_Interp *, Tcl_Obj *) ;
static int longEqual(Tcl_Obj *, Tcl_Obj *) ;
static int longCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned longHash(Tcl_Obj *) ;

static int isADouble(Tcl_Interp *, Tcl_Obj *) ;
static int doubleEqual(Tcl_Obj *, Tcl_Obj *) ;
static int doubleCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned doubleHash(Tcl_Obj *) ;

static int isABignum(Tcl_Interp *, Tcl_Obj *) ;
static int bignumEqual(Tcl_Obj *, Tcl_Obj *) ;
static int bignumCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned bignumHash(Tcl_Obj *) ;

#   ifndef NO_WIDE_TYPE
static int isAWideInt(Tcl_Interp *, Tcl_Obj *) ;
static int wideIntEqual(Tcl_Obj *, Tcl_Obj *) ;
static int wideIntCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned wideIntHash(Tcl_Obj *) ;
#   endif

static int isAList(Tcl_Interp *, Tcl_Obj *) ;
static int listEqual(Tcl_Obj *, Tcl_Obj *) ;
static int listCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned listHash(Tcl_Obj *) ;

static int isADict(Tcl_Interp *, Tcl_Obj *) ;
static int dictEqual(Tcl_Obj *, Tcl_Obj *) ;
static int dictCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned dictHash(Tcl_Obj *) ;

static int isAString(Tcl_Interp *, Tcl_Obj *) ;
static int stringEqual(Tcl_Obj *, Tcl_Obj *) ;
static int stringCompare(Tcl_Obj *, Tcl_Obj *) ;
static unsigned stringHash(Tcl_Obj *) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/
char ral_relationTypeName[] = "Relation" ;
char ral_tupleTypeName[] = "Tuple" ;

/*
STATIC DATA ALLOCATION
*/
/*
 * This table must be in "typeName" order as "bsearch()" is used to
 * find entries.
 */
static struct ral_type const Ral_Types[] = {
    {"bignum", isABignum, bignumEqual, bignumCompare, bignumHash},
    {"boolean", isABoolean, booleanEqual, booleanCompare, booleanHash},
    {"bytearray", isAByteArray, byteArrayEqual, byteArrayCompare,
            byteArrayHash},
    {"dict", isADict, dictEqual, dictCompare, dictHash},
    {"double", isADouble, doubleEqual, doubleCompare, doubleHash},
    {"int", isAnInt, intEqual, intCompare, intHash},
    {"list", isAList, listEqual, listCompare, listHash},
    {"long", isALong, longEqual, longCompare, longHash},
    {"string", isAString, stringEqual, stringCompare, stringHash},
#	ifndef NO_WIDE_TYPE
    {"wideInt", isAWideInt, wideIntEqual, wideIntCompare, wideIntHash},
#	endif
} ;

static char const openList = '{' ;
static char const closeList = '}' ;

/*
FUNCTION DEFINITIONS
*/

Ral_Attribute
Ral_AttributeNewTclType(
    char const *name,
    char const *typeName)
{
    Ral_Attribute a ;
    struct ral_type *ralType = findRalType(typeName) ;

    if (ralType == NULL) {
	return NULL ;
    }
    /* +1 for the NUL terminator */
    a = (Ral_Attribute)ckalloc(sizeof(*a) + strlen(name) + 1) ;
    a->name = strcpy((char *)(a + 1), name) ;
    a->typeName = ralType->typeName ;
    a->attrType = Tcl_Type ;
    a->heading = NULL ;

    return a ;
}

Ral_Attribute
Ral_AttributeNewTupleType(
    const char *name,
    Ral_TupleHeading heading)
{
    Ral_Attribute a ;

    a = (Ral_Attribute)ckalloc(sizeof(*a) + strlen(name) + 1) ;
    a->name = strcpy((char *)(a + 1), name) ;
    a->typeName = ral_tupleTypeName ;
    a->attrType = Tuple_Type ;
    Ral_TupleHeadingReference(a->heading = heading) ;

    return a ;
}

Ral_Attribute
Ral_AttributeNewRelationType(
    const char *name,
    Ral_TupleHeading heading)
{
    Ral_Attribute a ;

    a = (Ral_Attribute)ckalloc(sizeof(*a) + strlen(name) + 1) ;
    a->name = strcpy((char *)(a + 1), name) ;
    a->typeName = ral_relationTypeName ;
    a->attrType = Relation_Type ;
    Ral_TupleHeadingReference(a->heading = heading) ;

    return a ;
}

void
Ral_AttributeDelete(
    Ral_Attribute a)
{
    switch (a->attrType) {
    case Tcl_Type:
        assert(a->heading == NULL) ;
	break ;

    case Tuple_Type:
    case Relation_Type:
        assert(a->heading != NULL) ;
	Ral_TupleHeadingUnreference(a->heading) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeDelete: unknown attribute type: %d",
	    a->attrType) ;
    }
    ckfree((char *)a) ;
}

Ral_Attribute
Ral_AttributeDup(
    Ral_Attribute a)
{
    Ral_Attribute newAttr = NULL ; /* to silence the compiler over
				      the default case */

    switch (a->attrType) {
    case Tcl_Type:
	newAttr = Ral_AttributeNewTclType(a->name, a->typeName) ;
	break ;

    case Tuple_Type:
	newAttr = Ral_AttributeNewTupleType(a->name, a->heading) ;
	break ;

    case Relation_Type:
	newAttr = Ral_AttributeNewRelationType(a->name, a->heading) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeDup: unknown attribute type: %d",
	    a->attrType) ;
    }

    assert(newAttr != NULL) ;
    return newAttr ;
}

Ral_Attribute
Ral_AttributeRename(
    Ral_Attribute a,
    const char *newName)
{
    Ral_Attribute newAttr = NULL ; /* to silence the compiler over
				      the default case */

    switch (a->attrType) {
    case Tcl_Type:
	newAttr = Ral_AttributeNewTclType(newName, a->typeName) ;
	break ;

    case Tuple_Type:
	newAttr = Ral_AttributeNewTupleType(newName, a->heading) ;
	break ;

    case Relation_Type:
	newAttr = Ral_AttributeNewRelationType(newName, a->heading) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeRename: unknown attribute type: %d",
	    a->attrType) ;
    }

    assert(newAttr != NULL) ;
    return newAttr ;
}

int
Ral_AttributeEqual(
    Ral_Attribute a1,
    Ral_Attribute a2)
{
    if (a1 == a2) {
	return 1 ;
    }

    if (strcmp(a1->name, a2->name) != 0) {
	return 0 ;
    }

    if (a1->attrType != a2->attrType) {
	return 0 ;
    }

    return Ral_AttributeTypeEqual(a1, a2) ;
}

int
Ral_AttributeTypeEqual(
    Ral_Attribute a1,
    Ral_Attribute a2)
{
    int result = 0 ;

    switch (a1->attrType) {
    case Tcl_Type:
	result = strcmp(a1->typeName, a2->typeName) == 0 ;
	break ;

    case Tuple_Type:
    case Relation_Type:
	result = Ral_TupleHeadingEqual(a1->heading, a2->heading) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeTypeEqual: unknown attribute type: %d",
	    a1->attrType) ;
    }
    return result ;
}

int
Ral_AttributeValueEqual(
    Ral_Attribute a,
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    int isEqual = 0 ;
    int v1len ;
    int v2len ;

    switch (a->attrType) {
    case Tcl_Type:
        (void)Tcl_GetStringFromObj(v1, &v1len) ;
        (void)Tcl_GetStringFromObj(v2, &v2len) ;
        if (v1len == 0 && v2len == 0) {
	    isEqual = 1 ;
        } else if ((v1len != 0 && v2len == 0) || (v1len == 0 && v2len != 0)) {
            isEqual = 0 ;
	} else {
	    struct ral_type *type = findRalType(a->typeName) ;
	    isEqual = type ? type->isequal(v1, v2) : stringEqual(v1, v2) ;
	}
	break ;

    case Tuple_Type:
	if (Tcl_ConvertToType(NULL, v1, &Ral_TupleObjType) == TCL_OK &&
                Tcl_ConvertToType(NULL, v2, &Ral_TupleObjType) == TCL_OK) {
            isEqual = Ral_TupleEqualValues(v1->internalRep.otherValuePtr,
                v2->internalRep.otherValuePtr) ;
	}
	break ;

    case Relation_Type:
	if (Tcl_ConvertToType(NULL, v1, &Ral_RelationObjType) == TCL_OK &&
                Tcl_ConvertToType(NULL, v2, &Ral_RelationObjType) == TCL_OK) {
            isEqual = Ral_RelationEqual(v1->internalRep.otherValuePtr,
                v2->internalRep.otherValuePtr) ;
	}
	break ;

    default:
	Tcl_Panic("Ral_AttributeValueEqual: unknown attribute type: %d",
	    a->attrType) ;
    }

    return isEqual ;
}

int
Ral_AttributeValueCompare(
    Ral_Attribute a,
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    int result = 0 ;
    struct ral_type *type ;

    switch (a->attrType) {
    case Tcl_Type:
	type = findRalType(a->typeName) ;
	result = type ?  type->compare(v1, v2) : stringCompare(v1, v2) ;
	break ;

	/*
         * Relative comparisons of tuple and relations does not make a lot of
         * sense since you can't really place an ordering on relation values.
         * However, we will resort to string comparison for lack of a better
         * idea.  Higher order code should never make the request but just in
         * case.
	 */
    case Tuple_Type:
        /*
         * N.B. Fall through.
         */
    case Relation_Type:
	result = stringCompare(v1, v2) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeValueCompare: unknown attribute type: %d",
	    a->attrType) ;
    }

    return result ;
}

Tcl_Obj *
Ral_AttributeValueObj(
    Tcl_Interp *interp,
    Ral_Attribute a,
    Tcl_Obj *value)
{
    Tcl_Obj *result = NULL ;

    switch (a->attrType) {
    case Tcl_Type:
	result = value ;
	break ;

    case Tuple_Type:
	if (Tcl_ConvertToType(interp, value, &Ral_TupleObjType) == TCL_OK) {
	    Ral_Tuple tuple ;
	    char *valueStr ;

	    tuple = value->internalRep.otherValuePtr ;
	    valueStr = Ral_TupleValueStringOf(tuple) ;
	    result = Tcl_NewStringObj(valueStr, -1) ;
	    ckfree(valueStr) ;
	}
	break ;

    case Relation_Type:
	if (Tcl_ConvertToType(interp, value, &Ral_RelationObjType) == TCL_OK) {
	    Ral_Relation relation ;
	    char *valueStr ;

	    relation = value->internalRep.otherValuePtr ;
	    valueStr = Ral_RelationValueStringOf(relation) ;
	    result = Tcl_NewStringObj(valueStr, -1) ;
	    ckfree(valueStr) ;
	}
	break ;

    default:
	Tcl_Panic("Ral_AttributeValueObj: unknown attribute type: %d",
	    a->attrType) ;
	break ;
    }

    return result ;
}

/*
 * Construct an attribute from Tcl objects.
 */
Ral_Attribute
Ral_AttributeNewFromObjs(
    Tcl_Interp *interp,
    Tcl_Obj *nameObj,
    Tcl_Obj *typeObj,
    Ral_ErrorInfo *errInfo)
{
    Ral_Attribute attribute = NULL ;
    int typec ;
    Tcl_Obj **typev ;
    const char *attrName ;
    const char *typeName ;

    /*
     * The complication arises when the type is either Tuple or Relation.
     * So first see if the "type" object is a list that might contain
     * a Tuple or Relation type specification.
     */
    if (Tcl_ListObjGetElements(interp, typeObj, &typec, &typev) != TCL_OK) {
	return NULL ;
    }
    attrName = Tcl_GetString(nameObj) ;
    typeName = Tcl_GetString(*typev) ;
    if (strcmp(ral_tupleTypeName, typeName) == 0) {
	if (typec == 2) {
	    Ral_TupleHeading heading =
		Ral_TupleHeadingNewFromObj(interp, *(typev + 1), errInfo) ;
	    if (heading) {
		attribute = Ral_AttributeNewTupleType(attrName, heading) ;
	    }
	} else {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_HEADING_ERR, typeName) ;
	    Ral_InterpSetError(interp, errInfo) ;
	}
    } else if (strcmp(ral_relationTypeName, typeName) == 0) {
	if (typec == 2) {
	    Ral_TupleHeading heading = Ral_TupleHeadingNewFromObj(interp,
		*(typev + 1), errInfo) ;
	    if (heading) {
		attribute = Ral_AttributeNewRelationType(attrName, heading) ;
	    }
	} else {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_HEADING_ERR, typeName) ;
	    Ral_InterpSetError(interp, errInfo) ;
	}
    } else if (typec == 1) {
	attribute = Ral_AttributeNewTclType(attrName, typeName) ;

	if (attribute == NULL) {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_BAD_TYPE, typeName) ;
	    Ral_InterpSetError(interp, errInfo) ;
	}
    } else {
        Ral_ErrorInfoSetError(errInfo, RAL_ERR_BAD_TYPE, typeName) ;
	Ral_InterpSetError(interp, errInfo) ;
    }

    return attribute ;
}

/*
 * When an attribute is given a value in a Tuple or Relation, it must be able
 * to be coerced into the appropriate type.
 * N.B. that we always allow the empty string as a "control" value for all
 * types. This is a special dispensation that allows the cheap hack of using
 * the empty string to stand for an attribute value when used as a conditional
 * referential attribute. This ugliness has many ramifications.
 */
Tcl_Obj *
Ral_AttributeConvertValueToType(
    Tcl_Interp *interp,
    Ral_Attribute attr,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    Tcl_Obj *result = objPtr ;
    int status ;
    int len ;

    switch (attr->attrType) {
    case Tcl_Type:
	/*
	 * For simple Tcl types, attempt to convert the Tcl object to the type
	 * for the attribute. It is possible for the type convertion to
	 * succeed, but that the type be assigned something other than the one
	 * associated with the attribute.  Tcl type conversions, especially for
	 * numeric types, stop at the first type that can represent the number.
	 * For example, converting "30" to a double will end up as an "int"
	 * internally. This prevents excessive round off (I guess) and other
	 * issues. It is okay that this happens, as the "shadow" typing system
	 * handles the conversion to an actual double value when necessary.
	 *
	 * Also, we deem the empty string to be a valid value for any
	 * simple Tcl type.
	 */
        (void)Tcl_GetStringFromObj(objPtr, &len) ;
	if (len != 0) {
	    struct ral_type *type = findRalType(attr->typeName) ;
	    status = type ? type->isa(interp, objPtr) :
		    isAString(interp, objPtr) ;
	    if (status != TCL_OK) {
		Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_VALUE, objPtr) ;
		Ral_InterpSetError(interp, errInfo) ;
                result = NULL ;
	    }
	}
	break ;

    case Tuple_Type:
	if (objPtr->typePtr != &Ral_TupleObjType) {
            if (Tcl_IsShared(objPtr)) {
                result = Tcl_DuplicateObj(objPtr) ;
                if (Ral_TupleObjConvert(attr->heading, interp, objPtr, result,
                        errInfo) == TCL_OK) {
                    Tcl_InvalidateStringRep(result) ;
                } else {
                    Tcl_DecrRefCount(result) ;
                    result = NULL ;
                }
            } else {
                if (Ral_TupleObjConvert(attr->heading, interp, objPtr,
                        objPtr, errInfo) == TCL_OK) {
                    Tcl_InvalidateStringRep(result) ;
                } else {
                    result = NULL ;
                }
            }
	}
        /*
         * Check that the heading of the value matches that of the
         * attribute, i.e. make sure we are not trying to assign an
         * attribute value that is of the wrong heading type.
         */
        if (result) {
            Ral_Tuple tuple ;

            assert(result->typePtr == &Ral_TupleObjType) ;
            tuple = result->internalRep.otherValuePtr ;
            if (!Ral_TupleHeadingEqual(attr->heading, tuple->heading)) {
                Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_HEADING_NOT_EQUAL,
                        objPtr) ;
                Ral_InterpSetError(interp, errInfo) ;
                result = NULL ;
            }
        }
	break ;

    case Relation_Type:
        /*
         * It may be necessary to convert the object to relation type.
         */
	if (objPtr->typePtr != &Ral_RelationObjType) {
            if (Tcl_IsShared(objPtr)) {
                result = Tcl_DuplicateObj(objPtr) ;
                if (Ral_RelationObjConvert(attr->heading, interp, objPtr,
                        result, errInfo) == TCL_OK) {
                    Tcl_InvalidateStringRep(result) ;
                } else {
                    Tcl_DecrRefCount(result) ;
                    result = NULL ;
                }
            } else {
                if (Ral_RelationObjConvert(attr->heading, interp, objPtr,
                        objPtr, errInfo) == TCL_OK) {
                    Tcl_InvalidateStringRep(result) ;
                } else {
                    result = NULL ;
                }
            }
	}
        /*
         * Check that the heading of the value matches that of the
         * attribute, i.e. make sure we are not trying to assign an
         * attribute value that is of the wrong heading type.
         */
        if (result) {
            Ral_Relation relation ;

            assert(result->typePtr == &Ral_RelationObjType) ;
            relation = result->internalRep.otherValuePtr ;
            if (!Ral_TupleHeadingEqual(attr->heading, relation->heading)) {
                Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_HEADING_NOT_EQUAL,
                        objPtr) ;
                Ral_InterpSetError(interp, errInfo) ;
                result = NULL ;
            }
        }
	break ;

    default:
	Tcl_Panic("Ral_AttributeConvertValueToType: unknown attribute type: %d",
	    attr->attrType) ;
	break ;
    }

    return result ;
}

unsigned
Ral_AttributeHashValue(
    Ral_Attribute attr,
    Tcl_Obj *objPtr)
{
    struct ral_type *type ;
    unsigned result = 0 ;
    int olen ;

    switch (attr->attrType) {
    case Tcl_Type:
        (void)Tcl_GetStringFromObj(objPtr, &olen) ;
        if (olen == 0) {
	    result = 0 ;
	} else {
            /*
             * For simple Tcl types, use the shadow type system to hash the
             * value.
             */
            type = findRalType(attr->typeName) ;
            result = type ? type->hash(objPtr) : stringHash(objPtr) ;
        }
	break ;
        /*
         * N.B. That we don't allow tuple or relation valued attributes to
         * contribute to the hash value. To do so would imply that we must put
         * the attribute value into some canonical order because hashing
         * insists that value that are equal must produce the same hash value.
         * This approach probably makes the hashing function less effective but
         * avoids problems with defining a canonical order. In the end all
         * hashing must compare the key to the value found in a hash bucket and
         * the comparison is well defined even if computing a hash value is
         * not.
         */
    case Tuple_Type:
    case Relation_Type:
        result = 0 ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeHashValue: unknown attribute type: %d",
	    attr->attrType) ;
	break ;
    }

    return result ;
}

static int cmpTypeNames(
    void const *n1,
    void const *n2)
{
    struct ral_type const *t1 = n1 ;
    struct ral_type const *t2 = n2 ;
    return strcmp(t1->typeName, t2->typeName) ;
}

static struct ral_type *
findRalType(char const *typeName)
{
    struct ral_type key ;

    key.typeName = typeName ;
    return bsearch(&key, Ral_Types, COUNTOF(Ral_Types), sizeof(struct ral_type),
	    cmpTypeNames) ;
}

/*
 * This is just a simple sum. The hash table code randomized the bits.
 */
static unsigned
hashBytes(
    void const *bPtr,
    unsigned len)
{
    unsigned char const *bytes = (unsigned char const *)bPtr ;
    unsigned hash = 0 ;
    while (len-- != 0) {
        hash += *bytes++ ;
    }

    return hash ;
}

static int
isABoolean(
    Tcl_Interp *interp,
    Tcl_Obj *boolObj)
{
    int b ;
    return Tcl_GetBooleanFromObj(interp, boolObj, &b) ;
}

static int
booleanEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    int b1 ;
    int b2 ;

    return (Tcl_GetBooleanFromObj(NULL, v1, &b1) == TCL_OK &&
		Tcl_GetBooleanFromObj(NULL, v2, &b2) == TCL_OK) ?
	    b1 == b2 : 0 ;
}

static int
booleanCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    int b1 ;
    int b2 ;

    return (Tcl_GetBooleanFromObj(NULL, v1, &b1) == TCL_OK &&
	    Tcl_GetBooleanFromObj(NULL, v2, &b2) == TCL_OK) ?
            b1 - b2 : -1 ;
}

static unsigned
booleanHash(
    Tcl_Obj *boolObj)
{
    int b = 0 ;
    if (Tcl_GetBooleanFromObj(NULL, boolObj, &b) != TCL_OK) {
        Tcl_Panic("booleanHash: cannot convert, \"%s\"",
                Tcl_GetString(boolObj)) ;
    }
    return hashBytes(&b, sizeof(b)) ;
}

static int
isAByteArray(
    Tcl_Interp *interp,
    Tcl_Obj *byteArrayObj)
{
    /*
     * We call Tcl_GetByteArrayFromObj() because if we've been handed
     * as string, then there is conversion to bytes that happens here.
     * That conversion, however, never seems to fail.
     */
    (void)Tcl_GetByteArrayFromObj(byteArrayObj, NULL) ;
    return TCL_OK ;
}

static int
byteArrayEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    unsigned char const *ba1 ;
    int balen1 ;
    unsigned char const *ba2 ;
    int balen2 ;

    ba1 = Tcl_GetByteArrayFromObj(v1, &balen1) ;
    ba2 = Tcl_GetByteArrayFromObj(v2, &balen2) ;
    return balen1 == balen2 && memcmp(ba1, ba2, balen1) == 0 ;
}

static int
byteArrayCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    unsigned char const *ba1 ;
    int balen1 ;
    unsigned char const *ba2 ;
    int balen2 ;
    int minlen ;

    ba1 = Tcl_GetByteArrayFromObj(v1, &balen1) ;
    ba2 = Tcl_GetByteArrayFromObj(v2, &balen2) ;
    minlen = balen1 < balen2 ? balen1 : balen2 ;
    return memcmp(ba1, ba2, minlen) ;
}

static unsigned
byteArrayHash(
    Tcl_Obj *byteArrayObj)
{
    unsigned char const *ba ;
    int balen ;

    ba = Tcl_GetByteArrayFromObj(byteArrayObj, &balen) ;
    return hashBytes(ba, balen) ;
}

static int
isAnInt(
    Tcl_Interp *interp,
    Tcl_Obj *intObj)
{
    int i ;
    return Tcl_GetIntFromObj(interp, intObj, &i) ;
}

static int
intEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    int int1 ;
    int int2 ;

    return (Tcl_GetIntFromObj(NULL, v1, &int1) == TCL_OK &&
		Tcl_GetIntFromObj(NULL, v2, &int2) == TCL_OK) ?
	    int1 == int2 : 0 ;
}

static int
intCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    int int1 ;
    int int2 ;

    return (Tcl_GetIntFromObj(NULL, v1, &int1) == TCL_OK &&
	    Tcl_GetIntFromObj(NULL, v2, &int2) == TCL_OK) ?
            int1 - int2 : -1 ;
}

static unsigned
intHash(
    Tcl_Obj *intObj)
{
    int i = 0 ;
    if (Tcl_GetIntFromObj(NULL, intObj, &i) != TCL_OK) {
        Tcl_Panic("intHash: cannot convert, \"%s\"", Tcl_GetString(intObj)) ;
    }
    return hashBytes(&i, sizeof(i)) ;
}

static int
isALong(
    Tcl_Interp *interp,
    Tcl_Obj *longObj)
{
    long l ;
    return Tcl_GetLongFromObj(interp, longObj, &l) ;
}

static int
longEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    long l1 ;
    long l2 ;

    return (Tcl_GetLongFromObj(NULL, v1, &l1) == TCL_OK &&
		Tcl_GetLongFromObj(NULL, v2, &l2) == TCL_OK) ?
	    l1 == l2 : 0 ;
}

static int
longCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    long l1 ;
    long l2 ;

    return (Tcl_GetLongFromObj(NULL, v1, &l1) == TCL_OK &&
	    Tcl_GetLongFromObj(NULL, v2, &l2) == TCL_OK) ?
            l1 - l2 : -1 ;
}

static unsigned
longHash(
    Tcl_Obj *longObj)
{
    long l = 0 ;
    if (Tcl_GetLongFromObj(NULL, longObj, &l) != TCL_OK) {
        Tcl_Panic("longHash: cannot convert, \"%s\"", Tcl_GetString(longObj)) ;
    }
    return hashBytes(&l, sizeof(l)) ;
}

static int
isADouble(
    Tcl_Interp *interp,
    Tcl_Obj *dblObj)
{
    double d ;
    return Tcl_GetDoubleFromObj(interp, dblObj, &d) ;
}

static int
doubleEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    double d1 ;
    double d2 ;

    return (Tcl_GetDoubleFromObj(NULL, v1, &d1) == TCL_OK &&
		Tcl_GetDoubleFromObj(NULL, v2, &d2) == TCL_OK) ?
	    d1 == d2 : 0 ;
}

static int
doubleCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    double double1 ;
    double double2 ;

    if (Tcl_GetDoubleFromObj(NULL, v1, &double1) == TCL_OK &&
	    Tcl_GetDoubleFromObj(NULL, v2, &double2) == TCL_OK) {
	if (double1 > double2) {
	    return 1 ;
	} else if (double1 < double2) {
	    return -1 ;
	} else {
	    return 0 ;
	}
    }
    return -1 ;
}

static unsigned
doubleHash(
    Tcl_Obj *doubleObj)
{
    double d = 0 ;
    if (Tcl_GetDoubleFromObj(NULL, doubleObj, &d) != TCL_OK) {
        Tcl_Panic("doubleHash: cannot convert, \"%s\"",
                Tcl_GetString(doubleObj)) ;
    }
    return hashBytes(&d, sizeof(d)) ;
}

#   ifndef NO_WIDE_TYPE
static int
isAWideInt(
    Tcl_Interp *interp,
    Tcl_Obj *wintObj)
{
    Tcl_WideInt wint ;
    return Tcl_GetWideIntFromObj(interp, wintObj, &wint) ;
}

static int
wideIntEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    Tcl_WideInt wint1 ;
    Tcl_WideInt wint2 ;

    return (Tcl_GetWideIntFromObj(NULL, v1, &wint1) == TCL_OK &&
		Tcl_GetWideIntFromObj(NULL, v2, &wint2) == TCL_OK) ?
	    wint1 == wint2 : 0 ;
}

static int
wideIntCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    Tcl_WideInt wint1 ;
    Tcl_WideInt wint2 ;

    if (Tcl_GetWideIntFromObj(NULL, v1, &wint1) == TCL_OK &&
		Tcl_GetWideIntFromObj(NULL, v2, &wint2) == TCL_OK) {
	if (wint1 > wint2) {
	    return 1 ;
	} else if (wint1 < wint2) {
	    return -1 ;
	} else {
	    return 0 ;
	}
    }
    return -1 ;
}

static unsigned
wideIntHash(
    Tcl_Obj *wideIntObj)
{
    Tcl_WideInt wi = 0 ;
    if (Tcl_GetWideIntFromObj(NULL, wideIntObj, &wi) != TCL_OK) {
        Tcl_Panic("wideIntHash: cannot convert, \"%s\"",
                Tcl_GetString(wideIntObj)) ;
    }
    return hashBytes(&wi, sizeof(wi)) ;
}
#   endif

static int
isABignum(
    Tcl_Interp *interp,
    Tcl_Obj *bnObj)
{
    int result ;
    mp_int bn ;
    result = Tcl_GetBignumFromObj(interp, bnObj, &bn) ;
    if (result == TCL_OK) {
        mp_clear(&bn) ;
    }

    return result ;
}

static int
bignumEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    mp_int bn1 ;
    mp_int bn2 ;
    int result = 0 ;

    if (Tcl_GetBignumFromObj(NULL, v1, &bn1) == TCL_OK) {
	if (Tcl_GetBignumFromObj(NULL, v2, &bn2) == TCL_OK) {
	    result = mp_cmp(&bn1, &bn2) == 0 ;
	    mp_clear(&bn2) ;
	}
	mp_clear(&bn1) ;
    }

    return result ;
}

static int
bignumCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    mp_int bn1 ;
    mp_int bn2 ;
    int result = -1 ;

    if (Tcl_GetBignumFromObj(NULL, v1, &bn1) == TCL_OK) {
	if (Tcl_GetBignumFromObj(NULL, v2, &bn2) == TCL_OK) {
	    result = mp_cmp(&bn1, &bn2) ;
	    mp_clear(&bn2) ;
	}
	mp_clear(&bn1) ;
    }

    return result ;
}

static unsigned
bignumHash(
    Tcl_Obj *bignumObj)
{
    mp_int bn ;
    int result ;
    unsigned hash = 0 ;

    result = Tcl_GetBignumFromObj(NULL, bignumObj, &bn) ;
    if (result == TCL_OK) {
        hash = hashBytes(bn.dp, bn.used) ;
        mp_clear(&bn) ;
    }

    return hash ;
}

static int
isAList(
    Tcl_Interp *interp,
    Tcl_Obj *listObj)
{
    int elemc ;
    Tcl_Obj **elemv ;
    return Tcl_ListObjGetElements(interp, listObj, &elemc, &elemv) ;
}

static int
listEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    return stringEqual(v1, v2) ;
}

static int
listCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    return stringCompare(v1, v2) ;
}

static unsigned
listHash(
    Tcl_Obj *listObj)
{
    return stringHash(listObj) ;
}

static int
isADict(
    Tcl_Interp *interp,
    Tcl_Obj *dictObj)
{
    int size ;
    return Tcl_DictObjSize(interp, dictObj, &size) ;
}

static int
dictEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    return stringEqual(v1, v2) ;
}

static int
dictCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    return stringCompare(v1, v2) ;
}

static unsigned
dictHash(
    Tcl_Obj *dictObj)
{
    return stringHash(dictObj) ;
}

static int
isAString(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr)
{
    return TCL_OK ;
}

static int
stringEqual(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    return strcmp(Tcl_GetString(v1), Tcl_GetString(v2)) == 0 ;
}

static int
stringCompare(
    Tcl_Obj *v1,
    Tcl_Obj *v2)
{
    return strcmp(Tcl_GetString(v1), Tcl_GetString(v2)) ;
}

static unsigned
stringHash(
    Tcl_Obj *stringObj)
{
    int slen ;
    char const *s ;
    
    s = Tcl_GetStringFromObj(stringObj, &slen) ;
    return hashBytes(s, slen) ;
}

/*
 * The functions below deal with converting attributes from internal to
 * external form. They follow the traditional Tcl "scan / convert" approach
 * where the attribute is "scanned" to determine how much space it will take
 * and then "converted" to transform it into a string. The complicated part is
 * dealing with possible recursive aspects of Tuples and Relations, i.e. Tuple
 * and Relations can have Tuple valued or Relation valued attributes.
 */

int
Ral_AttributeScanName(
    Ral_Attribute a,
    Ral_AttributeTypeScanFlags *flags)
{
    flags->attrType = a->attrType ;
    return flags->nameLength = Tcl_ScanElement(a->name, &flags->nameFlags) ;
}

int
Ral_AttributeConvertName(
    Ral_Attribute a,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    return Tcl_ConvertElement(a->name, dst, flags->nameFlags) ;
}

int
Ral_AttributeScanType(
    Ral_Attribute a,
    Ral_AttributeTypeScanFlags *flags)
{
    int length = 0 ; /* to silence the compiler over the default case */

    flags->attrType = a->attrType ;
    switch (a->attrType) {
    case Tcl_Type:
	length = Tcl_ScanElement(a->typeName, &flags->flags.simpleFlags) ;
	break ;

    case Tuple_Type:
	length = sizeof(openList) +
                /*
                 * -1 to account for NUL char in "sizeof(ral_tupleTypename)"
                 *  and +1 to account for the space separator ==> net 0.
                 */
                sizeof(ral_tupleTypeName) +
                sizeof(openList) +
                Ral_TupleHeadingScan(a->heading, flags) +
                sizeof(closeList) +
                sizeof(closeList) ;
	break ;

    case Relation_Type:
	length = sizeof(openList) +
                sizeof(ral_relationTypeName) +
                sizeof(openList) +
                Ral_TupleHeadingScan(a->heading, flags) +
                sizeof(closeList) +
                sizeof(closeList) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeScanType: unknown attribute type: %d",
	    a->attrType) ;
    }

    return length ;
}

int
Ral_AttributeConvertType(
    Ral_Attribute a,
    char *dst,
    Ral_AttributeTypeScanFlags *flags)
{
    int length = 0 ; /* to silence the compiler over the default case */

    switch (a->attrType) {
    case Tcl_Type:
	length = Tcl_ConvertElement(a->typeName, dst,
		flags->flags.simpleFlags) ;
	break ;

    case Tuple_Type: {
	char *p = dst ;

	*p++ = openList ;
        strcpy(p, ral_tupleTypeName) ;
        p += sizeof(ral_tupleTypeName) - 1 ;
        *p++ = ' ' ;
	*p++ = openList ;
	p += Ral_TupleHeadingConvert(a->heading, p, flags) ;
	*p++ = closeList ;
	*p++ = closeList ;
	length = p - dst ;
    }
	break ;

    case Relation_Type: {
	char *p = dst ;

	*p++ = openList ;
        strcpy(p, ral_relationTypeName) ;
        p += sizeof(ral_relationTypeName) - 1 ;
        *p++ = ' ' ;
	*p++ = openList ;
	p += Ral_TupleHeadingConvert(a->heading, p, flags) ;
	*p++ = closeList ;
	*p++ = closeList ;
	length = p - dst ;
    }
	break ;

    default:
	Tcl_Panic("Ral_AttributeScanType: unknown attribute type: %d",
	    a->attrType) ;
    }

    return length ;
}

int
Ral_AttributeScanValue(
    Ral_Attribute a,
    Tcl_Obj *value,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    int length = 0 ; /* to silence the compiler over the default case */

    valueFlags->attrType = a->attrType ;
    switch (a->attrType) {
    case Tcl_Type:
	length = Tcl_ScanElement(Tcl_GetString(value),
	    &valueFlags->flags.simpleFlags) ;
	break ;

    case Tuple_Type:
	length = Ral_TupleScanValue(value->internalRep.otherValuePtr,
	    typeFlags, valueFlags) ;
	break ;

    case Relation_Type:
	length = Ral_RelationScanValue(value->internalRep.otherValuePtr,
	    typeFlags, valueFlags) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeScanValue: unknown attribute type: %d",
	    a->attrType) ;
    }

    return length ;
}

int
Ral_AttributeConvertValue(
    Ral_Attribute a,
    Tcl_Obj *value,
    char *dst,
    Ral_AttributeTypeScanFlags *typeFlags,
    Ral_AttributeValueScanFlags *valueFlags)
{
    int length = 0 ; /* to silence the compiler over the default case */

    switch (a->attrType) {
    case Tcl_Type:
	length = Tcl_ConvertElement(Tcl_GetString(value), dst,
	    valueFlags->flags.simpleFlags) ;
	break ;

    case Tuple_Type:
	length = Ral_TupleConvertValue(value->internalRep.otherValuePtr,
	    dst, typeFlags, valueFlags) ;
	break ;

    case Relation_Type:
	length = Ral_RelationConvertValue(value->internalRep.otherValuePtr,
	    dst, typeFlags, valueFlags) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeConvertValue: unknown attribute type: %d",
	    a->attrType) ;
    }

    return length ;
}

void
Ral_AttributeTypeScanFlagsFree(
    Ral_AttributeTypeScanFlags *flags)
{
    switch (flags->attrType) {
    case Tcl_Type:
	break ;

    case Tuple_Type:
    case Relation_Type: {
	int count = flags->flags.compoundFlags.count ;
	Ral_AttributeTypeScanFlags *typeFlags =
		flags->flags.compoundFlags.flags ;

	assert(typeFlags != NULL) ;

	while (count-- > 0) {
	    Ral_AttributeTypeScanFlagsFree(typeFlags++) ;
	}
	ckfree((char *)flags->flags.compoundFlags.flags) ;
	flags->flags.compoundFlags.flags = NULL ;
    }
	break ;

    default:
	Tcl_Panic("Ral_AttributeTypeScanFlagsFree: unknown flags type: %d",
	    flags->attrType) ;
    }
}

void
Ral_AttributeValueScanFlagsFree(
    Ral_AttributeValueScanFlags *flags)
{
    switch (flags->attrType) {
    case Tcl_Type:
	break ;

    case Tuple_Type:
    case Relation_Type: {
	int count = flags->flags.compoundFlags.count ;
	Ral_AttributeValueScanFlags *valueFlags =
		flags->flags.compoundFlags.flags ;

	assert(valueFlags != NULL) ;

	while (count-- > 0) {
	    Ral_AttributeValueScanFlagsFree(valueFlags++) ;
	}
	ckfree((char *)flags->flags.compoundFlags.flags) ;
	flags->flags.compoundFlags.flags = NULL ;
    }
	break ;

    default:
	Tcl_Panic("Ral_AttributeValueScanFlagsFree: unknown flags type: %d",
	    flags->attrType) ;
    }
}

/*
 * Returned string must be freed by the caller.
 */
char *
Ral_AttributeToString(
    Ral_Attribute a)
{
    Ral_AttributeTypeScanFlags flags ;
    int size ;
    char *str ;
    char *s ;

    size = Ral_AttributeScanName(a, &flags) + Ral_AttributeScanType(a, &flags)
	+ 1; /* +1 for separating space */
    s = str = ckalloc(size) ;
    s += Ral_AttributeConvertName(a, s, &flags) ;
    *s++ = ' ' ;
    Ral_AttributeConvertType(a, s, &flags) ;

    Ral_AttributeTypeScanFlagsFree(&flags) ;

    return str ;
}
