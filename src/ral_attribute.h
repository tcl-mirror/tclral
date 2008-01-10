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
$Revision: 1.12 $
$Date: 2008/01/10 16:38:53 $
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
#include "ral_utils.h"

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
 * "Tuple" or "Relation" type.  For the cases of "Tuple" or "Relation", these
 * types are not of constant structure. So for them we need a reference to the
 * headings to describe the details of the particular Tuple or Relation to
 * which an attribute refers. Remember Tuple and Relation are really type
 * generators rather than specific fixed types.
 */

typedef struct Ral_Attribute {
    char const *name ;		    /* name of the attribute */
    char const *typeName ;	    /* data type name */
    Ral_AttrDataType attrType ;	    /* encoding to distinguish the union */
    union {
	Tcl_ObjType *tclType ;
	struct Ral_TupleHeading *tupleHeading ;
	struct Ral_RelationHeading *relationHeading ;
    } heading ;
} *Ral_Attribute ;

/*
 * Data structures to hold scan flags. Scan flags are used to accumulate
 * information used when comverting list elements into strings. Here the
 * scan flags concept is broadened to deal with the issues that arise for
 * Tuple and Relation types. We also split the flags between those needed
 * for type information and those used for values. The complexity arises
 * here because of the possiblity of Tuple or Relation valued attributes and
 * those attributes will have a recursive structure.
 */
typedef struct Ral_AttributeTypeScanFlags {
    Ral_AttrDataType attrType ;
    int nameFlags ;
    int nameLength ;	/* Since attribute names appear several places,
			 * we save the scan length result and reuse it*/
    union {
	int simpleFlags ;
	struct {
	    int count ;
	    struct Ral_AttributeTypeScanFlags *flags ;
	} compoundFlags ;
    } ;
} Ral_AttributeTypeScanFlags ;

typedef struct Ral_AttributeValueScanFlags {
    Ral_AttrDataType attrType ;
    union {
	int simpleFlags ;
	struct {
	    int count ;
	    struct Ral_AttributeValueScanFlags *flags ;
	} compoundFlags ;
    } ;
} Ral_AttributeValueScanFlags ;

/*
DATA DECLARATIONS
*/
extern char tupleKeyword[] ;
extern char relationKeyword[] ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_Attribute Ral_AttributeNewTclType(char const *, char const *) ;
extern Ral_Attribute Ral_AttributeNewTupleType(char const *,
    struct Ral_TupleHeading *) ;
extern Ral_Attribute Ral_AttributeNewRelationType(char const *,
    struct Ral_RelationHeading *) ;
extern Ral_Attribute Ral_AttributeNewFromObjs(Tcl_Interp *, Tcl_Obj *,
    Tcl_Obj*, Ral_ErrorInfo *) ;
extern int Ral_AttributeConvertValueToType(Tcl_Interp *, Ral_Attribute,
    Tcl_Obj *, Ral_ErrorInfo *) ;
extern void Ral_AttributeDelete(Ral_Attribute) ;
extern Ral_Attribute Ral_AttributeDup(Ral_Attribute) ;
extern Ral_Attribute Ral_AttributeRename(Ral_Attribute, char const *) ;
extern int Ral_AttributeEqual(Ral_Attribute, Ral_Attribute) ;
extern int Ral_AttributeTypeEqual(Ral_Attribute, Ral_Attribute) ;
extern int Ral_AttributeValueEqual(Ral_Attribute, Tcl_Obj *, Tcl_Obj *) ;
extern int Ral_AttributeValueCompare(Ral_Attribute, Tcl_Obj *, Tcl_Obj *) ;
extern Tcl_Obj *Ral_AttributeValueObj(Tcl_Interp *, Ral_Attribute, Tcl_Obj *) ;
extern int Ral_AttributeScanName(Ral_Attribute, Ral_AttributeTypeScanFlags *) ;
extern int Ral_AttributeConvertName(Ral_Attribute, char *,
    Ral_AttributeTypeScanFlags *) ;
extern int Ral_AttributeScanType(Ral_Attribute, Ral_AttributeTypeScanFlags *) ;
extern int Ral_AttributeConvertType(Ral_Attribute, char *,
    Ral_AttributeTypeScanFlags *) ;
extern int Ral_AttributeScanValue(Ral_Attribute, Tcl_Obj *,
    Ral_AttributeTypeScanFlags *, Ral_AttributeValueScanFlags *) ;
extern int Ral_AttributeConvertValue(Ral_Attribute, Tcl_Obj *, char *,
    Ral_AttributeTypeScanFlags *, Ral_AttributeValueScanFlags *) ;
extern void Ral_AttributeTypeScanFlagsFree(Ral_AttributeTypeScanFlags *) ;
extern void Ral_AttributeValueScanFlagsFree(Ral_AttributeValueScanFlags *) ;
extern char *Ral_AttributeToString(Ral_Attribute) ;
extern char const *Ral_AttributeVersion(void) ;

#endif /* _ral_attribute_h_ */
