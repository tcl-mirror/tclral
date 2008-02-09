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
    ral_relationcmd -- command interface functions for the "relation" command

ABSTRACT:

$RCSfile: ral_relationcmd.c,v $
$Revision: 1.38 $
$Date: 2008/02/09 19:42:33 $
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
static int RelationChooseCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationComposeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationCreateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDegreeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
static int RelationDictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
#endif
static int RelationDivideCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationEliminateCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationEmptyofCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationExtendCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationExtractCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationForeachCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationGroupCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationHeadingCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIdentifiersCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIncludeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIntersectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsemptyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationIsnotemptyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationJoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationListCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationMinusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationProjectCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRankCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationReidentifyCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRenameCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRestrictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationRestrictWithCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemijoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemiminusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSummarizeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTagCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationTcloseCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
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
static const char rcsid[] = "@(#) $RCSfile: ral_relationcmd.c,v $ $Revision: 1.38 $" ;

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
	{"choose", RelationChooseCmd},
	{"compose", RelationComposeCmd},
	{"create", RelationCreateCmd},
	{"degree", RelationDegreeCmd},
#	    if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
	{"dict", RelationDictCmd},
#	    endif
	{"divide", RelationDivideCmd},
	{"eliminate", RelationEliminateCmd},
	{"emptyof", RelationEmptyofCmd},
	{"extend", RelationExtendCmd},
	{"extract", RelationExtractCmd},
	{"foreach", RelationForeachCmd},
	{"group", RelationGroupCmd},
	{"heading", RelationHeadingCmd},
	{"identifiers", RelationIdentifiersCmd},
	{"include", RelationIncludeCmd},
	{"intersect", RelationIntersectCmd},
	{"is", RelationIsCmd},
	{"isempty", RelationIsemptyCmd},
	{"isnotempty", RelationIsnotemptyCmd},
	{"join", RelationJoinCmd},
	{"list", RelationListCmd},
	{"minus", RelationMinusCmd},
	{"project", RelationProjectCmd},
	{"rank", RelationRankCmd},
	{"reidentify", RelationReidentifyCmd},
	{"rename", RelationRenameCmd},
	{"restrict", RelationRestrictCmd},
	{"restrictwith", RelationRestrictWithCmd},
	{"semijoin", RelationSemijoinCmd},
	{"semiminus", RelationSemiminusCmd},
	{"summarize", RelationSummarizeCmd},
	{"tag", RelationTagCmd},
	{"tclose", RelationTcloseCmd},
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
RelationArrayCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Tcl_Obj *arrayNameObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    int keyAttrIndex ;
    int valueAttrIndex ;
    Ral_RelationIter rEnd ;
    Ral_RelationIter rIter ;

    /* relation array relation arrayName ?keyAttr valueAttr? */
    if (!(objc == 4 || objc == 6)) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relation arrayName ?keyAttr valueAttr?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    if (objc ==4) {
	if (Ral_RelationDegree(relation) != 2) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptArray,
		RAL_ERR_DEGREE_TWO, relObj) ;
	    return TCL_ERROR ;
	}

	if (heading->idCount != 1) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptArray,
		RAL_ERR_SINGLE_IDENTIFIER, relObj) ;
	    return TCL_ERROR ;
	}
	if (Ral_IntVectorSize(*heading->identifiers) != 1) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptArray,
		RAL_ERR_SINGLE_ATTRIBUTE, relObj) ;
	    return TCL_ERROR ;
	}

	keyAttrIndex = Ral_IntVectorFetch(*heading->identifiers, 0) ;
	valueAttrIndex = (keyAttrIndex + 1) % 2 ;
    } else { /* objc == 6 */
	const char *keyAttrName = Tcl_GetString(objv[4]) ;
	const char *valueAttrName = Tcl_GetString(objv[5]) ;

	keyAttrIndex = Ral_TupleHeadingIndexOf(heading->tupleHeading,
	    keyAttrName) ;
	if (keyAttrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptArray,
		RAL_ERR_UNKNOWN_ATTR, keyAttrName) ;
	    return TCL_ERROR ;
	}
	valueAttrIndex = Ral_TupleHeadingIndexOf(heading->tupleHeading,
	    valueAttrName) ;
	if (valueAttrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptArray,
		RAL_ERR_UNKNOWN_ATTR, valueAttrName) ;
	    return TCL_ERROR ;
	}
    }

    arrayNameObj = objv[3] ;

    rEnd = Ral_RelationEnd(relation) ;
    for (rIter = Ral_RelationBegin(relation) ; rIter != rEnd ; ++rIter) {
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
    tupleHeading = relation->heading->tupleHeading ;

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
RelationChooseCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    Ral_Tuple key ;
    int idNum ;
    Ral_Relation newRelation ;
    Ral_RelationIter found ;
    Ral_ErrorInfo errInfo ;

    /* relation choose relValue attr value ?attr2 value2 ...? */
    if (objc < 5) {
	Tcl_WrongNumArgs(interp, 2, objv,
	    "relValue attr value ?attr2 value 2 ...?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;
    tupleHeading = heading->tupleHeading ;

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptChoose) ;
    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_ErrorInfoSetError(&errInfo, RAL_ERR_BAD_PAIRS_LIST,
	    "attribute / value arguments must be given in pairs") ;
	Ral_InterpSetError(interp, &errInfo) ;
	return TCL_ERROR ;
    }
    key = Ral_RelationObjKeyTuple(interp, relation, objc, objv, &idNum,
	&errInfo) ;
    if (key == NULL) {
	return TCL_ERROR ;
    }
    /*
     * Create the result relation.
     */
    newRelation = Ral_RelationNew(relation->heading) ;
    /*
     * Find the key tuple in the relation.
     */
    found = Ral_RelationFindKey(relation, idNum, key, NULL) ;
    Ral_TupleDelete(key) ;
    /*
     * If the key tuple can be found, then insert the tuple into
     * the result. Otherwise the result will have cardinality 0.
     */
    if (found != Ral_RelationEnd(relation)) {
	int inserted ;
	/*
	 * Should always be able to insert into an empty relation.
	 */
	inserted = Ral_RelationPushBack(newRelation, *found, NULL) ;
	assert(inserted != 0) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
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
 * relation create attrs ids ?tuple1 tuple2 ...?
 *
 * Returns a relation value with the given attributes, identifiers and body.
 * Cf. "tuple create"
 */

static int
RelationCreateCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Ral_RelationHeading heading ;
    Ral_ErrorInfo errInfo ;
    Ral_Relation relation ;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "attrs ids ?tuple1 tuple2 ...?") ;
	return TCL_ERROR ;
    }

    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptCreate) ;
    /*
     * Create the relation heading from the arguments.
     */
    heading = Ral_RelationHeadingNewFromObjs(interp, objv[2], objv[3],
	&errInfo) ;
    if (heading == NULL) {
	return TCL_ERROR ;
    }
    relation = Ral_RelationNew(heading) ;

    /*
     * Offset the argument bookkeeping to reference the tuples.
     */
    objc -= 4 ;
    objv += 4 ;
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

#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
static int
RelationDictCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Tcl_Obj *dictObj ;
    int keyAttrIndex ;
    int valueAttrIndex ;
    Ral_RelationIter rEnd ;
    Ral_RelationIter rIter ;

    /* relation dict relation ?keyAttr valueAttr? */
    if (!(objc == 3 || objc == 5)) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation ?keyAttr valueAttr?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;

    if (objc == 3) {
	if (Ral_RelationDegree(relation) != 2) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptDict,
		RAL_ERR_DEGREE_TWO, relObj) ;
	    return TCL_ERROR ;
	}

	if (heading->idCount != 1) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptDict,
		RAL_ERR_SINGLE_IDENTIFIER, relObj) ;
	    return TCL_ERROR ;
	}
	if (Ral_IntVectorSize(*heading->identifiers) != 1) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptDict,
		RAL_ERR_SINGLE_ATTRIBUTE, relObj) ;
	    return TCL_ERROR ;
	}

	keyAttrIndex = Ral_IntVectorFetch(*heading->identifiers, 0) ;
	valueAttrIndex = (keyAttrIndex + 1) % 2 ;
    } else { /* objc == 5 */
	const char *keyAttrName = Tcl_GetString(objv[3]) ;
	const char *valueAttrName = Tcl_GetString(objv[4]) ;

	keyAttrIndex = Ral_TupleHeadingIndexOf(heading->tupleHeading,
	    keyAttrName) ;
	if (keyAttrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptDict,
		RAL_ERR_UNKNOWN_ATTR, keyAttrName) ;
	    return TCL_ERROR ;
	}
	valueAttrIndex = Ral_TupleHeadingIndexOf(heading->tupleHeading,
	    valueAttrName) ;
	if (valueAttrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptDict,
		RAL_ERR_UNKNOWN_ATTR, valueAttrName) ;
	    return TCL_ERROR ;
	}
    }

    dictObj = Tcl_NewDictObj() ;
    rEnd = Ral_RelationEnd(relation) ;
    for (rIter = Ral_RelationBegin(relation) ; rIter != rEnd ; ++rIter) {
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
#endif

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
    static const char usage[] = "relationValue "
	"tupleVarName ?attr1 type1 expr1 ... attrN typeN exprN?" ;

    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    Tcl_Obj *varNameObj ;
    Ral_RelationHeading extHeading ;
    Ral_TupleHeading extTupleHeading ;
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
    tupleHeading = heading->tupleHeading ;

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
    extTupleHeading = Ral_TupleHeadingExtend(tupleHeading, objc / 3) ;
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1),
	    &errInfo) ;
	Ral_TupleHeadingIter inserted ;

	if (attr == NULL) {
	    Ral_TupleHeadingDelete(extTupleHeading) ;
	    return TCL_ERROR ;
	}
	inserted = Ral_TupleHeadingPushBack(extTupleHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(extTupleHeading)) {
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR, *v) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    Ral_TupleHeadingDelete(extTupleHeading) ;
	    return TCL_ERROR ;
	}
    }
    extHeading = Ral_RelationHeadingExtend(heading, extTupleHeading, 0) ;
    extRelation = Ral_RelationNew(extHeading) ;

    relEnd = Ral_RelationEnd(relation) ;
    extHeadingIter = Ral_TupleHeadingBegin(extTupleHeading) +
	Ral_RelationDegree(relation) ;
    for (relIter = Ral_RelationBegin(relation) ; relIter != relEnd ;
	++relIter) {
	Ral_Tuple tuple = *relIter ;
	Tcl_Obj *tupleObj = Ral_TupleObjNew(tuple) ;
	Ral_Tuple extTuple = Ral_TupleNew(extTupleHeading) ;
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

	    if (Tcl_ExprObj(interp, *v, &exprResult) != TCL_OK) {
		Ral_TupleDelete(extTuple) ;
		goto errorOut ;
	    }
	    if (Ral_AttributeConvertValueToType(interp, *attrIter++,
		exprResult, &errInfo) != TCL_OK) {
		Ral_TupleDelete(extTuple) ;
		Tcl_DecrRefCount(exprResult) ;
		Ral_InterpSetError(interp, &errInfo) ;
		goto errorOut ;
	    }
	    Tcl_IncrRefCount(*extIter++ = exprResult) ;
	    Tcl_DecrRefCount(exprResult) ;
	}
	/*
	 * Should always be able to insert the extended tuple since
	 * we have not changed the identifiers from the original relation.
	 */
	status = Ral_RelationPushBack(extRelation, extTuple, NULL) ;
	assert(status != 0) ;
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

    heading = relation->heading->tupleHeading ;
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
    Ral_IntVectorIter mapEnd ;
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
	    relation->heading->tupleHeading, interp, objv[4]) ;
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
	    relation->heading->tupleHeading, interp, objv[5]) ;
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
    mapEnd = Ral_IntVectorEnd(sortMap) ;
    for (mapIter = Ral_IntVectorBegin(sortMap) ;
	mapIter != mapEnd ; ++mapIter) {
	int appended ;
	Tcl_Obj *iterObj ;
	Ral_Relation iterRel = Ral_RelationNew(relation->heading) ;

	Ral_RelationReserve(iterRel, 1) ;
	appended = Ral_RelationPushBack(iterRel, *(relBegin + *mapIter),
	    NULL) ;
	assert(appended != 0) ;

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

    if (Ral_RelationCardinality(relation)) {
	Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    }
    Ral_IntVectorDelete(sortMap) ;
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
    Ral_Relation rel ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
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
    tupleHeading = heading->tupleHeading ;

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
	int attrIndex = Ral_TupleHeadingIndexOf(tupleHeading, attrName) ;
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
    if (Ral_IntVectorSize(grpAttrs) >= Ral_TupleHeadingSize(tupleHeading)) {
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
    index = Ral_TupleHeadingIndexOf(tupleHeading, relAttrName) ;
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
RelationIncludeCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_Relation newRel ;
    Ral_ErrorInfo errInfo ;

    /* relation include relationValue ?name-value-list ...? */
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
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptInclude) ;
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
    /* relation list relationValue ?attrName? */
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *listObj ;
    int attrIndex = 0 ;
    Ral_RelationIter iter ;
    Ral_RelationIter end ;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue ?attrName?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;

    /*
     * If no attribute name is mentioned, then we insist that the 
     * relation be of degree 1. The resulting list will necessarily be
     * a set.
     */
    if (objc == 3) {
	if (Ral_RelationDegree(relation) != 1) {
	    Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptList,
		RAL_ERR_DEGREE_ONE, relObj) ;
	    return TCL_ERROR ;
	}
    } else {
	/*
	 * Otherwise we need to find which attribute is referenced and
	 * will return the values of that attribute in all tuples of
	 * the relation. If the attribute does not constitute an identifier
	 * then, in general, the list will not be a set.
	 */
	const char *attrName = Tcl_GetString(objv[3]) ;
	attrIndex = Ral_TupleHeadingIndexOf(relation->heading->tupleHeading,
	    attrName) ;
	if (attrIndex < 0) {
	    Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptList,
		RAL_ERR_UNKNOWN_ATTR, attrName) ;
	    return TCL_ERROR ;
	}
    }

    listObj = Tcl_NewListObj(0, NULL) ;
    end = Ral_RelationEnd(relation) ;
    for (iter = Ral_RelationBegin(relation) ; iter != end ; ++iter) {
	Ral_Tuple tuple = *iter ;

	if (Tcl_ListObjAppendElement(interp, listObj, tuple->values[attrIndex])
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

/*
 * relation rank relationValue ?-ascending | -descending? rankAttr newAttr
 *
 * Generate a new relation extented by "newAttr". The type of "newAttr" is
 * "int" and its value is the number of tuples in "relationValue" where the
 * value of "rankAttr" is <= (-descending) or >= (-ascending) than its value
 * for a given tuple. Default ranking is "-ascending".  "rankAttr" must be of
 * type "int", "double", or "string" so that the comparison operation is well
 * defined.
 */
static int
RelationRankCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    const char *rankAttrName ;
    Ral_TupleHeadingIter rankAttrIter ;
    Ral_Attribute rankAttr ;
    int rankAttrIndex ;
    const char *newAttrName ;
    Ral_Attribute newAttr ;
    Ral_TupleHeading newTupleHeading ;
    Ral_TupleHeadingIter inserted ;
    Ral_RelationHeading newHeading ;
    Ral_Relation newRelation ;
    enum Ordering order = SORT_ASCENDING ;
    enum RankType {
	RANK_INT,
	RANK_DOUBLE,
	RANK_STRING,
    } rankType ;
    Ral_RelationIter rIter ;
    Ral_RelationIter rEnd ;

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
    tupleHeading = heading->tupleHeading ;

    /*
     * Find the rank attribute and check its type.
     */
    rankAttrName = Tcl_GetString(objv[objc - 2]) ;
    rankAttrIter = Ral_TupleHeadingFind(tupleHeading, rankAttrName) ;
    if (rankAttrIter == Ral_TupleHeadingEnd(tupleHeading)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptRank,
	    RAL_ERR_UNKNOWN_ATTR, rankAttrName) ;
	return TCL_ERROR ;
    }
    rankAttr = *rankAttrIter ;
    if (rankAttr->attrType != Tcl_Type) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptRank,
	    RAL_ERR_BAD_RANK_TYPE, rankAttrName) ;
	return TCL_ERROR ;
    } else if (strcmp(rankAttr->heading.tclType->name, "int") == 0) {
	rankType = RANK_INT ;
    } else if (strcmp(rankAttr->heading.tclType->name, "double") == 0) {
	rankType = RANK_DOUBLE ;
    } else if (strcmp(rankAttr->heading.tclType->name, "string") == 0) {
	rankType = RANK_STRING ;
    } else {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptRank,
	    RAL_ERR_BAD_RANK_TYPE, rankAttrName) ;
	return TCL_ERROR ;
    }
    rankAttrIndex = rankAttrIter - Ral_TupleHeadingBegin(tupleHeading) ;
    /*
     * Create the new ranked relation, extending it by the "newAttr".
     */
    newAttrName = Tcl_GetString(objv[objc - 1]) ;
    newAttr = Ral_AttributeNewTclType(newAttrName, "int") ;
    assert(newAttr != NULL) ;
    newTupleHeading = Ral_TupleHeadingExtend(tupleHeading, 1) ;
    inserted = Ral_TupleHeadingPushBack(newTupleHeading, newAttr) ;
    if (inserted == Ral_TupleHeadingEnd(newTupleHeading)) {
	Ral_InterpErrorInfo(interp, Ral_CmdRelation, Ral_OptRank,
	    RAL_ERR_DUPLICATE_ATTR, rankAttrName) ;
	Ral_TupleHeadingDelete(newTupleHeading) ;
	return TCL_ERROR ;
    }
    newHeading = Ral_RelationHeadingExtend(heading, newTupleHeading, 0) ;
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
    rEnd = Ral_RelationEnd(relation) ;
    for (rIter = Ral_RelationBegin(relation) ; rIter != rEnd ; ++rIter) {
	Ral_RelationIter tIter ;
	Ral_Tuple rankTuple = *rIter ;
	Tcl_Obj *rankObj = *(Ral_TupleBegin(rankTuple) + rankAttrIndex) ;
	int rankCount = 0 ;
	Ral_Tuple newTuple ;
	Ral_TupleIter newIter ;
	int status ;

	if (Tcl_ConvertToType(interp, rankObj, rankAttr->heading.tclType)
		!= TCL_OK) {
	    Ral_RelationDelete(newRelation) ;
	    return TCL_ERROR ;
	}

	for (tIter = Ral_RelationBegin(relation) ; tIter != rEnd ; ++tIter) {
	    Tcl_Obj *cmpObj = *(Ral_TupleBegin(*tIter) + rankAttrIndex) ;

	    if (Tcl_ConvertToType(interp, cmpObj, rankAttr->heading.tclType)
		!= TCL_OK) {
		Ral_RelationDelete(newRelation) ;
		return TCL_ERROR ;
	    }

	    switch (rankType) {
	    case RANK_INT:
		rankCount += order == SORT_DESCENDING ?
		    cmpObj->internalRep.longValue >
			rankObj->internalRep.longValue :
		    cmpObj->internalRep.longValue <
			rankObj->internalRep.longValue ;
		break ;

	    case RANK_DOUBLE:
		rankCount += order == SORT_DESCENDING ?
		    cmpObj->internalRep.doubleValue >
			rankObj->internalRep.doubleValue :
		    cmpObj->internalRep.doubleValue <
			rankObj->internalRep.doubleValue ;
		break ;

	    case RANK_STRING:
		rankCount += order == SORT_DESCENDING ?
		    strcmp(Tcl_GetString(cmpObj), Tcl_GetString(rankObj)) > 0 :
		    strcmp(Tcl_GetString(cmpObj), Tcl_GetString(rankObj)) < 0 ;
		break ;

	    default:
		Tcl_Panic("Ral_RelationRankCmd: unknown rank type, %d",
		    rankType) ;
	    }
	}

	newTuple = Ral_TupleNew(newTupleHeading) ;
	newIter = Ral_TupleBegin(newTuple) ;
	newIter += Ral_TupleCopyValues(Ral_TupleBegin(rankTuple),
	    Ral_TupleEnd(rankTuple), newIter) ;
	/*
	 * +1 because we are using simple > or <
	 */
	Tcl_IncrRefCount(*newIter = Tcl_NewIntObj(rankCount + 1)) ;
	/*
	 * Should always be able to insert the new tuple since
	 * we have not changed the identifiers from the original relation.
	 */
	status = Ral_RelationPushBack(newRelation, newTuple, NULL) ;
	assert(status != 0) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
    return TCL_OK ;
}

/*
 * relation reidentify relationValue id1 ?id2 id3 ...?
 *
 * Returns a new relation value where the identifiers are
 * changed to id1, id2  ...
 * Each "idN" is a list of attribute names that are to form the identifier.
 * The tuples of the new relation are the same as those of "relationValue"
 * less any that are duplicates under the new identification scheme.
 */
static int
RelationReidentifyCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const* objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading newHeading ;
    Ral_Relation newRelation ;
    Ral_ErrorInfo errInfo ;
    int idNum = 0 ;
    Ral_RelationIter riter ;
    Ral_RelationIter rend ;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "relationValue id1 ?id2 id3 ... ?") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptReidentify) ;

    /*
     * Adjust argument bookkeeping to the beginning of the identifiers.
     */
    objc -= 3 ;
    objv += 3 ;
    newHeading = Ral_RelationHeadingNew(relation->heading->tupleHeading, objc) ;

    /*
     * Iterate through the argument lists and add them as identifiers to
     * the heading.
     */
    for ( ; objc > 0 ; --objc, ++objv) {
	if (Ral_RelationHeadingNewIdFromObj(interp, newHeading, idNum++,
	    *objv, &errInfo) != TCL_OK) {
	    Ral_RelationHeadingDelete(newHeading) ;
	    return TCL_ERROR ;
	}
    }
    /*
     * Create a new relation with the new heading.
     */
    newRelation = Ral_RelationNew(newHeading) ;
    Ral_RelationReserve(newRelation, Ral_RelationCardinality(relation)) ;
    /*
     * Iterate through the tuples of the relation value and insert them
     * into the new relation. We ignore any duplicates.
     */
    rend = Ral_RelationEnd(relation) ;
    for (riter = Ral_RelationBegin(relation) ; riter != rend ; ++riter) {
	Ral_RelationPushBack(newRelation, *riter, NULL) ;
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
    Ral_TupleHeading tupleHeading ;
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
    tupleHeading = relation->heading->tupleHeading ;
    thBegin = Ral_TupleHeadingBegin(tupleHeading) ;
    thEnd = Ral_TupleHeadingEnd(tupleHeading) ;

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
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    Tcl_Obj *perObj ;
    Ral_Relation perRelation ;
    Ral_RelationHeading perHeading ;
    Ral_TupleHeading perTupleHeading ;
    Tcl_Obj *varNameObj ;
    Ral_Relation sumRelation ;
    Ral_RelationHeading sumHeading ;
    Ral_TupleHeading sumTupleHeading ;
    int c ;
    Tcl_Obj *const*v ;
    Ral_JoinMap joinMap ;
    int index = 0 ;
    Ral_RelationIter perIter ;
    Ral_RelationIter perEnd ;
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
    tupleHeading = heading->tupleHeading ;

    perObj = objv[3] ;
    if (Tcl_ConvertToType(interp, perObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    perRelation = perObj->internalRep.otherValuePtr ;
    perHeading = perRelation->heading ;
    perTupleHeading = perHeading->tupleHeading ;
    Ral_ErrorInfoSetCmd(&errInfo, Ral_CmdRelation, Ral_OptSummarize) ;

    /*
     * Create a join map so that we can find the tuples that match between the
     * relation and the per-relation.  The "per" relation must be a subset of
     * the summarized relation. We will know that because the common attributes
     * must encompass all those in the per relation.
     */
    joinMap = Ral_JoinMapNew(Ral_TupleHeadingSize(perTupleHeading),
	Ral_RelationCardinality(relation)) ;
    c = Ral_TupleHeadingCommonAttributes(perTupleHeading, tupleHeading,
	joinMap) ;
    if (c != Ral_TupleHeadingSize(perTupleHeading)) {
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
    sumTupleHeading = Ral_TupleHeadingExtend(perTupleHeading, objc / 3) ;
    /*
     * Add in the summary attributes to tuple heading.
     */
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1),
	    &errInfo) ;
	Ral_TupleHeadingIter inserted ;

	if (attr == NULL) {
	    Ral_TupleHeadingDelete(sumTupleHeading) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
	inserted = Ral_TupleHeadingPushBack(sumTupleHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(sumTupleHeading)) {
	    Ral_TupleHeadingDelete(sumTupleHeading) ;
	    Ral_ErrorInfoSetErrorObj(&errInfo, RAL_ERR_DUPLICATE_ATTR, *v) ;
	    Ral_InterpSetError(interp, &errInfo) ;
	}
    }
    sumHeading = Ral_RelationHeadingExtend(perHeading, sumTupleHeading, 0) ;
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
    sumHeadingIter = Ral_TupleHeadingBegin(sumTupleHeading) +
	Ral_RelationDegree(perRelation) ;
    perEnd = Ral_RelationEnd(perRelation) ;
    for (perIter = Ral_RelationBegin(perRelation) ; perIter != perEnd ;
	++perIter) {
	Ral_Tuple perTuple = *perIter ;
	Ral_IntVector matchSet = Ral_JoinMapMatchingTupleSet(joinMap, 0,
	    index++) ;
	Ral_Relation matchRel = Ral_RelationExtract(relation, matchSet) ;
	Tcl_Obj *matchObj = Ral_RelationObjNew(matchRel) ;
	Ral_Tuple sumTuple = Ral_TupleNew(sumTupleHeading) ;
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
	    if (Tcl_ExprObj(interp, *v, sumIter) != TCL_OK) {
		Ral_TupleDelete(sumTuple) ;
		goto errorOut ;
	    }
	    if (Ral_AttributeConvertValueToType(interp, *attrIter++,
		*sumIter, &errInfo) != TCL_OK) {
		Ral_TupleDelete(sumTuple) ;
		Tcl_DecrRefCount(*sumIter) ;
		goto errorOut ;
	    }
	    ++sumIter ;
	}
	status = Ral_RelationPushBack(sumRelation, sumTuple, NULL) ;
	assert(status != 0) ;
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

/*
 * relation tag relation ?-ascending | -descending sort-attr-list?
 *	?-within {idsubset-attr-list}? attrName
 *
 * Create a new relation extended by "attrName". "attrName" will be "int" type
 * and will consist of consecutive integers starting at zero. The tuples in
 * "relation" will extended in the order implied by ascending or descending
 * order of "sort-attr-list" if the option is given. If absent the order is
 * arbitrary.  If the "-within" option is given then "attrName" values will be
 * unique within the identifier for which the attributes in
 * "idsubset-attr-list" is a subset minus one attribute. In either case an
 * additional identifier is created that consists of just "attrName" or of
 * "idsubset-attr-list + attrName" if the -within option is given.
 */
static int
RelationTagCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    Tcl_Obj *attrNameObj ;
    Ral_Attribute tagAttr ;
    Ral_TupleHeading tagTupleHeading ;
    Ral_TupleHeadingIter inserted ;
    Ral_RelationHeading tagHeading ;
    Ral_Relation tagRelation ;
    int argc ;
    Tcl_Obj *const* argv ;
    int tagAttrIndex ;
    Ral_IntVector sortMap = NULL ;
    Ral_IntVector withinAttrs = NULL ;
    int tagIdNum = -1 ;

    if (objc < 4 || objc > 8) {
	Tcl_WrongNumArgs(interp, 2, objv, "relation "
	    "?-ascending | -descending sort-attr-list? "
	    "?-within idsubset-list? attrName") ;
	return TCL_ERROR ;
    }

    relObj = objv[2] ;
    if (Tcl_ConvertToType(interp, relObj, &Ral_RelationObjType) != TCL_OK) {
	return TCL_ERROR ;
    }
    relation = relObj->internalRep.otherValuePtr ;
    heading = relation->heading ;
    tupleHeading = heading->tupleHeading ;
    /*
     * The attribute name is always the last argument.
     */
    attrNameObj = objv[objc - 1] ;
    /*
     * Make a new relation, adding the extended attribute.
     */
    tagAttr = Ral_AttributeNewTclType(Tcl_GetString(attrNameObj), "int") ;
    assert(tagAttr != NULL) ;
    tagTupleHeading = Ral_TupleHeadingExtend(tupleHeading, 1) ;
    inserted = Ral_TupleHeadingPushBack(tagTupleHeading, tagAttr) ;
    if (inserted == Ral_TupleHeadingEnd(tagTupleHeading)) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTag,
	    RAL_ERR_DUPLICATE_ATTR, attrNameObj) ;
	Ral_TupleHeadingDelete(tagTupleHeading) ;
	return TCL_ERROR ;
    } else {
	tagAttrIndex = inserted - Ral_TupleHeadingBegin(tagTupleHeading) ;
    }
    tagHeading = Ral_RelationHeadingExtend(heading, tagTupleHeading, 1) ;
    tagRelation = Ral_RelationNew(tagHeading) ;
    /*
     * Parse the remaining arguments.
     */
    argc = objc - 4 ;
    argv = objv + 3 ;
    for ( ; argc > 0 ; argc -= 2, argv += 2) {
	static const struct {
	    const char *optName ;
	    enum {
		TAG_ASCENDING,
		TAG_DESCENDING,
		TAG_WITHIN,
	    } opt ;
	} optTable[] = {
	    {"-ascending", TAG_ASCENDING},
	    {"-descending", TAG_DESCENDING},
	    {"-within", TAG_WITHIN},
	    {NULL, 0},
	} ;
	Ral_IntVector sortAttrs ;
	int index ;

	if (Tcl_GetIndexFromObjStruct(interp, argv[0], optTable,
	    sizeof(optTable[0]), "tag option", 0, &index) != TCL_OK) {
	    Ral_RelationDelete(tagRelation) ;
	    return TCL_ERROR ;
	}

	/*
	 * Make sure that duplicated options are not present, e.g. no
	 * "-ascending a1 -descending b1" or "-within a1 -within b1".
	 */
	if ((sortMap && (optTable[index].opt == TAG_ASCENDING ||
		optTable[index].opt == TAG_DESCENDING))
	    || (tagIdNum >= 0 && optTable[index].opt == TAG_WITHIN)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "relation "
		"?-ascending | -descending sort-attr-list? "
		"?-within idsubset-list? attrName") ;
	    Ral_RelationDelete(tagRelation) ;
	    return TCL_ERROR ;
	}

	switch (optTable[index].opt) {
	case TAG_ASCENDING:
	    sortAttrs = Ral_TupleHeadingAttrsFromObj(tupleHeading, interp,
		argv[1]) ;
	    if (sortAttrs == NULL) {
		Ral_RelationDelete(tagRelation) ;
		return TCL_ERROR ;
	    }
	    sortMap = Ral_RelationSort(relation, sortAttrs, 0) ;
	    Ral_IntVectorDelete(sortAttrs) ;
	    break;

	case TAG_DESCENDING:
	    sortAttrs = Ral_TupleHeadingAttrsFromObj(tupleHeading, interp,
		argv[1]) ;
	    if (sortAttrs == NULL) {
		Ral_RelationDelete(tagRelation) ;
		return TCL_ERROR ;
	    }
	    sortMap = Ral_RelationSort(relation, sortAttrs, 1) ;
	    Ral_IntVectorDelete(sortAttrs) ;
	    break ;

	case TAG_WITHIN:
	{
	    Ral_RelationIdIter idIter ;
	    Ral_RelationIdIter idEnd = Ral_RelationHeadingIdEnd(heading) ;
	    int idCnt = 0 ;
	    int subSetIdNum = -1 ;
	    int status ;
	    Ral_IntVector newId ;

	    withinAttrs = Ral_TupleHeadingAttrsFromObj(tupleHeading, interp,
		argv[1]) ;
	    /*
	     * Check if the attributes are a subset - 1 of some identifier.
	     */
	    for (idIter = Ral_RelationHeadingIdBegin(heading) ;
		idIter != idEnd ; ++idIter, ++idCnt) {
		Ral_IntVector id = *idIter ;
		if (Ral_IntVectorSubsetOf(withinAttrs, id) &&
		    Ral_IntVectorSize(withinAttrs) ==
			Ral_IntVectorSize(id) - 1) {
		    subSetIdNum = idCnt ;
		    break ;
		}
	    }
	    if (subSetIdNum < 0) {
		Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTag,
		    RAL_ERR_WITHIN_NOT_SUBSET, argv[1]) ;
		Ral_RelationDelete(tagRelation) ;
		Ral_IntVectorDelete(withinAttrs) ;
		return TCL_ERROR ;
	    }
	    /*
	     * Construct a new identifier from that subset and the new
	     * attribute.
	     */
	    newId = Ral_IntVectorDup(withinAttrs) ;
	    Ral_IntVectorPushBack(newId, tagAttrIndex) ;
	    tagIdNum = tagHeading->idCount - 1 ;
	    status = Ral_RelationHeadingAddIdentifier(tagHeading, tagIdNum,
		newId) ;
	    /*
	     * Should always be able to add this identifier since we know
	     * it was formed from a new attribute.
	     */
	    assert(status != 0) ;
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
    /*
     * "tagIdNum" tells us the scope of the identifier. If the scope is
     * across the entire relation, then we can just iterate through and
     * update the tag attribute. Otherwise we have to set up to find the
     * subsets of tuples that have to be numbered separately.
     */
    if (tagIdNum < 0) {
	Ral_RelationIter relBegin = Ral_RelationBegin(relation) ;
	Ral_IntVectorIter mIter ;
	Ral_IntVectorIter mEnd = Ral_IntVectorEnd(sortMap) ;
	int tagValue = 0 ;
	int status ;

	tagIdNum = tagHeading->idCount - 1 ;
	status = Ral_RelationHeadingAddIdentifier(tagHeading, tagIdNum,
	    Ral_IntVectorNew(1, tagAttrIndex)) ;
	assert(status != 0) ;

	for (mIter = Ral_IntVectorBegin(sortMap) ; mIter != mEnd ; ++mIter) {
	    Ral_Tuple tuple = *(relBegin + *mIter) ;
	    Ral_Tuple tagTuple = Ral_TupleNew(tagTupleHeading) ;
	    Ral_TupleIter tagIter = Ral_TupleBegin(tagTuple) ;
	    int status ;

	    tagIter += Ral_TupleCopyValues(Ral_TupleBegin(tuple),
		Ral_TupleEnd(tuple), tagIter) ;

	    Tcl_IncrRefCount(*tagIter = Tcl_NewIntObj(tagValue++)) ;
	    /*
	     * Should always be able to insert the tagged tuple since
	     * we have not changed the identifiers from the original relation
	     * and have added a new identifier with the tag value.
	     */
	    status = Ral_RelationPushBack(tagRelation, tagTuple, NULL) ;
	    assert(status != 0) ;
	}
    } else {
	/*
	 * We must generate values for the tag attribute that are unique
	 * within the context of an identifier with multiple attributes.
	 * So we will build a hash table keyed by the attribute values
	 * in "withinAttrs" and the value of the hash entry will be the
	 * current value of the tag (starting at zero). Then it is a matter
	 * of iterating through the relation, computing the hash and
	 * adding a new tuple to the tagged relation.
	 */
	Tcl_HashTable tagHash ;
	Tcl_DString idKey ;
	Ral_RelationIter relBegin = Ral_RelationBegin(relation) ;
	Ral_IntVectorIter mIter ;
	Ral_IntVectorIter mEnd = Ral_IntVectorEnd(sortMap) ;

	assert(withinAttrs != NULL) ;
	Tcl_InitHashTable(&tagHash, TCL_STRING_KEYS) ;

	for (mIter = Ral_IntVectorBegin(sortMap) ; mIter != mEnd ; ++mIter) {
	    Ral_Tuple tuple = *(relBegin + *mIter) ;
	    Ral_Tuple tagTuple = Ral_TupleNew(tagTupleHeading) ;
	    Ral_TupleIter tagIter = Ral_TupleBegin(tagTuple) ;
	    Tcl_HashEntry *entry ;
	    int newPtr ;
	    int tagValue ;
	    int status ;

	    entry = Tcl_CreateHashEntry(&tagHash,
		Ral_RelationGetIdKey(tuple, withinAttrs, NULL, &idKey),
		&newPtr) ;
	    Tcl_DStringFree(&idKey) ;
	    if (newPtr) {
		/*
		 * New subset of tuples, initialize the entry to zero.
		 */
		tagValue = 0 ;
	    } else {
		tagValue = (int)Tcl_GetHashValue(entry) ;
	    }
	    Tcl_SetHashValue(entry, tagValue + 1) ;

	    tagIter += Ral_TupleCopyValues(Ral_TupleBegin(tuple),
		Ral_TupleEnd(tuple), tagIter) ;

	    Tcl_IncrRefCount(*tagIter = Tcl_NewIntObj(tagValue)) ;
	    /*
	     * Should always be able to insert the tagged tuple since
	     * we have not changed the identifiers from the original relation
	     * and have added a new identifier with the tag value.
	     */
	    status = Ral_RelationPushBack(tagRelation, tagTuple, NULL) ;
	    assert(status != 0) ;
	}

	Tcl_DeleteHashTable(&tagHash) ;
    }

    Ral_IntVectorDelete(sortMap) ;
    if (withinAttrs) {
	Ral_IntVectorDelete(withinAttrs) ;
    }
    Tcl_SetObjResult(interp, Ral_RelationObjNew(tagRelation)) ;
    return TCL_OK ;
}

static int
RelationTcloseCmd(
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const*objv)
{
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
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
    tupleHeading = heading->tupleHeading ;

    if (Ral_RelationDegree(relation) != 2) {
	Ral_InterpErrorInfoObj(interp, Ral_CmdRelation, Ral_OptTclose,
	    RAL_ERR_DEGREE_TWO, relObj) ;
	return TCL_ERROR ;
    }

    thIter = Ral_TupleHeadingBegin(tupleHeading) ;
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
    unionRel = Ral_RelationUnion(r1, r2, &errInfo) ;
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

	unionRel = Ral_RelationUnion(r1, r2, &errInfo) ;
	Ral_RelationDelete(r1) ;
	if (unionRel == NULL) {
	    Ral_InterpSetError(interp, &errInfo) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(unionRel)) ;
    return TCL_OK ;
}
