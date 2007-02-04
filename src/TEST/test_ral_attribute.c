#include <stdio.h>
#include <stdlib.h>
#include "tcl.h"
#include "ral_attribute.h"
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
    Tcl_Obj *attrName ;
    Tcl_Obj *attrType ;
    Tcl_Obj *attrValue ;
    char * a2Str ;
    Ral_AttributeTypeScanFlags typeFlags ;
    Ral_AttributeValueScanFlags valueFlags ;
    char * aValueStr ;
    Ral_ErrorInfo errInfo ;

    interp = Tcl_CreateInterp() ;
    Tcl_InitMemory(interp) ;
    strType = Tcl_GetObjType("string") ;

    logInfo("testing ral_attribute version: %s", Ral_AttributeVersion()) ;
    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", strType->name) ;
    logTest(a1->attrType, Tcl_Type) ;

    attrName = Tcl_NewStringObj("a2", -1) ;
    attrType = Tcl_NewStringObj("string", -1) ;
    attrValue = Tcl_NewStringObj("This is a value", -1) ;
    a2 = Ral_AttributeNewFromObjs(interp, attrName, attrType, &errInfo) ;
    a2Str = Ral_AttributeToString(a2) ;
    logInfo("attribute created from objs = \"%s\"", a2Str) ;
    ckfree(a2Str) ;

    Ral_AttributeScanName(a2, &typeFlags) ;
    Ral_AttributeScanType(a2, &typeFlags) ;
    aValueStr = ckalloc(Ral_AttributeScanValue(a2, attrValue, &typeFlags,
	&valueFlags)) ;
    Ral_AttributeConvertValue(a2, attrValue, aValueStr, &typeFlags,
	&valueFlags) ;
    Ral_AttributeTypeScanFlagsFree(&typeFlags) ;
    Ral_AttributeValueScanFlagsFree(&valueFlags) ;
    logInfo("attribute value = \"%s\"", aValueStr) ;
    ckfree(aValueStr) ;

    a3 = Ral_AttributeDup(a1) ;
    logTest(Ral_AttributeEqual(a1, a3), 1) ;

    Ral_AttributeDelete(a1) ;
    Ral_AttributeDelete(a2) ;
    Ral_AttributeDelete(a3) ;
    logSummarize() ;
    exit(0) ;
}
