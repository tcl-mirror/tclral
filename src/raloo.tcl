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
# $Revision: 1.4 $
# $Date: 2007/03/28 16:05:54 $
#  *--

package provide raloo 0.1

package require Tcl 8.5
package require ral 0.8.2
package require ralutil 0.8.2

namespace eval ::raloo {
    namespace export Domain
}

# Domain class.
# Instances of a Domain are encapsuled sets of classes and relationships.
# Domain Functions provide the explicit interface to a domain.
oo::class create ::raloo::Domain {
    # We want all domains to be given a name
    self.unexport new
    constructor {definition} {
	# Each domain also creates a namespace that is the same name as
	# the domain. Make sure that it doesn't already exist.
	if {[namespace exists [self]]} {
	    error "A namespace called, \"[self]\", already exists"
	}
	namespace eval [self] namespace import ::ral::*
	my eval namespace import ::ral::*

	my eval [list namespace path [concat [my eval namespace path] [self]]]

	# Each domain includes the necessary event queues to dispatch
	# state machine events.
	my variable selfEventQueue
	set selfEventQueue [raloo::EventQueue new]
	my variable nonSelfEventQueue
	set nonSelfEventQueue [raloo::EventQueue new]

	# Define a variables that are used to keep track of classes and
	# relationships created in within this domain. These need to be
	# cleaned up if the domain is deleted.
	my variable domainClasses
	set domainClasses [list]
	my variable domainRelationships
	set domainRelationships [list]

	my eval $definition
    }

    destructor {
	my variable selfEventQueue
	$selfEventQueue destroy
	my variable nonSelfEventQueue
	$nonSelfEventQueue destroy

	my variable domainRelationships
	foreach r $domainRelationships {
	    $r destroy
	}
	my variable domainClasses
	foreach c $domainClasses {
	    $c destroy
	}

	namespace delete [self]
    }

    # Unexported methods that are used in the definition of a domain
    # during construction.
    method Function {name argList body} {
	oo::define [self] method $name $argList [list my ExecFunc $body]
	oo::define [self] export $name
    }
    method Class {name definition} {
	set fullName [namespace eval [self] [list raloo::PassiveClass create\
		$name $definition]]
	my variable domainClasses
	lappend domainClasses $fullName
	return
    }
    method ActiveClass {name definition} {
	set fullName [namespace eval [self] [list raloo::ActiveClass create\
		$name $definition]]
	my variable domainClasses
	lappend domainClasses $fullName
	return
    }
    method Generalization {name definition} {
	set fullName [namespace eval [self] [list raloo::Generalization create\
		$name $definition]]
	my variable domainRelationships
	lappend domainRelationships $fullName
	return
    }
    method Relationship {name definition} {
	set fullName [namespace eval [self] [list raloo::Relationship create\
		$name $definition]]
	my variable domainRelationships
	lappend domainRelationships $fullName
	return
    }
    method AssocRelationship {name definition} {
	set fullName [namespace eval [self] [list raloo::AssocRelationship\
		create $name $definition]]
	my variable domainRelationships
	lappend domainRelationships $fullName
	return
    }
    # Exported methods used by other components but not intended to be called
    # by general Domain users.
    method __queueNonSelf {class ref event args} {
	my variable nonSelfEventQueue
	$nonSelfEventQueue putNormal $class $ref $event $args
    }
    export __queueNonSelf
    method __queueSelf {class ref event args} {
	my variable selfEventQueue
	$selfEventQueue putNormal $class $ref $event $args
    }
    export __queueSelf
    method __queueCreation {class ref event args} {
	my variable nonSelfEventQueue
	$nonSelfEventQueue putCreation $class $ref $event $args
    }
    export __queueCreation
    # Unexported method used internal to the Domain class.
    method ExecFunc {script} {
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

	set ecode [catch {my DispatchAllEvents} dispatchResult]
	if {$ecode != 0} {
	    $selfEventQueue clear
	    $nonSelfEventQueue clear
	    return -code $ecode $dispatchResult
	}
	return $scriptResult
    }
    method DispatchAllEvents {} {
	my variable selfEventQueue
	my variable nonSelfEventQueue
	while {1} {
	    if {[$selfEventQueue size]} {
		my DeliverEvent [$selfEventQueue get]
	    } elseif {[$nonSelfEventQueue size]} {
		my DeliverEvent [$nonSelfEventQueue get]
	    } else {
		break ;
	    }
	}
    }
    method DeliverEvent {evt} {
	set class [dict get $evt RelvarName]
	switch -exact -- [dict get $evt EventType] {
	    normal {
		set obj [$class new]
		$obj selectOne {expand}[dict get $evt Reference]
	    }
	    creation {
		set obj [$class new {expand}[dict get $evt Reference]\
		    __currentstate__ __created__]
	    }
	    polymorphic {
		# Find all the partition constraints where "class" is
		# a supertype.
		# If there are none ==> error, polymorphic event sent to
		# subtype leaf.
		# Foreach of those constraints, search the list of subtypes
		# to find the related one.
		# For the found subtype, determine if the event is consumed
		# in the subtype or passed along as a polymorphic event.
		# Generate the appropropriate type of event to the subtype.
	    }
	    default {
		error "unknown event type, \"[dict get $evt EventType]\""
	    }
	}
	$obj deliver [dict get $evt Event] {expand}[dict get $evt Params]
	$obj destroy
    }
}

# This is a meta-class for classes based on relvars.
oo::class create ::raloo::RelvarClass {
    superclass oo::class
    self.unexport new
    constructor {definition} {
	my eval namespace import ::ral::*

	my variable domain
	set domain [namespace qualifiers [self]]
	namespace path [concat [namespace path] $domain]

	my variable relvarName
	set relvarName [self]

	oo::define [self] {
	    constructor {args} {
		my variable relvarName
		set relvarName [self class]
		my variable domain
		set domain [namespace qualifiers $relvarName]
		next {expand}$args
	    }
	    self.method insert {args} {
		relvar insert [self] $args
		return
	    }
	    self.method delete {idValList} {
		foreach attrValSet $idValList {
		    relvar deleteone [self] {expand}$attrValSet
		}
	    }
	    self.method update {idValList attrName value} {
		foreach attrValSet $idValList {
		    relvar updateone [self] t $attrValSet {
			tuple update t $attrName $value
		    }
		}
	    }
	    self.method cardinality {} {
		relation cardinality [relvar set [self]]
	    }
	    self.method degree {} {
		relation degree [relvar set [self]]
	    }
	    self.method format {args} {
		relformat [relvar set [self]] [self] {expand}$args
	    }
	    self.method setUniqueAttrValue {attr op relvar tupValue} {
		my variable uniqueAttrMap
		dict incr uniqueAttrMap $attr
		tuple update tupValue $attr [dict get $uniqueAttrMap $attr]
	    }
	}
	# Set up variables that are used by the methods that are
	# invoked when evaluating the definition.
	relvar create __attributes {
	    Relation {
		AttrName string
		AttrType string
	    } {
		AttrName
	    }
	}
	relvar create __ids {
	    Relation {
		IdNum int
		AttrName string
	    } {
		{IdNum AttrName}
	    }
	}
	my variable uniqueAttrMap
	set uniqueAttrMap [dict create]

	# Evaluate the definition. The definition make calls to the
	# meta-class methods to specify the details of the newly minted class.
	my eval $definition
    }
    destructor {
	relvar unset [self]
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
		    relvar insert __ids [list\
			IdNum $idNum\
			AttrName $attrName]
		}
		if {[string match -nocase unique* $type]} {
		    my variable uniqueAttrMap
		    dict set uniqueAttrMap $attrName 0
		    set type int
		}
		relvar insert __attributes [list\
		    AttrName $attrName\
		    AttrType $type]
	    } else {
		error "unrecognized attribute name syntax, \"$name\""
	    }
	}
    }
    method InstOp {name argList body} {
	oo::define [self] method $name $argList $body
	oo::define [self] export $name
    }
    method ClassOp {name argList body} {
	oo::define [self] self.method $name $argList $body
	oo::define [self] export $name
    }
    method MakeRelvar {} {
	# Generate the relvar from the attributes specified in the
	# defintion.
	set idList [list]
	relation foreach idrel [relation summarize [relvar set __ids]\
		[relation project [relvar set __ids] IdNum] r\
		AttrList list {[relation list $r AttrName]}]\
		-ascending IdNum {
	    lappend idList [relation extract $idrel AttrList]
	}
	if {[llength $idList] == 0} {
	    error "all relvar classes must have at least one identifer"
	}
	relvar create [self] [list Relation\
		[relation dict [relvar set __attributes]] $idList]

	# Put on relvar traces for any unique attributes.
	my variable uniqueAttrMap
	dict for {attr value} $uniqueAttrMap {
	    relvar trace add variable [self] insert\
		[list [self] setUniqueAttrValue $attr]
	}

	# Define a method for each attribute. If the attribute is an
	# identifier, then updates are not allowed and the method
	# interface will accept no additional argument.
	# Make sure that there is no "currentstate" method. currentstate
	# is strictly controlled internally.
	set attributes [relation restrictwith [relvar set __attributes]\
	    {$AttrName ne "__currentstate__"}]
	set attrNotIds [relation semiminus [relvar set __ids] $attributes]
	relation foreach attr $attrNotIds {
	    relation assign $attr
	    oo::define [self] method $AttrName {{val {}}} [format {
		if {$val ne ""} {
		    my __UpdateAttr %s $val
		}
		relation extract [my __DeReference] %s
	    } $AttrName $AttrName]
	    oo::define [self] export $AttrName
	}
	set attrAsIds [relation semijoin [relvar set __ids] $attributes]
	relation foreach attr $attrAsIds {
	    relation assign $attr
	    oo::define [self] method $AttrName {} [format {
		relation extract [my __DeReference] %s
	    } $AttrName]
	    oo::define [self] export $AttrName
	}

	relvar unset __attributes __ids
    }
    method domain {} {
	my variable domain
	return $domain
    }
    method path {} {
	return [self]
    }
}

oo::class create ::raloo::PassiveClass {
    superclass ::raloo::RelvarClass
    constructor {definition} {
	oo::define [self] superclass ::raloo::PassiveRef
	next $definition
	my MakeRelvar
    }
    destructor {
	next
    }
}

oo::class create ::raloo::ActiveClass {
    superclass ::raloo::RelvarClass
    constructor {definition} {
	oo::define [self] superclass ::raloo::ActiveRef
	my eval namespace import ::ral::*

	relvar create State {
	    Relation {
		Name string
	    } {
		Name
	    }
	}
	my variable defaultInitialState
	my variable defaultTransition
	my variable finalStates
	relvar create Transition {
	    Relation {
		State string
		Event string
		NewState string
	    } {
		{State Event}
	    }
	}

	#relvar association R1 Transition State + State Name ?

	next $definition

	if {![info exists defaultTransition]} {
	    set defaultTransition __canthappen__
	}

	# Define a class method if there are any creation events.
	if {[ralutil::pipe {
	    relvar set Transition |
	    relation project ~ State |
	    relation choose ~ State __created__ |
	    relation isnotempty}]} {
	    oo::define [self] self.method signal {ref event args} {
		my variable domain
		$domain __queueCreation [self] $ref $event $args
	    }
	}

	# Add the current state attribute.
	my Attribute {__currentstate__ string}
	my MakeRelvar
    }
    destructor {
	relvar unset State Transition
	next
    }
    method State {name argList body} {
	oo::define [self] method $name $argList $body
	oo::define [self] unexport $name

	relvar insert State [list Name $name]

	my variable defaultInitialState
	if {![info exists defaultInitialState]} {
	    set defaultInitialState $name
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
		my variable finalStates
		lappend finalStates $newState
	    }
	} else {
	    error "unrecognized new state name, \"$newState\""
	}
	relvar insert Transition [list\
	    State $currState\
	    Event $event\
	    NewState $newState]
    }
    method DefaultInitialState {{defState {}}} {
	my variable defaultInitialState
	if {$defState ne ""} {
	    set defaultInitialState $defState
	}
	return $defaultInitialState
    }
    method DefaultTransition {{trans {}}} {
	my variable defaultTransition
	if {$trans ne ""} {
	    if {[string equal -nocase $trans "ignore"] ||\
		[string equal -nocase $trans "canthappen"]} {
		set defaultTransition $trans
	    } else {
		error "bad default transition name, \"$trans\""
	    }
	}
	return $defaultTransition
    }
    method getNewState {currState event} {
	set trans [relation choose [relvar set Transition]\
		State $currState Event $event]
	return [expr {[relation isempty $trans] ?\
	    [my DefaultTransition] : [relation extract $trans NewState]}]
    }
    method getDefaultInitialState {currState event} {
	my variable defaultInitialState
	return $defaultInitialState
    }
    method isFinalState {state} {
	my variable finalStates
	return [expr {[lsearch $finalStates $state] >= 0}]
    }
}

# This class is the primary base class for all relvar references.
# Instances of this class refer to a relvar, but have no state behavior.
oo::class create ::raloo::PassiveRef {
    constructor {args} {
	namespace import ::ral::*

	my variable ref
	my variable relvarName
	set ref [relation emptyof [relvar set $relvarName]]
	if {[llength $args] != 0} {
	    # relvar insert returns just what was inserted.
	    set ref [relvar insert $relvarName $args]
	}
	set ref [relation project $ref\
	    {expand}[lindex [relation identifiers $ref] 0]]

	set domain [namespace qualifiers $relvarName]
	namespace path [concat [namespace path] $domain]
    }
    method myClass {} {
	info object [self] class
    }
    method selectOne {args} {
	my variable relvarName
	my variable ref
	my __ToReference [::ralutil::pipe {
	    relvar set $relvarName |
	    relation choose ~ {expand}$args}]
    }
    method selectWhere {expr} {
	my variable relvarName
	::ralutil::pipe "
	    relvar set $relvarName |
	    relation restrictwith ~ [list $expr]" cmd
	my __ToReference [uplevel $cmd]
    }
    method selectAny {expr} {
	my variable relvarName
	::ralutil::pipe "
	    relvar set $relvarName |
	    relation restrictwith ~ [list $expr] |
	    relation tag ~ __Order__ |
	    relation choose ~ __Order__ 0" cmd
	my __ToReference [uplevel $cmd]
    }
    method selectRelated {args} {
	set r [my __DeReference]
	my variable relvarName
	my variable domain
	set dstRelvar $relvarName
	set sjCmd [list relation semijoin $r]
	foreach rship $args {
	    lassign [raloo::PassiveRef parseTraversal $rship]\
		    dirMark rName cMark endMark
	    set rInfo [relvar constraint info ${domain}::$rName]
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
		partition> {
		    set srcRelvar [lindex $rInfo 2]
		    set endPath [$endMark path]
		    lassign [raloo::PassiveRef findSubType $endPath $rInfo]\
			targetRelvar targetAttrs
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
	$selected __ToReference [eval $sjCmd]
	return $selected
    }
    method relate {rship target} {
	my variable ref
	my variable relvarName
	my variable domain
	lassign [raloo::PassiveRef parseTraversal $rship]\
		dirMark rName cMark endMark
	set rInfo [relvar constraint info ${domain}::$rName]
	set ctype [lindex $rInfo 0]
	switch -exact -- $dirMark$ctype$cMark {
	    association {
		# Traversal in the forward direction
		set refrngRelvar [lindex $rInfo 2]
		set refrngAttrs [lindex $rInfo 3]
		set refrngRef [my __RefBody]
		set refToRelvar [lindex $rInfo 5]
		set refToAttrs [lindex $rInfo 6]
		set refToRef [$target __RefValue]
		set srcRelvar $refrngRelvar
		set targetRelvar $refToRelvar
	    }
	    
	    ~association {
		set refrngRelvar [lindex $rInfo 5]
		set refrngAttrs [lindex $rInfo 6]
		set refrngRef [$target __RefBody]
		set refToRelvar [lindex $rInfo 2]
		set refToAttrs [lindex $rInfo 3]
		set refToRef [my __RefValue]
		set srcRelvar $refToRelvar
		set targetRelvar $refrngRelvar
	    }
	    partition {
		lassign [raloo::PassiveRef findSubType $relvarName $rInfo]\
		    refrngRelvar refrngAttrs
		set refrngRef [my __RefBody]
		set refToRelvar [lindex $rInfo 2]
		set refToAttrs [lindex $rInfo 3]
		set refToRef [$target __RefValue]
		set srcRelvar $refrngRelvar
		set targetRelvar $refToRelvar
	    }
	    partition> {
		set refToRelvar [lindex $rInfo 2]
		set refToAttrs [lindex $rInfo 3]
		set refToRef [my __RefValue]
		lassign [raloo::PassiveRef findSubType [$target myClass]\
		    $rInfo] refrngRelvar refrngAttrs
		set refrngRef [$target __RefBody]
		set srcRelvar $refToRelvar
		set targetRelvar $refrngRelvar
	    }
	    default {
		error "the \"relate\" method cannot be use for the given\
		    type of relationship traversal, \"$rship\""
	    }
	}
	if {$relvarName ne $srcRelvar} {
	    error "traversal of $rship begins at $srcRelvar,\
		not $relvarName"
	}
	if {$targetRelvar ne [$target myClass]} {
	    error "traversal of $rship ends at $targetRelvar, not at\
		[$target myClass]"
	}
	# Update the referential attributes in the relvar to have the same
	# values as the referred to attributes in the target.  The update can
	# change the values that are held in the "ref" if we are changing
	# identifiers. Need to make sure the "ref" is updated also.
	# Cf.  __UpdateAttr.
	set updated [list]
	foreach refTuple $refrngRef {
	    lappend updated [relvar updateone $refrngRelvar tup $refTuple {
		set avList [list]
		foreach sAttr $refrngAttrs tAttr $refToAttrs {
		    lappend avList $sAttr [dict get $refToRef $tAttr]
		}
		tuple update tup {expand}$avList
	    }]
	}
	if {$relvarName eq $refrngRelvar} {
	    my __ToReference [relation union\
		[relation emptyof [relvar set $relvarName]]\
		{expand}$updated]
	}
	return
    }
    method relateUsing {rship target args} {
	my variable ref
	my variable relvarName
	my variable domain
	lassign [raloo::PassiveRef parseTraversal $rship]\
		dirMark rName cMark endMark
	set rInfo [relvar constraint info ${domain}::$rName]
	set ctype [lindex $rInfo 0]
	switch -exact -- $dirMark$ctype$cMark {
	    correlation {
		set oneRelvar [lindex $rInfo 5]
		set oneRefToAttrs [lindex $rInfo 6]
		set oneRef [my __RefValue]
		set oneRefrngAttrs [lindex $rInfo 3]
		set otherRelvar [lindex $rInfo 9]
		set otherRefToAttrs [lindex $rInfo 10]
		set otherRef [$target __RefValue]
		set otherRefrngAttrs [lindex $rInfo 7]
		set srcRelvar $oneRelvar
		set targetRelvar $otherRelvar
	    }
	    
	    ~correlation {
		set oneRelvar [lindex $rInfo 9]
		set oneRefToAttrs [lindex $rInfo 10]
		set oneRef [$target __RefValue]
		set oneRefrngAttrs [lindex $rInfo 7]
		set otherRelvar [lindex $rInfo 5]
		set otherRefToAttrs [lindex $rInfo 6]
		set otherRef [my __RefValue]
		set otherRefrngAttrs [lindex $rInfo 3]
		set srcRelvar $otherRelvar
		set targetRelvar $oneRelvar
	    }
	    default {
		error "the \"relateUsing\" method cannot be use for the given\
		    type of relationship traversal, \"$rship\""
	    }
	}
	if {$relvarName ne $srcRelvar} {
	    error "traversal of $rship begins at $srcRelvar,\
		not $relvarName"
	}
	if {$targetRelvar ne [$target myClass]} {
	    error "traversal of $rship ends at $targetRelvar, not at\
		[$target myClass]"
	}
	# Create an instance of the association class, containing the
	# referential attribute values and the other attribute values
	# supplied as arguments.
	set assocTuple [list]
	foreach refrng $oneRefrngAttrs refto $oneRefToAttrs {
	    lappend assocTuple $refrng [dict get $oneRef $refto]
	}
	foreach refrng $otherRefrngAttrs refto $otherRefToAttrs {
	    lappend assocTuple $refrng [dict get $otherRef $refto]
	}
	set assocClass [lindex $rInfo 2]
	set assocInst [$assocClass new]
	$assocInst __ToReference [relvar insert $assocClass\
	    [concat $assocTuple $args]]
	return $assocInst
    }
    method unrelate {rship} {
	my variable ref
	my variable relvarName
	my variable domain
	lassign [raloo::PassiveRef parseTraversal $rship]\
		dirMark rName cMark endMark
	set rInfo [relvar constraint info ${domain}::$rName]
	set ctype [lindex $rInfo 0]
	switch -exact -- $dirMark$ctype$cMark {
	    association {
		set refrngRelvar [lindex $rInfo 2]
		set refrngAttrs [lindex $rInfo 3]
		set refrngRef [my __RefBody]
		set srcRelvar $refrngRelvar
	    }
	    
	    ~association {
		# When traversing the opposite direction, we have to find
		# the referring references.
		set refrngRelvar [lindex $rInfo 5]
		set refrngAttrs [lindex $rInfo 6]
		set refrngInsts [my selectedRelated $rship]
		set refrngRef [$refrngInsts __RefBody]
		$refrngInst destroy
		set srcRelvar [lindex $rInfo 2]
	    }
	    partition {
		lassign [raloo::PassiveRef findSubType $relvarName $rInfo]\
		    refrngRelvar refrngAttrs
		set refrngRef [my __RefBody]
		set srcRelvar $refrngRelvar
	    }
	    partition> {
		set refrngInst [my selectedRelated $rship]
		if {[$refrngInst isempty]} {
		    error "instance is not related to $endMark across $rship"
		}
		set refrngRef [$refrngInsts __RefBody]
		lassign [raloo::PassiveRef findSubType $endMark $rInfo]\
		    refrngRelvar refrngAttrs
		set srcRelvar [lindex $rInfo 2]
	    }
	    default {
		error "the \"relate\" method cannot be use for the given\
		    type of relationship traversal, \"$rship\""
	    }
	}
	if {$relvarName ne $srcRelvar} {
	    error "traversal of $rship begins at $srcRelvar,\
		not $relvarName"
	}
	# Update the referential attributes in the relvar to be the empty
	# string.  The update can change the values that are held in the "ref"
	# if we are changing identifiers. Need to make sure the "ref" is
	# updated also.
	set updated [list]
	foreach refTuple $refrngRef {
	    lappend updated [relvar updateone $refrngRelvar tup $refTuple {
		set avList [list]
		foreach sAttr $refrngAttrs {
		    lappend avList $sAttr {}
		}
		tuple update tup {expand}$avList
	    }]
	}
	if {$relvarName eq $refrngRelvar} {
	    my __ToReference [relation union\
		[relation emptyof [relvar set $relvarName]]\
		{expand}$updated]
	}
	return
    }
    method migrate {rName target args} {
	my variable ref
	my variable relvarName
	my variable domain
	set rInfo [relvar constraint info ${domain}::$rName]
	if {[lindex $rInfo 0] ne "partition"} {
	    error "$rname is not a generalization relationship"
	}
	lassign [raloo::PassiveRef findSubType $relvarName $rInfo]\
	    currRefrngRelvar currRefrngAttrs
	set targetRelvar [$target path]
	lassign [raloo::PassiveRef findSubType $targetRelvar $rInfo]\
	    newRefrngRelvar newRefrngAttrs

	set avList [list]
	set myValue [my __RefValue]
	foreach ca $currRefrngAttrs na $newRefrngAttrs {
	    lappend avList $na [dict get $myValue $ca]
	}

	set avList [concat $avList $args]
	relvar eval {
	    relvar deleteone $relvarName {expand}[my __RefTuple]
	    set newRel [relvar insert $targetRelvar $avList]
	}
	set ref [relation emptyof $ref]

	set newRef [$target new]
	$newRef __ToReference $newRel
	return $newRef
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
    method isempty {} {
	my variable ref
	relation isempty $ref
    }
    method isnotempty {} {
	my variable ref
	relation isnotempty $ref
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
	relation semijoin $ref [relvar set $relvarName]
    }
    unexport __DeReference
    # Convert a relation value of the base relvar into a reference.
    method __ToReference {relValue} {
	my variable relvarName
	if {[relation isnotempty $relValue] && [relation isempty\
		[relation intersect [relvar set $relvarName] $relValue]]} {
	    error "relation value is not contained in $relvarName"
	}
	my variable ref
	set ref [relation project $relValue {expand}[relation attributes $ref]]
    }
    export __ToReference
    # Obtain a dictionary of the reference. This can be used to
    # reconstruct the reference later. This is only valid for singleton
    # references.
    method __RefTuple {} {
	my variable ref
	return [tuple get [relation tuple $ref]]
    }
    export __RefTuple
    method __RefBody {} {
	my variable ref
	relation body $ref
    }
    export __RefBody
    method __RefValue {} {
	my variable relvarName
	my variable ref
	tuple get\
	    [relation tuple [relation semijoin $ref [relvar set $relvarName]]]
    }
    export __RefValue
    # Update an attribute in the base relvar to which this reference refers.
    method __UpdateAttr {attrName value} {
	my variable relvarName
	my variable ref

	set updates [list]
	relation foreach r $ref {
	    lappend updates [relvar updateone $relvarName tup\
		    [tuple get [relation tuple $r]] {
		    tuple update tup $attrName $value
		}]
	}
	my __ToReference [relation union\
	    [relation emptyof [relvar set $relvarName]]\
	    {expand}$updates]
    }
    unexport __UpdateAttr


    self.method parseTraversal {rship} {
	if {![regexp {\A(~)?([^>]+)(>.*)?\Z} $rship\
		match dirMark rName endMark]} {
	    error "bad relationship syntax, \"$rship\""
	}
	return [list $dirMark $rName [string index $endMark 0]\
	    [string range $endMark 1 end]]
    }
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
		lappend args __currentstate__ [my DefaultInitialState]
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
	    set queue QueueNonSelf
	} else {
	    set queue [expr {[lindex $caller 1] eq [self] ?\
		    "QueueSelf" : "QueueNonSelf"}]
	}
	# This reference may be multi-valued. Generate an event to each one.
	foreach tupValue [relation body $ref] {
	    my $queue $relvarName $tupValue $event $args
	}
    }
    method deliver {event args} {
	my variable relvarName
	set currentState [relation extract [my __DeReference] __currentstate__]
	set newState [my GetNewState $currentState $event]
	if {$newState ne "__ignore__"} {
	    if {$newState eq "__canthappen__"} {
		error "can't happen transition:\
		    $currentState -- $event --> [string trim $newState _]"
	    } else {
		# update the current state to the new state
		my __UpdateAttr __currentstate__ $newState
		# Execute the state action. This must be done in a transaction.
		# State actions must leave the relvar state consistent.
		relvar eval my $newState {expand}$args
		# if we enter a final state, then delete the instance.
		if {[my IsFinalState $newState]} {
		    my delete
		}
	    }
	}
    }
    method DefaultInitialState {} {
	[self class] defaultInitialState
    }
    method GetNewState {currState event} {
	[my myClass] getNewState $currState $event
    }
    method IsFinalState {state} {
	[my myClass] isFinalState $state
    }
    method QueueSelf {relvarName tupValue event argList} {
	[[my myClass] domain] __queueSelf $relvarName $tupValue\
	    $event $argList
    }
    method QueueNonSelf {relvarName tupValue event argList} {
	[[my myClass] domain] __queueNonSelf $relvarName\
	    $tupValue $event $argList
    }
}

# This is a class that constructs relationships that correspond to
# relvar constraints.
oo::class create ::raloo::Association {
    self.unexport create
    self.unexport new
    constructor {} {
	namespace import ::ral::*
	my variable domain
	set domain [namespace qualifiers [self]]
	namespace path [concat [namespace path] $domain]
    }
    destructor {
	relvar constraint delete [self]
    }
    method path {} {
	my variable domain
	return ${domain}::[self]
    }
    method info {} {
	relvar constraint info [my path]
    }
}

oo::class create ::raloo::Relationship {
    superclass ::raloo::Association
    constructor {definition} {
	next
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
	my variable domain
	dict set rshipInfo refrngRelvar [$formClass path]
	dict set rshipInfo refrngSpec $refrngSpec
	dict set rshipInfo refToRelvar [$partClass path]
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
	next

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
	dict set rshipInfo assocRelvar [$assocClass path]
	dict set rshipInfo oneRelvar [$oneClass path]
	dict set rshipInfo oneSpec $oneSpec
	dict set rshipInfo otherRelvar [$otherClass path]
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
	next

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
	set refToRelvar [$superRel path]
	set superIds [relation identifiers [relvar set $refToRelvar]]
	if {$id >= [llength $superIds]} {
	    error "requested identifier, \"$id\", but $refToRelvar has only\
		[llength $superIds] identifiers"
	}
	my variable refToAttrs
	set refToAttrs [lsort [lindex $superIds $id]]
    }
    method SubTypes {args} {
	foreach st $args {
	    relvar insert subType [list\
		RelvarName [$st path]\
		RefngAttrs {}\
	    ]
	}
    }
    method ReferenceById {subtype idNum} {
	set subtypeRelvar [$subtype path]
	set subIdSet [relation identifiers [relvar set $subtypeRelvar]]
	if {[llength $subIdSet] <= $idNum} {
	    error "requested reference by identifier, \"$idNum\", but
		$subtypeRelvar only has [llength $subIdSet] identifiers"
	}
	my Reference $subtype [lsort [lindex $subIdSet $idNum]]
    }
    method Reference {subtype attrList} {
	set subtypeRelvar [$subtype path]
	relvar updateone subType st [list RelvarName $subtypeRelvar] {
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
    }
    destructor {
	relvar unset [self]
    }
    method putNormal {rname ref event params} {
	relvar insert [self] [list Id {} RelvarName $rname\
	    Reference $ref Event $event EventType normal Params $params]
    }
    method putCreation {rname ref event params} {
	relvar insert [self] [list Id {} RelvarName $rname\
	    Reference $ref Event $event EventType creation Params $params]
    }
    method putPolymorphic {rname ref event params} {
	relvar insert [self] [list Id {} RelvarName $rname\
	    Reference $ref Event $event EventType polymorphic Params $params]
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
	relvar set [self] [relation emptyof [relvar set [self]]]
    }
}
