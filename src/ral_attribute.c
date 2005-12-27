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
$Revision: 1.1 $
$Date: 2005/12/27 23:17:19 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_attribute.c,v $
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
static const char rcsid[] = "@(#) $RCSfile: ral_attribute.c,v $ $Revision: 1.1 $" ;

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
	    Ral_RelationHeadingNewFromObj(interp, *(typev + 1)) ;

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
	if (objPtr->typePtr != &Ral_TupleObjType) {
	    Ral_Tuple tuple = Ral_TupleNew(attr->tupleHeading) ;

	    if (Ral_TupleSetValuesFromObj(tuple, interp, objPtr) != TCL_OK) {
		Ral_TupleDelete(tuple) ;
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
	    objPtr->typePtr = &Ral_TupleObjType ;
	    objPtr->internalRep.otherValuePtr = tuple ;
	}
	break ;

    case Relation_Type:
#if 0 /* HERE -- awaiting further relation implementation */
	if (objPtr->typePtr != &Ral_RelationType) {
	    Ral_Relation relation = Ral_RelationNew(attr->relationHeading) ;

	    if (Ral_RelationSetValuesFromObj(interp, relation, objPtr)
		!= TCL_OK) {
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
	     * Invalidate the string representation.
	     */
	    Tcl_InvalidateStringRep(objPtr) ;
	    /*
	     * Install the new internal representation.
	     */
	    objPtr->typePtr = &Ral_RelationType ;
	    objPtr->internalRep.otherValuePtr = relation ;
	}
#endif
	break ;

    default:
	Tcl_Panic("unknown attribute data type") ;
	break ;
    }

    return TCL_OK ;
}

void
Ral_AttributeDelete(
    Ral_Attribute a)
{
    ckfree((char *)a) ;
}

Ral_Attribute
Ral_AttributeCopy(
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
	Tcl_Panic("Ral_AttributeCopy: unknown attribute type: %d",
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
	Tcl_Panic("Ral_AttributeCopy: unknown attribute type: %d",
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
	return a1->tupleHeading == a2->tupleHeading ;

    case Relation_Type:
	return a1->relationHeading == a2->relationHeading ;

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
    int *flagsPtr)
{
    return Tcl_ScanElement(a->name, flagsPtr) ;
}

int
Ral_AttributeConvertName(
    Ral_Attribute a,
    char *dst,
    int flags)
{
    return Tcl_ConvertElement(a->name, dst, flags) ;
}

int
Ral_AttributeScanType(
    Ral_Attribute a,
    int **flagsHandle)
{
    int count ;

    switch (a->attrType) {
    case Tcl_Type: {
	int *flagsPtr ;

	flagsPtr = (int *)ckalloc(sizeof(*flagsPtr)) ;
	count = Tcl_ScanElement(a->tclType->name, flagsPtr) ;
	*flagsHandle = flagsPtr ;
	break ;
    }

    case Tuple_Type:
	Tcl_Panic("Ral_AttributeScanType: unsupported Tuple type") ;

    case Relation_Type:
	Tcl_Panic("Ral_AttributeScanType: unsupported Relation type") ;

    default:
	Tcl_Panic("Ral_AttributeScanType: unknown attribute type: %d",
	    a->attrType) ;
    }
    return count ;
}

int
Ral_AttributeConvertType(
    Ral_Attribute a,
    char *dst,
    int *flagsHandle)
{
    int count ;

    switch (a->attrType) {
    case Tcl_Type:
	count = Tcl_ConvertElement(a->tclType->name, dst, *flagsHandle) ;
	break ;

    case Tuple_Type:
	Tcl_Panic("Ral_AttributeScanType: unsupported Tuple type") ;

    case Relation_Type:
	Tcl_Panic("Ral_AttributeScanType: unsupported Relation type") ;

    default:
	Tcl_Panic("Ral_AttributeScanType: unknown attribute type: %d",
	    a->attrType) ;
    }

    ckfree((char *)flagsHandle) ;
    return count ;
}
