#!/bin/bash
# \
exec wish "$0" "$@"
# This software is copyrighted 2004 by G. Andrew Mangogna.  The following
# terms apply to all files associated with the software unless explicitly
# disclaimed in individual files.
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
# 
# ABSTRACT:
# 
# $RCSfile: ralview.tcl,v $
# $Revision: 1.4 $
# $Date: 2009/07/12 22:56:10 $
#  *--

lappend auto_path [file join .. src]

package require BWidget 1.7
package require Tktable 2.9
package require ral 0.8
package require struct 2.1

namespace eval ::ralview {
    namespace import ::ral::*

    variable status {}
    variable version "Version 0.2"
    variable filetypes {
	{{Metakit files} .mk}
	{{All Files} *}
    }

    option add *background ivory1
    option add *activeBackground ivory2
    option add *Table.borderWidth 1
    option add *Table.highlightThickness 2
    option add *Table.font {Courier 14}
}

proc ::ralview::main {{filename {}}} {
    set mf_menu {
	"&File" {} {} 0 {
	    {command "&New" {} "Create a new data set" {Ctrl n}\
		-command ::ralview::newmenu}
	    {command "&Open" {} "Open an existing data set" {Ctrl o}\
		-command ::ralview::openmenu}
	    {command "&Save" {} "Save file" {Ctrl s}\
		-command ::ralview::savemenu}
	    {separator}
	    {command "E&xit" {} "Exit application" {Ctrl q}\
		-command ::ralview::exitappl}
	}
	"&Edit" {} {} 0 {
	    {command "Cu&t" {} "Cut selection to clipboard" {Ctrl x}\
		-command ::ralview::cutmenu}
	    {command "&Copy" {} "Copy selection to clipboard" {Ctrl c}\
		-command ::ralview::copymenu}
	    {command "&Paste" {} "Paste clipboard to selection" {Ctrl v}\
		-command ::ralview::pastemenu}
	}
    }

    variable mainframe
    set mainframe [MainFrame .mf\
	-textvariable ::ralview::status\
	-menu $mf_menu\
	-separator both\
    ]

    $mainframe showstatusbar status
    $mainframe addindicator -textvariable ::ralview::version

    set tb [$mainframe addtoolbar]
    set bbox [ButtonBox $tb.bbox\
	-spacing 0\
	-padx 1\
	-pady 1\
    ]
    set tb_opts {
	-highlightthickness 0
	-takefocus 0
	-relief link
	-borderwidth 1
	-padx 1
	-pady 1
    }
    $bbox add\
	-image [Bitmap::get new]\
	-command ::ralview::openmenu\
	-helptext "Create a new data set"\
	{*}$tb_opts
    $bbox add\
	-image [Bitmap::get open]\
	-command ::ralview::openmenu\
	-helptext "Open an existing data set"\
	{*}$tb_opts
    $bbox add\
	-image [Bitmap::get save]\
	-command ::ralview::savemenu\
	-helptext "Save file"\
	{*}$tb_opts
    $bbox add\
	-image [Bitmap::get cut]\
	-command ::ralview::cutmenu\
	-helptext "Cut selection to clipboard"\
	{*}$tb_opts
    $bbox add\
	-image [Bitmap::get copy]\
	-command ::ralview::copymenu\
	-helptext "Copy selection to clipboard"\
	{*}$tb_opts
    $bbox add\
	-image [Bitmap::get paste]\
	-command ::ralview::pastemenu\
	-helptext "Paste clipboard to selection"\
	{*}$tb_opts
    pack $bbox -side left -anchor w
    pack $mainframe -fill both -expand yes

    if {[string length $filename]} {
	openfile $filename
    }

    return
}

proc ::ralview::newmenu {} {
    . configure -cursor watch

    clearDataDisplay

    variable mainframe
    set frame [$mainframe getframe]
    variable notebook
    set notebook [NoteBook $frame.nb]
    $notebook compute_size
    pack $notebook -expand yes -fill both -padx 4 -pady 4

    . configure -cursor ""

    return
}

proc ::ralview::openmenu {} {
    variable filetypes
    set filename [tk_getOpenFile\
	-title "Open RAL dump"\
	-filetypes $filetypes
    ]

    if {[string length $filename] == 0} {
	return
    }

    . configure -cursor watch
    openfile $filename
    . configure -cursor ""

    return
}

proc ::ralview::showRelVars {nb} {
    variable notebook
    foreach n [lsort -dictionary [relvar names]] {
	set n [namespace tail $n]
	$notebook insert end $n -text $n
	set pf [$notebook getframe $n]
	createRelvarWindow $pf $n
    }
    return
}

proc ::ralview::createRelvarWindow {frame relvarName} {
    set reltab [createRelationWindow $frame.rel_$relvarName\
	[relvar set $relvarName]]
    $reltab configure\
	-xscrollcommand [list $frame.hscroll set]\
	-yscrollcommand [list $frame.vscroll set]
    scrollbar $frame.hscroll\
	-orient horizontal\
	-command [list $reltab xview]
    scrollbar $frame.vscroll\
	-orient vertical\
	-command [list $reltab yview]
    grid $reltab $frame.vscroll\
	-sticky news
    grid $frame.hscroll\
	-sticky ew
    grid columnconfig $frame 0 -weight 1
    grid rowconfig $frame 0 -weight 1

    return
}

proc ::ralview::createRelationWindow {win relation} {
    # set up the array that is used as backing storage
    upvar ::ralview::tab$win tableArray
    upvar ::ralview::colMap$win colMap
    upvar ::ralview::typeMap$win typeMap
    set colMap [dict create]
    set typeMap [dict create]
    set colCntr 0
    # attribute names and types as title rows
    foreach {attrName attrType} [lindex [relation heading $relation] 1] {
	set tableArray(-2,$colCntr) $attrName
	set valueType [lindex $attrType 0]
	set tableArray(-1,$colCntr) $valueType
	dict set colMap $attrName $colCntr
	dict set typeMap $attrName $attrType
	incr colCntr
    }

    set reltab [table $win\
	-roworigin -2\
	-colorigin -1\
	-titlerows 2\
	-titlecols 1\
	-colstretchmode unset\
	-rows [expr {[relation cardinality $relation] + 3}]\
	-cols [relation degree $relation]\
	-width 0\
	-height 0\
	-flashmode on\
	-selectmode extended\
	-variable ::ralview::tab$win]

    # put in the data
    set rowCntr 0
    relation foreach r $relation {
	set t [relation tuple $r]
	dict for {attrName attrValue} [tuple get $t] {
	    set attrType [dict get $typeMap $attrName]
	    set col [dict get $colMap $attrName]

	    set tableArray($rowCntr,-1) $rowCntr
	    set value [tuple extract $t $attrName]
	    set tableArray($rowCntr,$col) $value
	    set valueType [lindex $attrType 0]
	    if {$valueType eq "Tuple"} {
		$reltab window configure $rowCntr,$col\
		    -window [createTupleWindow $reltab.attr_$rowCntr$col\
			$value]\
		    -sticky news\
		    -padx 2\
		    -pady 2
	    } elseif {$valueType eq "Relation"} {
		$reltab window configure $rowCntr,$col\
		    -window [createRelationWindow $reltab.attr_$rowCntr$col\
			$value]\
		    -sticky news\
		    -padx 2\
		    -pady 2
	    }
	}
	incr rowCntr
    }

    # Some configuration
    $reltab tag config title\
	-relief groove\
	-fg sienna\
	-bg beige\
	-font {Courier 12 bold}
    $reltab tag config sel\
	-bg ivory2
    $reltab tag config active\
	-bg ivory3
    $reltab width -1 4

    return $reltab
}

proc ::ralview::createTupleWindow {win tuple} {
    upvar ::ralview::tab$win tableArray
    upvar ::ralview::colMap$win colMap

    set tuptab [table $win\
	-colstretchmode unset\
	-rowstretchmode unset\
	-rows 3\
	-titlerows 2\
	-roworigin -2\
	-cols [tuple degree $tuple]\
	-width 0\
	-height 0\
	-selectmode extended\
	-variable ::ralview::tab$win\
    ]

    set col 0
    foreach {attrName attrType} [lindex [tuple heading $tuple] 1] {
	set tableArray(-2,$col) $attrName
	set valType [lindex $attrType 0]
	set tableArray(-1,$col) $valType
	set value [tuple extract $tuple $attrName]
	set tableArray(0,$col) $value
	if {$valType eq "Tuple"} {
	    $tuptab window configure 0,$col\
		-window [createTupleWindow $tuptab.tuple_$col $value]\
		-sticky news\
		-padx 2\
		-pady 2
	} elseif {$valType eq "Relation"} {
	    $tuptab window configure 0,$col\
		-window [createRelationWindow $tuptab.tuple_$col $value]\
		-sticky news\
		-padx 2\
		-pady 2
	}
	incr col
    }

    return $tuptab
}

proc ::ralview::savemenu {} {
    variable filetypes
    set filename [tk_getSaveFile\
	-title "Save RAL dump"\
	-filetypes $filetypes
    ]

    if {[string length $filename] == 0} {
	return
    }
}

proc ::ralview::exitappl {} {
    ::exit 0
}

proc ::ralview::cutmenu {} {
}

proc ::ralview::copymenu {} {
}

proc ::ralview::pastemenu {} {
}

proc ::ralview::openfile {filename} {
    clearDataDisplay
    if {[catch {loadFromMk $filename} msg]} {
	MessageDlg::create .openerror -icon error -type ok -message $msg
	return
    }

    variable mainframe
    set frame [$mainframe getframe]
    variable notebook
    set notebook [NoteBook $frame.nb]
    showRelVars $notebook

    $notebook compute_size
    pack $notebook -expand yes -fill both -padx 4 -pady 4
    $notebook raise [$notebook page 0]
    return
}

proc ::ralview::clearDataDisplay {} {
    variable mainframe
    set frame [$mainframe getframe]
    destroy {*}[winfo children $frame]
    relvar unset {*}[relvar names]
    return
}

::ralview::main [lindex $argv 0]
