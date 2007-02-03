package require ral
namespace import ::ral::*

# relvar with relation and tuple valued attributes
relvar create People {
    Relation
    {
	Name	{Tuple {First string Middle string Last string}}
	Address {Tuple {Number string Street string}}
	Location {Tuple {City string State string Zip string}}
	Phones	{Relation {Type string Area string Number string}
	    {Type {Area Number}}}
	Emails	{Relation {Type string Address string} {Type Address}}
    }
    Name
}

relvar insert People {
    Name {First Fred Middle Allen Last Jones}
    Address {Number 200 Street {Sweeping Winds Ter}}
    Location {City Sunnyvale State CA Zip 77700}
    Phones {
	{Type Home Area 408 Number 555-1212}
	{Type Work Area 408 Number 555-2121}
    }
    Emails {
	{Type Home Address fred@biteme.com}
	{Type Work Address fred@slavedrivers.com}
    }
} {
    Name {First Alice Middle Ann Last Jones}
    Address {Number 300 Street Elm}
    Location {City Sunnyvale State CA Zip 77701}
    Phones {
	{Type Home Area 408 Number 555-1122}
    }
    Emails {
	{Type Home Address alice@bitme.com}
	{Type Work Address alice@waytowork.com}
    }
}

# The famous "Date" example database.
relvar create S {
    Relation
    {S# string SNAME string STATUS int CITY string}
    S#
}
relvar create P {
    Relation
    {P# string PNAME string COLOR string WEIGHT double CITY string}
    P#
}
relvar create SP {
    Relation
    {S# string P# string QTY int}
    {{S# P#}}
}

relvar correlation R1 SP S# * S S# P# + P P#

relvar eval {
    relvar insert S\
	{S# S1 SNAME Smith STATUS 20 CITY London}\
	{S# S2 SNAME Jones STATUS 10 CITY Paris}\
	{S# S3 SNAME Blake STATUS 30 CITY Paris}\
	{S# S4 SNAME Clark STATUS 20 CITY London}\
	{S# S5 SNAME Adams STATUS 30 CITY Athens}

    relvar insert P\
	{P# P1 PNAME Nut COLOR Red WEIGHT 12.0 CITY London}\
	{P# P2 PNAME Bolt COLOR Green WEIGHT 17.0 CITY Paris}\
	{P# P3 PNAME Screw COLOR Blue WEIGHT 17.0 CITY Oslo}\
	{P# P4 PNAME Screw COLOR Red WEIGHT 14.0 CITY London}\
	{P# P5 PNAME Cam COLOR Blue WEIGHT 12.0 CITY Paris}\
	{P# P6 PNAME Cog COLOR Red WEIGHT 19.0 CITY London}

    relvar insert SP\
	{S# S1 P# P1 QTY 300}\
	{S# S1 P# P2 QTY 200}\
	{S# S1 P# P3 QTY 400}\
	{S# S1 P# P4 QTY 200}\
	{S# S1 P# P5 QTY 100}\
	{S# S1 P# P6 QTY 100}\
	{S# S2 P# P1 QTY 300}\
	{S# S2 P# P2 QTY 400}\
	{S# S3 P# P2 QTY 200}\
	{S# S4 P# P2 QTY 200}\
	{S# S4 P# P4 QTY 300}\
	{S# S4 P# P5 QTY 400}
}

# Something for a partition constraint.
relvar create Lamp {
    Relation
    {ModelNum int Color string}
    ModelNum
}

relvar create FloorLamp {
    Relation
    {ModelNum int Height int}
    ModelNum
}

relvar create TableLamp {
    Relation
    {ModelNum int Type string}
    ModelNum
}

relvar partition R3 Lamp ModelNum FloorLamp ModelNum TableLamp ModelNum

relvar eval {
    relvar insert Lamp\
	{ModelNum 1 Color Red}\
	{ModelNum 2 Color Blue}
    relvar insert FloorLamp\
	{ModelNum 1 Height 20}
    relvar insert TableLamp\
	{ModelNum 2 Type Banker}
}