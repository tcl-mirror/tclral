/*
This software is copyrighted 2005 by G. Andrew Mangogna.  The following
terms apply to all files associated with the software unless explicitly
disclaimed in individual files.

The authors hereby grant permission to use, copy, modify, distribute,
and license this software and its documentation for any purpose, provided
that existing copyright notices are retained in all copies and that this
notice is included verbatim in any distributions. No written agreement,
license, or royalty fee is required for any of the authorized uses.
Modifications to this software may be copyrighted by their authors and
need not follow the licensing terms described here, provided that the
new terms are clearly indicated on the first page of each file where
they apply.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES
THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
OR MODIFICATIONS.

GOVERNMENT USE: If you are acquiring this software on behalf of the
U.S. government, the Government shall have only "Restricted Rights"
in the software and related documentation as defined in the Federal
Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
are acquiring the software on behalf of the Department of Defense,
the software shall be classified as "Commercial Computer Software"
and the Government shall have only "Restricted Rights" as defined in
Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing,
the authors grant the U.S. Government and others acting in its behalf
permission to use and distribute the software in accordance with the
terms specified in this license.
*/
/*
 *++
MODULE:

ABSTRACT:

$RCSfile: ral_vector.h,v $
$Revision: 1.8 $
$Date: 2006/04/09 22:15:58 $
 *--
 */
#ifndef _ral_vector_h_
#define _ral_vector_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <stdio.h>

/*
MACRO DEFINITIONS
*/

/*
FORWARD CLASS REFERENCES
*/

/*
TYPE DECLARATIONS
*/

typedef int Ral_IntVectorValueType ;
typedef Ral_IntVectorValueType *Ral_IntVectorIter ;
typedef struct Ral_IntVector {
    Ral_IntVectorIter start ;
    Ral_IntVectorIter finish ;
    Ral_IntVectorIter endStorage ;
} *Ral_IntVector ;

typedef void *Ral_PtrVectorValueType ;
typedef Ral_PtrVectorValueType *Ral_PtrVectorIter ;
typedef struct Ral_PtrVector {
    Ral_PtrVectorIter start ;
    Ral_PtrVectorIter finish ;
    Ral_PtrVectorIter endStorage ;
} *Ral_PtrVector ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_IntVector Ral_IntVectorNew(int, Ral_IntVectorValueType) ;
extern Ral_IntVector Ral_IntVectorNewEmpty(int) ;
extern Ral_IntVector Ral_IntVectorDup(Ral_IntVector) ;
extern void Ral_IntVectorDelete(Ral_IntVector) ;

extern Ral_IntVectorIter Ral_IntVectorBegin(Ral_IntVector) ;
extern Ral_IntVectorIter Ral_IntVectorEnd(Ral_IntVector) ;

extern int Ral_IntVectorSize(Ral_IntVector) ;
extern int Ral_IntVectorCapacity(Ral_IntVector) ;
extern void Ral_IntVectorReserve(Ral_IntVector, int) ;

extern int Ral_IntVectorEmpty(Ral_IntVector) ;
extern void Ral_IntVectorFill(Ral_IntVector, Ral_IntVectorValueType) ;
extern void Ral_IntVectorFillConsecutive(Ral_IntVector,
    Ral_IntVectorValueType) ;

extern Ral_IntVectorValueType Ral_IntVectorFetch(Ral_IntVector, int) ;
extern void Ral_IntVectorStore(Ral_IntVector, int,
    Ral_IntVectorValueType) ;
extern Ral_IntVectorValueType Ral_IntVectorFront(Ral_IntVector) ;
extern Ral_IntVectorValueType Ral_IntVectorBack(Ral_IntVector) ;
extern void Ral_IntVectorPushBack(Ral_IntVector, Ral_IntVectorValueType) ;
extern void Ral_IntVectorPopBack(Ral_IntVector) ;
extern Ral_IntVectorIter Ral_IntVectorInsert(Ral_IntVector, Ral_IntVectorIter,
    int, Ral_IntVectorValueType) ;
extern Ral_IntVectorIter Ral_IntVectorErase(Ral_IntVector, Ral_IntVectorIter,
    Ral_IntVectorIter) ;

extern int Ral_IntVectorSetAdd(Ral_IntVector, Ral_IntVectorValueType) ;
extern Ral_IntVector Ral_IntVectorSetComplement(Ral_IntVector, int) ;
extern Ral_IntVector Ral_IntVectorBooleanMap(Ral_IntVector, int) ;

extern void Ral_IntVectorSort(Ral_IntVector) ;
extern Ral_IntVectorIter Ral_IntVectorFind(Ral_IntVector,
    Ral_IntVectorValueType) ;
extern int Ral_IntVectorIndexOf(Ral_IntVector, Ral_IntVectorValueType) ;
extern Ral_IntVector Ral_IntVectorIntersect(Ral_IntVector, Ral_IntVector) ;
extern Ral_IntVector Ral_IntVectorMinus(Ral_IntVector, Ral_IntVector) ;
extern int Ral_IntVectorEqual(Ral_IntVector, Ral_IntVector) ;
extern int Ral_IntVectorSubsetOf(Ral_IntVector, Ral_IntVector) ;
extern int Ral_IntVectorContainsAny(Ral_IntVector, Ral_IntVector) ;
extern Ral_IntVectorIter Ral_IntVectorCopy(Ral_IntVector, Ral_IntVectorIter,
    Ral_IntVectorIter, Ral_IntVector, Ral_IntVectorIter) ;

extern const char *Ral_IntVectorPrint(Ral_IntVector, Ral_IntVectorIter) ;

extern Ral_PtrVector Ral_PtrVectorNew(int) ;
extern Ral_PtrVector Ral_PtrVectorDup(Ral_PtrVector) ;
extern void Ral_PtrVectorDelete(Ral_PtrVector) ;

extern Ral_PtrVectorIter Ral_PtrVectorBegin(Ral_PtrVector) ;
extern Ral_PtrVectorIter Ral_PtrVectorEnd(Ral_PtrVector) ;

extern int Ral_PtrVectorSize(Ral_PtrVector) ;
extern int Ral_PtrVectorCapacity(Ral_PtrVector) ;
extern void Ral_PtrVectorReserve(Ral_PtrVector, int) ;

extern int Ral_PtrVectorEmpty(Ral_PtrVector) ;
extern void Ral_PtrVectorFill(Ral_PtrVector, Ral_PtrVectorValueType) ;

extern Ral_PtrVectorValueType Ral_PtrVectorFetch(Ral_PtrVector, int) ;
extern void Ral_PtrVectorStore(Ral_PtrVector, int,
    Ral_PtrVectorValueType) ;
extern Ral_PtrVectorValueType Ral_PtrVectorFront(Ral_PtrVector) ;
extern Ral_PtrVectorValueType Ral_PtrVectorBack(Ral_PtrVector) ;
extern void Ral_PtrVectorPushBack(Ral_PtrVector, Ral_PtrVectorValueType) ;
extern void Ral_PtrVectorPopBack(Ral_PtrVector) ;
extern Ral_PtrVectorIter Ral_PtrVectorInsert(Ral_PtrVector, Ral_PtrVectorIter,
    int, Ral_PtrVectorValueType) ;
extern Ral_PtrVectorIter Ral_PtrVectorErase(Ral_PtrVector, Ral_PtrVectorIter,
    Ral_PtrVectorIter) ;

extern int Ral_PtrVectorSetAdd(Ral_PtrVector, Ral_PtrVectorValueType) ;

extern void Ral_PtrVectorSort(Ral_PtrVector) ;
extern Ral_PtrVectorIter Ral_PtrVectorFind(Ral_PtrVector,
    Ral_PtrVectorValueType) ;
extern int Ral_PtrVectorEqual(Ral_PtrVector, Ral_PtrVector) ;
extern int Ral_PtrVectorSubsetOf(Ral_PtrVector, Ral_PtrVector) ;
extern Ral_PtrVectorIter Ral_PtrVectorCopy(Ral_PtrVector, Ral_PtrVectorIter,
    Ral_PtrVectorIter, Ral_PtrVector, Ral_PtrVectorIter) ;

extern const char *Ral_PtrVectorPrint(Ral_PtrVector, Ral_PtrVectorIter) ;


extern const char *Ral_VectorVersion(void) ;


#endif /* _ral_vector_h_ */
