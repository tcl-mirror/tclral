package require Tk
package require raloo

Domain create OvenGUI {
    SyncService newOven {id} {
	OvenMgmt newOven $id
	Oven createWidget $id
    }

    SyncService destroyOven {id} {
	OvenMgmt destroyOven $id
	set oven [Oven new OvenId $id]
	destroy [$oven readAttr OvenWidget]
	$oven delete
    }

    SyncService updateTimerTime {id min sec} {
	set guiTimer [OvenGUI::Timer new TimerId $id]
	$guiTimer updateTime $min $sec
    }

    DomainOp doorButton {id} {
	set door [Door new DoorId $id]
	$door generate Pressed
    }

    DomainOp turnOnLight {id} {
	set light [Light new LightId $id]
	$light generate TurnOn
    }

    DomainOp turnOffLight {id} {
	set light [Light new LightId $id]
	$light generate TurnOff
    }

    DomainOp energizeTube {id} {
	set tube [Tube new TubeId $id]
	$tube generate Energize
    }

    DomainOp deenergizeTube {id} {
	set tube [Tube new TubeId $id]
	$tube generate De-energize
    }

    Class Oven {
	package require img::png
	image create photo ovenOff -file mw-off.png
	image create photo ovenCook -file mw-cook.png

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

	    grid\
		[Timer createWidget $id .$id]\
		.$id.ctrlButton\
		[Door createWidget $id .$id]\
		-padx 3 -pady 3
	    grid\
		[Light createWidget $id .$id]\
		[Tube createWidget $id .$id]\
		-sticky ew -padx 3 -pady 3
	    grid\
		.$id.ovenImage
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
		}
	    }

	    State doorOpen {} {
		my with {DoorId DoorWidget} {
		    $DoorWidget configure -text Open
		    OvenMgmt doorOpened $DoorId
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
	    label $parent.light -bg black
	    my insert LightId $id LightWidget $parent.light
	    return $parent.light
	}
	Lifecycle {
	    State on {} {
		my with LightWidget {
		    $LightWidget configure -bg white
		}
	    }
	    State off {} {
		my with LightWidget {
		    $LightWidget configure -bg black
		}
	    }
	    Transition on - TurnOff -> off
	    Transition off - TurnOn -> on

	    DefaultInitialState off
	}
    }

    Class Tube {
	Attribute {
	    *TubeId string
	    *2TubeWidget string
	}

	ClassOp createWidget {id parent} {
	    label $parent.tube -bg black
	    my insert TubeId $id TubeWidget $parent.tube
	    return $parent.tube
	}

	Lifecycle {
	    State energized {} {
		my with TubeWidget {
		    $TubeWidget configure -bg red
		}
		.[my readAttr TubeId].ovenImage configure -image ovenCook
	    }
	    State de-energized {} {
		my with TubeWidget {
		    $TubeWidget configure -bg black
		}
		.[my readAttr TubeId].ovenImage configure -image ovenOff
	    }
	    Transition energized - De-energize -> de-energized
	    Transition de-energized - Energize -> energized

	    DefaultInitialState de-energized
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
