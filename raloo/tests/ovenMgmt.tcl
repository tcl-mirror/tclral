#!/usr/bin/env tclsh

source ../raloo.tcl

namespace import ::raloo::*

Domain create OvenMgmt {
    SyncService newOven {ovenId} {
	Oven insert OvenId $ovenId
	PowerTube insert OvenId $ovenId
	Light insert OvenId $ovenId
	Timer insert OvenId $ovenId Minutes 0 Seconds 0
    }
    DomainOp buttonPushed {ovenId} {
	set oven [Oven new OvenId $ovenId]
	$oven generate ButtonPushed
    }
    DomainOp doorOpened {ovenId} {
	set oven [Oven new OvenId $ovenId]
	$oven generate DoorOpened
    }
    DomainOp doorClosed {ovenId} {
	set oven [Oven new OvenId $ovenId]
	$oven generate DoorClosed
    }

    Class Oven {
	Attribute {
	    *OvenId int
	}
	Lifecycle {
	    State idleWithDoorClosed {} {
		set light [my selectRelated ~R2]
		$light generate TurnOff
	    }
	    State initialCookingPeriod {} {
		set timer [my selectRelated ~R3]
		$timer setOneMin
		set light [my selectRelated ~R2]
		$light generate TurnOn
		set tube [my selectRelated ~R1]
		$tube generate Energize
	    }
	    State cookingPeriodExtended {} {
		set timer [my selectRelated ~R3]
		$timer addOneMin
	    }
	    State cookingComplete {} {
		set light [my selectRelated ~R2]
		$light generate TurnOff
		set tube [my selectRelated ~R1]
		$tube generate De-energize
		puts "Buzz!!"
	    }
	    State idleWithDoorOpen {} {
		set light [my selectRelated ~R2]
		$light generate TurnOn
	    }
	    State cookingInterrupted {} {
		set tube [my selectRelated ~R1]
		$tube generate De-energize
		set timer [my selectRelated ~R3]
		$timer clear
	    }
	    DefaultInitialState idleWithDoorClosed

	    Transition idleWithDoorClosed - ButtonPushed -> initialCookingPeriod
	    Transition idleWithDoorClosed - DoorOpened -> idleWithDoorOpen

	    Transition initialCookingPeriod - TimerExpired -> cookingComplete
	    Transition initialCookingPeriod - ButtonPushed ->\
		    cookingPeriodExtended
	    Transition initialCookingPeriod - DoorOpened -> cookingInterrupted

	    Transition cookingPeriodExtended - ButtonPushed ->\
		    cookingPeriodExtended
	    Transition cookingPeriodExtended - DoorOpened -> cookingInterrupted
	    Transition cookingPeriodExtended - TimerExpired -> cookingComplete

	    Transition cookingComplete - DoorOpened -> idleWithDoorOpen
	    Transition cookingComplete - ButtonPushed -> IG

	    Transition idleWithDoorOpen - DoorClosed -> idleWithDoorClosed
	    Transition idleWithDoorOpen - ButtonPushed -> IG

	    Transition cookingInterrupted - DoorClosed -> idleWithDoorClosed
	    Transition cookingInterrupted - ButtonPushed -> IG
	}
    }

    Class PowerTube {
	Attribute {
	    *OvenId int
	}
	Lifecycle {
	    State energized {} {
		puts "Power to microwave tube is on!"
	    }
	    State de-energized {} {
		puts "Power to microwave tube is off!"
	    }
	    Transition energized - De-energize -> de-energized
	    Transition de-energized - Energize -> energized

	    DefaultInitialState de-energized
	}
    }

    Class Light {
	Attribute {
	    *OvenId int
	}
	Lifecycle {
	    State on {} {
		puts "Light is on!"
	    }
	    State off {} {
		puts "Light is off!"
	    }
	    Transition on - TurnOff -> off
	    Transition off - TurnOn -> on

	    DefaultInitialState off
	}
    }

    Class Timer {
	Attribute {
	    *OvenId int
	    Minutes int
	    Seconds int
	}
	InstOp setOneMin {} {
	    my writeAttr Minutes 1
	    my writeAttr Seconds 0
	    my generateDelayed 1000 Tick
	}
	InstOp addOneMin {} {
	    my with Minutes {
		incr Minutes
		my writeAttr Minutes $Minutes
	    }
	}
	InstOp clear {} {
	    my cancelDelayed Tick
	    my writeAttr Minutes 0 Seconds 0
	}
	Lifecycle {
	    State counting {} {
		lassign [my readAttr Minutes Seconds] min sec
		if {$sec == 0} {
		    set sec 59
		    incr min -1
		} else {
		    incr sec -1
		}
		my writeAttr Minutes $min Seconds $sec
		if {$min == 0 && $sec == 0} {
		    set oven [my selectRelated R3]
		    $oven generate TimerExpired
		} else {
		    my generateDelayed 1000 Tick
		}
	    }
	    Transition counting - Tick -> counting
	}
    }

    Relationship R1 PowerTube 1-->1 Oven
    Relationship R2 Light 1-->1 Oven
    Relationship R3 Timer 1-->1 Oven
}
