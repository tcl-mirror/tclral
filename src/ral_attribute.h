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

$RCSfile: ral_attribute.h,v $
$Revision: 1.1 $
$Date: 2005/12/27 23:17:19 $
 *--
 */
#ifndef _ral_attribute_h_
#define _ral_attribute_h_

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

/*
 * Attributes can be of either a base type supplied by Tcl or one of
 * the generated types of Tuple or Relation.
 */
typedef enum Ral_AttrDataType {
    Tcl_Type,
    Tuple_Type,
    Relation_Type
} Ral_AttrDataType ;

/*
 * An Attribute is an association of name to data type.  Name is just a simple
 * string. Data type is either one of the "known" Tcl data types or it can be a
 * "Tuple" or "Relation" type.  For the cases of "Tuple" or "Relation" these
 * types are not of constant structure. So for them we need a reference to the
 * headings that describe the details of the particular Tuple or Relation to
 * which this attribute refers.
 */

typedef struct Ral_Attribute {
    const char *name ;
    Ral_AttrDataType attrType ;
    union {
	Tcl_ObjType *tclType ;
	struct Ral_TupleHeading *tupleHeading ;
	struct Ral_RelationHeading *relationHeading ;
    } ;
} *Ral_Attribute ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_Attribute Ral_AttributeNewTclType(const char *, Tcl_ObjType *) ;
extern Ral_Attribute Ral_AttributeNewTupleType(const char *,
    struct Ral_TupleHeading *) ;
extern Ral_Attribute Ral_AttributeNewRelationType(const char *,
    struct Ral_RelationHeading *) ;
extern Ral_Attribute Ral_AttributeNewFromObjs(Tcl_Interp *, Tcl_Obj *,
    Tcl_Obj*) ;
extern int Ral_AttributeConvertValueToType(Tcl_Interp *, Ral_Attribute,
    Tcl_Obj *) ;
extern void Ral_AttributeDelete(Ral_Attribute) ;
extern Ral_Attribute Ral_AttributeCopy(Ral_Attribute) ;
extern Ral_Attribute Ral_AttributeRename(Ral_Attribute, const char *) ;
extern int Ral_AttributeEqual(Ral_Attribute, Ral_Attribute) ;
extern int Ral_AttributeScanName(Ral_Attribute, int *) ;
extern int Ral_AttributeConvertName(Ral_Attribute, char *, int) ;
extern int Ral_AttributeScanType(Ral_Attribute, int **) ;
extern int Ral_AttributeConvertType(Ral_Attribute, char *, int *) ;

#endif /* _ral_attribute_h_ */
