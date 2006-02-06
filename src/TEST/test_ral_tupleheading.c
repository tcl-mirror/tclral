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
    char *headingString ;

    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;
    strType = Tcl_GetObjType("string") ;

    logInfo("version: %s", Ral_TupleHeadingVersion()) ;
    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", strType) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", strType) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", strType) ;

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

    headingString = Ral_TupleHeadingToString(v1) ;
    logInfo("v1 as string = \"%s\"", headingString) ;
    ckfree(headingString) ;

    logInfo("creating \"attr1\"") ;
    a4 = Ral_AttributeNewTclType("attr1", strType) ;
    i1 = Ral_TupleHeadingStore(v1, Ral_TupleHeadingBegin(v1) + 1, a4) ;
    logTest(i1 == Ral_TupleHeadingEnd(v1), 1)

    /*
     * Test union.
     */
    v2 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(v2, Ral_AttributeNewTclType("b1", strType)) ;
    Ral_TupleHeadingPushBack(v2, Ral_AttributeNewTclType("b2", strType)) ;
    Ral_TupleHeadingPushBack(v2, Ral_AttributeNewTclType("b3", strType)) ;
    logTest(Ral_TupleHeadingSize(v2), 3) ;

    v3 = Ral_TupleHeadingUnion(v1, v2) ;
    logTest(v3 != NULL, 1) ;
    logTest(Ral_TupleHeadingSize(v3), vcapacity + 3) ;
    headingString = Ral_TupleHeadingToString(v3) ;
    logInfo("v3 as string = \"%s\"", headingString) ;
    ckfree(headingString) ;

    /*
     * Test intersect.
     */
    v4 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(v4, Ral_AttributeNewTclType("a1", strType)) ;
    Ral_TupleHeadingPushBack(v4, Ral_AttributeNewTclType("b1", strType)) ;
    Ral_TupleHeadingPushBack(v4, Ral_AttributeNewTclType("b3", strType)) ;
    logTest(Ral_TupleHeadingSize(v4), 3) ;

    v5 = Ral_TupleHeadingIntersect(v2, v4) ;
    logTest(v5 != NULL, 1) ;
    logTest(Ral_TupleHeadingSize(v5), 2) ;
    headingString = Ral_TupleHeadingToString(v5) ;
    logInfo("v5 as string = \"%s\"", headingString) ;
    ckfree(headingString) ;

    Ral_TupleHeadingUnreference(v1) ;
    Ral_TupleHeadingUnreference(v2) ;
    Ral_TupleHeadingUnreference(v3) ;
    Ral_TupleHeadingUnreference(v4) ;
    Ral_TupleHeadingUnreference(v5) ;

    logSummarize() ;
    exit(0) ;
}
