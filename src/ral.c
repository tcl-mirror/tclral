/*
This software is copyrighted 2004, 2005, 2006 by G. Andrew Mangogna.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files.

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
$Revision: 1.37 $
$Date: 2007/06/09 22:35:44 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <stdio.h>
#include <string.h>
#include "tcl.h"
#include "ral_tuplecmd.h"
#include "ral_relationcmd.h"
#include "ral_relationobj.h"
#include "ral_relvar.h"
#include "ral_relvarobj.h"
#include "ral_relvarcmd.h"

/*
 * We use Tcl_CreateNamespace() and Tcl_Export().
 * Before 8.5, they not part of the supported external interface.
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
static char const ral_pkgname[] = PACKAGE_NAME ;
static char const ral_version[] = PACKAGE_VERSION ;
static char const ral_rcsid[] =
    "$Id: ral.c,v 1.37 2007/06/09 22:35:44 mangoa01 Exp $" ;
static char const ral_copyright[] =
    "This software is copyrighted 2004, 2005, 2006, 2007 by G. Andrew Mangogna."
    " Terms and conditions for use are distributed with the source code." ;

#ifdef Tcl_RegisterConfig_TCL_DECLARED
static Tcl_Config ral_config[] = {
    {"pkgname", ral_pkgname},
    {"version", ral_version},
    {"rcsid", ral_rcsid},
    {"copyright", ral_copyright},
    {NULL, NULL}
} ;
#endif

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
    Tcl_Interp *interp)
{
#   define NAMEBUFSIZE	32 /* nice power of 2, for no good reason */

    static const char nsSep[] = "::" ;
    static const char tupleCmdName[] = "tuple" ;
    static const char relationCmdName[] = "relation" ;
    static const char relvarCmdName[] = "relvar" ;

    char cmdName[NAMEBUFSIZE] ;
    char *cmdSlot ;
    Tcl_Namespace *ralNs ;
    Ral_RelvarInfo rInfo ;

    Tcl_InitStubs(interp, TCL_VERSION, 0) ;

    /*
     * Register the new data types that this package defines.
     */
    Tcl_RegisterObjType(&Ral_TupleObjType) ;
    Tcl_RegisterObjType(&Ral_RelationObjType) ;

    strcpy(cmdName, nsSep) ;
    strcat(cmdName, ral_pkgname) ;
    ralNs = Tcl_CreateNamespace(interp, cmdName, NULL, NULL) ;

    strcat(cmdName, nsSep) ;
    cmdSlot = cmdName + strlen(cmdName) ;

    strcpy(cmdSlot, tupleCmdName) ;
    Tcl_CreateObjCommand(interp, cmdName, tupleCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, tupleCmdName, 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    strcpy(cmdSlot, relationCmdName) ;
    Tcl_CreateObjCommand(interp, cmdName, relationCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, relationCmdName, 0) != TCL_OK) {
	return TCL_ERROR ;
    }

    strcpy(cmdSlot, relvarCmdName) ;
    rInfo = Ral_RelvarNewInfo(ral_pkgname, interp) ;
    Tcl_CreateObjCommand(interp, cmdName, relvarCmd, rInfo, NULL) ;
    if (Tcl_Export(interp, ralNs, relvarCmdName, 0) != TCL_OK) {
	return TCL_ERROR ;
    }

#   ifdef Tcl_RegisterConfig_TCL_DECLARED
    Tcl_RegisterConfig(interp, ral_pkgname, ral_config, "UTF-8") ;
#   endif

    Tcl_PkgProvide(interp, ral_pkgname, ral_version) ;

    return TCL_OK ;

#   undef NAMEBUFSIZE
}

/*
 * No real difference between normal and safe interpreters.
 */
int
Ral_SafeInit(
    Tcl_Interp *interp)
{
    return Ral_Init(interp) ;
}

#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION >= 5
/*
 * If we have the "unload" command we can provide the functions it likes to
 * have.  Nothing really to do here. All the data will be cleaned up when the
 * interpreter is deleted.
 */
int
Ral_Unload(
    Tcl_Interp *interp)
{
    return TCL_OK ;
}

int
Ral_SafeUnload(
    Tcl_Interp *interp)
{
    return Ral_Unload(interp) ;
}
#endif
