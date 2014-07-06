# This software is copyrighted 2014 by G. Andrew Mangogna.
# The following terms apply to all files associated with the software unless
# explicitly disclaimed in individual files.
# 
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors and
# need not follow the licensing terms described here, provided that the
# new terms are clearly indicated on the first page of each file where
# they apply.
# 
# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
# OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES
# THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# 
# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
# OR MODIFICATIONS.
# 
# GOVERNMENT USE: If you are acquiring this software on behalf of the
# U.S. government, the Government shall have only "Restricted Rights"
# in the software and related documentation as defined in the Federal
# Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
# are acquiring the software on behalf of the Department of Defense,
# the software shall be classified as "Commercial Computer Software"
# and the Government shall have only "Restricted Rights" as defined in
# Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing,
# the authors grant the U.S. Government and others acting in its behalf
# permission to use and distribute the software in accordance with the
# terms specified in this license.
# 
#  *++
# MODULE:
#   merge1.test -- schema for serialization test
# 
# ABSTRACT:
#  *--

package require ral
package require ralutil

ral relvar create Field {
    Name    string
    Size    int
} Name

ral relvar create SubField {
    Name    string
    Sub     string
    Offset  int
} {Name Sub}

ral relvar association R1\
    SubField Name *\
    Field Name 1

ral relvar create BitField {
    Name    string
    Sub     string
} {Name Sub}

ral relvar create IntField {
    Name    string
    Sub     string
    Width   int
    Sign    boolean
} {Name Sub}

ral relvar partition R2\
    SubField {Name Sub}\
        BitField {Name Sub}\
        IntField {Name Sub}

ral relvar create SymValue {
    Type    string
    Name    string
    Value   int
} {Type Name} {Type Value}

ral relvar create SymFieldValue {
    FieldName       string
    SubFieldName    string
    SymType         string
    SymName         string
} {FieldName SubFieldName SymType SymName}

ral relvar correlation R3 SymFieldValue\
    {FieldName SubFieldName} * IntField {Name Sub}\
    {SymType SymName} + SymValue {Type Name}

# Make sure that the total width of IntField's and the total number of
# BitField's in a given field does not exceed the size of the field.
ral relvar procedural R4 IntField {
    # Group the intfields and bitfields together with the field size information
    set fields [ralutil pipe {
        ral relvar set Field |
        ral relation join ~ [ral relvar set IntField] |
        ral relation group ~ IntFields Sub Width Sign |
        ral relation join ~ [ral relvar set BitField] |
        ral relation group ~ BitFields Sub
    }]
    # Iterate across the fields
    ral relation foreach field $fields {
        # Sum all the widths of the subfields.
        set totalintfields [ralutil pipe {
            ral relation extract $field IntFields |
            ral relation summarizeby ~ {} f TotalWidth int {rsum($f, "Width")} |
            ral relation extract ~ TotalWidth
        }]
        set totalbitfields [ralutil pipe {
            ral relation extract $field BitFields |
            ral relation cardinality
        }]
        if {$totalintfields + $totalbitfields >\
                    [ral relation extract $field Width]} {
            return false
        }
    }
    return true
}
