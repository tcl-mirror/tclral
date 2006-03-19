/*
This software is copyrighted 2004 by G. Andrew Mangogna.  The following
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
    ral.c -- source for the TclRAL Relational Algebra library.

ABSTRACT:
    This file contains the "C" source to an extension of the TCL
    language that provides an implementation of the Relational
    Algebra.

$RCSfile: ral.c,v $
$Revision: 1.26 $
$Date: 2006/03/19 19:48:31 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"
#include "ral_tuplecmd.h"
#include "ral_relationcmd.h"

/*
 * We use Tcl_CreateNamespace() and Tcl_Export().
 * Before 8.4, they not part of the supported external interface.
 */
#if TCL_MINOR_VERSION <= 4
#   include "tclInt.h"
#endif

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
STATIC DATA ALLOCATION
*/
static const char ral_pkgname[] = "ral" ;
static const char ral_version[] = "0.8" ;
static const char ral_rcsid[] =
    "$Id: ral.c,v 1.26 2006/03/19 19:48:31 mangoa01 Exp $" ;
static const char ral_copyright[] =
    "This software is copyrighted 2004, 2005, 2006 by G. Andrew Mangogna."
    "Terms and conditions for use are distributed with the source code." ;

/*
FUNCTION DEFINITIONS
*/

/*
 * ======================================================================
 * Initialization Function
 * ======================================================================
 */

int
Ral_Init(
    Tcl_Interp * interp)
{
    Tcl_Namespace *ralNs ;

    Tcl_InitStubs(interp, TCL_VERSION, 0) ;

    Tcl_RegisterObjType(&Ral_TupleObjType) ;

    ralNs = Tcl_CreateNamespace(interp, "::ral", NULL, NULL) ;

    Tcl_CreateObjCommand(interp, "::ral::tuple", tupleCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, "tuple", 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    Tcl_CreateObjCommand(interp, "::ral::relation", relationCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, "relation", 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    Tcl_PkgProvide(interp, ral_pkgname, ral_version) ;

    return TCL_OK ;
}
