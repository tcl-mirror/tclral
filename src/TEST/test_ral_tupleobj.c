#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tcl.h"
#include "ral_tupleobj.h"
#include "log.h"

extern Tcl_ObjType tclStringType ;

int
main(
    int argc,
    char **argv)
{
    Tcl_Interp *interp ;
    Ral_Attribute a1 ;
    Ral_Attribute a2 ;
    Ral_Attribute a3 ;
    Ral_TupleHeading h1 ;
    Ral_Tuple t1 ;
    Tcl_Obj *t1obj ;
    Tcl_Obj *t2obj ;
    Tcl_Obj *t3obj ;
    int err ;
    Ral_ErrorInfo errInfo ;

    logInfo("version = %s", Ral_TupleObjVersion()) ;

    logInfo("creating interpreter") ;
    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;

    logInfo("registering type") ;
    Tcl_RegisterObjType(&Ral_TupleObjType) ;

    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", tclStringType.name) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", tclStringType.name) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", tclStringType.name) ;
    logInfo("creating tuple heading") ;
    h1 = Ral_TupleHeadingNew(3) ;

    Ral_TupleHeadingPushBack(h1, a1) ;
    Ral_TupleHeadingPushBack(h1, a2) ;
    Ral_TupleHeadingPushBack(h1, a3) ;

    logInfo("creating tuple") ;
    t1 = Ral_TupleNew(h1) ;
    logInfo("setting value of \"attr1\"") ;
    Ral_TupleUpdateAttrValue(t1, "attr1", Tcl_NewStringObj("a1", -1),
	&errInfo) ;
    logInfo("setting value of \"attr2\"") ;
    Ral_TupleUpdateAttrValue(t1, "attr2", Tcl_NewStringObj("a2", -1),
	&errInfo) ;
    logInfo("setting value of \"attr3\"") ;
    Ral_TupleUpdateAttrValue(t1, "attr3", Tcl_NewStringObj("a3", -1),
	&errInfo) ;

    logInfo("creating object from tuple") ;
    t1obj = Ral_TupleObjNew(t1) ;
    logInfo("tuple = \"%s\"", Tcl_GetString(t1obj)) ;

    logInfo("creating tuple object from a string") ;
    t2obj = Tcl_NewStringObj("Tuple {bttr1 string bttr2 int} {bttr1 \"foo bar\" bttr2 22}", -1) ;
    err = Tcl_ConvertToType(interp, t2obj, Tcl_GetObjType("Tuple")) ;
    if (err != TCL_OK) {
	logFail("failed to convert type") ;
    } else {
	logPass("converted string to tuple") ;
    }

    logInfo("recreating string representation") ;
    Tcl_InvalidateStringRep(t2obj) ;
    logInfo("tuple = \"%s\"", Tcl_GetString(t2obj)) ;

    logInfo("copying tuple obj") ;
    t3obj = Tcl_DuplicateObj(t2obj) ;
    logInfo("copied tuple = \"%s\"", Tcl_GetString(t3obj)) ;

    logInfo("deleting tuple obj") ;
    Tcl_DecrRefCount(t3obj) ;

    logSummarize() ;
    exit(0) ;
}
