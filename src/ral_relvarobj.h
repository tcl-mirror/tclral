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

$RCSfile: ral_relvarobj.h,v $
$Revision: 1.6 $
$Date: 2006/06/24 18:07:38 $
 *--
 */
#ifndef _ral_relvarobj_h_
#define _ral_relvarobj_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_relvar.h"
#include "ral_relationheading.h"

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
EXTERNAL DATA REFERENCES
*/

/*
FUNCTION DECLARATIONS
*/
extern int Ral_RelvarObjNew(Tcl_Interp *, Ral_RelvarInfo, const char *,
    Ral_RelationHeading) ;
extern int Ral_RelvarObjDelete(Tcl_Interp *, Ral_RelvarInfo, Tcl_Obj *) ;
extern Ral_Relvar Ral_RelvarObjFindRelvar(Tcl_Interp *, Ral_RelvarInfo,
    const char *, char **) ;
extern int Ral_RelvarObjCreateAssoc(Tcl_Interp *, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjCreatePartition(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintDelete(Tcl_Interp *, const char *,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintInfo(Tcl_Interp *, Tcl_Obj * const,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintNames(Tcl_Interp *, const char *,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjEndTrans(Tcl_Interp *, Ral_RelvarInfo, int) ;
extern int Ral_RelvarObjEndCmd(Tcl_Interp *, Ral_RelvarInfo, int) ;

#endif /* _ral_relvarobj_h_ */