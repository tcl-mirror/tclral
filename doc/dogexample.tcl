lappend auto_path [file join .. src]
package require ral
namespace import ::ral::*

relvar create DOG\
    {DogName string Breed string} DogName
relvar create OWNER\
    {OwnerName string Age int City string} OwnerName
relvar create OWNERSHIP\
    {OwnerName string DogName string Acquired string}\
    {{OwnerName DogName}}


relvar insert DOG\
    {DogName Fido Breed Poodle}\
    {DogName Sam Breed Collie}\
    {DogName Spot Breed Terrier}\
    {DogName Rover Breed Retriever}\
    {DogName Fred Breed Spaniel}

relvar insert OWNER\
    {OwnerName Sue Age 24 City Cupertino}\
    {OwnerName George Age 35 City Sunnyvale}\
    {OwnerName Alice Age 30 City {San Jose}}\
    {OwnerName Mike Age 50 City {San Jose}}\
    {OwnerName Jim Age 42 City {San Francisco}}

relvar insert OWNERSHIP\
    {OwnerName Sue DogName Fido Acquired 2001}\
    {OwnerName Sue DogName Sam Acquired 2000}\
    {OwnerName George DogName Fido Acquired 2001}\
    {OwnerName George DogName Sam Acquired 2000}\
    {OwnerName Alice DogName Spot Acquired 2001}\
    {OwnerName Mike DogName Rover Acquired 2002}\
    {OwnerName Jim DogName Fred Acquired 2003}

set grp [relation group [relvar set OWNERSHIP] DogAcquisition DogName Acquired]
puts $grp
#puts [relformat $grp Grouped]
relation foreach t $grp {
    set r [tuple extract $t DogAcquisition]
    puts $r
}
puts $grp

set ugrp [relation ungroup $grp DogAcquisition]
puts $ugrp
puts [relformat $ugrp Ungrouped]
