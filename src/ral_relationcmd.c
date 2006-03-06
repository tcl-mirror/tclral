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

$RCSfile: ral_relationcmd.c,v $
$Revision: 1.4 $
$Date: 2006/03/06 01:07:37 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_relationcmd.c,v $
Revision 1.4  2006/03/06 01:07:37  mangoa01
More relation commands done. Cleaned up error reporting.

Revision 1.3  2006/03/01 02:28:40  mangoa01
Added new relation commands and test cases. Cleaned up Makefiles.

Revision 1.2  2006/02/26 04:57:53  mangoa01
Reworked the conversion from internal form to a string yet again.
This design is better and more recursive in nature.
Added additional code to the "relation" commands.
Now in a position to finish off the remaining relation commands.

Revision 1.1  2006/02/20 20:15:07  mangoa01
Now able to convert strings to relations and vice versa including
tuple and relation valued attributes.

 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <assert.h>
#include "tcl.h"
#include "ral_relation.h"
#include "ral_relationobj.h"
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
static int RelationCardinalityCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDegreeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDivideCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationEliminateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationEmptyofCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationExtendCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationForeachCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationGroupCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationHeadingCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIdentifiersCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIntersectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsemptyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsnotemptyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationJoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationListCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationMinusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationProjectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRenameCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRestrictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRestrictOneCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemijoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemiminusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSummarizeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTimesCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTupleCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUngroupCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUnionCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_relationcmd.c,v $ $Revision: 1.4 $" ;

/*
FUNCTION DEFINITIONS
*/

/*
 * ======================================================================
 * Relation Ensemble Command Function
 * ======================================================================
 */

int
relationCmd(
    ClientData clientData,  /* Not used */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    static const struct cmdMap {
	const char *cmdName ;
	int (*const cmdFunc)(Tcl_Interp *, int, Tcl_Obj *const*) ;
    } cmdTable[] = {
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
	{"restrictone", RelationRestrictOneCmd},
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

const char *
Ral_RelationCmdVersion(void)
{
    return rcsid ;
}
/*
 * ======================================================================
 * Relations Sub-Command Functions
 * ======================================================================
 */
static int
RelationCardinalityCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation relation ;

    /* relation cardinality relValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewIntObj(Ral_RelationCardinality(relation))) ;
    return TCL_OK ;
}

static int
RelationDegreeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation relation ;

    /* relation degree relValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewIntObj(Ral_RelationDegree(relation))) ;
    return TCL_OK ;
}

static int
RelationDivideCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationEliminateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_IntVector elimAttrs ;
    Ral_IntVector projAttrs ;
    Ral_Relation projRel ;

    /* relation eliminate relationValue ?attr ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue ?attr ... ?") ;
	return TCL_ERROR ;
    }
    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    elimAttrs = Ral_TupleHeadingAttrsFromVect(relation->heading->tupleHeading,
	interp, objc, objv) ;
    if (elimAttrs == NULL) {
	return TCL_ERROR ;
    }
    projAttrs = Ral_IntVectorSetComplement(elimAttrs,
	Ral_RelationDegree(relation)) ;
    Ral_IntVectorDelete(elimAttrs) ;

    projRel = Ral_RelationProject(relation, projAttrs) ;
    Ral_IntVectorDelete(projAttrs) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(projRel)) ;

    return TCL_OK ;
}

static int
RelationEmptyofCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation relation ;

    /* relation emptyof relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    /*
     * An empty version of a relation is obtained by creating a new relation
     * from the heading of the old one and making the new relation into an
     * object.  Since relation headings are reference counted this works.
     */
    Tcl_SetObjResult(interp,
	Ral_RelationObjNew(Ral_RelationNew(relation->heading))) ;
    return TCL_OK ;
}

static int
RelationExtendCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
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
    Ral_Relation relation ;
    Ral_Relation sortedRel ;
    Ral_RelationIter relIter ;
    Ral_RelationIter relEnd ;
    int result = TCL_OK ;

    /*
     * relation foreach tupleVarName relationValue ?ascending | descending?
     * ?attr-list? script
     */
    if (objc < 5 || objc > 7) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleVarName relationValue ?ascending | descending? ?attr-list?"
	    "script") ;
	return TCL_ERROR ;
    }

    varNameObj = objv[2] ;
    relObj = objv[3] ;

    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    if (objc == 5) {
	/*
	 * No order specified.
	 */
	sortedRel = relation ;
	scriptObj = objv[4] ;
    } else if (objc == 6) {
	Ral_IntVector sortAttrs = Ral_TupleHeadingAttrsFromObj(
	    relation->heading->tupleHeading, interp, objv[4]) ;
	if (sortAttrs == NULL) {
	    return TCL_ERROR ;
	}
	sortedRel = Ral_RelationSortAscending(relation, sortAttrs) ;
	Ral_IntVectorDelete(sortAttrs) ;
	scriptObj = objv[5] ;
    } else /* objc == 7 */ {
	int index ;
	Ral_IntVector sortAttrs ;

	if (Tcl_GetIndexFromObj(interp, objv[4], orderKeyWords, "ordering", 0,
	    &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	sortAttrs = Ral_TupleHeadingAttrsFromObj(
	    relation->heading->tupleHeading, interp, objv[5]) ;
	if (sortAttrs == NULL) {
	    return TCL_ERROR ;
	}
	sortedRel = index == SORT_ASCENDING ?
	    Ral_RelationSortAscending(relation, sortAttrs) :
	    Ral_RelationSortDescending(relation, sortAttrs) ;
	Ral_IntVectorDelete(sortAttrs) ;
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

    relEnd = Ral_RelationEnd(sortedRel) ;
    for (relIter = Ral_RelationBegin(sortedRel) ;
	relIter != relEnd ; ++relIter) {
	Tcl_Obj *tupleObj ;

	tupleObj = Ral_TupleObjNew(*relIter) ;
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

    if (Ral_RelationCardinality(sortedRel)) {
	Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    }
    if (sortedRel != relation) {
	Ral_RelationDelete(sortedRel) ;
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
    return TCL_ERROR ;
}

static int
RelationHeadingCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    char *strRep ;
    Tcl_Obj *resultObj ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    strRep = Ral_RelationHeadingStringOf(relation->heading) ;
    resultObj = Tcl_NewStringObj(strRep, -1) ;
    ckfree(strRep) ;

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

static int
RelationIdentifiersCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    Tcl_Obj *idListObj ;
    int idCount ;
    Ral_IntVector *ids ;

    /* relation identifiers relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;
    tupleHeading = heading->tupleHeading ;

    idListObj = Tcl_NewListObj(0, NULL) ;
    for (idCount = heading->idCount, ids = heading->identifiers ;
	idCount > 0 ; --idCount, ++ids) {
	Tcl_Obj *idObj = Tcl_NewListObj(0, NULL) ;
	Ral_IntVector idVect = *ids ;
	Ral_IntVectorIter end = Ral_IntVectorEnd(idVect) ;
	Ral_IntVectorIter iter ;

	for (iter = Ral_IntVectorBegin(idVect) ; iter != end ; ++iter) {
	    Ral_Attribute attr = Ral_TupleHeadingFetch(tupleHeading, *iter) ;

	    if (Tcl_ListObjAppendElement(interp, idObj,
		Tcl_NewStringObj(attr->name, -1)) != TCL_OK) {
		Tcl_DecrRefCount(idObj) ;
		Tcl_DecrRefCount(idListObj) ;
		return TCL_ERROR ;
	    }
	}

	if (Tcl_ListObjAppendElement(interp, idListObj, idObj) != TCL_OK) {
	    Tcl_DecrRefCount(idObj) ;
	    Tcl_DecrRefCount(idListObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, idListObj) ;
    return TCL_OK ;
}

static int
RelationIntersectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r1 ;
    Ral_Relation r2 ;
    Ral_Relation intersectRel ;

    /* relation intersect relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
	return TCL_ERROR ;
    }

    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    intersectRel = Ral_RelationIntersect(r1, r2) ;
    if (intersectRel == NULL) {
	Ral_RelationObjSetError(interp, REL_HEADING_NOT_EQUAL,
	    Tcl_GetString(r2Obj)) ;
	return TCL_ERROR ;
    }

    /*
     * Increment past the first two relations and perform the intersect
     * on the remaining values.
     */
    objc -= 4 ;
    objv += 4 ;
    while (objc-- > 0) {
	r1 = intersectRel ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	intersectRel = Ral_RelationIntersect(r1, r2) ;
	Ral_RelationDelete(r1) ;
	if (intersectRel == NULL) {
	    Ral_RelationObjSetError(interp, REL_HEADING_NOT_EQUAL,
		Tcl_GetString(r2Obj)) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(intersectRel)) ;
    return TCL_OK ;
}

static int
RelationIsCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    static const struct cmdMap {
	const char *cmdName ;
	int (*const cmdFunc)(Ral_Relation, Ral_Relation) ;
    } cmdTable[] = {
	{"equal", Ral_RelationEqual},
	{"==", Ral_RelationEqual},
	{"notequal", Ral_RelationNotEqual},
	{"!=", Ral_RelationNotEqual},
	{"propersubsetof", Ral_RelationProperSubsetOf},
	{"<", Ral_RelationProperSubsetOf},
	{"propersupersetof", Ral_RelationProperSupersetOf},
	{">", Ral_RelationProperSupersetOf},
	{"subsetof", Ral_RelationSubsetOf},
	{"<=", Ral_RelationSubsetOf},
	{"supersetof", Ral_RelationSupersetOf},
	{">=", Ral_RelationSupersetOf},
	{NULL, NULL}
    } ;

    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r1 ;
    Ral_Relation r2 ;
    int index ;
    int result ;

    /* relation is relation1 compareop relation2 */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation1 compareop relation2") ;
	return TCL_ERROR ;
    }

    r1Obj = objv[2] ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    if (Tcl_GetIndexFromObjStruct(interp, objv[3], cmdTable,
	sizeof(struct cmdMap), "compareop", 0, &index) != TCL_OK) {
	return TCL_ERROR ;
    }

    r2Obj = objv[4] ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    result = cmdTable[index].cmdFunc(r1, r2) ;
    if (result < 0) {
	Ral_RelationObjSetError(interp, REL_HEADING_NOT_EQUAL,
	    Tcl_GetString(r2Obj)) ;
	return TCL_ERROR ;
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
    Ral_Relation relation ;

    /* relation isempty relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
	Ral_RelationCardinality(relation) == 0)) ;
    return TCL_OK ;
}

static int
RelationIsnotemptyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation relation ;

    /* relation isnotempty relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
	Ral_RelationCardinality(relation) != 0)) ;
    return TCL_OK ;
}

static int
RelationJoinCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationListCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation list relationValue */
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *listObj ;
    Ral_RelationIter iter ;
    Ral_RelationIter end ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    if (Ral_RelationDegree(relation) != 1) {
	Ral_RelationObjSetError(interp, REL_DEGREE_ONE, Tcl_GetString(relObj)) ;
	return TCL_ERROR ;
    }

    listObj = Tcl_NewListObj(0, NULL) ;
    end = Ral_RelationEnd(relation) ;
    for (iter = Ral_RelationBegin(relation) ; iter != end ; ++iter) {
	Ral_Tuple tuple = *iter ;

	if (Tcl_ListObjAppendElement(interp, listObj, *tuple->values)
	    != TCL_OK) {
	    Tcl_DecrRefCount(listObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, listObj) ;
    return TCL_OK ;
}

static int
RelationMinusCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r1 ;
    Ral_Relation r2 ;
    Ral_Relation diffRel ;

    /* relation minus relation1 relation2 */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation1 relation2") ;
	return TCL_ERROR ;
    }

    r1Obj = objv[2] ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    r2Obj = objv[3] ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    diffRel = Ral_RelationMinus(r1, r2) ;
    if (diffRel == NULL) {
	Ral_RelationObjSetError(interp, REL_HEADING_NOT_EQUAL,
	    Tcl_GetString(r2Obj)) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(diffRel)) ;
    return TCL_OK ;
}

static int
RelationProjectCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_IntVector projAttrs ;
    Ral_Relation projRel ;

    /* relation project relationValue ?attr ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue ?attr ... ?") ;
	return TCL_ERROR ;
    }
    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    projAttrs = Ral_TupleHeadingAttrsFromVect(relation->heading->tupleHeading,
	interp, objc, objv) ;
    if (projAttrs == NULL) {
	return TCL_ERROR ;
    }

    projRel = Ral_RelationProject(relation, projAttrs) ;
    Ral_IntVectorDelete(projAttrs) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(projRel)) ;

    return TCL_OK ;
}

static int
RelationRenameCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_Relation newRelation ;

    /* relation rename relationValue ?oldname newname ... ? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue ?oldname newname ... ?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_PAIRS_LIST,
	    "oldname / newname arguments must be given in pairs") ;
	return TCL_ERROR ;
    }

    newRelation = Ral_RelationDup(relation) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	if (!Ral_RelationRenameAttribute(newRelation, Tcl_GetString(objv[0]),
	    Tcl_GetString(objv[1]))) {
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
		Tcl_GetString(objv[1])) ;
	    Ral_RelationDelete(newRelation) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
    return TCL_OK ;
}

static int
RelationRestrictCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *exprObj ;
    Tcl_Obj *varNameObj ;
    Ral_Relation newRelation ;
    Ral_RelationIter iter ;
    Ral_RelationIter end ;

    /* relation restrict relValue tupleVarName expr */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue tupleVarName expr") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    varNameObj = objv[3] ;
    exprObj = objv[4] ;

    newRelation = Ral_RelationNew(relation->heading) ;

    Tcl_IncrRefCount(exprObj) ;
    Tcl_IncrRefCount(varNameObj) ;

    end = Ral_RelationEnd(relation) ;
    for (iter = Ral_RelationBegin(relation) ; iter != end ; ++iter) {
	Ral_Tuple tuple = *iter ;
	Tcl_Obj *tupleObj = Ral_TupleObjNew(tuple) ;
	int boolValue ;

	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    goto errorOut ;
	}

	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    goto errorOut ;
	}
	if (boolValue) {
	    int inserted ;
	    inserted = Ral_RelationPushBack(newRelation, tuple, NULL) ;
	    assert(inserted != 0) ;
	}
    }

    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(exprObj) ;
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(exprObj) ;
    Tcl_DecrRefCount(varNameObj) ;
    Ral_RelationDelete(newRelation) ;
    return TCL_ERROR ;
}

static int
RelationRestrictOneCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationSemijoinCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationSemiminusCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationSummarizeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationTimesCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r1 ;
    Ral_Relation r2 ;
    Ral_Relation prodRel ;

    /* relation times relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
	return TCL_ERROR ;
    }

    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    prodRel = Ral_RelationTimes(r1, r2) ;
    if (prodRel == NULL) {
	Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
	    Tcl_GetString(r2Obj)) ;
	return TCL_ERROR ;
    }

    /*
     * Increment past the first two relations and perform the multiplication
     * on the remaining values.
     */
    objc -= 4 ;
    objv += 4 ;
    while (objc-- > 0) {
	r1 = prodRel ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	prodRel = Ral_RelationTimes(r1, r2) ;
	Ral_RelationDelete(r1) ;
	if (prodRel == NULL) {
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
		Tcl_GetString(r2Obj)) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(prodRel)) ;
    return TCL_OK ;
}

static int
RelationTupleCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation tuple relationValue */
    Tcl_Obj *relObj ;
    Ral_Relation relation ;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    if (Ral_RelationCardinality(relation) != 1) {
	Ral_RelationObjSetError(interp, REL_CARDINALITY_ONE,
	    Tcl_GetString(relObj)) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_TupleObjNew(*Ral_RelationBegin(relation))) ;
    return TCL_OK ;
}

static int
RelationUngroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    return TCL_ERROR ;
}

static int
RelationUnionCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r1 ;
    Ral_Relation r2 ;
    Ral_Relation unionRel ;

    /* relation union relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
	return TCL_ERROR ;
    }

    r1Obj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, r1Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r1 = r1Obj->internalRep.otherValuePtr ;

    r2Obj = *(objv + 3) ;
    if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    r2 = r2Obj->internalRep.otherValuePtr ;

    unionRel = Ral_RelationUnion(r1, r2) ;
    if (unionRel == NULL) {
	Ral_RelationObjSetError(interp, REL_HEADING_NOT_EQUAL,
	    Tcl_GetString(r2Obj)) ;
	return TCL_ERROR ;
    }

    /*
     * Increment past the first two relations and perform the union
     * on the remaining values.
     */
    objc -= 4 ;
    objv += 4 ;
    while (objc-- > 0) {
	r1 = unionRel ;

	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;

	unionRel = Ral_RelationUnion(r1, r2) ;
	Ral_RelationDelete(r1) ;
	if (unionRel == NULL) {
	    Ral_RelationObjSetError(interp, REL_HEADING_NOT_EQUAL,
		Tcl_GetString(r2Obj)) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(unionRel)) ;
    return TCL_OK ;
}
