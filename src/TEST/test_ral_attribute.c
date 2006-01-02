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
    Ral_Attribute a1 ;
    Ral_Attribute a2 ;
    Ral_Attribute a3 ;

    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", NULL) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", NULL) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", NULL) ;

    logSummarize() ;
    exit(0) ;
}
