package require ral 0.6
::ral::relvar create P {P# string PNAME string COLOR string WEIGHT double CITY string} {P#}
::ral::relvar create SP {S# string P# string QTY int} {{S# P#}}
::ral::relvar create S {S# string SNAME string STATUS int CITY string} {S#}
::ral::relvar insert P {P# P1 PNAME Nut COLOR Red WEIGHT 12.0 CITY London}
::ral::relvar insert P {P# P2 PNAME Bolt COLOR Green WEIGHT 17.0 CITY Paris}
::ral::relvar insert P {P# P3 PNAME Screw COLOR Blue WEIGHT 17.0 CITY Oslo}
::ral::relvar insert P {P# P4 PNAME Screw COLOR Red WEIGHT 14.0 CITY London}
::ral::relvar insert P {P# P5 PNAME Cam COLOR Blue WEIGHT 12.0 CITY Paris}
::ral::relvar insert P {P# P6 PNAME Cog COLOR Red WEIGHT 19.0 CITY London}
::ral::relvar insert SP {S# S1 P# P1 QTY 300}
::ral::relvar insert SP {S# S1 P# P2 QTY 200}
::ral::relvar insert SP {S# S1 P# P3 QTY 400}
::ral::relvar insert SP {S# S1 P# P4 QTY 200}
::ral::relvar insert SP {S# S1 P# P5 QTY 100}
::ral::relvar insert SP {S# S1 P# P6 QTY 100}
::ral::relvar insert SP {S# S2 P# P1 QTY 300}
::ral::relvar insert SP {S# S2 P# P2 QTY 400}
::ral::relvar insert SP {S# S3 P# P2 QTY 200}
::ral::relvar insert SP {S# S4 P# P2 QTY 200}
::ral::relvar insert SP {S# S4 P# P4 QTY 300}
::ral::relvar insert SP {S# S4 P# P5 QTY 400}
::ral::relvar insert S {S# S1 SNAME Smith STATUS 20 CITY London}
::ral::relvar insert S {S# S2 SNAME Jones STATUS 10 CITY Paris}
::ral::relvar insert S {S# S3 SNAME Blake STATUS 30 CITY Paris}
::ral::relvar insert S {S# S4 SNAME Clark STATUS 20 CITY London}
::ral::relvar insert S {S# S5 SNAME Adams STATUS 30 CITY Athens}

