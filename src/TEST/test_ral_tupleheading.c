#include <stdio.h>
#include <stdlib.h>
#include "tcl.h"
#include "ral_tupleheading.h"
#include "log.h"

extern Tcl_ObjType tclStringType ;

int
main(
    int argc,
    char **argv)
{
    Tcl_Interp *interp ;
    Tcl_ObjType *strType ;

    Ral_Attribute a1 ;
    Ral_Attribute a2 ;
    Ral_Attribute a3 ;
    Ral_Attribute a4 ;
    unsigned vcapacity = 2 ;
    Ral_TupleHeading v1 ;
    Ral_TupleHeading v2 ;
    Ral_TupleHeading v3 ;
    Ral_TupleHeading v4 ;
    Ral_TupleHeading v5 ;
    Ral_TupleHeadingIter i1 ;

    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;
    strType = Tcl_GetObjType("string") ;

    logInfo("version: %s", Ral_TupleHeadingVersion()) ;
    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", strType->name) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", strType->name) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", strType->name) ;

    logInfo("creating attribute vector") ;
    v1 = Ral_TupleHeadingNew(vcapacity) ;
    logTest(Ral_TupleHeadingSize(v1), 0) ;
    logTest(Ral_TupleHeadingCapacity(v1), vcapacity) ;
    logTest(Ral_TupleHeadingEmpty(v1), 1) ;

    Ral_TupleHeadingPushBack(v1, a1) ;
    logTest(Ral_TupleHeadingSize(v1), 1) ;
    logTest(Ral_TupleHeadingEmpty(v1), 0) ;

    Ral_TupleHeadingPushBack(v1, a2) ;
    logTest(Ral_TupleHeadingSize(v1), 2) ;
    logTest(Ral_TupleHeadingEmpty(v1), 0) ;

    logTest(Ral_TupleHeadingFind(v1, "attr1") != Ral_TupleHeadingEnd(v1), 1) ;
    logTest(Ral_TupleHeadingFind(v1, "foo") == Ral_TupleHeadingEnd(v1), 1) ;
    logTest(Ral_TupleHeadingStore(v1, Ral_TupleHeadingBegin(v1) + 1, a3) 
	!= Ral_TupleHeadingEnd(v1), 1) ;

    logInfo("printing v1") ;
    Ral_TupleHeadingPrint(v1, "%s\n", stdout) ;

    logInfo("creating \"attr1\"") ;
    a4 = Ral_AttributeNewTclType("attr1", strType->name) ;
    i1 = Ral_TupleHeadingStore(v1, Ral_TupleHeadingBegin(v1) + 1, a4) ;
    logTest(i1 == Ral_TupleHeadingEnd(v1), 1)

    /*
     * Test union.
     */
    v2 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(v2, Ral_AttributeNewTclType("b1", strType->name)) ;
    Ral_TupleHeadingPushBack(v2, Ral_AttributeNewTclType("b2", strType->name)) ;
    Ral_TupleHeadingPushBack(v2, Ral_AttributeNewTclType("b3", strType->name)) ;
    logTest(Ral_TupleHeadingSize(v2), 3) ;

    v3 = Ral_TupleHeadingUnion(v1, v2) ;
    logTest(v3 != NULL, 1) ;
    logTest(Ral_TupleHeadingSize(v3), vcapacity + 3) ;

    logInfo("printing v3") ;
    Ral_TupleHeadingPrint(v3, "%s\n", stdout) ;

    /*
     * Test intersect.
     */
    v4 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(v4, Ral_AttributeNewTclType("a1", strType->name)) ;
    Ral_TupleHeadingPushBack(v4, Ral_AttributeNewTclType("b1", strType->name)) ;
    Ral_TupleHeadingPushBack(v4, Ral_AttributeNewTclType("b3", strType->name)) ;
    logTest(Ral_TupleHeadingSize(v4), 3) ;

    v5 = Ral_TupleHeadingIntersect(v2, v4) ;
    logTest(v5 != NULL, 1) ;
    logTest(Ral_TupleHeadingSize(v5), 2) ;
    logInfo("printing v5") ;
    Ral_TupleHeadingPrint(v5, "%s\n", stdout) ;

    Ral_TupleHeadingUnreference(v1) ;
    Ral_TupleHeadingUnreference(v2) ;
    Ral_TupleHeadingUnreference(v3) ;
    Ral_TupleHeadingUnreference(v4) ;
    Ral_TupleHeadingUnreference(v5) ;

    logSummarize() ;
    exit(0) ;
}
