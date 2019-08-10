/*
  $Id: dsp.hxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
   Copyright (c) 2000 Virtual Net

                    http://www.virtual-net.fr

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   Parts of this file are from isdn2h323, by Marco Budde.
   Copyright (c) 1999-2000 telos EDV Systementwicklung GmbH,
   Hamburg (Germany)          http://www.telos.de

   Some fixes made by Niklas Ögren <niklas.ogren@7l.se>
   http://www.7l.se at 2003-05-09
*/

#ifndef _DSP_HXX_
#define _DSP_HXX_

#include <ptlib.h>

#if !defined(P_USE_STANDARD_CXX_BOOL) && !defined(P_USE_INTEGER_BOOL)
	typedef int PBoolean;
#endif

class DspBuffer : public PObject
{
  PCLASSINFO(DspBuffer, PObject);

public:
  DspBuffer (int _size, int init = 0);
  ~DspBuffer ();
  PBoolean Empty ();
  PBoolean Full ();
  PBoolean Delete ();
  PBoolean ReadDelete (short &out);
  PBoolean ReadAt (int pos, short &out);
  PBoolean Write (const short in);
  int ReadArray (short *out, int len);
  int WriteArray (const short *out, int len);
  
private:
  int           m_buf_size;     // the size of the buffer
  short*        m_buf;          // the buffer
  int           m_nr;           // number of elements in the buffer
  int           m_start;  
  int           m_end;
  PMutex        m_mutex;
};


#define COEFF_NR 10


class DspEchoComp : public PObject
{
  PCLASSINFO(DspEchoComp, PObject);

public:
  DspEchoComp (PBoolean _calculate_max_in);
  ~DspEchoComp ();
  void Energy ();
  void In (short data);
  void Out (short &data);

private:
  DspBuffer*    m_buf_data;
  DspBuffer*    m_buf_energy;
  int           m_count_in, m_count_out;
  short         m_buf_energy_old[COEFF_NR];
  float         m_energy;
  float         m_attenuation;
  short         m_max_in;
  PBoolean          m_calculate_max_in;
  int           m_no_energy_info;
};


class DspGain : public PObject
{
  PCLASSINFO(DspGain, PObject);

public:
  DspGain ();
  short Convert (short in);
  void ConvertArray (short *in, short *out, int nr);
  
private:
  unsigned int  m_sample_max;
  int           m_counter;
  float         m_gain;
  int           m_silence_counter;
};


class DspEncodeDTMF : public PObject
{
  PCLASSINFO(DspEncodeDTMF, PObject);

public:
  DspEncodeDTMF (float time_on, float time_off,
                 int sample_rate, int max_value);
  ~DspEncodeDTMF ();
  const short *GetSound (char button);
  int GetNumberOfSamples ();
  
private:
  short*        m_buf[16];
  int           m_samples;
};

#endif /* _DSP_HXX_ */ 
