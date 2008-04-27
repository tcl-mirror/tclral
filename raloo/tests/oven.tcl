#!/usr/bin/env wish

if {[file exists ../raloo.tcl]} {
    set auto_path [linsert $auto_path 0 ..]
}

package require raloo

source ovenMgmt.tcl
source ovenGUI.tcl
source ovenBridge.tcl

#::raloo::arch::logLevel info

set ovenId 1
proc newOven {} {
    OvenMgmt newOven $::ovenId
    incr ::ovenId
}

button .b -text "Create New Oven" -command newOven
grid .b
