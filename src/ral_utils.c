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

$RCSfile: ral_utils.c,v $
$Revision: 1.2 $
$Date: 2006/02/20 20:15:10 $

ABSTRACT:

MODIFICATION HISTORY:
$Log: ral_utils.c,v $
Revision 1.2  2006/02/20 20:15:10  mangoa01
Now able to convert strings to relations and vice versa including
tuple and relation valued attributes.

Revision 1.1  2005/12/27 23:17:19  mangoa01
Update to the new spilt out file structure.

 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_utils.h"
#include <string.h>

/*
MACRO DEFINITIONS
*/

/*
TYPE DEFINITIONS
*/

/*
EXTERNAL FUNCTION REFERENCES
*/

/*
FORWARD FUNCTION REFERENCES
*/

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static const char rcsid[] = "@(#) $RCSfile: ral_utils.c,v $ $Revision: 1.2 $" ;

/*
FUNCTION DEFINITIONS
*/

/*
 * Ral_ObjEqual

 * Compare two objects. The comparison is based on comparing the string
 * representation of the objects.

 * Returns 1 if the objects are equal, 0 otherwise.
 */

/*
 * There's a problem here. Tuples and relations cannot compared based
 * on their string representations.
 */

int
Ral_ObjEqual(
    Tcl_Obj *o1,	/* first object pointer to compare */
    Tcl_Obj *o2)	/* second object pointer to compare */
{
    const char *s1 ;
    int l1 ;
    const char *s2 ;
    int l2 ;

    if (o1 == o2) {
	return 1 ;
    }
    s1 = Tcl_GetStringFromObj(o1, &l1) ;
    s2 = Tcl_GetStringFromObj(o2, &l2) ;
    if (l1 != l2) {
	return 0 ;
    }
    return memcmp(s1, s2, l1) == 0 ;
}

void
Ral_ObjSetError(
    Tcl_Interp *interp,
    const char *cmdName,
    const char *errorResult,
    const char *errorCode,
    const char *param)
{
    if (interp) {
	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), errorResult,
	    ", \"", param, "\"", NULL) ;
	Tcl_SetErrorCode(interp, "RAL", cmdName, errorCode, param, NULL) ;
    }
}
