#!/usr/bin/env tclsh

source ../raloo.tcl

namespace import ::raloo::*

# The only command exported by "raloo" is "Domain".
# Domain only supports three methods: create, delete and transaction.
#
# Domains are created by supplying a name and a definition script.
# The definition script is a Tcl script that is evaluated in a context
# were special definition procs are available. One such definition
# proc is "Class" which is used to define classes.

Domain create DogMgmt {
    # Similar to Domains, a Class is created by invoking "Class" with
    # a name and definition script.
    Class Dog {
	# The Attribute proc specifies the attributes of the class.  Attributes
	# are specified as a list of pairs, giving the attribute name and
	# attribute type. The attribute type is a Tcl type name. The
	# identifiers of a class are those attributes that begin with an
	# asterisk (*). If the class has multiple identifiers, then a decimal
	# digit from 1 - 9 may follow the asterisk. An asterisk with no digit
	# is taken as "*1". If a given attribute is part of multiple
	# identifiers then it must list all the  identifiers of which it is a
	# part, e.g. **2Name is part of identifier 1 and 2.  You may invoke the
	# "Attribute" command as many times as necessary and the effect is
	# cumulative.  In the example, the "Dog" class has two identifiers,
	# each consisting of a single attribute. The second identifier is typed
	# as "UNIQUE".  This is a special pseudo type which requests that the
	# system supply some identifier that is unique within the class.
	Attribute {
	    *Name string
	    *2Id UNIQUE
	    Breed string
	    Age int
	}

	# In addition to attributes one can define class-based and
	# instance-based operations. These are defined via the "ClassOp" and
	# "InstOp" commands. These commands have the same interface as "proc".
	ClassOp dogList {} {
	    return [relation list [my get] Name]
	}
	#
	# Later, there will be a way to specify lifecycle information.
    }
    # Each defined class creates a class command that is the same name
    # as the class.
    Class Owner {
	Attribute {
	    *Name string
	    Age int
	}
    }
    Class Ownership {
	Attribute {
	    *OwnerName string
	    *DogName string
	    Acquired string
	}
    }
    Class Contact {
	Attribute {
	    *Name string
	    *ContactOrder int
	}
    }
    Class PhoneNumber {
	Attribute {
	    *Name string
	    *ContactOrder int
	    AreaCode string
	    Number string
	}
    }
    Class EmailAddress {
	Attribute {
	    *Name string
	    *ContactOrder int
	    UserName string
	    DomainName string
	}
    }

    # Simple relationships are specified using the "Relationship" command.
    # This command takes the name of the relationship, the referring class, a
    # third argument giving the relationship details and finally the referred
    # to class.  The name of the relationship is arbitrary, but by convention
    # names of the form "Rddd" are chosen, where "ddd" is a decimal number.
    # The third argument gives the cardinality at each end.  The hyphens (-)
    # and greater than (>) are syntactic sugar, optional and ignored. The
    # relationship has a direction from left to right and the direction of the
    # relationship must be from the referring class to the referred to class.
    # The direction of a simple relationship is significant when the
    # relationship is reflexive.  The cardinality of the left end may be *, +,
    # ?  or 1 and that of the right side may be either ?  or 1.  In addition,
    # the corresponding attribute reference must be specified. This
    # correspondence specifies which attribute in the referring relation refers
    # to which attribute in the referred to class.

    Relationship R1 Contact +-->1 Owner

    # Frequently, the names of the referring and referred to attributes are the
    # same. In this case they need not be specified, as is the case of the
    # example.  If the referring attribute correspondence is specified, then
    # the "RefMap" command is used in the defintion of the relationship.
    # RefMap takes a list of triples to define the referential correspondence.
    # For the example it would be:
    # Relationship R1 Contact +-->1 Owner {
    #	RefMap {Name -> Name}
    # }
    # The middle part of the list is syntactic sugar and ignored. It is meant
    # to be mnemonic of the direction of reference of the attribute values.
    # The set of referred to attributes must constitute an identifier of the
    # referred to class.  By default the identifier of the referred to class is
    # "1". You may specify an alternate one using the asterisk notation on the
    # referred to class. Assuming some relationship R20, that refers to the
    # second identifier of Dog, it might be specified as:
    # Relationship R20 Certificate 1-->1 *2Dog {
    #	RefMap {DogId -> Id}
    # }

    # Associative relationships are specified with the "AssocRelationship"
    # command. This has similar syntax to the simple relationship. Here, the
    # third argument also specifies the name of the associative class.  As with
    # the simple relationship, the "-" and ">" are optional and ignored.
    # The definition of an associative relationship can also specify the
    # referential attribute mapping. For associative relationships the
    # mapping can be specified in the forward and backward directions using
    # the "FwrdRefMap" and "BackRefMap" commands.
    # FwrdRefMap specifies the attributes in the
    # associative class that refer in the forward direction (i.e. toward the
    # right hand Class. BackRefMap gives the attribute
    # references in the associate class that refer backwards (i.e.  toward the
    # left hand Class. For reflexive relationships, both lists must be
    # specified (since using the "same name" rule is ambiguous in the reflexive
    # case). For the non-reflexive case, one or both of the attribute reference
    # commands may be absent and then attributes of the same name are assumed.
    # Either or both of the participating classes may use the asterisk (*)
    # notation to request that the reference be to an identifier other than
    # "*1" (since both class are referred to by the associative class).
    # For the example, both attribute reference list arguments must be given
    # since the referred to attributes in Dog and Owner have the same name.

    AssocRelationship R2 Dog +--Ownership-->* Owner {
	FwrdRefMap {
	    OwnerName -> Name
	}
	BackRefMap {
	    DogName -> Name
	}
    }

    # The Generalization command is used to define a generalization
    # relationship. The arguments are the name of the relationships, the
    # supertype class name and a definition script. The definition script can
    # invoke the "SubType" command to define the subtypes of the
    # generalization. Similar to the other types of relationship commands, the
    # "SubType" command can take an optional list of triples giving the
    # attribute references from the subtype to the supertype. The supertype
    # class name may use the asterisk (*) notation if an idenifier other than
    # "*1" is to be referred to. In the very common case where the subtype
    # referential attributes are the same name as those in the supertype (and
    # are usually an identifier in the subtype) no additional specification is
    # necessary, as is the case in the example.
    Generalization R3 Contact {
	SubType PhoneNumber
	SubType EmailAddress
    }
}

# Domain definitions may be extended by the "define" method. This evaluates
# a definition script in much the same way as the "create" method, adding
# any additional components to the existing Domain. This allows separating
# domain definitions into more manageable chunks.
DogMgmt define {
    # Another type of component of a Domain is a synchronous service.  The
    # "SyncService" command defines a synchronous service. Its interface is the
    # same as the "proc" command. Synchronous services provide a procedural
    # interface to the Domain that leaves the class structure consistent. They
    # are run as a transaction on the data of the Domain, so at the end of each
    # domain operation the referential integrity of the domain data is
    # verified.

    SyncService newDog {name breed age} {
	Dog insert Name $name Id {} Breed $breed Age $age
	return
    }

    # Within a domain operation, classes and other domain components can
    # be referred to without any qualification.

    SyncService firstPurchase {ownerName age area phone dogName} {
	set dog [Dog new Name $dogName]
	if {[$dog isempty]} {
	    error "no such dog, \"$dogName\""
	}
	Owner insert Name $ownerName Age $age
	set owner [Owner new Name $ownerName]
	Contact insert Name $ownerName ContactOrder 1
	PhoneNumber insert Name $ownerName ContactOrder 1 AreaCode $area\
	    Number $phone
	$owner relateAssoc R2 $dog Acquired [clock format [clock seconds]]
	return
    }

    SyncService ownerReport {} {
	return [Ownership format OwnerName]
    }

    SyncService dogList {} {
	return [Dog dogList]
    }
}

# Domains also support executing arbitrary code within a transaction of the
# domain. This is useful to populate the classes in a domain.
DogMgmt transaction {
    Dog insert Name Alice Id {} Breed Retriever Age 1
    Dog insert Name Fred Id {} Breed Beagle Age 3
    Dog insert Name Rover Id {} Breed Terrier Age 1
}

DogMgmt newDog Fido Poodle 2
DogMgmt firstPurchase John 24 650 555-1212 Fido
puts [DogMgmt ownerReport]
puts [DogMgmt dogList]
