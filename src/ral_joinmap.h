/*
This software is copyrighted 2006 by G. Andrew Mangogna.  The following
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

$RCSfile: ral_joinmap.h,v $
$Revision: 1.6.2.1 $
$Date: 2009/01/19 01:45:46 $
 *--
 */
#ifndef _ral_joinmap_h_
#define _ral_joinmap_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_vector.h"

/*
MACRO DEFINITIONS
*/

/*
FORWARD CLASS REFERENCES
*/

/*
TYPE DECLARATIONS
*/

typedef int Ral_JoinMapComponentType ;

/*
 * Each mapping element is just a pair of integers.
 *
 * For attributes, the join map holds a vector of integer pairs.
 * The pairs represent mapping of an attribute index in the first
 * relation to an attribute index in the second relation. This is
 * one of the "join attributes".
 *
 * For tuples, the join map holds a vector of integer pairs also.
 * The pairs represent the mapping of a tuple in the first relation
 * that matches a tuple in the second relation across the join attributes.
 */
#define MAP_ELEMENT_COUNT 2
typedef struct {
    Ral_JoinMapComponentType m[MAP_ELEMENT_COUNT] ;
} Ral_JoinMapValueType ;

typedef Ral_JoinMapValueType *Ral_JoinMapIter ;

typedef struct Ral_JoinMap {
    Ral_JoinMapIter attrStart ;
    Ral_JoinMapIter attrFinish ;
    Ral_JoinMapIter attrEndStorage ;
    Ral_JoinMapIter tupleStart ;
    Ral_JoinMapIter tupleFinish ;
    Ral_JoinMapIter tupleEndStorage ;
} *Ral_JoinMap ;

/*
 * MEMBER ACCESS MACROS
 * N.B. many of these macros access their arguments multiple times.
 * No side effects for the arguments!
 */

#define Ral_JoinMapAttrBegin(m)     ((m)->attrStart)
#define Ral_JoinMapAttrEnd(m)       ((m)->attrFinish)
#define Ral_JoinMapAttrSize(m)      ((m)->attrFinish - (m)->attrStart)
#define Ral_JoinMapAttrCapacity(m)  ((m)->attrEndStorage - (m)->attrStart)
#define Ral_JoinMapAttrEmpty(m)     ((m)->attrFinish = (m)->attrStart)

#define Ral_JoinMapTupleBegin(m)    ((m)->tupleStart)
#define Ral_JoinMapTupleEnd(m)      ((m)->tupleFinish)
#define Ral_JoinMapTupleSize(m)     ((m)->tupleFinish - (m)->tupleStart)
#define Ral_JoinMapTupleCapacity(m) ((m)->tupleEndStorage - (m)->tupleStart)
#define Ral_JoinMapTupleEmpty(m)    ((m)->tupleFinish = (m)->tupleStart)

/*
FUNCTION DECLARATIONS
*/

extern Ral_JoinMap Ral_JoinMapNew(int, int) ;
extern void Ral_JoinMapDelete(Ral_JoinMap) ;
extern void Ral_JoinMapAttrReserve(Ral_JoinMap, int) ;
extern void Ral_JoinMapTupleReserve(Ral_JoinMap, int) ;
extern void Ral_JoinMapAddAttrMapping(Ral_JoinMap,
    Ral_JoinMapComponentType, Ral_JoinMapComponentType) ;
extern void Ral_JoinMapAddTupleMapping(Ral_JoinMap,
    Ral_JoinMapComponentType, Ral_JoinMapComponentType) ;

extern Ral_IntVector Ral_JoinMapGetAttr(Ral_JoinMap, int) ;
extern Ral_IntVector Ral_JoinMapAttrMap(Ral_JoinMap, int, int) ;
extern int Ral_JoinMapFindAttr(Ral_JoinMap, int, int) ;
extern void Ral_JoinMapSortAttr(Ral_JoinMap, int) ;
extern Ral_IntVector Ral_JoinMapTupleMap(Ral_JoinMap, int, int) ;
extern int Ral_JoinMapTupleCounts(Ral_JoinMap, int, Ral_IntVector) ;
extern Ral_IntVector Ral_JoinMapMatchingTupleSet(Ral_JoinMap, int, int) ;

#endif /* _ral_joinmap_h_ */
