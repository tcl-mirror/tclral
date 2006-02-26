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

$RCSfile: ral_relationheading.h,v $
$Revision: 1.4 $
$Date: 2006/02/26 04:57:53 $
 *--
 */
#ifndef _ral_relationheading_h_
#define _ral_relationheading_h_

/*
PRAGMAS
*/

/*
INCLUDE FILES
*/
#include "ral_tupleheading.h"
#include "ral_vector.h"
#include <stdio.h>

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
 * A Relation Heading is a Tuple Heading with the addition of identifiers.
 * Identifiers are sub sets of attributes for which the tuples may not have
 * duplicated values.
 */
typedef struct Ral_RelationHeading {
    int refCount ;			/* Relation headings are reference
					 * counted. */
    Ral_TupleHeading tupleHeading ;	/* The Tuple heading that describes
					 * all of the tuples contained in the
					 * relation */
    int idCount ;			/* The number of identfiers */
    Ral_IntVector *identifiers ;	/* An array of vectors holding the
					 * identifiers. Identifiers are a vector
					 * of offsets into the Tuple Heading
					 * giving the attributes that
					 * constitute the identifier. */
} *Ral_RelationHeading ;

/*
FUNCTION DECLARATIONS
*/

extern Ral_RelationHeading Ral_RelationHeadingNew(Ral_TupleHeading, int) ;
extern Ral_RelationHeading Ral_RelationHeadingDup(Ral_RelationHeading) ;
extern void Ral_RelationHeadingDelete(Ral_RelationHeading) ;
extern void Ral_RelationHeadingReference(Ral_RelationHeading) ;
extern void Ral_RelationHeadingUnreference(Ral_RelationHeading) ;
extern int Ral_RelationHeadingDegree(Ral_RelationHeading) ;
extern int Ral_RelationHeadingEqual(Ral_RelationHeading, Ral_RelationHeading) ;
extern int Ral_RelationHeadingAddIdentifier(Ral_RelationHeading, int,
    Ral_IntVector) ;
extern Ral_RelationHeading Ral_RelationHeadingUnion(Ral_RelationHeading,
    Ral_RelationHeading) ;
extern int Ral_RelationHeadingScan(Ral_RelationHeading,
    Ral_AttributeTypeScanFlags *) ;
extern int Ral_RelationHeadingConvert(Ral_RelationHeading, char *,
    Ral_AttributeTypeScanFlags *) ;
extern void Ral_RelationHeadingPrint(Ral_RelationHeading, const char *,
    FILE *) ;
extern char * Ral_RelationHeadingStringOf(Ral_RelationHeading) ;
extern const char *Ral_RelationHeadingVersion(void) ;

#endif /* _ral_relationheading_h_ */
