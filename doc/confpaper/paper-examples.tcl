#!/bin/sh
# \
exec tclsh "$0" "$@"

package require ral
namespace import ::ral::*

proc displayExample {script} {
    puts "\[example \{"

    set cmd ""
    foreach line [split $script "\n"] {
	if {[string length $line] == 0} continue
	set line [string trim $line]

	if {[string index $line 0] == "%"} {
	    catch [string range $line 1 end]
	    continue
	}
	if {[string length $cmd] == 0} {
	    puts "% $line"
	} else {
	    puts "> $line"
	}
	append cmd $line "\n"
	if {[info complete $cmd]} {
	    catch $cmd result
	    if {[string length $result]} {
		puts $result
	    }
	    set cmd ""
	}
    }
    puts "\}\]\n"
}

relvar create Dog {
    Relation
    {DogName string Breed string Age int}
    DogName
}
relvar set Dog {
    Relation
    {DogName string Breed string Age int}
    DogName
    {
	{DogName Fido Breed Poodle Age 2}
	{DogName Sam Breed Collie Age 4}
	{DogName Spot Breed Terrier Age 1}
	{DogName Rover Breed Retriever Age 5}
	{DogName Fred Breed Spaniel Age 7}
	{DogName Jumper Breed Mutt Age 3}
    }
}
puts [relformat $Dog Dog]

relvar create Owner {
    Relation
    {OwnerName string Age int}
    OwnerName
}
relvar set Owner {
    Relation
    {OwnerName string Age int}
    OwnerName
    {
	{OwnerName Sue Age 24}
	{OwnerName George Age 35}
	{OwnerName Alice Age 30}
	{OwnerName Mike Age 50}
	{OwnerName Jim Age 42}
    }
}
puts [relformat $Owner Owner]

relvar create Ownership {
    Relation
    {OwnerName string DogName string Acquired string}
    {{OwnerName DogName}}
}
relvar set Ownership {
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
puts [relformat $Ownership Ownership]

relvar create Contact {
    Relation
    {OwnerName string ContactOrder int}
    {{OwnerName ContactOrder}}
}
relvar set Contact {
    Relation
    {OwnerName string ContactOrder int}
    {{OwnerName ContactOrder}}
    {
	{OwnerName Sue ContactOrder 1}
	{OwnerName Sue ContactOrder 2}
	{OwnerName George ContactOrder 1}
	{OwnerName Alice ContactOrder 1}
	{OwnerName Mike ContactOrder 1}
	{OwnerName Mike ContactOrder 2}
	{OwnerName Mike ContactOrder 3}
	{OwnerName Jim ContactOrder 1}
    }
}
puts [relformat $Contact Contact]

relvar create PhoneNumber {
    Relation
    {OwnerName string ContactOrder int AreaCode string Number string}
    {{OwnerName ContactOrder}}
}
relvar set PhoneNumber {
    Relation
    {OwnerName string ContactOrder int AreaCode string Number string}
    {{OwnerName ContactOrder}}
    {
	{OwnerName Sue ContactOrder 1 AreaCode 111 Number 555-1212}
	{OwnerName George ContactOrder 1 AreaCode 408 Number 555-2020}
	{OwnerName Alice ContactOrder 1 AreaCode 555 Number 867-4309}
	{OwnerName Mike ContactOrder 1 AreaCode 800 Number 555-3890}
	{OwnerName Mike ContactOrder 2 AreaCode 866 Number 555-8821}
    }
}
puts [relformat $PhoneNumber PhoneNumber]

relvar create EmailAddress {
    Relation
    {OwnerName string ContactOrder int UserName string DomainName string}
    {{OwnerName ContactOrder}}
}
relvar set EmailAddress {
    Relation
    {OwnerName string ContactOrder int UserName string DomainName string}
    {{OwnerName ContactOrder}}
    {
	{OwnerName Sue ContactOrder 2 UserName sue DomainName mymail.com}
	{OwnerName Mike ContactOrder 3 UserName mikey DomainName yourmail.com}
	{OwnerName Jim ContactOrder 1 UserName jimbo DomainName domail.com}
    }
}
puts [relformat $EmailAddress EmailAddress]

relvar correlation R1 Ownership\
    OwnerName + Owner OwnerName\
    DogName * Dog DogName
relvar association R2 Contact OwnerName + Owner OwnerName 1
relvar partition R3\
    Contact {OwnerName ContactOrder}\
    PhoneNumber {OwnerName ContactOrder}\
    EmailAddress {OwnerName ContactOrder}

catch {relvar insert Owner {OwnerName Tom Age 22}} result
puts $result

puts [dict get [relation dict $Owner] Mike]
puts [relation array $Owner ownerarray]
parray ownerarray

relation array [relation eliminate $Dog Age] dogbreed
parray dogbreed

set newDog {
    Relation
    {DogName string Breed string Age int}
    DogName
    {
	{DogName Puffy Breed Poodle Age 1}
    }
}

puts [relformat [relation union $Dog $newDog]]

puts [relation is $Dog < [relation include $Dog\
    {DogName Puffy Breed Poodle Age 1}]]
puts [relation is $newDog == $Dog]

# restrict and friends

puts "========== restrict ==============="
puts [relformat [relation restrict $Dog d {[tuple extract $d Age] < 3}]]
puts [relformat [relation restrictwith $Dog {$Age <= 3}]]
puts [relformat [relation choose $Owner OwnerName Mike]]
puts [relformat [relation project $Ownership OwnerName Acquired]]
puts [relformat [relation eliminate $Dog Age]]
puts "========== restrict end ==============="


puts "========== Tcl data structures ==============="
puts [relation list [relation project $Dog DogName]]

puts "========== group ==============="
puts [relformat [relation group $Ownership DogAcquisition DogName Acquired]]
puts "========== group end ==============="


puts "========== introspection ==============="
displayExample {
%global Dog
relation cardinality $Dog
relation isempty [relation emptyof $Dog]
relation isnotempty $Dog
relation degree $Dog
relation heading $Dog
relation attributes $Dog
relation identifiers $Dog
}
puts "========== introspection end ==============="

puts "========== extend ==============="
displayExample {
%global Dog
relformat [relation eliminate [relation extend $Dog d AgeInMonths int {[tuple extract $d Age] * 12}] Age]
}
puts "========== extend end ==============="

puts "========== summarize ==============="
displayExample {
%global Ownership
relformat [relation summarize\
    $Ownership [relation project $Ownership Acquired] o\
    CountPerYear int {[relation cardinality $o]}]
}
displayExample {
%global Dog
proc ravg {rel attr} {
    set sum 0
    foreach val [relation list $rel $attr] {
	incr sum $val
    }
    return [expr {$sum / [relation cardinality $rel]}]
}
set DEE {Relation {} {{}} {{}}}
relformat [relation summarize\
    $Dog $DEE o\
    OverallAvgAge int {[ravg $o Age]}]
}
puts "========== summarize end ==============="

puts "========== mult and div ==============="
displayExample {
%global Dog
%global Owner
%global Contact
%global PhoneNumber
relformat [relation times $Owner [relation rename $Dog Age DogAge]]
relformat [relation join $Owner $Contact]
relformat [relation semijoin $PhoneNumber $Contact]
relformat [relation semiminus $PhoneNumber $Contact]
}
puts "========== mult and div end ==============="

puts "========== tclose ==============="
displayExample {
set includes {
    Relation
    {Src string Inc string}
    {{Src Inc}}
    {
	{Src a.c Inc a.h}
	{Src a.c Inc b.h}
	{Src a.h Inc aa.h}
	{Src aa.h Inc stdio.h}
	{Src b.h Inc stdio.h}
    }
}
relformat [set tclosure [relation tclose $includes]]
relation list [relation project\
    [relation restrictwith $tclosure {$Src eq "a.c"}]\
    Inc]
}
puts "========== tclose end ==============="

puts "========== foreach ==============="
displayExample {
%global Dog
relation foreach d $Dog -descending DogName {
    tuple assign [relation tuple $d]
    puts $DogName
}
}
puts "========== foreach end ==============="

puts "========== rank ==============="
displayExample {
%global Dog
relformat [set rankedDogs [relation rank $Dog Age AgeRank]]
relformat [relation project [relation restrictwith $rankedDogs {$AgeRank <= 2}] DogName]
}
puts "========== rank end ==============="

puts "========== tag ==============="
displayExample {
%global Ownership
relformat [relation tag $Ownership -ascending Acquired -within OwnerName AcqTag]
}
puts "========== tag end ==============="

puts "========== constraint ==============="
displayExample {
lsort [relvar constraint names]
relvar constraint info R1
relvar constraint info R3
}
puts "========== constraint end ==============="

puts "========== insert ==============="
displayExample {
%global Dog
relformat [relvar create MyDog [relation heading $Dog]]
relformat [relvar insert MyDog\
    {Age 10 DogName Joan Breed {Afghan Hound}}\
    {Breed Dachshund Age 1 DogName Alfred}]
}
puts "========== insert end ==============="

puts "========== delete ==============="
displayExample {
%global MyDog
relvar delete MyDog d {[tuple extract $d Age] > 2}
relformat $MyDog
relvar deleteone MyDog DogName Alfred
relformat $MyDog
}
puts "========== delete end ==============="
relvar insert MyDog\
    {Age 10 DogName Joan Breed {Afghan Hound}}\
    {Breed Dachshund Age 1 DogName Alfred}

puts "========== update ==============="
displayExample {
%global MyDog
relvar update MyDog d {[tuple extract $d Age] > 2} {
    tuple update d Age [expr {[tuple extract $d Age] + 1}]
}
relvar updateone MyDog d {DogName Alfred} {
    tuple update d Age [expr {[tuple extract $d Age] + 1}]
}
relformat $MyDog
}
puts "========== update end ==============="

set o {
    Relation
    {OwnerName string Age int}
    OwnerName
    {
	{OwnerName Alice Age 30}
	{OwnerName Sue Age 24}
	{OwnerName Mike Age 50}
    }
}
puts [relformat [relation rank $o Age AgeRank]]
