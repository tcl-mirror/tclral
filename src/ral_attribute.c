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

$RCSfile: ral_attribute.c,v $
$Revision: 1.5 $
$Date: 2006/02/26 04:57:53 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_attribute.c,v $
Revision 1.5  2006/02/26 04:57:53  mangoa01
Reworked the conversion from internal form to a string yet again.
This design is better and more recursive in nature.
Added additional code to the "relation" commands.
Now in a position to finish off the remaining relation commands.

Revision 1.4  2006/02/20 20:15:07  mangoa01
Now able to convert strings to relations and vice versa including
tuple and relation valued attributes.

Revision 1.3  2006/02/06 05:02:45  mangoa01
Started on relation heading and other code refactoring.
This is a checkpoint after a number of added files and changes
to tuple heading code.

Revision 1.2  2006/01/02 01:39:29  mangoa01
Tuple commands now operate properly. Fixed problems of constructing the string representation when there were tuple valued attributes.

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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ral_attribute.h"
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
static const char openList[] = "{" ;
static const char closeList[] = "}" ;
static const char rcsid[] = "@(#) $RCSfile: ral_attribute.c,v $ $Revision: 1.5 $" ;

/*
FUNCTION DEFINITIONS
*/

Ral_Attribute
Ral_AttributeNewTclType(
    const char *name,
    Tcl_ObjType *type)
{
    Ral_Attribute a ;

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

Ral_Attribute
Ral_AttributeNewFromObjs(
    Tcl_Interp *interp,
    Tcl_Obj *nameObj,
    Tcl_Obj *typeObj)
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
	    Ral_TupleHeadingNewFromObj(interp, *(typev + 1)) ;

	if (heading) {
	    attribute = Ral_AttributeNewTupleType(attrName, heading) ;
	}
    } else if (strcmp("Relation", typeName) == 0 && typec == 3) {
	Ral_RelationHeading heading =
	    Ral_RelationHeadingNewFromObjs(interp, typev[1], typev[2]) ;

	if (heading) {
	    attribute = Ral_AttributeNewRelationType(attrName, heading) ;
	}
    } else if (typec == 1) {
	Tcl_ObjType *tclType = Tcl_GetObjType(typeName) ;

	if (tclType != NULL) {
	    attribute = Ral_AttributeNewTclType(attrName, tclType) ;
	} else {
	    Tcl_ResetResult(interp) ;
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"unknown data type, \"", typeName, "\"",
		NULL) ;
	}
    } else {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    "bad type specification, \"", Tcl_GetString(typeObj),
	    "\"", NULL) ;
    }

    return attribute ;
}

int
Ral_AttributeConvertValueToType(
    Tcl_Interp *interp,
    Ral_Attribute attr,
    Tcl_Obj *objPtr)
{
    int result = TCL_OK ;

    switch (attr->attrType) {
    case Tcl_Type:
	result = Tcl_ConvertToType(interp, objPtr, attr->tclType) ;
	if (result == TCL_OK && strcmp(attr->tclType->name, "string") != 0) {
	    Tcl_InvalidateStringRep(objPtr) ;
	}
	break ;

    case Tuple_Type:
	if (objPtr->typePtr != &Ral_TupleObjType) {
	    result = Ral_TupleObjConvert(attr->tupleHeading, interp, objPtr,
		objPtr) ;
	}
	break ;

    case Relation_Type:
	if (objPtr->typePtr != &Ral_RelationObjType) {
	    result = Ral_RelationObjConvert(attr->relationHeading, interp,
		objPtr, objPtr) ;
	}
	break ;

    default:
	Tcl_Panic("unknown attribute data type") ;
	break ;
    }

    return result ;
}

void
Ral_AttributeDelete(
    Ral_Attribute a)
{
    ckfree((char *)a) ;
}

Ral_Attribute
Ral_AttributeDup(
    Ral_Attribute a)
{
    switch (a->attrType) {
    case Tcl_Type:
	return Ral_AttributeNewTclType(a->name, a->tclType) ;

    case Tuple_Type:
	return Ral_AttributeNewTupleType(a->name, a->tupleHeading) ;

    case Relation_Type:
	return Ral_AttributeNewRelationType(a->name, a->relationHeading) ;

    default:
	Tcl_Panic("Ral_AttributeDup: unknown attribute type: %d",
	    a->attrType) ;
    }
    /* Not reached */
    return NULL ;
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

    switch (a1->attrType) {
    case Tcl_Type:
	return a1->tclType == a2->tclType ;

    case Tuple_Type:
	return Ral_TupleHeadingEqual(a1->tupleHeading, a2->tupleHeading) ;

    case Relation_Type:
	return Ral_RelationHeadingEqual(a1->relationHeading,
	    a2->relationHeading) ;

    default:
	Tcl_Panic("Ral_AttributeEqual: unknown attribute type: %d",
	    a1->attrType) ;
    }
    /* Not reached */
    return 1 ;
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
    int length ;

    flags->attrType = a->attrType ;
    switch (a->attrType) {
    case Tcl_Type:
	length = Tcl_ScanElement(a->tclType->name, &flags->simpleFlags) ;
	break ;

    case Tuple_Type:
	length = Ral_TupleHeadingScan(a->tupleHeading, flags) ;
	length += sizeof(openList) - 1 + sizeof(closeList) - 1;
	break ;

    case Relation_Type:
	length = Ral_RelationHeadingScan(a->relationHeading, flags) ;
	length += sizeof(openList) - 1 + sizeof(closeList) - 1;
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
    int length ;

    switch (a->attrType) {
    case Tcl_Type:
	length = Tcl_ConvertElement(a->tclType->name, dst, flags->simpleFlags) ;
	break ;

    case Tuple_Type: {
	char *p = dst ;

	strcpy(p, openList) ;
	p += sizeof(openList) - 1 ;
	p += Ral_TupleHeadingConvert(a->tupleHeading, p, flags) ;
	strcpy(p, closeList) ;
	p += sizeof(closeList) - 1;
	length = p - dst ;
    }
	break ;

    case Relation_Type: {
	char *p = dst ;

	strcpy(p, openList) ;
	p += sizeof(openList) - 1 ;
	p += Ral_RelationHeadingConvert(a->relationHeading, p, flags) ;
	strcpy(p, closeList) ;
	p += sizeof(closeList) - 1;
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
    int length ;

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
    int length ;

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
