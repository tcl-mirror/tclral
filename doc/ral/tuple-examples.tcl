#!/bin/sh
# \
exec tclsh "$0" "$@"

package require ral

source displayExample.tcl

# Example for the tuple manual page.

namespace import ::ral::*

# tuple assign
displayExample {
    tuple assign {Tuple {Name string Age int} {Name John Age 25}}
    set Name
    set Age
}

# tuple create
displayExample {
    tuple create {Number int Street string Zip string}\
	{Number 100 Zip 90214 Street Elm}

    tuple create {Number int Street string Zip string}\
	{Number hundred Zip 90214 Street Elm}

    tuple create {Number int Street string Zip string}\
	{Number 100 Zip 90214}

    tuple create {Number int Street string Zip string}\
	{Number 100 Zip 90214 Number 100}

    tuple create {Number int Street string Zip string}\
	{Number 100 Zip 90214 Street Elm Email john@net.net}
}

# tuple degree
displayExample {
    tuple degree {Tuple {Name string Age int} {Name John Age 25}}
    tuple degree {Tuple {} {}}
}

# tuple eliminate
displayExample {
    set t1 [tuple create {Name string Age int} {Name John Age 25}]
    tuple eliminate $t1
    tuple eliminate $t1 Name
    tuple eliminate $t1 Street
}

# tuple equal
displayExample {
    set t1 [tuple create {Name string Status int} {Name Fred Status 20}]
    set t2 [tuple create {Status int Name string} {Name Fred Status 20}]
    tuple equal $t1 $t2
}

# tuple extend
displayExample {
    set t1 [tuple create {Name string} {Name Fred}]
    tuple extend $t1 {Age int 30} {Sex string Male}
    tuple extend $t1 {Name string Jane}
    tuple extend $t1 {Age int Thirty}
}

# tuple extract
displayExample {
    set t1 [tuple create {Name string Age int} {Name {Fred Jones} Age 27}]
    tuple extract $t1 Name
    tuple extract $t1 Age Name
    tuple extract $t1 Status
}

# tuple equal
displayExample {
    set t1 {Tuple {Name string Age int} {Name {Fred Jones} Age 27}}
    set tinfo [tuple get $t1]
    array set ainfo $tinfo
    parray ainfo
    dict keys $tinfo
}

# tuple heading
displayExample {
    set t {Tuple {Name string Age int} {Name {Fred Jones} Age 27}}
    tuple heading $t
}

# tuple project
displayExample {
    set t {Tuple {Name string Age int} {Name {Fred Jones} Age 27}}
    tuple project $t Age
    tuple project $t
}

# tuple rename
displayExample {
    set t {Tuple {Name string Age int} {Name {Fred Jones} Age 27}}
    tuple rename $t Name Person Age Status
    tuple rename $t Name Person Person Name
    tuple rename $t Name Person Name Nomen
    tuple rename $t Name Age
    tuple rename $t
}

# tuple unwrap
displayExample {
    set t {
	Tuple
	{Name string Address {Tuple {Number int Street string}}}
	{Name Fred Address {Number 100 Street Elm}}
    }
    tuple unwrap $t Address
    tuple unwrap $t Name

    set u {
	Tuple {Name string Address {Tuple {Number int Name string}}}
	{Name Fred Address {Number 100 Name Elm}}
    }
    tuple unwrap $u Address
}

# tuple update
displayExample {
    set t {Tuple {Name string Age int} {Name {Fred Jones} Age 27}}
    tuple update t Name {Jane Jones}
    tuple update t Status 20
}

# tuple wrap
displayExample {
    set t {Tuple {Name string Number int Street string}\
	{Name Fred Number 100 Street Elm}}
    tuple wrap $t Address {Number Street}
}
