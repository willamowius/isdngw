/*
  $Id: dsp.cxx,v 1.2 2009/05/22 22:50:33 jan Exp $
  
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

#include <ptlib.h>
#include <math.h>

#include "dsp.hxx"

#define SAMPLE_RATE 8000

/////////////////////////////////////////////////////
// class DspBuffer: a thread safe FIFO buffer      //
/////////////////////////////////////////////////////
//   size : size of the buffer                     //
//   init : number of elements in the buffer       //
//          (value = 0) after init                 //
/////////////////////////////////////////////////////

DspBuffer::DspBuffer (int _size, int init)
  : m_buf_size(_size)
{
  m_buf = (short*) malloc(m_buf_size * sizeof(short));

  if (init <= 0) {
    m_nr = 0;
    m_start = 0;
    m_end = 0;
  }

  for (int i=0; i < init; i++)
    m_buf[i] = 0;

  m_nr = init;
  m_start = 0;
  m_end = init;
}


DspBuffer::~DspBuffer ()
{
  free(m_buf);
}


PBoolean DspBuffer::Empty ()
{
  PWaitAndSignal lmutex(m_mutex);

  return (m_nr == 0);
}


PBoolean DspBuffer::Full ()
{
  PWaitAndSignal lmutex(m_mutex);

  return (m_nr == m_buf_size);
}


PBoolean DspBuffer::Delete ()
{
  PWaitAndSignal lmutex(m_mutex);

  if (m_nr > 0) {
    m_start = (m_start+1) % m_buf_size;
    m_nr--;
    return TRUE;
  }

  return FALSE;
}


PBoolean DspBuffer::ReadDelete (short &out)
{
  PWaitAndSignal lmutex(m_mutex);

  if (m_nr > 0) {
    out = m_buf[m_start];
    m_start = (m_start+1) % m_buf_size;
    m_nr--;
    return TRUE;
  }

  return FALSE;
}


PBoolean DspBuffer::ReadAt (int pos, short &out)
{
  PWaitAndSignal lmutex(m_mutex);

  if (pos < m_nr) {
    out = m_buf[(m_start + pos) % m_buf_size];
    return TRUE;
  }

  return FALSE;
}


PBoolean DspBuffer::Write (const short in)
{
  PWaitAndSignal lmutex(m_mutex);

  if (m_nr >= m_buf_size)
    return FALSE;

  m_buf[m_end] = in;
  m_nr++;
  m_end = (m_end + 1) % m_buf_size;

  return TRUE;
}


int DspBuffer::ReadArray (short *out, int len)
{
  int i;

  PWaitAndSignal lmutex(m_mutex);

  if (m_nr <= 0)
    return 0;

  for (i = 0; i < len; i++) {
    out[i] = m_buf[m_start];
    m_start = (m_start + 1) % m_buf_size;
    m_nr--;
    if (m_nr <= 0)
      return i;
  }

  return i;
}


int DspBuffer::WriteArray (const short *out, int len)
{
  int i;

  PWaitAndSignal lmutex(m_mutex);

  if (m_nr >= m_buf_size)
    return 0;

  for (i = 0; i < len; i++) {
    m_buf[m_end] = out[i];
    m_end = (m_end + 1) % m_buf_size;
    m_nr++;
    if (m_nr >= m_buf_size)
      return i;
  }

  return i;
}


/////////////////////////////////////////////////////
// class DspEchoComp: compensates echos            //
/////////////////////////////////////////////////////
//   _calculate_max_in:                            //
//        Should we use the full dynamic range     //
//        (FALSE) or not (TRUE). When using AGC    //
//        use false.                               //
/////////////////////////////////////////////////////
// Use the in() function for measuring one channel //
// and the out() function to mute the second       //
// channel.                                        //
/////////////////////////////////////////////////////

DspEchoComp::DspEchoComp (PBoolean _calculate_max_in) :
  m_calculate_max_in(_calculate_max_in)
{
  m_buf_data = new DspBuffer(SAMPLE_RATE/10, 200);
  
  // 0.5s buffer, init: 3 samples (-> delay)
  m_buf_energy = new DspBuffer(20, 3);  

  m_count_in = 0;
  m_count_out = 0;

  for (int i=0; i<COEFF_NR; i++)
    m_buf_energy_old[i] = 0;

  // are we using the full dynamic range?
  if (m_calculate_max_in)
    m_max_in = 15000;  
  else
    m_max_in = SHRT_MAX;

  m_no_energy_info = 0;
}


DspEchoComp::~DspEchoComp ()
{
  delete m_buf_data;
  delete m_buf_energy;
}


void DspEchoComp::Energy ()
{
  int   i;
  int   energy;
  short max_new;
  
  // calculate the energy of 200 samples and their maximum
  energy = 0;
  max_new = 0;
  for (i=0; i<200; i++) {
    short tmp;
    if (m_buf_data->ReadDelete(tmp)) {
      energy += abs(tmp);
      if (m_calculate_max_in)
        if (abs(tmp) > max_new)
          max_new = abs(tmp);
    }
    else
      break;
  }  
  if (i != 0) {
    energy /= i;

    // calculate the new maximum
    if ((i > 50) && (m_calculate_max_in)) {
      if (max_new > m_max_in)
        m_max_in += (short)((max_new - m_max_in) * 0.05);
      else
        m_max_in += (short)((max_new - m_max_in) * 0.001);
      
      if (m_max_in < 2000)
        m_max_in = 2000;
    }
  }

  // not enough space in the energy buffer 
  // -> delete oldest value  
  if (m_buf_energy->Full())
    m_buf_energy->Delete();
  
  m_buf_energy->Write((short)energy);
}


void DspEchoComp::In (short data)
{
  m_buf_data->Write (data);
  
  if (m_count_in == 0)
    Energy();
  m_count_in++;
  
  if (m_count_in == 200)
    m_count_in = 0;
}


void DspEchoComp::Out (short &data)
{
  const float fir[COEFF_NR] = 
      //        { 0.632149, 0.232555, 0.085552, 0.031473, 0.011578, 
      //          0.004259, 0.001567, 0.000576, 0.000212, 0.000078 };
      //        { 0.396139, 0.240270, 0.145731, 0.088390, 0.053612, 
      //          0.032517, 0.019723, 0.011962, 0.007256, 0.004401 };
                { 0.272762, 0.202067, 0.149695, 0.110897, 0.082154, 
                  0.060861, 0.045087, 0.033401, 0.024744, 0.018331 };


  // calculate energy (filter)?
  if (m_count_out == 0) {
    m_energy = 0.0;

    // create sample buffer
    for (int i=COEFF_NR-1; i>0; i--)     
      m_buf_energy_old[i] = m_buf_energy_old[i-1];
    // energy information available?
    if (!m_buf_energy->Empty()) {
      // yes
      m_buf_energy->ReadDelete(m_buf_energy_old[0]);
      m_no_energy_info = 0;
    }
    else {
      // no
      if (m_no_energy_info < 5)
        m_buf_energy_old[0] = m_buf_energy_old[1];
      else
        m_buf_energy_old[0] = (short)(m_buf_energy_old[1] * 0.5);
      m_no_energy_info++;
    }

    // low pass filter
    for (int i=0; i<COEFF_NR; i++)       
      m_energy += fir[i] * m_buf_energy_old[i];

    // calculate the attenuation
    m_attenuation = m_energy / m_max_in;
    if (m_attenuation > 0.475)
      m_attenuation = 0.95;
    else if (m_attenuation > 0.05)
      m_attenuation *= 2.0;
    m_attenuation = 1.0 - m_attenuation;
  }

  m_count_out++;
  if (m_count_out == 200)
    m_count_out = 0;

  data = (short)(data * m_attenuation);
}



/////////////////////////////////////////////////////
// class DspGain: automatic gain control (AGC)     //
/////////////////////////////////////////////////////

DspGain::DspGain ()
{
  m_sample_max = 1;
  m_counter = 0;
  m_gain = 1.0;
  m_silence_counter = 0;
}


short DspGain::Convert (short in)
{
  if ((unsigned int)abs(in) > m_sample_max)    // get max
    m_sample_max = abs(in);
  m_counter ++;

  // Will we get a overflow with the gain factor?
  // Yes: Calculate new gain.
  if (abs(in) * m_gain > 32000.0) {
    m_gain = ((float)SHRT_MAX / (float)m_sample_max) * 0.95;
    m_silence_counter = 0;
    return (short)(in * m_gain);
  }

  // Calculate new gain factor 10x per second
  if (m_counter >= SAMPLE_RATE/10) {
    if (m_sample_max > 800) {
      // speaking?
      float gain_new = ((float)SHRT_MAX / (float)m_sample_max) * 0.95;

      if (m_silence_counter > 40)  // pause -> speaking
        m_gain += (gain_new - m_gain) * 0.25;
      else
        m_gain += (gain_new - m_gain) * 0.05;

      m_silence_counter = 0;
    }
    else {
      // silence
      m_silence_counter++;
      // silence > 2s: reduce gain
      if ((m_gain > 1.0) && (m_silence_counter >= 20))
        m_gain *= 0.95; 
    }

    m_counter = 0;
    m_sample_max = 1;
  }

  return (short)(in * m_gain);
}


void DspGain::ConvertArray (short *in, short *out, int nr)
{
  for (int i=0; i<nr; i++)
    out[i] = Convert(in[i]);
}


/////////////////////////////////////////////////////
// class DspEncodeDTMF: a DTMF encoder             //
/////////////////////////////////////////////////////
//   time_on: duration of DTMF signal              //
//   time_out: pause after DTMF signal             //
//   sample_rate: needed sample rate of the signal //
/////////////////////////////////////////////////////

DspEncodeDTMF::DspEncodeDTMF (float time_on, float time_off, 
                              int sample_rate, int max_value)
{
  /*        1209    1336    1477    1633
   *  697     1       2       3       A
   *  770     4       5       6       B
   *  852     7       8       9       C
   *  941     *       0       #       D
   */


  const int f_low[16] = { 941, 697, 697, 697, 770, 770, 770, 852,
                          852, 852, 941, 941, 697, 770, 852, 941 };
  const int f_high[16] = { 1336, 1209, 1336, 1477, 1209, 1336,
                           1477, 1209, 1336, 1477, 1209, 1477,
                           1633, 1633, 1633, 1633 };

  // number of samples
  m_samples = (int)(sample_rate*(time_on+time_off));

  // calculate the 16 DTMF tones   
  for (int i=0; i<16; i++) {
    m_buf[i] = (short*)malloc(m_samples  * sizeof(short));
    for (int t=0; t<sample_rate*time_on; t++)
      m_buf[i][t] = (short)((sin(2*M_PI*f_low[i]*t/sample_rate) + sin(2*M_PI*f_high[i]*t/sample_rate)) * max_value);
    for (int t=(int)(sample_rate*time_on); t<m_samples; t++)
      m_buf[i][t] = 0;
  }
}


DspEncodeDTMF::~DspEncodeDTMF ()
{
  for (int i=0; i<16; i++)
    free(m_buf[i]);
}


const short *DspEncodeDTMF::GetSound (char button)
{
  switch(button) {
  case '0':
    return m_buf[0];
    
  case '1':
    return m_buf[1];
    
  case '2':
    return m_buf[2];
    
  case '3':
    return m_buf[3];
    
  case '4':
    return m_buf[4];
    
  case '5':
    return m_buf[5];

  case '6':
    return m_buf[6];
    
  case '7':
    return m_buf[7];
    
  case '8':
    return m_buf[8];
    
  case '9':
    return m_buf[9];
    
  case '*':
    return m_buf[10];
    
  case '#':
    return m_buf[11];
    
  case 'a':
  case 'A':
    return m_buf[12];
    
  case 'b':
  case 'B':
    return m_buf[13];
    
  case 'c':
  case 'C':
    return m_buf[14];
    
  case 'd':
  case 'D':
    return m_buf[15];
  }
  
  return NULL;
}


int DspEncodeDTMF::GetNumberOfSamples ()
{
  return m_samples;
}



