#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"
#include "ral_tuple.h"
#include "log.h"

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
    Ral_TupleHeading h1 ;
    Ral_Tuple t1 ;
    Ral_Tuple t2 ;
    Tcl_Obj *v1 ;
    int s1 ;

    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;

    strType = Tcl_GetObjType("string") ;
    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", strType) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", strType) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", strType) ;

    logInfo("creating tuple heading") ;
    h1 = Ral_TupleHeadingNew(3) ;

    Ral_TupleHeadingPushBack(h1, a1) ;
    Ral_TupleHeadingPushBack(h1, a2) ;
    Ral_TupleHeadingPushBack(h1, a3) ;

    logInfo("creating tuple") ;
    t1 = Ral_TupleNew(h1) ;

    logTest(Ral_TupleDegree(t1), 3) ;

    logInfo("setting value of \"attr1\"") ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("a1", -1)) ;
    logTest(s1, 1) ;

    logInfo("getting value of \"attr1\"") ;
    v1 = Ral_TupleGetAttrValue(t1, "attr1") ;
    logTest(strcmp(Tcl_GetString(v1), "a1"), 0) ;

    logInfo("setting value of \"attr2\"") ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("a2", -1)) ;
    logTest(s1, 1) ;

    logInfo("setting value of \"attr3\"") ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("a3", -1)) ;
    logTest(s1, 1) ;

    logInfo("printing tuple") ;
    Ral_TuplePrint(t1, "%s\n", stdout) ;

    logInfo("setting value of \"attr4\"") ;
    v1 = Tcl_NewStringObj("a4", -1) ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr4", v1) ;
    logTest(s1, 0) ;
    logTest(Ral_TupleLastError, TUP_UNKNOWN_ATTR) ;

    logInfo("printing tuple") ;
    Ral_TuplePrint(t1, "%s\n", stdout) ;

    logInfo("duplicating tuple") ;
    t2 = Ral_TupleDup(t1) ;
    logInfo("printing tuple copy") ;
    Ral_TuplePrint(t2, "%s\n", stdout) ;

    Ral_TupleDelete(t1) ;
    Ral_TupleDelete(t2) ;

    logSummarize() ;
    exit(0) ;
}
