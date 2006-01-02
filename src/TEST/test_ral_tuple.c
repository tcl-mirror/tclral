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
    Ral_Attribute a1 ;
    Ral_Attribute a2 ;
    Ral_Attribute a3 ;
    Ral_TupleHeading h1 ;
    Ral_Tuple t1 ;
    Ral_Tuple t2 ;
    Tcl_Obj *v1 ;
    Ral_TupleUpdateStatus s1 ;

    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", NULL) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", NULL) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", NULL) ;

    logInfo("creating tuple heading") ;
    h1 = Ral_TupleHeadingNew(3) ;

    Ral_TupleHeadingPushBack(h1, a1) ;
    Ral_TupleHeadingPushBack(h1, a2) ;
    Ral_TupleHeadingPushBack(h1, a3) ;

    logInfo("creating tuple") ;
    t1 = Ral_TupleNew(h1) ;

    if (Ral_TupleDegree(t1) == 3) {
	logPass("tuple has degree 3") ;
    } else {
	logFail("expected tuple to have degree 3") ;
    }

    logInfo("printing tuple") ;
    Ral_TuplePrint(t1, stdout) ;

    logInfo("setting value of \"attr1\"") ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("a1", -1)) ;
    if (s1 != AttributeUpdated) {
	logFail("failed to update attribute, \"attr1\"") ;
    }
    logInfo("getting value of \"attr1\"") ;
    v1 = Ral_TupleGetAttrValue(t1, "attr1") ;
    if (strcmp(Tcl_GetString(v1), "a1") == 0) {
	logPass("attribute, \"%s\" = \"%s\"", "attr1", "a1") ;
    } else {
	logFail("cannot retrieve attribute, \"attr1\"") ;
    }

    logInfo("setting value of \"attr2\"") ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("a2", -1)) ;
    if (s1 != AttributeUpdated) {
	logFail("failed to update attribute, \"attr2\"") ;
    }
    logInfo("setting value of \"attr3\"") ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("a3", -1)) ;
    if (s1 != AttributeUpdated) {
	logFail("failed to update attribute, \"attr3\"") ;
    }

    logInfo("setting value of \"attr4\"") ;
    v1 = Tcl_NewStringObj("a4", -1) ;
    s1 = Ral_TupleUpdateAttrValue(t1, "attr4", v1) ;
    if (s1 == NoSuchAttribute) {
	logPass("no attribute, \"attr4\"") ;
	Tcl_DecrRefCount(v1) ;
    } else {
	logFail("expected to fail to update attribute, \"attr4\"") ;
    }

    logInfo("printing tuple") ;
    Ral_TuplePrint(t1, stdout) ;

    logInfo("duplicating tuple") ;
    t2 = Ral_TupleDuplicate(t1) ;
    logInfo("printing tuple copy") ;
    Ral_TuplePrint(t2, stdout) ;

    Ral_TupleDelete(t1) ;
    Ral_TupleDelete(t2) ;

    logSummarize() ;
    exit(0) ;
}
