#!/usr/bin/env tclsh
# This software is copyrighted 2008 by G. Andrew Mangogna.
# The following terms apply to all files associated with the software unless
# explicitly disclaimed in individual files.
# 
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors and
# need not follow the licensing terms described here, provided that the
# new terms are clearly indicated on the first page of each file where
# they apply.
# 
# IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
# DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING
# OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES
# THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
# 
# THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE
# IS PROVIDED ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE
# NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS,
# OR MODIFICATIONS.
# 
# GOVERNMENT USE: If you are acquiring this software on behalf of the
# U.S. government, the Government shall have only "Restricted Rights"
# in the software and related documentation as defined in the Federal
# Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
# are acquiring the software on behalf of the Department of Defense,
# the software shall be classified as "Commercial Computer Software"
# and the Government shall have only "Restricted Rights" as defined in
# Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the foregoing,
# the authors grant the U.S. Government and others acting in its behalf
# permission to use and distribute the software in accordance with the
# terms specified in this license.
# 
#  *++
# MODULE:
#   ovenMgmt.tcl -- the one button microwave example
# 
# ABSTRACT:
# 
# $RCSfile: ovenMgmt.tcl,v $
# $Revision: 1.4 $
# $Date: 2008/04/20 19:20:17 $
#  *--

package require raloo
namespace import ::raloo::*

Domain create OvenMgmt {
    # "newOven" creates a new one button microwave oven along with all
    # of its component parts. This service provides an easy method of
    # populating the domain.
    SyncService newOven {ovenId} {
	Oven insert OvenId $ovenId
	PowerTube insert TubeId $ovenId
	Light insert LightId $ovenId
	Timer insert TimerId $ovenId Minutes 0 Seconds 0
    }

    SyncService destroyOven {ovenId} {
	Oven delete [list [list OvenId $ovenId]]
	PowerTube delete [list [list TubeId $ovenId]]
	Light delete [list [list LightId $ovenId]]
	Timer delete [list [list TimerId $ovenId]]
    }
    # Signal that the control button has been pressed.
    DomainOp buttonPushed {ovenId} {
	set oven [Oven selectOne OvenId $ovenId]
	$oven generate ButtonPushed
    }
    # Signal that the oven door has been opened.
    DomainOp doorOpened {ovenId} {
	set oven [Oven selectOne OvenId $ovenId]
	$oven generate DoorOpened
    }
    # Signal that the oven door has been closed.
    DomainOp doorClosed {ovenId} {
	set oven [Oven selectOne OvenId $ovenId]
	$oven generate DoorClosed
    }

    # The Oven class models the entire microwave oven.
    Class Oven {
	Attribute {
	    *OvenId int
	}
	Lifecycle {
	    State idleWithDoorClosed {} {
		# 1. Generate L2: Turn off light(light ID)
		set light [my selectRelated ~R2]
		$light generate TurnOff
	    }
	    State initialCookingPeriod {} {
		# 1. Set timer for 1 minute
		set timer [my selectRelated ~R3]
		$timer setOneMin
		# 2. Generate L1: Turn on light(light ID)
		set light [my selectRelated ~R2]
		$light generate TurnOn
		# 3. Generate P1: Energize power tube(tube ID)
		set tube [my selectRelated ~R1]
		$tube generate Energize
	    }
	    State cookingPeriodExtended {} {
		# 1. Add one minute to timer.
		set timer [my selectRelated ~R3]
		$timer addOneMin
	    }
	    State cookingComplete {} {
		# 1. Generate P2: De-energize power tube(tube ID)
		set tube [my selectRelated ~R1]
		$tube generate De-energize
		# 2. Generate L2: Turn off light(light ID)
		set light [my selectRelated ~R2]
		$light generate TurnOff
		# 3. Sound warning beep
		puts "Buzz!!"
	    }
	    State idleWithDoorOpen {} {
		# 1. Generate L1: Turn on light(light ID)
		set light [my selectRelated ~R2]
		$light generate TurnOn
	    }
	    State cookingInterrupted {} {
		# 1. Generate P2: De-energize power tube(tube ID)
		set tube [my selectRelated ~R1]
		$tube generate De-energize
		# 2. Clear the timer
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

    # The PowerTube class has a simple two state machine indicating that
    # it can be, easily enough, on and off.
    Class PowerTube {
	Attribute {
	    *TubeId int
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

    # The Light class has similar behavior as the power tube.
    Class Light {
	Attribute {
	    *LightId int
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

    # The original example does not include a timer class. We use one here
    # to make it easier to track the time in a user interface. Also the
    # Timer class encapsulates the use of delayed events as a means of doing
    # regular periodic behavior.
    Class Timer {
	Attribute {
	    *TimerId int
	    Minutes int
	    Seconds int
	}
	InstOp setOneMin {} {
	    my writeAttr Minutes 1 Seconds 0
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
	# This state machine is driven by a period 1 second delayed event
	# in order to keep a current tally of minutes and seconds.
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

    # The relationships emphasize the 1 to 1 nature of the components
    # of the oven. Consequently, we use the same identification scheme
    # for tube, lights, etc. as for the oven itself.
    Relationship R1 PowerTube 1-->1 Oven {
	RefMap {TubeId -> OvenId}
    }
    Relationship R2 Light 1-->1 Oven {
	RefMap {LightId -> OvenId}
    }
    Relationship R3 Timer 1-->1 Oven {
	RefMap {TimerId -> OvenId}
    }
}
