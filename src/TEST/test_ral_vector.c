#include <stdio.h>
#include <stdlib.h>
#include "ral_vector.h"
#include "log.h"

int
main(
    int argc,
    char **argv)
{
    const unsigned v1Size = 10 ;
    const unsigned v1Fill = 0 ;
    const unsigned v1FillOther = 3 ;
    const unsigned v1FillOther2 = 15 ;
    Ral_IntVector v1 ;
    Ral_IntVector v2 ;
    Ral_IntVector v3 ;
    Ral_IntVectorIter i ;

    logInfo("testing ral_vector version %s", Ral_IntVectorVersion()) ;

    logInfo("creating vector, \"v1\", of %d %d's", v1Size, v1Fill) ;
    v1 = Ral_IntVectorNew(v1Size, v1Fill) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;

    logTest(Ral_IntVectorSize(v1), v1Size) ;
    logTest(Ral_IntVectorCapacity(v1), v1Size) ;
    logTest(Ral_IntVectorEmpty(v1), 0) ;
    logTest(Ral_IntVectorFront(v1), v1Fill) ;

    logInfo("filing vector, \"v1\", with %d's", v1FillOther) ;
    Ral_IntVectorFill(v1, v1FillOther) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;

    logInfo("fetching element 2") ;
    logTest(Ral_IntVectorFetch(v1, 2), v1FillOther) ;

    logInfo("storing %d into element 2", v1FillOther2) ;
    Ral_IntVectorStore(v1, 2, v1FillOther2) ;
    logTest(Ral_IntVectorFetch(v1, 2), v1FillOther2) ;

    logTest(Ral_IntVectorFront(v1), v1FillOther) ;
    logTest(Ral_IntVectorBack(v1), v1FillOther) ;

    logInfo("pushing %d on the back", v1FillOther2) ;
    Ral_IntVectorPushBack(v1, v1FillOther2) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;
    logTest(Ral_IntVectorBack(v1), v1FillOther2) ;

    logInfo("poping the back") ;
    Ral_IntVectorPopBack(v1) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;
    logTest(Ral_IntVectorBack(v1), v1FillOther) ;

    logInfo("inserting 3 elements of value 2 at the beginning") ;
    Ral_IntVectorInsert(v1, Ral_IntVectorBegin(v1), 3, 2) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;
    logTest(Ral_IntVectorFetch(v1, 0), 2) ;
    logTest(Ral_IntVectorFetch(v1, 1), 2) ;
    logTest(Ral_IntVectorFetch(v1, 2), 2) ;

    logInfo("inserting 5 elements of value 6 at the end") ;
    Ral_IntVectorInsert(v1, Ral_IntVectorEnd(v1), 5, 6) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;
    i = Ral_IntVectorEnd(v1) - 5 ;
    logTest(*i, 6) ;
    ++i ;
    logTest(*i, 6) ;
    ++i ;
    logTest(*i, 6) ;
    ++i ;
    logTest(*i, 6) ;
    ++i ;
    logTest(*i, 6) ;

    logInfo("erasing 4 elements at offset 3") ;
    i = Ral_IntVectorBegin(v1) + 3 ;
    Ral_IntVectorErase(v1, i, i + 4) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;
    logTest(Ral_IntVectorFetch(v1, 5), v1FillOther) ;

    logInfo("sorting") ;
    Ral_IntVectorSort(v1) ;
    logInfo("v1 = \n%s", Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1))) ;

    logInfo("finding first element with value 6") ;
    logTest(Ral_IntVectorFind(v1, 6) - Ral_IntVectorBegin(v1), 9) ;
    logTest(*Ral_IntVectorFind(v1, 6), 6) ;

    logInfo("creating an empty vector") ;
    v2 = Ral_IntVectorNew(0, 0) ;
    logInfo("v2 = \n%s", Ral_IntVectorPrint(v2, Ral_IntVectorBegin(v2))) ;
    logInfo("copying to an empty vector") ;
    Ral_IntVectorCopy(v1, Ral_IntVectorBegin(v1), Ral_IntVectorEnd(v1),
	v2, Ral_IntVectorBegin(v2)) ;
    logInfo("v2 = \n%s", Ral_IntVectorPrint(v2, Ral_IntVectorBegin(v2))) ;
    logTest(Ral_IntVectorEqual(v1, v2), 1) ;

    v3 = Ral_IntVectorNew(0, 0) ;
    logTest(Ral_IntVectorSetAdd(v3, v1FillOther), 1) ;
    logTest(Ral_IntVectorSetAdd(v3, 6), 1) ;
    logInfo("v3 = \n%s", Ral_IntVectorPrint(v3, Ral_IntVectorBegin(v3))) ;
    logTest(Ral_IntVectorSubsetOf(v3, v1), 1) ;
    logTest(Ral_IntVectorSetAdd(v3, 6), 0) ;
    logTest(Ral_IntVectorSize(v3), 2) ;
    logTest(Ral_IntVectorSubsetOf(v3, v1), 1) ;
    logTest(Ral_IntVectorSetAdd(v3, 9), 1) ;
    logTest(Ral_IntVectorSubsetOf(v3, v1), 0) ;
    logInfo("v3 = \n%s", Ral_IntVectorPrint(v3, Ral_IntVectorBegin(v3))) ;

    Ral_IntVectorDelete(v1) ;
    Ral_IntVectorDelete(v2) ;
    Ral_IntVectorDelete(v3) ;

    logSummarize() ;
    exit(0) ;
}
