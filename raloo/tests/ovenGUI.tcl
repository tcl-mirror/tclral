package require Tk 8.5
package require raloo

Domain create OvenGUI {
    SyncService newOven {id} {
	Oven createWidget $id
    }

    SyncService destroyOven {id} {
	OvenMgmt destroyOven $id
	set oven [Oven selectOne OvenId $id]
	destroy [$oven readAttr OvenWidget]
	$oven delete
    }

    SyncService updateTimerTime {id min sec} {
	set guiTimer [OvenGUI::Timer selectOne TimerId $id]
	$guiTimer updateTime $min $sec
    }

    AsyncService doorButton {id} {
	set door [Door selectOne DoorId $id]
	$door generate Pressed
    }

    SyncService turnOnLight {id} {
	set light [Light selectOne LightId $id]
	$light on
    }

    SyncService turnOffLight {id} {
	set light [Light selectOne LightId $id]
	$light off
    }

    SyncService energizeTube {id} {
	set tube [Tube selectOne TubeId $id]
	$tube energize
    }

    SyncService deenergizeTube {id} {
	set tube [Tube selectOne TubeId $id]
	$tube deenergize
    }

    Class Oven {
	package require img::png
	image create photo ovenOff -file mw-off.png
	image create photo ovenCook -file mw-cook.png
	image create photo ovenOpen -file mw-open.png

	Attribute {
	    *OvenId string
	    *2OvenWidget string
	}

	ClassOp createWidget {id} {
	    toplevel .$id
	    wm title .$id "One Button Microwave $id"
	    my insert OvenId $id OvenWidget .$id
	    wm protocol .$id WM_DELETE_WINDOW [list OvenGUI destroyOven $id]
	    button .$id.ctrlButton -text Start/Add\
		-command [list OvenMgmt buttonPushed $id]
	    label .$id.ovenImage -image ovenOff

	    grid .$id.ovenImage [Timer createWidget $id .$id] -padx 3 -pady 3
	    grid ^              .$id.ctrlButton -padx 3 -pady 3
	    grid ^              [Door createWidget $id .$id] -padx 3 -pady 3
	    grid ^              [Light createWidget $id .$id]\
				-sticky ew -padx 3 -pady 3
	    grid ^              [Tube createWidget $id .$id]\
				-sticky ew -padx 3 -pady 3
	}

	InstOp showImage {which} {
	    set ow [my readAttr OvenWidget]
	    $ow.ovenImage configure -image $which
	}
    }

    Class Door {
	Attribute {
	    *DoorId string
	    *2DoorWidget string
	}

	ClassOp createWidget {id parent} {
	    button $parent.door\
		-text Closed\
		-command [list OvenGUI doorButton $id]
	    my insert DoorId $id DoorWidget $parent.door
	    return $parent.door
	}

	Lifecycle {
	    State doorClosed {} {
		my with {DoorId DoorWidget} {
		    $DoorWidget configure -text Closed
		    OvenMgmt doorClosed $DoorId
		    set oven [Oven selectOne OvenId $DoorId]
		    $oven showImage ovenOff
		}
	    }

	    State doorOpen {} {
		my with {DoorId DoorWidget} {
		    $DoorWidget configure -text Open
		    OvenMgmt doorOpened $DoorId
		    set oven [Oven selectOne OvenId $DoorId]
		    $oven showImage ovenOpen
		}
	    }

	    Transition doorClosed - Pressed -> doorOpen
	    Transition doorOpen - Pressed -> doorClosed

	    DefaultInitialState doorClosed
	}
    }

    Class Light {
	Attribute {
	    *LightId string
	    *2LightWidget string
	}

	ClassOp createWidget {id parent} {
	    label $parent.light -text LIGHT -fg white -bg black
	    my insert LightId $id LightWidget $parent.light
	    return $parent.light
	}
	InstOp on {} {
	    [my readAttr LightWidget] configure -fg black -bg white
	}
	InstOp off {} {
	    [my readAttr LightWidget] configure -fg white -bg black
	}
    }

    Class Tube {
	Attribute {
	    *TubeId string
	    *2TubeWidget string
	}

	ClassOp createWidget {id parent} {
	    label $parent.tube -text TUBE -fg red -bg black
	    my insert TubeId $id TubeWidget $parent.tube
	    return $parent.tube
	}

	InstOp energize {} {
	    my with {TubeId TubeWidget} {
		$TubeWidget configure -fg black -bg red
		set oven [Oven selectOne OvenId $TubeId]
		$oven showImage ovenCook
	    }
	}
	InstOp deenergize {} {
	    my with {TubeId TubeWidget} {
		$TubeWidget configure -fg red -bg black
		set oven [Oven selectOne OvenId $TubeId]
		# This is a hack!
		if {[[$oven readAttr OvenWidget].ovenImage cget -image] eq\
			"ovenCook"} {
		    $oven showImage ovenOff
		}
	    }
	}
    }

    Class Timer {
	Attribute {
	    *TimerId string
	    *2TimerWidget string
	}

	ClassOp createWidget {id parent} {
	    label $parent.timer -text 00:00
	    my insert TimerId $id TimerWidget $parent.timer
	    return $parent.timer
	}

	InstOp updateTime {min sec} {
	    set w [my readAttr TimerWidget]
	    $w configure -text [format %02d:%02d $min $sec]
	}
    }
}
