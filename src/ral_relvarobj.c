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

$RCSfile: ral_relvarobj.c,v $
$Revision: 1.1 $
$Date: 2006/04/16 19:00:12 $
 *--
 */

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include <assert.h>
#include <string.h>
#include "ral_relvarobj.h"
#include "ral_relvar.h"
#include "ral_utils.h"

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
static char *relvarTraceProc(ClientData, Tcl_Interp *, const char *,
    const char *, int) ;

/*
EXTERNAL DATA REFERENCES
*/

/*
EXTERNAL DATA DEFINITIONS
*/

/*
STATIC DATA ALLOCATION
*/
static int relvarTraceFlags = TCL_NAMESPACE_ONLY | TCL_TRACE_WRITES ;
static const char rcsid[] = "@(#) $RCSfile: ral_relvarobj.c,v $ $Revision: 1.1 $" ;

/*
FUNCTION DEFINITIONS
*/

int
Ral_RelvarObjNew(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    const char *name,
    Ral_RelationHeading heading)
{
    Tcl_DString resolve ;
    const char *resolvedName =
	Ral_RelvarObjResolveName(interp, name, &resolve) ;
    Ral_Relvar relvar = Ral_RelvarNew(info, resolvedName, heading) ;
    int status ;

    if (relvar == NULL) {
	/*
	 * Duplicate name.
	 */
	Ral_RelvarObjSetError(interp, Ral_RelvarLastError, resolvedName) ;
	Tcl_DStringFree(&resolve) ;
	return TCL_ERROR ;
    }
    /*
     * Create a variable by the same name.
     */
    if (Tcl_ObjSetVar2(interp, Tcl_NewStringObj(resolvedName, -1), NULL,
	relvar->relObj, TCL_LEAVE_ERR_MSG) == NULL) {
	goto errorOut ;
    }
    /*
     * Set up a trace to make the Tcl variable read only.
     */
    status = Tcl_TraceVar(interp, resolvedName, relvarTraceFlags,
	relvarTraceProc, info) ;
    assert(status == TCL_OK) ;

    Tcl_DStringFree(&resolve) ;
    Tcl_SetObjResult(interp, relvar->relObj) ;
    return TCL_OK ;

errorOut:
    Ral_RelvarDelete(info, resolvedName) ;
    Tcl_DStringFree(&resolve) ;
    return TCL_ERROR ;
}

int
Ral_RelvarObjDelete(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    Tcl_Obj *nameObj)
{
    char *fullName ;
    Ral_Relvar relvar ;
    int status ;

    relvar = Ral_RelvarObjFindRelvar(interp, info, Tcl_GetString(nameObj),
	&fullName) ;
    if (relvar == NULL) {
	return TCL_ERROR ;
    }

    /*
     * Remove the trace.
     */
    Tcl_UntraceVar(interp, fullName, relvarTraceFlags, relvarTraceProc,
	info) ;
    /*
     * Remove the variable.
     */
    status = Tcl_UnsetVar(interp, fullName, TCL_LEAVE_ERR_MSG) ;
    assert(status == TCL_OK) ;
    /*
     * HERE -- much more complicated with constraints processing.
     * For now, just clean up the variable space.
     */
    Ral_RelvarDelete(info, fullName) ;
    ckfree(fullName) ;

    Tcl_ResetResult(interp) ;
    return TCL_OK ;
}

Ral_Relvar
Ral_RelvarObjFindRelvar(
    Tcl_Interp *interp,
    Ral_RelvarInfo info,
    const char *name,
    char **fullName)
{
    Tcl_DString resolve ;
    const char *resolvedName =
	Ral_RelvarObjResolveName(interp, name, &resolve) ;
    Ral_Relvar relvar = Ral_RelvarFind(info, resolvedName) ;

    if (fullName) {
	*fullName = ckalloc(strlen(resolvedName) + 1) ;
	strcpy(*fullName, resolvedName) ;
    }
    Tcl_DStringFree(&resolve) ;

    if (relvar == NULL) {
	Ral_RelvarObjSetError(interp, RELVAR_UNKNOWN_NAME, name) ;
    }
    return relvar ;
}

const char *
Ral_RelvarObjResolveName(
    Tcl_Interp *interp,
    const char *name,
    Tcl_DString *resolvedName)
{
    int nameLen = strlen(name) ;

    Tcl_DStringInit(resolvedName) ;

    if (nameLen >= 2 && name[0] == ':' && name[1] == ':') {
	/*
	 * absolute reference.
	 */
	Tcl_DStringAppend(resolvedName, name, -1) ;
    } else if(interp) {
	/*
	 * construct an absolute name from the current namespace.
	 */
	Tcl_Namespace *curr = Tcl_GetCurrentNamespace(interp) ;
	if (curr->parentPtr) {
	    Tcl_DStringAppend(resolvedName, curr->fullName, -1) ;
	}
	Tcl_DStringAppend(resolvedName, "::", -1) ;
	Tcl_DStringAppend(resolvedName, name, -1) ;
    }

    return Tcl_DStringValue(resolvedName) ;
}

void
Ral_RelvarObjSetError(
    Tcl_Interp *interp,
    Ral_RelvarError error,
    const char *param)
{
    /*
     * These must be in the same order as the encoding of the Ral_RelationError
     * enumeration.
     */
    static const char *resultStrings[] = {
	"no error",
	"duplicate relvar name",
	"unknown relvar name",
	"relation heading mismatch",
    } ;
    static const char *errorStrings[] = {
	"OK",
	"DUP_NAME",
	"UNKNOWN_NAME",
	"HEADING_MISMATCH",
    } ;

    Ral_ObjSetError(interp, "RELVAR", resultStrings[error],
	errorStrings[error], param) ;
}

/*
 * PRIVATE FUNCTIONS
 */

static char *
relvarTraceProc(
    ClientData clientData,
    Tcl_Interp *interp,
    const char *name1,
    const char *name2,
    int flags)
{
    char *result = NULL ;

    /*
     * For write tracing, the value has been changed, so we must
     * restore it and spit back an error message.
     */
    if (flags & TCL_TRACE_WRITES) {
	Ral_RelvarInfo info = (Ral_RelvarInfo) clientData ;
	Tcl_DString resolve ;
	const char *resolvedName =
	    Ral_RelvarObjResolveName(interp, name1, &resolve) ;
	Ral_Relvar relvar = Ral_RelvarFind(info, resolvedName) ;
	Tcl_Obj *newValue ;

	assert(relvar != NULL) ;
	newValue = Tcl_SetVar2Ex(interp, resolvedName, NULL, relvar->relObj,
	    flags) ;
	Tcl_DStringFree(&resolve) ;
	/*
	 * Should not be modified because tracing is suspended while tracing.
	 */
	assert(newValue == relvar->relObj) ;
	result = "relvar may only be modified using \"::ral::relvar\" command" ;
    } else {
	fprintf(stderr, "relvarTraceProc: trace on non-write, flags = %#x\n",
	    flags) ;
    }

    return result ;
}
