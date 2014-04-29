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

    # We need lassign
    if {[info procs lassign] eq ""} {
        proc lassign {values args} {
            uplevel 1 [list foreach $args $values break]
            lrange $values [llength $args] end
        }
    }
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
create unique index __ral_version_id_index on __ral_version (Vnum) ;
create table __ral_relvar (
    Vname text not null,
    constraint __ral_relvar_id unique (Vname)) ;
create unique index __ral_relvar_id_index on __ral_relvar (Vname) ;
create table __ral_attribute (
    Vname text not null,
    Aname text not null,
    Type text not null,
    constraint __ral_attribute_id unique (Vname, Aname),
    constraint __ral_attribute_ref foreign key (Vname) references
        __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create unique index __ral_attribute_id_index on __ral_attribute (Vname, Aname) ;
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
create unique index __ral_identifier_id on
    __ral_identifier (IdNum, Vname, Attr) ;
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
create unique index __ral_association_id on __ral_association (Cname) ;
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
create unique index __ral_partition_id on __ral_partition (Cname) ;
create index __ral_partition_ref on __ral_partition (SuperType) ;
create table __ral_subtype (
    Cname text not null,
    SubType text not null,
    SubAttrs text not null,
    constraint __ral_subtype_id unique (Cname, SubType),
    constraint __ral_subtype_ref foreign key (SubType)
        references __ral_relvar (Vname) on delete cascade on update cascade
        deferrable initially deferred) ;
create unique index __ral_subtype_id on __ral_subtype (Cname, SubType) ;
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
create unique index __ral_correlation_id on __ral_correlation (Cname) ;
create index __ral_correlation_assocref on __ral_correlation (AssocRelvar) ;
create index __ral_correlation_oneref on __ral_correlation (OneRelvar) ;
create index __ral_correlation_otherref on __ral_correlation (OtherRelvar) ;
create table __ral_procedural (
    Cname text not null,
    Script text not null,
    constraint __ral_procedural_id unique(Cname)) ;
create unique index __ral_procedural_id on __ral_procedural (Cname) ;
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
#   <list of constaints> :
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
    set result [list]

    lappend result Version [getVersion]

    # Get the names
    set names [lsort [relvar names $pattern]]
    set relNameList [list]
    foreach name $names {
        lappend relNameList [namespace tail $name]\
            [relation heading [relvar set $name]] [relvar identifiers $name]
    }
    lappend result Relvars $relNameList

    # Constraint information contains fully qualified relvar names
    # and must be flattened to a single level.
    set constraints [list]
    foreach cname [lsort [relvar constraint names $pattern]] {
        lappend constraints [getRelativeConstraintInfo $cname]
    }
    lappend result Constraints $constraints

    set bodies [list]
    foreach name $names {
        lappend bodies [list [namespace tail $name] [relvar set $name]]
    }

    lappend result Values $bodies

    return $result
}

proc ::ral::serializeToFile {fileName {pattern *}} {
    set chan [::open $fileName w]
    set gotErr [catch {puts $chan [serialize $pattern]} result]
    ::close $chan
    if {$gotErr} {
        error $result
    }
    return
}

# Restore the relvar values from a string.
proc ::ral::deserialize {value {ns ::}} {
    if {[llength $value] == 4} {
        # Assume it is 0.8.X style serialization.
        deserialize-0.8.X $value [expr {$ns eq {} ? "::" : $ns}]
        return
    }
    set ns [string trimright $ns :]::
    if {[llength $value] != 8} {
        error "bad value format, expected list of 8 items,\
                got [llength $value] items"
    }
    set versionKeyWord [lindex $value 0]
    set versionNumber [lindex $value 1]
    set relvarKeyWord [lindex $value 2]
    set relvarDefs [lindex $value 3]
    set cnstrKeyWord [lindex $value 4]
    set cnstrDefs [lindex $value 5]
    set bodyKeyWord [lindex $value 6]
    set bodyDefs [lindex $value 7]

    if {$versionKeyWord ne "Version"} {
        error "expected keyword \"Version\", got \"$versionKeyWord\""
    }
    if {![package vsatisfies $versionNumber [getCompatVersion]]} {
        error "incompatible version number, \"$versionNumber\",\
            current library version is, \"[getVersion]\""
    }

    if {$relvarKeyWord ne "Relvars"} {
        error "expected keyword \"Relvars\", got \"$revarKeyWord\""
    }
    foreach {rvName rvHead rvIds} $relvarDefs {
        set fullName $ns[string trimleft $rvName :]
        set quals [namespace qualifiers $fullName]
        if {!($quals eq {} || [namespace exists $quals])} {
            namespace eval $quals {}
        }
        eval [list ::ral::relvar create $fullName $rvHead] $rvIds
    }

    if {$cnstrKeyWord ne "Constraints"} {
        error "expected keyword \"Constraints\", got \"$cnstrKeyWord\""
    }
    foreach constraint $cnstrDefs {
        eval ::ral::relvar [setRelativeConstraintInfo $ns $constraint]
    }

    if {$bodyKeyWord ne "Values"} {
        error "expected keyword \"Values\", got \"$bodyKeyWord\""
    }
    relvar eval {
        foreach body $bodyDefs {
            foreach {relvarName relvarBody} $body {
                ::ral::relvar set $ns[string trimleft $relvarName :] $relvarBody
            }
        }
    }

    return
}

proc ::ral::deserializeFromFile {fileName {ns ::}} {
    set chan [::open $fileName r]
    catch {deserialize [read $chan] $ns} result opts
    ::close $chan
    return -options $opts $result
}

proc ::ral::deserialize-0.8.X {value {ns ::}} {
    if {[llength $value] != 4} {
        error "bad value format, expected list of 4 items,\
                got [llength $value] items"
    }
    set version [lindex $value 0]
    set relvars [lindex $value 1]
    set constraints [lindex $value 2]
    set bodies [lindex $value 3]

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
    set ns [string trimright $ns :]::
    if {[llength $value] != 8} {
        error "bad value format, expected list of 8 items,\
                got [llength $value] items"
    }
    set versionKeyWord [lindex $value 0]
    set versionNumber [lindex $value 1]
    set relvarKeyWord [lindex $value 2]
    set relvarDefs [lindex $value 3]
    set cnstrKeyWord [lindex $value 4]
    set cnstrDefs [lindex $value 5]
    set bodyKeyWord [lindex $value 6]
    set bodyDefs [lindex $value 7]

    if {$versionKeyWord ne "Version"} {
        error "expected keyword \"Version\", got \"$versionKeyWord\""
    }
    if {![package vsatisfies $versionNumber [getCompatVersion]]} {
        error "incompatible version number, \"$versionNumber\",\
            current library version is, \"[getVersion]\""
    }

    if {$relvarKeyWord ne "Relvars"} {
        error "expected keyword \"Relvars\", got \"$revarKeyWord\""
    }
    foreach {rvName rvHead rvIds} $relvarDefs {
        set fullName $ns[string trimleft $rvName :]
        set quals [namespace qualifiers $fullName]
        if {!($quals eq {} || [namespace exists $quals])} {
            namespace eval $quals {}
        }
        if {![relvar exists $fullName]} {
            eval [list ::ral::relvar create $fullName $rvHead] $rvIds
        }
    }

    if {$cnstrKeyWord ne "Constraints"} {
        error "expected keyword \"Constraints\", got \"$cnstrKeyWord\""
    }
    foreach constraint $cnstrDefs {
        set cname [lindex $constraint 1]
        if {$cname eq "-complete"} {
            set cname [lindex $constraint 2]
        }
        set cname $ns[string trimleft $cname :]
        if {![relvar constraint exists $cname]} {
            eval ::ral::relvar [setRelativeConstraintInfo $ns $constraint]
        }
    }

    if {$bodyKeyWord ne "Values"} {
        error "expected keyword \"Values\", got \"$bodyKeyWord\""
    }

    set failedMerge [list]
    relvar eval {
        foreach body $bodyDefs {
            foreach {relvarName relvarBody} $body {
                set relvarName $ns[string trimleft $relvarName :]
                if {[catch {::ral::relvar union $relvarName $relvarBody}]} {
                    lappend failedMerge $relvarName $::errorCode
                }
            }
        }
    }

    return $failedMerge
}

proc ::ral::mergeFromFile {fileName {ns ::}} {
    set chan [::open $fileName r]
    catch {merge [read $chan] $ns} result opts
    ::close $chan
    return -options $opts $result
}

proc ::ral::storeToMk {fileName {pattern *}} {
    package require Mk4tcl

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
        set names [relvar names $pattern]
        set basenames [list]
        foreach name $names {
            set basename [namespace tail $name]
            lappend basenames $basename
            set heading [relation heading [relvar set $name]]
            set ids [relvar identifiers $name]
            ::mk::row append db.__ral_relvar Name_ral $basename\
                Heading_ral $heading Ids_ral $ids View_ral $basename
            # Determine the structure of the view that will hold the relvar
            # value.  Special attention is required for Tuple and Relation
            # valued attributes.
            set relvarLayout [list]
            foreach {attr type} $heading {
                lappend relvarLayout [mkHeading $attr $type]
            }
            ::mk::view layout db.$basename $relvarLayout
        }
        # Get the constraints and put them into the catalog.
        set partIndex 0
        foreach cname [relvar constraint names $pattern] {
            set cinfo [relvar constraint info $cname]
            # Constrain names must be made relative.
            ::mk::row append db.__ral_constraint Constraint_ral\
                [getRelativeConstraintInfo $cname]
        }
        # Populate the views for each relavar.
        foreach name $names basename $basenames {
            ::mk::cursor create cursor db.$basename 0
            relation foreach r [relvar set $name] {
                ::mk::row insert $cursor
                mkStoreTuple $cursor [relation tuple $r]
                ::mk::cursor incr cursor
            }
        }
    } result opts
    ::mk::file close db
    return -options $opts $result
}

proc ::ral::loadFromMk {fileName {ns {}}} {
    package require Mk4tcl

    set ns [string trimright $ns :]::
    namespace eval $ns {}

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
            eval [list ::ral::relvar create $relvarName\
                    $relvarInfo(Heading_ral)] $relvarInfo(Ids_ral)
        }
        # create the constraints
        set assocCols {Name RingRelvar RtoRelvar}
        ::mk::loop cnstrCursor db.__ral_constraint {
            eval ::ral::relvar [setRelativeConstraintInfo\
                $ns [lindex [::mk::get $cnstrCursor] 1]]
        }
        # fetch the relation values from the views
        relvar eval {
            ::mk::loop rvCursor db.__ral_relvar {
                array set relvarInfo [::mk::get $rvCursor]
                ::mk::loop vCursor db.$relvarInfo(View_ral) {
                    eval [list ::ral::relvar insert\
                        $ns[string trimleft $relvarInfo(Name_ral) :]\
                        [mkLoadTuple $vCursor $relvarInfo(Heading_ral)]]
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

    set ns [string trimright $ns :]::

    ::mk::file open db $fileName -readonly
    catch {
        mkCheckVersion db
        # determine the relvar names and types by reading the catalog
        ::mk::loop rvCursor db.__ral_relvar {
            array set relvarInfo [::mk::get $rvCursor]
            set relvarName ${ns}$relvarInfo(Name_ral)
            # Check if the relvar already exists
            if {![relvar exists $relvarName]} {
                # New relvar
                # check that the parent namespace exists
                set parent [namespace qualifiers $relvarName]
                if {!($parent eq {} || [namespace exists $parent])} {
                    namespace eval $parent {}
                }
                eval [list ::ral::relvar create $relvarName\
                        $relvarInfo(Heading_ral)] $relvarInfo(Ids_ral)
            }
        }
        # create the constraints
        set assocCols {Name RingRelvar RtoRelvar}
        ::mk::loop cnstrCursor db.__ral_constraint {
            catch {
                eval ::ral::relvar [setRelativeConstraintInfo\
                    $ns [lindex [::mk::get $cnstrCursor] 1]]
            }
        }
        # fetch the relation values from the views
        set failedMerge [list]
        relvar eval {
            ::mk::loop rvCursor db.__ral_relvar {
                array set relvarInfo [::mk::get $rvCursor]
                set body [list]
                ::mk::loop vCursor db.$relvarInfo(View_ral) {
                    lappend body [mkLoadTuple $vCursor $relvarInfo(Heading_ral)]
                }
                set value [eval\
                    [list relation create $relvarInfo(Heading_ral)] $body]
                if {[catch {::ral::relvar union\
                        $ns[string trimleft $relvarInfo(Name_ral) :] $value}]} {
                    lappend failedMerge\
                        $ns[string trimleft $relvarInfo(Name_ral) :]\
                        $::errorCode
                }
            }
        }
        set failedMerge
    } result opts

    ::mk::file close db

    return -options $opts $result
}

# Save all the relvars that match "pattern" in to a SQLite database named
# "filename".
proc ::ral::storeToSQLite {filename {pattern *}} {
    package require sqlite3

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
            foreach relvar [relvar names $pattern] {
                # First the relvars and their attributes and identifiers
                set basename [namespace tail $relvar]
                sqlitedb eval {insert into __ral_relvar (Vname)\
                        values ($basename) ;}
                foreach {attrName type} [relation heading\
                        [relvar set $relvar]] {
                    sqlitedb eval {insert into __ral_attribute (Vname, Aname,\
                            Type) values ($basename, $attrName, $type) ;}
                }
                set idCounter 0
                foreach identifier [relvar identifiers $relvar] {
                    foreach idattr $identifier {
                        sqlitedb eval {insert into\
                            __ral_identifier (IdNum, Vname, Attr)\
                            values ($idCounter, $basename, $idattr) ;}
                    }
                    incr idCounter
                }
                # Next the constraints
                foreach constraint [relvar constraint member $relvar] {
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
            foreach relvar [relvar names $pattern] {
                set relValue [relvar set $relvar]
                set sqlTableName [mapNamesToSQL [namespace tail $relvar]]
                set attrNames [relation attributes $relValue]
                # Map the attribute names to a set of SQLite column names.
                set sqlCols [mapNamesToSQL $attrNames]
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
                    lappend sqlVars \$$sqlCol
                }
                # This completes the composition of the "insert" statement.  We
                # just managed to add a list of variable references (i.e.
                # variable names each of which is preceded by a "$").
                append statement [join $sqlVars {, }] ") ;"
                # Finally after all of that, the actual populating of the
                # SQLite tables is trivial.
                relation foreach row $relValue {
                    # "relation assign" for a tuple or relation valued
                    # attribute gives the heading and the body. Later when we
                    # are loading the results, the "relvar insert" command will
                    # want that stripped off.
                    relation assign $row {*}$assignVars
                    sqlitedb eval $statement
                }
            }
        }
    } result opts

    sqlitedb close
    return -options $opts
}

proc ::ral::loadFromSQLite {filename {ns ::}} {
    if {![file exists $filename]} {
        error "no such file, \"$filename\""
    }
    set ns [string trimright $ns :]::
    namespace eval $ns {}
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
            foreach vname $relvarNames {
                set heading [list]
                foreach {aname type} [sqlitedb eval {select Aname, Type from\
                        __ral_attribute where Vname = $vname}] {
                    lappend heading $aname $type
                }
                set idents [dict create]
                foreach {idnum attrName} [sqlitedb eval {select IdNum, Attr\
                        from __ral_identifier where Vname = $vname}] {
                    dict lappend idents $idnum $attrName
                }
                ::ral::relvar create $ns$vname $heading {*}[dict values $idents]
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
                set sqlTableName [mapNamesToSQL $vname]
                set qualName $ns$vname
                set heading [relation heading [relvar set $qualName]]
                set attrNames [relation attributes [relvar set $qualName]]
                set sqlColNames [mapNamesToSQL $attrNames]
                sqlitedb eval "select [join $sqlColNames {, }]\
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
                    ::ral::relvar insert $ns$vname $insert
                }
            }
        }
    } result opts

    sqlitedb close
    return -options $opts
}

proc ::ral::dump {{pattern *}} {
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
            "    ::ral::relvar set [namespace tail  $name] [list [relvar set $name]]" \n
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
    set gotErr [catch {::csv::report printmatrix $m} result]
    $m destroy
    if {$gotErr} {
        error $result
    }
    return $result
}

proc ::ral::csvToFile {relValue fileName {sortAttr {}} {noheading 0}} {
    package require csv

    set m [relation2matrix $relValue $sortAttr $noheading]
    set chan [::open $fileName w]
    set gotErr [catch {::csv::report printmatrix2channel $m $chan} result]
    $m destroy
    ::close $chan
    if {$gotErr} {
        error $result
    }
    return
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
        # Find the attributes of the relationship that participate in
        # a conditional association. This is the one and only case
        # where we will allow the attribute to be NULL.
        set condAttrs [list]
        foreach constraint [relvar constraint member $name] {
            set info [relvar constraint info $constraint]
            set info [lassign $info ctype]
            if {$ctype eq "association"} {
                lassign $info cname rfering a1 c1 refto a2 c2
                if {$c2 eq "?"} {
                    lappend condAttrs {*}$a1
                }
            }
        }
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
            set id [join [mapNamesToSQL $id] {, }]
            append result "    constraint ${sqlBaseName}_ID$idNum\
                    unique ($id),\n"
            append indices "create unique index ${sqlBaseName}_INDEX$idNum\
                    on $sqlBaseName ($id) ;\n"
            incr idNum
        }
        # Define the referential constraints.
        foreach constraint [relvar constraint member $name] {
            set info [relvar constraint info $constraint]
            set info [lassign $info ctype]
            switch -exact -- $ctype {
                association {
                    lassign $info cname rfering a1 c1 refto a2 c2
                    set cname [mapNamesToSQL [namespace tail $cname]]
                    set rfering [namespace tail $rfering]
                    set refto [namespace tail $refto]
                    if {$rfering eq $baseName} {
                        set a1 [join [mapNamesToSQL $a1] {, }]
                        append result "    constraint ${sqlBaseName}_$cname\
                            foreign key ($a1)\
                            references $refto\
                                ([join [mapNamesToSQL $a2] {, }])\n"\
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
                            set subattrs [join [mapNamesToSQL $subattrs] {, }]
                            append result "    constraint ${sqlBaseName}_$cname\
                                foreign key ($subattrs)\
                                references $refto\
                                    ([join [mapNamesToSQL $sattrs] {, }])\n"\
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
                        set ref1Attr [join [mapNamesToSQL $ref1Attr] {, }]
                        append result "    constraint\
                                ${sqlBaseName}_${cname}_${rel1}\
                            foreign key ($ref1Attr)\
                            references $rel1\
                                ([join [mapNamesToSQL $rel1Attr] {, }])\n"\
                            "        on delete cascade on update cascade\
                            deferrable initially deferred,\n"
                        append indices "create index\
                            ${sqlBaseName}_${cname}INDEX0 on $sqlBaseName\
                            ($ref1Attr) ;\n"

                        set rel2 [namespace tail $rel2]
                        set ref2Attr [join [mapNamesToSQL $ref2Attr] {, }]
                        append result "    constraint\
                                ${sqlBaseName}_${cname}_${rel2}\
                            foreign key ($ref2Attr)\
                            references $rel2\
                                ([join [mapNamesToSQL $rel2Attr] {, }])\n"\
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

# If we have Tcl 8.5, then we can supply some "aggregate scalar functions" that
# are useful and have "expr" syntax. If we don't have Tcl 8.5 then we
# will define these in the "::ral" namespace and they will require
# "proc" type syntax.
set sfuncNS [expr {[package vsatisfies [package require Tcl] 8.5] ?\
    "::tcl::mathfunc" : "::ral"}]
# Count the number of tuples in a relation
proc ${sfuncNS}::rcount {relation} {
    return [::ral::relation cardinality $relation]
}
# Count the number of distinct values of an attribute in a relation
proc ${sfuncNS}::rcountd {relation attr} {
    return [::ral::relation cardinality\
        [::ral::relation project $relation $attr]]
}
# Compute the sum over an attribute
proc ${sfuncNS}::rsum {relation attr} {
    set result 0
    foreach v [::ral::relation list $relation $attr] {
        incr result $v
    }
    return $result
}
# Compute the sum over the distinct values of an attribute.
proc ${sfuncNS}::rsumd {relation attr} {
    set result 0
    ::ral::relation foreach v [::ral::relation list\
        [::ral::relation project $relation $attr]] {
        incr result $v
    }
    return $result
}
if {[package vsatisfies [package require Tcl] 8.5]} {
    # Compute the average of the values of an attribute
    proc ${sfuncNS}::ravg {relation attr} {
        return [expr {rsum($relation, $attr) / rcount($relation)}]
    }
    # Compute the average of the distinct values of an attribute
    proc ${sfuncNS}::ravgd {relation attr} {
        return [expr {rsumd($relation, $attr) / rcount($relation)}]
    }
} else {
    # Compute the average of the values of an attribute
    proc ${sfuncNS}::ravg {relation attr} {
        return [expr {[rsum $relation $attr] / [rcount $relation]}]
    }
    # Compute the average of the distinct values of an attribute
    proc ${sfuncNS}::ravgd {relation attr} {
        return [expr {[rsumd $relation $attr] / [rcount $relation]}]
    }
}
# Compute the minimum. N.B. this does not handle "empty" relations properly.
proc ${sfuncNS}::rmin {relation attr} {
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
proc ${sfuncNS}::rmax {relation attr} {
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
                lset cinfo $index\
                        ${ns}[string trimleft [lindex $cinfo $index] :]
            }
        }
        partition {
            lset cinfo 1 ${ns}[string trimleft [lindex $cinfo 1] :]
            lset cinfo 2 ${ns}[string trimleft [lindex $cinfo 2] :]
            for {set index 4} {$index < [llength $cinfo]} {incr index 2} {
                lset cinfo $index\
                        ${ns}[string trimleft [lindex $cinfo $index] :]
            }
        }
        correlation {
            variable correlIndices
            variable compCorrelIndices
            set cIndices [expr {[lindex $cinfo 1] eq "-complete" ?\
                $compCorrelIndices : $correlIndices}]
            foreach index $cIndices {
                lset cinfo $index\
                        ${ns}[string trimleft [lindex $cinfo $index] :]
            }
        }
        procedural {
            set endIndex [expr {[llength $cinfo] - 1}]
            for {set index 1} {$index < $endIndex} {incr index} {
                lset cinfo $index\
                        ${ns}[string trimleft [lindex $cinfo $index] :]
            }
        }
        default {
            error "unknown constraint type, \"[lindex $cinfo 0]\""
        }
    }

    return $cinfo
}

proc ::ral::mapNamesToSQL {names} {
    set newNames [list]
    foreach name $names {
        lappend newNames [regsub -all -- {[^[:alnum:]_]} $name _]
    }
    return $newNames
}

proc ::ral::mapTypeToSQL {type} {
    variable sqlTypeMap
    return [expr {[info exists sqlTypeMap($type)] ?\
            $sqlTypeMap($type) : "text"}]
}

package provide ral 0.11.1
