#!/bin/sh
# \
exec tclsh "$0" "$@"

package require ral

source displayExample.tcl

proc setOwner {} {
    relvar set OWNER {
	Relation
	{OwnerName string Age int City string}
	OwnerName
	{
	    {OwnerName Sue Age 24 City Cupertino}
	    {OwnerName George Age 35 City Sunnyvale}
	    {OwnerName Alice Age 30 City {San Jose}}
	    {OwnerName Mike Age 50 City {San Jose}}
	    {OwnerName Jim Age 42 City {San Francisco}}
	}
    }
}

proc setDog {} {
    relvar set DOG {
	Relation
	{DogName string Breed string}
	DogName
	{
	    {DogName Fido Breed Poodle}
	    {DogName Sam Breed Collie}
	    {DogName Spot Breed Terrier}
	    {DogName Rover Breed Retriever}
	    {DogName Fred Breed Spaniel}
	    {DogName Jumper Breed Mutt}
	}
    }
}

proc setOwnership {} {
    relvar set OWNERSHIP {
	Relation
	{OwnerName string DogName string Acquired string}
	{{OwnerName DogName}}
	{
	    {OwnerName Sue DogName Fido Acquired 2001}
	    {OwnerName Sue DogName Sam Acquired 2000}
	    {OwnerName George DogName Fido Acquired 2001}
	    {OwnerName George DogName Sam Acquired 2000}
	    {OwnerName Alice DogName Spot Acquired 2001}
	    {OwnerName Mike DogName Rover Acquired 2002}
	    {OwnerName Jim DogName Fred Acquired 2003}
	}
    }
}

proc setAll {} {
    relvar eval {
	setOwner
	setDog
	setOwnership
    }
}

# Example for the tuple manual page.

namespace import ::ral::*

# relvar create
displayExample {
    relvar create OWNER {
	Relation
	{OwnerName string Age int City string}
	OwnerName
    }
    puts [relformat $::OWNER]
    relvar create DOG {
	Relation
	{DogName string Breed string}
	DogName
    }
    relvar create OWNERSHIP {
	Relation
	{OwnerName string DogName string Acquired string}
	{{OwnerName DogName}}
    }
}

# relvar association
displayExample {
    relvar association A1 OWNERSHIP OwnerName + OWNER OwnerName 1
    relvar association A2 OWNERSHIP DogName * DOG DogName 1
}

setAll

#relvar delete
displayExample {
    puts [relformat $::OWNER "Before deleting Sue"]
    relvar delete OWNER o {[tuple extract $o OwnerName] eq "Sue"}
    puts [relformat $::OWNER "After deleting Sue"]
}
setOwner

#relvar deleteone
setOwner
displayExample {
    puts [relformat $::OWNER "Before deleting Mike"]
    relvar deleteone OWNER OwnerName Mike
    puts [relformat $::OWNER "After deleting Mike"]
    relvar deleteone OWNER OwnerName Alfonse
    puts [relformat $::OWNER]
}
setOwner

#relvar eval
displayExample {
    puts [relformat $::OWNER]
    relvar eval {
	relvar insert OWNER {OwnerName Tom Age 22 City Tulsa}
    }
    puts [relformat $::OWNER]
    relvar eval {
	relvar insert OWNER {OwnerName Tom Age 22 City Tulsa}
	relvar insert OWNERSHIP {OwnerName Tom DogName Skippy Acquired 2006}
    }
    puts [relformat $::OWNER]
    relvar eval {
	relvar insert OWNER {OwnerName Tom Age 22 City Tulsa}
	relvar insert OWNERSHIP {OwnerName Tom DogName Jumper Acquired 2006}
	return
    }
    puts [relformat $::OWNER]
}
setAll

#relvar insert
displayExample {
    relvar insert DOG {DogName Skippy Breed Dalmation}
    relvar insert OWNER {OwnerName Tom Age 22 City Tulsa}
}
setAll

#relvar names
displayExample {
    relvar names
    relvar names ::D*
}

#relvar constraint names
displayExample {
    relvar constraint names
    relvar constraint names *2
}

#relvar constraint info
displayExample {
    relvar constraint info A1
    relvar constraint info ::A2
}

#relvar partition
displayExample {
    relvar create Lamp {
	Relation
	{SerialNo string ModelNo string Make string}
	SerialNo
    }
    relvar create TableLamp {
	Relation
	{SerialNo string Shade string}
	SerialNo
    }
    relvar create FloorLamp {
	Relation
	{SerialNo string Height int Sockets int}
	SerialNo
    }

    relvar partition P1 Lamp SerialNo TableLamp SerialNo FloorLamp SerialNo

    relvar insert Lamp {SerialNo NF100 ModelNo FCN-22  Make Falcon}
    relvar eval {
	relvar insert Lamp {SerialNo NF100 ModelNo FCN-22  Make Falcon}
	relvar insert TableLamp {SerialNo NF100  Shade Blue}
	return
    }
    relvar insert FloorLamp {SerialNo NF100 Height 72 Sockets 3}
    relvar insert FloorLamp {SerialNo NF101 Height 72 Sockets 3}
}

#relvar constraint info
displayExample {
    relvar constraint info A1
    relvar constraint info ::P1
    relvar constraint info G
}

#relvar update
displayExample {
    puts [relformat $::OWNER]
    relvar update OWNER o {[tuple extract $o OwnerName] eq "Sue"} {
	tuple update o Age [expr {[tuple extract $o Age] + 1}]
    }
    puts [relformat $::OWNER]
    relvar update OWNER o {[tuple extract $o OwnerName] eq "George"} {
	tuple update o OwnerName Alfonse
    }
    puts [relformat $::OWNER]
}
setAll

#relvar updateone
displayExample {
    puts [relformat $::OWNER]
    relvar updateone OWNER {OwnerName George} {Age 37 City {New York}}
    puts [relformat $::OWNER]
}
setAll

#relvar set
displayExample {
    relvar create PUPPY {
	Relation
	{PuppyName string Dame string Sire string}
	PuppyName
    }
    relvar set PUPPY {
	Relation
	{PuppyName string Dame string Sire string}
	PuppyName
	{
	    {PuppyName Bitsy Dame Spot Sire Jumper}
	}
    }
    relvar set PUPPY $::DOG
}
