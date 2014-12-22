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
                    set cname [mapNamesToSQL [namespace tail $cname]]
                    set rfering [namespace tail $rfering]
                    set refto [namespace tail $refto]
                    if {$rfering eq $baseName} {
                        set a1 [join [mapNamesToSQL {*}$a1] {, }]
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

package provide ral 0.11.4
