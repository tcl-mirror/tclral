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
$Revision: 1.17.2.3 $
$Date: 2009/02/02 01:30:33 $
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
#include "ral_tupleheading.h"

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
extern int Ral_RelvarObjNew(Tcl_Interp *, Ral_RelvarInfo, char const *,
    Ral_TupleHeading, int, Tcl_Obj *const*, Ral_ErrorInfo *) ;
extern int Ral_RelvarObjDelete(Tcl_Interp *, Ral_RelvarInfo, Tcl_Obj *) ;
extern int Ral_RelvarObjCopyOnShared(Tcl_Interp *, Ral_RelvarInfo, Ral_Relvar) ;
extern Ral_Relvar Ral_RelvarObjFindRelvar(Tcl_Interp *, Ral_RelvarInfo,
    char const *) ;
extern int Ral_RelvarObjInsertTuple(Tcl_Interp *, Ral_Relvar, Tcl_Obj *,
    Ral_ErrorInfo *) ;
extern int Ral_RelvarObjUpdateTuple(Tcl_Interp *, Ral_Relvar, Ral_Relation,
    Ral_RelationIter, Tcl_Obj *, Tcl_Obj *, Ral_Relation, Ral_ErrorInfo *) ;

extern int Ral_RelvarObjCreateAssoc(Tcl_Interp *, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjCreatePartition(Tcl_Interp *, int, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjCreateCorrelation(Tcl_Interp *, Tcl_Obj *const*,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintDelete(Tcl_Interp *, char const *,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintInfo(Tcl_Interp *, Tcl_Obj * const,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintNames(Tcl_Interp *, char const *,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintMember(Tcl_Interp *, Tcl_Obj * const,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjConstraintPath(Tcl_Interp *, Tcl_Obj *const,
    Ral_RelvarInfo) ;
extern int Ral_RelvarObjEndTrans(Tcl_Interp *, Ral_RelvarInfo, int) ;
extern int Ral_RelvarObjEndCmd(Tcl_Interp *, Ral_RelvarInfo, int) ;

extern int Ral_RelvarObjTraceVarAdd(Tcl_Interp *, Ral_Relvar, Tcl_Obj *const,
    Tcl_Obj *const) ;
extern int Ral_RelvarObjTraceVarRemove(Tcl_Interp *, Ral_Relvar, Tcl_Obj *const,
    Tcl_Obj *const) ;
extern int Ral_RelvarObjTraceVarInfo(Tcl_Interp *, Ral_Relvar) ;
extern int Ral_RelvarObjTraceEvalAdd(Tcl_Interp *, Ral_RelvarInfo,
    Tcl_Obj *const) ;
extern int Ral_RelvarObjTraceEvalRemove(Tcl_Interp *, Ral_RelvarInfo,
    Tcl_Obj *const) ;
extern int Ral_RelvarObjTraceEvalInfo(Tcl_Interp *, Ral_RelvarInfo) ;

extern int Ral_RelvarObjExecDeleteTraces(Tcl_Interp *, Ral_Relvar,
    Tcl_Obj *) ;
extern Tcl_Obj *Ral_RelvarObjExecInsertTraces(Tcl_Interp *, Ral_Relvar,
    Tcl_Obj *) ;
extern Tcl_Obj *Ral_RelvarObjExecUpdateTraces(Tcl_Interp *, Ral_Relvar,
    Tcl_Obj *, Tcl_Obj *) ;
extern Tcl_Obj *Ral_RelvarObjExecSetTraces(Tcl_Interp *, Ral_Relvar,
    Tcl_Obj *, Ral_ErrorInfo *errInfo) ;
extern void Ral_RelvarObjExecUnsetTraces(Tcl_Interp *, Ral_Relvar) ;
extern void Ral_RelvarObjExecEvalTraces(Tcl_Interp *, Ral_RelvarInfo, int,
    int) ;

extern Tcl_InterpDeleteProc Ral_RelvarObjInterpDeleted ;

#endif /* _ral_relvarobj_h_ */
