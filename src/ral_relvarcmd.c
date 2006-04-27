/*
This software is copyrighted 2006 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_relvarcmd.c,v $
$Revision: 1.2 $
$Date: 2006/04/27 14:48:56 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <string.h>
#include "tcl.h"
#include "ral_relvarcmd.h"
#include "ral_relvarobj.h"
#include "ral_relvar.h"
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
static int RelvarAssociationCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarCreateCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarDeleteCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarDeleteOneCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarDestroyCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
static int RelvarEvalCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarInsertCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarNamesCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarSetCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUpdateCmd(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
static int RelvarUpdateOneCmd(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relvarcmd.c,v $ $Revision: 1.2 $" ;

/*
FUNCTION DEFINITIONS
*/

int
relvarCmd(
    ClientData clientData,  /* Contains an interpreter id */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    static const struct cmdMap {
	const char *cmdName ;
	int (*cmdFunc)(Tcl_Interp *, int, Tcl_Obj *const*, Ral_RelvarInfo) ;
    } cmdTable[] = {
	{"association", RelvarAssociationCmd},
	{"create", RelvarCreateCmd},
	{"delete", RelvarDeleteCmd},
	{"deleteone", RelvarDeleteOneCmd},
	{"destroy", RelvarDestroyCmd},
	{"eval", RelvarEvalCmd},
	{"insert", RelvarInsertCmd},
	{"names", RelvarNamesCmd},
	{"set", RelvarSetCmd},
	{"update", RelvarUpdateCmd},
	{"updateone", RelvarUpdateOneCmd},
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

    return cmdTable[index].cmdFunc(interp, objc, objv,
	(Ral_RelvarInfo)clientData) ;
}

const char *
Ral_RelvarCmdVersion(void)
{
    return rcsid ;
}
/*
 * ======================================================================
 * Relvar Sub-Command Functions
 * ======================================================================
 */

static int
RelvarAssociationCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    enum AssocCmdType {
	AssocCreate,
	AssocDelete,
	AssocInfo,
    } ;
    static const char *assocCmds[] = {
	"create",
	"delete",
	"info",
	NULL
    } ;
    int result = TCL_ERROR ;
    int index ;

    /* relvar association create name relvar1 attr-list1 spec1
     * relvar2 attr-list2 spec2 */
    /* relvar association delete name */
    /* relvar association info name */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "create | delete | info name ?args?") ;
	return TCL_ERROR ;
    }

    if (Tcl_GetIndexFromObj(interp, (Tcl_Obj *)objv[2], assocCmds,
	"association subcommand", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    switch ((enum AssocCmdType)index) {
    case AssocCreate:
	if (objc != 10) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		"name relvar1 attr-list1 spec1 relvar2 attr-list2 spec2") ;
	    return TCL_ERROR ;
	}
	result = Ral_RelvarObjCreateAssoc(interp, objv + 3, rInfo) ;
	break ;

    case AssocDelete:
	break ;

    case AssocInfo:
	break ;

    default:
	Tcl_Panic("Unknown association command type, %d", index) ;
    }

    Tcl_ResetResult(interp) ;
    return result ;
}

static int
RelvarCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_RelationHeading heading ;

    /* relvar create relvarName heading */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName heading") ;
	return TCL_ERROR ;
    }

    if (Tcl_ListObjGetElements(interp, objv[3], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }
    if (elemc != 3) {
	Ral_RelationObjSetError(interp, REL_FORMAT_ERR,
	    Tcl_GetString(objv[3])) ;
	return TCL_ERROR ;
    }
    if (strcmp(Ral_RelationObjType.name, Tcl_GetString(*elemv)) != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_KEYWORD,
	    Tcl_GetString(*elemv)) ;
	return TCL_ERROR ;
    }
    /*
     * Create the heading from the external representation.
     */
    heading = Ral_RelationHeadingNewFromObjs(interp, elemv[1], elemv[2]) ;
    if (!heading) {
	return TCL_ERROR ;
    }

    return Ral_RelvarObjNew(interp, rInfo, Tcl_GetString(objv[2]), heading) ;
}

static int
RelvarDeleteCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    return TCL_ERROR ;
}

static int
RelvarDeleteOneCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    return TCL_ERROR ;
}

static int
RelvarDestroyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    /* relvar destroy relvar */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvar") ;
	return TCL_ERROR ;
    }

    /*
     * HERE -- not this simple.
     */
    return Ral_RelvarObjDelete(interp, rInfo, objv[2]) ;
}

static int
RelvarEvalCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Tcl_Obj *scriptObj ;
    int result ;

    /* relvar eval script */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "script") ;
	return TCL_ERROR ;
    }
    scriptObj = objv[2] ;

    Ral_RelvarStartTransaction(rInfo, 0) ;

    result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
    if (result == TCL_ERROR) {
	static const char msgfmt[] =
	    "\n    (\"relvar eval\" body line %d)" ;
	char msg[sizeof(msgfmt) + TCL_INTEGER_SPACE] ;
	sprintf(msg, msgfmt, interp->errorLine) ;
	Tcl_AddObjErrorInfo(interp, msg, -1) ;
    }

    Ral_RelvarEndTransaction(rInfo, result == TCL_ERROR) ;

    return result ;
}

static int
RelvarInsertCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relation ;
    int inserted = 0 ;

    /* relvar insert relvarName ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvarName ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    relation = relvar->relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    Ral_RelvarStartCommand(rInfo, relvar) ;
    Ral_RelationReserve(relation, objc) ;
    while (objc-- > 0) {
	if (Ral_RelationInsertTupleObj(relation, interp, *objv++) != TCL_OK) {
	    Ral_RelvarEndCommand(rInfo, relvar, 1) ;
	    return TCL_ERROR ;
	}
	++inserted ;
    }

    if (inserted) {
	Tcl_InvalidateStringRep(relvar->relObj) ;
    }

    Ral_RelvarEndCommand(rInfo, relvar, 0) ;
    Tcl_SetObjResult(interp, relvar->relObj) ;
    return TCL_OK ;
}

static int
RelvarNamesCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    const char *pattern ;
    Tcl_Obj *nameList ;
    Tcl_HashEntry *entry ;
    Tcl_HashSearch search ;
    Tcl_HashTable *relvarMap = &rInfo->relvars ;

    /* relvar names ?pattern? */
    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "?pattern?") ;
	return TCL_ERROR ;
    }

    pattern = objc == 3 ? Tcl_GetString(objv[2]) : NULL ;
    nameList = Tcl_NewListObj(0, NULL) ;

    for (entry = Tcl_FirstHashEntry(relvarMap, &search) ; entry ;
	entry = Tcl_NextHashEntry(&search)) {
	const char *relvarName ;

	relvarName = (const char *)Tcl_GetHashKey(relvarMap, entry) ;
	if (pattern && !Tcl_StringMatch(relvarName, pattern)) {
	    continue ;
	}
	if (Tcl_ListObjAppendElement(interp, nameList,
	    Tcl_NewStringObj(relvarName, -1)) != TCL_OK) {
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
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    Ral_Relvar relvar ;
    Ral_Relation relvalue ;

    /* relvar set relvar relationValue */
    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relvar ?relationValue?") ;
	return TCL_ERROR ;
    }

    relvar = Ral_RelvarObjFindRelvar(interp, rInfo, Tcl_GetString(objv[2]),
	NULL) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }
    if (Tcl_ConvertToType(interp, relvar->relObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relvalue = relvar->relObj->internalRep.otherValuePtr ;

    if (objc > 3) {
	Tcl_Obj *valueObj ;
	Ral_Relation relation ;

	valueObj = objv[3] ;
	if (Tcl_ConvertToType(interp, valueObj, &Ral_RelationObjType)
	    != TCL_OK) {
	    return TCL_ERROR ;
	}
	relation = valueObj->internalRep.otherValuePtr ;

	if (!Ral_RelationHeadingEqual(relvalue->heading, relation->heading)) {
	    char *headingStr = Ral_RelationHeadingStringOf(relation->heading) ;
	    Ral_RelvarObjSetError(interp, RELVAR_HEADING_MISMATCH, headingStr) ;
	    ckfree(headingStr) ;
	    return TCL_ERROR ;
	}

	Ral_RelvarStartCommand(rInfo, relvar) ;
	Ral_RelvarSetRelation(relvar, relation) ;
	Ral_RelvarEndCommand(rInfo, relvar, 0) ;
    }

    Tcl_SetObjResult(interp, relvar->relObj) ;
    return TCL_OK ;
}

static int
RelvarUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    return TCL_ERROR ;
}

static int
RelvarUpdateOneCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv,
    Ral_RelvarInfo rInfo)
{
    return TCL_ERROR ;
}
