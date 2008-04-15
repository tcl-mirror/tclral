# This software is copyrighted 2008 by G. Andrew Mangogna.
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
#  This file contains the package definition for the "raloo" package.
#  Raloo is an relation oriented programming system that combines the
#  relational algebraic ideas of TclRAL with the object oriented programming
#  capabilities of TclOO.
# 
# $RCSfile: raloo.tcl,v $
# $Revision: 1.17 $
# $Date: 2008/04/15 15:33:26 $
#  *--

package require Tcl 8.5
package require TclOO
package require ral
package require ralutil

package provide raloo 0.1

namespace eval ::raloo {
    namespace export Domain
    namespace import ::ral::*
}

namespace eval ::raloo::mm {
    namespace import ::ral::*
    # The following relvars and constraints define a "meta-model" for raloo.
    # The commands that are used to define the domains and domain components
    # serve as a "user interface" to populating the meta-model. The TclOO
    # classes that are subsequently created, obtain the information they need
    # from the meta-model relvars. This design clearly separates the concerns
    # of definition syntax from the information structure required to support
    # raloo. It also offers the possiblity of creating the domain objects
    # from a serialized copy of a meta-model population (although that
    # functionality is not currently supported).

    relvar create Domain {
	Relation {
	    DomName string
	} {
	    DomName
	}
    }

    relvar create Class {
	Relation {
	    DomName string
	    ClassName string
	} {
	    {DomName ClassName}
	}
    }

    relvar association R1\
	Class DomName *\
	Domain DomName 1

    relvar create Attribute {
	Relation {
	    DomName string
	    ClassName string
	    AttrName string
	    AttrType string
	} {
	    {DomName ClassName AttrName}
	}
    }

    relvar association R2\
	Attribute {DomName ClassName} +\
	Class {DomName ClassName} 1

    relvar create Identifier {
	Relation {
	    DomName string
	    ClassName string
	    IdNum int
	} {
	    {DomName ClassName IdNum}
	}
    }

    relvar association R3\
	Identifier {DomName ClassName} +\
	Class {DomName ClassName} 1

    relvar create IdAttribute {
	Relation {
	    DomName string
	    ClassName string
	    IdNum int
	    AttrName string
	} {
	    {DomName ClassName IdNum AttrName}
	}
    }

    relvar correlation R4 IdAttribute\
	{DomName ClassName IdNum} + Identifier {DomName ClassName IdNum}\
	{DomName ClassName AttrName} * Attribute {DomName ClassName AttrName}

    relvar create DomainOp {
	Relation {
	    DomName string
	    OpName string
	    OpParams list
	    OpBody string
	} {
	    {DomName OpName}
	}
    }

    relvar association R5\
	DomainOp DomName *\
	Domain DomName 1

    relvar create SyncService {
	Relation {
	    DomName string
	    ServiceName string
	    Params list
	    Body string
	} {
	    {DomName ServiceName}
	}
    }

    relvar association R24\
	SyncService DomName *\
	Domain DomName 1

    relvar create ClassOp {
	Relation {
	    DomName string
	    ClassName string
	    OpName string
	    OpParams list
	    OpBody string
	} {
	    {DomName ClassName OpName}
	}
    }

    relvar association R6\
	ClassOp {DomName ClassName} *\
	Class {DomName ClassName} 1

    relvar create InstOp {
	Relation {
	    DomName string
	    ClassName string
	    OpName string
	    OpParams list
	    OpBody string
	} {
	    {DomName ClassName OpName}
	}
    }

    relvar association R20\
	InstOp {DomName ClassName} *\
	Class {DomName ClassName} 1

    relvar create Relationship {
	Relation {
	    DomName string
	    RelName string
	} {
	    {DomName RelName}
	}
    }
    relvar association R7\
	Relationship DomName *\
	Domain DomName 1

    relvar create SimpleRel {
	Relation {
	    DomName string
	    RelName string
	} {
	    {DomName RelName}
	}
    }

    relvar create GenRel {
	Relation {
	    DomName string
	    RelName string
	} {
	    {DomName RelName}
	}
    }

    relvar create AssocRel {
	Relation {
	    DomName string
	    RelName string
	} {
	    {DomName RelName}
	}
    }

    relvar partition R8\
	Relationship {DomName RelName}\
	    SimpleRel {DomName RelName}\
	    GenRel {DomName RelName}\
	    AssocRel {DomName RelName}

    relvar create SimpleReferring {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	    Cardinality string
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R9\
	SimpleReferring {DomName RelName} 1\
	SimpleRel {DomName RelName} 1

    relvar create SimpleRefTo {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	    Cardinality string
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R10\
	SimpleRefTo {DomName RelName} 1\
	SimpleRel {DomName RelName} 1

    relvar create SupertypeRole {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R11\
	SupertypeRole {DomName RelName} 1\
	GenRel {DomName RelName} 1

    relvar create SubtypeRole {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R12\
	SubtypeRole {DomName RelName} +\
	GenRel {DomName RelName} 1

    # This defines a procedural constraint to insure that no Subtypes
    # are also Supertypes.
    relvar trace add variable SubtypeRole insert\
	[namespace code superTypeNotSubType]
    proc superTypeNotSubType {op relvarName tupleValue} {
	set isSuper [ralutil::pipe {
	    tuple relation $tupleValue |
	    relation semijoin ~ $::raloo::mm::SupertypeRole -using\
		    {DomName DomName RelName RelName ClassName ClassName} |
	    relation isnotempty
	}]
	if {$isSuper} {
	    error "a supertype may not be its own subtype,\
		\"[tuple extract $tupleValue ClassName]\""
	}
	return $tupleValue
    }

    relvar create AssocSource {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	    Cardinality string
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R13\
	AssocSource {DomName RelName} 1\
	AssocRel {DomName RelName} 1

    relvar create Associator {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R14\
	Associator {DomName RelName} 1\
	AssocRel {DomName RelName} 1

    relvar create AssocTarget {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	    Cardinality string
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar association R15\
	AssocTarget {DomName RelName} 1\
	AssocRel {DomName RelName} 1

    # This defines a procedural constraint to insure that no
    # associative sources or targets have the same names as an associator.
    relvar trace add variable AssocSource insert [namespace code notAssociator]
    relvar trace add variable AssocTarget insert [namespace code notAssociator]
    proc notAssociator {op relvarName tupleValue} {
	set isAssoc [ralutil::pipe {
	    tuple relation $tupleValue {{DomName RelName ClassName RoleId}} |
	    relation semijoin ~ $::raloo::mm::Associator -using\
		    {DomName DomName RelName RelName ClassName ClassName} |
	    relation isnotempty
	}]
	if {$isAssoc} {
	    error "an associate relationship participant may not also be the\
		associative class, \"[tuple extract $tupleValue ClassName]\""
	}
	return $tupleValue
    }

    relvar create ReferringClass {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar partition R16\
	ReferringClass {DomName RelName ClassName RoleId}\
	    SimpleReferring {DomName RelName ClassName RoleId}\
	    SubtypeRole {DomName RelName ClassName RoleId}\
	    Associator {DomName RelName ClassName RoleId}

    relvar create RefToClass {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar partition R17\
	RefToClass {DomName RelName ClassName RoleId}\
	    SimpleRefTo {DomName RelName ClassName RoleId}\
	    SupertypeRole {DomName RelName ClassName RoleId}\
	    AssocSource {DomName RelName ClassName RoleId}\
	    AssocTarget {DomName RelName ClassName RoleId}

    relvar create ClassRoleInRel {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    RoleId int
	} {
	    {DomName RelName ClassName RoleId}
	}
    }

    relvar partition R18\
	ClassRoleInRel {DomName RelName ClassName RoleId}\
	    ReferringClass {DomName RelName ClassName RoleId}\
	    RefToClass {DomName RelName ClassName RoleId}

    relvar correlation R19 ClassRoleInRel\
	{DomName RelName} + Relationship {DomName RelName}\
	{DomName ClassName} * Class {DomName ClassName}

    relvar create RefToIdAttribute {
	Relation {
	    DomName string
	    ClassName string
	    RelName string
	    RoleId int
	    AttrName string
	    IdNum int
	} {
	    {DomName ClassName RelName RoleId AttrName IdNum}
	}
    }

    relvar correlation R21 RefToIdAttribute\
	{DomName ClassName RelName RoleId} +\
	    RefToClass {DomName ClassName RelName RoleId}\
	{DomName ClassName IdNum AttrName} *\
	    IdAttribute {DomName ClassName IdNum AttrName}

    relvar create AttributeRef {
	Relation {
	    DomName string
	    RelName string
	    RefngClassName string
	    RefngAttrName string
	    RefngRoleId int
	    RefToClassName string
	    RefToAttrName string
	    RefToRoleId int
	    RefToIdNum int
	} {
	    {DomName RelName RefngClassName RefngRoleId\
		RefToClassName RefToAttrName RefToRoleId RefToIdNum}
	}
    }

    relvar correlation R22 AttributeRef\
	{DomName RefngClassName RelName RefngRoleId} +\
	    ReferringClass {DomName ClassName RelName RoleId}\
	{DomName RefToClassName RelName RefToRoleId RefToAttrName RefToIdNum} +\
	    RefToIdAttribute {DomName ClassName RelName RoleId AttrName IdNum}

    relvar association R23\
	AttributeRef {DomName RefngClassName RefngAttrName} *\
	Attribute {DomName ClassName AttrName} 1

    relvar create InstStateModel {
	Relation {
	    DomName string
	    ClassName string
	} {
	    {DomName ClassName}
	}
    }

    relvar association R30\
	InstStateModel {DomName ClassName} ?\
	Class {DomName ClassName} 1

    relvar create AssignerStateModel {
	Relation {
	    DomName string
	    RelName string
	} {
	    {DomName RelName}
	}
    }

    relvar association R31\
	AssignerStateModel {DomName RelName} ?\
	Relationship {DomName RelName} 1

    relvar create SingleAssigner {
	Relation {
	    DomName string
	    RelName string
	} {
	    {DomName RelName}
	}
    }

    relvar create MultipleAssigner {
	Relation {
	    DomName string
	    RelName string
	    ClassName string
	    IdNum int
	} {
	    {DomName RelName}
	}
    }

    relvar partition R38\
	AssignerStateModel {DomName RelName}\
	    SingleAssigner {DomName RelName}\
	    MultipleAssigner {DomName RelName}

    relvar association R39\
	MultipleAssigner {DomName ClassName IdNum} *\
	Identifier {DomName ClassName IdNum} 1

    relvar create StateModel {
	Relation {
	    DomName string
	    ModelName string
	    InitialState string
	    DefaultTrans string
	} {
	    {DomName ModelName}
	}
    }

    relvar partition R32\
	StateModel {DomName ModelName}\
	    InstStateModel {DomName ClassName}\
	    AssignerStateModel {DomName RelName}

    relvar create StatePlace {
	Relation {
	    DomName string
	    ModelName string
	    StateName string
	} {
	    {DomName ModelName StateName}
	}
    }

    relvar create State {
	Relation {
	    DomName string
	    ModelName string
	    StateName string
	    Action string
	    SigId int
	} {
	    {DomName ModelName StateName}
	}
    }

    relvar association R33\
	State {DomName ModelName} +\
	StateModel {DomName ModelName} 1

    relvar create CreationState {
	Relation {
	    DomName string
	    ModelName string
	    StateName string
	} {
	    {DomName ModelName}
	}
    }

    relvar partition R37\
	StatePlace {DomName ModelName StateName}\
	    State {DomName ModelName StateName}\
	    CreationState {DomName ModelName StateName}

    relvar create ActiveState {
	Relation {
	    DomName string
	    ModelName string
	    StateName string
	} {
	    {DomName ModelName StateName}
	}
    }

    relvar association R34\
	StateModel {DomName ModelName InitialState} ?\
	ActiveState {DomName ModelName StateName} 1

    relvar association R40\
	CreationState {DomName ModelName} ?\
	StateModel {DomName ModelName} 1

    relvar create DeadState {
	Relation {
	    DomName string
	    ModelName string
	    StateName string
	} {
	    {DomName ModelName StateName}
	}
    }

    relvar partition R36\
	State {DomName ModelName StateName}\
	    ActiveState {DomName ModelName StateName}\
	    DeadState {DomName ModelName StateName}

    relvar create TransitionPlace {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    StateName string
	} {
	    {DomName ModelName EventName StateName}
	}
    }

    relvar create StateTrans {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    StateName string
	    NewState string
	    SigId int
	} {
	    {DomName ModelName EventName StateName}
	}
    }

    relvar association R46\
	StateTrans {DomName ModelName NewState} *\
	State {DomName ModelName StateName} 1

    relvar create NonStateTrans {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    StateName string
	    TransRule string
	} {
	    {DomName ModelName EventName StateName}
	}
    }

    relvar partition R45\
	TransitionPlace {DomName ModelName EventName StateName}\
	StateTrans {DomName ModelName EventName StateName}\
	NonStateTrans {DomName ModelName EventName StateName}

    relvar create TransitionRule {
	Relation {
	    RuleName string
	} {
	    RuleName
	}
    }

    relvar insert TransitionRule {
	RuleName CH
    } {
	RuleName IG
    }

    relvar association R44\
	NonStateTrans TransRule *\
	TransitionRule RuleName 1

    relvar association R35\
	StateModel DefaultTrans *\
	TransitionRule RuleName 1

    relvar create NewStateTrans {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    StateName string
	} {
	    {DomName ModelName EventName StateName}
	}
    }

    relvar create CreationTrans {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    StateName string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar partition R47\
	StateTrans {DomName ModelName EventName StateName}\
	    NewStateTrans {DomName ModelName EventName StateName}\
	    CreationTrans {DomName ModelName EventName StateName}

    relvar create EffectiveEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar correlation R41 TransitionPlace\
	{DomName ModelName EventName} *\
		EffectiveEvent {DomName ModelName EventName}\
	{DomName ModelName StateName} *\
		StatePlace {DomName ModelName StateName}

    relvar correlation R42 NewStateTrans\
	{DomName ModelName EventName} *\
		EffectiveEvent {DomName ModelName EventName}\
	{DomName ModelName StateName} *\
		ActiveState {DomName ModelName StateName}

    relvar correlation R43 CreationTrans\
	{DomName ModelName EventName} ?\
		EffectiveEvent {DomName ModelName EventName}\
	{DomName ModelName} *\
		CreationState {DomName ModelName}

    relvar create Event {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    SigId int
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar create DeferredEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar partition R50\
	Event {DomName ModelName EventName}\
	    DeferredEvent {DomName ModelName EventName}\
	    EffectiveEvent {DomName ModelName EventName}

    relvar create PolymorphicEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar create InheritedEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar partition R51\
	DeferredEvent {DomName ModelName EventName}\
	    PolymorphicEvent {DomName ModelName EventName}\
	    InheritedEvent {DomName ModelName EventName}

    relvar create MappedEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    ParentModel string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar association R53\
	MappedEvent {DomName ParentModel EventName} *\
	DeferredEvent {DomName ModelName EventName} 1

    relvar create LocalEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar partition R52\
	EffectiveEvent {DomName ModelName EventName}\
	    MappedEvent {DomName ModelName EventName}\
	    LocalEvent {DomName ModelName EventName}

    relvar create NonLocalEvent {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    RelName string
	    RoleId int
	} {
	    {DomName ModelName EventName}
	}
    }

    relvar partition R54\
	NonLocalEvent {DomName ModelName EventName}\
	    MappedEvent {DomName ModelName EventName}\
	    InheritedEvent {DomName ModelName EventName}

    relvar association R56\
	NonLocalEvent {DomName ModelName RelName RoleId} *\
	SubtypeRole {DomName ClassName RelName RoleId} 1\

    relvar create DeferralPath {
	Relation {
	    DomName string
	    ModelName string
	    EventName string
	    RelName string
	    RoleId int
	} {
	    {DomName ModelName EventName RelName RoleId}
	}
    }

    relvar correlation R55 DeferralPath\
	{DomName ModelName EventName} +\
	    DeferredEvent {DomName ModelName EventName}\
	{DomName ModelName RelName RoleId} *\
	    SupertypeRole {DomName ClassName RelName RoleId}

    relvar create Signature {
	Relation {
	    DomName string
	    ModelName string
	    SigId int
	} {
	    {DomName ModelName SigId}
	}
    }

    relvar create SignatureParam {
	Relation {
	    DomName string
	    ModelName string
	    SigId int
	    ParamName string
	    ParamOrder int
	} {
	    {DomName ModelName SigId ParamName}
	    {DomName ModelName SigId ParamOrder}
	}
    }

    relvar association R60\
	StateTrans {DomName ModelName SigId} *\
	Signature {DomName ModelName SigId} 1

    relvar association R62\
	SignatureParam {DomName ModelName SigId} *\
	Signature {DomName ModelName SigId} 1

    relvar association R63\
	Event {DomName ModelName SigId} *\
	Signature {DomName ModelName SigId} 1

    relvar association R64\
	State {DomName ModelName SigId} *\
	Signature {DomName ModelName SigId} 1


    # This defines a procedural constraint to insure that the signature
    # for a state transition matches both the signature of the event
    # and the signature for the state. This is a constrained loop of
    # relationships.
    # R60=R45+R41.EffectiveEvent+R50+R63
    # and
    # R60=R46+R64
    relvar trace add variable StateTrans insert [namespace code sameSig]
    proc sameSig {op relvarName tupleValue} {
	set trans [tuple relation $tupleValue\
		{{DomName ModelName EventName StateName}}]
	#puts [relformat $trans trans]

	# See what we get if we just directly semijoin across the composite
	# relationship.
	set transSig [relation semijoin $trans $::raloo::mm::Signature]
	#puts [relformat $transSig transSig]

	# Now traverse the R45+R41+R50+R63 path
	set eventSig [relation semijoin $trans\
	    $::raloo::mm::TransitionPlace\
	    $::raloo::mm::EffectiveEvent\
	    $::raloo::mm::Event\
	    $::raloo::mm::Signature\
	]
	#puts [relformat $eventSig eventSig]

	# Now traverse the R46+R64 path
	# N.B. be careful here. The join is to the destination of the
	# transition, i.e. from State Trans.NewState -> State.StateName
	set stateSig [relation semijoin $trans\
	    $::raloo::mm::State\
		-using {DomName DomName ModelName ModelName NewState StateName\
		    SigId SigId}\
	    $::raloo::mm::Signature\
	]
	#puts [relformat $stateSig stateSig]
	#puts [relformat $::raloo::mm::Signature Signature]
	#puts [relformat $::raloo::mm::SignatureParam SignatureParam]
	#puts [relformat $::raloo::mm::Event Event]
	#puts [relformat $::raloo::mm::State State]

	# Now compare to see if we got the same instance regardless of
	# the path by which we got there.
	if {[relation is $eventSig != $transSig]} {
	    #puts [relformat $::raloo::mm::TransitionPlace TransitionPlace]
	    #puts [relformat $::raloo::mm::EffectiveEvent EffectiveEvent]
	    #puts [relformat $::raloo::mm::Event Event]
	    #puts [relformat $::raloo::mm::Signature Signature]
	    relation assign $trans
	    error "for the transition,\
		\"$StateName - $EventName -> $NewState\",\
		the parameter signature for the event,\
		\"$EventName [list [formatSig $eventSig]]\",\
		does not match the signature of the destination state,\
		\"$NewState [list [formatSig $transSig]]\""
	}
	# This may never trigger since the signature id of a transition
	# is set to be the same as that of the destination state when the
	# StateTrans tuple is inserted. However, we leave it just in case
	# we change our minds later about how StateTrans tuples are inserted.
	if {[relation is $stateSig != $transSig]} {
	    #puts [relformat $::raloo::mm::State State]
	    #puts [relformat $::raloo::mm::Signature Signature]
	    relation assign $trans
	    error "for the transition,\
		\"$StateName - $EventName -> $NewState\",\
		the parameter signature for the destination state,\
		\"$NewState [list [formatSig $stateSig]]\",\
		does not match the signature of event,\
		\"$EventName [list [formatSig $transSig]]\""
	}

	return $tupleValue
    }

    # Utility proc to turn a signature into an Tcl style argument list.
    # "sig" is a singleton relation value.
    proc formatSig {sig} {
	set result [list]
	set params [relation semijoin $sig $::raloo::mm::SignatureParam]
	relation foreach param $params -ascending ParamOrder {
	    lappend result [relation extract $param ParamName]
	}
	return $result
    }


################################################################################

    # This relvar is used to hold system generated unique identifiers.
    relvar create __uniqueids__ {
	Relation {
	    RelvarName string IdAttr string IdNum int
	} {
	    {RelvarName IdAttr}
	}
    }

    # A convenience proc that installs a trace on an attribute of
    # a relvar in order to have the system assign a unique value
    # to that attribute.
    proc uniqueId {relvarName attrName} {
	relvar insert __uniqueids__ [list\
	    RelvarName $relvarName\
	    IdAttr $attrName\
	    IdNum 0
	]
	relvar trace add variable $relvarName insert\
	    [namespace code [list uniqueIdInsertTrace $attrName]]
    }

    # This proc can be used in a relvar trace on insert to
    # create a unique identifier for an attribute.
    proc uniqueIdInsertTrace {attrName op relvarName tup} {
	# Retrieve the Id number and update the stored value
	# We need the "relvar eval" here because this ends up being are
	# recursive invocation and TclRAL requires an ongoing transaction to
	# do recursive relvar commands (in case something fails).
	relvar eval {
	    relvar updateone __uniqueids__ uid\
		    [list RelvarName $relvarName IdAttr $attrName] {
		set idValue [tuple extract $uid IdNum]
		tuple update uid IdNum [incr idValue]
	    }
	}
	# Return the tuple to insert with the Id value.
	return [tuple update tup $attrName $idValue]
    }

    proc deleteStateModel {domName modelName} {
	relvar deleteone ::raloo::mm::StateModel\
	    DomName $domName ModelName $modelName
	relvar delete ::raloo::mm::StatePlace co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::State co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar deleteone ::raloo::mm::CreationState\
	    DomName $domName ModelName $modelName
	relvar delete ::raloo::mm::ActiveState co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::DeadState co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
################################################################################
	relvar delete ::raloo::mm::Event co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::DeferredEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::EffectiveEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::PolymorphicEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::InheritedEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::MappedEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::LocalEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::NonLocalEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::DeferredEvent co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	########################################################################
	relvar delete ::raloo::mm::TransitionPlace co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::StateTrans co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::NonStateTrans co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::NewStateTrans co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::CreationTrans co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::Signature co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
	relvar delete ::raloo::mm::SignatureParam co {
	    [tuple extract $co DomName] eq $domName &&\
	    [tuple extract $co ModelName] eq $modelName
	}
    }
}

# Domain class.
# Instances of a Domain are encapsuled sets of classes and relationships.
# Domain Operations provide the explicit interface to a domain.
oo::class create ::raloo::Domain {
    # We want all Domains to be given a name. No anonymous Domains.
    self.unexport new
    # We overload the create method so that each domain may be created in
    # a namespace that corresponds to its name.
    self.method create {name {script {}}} {
	# If the domain name is given as relative, make the domain in
	# the namespace of the caller.
	if {[string range $name 0 1] ne "::"} {
	    set name [uplevel 1 namespace current]::$name
	}
	# Make sure the namespace does not already exist. The docs say
	# that "createWithNamespace" will choose an arbitrary name if
	# the requested namespace already exists. We don't want that.
	if {[namespace exists $name]} {
	    error "A namespace with name, \"$name\", already exists\
		and so a Domain by the same name may not be created."
	}
	# This creates it in the given namespace and runs the constructor.
	return [my createWithNamespace $name $name $script]
    }

    constructor {script} {
	my eval namespace import ::ral::*
	my variable domName
	set domName [self]

	relvar insert ::raloo::mm::Domain [list\
	    DomName [self]\
	]
	my define $script
    }

    destructor {
	foreach inst\
		[info class instances ::raloo::SingleAssignerRel [self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::MultipleAssignerRel [self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::SingleAssignerAssocRel\
		[self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::MultipleAssignerAssocRel\
		[self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::AssocRelationship [self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::Relationship [self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::Generalization [self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::PasvRelvarClass [self]::*] {
	    $inst destroy
	}
	foreach inst\
		[info class instances ::raloo::ActiveRelvarClass [self]::*] {
	    $inst destroy
	}
	# Clean up the meta-model data
	if {[catch {
	    relvar eval {
		relvar delete ::raloo::mm::DomainOp d {
		    [tuple extract $d DomName] eq [self]
		}
		relvar delete ::raloo::mm::SyncService d {
		    [tuple extract $d DomName] eq [self]
		}
		relvar deleteone ::raloo::mm::Domain DomName [self]
	    }} result]} {
	    puts "Domain: $result"
	}
    }

    method define {{script {}}} {
	# Invoke the script arranging for defining procs in the script
	# to be aliased to unexported methods. This is just a convenience
	# so that the definition script does not have to contain a set of
	# "my" commands preceding the definition procs.
	my DefineWith Domain $script\
	    DomainOp\
	    SyncService\
	    Class\
	    Relationship\
	    Generalization\
	    AssocRelationship\
	    Polymorphic
    }
    method transaction {script} {
	my Transact $script
    }
    method Transact {script} {
	catch {
	    relvar eval {
		uplevel 1 $script
	    }
	} result options
	::raloo::arch::cleanupRefs
	return -options $options $result
    }
    method SyncService {name argList body} {
	relvar insert ::raloo::mm::SyncService [list\
	    DomName [self]\
	    ServiceName $name\
	    Params $argList\
	    Body $body\
	]
	oo::define [self] method $name $argList [list my Transact $body]
	oo::define [self] export $name
    }
    # Methods that are used in the definition of a domain during construction.
    method DomainOp {name argList body} {
	relvar insert ::raloo::mm::DomainOp [list\
	    DomName [self]\
	    OpName $name\
	    OpParams $argList\
	    OpBody $body\
	]

	oo::define [self] method $name $argList [format {
	    ::raloo::arch::newTOC %1$s [self] {Relation {} {{}} {{}}}\
		[list my __%2$s] [lrange [info level 0] 2 end]
	} [self] $name]
	oo::define [self] export $name

	oo::define [self] method __$name $argList $body
	oo::define [self] unexport __$name
	return
    }
    # Interpret the definition of a class.
    method Class {name script} {
	if {[string match {*::*} $name]} {
	    error "class names may not have namespace separators, \"$name\""
	}
	my variable domName
	my variable className
	set className $name
	# Define the class into the meta-model and then evaluate the
	# class definition. The class definition will invoke other methods
	# to populate the meta-model.
	relvar eval {
	    relvar insert ::raloo::mm::Class [list\
		DomName $domName\
		ClassName $name\
	    ]
	    my DefineWith Class $script\
		Attribute\
		Lifecycle\
		InstOp\
		ClassOp
	}
	# If we come out of the class definition without any errors, then
	# we can now create the relvar that represents the class.
	# If a lifecycle was defined for the class, then we create an
	# active class. Otherwise a passive one is fine.
	set classType [expr {[relation isempty\
	    [relation choose $::raloo::mm::InstStateModel DomName $domName\
		ClassName $className]] ?\
	    "PasvRelvarClass" : "ActiveRelvarClass"}]
	::raloo::$classType create $className

	return
    }
    # Simple relationship.
    # Syntax is : refng X-->Y refto {
    #	RefMap {...}
    #	Assigner {...}
    # }
    method Relationship {name refngClassName spec refToClassName {script {}}} {
	if {![regexp -- {([1?*+])-*>*([1?])} $spec\
		match refngCard refToCard]} {
	    error "unrecognized relationship specification, \"$spec\""
	}

	set refngRoleId 0
	set refToRoleId 1
	# Determine the identifier which is referred to in the relationship.
	set gotId [my ParseClassName $refToClassName refToClassName refToIdNum]
	my variable domName
	my variable relName
	set relName $name
	relvar eval {
	    relvar insert ::raloo::mm::Relationship [list\
		DomName $domName\
		RelName $name\
	    ]
	    relvar insert ::raloo::mm::SimpleRel [list\
		DomName $domName\
		RelName $name\
	    ]
	    relvar insert ::raloo::mm::SimpleReferring [list\
		DomName $domName\
		RelName $name\
		ClassName $refngClassName\
		RoleId $refngRoleId\
		Cardinality $refngCard\
	    ]
	    set refngClass [relvar insert ::raloo::mm::ReferringClass [list\
		DomName $domName\
		RelName $name\
		ClassName $refngClassName\
		RoleId $refngRoleId\
	    ]]
	    relvar insert ::raloo::mm::ClassRoleInRel [list\
		DomName $domName\
		RelName $name\
		ClassName $refngClassName\
		RoleId $refngRoleId\
	    ]
	    relvar insert ::raloo::mm::SimpleRefTo [list\
		DomName $domName\
		RelName $name\
		ClassName $refToClassName\
		RoleId $refToRoleId\
		Cardinality $refToCard\
	    ]
	    set reftoClass [relvar insert ::raloo::mm::RefToClass [list\
		DomName $domName\
		RelName $name\
		ClassName $refToClassName\
		RoleId $refToRoleId\
	    ]]
	    relvar insert ::raloo::mm::ClassRoleInRel [list\
		DomName $domName\
		RelName $name\
		ClassName $refToClassName\
		RoleId $refToRoleId\
	    ]
	    my variable attrRefMap
	    set attrRefMap [relation create\
		{RefToAttrName string RefngAttrName string}\
		{RefToAttrName RefngAttrName}]
	    my DefineWith Relationship $script\
		RefMap\
		Assigner

	    my MakeClassReferences $refngClass $reftoClass $gotId $refToIdNum\
		$attrRefMap
	}
	# Check for any assigner behavior
	if {[relation isempty [relation choose $::raloo::mm::AssignerStateModel\
		DomName $domName RelName $relName]]} {
	    ::raloo::Relationship create ${domName}::$name
	} elseif {[relation isnotempty\
	       [relation choose $::raloo::mm::SingleAssigner\
		DomName $domName RelName $relName]]} {
	    ::raloo::SingleAssignerRel create ${domName}::$name
	} elseif {[relation isnotempty\
		[relation choose $::raloo::mm::MultipleAssigner\
		DomName $domName RelName $relName]]} {
	    ::raloo::MultipleAssignerRel create ${domName}::$name
	}

	return
    }
    # "refList" is of the form:
    # refering -> referredTo
    # parse the refList, which is a list, and create the implied mapping.
    method RefMap {refList} {
	my variable attrRefMap
	set attrRefMap [relation union $attrRefMap [my MakeAttrRefMap $refList]]
	return
    }
    method Generalization {name supertype script} {
	my variable domName
	my variable relName
	set relName $name
	my variable superName
	my variable superIdNum
	my variable superIdGiven
	set superIdGiven [my ParseClassName $supertype superName superIdNum]
	relvar eval {
	    relvar insert ::raloo::mm::Relationship [list\
		DomName $domName\
		RelName $name\
	    ]
	    relvar insert ::raloo::mm::GenRel [list\
		DomName $domName\
		RelName $name\
	    ]
	    relvar insert ::raloo::mm::SupertypeRole [list\
		DomName $domName\
		RelName $name\
		ClassName $superName\
		RoleId 0\
	    ]
	    my variable superClass
	    set superClass [relvar insert ::raloo::mm::RefToClass [list\
		DomName $domName\
		ClassName $superName\
		RelName $name\
		RoleId 0\
	    ]]
	    relvar insert ::raloo::mm::ClassRoleInRel [list\
		DomName $domName\
		ClassName $superName\
		RelName $name\
		RoleId 0\
	    ]

	    my variable refToIdAttribute
	    set refToIdAttribute\
		    [my MakeRefToIdAttribute $superClass $superIdNum]

	    my variable subRoleId
	    set subRoleId 1
	    my DefineWith Generalization $script SubType
	}
	::raloo::Generalization create ${domName}::$name
	return
    }

    # If no script is present, then names are assumed to be the same
    # as those of the identifier in the supertype, otherwise there must be a
    # script to define the referential associations.
    method SubType {subName {script {}}} {
	my variable domName
	my variable relName
	set matchingSubtype [relation restrictwith $::raloo::mm::SubtypeRole {
		$DomName eq $domName && $RelName eq $relName &&
		$ClassName eq $subName}]
	if {[relation isnotempty $matchingSubtype]} {
	    error "duplicate subtype class, \"$subName\""
	}

	my variable subRoleId
	relvar insert ::raloo::mm::SubtypeRole [list\
	    DomName $domName\
	    RelName $relName\
	    ClassName $subName\
	    RoleId $subRoleId\
	]
	set refngClass [relvar insert ::raloo::mm::ReferringClass [list\
	    DomName $domName\
	    RelName $relName\
	    ClassName $subName\
	    RoleId $subRoleId\
	]]
	relvar insert ::raloo::mm::ClassRoleInRel [list\
	    DomName $domName\
	    RelName $relName\
	    ClassName $subName\
	    RoleId $subRoleId\
	]
	my variable refToIdAttribute
	if {$script eq {}} {
	    # No script ==> the same attribute names ==> the identity mapping.
	    my MakeAttributeRef $refngClass $refToIdAttribute\
		[my MakeIdentityRefMap\
		    [relation list $refToIdAttribute AttrName]]
	} else {
	    # Make a relation out of the script information.
	    set attrMapRel [my MakeAttrRefMap $script]
	    # Find the identifier number.
	    my variable superClass
	    set idNum [my FindRefToIdNum $superClass $attrMapRel]
	    my variable superName
	    my variable superIdNum
	    my variable superIdGiven
	    if {$superIdGiven && $idNum != $superIdNum} {
		error "referring attribute(s), \"[join\
		    [relation list $attrMapRel RefngAttrName] {, }]\",\
		    do(es) not refer to identifier,\
		    \"$superIdNum\" of class,\"$superName\""
	    }
	    my MakeAttributeRef $refngClass $refToIdAttribute $attrMapRel
	}

	incr subRoleId
	return
    }

    # Syntax is : source X-assoc->Y target
    # source and target can be of the form *DName to indicate
    # the identifier number that is being referred to.
    method AssocRelationship {name sourceClassName spec targetClassName\
	    {script {}}} {
	set gotSourceId [my ParseClassName $sourceClassName sourceClassName\
	    sourceIdNum]
	set gotTargetId [my ParseClassName $targetClassName targetClassName\
	    targetIdNum]
	if {![regexp -- {([1?*+])-*(\w+)-*>?([1?*+])} $spec\
		match sourceCard assocClassName targetCard]} {
	    error "unrecognized associative relationship specification,\
		\"$spec\""
	}
	my variable domName
	my variable relName
	set relName $name
	relvar eval {
	    relvar insert ::raloo::mm::Relationship [list\
		DomName $domName\
		RelName $name\
	    ]
	    relvar insert ::raloo::mm::AssocRel [list\
		DomName $domName\
		RelName $name\
	    ]
	    relvar insert ::raloo::mm::Associator [list\
		DomName $domName\
		RelName $name\
		ClassName $assocClassName\
		RoleId 0\
	    ]
	    set assocClass [relvar insert ::raloo::mm::ReferringClass [list\
		DomName $domName\
		RelName $name\
		ClassName $assocClassName\
		RoleId 0\
	    ]]
	    relvar insert ::raloo::mm::ClassRoleInRel [list\
		DomName $domName\
		RelName $name\
		ClassName $assocClassName\
		RoleId 0\
	    ]
	    relvar insert ::raloo::mm::AssocSource [list\
		DomName $domName\
		RelName $name\
		ClassName $sourceClassName\
		RoleId 1\
		Cardinality $sourceCard\
	    ]
	    set sourceClass [relvar insert ::raloo::mm::RefToClass [list\
		DomName $domName\
		RelName $name\
		ClassName $sourceClassName\
		RoleId 1\
	    ]]
	    relvar insert ::raloo::mm::ClassRoleInRel [list\
		DomName $domName\
		RelName $name\
		ClassName $sourceClassName\
		RoleId 1\
	    ]
	    relvar insert ::raloo::mm::AssocTarget [list\
		DomName $domName\
		RelName $name\
		ClassName $targetClassName\
		RoleId 2\
		Cardinality $targetCard\
	    ]
	    set targetClass [relvar insert ::raloo::mm::RefToClass [list\
		DomName $domName\
		RelName $name\
		ClassName $targetClassName\
		RoleId 2\
	    ]]
	    relvar insert ::raloo::mm::ClassRoleInRel [list\
		DomName $domName\
		RelName $name\
		ClassName $targetClassName\
		RoleId 2\
	    ]
	    my variable frwdAttrRefMap
	    my variable backAttrRefMap
	    set frwdAttrRefMap [relation create\
		{RefToAttrName string RefngAttrName string}\
		{RefToAttrName RefngAttrName}]
	    set backAttrRefMap $frwdAttrRefMap
	    my DefineWith AssocRelationship $script\
		FwrdRefMap\
		BackRefMap\
		Assigner

	    # For reflexive relationship it is not allowed to have both the
	    # forward and backward reference maps empty. The inherent ambiguity
	    # must be resolved by at least one reference map.
	    if {$sourceClassName eq $targetClassName &&\
		[relation isempty $frwdAttrRefMap] &&\
		[relation isempty $backAttrRefMap]} {
		error "reflexive relationships require at least one reference\
		    map definition to resolve the ambiguity"
	    }

	    # First we resolve the forward direction
	    my MakeClassReferences $assocClass $targetClass $gotTargetId\
		$targetIdNum $frwdAttrRefMap
	    # Then the backwards direction.
	    my MakeClassReferences $assocClass $sourceClass $gotSourceId\
		$sourceIdNum $backAttrRefMap
	}
	# Check for any assigner behavior
	if {[relation isempty [relation choose $::raloo::mm::AssignerStateModel\
		DomName $domName RelName $relName]]} {
	    ::raloo::AssocRelationship create ${domName}::$name
	} elseif {[relation isnotempty\
	       [relation choose $::raloo::mm::SingleAssigner\
		DomName $domName RelName $relName]]} {
	    ::raloo::SingleAssignerAssocRel create ${domName}::$name
	} elseif {[relation isnotempty\
		[relation choose $::raloo::mm::MultipleAssigner\
		DomName $domName RelName $relName]]} {
	    ::raloo::MultipleAssignerAssocRel create ${domName}::$name
	}
    }
    method FwrdRefMap {refList} {
	my variable frwdAttrRefMap
	set frwdAttrRefMap [relation union $frwdAttrRefMap\
	    [my MakeAttrRefMap $refList]]
	return
    }
    method BackRefMap {refList} {
	my variable backAttrRefMap
	set backAttrRefMap [relation union $backAttrRefMap\
	    [my MakeAttrRefMap $refList]]
	return
    }

    # Method to define polymorphic events
    method Polymorphic {className eventName signature} {
	my variable domName
	relvar eval {
	    set sig [my FindSig $domName $className $signature]
	    set sigId [relation extract $sig SigId]
	    #puts "sigId = $sigId"
	    relvar insert ::raloo::mm::Event [list\
		DomName $domName\
		ModelName $className\
		EventName $eventName\
		SigId $sigId\
	    ]
	    set defEvt [relvar insert ::raloo::mm::DeferredEvent [list\
		DomName $domName\
		ModelName $className\
		EventName $eventName\
	    ]]
	    relvar insert ::raloo::mm::PolymorphicEvent [list\
		DomName $domName\
		ModelName $className\
		EventName $eventName\
	    ]
	    # Find all the generalization for which this class is a supertype.
	    set superRoles [relation restrictwith $::raloo::mm::SupertypeRole {
		$DomName eq $domName && $ClassName eq $className
	    }]

	    # The DeferralPath is just a join of the deferred event and
	    # all of the super type roles played by the given class.
	    relvar union ::raloo::mm::DeferralPath [relation join\
		$defEvt $superRoles\
		-using {DomName DomName ModelName ClassName}]
	    #puts [relformat $::raloo::mm::DeferralPath DeferralPath]
	    # Find all the subtypes for this supertype. That is all the
	    # subtypes from any generalization for which the given class
	    # is a supertype.
	    set subroles [relation semijoin $superRoles\
		    $::raloo::mm::GenRel $::raloo::mm::SubtypeRole]
	    #puts [relformat $subroles subroles]
	    relation foreach subrole $subroles {
		my AddNonLocalEvent $className $subrole $eventName $signature
	    }
	}
	oo::define ${domName}::$className superclass ::raloo::ActiveRelvarRef
    }
    method Assigner {script} {
	my variable domName
	my variable relName
	my variable modelName
	set modelName $relName
	relvar insert ::raloo::mm::StateModel [list\
	    DomName $domName\
	    ModelName $modelName\
	    InitialState {}\
	    DefaultTrans CH\
	]
	relvar insert ::raloo::mm::AssignerStateModel [list\
	    DomName $domName\
	    RelName $relName\
	]
	relvar insert ::raloo::mm::SingleAssigner [list\
	    DomName $domName\
	    RelName $relName\
	]

	my DefineWith Assigner $script\
	    IdentifyBy\
	    State\
	    Transition\
	    DefaultTransition\
	    DefaultInitialState
	return
    }

    # Define a multiple assigner
    # We assume assigners are single until this method is called,
    # so we subtype migrate single assigners into a multple one.
    method IdentifyBy {partitionClass} {
	# Parse the class and identifier for the multiple assigner.
	my ParseClassName $partitionClass idClassName idNum
	# Subtype migrate
	my variable domName
	my variable relName
	relvar deleteone ::raloo::mm::SingleAssigner\
	    DomName $domName RelName $relName
	relvar insert ::raloo::mm::MultipleAssigner [list\
	    DomName $domName\
	    RelName $relName\
	    ClassName $idClassName\
	    IdNum $idNum\
	]
    }

    # Determine if the subtype given by "subRole" has any events
    # defined for it that match "eventName". If so, then that event
    # is subtype migrated to a MappedEvent. Otherwise, the event becomes
    # an InheritedEvent and we recursively descend the attached generalization
    # hierarchies until it is consumed or we encounter a leaf subtype.
    # Attempting to pass down an inherited event to a leaf subtype is
    # an error.
    method AddNonLocalEvent {parent subRole eventName signature} {
	my variable domName
	relation assign $subRole
	set local [relation choose $::raloo::mm::LocalEvent\
	    DomName $domName ModelName $ClassName EventName $eventName]
	if {[relation isnotempty $local]} {
	    # Found a local event by the same name as the polymorphic
	    # event being pressed downwards. Subtype migrate the local event
	    # to a mapped event. This event is considered consumed at this
	    # level.
	    relvar deleteone ::raloo::mm::LocalEvent\
		DomName $domName ModelName $ClassName EventName $eventName
	    relvar insert ::raloo::mm::MappedEvent [list\
		DomName $domName\
		ModelName $ClassName\
		EventName $eventName\
		ParentModel $parent\
	    ]
	} else {
	    # The event does not match one in this subtype. The event becomes
	    # inherited. This implies that this subtype is also the super
	    # type of some other generalization relationship.
	    set superRoles [relation restrict $::raloo::mm::SupertypeRole sr {
		[tuple extract $sr DomName] eq $domName &&\
		[tuple extract $sr ClassName] eq $ClassName
	    }]
	    if {[relation isempty $superRoles]} {
		error "class, \"$ClassName\", inherits event, \"$eventName\",\
		    from super type, \"$parent\", and neither consumes the\
		    event nor is itself a supertype in another generalization"
	    }
	    # Add the inherited event.
	    set sig [my FindSig $domName $ClassName $signature]
	    set sigId [relation extract $sig SigId]
	    relvar insert ::raloo::mm::Event [list\
		DomName $domName\
		ModelName $ClassName\
		EventName $eventName\
		SigId $sigId\
	    ]
	    set defEvt [relvar insert ::raloo::mm::DeferredEvent [list\
		DomName $domName\
		ModelName $ClassName\
		EventName $eventName\
	    ]]
	    relvar insert ::raloo::mm::InheritedEvent [list\
		DomName $domName\
		ModelName $ClassName\
		EventName $eventName\
	    ]
	    relvar union ::raloo::mm::DeferralPath [relation join\
		$defEvt $superRoles\
		-using {DomName DomName ModelName ClassName}]
	    #puts [relformat $::raloo::mm::DeferralPath DeferralPath]
	    # Now we must recursively decend to all the subtypes of this
	    # class, resolving the inherited events.
	    set subroles [relation semijoin $superRoles\
		    $::raloo::mm::GenRel $::raloo::mm::SubtypeRole]
	    relation foreach subrole $subroles {
		my AddNonLocalEvent $ClassName $subrole $eventName $signature
	    }
	}
	# Both MappedEvent and InheritedEvent are NonLocalEvent.
	relvar insert ::raloo::mm::NonLocalEvent [list\
	    DomName $domName\
	    ModelName $ClassName\
	    EventName $eventName\
	    RelName $RelName\
	    RoleId $RoleId\
	]
	#puts [relformat $::raloo::mm::DeferredEvent DeferredEvent]
	#puts [relformat $::raloo::mm::NonLocalEvent NonLocalEvent]
	#puts [relformat $::raloo::mm::InheritedEvent InheritedEvent]
	#puts [relformat $::raloo::mm::MappedEvent MappedEvent]
    }

    # Methods used to define classes.
    method Attribute {nameTypeList} {
	if {[llength $nameTypeList] % 2 != 0} {
	    error "attributes must be specified as name / value pairs:\
		    \"$nameTypeList\""
	}
	my variable domName
	my variable className
	foreach {name attrType} $nameTypeList {
	    set idNumList [my ParseAttrName $name attrName]
	    relvar insert ::raloo::mm::Attribute [list\
		DomName $domName\
		ClassName $className\
		AttrName $attrName\
		AttrType $attrType\
	    ]
	    foreach idNum $idNumList {
		relvar union ::raloo::mm::Identifier [relation create\
		    {DomName string ClassName string IdNum int}\
		    {{DomName ClassName IdNum}} [list\
		    DomName $domName\
		    ClassName $className\
		    IdNum $idNum\
		]]
		relvar insert ::raloo::mm::IdAttribute [list\
		    DomName $domName\
		    ClassName $className\
		    IdNum $idNum\
		    AttrName $attrName\
		]
	    }
	}
	return
    }
    method Lifecycle {script} {
	my variable domName
	my variable className
	my variable modelName
	set modelName $className
	relvar insert ::raloo::mm::StateModel [list\
	    DomName $domName\
	    ModelName $modelName\
	    InitialState {}\
	    DefaultTrans CH\
	]
	relvar insert ::raloo::mm::InstStateModel [list\
	    DomName $domName\
	    ClassName $className\
	]

	my DefineWith StateModel $script\
	    State\
	    Transition\
	    DefaultTransition\
	    DefaultInitialState

	return
    }
    method State {name argList body {final false}} {
	my variable domName
	my variable modelName
	# If the default initial state is not already set, then
	# the first mentioned state becomes the default as long as it
	# it not a final state.
	set sm [relation choose $::raloo::mm::StateModel\
	    DomName $domName ModelName $modelName]
	if {[relation extract $sm InitialState] eq "" && !$final} {
	    relvar updateone ::raloo::mm::StateModel stup [list\
		    DomName $domName ModelName $modelName] {
		tuple update stup InitialState $name
	    }
	}
	set sig [my FindSig $domName $modelName $argList]
	relvar insert ::raloo::mm::StatePlace [list\
	    DomName $domName\
	    ModelName $modelName\
	    StateName $name\
	]
	relvar insert ::raloo::mm::State [list\
	    DomName $domName\
	    ModelName $modelName\
	    StateName $name\
	    Action $body\
	    SigId [relation extract $sig SigId]\
	]
	relvar insert\
	    ::raloo::mm::[expr {$final ? "DeadState" : "ActiveState"}] [list\
	    DomName $domName\
	    ModelName $modelName\
	    StateName $name\
	]
	return
    }
    method Transition {current - eventName -> new} {
	my variable domName
	my variable modelName
	# The creation state is known as "@".
	if {$current eq "@"} {
	    # If the creation state is mentioned, the add it in.
	    # We use "relvar union" to do this in case there are multiple
	    # creation transitions.
	    set csTuple [tuple create\
		{DomName string ModelName string StateName string}\
		[list DomName $domName ModelName $modelName StateName $current]]
	    relvar union ::raloo::mm::CreationState\
		[tuple relation $csTuple {{DomName ModelName}}]
	    relvar union ::raloo::mm::StatePlace\
		[tuple relation $csTuple {{DomName ModelName StateName}}]
	}
	# Each transition must start from a Transition Place, the conceptual
	# cell in the transition matrix.
	relvar insert ::raloo::mm::TransitionPlace [list\
	    DomName $domName\
	    ModelName $modelName\
	    EventName $eventName\
	    StateName $current\
	]
	# Now we must determine if this is a state transition or a non-state
	# transition (i.e. a CH or IG). So if the "new" state matches a
	# TransitionRule then we have a NonState Transition (see R45 in the
	# meta-model)
	if {[relation isnotempty\
		[relation choose $::raloo::mm::TransitionRule RuleName $new]]} {
	    relvar insert ::raloo::mm::NonStateTrans [list\
		DomName $domName\
		ModelName $modelName\
		EventName $eventName\
		StateName $current\
		TransRule $new\
	    ]
	} else {
	    # Find the signature of the state for which the transition is the
	    # target. This must be the signature for the event.
	    set state [relation choose $::raloo::mm::State\
		DomName $domName\
		ModelName $modelName\
		StateName $new\
	    ]
	    # This test isn't strictly necessary, since the "extract" below
	    # would fail it the state wasn't found. But the resulting error
	    # message is opaque.
	    if {[relation isempty $state]} {
		error "unknown new state, \"$new\""
	    }
	    set sigId [relation extract $state SigId]
	    # When defining transitions for a class, we assume that all the
	    # events are "LocalEvent". Later, if any polymorphism is defined
	    # we will subtype migrate the LocalEvent appropriately. Again we
	    # use "relvar union" here in case the event name is mentioned
	    # in multiple transitions.
	    set event [relation create\
		{DomName string ModelName string EventName string}\
		{{DomName ModelName EventName}}\
		[list\
		    DomName $domName\
		    ModelName $modelName\
		    EventName $eventName\
		]]
	    relvar union ::raloo::mm::Event [relation extend $event ev\
		    SigId int {$sigId}]
	    relvar union ::raloo::mm::EffectiveEvent $event
	    relvar union ::raloo::mm::LocalEvent $event
	    relvar insert ::raloo::mm::StateTrans [list\
		DomName $domName\
		ModelName $modelName\
		EventName $eventName\
		StateName $current\
		NewState $new\
		SigId $sigId\
	    ]
	    # For state transition, we must determine if this is a creation
	    # transition or not (corresponds to R47 in the meta-model).
	    relvar insert ::raloo::mm::[expr {$current eq "@" ?\
		    "CreationTrans" : "NewStateTrans"}] [list\
		DomName $domName\
		ModelName $modelName\
		EventName $eventName\
		StateName $current\
	    ]
	}

	return
    }
    method DefaultTransition {trans} {
	my variable domName
	my variable modelName
	relvar updateone ::raloo::mm::StateModel sm [list\
		DomName $domName ModelName $modelName] {
	    tuple update sm DefaultTrans $trans
	}
	return
    }
    method DefaultInitialState {state} {
	my variable domName
	my variable modelName
	relvar updateone ::raloo::mm::StateModel sm [list\
		DomName $domName ModelName $modelName] {
	    tuple update sm InitialState $state
	}
	return
    }
    method InstOp {name argList body} {
	my variable domName
	my variable className
	relvar insert ::raloo::mm::InstOp [list\
	    DomName $domName\
	    ClassName $className\
	    OpName $name\
	    OpParams $argList\
	    OpBody $body\
	]
	return
    }
    method ClassOp {name argList body} {
	my variable domName
	my variable className
	relvar insert ::raloo::mm::ClassOp [list\
	    DomName $domName\
	    ClassName $className\
	    OpName $name\
	    OpParams $argList\
	    OpBody $body\
	]
	return
    }

    # Utility method to define a set of aliases that map to unexported
    # methods that are used as definition procs during Domain definition.
    method DefineWith {level script args} {
	set oldPath [namespace path]
	set ns [namespace current]::${level}::__Helpers
	namespace eval $ns {}
	namespace path [concat $oldPath $ns]
	foreach defFunc $args {
	    interp alias\
		{} ${ns}::$defFunc\
		{} {*}[namespace code [list my $defFunc]]
	}
	my eval $script
	foreach defFunc $args {
	    interp alias {} ${ns}::$defFunc {}
	}
	namespace delete $ns
	namespace path $oldPath
    }

    method ParseAttrName {name {attrNameRef attrName}} {
	upvar 1 $attrNameRef attrName
	if {![regexp {\A((?:\*[1-9]?)*)(\w.+)\Z} $name match idMark\
		attrName]} {
	    error "unrecognized attribute name syntax, \"$name\""
	}
	# Drop the first "*" that is split off. It will always be the
	# empty string since "idMark" will be of the form "*..."
	set idList [list]
	foreach id [lrange [split $idMark *] 1 end] {
	    lappend idList [expr {$id eq {} ? 1 : $id}]
	}

	return $idList
    }

    method ParseClassName {name {classNameRef className} {idNumRef idNum}} {
	upvar 1 $classNameRef className
	upvar 1 $idNumRef idNum
	if {![regexp {\A(\*[1-9]?)?(\w.+)\Z} $name match idMark\
		className]} {
	    error "unrecognized class name syntax, \"$name\""
	}
	set idNum [string index $idMark end]
	if {![string is digit -strict $idNum]} {
	    set idNum 1
	}
	return [expr {$idMark ne {}}]
    }
    method MakeClassReferences {refngClass reftoClass gotId refToIdNum\
				attrRefMap} {
	# By default if no reference map is given, then we assume the the
	# referring attributes have the same names as the attributes to which
	# they refer.  Which identifier they refer to may be given by the
	# "*[1-9]" convention applied to the referred to class name.  If no "*"
	# id syntax is used, then "*1" is used.  If a reference map is present,
	# then it determines which identifier to use. If both a reference map
	# and the "*[1-9]" syntax are used, then the identifier specified by
	# both must match.
	if {[relation isempty $attrRefMap]} {
	    set refToIdAttribute [my MakeRefToIdAttribute $reftoClass\
		    $refToIdNum]
	    # Since we assume the names are the same, we use the identity
	    # mapping.
	    set attrRefMap [my MakeIdentityRefMap\
		    [relation list $refToIdAttribute AttrName]]
	} else {
	    # Find the identifier number implied by the script. It must
	    # match the one specified for the referred to class if the
	    # "*[1-9]" syntax was used.
	    set idNum [my FindRefToIdNum $reftoClass $attrRefMap]
	    if {$gotId && $refToIdNum != $idNum} {
		error "referring attribute(s), \"[join\
		    [relation list $attrRefMap RefngAttrName] {, }]\",\
		    do(es) not refer to identifier,\
		    \"$refToIdNum\" of class,\
		    \"[relation extract $reftoClass ClassName]\""
	    }
	    set refToIdAttribute [my MakeRefToIdAttribute $reftoClass $idNum]
	}
	my MakeAttributeRef $refngClass $refToIdAttribute $attrRefMap
    }

    method MakeRefToIdAttribute {refToClass idNum} {
	# Find the identifying attributes for the given "idNum".
	# Create the referred to identifying attributes
	set refToIdAttribute [ralutil::pipe {
	    relation semijoin $refToClass $::raloo::mm::IdAttribute |
	    relation restrictwith ~ {$IdNum == $idNum} |
	    relation join $refToClass ~
	}]
	relvar union ::raloo::mm::RefToIdAttribute $refToIdAttribute
	return $refToIdAttribute
    }

    # Create instances of RefToIdAttribute and AttributeRef that formalize the
    # referential attributes of a relationship.
    # "referringClass" is a singleton relation with the same heading as
    # ReferringClass and is the referring class in the relationship.
    # "refToIdAttribute" is a relation with the same heading as
    # RefToIdAttribute and is the set of identifying attributes that are
    # referred to in the relationship.
    # "refMap" is a binary relation mapping the referred to attributes
    # to the corresponding referring attribute names.
    method MakeAttributeRef {referringClass refToIdAttribute refMap} {
	# Pair up the references.
	# Rename the ReferringClass attribute to match those of the
	# AttributeRef.
	set referringClass [relation rename $referringClass\
	    ClassName RefngClassName\
	    RoleId RefngRoleId\
	]
	# Likewise for the RefToIdAttribute
	set refToIdAttribute [relation rename $refToIdAttribute\
	    ClassName RefToClassName\
	    RoleId RefToRoleId\
	    AttrName RefToAttrName\
	    IdNum RefToIdNum\
	]
	# The new tuples of AttributeRef are just the join of the
	# ReferringClass and RefToIdAttributes with the addition of the
	# referring attribute name that implements the references.  That
	# attribute name comes from the "refMap"
	set attrRefs [ralutil::pipe {
	    relation join $referringClass $refToIdAttribute |
	    relation extend ~ af RefngAttrName string {
		[relation extract\
		    [relation choose $refMap RefToAttrName\
			[tuple extract $af RefToAttrName]]\
		    RefngAttrName]
	    }
	}]
	# Check if all the attribute refs are actually attributes.  The
	# referential integrity checks would find this, but we want to have a
	# better user error message.
	set refngAttrs [relation semijoin $attrRefs $::raloo::mm::Attribute\
	    -using {DomName DomName RefngClassName ClassName\
		RefngAttrName AttrName}]
	if {[relation cardinality $attrRefs] !=
		[relation cardinality $refngAttrs]} {
	    set refngAttrs [relation eliminate $refngAttrs AttrType]
	    set missing [ralutil::pipe {
		relation project $attrRefs\
			DomName RefngClassName RefngAttrName |
		relation rename ~\
			RefngClassName ClassName RefngAttrName AttrName |
		relation minus ~ $refngAttrs |
		relation list ~ AttrName
	    }]
	    error "\"[join $missing {, }]\", is(are) not attributes of class,\
		\"[relation extract $referringClass RefngClassName]\""
	}
	relvar union ::raloo::mm::AttributeRef $attrRefs
    }

    # "script" is of the form:
    # refering -> referredTo
    # parse the script, which is a list, and create the implied mapping.
    method MakeAttrRefMap {script} {
	if {[llength $script] % 3 != 0} {
	    error "formalizing attribute references must be in pairs:\n\
		    \"$script\""
	}
	foreach {refngAttrName x reftoAttrName} $script {
	    lappend mapTuples [list\
		RefToAttrName $reftoAttrName\
		RefngAttrName $refngAttrName]
	}
	return [relation create\
	    {RefToAttrName string RefngAttrName string}\
	    {RefToAttrName RefngAttrName} {*}$mapTuples]
    }
    # Make a relation that is the identity mapping of referring / referred
    # attributes.
    method MakeIdentityRefMap {attrList} {
	foreach attr $attrList {
	    lappend mapTuples [list RefToAttrName $attr RefngAttrName $attr]
	}
	return [relation create\
	    {RefToAttrName string RefngAttrName string}\
	    {RefToAttrName RefngAttrName} {*}$mapTuples]
    }

    # Find out the identifier to which the referred to attributes belongs. They
    # must all belong to the same one and all attributes in the identifier must
    # be present.  We do this with a relational divide, i.e. we are looking for
    # that identifier that contains all of the referred to attributes in the
    # mapping.
    method FindRefToIdNum {refToClass attrRefMap} {
	set reftoset [ralutil::pipe {
	    relation project $attrRefMap RefToAttrName |
	    relation rename ~ RefToAttrName AttrName
	}]
	set idAttribute [ralutil::pipe {
	    relation semijoin $refToClass $::raloo::mm::IdAttribute |
	    relation project ~ IdNum AttrName
	}]
	set possIds [relation project $idAttribute IdNum]
	set idNumRel [relation divide $possIds $reftoset $idAttribute]

	if {[relation cardinality $idNumRel] != 1} {
	    error "attribute(s),\
		\"[join [relation list $reftoset] {, }]\", do(es) not\
		constitute an identifier of,\
		\"[relation extract $refToClass ClassName]\""
	}

	return [relation extract $idNumRel IdNum]
    }

    # Find a signature that matches the argList. If one does not
    # exist, then create one. Otherwise, reuse the same signature. It
    # is important that the same argument list yield the same signature
    # so that we can match event parameters to state parameters.
    method FindSig {domName modelName argList} {
	# This will be a relational divide operation using the projection
	# of SigId from SignatureParam and a relation value built up from
	# the argument list.
	set paramTuples [list]
	set order -1
	foreach paramName $argList {
	    lappend paramTuples [list DomName $domName ModelName $modelName\
		ParamName $paramName ParamOrder [incr order]]
	}
	set dsor [relation create\
	    {DomName string ModelName string ParamName string\
		ParamOrder int}\
	    {{DomName ModelName ParamName} {DomName ModelName ParamOrder}}\
	    {*}$paramTuples]
	#puts [relformat $dsor dsor]
	# Find the signature Id which matches all of the parameters.
	set quot [ralutil::pipe {
	    relation project $::raloo::mm::SignatureParam SigId |
	    relation divide ~ $dsor $::raloo::mm::SignatureParam
	}]
	#puts [relformat $quot quot]
	# If none match, then create one.
	# N.B. when argList is empty, "dsor" is an empty relation and
	# the relational divide gives "quot" == dividend as all the dividend
	# tuples match the empty relation. Hence when the divisor is empty
	# we need to determine if there is already defined an "null" signature.
	my variable signatureIdCounter
	if {[relation isempty $dsor]} {
	    # The null signature is that one which has no parameters.
	    set sig [ralutil::pipe {
		relation semiminus $::raloo::mm::SignatureParam\
		    $::raloo::mm::Signature |
		relation restrictwith ~ {
		    $DomName eq $domName && $ModelName eq $modelName
		}
	    }]
	    #puts [relformat $sig sig]
	    if {[relation isempty $sig]} {
		set sig [relvar insert ::raloo::mm::Signature [list\
		    DomName $domName\
		    ModelName $modelName\
		    SigId [incr signatureIdCounter]\
		]]
	    }
	} elseif {[relation isempty $quot]} {
	    set sig [relvar insert ::raloo::mm::Signature [list\
		DomName $domName\
		ModelName $modelName\
		SigId [incr signatureIdCounter]\
	    ]]
	    ralutil::pipe {
		relation extend $dsor sp\
			SigId int {[relation extract $sig SigId]} |
		relation reidentify ~\
		    {DomName ModelName SigId ParamName}\
		    {DomName ModelName SigId ParamOrder} |
		relvar union ::raloo::mm::SignatureParam
	    }
	    #puts [relformat $::raloo::mm::Signature Signature]
	    #puts [relformat $::raloo::mm::SignatureParam SignatureParam]
	} else {
	    set sig [relation choose $::raloo::mm::Signature\
		DomName $domName\
		ModelName $modelName\
		SigId [relation extract $quot SigId]\
	    ]
	}

	return $sig
    }
}

# The relational operators are polymorphic with respect to the heading
# of the relation. This class encapsulates the relational operators
# that will be provided to all relvar backed classes.
oo::class create ::raloo::RelvarClass {
    # We do not want instances of this class created.
    self.unexport new
    self.unexport create
    # Insert a single tuple into the backing relvar.
    # "args" must be a set of attribute name / value pairs.
    method insert {args} {
	relvar insert [self] $args
    }
    method delete {idValList} {
	foreach attrValSet $idValList {
	    relvar deleteone [self] {*}$attrValSet
	}
    }
    method update {attrValueList attrName value} {
	relvar updateone [self] t $attrValueList {
	    tuple update t $attrName $value
	}
    }
    method set {relValue} {
	relvar set [self] $relValue
    }
    method get {} {
	return [relvar set [self]]
    }
    method cardinality {} {
	relation cardinality [relvar set [self]]
    }
    method degree {} {
	relation degree [relvar set [self]]
    }
    method format {args} {
	relformat [relvar set [self]] [self] {*}$args
    }
    method newFromRelation {relValue} {
	set obj [my new]
	$obj set $relValue
	return $obj
    }
    method newInsert {args} {
	return [my newFromRelation [my insert {*}$args]]
    }
}

# This is a meta-class for classes based on relvars.
oo::class create ::raloo::PasvRelvarClass {
    superclass ::raloo::RelvarClass ::oo::class
    self.unexport new
    constructor {} {
	namespace import ::ral::*

	set domName [namespace qualifiers [self]]
	namespace path [concat [namespace path] $domName]
	set className [namespace tail [self]]

	# Create a relvar by the same name as the class to serve as the
	# backing store for the class.
	# The main part of the work to construct a relvar is to construct the
	# heading of the relation that will be in the relvar. The heading
	# consists of three parts. First the "Relation" keyword.
	set relHeading [list Relation]
	# Look for any attributes whose types that are given as "UNIQUE". We
	# turn these into "int" types and put a trace on them to make sure the
	# identifier is unique within the class. First we have to squirrel
	# away the attribute names. We place the trace later.
	set uniqueAttrs [list]
	relvar update ::raloo::mm::Attribute attr {
	    [tuple extract $attr DomName] eq $domName &&
	    [tuple extract $attr ClassName] eq $className &&
	    [tuple extract $attr AttrType] eq "UNIQUE"} {
	    lappend uniqueAttrs [tuple extract $attr AttrName]
	    tuple update attr AttrType int
	}

	# Second a dictionary of attribute name / attribute type.
	lappend relHeading [ralutil::pipe {
	    relation restrictwith $::raloo::mm::Attribute {
		$DomName eq $domName && $ClassName eq $className} |
	    relation dict ~ AttrName AttrType
	}]

	# Third a list of identifiers. Each identifier is in turn a list
	# of the attribute names that make up the identifier.
	set ids [ralutil::pipe {
	    relation restrictwith $::raloo::mm::IdAttribute {
		$DomName eq $domName && $ClassName eq $className} |
	    relation group ~ IdAttr AttrName
	}]
	set idList [list]
	relation foreach id $ids -ascending IdNum {
	    lappend idList [relation list [relation extract $id IdAttr]]
	}
	lappend relHeading $idList

	# Finally create the relvar.
	relvar create [self] $relHeading
	# Now that the relvar exists, we can add traces
	foreach attr $uniqueAttrs {
	    ::raloo::mm::uniqueId [self] $attr
	}

	oo::define [self] {
	    superclass ::raloo::RelvarRef
	    constructor {args} {
		set relvarName [self class]
		next $relvarName {*}$args

		namespace path [concat [namespace path]\
		    [namespace qualifiers $relvarName]]
	    }
	}

	# Class operations are turned into class methods.
	relation foreach op [relation restrictwith $::raloo::mm::ClassOp {
		$DomName eq $domName && $ClassName eq $className}] {
	    relation assign $op OpName OpParams OpBody
	    oo::define [self] self.method $OpName $OpParams $OpBody
	}
	# Instance operations are turned into ordinary methods.
	relation foreach op [relation restrictwith $::raloo::mm::InstOp {
		$DomName eq $domName && $ClassName eq $className}] {
	    relation assign $op OpName OpParams OpBody
	    oo::define [self] method $OpName $OpParams $OpBody
	}
    }

    destructor {
	set domName [namespace qualifiers [self]]
	set className [namespace tail [self]]
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::mm::InstStateModel\
		    DomName $domName ClassName $className
		::raloo::mm::deleteStateModel $domName $className
################################################################################
		relvar delete ::raloo::mm::Attribute co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::mm::Identifier co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::mm::IdAttribute co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::mm::ClassOp co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::mm::InstOp co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar deleteone ::raloo::mm::Class\
			DomName $domName ClassName $className
	    }
	    relvar unset [self]
	} result]} {
	    puts "Class: $result"
	}
    }
}

oo::class create ::raloo::ActiveSingleton {
    constructor {} {
	# Define an unexported method for each state, prepending "__" to
	# prevent name conflicts.
	my variable domName
	my variable modelName
	set states [relation restrictwith $::raloo::mm::State {
	    $DomName eq $domName && $ModelName eq $modelName
	}]
	relation foreach state $states {
	    relation assign $state
	    set argList [::raloo::mm::formatSig\
		[relation semijoin $state $::raloo::mm::Signature]]
	    oo::define [self] method __$StateName $argList $Action
	    oo::define [self] unexport __$StateName
	}
	next
    }
    method states {} {
	my variable domName
	my variable modelName
	return [ralutil::pipe {
	    relation restrictwith $::raloo::mm::State {
		$DomName eq $domName && $ModelName eq $modelName} |
	    relation list ~ StateName
	}]
    }
    method defaultInitialState {} {
	my variable domName
	my variable modelName
	return [relation extract\
	    [relation choose $::raloo::mm::StateModel\
		DomName $domName ModelName $modelName]\
	    InitialState]
    }
    method defaultTransition {} {
	my variable domName
	my variable modelName
	return [relation extract\
	    [relation choose $::raloo::mm::StateModel\
		DomName $domName ModelName $modelName]\
	    DefaultTrans]
    }
}

oo::class create ::raloo::ActiveEntity {
    superclass ::raloo::ActiveSingleton
    # Generate a creation event
    # "avList" is a list of attribute name / value pairs used to create
    # the new instance.
    # "args" is a dictionary of event parameter name / value pairs
    method generate {avList eventName args} {
	my variable domName
	my variable modelName
	::raloo::arch::genCreation $domName $modelName $avList $eventName $args
	return
    }
    # Insert a single tuple into the backing relvar.
    # "args" must be a set of attribute name / value pairs.
    method insert {args} {
	# Add in the default initial state
	lappend args __CS__ [my defaultInitialState]
	relvar insert [self] $args
    }
    # Insert a single tuple specifying the initial state
    method insertInState {state args} {
	# Check that we have a valid state
	if {$state ni [my states]} {
	    variable modelName
	    error "\"$state\", is not a state of \"$modelName\""
	}
	lappend args __CS__ $state
	relvar insert [self] $args
    }
    method newInsertInState {state args} {
	return [my newFromRelation [my insertInState $state {*}$args]]
    }
}

oo::class create ::raloo::ActiveRelvarClass {
    superclass ::raloo::ActiveEntity ::raloo::PasvRelvarClass
    constructor {} {
	namespace import ::ral::*
	my variable domName
	set domName [namespace qualifiers [self]]
	my variable className
	set className [namespace tail [self]]
	my variable modelName
	set modelName $className
	# Active classes have an additional attribute of current state.
	relvar insert ::raloo::mm::Attribute [list\
	    DomName $domName\
	    ClassName $className\
	    AttrName __CS__\
	    AttrType string\
	]
	next

	# Now change our super class so that we get active references.
	oo::define [self] {
	    superclass ::raloo::ActiveRelvarRef
	}
    }
    destructor {
	next
    }
}

oo::class create ::raloo::InstRef {
    self.method idProjection {relValue {id 0}} {
	::ral::relation project $relValue\
	    {*}[lindex [::ral::relation identifiers $relValue] $id]
    }
    # Convert a relation value contained in the base relvar into a reference.
    method set {relValue} {
	my variable relvarName
	my variable ref
	catch {relation is $relValue subsetof [relvar set $relvarName]}\
	    isSubset
	if {!([relation isempty $ref] || [string is true -strict $isSubset])} {
	    error "relation value:\n[relformat $relValue]\nis not contained\
		in $relvarName:\n[relformat [relvar set $relvarName]]"
	}
	set ref [::raloo::InstRef idProjection $relValue]
    }
    # Find the tuples in the base relvar that this reference actually refers
    # to.
    method get {} {
	my variable relvarName
	my variable ref
	return [relation semijoin $ref [relvar set $relvarName]]
    }
    method classOf {} {
	my variable relvarName
	return [namespace tail $relvarName]
    }
    method relvarName {} {
	my variable relvarName
	return $relvarName
    }
    method ref {} {
	my variable ref
	return $ref
    }
    method delete {} {
	my variable relvarName
	my variable ref
	$relvarName delete [relation body $ref]
	set ref [relation emptyof $ref]
    }
}

# Tuples in relvars are referenced by objects of the RelvarRef class.
oo::class create ::raloo::RelvarRef {
    superclass ::raloo::InstRef
    constructor {name args} {
	namespace import ::ral::*
	my variable relvarName
	set relvarName $name
	namespace path [concat [namespace path]\
		[namespace qualifiers $relvarName]]

	# constructor for object ==> reference
	set nArgs [llength $args]
	my variable ref
	if {$nArgs == 0} {
	    set ref [relation emptyof [relvar set $relvarName]]
	} else {
	    set ref [::ralutil::pipe {
		relvar set $relvarName |
		relation choose ~ {*}$args |
		::raloo::InstRef idProjection
	    }]
	}
    }
    # readAttr attr1 ?att2 ...?
    method readAttr {args} {
	set instValue [my get]
	if {[llength $args] == 1} {
	    set attrValue [relation extract $instValue [lindex $args 0]]
	} else {
	    set attrValue [list]
	    foreach attrName $args {
		lappend attrValue [relation extract $instValue $attrName]
	    }
	}
	return $attrValue
    }
    # writeAttr attr1 value1 ?attr2 value2 ...?
    method writeAttr {args} {
	dict for {attr value} $args {
	    my UpdateAttr $attr $value
	}
	return
    }
    method with {attrList script} {
	uplevel 1 [list lassign [my readAttr {*}$attrList] {*}$attrList]
	uplevel 1 $script
    }
    method assign {attr cmd} {
	uplevel 1 [list set $attr [my readAttr $attr]]
	my writeAttr $attr [uplevel 1 [list expr $cmd]]
	uplevel 1 [list unset $attr]
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
	relformat [my get] {*}$args
    }
    method selectOne {args} {
	my variable ref
	my variable relvarName
	my set [::ralutil::pipe {
	    relvar set $relvarName |
	    relation choose ~ {*}$args
	}]
    }
    method selectAll {} {
	my variable relvarName
	my set [relvar set $relvarName]
    }
    method selectWhere {expr} {
	my variable relvarName
	::ralutil::pipe {
	    relvar set $relvarName |
	    relation restrictwith ~ [list $expr]
	} cmd
	my set [uplevel 1 $cmd]
    }
    method selectAny {expr} {
	my variable relvarName
	::ralutil::pipe {
	    relvar set $relvarName |
	    relation restrictwith ~ [list $expr] |
	    relation tag ~ __Order__ |
	    relation choose ~ __Order__ 0
	} cmd
	my set [uplevel 1 $cmd]
    }
    method selectRelated {args} {
	my variable relvarName
	set sjCmd [list relation semijoin [my get]]
	set srcClass [namespace tail $relvarName]
	foreach rship $args {
	    lassign [::raloo::RelBase parseTraversal $rship]\
		    dirMark rName endName
	    switch [$rName type] {
		association -
		partition {
		    lassign [$rName traversal $dirMark $srcClass $endName]\
			    dstClass joinAttrs
		    set dstRelvarName\
			[namespace qualifiers $relvarName]::$dstClass
		    lappend sjCmd [relvar set $dstRelvarName] -using $joinAttrs
		}
		correlation {
		    foreach {dstClass joinAttrs}\
			    [$rName traversal $dirMark $srcClass $endName] {
			set dstRelvarName\
			    [namespace qualifiers $relvarName]::$dstClass
			lappend sjCmd [relvar set $dstRelvarName]\
			    -using $joinAttrs
		    }
		}
	    }
	    set srcClass $dstClass
	}
	#puts $sjCmd
	return [$dstClass newFromRelation [eval $sjCmd]]
    }
    method relate {rship target} {
	lassign [::raloo::RelBase parseRelate $rship] dirMark rName
	if {[$rName type] eq "correlation"} {
	    error "the \"relate\" method cannot be use for an associative\
		type of relationship; use \"relateAssoc\" instead"
	}
	if {[my isempty]} {
	    error "relating empty source, \"[self]\""
	}
	if {[$target isempty]} {
	    error "relating empty target, \"$target\""
	}
	lassign [$rName relate $dirMark [self] $target]\
	    refngObj reftoObj refAttrMapping
	if {[$reftoObj cardinality] > 1} {
	    error "referred to instance must have cardinality 1,\
		\"$reftoObj\" is of cardinality,\
		\"[$reftoObj cardinality]\""
	}
	# Update the referential attributes in the relvar to have the same
	# values as the referred to attributes in the target.  The update can
	# change the values that are held in the referring reference if
	# we are changing the identifiers. So we must make sure to update the
	# referring reference.
	# Cf.  UpdateAttr.
	set refngRelvar [$refngObj relvarName]
	set allupdates [relation emptyof [relvar set $refngRelvar]]
	set reftoDict [tuple get [relation tuple [$reftoObj get]]]
	foreach refTuple [relation body [$refngObj ref]] {
	    set updated [relvar updateone $refngRelvar tup $refTuple {
		set avList [list]
		dict for {reftoAttr refngAttr} $refAttrMapping {
		    lappend avList $refngAttr [dict get $reftoDict $reftoAttr]
		}
		tuple update tup {*}$avList
	    }]
	    set allupdates [relation union $allupdates $updated]
	}
	$refngObj set [relation create {*}[lrange\
	    [relation heading [relvar set $refngRelvar]] 1 end]\
	    {*}[relation body $allupdates]]
	return
    }
    method relateAssoc {rship target args} {
	if {[llength $args] % 2 != 0} {
	    error "additional associative attributes names / values must\
		be given in pairs, \"$args\""
	}
	lassign [::raloo::RelBase parseRelate $rship] dirMark rName
	if {[$rName type] ne "correlation"} {
	    error "the \"relateAssoc\" method cannot be use for a [$rName type]\
		type of relationship; use \"relate\" instead"
	}
	if {[my isempty]} {
	    error "relating empty source, \"[self]\""
	}
	if {[$target isempty]} {
	    error "relating empty target, \"$target\""
	}
	lassign [$rName relate $dirMark [self] $target]\
	    selfAttrMap targetAttrMap assocClassName
	if {[my cardinality] > 1} {
	    error "referred to instance must have cardinality 1,\
		\"[self]\" is of cardinality, \"[my cardinality]\""
	}
	if {[$target cardinality] > 1} {
	    error "referred to instance must have cardinality 1,\
		\"$target\" is of cardinality,\
		\"[$target cardinality]\""
	}
	# Create an instance of the association class, containing the
	# referential attribute values and the other attribute values
	# supplied as arguments.
	# First we accumulate the attribute name / value dictionary for
	# the reference to the source object.
	set assocTuple [dict create]
	set reftoDict [tuple get [relation tuple [my get]]]
	dict for {refto refng} $selfAttrMap {
	    dict set assocTuple $refng [dict get $reftoDict $refto]
	}
	# Next for the target object
	set reftoDict [tuple get [relation tuple [$target get]]]
	dict for {refto refng} $targetAttrMap {
	    dict set assocTuple $refng [dict get $reftoDict $refto]
	}
	set assocInst [$assocClassName new]
	$assocInst set [relvar insert [$assocInst relvarName]\
	    [concat $assocTuple $args]]
	return $assocInst
    }
    method unrelate {rship target} {
	lassign [::raloo::RelBase parseRelate $rship] dirMark rName
	if {[$rName type] eq "correlation"} {
	    error "the \"unrelate\" method cannot be use for an associative\
		type of relationship"
	}
	if {[my isempty]} {
	    error "relating empty source, \"[self]\""
	}
	if {[$target isempty]} {
	    error "relating empty target, \"$target\""
	}
	lassign [$rName relate $dirMark [self] $target]\
	    refngObj reftoObj refAttrMapping
	# Update the referential attributes in the relvar to have the same
	# values as the referred to attributes in the target.  The update can
	# change the values that are held in the referring reference if
	# we are changing the identifiers. So we must make sure to update the
	# referring reference.
	# Cf.  UpdateAttr.
	set refngRelvar [$refngObj relvarName]
	set allupdates [relation emptyof [relvar set $refngRelvar]]
	foreach refTuple [relation body [$refngObj ref]] {
	    set updated [relvar updateone $refngRelvar tup $refTuple {
		set avList [list]
		dict for {reftoAttr refngAttr} $refAttrMapping {
		    lappend avList $refngAttr {}
		}
		tuple update tup {*}$avList
	    }]
	    set allupdates [relation union $allupdates $updated]
	}
	$refngObj set [relation create {*}[lrange\
	    [relation heading [relvar set $refngRelvar]] 1 end]\
	    {*}[relation body $allupdates]]
	return
    }
    method reclassify {rName targetClass args} {
	if {[$rName type] ne "partition"} {
	    error "the \"migrate\" method can only be used with a\
		generalization type of relationship"
	}
	if {[my isempty]} {
	    error "migrating empty source, \"[self]\""
	}
	if {[my cardinality] > 1} {
	    error "migrate only a single instance, \"[self]\""
	}
	# Get a map of the referring attributes of self. This maps supertype
	# attribute names to the corresponding subtype attribute names.
	set refAttrMap [$rName superTypeRefAttrs [my classOf]]
	# Get a map of the referring attributes of the new class. Here we
	# get the mapping of subtype attribute names to supertype attribute
	# names.
	set migAttrMap [$rName subTypeRefAttrs $targetClass]
	# Construct a list of attribute / value pairs appropriate to the class
	# to which we are migrating.  Attribute names are the subtype referring
	# attributes. Values are the corresponding values from "self".  This is
	# a bit tricky. We have to map to the super type attribute name and
	# then back down to the subtype attribute name in "self", just in case
	# the referring attribute names in the sub types are different.
	set avList [list]
	set valueDict [tuple get [relation tuple [my get]]]
	dict for {subAttr superAttr} $migAttrMap {
	    lappend avList $subAttr [dict get $valueDict\
		[dict get $refAttrMap $superAttr]]
	}
	# Transform "self" into the new subtype.
	my variable relvarName
	relvar eval {
	    my delete
	    $targetClass insert {*}[concat $avList $args]
	    set subObj [$targetClass new {*}$avList]
	}
	return $subObj
    }
    # Unexported methods.
    #
    # Update an attribute in the base relvar to which this reference
    # refers.
    method UpdateAttr {attrName value} {
	my variable ref
	my variable relvarName

	set updates [relation emptyof [relvar set $relvarName]]
	relation foreach r $ref {
	    set updates [relation union $updates\
		    [relvar updateone $relvarName tup\
			[tuple get [relation tuple $r]] {
		    tuple update tup $attrName $value
		    }]]
	}
	my set $updates
    }
}

oo::class create ::raloo::ActiveInstRef {
    superclass ::raloo::InstRef
    # "args" is a dictionary of parameter name / parameter values pairs
    method generate {eventName args} {
	my variable relvarName
	my variable ref
	::raloo::arch::genToInsts [namespace qualifiers $relvarName]\
	    [namespace tail $relvarName] $ref $eventName $args
	return
    }
}

oo::class create ::raloo::ActiveRelvarRef {
    superclass ::raloo::ActiveInstRef ::raloo::RelvarRef
    constructor {name args} {
	#puts "ActiveRelvarRef constructor: $name: $args"
	namespace import ::ral::*
	next $name {*}$args
    }
    method generateDelayed {time eventName args} {
	my variable relvarName
	my variable ref
	my Source
	::raloo::arch::genDelayedToInsts $time\
	    [namespace qualifiers $relvarName] $srcModel $srcRef\
	    [namespace tail $relvarName] $ref $eventName $args
    }
    method cancelDelayed {eventName} {
	my variable relvarName
	my variable ref
	my Source
	::raloo::arch::cancelDelayedToInsts [namespace qualifiers $relvarName]\
	    $srcModel $srcRef [namespace tail $relvarName] $ref $eventName
    }
    method delayedRemaining {eventName} {
	my variable relvarName
	my variable ref
	my Source
	::raloo::arch::remainingDelayed [namespace qualifiers $relvarName]\
	    $srcModel $srcRef [namespace tail $relvarName] $ref $eventName
    }
    method Source {{modelVar srcModel} {instVar srcRef}} {
	upvar $modelVar srcModel
	upvar $instVar srcRef
	if {[catch {uplevel 1 self caller} caller]} {
	    set srcModel [uplevel 2 namespace current]
	    set srcRef {Relation {} {{}} {{}}}
	} else {
	    set obj [lindex $caller 1]
	    if {[catch {$obj classOf} srcModel]} {
		set srcModel $obj
	    }
	    if {[catch {$srcModel ref} srcRef]} {
		set srcRef {Relation {} {{}} {{}}}
	    }
	}
    }
}

# Base class for relationships.
oo::class create ::raloo::RelBase {
    self.unexport new
    self.unexport create
    self.method create {name} {
	if {[string range $name 0 1] ne "::"} {
	    set name [uplevel 1 namespace current]::$name
	}
	if {[namespace exists $name]} {
	    error "A namespace with name, \"$name\", already exists\
		and so a relationship by the same name may not be created."
	}
	# This creates it in the given namespace and runs the constructor.
	return [my createWithNamespace $name $name]
    }
    destructor {
	#puts "RelBase destructor: \"[info level 0]\": \"[self next]\""
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	if {[catch {
	    relvar deleteone ::raloo::mm::Relationship\
		DomName $domName RelName $relName
	    relvar delete ::raloo::mm::ReferringClass r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::mm::RefToClass r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::mm::ClassRoleInRel r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::mm::RefToIdAttribute r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::mm::AttributeRef r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar deleteone ::raloo::mm::AssignerStateModel\
		DomName $domName RelName $relName
	    relvar deleteone ::raloo::mm::SingleAssigner\
		DomName $domName RelName $relName
	    relvar deleteone ::raloo::mm::MultipleAssigner\
		DomName $domName RelName $relName
	    ::raloo::mm::deleteStateModel $domName $relName
		} result]} {
	    puts "RelBase: $result"
	}
	next
    }
    self.method parseTraversal {rship} {
	if {![regexp {\A(~)?([^.]+)\.?(.*)?\Z} $rship\
		match dirMark rName endName]} {
	    error "bad relationship syntax, \"$rship\""
	}
	return [list $dirMark $rName $endName]
    }
    self.method parseRelate {rship} {
	if {![regexp {\A(~)?(.+)\Z} $rship match dirMark rName]} {
	    error "bad relationship syntax, \"$rship\""
	}
	return [list $dirMark $rName]
    }
    method info {} {
	return [relvar constraint info [self]]
    }
    method type {} {
	return [lindex [my info] 0]
    }
}

# The class that represents a simple relationship.
oo::class create ::raloo::Relationship {
    superclass ::raloo::RelBase
    self.unexport new
    constructor {} {
	namespace import ::ral::*

	# Query the meta-model data and extract the information necessary
	# to build a "relvar association" command. All relationships have
	# a backing relvar constraint that enforces the referential
	# integrity that the relationship traversal depends upon.
	# While we are making the meta-model queries, we cache away the
	# information needed to help relationship traversal. Since
	# relationship definitions don't change, the cache is a simple
	# way to avoid repeating the queries each time the relationship
	# is traversed. Traversal ultimately depends upon performing
	# "relation semijoin" commands and so the cached information is
	# specifically oriented to the requirements of "relation semijoin".
	# For a simple relationship, we need to know the class names
	# in the forward and reverse directions and the referential
	# attribute name mapping in both directions.

	my variable domName
	set domName [namespace qualifiers [self]]
	my variable relName
	set relName [namespace tail [self]]
	set simplerel [ralutil::pipe {
	    relation choose $::raloo::mm::Relationship DomName $domName\
		RelName $relName |
	    relation semijoin ~ $::raloo::mm::SimpleRel
	}]

	set simplereferring [relation semijoin $simplerel\
		$::raloo::mm::SimpleReferring]
	my variable refngClass
	set refngClass [relation extract $simplereferring ClassName]
	set refngCard [relation extract $simplereferring Cardinality]

	set simplerefto [relation semijoin $simplerel $::raloo::mm::SimpleRefTo]
	my variable reftoClass
	set reftoClass [relation extract $simplerefto ClassName]
	set reftoCard [relation extract $simplerefto Cardinality]

	# We create a dictionary that maps a referring attribute name
	# to the referred to attribute name. Note we are depending upon
	# the behavior of dictionaries to return keys and values in
	# the same order that they were put into the dictionary.
	set attrMap [ralutil::pipe {
	    relation semijoin $simplereferring $::raloo::mm::ReferringClass\
		$::raloo::mm::AttributeRef -using\
		{DomName DomName ClassName RefngClassName RelName RelName\
		    RoleId RefngRoleId} |
	    relation dict ~ RefngAttrName RefToAttrName
	}]

	# construct the "relvar association"
	relvar association [self]\
	    ${domName}::$refngClass [dict keys $attrMap] $refngCard\
	    ${domName}::$reftoClass [dict values $attrMap] $reftoCard

	my variable forwJoinAttrs
	my variable backJoinAttrs
	dict for {refng refto} $attrMap {
	    lappend forwJoinAttrs $refng $refto
	    lappend backJoinAttrs $refto $refng
	}
	next
    }
    destructor {
	#puts "Relationship destructor: \"[info level 0]\": \"[self next]\""
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::mm::SimpleRel\
		    DomName $domName RelName $relName
		relvar delete ::raloo::mm::SimpleReferring r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::mm::SimpleRefTo r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		next
	    }} result]} {
	    puts "Relationship: $result"
	}
	relvar constraint delete [self]
    }
    # Returns a two element list consisting of the ending class name and join
    # attributes for traversing the relationship in the given direction.
    # "beginName" is checked to see if it valid.
    method traversal {dir beginName endName} {
	my variable reftoClass
	my variable forwJoinAttrs
	my variable refngClass
	my variable backJoinAttrs

	if {$dir eq "~"} {
	    set srcClass $reftoClass
	    set dstClass $refngClass
	    set result [list $refngClass $backJoinAttrs]
	} else {
	    set srcClass $refngClass
	    set dstClass $reftoClass
	    set result [list $reftoClass $forwJoinAttrs]
	}
	if {$beginName ne $srcClass} {
	    error "traversing ${dir}[namespace tail [self]] begins at,\
		\"$srcClass\", and not at \"$beginName\""
	}
	if {$endName ne {} && $endName ne $dstClass} {
	    error "traversing ${dir}[namespace tail [self]] ends at,\
		\"$dstClass\", and not at \"$endName\""
	}
	return $result
    }
    method relate {dir sourceObj targetObj} {
	my variable reftoClass
	my variable refngClass
	my variable backJoinAttrs ; # refto -> refng mapping

	set sourceClass [$sourceObj classOf]
	set targetClass [$targetObj classOf]

	if {$reftoClass eq $refngClass} {
	    # Reflexive case
	    if {$sourceClass ne $reftoClass || $targetClass ne $reftoClass} {
		error "\"$sourceClass\" and \"$targetClass\" do not\
		    participate in relationship \"[self]\""
	    }
	    # The reflexive case requires the direction indicator to
	    # disambiguate the situation, i.e. the class names of the
	    # instances are not sufficient.
	    if {$dir eq "~"} {
		set result [list $targetObj $sourceObj]
	    } else {
		set result [list $sourceObj $targetObj]
	    }
	} else {
	    # For the non-reflexive case, an explicit direction is not needed
	    # to determine the referring -> referred to instances. So we ignore
	    # it here an use the class names to determine the direction of
	    # reference.
	    if {$sourceClass eq $refngClass && $targetClass eq $reftoClass} {
		set result [list $sourceObj $targetObj]
	    } elseif {$sourceClass eq $reftoClass &&\
		    $targetClass eq $refngClass} {
		set result [list $targetObj $sourceObj]
	    } else {
		error "\"$sourceClass\" and \"$targetClass\" do not\
		    participate in relationship \"[self]\""
	    }
	}
	lappend result $backJoinAttrs
	return $result
    }
}

oo::class create ::raloo::Generalization {
    superclass ::raloo::RelBase
    self.unexport new
    constructor {} {
	namespace import ::ral::*

	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	# construct the "relvar partition"
	set genrel [ralutil::pipe {
	    relation choose $::raloo::mm::Relationship DomName $domName\
		RelName $relName |
	    relation semijoin ~ $::raloo::mm::GenRel
	}]
	set supertype [relation semijoin $genrel $::raloo::mm::SupertypeRole]

	my variable superClass
	set superClass [relation extract $supertype ClassName]
	set partCmd [list relvar partition [self] ${domName}::$superClass]

	# Find all the attribute references by the subtypes.
	set attributeref [ralutil::pipe {
	    relation semijoin\
		$supertype\
		$::raloo::mm::RefToClass\
		$::raloo::mm::RefToIdAttribute\
		$::raloo::mm::AttributeRef -using\
		{DomName DomName ClassName RefToClassName RelName RelName\
		RoleId RefToRoleId AttrName RefToAttrName IdNum RefToIdNum} |
	    relation project ~ RefngClassName RefngAttrName RefToAttrName |
	    relation group ~ RefMap RefToAttrName RefngAttrName
	}]
	#puts [relformat $attributeref attributeref]

	# Subtype info is held in a dictionary, keyed by subtype name
	# whose value are the join attributes. There are two such dictionaries
	# one for the super->sub traversal and the other for the sub->super.
	my variable superJoinAttrs
	set superJoinAttrs [dict create]
	my variable subJoinAttrs
	set subJoinAttrs [dict create]
	relation foreach aref $attributeref {
	    relation assign $aref
	    set refmap [relation dict $RefMap RefToAttrName RefngAttrName]
	    lappend subspecs ${domName}::$RefngClassName [dict values $refmap]
	    set superAttrs [list]
	    set subAttrs [list]
	    dict for {refto refng} $refmap {
		lappend superAttrs $refto $refng
		lappend subAttrs $refng $refto
	    }
	    dict set superJoinAttrs $RefngClassName $superAttrs
	    dict set subJoinAttrs $RefngClassName $subAttrs
	}
	lappend partCmd [dict keys $refmap]
	#puts "$partCmd $subspecs"
	eval $partCmd $subspecs
    }
    destructor {
	#puts "Generalization destructor: \"[info level 0]\": \"[self next]\""
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	my variable superClass
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::mm::GenRel\
		    DomName $domName RelName $relName
		set supr [relation restrict $::raloo::mm::SupertypeRole r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName
		}]
		relvar minus ::raloo::mm::SupertypeRole $supr

		set subr [relation restrict $::raloo::mm::SubtypeRole r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName
		}]
		relvar minus ::raloo::mm::SubtypeRole $subr

		# This clean up is more complicated than might first appear.
		# For supertypes, we delete all the PolymorphicEvents
		# and the corresponding DeferralPath and Event instances
		set dp [relation semijoin $supr $::raloo::mm::DeferralPath\
		    -using {DomName DomName ClassName ModelName}]
		relvar minus ::raloo::mm::DeferralPath $dp
		set ev [relation semijoin $dp $::raloo::mm::DeferredEvent]
		relvar minus ::raloo::mm::Event\
		    [relation semijoin $ev $::raloo::mm::Event]
		relvar minus ::raloo::mm::DeferredEvent $ev
		relvar minus ::raloo::mm::PolymorphicEvent $ev

		# For subtypes, we turn MappedEvents into LocalEvents
		# and InheritedEvents into PolymorphicEvents. Thus destroying
		# a generalization that contains polymorphism, converts
		# the mapped events in the leaf nodes to local events and
		# converts the inherited events in the interior nodes into
		# polymorphic events. Strangely, the polymorphism migrates
		# as a result of destroying a generalization relationship
		# at the top of a hierarchy.
		set nle [relation semijoin $subr $::raloo::mm::NonLocalEvent\
		    -using {DomName DomName ClassName ModelName}]
		relvar minus ::raloo::mm::NonLocalEvent $nle
		set mapped [relation semijoin $nle $::raloo::mm::MappedEvent]
		relvar minus ::raloo::mm::MappedEvent $mapped
		relvar union ::raloo::mm::LocalEvent\
		    [relation eliminate $mapped ParentModel]
		set inhe [relation semijoin $nle $::raloo::mm::InheritedEvent]
		relvar minus ::raloo::mm::InheritedEvent $inhe
		relvar union ::raloo::mm::PolymorphicEvent $inhe

		next
	    }} result]} {
	    puts "Generalization: $result"
	}
	relvar constraint delete [self]
    }
    # Returns a two element list consisting of the ending class name and join
    # attributes for traversing the relationship in the given direction.
    # "beginName" is checked to see if it valid.
    # Traversal syntax for Generalizations is a little different.
    # Traversing from super to sub must be of the form "~RX.sub".
    # Traversal from sub to super is of the form "RX"
    # For super to sub we also accept "RX.sub" and "~RX" is always an error.
    method traversal {dir beginName endName} {
	my variable superClass
	my variable superJoinAttrs
	my variable subJoinAttrs

	#puts "$dir $beginName $endName"

	if {$dir eq "~"} {
	    # Supertype to Subtype traversal
	    if {$endName eq {}} {
		error "supertype to subtype traversal requires the\
		    specification of the subtype class name"
	    }
	    if {![dict exists $superJoinAttrs $endName]} {
		error "\"$endName\" is not a subtype in the generalization,\
		    \"[self]\""
	    }
	    if {$beginName ne $superClass} {
		error "\"$beginName\" is not the supertype for the\
		    generalization, \"[self]\""
	    }
	    set result [list $endName [dict get $superJoinAttrs $endName]]
	} else {
	    # Subtype to Supertype traversal if "endName" is empty.
	    # Otherwise we allow the "~" to be left off for a super to
	    # sub traversal as long as the subtype name is given.
	    if {$endName eq {}} {
		# Sub to Super
		if {![dict exists $subJoinAttrs $beginName]} {
		    error "\"$beginName\" is not a subtype in the\
			generalization, \"[self]\""
		}
		set result [list $superClass\
			[dict get $subJoinAttrs $beginName]]
	    } else {
		# Alternate form of Super to Sub
		if {![dict exists $superJoinAttrs $endName]} {
		    error "\"$endName\" is not a subtype in the generalization,\
			\"[self]\""
		}
		if {$beginName ne $superClass} {
		    error "\"$beginName\" is not the supertype for the\
			generalization, \"[self]\""
		}
		set result [list $endName [dict get $superJoinAttrs $endName]]
	    }
	}
	return $result
    }
    method relate {dir sourceObj targetObj} {
	my variable superClass
	my variable superJoinAttrs

	set sourceClass [$sourceObj classOf]
	set targetClass [$targetObj classOf]
	# We never need the direction indicator to resolve matters in a
	# generalization relationship. All the class names are distinct.
	if {$sourceClass eq $superClass &&\
		[dict exists $superJoinAttrs $targetClass]} {
	    set result [list $targetObj $sourceObj\
		    [dict get $superJoinAttrs $targetClass]]
	} elseif {$targetClass eq $superClass &&\
		[dict exists $superJoinAttrs $sourceClass]} {
	    set result [list $sourceObj $targetObj\
		    [dict get $superJoinAttrs $sourceClass]]
	} else {
	    error "\"$sourceClass\" and \"$targetClass\" do not participate in\
		relationship, \"[self]\""
	}

	return $result
    }
    method superTypeRefAttrs {subtypeName} {
	my variable superJoinAttrs
	if {![dict exists $superJoinAttrs $subtypeName]} {
	    error "\"$subtypeName\" is not a subtype of relationship,\
		\"[self]\""
	}
	return [dict get $superJoinAttrs $subtypeName]
    }
    method subTypeRefAttrs {subtypeName} {
	my variable subJoinAttrs
	if {![dict exists $subJoinAttrs $subtypeName]} {
	    error "\"$subtypeName\" is not a subtype of relationship,\
		\"[self]\""
	}
	return [dict get $subJoinAttrs $subtypeName]
    }
}

oo::class create ::raloo::AssocRelationship {
    superclass ::raloo::RelBase
    self.unexport new
    constructor {} {
	#puts "AssocRelationship constructor: \"[info level 0]\":\
		\"[self next]\""
	namespace import ::ral::*

	my variable domName
	set domName [namespace qualifiers [self]]
	my variable relName
	set relName [namespace tail [self]]

	set assocrel [ralutil::pipe {
	    relation choose $::raloo::mm::Relationship DomName $domName\
		RelName $relName |
	    relation semijoin ~ $::raloo::mm::AssocRel
	}]

	set associator [relation semijoin $assocrel $::raloo::mm::Associator]
	my variable assocClassName
	set assocClassName [relation extract $associator ClassName]

	set assocsource [relation semijoin $assocrel $::raloo::mm::AssocSource]
	my variable sourceClassName
	set sourceClassName [relation extract $assocsource ClassName]
	set sourceCard [relation extract $assocsource Cardinality]

	set assoctarget [relation semijoin $assocrel $::raloo::mm::AssocTarget]
	my variable targetClassName
	set targetClassName [relation extract $assoctarget ClassName]
	set targetCard [relation extract $assoctarget Cardinality]

	# Create two dictionaries. One for the references to the source
	# class and the other for the references to the target class.
	# First retrieve all the attributes references
	set attributeref [relation semijoin\
	    $associator\
	    $::raloo::mm::ReferringClass\
	    $::raloo::mm::AttributeRef -using\
		{DomName DomName ClassName RefngClassName RelName RelName\
		    RoleId RefngRoleId}
	]
	# Split the attribute references into those for the source and those
	# for the target. N.B. we have to use the "RefToRoleId" here to
	# handle the case of a reflexive relationship.
	set sourceAttrMap [ralutil::pipe {
	    relation restrictwith $attributeref\
		    {$RefToClassName eq $sourceClassName && $RefToRoleId == 1} |
	    relation dict ~ RefngAttrName RefToAttrName
	}]
	set targetAttrMap [ralutil::pipe {
	    relation restrictwith $attributeref\
		    {$RefToClassName eq $targetClassName && $RefToRoleId == 2} |
	    relation dict ~ RefngAttrName RefToAttrName
	}]
	# construct the "relvar correlation"
	# N.B. the swap in cardinalities -- this rationalizes the XUML view of
	# "relationships" to the TclRAL view of "correlation constraints"
	relvar correlation [self] ${domName}::$assocClassName\
	    [dict keys $sourceAttrMap] $targetCard\
		${domName}::$sourceClassName [dict values $sourceAttrMap]\
	    [dict keys $targetAttrMap] $sourceCard\
		${domName}::$targetClassName [dict value $targetAttrMap]

	# Create four attributes lists from the two attribute mapping
	# dictionaries.
	# 1. source -> associator -- forward
	# 2. associator -> target -- forward
	# 3. target -> associator -- backward
	# 4. associator -> source -- backward
	my variable srcAssocJoinAttrs
	my variable assocSrcJoinAttrs
	dict for {refng refto} $sourceAttrMap {
	    lappend assocSrcJoinAttrs $refng $refto
	    lappend srcAssocJoinAttrs $refto $refng
	}
	my variable assocTargetJoinAttrs
	my variable targetAssocJoinAttrs
	dict for {refng refto} $targetAttrMap {
	    lappend assocTargetJoinAttrs $refng $refto
	    lappend targetAssocJoinAttrs $refto $refng
	}
	next
    }
    destructor {
	#puts "AssocRelationship destructor: \"[info level 0]\":\
		\"[self next]\""
	my variable domName
	my variable relName
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::mm::AssocRel\
		    DomName $domName RelName $relName
		relvar delete ::raloo::mm::Associator r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::mm::AssocSource r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::mm::AssocTarget r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		next
	    }} result]} {
	    puts "AssocRelationship: $result"
	}
	relvar constraint delete [self]
    }
    # Returns a two or four element list consisting of the ending class name
    # and join attributes for traversing the relationship in the given
    # direction.  The four element list is used when we traverse completely
    # across the associative relationship. This gives the information for
    # the semijoin to the associative class and then to the ending class.
    method traversal {dir beginName endName} {
	my variable assocClassName
	my variable sourceClassName
	my variable targetClassName
	my variable srcAssocJoinAttrs
	my variable assocSrcJoinAttrs
	my variable assocTargetJoinAttrs
	my variable targetAssocJoinAttrs

	if {$endName ne {}} {
	    # stopping at the associative class
	    if {$endName ne $assocClassName} {
		error "\"$endName\" is not the associative class for\
		    relationship, \"[self]\""
	    }
	    if {$dir eq "~"} {
		# target -> assoc traversal
		if {$beginName ne $targetClassName} {
		    error "\"$beginName\" is not the target of relationship,\
			\"[self]\""
		}
		set result [list $assocClassName $targetAssocJoinAttrs]
	    } else {
		# source -> assoc traversal
		if {$beginName ne $sourceClassName} {
		    error "\"$beginName\" is not the source of relationship,\
			\"[self]\""
		}
		set result [list $assocClassName $srcAssocJoinAttrs]
	    }
	} else {
	    # complete traversal -- find out the direction
	    if {$dir eq "~"} {
		# target to source
		if {$beginName ne $targetClassName} {
		    error "traversal of $dir[namespace tail [self]] does not\
			begin at class, \"$beginName\""
		}
		set result [list $assocClassName $targetAssocJoinAttrs\
		    $sourceClassName $assocSrcJoinAttrs]
	    } else {
		# source to target
		if {$beginName ne $sourceClassName} {
		    error "traversal of $dir[namespace tail [self]] does not\
			begin at class, \"$beginName\""
		}
		set result [list $assocClassName $srcAssocJoinAttrs\
		    $targetClassName $assocTargetJoinAttrs]
	    }
	}
	return $result
    }
    method relate {dir oneObj otherObj} {
	my variable assocClassName
	my variable sourceClassName
	my variable targetClassName
	my variable srcAssocJoinAttrs
	my variable targetAssocJoinAttrs

	set oneClass [$oneObj classOf]
	set otherClass [$otherObj classOf]

	if {$sourceClassName eq $targetClassName} {
	    # Reflexive case
	    if {$oneClass ne $sourceClassName ||\
		    $otherClass ne $targetClassName} {
		error "\"$oneClass\" and \"$otherClass\" do not\
		    participate in relationship \"[self]\""
	    }
	    # The reflexive case requires the direction indicator to
	    # disambiguate the situation, i.e. the class names of the
	    # instances are not sufficient.
	    if {$dir eq "~"} {
		set result [list $targetAssocJoinAttrs $srcAssocJoinAttrs]
	    } else {
		set result [list $srcAssocJoinAttrs $targetAssocJoinAttrs]
	    }
	} else {
	    # For the non-reflexive case, an explicit direction is not needed
	    # to determine the referring -> referred to instances. So we ignore
	    # it here an use the class names to determine the direction of
	    # reference.
	    if {$oneClass eq $sourceClassName &&\
		    $otherClass eq $targetClassName} {
		set result [list $srcAssocJoinAttrs $targetAssocJoinAttrs]
	    } elseif {$oneClass eq $targetClassName &&\
		    $otherClass eq $sourceClassName} {
		set result [list $targetAssocJoinAttrs $srcAssocJoinAttrs]
	    } else {
		error "\"$oneClass\" and \"$otherClass\" do not\
		    participate in relationship \"[self]\""
	    }
	}
	lappend result $assocClassName
	return $result
    }
}

oo::class create ::raloo::SingleAssigner {
    superclass ::raloo::ActiveSingleton ::oo::class
    self.unexport new
    constructor {} {
	#puts "SingleAssigner constructor: \"[info level 0]\": \"[self next]\""
	my variable domName
	my variable relName
	my variable modelName
	set modelName $relName

	next
	namespace import ::ral::*
	# Single assigners do create a relvar. It only has the current state as
	# an attribute and will only ever have a cardinality of one.
	relvar create [self] {
	    Relation {
		__CS__ string
	    } {
		__CS__
	    }
	}
	relvar insert [self] [list\
	    __CS__ [relation extract [relation choose $::raloo::mm::StateModel\
		    DomName $domName ModelName $relName] InitialState]
	]

	oo::define [self] {
	    superclass ::raloo::SingleAssignerRef
	    constructor {args} {
		set relvarName [self class]
		next $relvarName {*}$args

		namespace path [concat [namespace path]\
		    [namespace qualifiers $relvarName]]
	    }
	}
    }
    destructor {
	#puts "SingleAssigner destructor: \"[info level 0]\": \"[self next]\""
	next
    }
    method generate {eventName args} {
	my variable domName
	my variable relName
	set ref [::raloo::arch::idProject [relvar set ${domName}::${relName}]]
	::raloo::arch::genToInsts $domName $relName $ref $eventName $args
	return
    }
}

oo::class create ::raloo::SingleAssignerRel {
    superclass ::raloo::Relationship ::raloo::SingleAssigner
    self.unexport new
    destructor {
	relvar unset [self]
	next
    }
}

oo::class create ::raloo::SingleAssignerAssocRel {
    superclass ::raloo::AssocRelationship ::raloo::SingleAssigner
    self.unexport new
    destructor {
	relvar unset [self]
	next
    }
}

oo::class create ::raloo::SingleAssignerRef {
    superclass ::raloo::InstRef
    constructor {name args} {
	namespace import ::ral::*
	my variable relvarName
	set relvarName $name
	namespace path [concat [namespace path]\
		[namespace qualifiers $relvarName]]
	my variable ref
	set ref [::raloo::InstRef idProjection [relvar set $relvarName]]
	next
    }
    method set {relValue} {
    }
}

oo::class create ::raloo::MultipleAssigner {
    superclass ::raloo::ActiveEntity ::oo::class
    self.unexport new
    constructor {} {
	my variable domName
	my variable relName
	my variable modelName
	set modelName $relName

	next

	namespace import ::ral::*
	# Multiple assigners also create a relvar. In this case it contains
	# the identifiers of the multiple assigner as well the current state.
	# Find the identifying attributes/types associated with multiple
	# assigner
	set idAttrs [ralutil::pipe {
	    relation choose $::raloo::mm::MultipleAssigner\
		DomName $domName RelName $relName |
	    relation semijoin ~ $::raloo::mm::Identifier\
		$::raloo::mm::IdAttribute $::raloo::mm::Attribute |
	    relation dict ~ AttrName AttrType
	}]
	relvar create [self] [list\
	    Relation\
	    [concat $idAttrs [list __CS__ string]]\
	    [dict keys $idAttrs]\
	]
	# Define an unexported method for each state, prepending "__" to
	# prevent name conflicts.
	set states [relation restrictwith $::raloo::mm::State {
	    $DomName eq $domName && $ModelName eq $relName
	}]
	relation foreach state $states {
	    relation assign $state
	    set argList [::raloo::mm::formatSig\
		[relation semijoin $state $::raloo::mm::Signature]]
	    oo::define [self] method __$StateName $argList $Action
	    oo::define [self] unexport __$StateName
	}

	oo::define [self] {
	    superclass ::raloo::MultipleAssignerRef
	    constructor {args} {
		set relvarName [self class]
		next $relvarName {*}$args

		namespace path [concat [namespace path]\
		    [namespace qualifiers $relvarName]]
	    }
	}
    }
    method insert {args} {
	relvar insert [self] $args
    }
}

oo::class create ::raloo::MultipleAssignerRel {
    superclass ::raloo::Relationship ::raloo::MultipleAssigner
    self.unexport new
    destructor {
	relvar unset [self]
	next
    }
}

oo::class create ::raloo::MultipleAssignerAssocRel {
    superclass ::raloo::AssocRelationship ::raloo::MultipleAssigner
    self.unexport new
    destructor {
	relvar unset [self]
	next
    }
}

oo::class create ::raloo::MultipleAssignerRef {
    superclass ::raloo::ActiveInstRef
    constructor {name args} {
	namespace import ::ral::*
	my variable relvarName
	set relvarName $name
	namespace path [concat [namespace path]\
		[namespace qualifiers $relvarName]]

	# constructor for object ==> reference
	set nArgs [llength $args]
	my variable ref
	if {$nArgs == 0} {
	    set ref [relation emptyof [relvar set $relvarName]]
	} else {
	    set ref [::raloo::InstRef idProjection\
		[relvar insert $relvarName $args]]
	}
	next
    }
    method set {relValue} {
	my variable relvarName
	my variable ref
	catch {relation is $relValue subsetof [relvar set $relvarName]}\
	    isSubset
	if {!([relation isempty $ref] || [string is true -strict $isSubset])} {
	    error "relation value:\n[relformat $relValue]\nis not contained\
		in $relvarName:\n[relformat [relvar set $relvarName]]"
	}
	set ref [::raloo::InstRef idProjection $relValue]
    }
    method selectOne {args} {
	my variable ref
	my variable relvarName
	my set [::ralutil::pipe {
	    relvar set $relvarName |
	    relation choose ~ {*}$args
	}]
    }
}

################################################################################
#
# State Machine Execution Engine
#
# These procs form the state machine execution engine that drives execution in
# raloo. The engine supports the notion of a thread of control.  A thread of
# control is started by a DomainOp or when a delayed event is dispatched. It
# evolves as a tree to record the events generated as the state machines of a
# domain interact with each other. The evolving tree is traversed in breadth
# first order for dispatching events. This gives the proper ordering of event
# delivery relative to event generation.
#
# The execution rules are:
# 1. Only one thread of control is operating at a time.
# 2. If a new thread of control is started, then it is deferred and executed
#    after any preceding threads are done.
# 3. The engine interface to the Tcl event loop is via idle task events.
#    The starting of a thread of control or the generation of an event inserts
#    an idle task event into the Tcl event loop which is then used to dispatch
#    the next state machine event.
# 4. Polymorphic events are synchonously mapped until they are consumed, i.e.
#    the mapped event is not re-inserted into thread of control tree. If the
#    polymorphic event propagates along multiple generalization hierachies,
#    then all the polymorphic events are synchronously dispatched and the
#    order of dispatch along the hierarchies is unspecified.
#
################################################################################

package require logger
package require struct::tree

namespace eval ::raloo::arch {
    namespace import ::ral::*

    # The list of threads of control
    variable controlTree [list]
    # This variable controls whether or not we are "single" stepping the
    # thread of control. When single stepping, some supervisor part of the
    # program must call "dispatchEvent" to cause each event to be delivered.
    # Otherwise, the execution engine will drive itself by queuing calls to
    # "dispatchEvent" as an idle task on the Tcl event queue.
    variable singleStep 0

    logger::initNamespace ::raloo::arch
}

# Interface to allow adjusting logging levels.
proc ::raloo::arch::logLevel {level} {
    log::setlevel $level
}

# This needs _not_ to be in the ::raloo::arch namespace lest it be traced also.
proc ::raloo::logtrace {traceDict} {
    puts "\[trace [lindex $traceDict 0]\] \'[lindex $traceDict 1]\'"
    namespace eval ::raloo::arch log::debug [list $traceDict]
}

# Interface for low level tracing
proc ::raloo::arch::trace {status} {
    if {[string is true -strict $status]} {
	log::logproc trace ::raloo::logtrace
	log::trace add -ns ::raloo::arch
	log::trace on
    } else {
	log::trace remove -ns ::raloo::arch
	log::trace off
    }
}

# Start a new Thread of Control. Threads of control are the implied tree of
# event generation. This proc creates a new tree and adds it to the list of
# threads of control that are to be executed.  The root node of a thread of
# control keeps a list that is used to record the breadth first order of the
# tree visits.
#
# domName -- name of the domain starting the thread of control
# modelName -- name of the class/namespace starting the thread. When a
#   DomainOp starts the thread, it should be set to the domain name itself.
#   When a delayed event starts the thread, it should be the class which
#   delayed the event.
# inst -- a dictionary corresponding to an identifier of "modelName". If
#   "modelName" is a domain name, then inst should be set to the empty list {}.
# script -- a Tcl script that is run to kick off the thread of control.
# params -- a list of paramters to be given to "script" when it is run.
#
proc ::raloo::arch::newTOC {domName modelName inst script params} {
    variable controlTree
    lappend controlTree [set tocTree [struct::tree]]

    $tocTree set root currNode root
    $tocTree set root queue [list]
    $tocTree set root event [dict create\
	domName $domName\
	modelName $modelName\
	inst $inst\
	script $script\
	params $params\
    ]

    if {[llength $controlTree] == 1} {
	stepEngine
    }

    return
}

# Generate an event to a set of instances. This proc is the primary interface
# used by the OO code to generate events. Since instance references can have
# cardinality > 1, this function can generate more than one event.
#
# domName -- name of the domain
# modelName -- name of the class or assigner
# refSet -- a relation whose heading is the projection of an identifier of
#   the relvar named ${domName}::${modelName}
# eventName -- the name of the event to generate
# argList -- a list of supplimental parameters to the event
#
proc ::raloo::arch::genToInsts {domName modelName refSet eventName argList} {
    set event [lookUpEvent $domName $modelName $eventName]
    set params [buildArguments $event $argList]
    # This reference may be multi-valued. Generate an event to each one.
    relation foreach inst $refSet {
	queueEvent $domName $modelName $inst $eventName $params
    }
}

proc ::raloo::arch::genDelayedToInsts {time domName srcModel srcRef\
				       dstModel dstRef eventName argList} {
    set event [lookUpEvent $domName $dstModel $eventName]
    set params [buildArguments $event $argList]
    # This reference may be multi-valued. Generate an event to each one.
    relation foreach inst $dstRef {
	queueDelayedEvent $time $domName $srcModel $srcRef $dstModel $inst\
	    $eventName $params
    }
}

proc ::raloo::arch::genCreation {domName modelName attrs eventName argList} {
    set event [lookUpEvent $domName $modelName $eventName]
    set params [buildArguments $event $argList]
    queueCreationEvent $domName $modelName $attrs $eventName $params

    return
}

proc ::raloo::arch::cancelDelayedToInsts {domName srcModel srcRef dstModel\
					  dstRef eventName} {
    set event [lookUpEvent $domName $dstModel $eventName]
    relation foreach refValue $dstRef {
	::raloo::arch::cancelDelayedEvent $domName $srcModel $srcRef $dstModel\
	    $dstRef $eventName
    }
}

proc ::raloo::arch::lookUpEvent {domName modelName eventName} {
    # Check if this event even exists.
    set event [relation choose $::raloo::mm::Event\
	DomName $domName\
	ModelName $modelName\
	EventName $eventName\
    ]
    if {[relation isempty $event]} {
	error "event, \"$eventName\", does not exist for\
	    \"$domName.$modelName\""
    }
    return $event
}
proc ::raloo::arch::buildArguments {event paramDict} {
    set sigParams [relation semijoin $event $::raloo::mm::Signature\
	$::raloo::mm::SignatureParam]
    if {[relation cardinality $sigParams] != [dict size $paramDict]} {
	error "mismatch in the number of event parameters:\
	    expected [::ral::relation cardinality $sigParams],\
	    got [dict size $paramDict]"
    }
    set argList [list]
    relation foreach param $sigParams -ascending ParamOrder {
	relation assign $param ParamName
	if {![dict exists $paramDict $ParamName]} {
	    error "parameter, \"$ParamName\", does not exist in argument\
		dictionary, \"$paramDict\""
	}
	lappend argList [dict get $paramDict $ParamName]
    }
    return $argList
}

# Queue an event to the current thread of control.
# "instRef" is a singleton relation value instance reference.
proc ::raloo::arch::queueEvent {domName modelName instRef eventName params} {
    setupTOC tocTree srcNode tocQueue
    # The front of the queue is the current event dispatch that is executing.
    # This invocation must come from executing a state transition for
    # the instance contained in that node of the TOC tree. Establish the
    # source of the event.
    set srcValues [$tocTree get $srcNode event]
    set srcClass [dict get $srcValues modelName]
    set srcInst [dict get $srcValues inst]

    log::debug "queueEvent: $domName:\
	$srcClass.[list [tuple get [relation tuple $srcInst]]] -\
	$eventName [list $params] ->\
	$modelName.[list [tuple get [relation tuple $instRef]]]"

    # Deal with self directed events. If an event is self directed, then
    # we insert it at the beginning of the list of child nodes. Otherwise,
    # events are simply placed at the end. One more complication is the placing
    # the node at the proper location in the queue.
    catch {relation is $srcInst == $instRef} sameInst
    if {$srcClass eq $modelName && [string is true -strict $sameInst]} {
	set dstNode [$tocTree insert $srcNode 0]
	$tocTree set root queue [linsert $tocQueue 0 $dstNode]
	log::debug "self directed event"
    } else {
	set dstNode [$tocTree insert $srcNode end]
	$tocTree lappend root queue $dstNode
	log::debug "non-self directed event: $srcClass, $modelName, $sameInst"
    }
    $tocTree set $dstNode event [dict create\
	domName $domName\
	modelName $modelName\
	inst $instRef\
	eventName $eventName\
	params $params\
	type N\
    ]
}

proc ::raloo::arch::queueCreationEvent {domName modelName attrs eventName\
					params} {
    setupTOC tocTree srcNode tocQueue
    # The front of the queue is the current event dispatch that is executing.
    # This invocation must come from executing a state transition for
    # the instance contained in that node of the TOC tree. Establish the
    # source of the event.
    set srcValues [$tocTree get $srcNode event]
    set srcClass [dict get $srcValues modelName]
    set srcInst [dict get $srcValues inst]

    log::debug "queueCreationEvent: $domName:\
	$srcClass.[list [tuple get [relation tuple $srcInst]]] -\
	$eventName [list $params] -> $modelName.[list $attrs]"

    set dstNode [$tocTree insert $srcNode end]
    $tocTree lappend root queue $dstNode
    $tocTree set $dstNode event [dict create\
	domName $domName\
	modelName $modelName\
	inst $attrs\
	eventName $eventName\
	params $params\
	type C\
    ]
}

# The idle callbacks come here to cause the next event in the thread of
# control to be dispatched.
proc ::raloo::arch::dispatchEvent {} {
    variable controlTree
    if {[llength $controlTree] == 0} {
	log::debug "empty control tree"
	return
    }
    set tocTree [lindex $controlTree 0]
    set dstNode [$tocTree get root currNode]
    set event [$tocTree get $dstNode event]
    log::debug "dispatchEvent: $dstNode \"$event\""

    set domName [dict get $event domName]
    set modelName [dict get $event modelName]
    # Threads of control begin at the "root" node of the tree. Here there are
    # no events to deliver, but instead a script to execute to get things
    # started. Most important, the data transaction is begun here.
    if {$dstNode eq "root"} {
	log::info "begin TOC: $domName"
	relvar transaction begin
	# Execute the script associated with thread of control. This gets
	# things going.
	set tocError [catch {
	    namespace inscope $domName\
		[dict get $event script] {*}[dict get $event params]
	} result options]
    } else {
	set srcNode [$tocTree parent $dstNode]
	# Check if we are dispatching a creation event. In that case, we
	# must create the instance and set the current state to "@". After that
	# it is an otherwise normal event dispatch.
	if {[dict get $event type] eq "C"} {
	    set instValue [dict get $event inst]
	    log::info "creation event:\
		${domName}::${modelName}.[list $instValue]"
	    # For creation events the "inst" property is a dictionary of
	    # attribute name / values for the new instance that is to be
	    # created. Set the initial state to creation pseudo-state.
	    dict set instValue __CS__ @
	    dict set event inst [idProject\
		[relvar insert ${domName}::${modelName} $instValue]]
	}
	# Measure the execution time of the state action and record that
	# in the thread of control tree.
	set startTime [clock microseconds]
	set tocError [catch {
	    deliverEvent [$tocTree get [$tocTree parent $dstNode] event] $event
	} result options]
	$tocTree set $dstNode time [expr {[clock microseconds] - $startTime}]
    }

    if {$tocError} {
	# Some error occurred while trying to propagate the thread of
	# control. Terminate the thread of control and throw the error.
	# Note we rollback the data transaction.
	endTOC
	relvar transaction rollback
	log::error "error TOC: $result"
	return -options $options $result
    } else {
	# Fetch the queue again. It could have been modified when the
	# event was delivered, e.g. another event could be generated.
	set tocQueue [$tocTree get root queue]
	# Check for the end of the thread of control.
	if {[llength $tocQueue] == 0} {
	    log::info "end TOC: $domName"
	    endTOC
	    # This needs to be last in case it throws an error. An error here
	    # indicates that the thread of control did not leave the class
	    # structure referentially consistent. Just let background error
	    # handling catch it.
	    relvar transaction end
	} else {
	    # Make the front of the queue the current node for dispatching
	    # the next event.
	    $tocTree set root currNode [lindex $tocQueue 0]
	    $tocTree set root queue [lrange $tocQueue 1 end]
	    stepEngine
	}
    }
}

# Deliver an event.
# "eventValue" is a dictionary containing all the event dispatch information.
# Here is where we determine if this is a monomorphic
# event or a polymorphic one.
proc ::raloo::arch::deliverEvent {srcEvent dstEvent} {
    # Determine if we are delivering a monomorphic event or a polymorphic event
    set domName [dict get $dstEvent domName]
    set modelName [dict get $dstEvent modelName]
    set eventName [dict get $dstEvent eventName]
    set effEvent [relation choose $::raloo::mm::EffectiveEvent\
	DomName $domName\
	ModelName $modelName\
	EventName $eventName\
    ]
    if {[relation isnotempty $effEvent]} {
	deliverEffEvent $srcEvent $dstEvent
	return
    }

    set defEvent [relation choose $::raloo::mm::DeferredEvent\
	DomName $domName\
	ModelName $modelName\
	EventName $eventName\
    ]
    if {[relation isnotempty $defEvent]} {
	deliverDefEvent $srcEvent $dstEvent
	return
    }

    log::error "cannot find EffectiveEvent or DeferredEvent associated\
	with, ${domName}::${modelName} $eventName"
}

# Effective events are monomorphic and cause transitions.
proc ::raloo::arch::deliverEffEvent {srcEvent dstEvent} {
    set domName [dict get $dstEvent domName]
    set dstModel [dict get $dstEvent modelName]
    set eventName [dict get $dstEvent eventName]
    set params [dict get $dstEvent params]
    set srcModel [dict get $srcEvent modelName]
    set srcInst [dict get $srcEvent inst]
    # Here "inst" is an instance reference. Get the singleton relation
    # value to which it refers from the relvar.
    set dstRef [dict get $dstEvent inst]
    set relvarName ${domName}::${dstModel}
    set instValue [relation choose [relvar set $relvarName]\
	    {*}[lindex [relation body $dstRef] 0]]
    # Detect the event in flight error
    if {[relation isempty $instValue]} {
	set msg "In Flight Error:\
	    [formatEvent $domName $srcModel $srcInst $dstModel $dstRef]"
	log::error $msg
	error $msg
    }
    # We need the current state to compute the transition.
    set cs [relation extract $instValue __CS__]
    # Find the transition and the new state to which we are headed.
    set trans [relation choose $::raloo::mm::TransitionPlace\
	DomName $domName\
	ModelName $dstModel\
	EventName $eventName\
	StateName $cs\
    ]
    if {[relation isnotempty $trans]} {
	# Determine if this transition goes to a new state or is a
	# non-state transition.
	set news [relation semijoin $trans $::raloo::mm::StateTrans]
	if {[relation isnotempty $news]} {
	    # Transition to a new state.
	    set newStateName [relation extract $news NewState]
	    log::info "transition:\
		[formatEvent $domName $srcModel $srcInst $dstModel $dstRef]:\
		$cs - $eventName [list $params] -> $newStateName"
	    # This is where it all happens! This code snippet comes from DKF
	    # and shows how "namespace inscope" can be used to invoke an
	    # unexported method for an object. A bit clever perhaps, but it
	    # allows us to have state actions as unexported methods and still
	    # have the software architecture invoke the action.
	    set inst [$relvarName new]
	    $inst set $instValue
	    if {[catch {namespace inscope $inst [list my __$newStateName]\
		    {*}$params} result options]} {
		log::error $result
	    } else {
		# Update the current state to be the new state
		relvar updateone $relvarName tup\
			[lindex [relation body $dstRef] 0] {
		    tuple update tup __CS__ $newStateName
		}
	    }
	    $inst destroy
	    return -options $options
	} else {
	    # A non-state transition. This is either a "can't happen" or an
	    # "ignore". In either case, no transition is actually performed.
	    set nons [relation semijoin $trans $::raloo::mm::NonStateTrans]
	    if {[relation isempty $nons]} {
		error "Panic: cannot find subtype of TransitionPlace"
	    }
	    deliverNonStateTrans $domName $srcModel $srcInst $dstModel\
		$dstRef $cs [relation extract $nons TransRule]\
		$eventName $params
	}
    } else {
	# For unspecified transitions, we use the default recorded in the
	# StateModel. Default transition are always non-state transitions (see
	# R35 of metamodel).  Unspecified creation transitions are always
	# deemed a "can't happen".
	set defTrans [expr {$cs eq "@" ? "CH" :\
	    [relation extract [relation choose $::raloo::mm::StateModel\
		DomName $domName ModelName $dstModel] DefaultTrans]}]
	deliverNonStateTrans $domName $srcModel $srcInst $dstModel\
	    $dstRef $cs $defTrans $eventName $params
    }
}

# Deliver a Deferred Event. Deferred events are either polymorphic events
# that have been injected at this level of the generalization hierarchy
# or they have been inherited down the hierarchy because they were not
# consumed in the supertype and this subtype acts as a supertype in another
# generalization (i.e. a multi-level generalization hierarchy).
proc ::raloo::arch::deliverDefEvent {srcEvent dstEvent} {
    set domName [dict get $dstEvent domName]
    set modelName [dict get $dstEvent modelName]
    set eventName [dict get $dstEvent eventName]
    set srcModel [dict get $srcEvent modelName]
    set srcInst [dict get $srcEvent inst]
    # Find the tuple associated with the instance reference. It is important
    # when attempting to find the associated subtype to use the full relation
    # value, since there may be multiple identifiers and we can be sure
    # which ones are used to implement the generalization relationships.
    set relvarName ${domName}::${modelName}
    set instRef [dict get $dstEvent inst]
    set params [dict get $dstEvent params]
    set superValue [relation semijoin $instRef [relvar set $relvarName]]
    # Find the super type roles. There can be many since a given supertype
    # may be the supertype for multiple generalizations.
    set superRoles [relation restrictwith $::raloo::mm::DeferralPath {
	$DomName eq $domName && $ModelName eq $modelName &&\
	$EventName eq $eventName}]
    # For each generalization hierarchy, find the set of subtypes.
    # It is in these subtypes that we must find the related instance.
    relation foreach superRole $superRoles {
	set superSrc ${domName}::[relation extract $superRole ModelName]
	set subRoles [relation semijoin $superRole $::raloo::mm::GenRel\
	    $::raloo::mm::SubtypeRole]
	# Loop through the subtypes finding the attributes that make up the
	# supertype to subtype reference -- subtypes are the referring class.
	# We will construct a semijoin from the supertype to the subtype, i.e.
	# from the referred to class to the referring class.
	set dispatched 0
	relation foreach subRole $subRoles {
	    set subDst ${domName}::[relation extract $subRole ClassName]
	    set joinAttrs [ralutil::pipe {
		relation semijoin $subRole $::raloo::mm::AttributeRef |
		relation dict ~ RefToAttrName RefngAttrName
	    }]
	    # Check if this subtype is the one related to the supertype.
	    set instValue [ralutil::pipe {
		relvar set ${domName}::[relation extract $subRole ClassName] |
		relation semijoin $superValue ~ -using $joinAttrs
	    }]
	    if {[relation isnotempty $instValue]} {
		# Okay, found the related subtype!
		# We modify the "dstEvent" in place and then redispatch
		# it when we find out where it is going.
		dict set dstEvent inst [idProject $instValue]

		log::info "polymorphic dispatch:\
		    $superSrc.[list [tuple get [relation tuple $superValue]]]\
		    ==> $subDst.[list [tuple get [relation tuple $instValue]]]:\
		    $eventName [list $params]"
		# Find the non-local event that this subtype receives
		set nlevt [relation semijoin $subRole\
		    $::raloo::mm::NonLocalEvent]
		# If the non-local event is a mapped event, then it is
		# consumed at this level.
		set effEvent [relation semijoin $nlevt\
		    $::raloo::mm::MappedEvent $::raloo::mm::EffectiveEvent]
		if {[relation isnotempty $effEvent]} {
		    # modify the reflect the new model and event type
		    dict set dstEvent modelName\
			[relation extract $effEvent ModelName]
		    dict set dstEvent type N
		    deliverEffEvent $srcEvent $dstEvent
		    set dispatched 1
		    break
		}
		# Otherwise if the non-local event is an inherited event,
		# then it must be passed down to the next level.
		set defEvent [relation semijoin $nlevt\
		    $::raloo::mm::InheritedEvent $::raloo::mm::DeferredEvent]
		if {[relation isnotempty $defEvent]} {
		    dict set dstEvent modelName\
			[relation extract $defEvent ModelName]
		    deliverDefEvent $srcEvent $dstEvent
		    set dispatched 1
		    break
		}
		# If we reach here, we have a tear in the space-time continuum.
		# Really, it implies that somehow we created a meta-model
		# population that was referentially inconsistent.
		error "Panic: cannot find subtype of\
		    NonLocalEvent:\n[relformat $nlevt]"
	    }
	}
	# It is possible to get here without actually dispatching the event
	# if we fail to find a related subtype in the above loop. This amounts
	# to an "event in flight" error, i.e. an event was sent to the
	# supertype at some point in the thread of control, but the
	# corresponding subtype was deleted before the event was taken off
	# the queue to be delivered. This is deemed an analysis error,so
	# we whine about it here.
	# HERE -- do better with this error message
	if {!$dispatched} {
	    set msg "In Flight Error:\
		failed to find subtype related to:\n[relformat $superRole]"
	    log::error $msg
	    error $msg
	}
    }
}

# Non state transitions are just special rules to avoid adding states
# to the state model for common circumstances. In our case, IG ==> ignore
# and CH ==> can't happen.
proc ::raloo::arch::deliverNonStateTrans {domName srcModel srcRef dstModel\
			      dstRef currstate transRule eventName params} {
    set msg "transition:\
	[formatEvent $domName $srcModel $srcRef $dstModel $dstRef]:\
	$currstate - $eventName [list $params] -> $transRule"
    if {$transRule eq "IG"} {
	log::info $msg
    } elseif {$transRule eq "CH"} {
	log::error $msg
	error $msg
    } else {
	error "Panic: unknown transition rule, \"$transRule\""
    }
}

################################################################################
#
# Delayed event architecture
#
# There are several important rules about delayed events.
#
# 1. There can only be one instance of a given delayed event outstanding
#    between any source and destination instances. We interpret any attempt
#    to create a second one as the intent to cancel the existing one and
#    create a new one with the new time. You could interpret the attempt as
#    an error, but that would just cause a lot of needless calls to
#    cancel events.
# 2. When a delayed event expires it starts a new Thread of Control.
# 3. When cancelling a delayed event, if we don't find it in the delayed
#    event queue, then we will search the thread of control queue in an
#    attempt to find it there. This solves the "race" condition associated
#    with cancelling a delayed event that might have expired. It makes the
#    state machines much easier if they can be sure that after cancelling
#    a delayed event it will not be delivered.
#
################################################################################

namespace eval ::raloo::arch {
    # Number of milliseconds that constitutes one tick for delayed events.
    variable delayTick 10
    # The delayed event "queue". Delayed events are held in a list of
    # dictionaries. The keys to the dictionary are:
    # DomName string
    # EventName string
    # SrcModel string
    # SrcInst Relation
    # DstModel string
    # DstInst Relation
    # Params list
    # Time int
    variable DelayQueue [list]
}

proc ::raloo::arch::queueDelayedEvent {time domName srcModel srcRef\
				       dstModel dstRef eventName params} {
    variable DelayQueue
    # See if we are already expired
    if {$time <= 0} {
	newTOC $domName $srcModel $srcRef ::raloo::arch::queueEvent [list\
		$domName $dstModel $dstRef $eventName $params]
	return
    }

    # Check if the event already exists
    set evtIndex [findDelayedEvent $domName $srcModel $srcRef $dstModel\
	    $dstRef $eventName]
    if {$evtIndex >= 0} {
	# Just update to the new time and parameters.
	set de [lindex $DelayQueue $evtIndex]
	dict set de Time $time
	dict set de Params $params
	lset DelayQueue $evtIndex $de
    } else {
	# Otherwise we insert the new delayed event.
	lappend DelayQueue [dict create\
	    DomName $domName\
	    EventName $eventName\
	    SrcModel $srcModel\
	    SrcInst $srcRef\
	    DstModel $dstModel\
	    DstInst $dstRef\
	    Time $time\
	    Params $params\
	]
    }
    log::info "delayed queued($time):\
	[formatEvent $domName $srcModel $srcRef $dstModel $dstRef]:\
	$eventName [list $params]"
    if {[llength $DelayQueue] == 1} {
	# Only start a timer to tick down the delay queue when the
	# first entry is inserted.
	variable delayTick
	after $delayTick ::raloo::arch::serviceDelayedQueue
    }
}

# Returns the index into DelayQueue where the event is found, or -1 if not
# if the event is not found.
proc ::raloo::arch::findDelayedEvent {domName srcModel srcRef dstModel dstRef\
				      eventName} {
    variable DelayQueue
    for {set index 0} {$index < [llength $DelayQueue]} {incr index} {
	set de [lindex $DelayQueue $index]
	catch {relation is [dict get $de SrcInst] == $srcRef} sameSrc
	catch {relation is [dict get $de DstInst] == $dstRef} sameDst
	if {[string is true -strict $sameSrc] &&\
	    [string is true -strict $sameDst] &&\
	    [dict get $de EventName] eq $eventName &&\
	    [dict get $de SrcModel] eq $srcModel &&\
	    [dict get $de DstModel] eq $dstModel &&\
	    [dict get $de DomName] eq $domName} {

	    return $index
	}
    }
    return -1
}

# Cancel a delayed event
proc ::raloo::arch::cancelDelayedEvent {domName srcModel srcRef dstModel dstRef\
				      eventName} {
    set evtIndex [findDelayedEvent $domName $srcModel $srcRef $dstModel\
	    $dstRef $eventName]
    if {$evtIndex >= 0} {
	variable DelayQueue
	set DelayQueue [lreplace $DelayQueue $evtIndex $evtIndex]
	log::info "delayed cancelled:\
	    [formatEvent $domName $srcModel $srcRef dstModel $dstRef]:\
	    $eventName"
    } else {
	# If we did not find the event in the delay queue, then we search for
	# the thread of control that was started when the delayed event
	# expired. This is done to overcome the annoyance of the race condition
	# between the event expiring and it being actually consumed by a state.
	# This architecture guarantees that after a cancel, the event will not
	# be delivered.  Three cases can come out of this:
	#
	# 1. The matching ToC is the currently executing one. In that case
	#    it is truly too late.
	# 2. We find a match that is not the current ToC and throw it away.
	# 3. We don't find anything. This amounts to a big noop, where the
	#    code has cancelled the event that it had never created in
	#    the first place.
	#
	# We are looking for that tree whose root node matches the source
	# information and whose script is "::raloo::arch::queueEvent".
	variable controlTree
	for {set tocIndex 0} {$tocIndex < [llength $controlTree]}\
	    {incr tocIndex} {
	    set toc [lindex $controlTree $tocIndex]
	    set event [$toc get root event]
	    catch {relation is [dict get $event inst] == $srcRef} sameRef
	    if {[dict get $event DomName] eq $domName &&\
		[dict get $event ModelName eq $srcModel] &&\
		[string is true -strict $sameRef] &&\
		[dict get $event script] eq "::raloo::arch::queueEvent"} {
		$toc destroy
		set controlTree [lreplace $controlTree $tocIndex $tocIndex]
		log::info "delayed cancelled ToC:\
		    [formatEvent $domName $srcModel $srcRef dstModel $dstRef]:\
		    $eventName"
		return
	    }
	}
	log::notice "delayed cancelled failed:\
	    [formatEvent $domName $srcModel $srcRef dstModel $dstRef]:\
	    $eventName"
    }
}

# Query the time remaining for a delayed event.
proc ::raloo::arch::remainingDelayed {domName srcModel srcRef dstModel dstRef\
				      eventName} {
    set evtIndex [findDelayedEvent $domName $srcModel $srcRef $dstModel\
	    $dstRef $eventName]
    variable DelayQueue
    return [expr {$evtIndex == -1 ?\
	    0 : [dict get [lindex $DelayQueue $evtIndex] Time]}]
}

proc ::raloo::arch::serviceDelayedQueue {} {
    variable delayTick
    set tmr [after $delayTick ::raloo::arch::serviceDelayedQueue]

    variable DelayQueue
    for {set index 0} {$index < [llength $DelayQueue]} {} {
	set event [lindex $DelayQueue $index]
	set time [expr {[dict get $event Time] - $delayTick}]
	if {$time <= 0} {
	    set DelayQueue [lreplace $DelayQueue $index $index]
	    newTOC\
		[dict get $event DomName]\
		[dict get $event SrcModel]\
		[dict get $event SrcInst]\
		::raloo::arch::queueEvent\
		[list\
		    [dict get $event DomName]\
		    [dict get $event DstModel]\
		    [dict get $event DstInst]\
		    [dict get $event EventName]\
		    [dict get $event Params]\
		]
	    log::info "delayed queued($time):\
		[formatEvent\
		    [dict get $event DomName]\
		    [dict get $event SrcModel]\
		    [dict get $event SrcInst]\
		    [dict get $event DstModel]\
		    [dict get $event DstInst]]:\
		[dict get $event EventName] [list [dict get $event Params]]"
	} else {
	    dict set event Time $time
	    lset DelayQueue $index $event
	    incr index
	}
    }

    # Cancel the timer if there are no events to be delayed.
    if {[llength $DelayQueue] == 0} {
	after cancel $tmr
    }
}

# We send all the queue of idle event to the Tcl event loop through here.
# A monitor program can single step the thread of control by calling
# "dispatchEvent" and making sure there are not Tcl events by setting
# the "singleStep" variable.
proc ::raloo::arch::stepEngine {} {
    variable singleStep
    variable controlTree
    if {!$singleStep} {
	after idle [list ::raloo::arch::dispatchEvent]
	log::debug "generated idle dispatch"
    }
}

proc ::raloo::arch::setupTOC {treeRef nodeRef queueRef} {
    upvar 1 $treeRef tocTree
    upvar 1 $nodeRef tocNode
    upvar 1 $queueRef tocQueue
    variable controlTree
    if {[llength $controlTree] == 0} {
	error "attempt to generate an event outside of\
	    a thread of control"
    }
    set tocTree [lindex $controlTree 0]
    set tocNode [$tocTree get root currNode]
    set tocQueue [$tocTree get root queue]
}

proc ::raloo::arch::endTOC {} {
    variable controlTree
    set tocTree [lindex $controlTree 0]
    $tocTree destroy
    set controlTree [lrange $controlTree 1 end]
    cleanupRefs
    if {[llength $controlTree] != 0} {
	stepEngine
    }
}

proc ::raloo::arch::cleanupRefs {} {
    # All instance reference sets get deleted.
    foreach instref [info class instances ::raloo::SingleAssignerRef] {
	$instref destroy
    }
    foreach instref [info class instances ::raloo::MultipleAssignerRef] {
	$instref destroy
    }
    foreach instref [info class instances ::raloo::RelvarRef] {
	$instref destroy
    }
    foreach instref [info class instances ::raloo::ActiveRelvarRef] {
	$instref destroy
    }
}

# Returns a relation value that consists of the projection of identifier 1.
proc ::raloo::arch::idProject {relValue} {
    relation project $relValue {*}[lindex [relation identifiers $relValue] 0]
}

# Returns a dictionary of attribute names / values that correspond to
# the values of identifier 1 for a relation. This is sufficient information
# to find the corresponding tuple in a relvar.
proc ::raloo::arch::idValues {relValue} {
    lindex [relation body [idProject $relValue]] 0
}

proc ::raloo::arch::formatEvent {domName src srcRef dst dstRef} {
    return "$domName:\
	$src.[list [idValues $srcRef]] ==> $dst.[list [idValues $dstRef]]"
}
