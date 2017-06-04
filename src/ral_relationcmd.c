/*
This software is copyrighted 2005 - 2017 by G. Andrew Mangogna.  The following
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
    ral_relationcmd -- command interface functions for the "relation" command

ABSTRACT:
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <assert.h>
#include <string.h>
#include "tcl.h"
#include "ral_attribute.h"
#include "ral_relation.h"
#include "ral_relationobj.h"
#include "ral_tupleobj.h"
#include "ral_joinmap.h"

/*
MACRO DEFINITIONS
*/

/*
TYPE DEFINITIONS
*/
static const struct {
    const char *optName ;
    enum Ordering {
	SORT_ASCENDING,
	SORT_DESCENDING
    } type ;
} orderOptions[] = {
    {"-ascending", SORT_ASCENDING},
    {"-descending", SORT_DESCENDING},
    {NULL, 0},
} ;

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static int RelationArrayCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationAssignCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationAttributesCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationBodyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationCardinalityCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationComposeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationCreateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDegreeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDivideCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDunionCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationEliminateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationEmptyofCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationExtendCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationExtractCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationForeachCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationFromdictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationFromlistCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationGroupCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationHeadingCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationInsertCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIntersectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsemptyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsnotemptyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIssametypeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationJoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationListCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationMinusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationProjectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRankCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRenameCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRestrictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRestrictWithCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemijoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemiminusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSummarizeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSummarizebyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTableCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTagCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTcloseCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTimesCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTupleCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUinsertCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUngroupCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUnionCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUpdateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationUnwrapCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationWrapCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/

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
	{"array", RelationArrayCmd},
	{"assign", RelationAssignCmd},
	{"attributes", RelationAttributesCmd},
	{"body", RelationBodyCmd},
	{"cardinality", RelationCardinalityCmd},
	{"compose", RelationComposeCmd},
	{"create", RelationCreateCmd},
	{"degree", RelationDegreeCmd},
	{"dict", RelationDictCmd},
	{"divide", RelationDivideCmd},
        {"dunion", RelationDunionCmd},
	{"eliminate", RelationEliminateCmd},
	{"emptyof", RelationEmptyofCmd},
	{"extend", RelationExtendCmd},
	{"extract", RelationExtractCmd},
	{"foreach", RelationForeachCmd},
	{"fromdict", RelationFromdictCmd},
	{"fromlist", RelationFromlistCmd},
	{"group", RelationGroupCmd},
	{"heading", RelationHeadingCmd},
	{"insert", RelationInsertCmd},
	{"intersect", RelationIntersectCmd},
	{"is", RelationIsCmd},
	{"isempty", RelationIsemptyCmd},
	{"isnotempty", RelationIsnotemptyCmd},
	{"issametype", RelationIssametypeCmd},
	{"join", RelationJoinCmd},
	{"list", RelationListCmd},
	{"minus", RelationMinusCmd},
	{"project", RelationProjectCmd},
	{"rank", RelationRankCmd},
	{"rename", RelationRenameCmd},
	{"restrict", RelationRestrictCmd},
	{"restrictwith", RelationRestrictWithCmd},
	{"semijoin", RelationSemijoinCmd},
	{"semiminus", RelationSemiminusCmd},
	{"summarize", RelationSummarizeCmd},
	{"summarizeby", RelationSummarizebyCmd},
	{"table", RelationTableCmd},
	{"tag", RelationTagCmd},
	{"tclose", RelationTcloseCmd},
	{"times", RelationTimesCmd},
	{"tuple", RelationTupleCmd},
	{"uinsert", RelationUinsertCmd},
	{"ungroup", RelationUngroupCmd},
	{"union", RelationUnionCmd},
	{"unwrap", RelationUnwrapCmd},
	{"update", RelationUpdateCmd},
	{"wrap", RelationWrapCmd},
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
 * Relations Sub-Command Functions
 * ======================================================================
 */
static int
RelationArrayCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Tcl_Obj *arrayNameObj ;
    char const *keyAttrName ;
    char const *valueAttrName ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    int keyAttrIndex ;
    int valueAttrIndex ;
    Ral_RelationIter rIter ;

    /* relation array relation arrayName keyAttr valueAttr */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation arrayName keyAttr valueAttr") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    arrayNameObj = objv[3] ;
    keyAttrName = Tcl_GetString(objv[4]) ;
    valueAttrName = Tcl_GetString(objv[5]) ;

    keyAttrIndex = Ral_TupleHeadingIndexOf(heading, keyAttrName) ;
    if (keyAttrIndex < 0) {
        Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptArray,
            RAL_ERR_UNKNOWN_ATTR, keyAttrName) ;
        return TCL_ERROR ;
    }
    valueAttrIndex = Ral_TupleHeadingIndexOf(heading, valueAttrName) ;
    if (valueAttrIndex < 0) {
        Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptArray,
            RAL_ERR_UNKNOWN_ATTR, valueAttrName) ;
        return TCL_ERROR ;
    }

    for (rIter = Ral_RelationBegin(relation) ;
            rIter != Ral_RelationEnd(relation) ; ++rIter) {
	Ral_TupleIter tBegin = Ral_TupleBegin(*rIter) ;
	if (Tcl_ObjSetVar2(interp, arrayNameObj, *(tBegin + keyAttrIndex),
	    *(tBegin + valueAttrIndex), TCL_LEAVE_ERR_MSG) == NULL) {
	    return TCL_ERROR ;
	}
    }

    Tcl_ResetResult(interp) ;
    return TCL_OK ;
}

static int
RelationAssignCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation assign relationValue ?attrName | attr-var-list ...? */
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_ErrorInfo errInfo ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue ?attrName | attr-var-list ...?") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptAssign) ;

    if (Ral_RelationCardinality(relation) != 1) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_CARDINALITY_ONE, relObj) ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    return Ral_TupleAssignToVars(*Ral_RelationBegin(relation), interp,
	objc - 3, objv + 3, &errInfo) ;
}

static int
RelationAttributesCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading tupleHeading ;
    Tcl_Obj *attrListObj ;
    Ral_TupleHeadingIter tIter ;
    Ral_TupleHeadingIter tEnd ;

    /* relation attributes relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    tupleHeading = relation->heading ;

    attrListObj = Tcl_NewListObj(0, NULL) ;
    tEnd = Ral_TupleHeadingEnd(tupleHeading) ;
    for (tIter = Ral_TupleHeadingBegin(tupleHeading) ;
	tIter != tEnd ; ++tIter) {
	Ral_Attribute attr = *tIter ;
	Tcl_Obj *attrObj = Tcl_NewStringObj(attr->name, -1) ;

	if (Tcl_ListObjAppendElement(interp, attrListObj, attrObj) != TCL_OK) {
	    Tcl_DecrRefCount(attrObj) ;
	    Tcl_DecrRefCount(attrListObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, attrListObj) ;
    return TCL_OK ;
}

/*
 * relation body relationValue
 *
 * Returns the body of relation value which is a string formatted as a list
 * dictionaries of the tuple values in the body.
 */
static int
RelationBodyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation relation ;
    char *strRep ;
    Tcl_Obj *resultObj ;

    /* relation cardinality relValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relationObj->internalRep.otherValuePtr ;

    strRep = Ral_RelationValueStringOf(relation) ;
    /*
     * The string representation of a relation value is a list of
     * lists of tuples.
     * We want to flatten that so that we end up just a list of tuple values.
     * So we get rid of the surrounding {}'s.
     */
    resultObj = Tcl_NewStringObj(strRep + 1, strlen(strRep) - 2) ;
    ckfree(strRep) ;

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

static int
RelationCardinalityCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Ral_Relation relation ;

    /* relation cardinality relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
	return TCL_ERROR ;
    }

    relationObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType)
	!= TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewIntObj(Ral_RelationCardinality(relation))) ;
    return TCL_OK ;
}

static int
RelationComposeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r2 ;
    Ral_Relation composeRel ;
    Ral_JoinMap joinMap ;
    Ral_ErrorInfo errInfo ;

    /*
     * relation compose relation1 relation2 ?-using joinAttrs relation3 ... ?
     */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?-using joinAttrs?") ;
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
    joinMap = Ral_JoinMapNew(0, 0) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptCompose) ;

    objc -= 4 ;
    objv += 4 ;

    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	&errInfo) != TCL_OK) {
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    composeRel = Ral_RelationCompose(r1, r2, joinMap, &errInfo) ;
    Ral_JoinMapDelete(joinMap) ;
    if (composeRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    while (objc-- > 0) {
	r1 = composeRel ;
	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;
	joinMap = Ral_JoinMapNew(0, 0) ;

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	    &errInfo) != TCL_OK) {
	    Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	composeRel = Ral_RelationCompose(r1, r2, joinMap, &errInfo) ;
	Ral_RelationDelete(r1) ;
	Ral_JoinMapDelete(joinMap) ;
	if (composeRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(composeRel)) ;
    return TCL_OK ;
}

/*
 * relation create heading ?tuple1 tuple2 ...?
 *
 * Returns a relation value with the given heading and body.
 * Cf. "tuple create"
 */

static int
RelationCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_TupleHeading heading ;
    Ral_ErrorInfo errInfo ;
    Ral_Relation relation ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "heading ?tuple1 tuple2 ...?") ;
	return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptCreate) ;
    /*
     * Create the relation heading from the arguments.
     */
    heading = Ral_TupleHeadingNewFromObj(interp, objv[2], &errInfo) ;
    if (heading == NULL) {
	return TCL_ERROR ;
    }
    relation = Ral_RelationNew(heading) ;

    /*
     * Offset the argument bookkeeping to reference the tuples.
     */
    objc -= 3 ;
    objv += 3 ;
    Ral_RelationReserve(relation, objc) ;
    /*
     * Iterate through the tuple arguments, inserting them into the relation.
     * Here duplicates matter as we deem creation to have "insert" semantics.
     */
    for ( ; objc > 0 ; --objc, ++objv) {
	if (Ral_RelationInsertTupleValue(relation, interp, *objv, &errInfo)
	    != TCL_OK) {
	    Ral_RelationDelete(relation) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(relation)) ;
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

    /* relation degree relationValue */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue") ;
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
RelationDictCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    char const *keyAttrName ;
    char const *valueAttrName ;
    Tcl_Obj *dictObj ;
    int keyAttrIndex ;
    int valueAttrIndex ;
    Ral_RelationIter rIter ;

    /* relation dict relation keyAttr valueAttr */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation keyAttr valueAttr") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    keyAttrName = Tcl_GetString(objv[3]) ;
    valueAttrName = Tcl_GetString(objv[4]) ;

    keyAttrIndex = Ral_TupleHeadingIndexOf(heading, keyAttrName) ;
    if (keyAttrIndex < 0) {
        Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptDict,
            RAL_ERR_UNKNOWN_ATTR, keyAttrName) ;
        return TCL_ERROR ;
    }
    valueAttrIndex = Ral_TupleHeadingIndexOf(heading, valueAttrName) ;
    if (valueAttrIndex < 0) {
        Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptDict,
            RAL_ERR_UNKNOWN_ATTR, valueAttrName) ;
        return TCL_ERROR ;
    }

    dictObj = Tcl_NewDictObj() ;
    for (rIter = Ral_RelationBegin(relation) ;
            rIter != Ral_RelationEnd(relation) ; ++rIter) {
	Ral_TupleIter tBegin = Ral_TupleBegin(*rIter) ;
	if (Tcl_DictObjPut(interp, dictObj, *(tBegin + keyAttrIndex),
	    *(tBegin + valueAttrIndex)) != TCL_OK) {
	    Tcl_DecrRefCount(dictObj) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, dictObj) ;
    return TCL_OK ;
}

static int
RelationDivideCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *dendObj ;
    Ral_Relation dend ;
    Tcl_Obj *dsorObj ;
    Ral_Relation dsor ;
    Tcl_Obj *medObj ;
    Ral_Relation med ;
    Ral_Relation quot ;
    Ral_ErrorInfo errInfo ;

    /* relation divide dividend divisor mediator */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "dividend divisor mediator") ;
	return TCL_ERROR ;
    }

    dendObj = objv[2] ;
    if (Tcl_ConvertToType(interp, dendObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    dend = dendObj->internalRep.otherValuePtr ;

    dsorObj = objv[3] ;
    if (Tcl_ConvertToType(interp, dsorObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    dsor = dsorObj->internalRep.otherValuePtr ;

    medObj = objv[4] ;
    if (Tcl_ConvertToType(interp, medObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    med = medObj->internalRep.otherValuePtr ;

    /*
     * Create the quotient. It has the same heading as the dividend.
     */
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptDivide) ;
    quot = Ral_RelationDivide(dend, dsor, med, &errInfo) ;
    if (quot == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(quot)) ;
    return TCL_OK ;
}

static int
RelationDunionCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r1 ;
    Ral_Relation r2 ;
    Ral_Relation unionRel ;
    Ral_ErrorInfo errInfo ;

    /* relation dunion relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptDunion) ;
    unionRel = Ral_RelationUnion(r1, r2, 1, &errInfo) ;
    if (unionRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
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

	unionRel = Ral_RelationUnion(r1, r2, 1, &errInfo) ;
	Ral_RelationDelete(r1) ;
	if (unionRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(unionRel)) ;
    return TCL_OK ;
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

    elimAttrs = Ral_TupleHeadingAttrsFromVect(relation->heading,
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
    static const char usage[] = "relationValue "
	"tupleVarName ?attr1 type1 expr1 ... attrN typeN exprN?" ;

    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    Tcl_Obj *varNameObj ;
    Ral_TupleHeading extHeading ;
    Ral_Relation extRelation ;
    int c ;
    Tcl_Obj *const*v ;
    Ral_RelationIter relEnd ;
    Ral_RelationIter relIter ;
    Ral_TupleHeadingIter extHeadingIter ;
    Ral_ErrorInfo errInfo ;

    /*
     * relation extend relationValue tupleVarName
     *	    ?attr1 type1 expr1...attrN typeN exprN?
     */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, usage) ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    varNameObj = objv[3] ;
    objc -= 4 ;
    objv += 4 ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptExtend) ;
    if (objc % 3 != 0) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_BAD_TRIPLE_LIST,
	    "attribute / type / expression arguments "
	    "must be given in triples") ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(varNameObj) ;
    /*
     * Make a new tuple heading, adding the extended attributes.
     */
    extHeading = Ral_TupleHeadingExtend(heading, objc / 3) ;
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1),
	    &errInfo) ;
	Ral_TupleHeadingIter inserted ;

	if (attr == NULL) {
	    Ral_TupleHeadingDelete(extHeading) ;
	    return TCL_ERROR ;
	}
	inserted = Ral_TupleHeadingPushBack(extHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(extHeading)) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR, *v) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(extHeading) ;
	    return TCL_ERROR ;
	}
    }
    extRelation = Ral_RelationNew(extHeading) ;

    relEnd = Ral_RelationEnd(relation) ;
    extHeadingIter = Ral_TupleHeadingBegin(extHeading) +
	Ral_RelationDegree(relation) ;
    for (relIter = Ral_RelationBegin(relation) ; relIter != relEnd ;
	++relIter) {
	Ral_Tuple tuple = *relIter ;
	Tcl_Obj *tupleObj = Ral_TupleObjNew(tuple) ;
	Ral_Tuple extTuple = Ral_TupleNew(extHeading) ;
	Ral_TupleIter extIter = Ral_TupleBegin(extTuple) ;
	Ral_TupleHeadingIter attrIter = extHeadingIter ;
	int status ;

	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, tupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(tupleObj) ;
	    goto errorOut ;
	}

	extIter += Ral_TupleCopyValues(Ral_TupleBegin(tuple),
	    Ral_TupleEnd(tuple), extIter) ;

	for (c = objc, v = objv + 2 ; c > 0 ; c -= 3, v += 3) {
	    Tcl_Obj *exprResult ;
	    Tcl_Obj *cvtResult ;

	    if (Tcl_ExprObj(interp, *v, &exprResult) != TCL_OK) {
		Ral_TupleDelete(extTuple) ;
		goto errorOut ;
	    }
	    cvtResult = Ral_AttributeConvertValueToType(interp, *attrIter++,
		exprResult, &errInfo) ;
	    if (cvtResult == NULL) {
		Tcl_DecrRefCount(exprResult) ;
		Ral_TupleDelete(extTuple) ;
		goto errorOut ;
	    }
	    Tcl_IncrRefCount(cvtResult) ;
	    Tcl_DecrRefCount(exprResult) ;
	    *extIter++ = cvtResult ;
	}
	/*
	 * Should always be able to insert the extended tuple since
	 * adding an attribute cannot make a tuple match another one
         * in the relation.
	 */
	status = Ral_RelationPushBack(extRelation, extTuple, NULL) ;
	assert(status != 0) ;
        (void)status ;
    }

    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(extRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Ral_RelationDelete(extRelation) ;
    return TCL_ERROR ;
}

static int
RelationExtractCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation extract relationValue attrName ?attrName2 ...? */
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading;
    Ral_Tuple tuple ;
    char const *attrName ;
    int attrIndex ;
    Tcl_Obj *resultObj ;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue attrName ?attrName2 ...?") ;
	return TCL_ERROR ;
    }

    relObj = *(objv + 2) ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    if (Ral_RelationCardinality(relation) != 1) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptExtract,
	    RAL_ERR_CARDINALITY_ONE, relObj) ;
	return TCL_ERROR ;
    }

    heading = relation->heading ;
    tuple = *Ral_RelationBegin(relation) ;

    objc -= 3 ;
    objv += 3 ;
    if (objc < 2) {
	attrName = Tcl_GetString(*objv) ;
	attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptExtract,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
	resultObj = tuple->values[attrIndex] ;
    } else {
	resultObj = Tcl_NewListObj(0, NULL) ;
	while (objc-- > 0) {
	    attrName = Tcl_GetString(*objv++) ;
	    attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	    if (attrIndex < 0) {
		Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptExtract,
		    RAL_ERR_UNKNOWN_ATTR, attrName) ;
		goto errorOut ;
	    }
	    if (Tcl_ListObjAppendElement(interp, resultObj,
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
RelationForeachCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *varNameObj ;
    Tcl_Obj *relObj ;
    Tcl_Obj *scriptObj ;
    Ral_Relation relation ;
    Ral_IntVector sortMap ;
    Ral_IntVectorIter mapIter ;
    Ral_RelationIter relBegin ;
    int result = TCL_OK ;

    /*
     * relation foreach varName relationValue ?-ascending | -descending?
     * ?attr-list? script
     */
    if (objc < 5 || objc > 7) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "tupleVarName relationValue ?-ascending | -descending? ?attr-list?"
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
	sortMap = Ral_IntVectorNew(Ral_RelationCardinality(relation), 0) ;
	Ral_IntVectorFillConsecutive(sortMap, 0) ;
	scriptObj = objv[4] ;
    } else if (objc == 6) {
	/*
	 * Attribute list specified, ascending order assumed.
	 */
	Ral_IntVector sortAttrs = Ral_TupleHeadingAttrsFromObj(
	    relation->heading, interp, objv[4]) ;
	if (sortAttrs == NULL) {
	    return TCL_ERROR ;
	}
	sortMap = Ral_RelationSort(relation, sortAttrs, 0) ;
	Ral_IntVectorDelete(sortAttrs) ;
	scriptObj = objv[5] ;
    } else /* objc == 7 */ {
	/*
	 * Both ordering keyword and attribute list given.
	 */
	int index ;
	Ral_IntVector sortAttrs ;

	if (Tcl_GetIndexFromObjStruct(interp, objv[4], orderOptions,
	    sizeof(orderOptions[0]), "ordering", 0, &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	sortAttrs = Ral_TupleHeadingAttrsFromObj(
	    relation->heading, interp, objv[5]) ;
	if (sortAttrs == NULL) {
	    return TCL_ERROR ;
	}
	sortMap = Ral_RelationSort(relation, sortAttrs,
	    orderOptions[index].type == SORT_DESCENDING) ;
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

    relBegin = Ral_RelationBegin(relation) ;
    for (mapIter = Ral_IntVectorBegin(sortMap) ;
            mapIter != Ral_IntVectorEnd(sortMap) ; ++mapIter) {
	int appended ;
	Tcl_Obj *iterObj ;
	Ral_Relation iterRel = Ral_RelationNew(relation->heading) ;

	Ral_RelationReserve(iterRel, 1) ;
	appended = Ral_RelationPushBack(iterRel, *(relBegin + *mapIter),
	    NULL) ;
	assert(appended != 0) ;
        (void)appended ;

	iterObj = Ral_RelationObjNew(iterRel) ;

	/*
	 * Count the relation object so that if there is any attempt
	 * to update or unset the relation variable we don't loose
	 * the relation.
	 */
	Tcl_IncrRefCount(iterObj) ;

	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, iterObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(iterObj) ;
	    result = TCL_ERROR ;
	    break; 
	}

	result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
	Tcl_DecrRefCount(iterObj) ;

	if (result != TCL_OK) {
            /*
             * We interpret continue and return as short circuiting
             * the script evaluation to start the next iteration.
             */
	    if (result == TCL_CONTINUE) {
		result = TCL_OK ;
	    } else if (result == TCL_BREAK) {
		result = TCL_OK ;
		break ;
	    } else if (result == TCL_ERROR) {
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
                Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		    "\n    (\"::ral::relation foreach\" body line %d)",
#                   if TCL_MINOR_VERSION < 6
                    interp->errorLine
#                   else
                    Tcl_GetErrorLine(interp)
#                   endif
#endif
                )) ;
		break ;
	    } else {
		break ;
	    }
	}
    }

    if (Ral_RelationCardinality(relation)) {
	Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    }
    Ral_IntVectorDelete(sortMap) ;
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_DecrRefCount(relObj) ;
    Tcl_DecrRefCount(scriptObj) ;

    return result ;
}

static int RelationFromdictCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_Attribute keyattr ;
    Ral_Attribute valattr ;
    Ral_ErrorInfo errInfo ;
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter iter ;
    Ral_Relation relation ;
    Tcl_DictSearch search ;
    Tcl_Obj *keyObj ;
    Tcl_Obj *valObj ;
    int done ;

    /* relation fromdict dictValue keyattr keytype valueattr valuetype */
    if (objc != 7) {
	Tcl_WrongNumArgs(interp, 2, objv,
                "dictValue keyattr keytype valueattr valuetype") ;
	return TCL_ERROR ;
    }

    /*
     * Build the binary heading.
     */
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptFromdict) ;
    keyattr = Ral_AttributeNewFromObjs(interp, objv[3], objv[4], &errInfo) ;
    if (keyattr == NULL) {
        Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    valattr = Ral_AttributeNewFromObjs(interp, objv[5], objv[6], &errInfo) ;
    if (valattr == NULL) {
        Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    heading = Ral_TupleHeadingNew(2) ;
    iter = Ral_TupleHeadingPushBack(heading, keyattr) ;
    assert(iter != Ral_TupleHeadingEnd(heading)) ;
    iter = Ral_TupleHeadingPushBack(heading, valattr) ;
    assert(iter != Ral_TupleHeadingEnd(heading)) ;
    (void)iter ;
    relation = Ral_RelationNew(heading) ;
    /*
     * Iterate through the dictionary, inserting the key / value pairs into
     * the relation.
     * N.B. that we should never get duplicates because the dictionaries
     * are appropriately keyed.
     */
    if (Tcl_DictObjFirst(interp, objv[2], &search, &keyObj, &valObj, &done)
            != TCL_OK) {
        goto errout ;
    }
    while (!done) {
        Ral_Tuple tuple ;
        int status ;

        tuple = Ral_TupleNew(relation->heading) ;
	if (!(Ral_TupleUpdateAttrValue(tuple, keyattr->name, keyObj, &errInfo)
                &&
            Ral_TupleUpdateAttrValue(tuple, valattr->name, valObj, &errInfo))) {
	    Ral_InterpSetError(interp, &errInfo) ;
            Ral_TupleDelete(tuple) ;
            Tcl_DictObjDone(&search) ;
            goto errout ;
	}
        status = Ral_RelationPushBack(relation, tuple, NULL) ;
        assert(status != 0) ;
        (void)status ;

        Tcl_DictObjNext(&search, &keyObj, &valObj, &done) ;
    }
    Tcl_DictObjDone(&search) ;

    Tcl_SetObjResult(interp, Ral_RelationObjNew(relation)) ;
    return TCL_OK ;

errout:
    Ral_RelationDelete(relation) ;
    return TCL_ERROR ;
}

static int RelationFromlistCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    int elemc ;
    Tcl_Obj **elemv ;
    Ral_Attribute attr ;
    Ral_ErrorInfo errInfo ;
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter iter ;
    Ral_Relation relation ;

    /* relation fromlist list attribute type */
    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "list attribute type") ;
	return TCL_ERROR ;
    }

    /*
     * Make sure the list is reasonable first.
     */
    if (Tcl_ListObjGetElements(interp, objv[2], &elemc, &elemv) != TCL_OK) {
	return TCL_ERROR ;
    }

    /*
     * Build the single attribute heading.
     */
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptFromlist) ;
    attr = Ral_AttributeNewFromObjs(interp, objv[3], objv[4], &errInfo) ;
    if (attr == NULL) {
        Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    heading = Ral_TupleHeadingNew(1) ;
    iter = Ral_TupleHeadingPushBack(heading, attr) ;
    assert(iter != Ral_TupleHeadingEnd(heading)) ;
    (void)iter ;

    relation = Ral_RelationNew(heading) ;
    /*
     * Iterate through the list, inserting the list elements into
     * the relation.
     * N.B. that we are ignoring duplicates, effectively converting the
     * list into a set.
     */
    for ( ; elemc > 0 ; --elemc, ++elemv) {
        Ral_Tuple tuple ;

        tuple = Ral_TupleNew(relation->heading) ;
	if (!Ral_TupleUpdateAttrValue(tuple, attr->name, *elemv, &errInfo)) {
	    Ral_InterpSetError(interp, &errInfo) ;
            Ral_TupleDelete(tuple) ;
	    Ral_RelationDelete(relation) ;
	    return TCL_ERROR ;
	}
        Ral_RelationPushBack(relation, tuple, NULL) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(relation)) ;
    return TCL_OK ;
}

static int
RelationGroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation rel ;
    Ral_TupleHeading heading ;
    Tcl_Obj *newAttrObj ;
    Ral_IntVector grpAttrs ;
    const char *relAttrName ;
    int index ;
    Ral_Relation grpRel ;

    /* relation group relation newattribute ?attr1 attr2 ...? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation newattribute ?attr1 attr2 ...?") ;
	return TCL_ERROR ;
    }
    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    rel = relObj->internalRep.otherValuePtr ;
    heading = rel->heading ;

    newAttrObj = objv[3] ;
    objc -= 4 ;
    objv += 4 ;

    /*
     * Examine the attribute arguments to determine if the attributes exist and
     * build a map to use later to determine which attributes will be in the
     * new relation valued attribute and which will remain in the tuple.
     */
    grpAttrs = Ral_IntVectorNewEmpty(objc) ;
    while (objc-- > 0) {
	const char *attrName = Tcl_GetString(*objv++) ;
	int attrIndex = Ral_TupleHeadingIndexOf(heading, attrName) ;
	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptGroup,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(grpAttrs) ;
	    return TCL_ERROR ;
	}
	Ral_IntVectorSetAdd(grpAttrs, attrIndex) ;
    }
    /*
     * You may not group away all of the attributes.
     */
    if (Ral_IntVectorSize(grpAttrs) >= Ral_TupleHeadingSize(heading)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptGroup,
	    RAL_ERR_TOO_MANY_ATTRS,
	    "attempt to group all attributes in the relation") ;
	Ral_IntVectorDelete(grpAttrs) ;
	return TCL_ERROR ;
    }
    /*
     * Check if the new relation valued attribute already exists. The name
     * either must not exist in the relation or it must be in the set of
     * attributes that are to be grouped into the relation valued attribute.
     */
    relAttrName = Tcl_GetString(newAttrObj) ;
    index = Ral_TupleHeadingIndexOf(heading, relAttrName) ;
    if (index >= 0 &&
	Ral_IntVectorFind(grpAttrs, index) == Ral_IntVectorEnd(grpAttrs)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptGroup,
	    RAL_ERR_DUPLICATE_ATTR, relAttrName) ;
	Ral_IntVectorDelete(grpAttrs) ;
	return TCL_ERROR ;
    }

    grpRel = Ral_RelationGroup(rel, relAttrName, grpAttrs) ;
    if (grpRel == NULL) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptGroup,
	    RAL_ERR_BAD_VALUE, "during group operation") ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(grpRel)) ;
    return TCL_OK ;
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

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    strRep = Ral_TupleHeadingStringOf(relation->heading) ;
    resultObj = Tcl_NewStringObj(strRep, -1) ;
    ckfree(strRep) ;

    Tcl_SetObjResult(interp, resultObj) ;
    return TCL_OK ;
}

static int
RelationInsertCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_Relation newRel ;
    Ral_ErrorInfo errInfo ;

    /* relation insert relationValue ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    newRel = Ral_RelationShallowCopy(relation) ;
    Ral_RelationReserve(newRel, objc) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptInsert) ;
    while (objc-- > 0) {
	/*
	 * Enforce insert style semantics, i.e. we error out on duplicates
	 * and other inconsistencies.
	 */
	if (Ral_RelationInsertTupleValue(newRel, interp, *objv++, &errInfo)
	    != TCL_OK) {
	    Ral_RelationDelete(newRel) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRel)) ;
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
    Ral_ErrorInfo errInfo ;

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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptIntersect) ;
    intersectRel = Ral_RelationIntersect(r1, r2, &errInfo) ;
    if (intersectRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
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

	intersectRel = Ral_RelationIntersect(r1, r2, &errInfo) ;
	Ral_RelationDelete(r1) ;
	if (intersectRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
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
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptIs,
	    RAL_ERR_HEADING_NOT_EQUAL, r2Obj) ;
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

    relationObj = objv[2] ;
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

    relationObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType) != TCL_OK)
	return TCL_ERROR ;
    relation = relationObj->internalRep.otherValuePtr ;

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
	Ral_RelationCardinality(relation) != 0)) ;
    return TCL_OK ;
}

static int
RelationIssametypeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r2 ;

    /* relation issametype relation1 relation2 */
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

    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(
	Ral_TupleHeadingEqual(r1->heading, r2->heading) != 0)) ;
    return TCL_OK ;
}

static int
RelationJoinCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r2 ;
    Ral_Relation joinRel ;
    Ral_JoinMap joinMap ;
    Ral_ErrorInfo errInfo ;

    /* relation join relation1 relation2 ?-using joinAttrs relation3 ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?-using joinAttrs relation3 ... ?") ;
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
    joinMap = Ral_JoinMapNew(0, 0) ;

    objc -= 4 ;
    objv += 4 ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptJoin) ;
    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	&errInfo) != TCL_OK) {
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    joinRel = Ral_RelationJoin(r1, r2, joinMap, &errInfo) ;
    Ral_JoinMapDelete(joinMap) ;
    if (joinRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    while (objc-- > 0) {
	r1 = joinRel ;
	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;
	joinMap = Ral_JoinMapNew(0, 0) ;

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	    &errInfo) != TCL_OK) {
	    Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	joinRel = Ral_RelationJoin(r1, r2, joinMap, &errInfo) ;
	Ral_RelationDelete(r1) ;
	Ral_JoinMapDelete(joinMap) ;
	if (joinRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(joinRel)) ;
    return TCL_OK ;
}

static int
RelationListCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    /* relation list relationValue ?attrName
            ??-ascending | -descending? sortAttrList?? */
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *listObj ;
    int attrIndex ;
    Ral_IntVector sortMap ;
    Ral_IntVectorIter mapIter ;
    Tcl_Obj *sortAttrList ;
    Ral_IntVector sortAttrs  ;
    int sortDir ;

    if (objc < 3 || objc > 6) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue ?attrName "
                "? ?-ascending | -descending? sortAttrList\? ?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    if (objc == 3) {
        /*
         * If no attribute name is mentioned, then we insist that the 
         * relation be of degree 1. The resulting list will necessarily be
         * a set.
         */
	if (Ral_RelationDegree(relation) != 1) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptList,
		RAL_ERR_DEGREE_ONE, relObj) ;
	    return TCL_ERROR ;
	}
        attrIndex = 0 ;
    }  else /* objc > 3 */ {
	/*
	 * Otherwise we need to find which attribute is referenced and
	 * will return the values of that attribute in all tuples of
	 * the relation.
	 */
	const char *attrName = Tcl_GetString(objv[3]) ;
	attrIndex = Ral_TupleHeadingIndexOf(relation->heading, attrName) ;
	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptList,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
    } 
    if (objc < 5) {
        /*
         * No sorting specified.
         */
        sortMap = Ral_IntVectorNew(Ral_RelationCardinality(relation), 0) ;
        Ral_IntVectorFillConsecutive(sortMap, 0) ;
    } else /* objc >= 5 */ {
        if (objc == 5) {
            /*
             * Attribute list specified, ascending order assumed.
             */
            sortDir = 0 ;
            sortAttrList = objv[4] ;
        } else /* objc > 5 */ {
            /*
             * Both ordering keyword and attribute list given.
             */
            int index ;

            if (Tcl_GetIndexFromObjStruct(interp, objv[4], orderOptions,
                sizeof(orderOptions[0]), "ordering", 0, &index) != TCL_OK) {
                return TCL_ERROR ;
            }
            sortDir = orderOptions[index].type == SORT_DESCENDING ;
            sortAttrList = objv[5] ;
        }
        sortAttrs = Ral_TupleHeadingAttrsFromObj(relation->heading, interp,
                sortAttrList) ;
        if (sortAttrs == NULL) {
            return TCL_ERROR ;
        }
        sortMap = Ral_RelationSort(relation, sortAttrs, sortDir) ;
        Ral_IntVectorDelete(sortAttrs) ;
    }

    listObj = Tcl_NewListObj(0, NULL) ;
    for (mapIter = Ral_IntVectorBegin(sortMap) ;
            mapIter != Ral_IntVectorEnd(sortMap) ; ++mapIter) {
	Ral_Tuple tuple = *(Ral_RelationBegin(relation) + *mapIter) ;

	if (Tcl_ListObjAppendElement(interp, listObj, tuple->values[attrIndex])
	    != TCL_OK) {
	    Tcl_DecrRefCount(listObj) ;
            Ral_IntVectorDelete(sortMap) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, listObj) ;
    Ral_IntVectorDelete(sortMap) ;
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
    Ral_ErrorInfo errInfo ;

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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptMinus) ;
    diffRel = Ral_RelationMinus(r1, r2, &errInfo) ;
    if (diffRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
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

    projAttrs = Ral_TupleHeadingAttrsFromVect(relation->heading,
	interp, objc, objv) ;
    if (projAttrs == NULL) {
	return TCL_ERROR ;
    }

    projRel = Ral_RelationProject(relation, projAttrs) ;
    Ral_IntVectorDelete(projAttrs) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(projRel)) ;

    return TCL_OK ;
}

/*
 * relation rank relationValue ?-ascending | -descending? rankAttr newAttr
 *
 * Generate a new relation extented by "newAttr". The type of "newAttr" is
 * "int" and its value is the number of tuples in "relationValue" where the
 * value of "rankAttr" is <= (-descending) or >= (-ascending) than its value
 * for a given tuple. Default ranking is "-ascending".  "rankAttr" must be of
 * type "int", "double", "string", or "bytearray" so that the comparison
 * operation is well defined.
 */
static int
RelationRankCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    const char *rankAttrName ;
    Ral_TupleHeadingIter rankAttrIter ;
    Ral_Attribute rankAttr ;
    int rankAttrIndex ;
    const char *newAttrName ;
    Ral_Attribute newAttr ;
    Ral_TupleHeadingIter inserted ;
    Ral_TupleHeading newHeading ;
    Ral_Relation newRelation ;
    enum Ordering order = SORT_ASCENDING ;
    Ral_RelationIter rIter ;

    if (objc < 5 || objc > 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue ?-ascending | -descending? rankAttr newAttr") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    /*
     * Find the rank attribute and check its type.
     */
    rankAttrName = Tcl_GetString(objv[objc - 2]) ;
    rankAttrIter = Ral_TupleHeadingFind(heading, rankAttrName) ;
    if (rankAttrIter == Ral_TupleHeadingEnd(heading)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptRank,
	    RAL_ERR_UNKNOWN_ATTR, rankAttrName) ;
	return TCL_ERROR ;
    }
    rankAttr = *rankAttrIter ;
    rankAttrIndex = rankAttrIter - Ral_TupleHeadingBegin(heading) ;
    /*
     * Create the new ranked relation, extending it by the "newAttr".
     */
    newAttrName = Tcl_GetString(objv[objc - 1]) ;
    newAttr = Ral_AttributeNewTclType(newAttrName, "int") ;
    assert(newAttr != NULL) ;
    newHeading = Ral_TupleHeadingExtend(heading, 1) ;
    inserted = Ral_TupleHeadingPushBack(newHeading, newAttr) ;
    if (inserted == Ral_TupleHeadingEnd(newHeading)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptRank,
	    RAL_ERR_DUPLICATE_ATTR, rankAttrName) ;
	Ral_TupleHeadingDelete(newHeading) ;
	return TCL_ERROR ;
    }
    newRelation = Ral_RelationNew(newHeading) ;

    if (objc == 6) {
	int index ;

	if (Tcl_GetIndexFromObjStruct(interp, objv[3], orderOptions,
	    sizeof(orderOptions[0]), "ordering", 0, &index) != TCL_OK) {
	    Ral_RelationDelete(newRelation) ;
	    return TCL_ERROR ;
	}
	order = orderOptions[index].type ;
    }

    /*
     * Now we iterate through the relation and count the number of tuples
     * whose "rankAttr" is <= (-descending) or >= (-ascending).
     */
    for (rIter = Ral_RelationBegin(relation) ;
            rIter != Ral_RelationEnd(relation) ; ++rIter) {
	Ral_RelationIter tIter ;
	Ral_Tuple rankTuple = *rIter ;
	Tcl_Obj *rankObj = *(Ral_TupleBegin(rankTuple) + rankAttrIndex) ;
	int rankCount = 0 ;
	Ral_Tuple newTuple ;
	Ral_TupleIter newIter ;
	int status ;

	for (tIter = Ral_RelationBegin(relation) ;
                tIter != Ral_RelationEnd(relation) ; ++tIter) {
	    Tcl_Obj *cmpObj = *(Ral_TupleBegin(*tIter) + rankAttrIndex) ;

	    int cmp = Ral_AttributeValueCompare(rankAttr, cmpObj, rankObj) ;
	    rankCount += order == SORT_DESCENDING ? cmp > 0 : cmp < 0 ;
	}

	newTuple = Ral_TupleNew(newHeading) ;
	newIter = Ral_TupleBegin(newTuple) ;
	newIter += Ral_TupleCopyValues(Ral_TupleBegin(rankTuple),
	    Ral_TupleEnd(rankTuple), newIter) ;
	/*
	 * +1 because we are using simple > or <
	 */
	Tcl_IncrRefCount(*newIter = Tcl_NewIntObj(rankCount + 1)) ;
	/*
	 * Should always be able to insert the new tuple.
	 */
	status = Ral_RelationPushBack(newRelation, newTuple, NULL) ;
	assert(status != 0) ;
        (void)status ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
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
    Ral_ErrorInfo errInfo ;

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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptRename) ;

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_BAD_PAIRS_LIST,
	    "oldname / newname arguments must be given in pairs") ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    newRelation = Ral_RelationDup(relation) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	if (!Ral_RelationRenameAttribute(newRelation, Tcl_GetString(objv[0]),
	    Tcl_GetString(objv[1]), &errInfo)) {
	    Ral_InterpSetError(interp, &errInfo) ;
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
            (void)inserted ;
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
RelationRestrictWithCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter thBegin ;
    Ral_TupleHeadingIter thEnd ;
    Ral_TupleHeadingIter thIter ;
    Tcl_Obj *exprObj ;
    Ral_Relation newRelation ;
    Ral_RelationIter iter ;
    Ral_RelationIter end ;

    /* relation restrictwith relValue expr */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relValue expr") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;
    thBegin = Ral_TupleHeadingBegin(heading) ;
    thEnd = Ral_TupleHeadingEnd(heading) ;

    exprObj = objv[3] ;

    newRelation = Ral_RelationNew(relation->heading) ;

    Tcl_IncrRefCount(exprObj) ;

    end = Ral_RelationEnd(relation) ;
    for (iter = Ral_RelationBegin(relation) ; iter != end ; ++iter) {
	Ral_Tuple tuple = *iter ;
	Ral_TupleIter tupEnd = Ral_TupleEnd(tuple) ;
	Ral_TupleIter tupIter ;
	int boolValue ;

	thIter = thBegin ;
	for (tupIter = Ral_TupleBegin(tuple) ; tupIter != tupEnd ; ++tupIter) {
	    Tcl_Obj *attrValueObj = *tupIter ;
	    Ral_Attribute attr = *thIter++ ;

	    if (Tcl_SetVar2Ex(interp, attr->name, NULL, attrValueObj,
		TCL_LEAVE_ERR_MSG) == NULL) {
		goto errorOut ;
	    }
	}

	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    goto errorOut ;
	}
	if (boolValue) {
	    int inserted ;
	    inserted = Ral_RelationPushBack(newRelation, tuple, NULL) ;
	    assert(inserted != 0) ;
            (void)inserted ;
	}
    }

    for (thIter = thBegin ; thIter != thEnd ; ++thIter) {
	Ral_Attribute attr = *thIter ;
	Tcl_UnsetVar(interp, attr->name, 0) ;
    }
    Tcl_DecrRefCount(exprObj) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
    return TCL_OK ;

errorOut:
    for (thIter = thBegin ; thIter != thEnd ; ++thIter) {
	Ral_Attribute attr = *thIter ;
	Tcl_UnsetVar(interp, attr->name, 0) ;
    }
    Tcl_DecrRefCount(exprObj) ;
    Ral_RelationDelete(newRelation) ;
    return TCL_ERROR ;
}

static int
RelationSemijoinCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r2 ;
    Ral_Relation semiJoinRel ;
    Ral_JoinMap joinMap ;
    Ral_ErrorInfo errInfo ;

    /*
     * relation semijoin relation1 relation2 ?-using joinAttrs relation3 ... ?
     */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?-using joinAttrs?") ;
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
    joinMap = Ral_JoinMapNew(0, 0) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptSemijoin) ;

    objc -= 4 ;
    objv += 4 ;

    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	&errInfo) != TCL_OK) {
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    semiJoinRel = Ral_RelationSemiJoin(r1, r2, joinMap) ;
    assert(semiJoinRel != NULL) ;
    Ral_JoinMapDelete(joinMap) ;

    while (objc-- > 0) {
	r1 = semiJoinRel ;
	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;
	joinMap = Ral_JoinMapNew(0, 0) ;

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	    &errInfo) != TCL_OK) {
	    Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	semiJoinRel = Ral_RelationSemiJoin(r1, r2, joinMap) ;
	assert(semiJoinRel != NULL) ;
	Ral_RelationDelete(r1) ;
	Ral_JoinMapDelete(joinMap) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(semiJoinRel)) ;
    return TCL_OK ;
}

static int
RelationSemiminusCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *r1Obj ;
    Ral_Relation r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r2 ;
    Ral_Relation semiMinusRel ;
    Ral_JoinMap joinMap ;
    Ral_ErrorInfo errInfo ;

    /*
     * relation semiminus relation1 relation2 ?-using joinAttrs relation3 ... ?
     */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?-using joinAttrs?") ;
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
    joinMap = Ral_JoinMapNew(0, 0) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptSemiminus) ;

    objc -= 4 ;
    objv += 4 ;

    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	&errInfo) != TCL_OK) {
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    semiMinusRel = Ral_RelationSemiMinus(r1, r2, joinMap) ;
    assert(semiMinusRel != NULL) ;
    Ral_JoinMapDelete(joinMap) ;

    while (objc-- > 0) {
	r1 = semiMinusRel ;
	r2Obj = *objv++ ;
	if (Tcl_ConvertToType(interp, r2Obj, &Ral_RelationObjType) != TCL_OK) {
	    Ral_RelationDelete(r1) ;
	    return TCL_ERROR ;
	}
	r2 = r2Obj->internalRep.otherValuePtr ;
	joinMap = Ral_JoinMapNew(0, 0) ;

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap,
	    &errInfo) != TCL_OK) {
	    Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	semiMinusRel = Ral_RelationSemiMinus(r1, r2, joinMap) ;
	assert(semiMinusRel != NULL) ;
	Ral_RelationDelete(r1) ;
	Ral_JoinMapDelete(joinMap) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(semiMinusRel)) ;
    return TCL_OK ;
}

static int
RelationSummarizeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    Tcl_Obj *perObj ;
    Ral_Relation perRelation ;
    Ral_TupleHeading perHeading ;
    Tcl_Obj *varNameObj ;
    Ral_Relation sumRelation ;
    Ral_TupleHeading sumHeading ;
    int c ;
    Tcl_Obj *const*v ;
    Ral_JoinMap joinMap ;
    int index = 0 ;
    Ral_RelationIter perIter ;
    Ral_TupleHeadingIter sumHeadingIter ;
    Ral_ErrorInfo errInfo ;
    /*
     * relation summarize relationValue perRelation relationVarName
     * ?attr1 type1 expr1 ... attrN typeN exprN?
     */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue perRelation relationVarName"
	    " ?attr1 type1 expr1 ... attrN typeN exprN?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    perObj = objv[3] ;
    if (Tcl_ConvertToType(interp, perObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    perRelation = perObj->internalRep.otherValuePtr ;
    perHeading = perRelation->heading ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptSummarize) ;

    /*
     * Create a join map so that we can find the tuples that match between the
     * relation and the per-relation.  The "per" relation must be a subset of
     * the summarized relation. We will know that because the common attributes
     * must encompass all those in the per relation.
     */
    joinMap = Ral_JoinMapNew(Ral_TupleHeadingSize(perHeading),
	Ral_RelationCardinality(relation)) ;
    c = Ral_TupleHeadingCommonAttributes(perHeading, heading, joinMap) ;
    if (c != Ral_TupleHeadingSize(perHeading)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_NOT_A_PROJECTION, perObj) ;
	Ral_InterpSetError(interp, &errInfo) ;
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }
    varNameObj = objv[4] ;

    objc -= 5 ;
    objv += 5 ;
    if (objc % 3 != 0) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_BAD_TRIPLE_LIST,
	    "attribute / type / expression arguments "
	    "must be given in triples") ;
	Ral_InterpSetError(interp, &errInfo) ;
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    /*
     * Construct the heading for the result. It the heading of the
     * "per" relation plus the summary attributes.
     */
    sumHeading = Ral_TupleHeadingExtend(perHeading, objc / 3) ;
    /*
     * Add in the summary attributes to tuple heading.
     */
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1),
	    &errInfo) ;
	Ral_TupleHeadingIter inserted ;

	if (attr == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(sumHeading) ;
            Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	inserted = Ral_TupleHeadingPushBack(sumHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(sumHeading)) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR, *v) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(sumHeading) ;
            Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
    }
    sumRelation = Ral_RelationNew(sumHeading) ;

    /*
     * The strategy is to iterate over each tuple in the per-relation and to
     * construct subsets of the relation that match the tuple. That subset
     * relation is then assigned to the variable name and each summary
     * attribute is computed by evaluating the expression and assigning the
     * result to the attribute.
     */
    Tcl_IncrRefCount(varNameObj) ;
    Ral_RelationFindJoinTuples(perRelation, relation, joinMap) ;
    sumHeadingIter = Ral_TupleHeadingBegin(sumHeading) +
	Ral_RelationDegree(perRelation) ;
    for (perIter = Ral_RelationBegin(perRelation) ;
            perIter != Ral_RelationEnd(perRelation) ; ++perIter) {
	Ral_Tuple perTuple = *perIter ;
	Ral_IntVector matchSet = Ral_JoinMapMatchingTupleSet(joinMap, 0,
	    index++) ;
	Ral_Relation matchRel = Ral_RelationExtract(relation, matchSet) ;
	Tcl_Obj *matchObj = Ral_RelationObjNew(matchRel) ;
	Ral_Tuple sumTuple = Ral_TupleNew(sumHeading) ;
	Ral_TupleIter sumIter = Ral_TupleBegin(sumTuple) ;
	Ral_TupleHeadingIter attrIter = sumHeadingIter ;
	int status ;

	Ral_IntVectorDelete(matchSet) ;

	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, matchObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Ral_TupleDelete(sumTuple) ;
	    Tcl_DecrRefCount(matchObj) ;
	    goto errorOut ;
	}

	sumIter += Ral_TupleCopyValues(Ral_TupleBegin(perTuple),
	    Ral_TupleEnd(perTuple), sumIter) ;

	for (c = objc, v = objv + 2 ; c > 0 ; c -= 3, v += 3) {
	    Tcl_Obj *exprResult ;
            Tcl_Obj *cvtResult ;

	    if (Tcl_ExprObj(interp, *v, &exprResult) != TCL_OK) {
		Ral_TupleDelete(sumTuple) ;
		goto errorOut ;
	    }
	    cvtResult = Ral_AttributeConvertValueToType(interp, *attrIter++,
		exprResult, &errInfo) ;
	    if (cvtResult == NULL) {
		Tcl_DecrRefCount(exprResult) ;
		Ral_TupleDelete(sumTuple) ;
		goto errorOut ;
	    }
            Tcl_IncrRefCount(cvtResult) ;
            Tcl_DecrRefCount(exprResult) ;
            *sumIter++ = cvtResult ;
	}
	status = Ral_RelationPushBack(sumRelation, sumTuple, NULL) ;
	assert(status != 0) ;
        (void)status ;
    }

    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(sumRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Ral_RelationDelete(sumRelation) ;
    return TCL_ERROR ;
}

static int
RelationSummarizebyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    Tcl_Obj *varNameObj ;
    Ral_Relation sumRelation ;
    Ral_TupleHeading sumHeading ;
    int elemc ;
    Tcl_Obj **elemv ;
    int c ;
    Tcl_Obj *const*v ;
    Ral_JoinMap joinMap ;
    Ral_IntVector perAttrs ;
    Ral_Relation perRelation ;
    int index = 0 ;
    Ral_RelationIter perIter ;
    Ral_TupleHeadingIter sumHeadingIter ;
    Ral_ErrorInfo errInfo ;
    /*
     * relation summarizeby relationValue attrList relationVarName
     * ?attr1 type1 expr1 ... attrN typeN exprN?
     */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue perRelation relationVarName"
	    " ?attr1 type1 expr1 ... attrN typeN exprN?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    if (Tcl_ListObjGetElements(interp, objv[3], &elemc, &elemv) != TCL_OK) {
        return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptSummarizeby) ;

    varNameObj = objv[4] ;
    objc -= 5 ;
    objv += 5 ;
    if (objc % 3 != 0) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_BAD_TRIPLE_LIST,
	    "attribute / type / expression arguments "
	    "must be given in triples") ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    /*
     * Iterate through the given attributes. We need to create a heading for
     * the result and we can do that as we iterate through the attributes.
     * Create a join map that we will use to find the sub relations for the
     * summarization.
     */
    sumHeading = Ral_TupleHeadingNew(elemc + objc / 3) ;
    joinMap = Ral_JoinMapNew(elemc, Ral_RelationCardinality(relation)) ;
    for (c = 0, v = elemv ; c < elemc ; ++c, ++v) {
        Ral_TupleHeadingIter attrIter ;

	attrIter = Ral_TupleHeadingFind(heading, Tcl_GetString(*v)) ;
        if (attrIter == Ral_TupleHeadingEnd(heading)) {
            Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_UNKNOWN_ATTR, *v) ;
            Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(sumHeading) ;
            Ral_JoinMapDelete(joinMap) ;
            return TCL_ERROR ;
        }
	if (!Ral_TupleHeadingAppend(heading, attrIter, attrIter + 1,
                sumHeading)) {
            Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR, *v) ;
            Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(sumHeading) ;
            Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
        Ral_JoinMapAddAttrMapping(joinMap, c,
                attrIter - Ral_TupleHeadingBegin(heading)) ;
    }

    /*
     * Add in the summary attributes to result tuple heading.
     */
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr ;
	Ral_TupleHeadingIter inserted ;

	attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1), &errInfo) ;
	if (attr == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(sumHeading) ;
            Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	inserted = Ral_TupleHeadingPushBack(sumHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(sumHeading)) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR, *v) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(sumHeading) ;
            Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
    }
    sumRelation = Ral_RelationNew(sumHeading) ;
    /*
     * Compute the "per" relation as the projection of the input
     * relation across the given attributes.
     */
    perAttrs = Ral_JoinMapGetAttr(joinMap, 1) ;
    perRelation = Ral_RelationProject(relation, perAttrs) ;
    Ral_IntVectorDelete(perAttrs) ;
    /*
     * The strategy is to iterate over each tuple in the per-relation and to
     * construct subsets of the relation that match the tuple. That subset
     * relation is then assigned to the variable name and each summary
     * attribute is computed by evaluating the expression and assigning the
     * result to the attribute.
     */
    Tcl_IncrRefCount(varNameObj) ;
    Ral_RelationFindJoinTuples(perRelation, relation, joinMap) ;
    sumHeadingIter = Ral_TupleHeadingBegin(sumHeading) +
            Ral_RelationDegree(perRelation) ;
    for (perIter = Ral_RelationBegin(perRelation) ;
            perIter != Ral_RelationEnd(perRelation) ; ++perIter) {
	Ral_Tuple perTuple = *perIter ;
	Ral_IntVector matchSet = Ral_JoinMapMatchingTupleSet(joinMap, 0,
	    index++) ;
	Ral_Relation matchRel = Ral_RelationExtract(relation, matchSet) ;
	Tcl_Obj *matchObj = Ral_RelationObjNew(matchRel) ;
	Ral_Tuple sumTuple = Ral_TupleNew(sumHeading) ;
	Ral_TupleIter sumIter = Ral_TupleBegin(sumTuple) ;
	Ral_TupleHeadingIter attrIter = sumHeadingIter ;
	int status ;

	Ral_IntVectorDelete(matchSet) ;

	if (Tcl_ObjSetVar2(interp, varNameObj, NULL, matchObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Ral_TupleDelete(sumTuple) ;
	    Tcl_DecrRefCount(matchObj) ;
	    goto errorOut ;
	}

	sumIter += Ral_TupleCopyValues(Ral_TupleBegin(perTuple),
	    Ral_TupleEnd(perTuple), sumIter) ;

	for (c = objc, v = objv + 2 ; c > 0 ; c -= 3, v += 3) {
	    Tcl_Obj *exprResult ;
            Tcl_Obj *cvtResult ;

	    if (Tcl_ExprObj(interp, *v, &exprResult) != TCL_OK) {
		Ral_TupleDelete(sumTuple) ;
		goto errorOut ;
	    }
	    cvtResult = Ral_AttributeConvertValueToType(interp, *attrIter++,
		exprResult, &errInfo) ;
	    if (cvtResult == NULL) {
		Tcl_DecrRefCount(exprResult) ;
		Ral_TupleDelete(sumTuple) ;
		goto errorOut ;
	    }
            Tcl_IncrRefCount(cvtResult) ;
            Tcl_DecrRefCount(exprResult) ;
            *sumIter++ = cvtResult ;
	}
	status = Ral_RelationPushBack(sumRelation, sumTuple, NULL) ;
	assert(status != 0) ;
        (void)status ;
    }

    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Ral_RelationDelete(perRelation) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(sumRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Ral_RelationDelete(perRelation) ;
    Ral_RelationDelete(sumRelation) ;
    return TCL_ERROR ;
}

/*
 * relation table heading ?value-list1 value-list2 ...?
 *
 * Create a relation using a tabular input form. <heading> is an attribute
 * name / type list. The <value-listN> arguments are lists of attribute
 * values only and are presumed to be in the same order as the <heading>.
 * This command is meant to save re-typing the attributes names over and
 * over when trying to create literal relation values.
 */
static int
RelationTableCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_TupleHeading heading ;
    Ral_ErrorInfo errInfo ;
    Ral_Relation relation ;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
                "heading ?value-list1 value-list2 ...?") ;
	return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptTable) ;
    /*
     * Create the relation heading from the arguments.
     */
    heading = Ral_TupleHeadingNewFromObj(interp, objv[2], &errInfo) ;
    if (heading == NULL) {
	return TCL_ERROR ;
    }
    relation = Ral_RelationNew(heading) ;

    /*
     * Offset the argument bookkeeping to reference the value lists.
     */
    objc -= 3 ;
    objv += 3 ;
    Ral_RelationReserve(relation, objc) ;
    /*
     * Iterate through the value list arguments, inserting them into the
     * relation. Values correspond to the same order as the heading.  Here
     * duplicates matter as we deem creation to have "insert" semantics.
     */
    for ( ; objc > 0 ; --objc, ++objv) {
        Ral_Tuple tuple ;

        tuple = Ral_TupleNew(heading) ;
        if (Ral_TupleSetFromValueList(tuple, interp, *objv, &errInfo)
                != TCL_OK) {
            Ral_TupleDelete(tuple) ;
            Ral_RelationDelete(relation) ;
            return TCL_ERROR ;
        }

        if (!Ral_RelationPushBack(relation, tuple, NULL)) {
            Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_TUPLE, *objv) ;
            Ral_InterpSetError(interp, &errInfo) ;
            Ral_RelationDelete(relation) ;
            return TCL_ERROR ;
        }
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(relation)) ;
    return TCL_OK ;
}

/*
 * relation tag relation attrName ?-ascending | -descending sort-attr-list?
 *	?-within {attr-list} -start tag-value?
 *
 * Create a new relation extended by "attrName". "attrName" will be "int" type
 * and will consist of consecutive integers starting at zero or "tag-value" if
 * the "-start" option is given. The tuples in "relation" will be extended in
 * the order implied by ascending or descending order of "sort-attr-list" if
 * the option is given. If absent the order is arbitrary.  If the "-within"
 * option is given then "attrName" values will be unique within the attributes
 * in "attr-list".
 */
static int
RelationTagCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    char const *attrName ;
    Ral_Relation tagRelation ;
    Ral_IntVector sortMap = NULL ;
    Ral_IntVector withinAttrs = NULL ;
    int tagStart = 0 ;
    Ral_ErrorInfo errInfo ;
    int result = TCL_ERROR ;

    static int gotOpt[4] ;

    if (objc < 4 || objc > 10) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation attrName "
	    "?-ascending | -descending sort-attr-list? ?-within attr-list "
            "-start tag-value?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptTag) ;
    /*
     * The attribute name is always the fourth argument.
     */
    attrName = Tcl_GetString(objv[3]) ;
    /*
     * Parse the remaining arguments.
     */
    memset(gotOpt, 0, sizeof(gotOpt)) ;
    objc -= 4 ;
    objv += 4 ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	static const struct {
	    const char *optName ;
	    enum {
		TAG_ASCENDING,
		TAG_DESCENDING,
		TAG_WITHIN,
                TAG_START
	    } opt ;
	} optTable[] = {
	    {"-ascending", TAG_ASCENDING},
	    {"-descending", TAG_DESCENDING},
	    {"-within", TAG_WITHIN},
            {"-start", TAG_START},
	    {NULL}
	} ;

	Ral_IntVector sortAttrs ;
	int index ;

	if (Tcl_GetIndexFromObjStruct(interp, objv[0], optTable,
	    sizeof(optTable[0]), "tag option", 0, &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	/*
	 * Make sure that duplicated options are not present, e.g. no
	 * "-ascending a1 -descending b1" or "-within a1 -within b1".
	 */
        if (gotOpt[index]) {
            Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_OPTION,
                    objv[0]) ;
            Ral_InterpSetError(interp, &errInfo) ;
	    goto errorOut ;
	}

	switch (optTable[index].opt) {
	case TAG_ASCENDING:
            gotOpt[index] = gotOpt[TAG_DESCENDING] = 1 ;
	    sortAttrs = Ral_TupleHeadingAttrsFromObj(relation->heading, interp,
                    objv[1]) ;
	    if (sortAttrs == NULL) {
                goto errorOut ;
	    }
	    sortMap = Ral_RelationSort(relation, sortAttrs, 0) ;
	    Ral_IntVectorDelete(sortAttrs) ;
	    break;

	case TAG_DESCENDING:
            gotOpt[index] = gotOpt[TAG_ASCENDING] = 1 ;
	    sortAttrs = Ral_TupleHeadingAttrsFromObj(relation->heading, interp,
                    objv[1]) ;
	    if (sortAttrs == NULL) {
                goto errorOut ;
	    }
	    sortMap = Ral_RelationSort(relation, sortAttrs, 1) ;
	    Ral_IntVectorDelete(sortAttrs) ;
	    break ;

	case TAG_WITHIN:
            gotOpt[index] = 1 ;
	    withinAttrs = Ral_TupleHeadingAttrsFromObj(relation->heading,
                    interp, objv[1]) ;
            if (withinAttrs == NULL) {
                goto errorOut ;
            }
	    break ;

        case TAG_START:
            gotOpt[index] = 1 ;
            if (Tcl_GetIntFromObj(interp, objv[1], &tagStart) != TCL_OK) {
                goto errorOut ;
            }
	    break ;

	default:
	    Tcl_Panic("Ral_TagCmd: unknown option, \"%d\"",
		optTable[index].opt) ;
	}
    }
    /*
     * At this point, "sortMap" tells if we are sorting. If we are not sorting
     * then just build an identity mapping.
     */
    if (sortMap == NULL) {
	sortMap = Ral_IntVectorNew(Ral_RelationCardinality(relation), 0) ;
	Ral_IntVectorFillConsecutive(sortMap, 0) ;
    }
    tagRelation = withinAttrs == NULL ?
            Ral_RelationTag(relation, attrName, sortMap, tagStart,
                    &errInfo) :
            Ral_RelationTagWithin(relation, attrName, sortMap, withinAttrs,
                    tagStart, &errInfo) ;

    if (tagRelation) {
        Tcl_SetObjResult(interp, Ral_RelationObjNew(tagRelation)) ;
        result = TCL_OK ;
    } else {
	Ral_InterpSetError(interp, &errInfo) ;
    }

errorOut:
    if (sortMap) {
        Ral_IntVectorDelete(sortMap) ;
    }
    if (withinAttrs) {
	Ral_IntVectorDelete(withinAttrs) ;
    }
    return result ;
}

static int
RelationTcloseCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    Ral_TupleHeadingIter thIter ;

    /* relation tclose relation */
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    if (Ral_RelationDegree(relation) != 2) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTclose,
	    RAL_ERR_DEGREE_TWO, relObj) ;
	return TCL_ERROR ;
    }

    thIter = Ral_TupleHeadingBegin(heading) ;
    if (!Ral_AttributeTypeEqual(*thIter, *(thIter + 1))) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTclose,
	    RAL_ERR_TYPE_MISMATCH, relObj) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(Ral_RelationTclose(relation))) ;
    return TCL_OK ;
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

    prodRel = Ral_RelationTimes(r1, r2) ;
    if (prodRel == NULL) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTimes,
	    RAL_ERR_DUPLICATE_ATTR, r2Obj) ;
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
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTimes,
		RAL_ERR_DUPLICATE_ATTR, r2Obj) ;
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
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTuple,
	    RAL_ERR_CARDINALITY_ONE, relObj) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_TupleObjNew(
	Ral_TupleDup(*Ral_RelationBegin(relation)))) ;
    return TCL_OK ;
}

static int
RelationUinsertCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_Relation newRel ;
    Ral_ErrorInfo errInfo ;

    /* relation sinsert relationValue ?name-value-list ...? */
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue ?name-value-list ...?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    objc -= 3 ;
    objv += 3 ;

    newRel = Ral_RelationShallowCopy(relation) ;
    Ral_RelationReserve(newRel, objc) ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptUinsert) ;
    while (objc-- > 0) {
	/*
	 * uinsert silently ignores duplicates and has "union" like semantics.
	 */
	Ral_RelationInsertTupleValue(newRel, interp, *objv++, &errInfo) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRel)) ;
    return TCL_OK ;
}

static int
RelationUngroupCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *attrObj ;
    const char *attrName ;
    Ral_Relation ungrpRel ;
    Ral_ErrorInfo errInfo ;

    /* relation ungroup relation attribute */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation attribute") ;
	return TCL_ERROR ;
    }
    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    attrObj = objv[3] ;
    attrName = Tcl_GetString(attrObj) ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptUngroup) ;
    ungrpRel = Ral_RelationUngroup(relation, attrName, &errInfo) ;
    if (ungrpRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(ungrpRel)) ;
    return TCL_OK ;
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
    Ral_ErrorInfo errInfo ;

    /* relation union relation1 relation2 ? ... ? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation1 relation2 ?relation3 ...?") ;
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

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptUnion) ;
    unionRel = Ral_RelationUnion(r1, r2, 0, &errInfo) ;
    if (unionRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
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

	unionRel = Ral_RelationUnion(r1, r2, 0, &errInfo) ;
	Ral_RelationDelete(r1) ;
	if (unionRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(unionRel)) ;
    return TCL_OK ;
}

static int
RelationUnwrapCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *attrObj ;
    const char *attrName ;
    Ral_Relation unwrapRel ;
    Ral_ErrorInfo errInfo ;

    /* relation unwrap relationValue attribute */
    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue attribute") ;
	return TCL_ERROR ;
    }
    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    attrObj = objv[3] ;
    attrName = Tcl_GetString(attrObj) ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptUnwrap) ;
    unwrapRel = Ral_RelationUnwrap(relation, attrName, &errInfo) ;
    if (unwrapRel == NULL) {
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(unwrapRel)) ;
    return TCL_OK ;
}

static int
RelationUpdateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *tupleVarNameObj ;
    Tcl_Obj *exprObj ;
    Tcl_Obj *scriptObj ;
    Ral_Relation updatedRel ;
    Ral_RelationIter rIter ;
    int result = TCL_OK ;

    /* relation update relationValue tupleVarName expr script */
    if (objc != 6) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relationValue tupleVarName expr script") ;
	return TCL_ERROR ;
    }
    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    Tcl_IncrRefCount(tupleVarNameObj = objv[3]) ;
    Tcl_IncrRefCount(exprObj = objv[4]) ;
    Tcl_IncrRefCount(scriptObj = objv[5]) ;

    updatedRel = Ral_RelationNew(relation->heading) ;

    for (rIter = Ral_RelationBegin(relation) ;
            rIter != Ral_RelationEnd(relation) ; ++rIter) {
	Tcl_Obj *oldTupleObj ;
	Tcl_Obj *newTupleObj ;
        Ral_Tuple newTuple ;
	int boolValue ;
        Ral_IntVector orderMap ;
        int status ;

	/*
	 * Place the tuple into a Tcl object and store it into the
	 * given variable.
	 */
	oldTupleObj = Ral_TupleObjNew(*rIter) ;
        Tcl_IncrRefCount(oldTupleObj) ;
	if (Tcl_ObjSetVar2(interp, tupleVarNameObj, NULL, oldTupleObj,
	    TCL_LEAVE_ERR_MSG) == NULL) {
	    Tcl_DecrRefCount(oldTupleObj) ;
	    result = TCL_ERROR ;
	    break ;
	}
	/*
	 * Evaluate the expression to see if this is a tuple that is
	 * to be updated.
	 */
	if (Tcl_ExprBooleanObj(interp, exprObj, &boolValue) != TCL_OK) {
	    result = TCL_ERROR ;
            Tcl_DecrRefCount(oldTupleObj) ;
	    break ;
	}
	if (boolValue) {
            /*
             * Evaluate the script and replace the tuple in the relation
             * with the return value of the script.
             */
            /*
             * Evaluate the script.
             */
            result = Tcl_EvalObjEx(interp, scriptObj, 0) ;
            if (result == TCL_ERROR) {
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
                Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                    "\n    (\"in ::ral::relation update\" body line %d)",
#                   if TCL_MINOR_VERSION < 6
                    interp->errorLine
#                   else
                    Tcl_GetErrorLine(interp)
#                   endif
                )) ;
#endif
                break ;
            } else if (result == TCL_RETURN) {
                result = TCL_OK ;
            } else if (result != TCL_OK) {
                /*
                 * TCL_CONTINUE or TCL_BREAK or custom codes are not allowed.
                 */
                switch (result) {
                case TCL_BREAK:
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                            "invoked \"break\" outside of a loop", -1)) ;
                    break ;

                case TCL_CONTINUE:
                    Tcl_SetObjResult(interp, Tcl_NewStringObj(
                            "invoked \"continue\" outside of a loop", -1)) ;
                    break ;

                default:
                    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
                            "unknown return code %d", -1)) ;
                    break ;
                }
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
                Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                    "\n    (\"in ::ral::relation update\" body line %d)",
#                   if TCL_MINOR_VERSION < 6
                    interp->errorLine
#                   else
                    Tcl_GetErrorLine(interp)
#                   endif
                )) ;
#endif
                result = TCL_ERROR ;
                break ;
            }
            /*
             * Fetch the return value of the script.  Once we get the new tuple
             * value, we can use it to update the relvar.
             */
            newTupleObj = Tcl_GetObjResult(interp) ;
            if (Tcl_ConvertToType(interp, newTupleObj, &Ral_TupleObjType)
                    != TCL_OK) {
                result = TCL_ERROR ;
                break ;
            }
            assert(newTupleObj->typePtr == &Ral_TupleObjType) ;
            newTuple = newTupleObj->internalRep.otherValuePtr ;
            if (!Ral_TupleHeadingEqual(relation->heading, newTuple->heading)) {
                Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptUpdate,
                        RAL_ERR_HEADING_NOT_EQUAL, newTupleObj) ;
                result = TCL_ERROR ;
                break ;
            }
        } else {
            newTuple = *rIter ;
        }
        orderMap = Ral_TupleHeadingNewOrderMap(updatedRel->heading,
                newTuple->heading) ;
        status = Ral_RelationPushBack(updatedRel, newTuple, orderMap) ;
        assert(status != 0) ;
        (void)status ;

        Ral_IntVectorDelete(orderMap) ;
        Tcl_DecrRefCount(oldTupleObj) ;
    }

    Tcl_UnsetVar(interp, Tcl_GetString(tupleVarNameObj), 0) ;
    Tcl_DecrRefCount(scriptObj) ;
    Tcl_DecrRefCount(tupleVarNameObj) ;
    Tcl_DecrRefCount(exprObj) ;

    if (result == TCL_OK) {
	Tcl_SetObjResult(interp, Ral_RelationObjNew(updatedRel)) ;
    } else {
	Ral_RelationDelete(updatedRel) ;
    }

    return result ;
}

static int
RelationWrapCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relationObj ;
    Tcl_Obj *newAttrNameObj ;
    Ral_Relation relation ;
    Ral_TupleHeading heading ;
    Ral_Relation resultRelation ;
    Ral_TupleHeading resultHeading ;
    Ral_TupleHeading wrapHeading ;
    Ral_IntVector wattrs ;
    Ral_IntVector rattrs ;
    Ral_IntVectorIter iviter ;
    int i ;
    Ral_Attribute newAttr ;
    Ral_TupleHeadingIter newAttrIter ;
    Ral_RelationIter riter ;
    Ral_ErrorInfo errInfo ;

    /* relation wrap relationValue newAttr ?attr1 attr2 ...? */
    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv,
                "relationValue newAttr ?attr attr2 ...?") ;
	return TCL_ERROR ;
    }

    relationObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relationObj, &Ral_RelationObjType)
            != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relationObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    newAttrNameObj = objv[3] ;

    objc -= 4 ;
    objv += 4 ;
    /*
     * Compose the heading for the wrapped attribute.  The wrapped tuple
     * valued attribute will have the same number of attributes as the
     * remaining arguments.  Accumulate a vector of the attribute indices going
     * into the tuple valued attribute.
     */
    wrapHeading = Ral_TupleHeadingNew(objc) ;
    wattrs = Ral_IntVectorNewEmpty(objc) ;
    for (i = 0 ; i < objc ; ++i) {
	char const *attrName ;
        Ral_TupleHeadingIter attrIter ;

	attrName = Tcl_GetString(objv[i]) ;
	attrIter = Ral_TupleHeadingFind(heading, attrName) ;
	if (attrIter == Ral_TupleHeadingEnd(heading)) {
	    Ral_TupleHeadingDelete(wrapHeading) ;
            Ral_IntVectorDelete(wattrs) ;
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptUnwrap,
                    RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
	if (!Ral_TupleHeadingAppend(heading, attrIter, attrIter + 1,
                wrapHeading)) {
	    Ral_TupleHeadingDelete(wrapHeading) ;
            Ral_IntVectorDelete(wattrs) ;
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptUnwrap,
                    RAL_ERR_DUPLICATE_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
        Ral_IntVectorPushBack(wattrs,
                attrIter - Ral_TupleHeadingBegin(heading)) ;
    }
    /*
     * Compose the heading for the result relation.  The result relation has
     * the same number of attributes as the input relation minus the number
     * that are to be wrapped into the tuple valued attribute plus one for the
     * new tuple valued attribute itself.
     */
    resultHeading = Ral_TupleHeadingNew(Ral_TupleHeadingSize(heading) -
            objc + 1) ;
    /*
     * The complement of the set of attributes in the tuple valued attribute
     * is the set that form the heading for the result.
     */
    rattrs = Ral_IntVectorSetComplement(wattrs, Ral_TupleHeadingSize(heading)) ;
    for (iviter = Ral_IntVectorBegin(rattrs) ;
            iviter != Ral_IntVectorEnd(rattrs) ; ++iviter) {
        int status ;
        Ral_TupleHeadingIter attrIter ;

        assert(*iviter < Ral_TupleHeadingSize(heading)) ;
        attrIter = Ral_TupleHeadingBegin(heading) + *iviter ;

        status = Ral_TupleHeadingAppend(heading, attrIter, attrIter + 1,
                resultHeading) ;
        assert(status != 0) ;
        (void)status ;
    }
    /*
     * Add the tuple valued attribute to the result relation heading.
     */
    newAttr = Ral_AttributeNewTupleType(Tcl_GetString(newAttrNameObj),
            wrapHeading) ;
    newAttrIter = Ral_TupleHeadingPushBack(resultHeading, newAttr) ;
    if (newAttrIter == Ral_TupleHeadingEnd(resultHeading)) {
	Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR,
                newAttrNameObj) ;
	Ral_InterpSetError(interp, &errInfo) ;
        Ral_AttributeDelete(newAttr) ;
        Ral_TupleHeadingDelete(resultHeading) ;
        Ral_TupleHeadingDelete(wrapHeading) ;
        Ral_IntVectorDelete(wattrs) ;
        Ral_IntVectorDelete(rattrs) ;
        return TCL_ERROR ;
    }
    /*
     * Iterate through the original relation, building the tuples for the
     * result. There will be one tuple in the result for each tuple
     * in the input relation.
     */
    resultRelation = Ral_RelationNew(resultHeading) ;
    Ral_RelationReserve(resultRelation, Ral_RelationCardinality(relation)) ;
    for (riter = Ral_RelationBegin(relation) ;
            riter != Ral_RelationEnd(relation) ; ++riter) {
        Ral_Tuple tuple ;
        Ral_Tuple resultTuple ;
        Ral_TupleIter resultDst ;
        Ral_Tuple wrapTuple ;
        Ral_TupleIter wrapDst ;

        tuple = *riter ;
        resultTuple = Ral_TupleNew(resultHeading) ;
        resultDst = Ral_TupleBegin(resultTuple) ;
        /*
         * Copy the values from the tuple in the input relation to the
         * new tuple for the result.
         */
        for (iviter = Ral_IntVectorBegin(rattrs) ;
                iviter != Ral_IntVectorEnd(rattrs) ; ++iviter) {
            Ral_TupleIter src ;

            assert(*iviter < Ral_TupleDegree(tuple)) ;
            src = Ral_TupleBegin(tuple) + *iviter ;
            resultDst += Ral_TupleCopyValues(src, src + 1, resultDst) ;
        }
        /*
         * Copy the values from the tuple in the input relation to the
         * tuple valued attribute.
         */
        wrapTuple = Ral_TupleNew(wrapHeading) ;
        wrapDst = Ral_TupleBegin(wrapTuple) ;
        for (iviter = Ral_IntVectorBegin(wattrs) ;
                iviter != Ral_IntVectorEnd(wattrs) ; ++iviter) {
            Ral_TupleIter src ;

            assert(*iviter < Ral_TupleDegree(tuple)) ;
            src = Ral_TupleBegin(tuple) + *iviter ;
            wrapDst += Ral_TupleCopyValues(src, src + 1, wrapDst) ;
        }
        /*
         * Make the wrapped tuple into an object and store it into the
         * result tuple.
         */
        *resultDst = Ral_TupleObjNew(wrapTuple) ;
        Tcl_IncrRefCount(*resultDst) ;
        /*
         * Add the result tuple to the result relation. We ignore any
         * duplicates that result, but there should not be any.
         */
        Ral_RelationPushBack(resultRelation, resultTuple, NULL) ;
    }

    Ral_IntVectorDelete(wattrs) ;
    Ral_IntVectorDelete(rattrs) ;

    Tcl_SetObjResult(interp, Ral_RelationObjNew(resultRelation)) ;
    return TCL_OK ;
}
