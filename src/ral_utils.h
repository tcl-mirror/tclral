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

$RCSfile: ral_utils.h,v $
$Revision: 1.4 $
$Date: 2006/07/09 03:48:13 $
 *--
 */
#ifndef _ral_utils_h_
#define _ral_utils_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "tcl.h"

/*
MACRO DEFINITIONS
*/

/*
FORWARD CLASS REFERENCES
*/

/*
TYPE DECLARATIONS
*/

typedef enum {
    Ral_CmdUnknown,
    Ral_CmdTuple,
    Ral_CmdRelation,
    Ral_CmdRelvar,
    Ral_CmdSetFromAny,
} Ral_Command ;

typedef enum {
    Ral_OptNone,
    Ral_OptArray,
    Ral_OptAssign,
    Ral_OptAssociation,
    Ral_OptCardinality,
    Ral_OptChoose,
    Ral_OptConstraint,
    Ral_OptCreate,
    Ral_OptDegree,
    Ral_OptDelete,
    Ral_OptDeleteone,
    Ral_OptDestroy,
    Ral_OptDict,
    Ral_OptDivide,
    Ral_OptEliminate,
    Ral_OptEmptyof,
    Ral_OptEqual,
    Ral_OptEval,
    Ral_OptExtend,
    Ral_OptExtract,
    Ral_OptForeach,
    Ral_OptGet,
    Ral_OptGroup,
    Ral_OptHeading,
    Ral_OptIdentifiers,
    Ral_OptInclude,
    Ral_OptInsert,
    Ral_OptIntersect,
    Ral_OptIs,
    Ral_OptIsempty,
    Ral_OptIsnotempty,
    Ral_OptJoin,
    Ral_OptList,
    Ral_OptMinus,
    Ral_OptNames,
    Ral_OptPartition,
    Ral_OptProject,
    Ral_OptRank,
    Ral_OptRename,
    Ral_OptRestrict,
    Ral_OptRestrictwith,
    Ral_OptSemijoin,
    Ral_OptSemiminus,
    Ral_OptSet,
    Ral_OptSummarize,
    Ral_OptTag,
    Ral_OptTclose,
    Ral_OptTimes,
    Ral_OptTuple,
    Ral_OptUngroup,
    Ral_OptUnion,
    Ral_OptUnwrap,
    Ral_OptUpdate,
    Ral_OptUpdateone,
    Ral_OptWrap,
} Ral_CmdOption ;

typedef enum {
    RAL_ERR_OK = 0,
    RAL_ERR_UNKNOWN_ATTR,
    RAL_ERR_DUPLICATE_ATTR,
    RAL_ERR_HEADING_ERR,
    RAL_ERR_FORMAT_ERR,
    RAL_ERR_BAD_VALUE,
    RAL_ERR_BAD_TYPE,
    RAL_ERR_BAD_KEYWORD,
    RAL_ERR_WRONG_NUM_ATTRS,
    RAL_ERR_BAD_PAIRS_LIST,

    RAL_ERR_NO_IDENTIFIER,
    RAL_ERR_IDENTIFIER_FORMAT,
    RAL_ERR_IDENTIFIER_SUBSET,
    RAL_ERR_DUP_ATTR_IN_ID,
    RAL_ERR_DUPLICATE_TUPLE,
    RAL_ERR_HEADING_NOT_EQUAL,
    RAL_ERR_DEGREE_ONE,
    RAL_ERR_DEGREE_TWO,
    RAL_ERR_CARDINALITY_ONE,
    RAL_ERR_BAD_TRIPLE_LIST,
    RAL_ERR_NOT_AN_IDENTIFIER,
    RAL_ERR_NOT_A_RELATION,
    RAL_ERR_NOT_A_TUPLE,
    RAL_ERR_NOT_A_PROJECTION,
    RAL_ERR_NOT_DISJOINT,
    RAL_ERR_NOT_UNION,
    RAL_ERR_TOO_MANY_ATTRS,
    RAL_ERR_TYPE_MISMATCH,
    RAL_ERR_SINGLE_IDENTIFIER,
    RAL_ERR_SINGLE_ATTRIBUTE,
    RAL_ERR_WITHIN_NOT_SUBSET,
    RAL_ERR_BAD_RANK_TYPE,

    RAL_ERR_DUP_NAME,
    RAL_ERR_UNKNOWN_NAME,
    RAL_ERR_REFATTR_MISMATCH,
    RAL_ERR_DUP_CONSTRAINT,
    RAL_ERR_UNKNOWN_CONSTRAINT,
    RAL_ERR_CONSTRAINTS_PRESENT,
    RAL_ERR_BAD_MULT,
    RAL_ERR_BAD_TRANS_OP,
} Ral_ErrorCode ;

typedef struct Ral_ErrorInfo {
    Ral_Command cmd ;
    Ral_CmdOption opt ;
    Ral_ErrorCode errorCode ;
    Tcl_DString param ;
} Ral_ErrorInfo ;

/*
FUNCTION DECLARATIONS
*/

extern int Ral_ObjEqual(Tcl_Obj *, Tcl_Obj *) ;
extern void Ral_ErrorInfoSetCmd(Ral_ErrorInfo *, Ral_Command, Ral_CmdOption) ;
extern void Ral_ErrorInfoSetError(Ral_ErrorInfo *info, Ral_ErrorCode,
    const char *) ;
extern void Ral_ErrorInfoSetErrorObj(Ral_ErrorInfo *, Ral_ErrorCode,
    Tcl_Obj *) ;
extern void Ral_InterpSetError(Tcl_Interp *, Ral_ErrorInfo *) ;
extern void Ral_InterpErrorInfo(Tcl_Interp *, Ral_Command, Ral_CmdOption,
    Ral_ErrorCode, const char *) ;
extern void Ral_InterpErrorInfoObj(Tcl_Interp *, Ral_Command, Ral_CmdOption,
    Ral_ErrorCode, Tcl_Obj *) ;

#endif /* _ral_utils_h_ */
