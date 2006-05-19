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
# $Revision: 1.6 $
# $Date: 2006/05/19 04:54:32 $
#  *--

namespace eval ::ral {
    namespace export tuple2matrix
    namespace export relation2matrix
    namespace export relformat
    namespace export serialize
    namespace export deserialize
    namespace export storeToMk
    namespace export loadFromMk

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
	::report::defstyle ::ral::relationAsTable {{idCols {}} {capRows 2}} {
	    ::ral::tupleAsTable $capRows
	    set sepTemplate [top get]
	    foreach col $idCols {
		lset sepTemplate [expr {2 * $col + 1}] =
	    }
	    top set $sepTemplate
	    bottom set [top get]
	    topcapsep set [top get]
	}
	namespace export relationAsTable
    }

    variable reportCounter 0
}

# Convert a tuple into a matrix.
proc ::ral::tuple2matrix {tupleValue {noheading 0}} {
    package require struct::matrix 2

    set m [::struct::matrix]
    $m add columns [tuple degree $tupleValue]
    set attrReportMap [addHeading $m [tuple heading $tupleValue]]
    addTuple $m $tupleValue $attrReportMap

    return $m
}

# Convert a relation into a matrix.
proc ::ral::relation2matrix {relValue {noheading 0}} {
    package require struct::matrix 2

    set m [::struct::matrix]
    $m add columns [relation degree $relValue]
    set attrReportMap [addHeading $m [relation heading $relValue]]

    relation foreach t $relValue {
	addTuple $m $t $attrReportMap
    }

    return $m
}

# This procedure is intended to be to relations what "parray" is to arrays.
# It provides a very simple text formatting of relation values into a
# tabular layout. Rather than writing the output to a channel "relformat"
# returns the string as its return value.
proc ::ral::relformat {relValue {title {}} {noheading 0}} {
    package require report

    # Determine which columns hold attributes that are part of some identifier
    set attrOffsets [list]
    set offset -1
    foreach {name type} [lindex [relation heading $relValue] 1] {
	lappend attrOffsets $name [incr offset]
    }
    array set attrOffsetMap $attrOffsets
    set idCols [list]
    foreach id [relation identifiers $relValue] {
	foreach idAttr $id {
	    lappend idCols $attrOffsetMap($idAttr)
	}
    }

    variable reportCounter
    set reportName rep[incr reportCounter]
    ::report::report $reportName [relation degree $relValue]\
	style ::ral::relationAsTable $idCols
    set m [relation2matrix $relValue $noheading]
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
	style ::ral::tupleAsTable
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
# List:
#   {Relvars {<list of relvar defs>}}
#   {Constraints {<list of constraints>}
#   {Bodies {<list of relvar bodies>}
#
#   <list of relvar defs> :
#	{<relvar name> <relvar heading>}
#
#   <list of constaints> :
#	{association | partition <constraint detail>}
#   <association constaint detail> :
#	<association name> <relvar name> {<attr list>} <mult/cond>\
#	    <relvar name> {<attr list>} <mult/cond>
#   <partition constraint detail> :
#	<partition name> <supertype> {<attr list>} <subtype1> {<attr list>} ...
#
#   <list of relvar bodies> :
#	{<relvar name> {<tuple value>}}

# Generate a string that encodes all the relvars.
proc ::ral::serialize {{ns {}}} {
    set result [list]
    set names [relvar names ${ns}*]

    # Convert the names to be relative
    set relNameList [list]
    foreach name $names {
	lappend relNameList [namespace tail $name]\
	    [relation heading [relvar set $name]]
    }
    lappend result [list Relvars $relNameList]

    set constraints [list]
    foreach cname [relvar constraint names] {
	lappend constaints [getConstraint $cname]
    }
    lappend result [list Constraints $constraints]

    set bodies [list]
    foreach name $names {
	set body [list]
	relation foreach t [relvar set $name] {
	    lappend body [tupleValue $t]
	}
	lappend bodies [list [namespace tail $name] $body]
    }

    lappend result [list Bodies $bodies]

    return $result
}

# Restore the relvar values from a string.
proc ::ral::deserialize {value {ns {}}} {
    set relvars [lindex $value 0]
    set constraints [lindex $value 1]
    set bodies [lindex $value 2]

    lassign $relvars relvarKeyWord revarDefs
    if {$relvarKeyWord ne "Relvars"} {
	error "expected keyword \"Relvars\", got \"$revarKeyWord\""
    }
    foreach {rvName rvHead} $revarDefs {
	relvar create ${ns}::$rvName $rvHead
    }

    lassign $constraints cnstrKeyWord cnstrDef
    if {$cnstrKeyWord ne "Constraints"} {
	error "expected keyword \"Constraints\", got \"$cnstrKeyWord\""
    }
    foreach constraint $cnstrDef {
	eval relvar $constraint
    }

    lassign $bodies bodyKeyWord bodyDefs
    if {$bodyKeyWord ne "Bodies"} {
	error "expected keyword \"Bodies\", got \"$bodyKeyWord\""
    }

    relvar eval {
	foreach body $bodyDefs {
	    foreach {relvarName relvarBody} $body {
		relvar insert ${ns}::$relvarName {expand}$relvarBody
	    }
	}
    }

    return
}

proc ::ral::storeToMk {fileName} {
    package require Mk4tcl

    # Back up an existing file
    if {[file exists $fileName]} {
	file rename -force $fileName $fileName~
    }

    ::mk::file open db $fileName
    # Create a set of views that are used as catalogs to hold
    # the relvar info that will be needed to reconstruct the values.
    ::mk::view layout db.__ral_relvar {Name Heading}
    set assocLayout {Name RingRelvar RingAttr RingMC RtoRelvar RtoAttr RtoMC}
    ::mk::view layout db.__ral_association $assocLayout
    ::mk::view layout db.__ral_partition\
	{Name SupRelvar SupAttr {SubTypes {SubRelvar SubAttr}}}

    # Get the names of the relvars and insert them into the catalog.
    # Convert the names to be relative before inserting.
    # Also create the views that will hold the values.
    set names [relvar names]
    foreach name $names {
	set heading [relation heading [relvar set $name]]
	::mk::row append db.__ral_relvar Name [namespace tail $name]\
	    Heading $heading
	# Determine the structure of the view that will hold the relvar value.
	# Be careful of Tuple and Relation valued attributes.
	set relvarLayout [list]
	foreach {attr type} [lindex $heading 1] {
	    lappend relvarLayout [mkHeading $attr $type]
	}
	::mk::view layout db.[namespace tail $name] $relvarLayout
    }
    # Get the constraints and put them into the appropriate catalog
    # depending upon the type of the constraint.
    set partIndex 0
    foreach cname [relvar constraint names] {
	set cinfo [getConstraint $cname]
	switch -exact [lindex $cinfo 0] {
	    association {
		set cindex 1
		set value [list]
		foreach col $assocLayout {
		    lappend value $col [lindex $cinfo $cindex]
		    incr cindex
		}
		::mk::row append db.__ral_association {expand}$value
	    }
	    partition {
		::mk::row append db.__ral_partition Name [lindex $cinfo 1]\
		    SupRelvar [lindex $cinfo 2] SupAttr [lindex $cinfo 3]
		foreach {subname subattr} [lrange $cinfo 4 end] {
		    ::mk::row append db.__ral_partition!$partIndex.SubTypes\
			SubRelvar $subname SubAttr $subattr
		}
		incr partIndex
	    }
	    default {
		error "unknown constraint type, \"[lindex $cinfo 0]\""
	    }
	}
    }

    # Populate the views for each relavar.
    foreach name $names {
	set simpleName [namespace tail $name]
	::mk::cursor create cursor db.$simpleName 0
	relation foreach t [relvar set $name] {
	    ::mk::row insert $cursor
	    mkStoreTuple $cursor $t
	    ::mk::cursor incr cursor
	}
    }

    ::mk::file close db
}

proc ::ral::loadFromMk {fileName {ns {}}} {
    package require Mk4tcl

    ::mk::file open db $fileName -readonly
    # determine the relvar names and types by reading the catalog
    ::mk::loop rvCursor db.__ral_relvar {
	set relvarInfo [::mk::get $rvCursor]
	relvar create [dict get $relvarInfo Name] [dict get $relvarInfo Heading]
    }

    # create the associations
    ::mk::loop assocCursor db.__ral_association {
	set assocCmd [list relvar association]
	foreach {col value} [::mk::get $assocCursor] {
	    lappend assocCmd $value
	}
	eval $assocCmd
    }

    # create the partitions
    ::mk::loop partCursor db.__ral_partition {
	set subtypes [list]
	::mk::loop subCursor $partCursor.SubTypes {
	    foreach {col value} [::mk::get $subCursor] {
		lappend subtypes $value
	    }
	}
	set partValues [::mk::get $partCursor]
	relvar partition [dict get $partValues Name]\
	    [dict get $partValues SupRelvar] {expand}$subtypes
    }

    # fetch the relation values from the views
    relvar eval {
	foreach name [relvar names ${ns}*] {
	    set viewName [namespace tail $name]
	    set heading [relation heading [relvar set $name]]
	    ::mk::loop vCursor db.$viewName {
		relvar insert $name [mkLoadTuple $vCursor $heading]
	    }
	}
    }

    ::mk::file close db
}

# PRIVATE PROCS

# Add heading rows to the matrix.
# Returns a dictionary mapping attribute names to a formatting function.
# Ordinary scalar values attributes are not contained in the mapping. Relation
# and tuple valued attributes will be in the dictionary keyed by the attribute
# name with values corresponding to the "relformat" or "tupleformat" command.
proc ::ral::addHeading {matrix heading} {
    set attrNames [list]
    set attrTypes [list]
    set attrReportMap [dict create]
    foreach {name type} [lindex $heading 1] {
	lappend attrNames $name
	set typeKey [lindex $type 0]
	if {$typeKey eq "Tuple"} {
	    dict set attrReportMap $name ::ral::tupleformat
	    lappend attrTypes $typeKey
	} elseif {$typeKey eq "Relation"} {
	    dict set attrReportMap $name ::ral::relformat
	    lappend attrTypes $typeKey
	} else {
	    lappend attrTypes $type
	}
    }
    $matrix add row $attrNames
    $matrix add row $attrTypes

    return $attrReportMap
}

# Add tuple values to a matrix
proc ::ral::addTuple {matrix tupleValue attrMap} {
    set values [list]
    foreach {attr value} [tuple get $tupleValue] {
	if {[dict exists $attrMap $attr]} {
	    set value [[dict get $attrMap $attr] $value]
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
	    lset cinfo 2 [namespace tail [lindex $cinfo 2]]
	    lset cinfo 5 [namespace tail [lindex $cinfo 5]]
	}
	partition {
	    lset cinfo 2 [namespace tail [lindex $cinfo 2]]
	    set endIndex [expr {[llength $cinfo] - 4}]
	    for {set index 4} {$index < $endIndex} {incr index 2} {
		lset cinfo $index [namespace tail [lindex $cinfo $index]]
	    }
	}
	default {
	    error "unknown constraint type, \"[lindex $cinfo 0]\""
	}
    }
    return $cinfo
}

proc ::ral::tupleValue {tuple} {
    set result [list]
    foreach {attr type} [lindex [tuple heading $tuple] 1]\
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
    relation foreach t $relation {
	lappend result [tupleValue $t]
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
    foreach {attr type} [lindex [tuple heading $tuple] 1]\
	{attr value} [tuple get $tuple] {
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
    relation foreach t $relation {
	mkStoreTuple $relCursor $t
	::mk::cursor incr relCursor
    }
    return
}

proc ::ral::mkLoadTuple {cursor heading} {
    set value [list]
    foreach {attr type} [lindex $heading 1] {
	switch -exact [lindex $type 0] {
	    Tuple {
		lappend value $attr [mkLoadTuple $cursor.$attr!0 $type]
	    }
	    Relation {
		lappend value $attr [mkLoadRelation $cursor.$attr $type]
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
    lset heading 0 Tuple
    ::mk::loop rCursor $cursor {
	lappend value [mkLoadTuple $rCursor $heading]
    }
    return $value
}
