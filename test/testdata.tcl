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
#   testdata.test -- schema and population for serialization test
# 
# ABSTRACT:
#  *--

package require ral
namespace import ::ral::*

# relvar with relation and tuple valued attributes
relvar create People {
    Name    {Tuple {First string Middle string Last string}}
    Address {Tuple {Number string Street string}}
    Location {Tuple {City string State string Zip string}}
    Phones  {Relation {Type string Area string Number string}}
    Emails  {Relation {Type string Address string}}
} Name

relvar insert People {
    Name {First Fred Middle Allen Last Jones}
    Address {Number 200 Street {Sweeping Winds Ter}}
    Location {City Sunnyvale State CA Zip 77700}
    Phones {
        {Type Home Area 408 Number 555-1212}
        {Type Work Area 408 Number 555-2121}
    }
    Emails {
        {Type Home Address fred@biteme.com}
        {Type Work Address fred@slavedrivers.com}
    }
} {
    Name {First Alice Middle Ann Last Jones}
    Address {Number 300 Street Elm}
    Location {City Sunnyvale State CA Zip 77701}
    Phones {
        {Type Home Area 408 Number 555-1122}
    }
    Emails {
        {Type Home Address alice@biteme.com}
        {Type Work Address alice@waytowork.com}
    }
}

relvar create A {AID1 string AID2 int A_ATTR1 string} {AID1 AID2}
relvar create B {AID1 string B_ATTR1 string} AID1

relvar association R15 A AID1 * B AID1 ?

relvar insert A\
    {AID1 A1 AID2 1 A_ATTR1 foo}\
    {AID1 A2 AID2 2 A_ATTR1 bar}\
    {AID1 A1 AID2 3 A_ATTR1 baz}

relvar insert B\
    {AID1 A1 B_ATTR1 foo}\
    {AID1 A2 B_ATTR1 bar}\
    {AID1 A4 B_ATTR1 baz}

# The famous "Date" example database.
relvar create S {S# string SNAME string STATUS int CITY string} S#
relvar create P {P# string PNAME string COLOR string WEIGHT double CITY string}\
        P#
relvar create SP {S# string P# string QTY int} {S# P#}

relvar correlation R1 SP S# * S S# P# + P P#

relvar eval {
    relvar insert S\
        {S# S1 SNAME Smith STATUS 20 CITY London}\
        {S# S2 SNAME Jones STATUS 10 CITY Paris}\
        {S# S3 SNAME Blake STATUS 30 CITY Paris}\
        {S# S4 SNAME Clark STATUS 20 CITY London}\
        {S# S5 SNAME Adams STATUS 30 CITY Athens}

    relvar insert P\
        {P# P1 PNAME Nut COLOR Red WEIGHT 12.0 CITY London}\
        {P# P2 PNAME Bolt COLOR Green WEIGHT 17.0 CITY Paris}\
        {P# P3 PNAME Screw COLOR Blue WEIGHT 17.0 CITY Oslo}\
        {P# P4 PNAME Screw COLOR Red WEIGHT 14.0 CITY London}\
        {P# P5 PNAME Cam COLOR Blue WEIGHT 12.0 CITY Paris}\
        {P# P6 PNAME Cog COLOR Red WEIGHT 19.0 CITY London}

    relvar insert SP\
        {S# S1 P# P1 QTY 300}\
        {S# S1 P# P2 QTY 200}\
        {S# S1 P# P3 QTY 400}\
        {S# S1 P# P4 QTY 200}\
        {S# S1 P# P5 QTY 100}\
        {S# S1 P# P6 QTY 100}\
        {S# S2 P# P1 QTY 300}\
        {S# S2 P# P2 QTY 400}\
        {S# S3 P# P2 QTY 200}\
        {S# S4 P# P2 QTY 200}\
        {S# S4 P# P4 QTY 300}\
        {S# S4 P# P5 QTY 400}
}

# Something for a partition constraint.
relvar create Lamp {ModelNum int Color string} ModelNum
relvar create FloorLamp {ModelNum int Height int} ModelNum
relvar create TableLamp {ModelNum int Type string} ModelNum

relvar partition R3 Lamp ModelNum FloorLamp ModelNum TableLamp ModelNum

relvar eval {
    relvar insert Lamp\
        {ModelNum 1 Color Red}\
        {ModelNum 2 Color Blue}
    relvar insert FloorLamp\
        {ModelNum 1 Height 20}
    relvar insert TableLamp\
        {ModelNum 2 Type Banker}
}

# Something for a procedural constraint
relvar procedural valstatus S {
    variable S
    set city [::ral::relation project\
        [::ral::relation restrictwith $S {$STATUS == 1}] CITY]
    return [expr {[::ral::relation cardinality $city] < 2}]
}
