# This software is copyrighted 2007 by G. Andrew Mangogna.
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
# raloo.tcl -- Relation Oriented Programming
# 
# ABSTRACT:
# 
# $RCSfile: raloo.tcl,v $
# $Revision: 1.2 $
# $Date: 2007/03/12 01:19:37 $
#  *--

package provide raloo 0.1

package require Tcl 8.5
package require ral 0.8.2
package require ralutil 0.8.2

namespace eval ::raloo {
    namespace export Domain
    namespace export RelvarClass
    namespace export Generalization
    namespace export Relationship
    namespace export AssocRelationship

    namespace import ::ral::*
    ::ralutil::sysIdsInit
}

# This class encapsulates the basic relvar operations
oo::class create ::raloo::RelvarOps {
    method insert {idValList} {
	relvar insert [self] $idValList
    }
    method delete {idValList} {
	foreach attrValSet $idValList {
	    relvar deleteone [self] {expand}$attrValSet
	}
    }
    method update {idValList attrName value} {
	foreach attrValSet $idValList {
	    relvar updateone [self] t $attrValSet {
		tuple update t $attrName $value
	    }
	}
    }
    method set {args} {
	relvar set [self] {expand}$args
    }
    method cardinality {} {
	relation cardinality [my set]
    }
    method degree {} {
	relation degree [my set]
    }
    method format {args} {
	relformat [my set] [self] {expand}$args
    }
}

# This class is the primary base class for all relvar references.
# Instances of this class refer to a relvar, but have no state behavior.
oo::class create ::raloo::PassiveRef {
    constructor {args} {
	namespace import ::ral::*

	my variable ref
	my variable relvarName
	set ref [relation emptyof [$relvarName set]]
	if {[llength $args] != 0} {
	    # relvar insert returns just what was inserted.
	    set ref [$relvarName insert $args]
	}
	set ref [relation project $ref\
	    {expand}[lindex [relation identifiers $ref] 0]]
    }
    method selectOne {args} {
	my variable relvarName
	my variable ref
	my toReference [::ralutil::pipe {
	    $relvarName set | relation choose ~ {expand}$args
	}]
    }
    method selectWhere {expr} {
	my variable relvarName
	my toReference [::ralutil::pipe {
	    $relvarName set | relation restrictwith ~ $expr
	}]
    }
    method selectAny {expr} {
	my variable relvarName
	my toReference [::ralutil::pipe {
	    $relvarName set |
	    relation restrictwith ~ $expr |
	    relation tag ~ __Order__ |
	    relation choose ~ __Order__ 0
	}]
    }
    method selectRelated {args} {
	set r [my __DeReference]
	my variable relvarName
	set dstRelvar $relvarName
	set sjCmd [list relation semijoin $r]
	foreach rship $args {
	    if {![regexp {\A(~)?([^>]+)(>.*)?\Z} $rship\
		    match dirMark rName endMark]} {
		error "bad relationship syntax, \"$rship\""
	    }
	    set cMark [string index $endMark 0]
	    set endMark [string range $endMark 1 end]
	    #puts "$dirMark $rName $cMark $endMark"
	    set rInfo [relvar constraint info $rName]
	    #puts $rInfo
	    set ctype [lindex $rInfo 0]
	    switch -exact -- $dirMark$ctype$cMark {
		association {
		    # Traversal in the forward direction
		    set srcRelvar [lindex $rInfo 2]
		    set targetRelvar [lindex $rInfo 5]
		    set jAttrs [raloo::PassiveRef mergeAttrs [lindex $rInfo 3]\
			[lindex $rInfo 6]]
		}
		
		~association {
		    set srcRelvar [lindex $rInfo 5]
		    set targetRelvar [lindex $rInfo 2]
		    set jAttrs [raloo::PassiveRef mergeAttrs [lindex $rInfo 6]\
			[lindex $rInfo 3]]
		}
		correlation {
		    set srcRelvar [lindex $rInfo 5]
		    set jAttr1 [raloo::PassiveRef mergeAttrs\
			[lindex $rInfo 6] [lindex $rInfo 3]]
		    set targetRelvar [lindex $rInfo 9]
		    set jAttr2 [raloo::PassiveRef mergeAttrs\
			[lindex $rInfo 7] [lindex $rInfo 10]]
		}
		correlation> {
		    set srcRelvar [lindex $rInfo 5]
		    set targetRelvar [lindex $rInfo 2]
		    set jAttrs [raloo::PassiveRef mergeAttrs\
			[lindex $rInfo 6] [lindex $rInfo 3]]
		}
		~correlation {
		    set srcRelvar [lindex $rInfo 9]
		    set jAttr1 [raloo::PassiveRef mergeAttrs\
			[lindex $rInfo 10] [lindex $rInfo 7]]
		    set targetRelvar [lindex $rInfo 5]
		    set jAttr2 [raloo::PassiveRef mergeAttrs\
			[lindex $rInfo 3] [lindex $rInfo 6]]
		}
		~correlation> {
		    set srcRelvar [lindex $rInfo 9]
		    set targetRelvar [lindex $rInfo 2]
		    set jAttrs [raloo::PassiveRef mergeAttrs\
			[lindex $rInfo 10] [lindex $rInfo 7]]
		}
		partition {
		    lassign [raloo::PassiveRef findSubType $dstRelvar $rInfo]\
			srcRelvar srcAttrs
		    set targetRelvar [lindex $rInfo 2]
		    set jAttrs [raloo::PassiveRef mergeAttrs $srcAttrs\
			[lindex $rInfo 3]]
		}
		~partition> {
		    set srcRelvar [lindex $rInfo 2]
		    lassign [raloo::PassiveRef findSubType\
			[namespace eval [my Domain] relvar path $endMark]\
			$rInfo] targetRelvar targetAttrs
		    set jAttrs [raloo::PassiveRef mergeAttrs [lindex $rInfo 3]\
			$targetAttrs]
		}
		default {
		    error "illegal relationship traversal, \"$rship\""
		}
	    }
	    if {$dstRelvar ne $srcRelvar} {
		error "traversal of $rship begins at $srcRelvar,\
		    not $dstRelvar"
	    }
	    if {$ctype eq "correlation" && $cMark eq ""} {
		# Correlations that don't stop at the associative class
		# require 2 semijoins. First to the associative class then
		# to the target class.
		lappend sjCmd\
		    [relvar set [lindex $rInfo 2]] -using $jAttr1\
		    [relvar set $targetRelvar] -using $jAttr2
	    } else {
		lappend sjCmd [relvar set $targetRelvar] -using $jAttrs
	    }
	    set dstRelvar $targetRelvar
	}
	set selected [$dstRelvar new]
	#puts $sjCmd
	$selected toReference [eval $sjCmd]
	return $selected
    }
    method delete {} {
	my variable relvarName
	my variable ref
	$relvarName delete [relation body $ref]
	set ref [relation emptyof $ref]
    }
    method cardinality {} {
	my variable ref
	relation cardinality $ref
    }
    method format {args} {
	my variable ref
	relformat [my __DeReference] {expand}$args
    }
    # Unexported methods.
    #
    # Find the tuples in the base relvar that this reference actually
    # refers to.
    method __DeReference {} {
	my variable relvarName
	my variable ref
	relation semijoin $ref [$relvarName set]
    }
    unexport __DeReference
    # Convert a relation value of the base relvar into a reference.
    method toReference {relValue} {
	my variable relvarName
	if {[relation isnotempty $relValue] && [relation isempty\
		[relation intersect [relvar set $relvarName] $relValue]]} {
	    error "relation value is not contained in $relvarName"
	}
	my variable ref
	set ref [relation project $relValue {expand}[relation attributes $ref]]
    }
    # Obtain a dictionary of the reference. This can be used to
    # reconstruct the reference later. This is only valid for singleton
    # references.
    method __Value {} {
	my variable ref
	return [tuple get [relation tuple $ref]]
    }
    unexport __Value
    # Update an attribute in the base relvar to which this reference refers.
    method __UpdateAttr {attrName value} {
	my variable relvarName
	my variable ref
	$relvarName update [relation body $ref] $attrName $value
    }
    unexport __UpdateAttr


    self.method mergeAttrs {alist1 alist2} {
	set result [list]
	foreach a1 $alist1 a2 $alist2 {
	    lappend result $a1 $a2
	}
	return $result
    }
    self.method findSubType {relvar rinfo} {
	foreach {rname attrList} [lrange $rinfo 4 end] {
	    if {$relvar eq $rname} {
		return [list $rname $attrList]
	    }
	}
	error "$relvar is not a subtype of \"$rinfo\""
    }
}

# This is the primary base class for all references that have associated
# state behavior.
oo::class create ::raloo::ActiveRef {
    superclass ::raloo::PassiveRef
    constructor {args} {
	namespace import ::ral::*

	# Check if we are creating a new tuple. If so, we have to add in
	# the current state attribute. We allow the current state attribute
	# to be specified at creation time so that an instance can be
	# created in an arbitrary state. This is the only time when
	# the current state can be directly manipulated.
	if {[llength $args]} {
	    set csIndex [lsearch -exact -regexp $args\
		{(?i)(?:__)?currentstate(?:__)?}]
	    if {$csIndex == -1} {
		my variable relvarName
		lappend args __currentstate__\
		    [[my Domain] defaultInitialState $relvarName]
	    } else {
		# Make sure the exact form of the "current state" attribute
		# is as the rest of the architecture expects it.
		lset args $csIndex __currentstate__
	    }
	}
	next {expand}$args
    }
    method signal {event args} {
	my variable ref
	my variable relvarName
	# Determine if this is a self-directed event or not.
	if {[catch {self caller} caller]} {
	    set queue queueNonSelf
	} else {
	    set queue [expr {[lindex $caller 1] eq [self] ?\
		    "queueSelf" : "queueNonSelf"}]
	}
	# This reference may be multi-valued. Generate an event to each one.
	relation foreach r $ref {
	    [my Domain] $queue $relvarName\
		[tuple get [relation tuple $r]] $event $args
	}
    }
    method deliver {event args} {
	my variable relvarName
	set currentState [relation extract [my __DeReference] __currentstate__]
	set newState [[my Domain] getTransition\
	    $relvarName $currentState $event]
	if {$newState ne "__ignore__"} {
	    if {$newState eq "__canthappen__"} {
		error "can't happen transition:\
		    $currentState -- $event --> [string trim $newState _]"
	    } else {
		# update the current state to the new state
		my __UpdateAttr __currentstate__ $newState
		# Execute the state action. This must be done in a transaction.
		# State actions must leave the relvar state consistent.
		relvar eval {
		    my $newState {expand}$args
		}
		# if we enter a final state, then delete the instance.
		if {[[my Domain] isFinalState $relvarName $newState]} {
		    my delete
		}
	    }
	}
    }
}

# This is a meta-class that constructs classes that correspond to relvars.
oo::class create ::raloo::RelvarClass {
    superclass oo::class
    constructor {definition} {
	puts "RelvarClass: [self]"
	namespace import ::ral::*

	my variable domain
	set domain [uplevel namespace current]
	namespace eval $domain namespace export [namespace tail [self]]

	set currPath [my eval namespace path]
	lappend currPath $domain
	my eval namespace path [list $currPath]
	#puts "[self namespace]: $currPath"

	set me [self object]
	set myClass [self class]
	oo::define $me {
	    self.mixin ::raloo::RelvarOps

	    constructor {args} {
		my variable relvarName
		set relvarName [self class]
		eval next $args
	    }
	}
	oo::define $me method Domain {} [list return $domain]
	# Set up variables that are used by the methods that are
	# invoked when evaluating the definition.
	my variable attributes
	set attributes [relation create {
		AttrName string
		AttrType string
	    } AttrName]
	my variable ids
	set ids [relation create {
		IdNum int
		AttrName string
	    } {
		{IdNum AttrName}
	    }]
	my variable uniqueAttrList
	set uniqueAttrList [list]
	my variable activeClass
	set activeClass {Class {} DefaultInitialState {} DefaultTransition {}}
	my variable finalState
	set finalState [relation create {
		Class string
		State string
	    } {
		{Class State}
	    }]
	my variable transition
	set transition [relation create {
		Class string
		State string
		Event string
		NewState string
	    } {
		{Class State Event}
	    }
	]

	my variable relvarName
	set relvarName $me

	# Evaluate the definition. The definition make calls to the
	# meta-class methods to specify the details of the newly minted class.
	my eval $definition

	#puts [relformat $attributes Attributes]
	#puts [relformat $ids Ids]
	#puts $activeClass
	#puts [relformat $finalState FinalState]
	#puts [relformat $transition Transition]

	# Check if any state behavior was specified in the definition.
	# If so, then the new class needs to be Active
	if {[dict get $activeClass Class] ne ""} {
	    set attributes [relation include $attributes\
		{AttrName __currentstate__ AttrType string}]
	    oo::define [self] self.method signal {ref event args} {
		my variable domain
		$domain queueCreation [self] $ref $event $args
	    }
	    set refClass ::raloo::ActiveRef
	} else {
	    set refClass ::raloo::PassiveRef
	}
	oo::define $me superclass $refClass

	# If the evaluation of the definition yielded up any attribute
	# definitions, then we will create the backing relvar. If not, then we
	# assume the relvar already exists and was created by other means.  So
	# we also remember if we created the relvar and unset it in the
	# destructor if we created it.
	my variable unsetOnDestroy
	if {[relation isnotempty $attributes]} {
	    set unsetOnDestroy 1
	    # Generate the relvar from the attributes specified in the
	    # defintion.
	    set idList [list]
	    relation foreach idrel [relation summarize $ids\
		    [relation project $ids IdNum] r\
		    AttrList list {[relation list $r AttrName]}]\
		    -ascending IdNum {
		lappend idList [relation extract $idrel AttrList]
	    }
	    if {[llength $idList] == 0} {
		error "all relvar classes must have at least one identifer"
	    }
	    relvar create $relvarName [list Relation\
		    [relation dict $attributes] $idList]

	    # Put on relvar traces for any unique attributes.
	    foreach attr $uniqueAttrList {
		::ralutil::sysIdsGenSystemId $me $attr
	    }
	} else {
	    set unsetOnDestroy 0
	    # Otherwise use relvar introspection to construct the attribute and
	    # id relation values to be as if they were declared. These are then
	    # fed into the next section of code to construct the methods for
	    # the class.
	    foreach {attrName attrType}\
		    [lindex [relation heading [relvar set $me]] 1] {
		set attributes [relation include $attributes [list\
		    AttrName $attrName\
		    AttrType $attrType]]
	    }
	    set idNum 0
	    foreach identifier [relation identifiers [relvar set $me]] {
		foreach idAttr $identifier {
		    set ids [relation include $ids\
			[list IdNum $idNum AttrName $idAttr]]
		}
		incr idNum
	    }
	}

	# Define a method for each attribute. If the attribute is an
	# identifier, then updates are not allowed and the method
	# interface will accept no additional argument.
	# Make sure that there is no "currentstate" method. currentstate
	# is strictly controlled internally.
	set attributes [relation restrictwith $attributes\
	    {$AttrName ne "__currentstate__"}]
	relation foreach attr [relation semiminus $ids $attributes] {
	    relation assign $attr
	    oo::define $me method $AttrName {{val {}}} [format {
		if {$val ne ""} {
		    my __UpdateAttr %s $val
		}
		relation extract [my __DeReference] %s
	    } $AttrName $AttrName]
	    oo::define [self] export $AttrName
	}
	relation foreach attr [relation semijoin $ids $attributes] {
	    relation assign $attr
	    oo::define $me method $AttrName {} [format {
		relation extract [my __DeReference] %s
	    } $AttrName]
	    oo::define [self] export $AttrName
	}

	# Add the state information to the architecture data.
	$domain addActiveClassInfo $activeClass $finalState $transition

	unset attributes ids uniqueAttrList activeClass finalState transition

	#puts [self]
	#puts [relvar names [self]]
    }
    destructor {
	my variable unsetOnDestroy
	if {$unsetOnDestroy} {
	    relvar unset [self]
	}
    }
    # Declare one or more attributes.
    # "nameTypeList" is a list of alternating attribute name / attribute type
    # values.
    method Attribute {nameTypeList} {
	if {[llength $nameTypeList] % 2 != 0} {
	    error "bad list of pairs: \"$nameTypeList\""
	}
	my variable attributes
	my variable ids
	foreach {name type} $nameTypeList {
	    if {[regexp {(\*([1-9])?)?(.+)} $name\
		    match idMark idNum attrName]} {
		if {$idMark ne ""} {
		    if {$idNum eq ""} {
			set idNum 0
		    }
		    set ids [relation include $ids [list\
			IdNum $idNum\
			AttrName $attrName]]
		}
		if {[string match -nocase unique* $type]} {
		    my variable uniqueAttrList
		    lappend uniqueAttrList $attrName
		    set type int
		}
		set attributes [relation include $attributes [list\
		    AttrName $attrName\
		    AttrType $type]]
	    } else {
		error "unrecognized attribute name syntax, \"$name\""
	    }
	}
    }
    method State {name argList body} {
	oo::define [self] method $name $argList $body
	oo::define [self] unexport $name
	my variable activeClass
	if {[dict get $activeClass Class] eq ""} {
	    dict set activeClass Class [self]
	    dict set activeClass DefaultInitialState $name
	    dict set activeClass DefaultTransition __canthappen__
	}
    }
    method Transition {currState event newState} {
	if {$currState eq "<"} {
	    set currState __created__
	}
	if {$newState eq "ignore"} {
	    set newState __ignore__
	} elseif {$newState eq "canthappen"} {
	    set newState __canthappen__
	} elseif {[regexp {(>)?(.+)} $newState match endMark stateName]} {
	    if {$endMark ne ""} {
		set newState $stateName
		my variable finalState
		set finalState [relation include $finalState [list\
		    Class [self]\
		    State $newState]]
	    }
	} else {
	    error "unrecognized new state name, \"$newState\""
	}
	my variable transition
	set transition [relation include $transition [list\
	    Class [self]\
	    State $currState\
	    Event $event\
	    NewState $newState]]
    }
    method DefaultInitialState {{defState {}}} {
	my variable activeClass
	if {$defState ne ""} {
	    my variable transition
	    if {[relation isempty [relation choose\
		[relation project $transition State] State $defState]]} {
		error "unknown state, \"$defState"
	    }
	    dict set activeClass DefaultInitialState $defState
	}
	return [dict get $activeClass DefaultInitialState]
    }
    method DefaultTransition {{trans {}}} {
	my variable activeClass
	if {$trans ne ""} {
	    if {[string equal -nocase $trans "ignore"] ||\
		[string equal -nocase $trans "canthappen"]} {
		my variable activeClass
		dict set activeClass DefaultTransition __${trans}__
	    } else {
		error "bad default transition name, \"$trans\""
	    }
	}
	return [dict get $activeClass DefaultTransition]
    }
    method InstOp {name argList body} {
	oo::define [self] method $name $argList $body
	oo::define [self] export $name
    }
    method ClassOp {name argList body} {
	oo::define [self] self.method $name $argList $body
	oo::define [self] export $name
    }
}

# This is a class that constructs relationships that correspond to
# relvar constraints.
oo::class create ::raloo::Association {
    unexport create
    unexport new
    method info {} {
	relvar constraint info [self]
    }
    destructor {
	relvar constraint delete [self]
    }
}

oo::class create ::raloo::Relationship {
    superclass ::raloo::Association
    constructor {definition} {
	namespace import ::ral::*

	my eval $definition

	my variable rshipInfo
	if {![info exists rshipInfo]} {
	    error "no relationship specification"
	}

	# By default if no formalization is given, then we assume the
	# attribute names are the same.
	if {![dict exists $rshipInfo refrngAttrs]} {
	    set refToId [lindex [relation identifiers\
		[relvar set [dict get $rshipInfo refToRelvar]]] 0]
	    dict set rshipInfo refrngAttrs $refToId
	    dict set rshipInfo refToAttrs $refToId
	}
	# construct and eval the "relvar association" command.
	relvar association [self]\
	    [dict get $rshipInfo refrngRelvar]\
	    [dict get $rshipInfo refrngAttrs]\
	    [dict get $rshipInfo refrngSpec]\
	    [dict get $rshipInfo refToRelvar]\
	    [dict get $rshipInfo refToAttrs]\
	    [dict get $rshipInfo refToSpec]

	unset rshipInfo
    }
    destructor {
	next
    }
    # name X-->X name
    method Relate {formClass spec partClass} {
	if {![regexp -- {([1?*+])-+>([1?*+])} $spec\
		match refrngSpec refToSpec]} {
	    error "unrecognized relationship specification, \"$spec\""
	}
	my variable rshipInfo
	dict set rshipInfo refrngRelvar [relvar path $formClass]
	dict set rshipInfo refrngSpec $refrngSpec
	dict set rshipInfo refToRelvar [relvar path $partClass]
	dict set rshipInfo refToSpec $refToSpec
    }
    method Formalize {args} {
	if {[llength $args] % 2 != 0} {
	    error "formalizing attributes must be in pairs, \"$args\""
	}
	my variable rshipInfo
	foreach {ref refTo} $args {
	    dict lappend rshipInfo refrngAttrs $ref
	    dict lappend rshipInfo refToAttrs $refTo
	}
    }
}

oo::class create ::raloo::AssocRelationship {
    superclass ::raloo::Association
    constructor {definition} {
	namespace import ::ral::*

	my eval $definition

	my variable rshipInfo
	if {![info exists rshipInfo]} {
	    error "no relationship specification"
	}

	# By default if no formalization is given, then we assume the
	# names of the referring attributes are the same as the attributes
	# in Id #0
	if {![dict exist $rshipInfo oneRefrngAttrs]} {
	    set oneIdAttrs [lindex [relation identifiers\
		[relvar set [dict get $rshipInfo oneRelvar]]] 0]
	    dict set rshipInfo oneRefrngAttrs $oneIdAttrs
	    dict set rshipInfo oneRefToAttrs $oneIdAttrs
	}
	if {![dict exist $rshipInfo otherRefrngAttrs]} {
	    set otherIdAttrs [lindex [relation identifiers\
		[relvar set [dict get $rshipInfo otherRelvar]]] 0]
	    dict set rshipInfo otherRefrngAttrs $otherIdAttrs
	    dict set rshipInfo otherRefToAttrs $otherIdAttrs
	}
	# construct and eval the "relvar association" command.
	relvar correlation [self]\
	    [dict get $rshipInfo assocRelvar]\
	    [dict get $rshipInfo oneRefrngAttrs]\
	    [dict get $rshipInfo oneSpec]\
	    [dict get $rshipInfo oneRelvar]\
	    [dict get $rshipInfo oneRefToAttrs]\
	    [dict get $rshipInfo otherRefrngAttrs]\
	    [dict get $rshipInfo otherSpec]\
	    [dict get $rshipInfo otherRelvar]\
	    [dict get $rshipInfo otherRefToAttrs]

	unset rshipInfo
    }
    destructor {
	next
    }
    # name X-->X name by name
    method Relate {oneClass spec otherClass by assocClass} {
	# N.B. the reversal of "one" and "other" here
	if {![regexp -- {([1?*+])-+>([1?*+])} $spec\
		match otherSpec oneSpec]} {
	    error "unrecognized relationship specification, \"$spec\""
	}
	my variable rshipInfo
	dict set rshipInfo assocRelvar [relvar path $assocClass]
	dict set rshipInfo oneRelvar [relvar path $oneClass]
	dict set rshipInfo oneSpec $oneSpec
	dict set rshipInfo otherRelvar [relvar path $otherClass]
	dict set rshipInfo otherSpec $otherSpec
    }
    method ForwardReference {args} {
	if {[llength $args] % 2 != 0} {
	    error "formalizing attributes must be in pairs, \"$args\""
	}
	my variable rshipInfo
	foreach {ref refTo} $args {
	    dict lappend rshipInfo oneRefrngAttrs $ref
	    dict lappend rshipInfo oneRefToAttrs $refTo
	}
    }
    method BackwardReference {args} {
	if {[llength $args] % 2 != 0} {
	    error "formalizing attributes must be in pairs, \"$args\""
	}
	my variable rshipInfo
	foreach {ref refTo} $args {
	    dict lappend rshipInfo otherRefrngAttrs $ref
	    dict lappend rshipInfo otherRefToAttrs $refTo
	}
    }
}

oo::class create ::raloo::Generalization {
    self.unexport new
    superclass ::raloo::Association
    constructor {definition} {
	namespace import ::ral::*

	my variable domain
	set domain [uplevel namespace current]
	namespace eval $domain namespace export [namespace tail [self]]

	my variable refToRelvar
	my variable refToAttrs
	relvar create subType {
	    Relation {
		RelvarName string
		RefngAttrs list
	    } {
		RelvarName
	    }
	}

	my eval $definition

	if {![info exists refToRelvar]} {
	    error "no super type class was defined"
	}
	if {[relation isempty [relvar set subType]]} {
	    errof "no sub type classes were defined"
	}
	set constrCmd [list relvar partition [self] $refToRelvar]
	lappend constrCmd $refToAttrs
	relation foreach st [relvar set subType] {
	    relation assign $st
	    if {$RefngAttrs eq ""} {
		set RefngAttrs [lindex [relation identifiers\
		    [relvar set $RelvarName]] 0]
	    }
	    lappend constrCmd $RelvarName $RefngAttrs
	}

	#puts $constrCmd
	eval $constrCmd

	unset refToRelvar refToAttrs
	relvar unset subType
    }
    destructor {
	next
    }
    method SuperType {superRel {id 0}} {
	my variable refToRelvar
	if {[info exists refToRelvar]} {
	    error "super type relvar has already been defined as,\
		\"$refToRelvar\""
	}
	my variable domain
	set refToRelvar [namespace eval $domain relvar path $superRel]
	set superIds [relation identifiers [relvar set $refToRelvar]]
	if {$id >= [llength $superIds]} {
	    error "requested identifier, \"$id\", but $refToRelvar has only\
		[llength $superIds] identifiers"
	}
	my variable refToAttrs
	set refToAttrs [lsort [lindex $superIds $id]]
    }
    method SubTypes {args} {
	my variable subType
	my variable domain
	foreach st $args {
	    relvar insert subType [list\
		RelvarName [namespace eval $domain relvar path $st]\
		RefngAttrs {}\
	    ]
	}
    }
    method ReferenceById {subtype idNum} {
	set subIdSet [relation identifiers [relvar set $subtype]]
	if {[llength $subIdSet] <= $idNum} {
	    error "requested reference by identifier, \"$idNum\", but
		$subtype only has [llength $subIdSet] identifiers"
	}
	my Reference $subtype [lsort [lindex $subIdSet $idNum]]
    }
    method Reference {subtype attrList} {
	my variable domain
	relvar updateone subType st\
		[list RelvarName [namespace eval $domain relvar path $subtype] {
	    tuple update st RefngAttrs $attrList
	}
    }
}

# The event queue dispatch is also done via object oriented relvar
# backed storage.
oo::class create ::raloo::EventQueue {
    constructor {} {
	namespace import ::ral::*
	relvar create [self] {
	    Relation {
		Id int
		RelvarName string
		Reference list
		EventType string
		Event string
		Params list
	    } {
		Id
	    }
	}
	::ralutil::sysIdsGenSystemId [relvar path [self]] Id
    }
    method putNormal {rname ref event params} {
	relvar insert [self] [list Id {} RelvarName $rname\
	    Reference $ref Event $event EventType normal Params $params]
    }
    method putCreation {rname ref event params} {
	relvar insert [self] [list Id {} RelvarName $rname\
	    Reference $ref Event $event EventType create Params $params]
    }
    method get {} {
       set rel [::ralutil::pipe {
	    relvar set [self] |
	    relation tag ~ -ascending Id __Order__ |
	    relation choose ~ __Order__ 0
       }]
       relvar deleteone [self] Id [relation extract $rel Id]
       ::ralutil::pipe {
	    relation eliminate $rel Id __Order__ |
	    relation tuple |
	    tuple get
       }
    }
    method size {} {
	relation cardinality [relvar set [self]]
    }
    method clear {} {
	set me [self]
	relvar set $me [relation emptyof [relvar set $me]]
    }
}

# HERE
# Make a "Domain" class and allow domain functions to be defined.
# They then implement the thread of control.
# Domain Methods:
#	Function -- define domain function
#	DataTransaction -- mask "relvar eval"
#	class and relationship definitions
#	all architecture stuff, including unique id stuff
#	Delayed events stuff ???
oo::class create ::raloo::Domain {
    # We want all domains to be given a name
    self.unexport new
    constructor {definition} {
	namespace eval [self] namespace import ::ral::*

	my variable selfEventQueue
	set selfEventQueue [raloo::EventQueue new]
	my variable nonSelfEventQueue
	set nonSelfEventQueue [raloo::EventQueue new]

	my eval {
	    namespace import ::ral::*
	    relvar create ActiveClass {
		Relation {
		    Class string
		    DefaultInitialState string
		    DefaultTransition string
		} {
		    Class
		}
	    }
	    relvar create FinalState {
		Relation {
		    Class string
		    State string
		} {
		    {Class State}
		}
	    }
	    relvar create Transition {
		Relation {
		    Class string
		    State string
		    Event string
		    NewState string
		} {
		    {Class State Event}
		}
	    }

	    relvar association R1\
		Transition Class *\
		ActiveClass Class 1
	    relvar association R2\
		Transition {Class NewState} 1\
		FinalState {Class State} ?
	}

	my eval $definition
    }

    destructor {
	my variable selfEventQueue
	$selfEventQueue destroy
	my variable nonSelfEventQueue
	$nonSelfEventQueue destroy
    }

    method Function {name argList body} {
	oo::define [self] method $name $argList [list my control $body]
	oo::define [self] export $name
    }

    method Class {name definition} {
	return [namespace eval [self] [list raloo::RelvarClass create\
	    $name $definition]]
    }

    method Generalization {name definition} {
	return [namespace eval [self] [list raloo::Generalization create\
	    $name $definition]]
    }

    method queueNonSelf {class ref event args} {
	my variable nonSelfEventQueue
	$nonSelfEventQueue putNormal $class $ref $event $args
    }

    method queueSelf {class ref event args} {
	my variable selfEventQueue
	$selfEventQueue putNormal $class $ref $event $args
    }

    method queueCreation {class ref event args} {
	my variable nonSelfEventQueue
	$nonSelfEventQueue putCreation $class $ref $event $args
    }

    method control {script} {
	relvar eval {
	    set ecode [catch {uplevel $script} scriptResult]
	}
	my variable selfEventQueue
	my variable nonSelfEventQueue
	if {$ecode != 0} {
	    $selfEventQueue clear
	    $nonSelfEventQueue clear
	    return -code $ecode $scriptResult
	}

	set ecode [catch {my dispatchAll} dispatchResult]
	if {$ecode != 0} {
	    $selfEventQueue clear
	    $nonSelfEventQueue clear
	    return -code $ecode $dispatchResult
	}
	return $scriptResult
    }

    method dispatchAll {} {
	while {[my dispatchOne]} {
	    #empty
	}
    }

    method dispatchOne {} {
	my variable selfEventQueue
	my variable nonSelfEventQueue
	set didEvent 1
	if {[$selfEventQueue size]} {
	    my DeliverEvent [$selfEventQueue get]
	} elseif {[$nonSelfEventQueue size]} {
	    my DeliverEvent [$nonSelfEventQueue get]
	} else {
	    set didEvent 0
	}
	return $didEvent
    }

    method addActiveClassInfo {activeClass finalState transition} {
	relvar eval {
	    relvar union ActiveClass\
		[relation create {Class string DefaultInitialState string\
		    DefaultTransition string} Class $activeClass]
	    relvar union FinalState $finalState
	    relvar union Transition $transition
	}
	#puts [ral::relformat [set [self namespace]::ActiveClass] ActiveClass]
	#puts [ral::relformat [set [self namespace]::FinalState] FinalState]
	#puts [ral::relformat [set [self namespace]::Transition] Transition]
    }

    method defaultTransition {class {defTrans {}}} {
	if {$defTrans ne ""} {
	    relvar updateone [self] ac [list Class $class] {
		tuple update ac DefaultTransition __${defTrans}__
	    }
	}
	::ralutil::pipe {
	    relvar set ActiveClass |
	    relation choose ~ Class $class |
	    relation extract ~ DefaultTransition
	}
    }
    method defaultInitialState {class} {
	::ralutil::pipe {
	    relvar set ActiveClass |
	    relation choose ~ Class $class |
	    relation extract ~ DefaultInitialState
	}
    }
    method isFinalState {class state} {
	::ralutil::pipe {
	    relvar set FinalState |
	    relation choose ~ Class $class State $state |
	    relation isnotempty
	}
    }
    method getTransition {class state event} {
	set trans [::ralutil::pipe {
	    relvar set Transition |
	    relation choose ~ Class $class State $state Event $event
	}]
	return [expr {[relation isnotempty $trans] ?\
	    [relation extract $trans NewState] :\
	    [my defaultTransition $class]}]
    }

    method DeliverEvent {evt} {
	set class [dict get $evt RelvarName]
	switch -exact -- [dict get $evt EventType] {
	    normal {
		set obj [$class new]
		$obj selectOne {expand}[dict get $evt Reference]
	    }
	    create {
		set obj [$class new {expand}[dict get $evt Reference]\
		    __currentstate__ __created__]
	    }
	    default {
		error "unknown event type, \"[dict get $evt EventType]\""
	    }
	}
	$obj deliver [dict get $evt Event] {expand}[dict get $evt Params]
	$obj destroy
    }
}
