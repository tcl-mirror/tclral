proc __copyAndLoad__ {offset size package} {
    set mchan [open [info script] rb]
    set lchan [file tempfile tempname]
    chan configure $lchan -translation binary
    try {
        chan seek $mchan $offset start
        chan copy $mchan $lchan -size $size
    } finally {
        chan close $mchan
        chan close $lchan
    }

    try {
        load $tempname $package
    } finally {
        catch {file delete $tempname}
    }
}
if {$::tcl_platform(os) eq "Linux"} {
    __copyAndLoad__ 77782 238944 ral
}
if {$::tcl_platform(os) eq "Windows NT"} {
    __copyAndLoad__ 316726 497381 ral
}
if {$::tcl_platform(os) eq "Darwin"} {
    __copyAndLoad__ 814107 324292 ral
}
rename __copyAndLoad__ {}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       
# This software is copyrighted 2004 - 2014 by G. Andrew Mangogna.
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
# ral.tcl -- Tcl scripts portion of TclRAL
# 
# ABSTRACT:
# This file contains the Tcl script portions of the TclRAL package.
#  *--

namespace eval ::ral {
    namespace export tuple2matrix
    namespace export relation2matrix
    namespace export relformat
    namespace export tupleformat
    namespace export serialize
    namespace export serializeToFile
    namespace export deserialize
    namespace export deserializeFromFile
    namespace export deserialize-0.8.X
    namespace export deserializeFromFile-0.8.X
    namespace export merge
    namespace export mergeFromFile
    namespace export storeToMk
    namespace export loadFromMk
    namespace export mergeFromMk
    namespace export storeToSQLite
    namespace export loadFromSQLite
    namespace export mergeFromSQLite
    namespace export dump
    namespace export dumpToFile
    namespace export csv
    namespace export csvToFile
    if {[package vsatisfies [package require Tcl] 8.5]} {
        # Configure the exported commands into the namespace ensemble
        # for the other package commands.
        set map [namespace ensemble configure ::ral -map]
        set map [dict merge $map {
            tuple2matrix ::ral::tuple2matrix
            relation2matrix ::ral::relation2matrix
            relformat ::ral::relformat
            tupleformat ::ral::tupleformat
            serialize ::ral::serialize
            serializeToFile ::ral::serializeToFile
            deserialize ::ral::deserialize
            deserializeFromFile ::ral::deserializeFromFile
            deserialize-0.8.X ::ral::deserialize-0.8.X
            deserializeFromFile-0.8.X ::ral::deserializeFromFile-0.8.X
            merge ::ral::merge
            mergeFromFile ::ral::mergeFromFile
            storeToMk ::ral::storeToMk
            loadFromMk ::ral::loadFromMk
            mergeFromMk ::ral::mergeFromMk
            storeToSQLite ::ral::storeToSQLite
            loadFromSQLite ::ral::loadFromSQLite
            mergeFromSQLite ::ral::mergeFromSQLite
            dump ::ral::dump
            dumpToFile ::ral::dumpToFile
            csv ::ral::csv
            csvToFile ::ral::csvToFile
        }]
        namespace ensemble configure ::ral -map $map
    } else {
        namespace export rcount
        namespace export rcountd
        namespace export rsum
        namespace export rsumd
        namespace export ravg
        namespace export ravgd
        namespace export rmin
        namespace export rmax
    }

    if {![catch {package require report}]} {
        # Default report style for Tuple types
        ::report::defstyle ::ral::tupleAsTable {{capRows 2}} {
            data set [split [string repeat "| " [columns]]|]
            set sepTemplate [split [string repeat "+ - " [columns]]+]
            top set $sepTemplate
            top enable
            bottom set [top get]
            bottom enable
            topdata set [data get]
            topcapsep set [top get]
            topcapsep enable
            tcaption $capRows
        }
        namespace export tupleAsTable

        # Default report style for Relation types
        ::report::defstyle ::ral::relationAsTable {{capRows 2}} {
            ::ral::tupleAsTable $capRows
            top set [top get]
            bottom set [top get]
            topcapsep set [top get]
        }
        namespace export relationAsTable
    }

    variable reportCounter 0 ; # used to make unique names
    # Maximum width of a column that holds a scalar value.
    # Relation and Tuple values are always shown in their "normal" width.
    variable maxColLen 30
    # The internal representation of NULL for the benefit of SQLite.
    variable nullValue {}

    # Define a proc to determine if the version of a serialized file
    # is compatible with the library. We use "pkgconfig" if it is
    # available, "package require" if not. If building for some older
    # versions of Tcl, "pkgconfig" may not be available.
    if {[llength [info commands ::ral::pkgconfig]]} {
        proc getVersion {} {
            return [::ral::pkgconfig get version]
        }
    } else {
        proc getVersion {} {
            return [package require ral]
        }
    }
    proc getCompatVersion {} {
        lassign [split [getVersion] .] maj min rev
        incr maj
        return 0.9-$maj.$min
    }

    variable sqlTypeMap ; array set sqlTypeMap {
        int         integer
        double      real
        bytearray   blob
    }

    # This is the SQL schema required to store the ral meta-data into SQL so
    # that we will have all the required information to reconstruct the relvars
    # when we load it back in.
    variable ralSQLSchema {
-- SQL Tables to hold RAL specific meta-data
create table __ral_version (
    Vnum text not null,
    Date timestamp not null,
    constraint __ral_version_id unique (Vnum)) ;
create table __ral_relvar (
    Vname text not null,
    constraint __ral_relvar_id unique (Vname)) ;
create table __ral_attribute (
    Vname text not null,
    Aname text not null,
    Type text not null,
    constraint __ral_attribute_id unique (Vname, Aname),
    constraint __ral_attribute_ref foreign key (Vname) references
        __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create index __ral_attribute_ref_index on __ral_attribute (Vname) ;
create table __ral_identifier (
    IdNum integer not null,
    Vname text not null,
    Attr text not null,
    constraint __ral_identifier_id unique (IdNum, Vname, Attr),
    constraint __ral_identifier_ref foreign key (Vname) references
        __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred,
    constraint __ral_identifier_ref2 foreign key (Vname, Attr) references
        __ral_attribute(Vname, Aname) on delete cascade on update cascade
        deferrable initially deferred) ;
create index __ral_identifier_ref_index on __ral_identifier (Vname) ;
create index __ral_identifier_ref2_index on __ral_identifier (Vname, Attr) ;
create table __ral_association (
    Cname text not null,
    Referring text not null,
    ReferringAttrs text not null,
    RefToSpec text not null,
    RefTo text not null,
    RefToAttrs text not null,
    ReferringSpec text not null,
    constraint __ral_association_id unique (Cname),
    constraint __ral_association_one foreign key (Referring)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred,
    constraint __ral_association_other foreign key (RefTo)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create index __ral_association_oneindex on __ral_association (Referring) ;
create index __ral_association_otherindex on __ral_association (RefTo) ;
create table __ral_partition (
    Cname text not null,
    SuperType text not null,
    SuperAttrs text not null,
    constraint __ral_partition_id unique (Cname),
    constraint __ral_partition_ref foreign key (SuperType)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create index __ral_partition_ref on __ral_partition (SuperType) ;
create table __ral_subtype (
    Cname text not null,
    SubType text not null,
    SubAttrs text not null,
    constraint __ral_subtype_id unique (Cname, SubType),
    constraint __ral_subtype_ref foreign key (SubType)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create index __ral_subtype_refindex on __ral_subtype (SubType) ;
create table __ral_correlation (
    Cname text not null,
    IsComplete integer not null,
    AssocRelvar text not null,
    OneRefAttrs text not null,
    OneRefSpec text not null,
    OneRelvar text not null,
    OneAttrs text not null,
    OtherRefAttrs text not null,
    OtherRefSpec text not null,
    OtherRelvar text not null,
    OtherAttrs text not null,
    constraint __ral_correlation_id unique (Cname),
    constraint __ral_correlation_assoc foreign key (AssocRelvar)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred,
    constraint __ral_correlation_oneref foreign key (OneRelvar)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred,
    constraint __ral_correlation_otherref foreign key (OtherRelvar)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create index __ral_correlation_assocref on __ral_correlation (AssocRelvar) ;
create index __ral_correlation_oneref on __ral_correlation (OneRelvar) ;
create index __ral_correlation_otherref on __ral_correlation (OtherRelvar) ;
create table __ral_procedural (
    Cname text not null,
    Script text not null,
    constraint __ral_procedural_id unique(Cname)) ;
create table __ral_proc_participant (
    Cname text not null,
    ParticipantRelvar text not null,
    constraint __ral_proc_participant_id unique (Cname, ParticipantRelvar),
    constraint __ral_proc_participant_proc foreign key (Cname)
        references __ral_procedural (Cname) on delete cascade on update cascade
        deferrable initially deferred,
    constraint __ral_proc_participant_relvar foreign key (ParticipantRelvar)
        references __ral_relvar(Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
    }
}

# Convert a tuple into a matrix.
proc ::ral::tuple2matrix {tupleValue {noheading 0}} {
    package require struct::matrix 2

    set m [::struct::matrix]
    $m add columns [tuple degree $tupleValue]
    set heading [tuple heading $tupleValue]
    if {!$noheading} {
        addHeading $m $heading
    }
    addTuple $m $tupleValue [getFormatMap $heading]

    return $m
}

# Convert a relation into a matrix.
proc ::ral::relation2matrix {relValue {sortAttr {}} {noheading 0}} {
    package require struct::matrix 2

    set m [::struct::matrix]
    $m add columns [relation degree $relValue]
    set heading [relation heading $relValue]
    set attrReportMap [getFormatMap $heading]
    if {!$noheading} {
        addHeading $m $heading
    }
    relation foreach r $relValue -ascending $sortAttr {
        addTuple $m [relation tuple $r] $attrReportMap
    }

    return $m
}

# This procedure is intended to be to relations what "parray" is to arrays.
# It provides a very simple text formatting of relation values into a
# tabular layout. Rather than writing the output to a channel "relformat"
# returns a string containing the formatted relation.
# There is much more that could be done here, e.g. adding a "-format" option
# where you could specify the details of the formatting.
proc ::ral::relformat {relValue {title {}} {sortAttrs {}} {noheading 0}} {
    package require report

    variable reportCounter
    set reportName rep[incr reportCounter]
    ::report::report $reportName [relation degree $relValue]\
        style ::ral::relationAsTable [expr {$noheading ? 0 : 2}]
    set m [relation2matrix $relValue $sortAttrs $noheading]
    set result [string trimright [$reportName printmatrix $m]]
    if {$title ne ""} {
        append result "\n" $title "\n" [string repeat - [string length $title]]
    }

    $reportName destroy
    $m destroy

    return $result
}

# Return a string that pretty prints a tuple.
proc ::ral::tupleformat {tupleValue {title {}} {noheading 0}} {
    package require report

    variable reportCounter
    set reportName rep[incr reportCounter]
    ::report::report $reportName [tuple degree $tupleValue]\
        style ::ral::tupleAsTable [expr {$noheading ? 0 : 2}]
    set m [tuple2matrix $tupleValue $noheading]
    set result [string trimright [$reportName printmatrix $m]]
    if {$title ne ""} {
        append result "\n" $title "\n" [string repeat - [string length $title]]
    }

    $reportName destroy
    $m destroy

    return $result
}

# Serialization format:
# Dictionary with keys:
#   Version <library version>
#   Relvars {<list of relvar defs>}
#   Constraints {<list of constraints>}
#   Values {<list of relvar names/relation values >}
#
#   <list of relvar defs> :
#       {<relvar name> <relation heading> <list of relvar identfiers}
#
#   <list of constraints> :
#       {association | partition | correlation | procedural <constraint detail>}
#   <association constaint detail> :
#       <association name> <relvar name> {<attr list>} <mult/cond>\
#           <relvar name> {<attr list>} <mult/cond>
#   <partition constraint detail> :
#       <partition name> <supertype> {<attr list>} <subtype1> {<attr list>} ...
#   <correlation constaint detail> :
#       <?-complete?> <correlation name> <correl relvar>
#           {<attr list>} <mult/cond> <relvarA> {<attr list>}
#           {<attr list>} <mult/cond> <relvarB> {<attr list>}
#   <procedural constraint detail> :
#       <procedural name> {<relvar name list>} {<script>}
#
#   <list of relvar names/relation values> :
#       {<relvar name> <relation value>}

# Generate a string that encodes all the relvars.

proc ::ral::serialize {{pattern *}} {
    set pattern [SerializePattern $pattern]
    set rnames [BaseNames [relvar names $pattern]]
    set cnames [BaseNames [relvar constraint names $pattern]]

    set result [list]

    lappend result Version [getVersion]

    # Gather the headings and identifiers
    set relNameList [list]
    relation foreach rname $rnames {
        relation assign $rname
        lappend relNameList $Base\
            [relation heading [relvar set $Qualified]]\
            [relvar identifiers $Qualified]
    }
    lappend result Relvars $relNameList

    # Gather the contraints
    set constraints [list]
    relation foreach cname $cnames {
        relation assign $cname
        lappend constraints [getRelativeConstraintInfo $Qualified]
    }
    lappend result Constraints $constraints

    # Gather the bodies
    set bodies [list]
    relation foreach rname $rnames {
        relation assign $rname
        lappend bodies [list $Base [relvar set $Qualified]]
    }

    lappend result Values $bodies

    return $result
}

proc ::ral::serializeToFile {fileName {pattern *}} {
    set pattern [SerializePattern $pattern]
    set chan [::open $fileName w]
    catch {puts $chan [serialize $pattern]} result opts
    ::close $chan
    return -options $opts $result
}

# Restore the relvar values from a string.
proc ::ral::deserialize {value {ns ::}} {
    if {[llength $value] == 4} {
        # Assume it is 0.8.X style serialization.
        deserialize-0.8.X $value [expr {$ns eq {} ? "::" : $ns}]
        return
    }
    set ns [DeserialNS $ns]

    if {[dict size $value] != 4} {
        error "bad value format, expected dictionary of 4 items,\
                got [dict size $value] items"
    }

    if {![package vsatisfies [dict get $value Version] [getCompatVersion]]} {
        error "incompatible version number, \"$versionNumber\",\
            current library version is, \"[getVersion]\""
    }

    foreach {rvName rvHead rvIds} [dict get $value Relvars] {
        ::ral::relvar create ${ns}$rvName $rvHead {*}$rvIds
    }

    foreach constraint [dict get $value Constraints] {
        ::ral::relvar {*}[setRelativeConstraintInfo $ns $constraint]
    }

    relvar eval {
        foreach body [dict get $value Values] {
            foreach {relvarName relvarBody} $body {
                ::ral::relvar set ${ns}$relvarName $relvarBody
            }
        }
    }

    return
}

proc ::ral::deserializeFromFile {fileName {ns ::}} {
    set ns [DeserialNS $ns]
    set chan [::open $fileName r]
    catch {deserialize [read $chan] $ns} result opts
    ::close $chan
    return -options $opts $result
}

proc ::ral::deserialize-0.8.X {value {ns ::}} {
    set ns [DeserialNS $ns]
    if {[llength $value] != 4} {
        error "bad value format, expected list of 4 items,\
                got [llength $value] items"
    }
    lassign $value version relvars constraints bodies

    lassign $version versionKeyWord verNum
    if {$versionKeyWord ne "Version"} {
	error "expected keyword \"Version\", got \"$versionKeyWord\""
    }
    if {![package vsatisfies $verNum 0.8-]} {
	error "incompatible version number, \"$verNum\",\
            while attempting to deserialize version 0.8 data"
    }

    lassign $relvars relvarKeyWord revarDefs
    if {$relvarKeyWord ne "Relvars"} {
	error "expected keyword \"Relvars\", got \"$revarKeyWord\""
    }
    # In version 0.8.X, relvar headings consisted of a list
    # of 3 items: a) the "Relation" keyword, b) the relation heading
    # and c) a list of identifiers. We must adapt this to 0.9.X
    # syntax
    foreach {rvName rvHead} $revarDefs {
        lassign $rvHead keyword heading ids
        # In 0.8.X, relation valued attributes have an embedded relation
        # heading that includes a list of identifiers. So we have to
        # examine the heading for any relation valued attribute and
        # patch things up accordingly. Fortunately, the tuple valued
        # attrbutes don't have any syntax change.
        set newHeading [list]
        foreach {attrName attrType} $heading {
            lappend newHeading $attrName\
                [expr {[lindex $attrType 0] eq "Relation" ?\
                [lrange $attrType 0 1] : $attrType}]
        }
	namespace eval $ns [list ::ral::relvar create $rvName $newHeading] $ids
    }

    # Constraint syntax is unmodified between 0.8.X and 0.9.X
    lassign $constraints cnstrKeyWord cnstrDef
    if {$cnstrKeyWord ne "Constraints"} {
	error "expected keyword \"Constraints\", got \"$cnstrKeyWord\""
    }
    foreach constraint $cnstrDef {
	namespace eval $ns ::ral::relvar $constraint
    }

    lassign $bodies bodyKeyWord bodyDefs
    if {$bodyKeyWord ne "Bodies"} {
	error "expected keyword \"Bodies\", got \"$bodyKeyWord\""
    }

    # The 0.8.X serialization format uses a list of tuple values
    # to represent the body. This works fine with "relvar insert".
    relvar eval {
	foreach body $bodyDefs {
	    foreach {relvarName relvarBody} $body {
		namespace eval $ns ::ral::relvar insert [list $relvarName]\
		    $relvarBody
	    }
	}
    }

    return
}

proc ::ral::deserializeFromFile-0.8.X {fileName {ns ::}} {
    set chan [::open $fileName r]
    catch {deserialize-0.8.X [read $chan] $ns} result opts
    ::close $chan
    return -options $opts $result
}

proc ::ral::merge {value {ns ::}} {
    set ns [DeserialNS $ns]
    if {[dict size $value] != 4} {
        error "bad value format, expected dictionary of 4 items,\
                got [dict size $value] items"
    }
    if {![package vsatisfies [dict get $value Version] [getCompatVersion]]} {
        error "incompatible version number, \"$versionNumber\",\
            current library version is, \"[getVersion]\""
    }

    set newRelvars [list]
    set matchingRelvars [list]
    set mismatched [list]
    foreach {rvName rvHead rvIds} [dict get $value Relvars] {
        set fullName ${ns}$rvName
        if {![relvar exists $fullName]} {
            # New relvars are just created.
            ::ral::relvar create $fullName $rvHead {*}$rvIds
            lappend newRelvars $rvName
        } else {
            # For existing relvars we test if the type is the same.
            set mergeRelation [relation create $rvHead]
            if {[relation issametype $mergeRelation [relvar set $fullName]]} {
                lappend matchingRelvars $rvName
            } else {
                lappend mismatched $rvName
            }
        }
    }

    foreach constraint [dict get $value Constraints] {
        set cname [lindex $constraint 1]
        if {$cname eq "-complete"} {
            set cname [lindex $constraint 2]
        }
        set cname ${ns}$cname
        if {![relvar constraint exists $cname]} {
            eval ::ral::relvar [setRelativeConstraintInfo $ns $constraint]
        }
    }

    relvar eval {
        foreach body [dict get $value Values] {
            foreach {relvarName relvarBody} $body {
                if {$relvarName in $matchingRelvars} {
                    relvar uinsert ${ns}$relvarName\
                            {*}[relation body $relvarBody]
                } elseif {$relvarName in $newRelvars} {
                    relvar insert ${ns}$relvarName\
                            {*}[relation body $relvarBody]
                }
            }
        }
    }

    return $mismatched
}

proc ::ral::mergeFromFile {fileName {ns ::}} {
    set ns [DeserialNS $ns]
    set chan [::open $fileName r]
    catch {merge [read $chan] $ns} result opts
    ::close $chan
    return -options $opts $result
}

proc ::ral::storeToMk {fileName {pattern *}} {
    package require Mk4tcl
    set pattern [SerializePattern $pattern]
    set rnames [BaseNames [relvar names $pattern]]
    set cnames [BaseNames [relvar constraint names $pattern]]

    # Back up an existing file
    catch {file rename -force -- $fileName $fileName~}

    ::mk::file open db $fileName
    catch {
        # Add some versioning information into a view. Just a sanity check
        # when the data is loaded later.
        ::mk::view layout db.__ral_version {Version_ral Date_ral Comment_ral}
        ::mk::row append db.__ral_version\
            Version_ral [getVersion]\
            Date_ral [clock format [clock seconds]]\
            Comment_ral "Created by: \"[info level 0]\""
        # Create a set of views that are used as catalogs to hold
        # the relvar info that will be needed to reconstruct the values.
        ::mk::view layout db.__ral_relvar\
                {Name_ral Heading_ral Ids_ral View_ral}
        ::mk::view layout db.__ral_constraint Constraint_ral

        # Get the names of the relvars and insert them into the catalog.
        # Convert the names to be relative before inserting.  Also create the
        # views that will hold the values.  Metakit doesn't like "::" and some
        # other characters in view names.
        relation foreach rname $rnames {
            relation assign $rname
            set heading [relation heading [relvar set $Qualified]]
            set ids [relvar identifiers $Qualified]
            ::mk::row append db.__ral_relvar Name_ral $Base\
                Heading_ral $heading Ids_ral $ids View_ral $Base
            # Determine the structure of the view that will hold the relvar
            # value.  Special attention is required for Tuple and Relation
            # valued attributes.
            set relvarLayout [list]
            foreach {attr type} $heading {
                lappend relvarLayout [mkHeading $attr $type]
            }
            ::mk::view layout db.$Base $relvarLayout
        }
        # Get the constraints and put them into the catalog.
        relation foreach cname $cnames {
            relation assign $cname
            # Constrain names must be made relative.
            ::mk::row append db.__ral_constraint Constraint_ral\
                [getRelativeConstraintInfo $Qualified]
        }
        # Populate the views for each relavar.
        relation foreach rname $rnames {
            relation assign $rname
            ::mk::cursor create cursor db.$Base 0
            relation foreach r [relvar set $Qualified] {
                ::mk::row insert $cursor
                mkStoreTuple $cursor [relation tuple $r]
                ::mk::cursor incr cursor
            }
        }
    } result opts
    ::mk::file close db
    return -options $opts $result
}

proc ::ral::loadFromMk {fileName {ns ::}} {
    package require Mk4tcl

    set ns [DeserialNS $ns]
    ::mk::file open db $fileName -readonly
    catch {
        # Check that a "version" view exists and that the information
        # is consistent before we proceed.
        set version [mkCheckVersion db]
        # determine the relvar names and types by reading the catalog
        ::mk::loop rvCursor db.__ral_relvar {
            array set relvarInfo [::mk::get $rvCursor]
            # Check for old style serialization that included namespace
            # information.
            if {[string first : $relvarInfo(Name_ral)] != -1} {
                set relvarName ${ns}[namespace tail $relvarInfo(Name_ral)]
            } else {
                set relvarName ${ns}$relvarInfo(Name_ral)
            }
            ::ral::relvar create $relvarName\
                    $relvarInfo(Heading_ral) {*}$relvarInfo(Ids_ral)
        }
        # create the constraints
        ::mk::loop cnstrCursor db.__ral_constraint {
            ::ral::relvar {*}[setRelativeConstraintInfo $ns\
                    [lindex [::mk::get $cnstrCursor] 1]]
        }
        # fetch the relation values from the views
        relvar eval {
            ::mk::loop rvCursor db.__ral_relvar {
                array set relvarInfo [::mk::get $rvCursor]
                #parray relvarInfo
                ::mk::loop vCursor db.$relvarInfo(View_ral) {
                    ::ral::relvar insert ${ns}$relvarInfo(Name_ral)\
                        [mkLoadTuple $vCursor $relvarInfo(Heading_ral)]
                }
            }
        }
        set version
    } result opts
    ::mk::file close db
    return -options $opts $result
}

# Merge data from a metakit store of relvars.
# All relvars that are in the file but not currently defined are created.
# All relvars whose names and headings match currently defined relvars
# will have their relation values unioned with those in the file.
proc ::ral::mergeFromMk {fileName {ns ::}} {
    package require Mk4tcl

    set ns [DeserialNS $ns]
    ::mk::file open db $fileName -readonly
    catch {
        mkCheckVersion db
        # determine the relvar names and types by reading the catalog
        set newRelvars [list]
        set sameTypeRelvars [list]
        set mismatched [list]
        ::mk::loop rvCursor db.__ral_relvar {
            array set relvarInfo [::mk::get $rvCursor]
            set relvarName ${ns}$relvarInfo(Name_ral)
            # Check if the relvar already exists
            if {![relvar exists $relvarName]} {
                # New relvar
                ::ral::relvar create $relvarName $relvarInfo(Heading_ral)]\
                        {*}$relvarInfo(Ids_ral)
            } else {
                set typeRelation [::ral::relation create\
                        $relvarInfo(Heading_ral)]
                if {[relation issametype $typeRelation\
                        [relvar set $relvarName]]} {
                    lappend sameTypeRelvars $relvarInfo(Name_ral)
                } else {
                    lappend mismatched $relvarInfo(Name_ral)
                }
            }
        }
        # create the constraints
        ::mk::loop cnstrCursor db.__ral_constraint {
            set cinfo [lindex [::mk::get $cnstrCursor] 1]
            set cName ${ns}[lindex $cinfo 1]
            if {![relvar constraint exists $cName]} {
                ::ral::relvar {*}[setRelativeConstraintInfo $ns $cinfo]
            }
        }
        # fetch the relation values from the views
        set failedMerge [list]
        relvar eval {
            ::mk::loop rvCursor db.__ral_relvar {
                array set relvarInfo [::mk::get $rvCursor]
                if {$relvarInfo(Name_ral) in $newRelvars} {
                    set op insert
                } elseif {$relvarInfo(Name_ral) in $sameTypeRelvars} {
                    set op uinsert
                } else {
                    continue
                }
                ::mk::loop vCursor db.$relvarInfo(View_ral) {
                    ::ral::relvar $op ${ns}$relvarInfo(Name_ral)\
                        [mkLoadTuple $vCursor $relvarInfo(Heading_ral)]
                }
            }
        }
        set mismatched
    } result opts

    ::mk::file close db

    return -options $opts $result
}

# Save all the relvars that match "pattern" in to a SQLite database named
# "filename".
proc ::ral::storeToSQLite {filename {pattern *}} {
    package require sqlite3

    set pattern [SerializePattern $pattern]
    set rnames [BaseNames [relvar names $pattern]]
    set cnames [BaseNames [relvar constraint names $pattern]]

    catch {file rename -force -- $filename $filename~}
    sqlite3 [namespace current]::sqlitedb $filename
    sqlitedb eval "pragma foreign_keys=ON;"

    catch {
        # First we insert the meta-data schema and populate it.
        variable ralSQLSchema
        sqlitedb eval $ralSQLSchema
        sqlitedb transaction {
            set version [getVersion]
            sqlitedb eval {insert into __ral_version (Vnum, Date)\
                values ($version, CURRENT_TIMESTAMP) ;}
            relation foreach rname $rnames {
                relation assign $rname
                # First the relvars and their attributes and identifiers
                sqlitedb eval {insert into __ral_relvar (Vname)\
                        values ($Base) ;}
                foreach {attrName type} [relation heading\
                        [relvar set $Qualified]] {
                    sqlitedb eval {insert into __ral_attribute (Vname, Aname,\
                            Type) values ($Base, $attrName, $type) ;}
                }
                set idCounter 0
                foreach identifier [relvar identifiers $Qualified] {
                    foreach idattr $identifier {
                        sqlitedb eval {insert into\
                            __ral_identifier (IdNum, Vname, Attr)\
                            values ($idCounter, $Base, $idattr) ;}
                    }
                    incr idCounter
                }
                # Next the constraints
                foreach constraint [relvar constraint member $Qualified] {
                    set cinfo [lassign [relvar constraint info $constraint]\
                        ctype]
                    switch -exact -- $ctype {
                    association {
                        lassign $cinfo cname rfering a1 c1 refto a2 c2
                        set cname [namespace tail $cname]
                        set rfering [namespace tail $rfering]
                        set refto [namespace tail $refto]
                        sqlitedb eval {insert or ignore into __ral_association\
                            (Cname, Referring, ReferringAttrs, RefToSpec,\
                            RefTo, RefToAttrs, ReferringSpec) values\
                            ($cname, $rfering, $a1, $c1, $refto, $a2, $c2) ;}
                    }
                    partition {
                        set subs [lassign $cinfo cname super sattrs]
                        set cname [namespace tail $cname]
                        set super [namespace tail $super]
                        sqlitedb eval {insert or ignore into __ral_partition\
                            (Cname, SuperType, SuperAttrs) values\
                            ($cname, $super, $sattrs) ;}
                        foreach {subtype subattrs} $subs {
                            set subtype [namespace tail $subtype]
                            sqlitedb eval {insert or ignore into __ral_subtype\
                                (Cname, SubType, SubAttrs) values\
                                ($cname, $subtype, $subattrs) ;}
                        }
                    }
                    correlation {
                        if {[lindex $cinfo 0] eq "-complete"} {
                            set comp 1
                            set cinfo [lrange $cinfo 1 end]
                        } else {
                            set comp 0
                        }
                        lassign $cinfo cname correl ref1Attr c1 rel1 rel1Attr\
                            ref2Attr c2 rel2 rel2Attr
                        set cname [namespace tail $cname]
                        set correl [namespace tail $correl]
                        set rel1 [namespace tail $rel1]
                        set rel2 [namespace tail $rel2]
                        sqlitedb eval {insert or ignore into __ral_correlation\
                            (Cname, IsComplete, AssocRelvar, OneRefAttrs,\
                            OneRefSpec, OneRelvar, OneAttrs, OtherRefAttrs,\
                            OtherRefSpec, OtherRelvar, OtherAttrs) values\
                            ($cname, $comp, $correl, $ref1Attr, $c1, $rel1,\
                            $rel1Attr, $ref2Attr, $c2, $rel2, $rel2Attr) ;}
                    }
                    procedural {
                        set cname [namespace tail [lindex $cinfo 0]]
                        set script [lindex $cinfo end]
                        sqlitedb eval {insert or ignore into __ral_procedural\
                            (Cname, Script) values ($cname, $script) ;}
                        foreach participant [lrange $cinfo 1 end-1] {
                            set basename [namespace tail $participant]
                            sqlitedb eval {insert or ignore into\
                                __ral_proc_participant\
                                (Cname, ParticipantRelvar) values\
                                ($cname, $basename) ;}
                        }
                    }
                    default {
                        error "unknown constraint type, \"$ctype\""
                    }
                    }
                }
            }
        }
        # Next we insert the schema that corresponds to the "pattern" as an SQL
        # schema. This gives us the ability to manipulate the relvar data as
        # SQL tables.
        sqlitedb transaction exclusive {
            sqlitedb eval [sqlSchema $pattern]
        }
        # Finally we insert the values of the relvars.  There is a tricky part
        # here. Since ral attributes can be any Tcl string and since SQL is
        # particular about names, we have to account for that.  Also, we have
        # to deal with the "run time" nature of what we are doing.  We
        # construct the "insert" statement in order to fill in the attributes
        # in the required order. We also want to use the ability of the SQLite
        # Tcl bindings to fetch the values of Tcl variables. Unfortunately, (as
        # determined by experiment) SQLite cannot deal with Tcl variable syntax
        # of the form "${name}". Consequently, any character in an attribute
        # name that would terminate scanning a variable reference of the form
        # "$name" creates a problem. The solution is to use the ability of
        # "relvar assign" to assign attributes to explicitly named variables.
        # We choose as the alternate variable names the names given to the
        # SQLite columns.
        sqlitedb transaction exclusive {
            relation foreach rname $rnames {
                relation assign $rname
                set relValue [relvar set $Qualified]
                set sqlTableName [mapNamesToSQL $Base]
                set attrNames [relation attributes $relValue]
                # Map the attribute names to a set of SQLite column names.
                set sqlCols [mapNamesToSQL {*}$attrNames]
                set statement "insert into $sqlTableName ([join $sqlCols {, }])\
                    values ("
                # Create two lists here, one is used to get "relvar assign" to
                # assign attributes to variable names that SQLite can later
                # resolve. The other is just a list of those resolvable names so
                # that we can finish out the "insert" statement.
                set assignVars [list]
                set sqlVars [list]
                foreach attr $attrNames sqlCol $sqlCols {
                    lappend assignVars [list $attr $sqlCol]
                    lappend sqlVars :$sqlCol
                }
                # Finally after all of that, the actual populating of the
                # SQLite tables is trivial.
                set condAttrs [FindNullableAttrs $Qualified]
                variable nullValue
                relation foreach row $relValue {
                    # "relation assign" for a tuple or relation valued
                    # attribute gives the heading and the body. Later when we
                    # are loading the results, the "relvar insert" command will
                    # want that stripped off.
                    relation assign $row {*}$assignVars
                    # Here we have to consider the case where an attribute is
                    # referential and the relationship is condition.  In SQL we
                    # will represent that by a NULL value. Since there are not
                    # NULL values in Tcl, we have to specify the value for
                    # these types of attribute. That is done in the "nullValue"
                    # variable. So we will pass a NULL value to SQLite only if
                    # the attribute is referential to a conditional
                    # relationship and its value matches the contents of the
                    # "nullValue" variable.
                    if {[llength $condAttrs] != 0} {
                        set valueClause {}
                        foreach attr $attrNames sqlVar $sqlVars {
                            if {$attr in $condAttrs &&
                                    [set $attr] eq $nullValue} {
                                append valueClause NULL ,
                            } else {
                                append valueClause $sqlVar ,
                            }
                        }
                        set valueClause [string trimright $valueClause ,]
                    } else {
                        # This completes the composition of the "insert"
                        # statement.  We just managed to add a list of variable
                        # references (i.e.  variable names each of which is
                        # preceded by a ":").
                        set valueClause [join $sqlVars ,]
                    }
                    append valueClause ") ;"
                    sqlitedb eval $statement$valueClause
                }
            }
        }
    } result opts

    sqlitedb close
    return -options $opts $result
}

proc ::ral::loadFromSQLite {filename {ns ::}} {
    if {![file exists $filename]} {
        error "no such file, \"$filename\""
    }
    set ns [DeserialNS $ns]

    package require sqlite3
    sqlite3 [namespace current]::sqlitedb $filename
    catch {
        # First we query the meta data and rebuild the relvars and constraints.
        set version [sqlitedb onecolumn {select Vnum from __ral_version ;}]
        if {![package vsatisfies $version [getCompatVersion]]} {
            error "incompatible version number, \"$version\",\
                current library version is, \"[getVersion]\""
        }
        set relvarNames [sqlitedb eval {select Vname from __ral_relvar}]
        sqlitedb transaction {
            # The relvars
            foreach relvarName $relvarNames {
                createRelvarFromSQLite sqlitedb $ns $relvarName
            }
            # The association constraints
            foreach {cname referring referringAttrs refToSpec refTo refToAttrs\
                    referringSpec} [sqlitedb eval {select Cname, Referring,\
                    ReferringAttrs, RefToSpec, RefTo, RefToAttrs, ReferringSpec\
                    from __ral_association}] {
                ::ral::relvar association $ns$cname $ns$referring\
                        $referringAttrs $refToSpec $ns$refTo $refToAttrs\
                        $referringSpec
            }
            # The partition constraints
            foreach {cname superType superAttrs} [sqlitedb eval {select Cname,\
                    SuperType, SuperAttrs from __ral_partition}] {
                set cmd [list ::ral::relvar partition $ns$cname $ns$superType\
                        $superAttrs]
                foreach {subtype subattr} [sqlitedb eval {select Subtype,\
                        SubAttrs from __ral_subtype where Cname = $cname}] {
                    lappend cmd $ns$subtype $subattr
                }
                eval $cmd
            }
            # The correlation contraints
            foreach {cname isComplete assocRelvar oneRefAttrs oneRefSpec\
                    oneRelvar oneAttrs otherRefAttrs otherRefSpec otherRelvar\
                    otherAttrs}\
                    [sqlitedb eval {select Cname, IsComplete, AssocRelvar,\
                    OneRefAttrs, OneRefSpec, OneRelvar, OneAttrs,\
                    OtherRefAttrs, OtherRefSpec, OtherRelvar, OtherAttrs\
                    from __ral_correlation}] {
                set corrCmd [list\
                    ::ral::relvar correlation $ns$cname\
                        $ns$assocRelvar $oneRefAttrs $oneRefSpec $ns$oneRelvar\
                        $oneAttrs $otherRefAttrs $otherRefSpec $ns$otherRelvar\
                        $otherAttrs]
                if {$isComplete} {
                    set corrCmd [linsert $corrCmd 3 -complete]
                }
                eval $corrCmd
            }
            # The procedural contraints
            foreach {cname script}\
                [sqlitedb eval {select Cname, Script from __ral_procedural}] {
                # Create a list of the participating relvars
                set partRels [list]
                foreach participantRelvar\
                    [sqlitedb eval {select ParticipantRelvar from\
                        __ral_proc_participant where (Cname = $cname)}] {
                    lappend partRels $ns$participantRelvar
                }
                ::ral::relvar procedural $ns$cname {*}$partRels $script
            }
        }
        # Now insert the data from the tables.
        # We are careful here not to depend upon SQL column ordering.
        relvar eval {
            foreach vname $relvarNames {
                loadRelvarFromSQLite sqlitedb $ns $vname insert
            }
        }
    } result opts

    sqlitedb close
    return -options $opts $result
}
# Merge data from a SQLite store of relvars.
# All relvars that are in the database but not currently defined are created.
# All relvars whose names and headings match currently defined relvars
# will have their relation values unioned with those in the file.
proc ::ral::mergeFromSQLite {filename {ns ::}} {
    package require sqlite3

    if {![file exists $filename]} {
        error "no such file, \"$filename\""
    }
    set ns [DeserialNS $ns]
    sqlite3 [namespace current]::sqlitedb $filename
    catch {
        # First we query database to find out its version and the
        # relvars it contains.
        set version [sqlitedb onecolumn {select Vnum from __ral_version ;}]
        if {![package vsatisfies $version [getCompatVersion]]} {
            error "incompatible version number, \"$version\",\
                current library version is, \"[getVersion]\""
        }
        # Compare the contents of the database to that already present.  We are
        # looking for things in the database that are not already present.
        # First we look for new relvars.
        set dbrelvars [sqlitedb eval {select Vname from __ral_relvar}]
        set newRelvars [list]
        set sameTypeRelvars [list]
        set mismatched [list]
        foreach dbrelvar $dbrelvars {
            set qualRelvar ${ns}$dbrelvar
            if {![::ral::relvar exists $qualRelvar]} {
                # New relvar, create it.
                createRelvarFromSQLite sqlitedb $ns $dbrelvar
                lappend newRelvars $dbrelvar
            } else {
                # Determine if the heading of the relvar in the database
                # matches that of the existing relvar. We will do this
                # by creating an empty relation with each heading and
                # invoke "relation issametype" to do the comparison.
                # If that returns true, the we will keep a list of those
                # that have the same type.
                set heading [sqlitedb eval {select Aname, Type from\
                        __ral_attribute where Vname = $dbrelvar}]
                set dbrel [::ral::relation create $heading]
                if {[::ral::relation issametype $dbrel\
                        [relvar set $qualRelvar]]} {
                    lappend sameTypeRelvars $dbrelvar
                }
            }
        }
        # Next do the same for constraints.
        # We consider each different type of association.
        # First associations
        set dbassocs [sqlitedb eval {select Cname from __ral_association}]
        foreach dbassoc $dbassocs {
            if {![::ral::relvar constraint exists ${ns}$dbassoc]} {
                createAssocFromSQLite sqlitedb $ns $dbassoc
            }
        }
        # Next partitions
        set dbparts [sqlitedb eval {select Cname from __ral_partition}]
        foreach dbpart $dbparts {
            if {![::ral::relvar constraint exists ${ns}$dbpart]} {
                createPartitionFromSQLite dsqlitedb $ns $dbpart
            }
        }
        # Next correlations
        set dbcorrels [sqlitedb eval {select Cname from __ral_correlation}]
        foreach dbcorrel $dbcorrels {
            if {![::ral::relvar constraint exists ${ns}$dbcorrel]} {
                createCorrelFromSQLite dbsqlitedb $ns $dbcorrel
            }
        }
        # Finally procedural constraints
        set dbprocs [sqlitedb eval {select Cname from __ral_procedural}]
        foreach dbproc $dbprocs {
            if {![::ral::relvar constraint exists ${ns}$dbproc]} {
                createProcFromSQLite dbsqlitedb $ns $dbproc
            }
        }
        # Now we populate the merge from the SQLite data base.  Newly created
        # relvars, as contained in the "newRelvars" variable can be loaded
        # straight in. For other relvars, we must verify that he heading is the
        # same as that in the database. For those that match, we union in the
        # data base contents.  N.B. that we use "uinsert" to merge in data base
        # contents.  Using "relvar union" has some corner cases that are not
        # what you might expect (i.e. the union might fail identity contraints
        # for those tuples which have the same identifier values but different
        # values for the other attributes.
        relvar eval {
            foreach dbrelvar $dbrelvars {
                if {$dbrelvar in $newRelvars} {
                    loadRelvarFromSQLite sqlitedb $ns $dbrelvar insert
                } elseif {$dbrelvar in $sameTypeRelvars} {
                    loadRelvarFromSQLite sqlitedb $ns $dbrelvar uinsert
                }
                # Else it must be that the type of the relvar in the database
                # did not match that of an existing relvar. So it is skipped.
            }
        }
        set mismatched
    } result opts

    sqlitedb close
    return -options $opts $result
}

proc ::ral::createRelvarFromSQLite {db ns relvarName} {
    set heading [list]
    foreach {aname type} [$db eval {select Aname, Type from\
            __ral_attribute where Vname = $relvarName}] {
        lappend heading $aname $type
    }
    set idents [dict create]
    foreach {idnum attrName} [$db eval {select IdNum, Attr\
            from __ral_identifier where Vname = $relvarName}] {
        dict lappend idents $idnum $attrName
    }
    ::ral::relvar create $ns$relvarName $heading {*}[dict values $idents]
}

proc ::ral::loadRelvarFromSQLite {db ns relvarName operation} {
    set sqlTableName [mapNamesToSQL $relvarName]
    set qualName $ns$relvarName
    set heading [relation heading [relvar set $qualName]]
    set attrNames [relation attributes [relvar set $qualName]]
    set sqlColNames [mapNamesToSQL {*}$attrNames]
    $db eval "select [join $sqlColNames {, }]\
            from $sqlTableName" valArray {
        # Build up the insert tuple
        set insert [list]
        # Note we have to adjust the values for Tuple and
        # Relation valued attributes. The "relvar insert"
        # command just wants the bodies of the values.
        foreach {attr type} $heading sqlCol $sqlColNames {
            lappend insert $attr
            switch -glob -- $type {
                Relation* {
                    lappend insert\
                            [relation body $valArray($sqlCol)]
                }
                Tuple* {
                    lappend insert [tuple get $valArray($sqlCol)]
                }
                default {
                    lappend insert $valArray($sqlCol)
                }
            }
        }
        ::ral::relvar $operation $qualName $insert
    }
}

proc ::ral::createAssocFromSQLite {db ns assocName} {
    # Fetch association from the data base.
    set assocElems [$db eval {select Cname, Referring,\
            ReferringAttrs, RefToSpec, RefTo, RefToAttrs, ReferringSpec\
            from __ral_association where Cname = $assocName}]
    # Put the keyword at the beginning.
    set assocElems [linsert $assocElems 0 association]
    # Create the association making the names fully qualified.
    eval ::ral::relvar [setRelativeConstraintInfo $ns $assocElems]
}

proc ::ral::createPartitionFromSQLite {db ns partitionName} {
    set partitionElems [$db eval {select Cname, SuperType, SuperAttrs\
            from __ral_partition where Cname = $partitionName}]
    foreach {subtype subattr} [$db eval {select Subtype,\
            SubAttrs from __ral_subtype where Cname = $cname}] {
        lappend partitionElems $subtype $subattr
    }
    set paritionElems [linsert $partitionElems 0 partition]
    eval ::ral::relvar [setRelativeConstraintInfo $ns $partitionElems]
}

proc ::ral::createCorrelFromSQLite {db ns correlName} {
    set correlElems [$db eval {select Cname, IsComplete, AssocRelvar,\
            OneRefAttrs, OneRefSpec, OneRelvar, OneAttrs,\
            OtherRefAttrs, OtherRefSpec, OtherRelvar, OtherAttrs\
            from __ral_correlation where Cname = $correlName}]
    if {[lindex $correlElems 2] == "true"} {
        lset correlElems 2 -complete
    }
    set correlElems [linsert $correlElems 0 correlation]
    eval ::ral::relvar [setRelativeConstraintInfo $ns $correlElems]
}

proc ::ral::createProcFromSQLite {db ns procName} {
    set script [$db eval {select Script from __ral__procedural\
            where Cname = $procName}]
    set participants [$db eval {select ParticipantRelvar from\
            __ral__proc_participant where Cname = $procName}]
    set procElems [list procedural $procName {*}$participants $scrip]
    eval ::ral::relvar [setRelativeConstraintInfo $ns $procElems]
}

proc ::ral::dump {{pattern *}} {
    set pattern [SerializePattern $pattern]
    set result {}
    set names [lsort [relvar names $pattern]]

    append result "# Generated via ::ral::dump\n"
    append result "package require ral [getVersion]\n"

    # Convert the names to be relative
    # Strip any leading "::" from names so that if the script is
    # sourced in, names are created relative to the namespace of the
    # source.
    foreach name $names {
        set rName [namespace tail $name]
        append result "::ral::relvar create $rName\
            [list [relation heading [relvar set $name]]]\
            [relvar identifiers $name]" \n
    }

    foreach cname [lsort [relvar constraint names $pattern]] {
        append result "::ral::relvar [getRelativeConstraintInfo $cname]\n"
    }

    # perform the inserts inside of a transaction.
    append result "::ral::relvar eval \{\n"
    foreach name $names {
        append result\
            "    ::ral::relvar set [namespace tail  $name]\
                [list [relvar set $name]]" \n
    }
    append result "\}"

    return $result
}

proc ::ral::dumpToFile {fileName {pattern *}} {
    set chan [::open $fileName w]
    catch {puts $chan [dump $pattern]} result opts
    ::close $chan
    return -options $opts $result
}

proc ::ral::csv {relValue {sortAttr {}} {noheading 0}} {
    package require csv

    set m [relation2matrix $relValue $sortAttr $noheading]
    catch {::csv::report printmatrix $m} result opts
    $m destroy
    return -options $opts $result
}

proc ::ral::csvToFile {relValue fileName {sortAttr {}} {noheading 0}} {
    package require csv

    set m [relation2matrix $relValue $sortAttr $noheading]
    set chan [::open $fileName w]
    catch {::csv::report printmatrix2channel $m $chan} result opts
    $m destroy
    ::close $chan
    return -options $opts $result
}

proc ::ral::FindNullableAttrs {name} {
    # Find the attributes of the relvar that participate in
    # a conditional association. This is the one and only case
    # where we will allow the attribute to be NULL.
    set condAttrs [list]
    foreach constraint [relvar constraint member $name] {
        set info [lassign [relvar constraint info $constraint] ctype]
        if {$ctype eq "association"} {
            lassign $info cname rfering a1 c1 refto a2 c2
            if {$name eq $rfering && $c2 eq "?"} {
                lappend condAttrs {*}$a1
            }
        }
    }
    return $condAttrs
}

# Returns a string that can be fed to SQLite that will create the necessary
# tables and contraints for the relvars that match "pattern".
proc ::ral::sqlSchema {{pattern *}} {
    set names [lsort [relvar names $pattern]]

    append result "-- Generated via ::ral::sqlSchema\n"
    append result "pragma foreign_keys=ON;\n"

    # Convert the names to be relative.  Strip any leading namespace qualifiers
    # from names so that if the script is sourced in, names are created
    # relative to the namespace of the source.
    foreach name $names {
        set indices {}
        set value [relvar set $name]
        set baseName [namespace tail $name]
        set sqlBaseName [mapNamesToSQL $baseName]

        append result "create table $sqlBaseName (\n"
        set condAttrs [FindNullableAttrs $name]
        # Define table attributes.
        foreach {attr type} [relation heading $value] {
            # N.B. need to map type from Tcl type to SQL type
            # Need to figure out what to do with Tuple and Relation type
            # attributes.
            append result "    [mapNamesToSQL $attr] [mapTypeToSQL $type]"
            if {$attr ni $condAttrs} {
                append result " not null"
            }
            append result ",\n"
        }
        # Define identification constraints.
        set idNum 0
        foreach id [relvar identifiers $name] {
            set id [join [mapNamesToSQL {*}$id] {, }]
            append result "    constraint ${sqlBaseName}_ID[incr idNum]\
                    unique ($id),\n"
        }
        # Define the referential constraints.
        foreach constraint [relvar constraint member $name] {
            set info [relvar constraint info $constraint]
            set info [lassign $info ctype]
            switch -exact -- $ctype {
                association {
                    lassign $info cname rfering a1 c1 refto a2 c2
                    set rfering [namespace tail $rfering]
                    if {$rfering eq $baseName} {
                        set a1 [join [mapNamesToSQL {*}$a1] {, }]
                        set cname [mapNamesToSQL [namespace tail $cname]]
                        set refto [namespace tail $refto]
                        append result "    constraint ${sqlBaseName}_$cname\
                            foreign key ($a1)\
                            references $refto\
                                ([join [mapNamesToSQL {*}$a2] {, }])\n"\
                            "        on delete [expr {$c2 eq "?" ?\
                                "set null" : "cascade"}]\
                            on update cascade\
                            deferrable initially deferred,\n"
                        append indices "create index\
                            ${sqlBaseName}_${cname}INDEX\
                            on $sqlBaseName ($a1) ;\n"
                    }
                }
                partition {
                    # The best we can do for a partition is to make it one to
                    # one conditional subtypes referring to the supertype. This
                    # will not enforce disjointedness, but it's SQL after all.
                    set subs [lassign $info cname super sattrs]
                    foreach {subrel subattrs} $subs {
                        set rfering [namespace tail $subrel]
                        if {$baseName eq $rfering} {
                            set cname [mapNamesToSQL [namespace tail $cname]]
                            set refto [namespace tail $super]
                            set subattrs [join [mapNamesToSQL {*}$subattrs] {, }]
                            append result "    constraint ${sqlBaseName}_$cname\
                                foreign key ($subattrs)\
                                references $refto\
                                    ([join [mapNamesToSQL {*}$sattrs] {, }])\n"\
                                "        on delete cascade on update cascade\
                                deferrable initially deferred,\n"
                            append indices "create index\
                                ${sqlBaseName}_${cname}INDEX on\
                                $sqlBaseName ($subattrs) ;\n"
                            break
                        }
                    }
                }
                correlation {
                    # The correlation constraint shows up as two foreign
                    # key references on the correlation table.
                    lassign $info cname correl ref1Attr c1 rel1 rel1Attr\
                        ref2Attr c2 rel2 rel2Attr
                    set correl [mapNamesToSQL [namespace tail $correl]]
                    if {$baseName eq $correl} {
                        set cname [mapNamesToSQL [namespace tail $cname]]
                        set rel1 [namespace tail $rel1]
                        set ref1Attr [join [mapNamesToSQL {*}$ref1Attr] {, }]
                        append result "    constraint\
                                ${sqlBaseName}_${cname}_${rel1}\
                            foreign key ($ref1Attr)\
                            references $rel1\
                                ([join [mapNamesToSQL {*}$rel1Attr] {, }])\n"\
                            "        on delete cascade on update cascade\
                            deferrable initially deferred,\n"
                        append indices "create index\
                            ${sqlBaseName}_${cname}INDEX0 on $sqlBaseName\
                            ($ref1Attr) ;\n"

                        set rel2 [namespace tail $rel2]
                        set ref2Attr [join [mapNamesToSQL {*}$ref2Attr] {, }]
                        append result "    constraint\
                                ${sqlBaseName}_${cname}_${rel2}\
                            foreign key ($ref2Attr)\
                            references $rel2\
                                ([join [mapNamesToSQL {*}$rel2Attr] {, }])\n"\
                            "        on delete cascade on update cascade\
                            deferrable initially deferred,\n"
                        append indices "create index\
                            ${sqlBaseName}_${cname}INDEX1 on $sqlBaseName\
                            ($ref2Attr) ;\n"
                    }
                }
                procedural {
                    # Procedural constraints do not add anything to the SQL
                    # schema since they are not "foreign key" oriented.
                }
                default {
                    error "unknown constraint type, \"$ctype\""
                }
            }
        }
        set result [string trimright $result ,\n]
        append result ") ;\n"
        append result $indices
    }

    return $result
}

# With Tcl 8.5, then we can supply some "aggregate scalar functions" that
# are useful and have "expr" syntax.
# Count the number of tuples in a relation
proc ::tcl::mathfunc::rcount {relation} {
    return [::ral::relation cardinality $relation]
}
# Count the number of distinct values of an attribute in a relation
proc ::tcl::mathfunc::rcountd {relation attr} {
    return [::ral::relation cardinality\
        [::ral::relation project $relation $attr]]
}
# Compute the sum over an attribute
proc ::tcl::mathfunc::rsum {relation attr} {
    set result 0
    foreach v [::ral::relation list $relation $attr] {
        incr result $v
    }
    return $result
}
# Compute the sum over the distinct values of an attribute.
proc ::tcl::mathfunc::rsumd {relation attr} {
    set result 0
    ::ral::relation foreach v [::ral::relation list\
        [::ral::relation project $relation $attr]] {
        incr result $v
    }
    return $result
}
# Compute the average of the values of an attribute
proc ::tcl::mathfunc::ravg {relation attr} {
    return [expr {rsum($relation, $attr) / rcount($relation)}]
}
# Compute the average of the distinct values of an attribute
proc ::tcl::mathfunc::ravgd {relation attr} {
    return [expr {rsumd($relation, $attr) / rcount($relation)}]
}
# Compute the minimum. N.B. this does not handle "empty" relations properly.
proc ::tcl::mathfunc::rmin {relation attr} {
    set values [::ral::relation list $relation $attr]
    set min [lindex $values 0]
    foreach v [lrange $values 1 end] {
        if {$v < $min} {
            set min $v
        }
    }
    return $min
}
# Again, empty relations are not handled properly.
proc ::tcl::mathfunc::rmax {relation attr} {
    set values [::ral::relation list $relation $attr]
    set max [lindex $values 0]
    foreach v [lrange $values 1 end] {
        if {$v > $max} {
            set max $v
        }
    }
    return $max
}

# PRIVATE PROCS

# Clean up a namespace name for deserialization and append trailing colons.
# Eval a null script in the namespace to make sure to create it
# paying careful attention to non-qualified names.
proc ::ral::DeserialNS {ns} {
    if {[string range $ns 0 1] ne "::"} {
        set callerns [string trimright [uplevel 2 namespace current] :]
        set ns ${callerns}::$ns
    }
    set ns [string trimright $ns :]::
    namespace eval $ns {}
    return $ns
}

proc ::ral::SerializePattern {pattern} {
    if {[string range $pattern 0 1] ne "::"} {
        set callerns [string trimright [uplevel 2 namespace current] :]
        set pattern ${callerns}::$pattern
    }
    return $pattern
}

# Returns a relation value with the heading {Qualified string Base string}
# that represents the fully qualified relvar names and their corresponding
# leaf base names. Checks that all the relvars that "names" have unique base
# names.
proc ::ral::BaseNames {names} {
    set rnames [relation extend [relation fromlist $names Qualified string]\
        rn Base string {
            [namespace tail [tuple extract $rn Qualified]]
        }]

    set bnames [relation list [relation project $rnames Base]]

    if {[relation cardinality $rnames] != [llength $bnames]} {
        error "set of relvar names results in duplicated base names, \"$names\""
    }

    return $rnames
}

# Add heading rows to the matrix.
proc ::ral::addHeading {matrix heading} {
    set attrNames [list]
    set attrTypes [list]
    foreach {name type} $heading {
        lappend attrNames $name
        # For Relation and Tuple types, just use the keyword.
        # The components of the types will be apparent from the headings
        # of the relation or tuple valued attributes. This saves quite
        # a bit of column space for these types of nested attributes.
        lappend attrTypes [lindex $type 0]
    }
    $matrix add row $attrNames
    $matrix add row $attrTypes

    return
}

# Returns a relation mapping attribute names to a formatting function.
# Ordinary scalar values attributes are not contained in the mapping. Relation
# and tuple valued attributes will be in the relation keyed by the attribute
# name with values corresponding to the "relformat" or "tupleformat" command.
proc ::ral::getFormatMap {heading} {
    set attrReportMap {{AttrName string AttrFunc string} {}}
    foreach {name type} $heading {
        set typeKey [lindex $type 0]
        if {$typeKey eq "Tuple"} {
            set attrReportMap [relation insert $attrReportMap\
                    [list AttrName $name AttrFunc ::ral::tupleformat]]
        } elseif {$typeKey eq "Relation"} {
            set attrReportMap [relation insert $attrReportMap\
                    [list AttrName $name AttrFunc ::ral::relformat]]
        }
    }
    return $attrReportMap
}

# Add tuple values to a matrix
proc ::ral::addTuple {matrix tupleValue attrMap} {
    set values [list]
    foreach {attr value} [tuple get $tupleValue] {
        set mapping [relation restrictwith $attrMap {$AttrName eq $attr}]
        if {[relation isnotempty $mapping]} {
            set attrfunc [relation extract $mapping AttrFunc]
            set value [$attrfunc $value]
        } else {
            # Limit the width of scalar values. We use the "textutil"
            # package to wrap the text to "maxColLen" characters.
            variable maxColLen
            if {[string length $value] > $maxColLen} {
                package require textutil
                set value [::textutil::adjust $value -justify left\
                    -length $maxColLen -strictlength true]
            }
        }
        lappend values $value
    }
    $matrix add row $values

    return
}

proc ::ral::tupleValue {tuple} {
    set result [list]
    foreach {attr type} [tuple heading $tuple]\
        {attr value} [tuple get $tuple] {
        switch [lindex $type 0] {
            Tuple {
                set value [tupleValue $value]
            }
            Relation {
                set value [relationValue $value]
            }
        }
        lappend result $attr $value
    }

    return $result
}

proc ::ral::relationValue {relation} {
    set result [list]
    relation foreach r $relation {
        lappend result [tupleValue [relation tuple $r]]
    }

    return $result
}

proc ::ral::mkCheckVersion {dbName} {
    # Check that a "version" view exists and that the information
    # is consistent before we proceed.
    set views [::mk::file views $dbName]
    if {[lsearch $views __ral_version] < 0} {
        error "Cannot find TclRAL catalogs:\
            file may not contain relvar information"
    }
    set result [tuple create\
        {Version_ral string Date_ral string Comment_ral string}\
        [::mk::get $dbName.__ral_version!0]]
    set verNum [tuple extract $result Version_ral]
    if {![package vsatisfies $verNum [getCompatVersion]]} {
        error "incompatible version number, \"$verNum\",\
            current library version is, \"[getVersion]\""
    }
    return $result
}

proc ::ral::mkHeading {attr type} {
    switch [lindex $type 0] {
        Tuple -
        Relation {
            set subHead [list]
            foreach {subattr subtype} [lindex $type 1] {
                lappend subHead [mkHeading $subattr $subtype]
            }
            set result [list $attr $subHead]
        }
        default {
            set result $attr
        }
    }
    return $result
}

proc ::ral::mkStoreTuple {cursor tuple} {
    foreach {attr type} [tuple heading $tuple] {attr value} [tuple get $tuple] {
        switch -exact [lindex $type 0] {
            Tuple {
                ::mk::cursor create tupCursor $cursor.$attr
                ::mk::row insert $tupCursor
                mkStoreTuple $tupCursor [tuple extract $tuple $attr]
            }
            Relation {
                mkStoreRelation $cursor.$attr [tuple extract $tuple $attr]
            }
            default {
                ::mk::set $cursor $attr $value
            }
        }
    }
    return
}

proc ::ral::mkStoreRelation {cursor relation} {
    ::mk::cursor create relCursor $cursor 0
    ::mk::row insert $relCursor [relation cardinality $relation]
    relation foreach r $relation {
        mkStoreTuple $relCursor [relation tuple $r]
        ::mk::cursor incr relCursor
    }
    return
}

proc ::ral::mkLoadTuple {cursor heading} {
    set value [list]
    foreach {attr type} $heading {
        switch -exact [lindex $type 0] {
            Tuple {
                lappend value $attr [mkLoadTuple $cursor.$attr!0\
                        [lindex $type 1]]
            }
            Relation {
                lappend value $attr [mkLoadRelation $cursor.$attr\
                        [lindex $type 1]]
            }
            default {
                lappend value $attr [::mk::get $cursor $attr]
            }
        }
    }
    return $value
}

proc ::ral::mkLoadRelation {cursor heading} {
    set value [list]
    ::mk::loop rCursor $cursor {
        lappend value [mkLoadTuple $rCursor $heading]
    }
    return $value
}

namespace eval ::ral {
    # The set of indices into the constraint information where fully qualified
    # path names exist.
    variable assocIndices {1 2 5}
    variable correlIndices {1 2 5 9}
    variable compCorrelIndices {2 3 6 10}
}

proc ::ral::getRelativeConstraintInfo {cname} {
    set cinfo [relvar constraint info $cname]
    switch -exact [lindex $cinfo 0] {
        association {
            variable assocIndices
            foreach index $assocIndices {
                lset cinfo $index [namespace tail [lindex $cinfo $index]]
            }
        }
        partition {
            lset cinfo 1 [namespace tail [lindex $cinfo 1]]
            lset cinfo 2 [namespace tail [lindex $cinfo 2]]
            for {set index 4} {$index < [llength $cinfo]} {incr index 2} {
                lset cinfo $index [namespace tail [lindex $cinfo $index]]
            }
        }
        correlation {
            variable correlIndices
            variable compCorrelIndices
            set cIndices [expr {[lindex $cinfo 1] eq "-complete" ?\
                $compCorrelIndices : $correlIndices}]
            foreach index $cIndices {
                lset cinfo $index [namespace tail [lindex $cinfo $index]]
            }
        }
        procedural {
            set endIndex [expr {[llength $cinfo] - 1}]
            for {set index 1} {$index < $endIndex} {incr index} {
                lset cinfo $index [namespace tail [lindex $cinfo $index]]
            }
        }
        default {
            error "unknown constraint type, \"[lindex $cinfo 0]\""
        }
    }
    return $cinfo
}

proc ::ral::setRelativeConstraintInfo {ns cinfo} {
    switch -exact -- [lindex $cinfo 0] {
        association {
            variable assocIndices
            foreach index $assocIndices {
                lset cinfo $index ${ns}[lindex $cinfo $index]
            }
        }
        partition {
            lset cinfo 1 ${ns}[lindex $cinfo 1]
            lset cinfo 2 ${ns}[lindex $cinfo 2]
            for {set index 4} {$index < [llength $cinfo]} {incr index 2} {
                lset cinfo $index ${ns}[lindex $cinfo $index]
            }
        }
        correlation {
            variable correlIndices
            variable compCorrelIndices
            set cIndices [expr {[lindex $cinfo 1] eq "-complete" ?\
                $compCorrelIndices : $correlIndices}]
            foreach index $cIndices {
                lset cinfo $index ${ns}[lindex $cinfo $index]
            }
        }
        procedural {
            set endIndex [expr {[llength $cinfo] - 1}]
            for {set index 1} {$index < $endIndex} {incr index} {
                lset cinfo $index\
                        ${ns}[lindex $cinfo $index]
            }
        }
        default {
            error "unknown constraint type, \"[lindex $cinfo 0]\""
        }
    }

    return $cinfo
}

# Handling SQL names is a pain. We must be prepared to deal with all the
# keywords and other characters. You might be tempted to use the SQL quoting
# mechanisms but this interfers with Tcl quoting in strange ways especially
# with embedded white space. We are luck in the sense that we only have to map
# the names to something SQL likes and not the inverse. It will make the names
# in a SQLite database appear different than those in relvars, but they are not
# so different as to be unrecognizable.
#
# So the convention is that SQL keywords have an "_K" append to them and all
# other names have anything that is _not_ an alphanumber or the underscore
# changed to an underscore.

namespace eval ::ral {
    # This list is sorted alphabetically and the "lsearch" invocation below
    # depends upon that sorting.
    variable sqlKeywords {
        ABORT ACTION ADD AFTER ALL ALTER ANALYZE AND AS ASC ATTACH AUTOINCREMENT
        BEFORE BEGIN BETWEEN BY
        CASCADE CASE CAST CHECK COLLATE COLUMN COMMIT CONFLICT CONSTRAINT
            CREATE CROSS CURRENT_DATE CURRENT_TIME CURRENT_TIMESTAMP
        DATABASE DEFAULT DEFERRABLE DEFERRED DELETE DESC DETACH DISTINCT DROP
        EACH ELSE END ESCAPE EXCEPT EXCLUSIVE EXISTS EXPLAIN
        FAIL FOR FOREIGN FROM FULL
        GLOB GROUP
        HAVING
        IF IGNORE IMMEDIATE IN INDEX INDEXED INITIALLY INNER INSERT INSTEAD
            INTERSECT INTO IS ISNULL
        JOIN
        KEY
        LEFT LIKE LIMIT
        MATCH
        NATURAL NO NOT NOTNULL NULL
        OF OFFSET ON OR ORDER OUTER
        PLAN PRAGMA PRIMARY
        QUERY
        RAISE RECURSIVE REFERENCES REGEXP REINDEX RELEASE RENAME REPLACE
            RESTRICT RIGHT ROLLBACK ROW
        SAVEPOINT SELECT SET
        TABLE TEMP TEMPORARY THEN TO TRANSACTION TRIGGER
        UNION UNIQUE UPDATE USING
        VACUUM VALUES VIEW VIRTUAL
        WHEN WHERE WITH WITHOUT
    }
}

proc ::ral::mapNamesToSQL {args} {
    variable sqlKeywords
    set newNames [list]
    foreach name $args {
        if {[lsearch -exact -sorted -increasing -ascii -nocase $sqlKeywords\
                $name] != -1} {
            set name ${name}_K
        } else {
            set name [regsub -all -- {[^[:alnum:]_]} $name _]
        }
        lappend newNames $name
    }
    return $newNames
}

proc ::ral::mapTypeToSQL {type} {
    variable sqlTypeMap
    return [expr {[info exists sqlTypeMap($type)] ?\
            $sqlTypeMap($type) : "text"}]
}

package provide ral 0.12.2
ELF          >    К      @       ��         @ 8  @                                                         �     �#     �#            �\                    �-     �-#     �-#     �      �                   �      �      �      $       $              P�td   Н     Н     Н     l      l             Q�td                                                  R�td   �     �#     �#     x      x                      GNU *��f%�����꘲B�&BH�                 F�@�O$G�
@ B��
%i*                                                                        #               %           '   )   *   ,   -   .   /   0   2               3   7       8   ;       <   ?       B   C       D       E   F   G   I                       J       L   M   O       P       Q   R   S       T   U       V   W   X       Y   [   ^       _                   `   a   b   d   e   g   h   k   m       n   q       s           u           v   w   y   {   |       ~           �   �   �   �   �   �   �   �   �       �   �   �       �   �       �   �   �       �   �   �   �           �   �   �   �                   �   �   �               �   �   �   �   �   �       �       �           �       �   �   �           �   �       �           �           �   �   �   �       �   �   �   �       �       �   �   �       �   �   �       �   �       �   �   �   �       �   �       �   �   �   �   �       �   �   �       �   �   �                               	  
�Mi��SXn�d���tt�
�<[N(-�
��K�V{Y�k�"TM]Mp�x!�������|.��w"1���X�PS��w˭YwO�:.�s���!2s��~$��s@"�}z��
S�{H%e�|�<\�+7�+��lj��T�\�:->���YG�i݁�%����!o���t�BCE��o�e�YH�#���D���Y/�������\�\8��|��-�)��`�S>�����>��_E0������Q"��M�T��������#���C���솟�Ce[d�o9��hV�Ay̪V+ig��f��8���V�+�ұ���ŏ��T�qP8����i�v�
��[��mPt	�_���O���
�(v��?�"��Y��x7�@GJ��2�H�w�+�_i;Ƥ�s��G� J�A��ʾ�*�j�K	�p�e�nנ�}���DU��W��z
��v^p�g+Ő��Ư<�	��2_�t�egl��󡕦�Ej�n���Q�R�Q��n>�
    ��      �       �    �\     F           0_     �       ?    �t     )       b     �      g       ~    ��      J       �
    @�      s       4    ��     �       �    ��      �       U    �b     P           ��     :       ~    �_     @       �     `�      d       V    ��      C       �    �a     E       �    0�     }       �    @�      �       �          l       \    �      h       �    {#             �    �u     ,       �     ��             �    �     A       �    p     e       �    �Y     O       �    pb     t       �    �Z     :       k    �^     �       +     k     A           �j     "       G    0�      S       �     \     B       �    ��     b      �          t       3    0g            W    �]           
    �C     Z      S	    p�      4      �     b     n       v    �     y       #    ��      B       �    0�      �       y    ��      S       �    �
    ph     H       /    �w     O       �    �:#                  W     J       �    `�             �    0     P      �    �     �       t    �	           �    p�     �       �    @=     �       �    ��     v      �
    PJ     7       ;    ��      C       u     Л      �      �    �     ;       �
    `�      �      �    �	     C       �    ��      k       ,    �     �      �    `     �       �    `N     �      �    ��      �       �    @�      ^       E    �[             �    @`     �       h    0�     {       W    P	     J       �
    pB     a      '
    �e     `       �    ��     0      �     H     �       $          (       L    �w     i       �    �v     E       |
    �     �       �    P     �       �    �;            �	    �8#     (           p\     1           ��      N       R
     �      -      4     �            �    ��      @      ,    �      4          �H     �       B    @]     D       M     �      C      C    `     "       g    PW     B       5    �      V       �     �s     �       �    pt                �     �       X    �t     #       &     9#     (       �	    P�      ^           0[            �     0^     �       u
     �            �     ��             �    в      b       �    `a     C       :    ��     T      �     a     @       �    �     {       ?
    0�      �          ��     ;      H          �      {    ��     �       	    ��      8      *    PQ     �      f    �     :       �	    �      �       #    ��      )       <	    P;     P       �         :       �    0     Q       B    p�     b      �     G     �       >     A     �       0    p�      �      6    �      �           �      �       ;

     p!#            �~     x!#            �;     �!#            �~     �!#            0L     �!#            �~     �!#            �#     �!#            �~     �!#            pZ     �!#            �~     �!#            pV     �!#            �     �!#            @T     �!#            �~     �!#            pS     �!#            �     �!#            �     �!#            �~     �!#            �Q      "#            �~     "#                 "#            �~     "#            �      "#                 ("#            P     0"#                 8"#            0     @"#            E     H"#            0�      P"#            �     X"#            0I     `"#            N     h"#             H     p"#                 x"#            F     �"#                 �"#            pB     �"#            $     �"#            �@     �"#            +     �"#            �     �"#            4     �"#            �     �"#            A     �"#            ��      �"#            J     �"#            ��      �"#            T     �"#            �5     �"#            ^     �"#            P/      ##            j     ##            �-     ##            p     ##             (      ##            t     (##            �"     0##            {     8##             !     @##            w�     H##                  P##            �     X##            0     `##            �     h##                  p##            �~     x##            0     �##            �     �##                  �##            �     �##            �
          �0#                   �0#        "          �0#                   �0#        ~           �0#        k           �0#        �           �0#        �           �0#        $          �0#                    1#        W           1#        {           1#        �           1#        �            1#        q           (1#                  01#        *           81#                  @1#        �           H1#        0           P1#        <           X1#        �           `1#        �           h1#        #          p1#                   x1#        g           �1#        n           �1#                   �1#        \           �1#        Y           �1#        o           �1#        �           �1#        8           �1#        �           �1#        r           �1#        h           �1#                  �1#        �           �1#        b           �1#                   �1#        �           �1#                    2#        !           2#        X           2#                  2#        �            2#                   (2#        �           02#                   82#                  @2#        �           H2#        �           P2#                  X2#        �           `2#                   h2#        I           p2#        �           x2#        %           �2#        ^           �2#        �           �2#        �           �2#        �           �2#        �           �2#                   �2#        �           �2#        �           �2#        S           �2#        �           �2#        �           �2#        )           �2#        O           �2#        p           �2#                  �2#        &           3#        �           3#        ,           3#        �           3#        s            3#        �           (3#        '           03#        @           83#                  @3#        �           H3#        L           P3#                  X3#                  `3#                   h3#        	           p3#        �           x3#        �           �3#        �           �3#        #           �3#        |           �3#                  �3#        _           �3#        
           �3#        (           �3#        $           �3#        d           �3#        A           �3#        c           �3#        &           �3#                   �3#        a           �3#        �           �3#        N            4#        E           4#        �           4#        �           4#                    4#        B           (4#        .           04#        +           84#        �           @4#                  H4#        4           P4#        m           X4#        �           `4#        �           h4#        e           p4#        �           x4#        �           �4#        v           �4#        2           �4#        �           �4#        Q           �4#        >           �4#        �           �4#                   �4#                  �4#        �           �4#        H           �4#        �           �4#        �           �4#        i           �4#        9           �4#        �           �4#        P            5#        
   �@����%z�" h   �0����%r�" h   � ����%j�" h
�" h   �P����%�" h   �@����%��" h   �0����%�" h   � ����%�" h   �����%�" h   � ����%ڣ" h   ������%ң" h    ������%ʣ" h!   ������%£" h"   ������%��" h#   �����%��" h$   �����%��" h%   �����%��" h&   �����%��" h'   �p����%��" h(   �`����%��" h)   �P����%��" h*   �@����%z�" h+   �0����%r�" h,   � ����%j�" h-   �����%b�" h.   � ����%Z�" h/   ������%R�" h0   ������%J�" h1   ������%B�" h2   ������%:�" h3   �����%2�" h4   �����%*�" h5   �����%"�" h6   �����%�" h7   �p����%�" h8   �`����%
�" h9   �P����%�" h:   �@����%��" h;   �0����%�" h<   � ����%�" h=   �����%�" h>   � ����%ڢ" h?   ������%Ң" h@   ������%ʢ" hA   ������%¢" hB   ������%��" hC   �����%��" hD   �����%��" hE   �����%��" hF   �����%��" hG   �p����%��" hH   �`����%��" hI   �P����%��" hJ   �@����%z�" hK   �0����%r�" hL   � ����%j�" hM   �����%b�" hN   � ����%Z�" hO   ������%R�" hP   ������%J�" hQ   ������%B�" hR   ������%:�" hS   �����%2�" hT   �����%*�" hU   �����%"�" hV   �����%�" hW   �p����%�" hX   �`����%
�" hY   �P����%�" hZ   �@����%��" h[   �0����%�" h\   � ����%�" h]   �����%�" h^   � ����%ڡ" h_   ������%ҡ" h`   ������%ʡ" ha   ������%¡" hb   ������%��" hc   �����%��" hd   �����%��" he   �����%��" hf   �����%��" hg   �p����%��" hh   �`����%��" hi   �P����%��" hj   �@����%z�" hk   �0����%r�" hl   � ����%j�" hm   �����%b�" hn   � ����%Z�" ho   ������%R�" hp   ������%J�" hq   ������%B�" hr   ������%:�" hs   �����%2�" ht   �����%*�" hu   �����%"�" hv   �����%�" hw   �p����%�" hx   �`����%
�" hy   �P����%�" hz   �@����%��" h{   �0����%�" h|   � ����%�" h}   �����%�" h~   � ����%ڠ" h   ������%Ҡ" h�   ������%ʠ" h�   ������% " h�   ������%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �p����%��" h�   �`����%��" h�   �P����%��" h�   �@����%z�" h�   �0����%r�" h�   � ����%j�" h�   �����%b�" h�   � ����%Z�" h�   ������%R�" h�   ������%J�" h�   ������%B�" h�   ������%:�" h�   �����%2�" h�   �����%*�" h�   �����%"�" h�   �����%�" h�   �p����%�" h�   �`����%
�" h�   �P����%�" h�   �@����%��" h�   �0����%�" h�   � ����%�" h�   �����%�" h�   � ����%ڟ" h�   ������%ҟ" h�   ������%ʟ" h�   ������%" h�   ������%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �p����%��" h�   �`����%��" h�   �P����%��" h�   �@����%z�" h�   �0����%r�" h�   � ����%j�" h�   �����%b�" h�   � ����%Z�" h�   ������%R�" h�   ������%J�" h�   ������%B�" h�   ������%:�" h�   �����%2�" h�   �����%*�" h�   �����%"�" h�   �����%�" h�   �p����%�" h�   �`����%
�" h�   �P����%�" h�   �@����%��" h�   �0����%�" h�   � ����%�" h�   �����%�" h�   � ����%ڞ" h�   ������%Ҟ" h�   ������%ʞ" h�   ������%" h�   ������%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �p����%��" h�   �`����%��" h�   �P����%��" h�   �@����%z�" h�   �0����%r�" h�   � ����%j�" h�   �����%b�" h�   � ����%Z�" h�   ������%R�" h�   ������%J�" h�   ������%B�" h�   ������%:�" h�   �����%2�" h�   �����%*�" h�   �����%"�" h�   �����%�" h�   �p����%�" h�   �`����%
�" h�   �P����%�" h�   �@����%��" h�   �0����%�" h�   � ����%�" h�   �����%�" h�   � ����%ڝ" h�   ������%ҝ" h�   ������%ʝ" h�   ������%" h�   ������%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �����%��" h�   �p����%��" h�   �`����%��" h�   �P����%��" h�   �@����%z�" h�   �0����%r�" h�   � ����%j�" h�   �����%b�" h�   � ����%Z�" h�   ������%R�" h�   ������%J�" h�   ������%B�" h�   ������%:�" h�   �����%2�" h�   �����%*�" h�   �����%"�" h�   �����%�" h�   �p����%*�" f�        H�=џ" H�џ" UH)�H��H��vH���" H��t	]��fD  ]�@ f.�     H�=��" H�5��" UH)�H��H��H��H��?H�H��tH���" H��t]��f�     ]�@ f.�     �=Y�"  u'H�=�"  UH��tH�=b�" �=����H���]�0�" ��@ f.�     H�=�" H�? u�^���fD  H��" H��t�UH����]�@���AWAVH�5%� AUAT1�USH��H����� H����   H�5� 1�1�H���� H����   H�-��" 1�1�H�5o� H��H�E ���  I��H�E ���  I��H�E E1�1�H�M�" H�5.� H����  H�E 1�H�� L��H�����  ��t0A�$�P���A�$�  A�   H��D��[]A\A]A^A_�@ H�E �����H�=�� L���  ���  I��H�E �����H�=�� ���  L��H��L��H��A�օ�u�H�E E1�1�H�ߒ" H�5`� H����  H�E 1�H�2� L��H�����  ���F���H�E �����H�=&� L���  ���  I��H�E �����H�=�� ���  L��H��L��H��A�օ������H�=�� H���:���L�M E1�H��H�1�" H�5�� H��A��  H�E 1�H�{� L��H�����  �������H�E �����H�=_� L���  ���  I��H�E �����H�=7� ���  L��H��L��H��A�օ��Z���H�E 1�L��H�5�� H�����  H�M L��H��H����  ��A���#���H�E H��H�
  H�=�� H��1���������f�H��H�]�" H��1�H� ��  1�H���H��dH�%(   H�D$1�H�-�" H�t$H� ��  H�D$��t�H�1�H�t
f�H���J��H9�u�H�|$dH3<%(   uH����7����    H��dH�%(   H�D$1�H���" H�T$H� ��@  H�L$dH3%(   uH�������� f.�     USH��H��1�H��H�s�" dH�%(   H�D$1�H��H���@  ��t1�H�L$dH3%(   u/H��[]�f�H�1�H�T$H����@  ��u΋D$9$�������e���D  USH��H��1�H��H���" dH�%(   H�D$1�H��H���@  ��u;H�1�H�T$H����@  ��u$�$+D$H�L$dH3%(   uH��[]�fD  ������������@ USH��H��1�H��H�-s�" dH�%(   H�D$1��D$    H�T$H�E ��@  ��u8�D$�T$��D$��D$�H�L$dH3%(   u/H��[]��    H�E H��H�h ���
  H�=�� H��1�����B���f�H��dH�%(   H�D$1�H���" H��H� ��H  H�L$dH3%(   uH��������D  f.�     USH��H��1�H��(H���" dH�%(   H�D$1�H�T$H���H  ��t1�H�L$dH3%(   u0H��([]�H�1�H�T$H����H  ��u�H�D$H9D$�������r���f�USH��H��1�H��(H��" dH�%(   H�D$1�H�T$H���H  ��u9H�1�H�T$H����H  ��u"H�D$+D$H�L$dH3%(   uH��([]�f�������������@ ATUH��SH��1�H��L�%��" dH�%(   H�D$1�H�$    H��H��I�$��H  ��u=H�s1�H��@ H���J��H9�u�H�|$dH3<%(   u3H��[]A\��     I�$H��L�` ���
  H�=`� H��1�A����I���f�     H��dH�%(   H�D$1�H���" H��H� ��(  H�L$dH3%(   uH��������D  f.�     USH��H��1�H��(H���" dH�%(   H�D$1�H�T$H���(  ��t1�H�L$dH3%(   u4H��([]�H�1�H�T$H����(  ��u��D$1�f.D$��D���n���@ f.�     USH��H��1�H��(H���" dH�%(   H�D$1�H�T$H���(  ��uYH�1�H�T$H����(  ��uB�D$�   �L$f.�w1�f.�����H�L$dH3%(   uH��([]�f�     ������������@ ATUH��SH��1�H��L�%Q�" dH�%(   H�D$1�H�$    H��H��I�$��(  ��u=H�s1�H��@ H���J��H9�u�H�|$dH3<%(   u3H��[]A\��     I�$H��L�` ���
  H�=P� H��1�A�������f�     H��dH�%(   H�D$1�H���" H��H� ��H  H�L$dH3%(   uH��������D  f.�     USH��H��1�H��(H�S�" dH�%(   H�D$1�H�T$H���H  ��t1�H�L$dH3%(   u0H��([]�H�1�H�T$H����H  ��u�H�D$H9D$�������B���f�USH��H��1�H��(H���" dH�%(   H�D$1�H�T$H���H  ��uYH�1�H�T$H����H  ��uBH�D$H9D$�   H�L$������H9L$O�H�L$dH3%(   uH��([]��    �����������@ ATUH��SH��1�H��L�%1�" dH�%(   H�D$1�H�$    H��H��I�$��H  ��u=H�s1�H��@ H���J��H9�u�H�|$dH3<%(   u3H��[]A\��     I�$H��L�` ���
  H�=X� H��1�A��������f�     SH��0dH�%(   H�D$(1�H�|�" H�\$H��H� ���  ��uH�h�" �D$H��H��R@�D$H�L$(dH3%(   uH��0[�����f�f.�     AVAUI��ATUH��S1�1�H��@L�%�" dH�%(   H�D$81�H��I�$���  ��t"H�L$8dH3%(   ��uhH��@[]A\A]A^� ��I�$L�t$ 1�L��H��L�����  ��L�%��" tI�$H���P@�D  I�$H��L��1��PP��I�$L�����P@������� AVAUI��ATUH��S1�A�����H��@H�-G�" dH�%(   H�D$81�H��H�E ���  ��u/H�E L�d$ 1�L��H��L�����  ��H�-�" t1H�E H���P@H�L$8dH3%(   D��u0H��@[]A\A]A^��    H�E H��L���PPA��H�E L���P@������    USH��1�1�H��(dH�%(   H�D$1�H���" H��H� ���  ��u;�$H��H�T$�ۍC�tH�L1� H���B��H9�u�H�J�" H��H� �P@H�|$dH3<%(   ��uH��([]��s��� H��(dH�%(   H�D$1�H���" H�T$H�L$H� ��x  H�T$dH3%(   uH��(��(����     H��dH�%(   H�D$1�H���" H�T$H� ���  H�L$dH3%(   uH�������� f.�     1�� f.�     H��dH�%(   H�D$1�H�M�" H�t$H� ��X  H�D$��t�H�1�H�t
f�H���J��H9�u�H�|$dH3<%(   uH����W����    �@ f.�     �{����f.�     AWAVI��AUATL�-nq" USA�
   1�H��I�,L��H��H��M�|� I�7������x!t/H�kL9�r�H��1�[]A\A]A^A_�D  H9�I��w���fD  H��L��[]A\A]A^A_�@ f.�     ATUI��SH�:�" H��H����
  H��H�L�����
  [H��H��]A\�x����     �@ f.�     ATUI��SH���" H��H����
  H��H�L�����
  H��H���,�������[��]A\�D  f.�     �@ f.�     ATUI��SH��H���" dH�%(   H�D$1�H��H���  H��H�H�t$L����  �$9T$H��NT$H��Hc��$���H�L$dH3%(   u	H��[]A\��v���fD  ATUI��SH��H��" dH�%(   H�D$1�H��H���  H��H�H�t$L����  Hc$1�;T$t"��H�L$dH3%(   u!H��[]A\�fD  H��H�����������������f�     AVAUI��ATUH��SH��L�%o�" dH�%(   H�D$1�L�t$I�$L����@  ��uq�|$��v?H���   t5I�$1�L��L��   ��X  H�=� H��1�A��I�$H��H����h  H�L$dH3%(   ��u.H��[]A\A]A^�f.�     I�$L��L��H����  ��������@ �k����f.�     �����f.�     ATUI��SH������H��tLL��H�������x!H�o�" H� �P(H�x H��L�������H�H�E �C    H�C    H�CH��[]A\�1���f�f.�     ATUI��SH���!����x!H��" H� �P(H�x H��L������H�H��~" �C   H�kH�C�E H��[]A\� f.�     ATUI��SH��������x!H���" H� �P(H�x H��L���"���H�H�H~" �C   H�kH�C�E H��[]A\� f.�     USH��H���w��t)��vH�-L�" H�=� 1�H�U �R �f�H�����H�-(�" H�E H��H�@0H��[]�� f.�     �w��tPr>��uH�wH�?�e���D  H��H���" H�=�� H�1��R 1�H����    H�wH�?����@ H�wH�?�����f�f.�     H���w��tMr;��uH�wH�������f�H��H�m�" H�=�� H�1��R 1�H����    H�wH������@ H�wH������f�f.�     H���G��t=��v H��" ��H�=h� 1�H��R 1�H��ÐH�vH�H���?����    H�vH��;�������H�����@ f.�     H9�tSUSH��H��H��H�6H�?������u�U9StH��[]ÐH��1�[]��    H��H��H��[]������    �   �f�AUATI��USH��H���wdH�%(   H�D$1�����   rr��uML�-�" H�|" 1�H��I�E H�����   ���  1�H�L$dH3%(   �9  H��[]A\A]�@ H���" H�=b� H�1��R ���     L�-��" H��H��H��I�E ��X  I�E H�t$L����X  �$����   �D$���x���H�{����H��L��H����   �P�Z���D  L�-I�" H�{" 1�H��I�E H�����   ���+���I�E 1�H��L�����   ������I�t$ H�} ����� ��� �T$1����������I�E 1�H��L�����   �������I�t$ H�} ���������� �������������SH��H���w��tA��v$H���" 1�H�=T� H��R H��1�[�fD  H��H��H��[�����     H�H�T$�r���H��H�T$t�H�@H��H��[H�����    ATUS�vH�Ӄ�tA��   ����   H�-�" H��y" H��H�E ���   ���}   H�{ ������,@ H�-��" H��y" H��H�E ���   ��uOH�{ �����I��H�E �����L�����  H��H�E L���P0H��[]A\�fD  H�y�" H�=z� H�1��R 1�H��[]A\��     AVAUI��ATUI��SH��H��H��0H�9�" dH�%(   H�D$(1�H����
  I��H�H�L$ H�T$L��H����x  ����   �D$����   ��urH�T$ H�H�:���
  H�=jx" H��H���/�������   H�=px" H���������   H�D$ L��H��H�p�L���H��t>H��L��������3f.�     H�L�����
  �   H��L���t���L��H������1�H�L$(dH3%(   ��   H��0[]A\A]A^�fD  H�T$ H�H�:���
  L��H��H���q���H��u�H�ھ   L��H�D$����L��H���,���H�D$�D  H�D$ L��H��H�p�|���H���j���H��L�������\��� H�ھ   �7��������@ f.�     AWAVI��AUATI��USI��H��H���vdH�%(   H�D$1����B  ��   ��t?H�@�" H�=y� H�1��R H��H�L$dH3%(   H���?  H��[]A\A]A^A_�H�w" H9B�  �:�F  L�=��" H��I����   I�|$M��H��H��L��H�����������   I�H����`  H����   H�E I�|$H�0��������^���H�ھ   L��1������L��L�������<���f�H�a�" H�t$H��H� ��X  �D$������I�|$�b���H��� ���H��L���P��H�ھ   ������fD  H��u" H9B�  �:��   L�=��" H��I����   I�|$M��H��H��L��H���r�������   �E �P����U ��   1��{����I�|$I��L��H��������u�H���" H��H��H� ��`  �����fD  I�|$I��L��H��������u�H�R�" H��H��H� ��`   H�E I�|$H�p���������������f�     I�H��1���   �����D  I�H����`  H��u�1�����H��� H���3����+����f.�     ATUH��SH���wdH�%(   H�D$1���tN��w)1�H�L$dH3%(   ��   H��[]A\�f.�     H�q�" H�=� H�1��R 1��fD  L�%Q�" H��H��H��I�$��X  �$1���t�H�{�T���H��tH���P ��    I�$H�t$H����X  H�D$���H��U���H�t
1�D  H���J��H9�u��5����'����    S�GH��H�?H�v�F�H���" H� ���  �C[�f.�     H���" �RH�?H� H���  ���    H���G���tbr@��t#H�\�" ��H�=� 1�H��R 1�H���@ H��/���H����
�D  f.�     ATUH��SI��H���w����   ��   ��u[H�5tr" H�}�E {H�]H�T$�.����E	 �E
{I�|$H�T$H������H�H�� }�@}H��H��)�[]A\�f�     H�a�" H�=� H�1��R H��1�[]A\��    H�9�" �RH��H�H� H���  H��[]A\�� H�5�q" H�}�E {H�T$H�]�����E �E{�P���fD  USI��H���G���t}H��rH��uH�~ H��H��[]L������fD  H���" ��H�=�� 1�H��R H��1�[]��    H���" H��H� H���  ���
  H��H�sH��[H��]��@ H�~ H��H��[]L�������f.�     USH��H��H��H���O��tyr?��uH�x H��L��[]����H��" ��H�= � H�1��R H��1�[]��    H�ٻ" A�hH��H�t$H�
H���  ���
  H�t$H����H��H��[]��H�x H��L��[]�>���@ f.�     �7��tZ��w]ATUI��S�GH���~)�h�H��H��H�D  H�_ ����H9�H��u�I�|$H�;�" H� �P0I�D$    []A\��fD  H��" H�=r� H� H�P 1���D  f.�     �7��tZ��w]ATUI��S�GH���~)��H�D@H�,��    H�_�����H9�H��u�I�|$H���" H� �P0I�D$    []A\��fD  H���" H�=� H� H�P 1���D  f.�     ATUH��SH��0H��dH�%(   H�D$(1��=���H��A��H������A�|H�+�" H� �P(H��H��H��I���l���Hc�H��H��L�� H������H���<���H�L$(dH3%(   uH��0L��[]A\��+���f.�     ��+Ðf.�     �G+F�f�     AVAULc�L�-��" AT�0   US��Lc�I�E �P(H��I�E B�<�    �P(H�CH�J���<�    H�CI�E �P(H�C H�CJ��H�C(H��[]A\A]A^�f�USH��H��H�?H�-%�" H��tH�E �P0H�{H��tH�E �P0H�E H��H�@0H��[]��@ f.�     ATULc�SH�H�GH)�H��L9�}7H�oH�ø" H����H��H)�H� H��Hc��P8H��H�J��H�SH�C[]A\�f.�     ATULc�SH�WH�G(H)�H��L9�}8H�o H�b�" H����H��H)�H� H��Hc��P8H��H�CJ��H�S H�C([]A\��     USH����H��H�OH�GH9�shH�E H9�t!;t;;Pu�D@ 9t,9Pt7H��H9�u��QH��H�MH��1�[]��     H���   []�@ H���   []�@ H+�T$H����������t�����H�M�T$�n���fD  H�G H�O(ATA��U��SH��H9�rH+OH����������t�
I9�t1H��Hc�f.�     Hc�H����I9�u�H��I)�I��A�@�1��f�f.�     AWAVA��AUATUSHc�H���   H�������I�ǍEL�c H�[����D�,A��A)�I9�t0Mc���    H��I9�tD94�u�B�4�L��H�������I9�u�H��L��[]A\A]A^A_�f.�     H���" SH��(   H� �P(H�X �H�@    [�f�     H�q�" S�8   H��H� �P(H�P(H�@    H�P H�H�P(�H�SH�P0[�fD  SH�G H��H�8�����H�!�" H��[H� H�@0��f�f.�     SH��H� ����H���" H��[H� H�@0��D  f.�     H�F H�wH�?H�HH��9���f�     H��H�vH�8�1����H�v �7����    H���(����     SH�G H��H�8�0���H�{�'���H�h�" H��[H� H�@0�� USH���8   H��H�C�" H� �P(H��H�@(�   H�C � ���H�CH�E H�C(� H�EH�C0H��H��[]�D  f.�     AWAVI��AUATUS����H��H�H�zH�H�XH�L$����I�H���L�,��r���H�L�$�I�GH�hL�0L9�u�[f.�     L9�t0IcH�|$I��������I�T$I�uH��H��H�4�������t�A�O���څ�E�H��[]A\A]A^A_�1����f.�     AVAUATU�l?S9�}`A��A��I���2D  ��L���D��������y=I�}�lD���A�������A9�~#�]A9�~ˉ�L����������N���     []A\A]A^��    AVAUI��ATUI��SH�����������usL���|���L��I���q���H��L��   I��H������L�%k�" �����H�5�� H��I�$���  I�$�����L��H�����  I�$L���P0I�$L���P0��[]A\A]A^�fD  AWAVI��AUATI��USH��H��H�cn" �����H��HdH�%(   H�D$81�H�۰" H� ��H
���L�t$8L�d$0H��I��H�D$ I�vL��L������I�VI�t$L��L������H�SH�sL��H�����������  H�|$ �M���H�MH�$H�EH�L$(H9�H�D$@�  fD  H�D$(H�\$01�H�-[" H� H�hH�@I��L`L+`H�D$8H+XH��" L�d$H� H�L�+L�����   ����   M�} H�<$I�wI+wH������I�GM�wH�KH�L$L9�H�D$tw@ H�|$ M�>I������L�hH��H��I��L���B���H�H�t$H�|$M�l� L���)���M�WI�H�I�T� H��IrI+r�
���H�<$1�L���L���L9t$u�H�D$(H�D$(H9D$@�����H�$H��X[]A\A]A^A_�H�|$HL��   �j���H�<$衽��H�$    ��H�|$HH�ھ   �����H�$    �H�|$HH�� �   �չ��H�|$ ����H�$    �H�|$HH�ھ   诹��H�$    �c���f�AWAVI��AUATM��USH��H�5&� H�߉�H��(H�T$�ü��I�>I�ž   �C���L��I��H��H�D$�л��I;D$��   H�|$����H�D$H�D$L�(L;h�   @ IcU I�FI��H�|$L�<��f���H�XI��I�I�GD�}H��H��HpH+p耾��H���D��H��H���" H� ���  H�� 1�H�|$L��蠶��H�D$L9hu�H�D$H��([]A\A]A^A_�H�ھ   L��聸��H�|$跽��H�D$    ��f�f.�     AWAVI��AUATM��USH��H�5� H��D��H���   H�T$H�L$(dH�%(   H��$�   1��g���I�?I�ľ   ����L��I��H��H�D$�t���I;F��  H�|$����H�D$H�D$pH�gW" �����H��H�D$ H���" H� ��H
���H;E�#  H��H� �x��  L�pH�D$H� H�PH+PI�FI+FH��H���|�����H�uH��H��H��I��H�D$�	���H�UH�sL��H�������I�VI�vL��L���������s  H�|$�A���L�t$H��H�D$I�vI+vH������H+]I�nH��I;n��   Hc�H��    H�D$ �   �     H�|$M�w H���n���L�hI�|$H��I��L��藺��H�H�{M�l� I�D$L��H�pH+pIt$�r���M�NI�~H�I�T� H��IqI+q�S���H�|$1�L��蔲��H�D$H9htZL�e H�\$ 1�H�o�" H�8T" I\$H� L�;L�����   ���D���H�|$(L���   袽��H�|$�ط��H�D$    H�D$H��8[]A\A]A^A_� H�|$(L��   ����H�D$    �� H�|$(H�=� �   ����H�|$�(���H�D$    �H�|$(L��   �˳��H�D$    �AUATI��USH��H)�H��H��H��H��I���[���H9�u�-@ H��H9�tH�3L��L���i�����u�H��H��[]A\A]�H����f�AUATI��USH��H)�H��H��H��H��I�������H9�tfD  H�3L��L��H������H9�u�H��[]A\A]�@ f.�     AVAUA��ATI��USH��H�6H�?H��I��1���������   H�;蜼��H�sH�{1�H��H���7���H�;I�4$�[���E��H��I�t$I�|$H��H��t[�����I;D$tTH�8�߷���   H��L��I���l���H�M�" L��H� �P0H��1�����H������[H��]A\A]A^�fD  賯��H���˯��[H��]A\A]A^�D  f.�     AWAVAUATI��USH��H��   I��H��   dH�%(   H��$�   1�覷��H�SH�sH�|$0H��H�D$H�|$����1�L���}���I�\$I;\$I��H�D$ H�D$t`@ H�H�t$L�|$(H�|$H�D$ �T$pH��t1H�hL�u L;ut#f�H��I+t$A�L��I��H��襵��L9uu�H��I9\$u�H��" H�|$H� ��x  L��跮��H�|$譮��H��$�   dH3%(   uH�Ę   []A\A]A^A_��#��� AWAVI��AUATI��USH��XH�H�.dH�%(   H�D$H1�H�GH�T$0H�L$8H�D$H�FH�D$ ����H�L$@M��L��H��H���?���H��H�D$(�q  H���9���I�w I+wH��H�D$H������I�_I;_ ��   f�HcH�L$H�|$(L�4�HcCH�L$ L�<�H�D$@L�(�u���I�~L�`H�D$I�FH��L��HpH+p葵��I�WH�I�wM�$�H�BH9BL�vtDD  I��I�~�L��A�}��tL��L���S���I�wH�I�WM�$�H��HBI��H+BH9�u�H�t$H�|$1��p�����tAH�D$0H��H9X �)���H�|$@�����H�L$HdH3%(   H�D$uUH��X[]A\A]A^A_�H�|$�����H�|$@�Ǭ��H�|$�}���H�|$8H�¾.   �[���H�D$    �H�D$    ��0���AWAVI��AUATI��USH��hH�H�.dH�%(   H�D$X1�H�GH�T$8H�D$ H�FH�D$(�����H�L$HL�D$PM��L��H��H��蟴��H��H�D$0��  H���I���I�w I+wH��H�D$H��� ���I�GI;G H�D$�'  D  H�L$H�\$ H�|$0HcL�4�HcAH�L$(H��耪��I�VH�D$L�xH�D$HI�vL� H�BH9BtML�nf�     I��I�}�L��A�|$��tL��L���r���I�vH�I�VM�<�H��HBI��H+BH9�u�H�D$PH�SH�sL�0H�BH9BL�ntAf�I��I�}�L��A�~��tL��L������H�sH�H�SM�<�H��HBI��H+BH9�u�H�t$H�|$1��0���H�L$8H�D$H�D$H9A �����H�|$H輪��H�|$P貪��H�L$XdH3%(   H�D$uH��h[]A\A]A^A_�H�D$    ������@ f.�     AUATUSH��H��H��L�n����H�;����H�u H+uH��I��H���X���H�]H;] t#fD  HcC1�L��H��I�t� �a���H9] u�H��L��[]A\A]� f.�     AVAUATUI��SH��荳��I�<$����I�T$I+T$H�߾   I��H��覬��I�\$I;\$I��H�(u�0f�H��I9\$t#H���E���t�H�31�L��H���ɩ��I9\$u�L���j���[L��]A\A]A^�@ f.�     AWAVAUATUSH��   H�GL�gdH�%(   H��$�   1�H�H�l$@H�|$ I��H�$H�Q�" H�\$H��H��H� ��P
�I��E9�~TB�|%  t�H�|$腥���4$H�XL��I������H�� D��L��I������H�C� 1�H�|$L���ئ��E9���$Hl$ �$D9�u�L�|$(H���" L��H� �P0H�|$�#���H��$�   dH3%(   H�D$u{H�Ĩ   []A\A]A^A_�H�D$L�pL+0H�^�" I��H� E��E��D���P(Ic�1�H��I��袨���I���H�|$����H�\$ H��H�D$H�sH+sH�������S����[����f.�     AWAVI��AUATI��USA��H��8L�gL+gH�nH+.1�dH�%(   H�D$(1�I��D��H���Ȱ��1�H��H�D$�	�����H�|$��   H�|$D�l$D��L�<$L�t$H�oH+/H��H����A���������x!I��@ ��L��D����������u�H�|$A��~*I��D  ��1�������1�L����������H�|$u�H�L$(dH3%(   H��uH��8[]A\A]A^A_��?���D  f.�     H��ATUSH��H��tIH�3�ɨ��I��H�{ H���S`L��H���q���H��tHcUH�C[]A\H��� H�C[]A\��    H�C H���S`H����f�     AWAVE1�AUATUSH��H��H��L�gL�vH�?H�6������t[H�3H�} �����H�;I������H�[I��I9�t0�    H�3L��H������I9�t
���H�D$8    H�D$8H��X[]A\A]A^A_�D  H�t$0H�|$81��Ǡ���H�|$0H�!� �   迢��H�D$8    �@ AUATI��USH��H��H�?�h���H�uH+u H��I��H���A���H�] H;]t&�    HcI�EL��H��H�4�1��G���H9]u�H��L��[]A\A]� AWAVAUATI��USH��I��H��H�GH9��  H;w�
  L9���   L;m��   M9���   t;L�5Ռ" L�} L��fD  H�3L��H���U`I�H����p  H�{��^���I9�u�H�UL��L��L��L)��T���L��HEL)�L�m I9�H�Et,D  H�3L���U`H��tH��H+UH��H�PH��H9]u�H��L��[]A\A]A^A_��    L�5)�" 1�H�= � I��R M9��3���L�5
�" H�=3� 1�I��R �!���fD  L�5�" 1�H�=�� I��R H�E�����f�     AWAVAUATI��USH�������H��L�oL�~H�?H�6�������tJH�3I�<$1��R���H�[I��I9�t(fD  H�3L��L��肦��I9���H�����I9�u�L������H����[]A\A]A^A_�D  USH��H��H��菦����x'H�UH�KH+UH+KH��H��9���9�����!�H��[]� f.�     H�������������I�H����    USH��H��H��������x'H�UH�KH+UH+KH��H��9���9�����!�H��[]� f.�     USH��H��H���ϥ����x'H�UH�KH+UH+KH��H��9���9�����!�H��[]� f.�     USH��H��H��������x'H�UH�KH+UH+KH��H��9���9�����!�H��[]� f.�     USH��H��H���/�����x'H�UH�KH+UH+KH��H��9���9�����!�H��[]� f.�     AVAUI��ATUI��SH�I��H��脦��H;Ct6H�8H��L��萚��H��H��H�����H;C�   t/[��]A\A]A^�@ L��L���   �8���1�[��]A\A]A^� L��   L������1��@ AWAVI��AUATUSH��H��(H�GH+GL�'H�|$H�t$H���F�B�@H���" ��H� ���P(1�H��H�EHc�����I�~I�NL�}H9�H�|$��   �   �    H�|$��I�\$I;\$H�H�|$L�rH�WtUL��f�     �rI��H�;I��H�j H��D�l0I�v�謡��I9\$A�DH��L��u�I;\$t��H�|$H�OH�D$��H�|$H9��p���H�|$H�WH)�H�QH�����H��([��]A\A]A^A_�H�ʸ   �� f.�     AUATI��USH��I��H��H�?腦��L��XL��H���t���H���[]A\A]��    AWAVH��AUATUSH��HL�yH�T$0H�H�|$(�{H�H�t$8H�T$H��H��H;xH�|$ ��   �    H�D$ H�ZH� H�hH�D$0H�HH�D$�{L�pL;p��   �M�&H��H��H��L�m�H�L$I��L������H�L$H�M��H�L��H�X�  I�GH�q H��H�D$H�t$L��蚣��H�|$H�H�L$H�L�|$�  H�XL9wu�H�|$(H�D$ H�P� }�@ H�t$ H9w�4���H�P�}H�BH+D$8H��H[]A\A]A^A_�H���AVI��AUATUI��SH�^�{H�?H��I��H�������Hc�L��H��H�L��H�^�}�F H��舤��H�H��H�L)�����L��� �����[]A\A]A^�D  AUATUSH��H��XdH�%(   H�D$H1�H���" L�d$ H��H�D$     H�D$0    L��H�D$8    H�$    H� H�D$(    �D$    H�D$    H�D$    �$   H�X(�(����x��H��L��H��H��H���ϖ��H�L$HdH3%(   H�� uH��XH��[]A\A]��F���fD  AUATUSH��H��XH�?L�d$ H�D$     H�$    dH�%(   H�D$H1�L��H�D$(    H�D$0    H�D$8    �D$    H�D$    H�D$    �$   �f���H�g�" H��L��H��H� H�X(�J����x��H��L��H��H��H������H�L$HdH3%(   H�� uH��XH��[]A\A]��h����     AWAVAUATUSH��H��(  dH�%(   H��$  1Ƀ��t$,H�T$ ��  H�D$ L�5Ã" H��8" H�hI�H�����   ��t8A�   H��$  dH3%(   D���  H��(  []A\A]A^A_�fD  H�D$ H�m H��H�]8" L�`I�L�����   ��u�1�1�M�|$ �C���I��H�D$0�2   �   H��H�D$I���Q���H�D$,H�T$ �l$,H�D$  H��H��H�T$H�D$H��AUM��M��H��訙����A��^_�  L��H��L���=���L��H��蒘���D$,�P����T$,�.  D�l$(L�|$�v 1�1�M�l$ 蒗��H��H��I��AWH�T$M��H�t$ H��I���.�����ZY��   L��L��H���Ơ��H��I������L�������D$,�P����T$,��   L��H�D$ H��H�PL� I�H�T$ H�7" L�����   ���^���H��薚���A����H��" H�
  H��H��I��袐��H;CI��u�H�|$�ύ��L���'���M��   H�|$ �@   �   �[����D$   �����fD  H�9s" H�
  H�t$H���ٍ��L�t$@H��H��L������I;F�k  H�|$@�B���H�\$8H��H�D$HH�sH+sH������H�CH9CH�D$��   �H�D$H�|$@H�(莃��M�} M;}H�D$(H�Xt* IcH�EI��H�<�H��H�w蝌��M;}H�H��u�H�|$�G���M�<$M;|$H�D$0L�pt+ IcH�EI��H�<�L��H�w�U���M;|$H�M�4�u�H�|$0�����H�� 1�H�t$(H�|$H�w���H�L$8H�D$H�D$H;A�-���L������L�������H�>q" H�|$HH� H��h  ���H�|$ H�������� H�|$�f���L��较��M��   ����H�T$H�l$P�   H���J���H�|$ H������H������H�|$@����H�|$����L���i���L���a����D$   �/�������D  f.�     AUATUSH��H����t.H�op" �   H�
�   � L�{I�$L��M�v H��L�����   ����u�I�$I�>L���  L��h  I�G H�0�^���1���@��A��H��H��A���_���D  AVAUATUH��SH��H��dH�%(   H�D$1���tIH��n" �   H�
  1�1�H��L����H�L$8��P������   H�L$ ��P������   H�L$(��P�����   �D$��tOH�|$�U����}���I�E H����   �n���fD  ��P�����X���I�E H����   �F���fD  I�E H�|$H��h  ����L��H��������fD  I�E H����   �o���fD  I�E H����   �B���fD  I�E H����   ������tf����   I�E �����H�=Pg H��h  ���  L��H���� I�E L��H��   H��   ���  H�=�f ��1���L��H�����k���I�E �����H�=�f H��h  ���  L��H�����I�U �����H�==k 1�H��h  ��   L��H�����x����}�� AWAVAUATI��USH��H��XdH�%(   H�D$H1���tOH�i" H�
  1�1�H��H����H�$��P������   A��P���A���   H�|$A�   �Z}������D  D�t$I�$L��H��   ���
  1�1�H��H����H�$��P����~?A��P���A�~"I�$H�|$H��h  ����H��H��������I�$L����   ��I�$H����   �I�$L����   �Q���I�$H�<$��   �,����Sx�� AWAVAUATI��USH��H���   dH�%(   H��$�   1���tQH��c" �   H�
  �@   I�Ǿ   H���Yu��H��L��L���{w��H��t&I�U H��H��h  蓀��L��H�����F���fD  H��L��   � ���+����&w��fD  AWAVI��AUATUSH��H��  dH�%(   H��$�   1Ƀ��t$�Q  L�-�b" H�ZH�" I�E H�����   ��t6A�   H��$�   dH3%(   D���a  H��  []A\A]A^A_�@ M�gI�E H��H�[ H�"" L�����   ��A��u�H�D$M�d$ �>   �   H��H�$�*t��H�$1�L��H���ix��H��I����   �D$I�_ ��I�D� H�D$�RfD  H��L�{�I�E H��" H��L�����   ����   I�w H�$1�L���x��L��I���z��M��tWM��H;\$u�I�E L��H��h  ��~��H��H���������D  H�9a" H�
  �=   I�Ǿ   H���Yp��H��L��L���Kz��H��t&I�U H��H��h  �{��L��H�����F���fD  H��L��   � z���+����&r��fD  AWAVI��AUATUSH��H��  dH�%(   H��$�   1����  H��]" H�j�t$H�" H� H�����   ��A�ċL$t8A�   H��$�   dH3%(   D����   H��  []A\A]A^A_�fD  �A�H�} �L$M�~A���At��D��D�l$L�l$H��H����w���<   �   L���'o���D$�L$��t&�A�M�t�  I��I�W�L��H��H����x��M9�u�H��\" H��H� L��h  �<z��H��H��A���4���fD  H��\" H�
  1�1�H��L�����g��� I�I+1�H����q��1�H��H�D$�&m��H�C H�D$�����     ��L�t$t4��tbI�O������I�E H����   �1���I�E L����   �
���I�G1�H�������I�E H��   ��   I�GH������1�����I�E L��H��   H��   ���  H�=EQ ��1���L��H�����i����Ug��D  AWAVAUATI��USH��H��X  dH�%(   H��$H  1��F��D$X    ����  L�%�R" L�r��H��" I�$L�����   ���D$t9�D$   H��$H  dH3%(   �D$��  H��X  []A\A]A^A_�@ I�F �7   �   M�} H�D$H�D$`H��H�D$@�gd��I�$I�}���
  ��H�D$HH��"     H��"     ��  �E�H�D$0    H�D$     L�t$\H�-$�! ��H��H��L�L�-�" H�D$H�D$XH�D$8�V�    ���W  ���6  A�D�    I�$H��I�wH�T$8��@  ���W  �    I��L;|$��  I�$H��E1�AV�   I�7L��S H��H�����	  ��Y^�����HcD$\A�T� ���e  H��H���t��te�W�����"    A�D�    H��H�D$I�WH�8�l��H��H����   1�H�|$H��H�L$(�e��H�L$(H�D$ H���fc���9�����v"    A�D�    H��H�D$I�WH�8�)l��H��H��tY�   �f.�     I�$H�=%O 1��R ����� A�D�    H�D$H��I�WH�8��k��H��H�D$0�����D  �D$   H�D$ H��tH���b��H�D$0H���o���H���b���b��� H��O" H�
   L���n��L��H����k���s��� H�|$  ��   H�|$0 tHL�L$@D�D$XH�L$0H�T$ H�t$HH�|$�qb��H��t?I�$H��H��h  ��l��H��H��������L�D$@�L$XH�T$ H�t$HH�|$�e���H�t$@H���Pk�������H�D$0    H�D$1�H�xH+xH����l��1�H��H�D$ �;h���J����!c���AWAVI��AUATUSH��H��  dH�%(   H��$�   1�����   L�d$�	   A���   L���`��I�wH��L���$m��H�Ǹ   H��tb�Bm��I��A�F�I�_L��ƉD$�i���D$����   A�F�M�t� �f�H��L9�toH�L��H��L���6j����t�L���f���   H��$�   dH3%(   u_H��  []A\A]A^A_�H��M" H�
  L�d$�6   �   L���c_��I�wL��H����k��I�Ÿ   M����   L��I�_��k��I�Ƌ$L�����ƉD$��g���D$����   �$��I�D� H�$�& 1�L��L����_������   H��H;$��   L���^��H�L��H��H��I���!k����t�L���Ea��L���e���   �     H��$�   dH3%(   ��   H��  []A\A]A^A_��    H�1L" H�
  L��H����f��I;D$I��u�L�t$0H��   H�\$ H���h��H��L����e��H�|$(��c��H�|$�d_���D$   �8����    H�qI" H�
H�E ��   H�|$P�C[��L�|$8H�E L��H��   ���
  1�1�H��L����A��P���A���   H�|$`��^��H�|$X��^�������I�U ����H�t$PH�|$X1��Y��H�L$`H�D$HH�D$H�T$|H;A����L�|$8H�E L��H��   ���
  1�1�H��L����A��P���A�~uH�|$`�^^��H�E H�|$XH��h  �9c��L��H�����M���H�E H�|$8��   �<�����Y��H�|$P�9Z��A�E �P���A�U �����H�E L����   �����H�E H�|$8��   �w���D  f.�     AWAVI��AUATUSH��H��x  dH�%(   H��$h  1�����  H��D" L�j��L�%��! H� L��L�����   ��t6A�   H��$h  dH3%(   D���Q  H��x  []A\A]A^A_�@ I�E M�nL��H��H�D$L�8L��H�xD" H� ���   ��u�I�E �4   �   L� H�D$H��$�   H��H�$�yV��H�L$I�|$I+|$H�qH+qH��H���%Y��L��H��L��H�D$��_��I�T$I+T$H�H��H9��9  �E��VUUU���D$$���A����)R��A)���   I�F L��M�f(H�D$(�U��I�ŋD$$�����  L�t$0M��L�$$�)D  H��L���[��I;E��  ��I������   I�VI�6L��H���[��H��u�H�4$H��A�   �_��L���]��H�|$�	Y���d���@ H�!C" H�
  1�1�H��H����A��P���A���   H�|$H��X���t���H�t$@H�|$H1��9S��H�L$H�D$8H�D$8�T$hH;A�����D�|$lH�@" L�t$(H� L��H��   ���
  1�1�H��H����A��P���A�~}H��?" H�|$HH� H��h  �R]��H��H���������H��?" H�|$(H� ��   �B���H�|$@�QT��A�$�P���A�$�����H�n?" L��H� ��   ������S��H�Q?" H�|$(H� ��   �i���fD  AWAVAUATUSH��H��h  dH�%(   H��$X  1���H�$�\  H�$L�=�>" A��H���! H�hI�H�����   ���D$    t3H��$X  dH3%(   �D$ �B  H��h  []A\A]A^A_�fD  H�$L�m �   �   H�@I�m H�D$A�D$�A��H�D$pH��H�D$�P��D��VUUUD����D��D�t$$��)R��)��L$ ��   H�D$H�� �$P��H��H�$L�p �D$$��A����   L�l$L�l$�,fD  H��H���U��H;E�  A��I��E����   I�VI�6L��H����U��H��u�H���X���D$    �����fD  H��=" H�
  1�1�H��H����A��P���A���   H�|$@��S���D$    �f����H�t$8H�|$@1��/N��H�D$0H�D$0H9D$X�+���L�t$I�L��H��   ���
  1�1�H��H����A��P���A�~YI�H�|$@H��h  �cX��H��H���������I�H�|$��   �[���A�E �P���A�U ����I�L����   � ���I�H�|$��   ���N��f�     AWAVI��AUATI��USH��  dH�%(   H��$�   1����%  H�.:" L�bA��H� �! H�L�����   ����t6�   H��$�   dH3%(   ���Y  H��  []A\A]A^A_�fD  I�D$ H�|$E�g��-   �   H�<$H�D$��K��A����   H�|$�W��E��H�D$��   A��M�fA��I��O�|>(�f�I��M9���   H�I�|$���
  I��H�I�<$���
  H�$H�|$L��H����J����u�H�4$L��   �lU��H�|$��Q������D  H�	9" H�
  L��H��H���T��I;D$�)  H�I+D$K�|7�H�L$H��H�D$H�i7" H� ���
  H�5@ H���O���   I��L���9I��L��H��I��H�D$0��N��I;F��  H�|$0E1���U��A��H�D$8�M  L�}Hc\$H�EL�|$I9�L�$�    ��   D  H�L$I9�H�	H�QH�L$N�,"��   L��1�M��I���D  �I��L;mt7I�E H�|$L��H�@J�4 ��N������A��u�1҅���I���L;muɃ�H�|$0��G��L�xI��H�D$H�xH�@L��H��HpH+p��P��H���M�<�H�*6" H� ���  I�� 1�H�|$8L���I��H�D$H�EH�L$H9�t	L�}����H��5" H�|$8H� H��h  �lS��H�|$(H���������   �Q���H��5" H��I�wH�T$LE1ɹ   L��7 H� RH�U�! H�|$8���	  ��ZYuvHcD$DH�8�! H��D�t�Z���H�|$(I�ع   �+   �   �NN���D$$   ����H�|$(I�ع   �+   �   �%N��H�|$0�kO���D$$   �����H�|$8�M���D$$   �����BI��f�AWAVI��AUATI��USH������   L�-�4" L�z��H���! I�E L�����   ����t�   H����[]A\A]A^A_�fD  M� �U�I�L$L��I�?�G��H��H��t�H��L���Q��H��I���G��I�E L��H��h  ��Q��L��H�����fD  H�)4" H�
�   � L�{I�$L��M�v H��L�����   ����u�M� �&   �   H���ID��H��L��L���KF��H��t!I�$H��L��h  �O��H��H��A���5���H��H���N���{���� F��AWAVI��AUATUSH��H��(dH�%(   H�D$1��F����h  H�-�1" L�rA��H���! H�E L�����   ���D$�  A��M�n �T  H�E I����
  I�} H��I���'C�����D$�4  A���C  A����  I�W E1�I�} H���L��H��I����   D��L��H����E��L��I���C��H�E 1�1����  Lcd$M�>H�$I��M;~u�  fD  I��M;~��   IcI�EH��H�4$H��H�@J� H�E ��p  ��t�H�$��P�����]  L���C���D$   H�L$dH3%(   �D$�n  H��([]A\A]A^A_��    H�)0" H�
  H�|$H��H����?����xCI�WH�H�t$L��H��H���p  ��u>L;t$u�H�H�t$L����h  �Pf.�     I��   �   �   L���F��H�L$��P����H�H����   f�     A�   H��(D��[]A\A]A^A_��     H�A-" H�
  H�|$H��H����>����x<I�WH�H��H�D$�&��� M��   �   �   L��A�   �`E���a���I��   �   �   L��A�   �E���<���fD  AWAVAUATI��USH��H����t7H�x," �   H�
  I��I�$I�} ���
  L��L��H�$��=��Lc�M��E����   H�4$L���=��Lc�E����   I�$I��I�����  H�$H�D$L�xL;xu�   D  H�D$I��L;xtqI�H�4$H��H�@J�0J�(I�$���  ��t�H�$��P��������I�$H�߽   ��   �����f�L�$�   �   �   H���D�������I�$H�4$H����h  ����f.�     AWAVI��AUATUSH��H��(��t7H��*" �   H�
  I��I�$I�~(���
  H�|$L��I���<����M����   H�|$L���D$��;�����T$xqM�wM9wtUH�Lc�H��I��H�D$@ I�H�L$A�   H�t$H��H�@H�J�(I�$��0  H���8���I��M;wu�I�$H�����  �����M��   �   �   H���}B��������     AWAVI��AUATUSH��H��  dH�%(   H��$�   1Ƀ��t$�Q  L�-*)" H�ZH��! I�E H�����   ��t6A�   H��$�   dH3%(   D���a  H��  []A\A]A^A_�@ M�gI�E H��H�[ H���! L�����   ��A��u�H�D$M�d$ �   �   H��H�$��:��H�$L��H���G��H��I����   �D$I�_ ��I�D� H�D$�R�     H��L�{�I�E H�E�! H��L�����   ����   I�w H�$L���IG��L��I���@��M��tYM��H;\$u�I�E L��H��h  �E��H��H����������    H��'" H�
  H�|$H��H���7����y�I�ع   H�|$�   �   A�   �>��L���]8���;����     H��%" H�
  H�|$H��H���7����x��L���`D��I;D$I�ع   �>���H�|$(L��H���5��H��t9H�U H��H��h  �uB��H�|$H�����i���fD  L��% �   �����H�|$L�\' �   �   �   A�   �=���'���@ AWAVAUATI��USH��H��  dH�%(   H��$�   1���tQH�R$" �   H�
A�   �f�L�uI�$L��M�m H��L�����   ��u�H�m I�$L��M�v H��H�����   ��A��u�H�E H�l$�   �   H��H�D$�o1��H�T$H��L��L���1��H��t I�$H��H��h  �<��H��H��������H��H���<;���M����B3��f���AVAUATUH��St0H��" �   H�
  H�=P H�ƹ   L�$L�D$�L�L$�l���I�BL�$H�L$0H�T$,L��H��I�H�t$H�D$I� H�D$H�E ��x  ��A���   �D$,L�$���   ��L��L�$������/���D$,����   H�D$0L�$�W�    ����   L�ω�D��L�$�/1����L�$��   ����   �D$,�P�H�D$0�T$,H����H�D$0��   H�U L�L$H�8���
  H�|$H���9'���$H�D$0H�xH�E ���
  H�|$H���'��D�$L�L$E���\���H�D$0�   L��H��o3���D  H�T$�	   L���V3��L��L���+1��A�   ����A�/H�����H�D$0�   L��H�P�3����H�D$0�   L��H��3���H�D$0�   L��H�P��2�����(���    USH��H��H�H�-d" ��P����~H�E H��H�@0H��[]��D  H�E ��   ��f�f.�     AWAVAUATI��USH�Ӻ����H��H�-" L�nL�>L�v(H��H��H�E ���  H�E �����H�5� H�����  H�E I�u �����H��L�-ż! ���  H�E �����H�5� H�����  IcT$IcD$H��H�P�����I�t� H�E ���  H�E �����H�5f H�����  H�E �����I�7H�����  A�D$H��tH�E �����H�52 H�����  H�E H�ߺ����H�5 ���  IcT$0IcD$4H��H�P�����I�t� H�E ���  H�E H�ߺ����H�5� ���  H�E I�6H�ߺ�������  H�E H�ߺ����H�5 H���  H��[]A\A]A^A_���f.�     USH��H���&���xH�p" H� �P(H�xH��H��H�x��#��H��H��[]�D  AUATUSH��H��H�H����   �L�%%" �P����!��   fD  ��+��H���*��H�}��%��H�}H�GH+H��H��w��h%��H�}�_%��H�} H��tfD  H��H���H��H��u��E0H�U8��t;��H�]@H�@H��L�lh�H�{� tI�$H����x  H�{��3$��H��`I9�u�I�$H�} �P0I�$H��H�@0H��[]A\A]���     L�%A" �B���I�$��   �3���D  ATUS�7H������   ��   ��tC����   L�gI�<$�}$��I�|$H��tE�H�-�" �P����9H�E ��   �-@ L�gI�|$ H��t�&��I�|$@H��t�~&��H�-�" H�E L���P0H�E H��[]A\H�@0��@ H�-y" H�=: 1�H�U �R ���    L�gI�|$(H��u���     L�gI�|$��#���AUATI��USH��(H�?dH�%(   H�D$1�H��t�#��H�-" M�l$H��H��L��H�E ���  H��tf�H�x����H�E H����  H��u�H�E L��M�l$`��x  H�E H��L�����  H��t!fD  H�x�o���H�E H����  H��u�H�E L����x  I��$�   H��u�9�H�E H��L���P0M��t&H�{L�+��P�����H�E ��   ���    H�E L���P0H�D$dH3%(   uH��([]A\A]��^#��@ f.�     AWAVAUATUSH��L�H�I9���   L�D$I��A��A��H���   �;f.�     ��u$E��uH��H���L!��H�|$���*��1�fD  H��I9�t.���t�~�E��u�H��H��H���!��L�����X*��1�I9�u�H��[]A\A]A^A_ø   ��f.�     AWHc�AVAUATL�$RUSI��I��H��8H�o N�l&@H�t$H��1�dH�%(   H�D$(1���(��H�]H;]H�D$tNL�D$L�t$M�H�L��L��H�D$A��$�   H��tH��H+uH�PL��H���'��H��H;]u�H�D$H���G ��H�D$(dH3%(   uH��8[]A\A]A^A_���!�� AWAVI��AUATI��USH��I�������H�5Y H��H�;
" H�D$`H�L$H�t$H�5� H�D$I� H�@ H�$I����  M�t$I������I�,$H�t$H��M�f���  I������H�5 H�����  I�H�u �����H�߽�������  I������H�5A H�����  M�6M9�t>f�     I���H��I��H�0I�H�6���  I���H�5 H�����  M9�uˋCH�ߍp�I����  I�H�ߺ����H�5� ���  H�D$H�$L��M�EL�L$H��H�0H��(H��[]A\A]A^A_�����D  f.�     AWAVI��AUATI��USH��M��H�ͺ����H��I� H�5t L�l$PH�@ H�D$H��" H� ���  H��L��L������H�D$M�D$M��H�u H��L��H��[]A\A]A^A_H�P�J���f.�     AWAVH��AUATH��USI��I��Hc�1�H��dH�%(   H�L$1�H�T$�PP�T$����   L�=]" I�ƍ|m���I��ǘ   �P(L��H���D���xI��P(L��H��H����L���'��H���%��H�C� �   �<��1�H�C�1��H�Tm H�{8H�CH�C     �C,    1��C(    �k0H���0��I�^H�L$dH3%(   H��uH��[]A\A]A^A_�����D  H��SH�H��H�6�PHH��" H��H���p  H��[�#��� H��H��H���PHH��t
��E1��H���^��I��M;ut4A�>t�L��L�����
��H���E1��4��I��M;uu�f.�     H�<$ H�D$L�pt+H�CH+H��H���+  H�EH+E H��H����   H���m
��H���e
��H�|$�[
���g���fD  H�%�  H��M�EM��L��PH�T$ H�t$8H�|$�����AZA[�����H�L$0H�SH+SH�\$8H�AH+AH�KH+KH��H��H��H��H9��:���H�#�! L�<$�����H�5�  H�L�����  H�|$ L��H�������H������H�5��  L�����  ����H�{�  H��I��M��PH�L$(H�T$ H�t$8H�|$����^_�����H�!�  H��I��M��PH�L$(H�T$ H�t$8H�|$�����AXAY����H���  H��M��M��L��H��PH�t$0H�|$����AZA[�����H���  H��M��M��H��PH�L$H�t$0H�|$�����^_�w���H�m�  H��M��M��L��H��PH�t$0H�|$����A[A\�(���H�q�  H��M��M��H��PH�L$H�t$0H�|$����_AX����H��  H��I��M��L��L��PH�t$ H�|$�D���_AX����H���  H��L��I��M��L��PH�t$ H�|$����Y^����H���  H��I��L��I��L��PH�t$ H�|$L�\$(�����AYAZL�\$�+���H�M�  H��H��M��M��L��PH�t$0H�|$����XZ����H�`�  H��M��M��H��PH�L$H�t$0H�|$����AXAY�
���H���  H��H��M��M��L��PH�t$0H�|$�]���XZ����H� �  H��M��M��H��PH�L$H�t$0H�|$�-���AYAZ�0���@ AUATUSH��H��H���{��H��H���p���C0��tI��L�k8L�d@L��I��M��
�H��`L9�t'H�3H���|����t�L)�H��i۫����f�     �����H�����H����[]A\A]�fD  SHc�H�� dH�%(   H�D$1�H�GH�$H�X H�vH��H��H�H�|@H�J8H�L$���   H��t$HcPH�CH��H�\$dH3%(   uH�� [�f�H�C�����D  AWAVAUATUSH��HdH�%(   H�D$81��G0H�L$H�t$ ����   ��I��H�w8H�@L�@L�d$H�l$ Hc�H��L�lh� I��`H�XM9�tcI�G�L��H��L��H�D$(A�WHD�D$E��u�L�|$M��t?L��D�D$�
��L��H�¾   H������H���! H��H� �P0��[]A\A]A^��     AUATUSH��(dH�%(   H�D$1��G0H�4$��tS��H�W8H�-U�! H�@H�_@I��H��L�lhf�     H�C�H��L��H�D$�S@H��H�E H��`��p  L9�u�H�D$dH3%(   uH��([]A\A]��H���     AUATI��USH��H��H�6���I�D$H�SH��H�h H���P��H;EI��t4H��@ H�3L���U��H��H+UH�31�L��H��H���	��H9]u�H��L��[]A\A]�D  USH��H��H��.��H�{H���B��H��H��H��[]1������D  f.�     SH��H�����H�����H�{[����H��! ATA��USH��H�ӿ   H� �P(H�U D�`H�X�H�H�E []A\�fD  AWAVI��AUATA��USE1�H��H�-��! H�_ H��H�E ���
  H��H�$t`E1�� I��H�H��tMD9ku�H�E H�{���
  H�4$H��������u�M��H�t9I�H�H�D$H��A�������H�\$H��u�H��D��[]A\A]A^A_�fD  H�D$I�F ��D  AWAVAUATUSH��8dH�%(   H�D$(1��F�H�<$����   ��H���.  L�=��! 1�I�1�1�L�d$���  H�D$I�H�}L�����  H��th��MDL�p �Q���uL�p H��tI�H��L�����  ��t+I������L��L��p  ���  H�t$H��H�<$A�Յ�urI�L����  H��u�I�H�t$H�<$��h  1�H�\$(dH3%(   ��   H��8[]A\A]A^A_�f�H���! H�
  H������������    AWAVAUATI��USH��H��8  dH�%(   H��$(  1���H�D$8    ��   L�=5�! I��H�zA��I����
  L��H��H�������H��H��tH�pI�H��H���! ���   ��taA�   H��$(  dH3%(   D���   H��8  []A\A]A^A_��    H���! H�
  H�t$H��H������H��H��tH�pI�H��H�G�! ���   ��t]A�   H��$(  dH3%(   D����  H��8  []A\A]A^A_� H��! H�
  H��H��H�������H�¸   H��t�I�$H�:�����H��h  ���  H��H����[1�]A\� f.�     ��ATUSH��t,H���! H�
  H��H��H���"���I�$H��H��h  t"�   ���  H��H����[1�]A\��     1����  H���� AWAVAUATUSH��H��8��t:H��! H�
  L��H��H���j���H��I��t H�pH�E H��H���! ���   ���D$t�D$   ��     I�D$1�1�L�x H�E ���  H�D$ I�D$8H�D$A�D$0����   ��H�D@H��HD$H�D$(H�D$1�1�L�(H�E ���  M�e M;eH�D$u�   D  I��M;etvA�4$I�?�r���H�U �����H�8L��p  ���  H�t$H��H��A�օ�t�H�L$��P����~vH�L$ ��P��������H�E H����   ������    H�E H�T$H��H�t$ ��p  ��u�H�D$`H�D$H;D$(� ���H�E H�t$ H����h  �J���H�E H����   �x���@ f.�     AWAVAUATUSH��  dH�%(   H��$x  1���H�|$H�T$(H�L$tPH���! �   H�
  H�t$H�|$H������H��H�D$0t)H�D$0H�O�! H�|$H�pH�?�! H� ���   ��t
�   �i���H�T$0H�t$H�|$�,�����u�H�D$0H��! H�|$H�@H�@ H�D$ H�D$(H�XH���! H� H�����   ��u�L�{ L�d$ I�H�XH;Xu$�^f�H�0H���U������  I�H��H;Xt<H�+I�<$H�u ����I�$H;Bu�L�E �   H�|$�C   �   �i����*���H�\$0�{0�����{0H�D$8����H�D$P�C0H�K8�D$@    H�L$HI��H�@H����   �I�E1�H�xH+8H�������H�D$I�H�H;XuC�x�    I�?���f���L��H���{�����tEH�|$��A��H�������I�H;X��  H�D$ �3H�8�'���I�?H�0I���y��������u�H�|$�8���E����  H�D$0I��`�D$@�@0H�@H��HD$HI9��8���H�L$8H�AH+H��H����  I�H�xH+xH�������H��H�D$8L�t$8L� L;`t2@ M�,$I�] I;]tf��3H��H�������I;]u�I��M;fu�I�H��H�pH+pH���{���H��H�D$X�n���H��$�   �C   �   H��H�D$@�����H�t$0H�|$��������R  H�D$0�@(���  H�D$ H�8�K���H�D$xI�GI;GH�D$(��   H��$�   H�l$0L�|$HH�D$@ H�D$(H�L$PL�(H�D$8L�1L�8H�pI9���  L��E1�I��H������L��I��D  L��$�   H�E H�t$H��$�   IcH�@H��I�|@A���   H��t���H�@t9��  A��Hc�H�D$8H��I��H;hu�H�0I��Mc�L��L��H)�H��L9���   H�L$HH�D$(H�D$(H;A�&���H�|$X����H�D$0H�t$1�H�|$�@(    ���������  H�|$x�   ����H�|$P�����H�D$8L�d$8H�(H;ht�H�} H������I;l$u�H�|$8�b�������D  ���Q���H�t$H�|$8�����t$@H�|$P������/���H�L$8L��Mc�Hc�L�yH�1�	���L�E �   �	���H�D$ H�@L�<�I��;~H������H��H�D$XL� L;@��   H�l$hL�|$pL��I��L�t$XL���f.�     H�H��� I;^��   H�MHcH�UI�H�IL�,�H��H�0�	���I�U H�|$I��H�L$@H�0�����I�WL+bI��Mc�I��L��IWH�:H��t���q����7�H�D$`H���! H���   L��H�D$`IW�\���L��H�l$hL�|$pH�������L�L$@L�D$xH��H�|$L��H��������������H�|$X�������H�D$0H�t$1�H�|$�����@(    ������������������H��! H�|$xH� H��h  ����H�|$H���������H�D$(H�|$�   �C   �   �   L�@�Q�������H�|$8�   �-���H�|$P�S���H�D$(H�|$�   �C   �   L�@��������E1�H�����������H�|$X����H�D$(�-   H�PH�\$@H������H�|$H���{��������H�|$X�����H�D$(�,   H�P���g����    AWAVI��AUATUSH��H��8  dH�%(   H��$(  1���tQH���! A�   H�
  H��H��H������H��I��tH�pI�$H��H�H�! ���   ��tA�   �|��� L��H��H���:�����u�I�GL�l$@�B   �   L��H�@ H�D$�2���I�$H�L$8H�T$0I�v H����x  ��u�H�L$8�T$0L�D$4M��L��H������H���x����t$4H��L��H�D$�
���H�L$H�D$H���X���L��H�����������  A�G(���D$�T  H�D$A�G(   H�8�@���H�D$ H�D$H�T$H;P�F  H�:I�FH�D$����I� I�$L��L�T$(1�A�   H�t$H����0  H��L�T$(�B  H��I�N(M��AUL�L$0L��H�T$(H��L�T$8�����A��A��XZL�T$(��   I�$L�T$I���`  L�T$I�$L�T$H�|$L��   ���
  1�1�H��H��A��L�T$A��P���A���   �T$A�G(    H��H���������unA��thA�������I�$H�|$ H��h  ����H��H�����b���M�F�-   �B   �   H����������1�A�G(    H��H���\�����A��t�H�|$ �{�������M�F�,   ��D$   ����A��D$   A�   �P���A������I�$L��L�T$��   L�T$�����I�$L����   ���������f.�     AWAVI��AUATUSH��H��X  dH�%(   H��$H  1���tQH�"�! �   H�
  L��H��H���j���H��I��tH�pI�$H��H���! ���   ��t�   �|���@ L��L��H��������u�I�F�A   �   H�@ H�D$H�D$`H��H�D$ �}���L��L����������  A�n(���>  I�GA�F(   H�D$� I�G � H�D$I�G(L�|$� I�?H�D$0�����I�OI9OH�T$\H�D$(�e  �l$LL�l$@H��L�t$8L�|$I���"f�A��P���A���  H��I;o�  H�} ����� I��I�$1�A�   L��H�t$H����0  H����  I�$L��H�t$H����P  ����  �t$\��t��D$H��H���t$(L�L$8M��H�L$@H�t$HH���=���Z��YD�\$�N���M��L�l$@L�t$8A��J���A���  1��tE1ۃ���A��@ A�F(    D��L��H���2���H�L$(���   E�H�AH+AH��H���-  I�$H�|$L��   ���
  1�1�H��H��A��H�L$0��P�����  H�L$��P�����&  H�L$��P������   ����   H�|$(I�$L��h  ����H��H��A��������    I�$L����   �H���fD  I�W�-   L�|$ L���:���L��H�������4���f.�     M��L�l$@L�t$8A��P���A���   A�   �   ����I�W�,   �L�t$8�l$LL�l$@E1�����I�$I�~��`  �����H�|$(������>���I�$H����   �����I�$H����   �����I�$H����   �����I�$L����   �e����Q���I�$D�\$ L���D$��   D�\$ �D$����� f.�     AUATI��USH����~?�F�I��H�ZH�l��D  H��H�S�L��L���5�����uH9�u�1�H��[]A\A]�H�p�! H�
  H�4$H��H������H��I��tH�pH�E H��H�ԅ! ���   ��tZ�   H��$  dH3%(   �_  H��(  []A\A]A^A_�@ H���! H�
  H�4$H��H������H��I��tH�pH�E H��H�Ԃ! ���   ��tZ�   H��$  dH3%(   �_  H��(  []A\A]A^A_�@ H���! H�
  L��H��H���d���H��������$����   ~]����   ����   A����  I�H�
  H��H��H�������H��I��t"H�pH��! H��H�y! H� ���   ��tA�   �w��� L��H��H��������A��u�I�EH�|$ �3   �   H�|$H�@ H�D$�����A��tH���! I�uH��H� ��h  ���� H���! M�|$H��H��x! H� L�����   ���h���I�W H�D$H�2H�8H�T$������H�T$��   L��H���������  A�E(���D$��   A�E(   L�d$L��A�L��H��L���+���H����   L��H��L��H�D$������H�L$��   ��P������   A��P���A���   �T$A�E(    H��H�������������A���
  H��H��H������H��I��tH�pI�E H��H�2v! ���   ����t^�   H��$  dH3%(   ���0  H��  []A\A]A^A_�fD  H���! H�
  H��H��H���
���H��I��tH�pI�$H��H�8s! ���   ��t�   �|���@ L��H��H���*�����u�I�GM�nH��H��r! H�@ L��H�D$I�$���   ��u�I�E L�l$ �&   �   L��H�D$�����L��H����������   A�G(���D$��   H�t$H�|$L��A�G(   �����H����   H������� L��H��L��H��I������H��tL��H��L��������uL��H�������D$   A��P���A�~~�T$A�G(    H��H���������^���I�$�D$H��I�w��h  �D$�@���I�V�-   L���?���L��H����������I�V�,   ��L��H��������D$   �I�$L����   �p���������     AWAVAUATI��USH��H��(  dH�%(   H��$  1���H�$~}H�-N�! H�zA��H�E ���
  H�4$H��H�������H��I��tH�pH�E H��H�q! ���   ��tZ�   H��$  dH3%(   �_  H��(  []A\A]A^A_�@ H�ѻ! H�
  L��H��H�������H��I��tH�pI�H��H�n! ���   ��tZ�   H��$  dH3%(   ����  H��  []A\A]A^A_�f�H�Ѹ! H�
  L��H��H������H��I��tH�pH�E H��H�0k! ���   ��tA�   �fD  L��L��H���"�����u�I�EL��L��H�@ I���X�������  A�E(���D$$�   I�FA�E(   H�$� I�F M�w� M;wH�D$�2  H�L$4L�l$L�d$(H�L$�'D  I��A�$�P���A�$��   M9w��  I�>������ I��H�E 1�A�   L��H�4$H����0  H��toH�E H�T$H��H�t$��P  ��A��uQ�T$4��t�H�t$L��H���Q�����u5H�|$L�������D$$I���_��� H�E L����   �^���fD  M��L�l$L�d$(A��P���A��&  A�   L�<$H�E D�D$A�E(    L��   L�����
  1�1�H��H��A��A�D�D$�P���A���   H�L$��P������   L��D��H��������A��������D$$��u_H�E �|$$H��h  ���  H��H�����m���M�F�,   �   �   H�����������E��L�d$(L�l$�-���M�F�-   ��H�E I�}��`  �H�E D�$H����   D�$�V���H�E H�<$��   D�D$�)���H�E L����   �����E1�����������@ AWAVAUATUSH��H��H���   dH�%(   H��$�   1�����   H�.�! I��H�zA��I��H� ���
  �	   �   H��I���;���H�sH��H������H���   H��tH��L�K E�D$�AUL��L��H������ZYH��$�   dH3%(   u8H���   []A\A]A^A_� H���! H�
  H��L��H���������@ H�L$dH3%(   ����  H��([]A\A]A^A_� H�!�! H�
  L��H��H����������   M9�u������fD  I�uL��H�������������f�I���H�=ܼ  1�1��R ������     I�I�}���
  L��H��H���E���I�1�H��@��H��h  ���  H��H����1��u���D  I�uL��H���������Z���f�I�uL��H���a������B���f.�     I�H�
  L�x�L9��E  H���! �����H�=��  H����  I�t$H�L��� H��H��H)����   H�1�1�H�5ɲ  H�����   H�I�uH���� 	  H�1�1�H�5��  H�����   H��   H��L����8	  �U �J����M �@  �����t!���p  ����   D�l$�����D  H�L��H��  ��@  H�T$H��L���Յ�u�H�L�����  D�l$E�������H�I�l$L��H���   ��@  E1�H��H�
  H�D$XH�HH�8H�L$X��L��H��H���������  ��H���������"  �D$T�P����T$T�H������H�D$8L9�H��u*�  f�     H�;H���u�����uH��`L9���   H�3H���Y�����t�H�D$H�|$�
  I�} H��H���8�����y�H��   L���T���H�|$ L���w���H�|$�ݭ��L��E1�����H��8L��[]A\A]A^A_�H�t$H�|$������x:H�L$(H��t�H�|$藭����H�L�Ͼ	   E1��2���H�|$ L�������H��  �   L��迯���f���f.�     AWAVAUATI��USH��H��H��H��8  dH�%(   H��$(  1��+�����uL�%X�! I�}I�$���
  H��H��H�������H��I��tH�pI�$H��H�"O! ���   ��t`A�   H��$(  dH3%(   D����  H��8  []A\A]A^A_�fD  L�~�  �)   �   �   H��A�   �Ĳ���f�I�FH�L$@H�T$0I�uH��H�@ H�$I�$��x  ���q���H��I�$I�uH�T$<E1�H��L���  �   RH��X! ���	  ��^_�6���I�$I�} ���
  H��H��H������H������H�pH�D$H��I�$H�N! ���   ��L�\$�����I�CH�L$HH�T$8I�u(H��H�@ H�D$I�$��x  �������H��I�$I�u0H�T$DE1ɹ   L��  H��RH��W! ���	  ��ZY�|���HcD$<H�
  H�|$H���{���H�T$H�$I�$H�:���
  H�|$ H���Y���D�$L�\$E���g���H�D$@L� �   H�ߺ   �   �د��H�|$��������M�E0�(   ������v���H�|$�   L�$����H�|$�   ����L�$H��H�D$L��L�\$�˫��H�T$�$H��苩���<$ L�\$�i  H�|$�   L�\$ 踵��I�$I�} ���
  H�L$PH��H��H��H�L$����H��H��H�D$�2���H��I��L�\$ ��   HcT$4H�
  L��H��L��H�D$蘨��H��H��tH�pH�L��H��I! ���   ��teA�   H��$x  dH3%(   D����  H�Ĉ  []A\A]A^A_� L�d�  �)   �(   �   L��A�   �l����f.�     H�EH��$�   H��$�   I�t$L��H�@ H�D$H���x  ��A���b�����$�   ����H�L$D�|$ I��L�d$(H�D$H�l$H�	I���R�    H�H���
  H��$�   H�HH�8H��$�   ��L��H��H��蟥�����  ��L��蝯�����-  ��$�   �P�����$�   �H�l$H�t$D�|$ L�d$(H���T������a  H�L$�D$I�<$H�AH+H��H�D$0H����
  H��$�   H��L��H��H�L$`�+���L��H��H�D$h�K���H��H�D$@�L$�/  H�t$@H�}H�FH�(�HH�D$H�<����   ����H�D$ I�D$H�D$�$����  ��D�|$|L�l$(��L�4$H��I�D(H�D$pH��$�   H�D$PH��$�   H�D$XH�L$H�H�9���
  H�|$H��H���˩�����   H�|$ H��覫������  L�t$(H�4$H��L������H��I����  H�pH�L��H�G! ���   ���m  I�GH�L$PL��H�T$XH�h H�D$H�pH���x  ���>  �D$0;�$�   �^  H��   �P(1�I��1�L�8赦��I�D$I��H�D$HL��H�x����H�D$L�u H�(�PfD  H�H���
  H��$�   H�HH�8H��$�   ��L��H��I���7�������   H���U���L��辬����$�   �P�����$�   ��   L������I�H�t$@�^���H�D$H�D$H;D$p�l���D�|$|L�l$(L�4$H�t$@L���M�������  H�|$�{���H�|$ �A���H�H�|$`���  �����fD  I��   L��(   �   莩��H�|$�4��������    I��   ��fD  L�l$(M��   �(   �   L�4$L���E���H�|$A�   ����H�|$ 諣��H�|$hL���N���H�H�|$`���  �L���M�D$�   �(   �   L��A�   �y���H�|$菢������M�$�%   �(   �   L���O���H�|$�e���H�H�|$`���  �����L�4$�T���L�l$(I��"   �(   �   L�4$L���v����,���H�D$L�l$(�$   �(   �   L�4$L�@L���ا�������L�l$(L�D$�*   �(   �   L�4$L������������d���H�|$hL��A�   �1����'���f�f.�     AWAVAUATI��USH��H��H��H��  dH�%(   H��$x  1��{�������   L�%��! A��H�;I�$���
  H�=�  H�ƹ
   �u
H��A�   I�$H�{���
  L��H��H������H��I��tH�pI�$H��H�AC! ���   ��t_�   H��$x  dH3%(   ���$  H�Ĉ  []A\A]A^A_��    L���  �)   �   �   H�������   � I�GH��$�   H�T$hH�sH��H�@ H�D$I�$��x  ���n���I�$H��$�   H�T$lH�s0H����x  ���H���H��I�$H�sH�T$|E1ɹ   L���  H��RH�|L! ���	  ��ZY�
  L��H��H������H��H�D$����H�pI�$H��H�AA! ���   �������H�D$H��$�   H�T$pH�s(H��H�@H�@ H�D$0I�$��x  �������I�$H�{@���
  L��H��H��藟��H��H�D$ �����H�pI�$H��H��@! ���   ���z���H�D$ H��$�   H�T$xH�sHH��H�@H�@ H�D$HI�$��x  �����@����D$h9D$p�&  �D$l9D$xtoL�CH�$   �   �   H���ܣ�������    H��I�$H�s8H��$�   E1ɹ   L�M�  H��RH�2J! ���	  ��ZY�}��������H���I�$�L$H�;���
  H��$�   H��H��H��H�L$@�����L��H��H�D$8�|���H��H�D$(�L$��  H�D$(H�T$1��L$\H�@H�PH��L�8HcD$tH��I! H�L$H�T$PH��HЋp�@�q�A1��\���H�L$H�T$P1�1�H�A H�D$ H�A(HcD$|H��HB�A0�B�A4�$���H�L$H�A@H�D$D�qHL�q �L$\H� �L$PH�D$H�D$0H� H�D$0�zH��$�   I�$H�:���
  H�|$H��譛��H��$�   �D$\I�$H�:���
  H�|$0H��臛���L$\����  ���Q  ��L������H��$�   H��$�   �D$p�P����T$p�s����   L���U����   L���(���H�|$H��H�D$0����H�L$H�T$0�AH���ћ��H�L$�y �L$P��  L���   �L$P�����H�D$�L$PL�p@H�D$H�L$HH� H�D$0�zH��$�   I�$H�:���
  H�|$H��蛚��H��$�   �D$PI�$H�:���
  H�|$0H���u����L$P����  ����  ��L�������H��$�   H��$�   �D$x�P����T$x�s����L$H�   L���L$0�;����   L������H�|$ H��H�D$�����H�L$H�T$�A8H��跚��H�L$�y8 �L$0�N  �   L���L$����H�\$H�t$(H�{� ���H�D$ H�;H�0� ������L$tH�D$ H�t$(H�x������L$H�\$(I��L$H���ܘ��H��H����������L$uH�|$8L��蜜���   I�$�L$H�|$@���  �L$����L�C(�����H��$�   L� �   �   �   H��谟��H�|$8L���C���I�$H�|$@���  �   �����H��$�   �L�D$8H��%   �   �   �ԟ��I�$H�|$@���  ����H��$�   �y���H��$�   �l���L�CH�   �   �   H�������f���L�C(��AWAVI��AUATM��USH��L��H��H��  dH�%(   H��$  1��t$�������D$(�X  L�%)�! I�>I�$���
  H�T$0H��H��H�T$����L��H��H�D$ �_���H��H�D$�q  H�D$L�x�D$I�_���I�?�ƉD$,I�^蘣���D$,���  �D$��I�D�H�D$�If�     H�pI�$H��H��:! ���   ��uLI�?L���ɖ��I�~H�t$軖��H;\$��   I�$H��H�{����
  L��H��H������H��I��u�H�|$ L���[���I�$H�|$���  �D$(   H��$  dH3%(   �D$(��   H��  []A\A]A^A_��    L���  �)   �)   �   H������D$(   ��     H�t$H���������c���I�$H�|$���  �s����    M��%   �)   �   H�������I�$H�|$���  �D$(   �6���蕘��D  AWAVAUATI��USH��1�H��8H��! dH�%(   H�D$(1�H�|$1�L�d$H����  H�$H�H�}`L�����  H��tjD  ���   L�p �Q���uL�p H�L��L�����  ��t+H������L��L��p  ���  H�4$H��H�|$A�ׅ�uLH�L����  H��u�H�H�4$H�|$��h  1�H�L$(dH3%(   uBH��8[]A\A]A^A_��     H�$��B�����   �H��D$H����   �D$��`���AWAVAUATI��USH��H��H��H�-�! H�E ���
  H��H��L��芖��H��tsH��H�E 1�1����  I��H�CL�pH�I9�u�g�    H��I9�tWH�H�E �����H�zL��p  ���  L��H��L��A�ׅ�t�A�E �P���A�U ~;H���   []A\A]A^A_�fD  H�E L��L����h  H��1�[]A\A]A^A_�H�E L����   ��     AVAUI��ATUH��SI��H��H���   dH�%(   H��$�   1��ݕ��H��v	�;:�~   H�l$H��L��H���{���H�|$L��A�������H��u	E����   L�5��! I�H�D$H�����  H�D$H��tEH��$�   dH3%(   ��   H���   []A\A]A^�@ �{:�x���L��H��苝��H��u�I�ع#   1Ҿ   L��H�D$����H�D$� L�5�! H��I����  I�H�����  I�H������H�5'�  ���  I�H��H���������  H�|$L���������������@ f.�     ATUI��SH��H��H��H���ؘ��H��tH�xH��觕��[1�]A\�M��H��&   �   �   �V���[�   ]A\�f�f.�     AWAVAUATI��USI��H��H��H��H��! H����
  L��H��L���X���H����  I��H�1�1����  A�4$H�Ń��O  ��  ����   ��usH������H�=��  M�t$L��p  ���  H��H��L��A�ׅ��v  fD  �U �B����E �   EH��$H����   �$H��[]A\A]A^A_�@ H�H�=��  1��R H�H��L����h  1�H��[]A\A]A^A_�f�     H������H�=Ň  M�t$L��p  ���  H��H��L��A�ׅ��[���A�FH��t0H������H�=^�  L��p  ���  H��H��L��A�ׅ��#���H�I�|$�����L��p  ���  H��H��L��A�ׅ������I�H������H�:L��p  ���  H��H��L��A�ԅ������I�I�V 1�L��H�p����H�������H�H��H��L����p  �������IcNIcVL�%�(! H������H�JL��p  I�<����  H��H��L��A�ׅ��N���I�VH����H�:L��p  ���  H��H��L��A�ׅ�� ���I�FI�V �   L��H�p�W���H�������H�H��H��L����p  �������I�I�V@1�L��H�p����H�������H�H��H��L����p  �������IcF0IcN4���H�H�L��p  H�I�<����  H��H��L��A�ׅ��s���I�V(H����H�:L��p  ���  H��H��L��A�ԅ��E���I�v(I�V@�   L��H���|���H���#���H�H��H��L����p  ���	����K���@ M�t$�����H�=Z�  I�FH�@H�$H�L��p  ���  H��H��L��A�ׅ������H�I�|$�����L��p  ���  H��H��L��A�ׅ������I�FI������L� H�:I�$H�D$H�L��p  ���  H��H��L��A�ׅ��R���H�D$I�6�   L��H�PH������H���,���H�H��H��L����p  ������L9$$uR�N����    I�6I�V1�L��H���;���H�������H�H��H��L����p  �������I��L9$$����M�4$H������I�L��p  H�:���  H��H��L��A�ׅ�t�����fD  H������H�=��  M�t$L��p  ���  H��H��L��A�ׅ��K���H�I�|$�����L��p  ���  H��H��L��A�ׅ�����I�H������H�:L��p  ���  H��H��L��A�ԅ������I�I�V(1�L��H�p�)���H�������H�H��H��L����p  �������IcFIcNL�%%! H����H�L��p  H�I�<����  H��H��L��A�ׅ��v���I�VH����H�:L��p  ���  H��H��L��A�ׅ��H���I�FI�V(�   L��H�p����H���&���H�H��H��L����p  ������IcFIcN���H�H�L��p  H�I�<����  H��H��L��A�ׅ����������fD  I��L��&   �   �   �Α��H���   []A\A]A^A_�f.�     H�I�|$�����L��p  ���  H��H��L��A�ׅ��b���I�L� L;`tCfD  I�$H������H�:L��p  ���  H��H��L��A�ׅ�� ���I�I��L9`u�H�I�VH��L����p  ��������:��� AUATI��USH��H��H��H�-^x! H�E ���
  L��H��H��I��蛐��H��t6H�U H�x�����H��h  ���  H��H����H��1�[]A\A]��    M��H�߹&   �   �   �����H���   []A\A]�fD  AWAVAUATI��USH��H�>H�GH+H��GH��w! �����H�=D�  H� H��h  ���  L��H���ӻ   H����[]A\A]A^A_�fD  ��I���>�����H�$t?H�$H�@H�hH�H9�tf�H�;H��脉��H9�u�   H�<$����I�>�����1��P���H��H�$H�@H�HL� L9�H�L$t;@ I�$H�@H�XL�8L9�t@ I�7H��I���)���L9�u�I��L9d$u�L�eH�] I9�u�/f.�     H��I9�tH�3L��������u�H�������%���H������H�$H�@H�hH�H9�t�    H�;H������H9�u�1�����ATI��USH�>H�������� ��u1���[��]A\�f.�     ��H��L��[]A\����D  f.�     USH��H��H��H��(H�T$dH�%(   H�D$1��7�����t#�   H�L$dH3%(   u'H��([]��    �t$H��H�߉D$轋���D$������f�ATUH��SH��I�̻   H��H�T$dH�%(   H�D$1��������u�t$L��H�������H�L$dH3%(   ��u	H��[]A\��{����f.�     AWAVAUATUSH��H��1�1�H��HL�-�t! dH�%(   H�D$81�I�E ���  H�[ I��H����   H�D$ L�t$H�D$�LfD  H�D$H�t$�   H�D$ H�CH�D$(I�E L��p  ���  L��H��H��A�ׅ�uH�H��tJ�sL��H��������t�A�$�P���A�$~@�   H�L$8dH3%(   u:H��H[]A\A]A^A_�@ I�E L��H����h  1���@ I�E L����   ��T���@ AVAUL�-�s! ATI��USH���E,@   H��1�I�E ��8	  ����t�E,    ��[]A\A]A^�@ I�E L��L��   L��   ���  H�=V�  ��1�A��L��H��A���@ USH��H�ӿ   H��H�`s! H� �P(H���   �@    H�X�H�H���   H��1�[]�f.�     AWAVH��AUATI��USH��L�%
  H��tnI��1���     H��H�H��tT�{ u�I�$H�{���
  L��H���&�����u�H��t=H�H�E L�;H�{��P����~/I�$H��L���P0H��u�H��1�[]A\A]A^A_�L�;M���   ��I�$��   ���     AUATI��USH��1�1�H��H�-=r! H�E ���  H���   I��H��u�Zf.�     H�H��tHH�E H�SL��L����p  ��t�A�$�B���A�$�   -H�U �D$L����   �D$�@ H�E L��L����h  1�H��[]A\A]� 1�H�~  t?�N,��t��fD  SH��E1�1��F,   H�������H���C,    ����[�fD  ��fD  �H�~  H��t�V,SH���t[��D  H�&! �F,   E1�H�������C,    [�� f.�     AWAVAUATI��USH��M��H��H�FH��H�L$L��L�p I�>莎��H��ti� H��H��L��H���Ŏ��I�ǋ�P������   M��t=I�_ I�>H�s������uAL���   L������L��H��辌��A��P���A�~HE1�H��L��[]A\A]A^A_� H�sI�>����H�L$H��H��L��H�L���Ӂ����t���D  H�p! L��H� ��   � H�p! H��H� ��   �F����     �H�~  H��t�N,SH���t[��D  H��H��$! �F,   I���'����C,    [�� f.�     AWAVI��AUATI��USH��H��H��(H�FL�-so! L�D$L�L$H��H�/$! H�@ H�$I�E ���   ��t�   H��(��[]A\A]A^A_� �I�>蝎��� H��H��L��H��H�D$�d���L�D$I��A� �P���A��  ��P������   M��t�I�E H��#! L��H�����   ���u���I�G H�pH�$H�8������uNH�\$L���   H�������H��H���Ί��A��P���A��+���I�E L����   ����f.�     I�6L������L�D$H�<$L��H��L���������tXA��P���A������I�E L����   �����@ I�E H����   ����fD  I�E L����   �����fD  H�L$H�|$L��H���{~����H�$L��I�61�L��H+PH���-������m���H�\$L���   H�������H��H��   ������C����f.�     AWAVM��AUATI��USI��H��1�H��H��H�-Bm! L�d$PH�E ��8	  ��A��t[�����t��tH��D��[]A\A]A^A_�D  H�E H����@  H��M��M��L��L��H��[]A\A]A^A_H���^���fD  �D$H�E H��H��   L��   ���  L��A������L��I���#}��D��H��L��H�=�|  1�A��H��H����D�D$�U���f�f.�     AWAVAUATUSH��H���H�~  t
�F,I����tH��H��[]A\A]A^A_�f�     I��H��H�!! E1��F,   H������H��H��A�E,    ��   L�5�k! L�x H��I�uH�� ! I����   ��ugI�EI�?H�@ H�0�������q���I�?����   H��L��I��豀��H��L���և��I�L���P0��P����~i1��/���f.�     I�H����@  L��H�¾   贉��L��H��艇����    H�4�  L��   �<���L��H���a��������@ I�H��1���   ����D  H�~  tS�F,   H��E1�1�1��R����C,    [���     AVAUATUI��SH��H��H���w~������   L�5�j! L��I����
  H��H��H���>~��H��I����   H�pI�H��H�i! ���   ��A����   I�T$H�BH+H��H��wjL��H���_���I�I��L�����I�4$H�߹"   1���  I�1�1�I�4$H����   L��H���O}��I�H�����  [D��]A\A]A^��    M�$�'   �   �   H���ł��A�   [D��]A\A]A^� L��r  H��A�   �)   �?   �   茂��[D��]A\A]A^�AWAVAUATUSH��HdH�%(   H�D$81�H���    ��  H�Ei! A��I��H��A�ξ����H�=q  H����  E��I�ž����H���  H�=�q  ���  H�$H�D�����  H�$A�E L��H�D$�� H���@  L���   � I��H�D$0H�D$H�D$,M��H�D$u��   M�?M����   H�1�1����  H��H�I�WH��L����h  ��u�H�L��H��L����p  ��u�H�H�$H��L����p  ��u�H�H�T$H��L����p  ��u�H�H�L$H��H�T$L����x  ���  �E �P����U �U���H�H����   �D���@ A�E �P���A�U ��   H�$��P������   H�L$��P����~wH�L��L����h  A��P���A�H�L����   fD  H�D$8dH3%(   ��   H��H[]A\A]A^A_�D  H�=�o  ���  H�$�C���f.�     H�H����   �x����    H�H����   �O����    H�L����   �#���H��   H�T$0�t$,L����0	  ������+{��f.�     �H��f! ATI��USH�_H+_H� �����P(Hc�H��1�H����{��H�E�E     L�eA�$H�EH��[]A\�D  f.�     USH��H��H��H����w��H�{H�H�pH9�t'L�E�     Hc
H��H��I��H�N��H9�u�H��[]�@ f.�     H����   AUATI��USH��H�_H�L�%�e! H��HoH+oH9�u�=f�     H��H9�t'H�;H��t��P�����I�$H����   H9�u�I�}��z��I�$L��H�@0H��[]A\A]����@ f.�     H��t������~��fD  �z�� AWAVAUATUSH��H�GL�vH�oL�nL�`H�XI9�u�K H��I9�t?L�;L��H��I�7��v��H�H�u�L��I�T� �B�����u�H��[]A\A]A^A_��    H���   []A\A]A^A_�@ H9�tCUSH��H��H��H�vH�������uH��[]��    H��H��H��[]�z���    �   �f�AUATE1�USH��L�oH�oI�]I;]t�H��H��H�u�H�{��;~��A�I9]u�H��D��[]A\A]�@ AUATUSH��H�H;^tDI��H��E1�@ HcH�UH��H�4�H�UH�RH�<���}��A�I9]u�H��D��[]A\A]�E1���@ AVAUI��ATUI��SH�I��H;^H�)u�I�H��H��I9]t:HcM I�VHcH��I�L$H�4�I�L$H�IH�<�踁����u�[]A\A]A^� [�   ]A\A]A^�f.�     AWAVI��AUATI��USH��I��H���?~H�c! H�=cs  H�1��R L�eL��L������I;D$H��tbH�01�L��L������H����   I+\$H��Hc�H��H��HUH�:H��t��q����7~FH�� �   H��[]A\A]A^A_�@ L��L��   �w��H��1�[]A\A]A^A_��    H�Yb! H�D$H���   H��H�D$HU�D  L��   L��萀��1��f�f.�     USH��H��H�_H�����H;CtH+CH�UH�H��[]�f�1���f�f.�     AUATUSH��H9�tTH��I��H��E1�� H��H�S�H��A��H�U��I9�t*H�} H��tۋ�B�����H��a! H� ��   �E1�H��D��[]A\A]�f�f.�     USH��H��H���r��H�}H��H�EH�SH��HpH+p��{��H��H��[]�fD  AWAVI��AUATUSH��H��H�QH�GH�IH�_H�T$H��L��L�hL�aL�q�P|����t&H�T$I��M)�M)�M)�J�4;J�<L��k{���   H��[]A\A]A^A_��    ATUI��SH�_H�{H+{H����q��H����q��H�SH�sH��L��H���fx����tH��[]A\�f.�     H��1��u����@ USH��H��H��q��H�{H��H�CH�UH��HpH+p�z��H��H��[]�D  USH��H��H��H���\q��H�{H�H9�t+L�EH�p�     Hc
H��H��I��H�N��H9�u�H��[]�@ f.�     AWAVAUATI��USH��H���FL�oL�w�@�BH�l_! ��H� ���P(Hc�1�H�EH���t��I�]I;]I�T$H�MtL�   �rI��H�;H�iL�b H��D�|0I�v��x��I9]A�DL��H��u˃�H��[]A\A]A^A_ø   ��f�AUATI��USH��I��H��H���}��L��XL��H���y��H���[]A\A]�fD  AWAVAUATUSH�^H��(H�GL�wH�jL�a�{H�t$L�xL;xH�D$��    M�/I��I�F�H��H��I��L��H�D$�u��Hc�H��M��H�L��H�� H�^� H�t$I��H���#{��H�L$Hc�H�� H�BL9yH��u��}H+D$H��([]A\A]A^A_�H�D$H��H���� AVI��AUATUI��SH�^�{H�H��I��H���n��Hc�L��H��H�L��H�^�}�F H���7n��H�H��H�L)���x��L���q����[]A\A]A^�@ AUATUSH��H��XL�d$ H��H�D$     dH�%(   H�D$H1�L��H�$    H�D$(    H�D$0    H�D$8    �D$    H�D$    H�D$    �$   �Fm���xH��\! H� �P(H��L��H��H��H���Bm��H�L$HdH3%(   H�� uH��XH��[]A\A]���p��f�     AUATUSH��H��XH�L�d$ H�D$     H�$    dH�%(   H�D$H1�L��H�D$(    H�D$0    H�D$8    �D$    H�D$    H�D$    �$   �{��H�\! H��L��H��H� H�X(�Iv���x��H��L��H��H��H���l��H�L$HdH3%(   H�� uH��XH��[]A\A]��p���    AWAVAUATI��US�n�H��  ��dH�%(   H��$  1�H�T$��VUUU�����)R)��L$��   H�D$H�|$ �Ӿ   �   H�|$L�p�|m�����l����I���W  ���+fD  H��L���r��I;D$�
  ��I������   I�VH�L$L��I�6�s��H��I��u�L���7u���D$   H��$  dH3%(   �D$�  H��  []A\A]A^A_�f�H��Z! H�
  H��H��L���q��I;EI��I��   tH� �xt"I��   �@   �   H���xm���fD  L��I+EI�VH��H��H�H��I�$H�	! H��H�$���   ����H�$�U���L�A I�FI�HH�PH+PL�D$H�AH+AH��H���|��@e��H���he��I�uH��L��L��H�$��k����L�D$tI�@H�$L��H�PH�p��k����u1L�O  �   �@   �   H�߽   �l��H�<$�Qh���v���I�UH�$I�wL���xk����t�H�<$I�$L��h  ��r��H��H��A���<���f�AWAVAUATI��USH��H��(���3  H�$S! L�j�t$H��! H�L�����   ���D$D�D$��   M�u A��I�FH�D$�  H�D�D$1�1�L�}���  D�D$I��A�@�H�D�H�D$�E H�I��I�����
  H�|$H��H���wd����x;I�VH�L��L��H��H���p  ��u8L;|$u�H�L��L����h  �FfD  I��   �   �   L���>k��A�E �P���A�U H�L����   f��D$   �D$H��([]A\A]A^A_�D  H��Q! H�
  H�|$H��H���c����xI�VH�L�,��1���I��   �   �   L���j���D$   �a���f�     AWAVAUATI��USH��8  dH�%(   H��$(  1���H�|$�;  L�-,Q! H�ZA��H��! H�|$I�E H�����   ���D$t<�D$   H��$(  dH3%(   �D$��  H��8  []A\A]A^A_��    H�C H�$H�hI�D$H�D$(A�G��ǉÉD$ ��a��H��H�D$�b����I����   A�G�I�\$ M�|�(�)�H�<$H�PL��H���mh������  H��L9���   I�E H�;���
  H��H��I���Mm��H;Eu�L����d��M��   H�|$�@   �   �i���D$   ����fD  H��O! H�
a  �   H� ��P  �D$   ������    L���8o��H�D$0H�$H�PH�BH+BH��+D$ �x��`��H��H�D$8�a��H�D$ H�EH;EL�xu
  H�t$H��H���Bj��L�t$8H��L���rf��I;F��   H�T$0H�|$ H��H����j����uTH�|$ �uc��H�L$0��P����~rH�|$H����j���D$   ����f.�     L���8c��M��   �D���I�E H�|$ H��h  ��m��H�|$H�����C���H��   H���Gc���s���I�E H����   �|����kb���f.�     AWAVAUATUSH��  dH�%(   H��$�   1���H�<$�  H��M! L�bA��H�T$H�<$H��! H�L�����   ������   E�~�L�L$A���   I�|$ L�d$L�L$�_���A   �   L��I���_��E��L�L$��   A��M�yA��I��O�t1(� I��M9���   H�I�?���
  I�WL��H��L���-i����u�H�<$L��   �Hi��L����a���fD  �   H��$�   dH3%(   ����   H��  []A\A]A^A_��     H��L! H�
  I��H�E H�{����
  L��L��I���Ah��H9D$I��u�H�|$ M��   �-   �   �d��H�|$(��_���D$   ��    �D$   �D$H��8[]A\A]A^A_�D  H��J! H�
  H�<$H��H���TZ����y�H�|$I�ع   �*   �   A�   �Na��L����Z���C����H�1H! H�
  L��H��H����U����y�H�|$I�ع   �   �   ��\��L���V���D$   � �D$   �D$H��([]A\A]A^A_�D  H��C! H�
J��H�<$�AO��H�|$�G��H�D$H�     H�$    �e���E1�����fD  AWAVAUATI��USH��H�I+}H�t$�����H���R��I�]I��I�EH9�tl1�E1��    H�H�|$E�gH�0�DF��1�D9�D����L��H��Չ�E����K��I�EH9�u�I�UHc�H)�H��H9�tH��L��[]A\A]A^A_�H��1���L��E1��F����@ AWAVAUATUSH��H�_H;_��   H�T$I��I��E1�fD  L�;L��I�7��P��I;D$H��t<H�0L���MP����t-H�D$H��tH��H��I+T$I+uH��H��H��� O��A��H��I9]u�H��D��[]A\A]A^A_�E1���f.�     AUATUSH��H�nH�L�gH9�t$I�� HcL��H��I��H�0��D���C�H9�u�H��[]A\A]��    AVH��2! I��AUATE1�USH��H�_H+_H� H���^�����P(Hc�H�E1�H���H��I�^I;^H�ms8�    L�#H��H��L���K��H��L��F�l(H�� ��D��I9^E�lw�[D��]A\A]A^Ðf.�     AWAVI��AUATUSH��H��L�L;H�t$L�bsgf�     M�/L��H��I��L���kI��H�L��L��H�I�� H�k� H���}P��H�H��E  M9~H�]w�H�C�H+D$H��[]A\A]A^A_À~� H��u�H�\$��f�ATUI��SH��0dH�%(   H�D$(1�H��1! H��H�$    H�D$    H�D$    H�D$    �$   H� H�X(�UP���x��H��H��L��H���?B��H�H��� �L��H�L$(dH3%(   uH��0H��[]A\��pE��SH��H� �K��H�CH���E���C[�f�SH� H���C��H�C � H���  H�C[�D  f.�     SH��H� �sJ��H�C     H�C    [ÐH��0! SH��H� ���  H�]�  H�X H�P�H�@    �@    [�f�     AUATI��USI��H��H��(dH�%(   H�D$1�H�>0! H�L$H�T$H� ��x  ����   �D$���   ��������QA��H�ŋD$��~`H�D$�5�    H��H���}G��H;Etg�D$�P�H�D$�T$H����H�D$~$H�PH�0L��L����G��H��H��u�H��1���I��H�L$dH3%(   H��u^H��([]A\A]��     H�L��   �D��L��L���K��H���H��� 1��H�ھ	   L���M��L��L��1��wK����C��AWAVI��AUATI��US�׉�H��H�t$��K����H��~j�C�L�-�.! M�t��fD  ��H����J��M9�tDI�E I��I�����
  L��H��H���@����y�H�|$I�ع   1�1��G��H��1��MA��H��H��[]A\A]A^A_Ðf.�     USH��H��H��H��H��(dH�%(   H�D$1�H�O.! H�T$H�L$H� ��x  ��1���uH�L$�T$H��H���hA��H�|$dH3<%(   uH��([]��\B��f�f.�     AWAVI��AUATI��USH��H��L��H��H��8dH�%(   H�D$(1�H��-! H�L$ H�T$H� ��x  ����   A��I�EHcL$L�pH�@L)�H��H��H9���   H��M�}u2�   f�     I�U� I��I�G�H�BH+BIEI9��}   H�D$ I��I�v�H��H��H�PH�T$ H��J��H��tNI�?H��t���J�����H�D$H��,! H���   H�D$� �   H��L���@K��H��H���I��A�   H�\$(dH3%(   D��uH��8[]A\A]A^A_���@���     AWAVAUATUSH��H��H��H��8L�5z,! dH�%(   H�D$(1�H�4$H�L$H�T$I�H�L$ H����x  ���C  �D$��;  ��������q=��H��I���=���T$I��H�D$ ��p��   �H�PL��H��H���nG������   I�D$H�L$L��H�<$H�p�H�X��H��H��t{I+\$I�WH�� �D$�P�H�D$ �T$H����H�D$ ~~I�H�8���
  H��H�D$ H��H��L�h�H��H;E�i���H�ھ   H�\$H���t@��H�<$H���G��L���@��1�H�L$(dH3%(   u_H��8[]A\A]A^A_��    L���J����fD  H�ھ   �fD  1��H��H�\$�	   H���OI��H�<$H���#G��1���*?��f.�     AWAVI��AUATI��USI��H��L��L��H��(H��*! A�   dH�%(   H�D$1�H�L$H�T$H���x  ��uq�T$����   ��A��H�D$ �V��D$�P�H�D$�T$H����H�D$~8H�H�8���
  H�T$H��H��L��H�R�0F����u�H��L��A�   �KF��H�\$dH3%(   D��u;H��([]A\A]A^A_�f�     L���	   H���0H��H��L��A�   ��E����>���     AWAVI��AUATH��USI��L���   H��8L�%�)! dH�%(   H�D$(1�H�T$H�$H�T$I�$H�L$ ��x  ��uJ�D$�uq�����I�E��Hc�H�PH+PH��H9�t`H�T$�   L�<$L���zG��L��L���OE��H�L$(dH3%(   ���  H��8[]A\A]A^A_�f.�     H�T$�	   �@ 1���F���T$H��H�D$ ��H�   @ ��H���V>������   �   D��H���.@���D$�P�H�D$ �T$H����H�D$ ~YI�$H�8���
  I�}H��H���5:����A��y�H�<$H��   �M=��H�߻   ��:��H�4$L���dD�������    H����:��H�$H�T$L��L���|:���������D  H�<$H��   ��<����(<���     AVAUM��ATUI��SI��H��H���#9��L��L��L��H��H����>����uCH�UH��tH�RH��t
  I�~H��H�D$�7��HcL$,H�T$0��H�t���7  I�VH�A�   H��H��I�1���0  H��t4A��I��D9�twI�H�L$H��H�$I�4$��x  ���f���fD  �   H�\$8dH3%(   ��   H��H[]A\A]A^A_��     H�GL�wL�=�$! L�hL�`M9�u0@ I���L��h  ���  H��H��A��1�떐I����M9�t�I�4$I�I��1�I�N�A�   H��H�6���	  H��u��W����    H�\$I�$�	   H����B��H��H���@���*���f.�     H�\$H�T$�   H���Y9��H��H���~@�������1�L�=($! �;����v8��fD  H��t�7�W�G    �� f.�     H��tCATUI��H�-�#! SH�_�wH��H�E ���  H�E L��H��[]A\H���  �������D  ��fD  H��t;US��H��H��H��H��#! H� ���
  H����H��[]H���8��f.�     ��@ f.�     AWAVAUATUSH��H��H��tH��u5H����   L�%-#! I�$H�{H���  H��[]A\A]A^A_��fD  L�%#! L�nH��I�$���  �KH���  I�$H��L���   L�<���@  E1�H��L�Q0  L��H�E0  L��1�A�֋SH�-�  �sM�$H��M��j H��H�ЋH�M�  H��H��  L��1�H�5u6  A��0  XZ�2����    H��[]A\A]A^A_ÐAUATM��USA��H��H���   H��dH�%(   H��$�   1��O4��L��D��H���!7��H��H���F>��H��$�   dH3%(   uH���   []A\A]��06��AUATM��USA��H��H���   H��dH�%(   H��$�   1���3��L��D��H���@��H��H����=��H��$�   dH3%(   uH���   []A\A]���5���H�'�  H���f��WH���  H��ÐH�H+�f�     �+Ðf.�     AUATLc�US�   H��L�%!! I�$�P(H��I�$B�<�    �P(H�H�CJ��H�CH��H��[]A\A]ÐAUATI��USH��L�-� ! H�_H+�   I�E �P(H��I�E ���P(H��H�E I�4$H��H��Hc�H��H�UH�UHc���8��H��H��[]A\A]�fD  H��t3USH��H��H�-S ! H�?H�E �P0H�E H��H�@0H��[]��fD  ��fD  ATULc�SH�H�GH)�H��L9�}7H�oH� ! H����H��H)�H� H��Hc��P8H��H�J��H�SH�C[]A\�f.�     H�H�WH9�s@ �0H��H9�r��� AVAULc�ATL�%�! A��US�   I�$�P(H��I�$B�<�    �P(H�J��D��H��H�CH�C�3>��H��[]A\A]A^��    H�H�WH9�s@ �0H����H9�r���USHc�H��H��H��HH;Gr H�! H��H�=6=  H�1��R H��HE � H��[]�@ f.�     ATUA��SHc�H��H��HH;Gr H��! H��H�==  H�1��R H��HE D� []A\�@ f.�     H�H;Gt� �@ H�q! SH��H�==  H�1��R H�[� �D  f.�     H�GH9t�@�� H�1! SH��H�=�<  H�1��R H�C[�@�� f.�     US��H��H��H�WH�GH9�rH+H����������t�/;��H�SH�BH�C�*H��[]�f.�     SH�GH��H9tH��H�C[�f�     H��! H�=b<  H�1��R H�CH��H�C[�f�f.�     AWAVA��AUATI��US��H��H��H�WH9�r~I�E H��H��H)�H)�H��H��I��I�MD�H)�H��9���   Mc�I��Mc�I��H9�rjL�E��I�UtA�V�H�T�D  H���k�H9�u�H��L�[]A\A]A^A_�f�H��! H+7H�=�;  H�1�H���R I�U�\���f.�     J�<;H)�H���Q8��I�UI�E �z���@ L��Mc���9��I�E I��I�UJ� �J���ATUI��SH��H�WH��H9�rJH9�v%H�6! H��H+u H�=p;  H�1�H���R H�UH)�H��L����7��L)�H)]L��[]A\� H��! H+7H�=/;  H�1�H���R H�U� f.�     ATUI��SHc�H��H��H�H�H�H9�vNHc�H��H�(H9�r H��! ��H�=�9  H�1��Q I�$H�(H؋1��0�H��[]A\�f.�     H�Q! �T$H�=~9  H�1��Q I�$I�|$�T$��    ATUSH����1�L�c�,9��H�H��I9�t��3�   H��H���2��I9�u�H��[]A\�H�H�wH�
fD  90tH��H9�r�������@ H)�H���1�� f.�     USH��H��H��H�H9�wH;wrH�'! H�=�8  H�1��R H�E H)�H��H��H��[]Ðf.�     AWAVAUATI��USH��H��L�g�   L�v�5��H�I��I9�t+�    �+L�����7��I9�t
��L����+��H��I9�u�H��L��[]A\A]A^A_�f�AWAVAUATI��USH��H��L�g�   L�v�:5��H�I��I9�t!�    �+L����7��I9�tH��I9�u�H��L��[]A\A]A^A_�@ ��L���>+����f�f.�     L�H�WH�NH�>L)�H��H)�Hc�H��H9�t1����fD  H��H��H��L���.������H�����AWAVAUATUSH��L�oH�L�~I9�tXI��I��1�f.�     �3L����6��I9���H�����I9�u�I�^I�Hc�H)�1�H��H9���H��[]A\A]A^A_�H��1��� AUATUSH��H��L�fH�L�oI9�u�1�H��I9�t'�3H���M6��I9�t�H���   []A\A]��     H��1�[]A\A]� AVAUI��ATUI��SH�GH��H��M��H9���   H9�wgM9l$s!H��! L��I+4$H�=�6  H�1�H���R H)�L��L��H��1ɉ�Hc��.��H��    H��H���.��[]A\A]A^�f.�     H�A! H��I+6H�=�5  H�1�H���R �t��� H�! H+7H�=�5  H�1�H���R I�F�F���f�AWAVI��AUATUSH��L+I��H;w��   L�5�! H���  L�-�7  I��I�� A�H��L��H)޺   H��M��H������A�iP1�I���Q&��H�A��H�M9|$XZu�H�Y�  H9�tH��� H��H�B�  []A\A]A^A_�H�0�  ��@ f.�     AUATUSHc߿   H��L�-! I�E �P(H��I�E �<�    H���P(H��H�EH�E 1�H���V*��H] H��H�]H��[]A\A]�AUATI��USH��L�-�! H�_H+�   I�E �P(H��I�E ���P(H��H�E I�4$H��H��Hc�H��H�UH�UHc���,��H��H��[]A\A]�fD  H��t3USH��H��H�-S! H�?H�E �P0H�E H��H�@0H��[]��fD  ��fD  ATULc�SH�H�GH)�H��L9�}7H�oH�! H����H��H)�H� H��Hc��P8H��H�J��H�SH�C[]A\�f.�     H�H;Gs�    H�0H��H9Gw��ÐUSHc�H��H��H��HH;Gr H��! H��H�=�3  H�1��R H��HE H� H��[]� f.�     ATUI��SHc�H��H��HH;Gr H�7! H��H�=�3  H�1��R H��HE L� []A\�@ f.�     H�H;GtH� � H��! SH��H�=�3  H�1��R H�[H� �@ f.�     H�GH9tH�@��f�H��! SH��H�=F3  H�1��R H�C[H�@��f�f.�     USH��H��H��H�WH�GH9�rH+H����������t�.0��H�SH�BH�CH�*H��[]��     SH�GH��H9tH��H�C[�f�     H�! H�=�2  H�1��R H�CH��H�C[�f�f.�     AWAVA��AUATI��USH��H��H��H�WH9���   I�E I�MH��I��H)�H)�I)�H��H��I��D�H��9���   Mc�I��Mc�I��H9�reL�E��I�UtA�F�H�T�H��H�k�H9�u�L��IE H��[]A\A]A^A_�D  H�9! H+7H�=/2  H�1�H���R I�U�Y���f�J�<;H)�H����,��I�U� Mc�L��I����.��L��I�UI] �X���f�     ATUI��SH��H�WH��H9�rJH9�v%H��! H��H+u H�=�1  H�1�H���R H�UH)�H��L���R,��L)�H)]L��[]A\� H�q! H+7H�=�1  H�1�H���R H�U� f.�     H�H�wH�
H�
! H�T$dH3%(   H����   H��[]A\A]A^A_� L�5�,  I�$H�����  H�?-  M�$j AVL�
    ("in ::ral::relation update" body line %d) invoked "break" outside of a loop       invoked "continue" outside of a loop    relation1 relation2 ?relation3 ...?     relationValue ?name-value-list ...?     tupleVarName relationValue ?-ascending | -descending? ?attr-list?script 
    ("::ral::relation foreach" body line %d)   relation attrName ?-ascending | -descending sort-attr-list? ?-within attr-list -start tag-value?        Ral_TagCmd: unknown option, "%d"        heading ?value-list1 value-list2 ...?   relationValue perRelation relationVarName ?attr1 type1 expr1 ... attrN typeN exprN?     attribute / type / expression arguments must be given in triples        relationValue ?oldname newname ... ?    oldname / newname arguments must be given in pairs      relationValue ?-ascending | -descending? rankAttr newAttr       relationValue ?attrName ? ?-ascending | -descending? sortAttrList? ?    relationValue attrName ?attrName2 ...?  relation arrayName keyAttr valueAttr    relation newattribute ?attr1 attr2 ...? attempt to group all attributes in the relation dictValue keyattr keytype valueattr valuetype   relationValue ?attrName | attr-var-list ...? relation1 relation2 relation1 compareop relation2 compareop unknown return code %d relValue expr relValue tupleVarName expr relationValue attribute relation attribute ordering tag option heading ?tuple1 tuple2 ...? relationValue ?attr ... ? relation keyAttr valueAttr during group operation list attribute type dividend divisor mediator subcommand ?arg? ... == notequal != propersubsetof < propersupersetof > <= >= -ascending -descending -within -start assign attributes body cardinality compose create degree divide dunion eliminate emptyof extend extract foreach fromdict fromlist heading intersect is isempty isnotempty issametype project rank rename restrict restrictwith semijoin semiminus summarize summarizeby table tag tclose times uinsert ungroup unwrap update  relationValue tupleVarName ?attr1 type1 expr1 ... attrN typeN exprN? -using (  <== [ ]   (Complete) ] ==>  , in relvar  tuple  for association  ] ==> [ for partition   is partitioned [ ])  |  for correlation  references multiple tuples references no tuple unknown constraint type, %d + * relvarConstraintCleanup: unknown constraint type, %d    Ral_ConstraintDelete: unknown constraint type, %d       is referenced by multiple tuples        is not referenced by any tuple  is referred to by multiple tuples       is not referred to by any tuple  does not form a complete correlation ?pattern? relvarName relationValue ?relvar1 relvar2 ...? transaction option arg ?arg ...? option type ?arg arg ...? trace option trace type info variable relvarName Unknown trace option, %d add transaction cmdPrefix remove transaction cmdPrefix info transaction Unknown trace type, %d relvar ?relationValue? relvarName tupleVarName expr constraint subcommand delete exists info names member path variable add remove suspend begin rollback association constraint correlation deleteone eval identifiers partition procedural restrictone trace unset updateone updateper     relvarName ?name-value-list ...?        relvarName tupleVarName idValueList script      relvarName tupleVarName expr script     relvarName ?relationValue ...?  Unknown transaction option, %d  
    ("in ::ral::relvar eval" body line %d)     add variable relvarName ops cmdPrefix   remove variable relvarName ops cmdPrefix        suspend variable relvarName script      suspending eval traces not implemented  relvarValue attr value ?attr2 value 2 ...?      attribute / value arguments must be given in pairs      name relvarName1 ?relvarName2 ...? script       name super super-attrs sub1 sub1-attrs ?sub2 sub2-attrs sub3 sub3-attrs ...?    relvarName relvarName ?attrName1 value1 attrName2 value2 ...?   relvarName heading id1 ?id2 id3 ...?    ?-complete? name corrRelvar corr-attr-list1 relvar1 attr-list1 spec1 corr-attr-list2 relvar2 attr-list2 spec2   delete | info | names ?args? | member <relvar> | path <name>    Unknown association command type, %d    name refrngRelvar refrngAttrList refToSpec refToRelvar refToAttrList refrngSpec @B��B���A��@A���A���B��:: traceOp namespace eval   { } ", failed procedural contraint, " -complete     bad operation list: must be one or more of delete, insert, update, set or unset relvar may only be modified using "::ral::relvar" command       relvarTraceProc: trace on non-write, flags = %#x
       returned "continue" from procedural contraint script for constraint, "  returned "break" from procedural contraint script for constraint, "     relvar creation not allowed during a transaction        during identifier construction operation        Ral_RelvarObjConstraintInfo: unknown constraint type, %d        end transaction with no beginning       
    ("in ::ral::relvar trace suspend variable" body line %d)   
    ("in ::ral::%s %s" body line %d)           multiplicity specification      Ral_TupleUpdate: attempt to update a shared tuple attr1 type1 value1 ... tuple1 tuple2 tupleValue tupleValue tupleAttribute tupleValue attr ?...? tupleValue ?attr? ... heading name-value-list get     tupleValue newAttr ?attr attr2 ...?     failed to copy attribute, "%s"  tupleValue ?attr1 value1 attr2 value2?  for attribute name / attribute value arguments  tupleValue ?oldname newname ... ?       for oldname / newname arguments tupleValue ?name type value ... ?       tupleValue ?attrName | attr-var-pair ... ?      Ral_TupleHeadingFetch: out of bounds access at offset "%d"      Ral_TupleHeadingStore: out of bounds access at offset "%d"      Ral_TupleHeadingStore: cannot find attribute name, "%s", in hash table  Ral_TupleHeadingStore: inconsistent index, expected %u, got %u  Ral_TupleHeadingPushBack: overflow      Ral_TupleHeadingAppend: out of bounds access at source offset "%d"      Ral_TupleHeadingAppend: overflow of destination RAL OK UNKNOWN_ATTR DUPLICATE_ATTR HEADING_ERR FORMAT_ERR BAD_VALUE BAD_TYPE BAD_KEYWORD WRONG_NUM_ATTRS BAD_PAIRS_LIST DUPLICATE_OPTION NO_IDENTIFIER IDENTIFIER_FORMAT IDENTIFIER_SUBSET IDENTITY_CONSTRAINT DUP_ATTR_IN_ID DUPLICATE_TUPLE HEADING_NOT_EQUAL DEGREE_ONE DEGREE_TWO CARDINALITY_ONE BAD_TRIPLE_LIST NOT_AN_IDENTIFIER NOT_A_RELATION NOT_A_TUPLE NOT_A_PROJECTION NOT_DISJOINT NOT_UNION TOO_MANY_ATTRS TYPE_MISMATCH SINGLE_IDENTIFIER SINGLE_ATTRIBUTE WITHIN_NOT_SUBSET BAD_RANK_TYPE DUP_NAME UNKNOWN_NAME REFATTR_MISMATCH DUP_CONSTRAINT UNKNOWN_CONSTRAINT CONSTRAINTS_PRESENT BAD_MULT BAD_TRANS_OP SUPER_NAME INCOMPLETE_SPEC ONGOING_CMD ONGOING_MODIFICATION INTERNAL_ERROR no error unknown attribute name duplicate attribute name bad heading format bad value format bad value type for value unknown data type bad type keyword bad list of pairs duplicate command option duplicate tuple headings not equal bad list of triples too many attributes specified duplicate relvar name unknown relvar name duplicate constraint name unknown constraint name serious internal error NONE destroy unknown command relvar setfromany updatefromobj       wrong number of attributes specified    relations of non-zero degree must have at least one identifier  identifiers must have at least one attribute    identifiers must not be subsets of other identifiers    tuple has duplicate values for an identifier    duplicate attribute name in identifier attribute set    relation must have degree of one        relation must have degree of two        relation must have cardinality of one   attributes do not constitute an identifier      attribute must be of a Relation type    attribute must be of a Tuple type       relation is not a projection of the summarized relation divisor heading must be disjoint from the dividend heading      mediator heading must be a union of the dividend and divisor headings   attributes must have the same type      only a single identifier may be specified       identifier must have only a single attribute    "-within" option attributes are not the subset of any identifier        attribute is not a valid type for rank operation        mismatch between referential attributes relvar has constraints in place referred to identifiers can not have non-singular multiplicities        operation is not allowed during "eval" command  a super set relvar may not be named as one of its own sub sets  correlation spec is not available for a "-complete" correlation recursively invoking a relvar command outside of a transaction  recursive attempt to modify a relvar already being changed      Ral_IntVectorFetch: out of bounds access at offset "%d" Ral_IntVectorStore: out of bounds access at offset "%d" Ral_IntVectorFront: access to empty vector      Ral_IntVectorPopBack: access to empty vector    Ral_IntVectorInsert: out of bounds access at offset "%d"        Ral_IntVectorErase: out of bounds access at offset "%d" Ral_IntVectorOffsetOf: out of bounds access     Ral_IntVectorCopy: out of bounds access at source offset "%d"   Ral_IntVectorCopy: out of bounds access at dest offset "%d"     Ral_PtrVectorFetch: out of bounds access at offset "%d" Ral_PtrVectorStore: out of bounds access at offset "%d" Ral_PtrVectorFront: access to empty vector      Ral_PtrVectorPopBack: access to empty vector    Ral_PtrVectorInsert: out of bounds access at offset "%d"        Ral_PtrVectorErase: out of bounds access at offset "%d" Ral_PtrVectorCopy: out of bounds access at source offset "%d"   Ral_PtrVectorCopy: out of bounds access at dest offset "%d" %d: %d
 %d: %X
     interpreter uses an incompatible stubs mechanism        Tcl missing stub table pointer epoch number mismatch requires a later revision tcl::tommath , actual version   (requested version  Error loading  ):    ;l  �  `����  �����   ����  � ��   ��0  ��H   ��`  ����   ���  ����  ���  P��(  ���H   ��x  ����  @���  ����  ��(  ���X  @���  ����   ���  ���  p	��H  �	��h  @
���  �
���  ���    ��(  ���p  �
8D0A(B BBBE        �   ����              �   ����              �   ����           ,   �   ����{    A�A�L0}
AAC      ,   $  ���|    A�A�L0Y
AAG     ,   T  X����    A�A�L0`
AAH        �  ����     D[    �  ����i    D _
A        �   ���C    D y
A      ,   �  P���{    A�A�L0}
AAC      ,     ����|    A�A�L0Y
AAG     ,   <  �����    A�A�L0`
AAH        l  `���A    D w
A      ,   �  ����~    A�A�L@
AAA      ,   �  ����|    A�A�L@]
AAC     4   �  0����    B�A�D �I0c
 AABI        $  ����A    D w
A      ,   D  �����    A�A�L@
AAA      ,   t  8����    A�A�L@v
AAJ     4   �  �����    B�A�D �I0c
 AABI        �   ���A    D w
A      ,   �  P���~    A�A�L@
AAA      ,   ,  �����    A�A�L@x
AAH     4   \  ����    B�A�D �I0c
 AABI     $   �  ����d    A�D@X
AA     D   �  �����    B�B�E �A(�D0�Hp~
0A(A BBBD      D     H����    B�B�E �A(�D0�Lpn
0A(A BBBH     ,   L  �����    A�A�K@x
AAA        |   ���H    D0~
A         �  P���C    D y
A         �  ����              �  x���i    D _
A        �  ����                ����           \   $  �����    B�B�E �B(�H0�A8�L@n
8C0A(B BBBFT8D0A(B BBB ,   �  ����8    B�A�D �cGB          �  ����           ,   �  ����A    B�A�D �sDB          �  ���           4     ���z    B�A�D �D0e
 AABA     4   L  P����    B�A�D �D0Z
 AABG     D   �  �����    B�B�E �A(�D0�D@�
0A(A BBBK        �  0���              �  (���           ,   �   ���d    B�A�D �U
ABA   ,   ,  `���S    B�A�D �HAB      ,   \  ����S    B�A�D �HAB      $   �  ����S    A�A�G FAA   �  ����d    d\    �  P���d    d\ $   �  ����b    Dj
BL
LV     D   	  ����^    F�A�J [
A�A�BD
C�A�HDG�A�   <   T	  ����   B�B�D �A(�G@e
(A ABBE     4   �	  x���y    A�G f
CGD
GM`D       4   �	  �����    B�A�A ��
ABG\AB  D   
  X ���   B�B�E �A(�D0�J`	
0A(A BBBG    L   L
  ����   B�B�E �B(�D0�A8�JP[
8A0A(B BBBA     4   �
  P���    B�A�D �D0v
 AABK         �
  ��&    A�d          �
  (��           ,     0���    Do
EM
K]
CM       T   <  ����    B�A�D �G0^
 GABJZ
 CABH_
 AABE       L   �  8���    A�A�G \
DAN\
CAH^
HDFHDA     L   �  ����    A�A�M0W
DAE\
CAHl
IABHDA     ,   4  ����    M�A�D �K�A�B�   ,   d  X���    M�A�D �K�A�B�   4   �  ����    B�A�D �DP}
 DABA        �   ��              �  ��           <   �  ��n    B�B�L �F(�A0�O(A BBB      $   <
CAID
FAED
FAE     ,     0	��J    J�D�C �uAB       4   <  P	��R    B�B�A �A(�J0x(D ABB 4   t  x	��T    B�B�A �A(�Q0s(D ABB    �  �	��N              �  �	��)           4   �  �	��T    B�B�A �A(�Q0s(D ABB      
��D           D   ,  P
���    B�B�E �B(�A0�A8�O@\8D0A(B BBB   t  �
��'    H�^          �  �
��:    H�q          �  �
��$    A�Z          �  �
��!    A�W          �  �
��                �
��              $  �
��	              <  �
��              T  �
��-    A�c       $   t  �
��Q    A�A�L }DA L   �   ���    B�B�E �B(�A0�A8�HP�
8A0A(B BBBA     <   �  ���y    B�B�B �A(�E0�e(A BBB      <   ,  ����    B�B�E �A(�D0��(A BBB      L   l  0���    B�B�E �B(�D0�A8�V��
8A0A(B BBBA    ,   �  ���g    B�H�A �XAB      $   �  ���h    A�A�G [AA,     (
0A(A BBBD     D   �  ���    B�B�B �B(�A0�A8�G@|8D0A(B BBB,   �  h��O    B�A�D �DAB      L     ���8   B�B�E �B(�D0�A8�J`�
8A0A(B BBBF     L   T  x��4   B�B�B �B(�D0�A8�G`

8A0A(B BBBA    <   �  h��y    B�B�E �A(�D0�`(D BBB      L   �  ����   B�B�E �B(�A0�A8�J��
8A0A(B BBBA   L   4  H���   B�B�B �B(�A0�A8�J�
8A0A(B BBBA   L   �  ���4   B�B�E �B(�D0�A8�S`�
8A0A(B BBBA     L   �  ���)   B�B�E �B(�D0�A8�W��
8A0A(B BBBA   L   $  x��p   B�B�B �B(�D0�A8�Dp�
8A0A(B BBBD    <   t  ���^    B�B�D �A(�P0v
(D ABBA      4   �  ���R    B�B�D �A(�P0r(A ABB L   �  ����    B�B�E �D(�A0��
(D BBBGN(D BBB    L   <  ���-   B�B�B �B(�D0�A8�U��
8A0A(B BBBA    L   �  `���   B�B�E �B(�D0�A8�D��
8A0A(B BBBA   L   �   ��   B�B�E �B(�D0�A8�D��
8A0A(B BBBA   4   ,  ���s    B�B�A �A(�J0Y(D ABB<   d   ���    B�B�B �A(�D0�|(D BBB      L   �  x ���   B�B�B �B(�A0�A8�G�
8A0A(B BBBA   L   �  �#��   B�B�E �B(�D0�A8�Gp�
8A0A(B BBBA     4   D  �$��g    E�A�A �v
ABHE
ABHD   |  �$���    B�B�E �B(�A0�A8�J@v8D0A(B BBBL   �  (%���    B�B�E �B(�A0�A8�J@i
8D0A(B BBBC     L     x%���   B�B�E �B(�D0�A8�D�4
8A0A(B BBBF   4   d  �'��m    B�B�D �A(�G0S(D ABBL   �  �'��W   B�B�B �B(�D0�A8�J@�
8D0A(B BBBH     D   �   )���    B�B�B �B(�D0�A8�L@e8C0A(B BBB$   4  H)��C    A�A�J tAA    \  p)��    DT $   t  x)��C    A�A�J tAA $   �  �)��C    A�A�J tAA $   �  �)��C    A�A�J tAA $   �  �)��C    A�A�J tAA L     *���    B�B�E �A(�D0�|
(C BBBES
(C BBBD  L   d  X*��C   B�B�E �B(�A0�A8�G`
8A0D(B BBBA    4   �  X+��9    B�B�D �A(�J0](C ABB L   �  `+��@   B�B�E �B(�A0�A8�D�
8A0A(B BBBA   <   <  P,��k    B�E�B �A(�D0�U(A BBB      <   |  �,���    B�B�A �A(�G��
(D ABBA    <   �  -���    B�B�A �A(�G��
(D ABBA    l   �  �-���   B�B�B �B(�A0�A8�J�s
8A0A(B BBBGp�R�T�A�S�H�[�A�  l   l  �/���   B�B�B �B(�A0�A8�J�s
8A0A(B BBBGp�R�T�A�S�H�[�A�  l   �  2���   B�B�B �B(�A0�A8�J�s
8A0A(B BBBGp�X�N�A���H�[�A�  l   L  `4���   B�B�B �B(�A0�A8�J�s
8A0A(B BBBGp�R�T�A���H�[�A�  L   �  �6��B   B�B�B �B(�A0�A8�GPk
8D0A(B BBBG      \     �7���    B�B�A �A(�G0j
(C ABBAi
(H ABBJn(C ABB     \   l  8���    B�B�A �A(�G0j
(C ABBAi
(H ABBJr(C ABB     \   �  p8���    B�B�A �A(�G0j
(C ABBAi
(H ABBJx(C ABB     \   ,   �8���    B�B�A �A(�G0j
(C ABBAi
(H ABBJx(C ABB     L   �   P9��A   B�B�B �B(�D0�A8�G�y
8A0A(B BBBI    \   �   P=���    B�B�A �A(�G0j
(C ABBAi
(H ABBJm(C ABB     L   <!  �=���    B�B�B �B(�A0�A8�J@j
8C0A(B BBBF      L   �!  @>��f   B�B�B �A(�D0�G@P
0A(A BBBFtHdPPHA@ L   �!  `?��=   B�B�B �B(�D0�A8�G�U
8A0A(B BBBE    L   ,"  PC���   B�B�B �B(�D0�A8�G�U
8A0A(B BBBE    L   |"  �E��}   B�B�B �B(�A0�A8�JpR
8A0A(B BBBH     L   �"  �G��*   B�B�B �B(�D0�A8�J�Y
8A0A(B BBBF    L   #  �H���   B�B�E �B(�A0�A8�J�j
8A0A(B BBBE    L   l#  `J���   B�B�E �B(�A0�A8�J�j
8A0A(B BBBE    L   �#   L��*   B�B�B �B(�D0�A8�J�Y
8A0A(B BBBF    L   $  �L��d   B�B�E �B(�A0�A8�J�p
8A0A(B BBBG    L   \$   N��h   B�B�E �B(�A0�A8�J�k
8A0A(B BBBD    \   �$   O���    B�B�A �A(�G0j
(C ABBAf
(C ABBBl(H ABB    L   %  �O���   B�B�B �B(�D0�A8�GPx
8D0A(B BBBG      \   \%  �P��   E�B�B �A(�D0�f
(A BBBE�
(A BBBGH(A BBB  \   �%  �Q��[   B�B�B �B(�D0�A8�G�i�`�P�A�a
8A0A(B BBBG    \   &  �U��/   B�B�B �B(�D0�A8�J�z
8A0A(B BBBE��E�^�A�   L   |&  pY��B   B�B�E �B(�A0�A8�J��
8A0A(B BBBA    L   �&  pZ���   B�B�E �B(�A0�A8�J�
8A0A(B BBBH   L   '  �[��q   B�B�E �B(�A0�A8�J�l
8A0A(B BBBC    L   l'   b���   B�B�E �B(�A0�A8�J�j
8A0A(B BBBE    L   �'  �g���   B�B�B �B(�A0�A8�J�s
8A0A(B BBBG    L   (  @l���   B�B�E �B(�D0�A8�G�h
8A0A(B BBBG    \   \(  �m���   B�B�E �B(�A0�A8�D�]
8A0A(B BBBHX�\�U�A�  L   �(  q���    B�B�E �B(�D0�A8�D@y
8C0A(B BBBG      L   )  �q��   B�B�E �B(�D0�A8�D@y
8C0A(B BBBG      L   \)  `r��0   B�B�B �B(�A0�A8�M�Y
8A0A(B BBBF    \   �)  @s���   B�B�E �B(�A0�A8�G`r
8A0A(B BBBH�h`pPhA`       L   *  �u��*   B�B�B �B(�D0�A8�G`V
8D0A(B BBBI    L   \*  �w���   B�B�B �B(�D0�A8�GPj
8C0A(B BBBF      L   �*   y��x   B�B�E �B(�A0�A8�G`j
8C0A(B BBBF      L   �*  Pz���   B�B�E �B(�A0�A8�J�j
8A0A(B BBBE    \   L+  �{���    E�B�B �A(�D0�f
(A BBBEm
(A BBBDA(A BBB  L   �+  P|��,   B�B�B �B(�A0�A8�GpF
8D0A(B BBBD     L   �+  0~���   B�B�B �B(�D0�A8�J�Y
8A0A(B BBBF    L   L,  ���,   B�B�E �B(�F0�A8�O��
8A0A(B BBBF      �,  ����8    Ie L   �,  ����^   B�B�B �B(�A0�A8�M�[
8A0A(B BBBD    \   -  Ȃ���    E�B�B �A(�D0�f
(A BBBEm
(A BBBDE(A BBB  L   d-  8���E   B�B�B �B(�D0�A8�J�g
8A0A(B BBBH    <   �-  8����    B�A�D �G0d8a@P8F0X
 AABK     �-  ؄��    A�\          .  ؄��#    A�a          4.  ���    A�]          T.  ���4    H�k       L   t.  ����    B�B�E �B(�D0�A8�J@w
8C0A(B BBBC      D   �.  X���   B�B�E �A(�D0�JP]
0A(A BBBC     4   /   ����    B�A�D �J�T
 AABG    L   D/  ؆���    B�B�E �B(�D0�A8�PPd
8C0A(B BBBH      L   �/  H����    B�B�E �B(�D0�A8�PPh
8D0A(B BBBK      L   �/  ȇ���   B�B�E �B(�A0�A8�D�J
8A0A(B BBBK    ,   40  ���D    A�A�G f
AAG      L   d0  (����   B�B�B �B(�D0�A8�L@p8A0A(B BBB       $   �0  x���;    A�A�G lDA <   �0  ����   B�B�A �A(�G0�
(A ABBJ     ,   1  p����    B�A�A ��
ABJ   <   L1  0���2   B�B�D �A(�DP
(A ABBA    L   �1  0����    B�B�B �B(�A0�A8�DP�
8A0A(B BBBA     L   �1  �����    B�E�B �B(�E0�A8�Kp�
8A0A(B BBBA     D   ,2   ���    B�B�E �B(�D0�A8�VP�8A0A(B BBBL   t2  �����   B�B�B �B(�D0�A8�O`j8E0A(B BBB       L   �2  H���a   B�B�B �B(�D0�A8�O`28E0A(B BBB       D   3  h����    B�B�E �B(�D0�A8�RPK8G0A(B BBBL   \3  ����   B�B�E �B(�E0�A8�OP�
8A0A(B BBBA        �3  p���-    D�d          �3  ����$    GT
E      4   �3  �����    B�B�D �A(�L0�(D ABB   $4  ���              <4  ���    A�S          \4  ���)    H�`       $   |4   ���#    A�A�I NAA 4   �4  (����    B�A�D �E
ABAmAB     �4  ����!    A�W          �4  ����,    JZ
D         5  ����O    A�M      ,   <5  Д��i    A�A�N0I
AAE     $   l5  ���E    A�A�D yDA ,   �5  8���i    A�A�N0I
AAE        �5  x���O    A�M      ,   �5  ����i    A�A�N0I
AAE     $   6  ���Q    A�A�D EDA,   <6   ���i    A�A�N0I
AAE     t   l6  `���   B�B�B �A(�A0�r
(A BBBO
(A BBBJZ
(A BBBO�(A BBB         �6  ���A    G�y       <  7  8���   B�B�B �B(�A0�A8�D�w
8D0A(B BBBF��M�P�A���K�V�B���G�Z�A�P�G�[�B�P�M�Q�B�P�J�U�A�P�M�Q�B�P�J�U�B�P�M�P�B�P�M�P�A�P�M�V�B�U�M�P�A�P�J�V�B�P�M�P�A�P�J�V�B�4   D8  ����    B�B�A �A(�J0q(C ABB$   |8  p���{    A�G0d
AC     L   �8  Ȣ���    B�B�B �B(�A0�A8�D��
8A0A(B BBBA    \   �8  x����    B�B�E �B(�D0�A8�JP�
8A0A(B BBBID8F0A(B BBB4   T9  ����    F�A�A �x
ABEvAB   <   �9  `���}    B�B�E �A(�D0�H
(J BBBJ   \   �9  �����    B�B�E �A(�D0�I
(A BBBBS
(A BBBFp(A BBB  <   ,:   ����    B�B�A �A(�DP�
(A ABBA     4   l:  `���{    B�B�D �A(�G0a(D ABB$   �:  ����1    A�A�G YGA    �:  ����    A�Y       ,   �:  ����:    I�D�A �hAB       L   ;  Х���    B�B�E �B(�D0�A8�GP�
8D0A(B BBBG     L   l;  @����   B�B�B �B(�A0�A8�Dp�
8A0A(B BBBC     L   �;  �����   B�B�B �B(�D0�A8�J��
8A0A(B BBBH    L   <  ���W   B�B�B �B(�D0�A8�J��
8A0A(B BBBD    4   \<  ����    E�A�A �g
ABGUCB  <   �<  X����    E�A�A �b
FBGJ
CBI       L   �<  ����   B�B�B �B(�A0�A8�Gpq
8A0A(B BBBD      L   $=  x���	   B�B�B �B(�A0�A8�G�h
8A0A(B BBBE    \   t=  8����   B�B�E �B(�A0�A8�J�[
8A0A(B BBBD��I�b�A�  \   �=  �����   B�B�E �B(�A0�A8�J�Y
8A0A(B BBBF��G�[�C�  <   4>  ����t    B�B�D �A(�D0}
(A ABBA      L   t>  �����   B�B�B �B(�D0�A8�J��
8A0A(B BBBE    L   �>  �����   B�B�B �B(�D0�A8�J��
8A0A(B BBBE    D   ?  X����   B�B�A �A(�G@P
(A ABBEKH`PRHA@ L   \?  ����?   E�B�E �D(�A0��
(C BBBHr
(C BBBE d   �?  �����   B�B�B �B(�D0�A8�GPdXb`PXAPa
8A0A(B BBBJDX``PXAP      L   @  ����!   B�B�B �B(�D0�A8�J�^
8A0A(B BBBA    L   d@  �����   B�B�E �B(�A0�A8�J��
8A0A(B BBBG       �@  8���J    de $   �@  p����    A�G0n
FDsAL   �@  ����h   B�B�E �B(�A0�A8�J�W
8A0A(B BBBH    L   DA  �����   B�B�B �B(�D0�A8�J��
8A0A(B BBBE    L   �A  �����   B�B�B �B(�A0�A8�M��
8A0A(B BBBC    L   �A  ����|   B�B�E �B(�A0�A8�G�R
8A0A(B BBBH    \   4B  ����    B�B�B �B(�A0�A8�M�n�K�O�A�Z
8A0A(B BBBD       �B  ����D    Le T   �B  �����   B�B�B �B(�D0�A8�G`hh_pUhA`�
8A0A(B BBBD    C  H���<    Ie D   C  p����    B�B�D �A(�G@dHdPPHF@X
(A ABBD  4   dC  ����    B�B�D �A(�M0q(D ABBT   �C  ����5   B�B�B �B(�A0�A8�Gp�xE�_xAp_
8A0A(B BBBAD   �C  h����    B�B�E �B(�D0�A8�OP�8A0A(B BBBL   <D  �����   B�B�E �B(�D0�A8�G��
8A0A(B BBBB    4   �D  0���x    B�A�D �q
ABEtAB   D   �D  x����    B�B�B �A(�A0�J��
0A(A BBBH    L   E   ���N   B�B�E �B(�D0�A8�G�s
8A0A(B BBBD    L   \E   ����    B�B�E �B(�D0�A8�IP�
8D0A(B BBBA     L   �E  �����   B�B�E �B(�D0�A8�P�K
8A0A(B BBBK   L   �E  P����    B�B�E �B(�A0�A8�D@M
8D0A(B BBBE      D   LF  ����b   B�B�E �A(�D0�M��
0A(A BBBE    L   �F  ���v   B�B�E �B(�A0�A8�Gp�
8D0A(B BBBA     l   �F  8���K   B�B�B �B(�D0�A8�P��
8A0A(B BBBG[�`�P�A���`�P�A�  L   TG  ���T   B�B�E �B(�D0�A8�M��
8A0A(B BBBD    |   �G  (���p   B�B�B �B(�D0�A8�P��
8A0A(B BBBH��`�P�A�S�c�P�A���c�P�A�L   $H   ��;   B�B�E �B(�D0�A8�P�n
8A0A(B BBBH   L   tH  ��0   B�B�B �B(�D0�A8�Ip�
8A0A(B BBBI     d   �H  ����    B�B�B �B(�D0�A8�J@�
8F0A(B BBBGT
8C0A(B BBBA     D   ,I  ���b   B�B�E �A(�D0�M��
0A(A BBBE    4   tI  ���T    B�A�D �c
CBA[FB   |   �I  ����   B�B�B �B(�D0�A8�MP�
8A0A(B BBBEd
8A0A(B BBBJ�
8F0A(B BBBK   L   ,J  ���    B�B�D �A(�J0M
(C ABBH^(F ABB      L   |J  `���   B�B�B �B(�D0�A8�DPD
8C0A(B BBBG     4   �J  �
DBKIAB   ,   K  �
AAH      4   4K  �
 AABA     L   lK  0��   B�B�B �B(�A0�A8�N��
8A0A(B BBBE    <   �K   ��|    B�B�I �D(�A0�j
(A BBBE    $   �K  @��F    A�A�O pCA L   $L  h���    B�B�E �B(�D0�A8�D@�
8C0A(B BBBA     4   tL  ����    B�B�D �A(�K@�(A ABB   �L  `��J    Y�h�      $   �L  ���C    Q�H�G�a�     L   �L  ���   B�B�B �B(�D0�A8�JP�
8D0A(B BBBD     $   DM  ���C    Q�H�G�a�     L   lM  ���%   B�B�E �B(�D0�A8�J`~
8C0A(B BBBD      d   �M  ����    B�B�E �B(�D0�A8�OPn
8D0A(B BBBFQ
8P0A(B BBBN      L   $N  (��k   B�B�B �B(�A0�A8�G@X
8D0A(B BBBJ         tN  H��(    H�^�      \   �N  X��P   B�B�B �A(�D0��
(D BBBHb
(D BBBDe(D BBB  L   �N  H���   B�B�B �B(�A0�A8�D�
8A0A(B BBBF   ,   DO  ���Q    I�D�A �AB       $   tO  ���R    A�A�M @AA<   �O  ���    K�B�D �A(�D0r(A� A�B�B�       �O  p��           \   �O  x���    B�B�B �B(�A0�A8�D@X
8A0A(B BBBHD8F0A(B BBB4   TP  ���N    F�A�J U
A�A�HDG�A� 4   �P  ���L    B�B�D �A(�D0u(D ABB <   �P  ���\    B�B�A �A(�D0C
(D ABBA     L   Q  ���v    B�B�E �A(�D0�O
(A BBBDA(F BBB    d   TQ  (��   B�B�E �B(�D0�A8�JP�
8A0A(B BBBET
8C0A(B BBBH     ,   �Q  ���4    A�A�G b
AAC      4   �Q  ���t    B�B�A �A(�D0`(D ABB$   $R  8��:    A�A�J hDA D   LR  P��y    B�B�E �B(�A0�A8�GPZ8A0A(B BBB,   �R  ���\    B�A�D �{
ABK    $   �R  ���;    A�A�G lDA $   �R  ���R    A�A�M @AAL   S  ���    B�B�B �B(�D0�A8�G@�
8A0A(B BBBA     4   dS  h��:    B�B�D �A(�J0^(C ABB L   �S  p���    B�B�B �B(�A0�A8�H`�
8A0A(B BBBA     <   �S  ���l    B�E�B �A(�D0�V(A BBB      <   ,T   ���    B�B�A �A(�G��
(D ABBA    <   lT  ����    B�B�A �A(�G��
(D ABBA    L   �T  P��   B�B�B �B(�D0�A8�J��
8A0A(B BBBC    L   �T    ���    B�B�B �B(�A0�A8�G@j
8C0A(B BBBI      L   LU  � ��D   B�B�B �B(�A0�A8�GPk
8D0A(B BBBG      \   �U  �!���    B�B�A �A(�G0j
(C ABBAi
(H ABBJs(C ABB     L   �U  "��U   B�B�B �B(�A0�A8�GPq
8A0A(B BBBD      L   LV   #���   B�B�B �B(�A0�A8�JPj
8C0A(B BBBF      L   �V  �$���   B�B�B �B(�D0�A8�G`,
8A0A(B BBBF    L   �V  `&��U   B�B�B �B(�D0�A8�G�z
8A0A(B BBBH    L   <W  p)���   B�B�B �B(�A0�A8�G�,
8A0A(B BBBI   L   �W  �*��   B�B�E �B(�A0�A8�DpO
8A0A(B BBBF    \   �W  �,���    E�B�B �A(�D0�f
(A BBBEm
(A BBBDI(A BBB  L   <X  0-���   B�B�E �B(�A0�A8�D`E
8D0A(B BBBE     \   �X  p.���    E�B�B �A(�D0�f
(A BBBEm
(A BBBDB(A BBB  L   �X  �.��0   B�B�E �B(�D0�A8�G�}
8A0A(B BBBB    L   <Y  �0���   B�B�E �B(�A0�A8�D`�
8A0A(B BBBF     D   �Y  02��   B�B�B �A(�D0�J�Y
0A(A BBBJ    D   �Y  3���    B�B�B �A(�D0�J�a
0A(A BBBB    <   Z  �3���    B�A�D �G0d8a@P8F0X
 AABK  4   \Z  @4��h    B�B�A �A(�O0I(D ABB$   �Z  x4��P    A�A�G CAA   �Z  �4��              �Z  �4��-    A�k       L   �Z  �4��J   B�B�B �B(�D0�A8�J`�
8A0A(B BBBF     4   D[  �5���    B�A�D �G0�
 AABD     4   |[  @6��d    B�B�D �A(�G0J(D ABBL   �[  x6��'   B�B�D �A(�M0t
(C ABBFD
(F ABBI   $   \  X7��:    A�A�G kDA $   ,\  p7��<    A�A�G mDA    T\  �7��&    A�X
GE L   t\  �7���    B�B�A �A(�D0g
(C ABBGL(F ABB         �\  �7��%    GT
A      ,   �\  �7���    B�A�D ��AB      D   ]  X8���    B�B�E �B(�D0�A8�D@�8D0A(B BBBL   \]  �8��a   B�B�E �B(�D0�A8�Jp�
8A0A(B BBBE     L   �]  �9��Z   B�B�E �B(�D0�A8�J��
8A0A(B BBBG   L   �]   <���    B�B�B �B(�D0�A8�DP�
8D0A(B BBBA     L   L^  p<���    B�B�B �B(�A0�A8�DP�
8D0A(B BBBA     4   �^  �<��I    B�B�A �A(�D0x(A ABB <   �^  �<���    B�L�B �D(�A0�u(D BBB      L   _  H=���    B�B�E �B(�A0�A8�GPo
8A0A(B BBBA     4   d_  �=���    B�A�D �DP�
 DABA        �_   >��    A�\          �_   >��!    A�_          �_  >��    A�]          �_  >��7    H�n       <   `  0>��@   B�B�D �A(�JP�
(A ABBI     D   \`  0?���    B�B�E �B(�D0�A8�HP8D0A(B BBB,   �`  �?��t    A�A�P@Z
AAA     L   �`  �?��X   B�B�E �B(�D0�A8�Pp(
8A0A(B BBBA    L   $a  �@���   B�B�B �B(�A0�A8�MpO
8A0A(B BBBH    L   ta  xB��   B�B�E �B(�D0�A8�P`�
8A0A(B BBBJ     L   �a  HC���   B�B�E �B(�D0�A8�Op�
8A0A(B BBBK     T   b  �D���    B�B�E �A(�D0�J@R
0A(A BBBFL0F(A BBB   4   lb   E���    B�A�D �J�T
 AABG    L   �b  �E��   B�B�E �B(�A0�A8�I�
8A0A(B BBBI      �b  �G��           ,   c  �G��J    G�A�K �_�A�B�    ,   <c  �G��B    F�A�L TF�A�       l   lc  �G��   B�B�B �B(�A0�A8�G@m
8A0A(B BBBHnHEPpHA@P8A0A(B BBB    <   �c  �H��p    B�B�D �A(�M�N
(A ABBA    <   d  �H��p    B�B�D �A(�M�N
(A ABBA       \d  �H��              td  �H��              �d  �H��              �d  �H��           4   �d  �H��O    B�B�D �A(�I0s(D ABB 4   �d  �H��j    B�B�D �A(�D0S(D ABB,   ,e   I��:    F�A�G `A�A�       ,   \e  0I��V    B�A�D �KAB         �e  `I��           <   �e  hI��Y    B�B�E �K(�A0�|(A BBB          �e  �I��            $   �e  �I��B    A�A�K rAA ,   $f  �I��B    B�A�D �wAB          Tf  �I��1    X�V�         tf  �I��3    X�W�      $   �f  J��F    A�A�I xAA    �f  @J��D    A�U
Jc L   �f  pJ��   B�B�E �B(�D0�A8�I@w
8D0A(B BBBC     ,   ,g  0K���    B�A�D �R
ABD   4   \g  �K���    B�A�D �K0O
 AABK     ,   �g  �K��@    B�A�A �xAB          �g  L��"           4   �g   L���    B�B�D �A(�F0�(D ABB   h  �L��5           4   ,h  �L��@    A�A�I Q
CAHNHA        dh  �L��C           $   |h  �L��E    A�A�J oHA D   �h  M��n    B�B�B �B(�D0�A8�G@L8D0A(B BBBL   �h  @M��t    B�B�B �B(�D0�A8�G@B
8D0A(B BBBE        <i  pM��P    tX L   Ti  �M��}    B�B�B �B(�A0�A8�D@]
8A0A(B BBBA     L   �i  �M��]    B�B�A �A(�G0o
(F ABBID(C ABB       <   �i  �M���    B�B�E �A(�D0�p
(A BBBK   T   4j  �N���    B�B�E �B(�A0�A8�D@vH]PYHA@Y
8H0A(B BBBA  4   �j  �N��`    B�B�A �A(�L0G(A ABB4   �j  O��j    B�B�D �A(�D0S(D ABB,   �j  PO��:    F�A�G `A�A�       ,   ,k  `O��V    B�A�D �KAB         \k  �O��           $   tk  �O��C    A�A�K sAA ,   �k  �O��B    B�A�D �wAB          �k  �O��2    X�V�         �k   P��4    X�W�      $   l   P��H    A�A�J yAA    4l  HP��D    A�U
Jc L   Tl  xP��   B�B�E �B(�D0�A8�J@~
8A0A(B BBBF     ,   �l  8Q���    B�A�D �R
ABD      �l  �Q��"              �l  �Q��5           4   m  �Q��A    A�A�J Q
CAGOHA        <m  �Q��P    tX L   Tm  (R��~    B�B�B �B(�A0�A8�D@^
8A0A(B BBBA     <   �m  XR���    B�B�E �A(�D0�p
(A BBBK   T   �m  �R���    B�B�E �B(�A0�A8�D@vH]PYHA@Y
8H0A(B BBBA  <   <n  `S���   B�B�A �A(�G@#
(A ABBJ    \   |n   U��.   B�B�E �B(�C0�A8�PP�
8A0A(B BBBDaXB`KhApgP                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      ��      `�              �u     `�      Щ      ��      P�      v     p�      �      p�      �      
     �~     �;     �~     0L     �~     �#     �~     pZ     �~     pV     �     @T     �~     pS     �     �     �~     �Q     �~          �~     �          P          0     E     0�      �     0I     N      H          F          pB     $     �@     +     �     4     �     A     ��      J     ��      T     �5     ^     P/     j     �-     p      (     t     �"     {      !     w�           �     0     �           �~     0     �           �     �
       .                            0#            @                           �s             �C             00      	              ���o    PC      ���o           ���o    �@      ���o    �                                                                                                                                                                                                      �-#                     F�      V�      f�      v�      ��      ��      ��      ��      Ƌ      ֋      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      ƌ      ֌      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      ƍ      ֍      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ǝ      ֎      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ə      ֏      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ɛ      ֐      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ƒ      ֑      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      ƒ      ֒      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ɠ      ֓      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ɣ      ֔      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      ƕ      ֕      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ɩ      ֖      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ɨ      ֗      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      Ƙ      ֘      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��      ƙ      ֙      �      ��      �      �      &�      6�      F�      V�      f�      v�      ��      ��      ��      ��              �7#                             q     *r     q     #r     $q     �q                     Tuple   Relation                      @�       �      ��      p�                                    @�       �      ��      ��                                    `�      P�      P�      ��                                      `_     0_     _     pa                             M�     T�     [�     `�     f�     m�                     r�     ��                     {�     �     [�     ��                                     ��     ��     ��             q             ��            �}            ��                                                   0J      J     �I     �S     GCC: (Ubuntu 6.3.0-12ubuntu2) 6.3.0 20170406                                  �                    �                    �                    �'                    �@                    PC                    �C                    �s                   	 �                   
 0�                    ��                    К                   
    �      �           ��      �       #    @�      A       .    ��      ~       ;    �      �       J    ��      �       V    `�      d       `    Щ      �       l    ��      �       z    P�      �       �    �      H       �    0�      C       �    ��             �    ��      i       �     �             �    �             �     �      �       �    �#     �      �    ��      8       �    �             �     �      A       �    P�                 `�      z           �      �       "    p�      �       -    @�             9    P�             C   ��                Q    0�             \    @�             g    0 #            u   ��                �    P�      '       �    ��      :       �    ��      $       �    ��      !       �     �             �    @�                 P�      	           `�             ,    p�      -       H    ��      Q       e     �      �       v    ��      y       �    @�      �       �    ��      �       �    `8#     (       �    �8#     (       �   ��                �    ��      �           ��      �          0�      �      $    ��      �      7    �     B      M          �       d    �     �       v    �     �       �    P     �       �          A      �    p
     �       �    0     �       �         f      �     -#     �       �    �
    �m            4
    �n     �      `
    Pp     a      �
    �q     �       �
    ��     �       �
    ��     �       �
   ��                �
    ��     �          ��     �          0�     W      )    ��     �       7    0�     �       G    Ж           \    ��     	      o    �     �      �    ��     �      �    0�     t       �    ��     �      �    ��     �      �    ��     �      �     :#             �    @�     ?      �    ��     �      
    �9#     (           �9#            ,     �     !      9    P�     �      N    0�     J       b    ��     �       u     �     h      �    p�     �      �    p�     �      �    �     |      �    ��     �       �    ��     D       �    ��     �      �    `9#     8       
    �;     -            @=     �       9     \     B       L    p�     �       e    @c     }       {    �     �      �    �k     ~       �    е      �       �    �j     2      �    @]     D       �    �     �       	     �      C          �     y       -                      <    �]           P    ��      �      i    P�      y       �                     �    ��     b      �    `�      d       �    �b     P       �    �     {       �     Y     p       
    �      h           �     F       7    �w     i       W    pB     a      l    �     4       �    ��     v      �    �:#     (       �    0�      &       �    �X     p       �    �j     5       �    @�      �       �    �      V            ��      S       %      m     �       8      k     A       L     �[             i     ��      J       �     p\     1       �     �      �       �     `�      m       �     �           �     �            �     �v     E       !    0h     4       !    p     e       :!    0�      �      K!    0�      �       b!     A     �       x!     f     j       �!    �
(                      $(    ��     ;      B(    ��      �      Z(    PW     B       s(     d     �       �(    �A     �       �(    ��      �       �(    �      �       �    0^     �       �(    �G     I       �(     M     X      )    ��      @      )    �J     @      7)     j     �       J)    0[            \)    ��      g       l)     H     �       �)    ��      )       �)  "                   �)    `a     C       �)   	 �              �)     �            �)     9#     (       �)    ��      �       
*    0�     }        *    �`     5       2*    P@     �       H*    �     :       X*    PJ     7       h*    P�      ^       �*    �     �       �*    0S     �       �*    @`     �        crtstuff.c __JCR_LIST__ deregister_tm_clones __do_global_dtors_aux completed.7561 __do_global_dtors_aux_fini_array_entry frame_dummy __frame_dummy_init_array_entry ral.c pkgNamespace.9478 tupleCmdName.9472 tupleStr.9473 relationCmdName.9474 relationStr.9475 ral_pkgname relvarCmdName.9476 relvarStr.9477 ral_config ral_version ral_copyright ral_attribute.c booleanEqual booleanCompare booleanHash isAByteArray byteArrayHash isAnInt intEqual intCompare intHash isALong longEqual longCompare longHash isADouble doubleEqual doubleCompare doubleHash isAWideInt wideIntEqual wideIntCompare wideIntHash isABignum bignumEqual bignumCompare bignumHash isAList isADict isAString stringHash dictHash listHash findRalType Ral_Types stringCompare dictCompare stringEqual dictEqual byteArrayCompare byteArrayEqual isABoolean listCompare listEqual ral_joinmap.c attr_0_cmp attr_1_cmp cmpFuncs.8347 ral_relation.c tupleHashEntryAlloc tupleAttrHashEntryAlloc tupleAttrHashEntryFree tupleHashEntryFree tupleAttrHashCompareKeys tupleAttrHashGenKey tupleHashCompareKeys tupleHashGenKey tupleAttrHashMultiEntryFree tupleAttrHashMultiEntryAlloc Ral_TupleCompare Ral_DownHeap Ral_HeadingMatch Ral_RelationIndexByAttrs.isra.1 tupleAttrHashMultiType tupleHashType ral_relationcmd.c RelationSemiminusCmd RelationSemijoinCmd RelationJoinCmd RelationComposeCmd RelationAttributesCmd RelationCardinalityCmd RelationDegreeCmd RelationIsemptyCmd RelationIsnotemptyCmd RelationWrapCmd RelationEmptyofCmd RelationIssametypeCmd RelationIsCmd cmdTable.9199 RelationUpdateCmd RelationRestrictWithCmd RelationRestrictCmd RelationUnwrapCmd RelationUnionCmd RelationDunionCmd RelationUngroupCmd RelationUinsertCmd RelationInsertCmd RelationTupleCmd RelationTimesCmd RelationTcloseCmd RelationForeachCmd orderOptions RelationTagCmd gotOpt.9552 optTable.9561 RelationCreateCmd RelationTableCmd RelationSummarizebyCmd RelationSummarizeCmd RelationExtendCmd usage.9017 RelationRenameCmd RelationRankCmd RelationProjectCmd RelationEliminateCmd RelationMinusCmd RelationListCmd RelationExtractCmd RelationDictCmd RelationArrayCmd RelationIntersectCmd RelationHeadingCmd RelationGroupCmd RelationFromlistCmd RelationFromdictCmd.part.40 RelationFromdictCmd RelationDivideCmd RelationBodyCmd RelationAssignCmd cmdTable.8854 ral_relationobj.c UpdateStringOfRelation DupRelationInternalRep FreeRelationInternalRep SetRelationFromAny ral_relvar.c relvarTraceCleanup relvarCorrelationConstraintToString condMultStrings Ral_ConstraintNew relvarCleanup relvarConstraintCleanup relvarEvalAssocTupleCounts relvarFindJoinTuples.isra.1 relvarConstraintErrorMsg.isra.2 relvarAssocConstraintErrorMsg.isra.3.part.4 relvarPartitionConstraintErrorMsg.isra.5.part.6 relvarCorrelationConstraintErrorMsg.isra.7.part.8 relvarIndexIds.isra.10 relvarSetIntRep ral_relvarcmd.c RelvarNamesCmd RelvarUinsertCmd RelvarInsertCmd RelvarPathCmd RelvarExistsCmd RelvarIdentifiersCmd RelvarUpdatePerCmd RelvarUpdateOneCmd RelvarUpdateCmd RelvarUnsetCmd RelvarUnionCmd RelvarDunionCmd RelvarTransactionCmd transactionOptions.9448 RelvarEvalCmd RelvarTraceCmd traceOptions.9417 traceTypes.9421 RelvarSetCmd RelvarRestrictOneCmd RelvarProceduralCmd RelvarPartitionCmd RelvarMinusCmd RelvarIntersectCmd RelvarDeleteOneCmd RelvarDeleteCmd RelvarCreateCmd RelvarCorrelationCmd RelvarConstraintCmd constraintCmds.9155 RelvarAssociationCmd cmdTable.9134 ral_relvarobj.c relvarGetNamespaceName Ral_RelvarObjDecodeTraceOps opsTable Ral_RelvarObjEncodeTraceFlag Ral_RelvarObjExecTraces relvarResolveName relvarTraceProc relvarObjConstraintEval relvarConstraintAttrNames.isra.3 specErrMsg specTable condMultStrings.9599 ral_tuple.c ral_tuplecmd.c TupleFromListCmd TupleEqualCmd TupleAttributesCmd TupleDegreeCmd TupleGetCmd TupleUnwrapCmd TupleExtractCmd TupleWrapCmd TupleUpdateCmd TupleRenameCmd TupleRelationCmd TupleProjectCmd TupleHeadingCmd TupleExtendCmd TupleEliminateCmd TupleCreateCmd TupleAssignCmd cmdTable.8433 ral_tupleheading.c ral_tupleobj.c UpdateStringOfTuple DupTupleInternalRep FreeTupleInternalRep SetTupleFromAny ral_utils.c resultStrings optStrings cmdStrings errorStrings ral_vector.c ptr_ind_compare int_ind_compare buf.8698 buf.8821 tclStubLib.c tclTomMathStubLib.c __FRAME_END__ __JCR_END__ tclStubsPtr tclTomMathStubsPtr tclPlatStubsPtr __dso_handle Tcl_InitStubs tclIntStubsPtr _DYNAMIC tclIntPlatStubsPtr __GNU_EH_FRAME_HDR __TMC_END__ _GLOBAL_OFFSET_TABLE_ TclTomMathInitializeStubs Ral_TupleAssignToVars Ral_TupleScan Ral_RelvarStartCommand Ral_TupleConvert Ral_JoinMapFindAttr Ral_ErrorInfoGetCommand Ral_RelationGroup Ral_TupleConvertValue Ral_PtrVectorSort Ral_PtrVectorFront Ral_RelationInsertTupleObj __snprintf_chk@@GLIBC_2.3.4 Ral_RelvarNewTransaction Ral_RelvarObjEndTrans Ral_TupleHeadingConvert Ral_PtrVectorFetch Ral_RelationEqual Ral_JoinMapAttrMap Ral_AttributeRename ral_tupleTypeName _ITM_deregisterTMCloneTable Ral_TupleHeadingNew Ral_PtrVectorPushBack Ral_RelationUpdateTupleObj Ral_TupleNew strcpy@@GLIBC_2.2.5 Ral_RelationRenameAttribute Ral_RelvarInsertTuple Ral_RelationConvert Ral_JoinMapTupleReserve Ral_RelvarNewInfo Ral_TupleHeadingIndexOf Ral_TupleHeadingExtend Ral_SafeUnload qsort@@GLIBC_2.2.5 Ral_RelvarObjCopyOnShared Ral_TupleDupShallow Ral_ErrorInfoSetCmd Ral_RelvarRestorePrev Ral_RelvarObjCreateCorrelation Ral_RelationDivide Ral_AttributeScanType Ral_IntVectorMinus Ral_TupleUpdateFromObj Ral_RelationUnionCopy Ral_IntVectorOffsetOf Ral_IntVectorDelete Ral_TupleDup Ral_RelvarTraceRemove Ral_RelvarObjTraceVarSuspend Ral_RelationProperSubsetOf Ral_RelationScan Ral_RelationPushBack Ral_RelationTagWithin Ral_IntVectorPushBack Ral_ConstraintNewCorrelation _edata Ral_TupleHeadingAttrsFromVect Ral_RelationProperSupersetOf Ral_RelvarDelete Ral_RelvarFindById Ral_RelvarObjEndCmd Ral_PtrVectorDelete Ral_RelvarDiscardPrev Ral_ConstraintNewProcedural Ral_RelvarObjFindRelvar Ral_RelvarIsTransOnGoing Ral_RelvarObjInsertTuple Ral_TupleHeadingStringOf Ral_SafeInit Ral_PtrVectorPopBack _fini strlen@@GLIBC_2.2.5 Ral_IntVectorIntersect Ral_JoinMapTupleCounts Ral_RelvarObjExecSetTraces Ral_AttributeValueScanFlagsFree Ral_RelationMinus __stack_chk_fail@@GLIBC_2.4 Ral_RelationUnwrap Ral_RelvarTransModifiedRelvar Ral_RelvarIdIndexTuple Ral_PtrVectorFill Ral_AttributeTypeEqual Ral_PtrVectorStore Ral_TupleSubset Ral_RelationSubsetOf Ral_RelationUpdate Ral_TupleDelete strrchr@@GLIBC_2.2.5 Ral_RelvarFindIdentifier Ral_RelationSort Ral_JoinMapNew Ral_ErrorInfoSetError Ral_RelationObjConvert tupleCmd Ral_ConstraintDeleteByName Ral_IntVectorErase Ral_RelvarObjExecDeleteTraces Ral_TupleHeadingUnreference memset@@GLIBC_2.2.5 Ral_TupleHeadingStore Ral_TupleHashAttr Ral_TupleEqualValues Ral_JoinMapTupleMap Ral_IntVectorFetch Ral_RelvarObjConstraintMember Ral_JoinMapDelete Ral_RelvarFind Ral_IntVectorContainsAny Ral_RelvarNew Ral_RelvarDeclConstraintEval Ral_PtrVectorInsert Ral_RelvarTraceAdd Ral_PtrVectorNew Ral_ConstraintDelete Ral_RelvarObjExecUpdateTraces Ral_ConstraintProceduralCreate Ral_AttributeNewRelationType Ral_RelationUnion Ral_RelationErase Ral_RelationTag Ral_TupleEqual Ral_RelationObjParseJoinArgs Ral_RelvarDeleteTransaction memcmp@@GLIBC_2.2.5 Ral_RelvarObjTraceEvalInfo Ral_PtrVectorCopy tupleAttrHashType Ral_IntVectorBack ral_relationTypeName Ral_TupleDupOrdered Ral_RelationValueStringOf Ral_PtrVectorEqual Ral_RelationShallowCopy Ral_RelationTclose Ral_TupleSetFromObj Ral_AttributeConvertName strcmp@@GLIBC_2.2.5 Ral_RelationSemiJoin Ral_TupleHeadingFetch Ral_TupleHeadingPushBack Ral_IntVectorStore Ral_RelvarIdUnindexTuple Ral_IntVectorSubsetOf Ral_RelvarObjExecEvalTraces Ral_PtrVectorSubsetOf Ral_AttributeValueObj Ral_RelvarDeleteInfo Ral_IntVectorPopBack Ral_RelvarObjConstraintPath Ral_RelationScanValue Ral_TupleCopy __gmon_start__ Ral_IntVectorInsert Ral_AttributeNewFromObjs Ral_AttributeValueCompare memcpy@@GLIBC_2.14 Ral_RelvarObjFindConstraint Ral_AttributeNewTclType Ral_IntVectorEqual Ral_RelvarDeleteTuple Ral_InterpErrorInfoObj Ral_RelationDelete Ral_RelvarObjTraceEvalAdd Ral_ConstraintCorrelationCreate Ral_TupleHeadingJoin Ral_TupleGetAttrValue Ral_RelvarObjKeyTuple Ral_TupleObjType Ral_AttributeScanName Ral_InterpErrorInfo Ral_PtrVectorFind Ral_RelationStringOf Ral_JoinMapAttrReserve Ral_AttributeDelete Ral_PtrVectorPrint Ral_PtrVectorSetAdd Ral_IntVectorFillConsecutive Ral_JoinMapAddTupleMapping Ral_IntVectorFront Ral_AttributeScanValue Ral_RelationExtract Ral_RelvarObjTraceVarInfo Ral_TupleUnreference Ral_ConstraintNewPartition Ral_PtrVectorBack Ral_RelvarObjTraceVarRemove Ral_RelationJoin Ral_AttributeHashValue Ral_TupleHeadingUnion Ral_PtrVectorDup Ral_RelvarObjUpdateTuple Ral_IntVectorDup _end Ral_TupleScanValue Ral_TupleHeadingDelete Ral_RelvarStartTransaction Ral_TupleStringOf Ral_RelvarObjConstraintNames Ral_RelationTimes Ral_RelvarObjCreatePartition Ral_TupleCopyValues Ral_TupleHeadingSubset Ral_RelationReserve __bss_start Ral_JoinMapGetAttr Ral_IntVectorExchange Ral_Init Ral_RelationFind Ral_TupleHeadingCompose Ral_TupleValueStringOf Ral_AttributeTypeScanFlagsFree Ral_AttributeNewTupleType Ral_RelationCompare Ral_TupleHeadingAppend Ral_JoinMapAddAttrMapping Ral_TupleHash Ral_TupleHeadingAttrsFromObj Ral_ConstraintAssocCreate memmove@@GLIBC_2.2.5 Ral_TupleHeadingCommonAttributes Ral_IntVectorBooleanMap Ral_ConstraintNewAssociation Ral_IntVectorSetAdd Ral_RelvarObjConstraintInfo Ral_TupleUpdateAttrValue Ral_RelationInsertTupleValue Ral_RelationUngroup Ral_InterpSetError Ral_ConstraintFindByName Ral_RelationFindJoinTuples Ral_TupleAttrEqual Ral_RelvarObjTraceUpdate Ral_Unload Ral_AttributeEqual _Jv_RegisterClasses Ral_RelationProject Ral_RelvarObjNew Ral_IntVectorNewEmpty Ral_AttributeConvertValueToType Ral_JoinMapMatchingTupleSet Ral_RelationNotEqual Ral_AttributeConvertValue Ral_TupleHeadingFind Ral_RelvarObjConstraintDelete Ral_IntVectorSort Ral_AttributeDup Ral_IntVectorReserve Ral_RelvarObjTraceVarAdd Ral_AttributeToString Ral_ErrorInfoGetOption Ral_IntVectorPrint Ral_RelvarObjCreateAssoc Ral_RelationObjNew Ral_RelvarObjDelete Ral_TuplePartialSetFromObj relvarCmd Ral_TupleHeadingNewOrderMap Ral_PtrVectorReserve Ral_TupleHeadingDup Ral_RelationDup Ral_RelvarObjExecInsertTraces Ral_IntVectorNew Ral_RelvarObjExecUnsetTraces Ral_RelationSupersetOf Ral_ConstraintPartitionCreate _ITM_registerTMCloneTable Ral_RelvarObjCreateProcedural Ral_AttributeValueEqual Ral_ErrorInfoSetErrorObj Ral_IntVectorCopy Ral_TupleHeadingIntersect Ral_RelationSemiMinus Ral_AttributeConvertType Ral_TupleHeadingMapIndices Ral_TupleSetFromValueList Ral_RelationConvertValue Ral_TupleHeadingNewFromObj Ral_PtrVectorErase Ral_IntVectorFill Ral_RelationNew Ral_TupleHeadingScan Ral_JoinMapSortAttr __cxa_finalize@@GLIBC_2.2.5 Ral_IntVectorIndexOf _init Ral_RelationCompose Ral_RelationObjType Ral_RelationIntersect Ral_RelvarSetRelation Ral_IntVectorFind Ral_TupleHeadingEqual Ral_TupleExtend Ral_TupleObjNew Ral_RelationDisjointCopy Ral_RelvarObjTraceEvalRemove Ral_TupleObjConvert Ral_IntVectorSetComplement  .symtab .strtab .shstrtab .note.gnu.build-id .gnu.hash .dynsym .dynstr .gnu.version .gnu.version_r .rela.dyn .rela.plt .init .plt.got .text .fini .rodata .eh_frame_hdr .eh_frame .init_array .fini_array .jcr .data.rel.ro .dynamic .got.plt .data .bss .comment                                                                              �      �      $                              .   ���o       �      �      �	                            8             �      �      8                          @             �'      �'      .                             H   ���o       �@      �@      Z                           U   ���o       PC      PC      P                            d             �C      �C      00                           n      B       �s      �s      @                          x             �      �                                    s             0�      0�      �                            ~             ��      ��                                    �             К      К      �                            �             �p     �p     	                              �              q      q     �,                              �             Н     Н     l                             �             @�     @�     �n                             �             �#     �                                  �             �#     �                                  �             �#     �                                   �             �#     �     0                              �             �-#     �-     �                           �             �/#     �/     p                             �              0#      0     �                            �             �7#     �7     �                              �             �:#     �:     H@                              �      0               �:     -                                                   �:     87         !                	                      r     �*                                                   ݜ                                  MZ�       ��  �       @                                   �   � �	�!�L�!This program cannot be run in DOS mode.
$       PE  d� kπY �   � &  �  n    �       �m                        p    �<  `                                     � u    � l           P �           � x                           � (                   ܱ �                          .text   H�     �                ` P`.data      �     �             @ `�.rdata  @I      J   �             @ `@.pdata  �   P     *             @ 0@.xdata     p     B             @ 0@.bss    P
   �             @ @B/70     	         �             @ B/81     8.   0  0   �             @ B/92     �   `     �             @ B                                                                                                                                                                                                                                                                                                                                                                                                SH�� �   ��� H��H���� H��3 H��H�H��3 H�t1�H�    H�� [ø   H�� [� AUATUWVSH��(��I��M��uz�� ���  ��H�3 1��z �   H�-ڡ ���  ��H���H�3H��u�H�=�2 �����   �   �t� �   H��([^_]A\A]�f.�     ���   u�eH�%0   H��2 H�p1�H�-e� ��    H9��  ��  ��H���H�3H��u�1�H�=j2 ����  �����   ����  ����   H��1 H� H��t
H��H�����L�D$8�T$4H�L$(�}� �� L�D$8�T$4H�L$(H��H�q����UH��]�f.�     UH��H�� H�=��  t0H�
  �   ��H��8[^_]A\A]�D  H������H�
� H����  L�I��L�� H��� H��H�D$     A��  H�E1�L��� H��H�����  �������H������H�
  H�
1�f�H���J��L9�u�H��8�f�f.�     H��8H��* L�D$,H� ��@  H��8�f�VSH��8H��* H��L�D$(H��1�H���@  ��t	1�H��8[^�H�1�L�D$,H����@  ��u��D$,9D$(����H��8[^�@ VSH��8H�S* H��L�D$(H��1�H���@  ��u)H�1�L�D$,H����@  ��u�D$(+D$,H��8[^� �����H��8[^�@ VSH��8H�5�) H��H���D$,    1�L�D$,H���@  ��u!�D$-�T$,��D$.��D$/�H��8[^�H�H��H�p ���
  H�
  H�
  H�
  H�
1�f�H���J��L9�u�H��8�f�f.�     �@ f.�     �@ f.�     H�H�	��� D  WVSH�� H�b" H��H��H����
  H��H�H�����
  H��H��H�� [^_�� f��@ f.�     WVSH�� H�" H��H��H����
  H��H�H�����
  H��H���Q� ������H�� [^_�f�     �@ f.�     WVSH��0H��! H��H�T$(H���  H��H�H�T$,H����  D�D$(D9D$,H��DND$,H��Mc��� H��0[^_��    WVSH��0H�R! H��H�T$(H���  H��H�H��H�T$,��  LcD$(1�D;D$,t��H��0[^_�@ H��H��蝿 ������H��0[^_�D  ATUWVSH��0H�=�  L�d$,H��H��H�M����@  ����uR�|$,v<H���   t2H�1�H��L��   ��X  H�
   H�T$0H��H�W� H�L$0H�D$ 訾 H��H��tPH���0� �H!H�� H� �P(H�H H��H���� H�H��C    H�C    H�CH��H��`[^_�D  1���f�f.�     WVSH�� H��H���ƽ �H!H�d H� �P(H�H H��H��诽 H�H�E� �C   H�sH�C�H��H�� [^_��     WVSH�� H��H���f� �H!H� H� �P(H�H H��H���O� H�H��� �C   H�sH�C�H��H�� [^_��     VSH��(�QH�˅�t)��vH�5� H�
� L���A�P 1�H��(�f�H�RH�IH��(�Os H�RH�I誻 ������H��(�D  VSH��(H9�H��H��t@H�H�	�|� ��u�V9StH��([^Ð1�H��([^��    H��H��H��([^�O����   H��([^� UWVSH��xH�֋QH��L�ǃ���   rQ��u/H�� 1�L�L H��H����   ���(  1�H��x[^_]�H�v H�
   H�D$@H�y���H�D$ 诺 H��H��H����   �P�R����     H�� 1�L�� H��H����   ���$���H�1�L�` H�����   ������H�W H�N �RM ������T$<1�����H��x[^_]�f�     H�1�L� H�����   �������H�W H�N �f8  �������������fD  VSH��hH�ӋQL�ƅ�t=��wL��H������H��h[^�fD  H�� H�
   H�D$0H�(���H�D$ �^� H��H��H��t��PH��h[^�f�     WVSH�� �RL�Ã�t0ru��u{H�5` L�	 H��H����   ��uqH�K �=  �'H�57 L�� H��H����   ��uHH�K �GT H��H������H�����  H��H�H���P0H��H�� [^_�H�� H�
  I��H�L�L$8L�D$4H��H����x  ����   �D$4����   ��utH�T$8H�H�
���
  H�
  �   I��H���� H��H��詉 1�H��@[^_]A\�H�T$8H�H�
���
  L��H��H���}���H��u�I�غ   H��H�D$(蓈 H��H���X� H�D$(몐H�D$8I��H��H�P�L} H��t�H��L������������    I�غ   �W��� AUATUWVSH��xH�׋RH��L��M�̃��\  ��   ��t1H�� H�
   H�D$@H�c���H�D$ 虵 H�������H��H���P��I�غ   ������i���f�H�i I9@�/  A�8��   L�-� L��I�E ���   H�OL�d$ I��I��H��H���� ����   ��P������   1�H��H��x[^_]A\A]�H�OL�L$ H��M���9�  ��u�H� H��H��H� ��`  ����f.�     H�OL�L$ H��M��茂 ��u�H�� H��H��H� ��`  f�H�F H�OH�P�k ���f���H��H��x[^_]A\A]��    I�E H��1���   �?���I�E H����`  H��u�1��&��� L���L������� WVSH��pH�֋QH�˅�t-��w
1�H��p[^_�H�6 H�
   H��H�D$@H�R���H�D$ 舳 H��tH���P H��p[^_�D  H�H��H����X  H�D$@���H��J���L�D
1��     H���J��L9�u��'���f�f.�     SH�� �AH�	H��H�R�B�H�D H� ���  �CH�� [�f�H�) E�@H�	H� H���  H��D  H��(�A���tVr4��tH�� H�
H��(Ðf.�     WVSH��0H�֋QH�σ���   ru��uPH�NH�m� �{H�^L�D$(贱 �F	 �F
{H�OL�D$(H���Kv H�H�� }�@}H��)�H��0[^_ÐH� H�
  L�L$(H��H��I�QH��0[H��f.�     I�J M��H��0[�OI D  f.�     WVSH�� �AL��M��L�L$`��tvr9��uH�J H��H�� [^_�2  H�� H�
  A��H��H��H��H�� [^_H���    H�J H��H�� [^_�I  f.�     WVSH�� �H�υ�tJ��wM�AH�I��~%�p�H��H��H�f�H�Y �����H9�H��u�H�OH�� H� �P0H�G    H�� [^_�H�� H�
 �(   H��H� �P(H�X �H�@    H�� [ÐSH�� H�4
 �8   H��H� �P(H�P(H�@    H�P H�H�SH�H(�H�P0H�� [�@ f.�     SH�� H�A H��H��; H��	 H��H� H�@0H�� [H�� SH�� H��H�I �_; H��	 H��H� H�@0H�� [H��fD  H�B H�QH�	L�HL� ��< f�     H��H�RH��a< �H�R �'; �    H����; �     SH�� H�A H��H���: H�K�c} H�	 H��H� H�@0H�� [H��f.�     VSH��(H�� �8   H��H� �P(H��H�@(�   H�C �P| H�CH�H�C(� H�FH�C0H��H��([^�AVAUATUWVSH�� I� �։�I�HM��H�XL�(�} I�NH���L�$��} H�H�,�I�FH�xH�0H9�u�OH9�t.HcL��H������^ H�MI�T$L��H��H��������t�A�N���څ�E�H�� [^_]A\A]A^�1���f�f.�     ATUWVSH�� �t	��A��L��9�|1�X@ ��I������"�����yBH�M�t��A�؉��*� A9�~)�^A9�~͉�I����������I���Nމ��������x�H�� [^_]A\ÐATUWVSH�� H��H��L���h] ����uqH����j H��I����j H��M��   H��H���Hw H�=1 A�����H�4� H��H����  H�A�����H��H�����  H�L���P0H�H���P0��H�� [^_]A\�@ AVAUATUWVSH��@H�� H��L��H��M��L�ز �����H� L�l$,L�d$0��H
���H�^H;^ tHcCE1�H��H��H�T� �H���H9^ u�H��H��([^_]�fD  ATUWVSH�� H��L������H������L�GL+GH�ٺ   H��I���W���H�_H;_I��H�0u�2D  H��H9_t#H���F���t�H�E1�H��H������H9_u�L����a H��H�� [^_]A\� f.�     AWAVAUATUWVSH���   H�H�yH�t$`L�qH��$  H�D$(H�Y� H��H� ��P
  ��H�D$xH�'     H�'     ��  �G�H�D$`    H�D$P    L��$�   L�-�� H�=�� ��H��H��L�L�%�& H�D$@H��$�   H�D$h�P����G  ���"  A��   H�E H��H�SL�D$h��@  ���=  �     H��H;\$@��  H�E A�   L�t$0�D$(    L�l$ I��H�H�����	  �������Hc�$�   A�����J  H��H���T��t]�T���L�CI�H����%    A��   ��@ H��I����   E1�L��L��L�L$X����L�L$XH�D$PL���N �=���L�CI�H����%    A��   �l@ H��I��tIA�   �H�E H�
   H���KI H��H���I �p���H�|$P ��   H�|$` H�D$ptLH�D$(��$�   L��L�L$`L�D$PH�T$x�D$ �����H��tAH�U H��H��h  �]  H��H��������D��$�   L�D$PL��H�T$xH�D$ �W����H�T$pH���I �����H�D$`    I�OI+O1�H���'K 1�H��H�D$P�M �D��� AWAVAUATUWVSH��H  ��H�ˉ�$�  L��$�  ��  H��$�  L�%�� L�l� H�pI�$H�����   ��t�   ��H��H  [^_]A\A]A^A_�H��$�  H�v H��L�&� H�xI�$H�����   ��u�L�t$P1�1�L�o L��$�  �����A�2   �   L��H���!G H��$�  L�t$0H�|$(L�l$ I��M��H��H�ك�$�  H��$�   H�D$@�3`  �����9  L��H��I������H��H���������$�  �P�����$�  �  H�`� H�D$H�}   fD  L�o 1�1��#���H�T$@L�t$0I��H�D$(L�l$ M��H��H���_  ����   L��I��H���(���H��I���}���H���U�����$�  �P�����$�  ��   L��H��$�  L�D$HH��H�PH�8I�$H��$�  H�����   ���U���H�������A����     H��� L�
  H��H��H���8# H;CI��u�H�L$P� H���< H�l$ A�   H��$�  A�@   �   �:: �D$<   �����H�6� L�
  H�T$PH������H�t$`H��H��H���k H;F�c  H�L$`�'���H�t$XH��H�D$hH�VH+VH�������L�nL9n��   H�L$`I�u ��  L�} L;}H�D$@H�Xt1f.�     IcH�FI��I��H��H�Q�%�  L;}H�H��u�H�L$P�?�  L�7L;wH�D$HL�xt'IcH�FM��I��H��H�Q���  L;wH�M�<�u�H�L$HI���H+ H�� E1�H�T$@H�L$h����H�D$XL;h�6���H���: H���: I�$H�L$hH��h  �lK  H��$�  H���������f.�     H�L$P� H���^: H�l$ A�   ����L�D$@H�t$p�   H���g6 H��$�  H����6 H������H�L$`�e H�L$P�[ H���: H����9 �D$<   �_���@ f.�     UWVSH��(��H��t0H��� L�
�D$<   �H�CL�v H�D$@� H�C I�� H�D$HH�C(� H�D$X軻��I�vI;vI���G  H�D$lH�D$P�GL�.I�UI�$�>" L��I��I��L��轼��L����5 ��P�����v  H��I;v��   H��?& � H��H�E E1��D$    I��H�T$@H����0  H���C  H�E L�D$PH��H�T$H��P  ���$  �T$l���W���H�E E1�H�T$XH����8	  ����  ������p  H�E H����@  I��H�E L��� L��H�����   ��u6M�o I�I�U�� �������L�|$ A�   A�A   �   H���3 �D$<   H�E H�L$@H��   ���
  E1�E1�H��H����H�t$X��P������   H�t$@��P������   H�t$H��P����~i�D$<��tAL���d�������H�E H����   �x�����P�����b���H�E H����   �P���H�E L��H��h  �E  H��H�����j���H�E H����   ����f�     H�E H����   �Y���H�E H����   �2�����t]��uH�E �����H�
A�   �f�L�~ H�EI�I��H�D$0H�qL�i� ���I�OH�D$8I�GA�H9�H�D$@�  H�D$lL�l$PI��D�d$\H�D$HM�7I��M�fI�FL��HhH+hI9�u�   �    I��L9���   I��I�U�H�E1��D$    H��M�$H����	  H��u�L�l$PL9�t!H�H�H��E1�E1�H��H���   I9�u�H�t$0��P������   H�L$8A�   舷������ H�L�D$HH��H�T$0��P  ��u��D$l��uqI��L9|$@����L�l$PD�d$\L9�t!H�H�H��E1�E1�H��H���   I9�u�H�t$0��P����~IH�H�L$8H��h  ��A  H��H��������H�L$8E1�L��護���z���H�H�L$0��   �-���H�H�L$0��   �D  AWAVAUATUWVSH��X��H��L��t6H�^� A�   L�
A�   �f�H�m L�s L�kH�M 諵��L�}H�]A�H�D$8A�E I9���   H�D$LH�D$0�
  E1�E1�H��H����A��P���A���   A�E �P���A�U ��   H�L$8A�   �)�������@ H�L��H��   ���
  E1�E1�H��H����A��P���A�~?A�E �P���A�U ~!H�H�L$8H��h  �?  H��H�����M���H�L����   ��H�L����   �H�L����   �Z���H�L����   �5���f�AUATUWVSH��  ��H��L��t6H�/� �   L�
  A�@   I�ź   H���) I��L��L�������H��t H�U H��H��h  �>  H��H�����Z���H��H���   �?* �E���f.�     AWAVAUATUWVSH��  ��H��A��M���  H�-$� I�XL�ɷ H�E H�����   ��tA�   D��H��  [^_]A\A]A^A_�H�{ H�E H��I�^L��� H�����   ��A��u�H�[ L�l$ A�>   �   L���( E1�H��M��H���8���H��H����   A�G�I�^ M�t� �LH��L�{�H�E L�� H��L�����   ����   I�W E1�M��H�������H��I���i���M��tTL��L9�u�H�E H��H��h  �9=  H��H�����
���H�� L�
  A�=   I�ź   H���B% I��L��L��蔷��H��t H�U H��H��h  �L:  H��H�����Z���H��H���   ��% �E���f.�     AWAVAUATUWVSH��(  ��H��A��M����   L�5Դ I�xL�y� I�H�����   ����t�   ��H��(  [^_]A\A]A^A_�H�O A�E�I�_A�������D��D�d$,L�d$0H��H������A�<   �   L���M$ �D$,��t$A�E�M�l� H��L�C�M��H��H���9  L9�u�I�H��H��h  �?9  H��H�����`���f�H�	� L�
H��h  �_�  H��� H��H���׉�H��8[^_]�@ H�|$ A�   A�;   �   H��   �-$ �Y����     �   ��H��8[^_]�AWAVAUATUWVSH��8��H��A��L����   L�%�� I�XL�,� I�$H�����   ��tA�   D��H��8[^_]A\A]A^A_ÐH�{ I�$H��H�]L�� H�����   ��A��u�H�S H�������H��H����   A�F�H�] L�t� �E H��H�k�I�$L��� H��H�����   ��u}H�U H��豮��H��I�������M��tqL��L9�u�I�$H��H��h  ��5  H��H�����4���f�     H��� L�
  E1�E1�H��H�����v���f�     I�MI+M1�H���� 1�H��I���  H�C H�D$H�����f���t4��t\M�}������H�E H����   �G���H�E L����   � ���M�}1������H�E H��   ��   M�}�����1������H�E H��H��   H��   ���  H�
  H��H���r H;EH��u�M�E �   H�|$8H��� H��H��A�   �r H�L$@��  H�L$H莗���V���f�     H�i� L�
  E1�E1�H��H����A��P���A���   H�L$x�i���H�L$p�_�������L�E ����H�L$pH�T$hE1�����H�L$xH�D$`H�D$`D��$�   H;A����D��$�   L�t$XH�L��H��   ���
  E1�E1�H��H����A��P���A�~mH�L$x�ל��H�H�L$pH��h  �'  H��H�����c���H�H�L$X��   �0���H�L$h��  A�$�P���A�$�����H�L����   �����H�H�L$X��   �@ AWAVAUATUWVSH��  ��H�ˉ�L����  L�=� I�hL��� I�H�����   ��t�   ��H�Ę  [^_]A\A]A^A_�L�u I�H��H�oL�j� M�&H�����   ��u�H�E A�4   �   L�(H�D$@H��$�   H��H�D$0�n I�VI�MI+VI+MH��H������L��I��L��H�D$8� I�UI+UH�H��H9��7  �F��VUUU���D$L�������)ƍv���)���   H�G L��H�o(H�D$P�j�  I�ŋD$L��A���
  E1�E1�H��H���֋�P������   H�L$h趗������H�T$`H�L$hE1��o���H�|$@H�D$XH�D$XD��$�   H;G�������$�   L�t$PI�L��H��   ���
  E1�E1�H��H����A��P���A�~_I�H�L$hH��h  �"  H��H��������I�H�L$P��   �N���H�L$`���  ��P���������I�H����   �����I�H�L$P��   � AWAVAUATUWVSH��  ��H�ˉ�L��$�  �(  H��$�  L�=h� A�   L�� H�pI�H�����   ��tD��H�Ĉ  [^_]A\A]A^A_�H��$�  L�n H��$�   �   A�   H��H�@I�u H�D$8�E�A���� D��VUUUD�t$x��D����)RA)���   H�D$8H�� �3�  H��H��$�  H�h �D$x��A��u/�   @ H��H����  H;F�  A��H��E����   L�EH�U I��H��见��H��u�H��A�   ���  ����H�H� L�
 �����H��$�   ��P����I���   f�H�L$P�F�  H�|$8I�H��H��   ���
  E1�E1�H��H���֋�P������   H�L$XA�   ��������H�T$PH�L$XE1��ɓ��H�D$HH�D$HH9D$p�!���D�t$|H�|$8I�H��H��   ���
  E1�E1�H��H���֋�P����~YI�H�L$XH��h  �  H��H��������I�H�L$8��   �X���A�$�P���A�$����I�L����   �����I�H�L$8��   � AWAVAUATUWVSH��  ��I�͉�M����   H�5� I�XL��� H�H�����   ����t�   ��H��  [^_]A\A]A^A_�L�d$ L�s �_�A�-   �   L���~ ����   L���M�����I����   ��I�_��H��I�|?(�
  I��H�H����
  M��M��H��L��������u�L��L��   �� L���8����@��� H��� L�
  H��H��H���_�  H;G�  L� H+GK�L.�H��H��H��� H� ���
  H��^ H���Su��H��I�ź   ���  L��H��H��H�D$P�@�  H;G��  H�L$PE1������A��H�D$X�;  L�vH�FHc�H�<�    I9�L�t$@M����   fD  H�L$@I9�L�1I�VH�,:��   1�� �I��L;~t4I�I��L��H�@H�8�y������A��u�1҅���I���L;~ũ�H�L$P���  I�NL�xH��I�FH��M��HPH+P��  H���M�4�H�b� H� ���  I�� E1�H�L$XH���C���H�D$@H�FH�\$@H9�t	L�~�&���H�� H�L$XH� H��h  �+  H��$�   H����������   �R���H�� H�L$lH�=<[ I�VA�   L��e H� H�L$0�D$(    H�|$ H��$�   ���	  ����   HcD$lH�Te H��D�l�\���H��$�   H�l$ A�   A�+   �   �S �D$L   � ���H��$�   A�   A�+   �   H�l$ �# H�L$P��  �D$L   �����H�L$X�B����D$L   �����D  AUATUWVSH��(��H�Ή�L����   L�-ܒ M�`L��� I�E L�����   ����t�   ��H��([^_]A\A]�f.�     M�d$ D�G�L�MH��I�$��  H��H��t�H��L���t���H��H��� I�E H��H��h  �v  H��H���׉�H��([^_]A\A]�H�8� L�
 H��H��� H��L���Q���H��I��� I�E L��H��h  �S  H��H���׉�H��([^_]A\A]�H�� L�
  I�$H��I���J�  Hc���  ���  ���]  M�E E1�I�$H�����  H��H����   L��E��H���Ф��H��I���E H�1�1�H�����  I�] I;]I��u��   f�H��I;]��   HcI�D$H��H��H�PH�L�*L����p  ��t�A��P���A��R  L���� A�   D��H��X[^_]A\A]A^A_��    H�i� L�
  H�L$8H��H���/�  ��x;I�VL�H�H��L��L��A��p  ��u;M9�u�H�L��H����h  �L�    H�t$ A�   A�   �   H�����  A�$�P���A�$H�L����   fD  �   ��H��H[^_]A\A]A^A_��     H��� L�
  H�L$8H��H���P�  ��x7I�VH�L�$��5���L�d$ A�   A�   �   H���   ��  �j���H�t$ A�   A�   �   H���   ���  �B��� f.�     AWAVAUATUWVSH��8��H��L��t5H�Ί �   L�
  I��H�E H�K ���
  L��L��H���+�  Lc�E����   L��H����  Lc�E����   H�E I��I�����  I�^I;^I��u�g�    H��I;^tVH�L�U L��H��H�@N�(N� A���  ��t�A��P���A��5���H�E L���   ��   �����f�     H�E L��H����h  �����L�|$ A�   A�   �   H���M�  ������     H�\$ ��f�     AWAVAUATUWVSH��H��H��L��t5H�� �   L�
  I��H�E H�K(���
  L��L��H���w�  ����   H��L��D$<�`�  ��D�D$<��   I�^I9^tSH�Mc�L�,�    I���     H�L��H��H�@N�(N� H�E �D$    ��0  H���E���H��I;^u�H�E H�����  �����@ L�d$ A�   A�   �   H�����  ����H�\$ ��fD  AWAVAUATUWVSH��  ��H��A��M���  H�-�� I�XL�9� H�E H�����   ��tA�   D��H��  [^_]A\A]A^A_�H�{ H�E H��I�^L��� H�����   ��A��u�H�[ L�l$ A�   �   L���	�  H��M��H���+���H��H����   A�G�I�^ M�t� �IH��L�{�H�E L��� H��L�����   ����   I�W M��H���ڝ��H��I���߀��M��tZL��L9�u�H�E H��H��h  �  H��H��������f�H�y� L�
  L��H��H���p�  ��y�H�\$ A�   A�   �   L���N�  H���   ��  �G���H�E� L�
  L��H��H�����  ��x��H����  H;GtzH�L$HI��H���B���H��t6H�H��H��h  ��  L��H��������H��M A�   H�D$ ����H��M A�   A�   �   L��   H�D$ �M�  �S���H�\$ A�   ������     AVAUATUWVSH��   ��H��L��t5H�� �   L�
L�����  �I��   L���
  �   H��L�L$(�u�H��$�   H�{L�D$4I�1L��L�L$8L�8I�E H����x  �����  �D$4���   ��H��$�   ������4g���D$4����   H�D$8�K�����   H��$�   ����g������   ����   �D$4�P�H�D$8�T$4H����H�D$8��   I�U H����
  H��H���^�  ��H�D$8H�HI�E ���
  L��H���>�  ��A���s���H�D$8H��$�   �   L� ��  ��    H��$�   I���	   �{�  H��$�   L�����  �   �m����m I�$�_���H�D$8H��$�   �   L�@�8�  �H�D$8H��$�   �   L� ��  �H�D$8H��$�   �   L�@���  낐������������VSH��(H�5st H��H�I��P����~H�H��H�@0H��([^H��H���   ��f�AUATUWVSH��(H�5-t H�jL��H��L�*L�b(A�����H��H�H�����  H�A�����H�F H�����  H�H�U A�����H��H�-H ���  H�A�����H��E H�����  HcWHcGA�����H��H�PH�T� H����  H�A�����H��E H�����  H�A�����I�U H�����  �GH��tH�A�����H�{E H�����  H�H��A�����H�kE ���  HcW0HcG4H��A�����H�PH�T� H����  H�H��A�����H�2E ���  H�I�$H��A��������  H�H�E A�����H��H���  H��([^_]A\A]H��f�VSH��(H���� �HH��r H� �P(H�HH��H��H�H�� H��H��([^�D  UWVSH��(H�=ar H��H�IH��t.�H�=Lr �P�����   ��  H���`l��H�N�'�  H�NH�AH+H��H��w����  H�N���  H�N H��tH��s���H��H��u��F0H�V8��t9��H�^@H�@H��H�lhH�{� tH�H����x  H�K����  H��`H9�u�H�H��P0H�H��H�@0H��([^_]H��H���   �T����f.�     WVSH�� �H�˃���   ��   ��t@��uH�yH���  H�OH��tF�H�5-q �P����:H���   �/�     H�yH�O H��t�b��H�O@H��t� b��H�5�p H�H���P0H�H��H�@0H�� [^_H��H�5�p H�
E���   u1���H��x[^_]A\A]A^A_�H�] H�
  H��I��tW1��@ H��H�H��tD9{u�H�E H�K���
  L��H����  ��u�H��t1H�H�L�;H��L��A���v���H��u�D��H��([^_]A\A]A^A_�L�;M�~ �ϐ���AWAVAUATUWVSH��X�B�I��L�σ���   1���H��N �  H�1�1�H�l$0���  I��H�H�OH�����  H��tff��ODL�x �Q���uL�x H��tH�H��L�����  ��t(H������L��L��p  ���  L��I��L��A�օ�u`H�H����  H��u�H�L��L����h  1�H��X[^_]A\A]A^A_�f.�     H��M L�
  H��������f.�     AWAVAUATUWVSH��X  ��H�ˉ�L��M��H�D$X    ~dL�-=M I�HI�E ���
  L��I��H���QH  H��H��tH�PI�E H��L��K ���   ��t@�   ��H��X  [^_]A\A]A^A_�f�H��L L�
  L��I��H���F  H��H��tH�PI�H��L�xI ���   ��tA�   ��H��X  [^_]A\A]A^A_� H��J L�
  L�L$(H��I��L���C  H���   H��t�H������H�	H��h  ���  H��H����1�H��8[^�@ f.�     VSH��8��H��t2H�{G L�
  L�L$(I��H��L���TB  H�H��H��h  t%�   ���  H��H����1�H��8[^�f�     1����  H���� AWAVAUATUWVSH��H��H��t8H��F A�   L�
  L�L$(I��H��L���A  H��H��tH�PH�H��L��D ���   ��A��tA�   �fD  H�G1�1�L�h H����  H�D$0H�G8H�D$(�G0����   ��H�D@H��HD$(H�D$8H�D$(1�1�H�(H����  L�} L;}H��u�y�     I��L;}tgA�I�M �J�  L������H�M��p  A���  H��I��H��A�օ�t���P����~jH�|$0��P�����"���H�H����   ����H�I��H�T$0H����p  ��u�H�D$(`H�D$(H;D$8�5���H�H�T$0H����h  �e���H�H����   � f.�     AWAVAUATUWVSH��  ��H��$�  M��L��$  t7H��D �   L�
  H��$  H��$�  I���?  H��I��t'H�PH�OD L��B H��$�  H� ���   ��t�   �H��$  H��$�  M���D>  ��u�I�GI�]L��B H��$�  H�@ H��H�D$`H��C H� ���   ��u�H�k H�|$`H�E H�XH;Xu#�cH�H���&�����
  H�E H��H;XtBH�3H�H��A�  H�H;Bu�H�A�   H�D$ H��$�  A�C   �   �R�  �*���A�O0�T�  A�O0H�D$X�ƶ  H�D$PI�G8�D$8    I��H�D$HA�G0H�@H����   L�|$@L�|$`L��$   f.�     I�E1�H�HH+H���j�  I��I�H�H;XuD�sf�     H�M ���e�  H��H���%����t?��L��A����  I�H��H;X��  �I��-�  H�M H�H���n�  �����u�L��诶  E����  H�D$@I��`�D$8�@0H�@H��HD$HI9��?���L�|$@L��$   H�|$XH�GH+H��H����  H�E H�HH+HH��胵  H��H�D$XL�t$XH�8H;xt-L�'I�$I;\$t�H��H���Ѻ  I;\$u�H��I;~u�H�E H��H�PH+PH����  H��H�D$H�۵  H��$�   A�C   �   H��H�D$h�k�  H��$  L���������v  A�G(���5  H�D$`H��
;��H��$�   H�EH;EH�D$8��   H�l$@H�l$XL��$�   H�D$8L�m H�UH�8H�D$PI9�L� ��  1�H������ H��$�   I�E L��H��$�   Ic$H�@H��I�L@A���   H��t���H�@t9���   ��Hc�I��I��L;mu�H�U Hc�M��I)�I��I9���   H�|$@H�D$8H�D$8H;G�E���H�L$H蘴  H��$  H��$�  E1�A�G(    �xg  �����P  H��$�   �   �L:��H�L$P�R�  H�D$XH�|$XH�0H;ptH�H���3�  H;wu�H�L$X賿  �:������p���H�L$XL���y�  �T$8H�L$P���  �P���Hc�L�mH�U Hc�����H�A�   H�D$ ����H�D$`H�@H��L�(H�D$xA�} ~L���v  I��H�D$HH�0H;p��   H��$�   L��$�   M��L��$�   I��L�d$HH�l$hH��$�  �D  H�H��� I;t$��   HcI�UI�NL�<�I�UH�RH��H�蘙  M�H�H��I��H���&��I�VH+ZH��Hc�H��H��IVH�
H��t�D�E�H�A��D�	�H�D$pH��> H���   I^H�D$pH���\���M��L��$�   H��$�   L��$�   L���	�  I��H�D$hL�D$xH��$�  L��H�D$(H��$�   H�D$ �:?  �������H�L$H���V�  H��$  H��$�  E1���A�G(    A���/e  ��������������H��= H��$�   H� H��h  �����H��$�  H��������L��$   H��$�  A�   A�C   �   �   I�EH�D$ �گ  �Y���H�L$X�   �6�  H�L$P蜱  I�EH��$�  A�   A�C   �   H�D$ 蕯  ����1�H����������H�L$H�X�  M�E�-   H�|$hH���r�  H��$�  H���ҭ  ����H�L$H�#�  M�E�,   ���     AWAVAUATUWVSH��h  ��H��M��L��t?H��< �   L�
  H��I��H���~7  H��I��tH�PH�H��L��: ���   ��t�   �@ M��H��H���B6  ��u�I�GH�l$pA�B   �   H��L�` �ޫ  H�I�V L�L$hL�D$`H����x  ��u�H�D$dL�L$hD�D$`H�l$(L��H��H�D$ �<;  H��I���u����T$dI��L������L��H�D$H�l  L��H���)�������  E�o(E���&  I�$A�G(   �5��H�D$PH�D$HI;D$�'  I�NH�L$XH��ݟ  � I��H�E1��D$    M��H�T$XH����0  H���/  H�D$PM�N(L��L�D$HH�l$0H��L�d$ H�D$(�7>  ������   H�I�O��`  H�H�L$XL��   ���
  E1�E1�H��H��A��A�$�P���A�$��   A�G(    E��H��H���a  ��ur��tm�������H�H�L$PH��h  �k���H��H��������I�FA�-   H�D$ A�B   �   H���|�  �����E1�A�G(    H��H���Aa  ����t�H�L$P�!4�������I�FA�,   H�D$ �A�   ����A�$A�   �   �P���A�$�����H�L����   �����H�L����   ����D  f.�     AWAVAUATUWVSH��  ��H��M��L��t?H�X9 �   L�
  H��I��H���.4  H��I��tH�PH�H��L��7 ���   ��t�   �@ M��H��H����2  ��u�I�D$A�A   �   L�x H��$�   H��H�D$H腨  L��H���:�������  A�t$(���F  I�EA�D$(   I�� H�D$hI�E � H�D$pI�E(� H�D$X� 2��M�oM9oH�D$PH��$�   �s  �t$|H��$�  I��L�d$`H�t$hH�l$p�'D  A�$�P���A�$��  I��M;o�  I�M 腜  � I��H�E1��D$    M��H��H����0  H����  H�M��H��H����P  ��A����  ��$�   ��t�H�D$HL�L$XM��H�T$`L�d$ H��D�\$xH�D$0H�D$PH�D$(�:  ��D�\$x�A���M��H��$�  L�d$`A��J���A���  1���tE1ۃ���A��H��A�D$(    E��H���5^  H�L$P���   E�H�AH+AH��H���&  H�H�L$hH��   ���
  E1�E1�H��H����H�L$X��P�����  H�L$h��P�����  H�L$p��P������   ����   H�L$PH�H��h  �q���H��H��������@ H�L����   �C���M�E�-   H�|$HH��艦  H��H����  �.���f�     M��H��$�  L�d$`A��P���A���   A�   �   �����M�E�,   �L�d$`�t$|H��$�  E1�����H�I�L$��`  �����H�L$P�/���J���H���   �����H���   �����H���   �����H�L����   �o���H�D�\$HL��D$x��   D�\$H�D$x����f.�     UWVSH��(��H��L��~5�B�I�XI�t��H��L�C�H��H���l1  ��uH9�u�1�H��([^_]�H��4 L�
  L��I��H���/  H��H��tH�PH�H��L�3 ���   ��t<�   H��(  [^_]A\A]A^A_�H�'4 L�
  L��I��H����,  H��H��tH�PH�H��L�A0 ���   ��t<�   H��(  [^_]A\A]A^A_�H�g1 L�
 H� �   I���   H����P  ��H��X[^_]A\A]�fD  I�$��H�
  L��I��H����%  H��������\$H����   ~\����   ����   ���6  I�$L�
  H��I��H���#  H��I��tH�PI�$H��L�' ���   ��t	�   �f�M��H��H���b"  ����u�I�FL�l$0A�3   �   L��H�@ H�D$ ���  A��tI�$I�VH����h  �C���L�I�$H��L��& L�����   ��u�M�G H�D$ I�H�L�D$(�}  ��L�D$(��   L��H���K�������   A�F(���D$ ��   A�F(   A�M��M��L��H���S  H��H����   M��H��L�����������   ��P������   A��P���A���   D�D$ A�F(    H��H���JN  ���������S���I�蓊  �   I��L��H����  L��H��赗  I�$H��   �P0����L�G�-   L��� �  L��H��腗  �W���L�G�,   ���D$    �T���L��H���]�  �D$    �,���I�$L����   �<���I�$H����   ����f�f.�     AWAVAUATUWVSH��8  ��H�Ή�L��L��~mL�-& I�HI�E ���
  H��I��H���*!  H��I��tH�PI�E H��L��$ ���   ����tG�   ��H��8  [^_]A\A]A^A_�f�     H��% L�
���H��I���/V  M;ftI�$E1�H������I�E H��H��h  ����H��H��������f.�     L�9�  L���	   �̔  L��H��葕  �����f�f.�     H��8��~'Hc�I�D��L�L$ I��I���[<  H��8�fD  H�i$ L�
�  �   H� ��P  �   H��8�f.�     SH��@��H��~��u.H�'$ L�
  H��I��H���m  H��I��tH�PH�E H��L��! ���   ��t	�   �f�M��H��H���2  ��u�I�GL�fH��L��! L�h H�E L�����   ��u�I�D$ L�d$0A�&   �   L��H�D$(襒  L��H���Z�������   E�w(E����   H�T$(A�G(   M��L���=:��H����   H��茧��� M��I��L��H��H���N  H��tM��H��L���������uL��H��A�   �
  L��I��H���z  H��H��tH�PH�H��L�� ���   ��t<�   H��(  [^_]A\A]A^A_�H�! L�
  H��I��H���  H��I��tH�PI�$H��L�  ���   ��tF�   ��H��H  [^_]A\A]A^A_�H�D L�
  H��I��H���,  H��H��tH�PH�H��L�� ���   ��t	�   �f�I��H��H����  ��u�H�EH��H��L�x �[�������  �E(���D$D��  I�D$I�L$ �E(   M�g� H�D$H�M;gH�L$0�!  H�L$\H��$�   H�l$8H��I��� I��A��P���A���   M9g�  I�$��  � I��H�E1��D$    M��H��H����0  H��tkH�M��H�T$0H����P  ����uQ�T$\��t�H�T$8M��H���(F  ��u5H�L$8L�������D$DI���c���f.�     H�L����   �Z���A�H�l$8H��$�   �P���A��  A�   L�|$HH��E(    L��   L�����
  E1�E1�H��H��A��A��P���A���   H�L$0��P������   H��E��H���GA  ����������D$D��upH��L$DH��h  ���  H��H��������I�D$A�,   H�D$ A�   �   H����  �����A��H��$�   H�l$8�-���I�D$A�-   H�D$ ��H�H�M��`  �H���   �S���H�H�L$H��   �+���H�L����   �����E1������f�f.�     AUATUWVSH��8  ��H�ω�L��M���~   H� H�l$@I�HH� ���
  A�	   �   H��I���Έ  H�SI��H���}  I���   M��t#H�� ��H�l$0H�\$(�t$ M��L��H���  H��8  [^_]A\A]�H�� L�
  H��M��H���
  M��H��H���`3  ����   H9�u�����H�UM��H���O;  ��������     I�E ��H�
  L��I��H���d1  I�U 1�H����H��h  ���  H��H����1��A���I�E L�
 E1��H��>����    H�L�����  H�H�wL��H���   ��@  H�D$     L�
 H�hH��1�1�H����  H��I�$H�H�@ H9�L� tVA������fD  H��H9�t?�L���`  L�D��H�M��p  A���  H��I��L��A�ׅ�tƋ�P����~1�H��H��([^_]A\A]A^A_�H�H��1���   ��@ AWAVAUATUWVSH��X  ��$�  I��H��I��L��L���R�����A����  L�t$pH��L��M������A��I��H��L��H������H��H�D$8��  H�=	 L��H����  H�D$8H��8��H��H�D$P�  �C�I��H�D@H��H�H�D$XH�D$hH�D$HH�D$dH�D$@H��$�  H�L��L�L$HL�D$@H���x  ����  �L$d���n  �|  H���EH�H���
  H�D$hH�HH�L$hH���H��H��H���Qc  ���	  ��H���O�  ���  �D$d�P����T$d�H�����  H�D$PL9�H��u!�  H�H����  ��uH��`L9���   H�H���Ǆ  ��t�H��$�  H��$�  �
�  H��v�;:tgH�t$@H��H��I���T���H�T$@H��A��蔜��H��u	E����   L�%o I�$H�D$8H�����  H�D$8H��t)H��   [^_]A\�D  �{:u�H��H���?���H��u�H�\$ A�#   E1��   H��H�D$8��u  H�D$8H��   [^_]A\��    L�%� H��I�$���  I�$H�����  I�$H��A�����H���  ���  I�$H��H��A��������  H�T$@H��袛������ f.�     ATUWVSH��0H��H��H��L���U������0  L�%V H��I�$���
  H��I��H���k���H��H����   H�PI�$H��L�� ���   ������   H�SH�BH+H��H����   H�{  t(�C,   E1�H�D$     E1�H��H��������C,    I�$H�����H�|$(E1�H��A�"   H�T$ H���  I�$E1�E1�H�H����   H��H���J���I�$H�����  ��H��0[^_]A\�H�A�'   A�   �   H��H�D$ �/t  �   ��H��0[^_]A\�H��  A�)   A�?   �   H��   H�D$ ��s  ��H��0[^_]A\�fD  AVAUATUWVSH��0H�BH��$�   I��H��H��M��H�h I��H�M ��i  H��H��tg� H�~  �P�t�N,����   �������   L�s H�M I�V�zW  ��uFI�غ   H����q  H��L���;r  ��P������   1�H��H��0[^_]A\A]A^��    I�VH�M �a  I��I��I�E L��H��������t���     L���  H���F,   H�D$     I��L�������I�Ƌ�F,    �P����~8M���q���L���)���@ H��  H��H� ��   �M����     I��H��  H��H� ��   ��     AWAVAUATUWVSH��(L��$�   A��H�L$pH�T$xD��L���  H�D$xD����D�H�@��L�` �s  I��I�$�u0  ��H����   ��L�=   ��H��H��H��*���L���&v  L�CM��H��H���3  ��t8H��H9�tgI�H����
  I�$H��H���Z  ��y�I��   L���o  H�L$pL���kp  L����s  H��1��0  H��H��([^_]A\A]A^A_�D  H�L$xL��賩����x@H��$�    t
H��$�   �L���rs  �M��	   L��1��o  H�L$pL����o  �L� �  �   L���o  �^���fD  AWAVAUATUWVSH��8H�-��  H�BH��H��M��L��L���  L��L�p H�E ���   ��t�   ��H��8[^_]A\A]A^A_�D  �I�M I���1c  � I�ăH�~  t�F,����   A�$�P���A�$�2  ��P�����  M��t�H�E L���  L��H�����   ���q���I�G I�H�P�T  ��uzH��$�   M���   �en  H��$�   H����n  A��P���A��&���H�E L����   �����L�y�  �F,   M��H�\$ H��H�������F,    I���*���I�U H���d���H��$�   M��I��L��L��H�D$ ��������tRA��P���A������H�E L����   ����H�E H����   �����f.�     H�E L����   ����L��$�   H��$�   M��H���Ä��M��M+FI�U E1�H���I���'������o���H��$�   M���   �   �%m  H��$�   H���m  �@���AVAUATUWVSH�� H�=��  L��$�   H��I��M��H��$�   E1�L��H���8	  ����t\�����t��t��H�� [^_]A\A]A^�H�H����@  M��I��L��H��H��$�   L��$�   H�� [^_]A\A]A^����@ H�H��L��   H��   ���  H��A���n  H��I���n  E��M��H��H�
  H��I��H������H��I��tH�PH�H��L���  ���   ��tSA�   D��H��h  [^_]A\A]A^A_�fD  H�g�  A�)   A�   �   H��A�   H�D$ ��l  � I�D$H�UL�L$pL�D$`H��L�h H���x  ��u�H�H�L$dL�5:�  H�UA�   �D$(    H�L$0L�t$ L�w�  H�����	  ���C���H�H�M ���
  H��I��H������H��I������H�PH�H��L���  ���   �������I�GH�U(L�L$xL�D$hH��H�@ H�D$PH���x  �������H�H�L$lH�U0A�   �D$(    L�t$ H�L$0L�Ƨ  H�����	  �������HcD$lH���  H��D�tE���*  �D$h9D$`t'H�E(A�$   H�D$ A�   �   H����k  �C���1�1��
  H�L$XH����S  H�T$xA��H�H�
���
  H�L$PH���S  E��A��y�H�D$pH� A�   H�D$ H��A�   �   ��j  H�L$H�����J���H�E0A�(   H�D$ �����L�l$H�   L������L��   �z���L��I��H��謢��L��D$P�l  �|$P �[  H�L$H�   L��$�   �N���H�H�M ���
  M��H��H���S���H��H��H������H��I����   HcT$dH�
  H��I��H��H�D$8����H��H��tH�PH�H��L��  ���   ��tJA�   D��H�Ĩ  [^_]A\A]A^A_�H���  A�)   A�(   �   H��A�   H�D$ ��g  �H�EI�T$L��$�   L��$�   H��L�h H���x  ��A��u���$�   �Di  I�U H�l$HD�|$`H�D$@I��H���Tf�     H�H���
  H��$�   H�HH��$�   H���H��H��I���_P  ���  ��L���]n  ���%  ��$�   �P�����$�   �H�l$HH�T$@D�|$`H��艟����A���d  H�T$@I�$H�BH+H��H�D$PH����
  H��$�   H��I��H��$�   H���.���H��H��H��$�   �k���H��H�D$`�;  H�T$`H�MH�BH�(D�hH�D$h�Ov  �   �s  H�D$HI�D$A��H����  A�F�H��$  D��$�   H����H��I�D(H��$�   H��$�   H�D$pH��$�   H�D$xH�H����
  H�L$8H��H��膒  ���@  H�L$HH����w  ���   H��$  I��H������H��I����  H�PH�H��L���  ���   ����  I�GH�WH��L�L$pL�D$xH�h H���x  ���Y  �D$P;�$�   �U  H��   �P(1�I��1�L�8�����I�D$I��H�D$hL��H�H�u  H�D$@L�u H�(�R�    H�H���
  H��$�   H�HH��$�   H���L��H��I����M  ����   H��D�E���L��������$�   �P�����$�   ��   L��H�������I�OH�T$`�qt  H;�$�   �y���D��$�   H��$  H�T$`H���&�������  H�L$@��f  H�L$H�Jr  H�H��$�   ���  � ���@ L�l$ A�   H��A�(   �   �2d  H�L$@�xf  ����� L�l$ A�   �� L�d$ A�   A�(   �   H��H��$  ��c  H�L$@A�   �*f  H�L$H�q  H��$�   H�������H�H��$�   ���  �v���I�D$A�   A�(   �   H��A�   H�D$ ��c  H�L$@��e  �>���I�$A�%   A�(   �   H��H�D$ ��c  H�L$@�e  H�H��$�   ���  �����H��$  �9���H��A�$   A�(   H�@�   H��H��$  H�D$ �fc  ����H�l$ A�"   A�(   �   H��H��$  ��b  �����H�D$8A�*   A�(   �   H��H��$  H�D$ �b  ����H��$�   H��A�   耏��������f.�     AWAVAUATUWVSH��  H��L��H��M���L�����A����   L�%J�  H�=��  H�I�$���
  �
   H���u
H��A�   I�$H�K���
  L��I��H���4���H��H��tH�PI�$H��L���  ���   ��tPA�   D��H�ĸ  [^_]A\A]A^A_� H��  A�)   A�   �   H��A�   H�D$ �a  � H�GH�SL��$�   L��$�   H��H�p I�$��x  ������I�$H�S0L��$�   L��$�   H����x  ���V���I�$H��$�   L�=��  H�S�D$(    A�   H�L$0L�|$ L���  H�����	  ������E���)  Hc�$�   H���  H��H�D�@E��t�H��u"H�CA�+   A�   �   H��H�D$ ��`  I�$H��$�   H�S8�D$(    L�|$ A�   H�L$0L�O�  H�����	  ���~���Hc�$�   H�/�  H��HЋP��t�@��u"H�C8A�+   A�   �   H��H�D$ �x`  I�$H�K ���
  L��I��H���,���H��H�D$H����H�PI�$H��L���  ���   �������H�D$HH�S(L��$�   L��$�   H��H�@H�@ H�D$hI�$��x  �������I�$H�K@���
  L��I��H������H��H�D$X�����H�PI�$H��L��  ���   ���i���H�D$XH�SHL��$�   L��$�   H��H�@H�@ H��$�   I�$��x  ��A���(�����$�   9�$�   ��  ��$�   9�$�   tlH�CHH�D$ A�$   A�   �   H���"_  �����I�$H��$�   H�S8�D$(    L�|$ A�   H�L$0L�n�  H�����	  ���e�������I�$H����
  H��$�   H��I��H�T$xH������L��H��H�D$p耈��H��H�D$`��  H�D$`H�T$HL��  L��$�   H�@H�PH��H�8Hc�$�   H�L$PH��L��P�@�Q�A1�1��r���H�L$PL��$�   1�H�A H�D$XH�A(Hc�$�   H��I�A�@�A0A�@�A41��2���H�L$PH�A@H�D�qHL�q H��$�   H�D$hH� H�D$h�xH��$�   I�$H�
���
  H��$�   H���BF  H��$�   ��I�$H�
���
  H�L$hH���F  ��A���o  ���  ��L���!���H��$�   H��$�   ��$�   �P�����$�   �o����   L���	����   L�������H�L$HH��H������H�T$PH��B��^  H�T$P�z �k  L��   �����H�D$PL�p@H��$�   H� H�D$h�xH��$�   I�$H�
���
  H��$�   H���@E  H��$�   ��I�$H�
���
  H�L$hH���E  ��A����  ����  ��L������H��$�   H��$�   ��$�   �P�����$�   �o����   L�������   L�������H�L$XH��H������H�T$PH��B8��]  H�T$P�z8 �B  �   L������H�t$HH�T$`H�N�+k  H�D$XH�H��Ӈ  ��tH�D$XH�T$`H�H�k  H�t$`H�OH����j  H��H���������uH�L$pL��A�   ����I�$H�L$x���  �3���H�C(�$���H��$�   H� A�   A�   �   H��H�D$ �;[  H�L$pL��A�   踇��I�$H�L$x���  �����H��$�   �H�D$pH��A�%   A�   �   H�D$ �wZ  I�$H�L$x���  ����H��$�   �j���H��$�   �]���H�CHH�D$ A�   A�   �   H���Z  �W���H�C(��f�f.�     AWAVAUATUWVSH��8  H��$�  H��A��M��L��H���������D$H�%  H�=��  I�H����
  H�T$PH��I��H�T$8H���
���H��H��H�D$@�z���H��I���.  L�hA�G��D$LI�]�I�^I�M �g  �D$L����   A�G�M�|��AD  H�PH�H��L���  ���   ��uHI�M L����h  I�NL����h  L9���   H�H��H�K����
  H��I��H���5���H��I��u�H�L$@H�������H�H�L$8���  �D$H   �D$HH��8  [^_]A\A]A^A_�f�H�3�  A�)   A�)   �   H��H�D$ �X  �D$H   뷐L��H��������t�H�H�L$8���  뗐I�A�%   A�)   �   H��H�D$ �X  H�H�L$8���  �D$H   �Z���@ AWAVAUATUWVSH��XH��  L��I��H��1�1�H�|$0H����  I��H�H�N`H�����  H��thfD  ���   L�x �Q���uL�x H�H��L�����  ��t(H������L��L��p  ���  L��I��L��A�օ�u3H�H����  H��u�H�L��L����h  1�H��X[^_]A\A]A^A_�A�$�B���A�$�   �H��D$,L����   �D$,�ĐAVAUATUWVSH�� H�5�  L��H��H��H����
  H��I��H���+���H��H��tiH�1�1�A��������  H��H�CL�`H�I9�u�[H��I9�tRH�H�H�JL��p  D�����  H��I��H��A�օ�t͋E �P����U ~:�   H�� [^_]A\A]A^�fD  H�H��H����h  1�H�� [^_]A\A]A^�H�H����   �f�ATUWVSH��   H��L��H��L���z�  H��v�;:tgH�t$@H��H��I�������H�L$@H��A���t~��H��u	E����   L�%��  I�$H�D$8H�����  H�D$8H��t)H��   [^_]A\�D  �{:u�H��H���~��H��u�H�\$ A�#   E1��   H��H�D$8�jU  H�D$8H��   [^_]A\��    L�%Y�  H��I�$���  I�$H�����  I�$H��A�����H�K�  ���  I�$H��H��A��������  H�L$@H���}������ f.�     WVSH��0L��H��I��H��H������H��t H�HH��贁��1�H��0[^_�f.�     H�|$ A�&   A�   �   H���T  �   H��0[^_�D  AWAVAUATUWVSH��HH�i�  L��H��H��H��H����
  H��I��H������H��H����  H�1�1����  �H�ƃ��O  ��  ����   ��ukH������H�
���H�H��H����p  �������I�E M�E@E1�H��H�P�L���H��I�������H�H��H����p  �������IcE0IcU4L�H�M��p  HЃ��H��A���  H��I��H��A�ԅ�����H�I�M(���H��p  H�	���  H��I��H���ׅ��R���I�E(M�E@A�   H��H�P����H��I���,���H�f�     H��H����p  �������D����    L�g�����H�
  H��I��H��H���n���H��t-L�H�H�����I��h  A���  H��H����1�H��8[^_]�H�|$ A�&   A�   �   H���NL  �   H��8[^_]�AWAVAUATUWVSH��8I��H�
H��H�AH+H��JH�#�  �����H�
H��D���Z  � ��u1�����H�� [^_� A��H��H��H�� [^_�+����f.�     VSH��HH��L��L�D$<L��������t�   H��H[^��    �T$<I��H�ىD$,�m����D$,H��H[^�f�WVSH��0H��L��L�D$,L���V�������t�   ��H��0[^_Ð�T$,I��H���a�����H��0[^_��    AVAUATUWVSH��@H�-��  H��H��1�1�H�E ���  H�[ H��H����   L�d$(L�l$0�DH�D$(L��   H�D$0H�CH�D$8H�E L��p  ���  H��I��H��A�օ�uH�H��t7�SM��H��������t���P����~:�   H��@[^_]A\A]A^�D  H�E H��H����h  1�H��@[^_]A\A]A^�H�E H����   �ATUWVSH�� H�-��  H��H��L���F,@   E1�H�E ��8	  ����t���F,    H�� [^_]A\��    H�E H��L��   H��   ���  H�
  H���H��H�H��tQ�{ u�H�H�K���
  H��H���t  ��u�H��t>H�H�L�+H�K��P����~2H�H��L���P0H��u�1�H��([^_]A\A]�D  L�+M��$�   �H���   ���     UWVSH��8H�51�  H��H��1�1�H����  H���   H��H��u
�MH�H��tEH�L�CH��H����p  ��t��B�����   .H��D$,H����   �D$,��    H�H��H����h  1�H��8[^_]�fD  SH��01�H�z  H��t5�R,��u.M���C,   E1�H�D$     H������H���C,    ����H��0[�@ SH��0A� H�z  H��L��t0�R,��u)M��L���  �C,   H�D$     H���1����C,    H��0[�@ SH��0A�H�z  H��L��t,�R,��u%L�L$ M��L���  �C,   H��������C,    H��0[��     AUATUWVSH��8A� H�z  H��H��L��L��t�B,��tH��H��8[^_]A\A]�fD  M��L��  �B,   H�D$     �q���H��H���F,    ��   L�%7�  L�h H��H�VL���  I�$���   ��ukH�FI�M H�@ H��)  ���u���I�M �6  �   I��H��H����B  H��H���C  I�$H���P0��P����~b1�H��H��8[^_]A\A]�I�$H����@  H��I���   ��B  H��H���dC  �f�L���  H��   �|B  H��H���AC  �����I�$H��1���   돐f.�     SH��0H�z  H��t"�B,   H�D$     E1�E1��6����C,    H��0[�f�     AWAVAUATUWVSH��HH���    H��H��E��D����  H�5��  �����H�
I��H��I��I�I��I9�u�H��([^�@ f.�     UWVSH��(H��H��tpH�YH�IH�-��  H��HqH+qH9�u�4H��H9�t'H�H��t��P�����H�E H����   H9�u�H�O�V$  H�E H��H�@0H��([^_]H��H��([^_]��    H��t������~���Y���f�     AUATUWVSH��(H�AL�jH�qL�bH�hH�XH9�u�ED  H��H9�t7H�;L��H��H���(  H�H�V�H��M��������u�H��([^_]A\A]�f��   H��([^_]A\A]�@ f.�     VSH��(H9�H��H��t0H�RH�I�#  ��uH��([^��    H��H��H��([^�/����   H��([^� UWVSH��(H�i1�H�qH�]H;]t!@ H��H��H�V�H�K�軷���H9]u��H��([^_]��     UWVSH��(1�H�H;ZH��H��t/�    H�OHcH��H�WH�IH��H���`����H9]u؉�H��([^_]� f.�     ATUWVSH�� H�H;ZH��H��M��I�1u�OH��H��H9]tAH�OHcLcM�D$H�WH�IH��O��H���y�����u�H�� [^_]A\�f.�     �   H�� [^_]A\�AUATUWVSH��8�9H��H��M��M��~H���  H�
H��tD�E�H�A��D�	~@H�� �   H��8[^_]A\A]��     I��   L���p;  1�H��8[^_]A\A]ÐH�I�  H�D$(H���   H��H�D$(HV�M��   L���;  1��f�     VSH��(H�YH��H���%  H;CtH+CH�VH�H��([^�f�1���f�f.�     ATUWVSH�� 1�H9�H��I��L��tGH�-��  �H��H�K�H����H�N��I9�t#H�H��t�D�A�@�A����H�E ��   ��H�� [^_]A\�f�VSH��(H��H������H�NH��H�FL�CH��HPH+P�`���H��H��([^�fD  AVAUATUWVSH�� H�AM�aM�IH�yH��L��H��H�hM�iM�q�#  ��tM)�H)�H)�H�H�7O�,������   H�� [^_]A\A]A^�fD  WVSH�� H�YH��H�KH+KH����  H�������H�SL�CI��H��H���S�����tH��H�� [^_�@ H��1��v�����@ VSH��(H��H�I����H�KH��H�CL�FH��HPH+P�O���H��H��([^�D  VSH��(L��H��H���\���L�KH�I9�t+L�VL�@�     Hc
H��I��I��I�H��I9�u�H��([^�@ f.�     AUATUWVSH��(�BL�aH�qL��H��A�@�@H���  ��H� ���P(Lc�1�H�GH����f  I�\$I;\$L�EL�OtL�   A�PH��H�I�yI�h H��D�lH�V�苵��I9\$A�DI��I��uɃ�H��([^_]A\A]ø   ��f�WVSH��0H��H�IH��L�D$(�U*  L�D$(�XH��H���"����H��0[^_��     AWAVAUATUWVSH��8L�aL�qI�pI�yL�j�{H��$�   I�\$I;\$�~   �L�;I��L��I��I�n�H��L���3���Hc�I��L��L�H�� L�j� H�|$ H��H��M���H���Lc�M�A�  I9\$I�@I��u�H+�$�   A� }H��8[^_]A\A]A^A_�H��$�   M��H����f�ATUWVSH�� I��H�Z�{H�IH��L��H��L����)  Hc�I��I��H�L��H�Z�}�B H�������H�H��H�H)��U���H���ݵ����H�� [^_]A\�UWVSH��hH�|$@H�l$ H��H�D$@    H�D$     I��H��H�D$H    H�D$P    H�D$X    �D$@   H�D$(    H�D$0    �D$    �����HH��  H� �P(I��I��H��H��H�������H�� H��H��h[^_]Ðf.�     UWVSH��hH��H�|$@H�IH�D$@    H�D$     H�l$ H��H�D$H    H�D$P    H�D$X    �D$@   H�D$(    H�D$0    �D$    ��'  H�Y�  I��H��H��H� H�X(�����H��I��I��H��H��H���k���H�� H��H��h[^_]Ð��������AWAVAUATUWVSH��  �z��VUUUI��L��$p  ��A�������)R��A)���   H��$p  L�l$ A�   �   L��H�X�4  ����  ��H���   ���$@ H��H���u  H;E��   ��H����~dL�CH�M��L��螪��H��I��u�H��A�   ��  D��H��  [^_]A\A]A^A_ÐH�)�  L�
A�   �f�H�C 1�1�L�pH�xH�E ���  I�^I9^I��u/�   @ H�E H��L�G�L��H����p  ��u6H��I;^tZH�E H������L��p  H�	���  L��I��H��A�ׅ�t�A�$�P���A�$�`���H�E L����   �N����     H�E L��H����h  ������f.�     AWAVAUATUWVSH��8��H��L��t5H��  �   L�
  H��H��L���  I;D$H���  H� �xt&H�t$ A�   A�@   �   H���(0  �fD  H��I+D$I�UL�ݼ  H��H��H�L�4�H�L�����   �����\���M�~ I�EI�OH�PH+PH�AH+AH��H���L���  H���%���I�T$I��I��L��I��������tI�GM��L��H�PL�@�t�����uGH�3�  A�   A�@   �   H�پ   H�D$ �V/  L���~�������H�t$ A�   �����M�D$H�UM��L��������t�H�L��H��h  ��!  H��H�����>���f�AWAVAUATUWVSH��H��H��A��L���  H��  I�hL���  H�H�����   ��A����   L�e A��I�D$H�D$8�  H�1�1�L�n���  H��A�F�L�t��DH�I��I�M����
  H�L$8H��H���]  ��x9I�T$L�H�H��L��H��A��p  ��u8M9�u�H�H��H����h  �J@ H�t$ A�   A�   �   H���.  �E �P����U H�H����   �     A�   D��H��H[^_]A\A]A^A_�fD  H�ɻ  L�
  H�L$8H��H���  ��xI�T$H�H�,��6���H�t$ A�   A�   �   H��A�   �G-  �h���f�AWAVAUATUWVSH��X  ��H��$�  A��L���  H�-�  I�XH��$�  L�̹  H�E H�����   ��A��tA�   D��H��X  [^_]A\A]A^A_�H�GL�{ H�D$@A�F�I�w���ÉD$8��  H��H�D$0������I����   A�F�H�_ L�t�(�)f�L�@M��H��L���^�������  H��L9���   H�E H����
  H��H��H����  H;Fu�L���`���H�|$ A�   H��$�  A�@   �   A�   �,  � ���f�H�	�  L�
���H�D$8H�FH;FL�ptlD�d$\M��L�t$0��     I��H9~tFI�\$�L��M�l$�L��H��N  ��y�L�L$8M��L��L���'�����u�H�E H�H�
  H�T$0H��H���t���H�|$PH��H����  H;G��   L�D$HH�L$8I��H��������uNH�L$8�����H�t$H��P����~oH��$�  H��A�   �p)  ����L������H�|$ A�   �>���H�E H�L$8H��h  �  H��$�  H�����Z���I��   H���L(  �v���H�E H����   ��     AWAVAUATUWVSH��8  ��I��A��M����   H�5��  I�XL���  H�H�����   ������   E�M�A����   H�K H�l$@D�L$<�b���A�A   �   H��I���'  D�L$<E����   A��I�_A��I��O�l/(��     H��L9���   H�H����
  L�CI��H��L���%�����u�H��L��   �(  L���I�����    �   ��H��8  [^_]A\A]A^A_�D  H���  L�
  I��H�H�K����
  L��H��I���5  H9�I��u�H��$�   L�d$ A�   A�-   �   �J'  H�L$8A�   �j�����     A�   D��H��H[^_]A\A]A^A_�fD  H��  L�
  L��H��H���
  H�L$8I��H�������H��I���Z&  H�L��H��h  �  L��H����������f.�     ATUWVSH�� ��H��t.H�ױ  L�
  L��L�d$0H��I�������L��H��A�   �   H�D$(�   H�GH��H�xH+xHyL�CH�M��H���=���H��I����   H��L����  I;Et`L�CM��L��H��螗��H��tdH����H�G�H��� ���H���  H�L$(H� H��h  �H  H��H���������A�   ����L��   L����  L��H���E   H�L$(A�   �u�������H�I�  L��H��H� ��h  �t���D  AWAVAUATUWVSH��H��I�Ή�M����   H�5�  I�xL���  H�H�����   ��A����   �ك��;  H�G M�|$H�D$8H�x�D"  H�ōC�M�d� ��    ��H���'  M9���   H�I��I�O����
  H��H��H���n	  ��y�A�   A�   �   L��H�\$ A�   �F   H���"  �A�   D��H��H[^_]A\A]A^A_�f�H�)�  L�
H��H������L�cH�H��L�5D�  I9�u�=H�H���ш��H��H������I9�t!HcH�GH�4�H;wH��r�I�L���P ��f�H��H�� [^_]A\A]A^�@ f.�     UWVSH��(H�AH��H��L��L��H9���   H9���   H)�H�FH��Hc�H��H9Fr^��tJ�G�H�l��H9�t;H��H�K�����H��H��H���`���H;Fu�H��袇��1�H��([^_]��    �   H��([^_]�f�H�9�  H�
  H��H���  H���m{����1�H��H��8[^_]�I�غ	   H���}
  H��H��1���
  �@ f.�     AVAUATUWVSH��0D��H��D��I��L���N
  H��H��H���q�����y�A�   E1�1�L��H�t$ �U  H��1��
  I��H�D$(H��L��L�x�h���H;C�n���M��   L���  L��H���s  L������1�H��8[^_]A\A]A^A_��     H�H�=�D  ���  H�xL�h A�E H�@    �@    �f�M��   �fD  1��I��	   L���  L��H����  1�� f.�     AUATUWVSH��8H��  I��M��H��L��L�D$$L�L$(H�L��L����x  ������   �T$$����   ��H�D$(%�ZfD  �D$$�P�H�D$(�T$$H����H�D$(~7H�H����
  H�T$(I��H��L�BH���@�����u�H��L��   �,  ��H��8[^_]A\A]ý   ��H��8[^_]A\A]�f�     M��H��	   �  �   H��L����  ��H��8[^_]A\A]ÐAWAVAUATUWVSH��8H�5ٔ  �   H��M��H��M��L�D$$L�L$(H�L��H����x  ��uL�D$$M��	   �u)�����H�G��Lc�H�PH+PH��I9�t.M��   L����  L��H���?  ��H��8[^_]A\A]A^A_�1��U  �T$$H��H�D$(��E�   ��H���F	  ����   D��A�   H���m	  �D$$�P�H�D$(�T$$H����H�D$(~PH�H����
  H�OH��I���������A��y�M���   L����  H�ٻ   ��  L��H���  �B����H����  M��M��H��H���������!���M���   L���  �@ f.�     WVSH��0H��L�D$(L������L�L$pL�D$(H��H��H���A�����u=H�VH��tH�RH��t
���
  I�MH��I������HcL$DH�T$H��H�T���)  I�MH�E1�L��H�H���D$    ��0  H��t3A��H��D9�tfH�M��L�D$8H�U H����x  ���f���fD  �   H��X[^_]A\A]A^A_�f.�     H�AL�aH��  L�pH�hI9�u0@ H���H��h  ���  H��H����1��f�H����I9�t�H�U H�I��M�L$�E1��D$    H��H����	  H��u��c��� H��$�   L�E �	   ��   H��$�   H���:  �5���D  H��$�   M��   �K   H��$�   H���  ����1�H��  �8�����������H��t
  H��H��t4H��{H��H�����  H�A�����H��H��H���  H��([^_]H��H��([^_]�fD  AUATUWVSH��HH��H��H��tH��u4H����   H�=�  H�H�KH���  H��H[^_]A\A]H���    H�=Ɏ  H�jH����  �KH�H�/�  L���   L�,�H����@  H�xu  I��H��H�D$(    L�^u  H�T$ L��A�ԋSH���  �KL�ЋH�ن  L��H��w  H�H�D$0    H�l$(H��H��H�T$ H�u  ��0  �+���H��H[^_]A\A]�f�UWVSH��  H�=��  H�t$0D�L$(H�͉T$ D�D$$H�\$ H�H�����  H�A�����H��$`  H�����  H��H�������H��  [^_]�D  VSH��  D�D$$H�\$ L��$P  H�ΉT$ D��H���D$(    �����H��H���@����H��  [^�D  �H�ǅ  H���f��QH�V�  H��ÐH�H+�f�     �+Ðf.�     H��8L�L$XL�L$XL�L$(�(+  H��8� ATUWVSH�� H�-ߌ  Lc�ӹ   H�E �P(H��H�E B��    �P(J��H�H9�H�NH�Ns�     �H��H9�w�H��H�� [^_]A\��    UWVSH��(H�=q�  Hc�   H��P(H��H���    �P(H�H�CH��H�CH��H��([^_]��     UWVSH��(H�-!�  H�YH+H�Ϲ   H�E �P(H��H�E ���P(H��H�Lc�H��H��Hc�H��H�VH�VH��h*  H��H��([^_]�f�f.�     VSH��(H��H��t#H�5��  H�	H��P0H�H��H�@0H��([^H��H��([^��     WVSH�� H��H�	Hc�H�CH)�H��H9�}1H�sH�U�  ��H)�H� H��Hc��P8H��H�H��H�SH�CH�� [^_��    H�H�IH9�s@ �H��H9�r��� H�H�IH9�s@ �H����H9�r���VSH��(Hc�H��H��H��HH;ArH���  H�
H��0[^_�H�%�  D�D$,H�
L�JM)�I��I)�Mc�I��M9�uI��H��L����!  ������H��(�f.�     SL�YH�L�BI9�tVL�
H��E1�fD  M9ȋvA;t4L���;t+H��I9�w�1�H��A�I9�u�Mc�I)�1�I��M9���[ø   ��E1���@ L�RL�
L�AM9�t*H�	I9�A�v;t!H���;tH��I9�w�I��M9�u�1�� �   �f.�     ATUWVSH�� H�AH�l$pI��H��L��L��H9�rH9�wYH9osH�:�  H��H+H�
L�JM)�I��I)�Mc�I��M9�uI��H��L����  ������H��(�f.�     SL�YH�L�BI9�tXL�
H��E1�fD  M9�H�vI;t5L���H;t+H��I9�w�1�H��A�I9�u�Mc�I)�1�I��M9���[ø   ��E1���f�ATUWVSH�� H�AH�l$pI��H��L��L��H9�rH9�wYH9osH��z  H��H+H�
H�
�  ���H��;5s�  �����H��Hg�  ���t�H�HA�0   H��A��H���  H�A�  M��H�U�H�M�D�A���D  L9��9���L�-pm  H�}��K�A�   H��H��L��E�����L9�r��C�����ug�L��I��I�� �����IH�H)�I�L��L�E�A�   �P��������L��I��I��  ��f��IH�H)�I�L��L�E�A�   ���������H�
  H����   H��ty�   ��1��l���f�     =  �t�=�  �t�   H��(ú   �   �
  ��	  1�H��(�1ҹ   �q
  H��H���X����   �   �U
  1������   H��(ú   �   �5
  1�������   �   �
  1�������     ATUWVSH�� �  H�ŋp�  ��u%H��t H�
D  ��  �   H�� [Ë��  ����   �t�  ��u�H�
    ("in ::ral::relation update" body line %d) invoked "break" outside of a loop       invoked "continue" outside of a loop unknown return code %d relValue expr relValue tupleVarName expr relationValue attribute    relation1 relation2 ?relation3 ...? relation attribute  relationValue ?name-value-list ...? relation    tupleVarName relationValue ?-ascending | -descending? ?attr-list?script ordering        
    ("::ral::relation foreach" body line %d) heading ?tuple1 tuple2 ...?       heading ?value-list1 value-list2 ...?   relationValue perRelation relationVarName ?attr1 type1 expr1 ... attrN typeN exprN?     attribute / type / expression arguments must be given in triples        relationValue ?oldname newname ... ?    oldname / newname arguments must be given in pairs      relationValue ?-ascending | -descending? rankAttr newAttr int relationValue ?attr ... ? relationValue ?attrName ? ?-ascending | -descending? sortAttrList? ?    relationValue attrName ?attrName2 ...? relation keyAttr valueAttr       relation arrayName keyAttr valueAttr    relation newattribute ?attr1 attr2 ...? attempt to group all attributes in the relation during group operation list attribute type      dictValue keyattr keytype valueattr valuetype dividend divisor mediator relationValue ?attrName | attr-var-list ...? subcommand ?arg? ... subcommand            relationValue tupleVarName ?attr1 type1 expr1 ... attrN typeN exprN? equal == notequal != propersubsetof < propersupersetof > subsetof <= supersetof >=         ��m    pa�m    ��m    pa�m    ��m    �a�m    ��m    �a�m    ��m    0b�m    ��m    0b�m    ��m    �b�m    ��m    �b�m    ��m    �a�m    ��m    �a�m    ��m    �b�m    ��m    �b�m                    -ascending -descending -within -start           ��m            ��m           ��m           ��m                           array assign attributes body cardinality compose create degree dict divide dunion eliminate emptyof extend extract foreach fromdict fromlist group heading insert intersect is isempty isnotempty issametype join list minus project rank rename restrict restrictwith semijoin semiminus summarize summarizeby table tag tclose times tuple uinsert ungroup union unwrap update wrap                           P�m     ��m    V�m    �Ɛm    ]�m    �v�m    h�m     Ɛm    m�m    �w�m    y�m    �s�m    ��m    @��m    ��m    �x�m    ��m    P��m    ��m    �Đm    ��m    ���m    ��m    @��m    ��m    �~�m    ��m    ���m    ��m    @��m    ��m    0��m    ��m    �Đm    ��m     ��m    ��m    �m    ��m     ��m    ��m    `��m    ��m    ���m    ��m    @��m    ��m    0y�m    �m    �y�m    �m    P�m    �m    `q�m    "�m    p��m    '�m    `��m    -�m    @��m    5�m    ���m    :�m    0��m    A�m    ���m    J�m    `��m    W�m    �n�m    `�m    @l�m    j�m    ��m    t�m    Л�m    ��m    P��m    ��m    @h�m    ��m     ��m    ��m    ���m    ��m    ���m    ��m    @��m    ��m    @��m    ��m    ���m    ��m    ���m    ��m    ���m    ��m    �z�m                    ��m            ��m                                           -using                          (  <== [ ]   (Complete)  [ ] ==>  )     relvarConstraintCleanup: unknown constraint type, %d , in relvar  
 tuple    for association  ] ==> [ for partition   is partitioned [ ])  |  for correlation   Ral_ConstraintDelete: unknown constraint type, %d       is referenced by multiple tuples        is not referenced by any tuple references multiple tuples references no tuple   is referred to by multiple tuples       is not referred to by any tuple correlation      does not form a complete correlation unknown constraint type, %d 1 + ? *       r�m    t�m    v�m    x�m    ?pattern?       relvarName ?name-value-list ...? relvarName relvarName relationValue    relvarName tupleVarName idValueList script      relvarName tupleVarName expr script ?relvar1 relvar2 ...?       relvarName ?relationValue ...? transaction option       Unknown transaction option, %d arg ?arg ...?    
    ("in ::ral::relvar eval" body line %d) option type ?arg arg ...? trace option trace type   add variable relvarName ops cmdPrefix   remove variable relvarName ops cmdPrefix info variable relvarName       suspend variable relvarName script Unknown trace option, %d add transaction cmdPrefix remove transaction cmdPrefix info transaction     suspending eval traces not implemented Unknown trace type, %d relvar ?relationValue?    relvarValue attr value ?attr2 value 2 ...?      attribute / value arguments must be given in pairs      name relvarName1 ?relvarName2 ...? script       name super super-attrs sub1 sub1-attrs ?sub2 sub2-attrs sub3 sub3-attrs ...?    relvarName relvarName ?attrName1 value1 attrName2 value2 ...? relvarName tupleVarName expr      relvarName heading id1 ?id2 id3 ...?    ?-complete? name corrRelvar corr-attr-list1 relvar1 attr-list1 spec1 corr-attr-list2 relvar2 attr-list2 spec2 * delete | info | names ?args? | member <relvar> | path <name> constraint subcommand name Unknown association command type, %d    ������������c��"��name refrngRelvar refrngAttrList refToSpec refToRelvar refToAttrList refrngSpec subcommand ?arg? ... subcommand delete exists info names member path variable transaction add remove suspend begin end rollback association constraint correlation create deleteone dunion eval identifiers insert intersect minus partition procedural restrictone set trace uinsert union unset update updateone updateper                            ��m    �.�m    ��m    @,�m    ��m    �+�m    ��m    +�m    h�m    �'�m    ��m    p%�m    ��m    P�m    �m    p�m    o�m    ���m    �m    `��m    �m    ��m    �m    �"�m    %�m    � �m    {�m    p��m    +�m      �m    ��m     ��m    5�m    ��m    @�m     �m    L�m    P�m    P�m    ��m    ��m    �m    V�m    ���m    ^�m    ��m    d�m     �m    j�m    �
�m    q�m    p�m    {�m    P��m                    ::      bad operation list: must be one or more of delete, insert, update, set or unset traceOp relvar may only be modified using "::ral::relvar" command       relvarTraceProc: trace on non-write, flags = %#x
 namespace eval   { } ", failed procedural contraint, " "      returned "continue" from procedural contraint script for constraint, "  returned "break" from procedural contraint script for constraint, "     relvar creation not allowed during a transaction unset  during identifier construction operation        
    ("in ::ral::%s %s" body line %d) association partition correlation -complete procedural    Ral_RelvarObjConstraintInfo: unknown constraint type, %d        end transaction with no beginning       
    ("in ::ral::relvar trace suspend variable" body line %d)  transaction begin end 1 + ? *                    �$�m    �$�m    �$�m    �$�m    multiplicity specification delete insert set update             �$�m           �$�m           �$�m           1#�m           �$�m                           Ral_TupleUpdate: attempt to update a shared tuple               attr1 type1 value1 ... tuple1 tuple2 tupleValue tupleValue tupleAttribute while unwrapping tuple tupleValue attr ?...?  tupleValue newAttr ?attr attr2 ...?     failed to copy attribute, "%s"  tupleValue ?attr1 value1 attr2 value2?  for attribute name / attribute value arguments  tupleValue ?oldname newname ... ?       for oldname / newname arguments tupleValue ?attr? ...   tupleValue ?name type value ... ? heading name-value-list       tupleValue ?attrName | attr-var-pair ... ? subcommand ?arg? ... subcommand assign attributes create degree eliminate equal extend extract fromlist get heading project relation rename unwrap update wrap                               �'�m    ���m    �'�m    ���m    �'�m    ���m    �'�m    0��m    �'�m    ��m    �'�m     ��m    �'�m     ��m    �'�m    0��m    �'�m    0�m    �'�m    ���m    �'�m    P��m    �'�m    ���m    �'�m    ���m    (�m    ���m    (�m    0��m    (�m     ��m    (�m    ���m                    Ral_TupleHeadingFetch: out of bounds access at offset "%d"      Ral_TupleHeadingStore: out of bounds access at offset "%d"      Ral_TupleHeadingStore: cannot find attribute name, "%s", in hash table  Ral_TupleHeadingStore: inconsistent index, expected %u, got %u  Ral_TupleHeadingPushBack: overflow      Ral_TupleHeadingAppend: out of bounds access at source offset "%d"      Ral_TupleHeadingAppend: overflow of destination                         , " " RAL OK UNKNOWN_ATTR DUPLICATE_ATTR HEADING_ERR FORMAT_ERR BAD_VALUE BAD_TYPE BAD_KEYWORD WRONG_NUM_ATTRS BAD_PAIRS_LIST DUPLICATE_OPTION NO_IDENTIFIER IDENTIFIER_FORMAT IDENTIFIER_SUBSET IDENTITY_CONSTRAINT DUP_ATTR_IN_ID DUPLICATE_TUPLE HEADING_NOT_EQUAL DEGREE_ONE DEGREE_TWO CARDINALITY_ONE BAD_TRIPLE_LIST NOT_AN_IDENTIFIER NOT_A_RELATION NOT_A_TUPLE NOT_A_PROJECTION NOT_DISJOINT NOT_UNION TOO_MANY_ATTRS TYPE_MISMATCH SINGLE_IDENTIFIER SINGLE_ATTRIBUTE WITHIN_NOT_SUBSET BAD_RANK_TYPE DUP_NAME UNKNOWN_NAME REFATTR_MISMATCH DUP_CONSTRAINT UNKNOWN_CONSTRAINT CONSTRAINTS_PRESENT BAD_MULT BAD_TRANS_OP SUPER_NAME INCOMPLETE_SPEC ONGOING_CMD ONGOING_MODIFICATION INTERNAL_ERROR                  *+�m    -+�m    :+�m    I+�m    U+�m    `+�m    j+�m    s+�m    +�m    �+�m    �+�m    �+�m    �+�m    �+�m    �+�m    �+�m    ,�m    ,�m    &,�m    1,�m    <,�m    L,�m    \,�m    n,�m    },�m    �,�m    �,�m    �,�m    �,�m    �,�m    �,�m    �,�m    �,�m    -�m    -�m    -�m    '-�m    8-�m    G-�m    Z-�m    n-�m    w-�m    �-�m    �-�m    �-�m    �-�m    �-�m    no error unknown attribute name duplicate attribute name bad heading format bad value format bad value type for value unknown data type bad type keyword        wrong number of attributes specified bad list of pairs duplicate command option relations of non-zero degree must have at least one identifier  identifiers must have at least one attribute    identifiers must not be subsets of other identifiers    tuple has duplicate values for an identifier    duplicate attribute name in identifier attribute set duplicate tuple headings not equal relation must have degree of one        relation must have degree of two        relation must have cardinality of one bad list of triples       attributes do not constitute an identifier      attribute must be of a Relation type    attribute must be of a Tuple type       relation is not a projection of the summarized relation divisor heading must be disjoint from the dividend heading      mediator heading must be a union of the dividend and divisor headings too many attributes specified     attributes must have the same type      only a single identifier may be specified       identifier must have only a single attribute    "-within" option attributes are not the subset of any identifier        attribute is not a valid type for rank operation duplicate relvar name unknown relvar name      mismatch between referential attributes duplicate constraint name unknown constraint name       relvar has constraints in place referred to identifiers can not have non-singular multiplicities        operation is not allowed during "eval" command  a super set relvar may not be named as one of its own sub sets  correlation spec is not available for a "-complete" correlation recursively invoking a relvar command outside of a transaction  recursive attempt to modify a relvar already being changed serious internal error                               X/�m    a/�m    x/�m    �/�m    �/�m    �/�m    �/�m    �/�m    �/�m    0�m    /0�m    H0�m    �0�m    �0�m    �0�m     1�m    U1�m    e1�m    x1�m    �1�m    �1�m    �1�m    2�m    82�m    `2�m    �2�m    �2�m     3�m    F3�m    h3�m    �3�m    �3�m    �3�m    84�m    i4�m    4�m    �4�m    �4�m    �4�m    �4�m    5�m    `5�m    �5�m    �5�m    6�m    P6�m    �6�m    NONE array assign association body cardinality compose constraint correlation create degree delete deleteone destroy dict divide dunion eliminate emptyof equal eval extend extract foreach fromdict fromlist get group heading identifiers insert intersect is isempty isnotempty issametype join list minus names partition procedural project rank relation rename restrict restrictone restrictwith semijoin semiminus set summarize summarizeby table tag tclose times trace tuple uinsert ungroup union unset unwrap update updateone updateper wrap              88�m    =8�m    C8�m    J8�m    V8�m    [8�m    g8�m    o8�m    z8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    �8�m    9�m    
9�m    9�m    9�m    $9�m    +9�m    59�m    89�m    @9�m    K9�m    V9�m    [9�m    `9�m    f9�m    l9�m    v9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    �9�m    :�m    
:�m    :�m    :�m     :�m    &:�m    ,:�m    3:�m    ::�m    D:�m    N:�m    unknown command relvar setfromany updatefromobj         �<�m    
:�m    �9�m    �<�m    �<�m    �<�m                    Ral_IntVectorFetch: out of bounds access at offset "%d" Ral_IntVectorStore: out of bounds access at offset "%d" Ral_IntVectorFront: access to empty vector      Ral_IntVectorPopBack: access to empty vector    Ral_IntVectorInsert: out of bounds access at offset "%d"        Ral_IntVectorErase: out of bounds access at offset "%d" Ral_IntVectorOffsetOf: out of bounds access     Ral_IntVectorCopy: out of bounds access at source offset "%d"   Ral_IntVectorCopy: out of bounds access at dest offset "%d" %d: %d
     Ral_PtrVectorFetch: out of bounds access at offset "%d" Ral_PtrVectorStore: out of bounds access at offset "%d" Ral_PtrVectorFront: access to empty vector      Ral_PtrVectorPopBack: access to empty vector    Ral_PtrVectorInsert: out of bounds access at offset "%d"        Ral_PtrVectorErase: out of bounds access at offset "%d" Ral_PtrVectorCopy: out of bounds access at source offset "%d"   Ral_PtrVectorCopy: out of bounds access at dest offset "%d" %d: %X
     interpreter uses an incompatible stubs mechanism Tcl            missing stub table pointer epoch number mismatch requires a later revision tcl::tommath  (requested version  Error loading  ):  , actual version                @��m    `��m    �ёm            Mingw-w64 runtime failure:
     Address %p has no image-section   VirtualQuery failed for %d bytes at address %p          VirtualProtect failed with code 0x%x    Unknown pseudo relocation protocol version %d.
         Unknown pseudo relocation bit size %d.
               .pdata           �m            ��m            ��m             �m            @I�m            @I�m            �A�m              �m            ̲�m            ��m            H��m            @��m            0��m            8��m             ��m            ��m            ��m             ��m            �m            �ǐm             /�m            8��m            @��m            ��m            ���m            GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119       GCC: (GNU) 6.2.1 20161119                                                                                                                                                                                                          M   p P  �  p �  �  p �    ,p    &  4p 0  �  <p �  �  Hp �  �  Pp �  �  dp �  �  hp �  �  lp �    pp    |  |p �  �  �p �    �p   T  �p `  ~  �p �  �  �p �  <  �p @  �  �p �  �  �p �  ,  �p 0  �  �p �  �  �p      �p    �  q �    q   �  q �  �  (q �    0q   �  <q �    Hq   O  Tq P  �  \q �  y   lq �   �   |q �   !  �q  !  >!  �q @!  C!  �q P!  �!  �q �!  �!  �q �!  �!  �q �!  �!  �q �!  "  �q "  "  �q  "  g"  �q p"  r"  �q �"  �"  �q �"  K#  �q P#  �#  �q  $  $  �q $  $  �q  $  �$   r �$  %  r  %  x%  r �%  �%  $r �%  @&  0r @&  �&  8r �&  '  @r '  m'  Hr p'  )  Tr  )  �)  dr �)  u*  pr �*  
V  �u V  �V  �u �V  �Y  �u  Z  �Z  �u �Z  _[  v `[  �[  v  \  �\  (v �\  _  <v  _  �_  Tv �_  �`  dv �`  ia  xv pa  �a  �v �a  �a  �v �a  #b  �v 0b  sb  �v �b  �b  �v �b  c  �v  c  �c  �v �c  �d  �v �d  e  �v  e  _f  w `f  �f  w �f  xg  ,w �g  6h  <w @h  =l  Lw @l  �n  dw �n  Uq  |w `q  �s  �w �s  }v  �w �v  �w  �w �w  wx  �w �x  ,y  �w 0y  �y  �w �y  �z  x �z  �~  x �~  G  4x P  <�  Dx @�  ��  Xx ��  W�  hx `�  ��  �x ��  �  �x ��  �  �x ��  ��  �x ��  3�  �x @�  6�  �x @�  V�  y `�  ��   y ��  ��  8y ��  �  Hy  �  /�  `y 0�  9�  py @�  I�  �y P�  ͛  �y Л  �  �y �  ��  �y ��  -�  �y 0�  ��   z ��  ;�  z @�  1�  0z @�  T�  Dz `�  b�  Xz p�  3�  lz @�  C�  �z P�  ��  �z  �  z�  �z ��  �  �z  �  �  �z �  ��  �z  �  ��  { ��  ��  ${ ��  ��  <{ ��  �  D{  �  ��  \{ ��  ��  l{ ��  ��  �{ ��  ��  �{  �  +�  �{ 0�  W�  �{ `�  ��  �{ ��  L�  �{ P�  J�  �{ P�  �  �{ �  ��  �{ ��  |�  �{ ��  ��  | ��  ��  $|  �  ��  0| ��  ��  D| ��  ��  P| ��  ��  `| ��  ��  l| ��  I�  || P�  ��  �|  �  2�  �| @�  ��  �| ��  s�  �| ��  �  �|  �  �   } �  E�  } P�  t�  } ��  )�  $} 0�  E�  4} P�  q�  8} ��  ��  @} ��  ��  H} ��  ��  T} ��  ��  `} ��  ��  h} ��  I�  p} P�  ��  |} ��  ��  �} ��  �  �}  �  y�  �} ��  ��  �} ��  �  �} �  N�  �} P�  /�  �} 0�  y�  �} ��  ��  �} ��  (�   ~ 0�  ��  ~ ��  X�  ~ `�  7�  0~ @�  ��  H~ ��  M�  T~ P�  �  d~  �  ��  t~ ��  
�  �~ �  B�  �~ P�  w�  �~ ��  ��  �~ ��  l�  �~ p�  ��  �~ ��  �  �~ �  ��  �~  �  ��   ��  ]�    `�  C�  , P�  h D p �
 \ �
  t   � � � F � P  �  b � p � � � A � P �  �   � � � � 0�    �  8� �  �" @� �" f% X� p% �' p� �' + �� + �+ �� �+ 2, �� @, �. �� �.  / Ѐ  / �/ ؀  0 �0 � �0 �1 �� �1 V2 � `2 �5 (� �5 
6 @� 6 �6 L� �6 �9 `� �9 �: t� �: o> �� p> b? �� p? �@ �� �@ JB ́ PB �C ܁ �C JE �� PE �G � �G �H  � �H zM 4� �M �S L� �S \ d�  \ ^ |� ^ _ �� _ �_ ��  ` Ca �� Pa �a Ԃ �a Xi �� `i �i �� �i xk � �k �k  � �k l ,�  l il 8� pl `m D� `m �m X� �m 6n h� @n �n t�  o �o �� �o �o �� �o <p �� @p �p �� �p �q ��  r 7r ă @r �t ̃ �t �t �  u Ru �� `u �u �� �u v � v �v � �v �v $� �v 8w 0� @w �w @� �w  x P�  x 'y `� 0y dy t� py �y �� �y z ��  z �z �� �z �z �� �z +{ �� 0{ �{ Ȅ �{ >| Ԅ @| x| � �| N} � P} �} � �} e~ � p~ ' ,� 0 �� <�  � ҁ T� �� "� h� 0� ݃ �� �� %� �� 0� .� �� 0� � �� �� � ؅  � �� �� �� ؏ � �� ��  � �� E� 0� P� � H�  � � X� � �� p� �� �� �� �� �� �� �� a� �� p� Й �� Й � ̆  � 4� ؆ @� �� ܆ �� � �  � %� � 0� �� � �� R� � `� i� (� p� �� 8� �� � D� � *� P� 0� U� X� `� � `� � �� l� �� � ��  � �� �� �� �� �� �� <� ȇ @� �� �� �� A� � P� � � � `� � `� �� $� �� �� ,� �� � 4� � /� <� 0� B� D� P� �� T�  � P� h� P� v� t� �� 3� �� @� O� �� P� Ұ �� � e� ̈ p� /� ؈ 0� (� � 0� D�  � P� �� � �� � �  � .�  � 0� �� 4� �� � D� � �� P�  � � T� � � X�  � %� \� 0� M� `� P� �� h� �� � x� � t� �� �� �� �� �� � ��  � =� �� @� `� �� `� �� �� �� � ĉ � /� Љ 0� q� ؉ �� ƺ �� к � �  �  � �  � �� � �� ;� � @� �  � �� +� (� 0� ž 8� о � L�  � (� P� 0� o� T� p� �� X� �� 4� d� @� �� t� �� � �� � |� �� �� �� �� �� �� �� �� 8� �� @� �� �� �� � ̊ � H� ܊ P� �� � �� �� � �� � �� � Z� � `� �� � �� �� � �� 8�  � @� �� ,� �� �� 4� �� � H�  � _� T� `� �� \� �� �� `� �� � d� � ~� l� �� H� t� P� �� �� �� �� �� �� �� �� �� �� �� �� �� ċ �� �� ̋ �� V� ԋ `� � �� �� V� � `� [� � `� �� � �� �� �  � � � � � �  � $�  � 0� �� $� ��  � 0�  � �� D�  � �� \� �� �� d� �� b� t� p� �� |� �� _� �� `� �� ��  � �� �� �� �� ��  � � ��  � e� �� p� �� ��  � �� Č �� �� ̌ �� B� Ԍ P� �� ܌ �� !� � 0� �� �  � � � P� V� �� `� f� �� p� u�  � � � �                                                                                                              20 B0`pP��  
 
20`pP� �  P2P  P b0`pP��            b0`   b0`   b0`   B   b   b   b0`   b0`   b0`   b   b0`   b0`   b0`   b   b0`   b0`   R0`p b   b0`   b0`   R0`p �0
 
�0`pP�
 
�0`pP� �0`   b   b      b            20`p    20`p    R0`p R0`p
 
R0`pP�       �0`p 20`p 20`p B0`   B   B   B   B0`   �0`pP   �0`   20`p
 
r0`pP� �0`pP��   �0`p 20    B   R0`p R0 20`p 20`p 20`p �0`pP        
 
20`pP� B0`   20`p 20`p b0`   b0`   B0`pP   B0`pP         B0`pP      B0`pP��   20 20 20 20             20 B0`   2
0	`pP���
 
20`pP�
 
20`pP� r
0	`pP��� B0`pP   B0`   20`p
 
r0`pP� B0`pP��   20`p R
0	`pP���	 b0`
p	P����  
 
20`pP�
  0`
p	P����	 �0`
p	P����  	 b0`
p	P����  
  0`
p	P����	 �0`
p	P����   B0`pP   B0`pP  
 
20`pP�
  0`
p	P����	 �0`
p	P����  	 �0`
p	P����   B0`pP  
 
20`pP�
  0`
p	P���� �0`pP��   20`p B0`pP��   B0`pP��  	 �0`
p	P����   B0`pP   B0`pP��   B0`pP��   B0`   B   B0`   B0`   B0`   B0`  
 
20`pP�	 B0`
p	P����   R0`p	 �0`
p	P����  
 
20`pP� �0`pP   �0`pP  
 1 0`
p	P����
 ) 0`
p	P����
 ) 0`
p	P����
 ) 0`
p	P����
 ) 0`
p	P����	 B0`
p	P����   B0`pP   B0`pP   B0`pP   B0`pP  
 - 0`
p	P���� B0`pP   B0`pP��  
 
�0`pP�	 �0`
p	P����  	 �0`
p	P����  	 �0`
p	P����   # 0`pP��
 # 0`
p	P����
 # 0`
p	P���� # 0`pP��
 % 0`
p	P����
 # 0`
p	P���� b0`pP  	 b0`
p	P����  
 
R0`pP�	 �0`
p	P����  	 " 
0	`pP���  
 # 0`
p	P����
 5 0`
p	P����
 3 0`
p	P����
 1 0`
p	P����
 # 0`
p	P����	 �0`
p	P����   B0`pP��   B0`pP��   # 0`pP��	 �0`
p	P����  	 �0`
p	P����  	 b0`
p	P����  	 �0`
p	P����  
 # 0`
p	P����
 
20`pP�	 �0`
p	P����  	 $ 
0	`pP���  
 1 0`
p	P���� B  	 " 
0	`pP���  
 
20`pP�	 $ 
0	`pP���   �0`p 20 20 20 20 B0`pP��  
 
R0`pP�
 
& 0`p   2
0	`pP��� 2
0	`pP���	 �0`
p	P����   B0`   B0`pP��   B0`   B0`pP   20`p �0`pP   2
0	`pP��� R
0	`pP���	 B0`
p	P����  	 b0`
p	P����  	 b0`
p	P����   2
0	`pP��� b0`pP��   20 B  
 
20`pP�    20 20 B0`   20`p 20 B   B0p   b0`   B0`   b0`   B0p   b0`   B0`   b0`  
 
20`pP� 20	 �0`
p	P����   B0`pP   R0	 �0`
p	P����  	 b0`
p	P����   20`p
 
20`pP�
 
20`pP� b0`pP   B0`pP   B0`   20 20`p	 B0`
p	P����  	 �0`
p	P����  
 + 0`
p	P����
 + 0`
p	P���� b0`   b0`  	 �0`
p	P����  
 5 0`
p	P����
 - 0`
p	P����
 1 0`
p	P���� B0`pP  
 % 0`
p	P����
 % 0`
p	P���� �0`pP  
 
20`pP� �0`pP��  
 % 0`
p	P����
 ' 0`
p	P���� b   r0
 % 0`
p	P����
 % 0`
p	P����
 ) 0`
p	P����	 �0`
p	P����   ' 0`pP�� B   �
0	`pP��� B   �0`pP   B0`pP  	 �0`
p	P����  	 b0`
p	P����  	 �0`
p	P����   20`p
p	P����  
 + 0`
p	P���� b0`pP��  
 
R0`pP� R
0	`pP���	 B0`
p	P����  	 b0`
p	P����   2
0	`pP���
 - 0`
p	P����
 5 0`
p	P����
 7 0`
p	P����
 ' 0`
p	P����	 �0`
p	P����   2
0	`pP���
p	P����   b0`pP  	 b0`
p	P����   20`p �0`   R0`p r
0	`pP���
 
20`pP� B0`   B0`pP��   b0`pP   R0 R0 R0 b0`pP��   R0	 �0`
p	P����   20`p B0`   B0`pP      B0`pP��   B0`   B0`pP   B0`pP  
 
20`pP� b0`pP��   B0`  
 
20`pP� B0`   2
0	`pP��� 20`p B0`   B0`   B0`pP��   R0`p	 b0`
p	P����  
 
20`pP� �0`pP   �0`pP  
 # 0`
p	P���� B0`pP��  	 B0`
p	P����   B0`pP  	 B0`
p	P����  	 b0`
p	P����  	 �0`
p	P����  
 + 0`
p	P����
 ' 0`
p	P����	 �0`
p	P����  
 
20`pP�	 �0`
p	P����  
 
20`pP�
 % 0`
p	P����	 �0`
p	P����  
 
20`pP� 20 R
0	`pP��� R0`p 2
0	`pP��� B0`pP   B0`   B0`   20 B   20`p 2
0	`pP���	 b0`
p	P����  	 b0`
p	P����  	 B0`
p	P����  	 B0`
p	P����   B0`pP��  
 
20`pP� 2
0	`pP��� r0`p 20 20 20 20 b0`pP   R
0	`pP��� b0`   r
0	`pP���	 b0`
p	P����   b0`pP��  	 b0`
p	P����   R0`p
 
& 0`p  	 �0`
p	P����      20`p B0`pP   �0`pP��   # 0`pP	 	# 0`             b  
 
20`pP� B0`pP   B0`pP   B0`   20`p       B0`   b0`   20 20 B0`   20 B0`pP��   20`p R0`p B   B0`pP   B0`pP��            B0`  
 
20`pP� b0`pP   B   0     
 
20`pP� R
0	`pP��� B0`pP   B0`pP   B0`   20`p    B0`   b0`   20 20 B0`   20 B0`pP��   20`p B         B   0  
 
20`pP� R
0	`pP��� �0`pP   �0`pP��   R0 B   B   B0`     
 
r0`pP�

�0`P   B   B0`            b0`   �0`pP��  
��0`
p	����P B  
 
20`pP� 20 B0`pP   B0`pP   20 20          20`p B   B   B   B   B   B              P                                                                                                                                                                                                                                                        kπY    F�          (� 4� @� �  �  �  R� [� j�     ral0122.dll Ral_Init Ral_SafeUnload Ral_Unload                                                                                                                                                                                                                                                                                                                                                                                                            <�         � ܱ �         `� ��                     |�     ��     ��     ��     ֳ     �     ��     �     "�     <�     L�     h�     ��     ��     ��     Ĵ     ޴     �     �     �     ,�     :�     V�     h�             x�     ��     ��     ��     ��     ��     ��     ʵ     ص     �     �     ��     ��     �     �     �     $�     .�     8�     @�     J�     T�     ^�     h�     r�     |�             |�     ��     ��     ��     ֳ     �     ��     �     "�     <�     L�     h�     ��     ��     ��     Ĵ     ޴     �     �     �     ,�     :�     V�     h�             x�     ��     ��     ��     ��     ��     ��     ʵ     ص     �     �     ��     ��     �     �     �     $�     .�     8�     @�     J�     T�     ^�     h�     r�     |�             � DeleteCriticalSection � EnterCriticalSection  �GetCurrentProcess �GetCurrentProcessId �GetCurrentThreadId  GetLastError  &GetModuleHandleA  XGetProcAddress  �GetSystemTimeAsFileTime �GetTickCount  �InitializeCriticalSection MLeaveCriticalSection  �QueryPerformanceCounter RtlAddFunctionTable RtlCaptureContext RtlLookupFunctionEntry  RtlVirtualUnwind  �SetUnhandledExceptionFilter �Sleep �TerminateProcess  �TlsGetValue �UnhandledExceptionFilter  �VirtualProtect  �VirtualQuery  O __dllonexit T __iob_func  y _amsg_exit  _initterm �_lock %_onexit �_unlock �_vsnprintf  �abort �bsearch �calloc  �free  �fwrite  �malloc  �memcmp  �memcpy  �memmove �memset  qsort signal  (strcmp  *strcpy  /strlen  2strncmp 6strrchr Qvfprintf   �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  �  � KERNEL32.dll    � � � � � � � � � � � � � � � � � � � � � � � � � � msvcrt.dll                                                                                                                                                                               �m                    �ёm    `ёm                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     Вm    `Вm    왒m    0��m                                                                                                                                                                                                                                                                                                                                                                                                                                                                     �    (�   � h    ���� �(�h�p�x��������������� � �(�0�8�@�`�h�p�x�����������ȡСء ��� �0�@�P�������������   p   ����������ȥХإ����� ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȦЦئ����� ���� �(�@�H�  (   ���� �(�0�8�@�H�P�X�`�h�p�x����������������� �� �0������ ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȣУأ����� ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȤФؤ����� ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȥХإ�� ��������������������ȯЯد�����      ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȠРؠ����� ���� �(�0�8�@�H��������� �� �0�@�@�H�P�X�`�h�p�x�������������������ȨШب����� ���� �(�0�8�@�H������ ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȮЮخ����� ���� �(�0�8�@�H�P� 0 �   ��ȦЦئ����� ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȧЧا����� ���� �(�0�`�h�p�x�������������������ȪЪت����� ���� �(�0�8�@�H�P�X�`�h�p�x�������������������ȫЫث����� ���� �(�0�8�@�H�P�X�`�h�p�x�����ȬЬج�� @ @   ������� �� �0�@�P�`�p�����������У�� �� �0�@�P�`�p� �    �0�8�   �     �(�0�8�                                                                                                                                                                                                                                                                                                                                                                                                        ,              �m                          ,    �d       �͑m    �                       ,    K�       �Αm    �                                                 ,    �T      �ϑm    �                      ,    
lc_clike �  
mb_cur_max �  
lconv_intl_refcount �  
lconv_num_refcount �  
lconv_mon_refcount �   
lconv �  (
ctype1_refcount �  0
ctype1 �%  8
pctype �+  @
pclmap �1  H
pcumap �1  P
lc_time_curr �]  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �L  	locinfo �(   	mbcinfo ��   _locale_tstruct �  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �d  
  	P1Home ��   	P2Home ��  	P3Home ��  	P4Home ��  	P5Home ��   	P6Home ��  (	ContextFlags �  0	MxCsr �  4	SegCs �  8	SegDs �  :	SegEs �  <	SegFs �  >	SegGs �  @	SegSs �  B	EFlags �  D	Dr0 ��  H	Dr1 ��  P	Dr2 ��  X	Dr3 ��  `	Dr6 ��  h	Dr7 ��  p	Rax ��  x	Rcx ��  �	Rdx ��  �	Rbx ��  �	Rsp ��  �	Rbp ��  �	Rsi ��  �	Rdi ��  �	R8 ��  �	R9 ��  �	R10 ��  �	R11 ��  �	R12 ��  �	R13 ��  �	R14 ��  �	R15 ��  �	Rip ��  ��   
VectorRegister �   
VectorControl ��  �
DebugControl ��  �
LastBranchToRip ��  �
LastBranchFromRip ��  �
LastExceptionToRip ��  �
LastExceptionFromRip ��  � WINBOOL   BYTE �7  WORD ��   DWORD �K  float LPVOID ��    __imp__pctype $[  %  __imp__wctype 3[  __imp__pwctype ?[  H  �   �  __newclmap H�  __newcumap I�  __ptlocinfo J(  __ptmbcinfo K�  __globallocalestatus L  __locale_changed M  __initiallocinfo NE  __initiallocalestructinfo OL  __imp___mb_cur_max �  signed char short int ULONG_PTR 	1�   DWORD64 	   PVOID ��  LONG   HANDLE ��  LONGLONG ��   ULONGLONG ��   _LIST_ENTRY ])
�
K   Data2 
�   Data3 
�   Data4 
�
B
R�
Z�
a�
Reserved4 z�  � XMM_SAVE_AREA32 {H  ���  	Header ��   	Legacy �q   	Xmm0 �c  �	Xmm1 �c  �	Xmm2 �c  �	Xmm3 �c  �	Xmm4 �c  �	Xmm5 �c  �
Xmm6 �c   
Xmm7 �c  
Xmm8 �c   
Xmm9 �c  0
Xmm10 �c  @
Xmm11 �c  P
Xmm12 �c  `
Xmm13 �c  p
Xmm14 �c  �
Xmm15 �c  � c  �  ?    �  FltSave ��  FloatSave ��   �   c  ,  ?   �  <  ?   !�`  Next ��  prev ��   _EXCEPTION_REGISTRATION_RECORD ��  "<   "�   `  !��  Handler �  handler �   !��  FiberData ��  Version �   _NT_TIB 8��  	ExceptionList ��   	StackBase ��  	StackLimit ��  	SubSystemTib ��  "�   	ArbitraryUserPointer ��  (	Self ��  0 �  NT_TIB ��  PNT_TIB ��  �  GUID_MAX_POWER_SAVINGS b�
�
�
$VT_BOOL $VT_VARIANT $VT_UNKNOWN 
�
�
�
�
�
�
�
�
�
�
�
  �[  �    2   _pRawDllMain #�\  �[  �[  VY  \   __xi_a &\  __xi_z '\  IY  :\   __xc_a (/\  __xc_z )/\  __dyn_tls_init_callback ,o)  ,__proc_attached .  	 ��m    __onexitbegin 0�\  IY  __onexitend 1�\  mingw_app_type 3  -pcinit ;VY  	��m    .__DllMainCRTStartup ��
  ��m    9      �
_  /w   ��      /n   �  �   /�   �2  0  0retcode ��
  �  1i__leave ���m    2��m    d  2��m    d  3��m    )d  �]  4Ru 4Qs 4Xv  3��m    �_  �]  4Ru 4Qs 4Xv  2�m    d  3*�m    �_  
^  4Ru 4Qs 4Xv  3;�m    )d  .^  4Ru 4Qs 4Xv  3T�m    �_  Q^  4Ru 4Q04Xv  2j�m    4d  3z�m    d  �^  4Ru 4Q14Xv  3��m    d  �^  4Ru 4Q04Xv  3��m    )d  �^  4Ru 4Q04Xv  3��m    �_  �^  4Ru 4Q04Xv  5��m    d  4Ru 4Q24Xv   6DllMainCRTStartup ��
  ��m    O       ��_  /w   ��  �  /n   �  X  /�   �2  �  7��m    �\  �_  4R�R4Q�Q4X�X 2�m    Ed  2�m    Pd  8�m    �\  4R�X4Q�d�4X�h  9_CRT_INIT K�
  P�m    ;      ��b  /w   K�    /n   K  �  /�   K2  z  :�   �a  ;L   V�  Q  0fiberid W�  u  0nested X  �  <c  ��m           W�`  =�c  ��m           � >�c  	  ?��m           @�c     Alc  ��m    �   Z�`  >�c  .  >�c  R  B�c   <c  0�m           ua  >Wc  u  BHc   3 �m    [d  $a  4R
� Cz�m    Ca  4R| 4Q24X}  3Y�m    jd  [a  DRDQ 3h�m    ud  ra  4RO 5��m    jd  DRDQ  E    FL   �  G��m    j       ;b  ;   ��\  �  G��m    G       b  0onexitend ��\  �  2��m    �d  5��m    �d  4Rv   Ac  ��m    �   �-b  >Wc    BHc   2��m    �d   Alc  ~�m    @   �jb  >�c  >  >�c  b  B�c   3��m    [d  �b  4R
� 5��m    ud  4RO   .pre_c_init >   �m    M       ��b  ;   @�\  �  3�m    �d  �b  4R
  5�m    �d  4Rs   _TEB HNtCurrentTeb � c  �b  I_InterlockedExchangePointer _�  fc  JTarget _fc  JValue _�   �  I_InterlockedCompareExchangePointer T�  �c  JDestination Tfc  JExChange T�  JComperand T�   I__readgsqword ��   d  JOffset �K  Kret ��    L      %MDllMain DllMain 5L�   �   7M__main __main $L�   �   #tL6   6   �MSleep Sleep $L�   �   #L�   �   "�L        "�Nfree free �Nmalloc malloc �L�   �   "� �R   �  GNU C99 6.2.1 20161119 -m64 -mtune=generic -march=x86-64 -g -O2 -std=gnu99 ./mingw-w64-crt/crt/atonexit.c �͑m    �       �  char long long unsigned int long long int intptr_t >�   wchar_t b�   short unsigned int �   int long int pthreadlocinfo �"  (  threadlocaleinfostruct `��  �   ��    	lc_codepage ��  	lc_collate_cp ��  	lc_handle �  	lc_id �C  $	lc_category �S  H
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �j  (
ctype1_refcount ��  0
ctype1 �p  8
pctype �v  @
pclmap �|  H
pcumap �|  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �/  	locinfo �   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �G  
5�  GUID_MAX_POWER_SAVINGS b�  GUID_MIN_POWER_SAVINGS c�  GUID_TYPICAL_POWER_SAVINGS d�  NO_SUBGROUP_GUID e�  ALL_POWERSCHEMES_GUID f�  GUID_POWERSCHEME_PERSONALITY g�  GUID_ACTIVE_POWERSCHEME h�  GUID_IDLE_RESILIENCY_SUBGROUP i�  GUID_IDLE_RESILIENCY_PERIOD j�  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�  GUID_VIDEO_SUBGROUP m�  GUID_VIDEO_POWERDOWN_TIMEOUT n�  GUID_VIDEO_ANNOYANCE_TIMEOUT o�  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�  GUID_VIDEO_DIM_TIMEOUT q�  GUID_VIDEO_ADAPTIVE_POWERDOWN r�  GUID_MONITOR_POWER_ON s�  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�  GUID_CONSOLE_DISPLAY_STATE x�  GUID_ALLOW_DISPLAY_REQUIRED y�  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�  GUID_DISK_SUBGROUP }�  GUID_DISK_POWERDOWN_TIMEOUT ~�  GUID_DISK_IDLE_TIMEOUT �  GUID_DISK_BURST_IGNORE_THRESHOLD ��  GUID_DISK_ADAPTIVE_POWERDOWN ��  GUID_SLEEP_SUBGROUP ��  GUID_SLEEP_IDLE_THRESHOLD ��  GUID_STANDBY_TIMEOUT ��  GUID_UNATTEND_SLEEP_TIMEOUT ��  GUID_HIBERNATE_TIMEOUT ��  GUID_HIBERNATE_FASTS4_POLICY ��  GUID_CRITICAL_POWER_TRANSITION ��  GUID_SYSTEM_AWAYMODE ��  GUID_ALLOW_AWAYMODE ��  GUID_ALLOW_STANDBY_STATES ��  GUID_ALLOW_RTC_WAKE ��  GUID_ALLOW_SYSTEM_REQUIRED ��  GUID_SYSTEM_BUTTON_SUBGROUP ��  GUID_POWERBUTTON_ACTION ��  GUID_SLEEPBUTTON_ACTION ��  GUID_USERINTERFACEBUTTON_ACTION ��  GUID_LIDCLOSE_ACTION ��  GUID_LIDOPEN_POWERSTATE ��  GUID_BATTERY_SUBGROUP ��  GUID_BATTERY_DISCHARGE_ACTION_0 ��  GUID_BATTERY_DISCHARGE_LEVEL_0 ��  GUID_BATTERY_DISCHARGE_FLAGS_0 ��  GUID_BATTERY_DISCHARGE_ACTION_1 ��  GUID_BATTERY_DISCHARGE_LEVEL_1 ��  GUID_BATTERY_DISCHARGE_FLAGS_1 ��  GUID_BATTERY_DISCHARGE_ACTION_2 ��  GUID_BATTERY_DISCHARGE_LEVEL_2 ��  GUID_BATTERY_DISCHARGE_FLAGS_2 ��  GUID_BATTERY_DISCHARGE_ACTION_3 ��  GUID_BATTERY_DISCHARGE_LEVEL_3 ��  GUID_BATTERY_DISCHARGE_FLAGS_3 ��  GUID_PROCESSOR_SETTINGS_SUBGROUP ��  GUID_PROCESSOR_THROTTLE_POLICY ��  GUID_PROCESSOR_THROTTLE_MAXIMUM ��  GUID_PROCESSOR_THROTTLE_MINIMUM ��  GUID_PROCESSOR_ALLOW_THROTTLING ��  GUID_PROCESSOR_IDLESTATE_POLICY ��  GUID_PROCESSOR_PERFSTATE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_INCREASE_POLICY ��  GUID_PROCESSOR_PERF_DECREASE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_TIME ��  GUID_PROCESSOR_PERF_DECREASE_TIME ��  GUID_PROCESSOR_PERF_TIME_CHECK ��  GUID_PROCESSOR_PERF_BOOST_POLICY ��  GUID_PROCESSOR_PERF_BOOST_MODE ��  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��  GUID_PROCESSOR_IDLE_DISABLE ��  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��  GUID_PROCESSOR_IDLE_TIME_CHECK ��  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��  GUID_PROCESSOR_PARKING_PERF_STATE ��  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��  GUID_PROCESSOR_PERF_HISTORY ��  GUID_PROCESSOR_PERF_LATENCY_HINT ��  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��  GUID_SYSTEM_COOLING_POLICY ��  GUID_LOCK_CONSOLE_ON_WAKE ��  GUID_DEVICE_IDLE_POLICY ��  GUID_ACDC_POWER_SOURCE ��  GUID_LIDSWITCH_STATE_CHANGE ��  GUID_BATTERY_PERCENTAGE_REMAINING ��  GUID_GLOBAL_USER_PRESENCE ��  GUID_SESSION_DISPLAY_STATUS ��  GUID_SESSION_USER_PRESENCE ��  GUID_IDLE_BACKGROUND_TASK ��  GUID_BACKGROUND_TASK_NOTIFICATION ��  GUID_APPLAUNCH_BUTTON ��  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��  GUID_PCIEXPRESS_ASPM_POLICY ��  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��  PPM_PERFSTATE_CHANGE_GUID ��  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��  PPM_IDLESTATE_CHANGE_GUID ��  PPM_PERFSTATES_DATA_GUID ��  PPM_IDLESTATES_DATA_GUID ��  PPM_IDLE_ACCOUNTING_GUID ��  PPM_IDLE_ACCOUNTING_EX_GUID ��  PPM_THERMALCONSTRAINT_GUID ��  PPM_PERFMON_PERFSTATE_GUID ��  PPM_THERMAL_POLICY_CHANGE_GUID ��  _RTL_CRITICAL_SECTION_DEBUG 0\\  	Type ]�   	CreatorBackTraceIndex ^�  	CriticalSection _�  	ProcessLocksList `^  	EntryCount a�   	ContentionCount b�  $	Flags c�  (	CreatorBackTraceIndexHigh d�  ,	SpareWORD e�  . _RTL_CRITICAL_SECTION (w�  	DebugInfo x     	LockCount y  	RecursionCount z  	OwningThread {  	LockSemaphore |  	SpinCount }�    \  PRTL_CRITICAL_SECTION_DEBUG f$   d  RTL_CRITICAL_SECTION ~\  CRITICAL_SECTION �*   VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
VT_BOOL VT_VARIANT VT_UNKNOWN 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec �   __MIDL_itf_msxml_0000_v0_0_s_ifspec �   LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �e  (
ctype1_refcount ��  0
ctype1 �k  8
pctype �q  @
pclmap �w  H
pcumap �w  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �/  	locinfo �   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �G  

I  IID_IRpcChannelBuffer3 #I  IID_IRpcSyntaxNegotiate =I  IID_IRpcProxyBuffer �I  IID_IRpcStubBuffer �I  IID_IPSFactoryBuffer �
I  IID_IReleaseMarshalBuffers uI  IID_IWaitMultiple �I  IID_IAddrTrackingControl <I  IID_IAddrExclusionControl �I  IID_IPipeByte I  IID_IPipeLong }I  IID_IPipeDouble �I  IID_IComThreadingInfo �I  IID_IProcessInitControl VI  IID_IFastRundown �I  IID_IMarshalingStream �I  IID_ICallbackWithNoReentrancyToApplicationSTA �I  GUID_NULL 
VT_BOOL VT_VARIANT VT_UNKNOWN 
I  IID_IOleInPlaceFrame �
I  IID_IOleInPlaceObject �I  IID_IOleInPlaceSite �I  IID_IContinue �
I  IID_ITypeComp 5I  IID_ITypeInfo �I  IID_ITypeInfo2 PI  IID_ITypeLib �I  IID_ITypeLib2 =I  IID_ITypeChangeEvents aI  IID_IErrorInfo �I  IID_ICreateErrorInfo }I  IID_ISupportErrorInfo  I  IID_ITypeFactory uI  IID_ITypeMarshal �I  IID_IRecordInfo �I  IID_IErrorLog  I  IID_IPropertyBag zI  __MIDL_itf_msxml_0000_v0_0_c_ifspec ��  __MIDL_itf_msxml_0000_v0_0_s_ifspec ��  LIBID_MSXML �Y  IID_IXMLDOMImplementation  Y  IID_IXMLDOMNode 'Y  IID_IXMLDOMDocumentFragment �Y  IID_IXMLDOMDocument fY  IID_IXMLDOMNodeList uY  IID_IXMLDOMNamedNodeMap �Y  IID_IXMLDOMCharacterData Y  IID_IXMLDOMAttribute �Y  IID_IXMLDOMElement Y  IID_IXMLDOMText �Y  IID_IXMLDOMComment %Y  IID_IXMLDOMProcessingInstruction �Y  IID_IXMLDOMCDATASection Y  IID_IXMLDOMDocumentType �Y  IID_IXMLDOMNotation Y  IID_IXMLDOMEntity Y  IID_IXMLDOMEntityReference �Y  IID_IXMLDOMParseError a	Y  IID_IXTLRuntime �	Y  DIID_XMLDOMDocumentEvents =
Y  CLSID_DOMDocument \
k  CLSID_DOMFreeThreadedDocument `
k  IID_IXMLHttpRequest g
Y  CLSID_XMLHTTPRequest �
k  IID_IXMLDSOControl �
Y  CLSID_XMLDSOControl 
I  IID_ICodeInstall �
I  IID_IUri -I  IID_IUriContainer �
  "=ϑm    �L  #R	�Αm       __do_global_dtors �Αm    5       ��L  p �L  	��m     |K  $atexit atexit ] �P   V  GNU C99 6.2.1 20161119 -m64 -mtune=generic -march=x86-64 -g -O2 -std=gnu99 ./mingw-w64-crt/crt/natstart.c 
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �Z  (
ctype1_refcount ��  0
ctype1 �`  8
pctype �f  @
pclmap �l  H
pcumap �l  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �  	locinfo ��    	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �7  
��  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
VT_BOOL VT_VARIANT VT_UNKNOWN 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec �Z   __MIDL_itf_msxml_0000_v0_0_s_ifspec �Z   LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
  char long long unsigned int long long int wchar_t b�   short unsigned int �   int long int pthreadlocinfo �    threadlocaleinfostruct `��  T  ��    	lc_codepage ��  	lc_collate_cp ��  	lc_handle ��  	lc_id �0  $	lc_category �@  H
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �W  (
ctype1_refcount ��  0
ctype1 �]  8
pctype �c  @
pclmap �i  H
pcumap �i  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �!  	locinfo ��    	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �9  
   	ExceptionFlags �	�
  �  �	5  	ExceptionAddress �	l  	NumberParameters �	�
  	ExceptionInformation �	�    �  _CONTEXT ��
  	P1Home �]   	P2Home �]  	P3Home �]  	P4Home �]  	P5Home �]   	P6Home �]  (	ContextFlags ��
  0	MxCsr ��
  4	SegCs ��
  8	SegDs ��
  :	SegEs ��
  <	SegFs ��
  >	SegGs ��
  @	SegSs ��
  B	EFlags ��
  D	Dr0 �]  H	Dr1 �]  P	Dr2 �]  X	Dr3 �]  `	Dr6 �]  h	Dr7 �]  p	Rax �]  x	Rcx �]  �	Rdx �]  �	Rbx �]  �	Rsp �]  �	Rbp �]  �	Rsi �]  �	Rdi �]  �	R8 �]  �	R9 �]  �	R10 �]  �	R11 �]  �	R12 �]  �	R13 �]  �	R14 �]  �	R15 �]  �	Rip �]  ��   
VectorRegister ��   
VectorControl �]  �
DebugControl �]  �
LastBranchToRip �]  �
LastBranchFromRip �]  �
LastExceptionToRip �]  �
LastExceptionFromRip �]  � BYTE �o  WORD ��   DWORD �  __imp__pctype $  ]  __imp__wctype 3  __imp__pwctype ?  �  P   E  __newclmap HP  __newcumap IP  __ptlocinfo J�   __ptmbcinfo K�  __globallocalestatus L�   __locale_changed M�   __initiallocinfo N  __initiallocalestructinfo O!  signed char short int UINT_PTR 	/�   (  ULONG_PTR 	1�   ULONG64 	��   DWORD64 	   PVOID ��  LONG �   LONGLONG ��   ULONGLONG ��   
   �  �y   
   �  �y   _LARGE_INTEGER �2
�
   Data2 
�   Data3 
�   Data4 
�
H
     �
      _ _XMM_SAVE_AREA32  jy  	ControlWord k�
   	StatusWord l�
  	TagWord m�
  	Reserved1 n�
  	ErrorOpcode o�
  	ErrorOffset p�
  	ErrorSelector q�
  	Reserved2 r�
  	DataOffset s�
  	DataSelector t�
  	Reserved3 u�
  	MxCsr v�
  	MxCsr_Mask w�
  	FloatRegisters x�
Reserved4 z
  � XMM_SAVE_AREA32 {  ���  	Header ��   	Legacy ��
Xmm6 ��
Xmm7 ��
Xmm8 ��
Xmm9 ��
Xmm10 ��
Xmm11 ��
Xmm12 ��
Xmm13 ��
Xmm14 ��
Xmm15 ��
   	EndAddress ��
  	UnwindData ��
   PRUNTIME_FUNCTION ��    =  �     EXCEPTION_RECORD �	;  PEXCEPTION_RECORD �	�  �  _EXCEPTION_POINTERS �	$  �  �	�   	ContextRecord �	   EXCEPTION_POINTERS �	�  $  GUID_MAX_POWER_SAVINGS b�
   dwHighDateTime ��
   FILETIME ��&  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
  !cookie j*  �P!controlPC lN  S!imgBase lN  ��!establisherFrame lN  �@%fctEntry my  �
  !hndData nl  �H&zБm    �*  =)  'R	`��m     &�Бm    �*  `)  'Rs 'Qvh'X0 &�Бm    �*  �)  'R0'Xs 'w 	`��m    'w(vx'w0vp'w80 &ёm    +  �)  'R0 &'ёm    +  �)  'R	�A�m     (-ёm    +  &;ёm    "+  �)  'Q����| (@ёm    -+   8  *     *  )__security_init_cookie 2�ϑm    �       ��*  %cookie 4(  �
  !systime 5�'  ��!perfctr 62
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �d  (
ctype1_refcount ��  0
ctype1 �j  8
pctype �p  @
pclmap �v  H
pcumap �v  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �.  	locinfo �
   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �F  
  __ptmbcinfo K�  __globallocalestatus L�   __locale_changed M�   __initiallocinfo N'  __initiallocalestructinfo O.  __imp___mb_cur_max ��  signed char short int ULONG_PTR 1�   PVOID ��  HANDLE ��  ULONGLONG ��   _GUID �  Data1 (   Data2 �   Data3 �   Data4 �   |  �     GUID K  �  double long double �  �  �      _sys_errlist 	��  _sys_nerr 	��   __imp___argc 	��  __imp___argv 	�0  6  �  __imp___wargv 	�Q  W  �  __imp__environ 	�0  __imp__wenviron 	�Q  __imp__pgmptr 	�6  __imp__wpgmptr 	�W  __imp__fmode 	��  __imp__osplatform 	 �  __imp__osver 		�  __imp__winver 	�  __imp__winmajor 	�  __imp__winminor 	$�  _amblksiz 
5�  GUID_MAX_POWER_SAVINGS b�  GUID_MIN_POWER_SAVINGS c�  GUID_TYPICAL_POWER_SAVINGS d�  NO_SUBGROUP_GUID e�  ALL_POWERSCHEMES_GUID f�  GUID_POWERSCHEME_PERSONALITY g�  GUID_ACTIVE_POWERSCHEME h�  GUID_IDLE_RESILIENCY_SUBGROUP i�  GUID_IDLE_RESILIENCY_PERIOD j�  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�  GUID_VIDEO_SUBGROUP m�  GUID_VIDEO_POWERDOWN_TIMEOUT n�  GUID_VIDEO_ANNOYANCE_TIMEOUT o�  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�  GUID_VIDEO_DIM_TIMEOUT q�  GUID_VIDEO_ADAPTIVE_POWERDOWN r�  GUID_MONITOR_POWER_ON s�  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�  GUID_CONSOLE_DISPLAY_STATE x�  GUID_ALLOW_DISPLAY_REQUIRED y�  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�  GUID_DISK_SUBGROUP }�  GUID_DISK_POWERDOWN_TIMEOUT ~�  GUID_DISK_IDLE_TIMEOUT �  GUID_DISK_BURST_IGNORE_THRESHOLD ��  GUID_DISK_ADAPTIVE_POWERDOWN ��  GUID_SLEEP_SUBGROUP ��  GUID_SLEEP_IDLE_THRESHOLD ��  GUID_STANDBY_TIMEOUT ��  GUID_UNATTEND_SLEEP_TIMEOUT ��  GUID_HIBERNATE_TIMEOUT ��  GUID_HIBERNATE_FASTS4_POLICY ��  GUID_CRITICAL_POWER_TRANSITION ��  GUID_SYSTEM_AWAYMODE ��  GUID_ALLOW_AWAYMODE ��  GUID_ALLOW_STANDBY_STATES ��  GUID_ALLOW_RTC_WAKE ��  GUID_ALLOW_SYSTEM_REQUIRED ��  GUID_SYSTEM_BUTTON_SUBGROUP ��  GUID_POWERBUTTON_ACTION ��  GUID_SLEEPBUTTON_ACTION ��  GUID_USERINTERFACEBUTTON_ACTION ��  GUID_LIDCLOSE_ACTION ��  GUID_LIDOPEN_POWERSTATE ��  GUID_BATTERY_SUBGROUP ��  GUID_BATTERY_DISCHARGE_ACTION_0 ��  GUID_BATTERY_DISCHARGE_LEVEL_0 ��  GUID_BATTERY_DISCHARGE_FLAGS_0 ��  GUID_BATTERY_DISCHARGE_ACTION_1 ��  GUID_BATTERY_DISCHARGE_LEVEL_1 ��  GUID_BATTERY_DISCHARGE_FLAGS_1 ��  GUID_BATTERY_DISCHARGE_ACTION_2 ��  GUID_BATTERY_DISCHARGE_LEVEL_2 ��  GUID_BATTERY_DISCHARGE_FLAGS_2 ��  GUID_BATTERY_DISCHARGE_ACTION_3 ��  GUID_BATTERY_DISCHARGE_LEVEL_3 ��  GUID_BATTERY_DISCHARGE_FLAGS_3 ��  GUID_PROCESSOR_SETTINGS_SUBGROUP ��  GUID_PROCESSOR_THROTTLE_POLICY ��  GUID_PROCESSOR_THROTTLE_MAXIMUM ��  GUID_PROCESSOR_THROTTLE_MINIMUM ��  GUID_PROCESSOR_ALLOW_THROTTLING ��  GUID_PROCESSOR_IDLESTATE_POLICY ��  GUID_PROCESSOR_PERFSTATE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_INCREASE_POLICY ��  GUID_PROCESSOR_PERF_DECREASE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_TIME ��  GUID_PROCESSOR_PERF_DECREASE_TIME ��  GUID_PROCESSOR_PERF_TIME_CHECK ��  GUID_PROCESSOR_PERF_BOOST_POLICY ��  GUID_PROCESSOR_PERF_BOOST_MODE ��  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��  GUID_PROCESSOR_IDLE_DISABLE ��  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��  GUID_PROCESSOR_IDLE_TIME_CHECK ��  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��  GUID_PROCESSOR_PARKING_PERF_STATE ��  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��  GUID_PROCESSOR_PERF_HISTORY ��  GUID_PROCESSOR_PERF_LATENCY_HINT ��  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��  GUID_SYSTEM_COOLING_POLICY ��  GUID_LOCK_CONSOLE_ON_WAKE ��  GUID_DEVICE_IDLE_POLICY ��  GUID_ACDC_POWER_SOURCE ��  GUID_LIDSWITCH_STATE_CHANGE ��  GUID_BATTERY_PERCENTAGE_REMAINING ��  GUID_GLOBAL_USER_PRESENCE ��  GUID_SESSION_DISPLAY_STATUS ��  GUID_SESSION_USER_PRESENCE ��  GUID_IDLE_BACKGROUND_TASK ��  GUID_BACKGROUND_TASK_NOTIFICATION ��  GUID_APPLAUNCH_BUTTON ��  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��  GUID_PCIEXPRESS_ASPM_POLICY ��  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��  PPM_PERFSTATE_CHANGE_GUID ��  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��  PPM_IDLESTATE_CHANGE_GUID ��  PPM_PERFSTATES_DATA_GUID ��  PPM_IDLESTATES_DATA_GUID ��  PPM_IDLE_ACCOUNTING_GUID ��  PPM_IDLE_ACCOUNTING_EX_GUID ��  PPM_THERMALCONSTRAINT_GUID ��  PPM_PERFMON_PERFSTATE_GUID ��  PPM_THERMAL_POLICY_CHANGE_GUID ��  PIMAGE_TLS_CALLBACK   �    *    �     _IMAGE_TLS_DIRECTORY64 (�  	StartAddressOfRawData 9   	EndAddressOfRawData 9  	AddressOfIndex 9  	AddressOfCallBacks 9  	SizeOfZeroFill �   	Characteristics �  $ IMAGE_TLS_DIRECTORY64 *  IMAGE_TLS_DIRECTORY 2�  
  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
  	�A�m    __xl_c n�  	0��m    __xl_d ��  	8��m    mingw_initltsdrot_force ��   	虒m    mingw_initltsdyn_force ��   	䙒m    mingw_initltssuo_force ��   	���m    __dyn_tls_dtor ��  `ёm    /       �{!  �  �*  D  o  ��  }  �  ��  �  �ёm    �"   __tlregdtor s�    ґm           ��!  func sY  R  __dyn_tls_init R�  "  !�  R*  !o  R�  !�  R�  "pfunc T"  "ps U�    Y  #�!  �ёm    i       ��"  $�!  �  $�!  z
-  	��m    __xi_z -  	 ��m    __xc_a -  	 ��m    __xc_z 
lc_clike �  
mb_cur_max �  
lconv_intl_refcount �
lconv_num_refcount �
lconv_mon_refcount �
lconv �  (
ctype1_refcount �
ctype1 �  8
pctype �$  @
pclmap �*  H
pcumap �*  P
lc_time_curr �V  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �E  	locinfo �!   	mbcinfo ��   _locale_tstruct �  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �]  
��	  _sys_nerr 
�  __imp___argc 
�
�>
  D
    __imp___wargv 
�_
  e
    __imp__environ 
�>
  __imp__wenviron 
�_
  __imp__pgmptr 
�D
  __imp__wpgmptr 
�e
  __imp__fmode 
�
 \  __imp__osver 
	\  __imp__winver 
\  __imp__winmajor 
\  __imp__winminor 
$\  _amblksiz 5  GUID_MAX_POWER_SAVINGS b�	  GUID_MIN_POWER_SAVINGS c�	  GUID_TYPICAL_POWER_SAVINGS d�	  NO_SUBGROUP_GUID e�	  ALL_POWERSCHEMES_GUID f�	  GUID_POWERSCHEME_PERSONALITY g�	  GUID_ACTIVE_POWERSCHEME h�	  GUID_IDLE_RESILIENCY_SUBGROUP i�	  GUID_IDLE_RESILIENCY_PERIOD j�	  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�	  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�	  GUID_VIDEO_SUBGROUP m�	  GUID_VIDEO_POWERDOWN_TIMEOUT n�	  GUID_VIDEO_ANNOYANCE_TIMEOUT o�	  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�	  GUID_VIDEO_DIM_TIMEOUT q�	  GUID_VIDEO_ADAPTIVE_POWERDOWN r�	  GUID_MONITOR_POWER_ON s�	  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�	  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�	  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�	  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�	  GUID_CONSOLE_DISPLAY_STATE x�	  GUID_ALLOW_DISPLAY_REQUIRED y�	  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�	  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�	  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�	  GUID_DISK_SUBGROUP }�	  GUID_DISK_POWERDOWN_TIMEOUT ~�	  GUID_DISK_IDLE_TIMEOUT �	  GUID_DISK_BURST_IGNORE_THRESHOLD ��	  GUID_DISK_ADAPTIVE_POWERDOWN ��	  GUID_SLEEP_SUBGROUP ��	  GUID_SLEEP_IDLE_THRESHOLD ��	  GUID_STANDBY_TIMEOUT ��	  GUID_UNATTEND_SLEEP_TIMEOUT ��	  GUID_HIBERNATE_TIMEOUT ��	  GUID_HIBERNATE_FASTS4_POLICY ��	  GUID_CRITICAL_POWER_TRANSITION ��	  GUID_SYSTEM_AWAYMODE ��	  GUID_ALLOW_AWAYMODE ��	  GUID_ALLOW_STANDBY_STATES ��	  GUID_ALLOW_RTC_WAKE ��	  GUID_ALLOW_SYSTEM_REQUIRED ��	  GUID_SYSTEM_BUTTON_SUBGROUP ��	  GUID_POWERBUTTON_ACTION ��	  GUID_SLEEPBUTTON_ACTION ��	  GUID_USERINTERFACEBUTTON_ACTION ��	  GUID_LIDCLOSE_ACTION ��	  GUID_LIDOPEN_POWERSTATE ��	  GUID_BATTERY_SUBGROUP ��	  GUID_BATTERY_DISCHARGE_ACTION_0 ��	  GUID_BATTERY_DISCHARGE_LEVEL_0 ��	  GUID_BATTERY_DISCHARGE_FLAGS_0 ��	  GUID_BATTERY_DISCHARGE_ACTION_1 ��	  GUID_BATTERY_DISCHARGE_LEVEL_1 ��	  GUID_BATTERY_DISCHARGE_FLAGS_1 ��	  GUID_BATTERY_DISCHARGE_ACTION_2 ��	  GUID_BATTERY_DISCHARGE_LEVEL_2 ��	  GUID_BATTERY_DISCHARGE_FLAGS_2 ��	  GUID_BATTERY_DISCHARGE_ACTION_3 ��	  GUID_BATTERY_DISCHARGE_LEVEL_3 ��	  GUID_BATTERY_DISCHARGE_FLAGS_3 ��	  GUID_PROCESSOR_SETTINGS_SUBGROUP ��	  GUID_PROCESSOR_THROTTLE_POLICY ��	  GUID_PROCESSOR_THROTTLE_MAXIMUM ��	  GUID_PROCESSOR_THROTTLE_MINIMUM ��	  GUID_PROCESSOR_ALLOW_THROTTLING ��	  GUID_PROCESSOR_IDLESTATE_POLICY ��	  GUID_PROCESSOR_PERFSTATE_POLICY ��	  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��	  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��	  GUID_PROCESSOR_PERF_INCREASE_POLICY ��	  GUID_PROCESSOR_PERF_DECREASE_POLICY ��	  GUID_PROCESSOR_PERF_INCREASE_TIME ��	  GUID_PROCESSOR_PERF_DECREASE_TIME ��	  GUID_PROCESSOR_PERF_TIME_CHECK ��	  GUID_PROCESSOR_PERF_BOOST_POLICY ��	  GUID_PROCESSOR_PERF_BOOST_MODE ��	  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��	  GUID_PROCESSOR_IDLE_DISABLE ��	  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��	  GUID_PROCESSOR_IDLE_TIME_CHECK ��	  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��	  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��	  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��	  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��	  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��	  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��	  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��	  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��	  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��	  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��	  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��	  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��	  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��	  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��	  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��	  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��	  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��	  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��	  GUID_PROCESSOR_PARKING_PERF_STATE ��	  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��	  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��	  GUID_PROCESSOR_PERF_HISTORY ��	  GUID_PROCESSOR_PERF_LATENCY_HINT ��	  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��	  GUID_SYSTEM_COOLING_POLICY ��	  GUID_LOCK_CONSOLE_ON_WAKE ��	  GUID_DEVICE_IDLE_POLICY ��	  GUID_ACDC_POWER_SOURCE ��	  GUID_LIDSWITCH_STATE_CHANGE ��	  GUID_BATTERY_PERCENTAGE_REMAINING ��	  GUID_GLOBAL_USER_PRESENCE ��	  GUID_SESSION_DISPLAY_STATUS ��	  GUID_SESSION_USER_PRESENCE ��	  GUID_IDLE_BACKGROUND_TASK ��	  GUID_BACKGROUND_TASK_NOTIFICATION ��	  GUID_APPLAUNCH_BUTTON ��	  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��	  GUID_PCIEXPRESS_ASPM_POLICY ��	  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��	  PPM_PERFSTATE_CHANGE_GUID ��	  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��	  PPM_IDLESTATE_CHANGE_GUID ��	  PPM_PERFSTATES_DATA_GUID ��	  PPM_IDLESTATES_DATA_GUID ��	  PPM_IDLE_ACCOUNTING_GUID ��	  PPM_IDLE_ACCOUNTING_EX_GUID ��	  PPM_THERMALCONSTRAINT_GUID ��	  PPM_PERFMON_PERFSTATE_GUID ��	  PPM_THERMAL_POLICY_CHANGE_GUID ��	  PIMAGE_TLS_CALLBACK    �  #   8   �  F  �   _RTL_CRITICAL_SECTION_DEBUG 0\0!  	Type ]:   	CreatorBackTraceIndex ^:  	CriticalSection _�!  	ProcessLocksList `	  	EntryCount aF   	ContentionCount bF  $	Flags cF  (	CreatorBackTraceIndexHigh d:  ,	SpareWORD e:  . _RTL_CRITICAL_SECTION (w�!  	DebugInfo x�!   	LockCount y�  	RecursionCount z�  	OwningThread {�  	LockSemaphore |�  	SpinCount }�    0!  PRTL_CRITICAL_SECTION_DEBUG f�!  8   RTL_CRITICAL_SECTION ~0!  CRITICAL_SECTION ��!  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
�	  IID_IRpcChannelBuffer3 #�	  IID_IRpcSyntaxNegotiate =�	  IID_IRpcProxyBuffer ��	  IID_IRpcStubBuffer ��	  IID_IPSFactoryBuffer �
�	  IID_IReleaseMarshalBuffers u�	  IID_IWaitMultiple ��	  IID_IAddrTrackingControl <�	  IID_IAddrExclusionControl ��	  IID_IPipeByte �	  IID_IPipeLong }�	  IID_IPipeDouble ��	  IID_IComThreadingInfo ��	  IID_IProcessInitControl V�	  IID_IFastRundown ��	  IID_IMarshalingStream ��	  IID_ICallbackWithNoReentrancyToApplicationSTA ��	  GUID_NULL 
VT_BOOL VT_VARIANT VT_UNKNOWN 
�	  IID_IOleInPlaceFrame �
�	  IID_IOleInPlaceObject ��	  IID_IOleInPlaceSite ��	  IID_IContinue �
�	  IID_ITypeComp 5�	  IID_ITypeInfo ��	  IID_ITypeInfo2 P�	  IID_ITypeLib ��	  IID_ITypeLib2 =�	  IID_ITypeChangeEvents a�	  IID_IErrorInfo ��	  IID_ICreateErrorInfo }�	  IID_ISupportErrorInfo  �	  IID_ITypeFactory u�	  IID_ITypeMarshal ��	  IID_IRecordInfo ��	  IID_IErrorLog  �	  IID_IPropertyBag z�	  __MIDL_itf_msxml_0000_v0_0_c_ifspec �a"  __MIDL_itf_msxml_0000_v0_0_s_ifspec �a"  LIBID_MSXML ��	  IID_IXMLDOMImplementation  �	  IID_IXMLDOMNode '�	  IID_IXMLDOMDocumentFragment ��	  IID_IXMLDOMDocument f�	  IID_IXMLDOMNodeList u�	  IID_IXMLDOMNamedNodeMap ��	  IID_IXMLDOMCharacterData �	  IID_IXMLDOMAttribute ��	  IID_IXMLDOMElement �	  IID_IXMLDOMText ��	  IID_IXMLDOMComment %�	  IID_IXMLDOMProcessingInstruction ��	  IID_IXMLDOMCDATASection �	  IID_IXMLDOMDocumentType ��	  IID_IXMLDOMNotation �	  IID_IXMLDOMEntity �	  IID_IXMLDOMEntityReference ��	  IID_IXMLDOMParseError a	�	  IID_IXTLRuntime �	�	  DIID_XMLDOMDocumentEvents =
�	  CLSID_DOMDocument \
�	  CLSID_DOMFreeThreadedDocument `
�	  IID_IXMLHttpRequest g
�	  CLSID_XMLHTTPRequest �
�	  IID_IXMLDSOControl �
�	  CLSID_XMLDSOControl 
�	  IID_ICodeInstall �
�	  IID_IUri -�	  IID_IUriContainer �
  __imp___initenv !v>
  __imp__acmdln !{D
  __imp__wcmdln !�D
  �    !��Q  __uninitialized  __initializing __initialized   �  !�rQ  �Q  __native_startup_state !��Q  __native_startup_lock !�R  R  !__native_dllmain_reason !�#  __native_vcclrit_reason !�#  __security_cookie "|�   �	  __imp__HUGE #�_R  signgam #�  __dyn_tls_init_callback    "mingw_app_type   	�m    #_encode_pointer 8  �R  $ptr 8   %_decode_pointer 8  S  $codedptr 8   &�R   ґm           �'�R  R  �\   �  GNU C99 6.2.1 20161119 -m64 -mtune=generic -march=x86-64 -g -O2 -std=gnu99 ./mingw-w64-crt/crt/pseudo-reloc.c 0ґm    �      m  __gnuc_va_list �   __builtin_va_list �   char �   va_list �   size_t #�   long long unsigned int long long int intptr_t >  ptrdiff_t X  wchar_t bB  short unsigned int B  int long int pthreadlocinfo ��  �  threadlocaleinfostruct `�$  	d  �]   
lc_codepage �b  
lc_collate_cp �b  
lc_handle �w  
lc_id ��  $
lc_category ��  Hlc_clike �]  mb_cur_max �]  lconv_intl_refcount �\  lconv_num_refcount �\  lconv_mon_refcount �\   lconv ��  (ctype1_refcount �\  0ctype1 ��  8pctype ��  @pclmap ��  Hpcumap ��  Plc_time_curr �
locinfo �p   
mbcinfo �$   _locale_tstruct �V  
wLanguage �B   
wCountry �B  
wCodePage �B   LC_ID ��   �P  
locale �P   
wlocale �V  	d  �\  
wrefcount �\   �   3  ]  unsigned int b  �  �  �   sizetype long unsigned int �  �  �     �  �   lconv �  B  X  �  unsigned char �  __lc_time_data �  _PHNDLR ?"  (  3  ]   _XCPT_ACTION A{  XcptNum B�   SigNum C]  XcptAction D   3  �   _XcptActTab G{  _XcptActTabCount H]  _XcptActTabSize I]  _First_FPE_Indx J]  _Num_FPE K]  BYTE ��  WORD �B  DWORD ��  float PBYTE �,  �  LPBYTE �,  LPVOID ��  T  b  __imp__pctype $p  �  __imp__wctype 3p  __imp__pwctype ?p  �  �   �  __newclmap H�  __newcumap I�  __ptlocinfo Jp  __ptmbcinfo K$  __globallocalestatus L]  __locale_changed M]  __initiallocinfo N�  __initiallocalestructinfo O�  __imp___mb_cur_max �\  signed char short int ULONG_PTR 1�   SIZE_T ��  PVOID ��  LONG d  HANDLE ��  
Flink ^   
Blink _   �  LIST_ENTRY `�  _GUID 	z  Data1 	�   Data2 	B  Data3 	B  Data4 	z   �  �  �   GUID 	3  �  IID 	R�  �  CLSID 	Z�  �  FMTID 	a�  �  double long double P  �  �    _sys_errlist 
��  _sys_nerr 
�]  __imp___argc 
�\  __imp___argv 
�E	  K	  P  __imp___wargv 
�f	  l	  V  __imp__environ 
�E	  __imp__wenviron 
�f	  __imp__pgmptr 
�K	  __imp__wpgmptr 
�l	  __imp__fmode 
�\  __imp__osplatform 
 U  __imp__osver 
	U  __imp__winver 
U  __imp__winmajor 
U  __imp__winminor 
$U  _amblksiz 5b  
BaseAddress !�   
AllocationBase "�  
AllocationProtect #	  
RegionSize $�  
State %	   
Protect &	  $
Type '	  ( MEMORY_BASIC_INFORMATION (d
  GUID_MAX_POWER_SAVINGS b�  GUID_MIN_POWER_SAVINGS c�  GUID_TYPICAL_POWER_SAVINGS d�  NO_SUBGROUP_GUID e�  ALL_POWERSCHEMES_GUID f�  GUID_POWERSCHEME_PERSONALITY g�  GUID_ACTIVE_POWERSCHEME h�  GUID_IDLE_RESILIENCY_SUBGROUP i�  GUID_IDLE_RESILIENCY_PERIOD j�  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�  GUID_VIDEO_SUBGROUP m�  GUID_VIDEO_POWERDOWN_TIMEOUT n�  GUID_VIDEO_ANNOYANCE_TIMEOUT o�  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�  GUID_VIDEO_DIM_TIMEOUT q�  GUID_VIDEO_ADAPTIVE_POWERDOWN r�  GUID_MONITOR_POWER_ON s�  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�  GUID_CONSOLE_DISPLAY_STATE x�  GUID_ALLOW_DISPLAY_REQUIRED y�  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�  GUID_DISK_SUBGROUP }�  GUID_DISK_POWERDOWN_TIMEOUT ~�  GUID_DISK_IDLE_TIMEOUT �  GUID_DISK_BURST_IGNORE_THRESHOLD ��  GUID_DISK_ADAPTIVE_POWERDOWN ��  GUID_SLEEP_SUBGROUP ��  GUID_SLEEP_IDLE_THRESHOLD ��  GUID_STANDBY_TIMEOUT ��  GUID_UNATTEND_SLEEP_TIMEOUT ��  GUID_HIBERNATE_TIMEOUT ��  GUID_HIBERNATE_FASTS4_POLICY ��  GUID_CRITICAL_POWER_TRANSITION ��  GUID_SYSTEM_AWAYMODE ��  GUID_ALLOW_AWAYMODE ��  GUID_ALLOW_STANDBY_STATES ��  GUID_ALLOW_RTC_WAKE ��  GUID_ALLOW_SYSTEM_REQUIRED ��  GUID_SYSTEM_BUTTON_SUBGROUP ��  GUID_POWERBUTTON_ACTION ��  GUID_SLEEPBUTTON_ACTION ��  GUID_USERINTERFACEBUTTON_ACTION ��  GUID_LIDCLOSE_ACTION ��  GUID_LIDOPEN_POWERSTATE ��  GUID_BATTERY_SUBGROUP ��  GUID_BATTERY_DISCHARGE_ACTION_0 ��  GUID_BATTERY_DISCHARGE_LEVEL_0 ��  GUID_BATTERY_DISCHARGE_FLAGS_0 ��  GUID_BATTERY_DISCHARGE_ACTION_1 ��  GUID_BATTERY_DISCHARGE_LEVEL_1 ��  GUID_BATTERY_DISCHARGE_FLAGS_1 ��  GUID_BATTERY_DISCHARGE_ACTION_2 ��  GUID_BATTERY_DISCHARGE_LEVEL_2 ��  GUID_BATTERY_DISCHARGE_FLAGS_2 ��  GUID_BATTERY_DISCHARGE_ACTION_3 ��  GUID_BATTERY_DISCHARGE_LEVEL_3 ��  GUID_BATTERY_DISCHARGE_FLAGS_3 ��  GUID_PROCESSOR_SETTINGS_SUBGROUP ��  GUID_PROCESSOR_THROTTLE_POLICY ��  GUID_PROCESSOR_THROTTLE_MAXIMUM ��  GUID_PROCESSOR_THROTTLE_MINIMUM ��  GUID_PROCESSOR_ALLOW_THROTTLING ��  GUID_PROCESSOR_IDLESTATE_POLICY ��  GUID_PROCESSOR_PERFSTATE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_INCREASE_POLICY ��  GUID_PROCESSOR_PERF_DECREASE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_TIME ��  GUID_PROCESSOR_PERF_DECREASE_TIME ��  GUID_PROCESSOR_PERF_TIME_CHECK ��  GUID_PROCESSOR_PERF_BOOST_POLICY ��  GUID_PROCESSOR_PERF_BOOST_MODE ��  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��  GUID_PROCESSOR_IDLE_DISABLE ��  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��  GUID_PROCESSOR_IDLE_TIME_CHECK ��  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��  GUID_PROCESSOR_PARKING_PERF_STATE ��  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��  GUID_PROCESSOR_PERF_HISTORY ��  GUID_PROCESSOR_PERF_LATENCY_HINT ��  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��  GUID_SYSTEM_COOLING_POLICY ��  GUID_LOCK_CONSOLE_ON_WAKE ��  GUID_DEVICE_IDLE_POLICY ��  GUID_ACDC_POWER_SOURCE ��  GUID_LIDSWITCH_STATE_CHANGE ��  GUID_BATTERY_PERCENTAGE_REMAINING ��  GUID_GLOBAL_USER_PRESENCE ��  GUID_SESSION_DISPLAY_STATUS ��  GUID_SESSION_USER_PRESENCE ��  GUID_IDLE_BACKGROUND_TASK ��  GUID_BACKGROUND_TASK_NOTIFICATION ��  GUID_APPLAUNCH_BUTTON ��  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��  GUID_PCIEXPRESS_ASPM_POLICY ��  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��  PPM_PERFSTATE_CHANGE_GUID ��  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��  PPM_IDLESTATE_CHANGE_GUID ��  PPM_PERFSTATES_DATA_GUID ��  PPM_IDLESTATES_DATA_GUID ��  PPM_IDLE_ACCOUNTING_GUID ��  PPM_IDLE_ACCOUNTING_EX_GUID ��  PPM_THERMALCONSTRAINT_GUID ��  PPM_PERFMON_PERFSTATE_GUID ��  PPM_THERMAL_POLICY_CHANGE_GUID ��  �  �  �   C   PhysicalAddress D	  VirtualSize E	   
Name B�   
Misc F�  
VirtualAddress G	  
SizeOfRawData H	  
PointerToRawData I	  
PointerToRelocations J	  
PointerToLinenumbers K	  
NumberOfRelocations L�   
NumberOfLinenumbers M�  "
Characteristics N	  $ PIMAGE_SECTION_HEADER OJ!     
Type ]�   
CreatorBackTraceIndex ^�  
CriticalSection _�"  
ProcessLocksList `   
EntryCount a	   
ContentionCount b	  $
Flags c	  (
CreatorBackTraceIndexHigh d�  ,
SpareWORD e�  . 
DebugInfo x�"   
LockCount y�  
RecursionCount z�  
OwningThread {�  
LockSemaphore |�  
SpinCount }�    H"  PRTL_CRITICAL_SECTION_DEBUG f#  P!  RTL_CRITICAL_SECTION ~H"  CRITICAL_SECTION �#  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
VT_BOOL VT_VARIANT VT_UNKNOWN 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec �y#  __MIDL_itf_msxml_0000_v0_0_s_ifspec �y#  LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
IC    �C�  �  �CJ  J  $-Gmemcpy memcpy  �]   H  GNU C99 6.2.1 20161119 -m64 -mtune=generic -march=x86-64 -g -O2 -std=gnu99 ./mingw-w64-crt/crt/crt_handler.c  בm    b      �  char size_t #�   long long unsigned int long long int wchar_t b�   short unsigned int �   int long int pthreadlocinfo �#  )  threadlocaleinfostruct `��  �  ��    	lc_codepage ��  	lc_collate_cp ��  	lc_handle �  	lc_id �?  $	lc_category �O  H
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �f  (
ctype1_refcount ��  0
ctype1 �l  8
pctype �r  @
pclmap �x  H
pcumap �x  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �0  	locinfo �   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �H  
  	NumberParameters �	P	  	ExceptionInformation �	0    >  _CONTEXT �+	  	P1Home ��
   	P2Home ��
  	P3Home ��
  	P4Home ��
  	P5Home ��
   	P6Home ��
  (	ContextFlags �P	  0	MxCsr �P	  4	SegCs �D	  8	SegDs �D	  :	SegEs �D	  <	SegFs �D	  >	SegGs �D	  @	SegSs �D	  B	EFlags �P	  D	Dr0 ��
  H	Dr1 ��
  P	Dr2 ��
  X	Dr3 ��
  `	Dr6 ��
  h	Dr7 ��
  p	Rax ��
  x	Rcx ��
  �	Rdx ��
  �	Rbx ��
  �	Rsp ��
  �	Rbp ��
  �	Rsi ��
  �	Rdi ��
  �	R8 ��
  �	R9 ��
  �	R10 ��
  �	R11 ��
  �	R12 ��
  �	R13 ��
  �	R14 ��
  �	R15 ��
  �	Rip ��
  �j   
VectorRegister ��   
VectorControl ��
  �
DebugControl ��
  �
LastBranchToRip ��
  �
LastBranchFromRip ��
  �
LastExceptionToRip ��
  �
LastExceptionFromRip ��
  � ULONG *  BYTE �~  WORD ��   DWORD �*  float PBYTE �s	  8	  LPBYTE �s	  �  __imp__pctype $�	  l  __imp__wctype 3�	  __imp__pwctype ?�	  �  �	   �	  __newclmap H�	  __newcumap I�	  __ptlocinfo J  __ptmbcinfo K�  __globallocalestatus L�   __locale_changed M�   __initiallocinfo N)  __initiallocalestructinfo O0  __imp___mb_cur_max ��  signed char short int ULONG_PTR 1�   DWORD64 ¤   PVOID �6  LONG    LONGLONG ��   ULONGLONG ��   _GUID t  Data1 *   Data2 �   Data3 �   Data4 t   ~  �     GUID -  �  IID R�  �  CLSID Z�  �  FMTID a�  �  _M128A G�  	Low H   	High I
   M128A J�  �       �  #     8	  3    _ double long double �  \      _sys_errlist 	�L  _sys_nerr 	��   __imp___argc 	��  __imp___argv 	��  �  �  __imp___wargv 	��  �  �  __imp__environ 	Щ  __imp__wenviron 	��  __imp__pgmptr 	�  __imp__wpgmptr 	��  __imp__fmode 	��  __imp__osplatform 	 �	  __imp__osver 		�	  __imp__winver 	�	  __imp__winmajor 	�	  __imp__winminor 	$�	  _amblksiz 
5�  _XMM_SAVE_AREA32  j'  	ControlWord kD	   	StatusWord lD	  	TagWord m8	  	Reserved1 n8	  	ErrorOpcode oD	  	ErrorOffset pP	  	ErrorSelector qD	  	Reserved2 rD	  	DataOffset sP	  	DataSelector tD	  	Reserved3 uD	  	MxCsr vP	  	MxCsr_Mask wP	  	FloatRegisters x   	XmmRegisters y  �
Reserved4 z#  � XMM_SAVE_AREA32 {�
Xmm6 ��   
Xmm7 ��  
Xmm8 ��   
Xmm9 ��  0
Xmm10 ��  @
Xmm11 ��  P
Xmm12 ��  `
Xmm13 ��  p
Xmm14 ��  �
Xmm15 ��  � �  j      ��  FltSave �'  FloatSave �'  ?   �  �     PCONTEXT �8  _RUNTIME_FUNCTION �  	BeginAddress �P	   	EndAddress �P	  	UnwindData �P	   RUNTIME_FUNCTION ��  �
  @     EXCEPTION_RECORD �	�  PEXCEPTION_RECORD �	s  @  _EXCEPTION_POINTERS �	�  �  �	Y   �  �	�   EXCEPTION_POINTERS �	y  y  GUID_MAX_POWER_SAVINGS b�  GUID_MIN_POWER_SAVINGS c�  GUID_TYPICAL_POWER_SAVINGS d�  NO_SUBGROUP_GUID e�  ALL_POWERSCHEMES_GUID f�  GUID_POWERSCHEME_PERSONALITY g�  GUID_ACTIVE_POWERSCHEME h�  GUID_IDLE_RESILIENCY_SUBGROUP i�  GUID_IDLE_RESILIENCY_PERIOD j�  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�  GUID_VIDEO_SUBGROUP m�  GUID_VIDEO_POWERDOWN_TIMEOUT n�  GUID_VIDEO_ANNOYANCE_TIMEOUT o�  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�  GUID_VIDEO_DIM_TIMEOUT q�  GUID_VIDEO_ADAPTIVE_POWERDOWN r�  GUID_MONITOR_POWER_ON s�  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�  GUID_CONSOLE_DISPLAY_STATE x�  GUID_ALLOW_DISPLAY_REQUIRED y�  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�  GUID_DISK_SUBGROUP }�  GUID_DISK_POWERDOWN_TIMEOUT ~�  GUID_DISK_IDLE_TIMEOUT �  GUID_DISK_BURST_IGNORE_THRESHOLD ��  GUID_DISK_ADAPTIVE_POWERDOWN ��  GUID_SLEEP_SUBGROUP ��  GUID_SLEEP_IDLE_THRESHOLD ��  GUID_STANDBY_TIMEOUT ��  GUID_UNATTEND_SLEEP_TIMEOUT ��  GUID_HIBERNATE_TIMEOUT ��  GUID_HIBERNATE_FASTS4_POLICY ��  GUID_CRITICAL_POWER_TRANSITION ��  GUID_SYSTEM_AWAYMODE ��  GUID_ALLOW_AWAYMODE ��  GUID_ALLOW_STANDBY_STATES ��  GUID_ALLOW_RTC_WAKE ��  GUID_ALLOW_SYSTEM_REQUIRED ��  GUID_SYSTEM_BUTTON_SUBGROUP ��  GUID_POWERBUTTON_ACTION ��  GUID_SLEEPBUTTON_ACTION ��  GUID_USERINTERFACEBUTTON_ACTION ��  GUID_LIDCLOSE_ACTION ��  GUID_LIDOPEN_POWERSTATE ��  GUID_BATTERY_SUBGROUP ��  GUID_BATTERY_DISCHARGE_ACTION_0 ��  GUID_BATTERY_DISCHARGE_LEVEL_0 ��  GUID_BATTERY_DISCHARGE_FLAGS_0 ��  GUID_BATTERY_DISCHARGE_ACTION_1 ��  GUID_BATTERY_DISCHARGE_LEVEL_1 ��  GUID_BATTERY_DISCHARGE_FLAGS_1 ��  GUID_BATTERY_DISCHARGE_ACTION_2 ��  GUID_BATTERY_DISCHARGE_LEVEL_2 ��  GUID_BATTERY_DISCHARGE_FLAGS_2 ��  GUID_BATTERY_DISCHARGE_ACTION_3 ��  GUID_BATTERY_DISCHARGE_LEVEL_3 ��  GUID_BATTERY_DISCHARGE_FLAGS_3 ��  GUID_PROCESSOR_SETTINGS_SUBGROUP ��  GUID_PROCESSOR_THROTTLE_POLICY ��  GUID_PROCESSOR_THROTTLE_MAXIMUM ��  GUID_PROCESSOR_THROTTLE_MINIMUM ��  GUID_PROCESSOR_ALLOW_THROTTLING ��  GUID_PROCESSOR_IDLESTATE_POLICY ��  GUID_PROCESSOR_PERFSTATE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_INCREASE_POLICY ��  GUID_PROCESSOR_PERF_DECREASE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_TIME ��  GUID_PROCESSOR_PERF_DECREASE_TIME ��  GUID_PROCESSOR_PERF_TIME_CHECK ��  GUID_PROCESSOR_PERF_BOOST_POLICY ��  GUID_PROCESSOR_PERF_BOOST_MODE ��  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��  GUID_PROCESSOR_IDLE_DISABLE ��  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��  GUID_PROCESSOR_IDLE_TIME_CHECK ��  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��  GUID_PROCESSOR_PARKING_PERF_STATE ��  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��  GUID_PROCESSOR_PERF_HISTORY ��  GUID_PROCESSOR_PERF_LATENCY_HINT ��  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��  GUID_SYSTEM_COOLING_POLICY ��  GUID_LOCK_CONSOLE_ON_WAKE ��  GUID_DEVICE_IDLE_POLICY ��  GUID_ACDC_POWER_SOURCE ��  GUID_LIDSWITCH_STATE_CHANGE ��  GUID_BATTERY_PERCENTAGE_REMAINING ��  GUID_GLOBAL_USER_PRESENCE ��  GUID_SESSION_DISPLAY_STATUS ��  GUID_SESSION_USER_PRESENCE ��  GUID_IDLE_BACKGROUND_TASK ��  GUID_BACKGROUND_TASK_NOTIFICATION ��  GUID_APPLAUNCH_BUTTON ��  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��  GUID_PCIEXPRESS_ASPM_POLICY ��  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��  PPM_PERFSTATE_CHANGE_GUID ��  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��  PPM_IDLESTATE_CHANGE_GUID ��  PPM_PERFSTATES_DATA_GUID ��  PPM_IDLESTATES_DATA_GUID ��  PPM_IDLE_ACCOUNTING_GUID ��  PPM_IDLE_ACCOUNTING_EX_GUID ��  PPM_THERMALCONSTRAINT_GUID ��  PPM_PERFMON_PERFSTATE_GUID ��  PPM_THERMAL_POLICY_CHANGE_GUID ��  8	  �&     _IMAGE_DOS_HEADER @��'  	e_magic �D	   	e_cblp �D	  	e_cp �D	  	e_crlc �D	  	e_cparhdr �D	  	e_minalloc �D	  
	e_maxalloc �D	  	e_ss �D	  	e_sp �D	  	e_csum �D	  	e_ip �D	  	e_cs �D	  	e_lfarlc �D	  	e_ovno �D	  	e_res ��'  	e_oemid �D	  $	e_oeminfo �D	  &	e_res2 ��'  (	e_lfanew ��
  < D	  �'     D	  �'    	 IMAGE_DOS_HEADER ��&  CE(  PhysicalAddress DP	  VirtualSize EP	   _IMAGE_SECTION_HEADER (AY)  	Name Bq&   	Misc F(  	VirtualAddress GP	  	SizeOfRawData HP	  	PointerToRawData IP	  	PointerToRelocations JP	  	PointerToLinenumbers KP	  	NumberOfRelocations LD	   	NumberOfLinenumbers MD	  "	Characteristics NP	  $ PIMAGE_SECTION_HEADER Ow)  E(  �)  �
  �)  �   PTOP_LEVEL_EXCEPTION_FILTER })  LPTOP_LEVEL_EXCEPTION_FILTER �)  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
 VT_BOOL  VT_VARIANT  VT_UNKNOWN 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec �*  __MIDL_itf_msxml_0000_v0_0_s_ifspec �*  LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
lc_codepage ��  
lc_collate_cp ��  
lc_handle �  
lc_id �2  $
lc_category �B  Hlc_clike ��   mb_cur_max ��   lconv_intl_refcount ��  lconv_num_refcount ��  lconv_mon_refcount ��   lconv �Y  (ctype1_refcount ��  0ctype1 �_  8pctype �e  @pclmap �k  Hpcumap �k  Plc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct 
locinfo ��    
mbcinfo ��   _locale_tstruct ��  
wLanguage ��    
wCountry ��   
wCodePage ��    LC_ID �;   ��  
locale ��   
wlocale ��  	=  ��  
wrefcount ��   �   �   �   unsigned int        sizetype long unsigned int �  B     �  R     lconv R  �   �   �  unsigned char q  __lc_time_data �  _PHNDLR ?�  �  �  �    _XCPT_ACTION A  XcptNum B   SigNum C�   XcptAction D�   �     _XcptActTab G  _XcptActTabCount H�   _XcptActTabSize I�   _First_FPE_Indx J�   _Num_FPE K�   WINBOOL �   WORD ��   DWORD �  float LPVOID �y  �  __imp__pctype $�  _  __imp__wctype 3�  __imp__pwctype ?�  �       __newclmap H  __newcumap I  __ptlocinfo J�   __ptmbcinfo K�  __globallocalestatus L�   __locale_changed M�   __initiallocinfo N  __initiallocalestructinfo O#  __imp___mb_cur_max ��  signed char short int ULONG_PTR 1�   LONG �   HANDLE �y  
Flink ^d   
Blink _d   0  LIST_ENTRY `0  _GUID �  Data1    Data2 �   Data3 �   Data4 �   q  �     GUID }  �  double long double �        _sys_errlist 	��  _sys_nerr 	��   __imp___argc 	��  __imp___argv 	�[  a  �  __imp___wargv 	�|  �  �  __imp__environ 	�[  __imp__wenviron 	�|  __imp__pgmptr 	�a  __imp__wpgmptr 	�  __imp__fmode 	��  __imp__osplatform 	 �  __imp__osver 		�  __imp__winver 	�  __imp__winmajor 	�  __imp__winminor 	$�  _amblksiz 
5�  GUID_MAX_POWER_SAVINGS b�  GUID_MIN_POWER_SAVINGS c�  GUID_TYPICAL_POWER_SAVINGS d�  NO_SUBGROUP_GUID e�  ALL_POWERSCHEMES_GUID f�  GUID_POWERSCHEME_PERSONALITY g�  GUID_ACTIVE_POWERSCHEME h�  GUID_IDLE_RESILIENCY_SUBGROUP i�  GUID_IDLE_RESILIENCY_PERIOD j�  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�  GUID_VIDEO_SUBGROUP m�  GUID_VIDEO_POWERDOWN_TIMEOUT n�  GUID_VIDEO_ANNOYANCE_TIMEOUT o�  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�  GUID_VIDEO_DIM_TIMEOUT q�  GUID_VIDEO_ADAPTIVE_POWERDOWN r�  GUID_MONITOR_POWER_ON s�  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�  GUID_CONSOLE_DISPLAY_STATE x�  GUID_ALLOW_DISPLAY_REQUIRED y�  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�  GUID_DISK_SUBGROUP }�  GUID_DISK_POWERDOWN_TIMEOUT ~�  GUID_DISK_IDLE_TIMEOUT �  GUID_DISK_BURST_IGNORE_THRESHOLD ��  GUID_DISK_ADAPTIVE_POWERDOWN ��  GUID_SLEEP_SUBGROUP ��  GUID_SLEEP_IDLE_THRESHOLD ��  GUID_STANDBY_TIMEOUT ��  GUID_UNATTEND_SLEEP_TIMEOUT ��  GUID_HIBERNATE_TIMEOUT ��  GUID_HIBERNATE_FASTS4_POLICY ��  GUID_CRITICAL_POWER_TRANSITION ��  GUID_SYSTEM_AWAYMODE ��  GUID_ALLOW_AWAYMODE ��  GUID_ALLOW_STANDBY_STATES ��  GUID_ALLOW_RTC_WAKE ��  GUID_ALLOW_SYSTEM_REQUIRED ��  GUID_SYSTEM_BUTTON_SUBGROUP ��  GUID_POWERBUTTON_ACTION ��  GUID_SLEEPBUTTON_ACTION ��  GUID_USERINTERFACEBUTTON_ACTION ��  GUID_LIDCLOSE_ACTION ��  GUID_LIDOPEN_POWERSTATE ��  GUID_BATTERY_SUBGROUP ��  GUID_BATTERY_DISCHARGE_ACTION_0 ��  GUID_BATTERY_DISCHARGE_LEVEL_0 ��  GUID_BATTERY_DISCHARGE_FLAGS_0 ��  GUID_BATTERY_DISCHARGE_ACTION_1 ��  GUID_BATTERY_DISCHARGE_LEVEL_1 ��  GUID_BATTERY_DISCHARGE_FLAGS_1 ��  GUID_BATTERY_DISCHARGE_ACTION_2 ��  GUID_BATTERY_DISCHARGE_LEVEL_2 ��  GUID_BATTERY_DISCHARGE_FLAGS_2 ��  GUID_BATTERY_DISCHARGE_ACTION_3 ��  GUID_BATTERY_DISCHARGE_LEVEL_3 ��  GUID_BATTERY_DISCHARGE_FLAGS_3 ��  GUID_PROCESSOR_SETTINGS_SUBGROUP ��  GUID_PROCESSOR_THROTTLE_POLICY ��  GUID_PROCESSOR_THROTTLE_MAXIMUM ��  GUID_PROCESSOR_THROTTLE_MINIMUM ��  GUID_PROCESSOR_ALLOW_THROTTLING ��  GUID_PROCESSOR_IDLESTATE_POLICY ��  GUID_PROCESSOR_PERFSTATE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_INCREASE_POLICY ��  GUID_PROCESSOR_PERF_DECREASE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_TIME ��  GUID_PROCESSOR_PERF_DECREASE_TIME ��  GUID_PROCESSOR_PERF_TIME_CHECK ��  GUID_PROCESSOR_PERF_BOOST_POLICY ��  GUID_PROCESSOR_PERF_BOOST_MODE ��  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��  GUID_PROCESSOR_IDLE_DISABLE ��  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��  GUID_PROCESSOR_IDLE_TIME_CHECK ��  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��  GUID_PROCESSOR_PARKING_PERF_STATE ��  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��  GUID_PROCESSOR_PERF_HISTORY ��  GUID_PROCESSOR_PERF_LATENCY_HINT ��  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��  GUID_SYSTEM_COOLING_POLICY ��  GUID_LOCK_CONSOLE_ON_WAKE ��  GUID_DEVICE_IDLE_POLICY ��  GUID_ACDC_POWER_SOURCE ��  GUID_LIDSWITCH_STATE_CHANGE ��  GUID_BATTERY_PERCENTAGE_REMAINING ��  GUID_GLOBAL_USER_PRESENCE ��  GUID_SESSION_DISPLAY_STATUS ��  GUID_SESSION_USER_PRESENCE ��  GUID_IDLE_BACKGROUND_TASK ��  GUID_BACKGROUND_TASK_NOTIFICATION ��  GUID_APPLAUNCH_BUTTON ��  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��  GUID_PCIEXPRESS_ASPM_POLICY ��  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��  PPM_PERFSTATE_CHANGE_GUID ��  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��  PPM_IDLESTATE_CHANGE_GUID ��  PPM_PERFSTATES_DATA_GUID ��  PPM_IDLESTATES_DATA_GUID ��  PPM_IDLE_ACCOUNTING_GUID ��  PPM_IDLE_ACCOUNTING_EX_GUID ��  PPM_THERMALCONSTRAINT_GUID ��  PPM_PERFMON_PERFSTATE_GUID ��  PPM_THERMAL_POLICY_CHANGE_GUID ��  
Type ]�   
CreatorBackTraceIndex ^�  
CriticalSection _�  
ProcessLocksList `j  
EntryCount a�   
ContentionCount b�  $
Flags c�  (
CreatorBackTraceIndexHigh d�  ,
SpareWORD e�  . 
DebugInfo x�   
LockCount y  
RecursionCount z  
OwningThread {!  
LockSemaphore |!  
SpinCount }      PRTL_CRITICAL_SECTION_DEBUG f�    RTL_CRITICAL_SECTION ~  CRITICAL_SECTION ��     %   y   VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
  
  
/F  F  
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �f  (
ctype1_refcount ��  0
ctype1 �l  8
pctype �r  @
pclmap �x  H
pcumap �x  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �0  	locinfo �   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �H  
5�  GUID_MAX_POWER_SAVINGS b�  GUID_MIN_POWER_SAVINGS c�  GUID_TYPICAL_POWER_SAVINGS d�  NO_SUBGROUP_GUID e�  ALL_POWERSCHEMES_GUID f�  GUID_POWERSCHEME_PERSONALITY g�  GUID_ACTIVE_POWERSCHEME h�  GUID_IDLE_RESILIENCY_SUBGROUP i�  GUID_IDLE_RESILIENCY_PERIOD j�  GUID_DISK_COALESCING_POWERDOWN_TIMEOUT k�  GUID_EXECUTION_REQUIRED_REQUEST_TIMEOUT l�  GUID_VIDEO_SUBGROUP m�  GUID_VIDEO_POWERDOWN_TIMEOUT n�  GUID_VIDEO_ANNOYANCE_TIMEOUT o�  GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE p�  GUID_VIDEO_DIM_TIMEOUT q�  GUID_VIDEO_ADAPTIVE_POWERDOWN r�  GUID_MONITOR_POWER_ON s�  GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS t�  GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS u�  GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS v�  GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS w�  GUID_CONSOLE_DISPLAY_STATE x�  GUID_ALLOW_DISPLAY_REQUIRED y�  GUID_VIDEO_CONSOLE_LOCK_TIMEOUT z�  GUID_ADAPTIVE_POWER_BEHAVIOR_SUBGROUP {�  GUID_NON_ADAPTIVE_INPUT_TIMEOUT |�  GUID_DISK_SUBGROUP }�  GUID_DISK_POWERDOWN_TIMEOUT ~�  GUID_DISK_IDLE_TIMEOUT �  GUID_DISK_BURST_IGNORE_THRESHOLD ��  GUID_DISK_ADAPTIVE_POWERDOWN ��  GUID_SLEEP_SUBGROUP ��  GUID_SLEEP_IDLE_THRESHOLD ��  GUID_STANDBY_TIMEOUT ��  GUID_UNATTEND_SLEEP_TIMEOUT ��  GUID_HIBERNATE_TIMEOUT ��  GUID_HIBERNATE_FASTS4_POLICY ��  GUID_CRITICAL_POWER_TRANSITION ��  GUID_SYSTEM_AWAYMODE ��  GUID_ALLOW_AWAYMODE ��  GUID_ALLOW_STANDBY_STATES ��  GUID_ALLOW_RTC_WAKE ��  GUID_ALLOW_SYSTEM_REQUIRED ��  GUID_SYSTEM_BUTTON_SUBGROUP ��  GUID_POWERBUTTON_ACTION ��  GUID_SLEEPBUTTON_ACTION ��  GUID_USERINTERFACEBUTTON_ACTION ��  GUID_LIDCLOSE_ACTION ��  GUID_LIDOPEN_POWERSTATE ��  GUID_BATTERY_SUBGROUP ��  GUID_BATTERY_DISCHARGE_ACTION_0 ��  GUID_BATTERY_DISCHARGE_LEVEL_0 ��  GUID_BATTERY_DISCHARGE_FLAGS_0 ��  GUID_BATTERY_DISCHARGE_ACTION_1 ��  GUID_BATTERY_DISCHARGE_LEVEL_1 ��  GUID_BATTERY_DISCHARGE_FLAGS_1 ��  GUID_BATTERY_DISCHARGE_ACTION_2 ��  GUID_BATTERY_DISCHARGE_LEVEL_2 ��  GUID_BATTERY_DISCHARGE_FLAGS_2 ��  GUID_BATTERY_DISCHARGE_ACTION_3 ��  GUID_BATTERY_DISCHARGE_LEVEL_3 ��  GUID_BATTERY_DISCHARGE_FLAGS_3 ��  GUID_PROCESSOR_SETTINGS_SUBGROUP ��  GUID_PROCESSOR_THROTTLE_POLICY ��  GUID_PROCESSOR_THROTTLE_MAXIMUM ��  GUID_PROCESSOR_THROTTLE_MINIMUM ��  GUID_PROCESSOR_ALLOW_THROTTLING ��  GUID_PROCESSOR_IDLESTATE_POLICY ��  GUID_PROCESSOR_PERFSTATE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_DECREASE_THRESHOLD ��  GUID_PROCESSOR_PERF_INCREASE_POLICY ��  GUID_PROCESSOR_PERF_DECREASE_POLICY ��  GUID_PROCESSOR_PERF_INCREASE_TIME ��  GUID_PROCESSOR_PERF_DECREASE_TIME ��  GUID_PROCESSOR_PERF_TIME_CHECK ��  GUID_PROCESSOR_PERF_BOOST_POLICY ��  GUID_PROCESSOR_PERF_BOOST_MODE ��  GUID_PROCESSOR_IDLE_ALLOW_SCALING ��  GUID_PROCESSOR_IDLE_DISABLE ��  GUID_PROCESSOR_IDLE_STATE_MAXIMUM ��  GUID_PROCESSOR_IDLE_TIME_CHECK ��  GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD ��  GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY ��  GUID_PROCESSOR_CORE_PARKING_MAX_CORES ��  GUID_PROCESSOR_CORE_PARKING_MIN_CORES ��  GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING ��  GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD ��  GUID_PROCESSOR_PARKING_CORE_OVERRIDE ��  GUID_PROCESSOR_PARKING_PERF_STATE ��  GUID_PROCESSOR_PARKING_CONCURRENCY_THRESHOLD ��  GUID_PROCESSOR_PARKING_HEADROOM_THRESHOLD ��  GUID_PROCESSOR_PERF_HISTORY ��  GUID_PROCESSOR_PERF_LATENCY_HINT ��  GUID_PROCESSOR_DISTRIBUTE_UTILITY ��  GUID_SYSTEM_COOLING_POLICY ��  GUID_LOCK_CONSOLE_ON_WAKE ��  GUID_DEVICE_IDLE_POLICY ��  GUID_ACDC_POWER_SOURCE ��  GUID_LIDSWITCH_STATE_CHANGE ��  GUID_BATTERY_PERCENTAGE_REMAINING ��  GUID_GLOBAL_USER_PRESENCE ��  GUID_SESSION_DISPLAY_STATUS ��  GUID_SESSION_USER_PRESENCE ��  GUID_IDLE_BACKGROUND_TASK ��  GUID_BACKGROUND_TASK_NOTIFICATION ��  GUID_APPLAUNCH_BUTTON ��  GUID_PCIEXPRESS_SETTINGS_SUBGROUP ��  GUID_PCIEXPRESS_ASPM_POLICY ��  GUID_ENABLE_SWITCH_FORCED_SHUTDOWN ��  PPM_PERFSTATE_CHANGE_GUID ��  PPM_PERFSTATE_DOMAIN_CHANGE_GUID ��  PPM_IDLESTATE_CHANGE_GUID ��  PPM_PERFSTATES_DATA_GUID ��  PPM_IDLESTATES_DATA_GUID ��  PPM_IDLE_ACCOUNTING_GUID ��  PPM_IDLE_ACCOUNTING_EX_GUID ��  PPM_THERMALCONSTRAINT_GUID ��  PPM_PERFMON_PERFSTATE_GUID ��  PPM_THERMAL_POLICY_CHANGE_GUID ��  �  P     _IMAGE_DOS_HEADER @��  	e_magic ��   	e_cblp ��  	e_cp ��  	e_crlc ��  	e_cparhdr ��  	e_minalloc ��  
	e_maxalloc ��  	e_ss ��  	e_sp ��  	e_csum ��  	e_ip ��  	e_cs ��  	e_lfarlc ��  	e_ovno ��  	e_res ��  	e_oemid ��  $	e_oeminfo ��  &	e_res2 ��  (	e_lfanew �Q  < �  �     �  �    	 IMAGE_DOS_HEADER �P  PIMAGE_DOS_HEADER ��  P  _IMAGE_FILE_HEADER &�   	Machine '�   	NumberOfSections (�  �  )�  	PointerToSymbolTable *�  	NumberOfSymbols +�  	SizeOfOptionalHeader ,�  �  -�   IMAGE_FILE_HEADER .�  _IMAGE_DATA_DIRECTORY b
!  �  c�   	Size d�   IMAGE_DATA_DIRECTORY e�   
!  7!     _IMAGE_OPTIONAL_HEADER64 ��e$  	Magic ��   	MajorLinkerVersion ��  	MinorLinkerVersion ��  	SizeOfCode ��  	SizeOfInitializedData ��  	SizeOfUninitializedData ��  	AddressOfEntryPoint ��  	BaseOfCode ��  	ImageBase �^  	SectionAlignment ��   	FileAlignment ��  $	MajorOperatingSystemVersion ��  (	MinorOperatingSystemVersion ��  *	MajorImageVersion ��  ,	MinorImageVersion ��  .	MajorSubsystemVersion ��  0	MinorSubsystemVersion ��  2	Win32VersionValue ��  4	SizeOfImage ��  8	SizeOfHeaders ��  <	CheckSum ��  @	Subsystem ��  D	DllCharacteristics ��  F	SizeOfStackReserve �^  H	SizeOfStackCommit �^  P	SizeOfHeapReserve �^  X	SizeOfHeapCommit �^  `	LoaderFlags ��  h	NumberOfRvaAndSizes ��  l	DataDirectory �'!  p IMAGE_OPTIONAL_HEADER64 �7!  PIMAGE_OPTIONAL_HEADER64 ��$  7!  PIMAGE_OPTIONAL_HEADER ��$  _IMAGE_NT_HEADERS64 �)%  	Signature ��   	FileHeader ��   	OptionalHeader �e$   PIMAGE_NT_HEADERS64 �E%  �$  PIMAGE_NT_HEADERS �)%  C�%  PhysicalAddress D�  VirtualSize E�   _IMAGE_SECTION_HEADER (A�&  	Name B@   	Misc Fe%  �  G�  	SizeOfRawData H�  	PointerToRawData I�  	PointerToRelocations J�  	PointerToLinenumbers K�  	NumberOfRelocations L�   	NumberOfLinenumbers M�  "�  N�  $ PIMAGE_SECTION_HEADER O�&  �%  ?�&  �  @�  OriginalFirstThunk A�   _IMAGE_IMPORT_DESCRIPTOR >]'  �&   �  C�  	ForwarderChain E�  	Name F�  	FirstThunk G�   IMAGE_IMPORT_DESCRIPTOR H�&  PIMAGE_IMPORT_DESCRIPTOR I�'  ]'  VIRTUAL_STORAGE_TYPE_VENDOR_MICROSOFT 
�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec ��'  __MIDL_itf_msxml_0000_v0_0_s_ifspec ��'  LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �f  (
ctype1_refcount ��  0
ctype1 �l  8
pctype �r  @
pclmap �x  H
pcumap �x  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �0  	locinfo �   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �H  

�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec �  __MIDL_itf_msxml_0000_v0_0_s_ifspec �  LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
lc_clike ��   
mb_cur_max ��   
lconv_intl_refcount ��  
lconv_num_refcount ��  
lconv_mon_refcount ��   
lconv �e  (
ctype1_refcount ��  0
ctype1 �k  8
pctype �q  @
pclmap �w  H
pcumap �w  P
lc_time_curr ��  X pthreadmbcinfo ��  �  threadmbcinfostruct localeinfo_struct �/  	locinfo �   	mbcinfo ��   _locale_tstruct ��  tagLC_ID ��  	wLanguage ��    	wCountry ��   	wCodePage ��    LC_ID �G  

�  IID_IRpcChannelBuffer3 #�  IID_IRpcSyntaxNegotiate =�  IID_IRpcProxyBuffer ��  IID_IRpcStubBuffer ��  IID_IPSFactoryBuffer �
�  IID_IReleaseMarshalBuffers u�  IID_IWaitMultiple ��  IID_IAddrTrackingControl <�  IID_IAddrExclusionControl ��  IID_IPipeByte �  IID_IPipeLong }�  IID_IPipeDouble ��  IID_IComThreadingInfo ��  IID_IProcessInitControl V�  IID_IFastRundown ��  IID_IMarshalingStream ��  IID_ICallbackWithNoReentrancyToApplicationSTA ��  GUID_NULL 
�  IID_IOleInPlaceFrame �
�  IID_IOleInPlaceObject ��  IID_IOleInPlaceSite ��  IID_IContinue �
�  IID_ITypeComp 5�  IID_ITypeInfo ��  IID_ITypeInfo2 P�  IID_ITypeLib ��  IID_ITypeLib2 =�  IID_ITypeChangeEvents a�  IID_IErrorInfo ��  IID_ICreateErrorInfo }�  IID_ISupportErrorInfo  �  IID_ITypeFactory u�  IID_ITypeMarshal ��  IID_IRecordInfo ��  IID_IErrorLog  �  IID_IPropertyBag z�  __MIDL_itf_msxml_0000_v0_0_c_ifspec �  __MIDL_itf_msxml_0000_v0_0_s_ifspec �  LIBID_MSXML ��  IID_IXMLDOMImplementation  �  IID_IXMLDOMNode '�  IID_IXMLDOMDocumentFragment ��  IID_IXMLDOMDocument f�  IID_IXMLDOMNodeList u�  IID_IXMLDOMNamedNodeMap ��  IID_IXMLDOMCharacterData �  IID_IXMLDOMAttribute ��  IID_IXMLDOMElement �  IID_IXMLDOMText ��  IID_IXMLDOMComment %�  IID_IXMLDOMProcessingInstruction ��  IID_IXMLDOMCDATASection �  IID_IXMLDOMDocumentType ��  IID_IXMLDOMNotation �  IID_IXMLDOMEntity �  IID_IXMLDOMEntityReference ��  IID_IXMLDOMParseError a	�  IID_IXTLRuntime �	�  DIID_XMLDOMDocumentEvents =
�  CLSID_DOMDocument \
�  CLSID_DOMFreeThreadedDocument `
�  IID_IXMLHttpRequest g
�  CLSID_XMLHTTPRequest �
�  IID_IXMLDSOControl �
�  CLSID_XMLDSOControl 
�  IID_ICodeInstall �
�  IID_IUri -�  IID_IUriContainer �
'  p�m           �  s 
:  T-  n 
�   �-  format 
  �-  arg 
�   �-  	u�m      
R�R
Q�Q
X�X
Y�Y  �   �  �                                                                                                                                                                                                   %  $ >   :;I  & I   :;I   I  :;  

 :;  2�� 1  3��1  4�� �B  5��1  6.?:;'I@�B  7���B1  8���B1  9.?:;'I@�B  :U  ;4 :;I  <1XY  =1XY  > 1  ?  @4 1  A1RUXY  B 1  C��  D��   EU  F4 :;I  G  H. ?:;'I   I.?:;'I   J :;I  K4 :;I  L. ?<n:;  M. ?<n:;  N. ?<n:;   %  $ >   :;I  & I   :;I   I  :;  












�� �B  . ?<n:;                                                                                                                                     �   {  �
�/?�u*�/ .�X��Ɂ=�A:h`�Y>�ZYL�-=0Y�YY�-=h��Y�Ku;g6I7Z2I I     �
��f�L!<b�A	.wJ7<x<D ȑNJ�~�{u�0��� �A=�M=z�.�[h�=g�h=y�s�sX[�=�~�ruI��
 �   &  �
�KzJ twYz O    5   �
     ���� x �      4        �m    M       A�D0{
A�AIA�       �       P�m    ;      B�B�A �A(�A0�A8�D`q
8A�0A�(A� A�B�B�K�
8A�0A�(A� A�B�B�A�
8A�0A�(A� A�B�B�E  L       ��m    9      B�A�A �A(�A0�DPn
0A�(A� A�A�B�B $       ��m    O       DPV
Fj       ���� x �      4   `  �͑m    �       A�D@�
A�AVA�         `  �Αm           D0T     ���� x �         �  �Αm    5       D0p  4   �  �Αm    f       A�A�D@@
A�A�H       �  `ϑm              ���� x �      l   X  �ϑm    �       B�A�A �A(�A0�Dpm
0A�(A� A�A�B�Cs
0A�(A� A�A�B�G       ,   X  `Бm    �       A�A�A �C
JN    D     �ёm    i       A�A�D@e
A�A�Co
A�A�A             ґm              ���� x �         �  ґm              �   ґm              ���� x �      $   �  0ґm    a       A�A�DP   \   �  �ґm    `      B�B�A �A(�A0�A8�D��
8A�0A�(A� A�B�B�E       \   �   ԑm    �      A�B�B �B(�B0�A8�A@�AH�	D�H0O
�A�A�B�B�B�B�A�A     ���� x �      <   �   בm    �      D0c
IK
EW
AZ
Ax
A    L   �  �ؑm    �       B�A�A �A(�A0�DP{
0A�(A� A�A�B�E  T   �  �ّm    �      A�D0|
A�Ae
A�GF
A�IS
A�A}
A�A       ���� x �      D   �  pۑm    j       A�A�A �A(�DPW(A� A�A�A�     \   �  �ۑm           A�A�A �A(�DPW
(A� A�A�A�AO
(A� A�A�A�A    4   �  `ܑm    �       A�D0R
A�HF
A�I   4   �   ݑm    �       A�D0y
A�A}
A�B      ���� x �         (  �ݑm              (   ޑm              (   ޑm    E       L   (  pޑm    �       A�A�A �D@e
 A�A�A�AW A�A�A�    $   (   ߑm    �       D0[
Af   $   (  �ߑm    >       D0X
D]    $   (  �ߑm    r       D0[
AQ   $   (  P��m    6       D0X
DU    ,   (  ���m    �       D0X
Dc
AL     ,   (  0�m    �       D0\
Af
Iy        ���� x �         �   �m              ���� x �         	  P�m              ���� x �         8	  `�m              ���� x �         h	  p�m                                                                                                                   _decode_pointer onexitbegin _pei386_runtime_relocator __mingw_init_ehandler lock_free __enative_startup_state dwReason hDllHandle __security_init_cookie DllEntryPoint _amsg_exit lpreserved _encode_pointer refcount _initterm _encode_pointer refcount _decode_pointer __enative_startup_state __dllonexit refcount __enative_startup_state refcount GetCurrentProcessId SetUnhandledExceptionFilter HighPart ExceptionRecord RtlCaptureContext RtlVirtualUnwind TerminateProcess RtlLookupFunctionEntry GetCurrentThreadId GetSystemTimeAsFileTime QueryPerformanceCounter UnhandledExceptionFilter GetTickCount refcount GetCurrentProcess dwReason refcount hDllHandle lpreserved __mingw_TLScallback __enative_startup_state refcount _GetPEImageBase VirtualProtect __mingw_GetSectionCount sSecInfo vfprintf __mingw_GetSectionForAddress __enative_startup_state GetLastError VirtualQuery refcount __iob_func _GetPEImageBase old_handler _FindPESectionExec RtlAddFunctionTable refcount ContextRecord reset_fpu _FindPESectionByName ExceptionRecord _fpreset InitializeCriticalSection GetLastError TlsGetValue refcount LeaveCriticalSection EnterCriticalSection DeleteCriticalSection _fpreset pSection TimeDateStamp pNTHeader Characteristics pImageBase VirtualAddress iSection refcount refcount refcount _vsnprintf                                                                                                                                                                                                                                                        �      �       R�             U             �R�             R      %       U%      )       R)      �       U                �      �       Q�      	       S	             �Q�             Q      %       S%      )       Q)      �       S                �      �       X�             V             �X�      �       V                �      �       1��      �       \�      �       P�      �       \�      �       0��             \             P      *       1�*      :       P@      S       PS      V       \V      i       Pi             \      �       P�      �       \�      �       P�      �       \�      �       P                �      �       R�      �       �R��             R             �X                �      �       Q�      �       �Q��             Q             �d                �      �       X�      �       �X��             X             �h                P       �        R�       �        \�       �        �R��       	       R	      �       \�      �       �R��      �       R�      �       \�             �R�      �       \                P       �        Q�       �        �Q��       	       Q	      �       �Q��      �       Q�      �       �Q�                P       �        X�       �        ]�       �        �X��       	       X	      �       ]�      �       �X��      �       X�      �       ]�             �X�      �       ]                �       	       0�                �       /       T      %       T                �       /       0�/      �       T      *       0�*      �       T                �       �        0�                       (       0�                       (       T                *      :       0�                �      �       P�             V                �      �       T�      �       t��      �       T                             0�                �       �        0�                �       �        1�                               P       A        SB       L        S                �       �        R�       �        �R�                                R       e        Se       �        �R��       �        S�       �        �R�                e       i        Pi       �        S�       �        P                U       `        P`       g        Q�       �        P�       �        Q�       �        P                W       `        P`       g        Q                �       �        R�       �       T                      !       P!      R       Y�      �       P�      �       Y                P       Y        UY       ^        p ����u '�^       a        | ����u '�a       f        | ����p ����'u '�f       n        | ����v ����'u '�n       s        | ����v ����'p ����'u '�s       y        | ����v ����'t ����'u '��       �        P�       �        t ����u '������?�                        $        R$       /        �R�                        $        Q$       /        �Q�                        $        X$       /        �X�                0       R        RR       ^        �R�^       s        Rs       �        �R��       �        R�       �        �R�                0       R        QR       ^        �Q�^       s        Qs       �        �Q��       �        Q�       �        �Q�                0       R        XR       ^        �X�^       s        Xs       �        �X��       �        X�       �        �X�                ^       s        Rs       �        �R�                ^       �        2�                ^       s        Xs       �        �X�                s       �        S�       �        sx�                ^       g       
 H��m    �g       �        S                             P                �      �       P�             X             p �             X             p %      B       XB      I       p Y      l       Xm      �       X�      �       p                 V      �       P�      %       R�      �       P�      �       s�����} "��      �       s|�����} "��      �       R�             R%      S       RY      `       R`      l       s�����} "�m      �       R�      �       P                �      �       S�      �       st��      �       S                �      /       S�      �       S                I      Y       2�                I      Y       ^                I      S       R                      &       8�                      &       ^                      %       R                �      �       S�      �       sx��      �       S                �      �       4�                �      �       U                �      �       R                      %       1�                      %       ^                             R                �      �       4�                �      �       ^                �      �       R                6      T       0�T      �       T�      �       T                        &        R&       a        S                p       �        R�       }       T}      �       R�      �       �R��      �       T                p       �        Q�       ~       U~      �       Q�      �       �Q��      �       Q�      �       U                p       �        X�              V      �       X�      �       �X��      �       V                       �        R�       n       T�      �       T                �               P       n       \�      �       \�      �       P�      �       \                       �        0��       �        S�      �       0�                �      �       R�             S             �R�      9       S9      ;       R;      <       �R�<      �       S�      �       �R��      �       S�      �       �R��      �       S�      �       �R��      b       S                �             Pg      |       P�      �       P�             P      !       P,      >       PI      W       P                �             0�      
                      ~       �       �       �       �       �                       �                  
                      �       �             �                      �                     (                             �       �       n      �      �                      )      7      P      6      �      �      �      �                      �      �      (      +      B      E      I      Y                                                    &                      �      �      �      �                      �      �      �      �                      �      �                        %                      o      r      �      �      �      �                      6      �      �      �                      �      �      `      j                      �       �       �       �       �       �                       $      +      0      8      @      H                      L      P      V      �                      �      �      �      �      �      �                      �      �                                               t      {      }      �      �      �                      �      �      �      �      �      �                      �      �      �      -                      T      [      a      i      q      y                                                                                                                                      .file   <   ��  gcrtdll.c              j                                u   �C                        �   �C                        �   P           �               �   �C                        �   �C                          PC                        9   D                        O  �C                        e  �C                        {  �C                        �  �          �  �C                        �  �          �  D                    .text            *             .data                            .bss                            .xdata         4                 .pdata         0                    �     	                             
      isALong �
          >  �
          H  0      longHash�          T             ^             j  �          x  
  @+          4
  �+          E
  @,          R
  �,          c
  `-          �
  `           �
   .          �
  �           �
  p.          �
  �.          �
  @/          �
  0            �0            1          /   2          A  @3          U  �3          g  �6          {  P9          �  �:          �  �<          �  0?          �  �?          �  �?          �  �@            �A          !  �C          5  �E          J  F          `  �F          s   J          �  �J          �  `K          �   L          �  �L          �   O          �  �O          �  �P          

  !             .data   �                       .bss                             .xdata  �     X                .pdata  <       E             .rdata  `%     2                     _  �E                      .file   �  ��  gral_tuplecmd.c        L   0o                          ]    q         k   �q         ~   0s         �   �s         �   0u         �   0w         �   �x         �    |         �   �}         �   �         �   ��         !  P�         !   �         #!  �         5!  ��         D!  ��     tupleCmd��         S!  @(      .text   0o    1  �             .data   �                       .bss                             .rdata  �%     �  "             .xdata  <     �                .pdata  P     �   6                 _   F                      .file   �  ��  gral_tupleheading.c    a!  p�                          u!  Љ         �!   �         �!  @�         �!  ��         �!   �         �!  0�         "  ��         "  `�         1"  p�         E"  ��         \"  ��         q"  0�         �"  `�         �"  ��         �"  ��         �"   �         �"  ��         #  ��         ##  @�         >#  ��         S#  P�         k#  ��     .text   p�    �  2             .data   �                       .bss                             .xdata  �     h                .pdata  (       E             .rdata  `)     �                    _   F                      .file   �  ��  gral_tupleobj.c        �#  `�                          �#  ��         �#  ��         �#  �         �#  0�         �#  P�         
$   �         '$  P�         A$  ��         \$  @�         s$  P�         �$  �         �$  p�         �$  0�     .text   `�    �  H             .data   �     (                .bss                             .xdata  $     �                 .pdata  <     �   *                 _  @F                      .file     ��  gral_utils.c           �$  0�                          �$  P�         �$  ��         %   �         %  �6          %%  `:          0%  �<          ;%  �-          H%  0�         \%  ��         s%  �         �%   �     .text   0�    �               .data   �                       .bss                             .xdata        X                 .pdata  �     `                .rdata   +     �  �                 _  `F                      .file   M  ��  gral_vector.c          �%  �                          �%   �     snprintf0�         �%  P�         �%  ��         �%  �         �%  ��         &  ��         #&   �         5&  @�         R&  `�         e&  ��         x&  �         �&  0�         �&  ��         �&  Ъ         �&   �         �&   �         �&  ��         '  @�         '  ��         4'  0�         L'  Ю         ^'   �         p'  0�         �'  p�         �'  ��         �'  @�         �'  ��         �'  �         �'  ��         (  б         (  ��     buf.9816           ,(  @�         =(  ��         N(  �         b(  P�         w(  ��         �(  д         �(  �         �(  `�         �(  ��         �(  �         �(  @�         �(  ��         )  ��         &)   �         :)  `�         L)  ��         ^)  ��         q)  �         �)  ��         �)  P�     buf.9939        .text   �    �  V             .data   �                       .bss                            .xdata  X     @                .pdata  D     |  �             .rdata   =     �                    _  �F                      .file   _  ��  gtclStubLib.c          �)  �                      .text   �    �               .data   �                       .bss                            .rdata  �@     5                 .xdata  �                      .pdata  �                         _  �F                      .file   q  ��  g    �)                �)  ��                      .text   ��    ,               .data   �                       .bss    @                      .rdata  A     �                 .xdata  �                      .pdata  �                         _  �F                      .file   �  ��  gatonexit.c            �)  �                          �)  pC                    atexit  ��     .text   �    �                .data   �                       .bss    P                       .xdata  �                      .pdata  �                           �d  
                   V     �                    +  �                           H  
     �                    +  �      0                    H  '
     �                   T  W                         _  @G                          j  X     �                .file   *  ��  gtlssup.c              �*  `�                          +  ��         +  C                    __xd_a  H   	    __xd_z  P   	        2+   �     .text   `�    �                .data   �                       .bss    �	                      .xdata                        .pdata  ,     $   	             .CRT$XLD8   	                   .CRT$XLC0   	                   .rdata  �A                     .CRT$XDZP   	                    .CRT$XDAH   	                    .tls        
   (                .CRT$XLZ@   	                    .CRT$XLA(   	                    .tls$ZZZ`   
                    .tls$AAA    
                          
      the_secs
          �+   �         �+   
          �+  0C                        �+  @C                        ,  `C                    .text   0�    �  *             .data   �                       .bss     
                      .rdata  �A                     .xdata  $     8                 .pdata  h     $   	                   �� 
          s,  @          },  @
          �,  ��     .text    �    b               .data   �                       .bss     
     �                .xdata  \                       .pdata  �     $   	             .rdata  �B                            fT 
-  `�         *-   �     .text   p�    j  '             .data   �                       .bss    �     H                 .xdata  |     0                 .pdata  �     0                      )� 
     �                     _   H                          j  �     0               .file   �  ��  gtlsmcrt.c         .text   ��                      .data   �                      .bss     
                    �.                            �.  �                         �.  (�                        _   I                      __xc_z     	        �.         strcpy  ��         �.  @I          �.  �          �.  �           /  �          /  0D          ./              =/  8�         L/  �          X/  \          o/  ��         �/  �         �/  �      _lock   (�     memmove ��         �/      
        �/  �C          �/    �m��       �/  pC      __xl_a  (   	        �/  ��          0  ��         0  �	          /0  @I          C0  �          U0  `  ��       m0  @           0     ��       �0  �B          �0  @�         �0      ��       �0     ��       �0  (   	        1  �      __xl_d  8   	    bsearch  �     _tls_end`   
        #1   C          91  P�         F1     	    qsort   ��         X1  �          i1  (   	        y1      
    memcpy  ��         �1  @C          �1   
          �1  �      malloc  ��         �1  ��     _CRT_MT �          �1  h�         �1  p�         
        b4  ,          o4  �          }4   D          �4  `�         �4  |          �4  (          �4  `D          �4            �4  @I          
5  $          5  D      Sleep   x�         25            ?5            L5   �         Z5              t5  ��         �5  P
8  �C           8  �          .8  �      __xl_z  @   	    __end__              M8  d          o8  D          �8  H           �8  �      strcmp  ��         �8  ��         �8  8�         �8  ��         �8  �       __xi_a     	        �8  l          �8  ��     __xc_a      	        
9     ��       #9  H   	        59     ��       C9  8�         N9             k9  |          }9  ��         �9  PD          �9  �      __xl_c  0   	        �9  �          �9  h   
        �9  4          �9  8
  
 *                 4           �   /usr/lib/libSystem.B.dylib  &      L �  )                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            U��SWV��,�    [������ j W�u�گ ��1�A����  1�VVW�u�K� �����l  �]����� �}������ �M�VVQ�MQ���  ���E����  �E������� ��N� �U�VVQR�u���  �� ���[� VS�u��u���  ������  �u�����  �M؃�1�OWS���   ���Ë��W�u����   ��PS�u��u�U؃�����  �]����1��}����� ��a� �U�VVQR�M��V���  �� ���q� �    QW�u�V���  �����3  �u�����  �M؃�1�KSW���   ���ǋ��S�u����   ��PW�u��u�U؃�����   ���}����� �MԋEPQ�/�  �����u���E������ ��z� �E�1�S�    QRP�u�E����  �� ����� SW�u��u���  ����u{�u�����  �M��1�IQW���   ���Ë��1�IQ�u����   ��PS�u��u�U����u3�}��j �u��u܋M��V��|  ������u�PV���  ����t0�U��u��H���������Q���   ��1�A�ȃ�,^_[]Ë]���}����� ���� QR�u�V�MQ���  ������� j QV�u�P��1��U��]�����U��1�]�U��1�]�U��SWV���    [�E�E�����p  ��� �U�Pjj
QR�T� �� ��1���tI�M���� � �x��Q���Z� ������P�׃��ǍG��SP�6� �����G1��G�G����^_[]�U��SWV���    _�]��8� � �p�$��� ���$�։ƍF�\$�$�ܮ ���� �F�F   �E�F� ����^_[]�U��SWV���    _�]���� � �p�$虮 ���$�։ƍF�\$�$�z� ����� �F�F   �E�F� ����^_[]�U��V���    ^�M�A�P���s
   膫 ���  �M�L$�M�$�P���  ���� ����� �|$�T$�$    �PP1�����   ��|$�M�L$�$    �PP����   �E�@�M�I�L$�$�@@  ���   ���� ����� �|$�T$�$    �PP1�����   ��|$�M�L$�$    �PP��ur�E�@�M�I�L$�$�PZ ���V���� �	�D$�� � �$�Q1��:1�F�5�}ԋ�M�$��X  �Ƌ�M�$��X  �D$�4$�n� ����������<^_[]�U��SWV���    X�}�M���� ��$��X  �Ƌ�<$��X  �D$�4$�� ��������^_[]�U��SWV��,�    [�}�u�M�A�P���rI��uq�A�E����  �D$��}� �D$�E��$�D$   �D$
   蓩 ��t�|$�4$�P�F��1� ��4$��X  �Ƌ�<$��X  �D$�4$�w� ���1� �	�D$��� �$�Q1���,^_[]�U��SWV���    X�}�M��w� ��$��X  �Ƌ�<$��X  �D$�4$�� ��^_[]�U��SWV���    X�u�M�Q��tB�M��t>����   ��0� ���<� �D$�t$�$�RP��1���u}�F�$��^ �3���l��0� ���8� �D$�t$�$�RP��1���uH�F�$��B  �Ƌ�4$�D$�������   �ǋ�4$�P�����0� �	�T$��&� �$�Q1���^_[]�U��SWV���    [��Z� ����u��X  ���Ƌ�M�U�QR�u�u���   ��1Ʌ��8  �M�E���t;����   ��M���1��X  ���ǃ�WV������������   ��W�v��M���1��X  ���ǃ���;� WP耧 ����tm����2� W��P�g� �����}t}��Sj�E��V�� ����VW�*���Q��X  ����Pj�E��V蹏 ����V�u�`� ��1��Y�E���u�p�u�E� �����    t9��PV������(�E���u�pW�� �����    t��PV���������ȃ�^_[]�U��SWV��,�    _�M�U�]�u�C����   ���  ���j  ���� � ���M�QR�����   ���}� �\  �C�E�����t  ��� �U�Pjj
QR�
� �� ������  ���}W�uV�Q������  ��Wj�E��W�ێ ����WV��  �B;��� ��   �:��   ���� �E؋ ��R�P|���ǃ��uW�uV�s�3�  �� ��uq�E؋ ��W���   ��1�����  ����   �B;��� ��   �:��   ���� �E؋ ��R�P|���ǃ��uW�uV�s胄 �� ����   �U؋�H��1����%  ���W���   ����� �	���֍��� PR�Q����   �����E�����   ��QRRV�s�X�  �� 1�����   ������ � ���M��V���   �����G���0�j��QRRV�s�Ѓ �� 1�����   ������ � ���M��V���   �����$�u�_���[�E؋ ��W���   ��1���tC���G���p�3�x ������u'���uj�E��V�� ����V�u�B� ��1�����,^_[]�U��1�]�U��SWV���    [�}�G1��H���r|��u_��� �E؋ ���M�Q�u���   ���}� tT�G�E������  ��O� �u�Pjj
QV�V� �� ��t3���u�P�������� �	����g� PR�Q������^_[]ËE؋ ��V�u���   ���M�1���t��@�Iu���U��VP�    X��9� � ���M�Q�u���   �����U�1���t	�1A�Ju���^]�U��VP�    X�u�M�Q����� � �V��R�1��p  ���F��^]�U����    X�M�U���� � ���q�u�2��X  ��]�U��VP�    Z�M�E�p�1��t&��t5��uE���� �����Q�p��p  ���F��Q�p�1~ ����
����� � ����'� VQ�P��1���^]�U��SWV���    X�]�u�}�O��t-��tn����   ��� � ���sV�w��X  ���   �N�{������ PQ聡 ��f�F	 {�F��SP�w�~ �����}�T�D
�T	)����� ������� QP�R��1���^_[]�U��WV�    Y�u�U�E�}��>��t0��t:��uH��
� �	��p  ��P��X  ������VP�����VR�p�7  �
� � ����Ӥ WQ�P��1�^_]�U��SWV���    Y�}�U�u�E�]�[��t0��t8��uD��u� �	��X  ��P��X  �����wVP���WRV�p��8  �WRV�p�T �����u� � ����q� SQ�P��1���^_[]�U��SWV���    Z�}��H���s<�w�_��~�U�F�$�������N���_�U����� � �$�P�G    ���t���� �	�D$��� �$�Q��^_[]�U��SWV���    Z�}��H���s<�w�_��~�U�F�$�������N���_�U���c� � �$�P�G    ���t��c� �	�D$��̣ �$�Q��^_[]�U��SWV���    X�}�O�M����� ��M��Q�7��p  ���Ɖu���E�PW�������D���P�Q���Ƌ���u�V�7��X  ���L� ���]�SQW�b�������S�y���������^_[]�U����E�M���0�1�� ��]�U��SWV���    _��.� � ���]�S�u�u���  ���ƅ�u��>� � ��S�P ������^_[]�U��SWV��,�    _���� ����M�Q�uj ���  ��1���ua����]�S�uj ���  ��1���u/�ً��� ���Q�M�Q�P(������������M�Q�P ������ � ���M�Q�P ������,^_[]�U��SWV��,�    _��B� ����]�S�uj ���  ����t1�N�[����u�V�uj ���  ����t1�N�)��R� �E̋ ��VS�P(����ƋE̋ ��Q�P ����R� � ��S�P ������,^_[]�U��SWV���    _���� � ���]�S�uj ���  ��1���u)�E�1���t1��M��A�Hu����� � ��S�P ������^_[]�U��SWV���    ^��C� ����]�S�u�u���   ����t���S�u�u���   ���T�]�M���1�1�B9�tB��t;���	  ��j �u���   ������&� PQ�Ӄ����P�u���  ��1�@��^_[]�U��WV���    X���� ����M�Q�uj ���   ��1���u'����M�Q�uj ���   ����u�E�;E���������^_]�U��WV���    X��?� ����M�Q�uj ���   ��1�N��u!����M�Q�uj ���   ����u�u�+u�����^_]�U��SWV���    _1��u����� ����M�Q�uV���   ����t$��X���u��X  ������ݟ PQ�Ӄ�1��L5��F��u��^_[]�U����    X��v� � ��j �u���   ��1���]�U��SWV���    X�}��G� ����M�Q�u���   ���Ƌ���M�QW���   ���M�;M�u��QPV趙 �������1�����^_[]�U��SWV���    X�}���� ����M�Q�u���   ���Ƌ���M�QW���   ���M��U�9�Nу�RPV�G� ��^_[]�U��VP�    X��� � ���M�Q�u���   �����U�1���t	�1A�Ju���^]�U����    X��>� � ���M�Q�u�u���  ��]�U��SWV���    X�}�M��� ��$��X  �Ƌ�<$��X  �D$�4$謘 ��������^_[]�U��SWV���    X�}�M���� ��$��X  �Ƌ�<$��X  �D$�4$�]� ��^_[]�U��VP�    X��}� � ���M�Q�u���   �����U�1���t	�1A�Ju���^]�U����    X��<� � ���M�Q�u�u���   ��]�U��WV���    X��� ����M�Q�uj ���   ��1���u-����M�Q�uj ���   ����u�E���E� f~ƃ�����^_]�U��WV���    X���� ����M�Q�uj ���   ��1�N��u<����M�Q�uj ���   ����u!�E��M�f.�v1�F�1�1�Nf.�F�����^_]�U��SWV���    _1��u�u苟/� ����M�Q�uV���   ����t$��X���u��X  ������L� PQ�Ӄ�1��L5��F��u��^_[]�U����    X���� � ���M�Q�u�u���   ��]�U��WV���    X���� ����M�Q�uj ���   ��1���u'����M�Q�uj ���   ����u�E�;E���������^_]�U��WV���    X��1� ����M�Q�uj ���   ��1�N��u!����M�Q�uj ���   ����u�u�+u�����^_]�U��SWV���    _1��u����� ����M�Q�uV���   ����t$��X���u��X  ������� PQ�Ӄ�1��L5��F��u��^_[]�U����    X��h� � �M��U�QR�u�u���   ��]�U��SWV���    X�}�M��8� ��$��X  �Ƌ�<$��X  �D$�4$�Ք ��������^_[]�U��SWV���    X�}�M���� ��$��X  �Ƌ�<$��X  �D$�4$膔 ��^_[]�U��VP�    X���� � ���M�Q�u���   �����U�1���t	�1A�Ju���^]�U����    X��e� � ���M�Q�u�u���   ��]�U��WV���    X��7� ����M�Q�uj ���   ��1���u'����M�Q�uj ���   ����u�E�;E���������^_]�U��WV���    X��ֿ ����M�Q�uj ���   ��1�N��u!����M�Q�uj ���   ����u�u�+u�����^_]�U��SWV���    _1��u���y� ����M�Q�uV���   ����t$��X���u��X  ������՘ PQ�Ӄ�1��L5��F��u��^_[]�U����    X��
�Z��~�}��$����1Ʌ�tp��M�΋�L$�$�@` �ǋ�$�����E�v9�t:��|$�D$�E�$�:   ;E�t��D$�E�$�D$    �������9�uƉ<$�Yr �M�ȃ�^_[]�U��SWV���M�E�]��t/��Q�3P�8 ���ǍC��WP�S<���ƃ�W��3 ����K��PQ�S<���ƅ�t�F��C��C��^_[]�U��SWV���u�U�E�
�Z��~�}��$�j���1Ʌ�tp��M�΋�L$�$�*_ �ǋ�$�j����E�v9�t:��|$�D$�E�$�$���;E�u��D$�E�$�D$    ������9�uƉ<$�Cq �M�ȃ�^_[]�U��SWV���u�}���6�7�Z ���É]�1�����   ��S��������E�W+W���N+N���ʃ�QP��������G�W9���   �N�E�~9���   �0��E���S�F1 ���ËF�N�P+Pʃ��sRQ�2 ���M��A�I�P+PʋE� �p+ps��V�uRQ��1 ����j S�]��u������E����;~�z����M�Q����9��Y����E��^_[]�U��SWV���u���u�6�T ���E���P��������ǋF+F����PW��������^�%���u�u��3�0 ����j PW��������;^u։���^_[]�U��SWV��   �    X�E��}�E��C+C���D$�<$�s �E��|$�$��S �E��C+C���O+��@)ȉ$�cS �ƋE��8�&�C����D��t$�D$�T$�$�(U ���E�;xuՋE��D$�E�$�w����D$�4$�QT �4$�����E��E��$�}����U��  �E���� �E�� �M��L$�<$��L  �ǅ���|����n  �E���t���u�G��G� �E��4$�/ �p�M��;Yt5�}��E��}��G����D��t$���D$�$�/ ���E�;Xu���}��E��E��$�����ǉ}��<$謅  �� �E��p�u��F+���D$�<$������;N��   �M��E�@�	���E��E��$�n. �E��}�;_t0�E��p�E��@����D��t$���D$�$�/ ��;_u֋E��D$�E��$�D$    ������M����E�;H�{����E��D$�E��$�D$    �����E�� �M��$��  �ǅ���|���������E��$�"m �E�� �M��$���  �E��Č   ^_[]�U��SWV���։��    X��Y� �	����q� Pj�W���  ���^�7��E�E�E���E�P�E�PW�W0����+N����Q�p�2n ����;^uă�^_[]�U��SWV��<�    _�]�E�0��SV�T ��;F��  ��y��  �}�y�}��M��E̋J+J���G+G���D���P�JP ����S�}�W�u��vV�"R ����S�vWV�R ��S�E��p�pP��Q �����v  ��S�������E�M�q�I�M�9��K  �]��U싊� �M���� �M��]�uЉ�� �H�x�A+A�E�M̋E�+H�M�4�E�� ���u�Vj �PP�����  �v�F+F����P�u��E������^�F�E�9ËE��M���   �}�}���M�I�Mԋ;��P�+ ���E��p��V�u��u��n, ���4���V�u��u��Y, �����O�W�q+qփ�PVR�:, ����j �u��u��	����E܃���9؋E�u��uЃ�;uċ]�E���������Sj���Sj�u�?f ��1���<^_[]Ã��E썀�� Pj�u�f ����S�xQ �΃�VjS�Tf �����u������U��SWV��,�]�E�M�1�8�Q�U��P�U܃�SPQ�m  �����E��uPSWV��T �� 1����A  ��P�E��������Ɖu؋C+C����PV�������K;K��   �M��I�U��<��E܋4��u�E�����u��* ���E�P�G�O�׋P+Pʃ�WRQ��* ���N�Q;Qt>�U�R�<��Ѝp�;�t��WVP�* ���<��E�H�P���A+A�9Ɖ�u̓�j �}�W�u�V�P��������Et�M��;H�>������u���h ���9��V�6��������u��h ����W��Z ����Pj.�u�d ��1�����,^_[]�U��SWV��l�    X�E��u�}�]�4$�D$   �]����E��$�M����L����4$�D$    �=����E��_;_td�]���E��E��E��E��D$�E��$�U����t6�@�E��8�'��+A���ˋ�L$�D$�4$�����E��ً]���;xuԃ�;Y��u��E���� � �M��$���  �E��$�g �E��$�g ��l^_[]�U��SWV��,�u�E�M�9��Q�UԋP�UЃ�VPQ����������E�M��uPQVSW�<S �� �É]�1����K  ��S�������ǉ}ȋF+F����PW� ������N;N��   �M܋�I�Uԋ4��EЋ��E��S�' ���H�M�^�K;Ku�E��I�E؋N�u��U��2�ʍz�>�t ���]�SWR�)( �����]�E��X�H���S+S�9׉�uǋE�@�H;H�]�t@�M�I�U�2�ʍz�>�t��SWR��' �����M�A�I���P+P�9׉�u̓�j �u؋}�W�������M܃��E;H�]��������u��	f �����u���e ������,^_[]�U��SWV���]�u�F�E���SV�u�B��������6�������ƋC+C����PV�������{��G��j �M��4�V���������;{u����^_[]�U��SWV���]�}��WS�u����������3�(������ƋC+C����PjW�e������E��{�C9�t*�M���; t��j �7V�f������E�@����9�uۃ��u���d ������^_[]�U��SWV��,�E�M��K+K�M���M�Q+Q�U��x+x�E� �E�p+p�    X�E��j QS��R ����t���E썀�} Pj�  �u؉}܋u����u���j �}�WS�]��R ��9����]�}��e  ���u�����j QS�xR �ډ��]���}��9��9  9��1  ���}�W����������E؃�VW�M ���ǃ�W�~$ ���Ɖu���WS�^Q ���E�E�H;H�E���   ���E܉M��U��~�B�H+H�B���WQP�% ���M�Y1�;YtQ���E�}��H�@�Q+Q��u�RP��$ �����u��u�W�������;G����ƃ��E;Xu��}��E�9�u��j �u��u��l������M���E;H���J�����V��$ �����u���b ���E�����E썀�} Pj�u�^ ��1���,^_[]�U��SWV��,�    ^�M�]���z �D$�$�"����ǋ�$�D$   �{I �É|$�$��G ;C��   �}�$�6������9;y��   �]��]��� �M܉E�E�@��4��E��$��" �E��@�E�F�N�P+Pʋu�t$�T$�$�t# �E�E܋ �$C���   �M��� �E��D$�E�$�D$    �"����E��;xu��E��$�E�M�L$�$�D$   �k] �$��H 1���,^_[]�U��SWV��l�    ^�]�����y P�u��������ǃ�j�3�UH ���Ã�WS�F ��;C�  ��S�]��������ǉ}���� �E�� ����Ψ �U�Qj�R���  ���E��`�M��F�N�P+Pʃ��u�VRQ�V" ���E��E�� ��WG���   ���M���� �E��x��j �u��}�W� ��������E;Xt`�E�@��4����u��! ���ǋG�E��u��E�E����E�P�E�P�E�P�U�����}� t�}��}�y�F����}��y�;����E�� ���M�Q���  ���!���uj�u��[ ����S�;G ��1�����l^_[]�U��SWV��,�    X�E��]�u�E�8��VW��G ��;G��  ��y��  �Y�]�u��E��J+J���C+C���D���P�C ����S�u��wW�E ���E���S�wPW�|E ��S�E��p�pP�iE �����E  ���]�S�������É]܋F+F����PS� ������V;V�  �G�M�)����M��E���P� �uԋ�\� �EЉU���E�X�<��Eԋ ���u�Wj ���PP������   ���G�E���u��J ���E�x��WS�u��v�  ���<����F�H+HN��WQS�� �����U�J�R�q+qփ�PVR�� ����j �u�]�S�������U����E;P�M��9������Vj���VjS��Y ��1ۉ؃�,^_[]Ã��E���Yy Pj�u�Y ����S�E �̃�Wj�u��Y �����u��2����U��SWV��|�    X�u��M��~�}��^�]���� �E�� �M��$���  �F+F�����$�g �E�9���   �E�    �]���x�7�E��D$�t$�E��$�U�}� t�M��HA�M��t$�E��$�i �E��w�M��L$�t$�$�U�}� t�M��HA�M��t$�E��$��h �u���9�u��E��X+�]����]������}��E�� �<$�P�E��|$�$�o �U�9�}�tA�U��E�� �p��D$�<$�U�N�p��L$�<$�U�p�U��E��0�E���9�u��E�� �<$���  �}� ~b�E�    �E��E��}��}� �    t9�M��E���E���E�1ɋE��ڊ�u�"7�u���A@9�|�M��;M�|ǋE�@�E�]�9�|��E��$�����U��E���A+A���D$�$������}� ��   �E�    �}��}�1��? t^�E��$�9 �E��@�E��E��D$�]��$�}f �M��� �t$�$�]��gf �M��A� �E��D$�E��$�D$    ����FG9�|��E�@�E��}��9�|��E�� �M��$�P�E��$�oe �E���|^_[]�U��SWV��,�E�M�q+q�u����x+81ۉ\$�4$��Y �E�\$�É$�[[ ��~t�E�M�M��M�M�E�{+;�]؃�|#���������C�K��E��$���B   K���}��]�|)�}�N�t$�$�D$    ��] �<$1ɉ��   ��ډ؃�,^_[]�U��SWV���U�΍\69�}W�}�u��s9�}�<$�����  ��OމދE��<$�Éى��  ���M�y�G�t$�\$�$���j] �ڍ\69�|���^_[]�U��SWV���}�E���0�������ƋG+����PV���������E�@���j �4�V���������;_u߉���^_[]�U��SWV���    [�M�u�U�B9�w9rs��ј � ���r �$���P�M���B9�w9Js��ј � �΍�!s �$���P��u��9�v��ј � �΍�Ps �$���P��u��9�tH�B�E���ј �E�}��D$�E��$�։��V<�M�	�$���  ��$� �ى��9�uʋB)ȉD$�L$�]�$�։��k ��)ߋB)��B9�t-�r�}��D$�4$���S<�څ�t��+J���H��;zuًE��^_[]�U��SWV���}�u�F�E��_���7�6�? ����tT�]���6�7�E ���ǋ^1��E�9�t&��W�3�u�����M���;E�����ƃ�9�uڃ�W��W ���1�N����^_[]�U��WV�u�}��VW�_�������x!�O+O���V+V��9���9��� ���^_]�U������u�u������������I���]�U��WV�u�}��VW���������x!�O+O���V+V��9���9��� ���^_]�U��WV�u�}��VW��������x!�O+O���V+V��9���9��� ���^_]�U��WV�u�}��VW�u�������x!�O+O���V+V��9���9��� ���^_]�U��WV�u�}��VW�4�������x!�O+O���V+V��9���9��� ���^_]�U��SWV���}�E���WS�F> ����;st)�}��W�6軻������PVS�P> ��;Ct
6 ���M�I�W���4��4�P��������u9u��vuȉ��ًU1�9rDȉȃ�^_[]�U��WV���    X�}���� � �$   �P�ƍF�F�$   ��O �F��F� �G�F����^_]�U��WV���    ^�}�G� �$� �G�$�%P ���� � �@��^_]����U��SWV���    _�]�u�U��j� � �����k QSjR��(  ���*���M�Qj ���k Qj���� Q�sR���  �� ��t1�@��E���SV�u��Ǌ� ����^_[]�U��SWV���    X�]�U�}��   �s��� ������ �D$�t$�$�QP��1�A���  �F�E�� �E�C�E�����s��X  ���Ƌ���s��X  ���E��V�]�S�d8 ���E��xz���u�S�N8 ���E���uxq�u��^���;^t{��@���h   �U��4��U��4��u��u��  �� ��u����� �	��'k PSjR��(  ��1�A�=��1�GVWWj�u���1�G�u�WWjV�K ���� �����u��l  ��1ɉȃ�^_[]�U��SWV���   �    X�]�}�U���� ������	�M���$���� �	���i PSjR��(  ��1�F��   �s��������� �	������ �D$�t$�$�QP��1�F����   ������@��������   �����PPQ�)I ��������A�I)��   9�u#������������QSW�u�0�<G �� ���+�������j�������W�DI ����W�u�I �������� ;E�u
���   ���R���   ������^_[]�U��SWV���    X�M�]�}uz�q��� ���%� �D$�t$�$�QP��u~�F�$�����Ƌ���   �E��F�E�4$�^ ����D$�E�$�U��E���4$�P��M��L$�$���  1��*��� ����g �D$�L$�$�D$   ��(  1�@��^_[]�U��SWV���    X�M�u�}uX�y��V� �����^� �D$�|$�4$�QP����uK�G����  �P+P����R���   ����PV�׃�1����V� ����f PQjV��(  ��1�@��^_[]�U��SWV��  �    X�}�]�U��͉ ��M�������������$���� �	��]f PWjR��(  ��1�A�p  ������w���� �� ����	������ �������D$�t$�$�QP��1�A���(  �F�������w�� ���� ��������V�u�PP������   �F��������1�PP�U������ƃ������jjP�E ����������������������������������RV������������SPQ�u�c  �� ��t��V�R����������1�A�n  �����WV������S�V������Ã�V�������ۋ������   �U�����������H����������   ������������H������0�� ���� ��������QVR���PP������   �^��������1�PP�6������������������VPS������S�����P�����PW�b  �� ����   V������V������S�x������ǃ�S�w�������V�5���������������U�����������PR���W�u�MD ������ ���� ���  ��S���?_  ����PW�փ�1ɋ�����;E�u.�ȁ�  ^_[]Ã�������������Z������������G����oZ U��SWV���   �    Y�u�]��Ȇ ��E���&���� � ��c QVj�u��(  ��1�A��   ������������j	jP�B ���������P�v�u�a; ��1�A����   �������P�������ǍC���PW��������|(����������P�6�uW�_  ����u?��K��ދ�������� � ���  ��W��]  ����P�u�փ�1ɋ�������W�����������1�A�;E�u
��a���R���   ���ل �	���a PWjR��(  ���5��S�����u��u���  ��1�����u�jjj�u�A �� 1�A�ȃ�^_[]�U��SWV���   �    X�u�]�M��w� ��U�����   �v��c� ������	���ڋ�k� �\$�t$�$�QP��1�A����   �F������E�p������ ��SV�u�PP������   �F������E�p������ ��SV�u�PP����uy�v�������jjS��> ��SV�����������B�������t]������	���  ��P�Z  ����P�u�փ�1����c� �	��e` PVjS��(  ��1�A�;E�u�ȁ��   ^_[]Ã�S�u�6? ���U U��SWV���   �    X�u�}�U��� ��M���$��� �	��_ PVjR��(  ��1�A�  ������^��� ������	����� ������D$�\$�$�QP��1�A��t������q  �C�� ����^������ �������S�u�PP�����9  �C���������   �����PjS�t= ��Sj�������� ����x���������   �����������������M�� �������   ������������� �������SQ�PP������   O�������������Pj�s�� ���V��������Ã�V�������ۉ؋�����Mu��������PQ�= �����������P�u�q= �=������ ���  ���� ������aX  ����PW�փ�1������ ����!����������1�A�;E�u
������R���   �^���U��SWV��,�    _�u�E�U�@���r!��gw � ��)U QVjR��(  ���q  �F�E�^��gw �����ow �L$�\$�$�PP�����>  �}܉u�s�E��t;����  �F+F�u�����1�VP�I6 ����V��V��7 ���E�x�K���}�w�u�6��, ���ǅ��   ��  ��j WV�u��
������ƃ�W�7 ���E�x�E�U�uԉ}؉E�M����]��� ��R��l  ���1�;^�M���  �A�EЉ]���1落�����ǃ�jW覾�������1�V�M��4�W��������W�N  ������}���h   SV�u�uV��  �� ���  ���j �M�QV���  ���ǋ�H�����E� ��S���   �����M��u��2  �U܋���  Ћ]�����;^�1���1��  �u��E� ���M��u܍�qU ��z Qj RjV�}�w�uV���  �� ��uV���wV�E��0�@+ ����t>�}����Ƀ�QP�u��E��p������Eԃ��u��n5 ����E��u��_���1�@��,^_[]Ë�H��1�G���u��E� ��S���   ���M��I�E� �� 	  ��	  ���M��S��|	  �����M܍�zU PQ�׃���PS�֋uԋM���1�G�}�A;A�]�t)�E� ����   ��S��X  ��1�QQP�u�։�����V�4 ����H�����u��E� ��S���   ����H�����}��E� ��V���   ����H�����E� ��W���   ���E������    �  !       U��SWV��  �    [�E���s ��M�����  �������jjV�/ ��V�E�p�p�u�˞��������  �����������V�E�p�p�u袞��������  ��������j�Ɖ������ ���ǃ�������PW�� ����VW�� ����W�,������ǋ�ss ����������������� ���������R������RQS�M�q�u���  �� ���^  ������ ��   ���7����  ���Ǎ����S�������������0W��  ������   S�������������0W���  ������   ��1�PW��W�������������������Q������Q������Q�� ���S���  ��1�9������\������S���  ������  ��W��I  ����P�u�փ�1��   ��ss � ���Q Q�uj�u��(  ��������P���V�u�j. ��1�@�M��S�u�V. ����W��  ��������� ���� ���Q���  ������W������1�@�������;M�u��  ^_[]��D U��SWV���   �    Y�u�}�E���p ��U������p ��(  ���������������QR�vW���   ��1�A���  �������jjP�, �������P�v�vW���������ǅ���   �� �����j�� ���ƃ�WV�+ ����V萷�����Ã���� ~[���3�:�  ���Ƌ���������Q�0�7V���  ������   ��j VS�ĸ����������H�������������������� ���  ��S��G  ����P�u�փ�1ɋ� ������XO QVjW��(  ��1�A�;E�uW�ȁ��   ^_[]Ã������PQ�d, �ԃ������P�u�P, ����V��  ����S�'������� ���1�A��B U��SWV���    [�}�} ���n � ���M QWj�u��(  ��  �W�U����n �����o �L$�T$�M�$�PP��1�A����  �u�]�E��@�E܋ �E��G�E��E�p���V�d. ���É]�}|G���E� ���7��X  ���Ã�S�u��H ������   ����P�]�S�2 ��N����C+�u��N+N9�}}�E� ���u���X  ���߉Ã�SV�� ����x��PW�2 ��;G��   ��WS�u�迾������tk�M�	���  ��P��E  ����P�u�փ�1��   ���E䍀�M Pjjj�u�u+ �� ��S�\��Sjjj�u�Z+ �� ���u��?���E䍀�M Pjjj�u�4+ �� 1�A�'���   SPjP�u�+ �� ��W�- ��1�A�ȃ�^_[]�U��SWV���    X�M�]�}uf�q���l ����l �D$�t$�$�QP��uj�F� �$� �Ƌ�4$�D$�������   �E���4$�P��M��L$�$���  1��*���l ����I �D$�L$�$�D$   ��(  1�@��^_[]�U��SWV���   �    X�]�u��Jl ��M���&��6l �	��?K PSj�u��(  ��1�A��   ������S�������6l �����>l �D$�T$�E�$�QP��1�A����   ������������������p�O������ǃ�VW�'������������jjP�' ����~4N�����P�3���uW�D  ����t߃�W�U����������1�A�.������ ���  ��W�QC  ����P�u�փ�1ɋ�����;E�u
�E�@1��E����M����@ ��*e Qj RjV�u�v�MQ���  �� �������}��������A��P�u�3�~ ���ǅ��������VWS�]��������ƃ�W�  ����U��SWV���   �    X�u�]�M��` ��U�����   ������ڋ^��` �����` �|$�\$�$�QP���M1�B��uw�C������E�X���WSQ�PP������   �{�������j&jS� ����SW�������������tK����  ��P�w7  ����P�u�փ�1ҋ�����9��` �	���? PVjS��(  ��1�B���S�u�! �������1�B�;E�u
1�9�tK�U؋A�M܋M�<���@��W�M��4��u�諅�����������}�t����Ã��E�;pu���M܉U؃��u����  ���ǋw�M܋A�I�P+Pʃ�VRQ��  ���E܋EЋ C��S���   ���M܉�� ��j W�u��S������U؃��M�A9��/����EЋ ���  ���u��_3  ����P�u�փ�1ɉȃ�,^_[]�U��SWV���   �    [�u�}��[ ������ �E���&��[ � ��y; QVj�u��(  ��1�A�  �V�������[ ������ ����[ �L$�T$�M�$�PP��1�A����   ������@������������j-jP� ���G����   ��������B������������������|Y��O���6��X  ���Ë����� ���v��X  �������QPS������I�������t}�����������������  ���������1  ����P�u�փ�1ɋ����� ;E�ui�ȁ��   ^_[]Ã����; Pj	�������V�� ����V�u� �#�������P�u�x ����������V����������1�A���, U��SWV���    X�}�U�}�e  �_��+Y �����3Y �D$�\$�$�QP��1�A���T  �u��u�[�G�E��}���3�>�������M���E؋{�C�E�9�tu��E��P� ���]����h   Pj �u�V��  �� ����   ����M�Q�u�V��(  ������   �}� t��j �u��u��9�������9}�u��]����   �M���}�W��X  ��1�QQPV�U���U��H��
�����R���   ����H�������W���   ������  ���u���/  ����PV�׃�1����+Y �	���9 PWjR��(  ��1�A�ȃ�^_[]Ë��   ���u���X  ��1�QQPV�׃��U��H��
�����R���   ���U��H��
�����R���   �����u��-����U��SWV��,�    X�u�U�}��  �~��W �����W �D$�|$�$�QP��1�A����  �]���H�M܋H�M�v�u���P�"�������EԋG�O�M�9���   �E؋ �E�@�X+X��t@�E�p���}܋�M�	��h   �6j �0�u���  �� ����   �������űE� ���M�Q�u��u��(  ������   �}� t��j �u��u���������E؃�;E��_����E܋]�9؋}��t&�ǋ�M�	1�VV�0R����   �������9�uډ׋U���H��
���u����R���   ������  ���u��-  ����PW�փ�1��   ��W �	���7 PVjR��(  �e�E܋M�9ȋ}���u�t*�ǋ�1�SS�0R�����   ��ދM������9�u֋U���H��
�����R���   �����u�� �����1�A�ȃ�,^_[]�U��SWV��  �    X�u�]�U���T ��M�������������$���T �	��y1 PVjR��(  ��1�A�M  ������~���T �� ����	�����T �������D$�|$�$�QP��1�A���  �G�������~�� ���� ��������W�u�PP������   �G��������1�PP�q������ǃ������j1jP�4 ����������������������������������������RWS������VPQ�u�.  �� ��t��W�m����������1�A�J  ��WSV�'������ƃ�W�C�����������H����������   �� ���������������H������0���������V�]S�PP�����������   �v��������1�PP�k��������������������PQV������V�����P�����PS��-  �� ����   ��������W������V�V������Ã�V訜������W�f�����������H���������ދ� ����!������ �������  ��S�*  ����P�u�׃�1ɋ�����;E�u3�ȁ�  ^_[]Ã��������&����������������ݔ���n����% U��SWV��  �    X�u�]�U��R ��M�������������$���Q �	���. PVjR��(  ��1�A�M  ������~���Q �� ����	�����Q �������D$�|$�$�QP��1�A���  �G�������~�� ���� ��������W�u�PP������   �G��������1�PP芓�����ǃ������j2jP�M
 ���C+C���N+N����������QP�j������ƃ�V������S���  ���K+K��9ȋ����u\�������E�ƍN��VUUU�������Ѝ@9�tp����a, Pj�������V��	 ����V�u�
 �����������/���� ���j�������W�
 ����W�u�j
 ����V�����h����E�@�������ȹVUUU�����Ѓ�P��������  ���������������|X�E�H��E�x�� ���V�7�w��u�wx��������  ��P������S���  ��;C��  �� �������������������S������������������� ��������V������������W�y������O;O����  ������� �P+PW�������������E�@�������1�1҉��������������RPV�j������� �����P������躲������P�$  ���ƃ�W��  ���������x���� ����_ �������� ��h   V1�Q�������u��  �� ����  �C�K�P+Pʃ�WRQ��  ���}��   �4��������������������������� ��������� �������Q�2�}W��<  �����6  �����P������3W�6x�����ǅ��-  ��������Q��1�B9�������	��P���   �����>������������ ���������[���������G��j ������������苓��������������������;H�������������    �?�����������   ��������W��X  ��1�QQP�u�փ���H�������W���   ������  ���������7"  ����P�u�׃������1��  ���������`�  ��������>�������Q���������������	��P���   ������������  ����������   �������������W��X  ��1�QQP�u���������H�������W���   �����������B����]������w�jV�� ����V�u�1 �������������  �����������ȋ���������1�A�;E�uK�ȁ�,  ^_[]Ã��������D�  ����H������������������� ��V���   ������; U��SWV��<  �    _�u�U���H ��E��}$���H � ��r) QVjR��(  ��1�A�  �� ����v���H ������H �L$�t$�$�PP��1�A���f  �v������������������QR�M�q�u���   �����E�
����P�����P����v�j�������V��  ����V�u���  ������������  ����������肄����1�A�� ����;E�uK�ȁ�<  ^_[]Ã����������  ����H�����������������������W���   ������� U��SWV���   �    Y�}�]��NA ��E���&��:A � ���" QWj�u��(  ��1�@�N  ������������j6jP�+�  ���������P�w�u���  ����1�@���
�������V�r�  ����V�u���  ��1�@�������������������������t��S��  ����t��W��  ���������f���� ��  �  D  �  U��SWV���    X�M�u�}��   �Y��>; �����F; �D$�\$�4$�QP��1�F����   �C�E���A�I)��   9�ug���p�0��a������t[����  ���u�藝������P��  ����P�u�փ�1��>��>; ���F PQjV��(  ��1�F���Sj���Sjj8j�u���  �� ����^_[]�U��SWV���    X�]�}�U��!��H: �	��d PSjR��(  ���  �s��H: �M��	����P: �E�D$�t$�$�QP��1�A����   �F�E�s�E�� ���u�V�u�PP������   ���v�u���������ty������M�Ɖu���E��U���   �3�u� ��RVQ�PP������   O�����E��p�u�V輈������ƃ�Q�T��������Mu����   �u�Pj9PQ����   VPj9P�u��  �� 1�A�ȃ�^_[]Ë ���  ���u���  ����PW�փ�1��Ѓ��u���������U��SWV���    X�M�u�}uq�Y���8 ������8 �D$�\$�4$�QP��1�F��u{�K�A�I)��   9�uR����  ���0�Ǿ  ����P�@�  ����P�u�փ�1��6���8 ���[ PQjV��(  ��1�F���Sjj;j�u��  �� ����^_[]�U��SWV���   �    X�]�}��
8 ��M���&���7 �	��� PSj�u��(  ��1�@��   ������S��������7 ������7 �D$�T$�E�$�QP����t1�@�   ������G��� �����������p�������ƃ��� ���V�������������j<jP�v�  ����|$����������P�3���uV�3  ��O��⋅���� ���  ��V�  ����P�u�׃�1�������;M�u���   ^_[]��h
 U��SWV���   �    X�}�u�M���6 ��U�����   ����6 ������	�����6 �D$�|$�4$�QP��1�F����   �G����������� ���M�q��X  ��������������j=jW�d�  ����W������������������t]������	���  ��P�  ����P�u�փ�1�����6 �	��� PWjV��(  ��1�F�;E�u$�����   ^_[]Ã������P�u��  �����/	 U��SWV���   �    X�u�}�U���5 ��M���$��t5 �	��� PVjR��(  ��1�A�  ������^��t5 ������	����|5 ������D$�\$�$�QP��1�A��t������m  �C�� ����^������ �������S�u�PP�����5  �C�������������j>jS���  ��Sj �������� �������������   �����������������M�� �������   ������������� �������SQ�PP������   O�������������Pj �s�� ���V�t�����Ã�V�~�����ۉ؋�����Mu��������PQ���  �����������P�u���  �=������ ���  ���� �������  ����PW�փ�1������ ����}���������1�A�;E�u
  ����P�u�փ�1����V3 �	��z PWjV��(  ��1�F�;E�u$�����   ^_[]Ã������P�u�S�  ������ U��SWV��L�    [�u�U�}��  �~�� 2 ���(2 �L$�|$�$�PP1�A����  �]̉u��}܋E���A�E�� �q�u���A�E�� ��$�%y���EԋO;O�    �]��  �}�M��$�q�  �E�� ��D$�M؉L$�$�D$   �D$    ��  ���G  ��M��L$�t$�$��(  ���)  �}� ��   ��MЉL$�$�D$    ���  ���V  �M̋��  �����$�މ����  ���ǋ��,2 ��L$�|$�$�PP����  �_�E܋ �K�L$�$���  ��u
��   �E��uԋ�K�L$�$��  �ǉ|$�\$�4$�sy���<$��  �U��H��
���}���$���   �M���E�;H�]�u������1��g  �� 2 � ��\ �L$�t$�$�D$   ��(  ��  �u��}�U��H��
�у��  �E� �$���   �  �|$�]�$�D$   �D$A   �D$   �P�  ��   �u�����  �M̍�� �$�D$������	  �v��}�u��� 	  �U싸	  �$�މ���|	  �D$��� ��$�׉D$�$�U��s�u�����  ��� ��u�����  ��� �$�D$�������   �D$�$�֋�}苰 	  ��	  �$��|	  �D$�E̍�� �$�׉D$�$��1�@�u��E�u��E� ��   �u؉4$��X  1ɉL$�L$�D$�$�׋UЋ�H��
���E� �$���   ��H�����}��E� �4$���   ��H�����E� �<$���   �}� t�Eԉ$�Kx��1�A�ȃ�L^_[]ËE� ���  �Eԉ$�I  �D$�$��1��Ӑ!  �  !  �  �  U��SWV��,  �    X�u�U���- ��M��}/���- �	��� �D$�t$�$�D$   ��(  1�A�J  �v���- ����������- �D$�t$�$�QP1�A���  �������F�������8�E�@�� ����E�p�������4$��  �������4$�&�  ������}|}1���M�L��$��X  ������D$�<$��  ��;_�	  �C�������L$�D$�\$�<$��  ���*  +_���\$������$���  F;����������|��������G+G���   +M�D�$�K�  ������G+G���D$������$�X�  �Ë3�)�G����D�������L$�D$�T$�<$���  ��;su҉������������ �� ����4$��X  �������L$�$�P���ǉ|$������$���  ;C��  �$�Ss���������������N+N���L$�$�Xt���N;N�	  �������������$�Ҭ  �������x��������(������@����D��|$�D$�$�x�  �<���;^uӉ������������$�~�  �� ���������;Xt<�� ����p������@����D��t$�D$�$��  ������4���;Yu͋� ����$���  �� �������D$�������$�D$    �s����������������;H����������������$�4�  �������$�&�  ������� ���  �������$�  �D$�E�$��1ɋ�������   �t$�ލ�����$�D$   ���  �\$�E�$�I�  �<$�O���4$���  �������$���  ������$��  �������$��  �   �������$��  ������$�k�  ������D$�E�$�D$   �D$@   �D$   �A�������$�i�  ������$�%�  ������D$�   �D$�D$�E�$�D$@   �d�  ������1�A�;E�u
 � �$   �P���$   �(�  �F����^]�U��E� �H+������]�U��SWV���    Y�}��P;u>��� � �$   �P��1�C�$���  �F���t$�$���  ��$��  ��$��  1��8 u@�]��$��  �@�\$�$��  1�F��t�C�{�@�$��f���D$�<$��  ����^_[]�U��V���u�E� �$�J�  �@�t$�$�Y�  ��^]�U��VP�    ^�E�@�$��  ��� � �@��^]��U��WV���}�E�H<���U�RWQ�Pl����1��}� t��W�
�  ���N;At����RPQ�"�  �����u��s���  ���K;At����RPQ���  �����u��w��  ���O;At[����RPQ���  ���F�E��x������;Xt0�3���u��v�l�  ���N;Atۉ���RPQ��  ���ƋM���^_[]�[���   F   �   �  U����E�H<���uQ�Ph����1���t�A��]�U��SWV��<�    X�E̋E����   ���S  ����  �H�M��p��M܋V�U؋A�@�E��B�x���v�v�J  ���G+G����1�Q1�P�a�  ���ǃ�Wj�v��\������S��  ���É]��1�P��  ���E�V����PS�v�o  ���É]��W�$�  ����u:�}̍���  �M�U�P�u�]�SV�  �����  �M�U�P�u�SV�  �����u����  �����u����  ���M��A+A����1�SP��  ���ǃ�WS�v�!\������S���  ���É]��1�P���  ���E�V����PS�v�
�  ���C�H�H�S�UЋB�p�uă��M��s(�s$�
  ���F+F����1�Q1�P��  ���ǃ�Wj�s(�5Y������V���  ���Ɖu��1�P���  ���E��S ����PV�s�
  ���Ɖuԃ�W�i�  ����u:�}̍���  �M�U�P�u�u�VS�P  �����  �M�U�P�u�VS�6  �����u���  �����u���  ���M�A+A����1�VP���  ���ǃ�WV�s(�fX������V�/�  ���]��Ã�V��  ���E��1҉�PSV��	  ���ƉuЃ�W��  �����u�u9�}̍�7�  �M�U�PS�u�V�  ����R�  �M�U�P�u��u�V�j  ����S�S�  �����u��E�  ���F(�H�H1�@�~, ����   �U�J+J���uȋV+V���}ċw+w����9�tY�}̋�U �E� ��1�J���  RQ�uV���  �����M���V�R  ���E� ���� �  1�JRQV���  ��1�1Ƀ}� t�}� t�}� t�}� t�������;  �H�Mԋx�}��M�G�p�u܋A�@�H+H����j Q�-�  ���E��G�1�C9���   �E̍�R�  �EЉM����M�x�A�p���U�W�E��p�  �����u�jW�wV�����N+N��9���   ��j�.�  ���ËF+F����Pj W�}���U�����Ƌ>�F���9�t$�? t��WV�x�  ����PS��  ���F�Ճ�V�x�  ���M�U��u�S�u��u���
  ����S�T�  ��1ۋ}؋G�G�M���;M������]���1�V��  ���E��V�s�  ���E܋]��3���;stW�>��t��VS���  ������|P�u��P�u��p�  ���E�    ���Ű�U � ������  QR�P��1��j�u̍���  �}���U�P�u��u�}�W�!
  ������  �M�U�P�u�V�u�W�
  �����u��b�  ����V�V�  ����S�J�  ���E���<^_[]�U��SWV���M�E�I�y�p���L$�$裼  �Å�t0��$��U���ǋF�N�\$�|$�L$�$�S[���$���  �
�4$��W���ǋE�$�M���   ��^_[]�U��SWV���׉��    [�E�$�  ���Ft1�H�$�Q�F�x�F�� �H�� �	�$���   1�@��P�$    ���R  1���^_[]�U��SWV���    X�E��]�}�G�p���uSV�zV������t(�F�N)����Q��uR�t��W�]   ����t81�@�I��S輖  ���ƃ�Vj�u��  ���E����
�M�r���E�PSV�R4���}� t�M�H��8O��u�1�@�=�}��t4���u��  ���ƃ�VjW���  ���E���� � ��V�P��1���^_[]�U��SWV���}�u���6W�\   ���G�X�F��PVS�Xs�����E�;Ct/�u����6W�,   ����+C��j P�6W� �������;suԋE���^_[]�U��SWV���    X�u�M�M�~��t/�� ��� ��E�F�M�L$�$�V0��$���  ��<Ouڃ�^_[]�U��SWV���]�E�$���  �Ɖ4$��  �{��t�� �]���D$�4$�M�  ��u��<Ou�1�O�+]���i�����4$���  ����^_[]�U��WV���E�M�Q�r�E�kE<�T �U�T$���}�WR�TP����t�@��F��F��^_]�U��WV���u�F�$��  �ǋF�$��  �$    ���������^_]�U��VP�u�F�$���  �$�T���F�$�I�  ��^]�U��SWV���    X�u�}�]��q
 � �$   �P�X�x��N��F��^_[]�U��SWV���    X�U�M�Z��/
 �E�� �$��X  �E�1�����   �M1�1��E��9Nu(�E�� �ˋN�$��X  �M�L$�$��  �م�t
���6��u��K���t����E�X�F��Q�����M��	�$���   �E�� �4$�P�E�@�ۋM�{�����E��^_[]�U��SWV���U�M��    X�Y��t>�M�y$��W	 ��o	 �E����W���  ������u�j�W���  ����<Ku׋u�^1�@;^tD�uj �3�}�W���������t&��;^t"��+F���uP�3W�����������u�1��1�@��^_[]�U��SWV���׋A�X��j �u�/M�����E�s;stLkE<�L$�M�|P��E���E�P�u������t��+K�����pQ�u�L������;suƋE��P���  ��,^_[]�U��SWV���։M���y1�@9�tT���tF��u��|��S�u��r�  ����P�u�Mu ��S�u��U�  ����P�u�b�  ��1���9�u���^_[]�U��SWV�����    [��t
�M�A;u���x  �E�@�@�E���� ����U�1�J����  RQV���  ���E��M�@�E���1�IQ�u�V���  ���������  1�JRQV���  �����1�IQ�M��1V���  ���������  1�JRQV���  ����E�E�H�P����
 ��1�HP�4�V�E����  ���������  1�JRQV���  ����E�U�J�R����
 ��1�HP�4�V�E����  ���������  1�JRQV���  �����1�IQ�M��1V���  ���������  1�JRQV���  ���E������u�u�u��   ��,^_[]�U��SWV��,�U����    _�E�@�E苟 �]���1�J��=�  RQV���  �����1�IQ�u�V���  �������J�  �M�1�JRQV�����  ���M�	;M���   ��L�  �E܍�S�  �E؋}�1�N�M��E�@�M��	���4��\�  ���E���V�u�S���  �����V�u�S���  ������u��P�����V�u�S���  �����V�uS���  �����V�u�S���  �M��E����9��c�����,^_[]�U��SWV�����    _��t
�M�A;u���K  �E�@�@�E���� ����U�1�J��Z�  RQV�}����  ���M��E܋A�E�@�E���1�IQ�u�V���  ���������  1�JRQV���  �����1�IQ�M��1V���  �������i�  1�OW��U���QR���  ���M�9;}�tC�E䍀{�  �E�� ���V�0�u����  �����V�u��u����  �E����9�uɋ�u��N�����QV���  ������M䍉�  j�QV���  ���E������u�u�u��Q�����,^_[]�U��SWV���׉��    X��t
�]�K;u���T�M�I�I�M���K �	����<�  j�PV���  �������UV�!   ���E������uS�u��������^_[]�U��SWV�����    [��E�F�E�F�E���� ���1�JRQ�u���  ���������  1�JRQ�u���  �����1�IQ�M��1�MQ���  ���������  1�JRQ�u���  ����E�N�V���" ��1�HP�4��u�E����  ���������  1�JRQ�MQ���  �����1�IQ�M��1�u���  ���~, t�������  j�Q�u���  ���������  1�JRQ�MQ���  ����N�V ���" ��1�NV�4��u���  ���������  1�JRQ�uV���  �����1�IQ�M��1V���  ������� �  1�JRQV���  ��^_[]ÐU��SWV���    _�]�u�U��& � ����J�  QSjR��(  ���*���M�Qj ��_�  Qj��� Q�sR���  �� ��t1�@��E��uSV�u��ǎ ����^_[]�U��VP�    Z�M�E�}	u�����uQP�$L  ���"���  �6��g�  RQjP��(  ��1�@��^]�U��SWV���    [�u�}��N  �E� ����n�  QVj�u��(  ��1�F�	  ���M�����  Qj Rj��� Q�v�u���  �� 1�F����  �E�E�E�����   ���J  �������E��]�1�����  O����M��1���M���X  �����uP�u�Ef  ���ƅ�t��n  ����   ���u�E�p�u��g  �G  ����   ���u�E�p�u�q  �%  �M�	������  PR�Q�Y���}���   ���E�p��X  ����P�u�E��S�f  ������  ��������P���   ����PS�փ�1��   ���U~V�E� ����  Q�ujR�7���u�E�p�u��q  �{�E� ����  ��E� ����  Q�uj�u��(  ���T��u"����  �5����  P�uj�u��(  ���-�E� ���M�q����X  ������uPR�o  ���Ɖ���^_[]�f��   )  �   �  �   �  U��VP�    Z�M�E�u�����u�����uQP�Y  ���"����  �6����  RQjP��(  ��1�@��^]�U��SWV���   �    Y�]�u����  ��E�����  � ������  QSj�u��(  ���p���s��X  ��������������j	jP�m�  ���������P�s�u�)�  ����t)������������QSVP������u�u��8  �� �1�@�;M�u���   ^_[]��n�  U��SWV��,�    [�U�}�}����  ���   ���r��X  ����P�uW�
?  ��1�A����   �>�E�H������  �T$�L$�E�$�WP����u]�u���]�S�uV�u�O@  ����u@�C�x��SV�V�������t6�{ �؋MtG���qj-�,����  QRjW��(  ��1�A�ȃ�,^_[]Ã��E�pj,jj�u�]�  �� ���@   �A�E�� �A�E�� �}؋w;w�E�    �    ��   �E�    �u���6�d�  ������u���h   Wj �u��]S��  �� ����   ����M�Q�u�S��(  ���Å�ui�}� t/��W�u�V�u�u  ���Å�uJ���u�V���������E���u����H�����E� ��W���   ��1ۋE�;p�A����*��H���
��H��1�C���E� ��W���   ���E�1ɉH�E� ��   ���u�V��X  ��1�QQP�u�׃���H�����E� ��V���   ���U܋�H��
���u�E� ��R���   ����������PV�]S�Ro  �������!����ߋ]ԅۋu�t����M��q���   ������  ��S���   ����PW�փ�1������U��SWV���   �    _�u�U����  ������ �E�����  �}����  QVjR��(  ���w���v����X  ����P�uV��;  ��1�A��tR�3������H������  �T$�L$�}�<$�VP����u#�� �����������]SW�&=  �����ut!1�A������ ;E���  �ȁ��   ^_[]Ë�����G�@�������������jjP��  ����WS���������t#������ tA���vj-S�E�  ����S����vj,�������V�&�  ����V�u�z�  ������G   �E������������SPVRW�u���1?  �� �ǅ���   ��W�����V��������Ã�W�z  ��ǅ����    ������;X�    ��   ���3���  �������WV�u�1r  ����1���u+��SV���G������� ���� ���v���   �ڃ�1�@��������H������ ���� ��W�����   ����������ǅ����    �F    ����P�u�]S�rl  �������-����� ���� ���  �����������   ����PS�׃�1��������  U��SWV��  �    _�M�U����  ������ �E�����  �}��΍���  QVjR��(  ��1�A��   ���q����X  ����P�uV�9  ��1�A����   �������H������  �|$�L$�4$�RP������   �������� �����������}WV�C:  ������   ������C�@�������������j>jV�?�  ����SW�$�������t'�{ �؋]��t^���qj-��������������,���E�pj,V�d�  ����V�u踲  �������1�A�;E���  �ȁ�  ^_[]��@   �u�������������������M��   ������8�� ���� ��������WQ�PP������   �����Pj�wS�y@������;�����t��S�?����N�����������u��������P�E��V���  ����������   ��S�����������������SW�����V�Or  ���������t��SP������������1Ʌ�u��S�u茱  ��1�A�������H������ ���� ��W���   ���E�����������������E�����1�A�C    ��Q�uP���Ei  �������m����� ���� ���sW���  ��1��N�����  U��SWV���    Y�u�]��&����  � ����  QVj�u��(  ��1�G��   �M��}��j W�%�������H+��QjW�u�r  ����u�F�}���}�����  � �������VS�PL������  ���j P�uV���  ���E���u9��� 	  �M싘	  ��V��|	  ��������  PQ�Ӄ���PV�U��1�G�]�9��E�E��H+��Qj RV��q  ���E��P�uV�yf  ����D�����^_[]�U��SWV���    Z�M�u�}����  �u9�]���q��X  ����PSV��4  ������  �����   t ��j����  RQjV��(  ��1�@���j �Ѓ���PV�׃�1���^_[]�U��SWV���    ^�M�}����  ��?  �}���q��X  ����PW�}W�O4  ��1�A���,  ��M��ǋO�����  �T$�L$�E�$�E��PP�����:  �G�@�E���1�VV���   ���E�O����   �� �M܉}��?�}���VV���   ���E��7;wtH���6�E��0��  ������   ��j��0���   ����P�u��u�׃���up���E�;pu�����u��u��u���   ����uJ�}���<�M�I�    �c�������u��u���  ���`��~�  RQj�u��(  ��1�F�E���A�U���H��
�����R���   ���U��H��
�����R���   ��1�@�Ɖ���^_[]�U��SWV��  �    ^�U�]��@�  ������ �E�ǅ���    ��,�  �}���%�  QRjS��(  ��1�A�   ���r��X  ����P�uS�M2  ��1�A����   �������H����4�  �T$�L$�E�$�SP������   �� ����������W�uV�u�3  ����ug�������jjS藫  ����WV�|�������t� ���EtV���pj-�������,���E�pj,S�ƫ  ����S�u��  �������1�A�;E��  �ȁ�  ^_[]��A   �A�@���0���56����������ǅ����    �E����   �ǃ�j �/�  ���������SP�E�pV�u�4  �� �Å���   ���1���������s�������C7�������6  ��������H������ ���� ��S���   ��1���~Q�������sO�������虮  ���������P�����P�E�t�������u�r3  �� �Å��c���1�F�������1��1�F������]��������:�  ��������@    ��V�uS�b  ���م�t�����������v7������o���������˃����� �� ���t����r���   ������  ���������O�������PS�փ�1�������������Sj�������V让  ����V�]S��  ��1�F������%����z�  U��SWV��  �    _�M�U����  ������ �E�����  �}��΍�_�  QVjR��(  ��1�A��   ���q����X  ����P�uV��.  ��1�A����   �������H������  �|$�L$�4$�RP������   �������� �����������}WV�0  ������   ������C�@�������������jjV��  ����SW���������t'�{ �؋]��t^���qj-��������������,���E�pj,V�?�  ����V�u蓨  �������1�A�;E���  �ȁ�  ^_[]��@   �u�������������������M��   ������8�� ���� ��������WQ�PP������   �������P�wS�8������;�����t��S��4����N�����������u��������P�E��V�Χ  ����������   ��S�����������������SW�����V�)h  ���������t��SP�����������1Ʌ�u��S�u�f�  ��1�A�������H������ ���� ��W���   ���E�����������������E�����1�A�C    ��Q�uP���_  �������l����� ���� ���sW���  ��1��M����^�  U��SWV���   �    [�U�}�E����  ������	�M�������  ���   ���r��X  ����P�uW��+  ��1�G����   �������H������  �\$�L$�E�$�RP������   ��������u�u�$-  ������   ������@�@������E�H���S��S�u�PP����ug�C�� ����������j&jS���  ����������u���������tN������x �Mtd���qj-�@����  QRjW��(  ��1�G������ ;E��3  �����   ^_[]Ã��E�pj,S��  ����S�u�F�  ����@   ����W�� ������������6��������   ��P�����������QWS���u�e  ����t��SP������!�����1Ʌ�u��S�u�ˤ  ��1�A�������H�������W���   ���}������E��������W�}W胤  ��1�A�E�C    ��QPW�\  �����ǅ����������sQ���  ��1������ʺ  U��SWV���    _�E�M�ʃ������   �U�Z1҃��    u���  �	���p��X  1҃��Ƌ��  �}����RR���   ���E����M�QS��L  �����M�Q��  ���Å���   �E�@,��t���u�[�����t���VS���  ����t�������   ��j�S���   ����P�u��u�։��}�����t��U��H��
1�F��J���R���   ���9���  �	����  RPj�u��(  ��1�F�����u��u���  ��1�����^_[]�U��SWV���    Y�}�]�u������  � |*�ڃ�t#��V��l  ��������uWSV�:  �������  QWjV��(  ��1�@��^_[]�U��SWV���    Z�M�u�}��C�  �uJ�]���q��X  ����PSV�'  ����t:����  ��j��0���   ����PV�׃�1������  RQjV��(  ��1�@��^_[]�U��VP�    ^�M�U�E��}$����  ���;�  VQjP��(  ��1�@������u�t�������QRP��H  ��$^]�U��SWV���   �    ^�}�]�U��m�  ������ �E���Y�  ������  QWjR��(  ���i��������w����X  ����P�uW�&  ��1�A��t>������	�� ����H����������a�  �T$�L$�<$�� ����PP����t!1�A������ ;E��)  �ȁ��   ^_[]Ë������@�@�� ����������j/jP賟  ���������   �E��������������QPRS�������u�5*  �� �ǅ��   �s������� ����3�*�����ƃ�W���������������������ǃ�Q�fe  ��;{t��j �7V��+���������� ���  ��V��������P�u�׃�1����������.�  Pj	�������V��  ����V�u蕟  �U�����  U��SWV���   �    ^�U�M�}��s�  ������ �E��A�����_�  �r��g�  QRjW��(  ���l���r��X  ����P�uW�$  ��1�G��tI��ǋO����g�  �t$�L$�E�$�RP����u���������S�u�u��%  ����t!1�G������ ;E��+  �����   ^_[]ËC�@������������j3jP蹝  ���}�   ��  �� ����E�X������ ��VS�u��Q�PP����u��s���������6������0�S�  ����t2���� ���S�u�<�������te�{ �؋M��   ���qj-�W���6��  ���Ã�Sj�������V�%�  ����V�u�̝  �������� ��S�P����������E�pj,�������V�6�  ����V�u芝  �������@   ������������SQP�u����]  ���ǅ�tQ��SWV������1����؋]u��P�u�2�  ��1�F��H���������� ��W���   ���}�	1�F�}�]��������H��
�������� ��R���   ���� ����@    ��VS��W��T  ���ǅ������������ ���s�u���  ��1��������  U��SWV���    _�]��`�  ��}����  Q�ujS��(  ����   �M���M�����  ����  Qj RjV�ދ]���qV���  �� 1�A����   �E� ���M썗��  ����  Qj RjV�ދ]�sV���  �� ��un�M�E��tp����   �E� ���s��X  ����P�uV�!  ����t3�M����#  ����  ���}�t  �s�sPV��S  �'  1�A�ȃ�^_[]ËE���w9����  ����}�  ���sQ�u�W  ��   �M�	������  ��M�	����X�  PR�Q�����}��   ���sQ�u��V  �   �}��   ��Q�u�W  �   �E� ���  ������  j�Q���   ����P�u���L����}��   ��PV�)T  �D�}��   ���sPV�U  �+�E� ����X�  QR�P�����}uo�s�sPV�S  ����������E� ��q�  ��E� ����  ��E� ����  QSj�u�����E� ����  �%�E� ���  ��E� ��5�  ��E� ����  QSjV�u���f�,  r  �  �  �      �  �  U��SWV���    [�M�}����  �up���U���0�  Rj V�uj���  R�qV���  �� ��uY�M�E���tY��t{����   ��j Q��辽������H+��QjV�u�'[  �   ��0�  RQj�u��(  ��1�@��^_[]Ë�ϋH+��Qj WV��Z  ����jWV�O  �?��ˋH+��1�QVS�E��W�Z  ����VSW�\O  ��뢋����C�  PR�Q��1��U��SWV���   �    _�]�u�U��y�  ������ �E�ǅ���    ��e�  �����^�  QSjR��(  ���   �� ������s����X  ����P�uS�  ��1�A��tX�� ����	������������H����m�  �T$�L$�$�������PP����u��������}WS�  ����t!1�A������ ;E���  �ȁ��   ^_[]Ã������j<jS誖  ���������W芼������t.������x �]tD���sj-�������V�ݖ  ����V����E�pj,S�Ö  ����S�u��  ������@   ������@�@��ǅ����    �0�C!�������������������������I�  ����~nN�������P�����P�3��������u�  �� �ǅ�t���������w�������W"������H������� ���� ��W���   ���w���1�������G��P�u�uV�[N  ����t�����������#������l������������� �� ���t����q���   ������  ����������������PW�փ�1������W�  U��SWV��  �    _�M�U����  ������ �E�����  �}��΍�<�  QVjR��(  ��1�A��   ���q����X  ����P�uV��  ��1�A����   �������H������  �|$�L$�4$�RP������   �������� �����������}WV��  ������   ������C�@���������   �����PjV��  ����SW�ع������t'�{ �؋]��t^���qj-��������������,���E�pj,V��  ����V�u�l�  �������1�A�;E���  �ȁ�  ^_[]��@   �u�������������������M��   ������8�� ���� ��������WQ�PP������   �����Pj �wS�-"������;�����t��S� ����N�����������u��������P�E��V訓  ����������   ��S��蝮������������SW�����V�T  ���������t��SP�����������1Ʌ�u��S�u�@�  ��1�A�������H������ ���� ��W���   ���E�����������������E�����1�A�C    ��Q�uP����J  �������m����� ���� ���sW���  ��1��N����8�  U��SWV���    X�}�]�u��!����  �	��\�  PWjV��(  ��1�@�%�����1���~K���7���uV�a  ����t��^_[]�U��SWV��  �    _�U�]�E��&�  ������	�M������  ���   ���r��X  ����P�uS�S  ����1�@����   ��J����������  �T$�L$�$�PP������   �� �����������uVS�  ������   �ߋ�����C�@�������������jAjP艐  ����SV�n�������t:������{ �Mts���qj-���2���  QRjS��(  ��1�@������2���E�pj,�������V蓐  ����VW��  �������1�@�;M��K  ��  ^_[]��C   �A������� �A������� �A������� ���������0���������������؋Xǅ����    ;X�D  ���������3�'�  ������� ������h   Sj ������W��  �� ����   ��������Q������W��(  ������   ����� �ދ�����t1�������P������V������S�����W�(  �� ����   ��H������ ���� ��V���   ����������;X�%����   �ڋ�H��
1�A��������������uJ�� ���� ��R���   ���3��H��1�A���������u�� ���� ��S���   ���������������D��H������ ���� ��V�����   ���1���E������������������u�B    ������������PVQ����F  ��������������A;At�� ���� ���v���   ���� ���� ��   ��������V��X  ��1�QQP�u�׃���������H��
���� ���� ��R���   ����H������ ���� ��V���   ����������H��
���� ���� ��R���   ���������� ���t9����� u0� ���  ���������ʨ������P�u�փ�������������������������1�@������  U��SWV��  �    [�U�}�E��j�  �� ����	�M�����V�  ��F  ���r��X  ����P�uW�  ��1�A���3  �>�������H����^�  �T$�L$�]�$�WP�����   ��������W�uS��  ������   �G�@�������������jBjP�ڋ  �������������QR�}���qS�����   ������   ������������QP����������������SW�;  �� ��tc��P�����S�����������������W�Q  ����S�u�9�������tJ�{ �؋Mt[���qj-�@��l�  QRjW��(  ��1�A�� ���� ;E���  �ȁ�  ^_[]Ã��E�pj,jBj�u�,�  �� ���@   ���������3������������1ɋ�����;S�   �E�X���������2�2�  ��������h   Wj S�]S��  �� ��tB�������P������W�E�p������������S�o  �� ��������u.1�@�������:��H��1�A��������%���W���   �����������q���   �����   ����������X  ��1�QQP�u�Ӄ���H�������W���   ���������������@    1�G9�������P�u�u���sB  ������Dù   ���T�����u����������������7�������  ����������������P�u�փ��������}�  U��SWV��<  �    ^�U�E����  ��M�������  ���  ���r��X  ����P�u�u�
  ��1�A����  ��H������  �t$�L$�M�$�������RP������  ���������u�u�J  ������  �������������@�@�������E�x���VW�u�PP������  �������G������� �p;p������tB�>���7�3�xt  ���;A�Z  ���0W�\��������M  ��������� ;pu����������v軕  �����������v�Y�  ����������y �u�R  �y ǅ����    �� �����H+����Q��  ����������8;x��   1ɉ�������7�3��p  ���Ã��3�������6��t  �������   ��P�6���p  ����PS�w�������tp��V�������ی  �������A���� ���� ;x�������w������؋]�މ�xY����������������  ������������������  ���)���������֊  ������� �]�ދ�������  �� �����<������������kA<�D 9���������������F;uh��V�Ӕ  �����������m�  �����E�pjjCjW�Q�  �{  ��ܴ  QRj�u��(  ��1�A�;E��v  �ȁ�<  ^_[]É���������������������������� �H+H����Q�G�  ���ǋ�F�	�������9�t/�������3;st���6W蹍  ����;suꋅ����� �ċ������ �H+H����QW��  ���Ã�W�}�  ���������jCjW��  ����������V�EP����������   �������~ �������u��   ����������  �����E�pj-�������S�#�  ����SV�y�  �������7j����7jjCj�u�>�  �� �������������E�pjjCjV讆  �� ���������1�@������������  ��S臈  �����E�pj,W蓄  ����W�u��  ��덃��������0�*�����������������Q1�;Q��������  ��������F9���   �������������0ǅ����    1�H�������ǋ������� �������������������k<�L$�������WQ�TP����t9�x�����������9��������������tG��������������������������������� �������� 9��n����������1�N��1��"������� �������������������������)���9Ƌ������m  �������p���8|��������P�M  ������������������������������������������������������;X��   ������A�I�� �����������@�����0�������v�n  ���Ǎ����P�� ����������4��7�u��������N�F+y�8�����u����� �����r��1���� ����Y���������� ��Q�����   ����������@�2������������8x  ���������Q������P�������������u�{  �� ��������������������u��1�;Q�o�������������苅  ���������@    ��������P�u�}W�:  ����t9��u5������������ �󋰴  ���������Ŝ������PW�։ރ���������������1�@���������������������������  ���6����6�܄  ����;3u����������  ������������×  �U��SWV��  �    _�] �u���  ������ �E���V��������t����H�  Pj)S�2�  ����S�@  �U��������]��P�  �����u�uSV�D��������i  ��������������  ��������Q���  ���E����  �]������I ������ǅ����    ����������ǅ���    �� ����QR�3�uV���   ������  ���������  ������k����<������������L �������� �����P蹂  ���Ë�����H��������~P���X  ������Q��������1�Ѓ��ƃ�V�u�l  ������  ��PS��  ����u��  ��������S�ڇ  ������� ������������t4���7S�X�  �����e  ��S�7�B�  �����O  ��<���ű�������������������������L$�����������  �T$�$�D$�������  ���������������������<�� ����������������e������h   ������sj �3�uV���  �� ����   �����������a  �uQj"j �3V���  �� ����sV���  ��1��   ��Sj"�E ��V�x}  ����V�u�~  �����  � �������Q���  �n��Vj�u V�=}  �.���������0j���S�   ���������0j
  ��þ  �E�� ���u��X  ����PSW��   ����1�G����   �E�� �K����˾  �T$�L$�M�$�PP������   �U�C�H;��   �{ t�C   ��1��M��WWW�8  ���{�E�� �������1��URQj"W�3�MQ��  �� �u��WW�3�u��   ����S�u�.���������u��l  ������3j'jjR�{  �� ����^_[]�U��SWV���   �    X������]�}��z�  ��E���S��  ����r'���:u�C��:u��S�u�����������   ����������������W��  ���ƃ�������u辛�����Å�us��uo�������f�  ���W���  �����W���  �����1�J��ب  R��QW���  �����S�uW���  ����������u�G������Ë������f�  � ��W���  ���������u���uj#j j�u�3z  �� �;E�u
b  ����t.���s�6��g  ���M��uVPS�u���������t���G��Wj�uV�Gv  ����V�u�v  ����H�����E싀X�  � ��W���   ��1���^_[]�U��SWV���    Z�E�u� �~ t+�~ u%�M�F   ��1���M�  ��WPS�3  ���~��^_[]�U��SWV���    X�u�}�M����   �E��E�@�X�]���������P���x  ���E���3��9  ����ƅ�~m���E����  �E�M��E� ���7��X  ���Ã�S�E��0�rb  ����x}��P�u��z  ���u�wSV�(=  ����t|�M������������u��u艬������x?�M��t����u��x  ���m���7j	V�t  ����V�u�u  �J��Sj����E���f�  Pj�u�-t  �����u�u��t  �����u��Ax  ����V�:  ��1�����^_[]�U��SWV���    ^�} ����  ���j �u�u���  ����ug��� 	  �M���	  �E��W�v  ���E��W�v  ���ǋ���M��S��|	  �����  PW�u�Q�U����PS�U���1�@��U��t������t���)�u�����S���  ����WVP�u�uS�   ��,^_[]�U��SWV���    X�u�M�U�I�I�M�����  �������  �|$�t$�$�QP������  �}�]�E�M������0�0i  �����VS�u�u�  ���ǋ�H��1�A9��E� ��S���   ����H�����]����V���   �����M��&  ���QW���}W�PP�����  �F���p�]��3��]  ������   ���}�7�EP謩�������uV�EPWS�׏���� ���߉�uY�uV�u�u�������ËE��+G��j P�1�u�J�������u&��Vj�E��S��q  ����S�u�Fr  ��1�C��H�����E� ��V���   �����D��Vj�E��S�q  ����S�u��q  ����H�����E� ��V���   ��1�@��^_[]�U��SWV���    Z�E�u� �~ t0�~ u*�}�M�F   ������  ��PWS�#/  ���F    ��^_[]�U��SWV��  �    [�}��e�  ��E���W�[�������t&����m�  �   Pj)QQ�u�r  �� 1�A�u  �������M��Q�  ����q��X  ����PW�u�h�����1�A���4  ��������ǋO����Y�  �t$�L$�E�$�RP�����F  �������������G�@�����������������������QR�]�s�u���   ������  ����������������������׉  ���������  Rj P�   PW�s�u���  �� ����  ����������s����X  ����P�u�u�m������Å��~  ����������s�u�PP�����_  �C�@�����������������QR�w�u���   �����,  ����� ���Qj ������j�������w�u���  �� ����  �� ����@����������   t���   �wj(��  �����������;������  ��1�PP�o������ǋ������ ������������� ������������H����������   ���������������0��X  ����P�������[  ���Ë��������1��X  ����P�������}[  �����M  ���M  ��PSW�)������������������   ����������Q���������e�����1�FVW��������VW�������������ǃ�W�������T������Ɖ�������W�tq  ������  ��j�������:������������ ���M�1��X  ����������M��V�p��������������uP输�����ǅ���  �G��������ʋ�����I����������  �H������I����  �H�������p�� ����I����  �H�� ����I����  �H�������H�������H��W�r���h|  �����6�3覃  ����t��W�v�F|  ���M���:  1���u���u�������˕����1�G������� �������Q���  ������������   ���   �wj$PP�u��m  �� ������1�A�   ���   �M�qjPP�u��m  �� ��������������{��������������   �0jQQ�u�m  �� ��W�������됃��   ������j%PP�u��l  �� ��������������������� �������Q���  ��1�A�������;E�u
���(�   u���������pj+jj�u�a  �� ���������Qj ������j�������������q�u���  �� ����  ������ u:�������@���$�   t
���(�   u���������pj+jj�u�`  �� ����������q��X  ����P�u�EP�g��������0  ���������R�p�u���QP�����  �G�������@������������� �� ��������QR�������w�u���   ������  ������� ���w ��X  ����P�u�u��������ǅ���  ������� ���������w�u�PP�����q  �G�������@�������������������������QR�������q$�u���   �����&  �����;�����G  ������;�����E  ����������1��X  ����������M��W�`��������������uP���������������  �������x��������������������G�������@���$�  �G�������@���(�  �G��������������1�PP�h������G�������G�������@���$�  �G�������@���(�  �G ��1�PP�'������G(�������G,������� ������������� �������w������H����������   ���������0��X  ����P�������XI  ���ǋ�� ������1��X  ����P�������.I  ������  ���  ��PWV��������������������   � ���������Q���������e�����1�GW������V�;�������WV�K������ǃ�W�������������������F��W�$_  ���~ �c  ��j��������������������@(�������������0�������H�����������   ���������0��X  ����P������� H  ���ǋ���������1��X  ����PV��G  ������  ����  ��PW�������������������������   ������������Q����������e�����1�@��V������W��������VW�������ǃ�W�������ˑ�����������F$��W��]  ���~$ �V  ��j���������������������������v�i  �����������0�6��p  ����t���������������p�ii  ����������V�������p�Ni  ���M���B���1���u���u�������ӂ����1�F��������Q���  ������������   ��������   ���������pj$����$   �������q$Pjj�u��Z  �� �   ��������j%jj�u�*Z  �� �f������$���������p�+�� �������������������0j����������p$jjj�u�iZ  �� ���u��������������������Q���  ��������1�@�;M�u��<  ^_[]��3o  U��SWV��  �    _�]����  ��E���S�}������t&������  �)   PQQj�u�?Y  �� 1�@��  �M��x�  ������ ���1��X  �����M�����P���������������SP�\��������M  �������M�u�P�����������J����F�P�2��e  ����H�M��   1�@)�������  �� ����������������������1��X  ����PS�]S��������ǅ���   ����� ����wS�PP����ur��W������0�f  ����������w�f  ��������������@�]�o����M������n����������t��������Q���  ��1��,���u��������������������Q���  ��1�@�������7���E�0j%j)j�u�X  �� ������ �������Q���  ��1�@�;M�u��  ^_[]���l  U��SWV���}�]�u��SWV�8   ����t��W�p�A����1����Sj&jjV�W  �� 1�@��^_[]�U��SWV���   �    X������]�}����  ��E���S�l  ����r'���:u�C��:u���uS����������   ����������������W�p������ƃ��u������ր�����Å�us��uo��������  ���W���  �����W���  �����1�J��S�  R��QW���  �����S�uW���  �����u������_������Ë�������  � ��W���  ���������u���uj#j j�u�U  �� �;E�u
1����E� ��R���   ������^_[]�U��SWV���    X�}����  �1ɉL$�$���   �E��O<�U�T$�$��L  ���M�$��  �Å�t}�E�@d��t���u�[�����M�L$�$���  ��t�����   �$�D$�������   �D$�E�D$�E�$�ׅ�t��U��H��
1�G��$��$���   ���M�L$�M�$���  1�����^_[]�U��SWV���    X�u����  ����u��X  ����PV�u���������1�@����   ���1�QQ���   ���E��C��@�E�9�t6�����   ��j��p���   ����P�u��u�փ���u ��9]�uʋ���u��u���  ��1��!�U���H��
�����R���   ��1�@��^_[]�U��SWV���    X�}����  ����u��X  ���ƃ�VW�}W��������t'����  ��j��p���   ����PW�փ�1����Vj&jjW�EJ  �� 1�@��^_[]�U��SWV���    X�u�}��Q+��~�]�]��$�TX  �ǅ�t@�}����   ��(�  �	���  ��/x  �$�D$�������   �D$�<$��1���   �E��$    ��U  �ËG�}��@�E�9�t1�M���@�8�p9�t��D$�$��Y  ��9�u�M���;M�uσE��;�s�E�    9�t#�����M�����9�t����u�������E��$�V  �u�E� �8�X�}� t9�t(��$�)�����9�u��9�t��$�L�����9�u�E�$��m����$�W  �}� ��������^_[]�U��VP�u�E� �$��V  �8 t
��^]�l�����������^]�U����M�U���E�P�'   ����t1�@����u�u��u������1���]�U��SWV���    [��G�  ��u�V�u�V��RV���   ��1�A��t���   �E���th�u���    ~S1��E����U�Rj ��kw  Rj����  R�0�u����  �� ��uB�E��ß�  �M��A��E��E���E����E�01�������w  j QV���  ��1�@��^_[]�U����M�U���E�P��������t1�@����u�u��u�����1���]�U��SWV��,�    X�u�]��P�  �M؋	�M���<�  �E܋ 1ɉL$�$���   �E��[����tx�S�E�$���   ��u?�E�E�C�E�E܋ ���   �M�L$�$   ���   �D$�E��D$�4$�ׅ�t��u���H�����E܋ �4$���   1�@�M���E܋ �M��L$�4$���  �M�1��	;M�u��,^_[]��\  U��SWV���׉M��    [��F�  �E� ��1�QQ���   ���E�����  1�@��t.�E� ���   ��j��3���   ����P�u��u��փ���u�C����uċE��M�1��$�U���H��
1�G���E� ��R���   ������^_[]�U��SWV���    _�M�E�A@   ����  ���j P�u���  ���ƃ�u>��� 	  �M���	  ���MQ��|	  �������s  PQ�Ӄ���P�u�U����E�@    ����^_[]�U��WV���    X�u�}���  � �$   �P�@    �x��Nt��Ft1���^_]�U��SWV���    X�U�M�rt��Ȇ  �E�� �$��X  �E��tx1������ uf�E�� �O�$��X  �M�L$�$�EZ  ��uB�ۋ7t�3�7��E�pt�G��Q�����M��	�$���   �E�� �<$�P����u���7��u�1���^_[]�U��SWV���    X�]���  ���1�QQ���   ���ǋ[t���ۋt7���sW�u���   ����t��H��1�C��#���W���   �����W�u���  ��1ۉ؃�^_[]�U��WV�u1��~ t0�~ u*�E�M�F   ��1���WPW�   ���������~^_]�U��SWV��<�Ӊ]؉��    X��<�  �Eԋ ��j��3���   ������S���E���P��������t+�uԋ�J��1����K  ���W���   1����5  �uЉ}ċuԋ}�E�E�����DЋM��Mȋ@��]؋K�M܅ɉ���  �]܋A�C��  �Ű��1�QQ���   ���E����s�]�P�]�S���   ������  ����u��u�S���   ������  ����u��u�S���   ������  �}� t����u��u�S���   �����a  ��t���W�u�S���   �����B  ��M�Q�M�Q�u�S���   �����#  �h   �u��u�S���  �����  �} t~�}��]̅�t��H�������S���   ������}�W���  ��������W��l  ������uSW�PP�����}���   �M��ɉډ�D���E�DȉM���Ű]��H�������S�����   �ڃ��M؋]܋���H����Eȋ �}ĉӍH��Uȉ
�����R���   ����H�������W���   ���؃�<^_[]ËU��H��
�����R���   ���Uȋ�H��
���}��g������R���   ���S���U��SWV���    ^�E�}� � t� t�É؃�^_[]ËM�G   ��1ۋ�N�  �U���SP�u��������_��t`�X��F�  �M�	���u��w�u���QP����te�E� ���M��S���  ����Pj�E��W�a>  ����WS�>  �   �����n  Pj�E��V��=  ����V�u�>  �   �G�@���0�3�)  ����t���������3�K2  ���ǃ�Wj�E��S�=  ����S�u�8>  ���E� ��W�P����H��1ۃ�������E� ��V���   ������U��SWV���    _�E�xt ��  �]��Ԁ  �����rf  j�Q���   ���E�} ����   t����kk  �	����qk  j�Q�Ѓ��ǉ}�����u���   ���M����E�� ���S���  ���E�� �E�xt��?����   ���1�QQ���   ���E����wPS���   ����uʋ���u��u�S���   ����u�����u��u�S���   ����u�����u��u�S���   �����{�����M�Q�M�Q�u�S���   ����u�h   �u��u�S���  ���U��H��
�����4������Q���   ��� ����U��H��
�����R���   ���U���H��
���}����R���   ���U܋�H��
�����R���   �����WS���  ����H�������W���   ����^_[]�U��SWV���    X�}�_+_�����~  � �$�P�Ɖ\$�4$�2R  �    �~��F�F����^_[]�U��SWV���u�E�$�����E���V9�t�E�u��~��p�4��7�����9�u�E���^_[]�U��WV�}���u�N������ƋG�O�P+Pʃ��vRQ�	   ����^_]�U��SWV���    Y�}�]1�9�tK�u�G�)����E싁�}  �E����t��Q�����M��	�$���   ������� 9�uЋE�@��^_[]�U��SWV���    _�M��tj�A�X+X��t@�M�q���}���t}  ���t��Q������$���   �����uۋE�@�}��$�/%  ��t}  � �@��^_[]����^_[]�U��E��t��Q����]�\���]�U����M�E9�t"���q�p��$  ����1���t��]�   1�@��]�U��SWV���E�H�y�Q�U�1�A9�tP�M�Q�U�I�M�p����3�u��i&  �����M��4��6S謣������1���t����9}�u�1�@��ȃ�^_[]�U��SWV���E�H�M��Y1�;Yt$�x1����7���3�������M����;Yu����^_[]�U��SWV���E�1�;Xt.�}1��G�O�@����4��4�蛫����ƋE��;Xu׉���^_[]�U��SWV���M�1�@;YtT�E�u�8�F�N�M��@�E��E�H����4��E��4��E��4�袢������1���t�����E;Xu�1�@��^_[]�U��SWV���    [�}�u�>|��{  � ����i  Q�P���]��^��WS�#  ����;st_�u�u�6j �������ǅ�tW+s�E�@�0����t*��Z�����E���{  � ��Q���   ���E�@�<��1�@�%��Wj�u��6  ����uj�u�
�M����U���u�VSP�5�������M��L� ����]�SVQ�u��u��V����U�� �}� �L8���u�u؃��]���\8�M�;qu��M��\�}C+]�؃�,^_[]�U��SWV��<�    X�Eċ}1��E�W�)E�1�A�M؉ẺEȉEЉMȃ��]�S�w�6&  ���ƃ��E�PSW�������D�Mċ��v  �	��P�Q���ƍE�PSVW�C������ ����<^_[]�U��SWV��,�    ^�E�@1ɉM�W�)E�1�B�U؉M̉MȉMЉUȃ��]�SP�%  �����u  � �p���}�WS�u�$�����@��P�փ���WSV�u�.������ ����,^_[]�U��SWV���    _�]�u�U��mu  � �����Q  QSjR��(  ���*���M�Qj ���Q  Qj���y  Q�sR���  �� ��t1�@��E���SV�u����y  ����^_[]�U��SWV���   �    Y�]�}���t  ��E����t  � ����	c  QSj�u��(  ���*�S����������t  �L$�T$�M�$�PP����t1�@�H������@������������jjP�0  ��������������PSW�u�������.  �� �;M�u���   ^_[]���G  U��SWV���    X�M�]�}u3�y��t  �����t  �D$�|$�$�QP��1�A��t+����   ��t  ���ab  PQjS��(  ��1�@�   �G�X�]���1�QQ���   ���E��{;{t:����j��0���   ���Ë��S�u��u���   ����u#���E�;xuƋ���u��u���  ��1��@��H�������S���   ���U���H��
���   ���R���   ������^_[]�U��SWV���   �    _�E���r  ��M���uz�������j	jV��.  ����V�E�p�u�'  ��1�F����   ��P������������������P�E�p�uQ��%  ����t8���������������R���r  � ��7a  Q�uj�u��(  ��1�F�-���r  � ���  ���������$  ����P�u�փ�1��;E�u
����������R���   ����U��SWV���   �    [�M���j  ������ �E��q��VUUU�������Ѝ@9�t"���j  � ��zY  Q�uj�u��(  �  �������jjP�&  ���������Ѓ�P��  �M������   ������� ����E�X�q�E����������Q�s�3P舕�������:  ��P�����W��  ��;G��   ��������E��������V����������E��������  �}���Q�������v@�]���������������P�7�6S臖��������   � ������������������������� �����������������P�N���������������j  � ���  ��Q��  ����P�u�փ�1��S��������7j�������V�.%  ����V�u��%  ����W�,�������������k  ��1�@������;M�u&���   ^_[]Ë� ����f�������������������	<  U��SWV���    X�M�]�}u0�q��Qh  �����]h  �D$�t$�$�QP��1�A��t+�����Qh  ����V  PQjS��(  ��1�@��^_[]ËF�H�M��@�E����u���1�QQ���   ���E�E��H��u����}�M���E�;Hto�M�����   ��j��0���   ����P�u�VS�׃����}�u�E�� ���7VS���   ����t��U���H�����O������V���   �;�������u�S���  ��1��(���U��SWV���    X�M�]�}ug�q��g  ���g  �D$�t$�$�QP��uk�F�@�$��  �Ƌ�4$�D$�������   �E���4$�P��M��L$�$���  1��*��g  ���`U  �D$�L$�$�D$   ��(  1�@��^_[]�U��SWV���    X�u�U�}��[f  �	���T  PVjR��(  ���)�~��[f  �����gf  �D$�|$�$�QP����t
  ����VP�u���������Ã�V��%  ���E�� ���  ��S�/  ����P�u�փ�1��/�����1�GSWj*W�u�!#  �� ���u��%  ������U��SWV���    X�M�}�}u}�q���d  �����e  �D$�t$�<$�QP����up�~���w�+������ƃ�jV�A�������j WV苭��������  ��V�<������P�u�׃�1�����d  ���MS  PQjW��(  ��1�@��^_[]�U��SWV���    ^�}�U�}$��=d  � ��$S  QWjR��(  ��1�A�@  ���x��=d  �����Id  �L$�|$�$�PP��1�A���  �M�A������   �w��� ����������   �P�U�E؋B�E��u��O�}�]܉u���6��X  ���ǋ���v��X  ���E���W�]�S�  ���ƋE�9���   ���u��6��膉������PVS�  ��9�tx�]܋�E����E�u�����{���� ��FS  Pj	j-j�u��   �� ������E؋��  ���u���  ����P�u�փ�1ɉȃ�^_[]Ã�1�@WPj-P����u�jj-j�u�   �� ���u������w���U��SWV���    Y�u�U�}�i  �^���b  ������b  �|$�\$�$�M��PP��1�A���U  �}�C�E�X����M�q��X  ���ǃ�WS��
  �ك�;A���#  ��z�1  ��+Q�ϋM�I�4����u�V�u�E��QP������   �]܋V�U�u�F�H+H���B�P+P���D���P�X  ����P��������S�u��w�}�V�]���������   �M�AS�p�pQ�?���������   �M��S�E��pQV� ���������   �E܋ ���  ��S��  ����P�u�փ�1�����b  � ���Q  QVjR��(  ��1�A�ȃ�^_[]Ã�1�FWVj@V�u��  ��� �܃�Wjj@j�u�  �� ���E����<  Pjj@j�u�  �� ��S�����U��SWV���   �    [�}�u���`  ������ �E���&��r`  � ���O  QWj�u��(  ��1�A��   �W�������r`  ������ ����~`  �L$�T$�M�$�PP��1�A����   ������F������   �q������������������jAjP��  ����������|<��N���7��X  �������Q�wP������`�������tf��������ȋ��  ��������>  ����P�u�փ�1ɋ����� ;E�uX�ȁ��   ^_[]Í��O  Pj	jAj�u�  �� �&�������P�u�  ����������n����������1�A��2  U��SWV��  �    Y�u�U���^  ��E��}$���^  � ��eN  QVjR��(  ��1�G�9  �������~��������^  ����΋��^  �L$�|$�$�PP��1�G����  �������� ���������@������X�E�@�������E�ƍF���������P��  ���������P�4�������������|a1��� ���� ���M�t���X  ���ƃ�VS�  ��;C�
�<$��}��1���^_[]�U��WV�}�G+G��E��P��������V�w�wW���������^_]�U��SWV���    _�u�^�
��)ȃ�P�������E�P�w�wW��������C+C����Pj�u��������M��{9�t}�0�E�_�> t�u�SW�u��������t�M���A�M��1�H���9]��u��@����0j�u�   �����u��{��������u��7  ���E�     �E�    �E��^_[]�U��SWV���u�U�}�G+G���J+J����F+��)��]��Q�������E�G+G����Pj V�������E��_�G�E��E�    9�tF�E�0�E�    �{�> t#�u�WS�u����������   �M���A�M��1�H���9}��uƋ}�E�ƋF+F����Pj�u莗������^�N�M�9�tA�0�E�E�{�> t PWS�u�,�������tY�E���@�E��E��1�I���9}��u��u�E��p����0j�u�  �����u�� ��������u��  ���E�3����0j�u�p  �����u�����������u��  ���E�     1���^_[]�U��SWV���u�F+F����j�P�3  ���V�U�1�;Vtl�E��E�@�E�1�1�1ۋ����0�u�E�P<���������t�H9�����ǃ�QSC�u���  ���U�D2���M;Au��E�H�E����)���9�u��P��  ��1���^_[]�U��SWV���E�X�E�    ;X��   �}�G�E��E�    �3���6�u��W<����tQ���x��y;yt>���7V�rw������t,�U��t"�؋M+A���M+y����WPR�b������E�}���E;Xu��E��^_[]�U��SWV���E�0�@�E�9�t6�M�E�X�y������0W�E�P<���������t�H���9u�uփ�^_[]�U��SWV���    X�u�]�K+K�<����N��P  � �<$�P�F�|$�$�y#  �؋X1�;Xs?�v1ɉM���E�t$�$賀���ǉt$�E�$����}��L8�E����;XrƉȃ�^_[]�U��SWV���}�E�H;Hs`�E�p�}� �M���E��VWP臀������M�L� ���VQ�u��"����M���U�\�|���E��E�;A�M�r��G��O��� E�+E��^_[]�U��SWV��,�    X�}W�)E��E�    �E�   ���N  � �p�]؉\$�<$����@�$�։Ɖ\$�t$�<$����� �$襂������,^_[]�U��VP�u�F�$�{���1��F�F��^]�U��WV���    ^�}�E�@�$�����G� ��&W  �G��^_]�U��VP�u�F�$�����F�$��!  �F��^]�U��SWV���   �    X�]��N  ��M����M  � ����������QRS�u���   ��1�F����   �������j jP��	  �������uF������������Q�0�EP�  ����tJ������������RS�q�uP�|   �� ���&��Sj�������S��	  ����S�u�Q
  ���;E�u
�������8  QR�P���U�B9�s,��q<  � ��+
�������8  �]���Q�u��P���C���)���PWV�  ��)�){����^_[]�U��SWV���    ^�U�]��K��9ʋ}r ���;  � ����7  �uQ�P����K��9�r���;  � ����7  WQ�P����M�΋���������^_[]�U��WV�U�M�1�y���9�s
9u�1�9�u��RQ�k�����1�@^_]�U��E��@9�s�U9t	��9�r����]�U��SWV���    X�E�}�u�F+����)���j�Q�[������E����N)����U썒<  R�   RQP�  ����V1�9�u
1�����@�19�|�F��9щ�u�9�}
�3��F9�u��E���^_[]�U��VP�    X�M��I)������   �   PVQR�  ��^]�U��SWV���    _�]�s��j �u��������E��9�tG�0��7:  �M썏�5  �M�;��;Hr�E� ��W�u���P��E����0��   ��9�u̓�^_[]�U��M�E� +]�U��WV�E��P1�H9�s�}��9>t	��9�r��)�����^_]�U��WV���    X�u�}�9�w9ww��y9  �	���5  �$�Q�)�������^_]�U��SWV���    X�u�M�Y�N�M���59  �E� �$   �P�ǉ}�E� �   �$�P��G���G�E�8���9�t,���N���9�s9u���;M�tމD$�E�$������͋E��^_[]�U��SWV���    X�u�M�Y�N�M����8  �E� �$   �P�ǉ}�E� �   �$�P��G���G�E�8���9�t,���N���9�s9u���;M�uމD$�E�$�N����͋E��^_[]�U��VP�u�M��I)���v)�9�u��QRP�  ���������1���^]�U��SWV���E��M�@�E�1�9�t6�E�H�81ҋ]�9ω�s���9t	��9�r���9������;]�u׋E�+E���9�������^_[]�U��SWV�E��X1�9�t1�U�r�:��9�s���9t	��9�r��9�u��9�u�1��1�@^_[]�U��SWV���    ^�]�}�G;Es$��7  � �M+�������3  QR�P���G�M�U9�s'��7  � ��+�����׍��3  QR�P�M����9Js%��7  � ��+
�������3  QR�P���U���E��)�����j PQR�������ƃ�SWV�
  ������^_[]�U��SWV���    Z�}�E9xt_���E+��1���G?  ��^3  �u�Ɖu�   )����7SC�u�j�j PQ���	  ��� E�����G?  �u;~uÅ�t	I���G?  � ��G?  ��^_[]�U��SWV���    X�}���5  ��$   �P�Ƌ���<$�P�F��|$�$�	  >�~����^_[]�U��SWV���    X�M�Y+��b5  ��$   �P�Ƌ�$�P���N�N�M�	�\$�L$�$��  ����^_[]�U��VP�    Y�E��t��	5  �� �$�Q��@��^]����^]�U��SWV���    Y�}�u��V)���9�},�^)Ë��4  �	��    ��RP�Q���É^���F��^_[]�U��E�;Hs
������B0  QR�P���U�B9�s,��2  � ��+
������B0  �]���Q�u��P���C���)���PWV�j  ��)�){����^_[]�U��WV�U�M�1�y���9�s
9u�1�9�u��RQ�������1�@^_]�U��E��@9�s�U9t	��9�r����]�U��VP�    X�M��I)�����(   �   PVQR��  ��^]�U��M�E� +]�U��VP�u�M��I)���v)�9�u��QRP�  ���������1���^]�U��SWV���E��M�@�E�1�9�t6�E�H�81ҋ]�9ω�s���9t	��9�r���9������;]�u׋E�+E���9�������^_[]�U��SWV���    ^�]�}�G;Es$��D0  � �M+�������.  QR�P���G�M�U9�s'��D0  � ��+�����׍��.  QR�P�M����9Js%��D0  � ��+
�������.  QR�P���U���E��)�����j PQR�������ƃ�SWV�L  ������^_[]�U��SWV���    Z�}�E9xt_���E+��1���y<  ��K.  �u�Ɖu�   )����7SC�u�j�j PQ����  ��� E�����y<  �u;~uÅ�t	I���y<  � ��y<  ��^_[]�U���h�]�u��}��    [�}�E�    �w��t�>Ϻ��t���-  ��G    �   �E�D$�D$    �E�D$���-  �M̉L$�<$�V���
  �M�E�    �E��u�   A����0��	����E����u�UԋE�Eă}�t�O�E��EԋM����t	�M�:t���M����0��	wR�D$    �D$   �E�D$�M̉L$�<$�V1��z�D$    �D$   �E�D$�M̉L$�<$�V��tP�M䉋�7  �A��t"� ���7  �A�@���7  �A�@���7  �ǃ�7      ǃ�7      ǃ�7      �Ћ]�u��}���U��WVS��\�    [�E�    ��Y-  ��E�D$�D$    �E�D$��n,  �EԉD$�E�$�R�ƅ���   �E��t*�P��{,  ;Eu"�E��P���,  ;Eu�E䉃I6  �v���,  ��Y-  ��U�$��l  ��Y-  ��D$$    �|$ ���,  �D$�t$���,  �D$�E�D$���,  �D$�EԉD$���,  �D$�E�$��   1�����\[^_���%,� �%0� �%4� �%8� �%<� �%@� �%D� �%H� �%L� �%P� �%T� �%X�   h� �% � �h    �����h   �����h%   �����h=   �����hL   �����hZ   ����hh   ����hw   ����h�   ����h�   ����h�   ����h�   �|���::ral::tuple tuple ::ral::relation relation ::ral::relvar relvar ral ::ral 0.12.2 This software is copyrighted 2004 - 2014 by G. Andrew Mangogna. Terms and conditions for use are distributed with the source code. relationValue tupleVarName ?attr1 type1 expr1 ... attrN typeN exprN? multiplicity specification    8.6 iso8859-1 pkgname version copyright Ral_AttributeDelete: unknown attribute type: %d Ral_AttributeDup: unknown attribute type: %d Ral_AttributeRename: unknown attribute type: %d Ral_AttributeTypeEqual: unknown attribute type: %d Ral_AttributeValueEqual: unknown attribute type: %d Ral_AttributeValueCompare: unknown attribute type: %d Ral_AttributeValueObj: unknown attribute type: %d Ral_AttributeConvertValueToType: unknown attribute type: %d Ral_AttributeHashValue: unknown attribute type: %d Ral_AttributeScanType: unknown attribute type: %d Ral_AttributeScanValue: unknown attribute type: %d Ral_AttributeConvertValue: unknown attribute type: %d Ral_AttributeTypeScanFlagsFree: unknown flags type: %d Ral_AttributeValueScanFlagsFree: unknown flags type: %d bignum boolean bytearray dict double int list long string wideInt bad boolean value, "%s" booleanHash: cannot convert, "%s" doubleHash: cannot convert, "%s" intHash: cannot convert, "%s" longHash: cannot convert, "%s" wideIntHash: cannot convert, "%s" Ral_RelationUpdate: attempt to update non-existant tuple while ungrouping relation while computing quotient while unwrapping tuple Ral_RelationErase: first iterator out of bounds Ral_RelationErase: last iterator out of bounds Ral_RelationErase: first iterator greater than last  !=  array assign attributes body cardinality compose create degree divide dunion eliminate emptyof extend extract foreach fromdict fromlist group heading insert intersect is isempty isnotempty issametype join minus project rank rename restrict restrictwith semijoin semiminus summarize summarizeby table tag tclose times tuple uinsert ungroup union unwrap update wrap subcommand ?arg? ... subcommand relation arrayName keyAttr valueAttr relationValue ?attrName | attr-var-list ...? relationValue relation1 relation2 ?-using joinAttrs? heading ?tuple1 tuple2 ...? relation keyAttr valueAttr dividend divisor mediator relation1 relation2 ?relation3 ...? relationValue ?attr ... ? attribute / type / expression arguments must be given in triples relationValue attrName ?attrName2 ...? tupleVarName relationValue ?-ascending | -descending? ?attr-list?script ordering 
    ("::ral::relation foreach" body line %d) -ascending -descending dictValue keyattr keytype valueattr valuetype list attribute type relation newattribute ?attr1 attr2 ...? attempt to group all attributes in the relation during group operation relationValue ?name-value-list ...? equal == notequal != propersubsetof < propersupersetof > subsetof <= supersetof >= relation1 compareop relation2 compareop relation1 relation2 relation1 relation2 ?-using joinAttrs relation3 ... ? relationValue ?attrName ? ?-ascending | -descending? sortAttrList? ? relationValue ?-ascending | -descending? rankAttr newAttr relationValue ?oldname newname ... ? oldname / newname arguments must be given in pairs relValue tupleVarName expr relValue expr relationValue perRelation relationVarName ?attr1 type1 expr1 ... attrN typeN exprN? heading ?value-list1 value-list2 ...? relation attrName ?-ascending | -descending sort-attr-list? ?-within attr-list -start tag-value? -within -start tag option Ral_TagCmd: unknown option, "%d" relation relation attribute relationValue attribute relationValue tupleVarName expr script 
    ("in ::ral::relation update" body line %d) invoked "break" outside of a loop invoked "continue" outside of a loop unknown return code %d relationValue newAttr ?attr attr2 ...? -using Ral_ConstraintDelete: unknown constraint type, %d unknown constraint type, %d relvarConstraintCleanup: unknown constraint type, %d is referenced by multiple tuples is not referenced by any tuple references multiple tuples references no tuple for association  (  [ ] ==> [ ]  ) 1 + ? * , in relvar  
 tuple    is referred to by multiple tuples is not referred to by any tuple for partition   is partitioned [  |  ]) correlation   does not form a complete correlation for correlation   <== [  (Complete) ] ==>  association constraint correlation delete deleteone eval exists identifiers names partition path procedural restrictone set trace transaction unset updateone updateper name refrngRelvar refrngAttrList refToSpec refToRelvar refToAttrList refrngSpec info member delete | info | names ?args? | member <relvar> | path <name> constraint subcommand name ?pattern? relvarName Unknown association command type, %d ?-complete? name corrRelvar corr-attr-list1 relvar1 attr-list1 spec1 corr-attr-list2 relvar2 attr-list2 spec2 relvarName heading id1 ?id2 id3 ...? relvarName tupleVarName expr relvarName relvarName ?attrName1 value1 attrName2 value2 ...? relvarName ?relationValue ...? arg ?arg ...? 
    ("in ::ral::relvar eval" body line %d) relvarName ?name-value-list ...? relvarName relationValue name super super-attrs sub1 sub1-attrs ?sub2 sub2-attrs sub3 sub3-attrs ...? name relvarName1 ?relvarName2 ...? script relvarValue attr value ?attr2 value 2 ...? attribute / value arguments must be given in pairs relvar ?relationValue? add remove suspend variable option type ?arg arg ...? trace option trace type add variable relvarName ops cmdPrefix remove variable relvarName ops cmdPrefix info variable relvarName suspend variable relvarName script Unknown trace option, %d add transaction cmdPrefix remove transaction cmdPrefix info transaction suspending eval traces not implemented Unknown trace type, %d begin end rollback transaction option Unknown transaction option, %d ?relvar1 relvar2 ...? relvarName tupleVarName expr script relvarName tupleVarName idValueList script relvar creation not allowed during a transaction :: during identifier construction operation 
    ("in ::ral::%s %s" body line %d) -complete Ral_RelvarObjConstraintInfo: unknown constraint type, %d end transaction with no beginning 
    ("in ::ral::relvar trace suspend variable" body line %d)  relvar may only be modified using "::ral::relvar" command relvarTraceProc: trace on non-write, flags = %#x
 bad operation list: must be one or more of delete, insert, update, set or unset traceOp namespace eval   { } procedural contraint, " ", failed returned "continue" from procedural contraint script for constraint, " " returned "break" from procedural contraint script for constraint, " Ral_TupleUpdate: attempt to update a shared tuple get tupleValue ?attrName | attr-var-pair ... ? tupleValue heading name-value-list tupleValue ?attr? ... tuple1 tuple2 tupleValue ?name type value ... ? tupleValue attr ?...? attr1 type1 value1 ... tupleValue ?oldname newname ... ? for oldname / newname arguments tupleValue tupleAttribute tupleValue ?attr1 value1 attr2 value2? for attribute name / attribute value arguments tupleValue newAttr ?attr attr2 ...? failed to copy attribute, "%s" Ral_TupleHeadingAppend: out of bounds access at source offset "%d" Ral_TupleHeadingAppend: overflow of destination Ral_TupleHeadingFetch: out of bounds access at offset "%d" Ral_TupleHeadingStore: out of bounds access at offset "%d" Ral_TupleHeadingStore: cannot find attribute name, "%s", in hash table Ral_TupleHeadingStore: inconsistent index, expected %u, got %u Ral_TupleHeadingPushBack: overflow , " RAL no error unknown attribute name duplicate attribute name bad heading format bad value format bad value type for value unknown data type bad type keyword wrong number of attributes specified bad list of pairs duplicate command option relations of non-zero degree must have at least one identifier identifiers must have at least one attribute identifiers must not be subsets of other identifiers tuple has duplicate values for an identifier duplicate attribute name in identifier attribute set duplicate tuple headings not equal relation must have degree of one relation must have degree of two relation must have cardinality of one bad list of triples attributes do not constitute an identifier attribute must be of a Relation type attribute must be of a Tuple type relation is not a projection of the summarized relation divisor heading must be disjoint from the dividend heading mediator heading must be a union of the dividend and divisor headings too many attributes specified attributes must have the same type only a single identifier may be specified identifier must have only a single attribute "-within" option attributes are not the subset of any identifier attribute is not a valid type for rank operation duplicate relvar name unknown relvar name mismatch between referential attributes duplicate constraint name unknown constraint name relvar has constraints in place referred to identifiers can not have non-singular multiplicities operation is not allowed during "eval" command a super set relvar may not be named as one of its own sub sets correlation spec is not available for a "-complete" correlation recursively invoking a relvar command outside of a transaction recursive attempt to modify a relvar already being changed serious internal error unknown command relvar setfromany updatefromobj NONE destroy OK UNKNOWN_ATTR DUPLICATE_ATTR HEADING_ERR FORMAT_ERR BAD_VALUE BAD_TYPE BAD_KEYWORD WRONG_NUM_ATTRS BAD_PAIRS_LIST DUPLICATE_OPTION NO_IDENTIFIER IDENTIFIER_FORMAT IDENTIFIER_SUBSET IDENTITY_CONSTRAINT DUP_ATTR_IN_ID DUPLICATE_TUPLE HEADING_NOT_EQUAL DEGREE_ONE DEGREE_TWO CARDINALITY_ONE BAD_TRIPLE_LIST NOT_AN_IDENTIFIER NOT_A_RELATION NOT_A_TUPLE NOT_A_PROJECTION NOT_DISJOINT NOT_UNION TOO_MANY_ATTRS TYPE_MISMATCH SINGLE_IDENTIFIER SINGLE_ATTRIBUTE WITHIN_NOT_SUBSET BAD_RANK_TYPE DUP_NAME UNKNOWN_NAME REFATTR_MISMATCH DUP_CONSTRAINT UNKNOWN_CONSTRAINT CONSTRAINTS_PRESENT BAD_MULT BAD_TRANS_OP SUPER_NAME INCOMPLETE_SPEC ONGOING_CMD ONGOING_MODIFICATION INTERNAL_ERROR Ral_IntVectorFetch: out of bounds access at offset "%d" Ral_IntVectorStore: out of bounds access at offset "%d" Ral_IntVectorFront: access to empty vector Ral_IntVectorPopBack: access to empty vector Ral_IntVectorInsert: out of bounds access at offset "%d" Ral_IntVectorErase: out of bounds access at offset "%d" Ral_IntVectorOffsetOf: out of bounds access Ral_IntVectorCopy: out of bounds access at source offset "%d" Ral_IntVectorCopy: out of bounds access at dest offset "%d" %d: %d
 Ral_PtrVectorFetch: out of bounds access at offset "%d" Ral_PtrVectorStore: out of bounds access at offset "%d" Ral_PtrVectorFront: access to empty vector Ral_PtrVectorPopBack: access to empty vector Ral_PtrVectorInsert: out of bounds access at offset "%d" Ral_PtrVectorErase: out of bounds access at offset "%d" Ral_PtrVectorCopy: out of bounds access at source offset "%d" Ral_PtrVectorCopy: out of bounds access at dest offset "%d" %d: %X
   interpreter uses an incompatible stubs mechanism Tcl tcl::tommath epoch number mismatch requires a later revision missing stub table pointer ):  , actual version   (requested version  Error loading                             4   4   ��     4                          �`  ��  �� �z @� ��  �     �� � � � "� ,� 6� @� J� T� ^� h� r� �� �%  #&  �&  Z'  �� �'  c(  �(   )  �� �)  �)  '*  �*  �� �*  �*  B+  �+  �� �+  �+  \,  �,  �� E-  q-  �-  ..  �� �.  �.  /  `/  �� �/  �/  -0  �0  �� �  �  �  �   �� �0  $1  �1  
2  �5  �5  �� a  �� ~b  �� �c  �� �d  �� �e  �� Ff  �� Ki   � �j  �� )k  � �l  � �m  � p  � (q  '� �q  .� �v  6� �x  >� �|  G� -  P� �  V� �  ^� Ƀ  e� �  o� :�  r� r�  z� �  �� ��  �� ��  �� ��  �� ��  �� /�  �� �  �� ��  �� ז  �� �  �� *�  �� �  �� ��  �� �  �� Ů  �� x�  �� Ĵ  �� ��  � =�  � 	�  � R�  � ��  !� ��  (� �  /� �          Q�     \�            =� �X  C� �X  F� ;Y  O� ;Y  R� �Y  a� �Y  c� %Z  t� %Z  v� bY  � bY  �� �Y  �� �Y          Q�     \�    ��    ��            D� F� H� J� ,� c�  8� ��  C� $�  �� z�  O� L�  V� K�  � �  `� 6�  e� s�  l� �  ^� ��  e� @ �� \ x� � ~� Q
 �� �
 �� K �� � �� �
`  Z`  �� ��  ��  "�  I�  O� e� $� x� )� ��     /� 3� $� :�     B� ��     �� �� ��     D�         F�        H�        J�                   �� �� �� ݡ �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 !WA`�Bpp`B�p`:BUA�p`"B`�HTBTB`ATARASA�pU ���ppQ  @___stack_chk_guard Qq$�@dyld_stub_binder ����������� q,@___bzero � q0@___snprintf_chk � q4@___stack_chk_fail � q8@_bsearch � q<@_memcmp � q@@_memcpy � qD@_memmove � qH@_qsort � qL@_strcmp � qP@_strcpy � qT@_strlen � qX@_strrchr �     _  Ral_ r �tuple � 
In xSafe �Unload �Attribute �JoinMap �Rel �Constraint �Tuple �ErrorInfo � PtrVector �% it �t �  �"  Init �Unload � �(  �(  �(  	New �D �Rename �Equal �T �Value �Convert �HashValue �Scan � T �RelationType �FromObjs � clType �upleType � �(  �)  �*  elete �up � �+  �+  �,  �-  ype �oString � Equal �ScanFlagsFree � �.  Equal �Compare �Obj �ScanFlagsFree � �.  �3  �5  �7  Value �Name �Type � ToType � � �:  �?  Name �Type �Value � �B  �B  �B  �D  �E  �G  �H  �I  �J  New �Delete �A �Tuple �GetAttr �FindAttr �SortAttr �MatchingTupleSet � �d  �e  ttr �dd � Reserve �Map � �f  Reserve �Map �Counts � �g  AttrMapping �TupleMapping � �g  �h  �i  �i  �j  �k  �k  �l  �l  ation �var �
Co �
E �
ValueStringOf �Obj � ew �otEqual � �o  up �elete �i � �p  serve �nameAttribute � �q  ushBack �ro �	 �r  hallowCopy �emi �
ort �
u �can �tringOf � �s  �t  pdate �n � �tTupleObj �
 �vCopy � �z  sjointCopy �	vide �
 �z  tersect �	sertTuple �
 �}  imes �	ag �
close �
 �~  ject �	perSu � ��  ��  ��  ��  ��  mp �
nvert � ose �
are � ��  Join �
Minus �
 ��  ��  Ǘ  �Within �
 ��  ��  �  ��  xtract �rase �qual � ϭ  ��  �  ��  ��  bsetOf �persetOf � �  bsetOf �persetOf � ��  �  ��  �  ׵Value � ��  ��Value � ��  ��  Ǽ  el �al_ �' ationCmd �varCmd � ��  New �
New �%D �%Reserve �%F �%S �&Back �&P �&Insert �'E �'Copy �' ��  up �%elete �% ��  ��  ��  i �&etch �&ront �& ll �&nd �' ��  ��  tore �&etAdd �'ort �'ubsetOf �' ��  ��  ��  ushBack �&opBack �&rint �' ��  ��  ��  rase �'qual �' ��  ��  ��  ��  ��  ��  ��  ��  relationTypeName �'tupleTypeName �' ��  ��  ��  ��  ��    �"�	�bbVpp=m�O�G����@90����||�O��e�a\o,l`@,OG@,gwr,a\o-OG@,a\o,i}rhMWYsROQC1R;q#@-��Y�Xd���LV�o��|�z����x�������ub��A'AAAAq;�s���2+�P>���������������
�����������������������
2  �    �5  �    �5  �    �6  �    �6       7      V7  1    \<  C    �C  ]    ZV  k    �^  |    �^  �    _  �    5_  �    `_  �    
`  �    Z`      a      ~b  -    �c  D    �d  U    �e  m    Ff  �    Ki  �    �j  �    )k  �    �l  �    �m  �    p  �    (q      �q      �v  /    �x  C    �|  X    -  m    �      �  �    Ƀ  �    �  �    :�  �    r�  �    �  �    ��  
 _#    �
 n#    K �#    � �#    �
  �� �(  
  � �(  
  (� �(  
  T� �(  
  p� )  
  �� *)  
  �� S)  
  �� ^)  
  �� k)  
  �� |)  
  �� �)  
  �� �)  
   � �)    � �)    � �)    �      F!        "  6     �#  Q     5  r     �  �     �  �     �  �     �  �     �  �     ,  �     9      �  1    T  F    
    �K  #
    
8  4
    �X  G
    1W  Z
    �V  o
    \>  �
    H  �
    A  �
    ��  �
    7�  �
    �=      �F      �>  &    �7  7    ;Y  M    o�  e    ;�  y    �  �  
  @� �    �@  �    �Y  �    %Z  �    9      fZ  .    �8  C    �Z  U    [  l    �J  �    2K  �    �9  �    �U  �    �]  �    bY  �    �Y  
  �� �    � �    Rw �    �w     ��     �� 5    |y H    �q Y    s o    �t �    >� �    z �    +  �  
  �� �  
  �� �    �`  �    ��    
  ��     �z #        ,        <        N        a        j        r        z        �        �        �        �        �        �        �  �  �  �  �  �  �  �  �              @   �   �   �   �   �   �   ��     ��  �  �  �  �  �  �  �  �           _Ral_AttributeConvertName _Ral_AttributeConvertType _Ral_AttributeConvertValue _Ral_AttributeConvertValueToType _Ral_AttributeDelete _Ral_AttributeDup _Ral_AttributeEqual _Ral_AttributeHashValue _Ral_AttributeNewFromObjs _Ral_AttributeNewRelationType _Ral_AttributeNewTclType _Ral_AttributeNewTupleType _Ral_AttributeRename _Ral_AttributeScanName _Ral_AttributeScanType _Ral_AttributeScanValue _Ral_AttributeToString _Ral_AttributeTypeEqual _Ral_AttributeTypeScanFlagsFree _Ral_AttributeValueCompare _Ral_AttributeValueEqual _Ral_AttributeValueObj _Ral_AttributeValueScanFlagsFree _Ral_ConstraintAssocCreate _Ral_ConstraintCorrelationCreate _Ral_ConstraintDelete _Ral_ConstraintDeleteByName _Ral_ConstraintFindByName _Ral_ConstraintNewAssociation _Ral_ConstraintNewCorrelation _Ral_ConstraintNewPartition _Ral_ConstraintNewProcedural _Ral_ConstraintPartitionCreate _Ral_ConstraintProceduralCreate _Ral_ErrorInfoGetCommand _Ral_ErrorInfoGetOption _Ral_ErrorInfoSetCmd _Ral_ErrorInfoSetError _Ral_ErrorInfoSetErrorObj _Ral_Init _Ral_IntVectorBack _Ral_IntVectorBooleanMap _Ral_IntVectorContainsAny _Ral_IntVectorCopy _Ral_IntVectorDelete _Ral_IntVectorDup _Ral_IntVectorEqual _Ral_IntVectorErase _Ral_IntVectorExchange _Ral_IntVectorFetch _Ral_IntVectorFill _Ral_IntVectorFillConsecutive _Ral_IntVectorFind _Ral_IntVectorFront _Ral_IntVectorIndexOf _Ral_IntVectorInsert _Ral_IntVectorIntersect _Ral_IntVectorMinus _Ral_IntVectorNew _Ral_IntVectorNewEmpty _Ral_IntVectorOffsetOf _Ral_IntVectorPopBack _Ral_IntVectorPrint _Ral_IntVectorPushBack _Ral_IntVectorReserve _Ral_IntVectorSetAdd _Ral_IntVectorSetComplement _Ral_IntVectorSort _Ral_IntVectorStore _Ral_IntVectorSubsetOf _Ral_InterpErrorInfo _Ral_InterpErrorInfoObj _Ral_InterpSetError _Ral_JoinMapAddAttrMapping _Ral_JoinMapAddTupleMapping _Ral_JoinMapAttrMap _Ral_JoinMapAttrReserve _Ral_JoinMapDelete _Ral_JoinMapFindAttr _Ral_JoinMapGetAttr _Ral_JoinMapMatchingTupleSet _Ral_JoinMapNew _Ral_JoinMapSortAttr _Ral_JoinMapTupleCounts _Ral_JoinMapTupleMap _Ral_JoinMapTupleReserve _Ral_PtrVectorBack _Ral_PtrVectorCopy _Ral_PtrVectorDelete _Ral_PtrVectorDup _Ral_PtrVectorEqual _Ral_PtrVectorErase _Ral_PtrVectorFetch _Ral_PtrVectorFill _Ral_PtrVectorFind _Ral_PtrVectorFront _Ral_PtrVectorInsert _Ral_PtrVectorNew _Ral_PtrVectorPopBack _Ral_PtrVectorPrint _Ral_PtrVectorPushBack _Ral_PtrVectorReserve _Ral_PtrVectorSetAdd _Ral_PtrVectorSort _Ral_PtrVectorStore _Ral_PtrVectorSubsetOf _Ral_RelationCompare _Ral_RelationCompose _Ral_RelationConvert _Ral_RelationConvertValue _Ral_RelationDelete _Ral_RelationDisjointCopy _Ral_RelationDivide _Ral_RelationDup _Ral_RelationEqual _Ral_RelationErase _Ral_RelationExtract _Ral_RelationFind _Ral_RelationFindJoinTuples _Ral_RelationGroup _Ral_RelationInsertTupleObj _Ral_RelationInsertTupleValue _Ral_RelationIntersect _Ral_RelationJoin _Ral_RelationMinus _Ral_RelationNew _Ral_RelationNotEqual _Ral_RelationObjConvert _Ral_RelationObjNew _Ral_RelationObjParseJoinArgs _Ral_RelationObjType _Ral_RelationProject _Ral_RelationProperSubsetOf _Ral_RelationProperSupersetOf _Ral_RelationPushBack _Ral_RelationRenameAttribute _Ral_RelationReserve _Ral_RelationScan _Ral_RelationScanValue _Ral_RelationSemiJoin _Ral_RelationSemiMinus _Ral_RelationShallowCopy _Ral_RelationSort _Ral_RelationStringOf _Ral_RelationSubsetOf _Ral_RelationSupersetOf _Ral_RelationTag _Ral_RelationTagWithin _Ral_RelationTclose _Ral_RelationTimes _Ral_RelationUngroup _Ral_RelationUnion _Ral_RelationUnionCopy _Ral_RelationUnwrap _Ral_RelationUpdate _Ral_RelationUpdateTupleObj _Ral_RelationValueStringOf _Ral_RelvarDeclConstraintEval _Ral_RelvarDelete _Ral_RelvarDeleteInfo _Ral_RelvarDeleteTransaction _Ral_RelvarDeleteTuple _Ral_RelvarDiscardPrev _Ral_RelvarFind _Ral_RelvarFindById _Ral_RelvarFindIdentifier _Ral_RelvarIdIndexTuple _Ral_RelvarIdUnindexTuple _Ral_RelvarInsertTuple _Ral_RelvarIsTransOnGoing _Ral_RelvarNew _Ral_RelvarNewInfo _Ral_RelvarNewTransaction _Ral_RelvarObjConstraintDelete _Ral_RelvarObjConstraintInfo _Ral_RelvarObjConstraintMember _Ral_RelvarObjConstraintNames _Ral_RelvarObjConstraintPath _Ral_RelvarObjCopyOnShared _Ral_RelvarObjCreateAssoc _Ral_RelvarObjCreateCorrelation _Ral_RelvarObjCreatePartition _Ral_RelvarObjCreateProcedural _Ral_RelvarObjDelete _Ral_RelvarObjEndCmd _Ral_RelvarObjEndTrans _Ral_RelvarObjExecDeleteTraces _Ral_RelvarObjExecEvalTraces _Ral_RelvarObjExecInsertTraces _Ral_RelvarObjExecSetTraces _Ral_RelvarObjExecUnsetTraces _Ral_RelvarObjExecUpdateTraces _Ral_RelvarObjFindConstraint _Ral_RelvarObjFindRelvar _Ral_RelvarObjInsertTuple _Ral_RelvarObjKeyTuple _Ral_RelvarObjNew _Ral_RelvarObjTraceEvalAdd _Ral_RelvarObjTraceEvalInfo _Ral_RelvarObjTraceEvalRemove _Ral_RelvarObjTraceUpdate _Ral_RelvarObjTraceVarAdd _Ral_RelvarObjTraceVarInfo _Ral_RelvarObjTraceVarRemove _Ral_RelvarObjTraceVarSuspend _Ral_RelvarObjUpdateTuple _Ral_RelvarRestorePrev _Ral_RelvarSetRelation _Ral_RelvarStartCommand _Ral_RelvarStartTransaction _Ral_RelvarTraceAdd _Ral_RelvarTraceRemove _Ral_RelvarTransModifiedRelvar _Ral_SafeInit _Ral_SafeUnload _Ral_TupleAssignToVars _Ral_TupleAttrEqual _Ral_TupleConvert _Ral_TupleConvertValue _Ral_TupleCopy _Ral_TupleCopyValues _Ral_TupleDelete _Ral_TupleDup _Ral_TupleDupOrdered _Ral_TupleDupShallow _Ral_TupleEqual _Ral_TupleEqualValues _Ral_TupleExtend _Ral_TupleGetAttrValue _Ral_TupleHash _Ral_TupleHashAttr _Ral_TupleHeadingAppend _Ral_TupleHeadingAttrsFromObj _Ral_TupleHeadingAttrsFromVect _Ral_TupleHeadingCommonAttributes _Ral_TupleHeadingCompose _Ral_TupleHeadingConvert _Ral_TupleHeadingDelete _Ral_TupleHeadingDup _Ral_TupleHeadingEqual _Ral_TupleHeadingExtend _Ral_TupleHeadingFetch _Ral_TupleHeadingFind _Ral_TupleHeadingIndexOf _Ral_TupleHeadingIntersect _Ral_TupleHeadingJoin _Ral_TupleHeadingMapIndices _Ral_TupleHeadingNew _Ral_TupleHeadingNewFromObj _Ral_TupleHeadingNewOrderMap _Ral_TupleHeadingPushBack _Ral_TupleHeadingScan _Ral_TupleHeadingStore _Ral_TupleHeadingStringOf _Ral_TupleHeadingSubset _Ral_TupleHeadingUnion _Ral_TupleHeadingUnreference _Ral_TupleNew _Ral_TupleObjConvert _Ral_TupleObjNew _Ral_TupleObjType _Ral_TuplePartialSetFromObj _Ral_TupleScan _Ral_TupleScanValue _Ral_TupleSetFromObj _Ral_TupleSetFromValueList _Ral_TupleStringOf _Ral_TupleSubset _Ral_TupleUnreference _Ral_TupleUpdateAttrValue _Ral_TupleUpdateFromObj _Ral_TupleValueStringOf _Ral_Unload _ral_relationTypeName _ral_tupleTypeName _relationCmd _relvarCmd _tupleAttrHashType _tupleCmd ___bzero ___snprintf_chk ___stack_chk_fail ___stack_chk_guard _bsearch _memcmp _memcpy _memmove _qsort _strcmp _strcpy _strlen _strrchr dyld_stub_binder _stringEqual _stringCompare _isAString _stringHash _cmpTypeNames _isABignum _bignumEqual _bignumCompare _bignumHash _isABoolean _booleanEqual _booleanCompare _booleanHash _isAByteArray _byteArrayEqual _byteArrayCompare _byteArrayHash _isADict _dictEqual _dictCompare _dictHash _isADouble _doubleEqual _doubleCompare _doubleHash _isAnInt _intEqual _intCompare _intHash _isAList _listEqual _listCompare _listHash _isALong _longEqual _longCompare _longHash _isAWideInt _wideIntEqual _wideIntCompare _wideIntHash _attr_0_cmp _attr_1_cmp _tupleAttrHashGenKey _tupleAttrHashCompareKeys _tupleAttrHashEntryAlloc _tupleAttrHashEntryFree _Ral_HeadingMatch _Ral_RelationIndexByAttrs _Ral_DownHeap _tupleHashGenKey _tupleHashCompareKeys _tupleHashEntryAlloc _tupleHashEntryFree _Ral_TupleCompare _tupleAttrHashMultiEntryAlloc _tupleAttrHashMultiEntryFree _RelationArrayCmd _RelationAssignCmd _RelationAttributesCmd _RelationBodyCmd _RelationCardinalityCmd _RelationComposeCmd _RelationCreateCmd _RelationDegreeCmd _RelationDictCmd _RelationDivideCmd _RelationDunionCmd _RelationEliminateCmd _RelationEmptyofCmd _RelationExtendCmd _RelationExtractCmd _RelationForeachCmd _RelationFromdictCmd _RelationFromlistCmd _RelationGroupCmd _RelationHeadingCmd _RelationInsertCmd _RelationIntersectCmd _RelationIsCmd _RelationIsemptyCmd _RelationIsnotemptyCmd _RelationIssametypeCmd _RelationJoinCmd _RelationListCmd _RelationMinusCmd _RelationProjectCmd _RelationRankCmd _RelationRenameCmd _RelationRestrictCmd _RelationRestrictWithCmd _RelationSemijoinCmd _RelationSemiminusCmd _RelationSummarizeCmd _RelationSummarizebyCmd _RelationTableCmd _RelationTagCmd _RelationTcloseCmd _RelationTimesCmd _RelationTupleCmd _RelationUinsertCmd _RelationUngroupCmd _RelationUnionCmd _RelationUnwrapCmd _RelationUpdateCmd _RelationWrapCmd _FreeRelationInternalRep _DupRelationInternalRep _UpdateStringOfRelation _SetRelationFromAny _relvarCleanup _relvarConstraintCleanup _relvarSetIntRep _relvarIndexIds _relvarFindJoinTuples _relvarEvalAssocTupleCounts _relvarAssocConstraintErrorMsg _relvarConstraintErrorMsg _relvarPartitionConstraintErrorMsg _relvarCorrelationConstraintErrorMsg _relvarCorrelationConstraintToString _RelvarAssociationCmd _RelvarConstraintCmd _RelvarCorrelationCmd _RelvarCreateCmd _RelvarDeleteCmd _RelvarDeleteOneCmd _RelvarDunionCmd _RelvarEvalCmd _RelvarExistsCmd _RelvarIdentifiersCmd _RelvarInsertCmd _RelvarIntersectCmd _RelvarMinusCmd _RelvarNamesCmd _RelvarPartitionCmd _RelvarPathCmd _RelvarProceduralCmd _RelvarRestrictOneCmd _RelvarSetCmd _RelvarTraceCmd _RelvarTransactionCmd _RelvarUinsertCmd _RelvarUnionCmd _RelvarUnsetCmd _RelvarUpdateCmd _RelvarUpdateOneCmd _RelvarUpdatePerCmd _relvarResolveName _relvarTraceProc _relvarGetNamespaceName _relvarObjConstraintEval _relvarConstraintAttrNames _Ral_RelvarObjDecodeTraceOps _Ral_RelvarObjEncodeTraceFlag _Ral_RelvarObjExecTraces _TupleAssignCmd _TupleAttributesCmd _TupleCreateCmd _TupleDegreeCmd _TupleEliminateCmd _TupleEqualCmd _TupleExtendCmd _TupleExtractCmd _TupleFromListCmd _TupleGetCmd _TupleHeadingCmd _TupleProjectCmd _TupleRelationCmd _TupleRenameCmd _TupleUnwrapCmd _TupleUpdateCmd _TupleWrapCmd _FreeTupleInternalRep _DupTupleInternalRep _UpdateStringOfTuple _SetTupleFromAny _int_ind_compare _ptr_ind_compare _Tcl_InitStubs _TclTomMathInitializeStubs _Ral_Init.tupleCmdName _Ral_Init.tupleStr _Ral_Init.relationCmdName _Ral_Init.relationStr _Ral_Init.relvarCmdName _Ral_Init.relvarStr _ral_pkgname _ral_version _ral_copyright _RelationExtendCmd.usage _specErrMsg _Ral_Types _Ral_JoinMapSortAttr.cmpFuncs _relationCmd.cmdTable _orderOptions _RelationIsCmd.cmdTable _RelationTagCmd.optTable _condMultStrings _relvarCmd.cmdTable _relvarAssocSpec.condMultStrings _opsTable _tupleCmd.cmdTable _resultStrings _cmdStrings _optStrings _errorStrings _ral_config _tupleHashType _tupleAttrHashMultiType _RelvarConstraintCmd.constraintCmds _RelvarTraceCmd.traceOptions _RelvarTraceCmd.traceTypes _RelvarTransactionCmd.transactionOptions _specTable _tclStubsPtr _tclPlatStubsPtr _tclIntStubsPtr _tclIntPlatStubsPtr _tclTomMathStubsPtr _RelationTagCmd.gotOpt _Ral_IntVectorPrint.buf _Ral_PtrVectorPrint.buf                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          ����        
  
 *                 8           �   /usr/lib/libSystem.B.dylib      &      P5 �  )      8                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     UH��AWAVAUATSPI��H�5h� 1�褲 H���h  H�5Q� 1�1�L���� H���L  L�=-� I�H�5� 1�1�L�����  I��I����  I��I�H�5�� H�
� H� H�=ȴ ��������  L��L��H��H��A��L�=�� ����   H�=ƴ L���M�  M�H�5�� H��  E1�L��H��A��  I�H��� 1�L��L�����  ����   I�L���  H�=e� ��������  H��H�a� H� H�=8� ��������  L��L��H��H��A��L�=6� ��u0I�H�5"� 1�1�L��L�����  I�L��H��L����  ��t0A��H�A���I�L����   �   ��H��[A\A]A^A_]�I�L�5�� H��� H�
   �(   �2� I��1�M��tCH�7� H� H�X(L���<� �x!��H��H�{ L���"� H�I�H�C�C    H�C    H��H��([A^A_]�UH��AWAVSPI��I��H��� H� H�X(�ޱ �x!��H��H�{ L���ı H�H��� H�C�C   L�sA�H��H��[A^A_]�UH��AWAVSPI��I��H�u� H� H�X(�}� �x!��H��H�{ L���c� H�H�s� H�C�C   L�sA�H��H��[A^A_]�UH��SPH���s�F���sH�{�u� ���tH�� H�H�='� 1��Q H��� H� H��H��[]�`0�w��t��t"��u-H�H�wH���?���H�H�wH���!���H�H�wH������UH��H��� H�H�=� 1��Q 1�]�H���w��t��t��u$H�wH�������H�wH�������H�wH���[���UH��H�=� H�H�=�� 1��Q 1�]�UH��AVSI��H���   L9�t+H�;I�6�� ��1���u�KA;NuH��L��[A^]�   [A^]�H���w�N���s
   �(   謮 H����   L��L���PA���   H��� H�H�G� E1�1�L�����   ����   H�H�'� E1�1�L�����   ��u|I� I�v ��A  A���jH�L� H�H�� E1�1�L�����   ��uGH�H��� E1�1�L�����   ��u+I� I�v �[ A���H��� H�H�=� E1�1��Q D��H��8[A\A]A^A_]�I�E L�����
  H��I�E L�����
  H��H��跭 ����D���UH��AWAVSPI��L�=�� I����
  H��I�L�����
  H��H���u� ������H��[A^A_]�UH��AWAVSH��(I��H��w�F���s7L�=5� I�H�����
  H��I�L�����
  H��H��H��([A^A_]�
   �(   �¬ H��t'H��L���P���IH��� H�H�=ٯ 1�1��Q �/L�=�� I�H�����
  H��I�L�����
  H��H��臬 �É�H��([A^A_]�UH��AWAVSPI��L�=\� I����
  H��I�L�����
  H��H��H��[A^A_]�7� UH��AWAVSPI�֋v��t5��t5��u~L�=
� I�H��� L�����   1ۅ�uvI�~ �k` �.L���fL�=�� I�H��� L�����   1ۅ�uFI�~ ��D  H��I������H�����  I��I�H���P0L���H��� H�H�=ܮ 1�1��Q H��H��[A^A_]�UH��AWAVAUATSH��I��H��I��L�-L� I�E H�����
  I��I�E H�U�H�M�L��H����x  ��1�����   �Eԃ�t0��urI�E H�M�H�9���
  H��L��H���o���H��ux�   �?I�E H�M�H�9���
  H��H�=�� H���Ū ��tYH�=�� H��貪 ��tp�   L��H���I�E H�����
  �   L��H���V� L��L���� 1�H��[A\A]A^A_]�H�E�H�pL��L��臊 H��1�H��t�L��H���K�����H�E�H�pL��L���]� H��1�H��t�L��H�������UH��AWAVAUATSH��8I��I��H��I���s����   ����   H��� ���]  H� H�u�L����X  �}� �R  H�CH�E�H�5�� L��  H�}��
   �(   �r� H��H���  L��L���Q���  �   ��  I�D$H;�� ��   A�<$��   H�=� H� L�����   I��H�{L��L��L��M���Ԫ  ��uwH�� H� L����`  1�M����  H����   I�D$H;R� ��   A�<$��   H��� H� L�����   I��H�{L��L��L��M���҆ ����   H��� A�M �A�A�E 1����  H�L����   1���   H�H�=� 1��Q L����   H���DH��M���   H�{L��L��L��M����  ��1�����   H��H�� H� L����`  M��I�E H�;H�0�bH�{L��L��L��M���� ��1���unH��H��� H� L����`  M���H��� H� L����`  1�M��t8H��I�E H�;H�p�z ��L��u�   L��L��赏 L��L���� 1�H��8[A\A]A^A_]�UH��1�]�UH��AWAVATSH��0I��I��A�w1ۍF���rr��uVL�%.� I�$H�u�L����X  �}� tPI�GH�E�H�5 � L�^  H�}��
   �(   �ئ H��t1L���P ���H��� H�H�=�� 1�1��Q ��H��0[A\A^A_]�I�$H�u�L����X  �M�1ۅ�t��H�����u���UH��H��H��� H� H�u���X  H���U�1���t�1H�����u�H��]�UH��SPH��G�H�A� H� H�?H�s���  �CH��[]�UH��H�� H� H���  H�?�R]��UH��SP�O���t*��t5H��� ��u:H� H���  H�H��H��[]��H��{� �Ã�
�H�H�=�� 1�1����R ��H��[]�UH��AWAVATSI��I��I��A�w��t3��twH�j� ����   H� H���  I�A�VL��[A\A^A_]��I�|$A�${H�5Z� �;� fA�D$	 {I�t$I�L���� H�I�\�}A�L��A�LD)��ZI�|$A�${H�5� �� fA�D$ {I�t$I�L���6� H�I�\	�}A�L��A�L	D)��H�H�=�� 1�1��Q ��[A\A^A_]�UH��AVSH��H���w�3��t6��tD��uSH�
  H��H��H��L��[A^]��H�x H��H��[A^]��9  H�x H��H��[A^]�V H�� H�H�=>� 1��Q 1�[A^]�UH��AWAVSPM��H��H���w��t<��tS��ukH�
  A�VH��H��L��H��[A^A_]��H�x H��H��L��H��[A^A_]��:  H�x H��H��L��H��[A^A_]�V H�o� H�H�=ŧ 1��Q 1�H��[A^A_]�UH��AWAVSPI��A�6�F���s?A�^M�~��~��L��M� ������˃��M�~H�� H� L���P0I�F    ���uH��[A^A_]�H��� H� H�H H�=n� 1�H��[A^A_]��UH��AWAVSPI��A�6�F���s?A�^M�~��~��L��M�������˃��M�~H��� H� L���P0I�F    ���uH��[A^A_]�H�]� H� H�H H�=� 1�H��[A^A_]��UH��AWAVAUATSH��(I��A�D$�E�L�-� I�E I�<$H�u����  A��D�}�L�u�L��L�������A�|I�E �P(H��I�E I�<$�U�H�����  H�H�t� L��L���*���L���b���H��H��([A\A]A^A_]�UH��H�?H�6]雡 UH��SH��H��� H� H�U����  �Å�uH��� H� H�}��P@��H��[]�UH��AWAVSH��8I��H��L�=<� I�1�H�U�1�H�����  ��uRI�1�H�U�1�L�����  ��u)L�=%� I�H�}�L�u�L���PP������I�L���P@H��� H� H�}��P@��H��8[A^A_]�UH��AWAVSH��8I��H��L�=�� I�H�U�1�H�����  �������uOI�H�U�1�L�����  �������u#L�=�� I�H�}�L�u�L���PP��I�L���P@H�k� H� H�}��P@��H��8[A^A_]�UH��SH��H��H�
  H��H�=�� 1�1�H��A��1��L��H�Ã�u�H��[A^A_]�UH��H��H�
  H��I�L�����
  H��H��蟜 ������H��[A^A_]�UH��AWAVSPI��L�=p� I����
  H��I�L�����
  H��H��H��[A^A_]�K� UH��H��H�4� H� H�u���X  H���U�1���t�1H�����u�H��]�UH��H��H��� H� H�U���(  H��]�UH��AWAVSH��I��H��L�=�� I�1�H�U�1�H����(  ��u+I�1�H�U�1�L����(  ��u�E���E� fH~Ã���H��[A^A_]�UH��AWAVSH��I��H��L�=c� I�H�U�1�H����(  �������u9I�H�U�1�L����(  ��u#�E��Mػ   f.�w1�f.Ȼ����F؉�H��[A^A_]�UH��AWAVSPI��H�E�    L�=�� I�1�H�U�1�L����(  ���    t&I�L�x L�����
  H��H�=0� 1�1�H��A��1��L��H�Ã�u�H��[A^A_]�UH��H��H�z� H� H�U���@  H��]�UH��AWAVSPI��H��L�=P� I�1�H�U�1�H����@  ��u$I�1�H�U�1�L����@  ��u�E�;E����؉�H��[A^A_]�UH��AWAVSPI��H��L�=�� I�H�U�1�H����@  �������uI�H�U�1�L����@  ��u�]�+]���H��[A^A_]�UH��AWAVSPI���E�    L�=�� I�1�H�U�1�L����@  ���    t&I�L�x L�����
  H��H�=�� 1�1�H��A��1��L��H�Ã�u�H��[A^A_]�UH��H��H�%� H� H�U�H�M���x  H��]�UH��AWAVSPI��L�=�� I����
  H��I�L�����
  H��H���ߘ ������H��[A^A_]�UH��AWAVSPI��L�=�� I����
  H��I�L�����
  H��H��H��[A^A_]鋘 UH��H��H�t� H� H�u���X  H���U�1���t�1H�����u�H��]�UH��H��H�:� H� H�U���H  H��]�UH��AWAVSH��I��H��L�=
  H��H�=ќ 1�1�H��A��1��L��H�Ã�u�H��[A^A_]�UH��H��H��� H� H�U���H  H��]�UH��AWAVSH��I��H��L�=�� I�1�H�U�1�H����H  ��u&I�1�H�U�1�L����H  ��uH�E�H;E����؉�H��[A^A_]�UH��AWAVSH��I��H��L�=J� I�H�U�1�H����H  �������u5I�H�U�1�L����H  ��uH�E�H�Mػ   H9�
  H��H�=y� 1�1�H��A��1��L��H�Ã�u�H��[A^A_]�UH��AWAVATSA��A��L�%\� I�$�0   �P(H��I�$Mc�B�<�    �P(H�CH�J��H�CI�$Mc�B�<�    �P(H�C H�CJ��H�C(H��[A\A^A_]�UH��SPH��H�;H��t
H�{ �S`I��M��tIcFH��HC�H�CH��[A^A_]�UH��AWAVAUATSPI��I��I�>M�nI�7M�g����1Ʌ�tYI�?I�6�]a H��I�>�a���H�E�M�vM9�t*I�6L��H���<���L9�uI�61�H�}�����I��M9�u�H���t H�M�H��H��[A\A]A^A_]�UH��AWAVAUATSH��(H��H�]�I��I�<$H�3�x\ I��E1�M���  L�������I��L�m�I�D$I+D$H��H�sH+sH����L���
���I�t$I�D$H9���   L��H�U�H�KH�u�L�{I9���   M��L�&M�/H�}��2 H��I�D$I�|$M��H�pH+pH�H�S�v3 I�EI�}L�m�H�pH+pH�I�$H�PH+PHS�J3 1�L��H��H�]������H�U�H�u�I��L;{�w���I�D$L��H��H9��Q���L��H��([A\A]A^A_]�UH��AWAVAUATSPI��I��I�<$�2V I��L������I��I�t$I+t$H��L�������I�\$�H�;L��L���2 1�L��H���.���H��I;\$u�L��H��[A\A]A^A_]�UH��AWAVAUATSH���   H��H��`���H��@���L�/I�uI+uH��H����v I��L��H���U H��X���I�}I+}H��H�CH+I��H����)���T I��I�$�I�EHcH�4�H�T�L��L���V H��I;\$u�H��`���H��X�������L��H����U L��L��8����s���H��(���L�u�L��H��@���L����  H�g� H� H��h���L�����  H��L��H�����M��L�� �����  �M���t���uH�H �H�H I��H��8���H��8���L�9�E0 L�pI�] I;]t/H��0���I�GHcH�<�H�t�L��M�v��0 H��I;]u��H��0���H��X�������I��L��P���L���"�  I�� I�\$H��H���H�sH+3H��L�������L�+L;kH�������   H��@���H�@IcM L�<�H��X����/ H��`���L�3L;st2H��`���L�`I�GIcH�<�H�t�L��M�d$�70 I��L;su�1�H��P���H��`�������I��H��H���L;h�u���1�H��(���H��0�������H��� H� H��h�����  H��L�� ����o���L���p H��� H� H�}���x  H��(���H���   [A\A]A^A_]�UH��AWAVAUATSH��I��H��I��H�P� H� H��� �������H
H�E�H�]��QH�E�H�]�I�MH�U�L�"H��L�A�<$�tL��L����( H�M�4�I�uI�MI��H�VH+VH�I9�L��u�L�m�I�EH�HH;HL�e�tGI�MH�U�H�H��L��;�tL��L���( H�M�4�I�EI�MH��H�PH+PH�I9�L��u�1�L�}�L��H�u������H�]�H��I;\$ L�u������H�}��vh H�}��mh L��H��X[A\A]A^A_]�UH��AWAVATSI��H��L�{�(���H�;����I��I�v I+vH��L���
���I�^�HcCI�4�1�L���X���H��I;^ u�L��[A\A^A_]�UH��AWAVAUATSPI��I�������I�<$�R���I��I�T$I+T$H���   L���t���I��M�l$I�D$I9�t(I��; tI�u 1�L�������I�D$H��I��I9�u�L���rg L��H��[A\A]A^A_]�UH��AWAVAUATSH��XH�M�H�U�I��H�}�L�?I�GI+GH�E�M�&M�nI�D$I+D$H�E�M+nH�H�E�H�XH+X1�L��L���8T ��tH�݀ �   �  H�]�L�e�L�m�L�u�L�e�I��1�L��L�u�L����S D9�H�]��w  H��L�m�I��1�H�}�L����S A�E9��Q  9��I  L�������H�E�L��H�u��~N I��L���6% H��H�]�L��L���R H�E�H�E�H�HH;HL�e�H�E���   H��H�E�H�M�H�	H�M�L�{H�AH�yH�pH+pH�L���% M�t$M;t$L��A�    H�]�L�m�tXH�I��H�E�E1�I�I��H�HH�xH�qH+qH�H�U��k% H��H�u�L������L��H;C����A�I��L;qu�H�]�I��H�E�A9�L�u�u1�H�}�H�u�����H�M�H��I;NM���)���H���}% H�}��@e H�E��H� �   H�}��` 1�H��X[A\A]A^A_]�UH��AWAVAUATSH��(L�E�A��I��I��H��H�]�H�5�} L�������I��H�;�   �QJ H��H�]�H��L���H H;C��   H���,���M�&M;f��   H�E�H�E�H�@Ic$L�,�H���Y# H�E�H�XI�EI�}H�pH+pH�H���$ M��Lc�H�� H� A�O�M�D�����  J��H�]�M��� 1�H�}�H�u��r���I��M;f�E�A���u���H�E���   H�}�L���k_ H���I 1�H��([A\A]A^A_]�UH��AWAVAUATSH��   M��D��`���H��P���I��I��H��H��H���H�5f| L������I��H�;�   �
� H� D�����  K��� I�� H�       I�I�� H��X���L�h1�L��0���L��H��`����v���I��H��@���L;`�����H��� H� H�}���x  ��   L��L���_] H���G E1�L��H�ĸ   [A\A]A^A_]�UH��AWAVAUATSH��8I��H��I��M�<$L���RH I��M;w��  I��x��  L�m�L�hL��H�]�H�H�HH+HH��I�EI+EH���|�� D I��I�wL��L��L����E L��H��I�WL��L���E I�uI�UL��L���E ���B  L������H�E�H�sH+sH��H�������L�kL;k��   L�e�I�GI)�I��Mc�L�u�M�e I�\$N�4�H�T� H� 1�L��H�� ���   ����   H�E�L�<�I�F H�E�H�}��\ H�E�L�pI�|$L��L���'  H�I��I��I�D$H�pH+pIt$L��L�u�H���� H�H��H�M�H�AH�yH�pH+pH��� 1�H�}�H�u��`���I��H�E�L;h�0���H�E���   ��   L��H���U[ 1�H��8[A\A]A^A_]�H��y �   H�}��/[ L���qE 1��о   H�}�L���m[ H�}������1��UH��AWAVAUATSH��   H��H��@���H�H��X���L�cL�sL��p���H��� H� H�}���P
  I��H�f� H� H�{(���
  H��L��L���7 ����   �E�L��H���7 ����   L�}�I�_I;_tGLc}�Lc�H�H�@H�
� L�	J��J��A�   L��H�u�A��0  H��tNH��H�E�H;Xu�H�Ѧ H� L�����  E1��(H��� H� H�
� H� H�
  I��I�E H�{ ���
  H��L��L����- ����   A��L��H����- A��E����   I�E ���  H��H�E�H�XH;XtEMc�Mc�H�H�@H�
  1�1�L������L��H��A��A��H�A���H�L����   H�L��h  H�������P  L��H��A��1���   H��������  L�%k� M�$$I��H�� �DH��������H����H�˒ L������	H���   L���M�  L�%"� M�$$L������H�L��   L������L�����
  1�1�L��H��A��L���H����	H���   H�������!����6H�S�H�������   H���o6 L��H����6 L���K  L�%�� M�$$�   L;e�uC��H��X  [A\A]A^A_]�H�ߋ�H����L������H�� �=���H���   �/����M UH��AWAVAUATSH��I�ԉ�I����,H��� H� H�
  H��L��H���^! ����   H�H�M�H�IH�4��   �   �   �   L��M���7 �@���L�u�1�1����  H�Eȅ�~pI�E I�<$���
  M��I��H�}�L����  ��xp��I��I�H�H�U�H�RH��L��H�u���p  ��M��t��`�   �   �   L��I����5 ����H�u�I�E L����h  1ɉ�H��[A\A]A^A_]þ   �   �   L��M���5 M��H�}ȋ�H�����e���I�E ��   �V���UH��AWAVAUATSH��HI��A��H��A�F���r,H��� H� H�
  1�1�H�}�H��A��L����4 ��H����I�H����   H�}���H����	I���   H�}���H����	I���   D���H��H[A\A]A^A_]�f����� ���f�����������UH��AWAVAUATSH��(  I��I��H�Gz H�H�]Ѓ���  L�������   �   L���i/ I�t$I�T$ L��L���$���I��M����  I�t$(I�T$0H������L�������H������H���y  �   �` I��L��L���} L��H�������n L������I��H�#� H� I�t$H������H������L������L������L�����  ���O  ������ M����   L������I�?�
  H��L��H���+ ����   I��L�����E3 A��E���I�GI+H��I�L$I+L$H��H9�A�   }tL�-�� I�E H�}����
  H��L��H���� ��xL�����3 I;G��   H�}�H��L������H��H�]�tTI�M L��h  H���C  H��H��A��E1��vL�9M �   �   �   H�}��MA�   �   �   �   �/L�6M �   �   �   H���u+ �#�   �   �   H�}�I���X+ L���. D��H��[A\A]A^A_]�UH��AWAVATSH����uoL�rL�%t� I�$H�� H��L�����   ���   ��ufI�F H�8�S I��I�$�����L�����  I��I�$L���P0I�$H��L����h  1��$H�	� H� H�
  I�?I��L���
  I��H��L���; H��H;K��   L�m�H�H�E�H+KH�M�H�v H� K�|�����
  H�57 H���g��I��A�   �   H��� H��H��L���� H;C��   H��薡��H�E��E�    H�E�����   H��u H� I�t$H�M�H�$L�50i L��; �   E1�L��L�����	  ��taH�}�����A�   �  A�   �   �+   �   L��M���� �_  �   �+   �   L��L�E�� H���! �7  HcE�H��B�D0�E�H�]�L�}�L�e�M�l$I�D$I9���   H�M�H��Lc�L���M�l$H�M�H�	H�M�I9Ż    tFH�E�H�@N�4�1�I�E H�@J�4�H�}�L�������������}�t�����I��M;l$u�H�}���  H�E�L�pH�M�H�AH�yH�pH+pH�L���J�  Lc�H�t H� �É����  K��� 1�H�}�H�u�賡��H�M�H��I�D$H9��.���H��s H� H��h  H�}��J1  H�}�H����E1�D��H��X[A\A]A^A_]�UH��AWAVAUATSH���   I��A��I��H�b H�H�]�A��,H�ws H� H�
  I��H�I�|$���
  H������L��H��H�������{�����tbH�A���I��A���H��h  H��������/  L��H����1�H��` H��RH�_; H�������	   H���+ L��H���� �H������L���� H�����������H��` H��   H;]�u��H���   [A\A]A^A_]���- UH��AWAVAUATSH��(H��I�����E  L�sL�-�q I�E H�up L��L�����   �   ���=  M�v L�cL�e�H�[ H�]�I�>�x����A�$H�E�I�^M�fL9�tkL�3L���Z M�M 1�A�   L��H�u�H��A��0  H����   I�E L��H�u�H�U���P  ����   �}� t1�H�}�L���͞��H��I9�u�I�E L��   H�]�H�����
  1�1�L��H��A��H�}���H����
I�E ��   ��H����
  1�1�L��H��A��H�}���H����
I�E ��   ��H����
���I��L��H������L���
 H������L��� L�������9L�������   L��L����
 H������L���_ L��贍��L�%#U M�$$�   L;e���  ��H��x  [A\A]A^A_]�L������H������L�q ��T�  I�Ń�|UD�c�H������H�X0H�s�H�H������H��������v��H����  L��H���_�  I;E��  A���H��A���L������H������A�L������H������H��M��L������M��L������L��L������L������H�KH;K��  H������H� H�PH+PIT$H��x���H������8H�������@���t���1�H������H������M��L�!1�L��������I��L��L��蚲��H����"  H��L���|�  H������L�hL���
  1�1�H������H��A��M����H����I�H����   I�H��h  H��������   H������H����L�%�Q M�$$1�����L�=@c H���������  L�%�Q M�$$I��H�������HL�=c H��������H����L�%vQ M�$$H������	I���   H�������x�  L������I�L��   H�����
  1�1�L��H��A�Ջ�H����I�H����   H�������^��������H������H�������"H�S�H�������   H��� H������H���� L���q�  H����������L�=:b H���������  ��H��H�߃�L�%�P M�$$L������H�������-���I���   ������ UH��AWAVAUATSH��x  I��L�=IP M�?L�}Ѓ�$H��a H� H�
+ �   L����P  �d  H������I��I�]L�5�a I�H�2` L��H�����   �   ���1  H�[ L��L�3H� I�uH������H������L����x  ����  H�������   �5   �� H�������@�Hc�Hi�VUUUH��H��?H�� ��R9�t+H��& H�������   H��� L��H���y �  L������I�M L������H��������������  I�ŋ�����H������H�sH+sH������H������������ ~wH������E1�H�g` H� J�<����
  L��H���b�  I��M;f��  I�T$L��L��L���!�  ����  M+fI��H������D��D���#���I��D;�����|�H��������L������|QD�x�H������L�p0I�v�I�L��H�������\p��H���6  L��H�����  I;E�-  A���I��A���L��p���M��L���s���H�������   L������L���_���H��L������L��H��耔��I��L��x���H��� H������� L��L��L��M���~���I�MI;M��  I�E H�PH+PIWH��h���H������8H�������@���d���1�H������H������L�)1�H��������菉��I��L��L���
���H���6  I��L�����  H������L�xL��� H��^ H� 1�A�   H������H��H������L����0  H���H  I�EI�}H�pH+pH�L���l�  H����������   H�M�4�D��d���L������L��h���H�^ H� I�u H������H��H��������x  ����  I�4$H������H��H��������o��H��H����  �H��������H���   9�H��] H� ��   I��I�I��I��A���A���g���H��������1�H������H����������H������H��L��x���I;ML������L��p����O���L�=8] I�L��   H������H�����
  1�1�L������L��H��A�ԋ�H����I�H����   L��跋��I�H��h  H�������Q  L��H����L�==K M�?1��  J��H�������   H����  H������H���9 L����  H������肃���   L�=�J M�?�4  J��H�������   H���  H��������   H���������  L�=�J M�?�@H��������H����L�=�J M�?H�\ H� ��   H��������  H������L�%�[ I�$L��   L������L�����
  1�1�H��H��A��A�E �H�A�M ��
  H������W�)�X 1�H������A����  H�� A���L������1�H������1�H������I�H�3L�$$�   E1�L��H��L L�I" ���	  ������Hc�����H�
   H�����  L��H���3�  A�   H������L������M��tL�����  H��tH�����  H��C H� H;E�uD��H��(  [A\A]A^A_]�� f�|�������<���~���UH��AWAVAUATSPI����uL�zL�-U I�E H��S L��L�����   �   ����   M�g I�$H�AH�IH)�H��u]H�8H�p��`����t^I�E H��h  L���x���H���  L��H����1��QH��T H� H�
  H��H�������   �=   ���  L��H��H�������9���H��tiH�
  H��H�������   �@   ���  L��H��H������薓��H��tiH�
  L��H����E1��(H��L H� H�
  1�1�L��H��A��H�}���H����	I���   A�$�H�A�$��I�L����   H�}���H����	I���   ��tH�}���w���   ��H��H[A\A]A^A_]�I�H��h  H�}��I  L��H����1���j�������j�����������UH��AWAVAUATSH��8  A��H��L�%7 M�$$L�e�A��*H�vH H� H�
  I��L��L����  H��I;^��  H�SL��H��H�������`�  ���  I+^H��H���������W�  I��L;�����|�I�~I+~H���   +�������4�  I��I�vI+vH��H���������  I��I�I;_L�%r5 M�$$t%I�FHcH�4�H�T�L��L�����  H��I;_u�L������H��F H� L������L�����
  H��H������H���P��I��L��L�����  I;E�{  L��L�������]r��H������H������H�sH+sH��H���s��H�KH;KH�������<H�������"�  I�� 1�H������H��������s��H������H��H������H;HL��������   H������I��L�)H��������  H������L�pI�$�"I�EHcH�<�H�t�L���ح  H�M�4�H��I;\$u�H�������ج  H������L��L�#L;c�A���H������L�xI�EIc$H�<�H�t�L��耭  H�M�<�I��L;cu�����H����  L����  H�3E H� H��h  H�������  H������H����E1��   H�������   L���*�  H������H��������  L���O��L�����  H�����  H�������&�  H��������  A�   �   H��������  H���������  A�   �   �@   �   H������M����  L�%�2 M�$$�GH�������v�  H��������  �   �@   �   H������M�����  L�%�2 M�$$A�   L;e�uD��H��8  [A\A]A^A_]����  UH��SPH��H�{ �r��H�C     H�C    H��[]�UH��SPH��H� �jp��H��tH�C H�\B H�CH��[]�UH��SPH��H�{ ����H�CH����  �CH��[]�UH��AWAVAUATSH��  I��H��L�-�1 M�m L�m�H�IC H� H������H��������x  A�   ����   H�������   1����  ������uCH������H�0H������H���)�  H��tGH������H�QL������H��H��L���   A���"L�������   L��L�����  H��L���H�  L;m�uD��H��  [A\A]A^A_]��g�  UH��SPH��H�sB H� ���  H�X H�
H���ө  �4L�u�E1�1�L��H����n����u"�   L��L���B�  H�}�L����  A�   D��H��[A\A]A^A_]�UH��AWAVAUATSPI��H��I��I��H��@ H� H��@ L��H�����   A�   ��uZL�e�L�s I�} I�v�l�  I��L��L��L���8n��E1���u%�   L�u�L��H����  L��L����  A�   L����  D��H��[A\A]A^A_]�UH��AWAVAUATSPM��H��I��H�u�I��H�@ H� H��? L��H�����   A�   ��uYL�c I�>I�t$��  L��I��H�u�L��L���o��E1��u$�   M��L��H�����  L��L���Q�  A�   L����  D��H��[A\A]A^A_]�UH��AWAVAUATSH��8M��L��I��H�u�H�}��> t-L�"H�[? I��H� I�<$���
  H�5�
 H���D�  ��tI�?H�3L�����  1��_  L�m�L�u�M�d$M�/L�3H�	? H��H�H�U�H�M�L�}�L��L����x  ��H���  L��L�m�I��L�e�EԨ��   �������H�}��f���}� M��~uH�E�I�M H�8���
  H�}�H�����  ��I�E H�M�H�y���
  L��H�����  ��x^��xcH�}��މ��qf����t`��te�MԍA��E�H�E�H��H�Eȃ��H�E�� �H�E�H� ������	   L���K�  L��M���:H�E�H��H�E�H�P�   �H�E�H�P�H�E�H��   H�]H���	�  L��H���n�  �   H��8[A\A]A^A_]ÐUH��AWAVAUATSH��A��I��I��H��H�xH�U��PP1ۃ}� ��   L�5y= H�E�I�C�<����8�P(H��I�L�p(L���g�  �xA��H�H��L���M�  L���i��H������H�C� �   �S�  H�C1��H�  H�CH�C(    H�C     D�{0H�{8Ic�H��H�4@���  H�E�H�XH��H��[A\A]A^A_]�UH��SPH��H��H�xH�3�PHH�
I�$��   I�$H���P0M��L��u�I�$L���P0H�� [A\A^A_]�UH��AVSH���3H��wH��   Hc�H���L�sI�~(�=H��9 H�H�=� 1��Q �rL�sI�~��  �SL�sI�~ H��t�`��I�~@H��t8�`���1L�sI�>�|�  I�~H��t��H����H�S9 H� ��   H�C9 H� L���P0H�39 H� H��[A^]�`0f�V���x�����������UH��AWAVSPA��I��H��8 H� �   �P(H�ÿ   ��  H�CD�;I�>H��H��[A^A_]�s�  UH��SPH��8 H� �   �P(H�ÿ   ���  H�CH��H��[]�UH��H�H�HH+H������]�UH��AWAVATSI��I��I�<$H�GH;u@H�X8 H� �   �P(H�ÿ   �t�  H�C�   I�<$H�����  I�<$��  �
  H�{H+{H��1��+�  I��I�~@�   L���{X��1���  I��1���  I��A�V0A�v4L��L��M���  �É]�L���+�  ��u@L�
  �É]�L���x�  ��uGL�
  L�
  H�}��R�  H���J�  H�}��A�  �$�]�1Ʌ�t�}� t�}� t�}� t����D��D��H��X[A\A]A^A_]�UH��AWAVAUATSPI��I��I�GL�h H�^ I�} H�3�N�  I��M��t)I�} �LU��I��H�{H�sL��L����Z��L����  �H���W��I��L��L��L��H��[A\A]A^A_]�    UH��AVSI��H����  ��H�{t6H�G�PH�CL�p H�{H��' H�GH��( H� ��`  A�   �H�w E1�1�H���  D��[A^]�UH��AWAVATSI��H��I��I�GL�` L���FV����tII�D$I�L$H)�H�������H�Hc�H�4�L��L���T   ���   ��u@I�T$H�r�L����u���+H���%�  H�þ   L��H����  H�!( H� H���P01�[A\A^A_]�UH��AWAVAUATSH��(H��H�u�D�{0�   E��tH�u�H�M�H��8Lc�L�m�H�H�M�L�sL��H�u�L���SP�}� tL�`I��XA��L��uҸ   �7H�]�H��t,H�}��w�  I�ƾ   H��L���T�  H�s' H� L���P01�H��([A\A]A^A_]�UH��AWAVATSI��I��I�7�_   I�FL�` I�WL��L����t��I��M;|$t2L��H�3L���0   H�3H��I+T$H��1�L�������H��I;\$u�L��[A\A^A_]�UH��AWAVATSH��H��H�u�D�{0E��t6H��8L�u�L�%�& H�H�E�H�{L���SHI�$H����p  H��`A��u�H��[A\A^A_]�UH��AWAVAUATSPI��H���M�  I��L�����  E�n0A�����E��t/I��8L��H�3L����  ��uH��`A��u��L)�H��Di�����L���}�  D��H��[A\A]A^A_]�UH��SH��H��H�HH�Y H�U�Hc�H�IH��H�T8H�U�H�|@H�u����   H��tHc@H��HC�H�CH��[]�UH��AVSH��H�{�	�  I��H�{�~�  1�H��L��[A^]�\���UH��SPH��H�{���  H���,T��H�{H��[]�D�  UH��AWAVSPH��A��I��H�6% H� �   �P(D�pH�X�I�O H�I�G H��[A^A_]�UH��AWAVAUATSH��A��H�}�H�_ L�5�$ I�H�����
  I��1�H����   E1�1��E�D9kuI�H�{���
  H��L����  ��t
  L��H��L����e  A��E��t��d  ����   I�t$L��L���fg  A���C  ����   I�t$L��L���7q  A���"  H�� H�H�=k�  E1�1��Q �  ��H�c H���   I�|$I�����
  L��L��H���e  I�$H��h  H���������  L��H����E1��   ��H�
 ~XH� H�
  H��L��L���o  A��D��H��[A\A]A^A_]� @�����������@�������X��������uH��H��H���_Y  UH��H� H� H�
  H������L�������   �	   L���:�  H�sL��L��袳  �   H��t)A���H�� L�,$L��L��H������H��E��I����8  ��H�� H� H;E�u��H��  [A\A]A^A_]����  UH��AWAVAUATSH��HI��H��I����L�=� I���   H�{���
  L��L��H���?  A�   H����   I�H�pH�Z L��I�����   ��ugL��L��L����@  ��uUI�GH�@ H�E�L��L���������tLA�( tbL�C�   �   �-   �CH�
  1�1�L��H��A�׋�H����H�u H� H����   H�}���H����H�R H� ��   E������L��L����n  A��E�������L�u�E��H� tH�H�M�H�y��`  H�H��h  D�����  L��H����E1�����UH��AWAVAUATSH��  I��I��A��I��H�6 H�H�]�H�� H� A��#H�
  L��L��L��H���2<  H��A�   H����   H�P H� H�qH�� L��H���������   ����   L��H��H�������z=  ����   L������L������I�D$H�@ H������H�������   �   蟶  H��H������L���M�����t0L���x( L������H��tmI�WH�������-   H���ж  L���I�WH�������,   H��賶  H������H����  H��  H�H;]��a  D��H��  [A\A]A^A_]��F(   A���I��L������L������L��D��I��L���R?  I��M����   ������L��L�������H��L���z  E1�H������H;X�    ��   H�;H������贫  I��A�L��L��L���
r  ��E1���u)L��H�������C���H�� H� I�}��`  A�   A��H�A���H�m H� L����   ������E1�A�E(    ��L��H��������k  A��E��H���  H������H� H� L��h  D�����  L��H��A��E1��������  UH��AWAVAUATSH��  I��I��A��I��H�F�  H�H�]�H�� H� A��#H�
  L��L��L��H���B9  H��M��A�   H����   H�] H� H�qH�� L��H���������   ����   L��H��H�������:  ����   L������L������I�FH�@ H������H�������   �>   譳  H��L���b�����tA�~( tbI�UL�������-   �I�UL�������,   L���߳  H������L���@�  H��  H�H;]���  D��H��  [A\A]A^A_]�H������A�F(   A���I��L������L��E����   M�u H�= H� H������L��H��

  L��H��H����4  I�H��h  H��H���  t#�   �H�
  L��L��H���R4  I��M���
  H�I�vH�
 L�����   ����   L�}�I�FH�@ H�E�H�1�1����  H�E�A�F0����   I��8�E�L�u�M�&H�1�1����  I��M�<$�I��M;|$t:H�E�H�8A�7���  H�L��p  H�8��������  H�}�L��H��A�օ�t��iH�H�}�H�u�L����p  ��uQL�u�I��`�E����p���H�H�}�H�u���h  1��H�
  L��H��H���l2  A�   H����   I�H�pH�6 L��I�����   ����   L��H��L����3  ��uvH�������   �   �
  L��L��L��H���1/  H��M��A�   H����   H�L H� H�qH�� L��H���������   ����   L��H��H�������v0  ����   L������L������I�FH�@ H������H�������   �   蜩  H��L���Q�����tA�~( tbI�UL�������-   �I�UL�������,   L���Ω  H������L���/�  H���  H�H;]���  D��H��  [A\A]A^A_]�H������A�F(   A���I��L������L��E����   M�u H�, H� H������L��H�� ���   �   ���  I�v H��H�������87��I��L9�tH���3��A��I��M��L��u�H������L������L���b�  L�5.�  M�6M��H������L�������   ��   H���
  L��L��H����+  A�   H����   I�H�pH�� L��H���������   ����   L��L��H�������H-  ����   H������H�@H�@ H������H�sI�H�j L��I�����   ��uqI�G H������H�������   �&   �L�  L��H�������������tfH�������x( t~H�SH�������-   �WH�
  H��L�5�  I�1�1����  H�E�I�H�u�L�����  �
  L��H��H���'  H���   H��t>I�H��h  H�9��������  L��H����1��H�
  H��L��H���&  I��A�   M���+  H���  H� I�t$H�c�  H�����   ���  H������I�D$H�X H�������   �/   �?�  A���A����   H������I��L������L������H������L��D��L���z*  H��H����   L������I�} �)��I�ǋ�����L��H���
  L��H��H����$  H��A�   H���c  H���  H� H�qH������H�y�  L�����   ���6  L��H��H�������&  ���  H������H������H�@H�X H�������   �3   �5�  A����  I��I�L$H�Y�  H� H���  L��I��H��L�����   ����  H������I�] I�?H�3裉  ��t:H������H������H��������tf�{( H����   I�T$H�������-   �WH�;��  H��L�������   L��H��虞  H������L���U�  H���  H� H���P0�  I�T$H�������,   H��跞  H������H����  ��   �F(   A�E H������L������L��H��L����]  I��A�   M��tOH������H��L������E1���uH������L��趞  A�   A��H�A���H���  H� L����   A�E �H�A�M ��H���  H� L����   �C(    L��M��H������D���DT  A��E��u!H���  H� H������H�qL����h  E1�H���  H� H;E�uD��H��  [A\A]A^A_]��<�  UH��AWAVAUATSH��I��I�ԉ�I��L�55�  I���H�
  L��L��H���=!  H����   �u�H��wtH�
I�H�=@�  1��Q �   ��H��[A\A]A^A_]Ã���   I�T$ L��L���V  ���҃���   L��L���WW  ���I�H��h  H�=@�  ��������  L��H����돃���   L��H���S  ���y�������   I�T$(L��H���iU  ���Y�����uvI�T$(I�L$0L��H���4S  ���8���I�H�
  L��L��L������H���  L��I��A�   M����   H���  H� I�vH�L�  H�����   ��u}H��H������L����  ��ugH�������   �<   �&�  H������L���׻����tA�~( t`I�T$L�������-   �I�T$L�������,   L���R�  H��L��跘  H���  H�H;]��;  D��H��  [A\A]A^A_]�A�F(   A���I��I�FH�@ H�8���H������ǅ����    �������H��������  E��~fA��I�$I��H��L��H������L�������N  I��M��t�I�w H������H�������!��A��H�A����H�G�  H� L����   �A�F(    1�H��H�������M  A��E��tH��������!������������� L�=��  t
  L��L��L��H���  H��M��A�   H����   H�&�  H� H�qH���  L��H���������   ����   L��H��H�������P  ����   L������L������I�FH�@ H������H�������   �   �v�  H��L���+�����tA�~( tbI�UL�������-   �I�UL�������,   L��訕  H������L���	�  H���  H�H;]���  D��H��  [A\A]A^A_]�H������A�F(   A���I��L������L��E����   M�u H��  H� H������L��H���  ���   �   ���  I�v 1�H��H�������� ��I��L9�tH�����A��I��M��L��u�H������L������L���:�  L�5�  M�6M��H������L�������   ��   H������I��A�H������L������L��H������L����S  H��L�5��  M�6M��tH������H������H���y���1Ʌ�uH������L��誔  �   ������A��H�A���H������H���  H� L����   L�������������"L�54�  M�6M��L������H������L������A�F(    L��H���'J  L��A��E��L���"���H�u�  H� I�v��h  E1������?�  UH��AWAVATSI��I�ԉ�I����)H�9�  H� H�
  L��L��H���X  I�ĸ   M����   H�I�t$H��  L�����   ����   L��L��L���  ����   I�D$H�X H�������   �A   ��  L��L��衵����tIA�|$( ��   I�VH�������-   �;H�
�  I��A�H��  L�1�A�   L��H������L��A��0  H����   H�L��H������H��������P  ��u{������ L��L������t0H������H�$L��L��L��H������I��L�������  ��ue��H����H�u�  H� H����   I��1�H������L;p�%����XA��H�A��   ��EH�6�  H� L����   �   �+��H����H��  H� H�߉���   ��1Ƀ�Eʉ�����A�D$(    ������L��L���nF  ������L������I�GI;GtH���  H� I�|$��`  L�5��  I�H��   L������L�����
  1�1�L��H����H��������H����	I���   A�$�H�A�$��H���  H�I�L����   H��������H����	I���   ��������t/������ u&I�L��h  L��A���r���L��H��A��D���E���L������   �3���豦  UH��AWAVAUATSH��8  H��I��I��H�!�  H� H�EЃ�L�5��  I��P  I�}���
  L��H��H���@  A�   H���G  I�H�pM��I��H��  L�����   ���"  L��H��L���  ���  H������I�D$H�@ H������H�������   �B   �Í  H���  H� I�u H������H������L����x  ����   ������H������L������L������L��L����  H����   L������L��H��I������H������L���R  H������L��������tsA�|$( I����   M�E�   �B   �-   L��肏  �H�
  1�1�L��H��A�׋�H����
  L��L��H���V  H�ø   H����  I�$H�sH��  L�����   ����  L��L��H���  ���s  L������H�CH�@ H������H������L�pI�$H���  L��L�����   ���3  I�F H������H� L�xL;xL������tDM�7I�<$I�6��t  I�$H;A�<  H�0L���������Z  I��H������H� L;xu�L������I��L������A�~0轗  H������A�~0��  H������A�~0 �)  I�^8E1�L������H�H�xH+8H���݌  H������H�L�0L;p��   D������1ɉ�����H������H�8A�6�q  I��I�<$I�u �Yu  A��A���toI�<$D����p  L��H��������tTH������D���Ҏ  ��������I��H�L;pu�E��D������xDH������H�������A�  H������D��菎  � H��������  ������ D�������5  H��`A��L������A�F0H�@H��I�D8H9������L������H������H�CH;uuH���,�  H�������{�  H������L�@�   �C   �   H�������.�  �  H�