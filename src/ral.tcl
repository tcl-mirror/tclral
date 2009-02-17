# This software is copyrighted 2004, 2005, 2006 by G. Andrew Mangogna.
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
# 
# $RCSfile: ral.tcl,v $
# $Revision: 1.40.2.4 $
# $Date: 2009/02/17 02:28:11 $
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
    namespace export storeToMk
    namespace export loadFromMk
    namespace export dump
    namespace export dumpToFile
    namespace export csv
    namespace export csvToFile
    if {![package vsatisfies [package require Tcl] 8.5]} {
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
#       {association | partition | correlation <constraint detail>}
#   <association constaint detail> :
#       <association name> <relvar name> {<attr list>} <mult/cond>\
#           <relvar name> {<attr list>} <mult/cond>
#   <partition constraint detail> :
#       <partition name> <supertype> {<attr list>} <subtype1> {<attr list>} ...
#   <correlation constaint detail> :
#       <?-complete?> <correlation name> <correl relvar>
#           {<attr list>} <mult/cond> <relvarA> {<attr list>}
#           {<attr list>} <mult/cond> <relvarB> {<attr list>}
#
#   <list of relvar names/relation values> :
#       {<relvar name> <relation value>}

# Generate a string that encodes all the relvars.

proc ::ral::serialize {{pattern *}} {
    set result [list]

    lappend result Version [getVersion]

    # Convert the names to be relative to the global namespace
    set names [lsort [relvar names $pattern]]
    set relNameList [list]
    foreach name $names {
        lappend relNameList [string trimleft $name ":"]\
            [relation heading [relvar set $name]] [relvar identifiers $name]
    }
    lappend result Relvars $relNameList

    # Constraint information contains fully qualified relvar names
    # and must be converted to be relative to the global namespace.
    set constraints [list]
    foreach cname [lsort [relvar constraint names $pattern]] {
        lappend constraints [getRelativeConstraintInfo $cname]
    }
    lappend result Constraints $constraints

    set bodies [list]
    foreach name $names {
        lappend bodies [list [string trimleft $name ":"] [relvar set $name]]
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
proc ::ral::deserialize {value {ns {}}} {
    if {$ns eq "::"} {
        set ns ""
    }
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
    if {![package vsatisfies [getVersion] $versionNumber]} {
        error "incompatible version number, \"$versionNumber\",\
            current library version is, \"[getVersion]\""
    }

    if {$relvarKeyWord ne "Relvars"} {
        error "expected keyword \"Relvars\", got \"$revarKeyWord\""
    }
    foreach {rvName rvHead rvIds} $relvarDefs {
        set fullName ${ns}::$rvName
        set quals [namespace qualifiers $fullName]
        if {![namespace exists $quals]} {
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
                ::ral::relvar set ${ns}::$relvarName $relvarBody
            }
        }
    }

    return
}

proc ::ral::deserializeFromFile {fileName {ns {}}} {
    set chan [::open $fileName r]
    set gotErr [catch {deserialize [read $chan] $ns} result]
    ::close $chan
    if {$gotErr} {
        error $result
    }
    return
}

proc ::ral::storeToMk {fileName {pattern *}} {
    package require Mk4tcl

    # Back up an existing file
    if {[file exists $fileName]} {
        file rename -force $fileName $fileName~
    }

    ::mk::file open db $fileName
    set err [catch {
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
        # views that will hold the values.  In order to preserve the namespace
        # names for reloading, the view name is constructed from the relvar
        # name by substituting "::" with "__". Metakit doesn't like "::" in
        # view names. To insure that the constructed view names are unique, an
        # integer tag is added (i.e. we need to make sure that two relvars such
        # as ::a::b and ::a__b don't collide).
        set tagCtr 0
        set names [relvar names $pattern]
        foreach name $names {
            set heading [relation heading [relvar set $name]]
            set ids [relvar identifiers $name]
            # Strip the leading namespace separator. This is restored
            # when the relvars are loaded back in.
            set tName [string trimleft $name ":"]
            set viewName [string map {:: __ - _} $tName]_[incr tagCtr]
            set nameViewMap($name) $viewName
            ::mk::row append db.__ral_relvar Name_ral $tName\
                Heading_ral $heading Ids_ral $ids View_ral $viewName
            # Determine the structure of the view that will hold the relvar
            # value.  Special attention is required for Tuple and Relation
            # valued attributes.
            set relvarLayout [list]
            foreach {attr type} $heading {
                lappend relvarLayout [mkHeading $attr $type]
            }
            ::mk::view layout db.$viewName $relvarLayout
        }
        # Get the constraints and put them into the catalog.
        set partIndex 0
        foreach cname [relvar constraint names $pattern] {
            ::mk::row append db.__ral_constraint\
                Constraint_ral [getRelativeConstraintInfo $cname]
        }
        # Populate the views for each relavar.
        foreach name [array names nameViewMap] {
            ::mk::cursor create cursor db.$nameViewMap($name) 0
            relation foreach r [relvar set $name] {
                ::mk::row insert $cursor
                mkStoreTuple $cursor [relation tuple $r]
                ::mk::cursor incr cursor
            }
        }
    } errMsg]

    if {$err} {
        set einfo $::errorInfo
        set ecode $::errorCode
        catch {::mk::file close db}
        return -code $err -errorcode $ecode -errorinfo $einfo $errMsg
    }

    ::mk::file close db
    return
}

proc ::ral::loadFromMk {fileName {ns {}}} {
    package require Mk4tcl

    if {$ns eq "::"} {
        set ns {}
    }

    ::mk::file open db $fileName -readonly
    set err [catch {
        # Check that a "version" view exists and that the information
        # is consistent before we proceed.
        set result [mkCheckVersion db]
        # determine the relvar names and types by reading the catalog
        ::mk::loop rvCursor db.__ral_relvar {
            array set relvarInfo [::mk::get $rvCursor]
            set relvarName ${ns}::$relvarInfo(Name_ral)
            # check that the parent namespace exists
            set parent [namespace qualifiers $relvarName]
            if {![namespace exists $parent]} {
                namespace eval $parent {}
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
                        ${ns}::$relvarInfo(Name_ral)\
                        [mkLoadTuple $vCursor $relvarInfo(Heading_ral)]]
                }
            }
        }
    } errMsg]

    if {$err} {
        set einfo $::errorInfo
        set ecode $::errorCode
        catch {::mk::file close db}
        return -code $err -errorcode $ecode -errorinfo $einfo $errMsg
    }

    ::mk::file close db
    return $result
}

# Merge data from a metakit store of relvars.
# All relvars that are in the file but not currently defined are created.
# All relvars whose names and headings match currently defined relvars
# will have their relation values unioned with those in the file.
proc ::ral::mergeFromMk {fileName {ns {}}} {
    package require Mk4tcl

    if {$ns eq "::"} {
        set ns {}
    }

    ::mk::file open db $fileName -readonly
    set err [catch {
        set result [mkCheckVersion db]
        # determine the relvar names and types by reading the catalog
        ::mk::loop rvCursor db.__ral_relvar {
            array set relvarInfo [::mk::get $rvCursor]
            set relvarName ${ns}::$relvarInfo(Name_ral)
            # Check if the relvar already exists
            if {![relvar exists $relvarName]} {
                # New relvar
                # check that the parent namespace exists
                set parent [namespace qualifiers $relvarName]
                if {![namespace exists $parent]} {
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
        relvar eval {
            ::mk::loop rvCursor db.__ral_relvar {
                array set relvarInfo [::mk::get $rvCursor]
                ::mk::loop vCursor db.$relvarInfo(View) {
                    set value [relation create $relvarInfo(Heading_ral)
                        [mkLoadTuple $vCursor $relvarInfo(Heading_ral)]]
                    ::ral::relvar union ${ns}::$relvarInfo(Name_ral) $value
                }
            }
        }
    } errMsg]

    if {$err} {
        set einfo $::errorInfo
        set ecode $::errorCode
        catch {::mk::file close db}
        return -code $err -errorcode $ecode -errorinfo $einfo $errMsg
    }

    ::mk::file close db
    return $result
}

proc ::ral::dump {{pattern *}} {
    set result {}
    set names [lsort [relvar names $pattern]]

    append result "# Generated via ::ral::dump\n"
    append result "package require ral [getVersion]\n"

    array set qualMap {}
    # Convert the names to be relative
    foreach name $names {
        set rName [string trimleft $name ":"]
        set quals [namespace qualifiers $rName]
        if {![info exists qualMap($quals)]} {
            append result "namespace eval $quals {}\n"
            set qualMap($quals) 1
        }
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
        set rName [string trimleft $name ":"]
        append result\
                "    ::ral::relvar set $rName [list [relvar set $name]]" \n
    }
    append result "\}"

    return $result
}

proc ::ral::dumpToFile {fileName {pattern *}} {
    set chan [::open $fileName w]
    set gotErr [catch {puts $chan [dump $pattern]} result]
    ::close $chan
    if {$gotErr} {
        error $result
    }
    return
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

# Get the contraint info and convert the absolute relvar names
# to relative ones.
proc ::ral::getConstraint {cname} {
    set cinfo [relvar constraint info $cname]
    switch -exact [lindex $cinfo 0] {
        association {
            lset cinfo 1 [namespace tail [lindex $cinfo 1]]
            lset cinfo 2 [namespace tail [lindex $cinfo 2]]
            lset cinfo 5 [namespace tail [lindex $cinfo 5]]
        }
        partition {
            lset cinfo 1 [namespace tail [lindex $cinfo 1]]
            lset cinfo 2 [namespace tail [lindex $cinfo 2]]
            for {set index 4} {$index < [llength $cinfo]} {incr index 2} {
                lset cinfo $index [namespace tail [lindex $cinfo $index]]
            }
        }
        correlation {
            lset cinfo 1 [namespace tail [lindex $cinfo 1]]
            lset cinfo 2 [namespace tail [lindex $cinfo 2]]
            lset cinfo 5 [namespace tail [lindex $cinfo 5]]
            lset cinfo 9 [namespace tail [lindex $cinfo 9]]
        }
        default {
            error "unknown constraint type, \"[lindex $cinfo 0]\""
        }
    }
    return $cinfo
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
    if {![package vsatisfies [getVersion] $verNum]} {
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
                set tupCursor $cursor.$attr!0
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
                lset cinfo $index [string trimleft [lindex $cinfo $index] ":"]
            }
        }
        partition {
            lset cinfo 1 [string trimleft [lindex $cinfo 1] ":"]
            lset cinfo 2 [string trimleft [lindex $cinfo 2] ":"]
            for {set index 4} {$index < [llength $cinfo]} {incr index 2} {
                lset cinfo $index [string trimleft [lindex $cinfo $index] ":"]
            }
        }
        correlation {
            variable correlIndices
            variable compCorrelIndices
            set cIndices [expr {[lindex $cinfo 1] eq "-complete" ?\
                $compCorrelIndices : $correlIndices}]
            foreach index $cIndices {
                lset cinfo $index [string trimleft [lindex $cinfo $index] ":"]
            }
        }
        default {
            error "unknown constraint type, \"[lindex $cinfo 0]\""
        }
    }
    return $cinfo
}

proc ::ral::setRelativeConstraintInfo {ns cinfo} {
    switch -exact [lindex $cinfo 0] {
        association {
            variable assocIndices
            foreach index $assocIndices {
                lset cinfo $index ${ns}::[lindex $cinfo $index]
            }
        }
        partition {
            lset cinfo 1 ${ns}::[lindex $cinfo 1]
            lset cinfo 2 ${ns}::[lindex $cinfo 2]
            for {set index 4} {$index < [llength $cinfo]} {incr index 2} {
                lset cinfo $index ${ns}::[lindex $cinfo $index]
            }
        }
        correlation {
            variable correlIndices
            variable compCorrelIndices
            set cIndices [expr {[lindex $cinfo 1] eq "-complete" ?\
                $compCorrelIndices : $correlIndices}]
            foreach index $cIndices {
                lset cinfo $index ${ns}::[lindex $cinfo $index]
            }
        }
        default {
            error "unknown constraint type, \"[lindex $cinfo 0]\""
        }
    }

    return $cinfo
}

package provide ral 0.9.0
