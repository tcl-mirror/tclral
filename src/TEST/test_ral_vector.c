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
    Ral_IntVectorIter i ;
    int n ;

    logInfo("creating vector, \"v1\", of %d %d's", v1Size, v1Fill) ;
    v1 = Ral_IntVectorNew(v1Size, v1Fill) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("size = %d", Ral_IntVectorSize(v1)) ;
    logInfo("capacity = %d", Ral_IntVectorCapacity(v1)) ;
    logInfo("is empty? = %d", Ral_IntVectorEmpty(v1)) ;

    logInfo("filing vector, \"v1\", with %d's", v1FillOther) ;
    Ral_IntVectorFill(v1, 3) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("fetching element 2") ;
    n = Ral_IntVectorFetch(v1, 2) ;
    if (n == v1FillOther) {
	logPass("element 2 = %d", n) ;
    } else {
	logFail("element 2 = %d, expected %d", n, v1FillOther) ;
    }

    logInfo("storing %d into element 2", v1FillOther2) ;
    Ral_IntVectorStore(v1, 2, v1FillOther2) ;
    n = Ral_IntVectorFetch(v1, 2) ;
    if (n == v1FillOther2) {
	logPass("element 2 = %d", n) ;
    } else {
	logFail("element 2 = %d, expected %d", n, v1FillOther2) ;
    }

    logInfo("front element = %d", Ral_IntVectorFront(v1)) ;
    logInfo("back element = %d", Ral_IntVectorBack(v1)) ;

    logInfo("pushing 20 on the back") ;
    Ral_IntVectorPushBack(v1, 20) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("poping the back") ;
    Ral_IntVectorPopBack(v1) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("inserting 3 elements of value 2 at the beginning") ;
    Ral_IntVectorInsert(v1, Ral_IntVectorBegin(v1), 3, 2) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("inserting 5 elements of value 6 at the end") ;
    Ral_IntVectorInsert(v1, Ral_IntVectorEnd(v1), 5, 6) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("erasing 4 elements at offset 3") ;
    i = Ral_IntVectorBegin(v1) + 3 ;
    Ral_IntVectorErase(v1, i, i + 4) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("sorting") ;
    Ral_IntVectorSort(v1) ;
    Ral_IntVectorPrint(v1, Ral_IntVectorBegin(v1), stdout) ;

    logInfo("finding first element with value 6") ;
    i = Ral_IntVectorFind(v1, 6) ;
    logInfo("element at offset %d = %d", i - Ral_IntVectorBegin(v1), *i) ;

    v2 = Ral_IntVectorNew(0, 0) ;
    logInfo("copying to an empty vector") ;
    Ral_IntVectorCopy(v1, Ral_IntVectorBegin(v1), Ral_IntVectorEnd(v1),
	v2, Ral_IntVectorBegin(v2)) ;
    Ral_IntVectorPrint(v2, Ral_IntVectorEnd(v2), stdout) ;

    logInfo("is copy equal? = %d", Ral_IntVectorEqual(v1, v2)) ;

    Ral_IntVectorDelete(v1) ;
    Ral_IntVectorDelete(v2) ;

    logSummarize() ;
    exit(0) ;
}
