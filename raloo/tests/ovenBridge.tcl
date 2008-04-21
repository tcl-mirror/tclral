
# Bridging

oo::define OvenMgmt {
    filter ovenmgmt

    method ovenmgmt {args} {
	#puts "ovenmgmt: [self target] $args"
	next {*}$args

	if {[lindex [self target] 1] eq "newOven"} {
	    OvenGUI newOven {*}$args
	}
    }
}

# Wire the GUI light to the oven light
oo::define OvenMgmt::Light {
    filter ovengui_light

    method ovengui_light {args} {
	set result [next {*}$args]

	switch -exact -- [lindex [self target] 1] {
	    __on {
		OvenGUI turnOnLight [my readAttr LightId]
	    }
	    __off {
		OvenGUI turnOffLight [my readAttr LightId]
	    }
	}

	return $result
    }
}

# Wire the GUI tube to the oven tube
oo::define OvenMgmt::PowerTube {
    filter ovengui_tube

    method ovengui_tube {args} {
	set result [next {*}$args]

	switch -exact -- [lindex [self target] 1] {
	    __energized {
		OvenGUI energizeTube [my readAttr TubeId]
	    }
	    __de-energized {
		OvenGUI deenergizeTube [my readAttr TubeId]
	    }
	}

	return $result
    }
}

oo::define OvenMgmt::Timer {
    filter ovengui_timer

    method ovengui_timer {args} {
	set result [next {*}$args]

	switch -exact -- [lindex [self target] 1] {
	    writeAttr {
		OvenGUI updateTimerTime {*}[my readAttr TimerId Minutes Seconds]
	    }
	}

	return $result
    }
}
