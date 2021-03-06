# -*- Mode: makefile -*-
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/NPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is Mozilla Communicator client code, released
# March 31, 1998.
#
# The Initial Developer of the Original Code is Netscape
# Communications Corporation.  Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#
# Alternatively, the contents of this file may be used under the
# terms of the GNU Public License (the "GPL"), in which case the
# provisions of the GPL are applicable instead of those above.
# If you wish to allow use of your version of this file only
# under the terms of the GPL and not to allow others to use your
# version of this file under the NPL, indicate your decision by
# deleting the provisions above and replace them with the notice
# and other provisions required by the GPL.  If you do not delete
# the provisions above, a recipient may use your version of this
# file under either the NPL or the GPL.
#

#
# Config stuff for HPUX
#

#ifdef NS_USE_NATIVE
#  CC  = XXX
#  LD  = XXX
#else
  CC = gcc -Wall -Wno-format -fPIC
  CCC = g++ -Wall -Wno-format -fPIC
#endif


CFLAGS += -DXP_UNIX -DAIX -DAIXV3 -DSYSV -DHAVE_LOCALTIME_R
RANLIB = ranlib
MKSHLIB = $(LD) -b

SO_SUFFIX = sl

#.c.o:
#	$(CC) -c -MD $*.d $(CFLAGS) $<

ARCH := aix
CPU_ARCH = rs6000
GFX_ARCH = x
INLINES = js_compare_and_swap:js_fast_lock1:js_fast_unlock1:js_lock_get_slot:js_lock_set_slot:js_lock_scope1


MKSHLIB = $(CCC) -Wl,-berok
#MKSHLIB = $(LD) -bM:SRE -bh:4 -bnoentry -berok
#MKSHLIB = $(LD) -G
#Ab 5.3: MKSHLIB = $(LD) -bsvr4 -G -z nodefs
#MKSHLIB = /usr/lpp/xlC/bin/makeC++SharedLib_r -p 0 -G -berok

ifdef JS_THREADSAFE
XLDFLAGS += -ldl
endif
