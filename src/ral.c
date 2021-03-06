/*
This software is copyrighted 2004 - 2011 by G. Andrew Mangogna.
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
$Revision: 1.45 $
$Date: 2012/02/26 19:09:04 $
 *--
 */
#if TCL_MAJOR_VERSION >= 8 && TCL_MINOR_VERSION < 5
#error "Tcl version must be at least 8.5"
#endif

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <stdio.h>
#include <string.h>
#include "tcl.h"
#include "tclTomMath.h"
#include "ral_tuplecmd.h"
#include "ral_relationcmd.h"
#include "ral_relationobj.h"
#include "ral_relvar.h"
#include "ral_relvarobj.h"
#include "ral_relvarcmd.h"

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
static char const ral_copyright[] =
    "This software is copyrighted 2004 - 2014 by G. Andrew Mangogna."
    " Terms and conditions for use are distributed with the source code." ;

static Tcl_Config ral_config[] = {
    {"pkgname", ral_pkgname},
    {"version", ral_version},
    {"copyright", ral_copyright},
    {NULL, NULL}
} ;

/*
FUNCTION DEFINITIONS
*/

/*
 * ======================================================================
 * Initialization Function
 * ======================================================================
 */

DLLEXPORT
int
Ral_Init(
    Tcl_Interp *interp)
{
    static char const tupleCmdName[] = "::ral::tuple" ;
    static char const tupleStr[] = "tuple" ;
    static char const relationCmdName[] = "::ral::relation" ;
    static char const relationStr[] = "relation" ;
    static char const relvarCmdName[] = "::ral::relvar" ;
    static char const relvarStr[] = "relvar" ;
    static char const pkgNamespace[] = "::ral" ;

    Ral_RelvarInfo rInfo ;
    Tcl_Obj *mapObj ;
    Tcl_Command ensembleCmdToken ;
    Tcl_Namespace *ralNs ;

    if (Tcl_InitStubs(interp, TCL_VERSION, 0) == NULL) {
        return TCL_ERROR ;
    }
    if (Tcl_TomMath_InitStubs(interp, TCL_VERSION) == NULL) {
        return TCL_ERROR ;
    }
    /*
     * Create the namespace in which the package command reside.
     */
    ralNs = Tcl_CreateNamespace(interp, pkgNamespace, NULL, NULL) ;
    /*
     * Create the package commands.
     * First the "tuple" command.
     */
    mapObj = Tcl_NewDictObj() ;
    Tcl_CreateObjCommand(interp, tupleCmdName, tupleCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, tupleStr, 0) != TCL_OK) {
        goto errorout ;
    }
    if (Tcl_DictObjPut(interp, mapObj,
                Tcl_NewStringObj(tupleStr, -1),
                Tcl_NewStringObj(tupleCmdName, -1)) != TCL_OK) {
        goto errorout ;
    }
    /*
     * Next the "relation" command.
     */
    Tcl_CreateObjCommand(interp, relationCmdName, relationCmd, NULL, NULL) ;
    if (Tcl_Export(interp, ralNs, relationStr, 0) != TCL_OK) {
        goto errorout ;
    }
    if (Tcl_DictObjPut(interp, mapObj,
                Tcl_NewStringObj(relationStr, -1),
                Tcl_NewStringObj(relationCmdName, -1)) != TCL_OK) {
        goto errorout ;
    }
    /*
     * Finally, the "relvar" command.
     */
    rInfo = Ral_RelvarNewInfo(ral_pkgname, interp) ;
    Tcl_CreateObjCommand(interp, relvarCmdName, relvarCmd, rInfo, NULL) ;
    if (Tcl_Export(interp, ralNs, relvarStr, 0) != TCL_OK) {
        goto errorout ;
    }
    if (Tcl_DictObjPut(interp, mapObj,
                Tcl_NewStringObj(relvarStr, -1),
                Tcl_NewStringObj(relvarCmdName, -1)) != TCL_OK) {
        goto errorout ;
    }
    /*
     * Create an ensemble command on the namespace.
     */
    ensembleCmdToken = Tcl_CreateEnsemble(interp, "::ral", ralNs, 0) ;
    /*
     * Following the pattern in Tcl sources, we set the mapping dictionary
     * of the ensemble. This will allow the ensemble to be extended
     * at the script level.
     */
    if (Tcl_SetEnsembleMappingDict(interp, ensembleCmdToken, mapObj) !=
            TCL_OK) {
        goto errorout ;
    }
    /*
     * Support for package configuration.
     */
    Tcl_RegisterConfig(interp, ral_pkgname, ral_config, "iso8859-1") ;

    Tcl_PkgProvide(interp, ral_pkgname, ral_version) ;

    return TCL_OK ;

errorout:
    Tcl_DecrRefCount(mapObj) ;
    return TCL_ERROR ;
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

/*
 * If we have the "unload" command we can provide the functions it likes to
 * have.  Nothing really to do here. All the data will be cleaned up when the
 * interpreter is deleted.
 */
DLLEXPORT
int
Ral_Unload(
    Tcl_Interp *interp)
{
    return TCL_OK ;
}

DLLEXPORT
int
Ral_SafeUnload(
    Tcl_Interp *interp)
{
    return Ral_Unload(interp) ;
}
