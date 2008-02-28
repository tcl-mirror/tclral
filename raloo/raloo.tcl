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
# $Revision: 1.4 $
# $Date: 2008/02/28 02:27:16 $
#  *--

package require Tcl 8.5
package require TclOO
package require ral
package require ralutil

package provide raloo 0.1

namespace eval ::raloo {
    namespace export Domain
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

    relvar create Supertype {
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
	Supertype {DomName RelName} 1\
	GenRel {DomName RelName} 1

    relvar create Subtype {
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
	Subtype {DomName RelName} +\
	GenRel {DomName RelName} 1

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
	    Subtype {DomName RelName ClassName RoleId}\
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
	    Supertype {DomName RelName ClassName RoleId}\
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

    # This relvar is used to hold system generated unique identifiers.
    relvar create __uniqueids__ {
	Relation {
	    RelvarName string IdAttr string IdNum int
	} {
	    {RelvarName IdAttr}
	}
    }
}

# A convenience proc that installs a trace on an attribute of
# a relvar in order to have the system assign a unique value
# to that attribute.
proc ::raloo::uniqueId {relvarName attrName} {
    relvar insert __uniqueids__ [list\
	RelvarName $relvarName\
	IdAttr $attrName\
	IdNum 0
    ]
    relvar trace add variable $relvarName insert\
	[list ::raloo::uniqueIdInsertTrace $attrName]
}

# This proc can be used in a relvar trace on insert to
# create a unique identifier for an attribute.
proc ::raloo::uniqueIdInsertTrace {attrName op relvarName tup} {
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

	relvar insert ::raloo::Domain [list\
	    DomName [self]\
	]
	my define $script
    }

    destructor {
	foreach assocrel\
		[info class instances ::raloo::AssocRelationship [self]::*] {
	    $assocrel destroy
	}
	foreach simprel\
		[info class instances ::raloo::Relationship [self]::*] {
	    $simprel destroy
	}
	foreach genrel\
		[info class instances ::raloo::Generalization [self]::*] {
	    $genrel destroy
	}
	foreach class\
		[info class instances ::raloo::PasvRelvarClass [self]::*] {
	    $class destroy
	}
	# Clean up the meta-model data
	if {[catch {
	    relvar eval {
		relvar delete ::raloo::DomainOp d {
		    [tuple extract $d DomName] eq [self]
		}
		relvar deleteone ::raloo::Domain DomName [self]
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
	    Class\
	    Relationship\
	    Generalization\
	    AssocRelationship
    }
    method transaction {script} {
	my Transact $script
    }
    # This method wraps a script in a relvar transaction.
    method Transact {script} {
	catch {
	    relvar eval {
		uplevel 1 $script
	    }
	} result options
	foreach instref [info class instances ::raloo::RelvarRef] {
	    $instref destroy
	}
	return -options $options $result
    }
    # Methods that are used in the definition of a domain during construction.
    method DomainOp {name argList body} {
	oo::define [self] method $name $argList [list my Transact $body]
	oo::define [self] export $name
    }
    # Interpret the definition of a class.
    method Class {name script} {
	if {[string match {*::*} $name]} {
	    error "class names may not have namespace separators, \"$name\""
	}
	my variable className
	set className $name
	# Define the class into the meta-model and then evaluate the
	# class definition. The class definition will invoke other methods
	# to populate the meta-model.
	relvar eval {
	    relvar insert ::raloo::Class [list\
		DomName [self]\
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
	::raloo::PasvRelvarClass create $className

	return
    }
    # Simple relationship.
    # Syntax is : refng X-->Y refto
    method Relationship {name refngClassName spec refToClassName {script {}}} {
	if {![regexp -- {([1?*+])-*>*([1?])} $spec\
		match refngCard refToCard]} {
	    error "unrecognized relationship specification, \"$spec\""
	}

	set refngRoleId 0
	set refToRoleId 1
	# Determine the identifier which is referred to in the relationship.
	set gotId [my ParseClassName $refToClassName refToClassName refToIdNum]
	relvar eval {
	    relvar insert ::raloo::Relationship [list\
		DomName [self]\
		RelName $name\
	    ]
	    relvar insert ::raloo::SimpleRel [list\
		DomName [self]\
		RelName $name\
	    ]
	    relvar insert ::raloo::SimpleReferring [list\
		DomName [self]\
		RelName $name\
		ClassName $refngClassName\
		RoleId $refngRoleId\
		Cardinality $refngCard\
	    ]
	    set refngClass [relvar insert ::raloo::ReferringClass [list\
		DomName [self]\
		RelName $name\
		ClassName $refngClassName\
		RoleId $refngRoleId\
	    ]]
	    relvar insert ::raloo::ClassRoleInRel [list\
		DomName [self]\
		RelName $name\
		ClassName $refngClassName\
		RoleId $refngRoleId\
	    ]
	    relvar insert ::raloo::SimpleRefTo [list\
		DomName [self]\
		RelName $name\
		ClassName $refToClassName\
		RoleId $refToRoleId\
		Cardinality $refToCard\
	    ]
	    set reftoClass [relvar insert ::raloo::RefToClass [list\
		DomName [self]\
		RelName $name\
		ClassName $refToClassName\
		RoleId $refToRoleId\
	    ]]
	    relvar insert ::raloo::ClassRoleInRel [list\
		DomName [self]\
		RelName $name\
		ClassName $refToClassName\
		RoleId $refToRoleId\
	    ]

	    # By default if no formalization script is given, then we assume
	    # the the referring attributes have the same names as the
	    # attributes to which they refer.  Which identifier they refer to
	    # may be given by the "*[1-9]" convention applied to the referred
	    # to class name.  If no "*" id syntax is used, then "*1" is used.
	    # If a formalization script is present, then the script determines
	    # which identifier to use. If both a script and the "*[1-9]" syntax
	    # are used, then the identifier specified by both must match.
	    if {$script eq {}} {
		set refToIdAttribute [my MakeRefToIdAttribute $reftoClass\
			$refToIdNum]
		# Since we assume the names are the same, we use the identity
		# mapping.
		my MakeAttributeRef $refngClass $refToIdAttribute\
		    [my MakeIdentityRefMap\
			[relation list $refToIdAttribute AttrName]]
	    } else {
		# Make a relation out of the script information.
		set attrMapRel [my MakeAttrRefMap $script]
		# Find the identifier number implied by the script. It must
		# match the one specified for the referred to class if the
		# "*[1-9]" syntax was used.
		set idNum [my FindRefToIdNum $reftoClass $attrMapRel]
		if {$gotId && $refToIdNum != $idNum} {
		    error "referring attribute(s), \"[join\
			[relation list $attrMapRel RefngAttrName] {, }]\",\
			do(es) not refer to identifier,\
			\"$refToIdNum\" of class,\"$refToClassName\""
		}
		my MakeAttributeRef $refngClass\
		    [my MakeRefToIdAttribute $reftoClass $idNum] $attrMapRel
	    }
	}
	::raloo::Relationship create [self]::$name

	return
    }
    method Generalization {name supertype script} {
	my variable relName
	set relName $name
	my variable superName
	my variable superIdNum
	my variable superIdGiven
	set superIdGiven [my ParseClassName $supertype superName superIdNum]
	relvar eval {
	    relvar insert ::raloo::Relationship [list\
		DomName [self]\
		RelName $name\
	    ]
	    relvar insert ::raloo::GenRel [list\
		DomName [self]\
		RelName $name\
	    ]
	    relvar insert ::raloo::Supertype [list\
		DomName [self]\
		RelName $name\
		ClassName $superName\
		RoleId 0\
	    ]
	    my variable superClass
	    set superClass [relvar insert ::raloo::RefToClass [list\
		DomName [self]\
		ClassName $superName\
		RelName $name\
		RoleId 0\
	    ]]
	    relvar insert ::raloo::ClassRoleInRel [list\
		DomName [self]\
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
	::raloo::Generalization create [self]::$name
	return
    }

    # If no script is present, then names are assumed to be the same
    # as those of the identifier in the supertype, otherwise there must be a
    # script to define the referential associations.
    method SubType {subName {script {}}} {
	# Check that the request makes sense
	my variable superName
	if {$superName eq $subName} {
	    error "a supertype may not be its own subtype, \"$subName\""
	}

	my variable relName
	set matchingSubtype [relation restrictwith $::raloo::Subtype {
		$DomName eq [self] && $RelName eq $relName &&
		$ClassName eq $subName}]
	if {[relation isnotempty $matchingSubtype]} {
	    error "duplicate subtype class, \"$subName\""
	}

	my variable subRoleId
	relvar insert ::raloo::Subtype [list\
	    DomName [self]\
	    RelName $relName\
	    ClassName $subName\
	    RoleId $subRoleId\
	]
	set refngClass [relvar insert ::raloo::ReferringClass [list\
	    DomName [self]\
	    RelName $relName\
	    ClassName $subName\
	    RoleId $subRoleId\
	]]
	relvar insert ::raloo::ClassRoleInRel [list\
	    DomName [self]\
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
	    {fscript {}} {bscript {}}} {
	set gotSourceId [my ParseClassName $sourceClassName sourceClassName\
	    sourceIdNum]
	set gotTargetId [my ParseClassName $targetClassName targetClassName\
	    targetIdNum]
	if {![regexp -- {([1?*+])-*(\w+)-*>?([1?*+])} $spec\
		match sourceCard assocClassName targetCard]} {
	    error "unrecognized associative relationship specification,\
		\"$spec\""
	}
	if {$assocClassName eq $sourceClassName} {
	    error "the associative class, \"$assocClassName\", may not be\
		the same class as the source class, \"$sourceClassName\""
	}
	if {$assocClassName eq $targetClassName} {
	    error "the associative class, \"$assocClassName\", may not be\
		the same class as the target class, \"$targetClassName\""
	}
	# For reflexive relationship it is not allowed to have both
	# the forward and backward scripts empty. The inherent ambiguity
	# must be resolved by at least on reference script.
	if {$sourceClassName eq $targetClassName && $fscript eq {} &&\
		$bscript eq {}} {
	    error "reflexive relationships require at least one referential\
		definition to resolve the ambiguity"
	}
	relvar eval {
	    relvar insert ::raloo::Relationship [list\
		DomName [self]\
		RelName $name\
	    ]
	    relvar insert ::raloo::AssocRel [list\
		DomName [self]\
		RelName $name\
	    ]
	    relvar insert ::raloo::Associator [list\
		DomName [self]\
		RelName $name\
		ClassName $assocClassName\
		RoleId 0\
	    ]
	    set assocClass [relvar insert ::raloo::ReferringClass [list\
		DomName [self]\
		RelName $name\
		ClassName $assocClassName\
		RoleId 0\
	    ]]
	    relvar insert ::raloo::ClassRoleInRel [list\
		DomName [self]\
		RelName $name\
		ClassName $assocClassName\
		RoleId 0\
	    ]
	    relvar insert ::raloo::AssocSource [list\
		DomName [self]\
		RelName $name\
		ClassName $sourceClassName\
		RoleId 1\
		Cardinality $sourceCard\
	    ]
	    set sourceClass [relvar insert ::raloo::RefToClass [list\
		DomName [self]\
		RelName $name\
		ClassName $sourceClassName\
		RoleId 1\
	    ]]
	    relvar insert ::raloo::ClassRoleInRel [list\
		DomName [self]\
		RelName $name\
		ClassName $sourceClassName\
		RoleId 1\
	    ]
	    relvar insert ::raloo::AssocTarget [list\
		DomName [self]\
		RelName $name\
		ClassName $targetClassName\
		RoleId 2\
		Cardinality $targetCard\
	    ]
	    set targetClass [relvar insert ::raloo::RefToClass [list\
		DomName [self]\
		RelName $name\
		ClassName $targetClassName\
		RoleId 2\
	    ]]
	    relvar insert ::raloo::ClassRoleInRel [list\
		DomName [self]\
		RelName $name\
		ClassName $targetClassName\
		RoleId 2\
	    ]

	    # First we resolve the forward direction
	    if {$fscript eq {}} {
		# Make up the references from the associative class
		# to the target class.
		set targetRefToIdAttribute [my MakeRefToIdAttribute\
		    $targetClass $targetIdNum]
		my MakeAttributeRef $assocClass $targetRefToIdAttribute\
		    [my MakeIdentityRefMap\
			[relation list $targetRefToIdAttribute AttrName]]
	    } else {
		# Make a relation out of the script information.
		set targetAttrMapRel [my MakeAttrRefMap $fscript]
		# Find the identifier number implied by the script. It must
		# match the one specified for the referred to class if the
		# "*[1-9]" syntax was used.
		set foundIdNum [my FindRefToIdNum $targetClass\
			$targetAttrMapRel]
		if {$gotTargetId && $foundIdNum != $targetIdNum} {
		    error "referring attribute(s), \"[join\
			[relation list $attrMapRel RefngAttrName] {, }]\",\
			do(es) not refer to identifier,\
			\"$refToIdNum\" of class,\"$refToClassName\""
		}
		my MakeAttributeRef $assocClass\
		    [my MakeRefToIdAttribute $targetClass $targetIdNum]\
		    $targetAttrMapRel
	    }
	    # Then the backwards direction.
	    if {$bscript eq {}} {
		# Without a script we assume the attribute names are the same.
		# Make up the references from the associative class
		# to the source class.
		set srcRefToIdAttribute [my MakeRefToIdAttribute\
		    $sourceClass $sourceIdNum]
		my MakeAttributeRef $assocClass $srcRefToIdAttribute\
		    [my MakeIdentityRefMap\
			[relation list $srcRefToIdAttribute AttrName]]
	    } else {
		# Make a relation out of the script information.
		set sourceAttrMapRel [my MakeAttrRefMap $bscript]
		# Find the identifier number implied by the script. It must
		# match the one specified for the referred to class if the
		# "*[1-9]" syntax was used.
		set foundIdNum [my FindRefToIdNum $sourceClass\
			$sourceAttrMapRel]
		if {$gotSourceId && $foundIdNum != $sourceIdNum} {
		    error "referring attribute(s), \"[join\
			[relation list $attrMapRel RefngAttrName] {, }]\",\
			do(es) not refer to identifier,\
			\"$refToIdNum\" of class,\"$refToClassName\""
		}
		my MakeAttributeRef $assocClass\
		    [my MakeRefToIdAttribute $sourceClass $sourceIdNum]\
		    $sourceAttrMapRel
	    }
	}
	::raloo::AssocRelationship create [self]::$name
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
	    relvar insert ::raloo::Attribute [list\
		DomName $domName\
		ClassName $className\
		AttrName $attrName\
		AttrType $attrType\
	    ]
	    foreach idNum $idNumList {
		relvar union ::raloo::Identifier [relation create\
		    {DomName string ClassName string IdNum int}\
		    {{DomName ClassName IdNum}} [list\
		    DomName $domName\
		    ClassName $className\
		    IdNum $idNum\
		]]
		relvar insert ::raloo::IdAttribute [list\
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
	my variable defTrans
	set defTrans CH
	my DefineWith StateModel $script\
	    State\
	    Transition\
	    DefaultTransition\
	    DefaultInitialState\
	return
    }
    method State {name argList body} {
	my variable defState
	if {![info exists defState]} {
	    set defState $name
	}
	return
    }
    method Transition {start event end} {
	return
    }
    method DefaultTransition {trans} {
	my variable defTrans
	set defTrans $trans
	return
    }
    method DefaultInitialState {state} {
	my variable defState
	set defState $state
	return
    }
    method InstOp {name argList body} {
	my variable domName
	my variable className
	relvar insert ::raloo::InstOp [list\
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
	relvar insert ::raloo::ClassOp [list\
	    DomName $domName\
	    ClassName $className\
	    OpName $name\
	    OpParams $argList\
	    OpBody $body\
	]
	return
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

    method MakeRefToIdAttribute {refToClass idNum} {
	# Find the identifying attributes for the given "idNum".
	# Create the referred to identifying attributes
	set refToIdAttribute [ralutil::pipe {
	    relation semijoin $refToClass $::raloo::IdAttribute |
	    relation restrictwith ~ {$IdNum == $idNum} |
	    relation join $refToClass ~
	}]
	relvar union ::raloo::RefToIdAttribute $refToIdAttribute
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
	set refngAttrs [relation semijoin $attrRefs $::raloo::Attribute\
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
	relvar union ::raloo::AttributeRef $attrRefs
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
	    relation semijoin $refToClass $::raloo::IdAttribute |
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
    method selectOne {args} {
	set obj [my new]
	$obj set [::ralutil::pipe {
	    relvar set [self] |
	    relation choose ~ {*}$args
	}]
	return $obj
    }
}

oo::class create ::raloo::RelvarRef {
    constructor {name args} {
	namespace import ::ral::*
	my variable relvarName
	set relvarName $name
	my variable ref
	set ref [relation emptyof [relvar set $relvarName]]
	namespace path [concat [namespace path]\
		[namespace qualifiers $relvarName]]

	# constructor for object ==> reference
	if {[llength $args] != 0} {
	    # relvar insert returns just what was inserted.
	    set ref [relvar insert $relvarName $args]
	}
	set ref [relation project $ref\
	    {*}[lindex [relation identifiers $ref] 0]]
    }
    # Convert a relation value contained in the base relvar into a reference.
    method set {relValue} {
	my variable relvarName
	if {[relation isnotempty $relValue] && [relation isempty\
	    [relation intersect [relvar set $relvarName] $relValue]]} {
	    error "relation value is not contained in $relvarName"
	}
	my variable ref
	set ref [relation project $relValue\
	    {*}[lindex [relation identifiers $relValue] 0]]
    }
    # Find the tuples in the base relvar that this reference actually refers
    # to.
    method get {} {
	my variable ref
	my variable relvarName
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
    method selectWhere {expr} {
	my variable relvarName
	::ralutil::pipe {
	    relvar set $relvarName |
	    relation restrictwith ~ [list $expr]
	} cmd
	my set [uplevel $cmd]
    }
    method selectAny {expr} {
	my variable relvarName
	::ralutil::pipe {
	    relvar set $relvarName |
	    relation restrictwith ~ [list $expr] |
	    relation tag ~ __Order__ |
	    relation choose ~ __Order__ 0
	} cmd
	my set [uplevel $cmd]
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
	# Cf.  __UpdateAttr.
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
	# Cf.  __UpdateAttr.
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
    method migrate {rName targetClass args} {
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
	# Add on any addition attribute/values required by the migration.
	set avList [concat $avList $args]
	# Transform "self" into the new subtype.
	my variable relvarName
	relvar eval {
	    my delete
	    set subObj [$targetClass new {*}$avList]
	}
	return $subObj
    }
    # Unexported methods.
    #
    # Update an attribute in the base relvar to which this reference
    # refers.
    method __UpdateAttr {attrName value} {
	my variable ref
	my variable relvarName

	set updates [list]
	relation foreach r $ref {
	    lappend updates [relvar updateone $relvarName tup\
		    [tuple get [relation tuple $r]] {
		    tuple update tup $attrName $value
		}]
	}
	my set [relation union\
	    [relation emptyof [relvar set $relvarName]] {*}$updates]
    }
    unexport __UpdateAttr
}

# This is a meta-class for classes based on relvars.
oo::class create ::raloo::PasvRelvarClass {
    superclass ::oo::class ::raloo::RelvarClass
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
	relvar update ::raloo::Attribute attr {
	    [tuple extract $attr DomName] eq $domName &&
	    [tuple extract $attr ClassName] eq $className &&
	    [tuple extract $attr AttrType] eq "UNIQUE"} {
	    lappend uniqueAttrs [tuple extract $attr AttrName]
	    tuple update attr AttrType int
	}

	# Second a dictionary of attribute name / attribute type.
	lappend relHeading [ralutil::pipe {
	    relation restrictwith $::raloo::Attribute {
		$DomName eq $domName && $ClassName eq $className} |
	    relation dict ~ AttrName AttrType
	}]

	# Third a list of identifiers. Each identifier is in turn a list
	# of the attribute names that make up the identifier.
	set ids [ralutil::pipe {
	    relation restrictwith $::raloo::IdAttribute {
		$DomName eq $domName && $ClassName eq $className} |
	    relation group ~ IdAttr AttrName
	}]
	set idList [list]
	relation foreach id $ids -ascending IdNum {
	    lappend idList [relation list [relation extract $id IdAttr]]
	}
	lappend relHeading $idList

	relvar create [self] $relHeading
	# Now that the relvar exists, we can add traces
	foreach attr $uniqueAttrs {
	    ::raloo::uniqueId [self] $attr
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
	# Define a method for each attribute. If the attribute is an
	# identifier, then updates are not allowed and the method interface
	# will not accept the additional argument.  Make sure that there is no
	# "currentstate" method. "currentstate" is strictly controlled
	# internally.
	set attributes [relation restrictwith $::raloo::Attribute {
		$DomName eq $domName && $ClassName eq $className &&\
		$AttrName ne "__currentstate__"}]
	set attrNotIds [ralutil::pipe {
	    relvar set ::raloo::IdAttribute |
	    relation semiminus ~ $attributes |
	    relation list ~ AttrName
	}]
	foreach attr $attrNotIds {
	    oo::define [self] method $attr {{val {}}} [format {
		if {$val ne ""} {
		    my __UpdateAttr %1$s $val
		}
		relation extract [my get] %1$s
	    } $attr]
	    oo::define [self] export $attr
	}
	set attrAsIds [ralutil::pipe {
	    relvar set ::raloo::IdAttribute |
	    relation semijoin ~ $attributes |
	    relation list ~ AttrName
	}]
	foreach attr $attrAsIds {
	    oo::define [self] method $attr {} [format {
		relation extract [my get] %s
	    } $attr]
	    oo::define [self] export $attr
	}

	# Class operations are turned into class methods.
	relation foreach op [relation restrictwith $::raloo::ClassOp {
		$DomName eq $domName && $ClassName eq $className}] {
	    relation assign $op OpName OpParams OpBody
	    oo::define [self] self.method $OpName $OpParams $OpBody
	}
	# Instance operations are turned into ordinary methods.
	relation foreach op [relation restrictwith $::raloo::InstOp {
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
		relvar delete ::raloo::Attribute co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::Identifier co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::IdAttribute co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::ClassOp co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar delete ::raloo::InstOp co {
		    [tuple extract $co DomName] eq $domName &&\
		    [tuple extract $co ClassName] eq $className
		}
		relvar deleteone ::raloo::Class\
			DomName $domName ClassName $className
	    }
	    relvar unset [self]
	} result]} {
	    puts "Class: $result"
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
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	if {[catch {
	    relvar deleteone ::raloo::Relationship\
		DomName $domName RelName $relName
	    relvar delete ::raloo::ReferringClass r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::RefToClass r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::ClassRoleInRel r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::RefToIdAttribute r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
	    relvar delete ::raloo::AttributeRef r {
		[tuple extract $r DomName] eq $domName &&
		[tuple extract $r RelName] eq $relName}
		} result]} {
	    puts "RelBase: $result"
	}
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

	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	set simplerel [ralutil::pipe {
	    relation choose $::raloo::Relationship DomName $domName\
		RelName $relName |
	    relation semijoin ~ $::raloo::SimpleRel
	}]

	set simplereferring [relation semijoin $simplerel\
		$::raloo::SimpleReferring]
	my variable refngClass
	set refngClass [relation extract $simplereferring ClassName]
	set refngCard [relation extract $simplereferring Cardinality]

	set simplerefto [relation semijoin $simplerel $::raloo::SimpleRefTo]
	my variable reftoClass
	set reftoClass [relation extract $simplerefto ClassName]
	set reftoCard [relation extract $simplerefto Cardinality]

	# We create a dictionary that maps a referring attribute name
	# to the referred to attribute name. Note we are depending upon
	# the behavior of dictionaries to return keys and values in
	# the same order that they were put into the dictionary.
	set attrMap [ralutil::pipe {
	    relation semijoin $simplereferring $::raloo::ReferringClass\
		$::raloo::AttributeRef -using\
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
    }
    destructor {
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::SimpleRel\
		    DomName $domName RelName $relName
		relvar delete ::raloo::SimpleReferring r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::SimpleRefTo r {
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
	    relation choose $::raloo::Relationship DomName $domName\
		RelName $relName |
	    relation semijoin ~ $::raloo::GenRel
	}]
	set supertype [relation semijoin $genrel $::raloo::Supertype]

	my variable superClass
	set superClass [relation extract $supertype ClassName]
	set partCmd [list relvar partition [self] ${domName}::$superClass]

	# Find all the attribute references by the subtypes.
	set attributeref [ralutil::pipe {
	    relation semijoin\
		$supertype\
		$::raloo::RefToClass\
		$::raloo::RefToIdAttribute\
		$::raloo::AttributeRef -using\
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
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::GenRel\
		    DomName $domName RelName $relName
		relvar delete ::raloo::Supertype r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::Subtype r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
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
	namespace import ::ral::*

	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]

	set assocrel [ralutil::pipe {
	    relation choose $::raloo::Relationship DomName $domName\
		RelName $relName |
	    relation semijoin ~ $::raloo::AssocRel
	}]

	set associator [relation semijoin $assocrel $::raloo::Associator]
	my variable assocClassName
	set assocClassName [relation extract $associator ClassName]

	set assocsource [relation semijoin $assocrel $::raloo::AssocSource]
	my variable sourceClassName
	set sourceClassName [relation extract $assocsource ClassName]
	set sourceCard [relation extract $assocsource Cardinality]

	set assoctarget [relation semijoin $assocrel $::raloo::AssocTarget]
	my variable targetClassName
	set targetClassName [relation extract $assoctarget ClassName]
	set targetCard [relation extract $assoctarget Cardinality]

	# Create two dictionaries. One for the references to the source
	# class and the other for the references to the target class.
	# First retrieve all the attributes references
	set attributeref [relation semijoin\
	    $associator\
	    $::raloo::ReferringClass\
	    $::raloo::AttributeRef -using\
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
    }
    destructor {
	set domName [namespace qualifiers [self]]
	set relName [namespace tail [self]]
	if {[catch {
	    relvar eval {
		relvar deleteone ::raloo::AssocRel\
		    DomName $domName RelName $relName
		relvar delete ::raloo::Associator r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::AssocSource r {
		    [tuple extract $r DomName] eq $domName &&
		    [tuple extract $r RelName] eq $relName}
		relvar delete ::raloo::AssocTarget r {
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
