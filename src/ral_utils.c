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

$RCSfile: ral_utils.c,v $
$Revision: 1.14 $
$Date: 2007/01/01 01:48:17 $
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
static const char rcsid[] = "@(#) $RCSfile: ral_utils.c,v $ $Revision: 1.14 $" ;

static char const * const cmdStrings[] = {
    "unknown command",
    "tuple",
    "relation",
    "relvar",
    "setfromany",
    "updatefromobj",
} ;

/*
 * The order here must match that of the definition of "enum Ral_CmdOption"!
 */
static char const * const optStrings[] = {
    "NONE",
    "array",
    "assign",
    "association",
    "body",
    "cardinality",
    "choose",
    "constraint",
    "correlation",
    "create",
    "degree",
    "delete",
    "deleteone",
    "destroy",
    "dict",
    "divide",
    "eliminate",
    "emptyof",
    "equal",
    "eval",
    "extend",
    "extract",
    "foreach",
    "get",
    "group",
    "heading",
    "identifiers",
    "include",
    "insert",
    "intersect",
    "is",
    "isempty",
    "isnotempty",
    "join",
    "list",
    "minus",
    "names",
    "partition",
    "project",
    "rank",
    "reidentify",
    "rename",
    "restrict",
    "restrictwith",
    "semijoin",
    "semiminus",
    "set",
    "summarize",
    "tag",
    "tclose",
    "times",
    "trace",
    "tuple",
    "ungroup",
    "union",
    "unwrap",
    "update",
    "updateone",
    "wrap",
} ;

/*
 * The order here must match that of the definition of "enum Ral_ErrorCode"!
 */
static char const * const resultStrings[] = {
    "no error",
    "unknown attribute name",
    "duplicate attribute name",
    "bad heading format",
    "bad value format",
    "bad value type for value",
    "unknown data type",
    "bad type keyword",
    "wrong number of attributes specified",
    "bad list of pairs",

    "relations of non-zero degree must have at least one identifier",
    "identifiers must have at least one attribute",
    "identifiers must not be subsets of other identifiers",
    "duplicate attribute name in identifier attribute set",
    "duplicate tuple",
    "headings not equal",
    "relation must have degree of one",
    "relation must have degree of two",
    "relation must have cardinality of one",
    "bad list of triples",
    "attributes do not constitute an identifier",
    "attribute must be of a Relation type",
    "attribute must be of a Tuple type",
    "relation is not a projection of the summarized relation",
    "divisor heading must be disjoint from the dividend heading",
    "mediator heading must be a union of the dividend and divisor headings",
    "too many attributes specified",
    "attributes must have the same type",
    "only a single identifier may be specified",
    "identifier must have only a single attribute",
    "\"-within\" option attributes are not the subset of any identifier",
    "attribute is not a valid type for rank operation",

    "duplicate relvar name",
    "unknown relvar name",
    "mismatch between referential attributes",
    "duplicate constraint name",
    "unknown constraint name",
    "relvar has constraints in place",
    "referred to identifiers can not have non-singular multiplicities",
    "operation is not allowed during \"eval\" command",
    "a super set relvar may not be named as one of its own sub sets",
    "correlation spec is not available for a \"-complete\" correlation",
    "recursively invoking a relvar command outside of a transaction",
} ;

static char const * const errorStrings[] = {
    "OK",
    "UNKNOWN_ATTR",
    "DUPLICATE_ATTR",
    "HEADING_ERR",
    "FORMAT_ERR",
    "BAD_VALUE",
    "BAD_TYPE",
    "BAD_KEYWORD",
    "WRONG_NUM_ATTRS",
    "BAD_PAIRS_LIST",

    "NO_IDENTIFIER",
    "IDENTIFIER_FORMAT",
    "IDENTIFIER_SUBSET",
    "DUP_ATTR_IN_ID",
    "DUPLICATE_TUPLE",
    "HEADING_NOT_EQUAL",
    "DEGREE_ONE",
    "DEGREE_TWO",
    "CARDINALITY_ONE",
    "BAD_TRIPLE_LIST",
    "NOT_AN_IDENTIFIER",
    "NOT_A_RELATION",
    "NOT_A_TUPLE",
    "NOT_A_PROJECTION",
    "NOT_DISJOINT",
    "NOT_UNION",
    "TOO_MANY_ATTRS",
    "TYPE_MISMATCH",
    "SINGLE_IDENTIFIER",
    "SINGLE_ATTRIBUTE",
    "WITHIN_NOT_SUBSET",
    "BAD_RANK_TYPE",

    "DUP_NAME",
    "UNKNOWN_NAME",
    "REFATTR_MISMATCH",
    "DUP_CONSTRAINT",
    "UNKNOWN_CONSTRAINT",
    "CONSTRAINTS_PRESENT",
    "BAD_MULT",
    "BAD_TRANS_OP",
    "SUPER_NAME",
    "INCOMPLETE_SPEC",
    "ONGOING_CMD",
} ;
/*
FUNCTION DEFINITIONS
*/

void
Ral_ErrorInfoSetCmd(
    Ral_ErrorInfo *info,
    Ral_Command cmd,
    Ral_CmdOption opt)
{
    if (info) {
	info->cmd = cmd ;
	info->opt = opt ;
    }
}

void
Ral_ErrorInfoSetError(
    Ral_ErrorInfo *info,
    Ral_ErrorCode errorCode,
    const char *param)
{
    if (info) {
	info->errorCode = errorCode ;
	Tcl_DStringInit(&info->param) ;
	Tcl_DStringAppend(&info->param, param, -1) ;
    }
}

void
Ral_ErrorInfoSetErrorObj(
    Ral_ErrorInfo *info,
    Ral_ErrorCode errorCode,
    Tcl_Obj *objPtr)
{
    if (info) {
	info->errorCode = errorCode ;
	Tcl_DStringInit(&info->param) ;
	Tcl_DStringAppend(&info->param, Tcl_GetString(objPtr), -1) ;
    }
}

void
Ral_InterpSetError(
    Tcl_Interp *interp,
    Ral_ErrorInfo *info)
{
    if (interp && info) {
	const char *param = Tcl_DStringValue(&info->param) ;

	Tcl_ResetResult(interp) ;
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
	    resultStrings[info->errorCode], ", \"", param, "\"", NULL) ;
	Tcl_SetErrorCode(interp, "RAL", cmdStrings[info->cmd],
	    optStrings[info->opt], errorStrings[info->errorCode], param, NULL) ;
    }
    if (info) {
	Tcl_DStringFree(&info->param) ;
    }
}

void
Ral_InterpErrorInfo(
    Tcl_Interp *interp,
    Ral_Command cmd,
    Ral_CmdOption opt,
    Ral_ErrorCode errorCode,
    const char *param)
{
    Ral_ErrorInfo errInfo ;

    Ral_ErrorInfoSetCmd(&errInfo, cmd, opt) ;
    Ral_ErrorInfoSetError(&errInfo, errorCode, param) ;
    Ral_InterpSetError(interp, &errInfo) ;
}

void
Ral_InterpErrorInfoObj(
    Tcl_Interp *interp,
    Ral_Command cmd,
    Ral_CmdOption opt,
    Ral_ErrorCode errorCode,
    Tcl_Obj *objPtr)
{
    Ral_ErrorInfo errInfo ;

    Ral_ErrorInfoSetCmd(&errInfo, cmd, opt) ;
    Ral_ErrorInfoSetErrorObj(&errInfo, errorCode, objPtr) ;
    Ral_InterpSetError(interp, &errInfo) ;
}

char const *
Ral_ErrorInfoGetCommand(
    Ral_ErrorInfo *info)
{
    return cmdStrings[info->cmd] ;
}

char const *
Ral_ErrorInfoGetOption(
    Ral_ErrorInfo *info)
{
    return optStrings[info->opt] ;
}
