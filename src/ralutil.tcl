# This software is copyrighted 2006, 2007, 2008, 2009 by G. Andrew Mangogna.
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
# $Revision: 1.15.2.3 $
# $Date: 2009/02/17 02:28:11 $
#  *--

package provide ralutil 0.9.0

package require ral

namespace eval ::ralutil {
    namespace export pipe
    namespace export sysIdsInit
    namespace export sysIdsGenSystemId
    namespace export crosstab

    namespace import ::ral::*

    # We define the very special relation values of DEE and DUM.
    # DUM is the relation value that has an empty heading and an empty body.
    # DEE is the relation value that has an empty heading and a body consisting
    # of one tuple that is the empty tuple (the only body that an empty heading
    # relation value could have).
    variable DUM {{} {}}
    variable DEE {{} {{}}}
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

# These procs form a scheme for system defined identifiers.
# This scheme works as a reasonable way to assign system identifiers
# that are based on incrementing integers.
# Note that is possible to save and restore the contents of the
# relvar holding the identifiers, although that is not done here.
proc ::ralutil::sysIdsInit {} {
    if {![relvar exists ::__ral_systemids__]} {
	relvar create ::__ral_systemids__\
            {RelvarName string IdAttr string IdNum int} {RelvarName IdAttr}
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
	relvar restrictone  __ral_systemids__\
                RelvarName $relvarName IdAttr $attrName |
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
	if {[relation isempty $updated]} {
	    # If we don't update then we need to 
	    # find the maximum value of the atttribute in any tuple
	    # to use as our counter value.
	    set idValue [findMaxAttrValue $relvarName $attrName]
	    # The next one to use is one past the max
	    relvar insert __ral_systemids__ [list RelvarName $relvarName\
		IdAttr $attrName IdNum [incr idValue]]
	}
    }
    return [tuple update tup $attrName $idValue]
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

# crosstab relValue crossAttr ?attr1 attr2 ...?
#
# Generate a cross tabulation of "relValue" for the "crossAttr" against the
# variable number of attributes given. The "crossAttr" argument is the name of
# an attribute of "relValue". The idea is to create new relation that contains
# all the attributes in "args" plus a new attribute for each distinct value of
# "crossAttr". The value of the new attributes is the count of tuples that have
# the corresponding value of "crossAttr".  Relationally, the "summarize"
# command is used when computations are required across groups of tuples.
proc ::ralutil::crosstab {relValue crossAttr args} {
    # We start by projecting the attributes that will be retained
    # in the resulting relation.
    set subproj [relation project $relValue {*}$args]
    # The strategy is to build up a summarize command on the fly, adding new
    # attributes. So we start with the constant part of the command.
    set sumCmd [list relation summarize $relValue $subproj r]

    # By projecting on the "crossAttr" we get the unique set of values
    # for that attribute since there are no duplicates in relations.
    set crossproj [relation project $relValue $crossAttr]
    # For each distinct value of the "crossAttr" extend the relation with
    # a new attribute by the same name as the value and whose value is
    # the number of tuples which match the value.
    foreach val [lsort [relation list $crossproj]] {
	 set sumexpr [format\
	     {[relation cardinality [relation restrictwith $r {$%s == "%s"}]]}\
	     $crossAttr $val]
	 lappend sumCmd $val int $sumexpr
    }
    # Finally we want the total for all the "crossAttr" matches.
    lappend sumCmd Total int {[relation cardinality $r]}
    return [eval $sumCmd]
}
