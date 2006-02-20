#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"
#include "ral_relation.h"
#include "log.h"

int
main(
    int argc,
    char **argv)
{
    Tcl_Interp *interp ;
    Tcl_ObjType *strType ;
    Ral_TupleHeading th1 ;
    Ral_RelationHeading rh1 ;
    Ral_Relation r1 ;
    Ral_Tuple t1 ;
    Ral_IntVector v1 ;

    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;
    strType = Tcl_GetObjType("string") ;

    logInfo("creating relation") ;
    th1 = Ral_TupleHeadingNew(3) ;
    Ral_TupleHeadingPushBack(th1, Ral_AttributeNewTclType("attr1", strType)) ;
    Ral_TupleHeadingPushBack(th1, Ral_AttributeNewTclType("attr2", strType)) ;
    Ral_TupleHeadingPushBack(th1, Ral_AttributeNewTclType("attr3", strType)) ;
    rh1 = Ral_RelationHeadingNew(th1, 1) ;
    v1 = Ral_IntVectorNew(1, -1) ;
    Ral_IntVectorStore(v1, 0, Ral_TupleHeadingIndexOf(th1, "attr1")) ;
    logTest(Ral_RelationHeadingAddIdentifier(rh1, 0, v1), 1) ;
    r1 = Ral_RelationNew(rh1) ;

    logInfo("printing empty relation") ;
    Ral_RelationPrint(r1, "\"%s\"\n", stdout) ;

    logInfo("adding a tuple") ;
    t1 = Ral_TupleNew(th1) ;
    Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("foo1", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("foo2", -1)) ;
    Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("foo3", -1)) ;

    logTest(Ral_RelationPushBack(r1, t1), 1) ;
    logInfo("printing relation") ;
    Ral_RelationPrint(r1, "\"%s\"\n", stdout) ;

    Ral_RelationDelete(r1) ;

    logSummarize() ;
    exit(0) ;
}
