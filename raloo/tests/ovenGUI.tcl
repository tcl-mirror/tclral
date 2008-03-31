package require Tk

source ovenMgmt.tcl

OvenMgmt newOven 1

::raloo::arch::logLevel info

namespace import ::ral::*

set ::timerTime 00:00
set ::doorState Closed

proc ::bgerror {msg} {
    puts stderr "*** Background: $::errorInfo"
}

proc toggleDoor {} {
    if {$::doorState eq "Closed"} {
	set ::doorState Open
	OvenMgmt doorOpened 1
    } else {
	set ::doorState Closed
	OvenMgmt doorClosed 1
    }
}

proc updateTimer {op relvarName oldTuple newTuple} {
    tuple assign $newTuple
    if {$OvenId == 1} {
	set ::timerTime [format "%02d:%02d" $Minutes $Seconds]
    }
    return $newTuple
}

proc updateLight {op relvarName oldTuple newTuple} {
    tuple assign $newTuple
    if {$OvenId == 1} {
	.light configure -background\
	    [expr {$__CS__ eq "on" ? "white" : "black"}]
    }
    return $newTuple
}

proc updateTube {op relvarName oldTuple newTuple} {
    tuple assign $newTuple
    if {$OvenId == 1} {
	.tube configure -background\
	    [expr {$__CS__ eq "energized" ? "red" : "white"}]
    }
    return $newTuple
}

proc center_window {w} {
    wm withdraw $w
    update idletasks
    set x [expr {[winfo screenwidth $w] / 2 - [winfo reqwidth $w] / 2}]
    set y [expr {[winfo screenheight $w] / 2 - [winfo reqheight $w] / 2}]
    wm geom $w +$x+$y
    wm deiconify $w
}

relvar trace add variable ::OvenMgmt::Timer update updateTimer
relvar trace add variable ::OvenMgmt::Light update updateLight
relvar trace add variable ::OvenMgmt::PowerTube update updateTube

label .time -textvariable ::timerTime

button .ctrlButton -text Button -command {OvenMgmt buttonPushed 1}

button .doorButton -textvariable ::doorState -command {toggleDoor}

label .light -background black

label .tube -background white

grid .time .ctrlButton .doorButton
grid .light .tube x -sticky ew

wm protocol . WM_DELETE_WINDOW exit

center_window .
