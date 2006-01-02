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
    Ral_Attribute a1 ;
    Ral_Attribute a2 ;
    Ral_Attribute a3 ;
    Ral_Attribute a4 ;
    unsigned vcapacity = 2 ;
    unsigned vsize ;
    unsigned cap ;
    int emptyState ;
    Ral_TupleHeading v1 ;
    Ral_TupleHeadingIter i1 ;
    char *headingString ;

    logInfo("creating \"attr1\"") ;
    a1 = Ral_AttributeNewTclType("attr1", &tclStringType) ;
    logInfo("creating \"attr2\"") ;
    a2 = Ral_AttributeNewTclType("attr2", &tclStringType) ;
    logInfo("creating \"attr3\"") ;
    a3 = Ral_AttributeNewTclType("attr3", &tclStringType) ;

    logInfo("creating attribute vector") ;
    v1 = Ral_TupleHeadingNew(vcapacity) ;
    vsize = Ral_TupleHeadingSize(v1) ;
    if (vsize == 0) {
	logPass("size of vector = %u", vsize) ;
    } else {
	logFail("size of vector = %u, expected 0", vsize) ;
    }
    cap = Ral_TupleHeadingCapacity(v1) ;
    if (cap == vcapacity) {
	logPass("capacity of vector = %u", cap) ;
    } else {
	logFail("capacity of vector = %u, expected %u", cap, vcapacity) ;
    }
    emptyState = Ral_TupleHeadingEmpty(v1) ;
    if (emptyState) {
	logPass("vector is empty") ;
    } else {
	logFail("vector is not empty, expected empty") ;
    }

    Ral_TupleHeadingPushBack(v1, a1) ;
    vsize = Ral_TupleHeadingSize(v1) ;
    if (vsize == 1) {
	logPass("size of vector = %u", vsize) ;
    } else {
	logFail("size of vector = %u, expected 1", vsize) ;
    }
    emptyState = Ral_TupleHeadingEmpty(v1) ;
    if (emptyState) {
	logFail("vector is empty, expected not empty") ;
    } else {
	logPass("vector is not empty") ;
    }

    Ral_TupleHeadingPushBack(v1, a2) ;
    vsize = Ral_TupleHeadingSize(v1) ;
    if (vsize == 2) {
	logPass("size of vector = %u", vsize) ;
    } else {
	logFail("size of vector = %u, expected 2", vsize) ;
    }
    emptyState = Ral_TupleHeadingEmpty(v1) ;
    if (emptyState) {
	logFail("vector is empty, expected not empty") ;
    } else {
	logPass("vector is not empty") ;
    }

    i1 = Ral_TupleHeadingFind(v1, "attr1") ;
    if (i1 != Ral_TupleHeadingEnd(v1)) {
	logPass("found attribute named, \"%s\"", (*i1)->name) ;
    } else {
	logFail("cannot find attribute, \"attr1\"") ;
    }

    i1 = Ral_TupleHeadingFind(v1, "foo") ;
    if (i1 != Ral_TupleHeadingEnd(v1)) {
	logFail("found attribute named, \"%s\"", (*i1)->name) ;
    } else {
	logPass("no such attribute, \"foo\"") ;
    }

    i1 = Ral_TupleHeadingStore(v1, Ral_TupleHeadingBegin(v1) + 1, a3) ;
    if (i1 != Ral_TupleHeadingEnd(v1)) {
	logPass("stored attribute named, \"%s\"", (*i1)->name) ;
    } else {
	logFail("cannot store attribute, \"attr3\"") ;
    }

    headingString = Ral_TupleHeadingToString(v1) ;
    logInfo("v1 as string = \"%s\"", headingString) ;
    ckfree(headingString) ;

    Ral_TupleHeadingPrint(v1, Ral_TupleHeadingBegin(v1), stdout) ;

    logInfo("creating \"attr1\"") ;
    a4 = Ral_AttributeNewTclType("attr1", &tclStringType) ;
    i1 = Ral_TupleHeadingStore(v1, Ral_TupleHeadingBegin(v1) + 1, a4) ;
    if (i1 != Ral_TupleHeadingEnd(v1)) {
	logFail("stored attribute named, \"%s\"", (*i1)->name) ;
    } else {
	logPass("cannot store duplicate attribute, \"attr1\"") ;
	Ral_AttributeDelete(a4) ;
    }

    Ral_TupleHeadingDelete(v1) ;

    logSummarize() ;
    exit(0) ;
}
