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

$RCSfile: ral_attribute.c,v $
$Revision: 1.13 $
$Date: 2006/07/01 23:56:31 $
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
#include "ral_relationheading.h"
#include "ral_tupleobj.h"
#include "ral_relationobj.h"

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
static const char openList = '{' ;
static const char closeList = '}' ;
static const char rcsid[] = "@(#) $RCSfile: ral_attribute.c,v $ $Revision: 1.13 $" ;

/*
FUNCTION DEFINITIONS
*/

Ral_Attribute
Ral_AttributeNewTclType(
    const char *name,
    Tcl_ObjType *type)
{
    Ral_Attribute a ;

    /* +1 for the NUL terminator */
    a = (Ral_Attribute)ckalloc(sizeof(*a) + strlen(name) + 1) ;
    a->name = strcpy((char *)(a + 1), name) ;
    a->attrType = Tcl_Type ;
    a->tclType = type ;

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
    a->attrType = Tuple_Type ;
    Ral_TupleHeadingReference(a->tupleHeading = heading) ;

    return a ;
}

Ral_Attribute
Ral_AttributeNewRelationType(
    const char *name,
    Ral_RelationHeading heading)
{
    Ral_Attribute a ;

    a = (Ral_Attribute)ckalloc(sizeof(*a) + strlen(name) + 1) ;
    a->name = strcpy((char *)(a + 1), name) ;
    a->attrType = Relation_Type ;
    Ral_RelationHeadingReference(a->relationHeading = heading) ;

    return a ;
}

void
Ral_AttributeDelete(
    Ral_Attribute a)
{
    switch (a->attrType) {
    case Tcl_Type:
	break ;

    case Tuple_Type:
	Ral_TupleHeadingUnreference(a->tupleHeading) ;
	break ;

    case Relation_Type:
	Ral_RelationHeadingUnreference(a->relationHeading) ;
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
	newAttr = Ral_AttributeNewTclType(a->name, a->tclType) ;
	break ;

    case Tuple_Type:
	newAttr = Ral_AttributeNewTupleType(a->name, a->tupleHeading) ;
	break ;

    case Relation_Type:
	newAttr = Ral_AttributeNewRelationType(a->name, a->relationHeading) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeDup: unknown attribute type: %d",
	    a->attrType) ;
    }
    return newAttr ;
}

Ral_Attribute
Ral_AttributeRename(
    Ral_Attribute a,
    const char *newName)
{
    switch (a->attrType) {
    case Tcl_Type:
	return Ral_AttributeNewTclType(newName, a->tclType) ;

    case Tuple_Type:
	return Ral_AttributeNewTupleType(newName, a->tupleHeading) ;

    case Relation_Type:
	return Ral_AttributeNewRelationType(newName, a->relationHeading) ;

    default:
	Tcl_Panic("Ral_AttributeRename: unknown attribute type: %d",
	    a->attrType) ;
    }
    /* Not reached */
    return NULL ;
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
	result = a1->tclType == a2->tclType ;
	break ;

    case Tuple_Type:
	result = Ral_TupleHeadingEqual(a1->tupleHeading, a2->tupleHeading) ;
	break ;

    case Relation_Type:
	result = Ral_RelationHeadingEqual(a1->relationHeading,
	    a2->relationHeading) ;
	break ;

    default:
	Tcl_Panic("Ral_AttributeEqual: unknown attribute type: %d",
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
    switch (a->attrType) {
    case Tcl_Type:
	return Ral_ObjEqual(v1, v2) ;

    case Tuple_Type:
	if (Tcl_ConvertToType(NULL, v1, &Ral_TupleObjType) != TCL_OK ||
	    Tcl_ConvertToType(NULL, v2, &Ral_TupleObjType) != TCL_OK) {
	    Tcl_Panic("Ral_AttributeValueEqual: cannot convert to tuple") ;
	}
	return Ral_TupleEqual(v1->internalRep.otherValuePtr,
	    v2->internalRep.otherValuePtr) ;

    case Relation_Type:
	if (Tcl_ConvertToType(NULL, v1, &Ral_RelationObjType) != TCL_OK ||
	    Tcl_ConvertToType(NULL, v2, &Ral_RelationObjType) != TCL_OK) {
	    Tcl_Panic("Ral_AttributeValueEqual: cannot convert to relation") ;
	}
	return Ral_RelationEqual(v1->internalRep.otherValuePtr,
	    v2->internalRep.otherValuePtr) ;

    default:
	Tcl_Panic("Ral_AttributeValueEqual: unknown attribute type: %d",
	    a->attrType) ;
    }
    /* Not reached */
    return 1 ;
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
    if (strcmp("Tuple", typeName) == 0 && typec == 2) {
	Ral_TupleHeading heading =
	    Ral_TupleHeadingNewFromObj(interp, *(typev + 1), errInfo) ;

	if (heading) {
	    attribute = Ral_AttributeNewTupleType(attrName, heading) ;
	}
    } else if (strcmp("Relation", typeName) == 0 && typec == 3) {
	Ral_RelationHeading heading = Ral_RelationHeadingNewFromObjs(interp,
	    typev[1], typev[2], errInfo) ;

	if (heading) {
	    attribute = Ral_AttributeNewRelationType(attrName, heading) ;
	}
    } else if (typec == 1) {
	Tcl_ObjType *tclType = Tcl_GetObjType(typeName) ;

	if (tclType != NULL) {
	    attribute = Ral_AttributeNewTclType(attrName, tclType) ;
	} else {
	    Ral_ErrorInfoSetError(errInfo, RAL_ERR_BAD_TYPE, typeName) ;
	    Ral_InterpSetError(interp, errInfo) ;
	}
    } else {
	Ral_ErrorInfoSetError(errInfo, RAL_ERR_BAD_KEYWORD, typeName) ;
	Ral_InterpSetError(interp, errInfo) ;
    }

    return attribute ;
}

/*
 * When an attribute is given a value in a Tuple or Relation, it must
 * be able to be coerced into the appropriate type.
 * N.B. that we always allow the empty string as a "control" value
 * for all types. This is necessary for conditional referential attributes
 * that must never match a value.
 */
int
Ral_AttributeConvertValueToType(
    Tcl_Interp *interp,
    Ral_Attribute attr,
    Tcl_Obj *objPtr,
    Ral_ErrorInfo *errInfo)
{
    int result = TCL_OK ;

    if (strlen(Tcl_GetString(objPtr)) == 0) {
	return TCL_OK ;
    }

    /*
     */
    switch (attr->attrType) {
    case Tcl_Type:
	result = Tcl_ConvertToType(interp, objPtr, attr->tclType) ;
	if (result != TCL_OK) {
	    Ral_ErrorInfoSetErrorObj(errInfo, RAL_ERR_BAD_VALUE, objPtr) ;
	    Ral_InterpSetError(interp, errInfo) ;
	} else if (strcmp(attr->tclType->name, "string") != 0) {
	    Tcl_InvalidateStringRep(objPtr) ;
	    objPtr->length = 0 ;
	}
	break ;

    case Tuple_Type:
	if (objPtr->typePtr != &Ral_TupleObjType) {
	    result = Ral_TupleObjConvert(attr->tupleHeading, interp, objPtr,
		objPtr, errInfo) ;
	}
	break ;

    case Relation_Type:
	if (objPtr->typePtr != &Ral_RelationObjType) {
	    result = Ral_RelationObjConvert(attr->relationHeading, interp,
		objPtr, objPtr, errInfo) ;
	}
	break ;

    default:
	Tcl_Panic("Ral_AttributeConvertValueToType: unknown attribute type: %d",
	    attr->attrType) ;
	break ;
    }

    return result ;
}

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
	length = Tcl_ScanElement(a->tclType->name, &flags->simpleFlags) ;
	break ;

    case Tuple_Type:
	length = Ral_TupleHeadingScan(a->tupleHeading, flags) ;
	length += sizeof(openList) + sizeof(closeList) ;
	break ;

    case Relation_Type:
	length = Ral_RelationHeadingScan(a->relationHeading, flags) ;
	length += sizeof(openList) + sizeof(closeList) ;
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
	length = Tcl_ConvertElement(a->tclType->name, dst, flags->simpleFlags) ;
	break ;

    case Tuple_Type: {
	char *p = dst ;

	*p++ = openList ;
	p += Ral_TupleHeadingConvert(a->tupleHeading, p, flags) ;
	*p++ = closeList ;
	length = p - dst ;
    }
	break ;

    case Relation_Type: {
	char *p = dst ;

	*p++ = openList ;
	p += Ral_RelationHeadingConvert(a->relationHeading, p, flags) ;
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
	    &valueFlags->simpleFlags) ;
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
	    valueFlags->simpleFlags) ;
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
	int count = flags->compoundFlags.count ;
	Ral_AttributeTypeScanFlags *typeFlags = flags->compoundFlags.flags ;

	assert(typeFlags != NULL) ;

	while (count-- > 0) {
	    Ral_AttributeTypeScanFlagsFree(typeFlags++) ;
	}
	ckfree((char *)flags->compoundFlags.flags) ;
	flags->compoundFlags.flags = NULL ;
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
	int count = flags->compoundFlags.count ;
	Ral_AttributeValueScanFlags *valueFlags = flags->compoundFlags.flags ;

	assert(valueFlags != NULL) ;

	while (count-- > 0) {
	    Ral_AttributeValueScanFlagsFree(valueFlags++) ;
	}
	ckfree((char *)flags->compoundFlags.flags) ;
	flags->compoundFlags.flags = NULL ;
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


const char *
Ral_AttributeVersion(void)
{
    return rcsid ;
}
