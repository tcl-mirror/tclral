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

$RCSfile: ral_relationcmd.c,v $
$Revision: 1.10 $
$Date: 2006/04/16 19:00:12 $
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
enum Ordering {
    SORT_ASCENDING,
    SORT_DESCENDING
} ;

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/
static int RelationCardinalityCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationChooseCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationDegreeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
static int RelationDictCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
#endif
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
static int RelationRestrictWithCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemijoinCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSemiminusCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
static int RelationSummarizeCmd(Tcl_Interp *, int, Tcl_Obj *const*) ;
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
static const char *orderOptions[] = {
    "-ascending",
    "-descending",
    NULL
} ;
static const char rcsid[] = "@(#) $RCSfile: ral_relationcmd.c,v $ $Revision: 1.10 $" ;

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
	{"choose", RelationChooseCmd},
	{"degree", RelationDegreeCmd},
#	    if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
	{"dict", RelationDictCmd},
#	    endif
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
	{"restrictwith", RelationRestrictWithCmd},
	{"semijoin", RelationSemijoinCmd},
	{"semiminus", RelationSemiminusCmd},
	{"summarize", RelationSummarizeCmd},
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
    Ral_IntVector id ;
    Ral_Tuple key ;
    int idNum ;
    Ral_Relation newRelation ;
    Ral_RelationIter found ;

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

    objc -= 3 ;
    objv += 3 ;
    if (objc % 2 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_PAIRS_LIST,
	    "attribute / value arguments must be given in pairs") ;
	return TCL_ERROR ;
    }
    /*
     * Iterate through the name/value list and construct an identifier
     * vector from the attribute names and a key tuple from the corresponding
     * values.
     */
    id = Ral_IntVectorNewEmpty(objc / 2) ;
    key = Ral_TupleNew(tupleHeading) ;
    for ( ; objc > 0 ; objc -= 2, objv += 2) {
	const char *attrName = Tcl_GetString(*objv) ;
	int attrIndex = Ral_TupleHeadingIndexOf(tupleHeading, attrName) ;
	int updated ;

	if (attrIndex < 0) {
	    Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR, attrName) ;
	    goto error_out ;
	}
	Ral_IntVectorPushBack(id, attrIndex) ;

	updated = Ral_TupleUpdateAttrValue(key, attrName, *(objv + 1)) ;
	if (!updated) {
	    Ral_TupleObjSetError(interp, Ral_TupleLastError,
		Tcl_GetString(*(objv + 1))) ;
	    goto error_out ;
	}
    }

    /*
     * Check if the attributes given do constitute an identifier.
     */
    idNum = Ral_RelationHeadingFindIdentifier(heading, id) ;
    if (idNum < 0) {
	Ral_RelationObjSetError(interp, REL_NOT_AN_IDENTIFIER,
	    "during choose operation") ;
	goto error_out ;
    }
    Ral_IntVectorDelete(id) ;
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
	int inserted = Ral_RelationPushBack(newRelation, *found, NULL) ;
	/*
	 * Should always be able to insert into an empty relation.
	 */
	assert(inserted != 0) ;
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(newRelation)) ;
    return TCL_OK ;

error_out:
    Ral_IntVectorDelete(id) ;
    Ral_TupleDelete(key) ;
    return TCL_ERROR ;
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

    /* relation dict relation */
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
	Ral_RelationObjSetError(interp, REL_DEGREE_TWO, Tcl_GetString(relObj)) ;
	return TCL_ERROR ;
    }

    if (heading->idCount != 1) {
	Ral_RelationObjSetError(interp, REL_SINGLE_IDENTIFIER,
	    Tcl_GetString(relObj)) ;
	return TCL_ERROR ;
    }
    if (Ral_IntVectorSize(*heading->identifiers) != 1) {
	Ral_RelationObjSetError(interp, REL_SINGLE_ATTRIBUTE,
	    Tcl_GetString(relObj)) ;
	return TCL_ERROR ;
    }

    dictObj = Ral_RelationObjDict(interp, relation) ;
    if (dictObj == NULL) {
	return TCL_ERROR ;
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
    quot = Ral_RelationDivide(dend, dsor, med) ;
    if (quot == NULL) {
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
	    "during divide operation") ;
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
	"?-ascending | -descending attrList? "
	"tupleVarName ?attr1 type1 expr1 ... attrN typeN exprN?" ;

    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Ral_RelationHeading heading ;
    Ral_TupleHeading tupleHeading ;
    const char *orderOption ;
    Tcl_Obj *varNameObj ;
    Ral_IntVector sortMap ;
    Ral_RelationHeading extHeading ;
    Ral_TupleHeading extTupleHeading ;
    Ral_Relation extRelation ;
    int c ;
    Tcl_Obj *const*v ;
    Ral_IntVectorIter mapIter ;
    Ral_IntVectorIter mapEnd ;
    Ral_RelationIter relBegin ;
    Ral_TupleHeadingIter extHeadingIter ;

    /*
     * relation extend relationValue ?-ascending | -descending attr-list?
     *	    tupleVarName ?attr1 type1 expr1...attrN typeN exprN?
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

    orderOption = Tcl_GetString(objv[3]) ;
    if (*orderOption == '-') {
	/*
	 * Ordering option specified.
	 */
	int index ;
	Ral_IntVector sortAttrs ;

	if (objc < 6) {
	    Tcl_WrongNumArgs(interp, 2, objv, usage) ;
	    return TCL_ERROR ;
	}

	if (Tcl_GetIndexFromObj(interp, objv[3], orderOptions, "ordering", 0,
	    &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	sortAttrs = Ral_TupleHeadingAttrsFromObj(
	    relation->heading->tupleHeading, interp, objv[4]) ;
	if (sortAttrs == NULL) {
	    return TCL_ERROR ;
	}
	sortMap = index == SORT_ASCENDING ?
	    Ral_RelationSortAscending(relation, sortAttrs) :
	    Ral_RelationSortDescending(relation, sortAttrs) ;
	Ral_IntVectorDelete(sortAttrs) ;
	varNameObj = objv[5] ;
	objc -= 6 ;
	objv += 6 ;
    } else {
	sortMap = Ral_IntVectorNew(Ral_RelationCardinality(relation), 0) ;
	Ral_IntVectorFillConsecutive(sortMap, 0) ;
	varNameObj = objv[3] ;
	objc -= 4 ;
	objv += 4 ;
    }

    if (objc % 3 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_TRIPLE_LIST,
	"attribute / type / expression arguments must be given in triples") ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(varNameObj) ;
    /*
     * Make a new tuple heading, adding the extended attributes.
     */
    extTupleHeading = Ral_TupleHeadingExtend(tupleHeading, objc / 3) ;
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1)) ;
	Ral_TupleHeadingIter inserted ;

	if (attr == NULL) {
	    goto errorOut ;
	}
	inserted = Ral_TupleHeadingPushBack(extTupleHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(extTupleHeading)) {
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
		Tcl_GetString(*v)) ;
	    goto errorOut ;
	}
    }
    extHeading = Ral_RelationHeadingExtend(heading, extTupleHeading) ;
    extRelation = Ral_RelationNew(extHeading) ;

    relBegin = Ral_RelationBegin(relation) ;
    extHeadingIter = Ral_TupleHeadingBegin(extTupleHeading) +
	Ral_RelationDegree(relation) ;
    mapEnd = Ral_IntVectorEnd(sortMap) ;
    for (mapIter = Ral_IntVectorBegin(sortMap) ;
	mapIter != mapEnd ; ++mapIter) {
	Ral_Tuple tuple = *(relBegin + *mapIter) ;
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
		exprResult) != TCL_OK) {
		Ral_TupleDelete(extTuple) ;
		Tcl_DecrRefCount(exprResult) ;
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
    Ral_IntVectorDelete(sortMap) ;
    Tcl_SetObjResult(interp, Ral_RelationObjNew(extRelation)) ;
    return TCL_OK ;

errorOut:
    Tcl_UnsetVar(interp, Tcl_GetString(varNameObj), 0) ;
    Tcl_DecrRefCount(varNameObj) ;
    Ral_IntVectorDelete(sortMap) ;
    Ral_RelationDelete(extRelation) ;
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
     * relation foreach tupleVarName relationValue ?-ascending | -descending?
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
	sortMap = Ral_RelationSortAscending(relation, sortAttrs) ;
	Ral_IntVectorDelete(sortAttrs) ;
	scriptObj = objv[5] ;
    } else /* objc == 7 */ {
	/*
	 * Both ordering keyword and attribute list given.
	 */
	int index ;
	Ral_IntVector sortAttrs ;

	if (Tcl_GetIndexFromObj(interp, objv[4], orderOptions, "ordering", 0,
	    &index) != TCL_OK) {
	    return TCL_ERROR ;
	}
	sortAttrs = Ral_TupleHeadingAttrsFromObj(
	    relation->heading->tupleHeading, interp, objv[5]) ;
	if (sortAttrs == NULL) {
	    return TCL_ERROR ;
	}
	sortMap = index == SORT_ASCENDING ?
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

    relBegin = Ral_RelationBegin(relation) ;
    mapEnd = Ral_IntVectorEnd(sortMap) ;
    for (mapIter = Ral_IntVectorBegin(sortMap) ;
	mapIter != mapEnd ; ++mapIter) {
	Tcl_Obj *tupleObj = Ral_TupleObjNew(*(relBegin + *mapIter)) ;

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
	    Ral_RelationObjSetError(interp, REL_UNKNOWN_ATTR, attrName) ;
	    Ral_IntVectorDelete(grpAttrs) ;
	    return TCL_ERROR ;
	}
	Ral_IntVectorSetAdd(grpAttrs, attrIndex) ;
    }
    /*
     * You may not group away all of the attributes.
     */
    if (Ral_IntVectorSize(grpAttrs) >= Ral_TupleHeadingSize(tupleHeading)) {
	Ral_RelationObjSetError(interp, REL_TOO_MANY_ATTRS,
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
	Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR, relAttrName) ;
	Ral_IntVectorDelete(grpAttrs) ;
	return TCL_ERROR ;
    }

    grpRel = Ral_RelationGroup(rel, relAttrName, grpAttrs) ;
    if (grpRel == NULL) {
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
	    "during group operation") ;
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
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
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
	    Ral_RelationObjSetError(interp, Ral_RelationLastError,
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
    Tcl_Obj *r1Obj ;
    Ral_Relation r1 ;
    Tcl_Obj *r2Obj ;
    Ral_Relation r2 ;
    Ral_Relation joinRel ;
    Ral_JoinMap joinMap ;

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

    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap)
	!= TCL_OK) {
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    joinRel = Ral_RelationJoin(r1, r2, joinMap) ;
    Ral_JoinMapDelete(joinMap) ;
    if (joinRel == NULL) {
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
	    Tcl_GetString(r2Obj)) ;
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

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap)
	    != TCL_OK) {
	    Ral_JoinMapDelete(joinMap) ;
	    return TCL_ERROR ;
	}
	joinRel = Ral_RelationJoin(r1, r2, joinMap) ;
	Ral_RelationDelete(r1) ;
	Ral_JoinMapDelete(joinMap) ;
	if (joinRel == NULL) {
	    Ral_RelationObjSetError(interp, Ral_RelationLastError,
		Tcl_GetString(r2Obj)) ;
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
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
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
	    Tcl_Obj *attrNameObj = Tcl_NewStringObj(attr->name, -1) ;

	    if (Tcl_ObjSetVar2(interp, attrNameObj, NULL, attrValueObj,
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

    objc -= 4 ;
    objv += 4 ;

    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap)
	!= TCL_OK) {
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

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap)
	    != TCL_OK) {
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

    objc -= 4 ;
    objv += 4 ;

    if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap)
	!= TCL_OK) {
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

	if (Ral_RelationObjParseJoinArgs(interp, &objc, &objv, r1, r2, joinMap)
	    != TCL_OK) {
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
	Ral_RelationObjSetError(interp, REL_NOT_A_PROJECTION,
	    Tcl_GetString(perObj)) ;
	Ral_JoinMapDelete(joinMap) ;
	return TCL_ERROR ;
    }

    varNameObj = objv[4] ;

    objc -= 5 ;
    objv += 5 ;
    if (objc % 3 != 0) {
	Ral_RelationObjSetError(interp, REL_BAD_TRIPLE_LIST,
	"attribute / type / expression arguments must be given in triples") ;
	return TCL_ERROR ;
    }

    Tcl_IncrRefCount(varNameObj) ;
    /*
     * Construct the heading for the result. It the heading of the
     * "per" relation plus the summary attributes.
     */
    sumTupleHeading = Ral_TupleHeadingExtend(perTupleHeading, objc / 3) ;
    /*
     * Add in the summary attributes to tuple heading.
     */
    for (c = objc, v = objv ; c > 0 ; c -= 3, v += 3) {
	Ral_Attribute attr = Ral_AttributeNewFromObjs(interp, *v, *(v + 1)) ;
	Ral_TupleHeadingIter inserted ;

	if (attr == NULL) {
	    goto errorOut ;
	}
	inserted = Ral_TupleHeadingPushBack(sumTupleHeading, attr) ;
	if (inserted == Ral_TupleHeadingEnd(sumTupleHeading)) {
	    Ral_RelationObjSetError(interp, REL_DUPLICATE_ATTR,
		Tcl_GetString(*v)) ;
	    goto errorOut ;
	}
    }
    sumHeading = Ral_RelationHeadingExtend(perHeading, sumTupleHeading) ;
    sumRelation = Ral_RelationNew(sumHeading) ;

    /*
     * The strategy is to iterate over each tuple in the per-relation and to
     * construct subsets of the relation that match the tuple. That subset
     * relation is then assigned to the variable name and each summary
     * attribute is computed by evaluating the expression and assigning the
     * result to the attribute.
     */
    Ral_RelationFindJoinTuples(perRelation, relation, joinMap) ;
    perEnd = Ral_RelationEnd(perRelation) ;
    sumHeadingIter = Ral_TupleHeadingBegin(sumTupleHeading) +
	Ral_RelationDegree(perRelation) ;
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
	Tcl_IncrRefCount(matchObj) ;

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

	    if (Tcl_ExprObj(interp, *v, &exprResult) != TCL_OK) {
		Ral_TupleDelete(sumTuple) ;
		Tcl_DecrRefCount(matchObj) ;
		goto errorOut ;
	    }
	    if (Ral_AttributeConvertValueToType(interp, *attrIter++,
		exprResult) != TCL_OK) {
		Ral_TupleDelete(sumTuple) ;
		Tcl_DecrRefCount(matchObj) ;
		Tcl_DecrRefCount(exprResult) ;
		goto errorOut ;
	    }
	    Tcl_IncrRefCount(*sumIter++ = exprResult) ;
	    Tcl_DecrRefCount(exprResult) ;
	}
	status = Ral_RelationPushBack(sumRelation, sumTuple, NULL) ;
	assert(status != 0) ;
	Tcl_DecrRefCount(matchObj) ;
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
	Ral_RelationObjSetError(interp, REL_DEGREE_TWO, Tcl_GetString(relObj)) ;
	return TCL_ERROR ;
    }

    thIter = Ral_TupleHeadingBegin(tupleHeading) ;
    if (!Ral_AttributeTypeEqual(*thIter, *(thIter + 1))) {
	Ral_RelationObjSetError(interp, REL_TYPE_MISMATCH,
	    Tcl_GetString(relObj)) ;
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
    Tcl_Obj *relObj ;
    Ral_Relation relation ;
    Tcl_Obj *attrObj ;
    const char *attrName ;
    Ral_Relation ungrpRel ;

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

    ungrpRel = Ral_RelationUngroup(relation, attrName) ;
    if (ungrpRel == NULL) {
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
	    "during ungroup operation") ;
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

    unionRel = Ral_RelationUnion(r1, r2) ;
    if (unionRel == NULL) {
	Ral_RelationObjSetError(interp, Ral_RelationLastError,
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
	    Ral_RelationObjSetError(interp, Ral_RelationLastError,
		Tcl_GetString(r2Obj)) ;
	    return TCL_ERROR ;
	}
    }

    Tcl_SetObjResult(interp, Ral_RelationObjNew(unionRel)) ;
    return TCL_OK ;
}
