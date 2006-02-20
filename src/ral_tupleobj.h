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

$RCSfile: ral_tupleobj.h,v $
$Revision: 1.4 $
$Date: 2006/02/20 20:15:10 $
 *--
 */
#ifndef _ral_tupleobj_h_
#define _ral_tupleobj_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_tupleheading.h"
#include "ral_tuple.h"

/*
MACRO DEFINITIONS
*/

/*
FORWARD CLASS REFERENCES
*/

/*
TYPE DECLARATIONS
*/
typedef enum Ral_TupleError {
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
EXTERNAL DATA REFERENCES
*/
extern Tcl_ObjType Ral_TupleObjType ;

/*
FUNCTION DECLARATIONS
*/

extern Tcl_Obj *Ral_TupleObjNew(Ral_Tuple) ;
extern int Ral_TupleObjConvert(Ral_TupleHeading, Tcl_Interp *, Tcl_Obj *,
    Tcl_Obj *) ;
extern Ral_TupleHeading Ral_TupleHeadingNewFromObj(Tcl_Interp *, Tcl_Obj *) ;
extern int Ral_TupleSetFromObj(Ral_Tuple, Tcl_Interp *, Tcl_Obj *) ;
extern int Ral_TupleUpdateFromObj(Ral_Tuple, Tcl_Interp *, Tcl_Obj *) ;
extern int Ral_TupleUpdateAttrFromObj(Ral_Tuple, Tcl_Interp *, Tcl_Obj *,
    Tcl_Obj *) ;
extern const char *Ral_TupleObjVersion(void) ;
extern void Ral_TupleObjSetError(Tcl_Interp *, Ral_TupleError, const char *) ;

#endif /* _ral_tupleobj_h_ */
