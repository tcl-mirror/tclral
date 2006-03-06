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

$RCSfile: ral_tuple.h,v $
$Revision: 1.6 $
$Date: 2006/03/06 01:07:37 $
 *--
 */
#ifndef _ral_tuple_h_
#define _ral_tuple_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_attribute.h"
#include "ral_tupleheading.h"
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

/*
 * A Tuple type is a tuple heading combined with a list of values, one for each
 * attribute.
 *
 * The string representation of a "Tuple" is a specially formed list.  The list
 * consists of three elements:
 *
 * 1. The keyword "Tuple".
 *
 * 2. A heading definition.
 *
 * 3. A value definition.
 *
 * The keyword distinguishes the string as a Tuple.  The heading is as
 * described above.  The heading consists of a list Attribute Name and Data
 * Type pairs.  The value definition is also a list consisting of Attribute
 * Name / Attribute Value pairs.
 * E.G.
 *	{Tuple {Name string Street int Wage double}\
 *	{Name Andrew Street Blackwood Wage 5.74}}
 *
 * Internally tuples are reference counted. The same tuple is sometimes
 * referenced by many different relations.
 */

typedef Tcl_Obj **Ral_TupleIter ;
typedef struct Ral_Tuple {
    int refCount ;		/* Reference Count */
    Ral_TupleHeading heading ;	/* Pointer to Tuple heading */
    Ral_TupleIter values ;	/* Pointer to an array of values.
				 * The size of the array is the same as the
				 * degree of the heading */
} *Ral_Tuple ;

typedef enum Ral_TupleError {
    TUP_OK = 0,
    TUP_UNKNOWN_ATTR,
    TUP_HEADING_ERR,
    TUP_FORMAT_ERR,
    TUP_DUPLICATE_ATTR,
    TUP_BAD_VALUE,
    TUP_BAD_KEYWORD,
    TUP_WRONG_NUM_ATTRS,
    TUP_BAD_PAIRS_LIST,
} Ral_TupleError ;

/*
DATA DECLARATIONS
*/ 
extern Ral_TupleError Ral_TupleLastError ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_Tuple Ral_TupleNew(Ral_TupleHeading) ;
extern Ral_Tuple Ral_TupleSubset(Ral_Tuple, Ral_TupleHeading, Ral_IntVector) ;
extern void Ral_TupleDelete(Ral_Tuple) ;
extern void Ral_TupleReference(Ral_Tuple) ;
extern void Ral_TupleUnreference(Ral_Tuple) ;
extern int Ral_TupleDegree(Ral_Tuple) ;

extern Ral_TupleIter Ral_TupleBegin(Ral_Tuple) ;
extern Ral_TupleIter Ral_TupleEnd(Ral_Tuple) ;

extern int Ral_TupleEqual(Ral_Tuple, Ral_Tuple) ;
extern int Ral_TupleEqualValues(Ral_Tuple, Ral_Tuple) ;
extern int Ral_TupleUpdateAttrValue(Ral_Tuple,
    const char *, Tcl_Obj *) ;
extern Tcl_Obj *Ral_TupleGetAttrValue(Ral_Tuple, const char *) ;
extern int Ral_TupleCopy(Ral_Tuple, Ral_TupleHeadingIter,
    Ral_TupleHeadingIter, Ral_Tuple) ;
extern void Ral_TupleCopyValues(Ral_TupleIter, Ral_TupleIter, Ral_TupleIter) ;
extern Ral_Tuple Ral_TupleDup(Ral_Tuple) ;
extern Ral_Tuple Ral_TupleDupOrdered(Ral_Tuple, Ral_TupleHeading,
    Ral_IntVector) ;
extern int Ral_TupleScan(Ral_Tuple, Ral_AttributeTypeScanFlags *,
    Ral_AttributeValueScanFlags *) ;
extern int Ral_TupleConvert(Ral_Tuple, char *, Ral_AttributeTypeScanFlags *,
    Ral_AttributeValueScanFlags *) ;
extern int Ral_TupleScanValue(Ral_Tuple, Ral_AttributeTypeScanFlags *,
    Ral_AttributeValueScanFlags *) ;
extern int Ral_TupleConvertValue(Ral_Tuple, char *,
    Ral_AttributeTypeScanFlags *, Ral_AttributeValueScanFlags *) ;
extern void Ral_TuplePrint(Ral_Tuple, const char *, FILE *) ;
extern char *Ral_TupleStringOf(Ral_Tuple) ;
extern const char *Ral_TupleVersion(void) ;

#endif /* _ral_tuple_h_ */
