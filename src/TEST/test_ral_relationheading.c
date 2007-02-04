#include <stdio.h>
#include <stdlib.h>
#include "tcl.h"
#include "ral_relationheading.h"
#include "log.h"

extern Tcl_ObjType tclStringType ;

int
main(
    int argc,
    char **argv)
{
    Tcl_Interp *interp ;
    Tcl_ObjType *strType ;

    Ral_IntVector v1 ;
    Ral_IntVector v2 ;
    Ral_IntVector v3 ;
    Ral_TupleHeading t1 ;
    Ral_TupleHeading t2 ;
    Ral_TupleHeading t3 ;
    Ral_RelationHeading r1 ;
    Ral_RelationHeading r2 ;
    Ral_RelationHeading r3 ;

    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;
    strType = Tcl_GetObjType("string") ;

    logInfo("version: %s", Ral_RelationHeadingVersion()) ;

    logInfo("creating first relation heading") ;
    t1 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(t1, Ral_AttributeNewTclType("attr1",
	strType->name)) ;
    Ral_TupleHeadingPushBack(t1, Ral_AttributeNewTclType("attr2",
	strType->name)) ;
    Ral_TupleHeadingPushBack(t1, Ral_AttributeNewTclType("attr3",
	strType->name)) ;
    r1 = Ral_RelationHeadingNew(t1, 1) ;
    v1 = Ral_IntVectorNew(1, -1) ;
    Ral_IntVectorStore(v1, 0, Ral_TupleHeadingIndexOf(t1, "attr1")) ;
    logTest(Ral_RelationHeadingAddIdentifier(r1, 0, v1), 1) ;

    logInfo("printing first relation heading") ;
    Ral_RelationHeadingPrint(r1, "%s\n", stdout) ;

    logInfo("creating second relation heading") ;
    t2 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(t2, Ral_AttributeNewTclType("attr2",
	strType->name)) ;
    Ral_TupleHeadingPushBack(t2, Ral_AttributeNewTclType("attr3",
	strType->name)) ;
    Ral_TupleHeadingPushBack(t2, Ral_AttributeNewTclType("attr1",
	strType->name)) ;
    r2 = Ral_RelationHeadingNew(t2, 1) ;
    v2 = Ral_IntVectorNew(1, -1) ;
    Ral_IntVectorStore(v2, 0, Ral_TupleHeadingIndexOf(t2, "attr1")) ;
    logTest(Ral_RelationHeadingAddIdentifier(r2, 0, v2), 1) ;
    logInfo("testing for heading equality") ;
    logTest(Ral_RelationHeadingEqual(r1,r2), 1) ;

    logInfo("printing second relation heading") ;
    Ral_RelationHeadingPrint(r2, "%s\n", stdout) ;

    logInfo("creating third relation heading") ;
    t3 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(t3, Ral_AttributeNewTclType("attr3",
	strType->name)) ;
    Ral_TupleHeadingPushBack(t3, Ral_AttributeNewTclType("attr1",
	strType->name)) ;
    Ral_TupleHeadingPushBack(t3, Ral_AttributeNewTclType("attr2",
	strType->name)) ;
    r3 = Ral_RelationHeadingNew(t3, 1) ;

    v3 = Ral_IntVectorNew(2, -1) ;
    Ral_IntVectorStore(v3, 0, Ral_TupleHeadingIndexOf(t3, "attr2")) ;
    Ral_IntVectorStore(v3, 1, Ral_TupleHeadingIndexOf(t3, "attr3")) ;

    logTest(Ral_RelationHeadingAddIdentifier(r3, 0, v3), 1) ;
    logInfo("testing for heading inequality") ;
    logTest(Ral_RelationHeadingEqual(r1,r3), 0) ;

    logInfo("printing third relation heading") ;
    Ral_RelationHeadingPrint(r3, "%s\n", stdout) ;

    Ral_RelationHeadingUnreference(r1) ;
    Ral_RelationHeadingUnreference(r2) ;
    Ral_RelationHeadingUnreference(r3) ;

    logSummarize() ;
    exit(0) ;
}
