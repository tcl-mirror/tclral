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
# $RCSfile: ral.tcl,v $
# $Revision: 1.2 $
# $Date: 2004/07/10 18:02:10 $
#  *--

namespace eval ::ral {
    namespace export relformat
}

proc ::ral::relformat {relValue {title {}}} {
    set result {}
    if {[string length $title]} {
	append result $title\n
	append result [string repeat - [string length $title]]\n
    }
    set heading [lindex [relation typeof $relValue] 1 0]
    foreach h $heading {
	foreach {attr type} $h {
	    set colWidth($attr) [string length $attr]
	    set typeWidth [string length [lindex $type 0]]
	    if {$typeWidth > $colWidth($attr)} {
		set colWidth($attr) $typeWidth
	    }
	}
    }
    relation foreach t $relValue {
	foreach h $heading {
	    foreach {attr type} $h {
		set v [tuple extract $t $attr]
		switch [lindex $type 0] {
		    Tuple -
		    Relation {
			set v [lindex $v 2]
		    }
		}
		set valueLen [string length $v]
		if {$valueLen > $colWidth($attr)} {
		    set colWidth($attr) $valueLen
		}
	    }
	}
    }
    set fmtStr {}
    foreach h $heading {
	foreach {attr type} $h {
	    append fmtStr "%$colWidth($attr)s  "
	}
    }
    set fmtStr [string trimright $fmtStr]
    append fmtStr "\n"

    # Attributes
    set fmtCmd [list format $fmtStr]
    set fmtLine $fmtCmd
    foreach h $heading {
	foreach {attr type} $h {
	    lappend fmtLine $attr
	}
    }
    append result [eval $fmtLine]

    # Types
    set fmtLine $fmtCmd
    foreach h $heading {
	foreach {attr type} $h {
	    lappend fmtLine [lindex $type 0]
	}
    }
    set typesLine [eval $fmtLine]
    append result $typesLine
    append result [string repeat "=" [expr {[string length $typesLine] - 1}]]\n

    # Values
    relation foreach t $relValue {
	set fmtLine $fmtCmd
	foreach h $heading {
	    foreach {attr type} $h {
		set v [tuple extract $t $attr]
		switch [lindex $type 0] {
		    Tuple -
		    Relation {
			set v [lindex $v 2]
		    }
		}
		lappend fmtLine $v
	    }
	}
	append result [eval $fmtLine]
    }

    return [string trimright $result "\n"]
}
