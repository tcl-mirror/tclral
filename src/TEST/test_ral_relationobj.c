#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"
#include "ral_relationobj.h"
#include "log.h"

extern Tcl_ObjType tclStringType ;

int
main(
    int argc,
    char **argv)
{
    Tcl_Interp *interp ;
    Ral_TupleHeading th1 ;
    Ral_RelationHeading rh1 ;
    Ral_IntVector id1 ;
    Ral_Relation r1 ;
    Ral_Tuple t1 ;
    Tcl_Obj *r1Obj ;
    Tcl_Obj *r2Obj ;
    Tcl_Obj *r3Obj ;

    logInfo("version = %s", Ral_RelationObjVersion()) ;

    logInfo("creating interpreter") ;
    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;

    logInfo("registering type") ;
    Tcl_RegisterObjType(&Ral_RelationObjType) ;

    logInfo("creating tuple heading") ;
    th1 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(th1,
	Ral_AttributeNewTclType("attr1", &tclStringType)) ;
    Ral_TupleHeadingPushBack(th1,
	Ral_AttributeNewTclType("attr2", &tclStringType)) ;
    Ral_TupleHeadingPushBack(th1,
	Ral_AttributeNewTclType("attr3", &tclStringType)) ;

    logInfo("creating relation heading") ;
    rh1 = Ral_RelationHeadingNew(th1, 1) ;

    logInfo("adding identifier") ;
    id1 = Ral_IntVectorNew(1, -1) ;
    Ral_IntVectorStore(id1, 0, Ral_TupleHeadingIndexOf(th1, "attr1")) ;
    Ral_RelationHeadingAddIdentifier(rh1, 0, id1) ;

    logInfo("creating relation") ;
    r1 = Ral_RelationNew(rh1) ;
    Ral_RelationReserve(r1, 3) ;

    logInfo("adding first tuple") ;
    t1 = Ral_TupleNew(th1) ;
    Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("a1", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("a2", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("a3", -1)) ;
    logTest(Ral_RelationPushBack(r1, t1), 1) ;

    logInfo("adding second tuple") ;
    t1 = Ral_TupleNew(th1) ;
    Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("b1", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("b2", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("b3", -1)) ;
    logTest(Ral_RelationPushBack(r1, t1), 1) ;

    logInfo("adding third tuple") ;
    t1 = Ral_TupleNew(th1) ;
    Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("c1", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("c2", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("c3", -1)) ;
    logTest(Ral_RelationPushBack(r1, t1), 1) ;

    logInfo("creating object from tuple") ;
    r1Obj = Ral_RelationObjNew(r1) ;
    logInfo("relation 1 = \"%s\"", Tcl_GetString(r1Obj)) ;
    Tcl_DecrRefCount(r1Obj) ;

    logInfo("creating relation object from a string") ;
    r2Obj = Tcl_NewStringObj("Relation {bttr1 string bttr2 int} {bttr1} {{bttr1 a1 bttr2 20} {bttr1 a2 bttr2 40} {bttr1 a3 bttr2 60}}", -1) ;
    logTest(Tcl_ConvertToType(interp, r2Obj, Tcl_GetObjType("Relation")),
	TCL_OK) ;
    logInfo("recreating string representation") ;
    Tcl_InvalidateStringRep(r2Obj) ;
    logInfo("relation 2 = \"%s\"", Tcl_GetString(r2Obj)) ;
    logInfo("deleting relation obj") ;
    Tcl_DecrRefCount(r2Obj) ;

    logInfo("creating relation object with tuple valued attribute") ;
    r2Obj = Tcl_NewStringObj("Relation {a1 {Tuple {t1 string t2 string}}} {a1} {{a1 {t1 foo t2 bar}}}", -1) ;
    logTest(Tcl_ConvertToType(interp, r2Obj, Tcl_GetObjType("Relation")),
	TCL_OK) ;
    logInfo("recreating string representation") ;
    Tcl_InvalidateStringRep(r2Obj) ;
    logInfo("relation 2 = \"%s\"", Tcl_GetString(r2Obj)) ;
    logInfo("deleting relation obj") ;
    Tcl_DecrRefCount(r2Obj) ;

    logInfo("copying relation obj") ;
    r3Obj = Tcl_DuplicateObj(r2Obj) ;
    logInfo("copied tuple = \"%s\"", Tcl_GetString(r3Obj)) ;
    logInfo("deleting relation obj") ;
    Tcl_DecrRefCount(r3Obj) ;

    logSummarize() ;
    Tcl_DeleteInterp(interp) ;
    exit(0) ;
}
