# This software is copyrighted 2006 by G. Andrew Mangogna.
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
# ralutil.tcl -- Convenience scripts for use with TclRAL
# 
# ABSTRACT:
# This file contains a set of utility procs that are useful in
# conjunction with TclRAL. N.B. that many of these are experimental
# in nature and interfaces are subject to much change. Part of the
# purpose for this package is to determine what works well in practice
# without cluttering the TclRAL package proper.
# 
# $RCSfile: ralutil.tcl,v $
# $Revision: 1.1 $
# $Date: 2007/02/03 20:07:35 $
#  *--

package provide ralutil 0.8.1

namespace eval ::ralutil {
    namespace export pipe
    namespace export navigate
    namespace export sysIdsInit
    namespace export sysIdsGenSystemId

    namespace import ::ral::*

    variable DEE {Relation {} {{}} {{}}}
    variable DUM {Relation {} {{}} {}}
}

# Relational expressions are frequently nested to achieve some desired
# result. This nesting in normal Tcl syntax quickly gets difficult to read.
# Often the expression is naturally a pipe of the output of one command
# to the input of another. This idea came from the "Commands pipe" page
# on the wiki: http://wiki.tcl.tk/17419. The code here is different,
# but the idea is the same.
proc ::ralutil::pipe {fns {var {}} {sep |~}} {
    # Split out the separator characters
    lassign [split $sep ""] s p
    # create a valid tcl command
    set cmd [nestcmds [string trim [split $fns $s]] $p]
    # perform command
    if {$var eq ""} {
	return [uplevel $cmd]
    } else {
	upvar $var v
	set v $cmd
    }
}

# nest one command inside another in reverse list order.
# "p" is a place holder character denoting where the previous result
# is to be placed in the command. If "p" is absent in the command,
# then the previous result is simply appended to the command.
proc ::ralutil::nestcmds {cmdList p} {
    set cmd [string trim [lindex $cmdList end]]
    if {[llength $cmdList] > 1} {
	set innercmd "\[[nestcmds [lrange $cmdList 0 end-1] $p]\]"
	set cmd [expr {[string first $p $cmd] < 0 ?\
	    "$cmd $innercmd" : [string map [list $p $innercmd] $cmd]}]
    }
    return $cmd
}

# traverse via semijoins across constraints.
# navigate relValue constraint relvarName ?-ref | -refto | <attr list>?
# where "relValue" is a value of the same type as the value contained
# in "srcRelvar" and presumably contains a subset of the tuples of "srcRelvar"
#
# This is very experimental and rather complicated for what it accomplishes.
proc ::ral::navigate {relValue constraintName relvarName {dir {}}} {
    set relvarName [uplevel ::ral::relvar path $relvarName]
    set constraintName [uplevel ::ral::relvar constraint path $constraintName]
    set cmd [list ::ral::relation semijoin $relValue [relvar set $relvarName]\
	-using]
    set cInfo [relvar constraint info $constraintName]
    switch -- [lindex $cInfo 0] {
	association {
	    lassign $cInfo type name rngRelvar rngAttrs rngSpec\
		rtoRelvar rtoAttrs rtoSpec
	    if {$rngRelvar eq $rtoRelvar} {
		if {$dir eq "-ref"} {
		    set srcAttrs $rngAttrs
		    set dstAttrs $rtoAttrs
		} elseif {$dir eq "-refto"} {
		    set srcAttrs $rtoAttrs
		    set dstAttrs $rngAttrs
		} else {
		    error "bad navigation direction, \"$dir\""
		}
	    } else {
		# If the constraint is not reflexive, then we
		# determine the direction from relvar name
		if {$rngRelvar eq $relvarName} {
		    set srcAttrs $rngAttrs
		    set dstAttrs $rtoAttrs
		} elseif {$rtoRelvar eq $relvarName} {
		    set srcAttrs $rngAttrs
		    set dstAttrs $rtoAttrs
		} else {
		    error "relvar, \"$relvarName\", does not participate in\
			constraint, \"$constraintName\""
		}
	    }
	    lappend cmd [mergeJoinAttrs $srcAttrs $dstAttrs]
	}
	partition {
	    set subSets [lassign $cInfo type name superRelvar superAttrs]
	    if {$superRelvar eq $relvarName} {
		# sub set to super set reference
		# but we don't know which sub set it is -- so we compare
		# the headings of the subsets and find the first one
		# that matching the heading of the relation value.
		set srcAttrs [findSubSetType $relValue $subSets]
		set dstAttrs $superAttrs
	    } else {
		# super set to sub set navigation
		# determine which subset we are navigating to
		set srcAttrs $superAttrs
		set dstAttrs [findSubSetRef $relvarName $subSets]
	    }
	    lappend cmd [mergeJoinAttrs $srcAttrs $dstAttrs]
	}
	correlation {
	    # Navigation across a correlation constraint may involve
	    # two semijoins.
	    lassign $cInfo type name rngRelvar rngAttrsA rngSpecA\
		rtoRelvarA rtoAttrsA rngAttrsB rngSpecB rtoRelvarB rtoAttrsB
	    # If the navigation is to the correlation relvar, then a
	    # single semijoin will do the job.
	    if {$rngRelvar eq $relvarName} {
		# This is very much like the "association" case
		# First, we must deal with the reflexive case.
		# If the correlation is reflexive, then the ambiguity
		# has to be broken by supplying the referred to attributes
		if {$rtoRelvarA eq $rtoRelvarB} {
		    if {$dir eq $rtoAttrsA} {
			set srcAttrs $rtoAttrsA
			set dstAttrs $rngAttrsA
		    } elseif {$dir eq $rtoAttrsB} {
			set srcAttrs $rtoAttrsB
			set dstAttrs $rngAttrsB
		    } else {
			error "bad navigation attributes, \"$dir\""
		    }
		} else {
		    # Otherwise, we must find out which side of the
		    # correlation we are starting from.
		    if {[matchValueType $relValue $rtoRelvarA]} {
			set srcAttrs $rtoAttrsA
			set dstAttrs $rngAttrsA
		    } elseif {[matchValueType $relValue $rtoRelvarB]} {
			set srcAttrs $rtoAttrsB
			set dstAttrs $rngAttrsB
		    } else {
			error "the type of \"$relValue\" does not match the\
			    types of either \"$rtoRelvarA\" or \"$rtoRelvarB\""
		    }
		}
		lappend cmd [mergeJoinAttrs $srcAttrs $dstAttrs]
	    } elseif {[matchValueType $relValue $rngRelvar]} {
		# navigation is from the correlation relvar to one of
		# the ends of the correlation
		# Again reflexivity complicates things.
		if {$rtoRelvarA eq $rtoRelvarB} {
		    if {$dir eq $rngAttrsA} {
			set srcAttrs $rngAttrsA
			set dstAttrs $rtoAttrsA
		    } elseif {$dir eq $rngAttrsB} {
			set srcAttrs $rngAttrsB
			set dstAttrs $rtoAttrsB
		    } else {
			error "bad navigation attributes, \"$dir\""
		    }
		} elseif {$rtoRelvarA eq $relvarName} {
		    set srcAttrs $rngAttrsA
		    set dstAttrs $rtoAttrsA
		} elseif {$rtoRelvarB eq $relvarName} {
		    set srcAttrs $rngAttrsB
		    set dstAttrs $rtoAttrsB
		} else {
		    error "relvar, \"$relvarName\", does not participate in\
			constraint, \"$constraintName\""
		}
		lappend cmd [mergeJoinAttrs $srcAttrs $dstAttrs]
	    } elseif {$rtoRelvarA eq $relvarName ||\
		      $rtoRelvarB eq $relvarName} {
		# The destination is one of the two referred to relvars.
		# If the source is one of the referred to relvars then
		# two semijoins are necessary to complete the traversal.
		# If the source is the correlation relvar, then only
		# a single semijoin is required.
		# Reflexivity requires more information to break the ambiguity.
		if {$rtoRelvarA eq $rtoRelvarB} {
		    if {$dir eq $rngAttrsA} {
			set srcAttrs $rtoAttrsA
			set correlSrc $rngAttrsA
			set correlDst $rngAttrsB
			set dstAttrs $rtoAttrsB
			set dst $rtoRelvarB
		    } elseif {$dir eq $rngAttrsB} {
			set srcAttrs $rtoAttrsB
			set correlSrc $rngAttrsB
			set correlDst $rngAttrsA
			set dstAttrs $rtoAttrsA
			set dst $rtoRelvarA
		    } else {
			error "bad navigation attributes, \"$dir\""
		    }
		} else {
		    if {[matchValueType $relValue $rtoRelvarA]} {
			set srcAttrs $rtoAttrsA
			set correlSrc $rngAttrsA
			set correlDst $rngAttrsB
			set dstAttrs $rtoAttrsB
			set dst $rtoRelvarB
		    } elseif {[matchValueType $relValue $rtoRelvarB]} {
			set srcAttrs $rtoAttrsB
			set correlSrc $rngAttrsB
			set correlDst $rngAttrsA
			set dstAttrs $rtoAttrsA
			set dst $rtoRelvarA
		    } else {
			error "type of \"$relValue\" does matches neither that\
			    of \"$rtoRelvarA\" nor \"$rtoRelvarB\""
		    }
		}
		# So the semijoins will traverse:
		# "src" -> correlation -> "dst"
		set correlRel [::ral::relation semijoin $relValue\
		    [::ral::relvar set $rngRelvar]\
		    -using [mergeJoinAttrs $srcAttrs $correlSrc]]
		set cmd [list\
		    ::ral::relation semijoin $correlRel\
		    [::ral::relvar set $dst]\
		    -using [mergeJoinAttrs $correlDst $dstAttrs]\
		]
	    } else {
		error "\"$relvarName\" does not participate in correlation,\
		    \"$constraintName\""
	    }
	}
    }
    return [eval $cmd]
}

# ::ral::alter <relvarName> <varName> <script>
# Alter a relvar's structure.
# "relvarName" is the name of the relvar.
# "varName" is the name of a Tcl variable that is assigned
#   the value in "relvarName".
# "script" is an expression that is evaluated in the context of the caller.
# The value found in "varName" after "script" is evaluated is taken
# to be the new value to store in "relvarName".
# The heading of "relvarName" is modified to reflect the new structure.
# This is all accomplished by deleting and re-creating "relvarName" taking
# care to remove and replace the constraints.
#
# N.B. very experimental and mildly dangerous as it must delete the
# relvar and recreate it along with the constraints.
#
proc ::ral::alter {relvarName varName script} {
    # Get the resolved name
    set relvarName [relvar names $relvarName]
    # Store all the constraint info to restore later. In order to delete
    # a relvar, all the constraints must be deleted.
    set cnstrCmds [list]
    foreach constraint [relvar constraint member $relvarName] {
	lappend cnstrCmds [relvar constraint info $constraint]
	relvar constraint delete $constraint
    }
    upvar $varName var
    set prevValue [set var [relvar set $relvarName]]
    set prevHeading [relation heading $prevValue]
    relvar unset $relvarName

    # Evaluate the script.
    if {![set rCode [catch {uplevel $script} result]]} {
	# Make sure the result is still a relation, so any old relation command
	# will do. Since we need the heading later, we use that.
	if {![set rCode [catch {relation heading $var} result]]} {
	    # If we make it this far without any errors, we can re-create the
	    # relvar and set its value.
	    relvar create $relvarName $result
	    relvar set $relvarName $var
	    # Now attempt to restore the constraints
	    set rCode [catch {
		foreach cmd $cnstrCmds {
		    eval relvar $cmd
		}} result]
	}
    }
    if {$rCode} {
	# Something went wrong, restore the original relvar, its value and
	# the constraints.
	global errorCode
	global errorInfo

	set eCode $errorCode
	set eInfo $errorInfo
	# If something goes wrong here, we will have corrupted the
	# state of the relvars. Not good. This is why, ultimately, this
	# may have to be moved into the "C" part of the library.
	relvar create $relvarName $prevHeading
	relvar set $relvarName $prevValue
	foreach cmd $cnstrCmds {
	    eval relvar $cmd
	}
	return -code $rCode -errorinfo $eInfo -errorcode $eCode $result
    }
    return [relvar set $relvarName]
}

# These procs form a scheme for system defined identifiers.
# This scheme works as a reasonable way to assign system identifiers
# that are based on incrementing integers.
# Note that is possible to save and restore the contents of the
# relvar holding the identifiers, although that is not done here.
proc ::ralutil::sysIdsInit {} {
    if {[llength [relvar names ::__ral_systemids__]] == 0} {
	relvar create ::__ral_systemids__ {
	    Relation
	    {RelvarName string IdAttr string IdNum int}
	    {
		{RelvarName IdAttr}
	    }
	}
    }
}

# A convenience proc that installs a trace on an attribute of
# a relvar in order to have the system assign a unique value
# to that attribute.
proc ::ralutil::sysIdsGenSystemId {relvarName attrName} {
    relvar trace add variable $relvarName insert\
	[list ::ralutil::sysIdsCreateIdFor $attrName]
}

# Return the last system id assigned to a given attribute
proc ::ralutil::sysIdsLastId {relvarName attrName} {
    return [pipe {
	relvar set __ral_systemids__ |
	relation choose ~ RelvarName $relvarName IdAttr $attrName |
	relation extract ~ IdNum
    }]
}

# This proc can be used in a relvar trace on insert to
# create a unique identifier for an attribute.
proc ::ralutil::sysIdsCreateIdFor {attrName op relvarName tup} {
    relvar eval {
	set updated [\
	    relvar updateone __ral_systemids__ sysid\
		[list RelvarName $relvarName IdAttr $attrName] {
		set idValue [tuple extract $sysid IdNum]
		tuple update sysid IdNum [incr idValue]
	    }]
	if {!$updated} {
	    # If we don't update then we need to 
	    # find the maximum value of the atttribute in any tuple
	    # to use as our counter value.
	    set idValue [findMaxAttrValue $relvarName $attrName]
	    # The next one to use is one past the max
	    relvar insert __ral_systemids__ [list RelvarName $relvarName\
		IdAttr $attrName IdNum [incr idValue]]
	}
    }
    tuple update tup $attrName $idValue
    return $tup
}

# A helper procedure in finding the value for a system identifiers
proc ::ralutil::findMaxAttrValue {relvarName attrName} {
    set relValue [relvar set $relvarName]
    # If we don't find any, then we just start at 0
    set idValue 0
    if {[relation isnotempty $relValue]} {
	# We find the maximum by summarizing over DEE
	variable DEE
	set idValue [pipe {
	    relation summarize $relValue $DEE r\
		maxValue int {rmax($r, $attrName)} |
	    relation extract ~ maxValue}]
    }
    return $idValue
}
