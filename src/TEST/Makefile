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
# $RCSfile: Makefile,v $
# $Revision: 1.3 $
# $Date: 2006/02/20 20:15:11 $
#  *--

PROGS = \
    test_ral_vector \
    test_ral_attribute \
    test_ral_tupleheading \
    test_ral_tuple \
    test_ral_tupleobj \
    test_ral_relationheading \
    test_ral_relation \
    $(NULL)

# This is set up to use either 8.4 or 8.5. Just uncomment the one
# appropriate to your Tcl installation.
#TCLBASE = /usr/local/ActiveTcl-8.4.7
#LDLIBS = -ltclstub8.4
TCLBASE = /usr/local/ActiveTcl
LDLIBS = -ltclstub8.5

CFLAGS = -g -I.. -I$(TCLBASE)/include/tcl8.5 -Wall
# Debugging may be easier if optimization is turned off.
#CFLAGS += -O2

# Uncomment this for test coverage profiling.
#CFLAGS += -fprofile-arcs -ftest-coverage

LDFLAGS = -g
LOADLIBES = -L$(TCLBASE)/lib -L..
CLEANFILES=\
    $(PROGS)\
    core.*\
    $(NULL)

.PHONY : runtests testprogs

runtests : testprogs
	@for p in $(PROGS) ; do echo $$p ; $$p ; done

testprogs : $(PROGS)

test_ral_vector : test_ral_vector.c libraltest.a ../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

test_ral_attribute : test_ral_attribute.c libraltest.a ../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

test_ral_tupleheading : test_ral_tupleheading.c libraltest.a ../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

test_ral_tuple : test_ral_tuple.c libraltest.a ../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

test_ral_tupleobj : test_ral_tupleobj.c libraltest.a ../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

test_ral_relationheading : test_ral_relationheading.c libraltest.a\
	../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

test_ral_relation : test_ral_relation.c libraltest.a ../libral0.8.so
	$(CC) $(CFLAGS) $^ $(LOADLIBES) -lral0.8 -ltcl8.5 libraltest.a -o $@

libraltest.a : libraltest.a(log.o)

libraltest.a(log.o) : log.o

log.o : log.h

clean :
	$(RM) $(CLEANFILES)
