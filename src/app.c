/******************************************************************************

 Copyright (c) 2015, Focusrite Audio Engineering Ltd.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 * Neither the name of Focusrite Audio Engineering Ltd., nor the names of its
 contributors may be used to endorse or promote products derived from
 this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *****************************************************************************/

//______________________________________________________________________________
//
// Headers
//______________________________________________________________________________

#include "app.h"

//______________________________________________________________________________
//
// This is where the fun is!  Add your code to the callbacks below to define how
// your app behaves.
//
// In this example, we either render the raw ADC data as LED rainbows or store
// and recall the pad state from flash.
//______________________________________________________________________________

// store ADC frame pointer
//static const u16 *g_ADC = 0;

// buffer to store pad states for flash save
//#define BUTTON_COUNT 100

//u8 g_Buttons[BUTTON_COUNT] = {0};

struct COLOR
{
  u8 r;
  u8 g;
  u8 b;
};


// Colors: 0 white   1 red   2 orange   3 green   4 cyan   5 blue   6 purple   7 off
const struct COLOR ColorsBright[8] = {{MAXLED, MAXLED, MAXLED}, {MAXLED, 0, 0},
				      {MAXLED, MAXLED/2, 0}, {0, MAXLED, MAXLED/4},
				      {0, MAXLED, MAXLED}, {0, 0, MAXLED},
				      {MAXLED-24, 0, MAXLED}, {0, 0, 0}};

const struct COLOR ColorsDim[8] = {{MAXLED/8, MAXLED/8, MAXLED/8}, {MAXLED/8, 0, 0},
				   {MAXLED/8, MAXLED/16, 0}, {0, MAXLED/8, MAXLED/16},
				   {0, MAXLED/8, MAXLED/8}, {0, 0, MAXLED/8},
				   {(MAXLED-24)/8, 0, MAXLED/8}, {0, 0, 0}};

const u8 PadColors[100] = {7, 7, 7, 7, 7,  // 0-4
			   7, 7, 7, 7, 7,  // 5-9
			   7, 2, 7, 0, 0,  // 10-14
			   0, 0, 7, 1, 7,  // 15-19
			   7, 0, 7, 7, 7,  // 20-24
			   7, 7, 7, 3, 7,  // 25-29
			   7, 7, 4, 4, 4,  // 30-34
			   7, 7, 7, 7, 7,  // 35-39
			   7, 7, 6, 6, 6,  // 40-44
			   7, 5, 7, 0, 7,  // 45-49
			   7, 7, 5, 5, 5,  // 50-54
			   7, 6, 5, 0, 7,  // 55-59
			   7, 7, 1, 1, 1,  // 60-64
			   6, 1, 6, 0, 7,  // 65-69
			   7, 0, 0, 0, 0,  // 70-74
			   5, 0, 0, 0, 7,  // 75-79
			   7, 2, 2, 2, 2,  // 80-84
			   0, 2, 2, 0, 7,  // 85-89
			   7, 7, 7, 7, 7,  // 90-94
			   7, 7, 7, 7, 7};  // 95-100


const u8 PeerList[12][6] = {{81, 71, 0, 0, 0, 0}, {82, 72, 62, 52, 42, 32},
			    {83, 73, 63, 53, 43, 33}, {84, 74, 64, 54, 44, 34},
			    {85, 75, 65, 0, 0, 0}, {86, 76, 66, 56, 46, 0},
			    {87, 77, 67, 57, 0, 0}, {88, 78, 68, 58, 48, 0},
			    {21, 11, 0, 0, 0, 0}, {13, 14, 15, 16, 0, 0},
			    {28, 18, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0}};

static u8 PadState[100];

void app_surface_event(u8 type, u8 index, u8 value)
{
  u8 peers = 11;
  u8 k = 0;

  switch (type)
    {
        case  TYPEPAD:
        {
	  if(0 < value) // only act onPress
	    {
	      if(1 == PadState[index]) // we're already lit
		{
		  hal_plot_led(TYPEPAD, index, ColorsDim[PadColors[index]].r,
			       ColorsDim[PadColors[index]].g, ColorsDim[PadColors[index]].b);
		  PadState[index] = 0;
		  hal_send_midi(USBMIDI, NOTEON, index, value);
		}
	      else // light us up
		{
		  hal_plot_led(TYPEPAD, index, ColorsBright[PadColors[index]].r,
			       ColorsBright[PadColors[index]].g, ColorsBright[PadColors[index]].b);
		  PadState[index] = 1;
		  hal_send_midi(USBMIDI, NOTEON, index, value);

		  for(int i = 0; i < 11; i++) // find our peers
		    {
		      for(int j = 0; j < 6; j++)
			{
			  if(PeerList[i][j] == index)
			    {
			      peers = i;
			      goto DONE;
			    }
			}
		    }
		DONE:
		  if(11 == peers) return; // done if no peers

		  for(int i=0; i < 6; i++) // turn 'em off
		    {
		      k = PeerList[peers][i];

		      if(k == index) // don't dim ourselves
			{
			  continue;
			}
		      else
			{
			  hal_plot_led(TYPEPAD, k, ColorsDim[PadColors[k]].r,
				       ColorsDim[PadColors[k]].g, ColorsDim[PadColors[k]].b);
			  PadState[k] = 0;
			}
		    }
		}
	    }
	}
        break;
    }
}

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2)
{
    if (port == USBMIDI)
    {
        hal_send_midi(DINMIDI, status, d1, d2);
    }

    if (port == DINMIDI)
    {
        hal_send_midi(USBMIDI, status, d1, d2);
    }
}

/*
struct SYSEXHEADER {
  u8 mType;
  u8 mCat;
  u8 devID;
  u8 subID1;
  u8 subID2;
};
*/
void app_sysex_event(u8 port, u8 * data, u16 count)
{
/*
#define SYSEXSTART 0xF0
#define UNIVNONREAL 0x7E
#define ALLCALL 0x7F
#define ID 0x06
#define IDQ 0x01
#define IDR 0x02
#define IDEXTEND 0x00
#define SYSEXTERM 0xF7

#define DEVICEID 0x00
#define MANUFACTID 0x7D
#define DEVICEFAM1 0x42
#define DEVICEFAM2 0x42
#define DEVICEMEM1 0x42
#define DEVICEMEM2 0x42
#define SOFTREV1 0x42
#define SOFTREV2 0x42
#define SOFTREV3 0x42
#define SOFTREV4 0x42

  //Midi Device ID
  //F0 7E 7F 06 01 F7
  //F0 = System Exclusive
  //7E = Universal Non-Realtime
  //7F = Universal, all nodes respond
  //06 = Identification
  //01 = ID Query
  //F7 = Terminator
  
  //Device ID Response
  //F0 7E <ID> 06 02 mm ff ff dd dd ss ss ss ss F7
  //02 = ID Response
  //ID = 00 [extended tag] xx xx
  //mm = Manufacturer sysex code
  //ff = 14-bit little-endian device family
  //mm = 14-bit little-endian device family member
  //ss = 32-bit software revision level

  hal_plot_led(TYPEPAD, 98, 64, 64, 0);

  if(12 > count)
    {
      hal_plot_led(TYPEPAD, 91, 64, 0, 0);
      return; // not enough data for a valid packet
    }
  
  struct SYSEXHEADER h;
  h.mType  = data[0];
  h.mCat   = data[1];
  h.devID  = data[2];
  h.subID1 = data[3];
  h.subID2 = data[4];
  
  if(SYSEXSTART == h.mType)
    {
      hal_plot_led(TYPEPAD, 1, 64, 64, 64);
      if(UNIVNONREAL == h.mCat)
	{
	  hal_plot_led(TYPEPAD, 2, 64, 64, 64);
	  if((ALLCALL || DEVICEID) == h.devID)
	    {
	      hal_plot_led(TYPEPAD, 3, 64, 64, 64);
	      if(ID == h.subID1 && IDQ == h.subID2) //send an ID response
		{
		  hal_plot_led(TYPEPAD, 4, 64, 64, 64);
		  u8 response[15] = {SYSEXSTART, UNIVNONREAL, DEVICEID, ID, IDR,
				     MANUFACTID, DEVICEFAM1, DEVICEFAM2,
				     DEVICEMEM1, DEVICEMEM2, SOFTREV1, SOFTREV2,
				     SOFTREV3, SOFTREV4, SYSEXTERM};
		  hal_send_sysex(port, response, 15);
		  hal_plot_led(TYPEPAD, 5, 64, 64, 64);
		}
	    }
	  else //not for us; ignore
	    {
	      hal_plot_led(TYPEPAD, 94, 64, 0, 0);
	    }
	}
      else //we don't handle sysex realtime
	{
	  hal_plot_led(TYPEPAD, 93, 64, 0, 0);
	}
    }
  else //not a SYSEX; shouldn't happen
    {
      hal_plot_led(TYPEPAD, 92, 64, 0, 0);
    } */
}
void app_aftertouch_event(u8 index, u8 value) {}
void app_cable_event(u8 type, u8 value) {}
void app_timer_event() {}

void app_init(const u16 *adc_raw)
{
    // Set all our pad colors to their defaults; initialize them all off
    for (int i=0; i < 100; i++)
      {
	hal_plot_led(TYPEPAD, i, ColorsDim[PadColors[i]].r, ColorsDim[PadColors[i]].g, ColorsDim[PadColors[i]].b);
	PadState[i] = 0;
      }
}
