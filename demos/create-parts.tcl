#!/bin/bash
# \
exec tclsh "$0" "$@"

lappend auto_path [file join .. src]

package require ral
namespace import ::ral::*

relvar create S\
    {S# string SNAME string STATUS int CITY string}\
    S#\
    {S# S1 SNAME Smith STATUS 20 CITY London}\
    {S# S2 SNAME Jones STATUS 10 CITY Paris}\
    {S# S3 SNAME Blake STATUS 30 CITY Paris}\
    {S# S4 SNAME Clark STATUS 20 CITY London}\
    {S# S5 SNAME Adams STATUS 30 CITY Athens}

relvar create P\
    {P# string PNAME string COLOR string WEIGHT double CITY string}\
    P#\
    {P# P1 PNAME Nut COLOR Red WEIGHT 12.0 CITY London}\
    {P# P2 PNAME Bolt COLOR Green WEIGHT 17.0 CITY Paris}\
    {P# P3 PNAME Screw COLOR Blue WEIGHT 17.0 CITY Oslo}\
    {P# P4 PNAME Screw COLOR Red WEIGHT 14.0 CITY London}\
    {P# P5 PNAME Cam COLOR Blue WEIGHT 12.0 CITY Paris}\
    {P# P6 PNAME Cog COLOR Red WEIGHT 19.0 CITY London}

relvar create SP\
    {S# string P# string QTY int}\
    {{S# P#}}\
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

puts -nonewline [relvar dump]