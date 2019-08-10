# $Id: Makefile,v 1.3 2015/01/27 20:43:57 jan Exp $
#
# Makefile for isdngw, an ISDN-H.323 gateway
# based on isdn2h323 by telos EDV Systementwicklung GmbH.
#
# Copyright (c) 2000 Virtual Net
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# Bertrand Croq <bcroq@virtual-net.fr>

#   Some fixes made by Niklas Ögren <niklas.ogren@7l.se>
#   http://www.7l.se at 2003-05-09

PROG    = isdngw
SOURCES := audio_delay.cxx \
          connection.cxx \
          dsp.cxx \
          endpoint.cxx \
          potschannel.cxx \
          lcr.cxx \
          loopchannel.cxx \
          main.cxx \
          nullchannel.cxx \
          pots.cxx \
          resource.cxx \
          routing.cxx \
          config.cxx

ifndef OPENH323DIR
OPENH323DIR=$(HOME)/h323plus
endif

include $(OPENH323DIR)/openh323u.mak

