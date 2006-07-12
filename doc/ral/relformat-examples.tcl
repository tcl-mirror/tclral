#!/bin/sh
# \
exec tclsh "$0" "$@"

package require ral

proc displayExample {script} {
    puts "\[example \{"

    set cmd ""
    foreach line [split $script "\n"] {
	if {[string length $line] == 0} continue
	if {[string length $cmd] == 0} {
	    puts "% [string trim $line]"
	} else {
	    puts "> $line"
	}
	append cmd " " [string trim $line]
	if {[info complete $cmd]} {
	    catch $line result
	    if {[string length $result]} {
		puts $result
	    }
	    set cmd ""
	}
    }
    puts "\}\]\n"
}

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

# Example for the relformat manual page.

namespace import ::ral::*
relvar create OWNER {
    Relation
    {OwnerName string Age int City string}
    OwnerName
}
relvar create DOG {
    Relation
    {DogName string Breed string}
    DogName
}
relvar create OWNERSHIP {
    Relation
    {OwnerName string DogName string Acquired string}
    {
	{OwnerName DogName}
    }
}
setOwner
setDog
setOwnership

# relformat
displayExample {
    puts [relformat $::DOG Dog]
    puts [relformat $::OWNERSHIP Ownership {DogName Acquired}]
    puts [relformat $::OWNER Owner "" 1]
    puts [relformat [relation group $::OWNERSHIP Owners OwnerName] "" DogName]
}
