#include "TinyTimber.h"

#include "sciTinyTimber.h"

#include "canTinyTimber.h"

#include <stdlib.h>

#include <stdio.h>


/* ###################### PogChamp Manual ##########################
 * To input, type one character at a time and when you have input the full number, you press 'e' to submit
 * E.g to input 20, type 2, then 0, and then e. To input -7, type -, then 7, then e.
 * To clear the three history input F, as in shift+f.
 * 
 * 
 * 
 * */

typedef struct {
  Object super;
  int count;
  char c;
  int * spointer;
  int volume;
  int flip;
  int muted;
  int mutevalue;
  int backgroundpnt;
}
App;

App app = {
  initObject(),
  0,
  'X',
  0x4000741C,
  0x7,
  0,
  0,
  0,
  0
};

typedef struct{
	Object super;
	int background_loop_range;
	} Background;
	
Background background = {
	initObject(),
	1000,
	};

void reader(App * , int);
void receiver(App * , int);

Serial sci0 = initSerial(SCI_PORT0, & app, reader);

Can can0 = initCan(CAN_PORT0, & app, receiver);

void receiver(App * self, int unused) {
  CANMsg msg;
  CAN_RECEIVE( & can0, & msg);
  SCI_WRITE( & sci0, "Can msg received: ");
  SCI_WRITE( & sci0, msg.buff);
}

int brotherjohn[] = {
  0,
  2,
  4,
  0,
  0,
  2,
  4,
  0,
  4,
  5,
  7,
  4,
  5,
  7,
  7,
  9,
  7,
  5,
  4,
  0,
  7,
  9,
  7,
  5,
  4,
  0,
  0,
  -5,
  0,
  0,
  -5,
  0
};
int period[] = {
  2024,
  1911,
  1803,
  1702,
  1607,
  1516,
  1431,
  1351,
  1275,
  1203,
  1136,
  1072,
  1012,
  955,
  901,
  851,
  803,
  758,
  715,
  675,
  637,
  601,
  568,
  536,
  506
};
void reader(App * self, int c) {
  SCI_WRITE( & sci0, "\Control - w: Volume up - s: Volume down- m: Mute/Unmute - q: background up - a: background down: ");
  // Volume up
  if ((char) c == 'w') {
    if (self -> muted == 1) {
      SCI_WRITE( & sci0, "Volume Muted");
      return;
    }
    int cvol = self -> volume;
    if (cvol != 0x14) {
      self -> volume = cvol + 1;
      SCI_WRITE( & sci0, "\nVolume increased to: ");
      SCI_WRITE( & sci0, self -> volume);
      SCI_WRITE( & sci0, "\0");
    }
  }
  // Volume down
  if ((char) c == 's') {
    if (self -> muted == 1) {
      SCI_WRITE( & sci0, "Volume Muted");
      return;
    }
    int cvol = self -> volume;
    if (cvol != 0x1) {
      self -> volume = cvol - 1;
      SCI_WRITE( & sci0, "Volume decreased to: ");
      SCI_WRITE( & sci0, self -> volume + '0');
      SCI_WRITE( & sci0, "\n");
    }
  }
  // Mute
  if ((char) c == 'm') {

    if (self -> muted == 0) {
      self -> mutevalue = self -> volume;
      self -> muted = 1;
      self -> volume = 0x0;
      SCI_WRITE( & sci0, "Volume Muted \n");
    } else {
      self -> muted = 0;
      self -> volume = self -> mutevalue;
      SCI_WRITE( & sci0, "Volume Unmuted \n");
    }
  }
  if ((char) c == 'q') {
	  Background *back = (Background*)self->backgroundpnt;
	  back->background_loop_range += 500;
  }
    if ((char) c == 'a') {
	  Background *back = (Background*)self->backgroundpnt;
	  back->background_loop_range -= 500;
  }
  SCI_WRITE( & sci0,  "\n");
}

void play(App * self, int c) {
  if (self -> flip == 0) {
    self -> flip = 1;
    *self->spointer = self -> volume;
    //		SCI_WRITE( & sci0, "On");
  } else {
    self -> flip = 0;
    *self->spointer = 0x0;
    //		SCI_WRITE( & sci0, "Off");
  }
  AFTER(USEC(500), self, play, 123);

}
void background_task(Background * self, int c) {
	for(int i = 0; i < self->background_loop_range; i++){}
	
	AFTER(USEC(1300), self, background_task, 123);
	}


void startApp(App * self, int arg) {
  CANMsg msg;
  
  
  self->backgroundpnt = &background;
  CAN_INIT( & can0);
  SCI_INIT( & sci0);
  SCI_WRITE( & sci0, "Hello, hello...\n");
  play(self, 123);
  background_task(&background, 123);
  msg.msgId = 1;
  msg.nodeId = 1;
  msg.length = 6;
  msg.buff[0] = 'H';
  msg.buff[1] = 'e';
  msg.buff[2] = 'l';
  msg.buff[3] = 'l';
  msg.buff[4] = 'o';
  msg.buff[5] = 0;
  CAN_SEND( & can0, & msg);

}

int main() {
  INSTALL( & sci0, sci_interrupt, SCI_IRQ0);
  INSTALL( & can0, can_interrupt, CAN_IRQ0);
  TINYTIMBER( & app, startApp, 0);
  return 0;
}