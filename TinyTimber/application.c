#include "TinyTimber.h"

#include "sciTinyTimber.h"

#include "canTinyTimber.h"

#include <stdlib.h>

#include <stdio.h>

#define true 1
#define false 0
#define bool int
#define MUTE (uchar)'m'
#define VOLUP (uchar)'w'
#define VOLDOWN (uchar)'s'
#define TEMPO (uchar)'t'
#define KEY (uchar)'k'
#define PAUSE (uchar)'p'
#define POS (uchar)1
#define NEG (uchar)0

typedef struct {
	Object super;
	int volume;
	int period;
	int stop;
	int flip;
	int * spointer;
	int volume_restore;
	int muted;
	
} Player;

Player player = {
	initObject(),
	0x10,
	0,
	0,
	0,
	(int * ) 0x4000741C
};

typedef struct {
	Object super;
	int tempo;
	int key;
	int note_index;
	Player * player_pointer;
	int flip;
	bool paused;
} Controller;

Controller controller = {
	initObject(),
	120,
	0,
	0,
	&player,
	0,
	true,

};

typedef struct {
    Object super;
    int count;
    char c;
    Controller *controller_pointer;
	Player *player_pointer;
	int mode;
	int buffer_pointer;
	char buffer[20];
}
App;

App app = {
    initObject(),
    0,
    'X',
    &controller,
    &player,
	0,
	0,
	{0},
};

bool leader = true;

void reader(App * , int);
void receiver(App * , int);

Serial sci0 = initSerial(SCI_PORT0, & app, reader);

Can can0 = initCan(CAN_PORT0, & app, receiver);

void inc_volume(Player * self, int c);
void dec_volume(Player * self, int c);
void mute_volume(Player * self, int c);
void set_key(Controller * self, int c);
void set_tempo(Controller * self, int c);
void pause_play(Controller * self, int c);

void receiver(App * self, int unused) {
    CANMsg msg;
    CAN_RECEIVE( & can0, & msg);
    SCI_WRITE( & sci0, "\nCan msg received: ");
    SCI_WRITE( & sci0, msg.buff);
	
	switch(msg.buff[0]){
		case MUTE :
			SCI_WRITE( & sci0, "\nMute msg received: ");
			if(!leader){
				ASYNC(self->player_pointer, mute_volume, 123);
			}
			break;
		case VOLUP :
			SCI_WRITE( & sci0, "\nVolume Up msg received: ");
			if(!leader){
				ASYNC(self->player_pointer, inc_volume, 123);
			}
			break;
		case VOLDOWN :
			SCI_WRITE( & sci0, "\nVolume Down received: ");
			if(!leader){
				ASYNC(self->player_pointer, dec_volume, 123);
			}
			break;
		case TEMPO :
			SCI_WRITE( & sci0, "\nTempo msg received: ");
			if(!leader){
				ASYNC(self->controller_pointer, set_tempo, (int)msg.buff[1]);
			}
			break;
		case KEY :
			SCI_WRITE( & sci0, "\nKey msg received: ");
			if(!leader){
				int number = (int)msg.buff[2];
				if(msg.buff[1] == NEG){
					number *= -1;
				}
				ASYNC(self->player_pointer, set_tempo, number);
			}
			break;
		case PAUSE :
			SCI_WRITE( & sci0, "\nPause msg received: ");
			if(!leader){
				ASYNC(self->player_pointer, pause_play, 123);
			}
			break;
	}
}

int notes[] = {
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
int periods[] = {
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

typedef enum {
	A,
	B,
	C
}Beat;

Beat lengths[] = {
	A,
	A,
	A,
	A,
	A,
	A,
	A,
	A,
	A,
	A,
	B,
	A,
	A,
	B,
	C,
	C,
	C,
	C,
	A,
	A,
	C,
	C,
	C,
	C,
	A,
	A,
	A,
	A,
	B,
	A,
	A,
	B
};

void set_period(Player * self, int c) {
	self->period = c;
}

void inc_volume(Player * self, int c) {
	if(leader){
		if(self->muted){
			SCI_WRITE( & sci0, "\nVolume muted");
			return;
		}
		char valprint[10] = {
			0
		};
		if(self->volume == 20){
			SCI_WRITE( & sci0, "\nVolume already capped at 20");
		}else{
			self->volume++;
			SCI_WRITE( & sci0, "\nVolume at:");
			snprintf(valprint, 10, "%d", self -> volume);
			SCI_WRITE( & sci0, valprint);
		}
	}
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 1;
    msg.buff[0] = VOLUP;
    CAN_SEND( & can0, & msg);
}

void dec_volume(Player * self, int c) {
	if(leader) {
		if(self->muted){
			SCI_WRITE( & sci0, "\nVolume muted");
			return;
		}
		char valprint[10] = {
			0
		};
		if(self->volume == 0){
			SCI_WRITE( & sci0, "\nVolume already muted at 0");
		}else{
			self->volume--;
			SCI_WRITE( & sci0, "\nVolume at:");
			snprintf(valprint, 10, "%d", self -> volume);
			SCI_WRITE( & sci0, valprint);
		}
	}
	
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 1;
    msg.buff[0] = VOLDOWN;
    CAN_SEND( & can0, & msg);
	
}

void mute_volume(Player * self, int c){
	if(leader) {
		if(self->muted){
			SCI_WRITE( & sci0, "\nUnmuted");
			self->volume = self->volume_restore;
			self->muted = 0;
		}else{
			SCI_WRITE( & sci0, "\nMuted");
			self->volume_restore = self->volume;
			self->volume = 0;
			self->muted = 1;
		}
	}
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 1;
    msg.buff[0] = MUTE;
    CAN_SEND( & can0, & msg);
}

void stop_play(Player * self, int c) {
	self->stop = 1;
}

void set_key(Controller * self, int c) {
	if(leader) {
		self->key = c;
	}
	int sign = NEG;
	if(c >= 0) {
		sign = POS;
	}
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 3;
    msg.buff[0] = KEY;
	msg.buff[1] = sign;
	msg.buff[2] = (uchar) abs(c);
    CAN_SEND( & can0, & msg);
}

void set_tempo(Controller * self, int c) {
	if(leader) {
		self->tempo = c;
	}
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 2;
    msg.buff[0] = TEMPO;
	msg.buff[1] = (uchar) c;
    CAN_SEND( & can0, & msg);
}

void calc_next_note(Controller * self, int c);

void pause_play(Controller * self, int c) {
	if(leader) {
		if(self->paused){
			SCI_WRITE( & sci0, "\nUnpaused");
			self->paused = false;
			ASYNC(self, calc_next_note, 123);
		}else{
			SCI_WRITE( & sci0, "\nPaused");
			self->paused = true;
			ASYNC(self->player_pointer, stop_play, 123);
		}
	}
	CANMsg msg;
	msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 1;
    msg.buff[0] = PAUSE;
    CAN_SEND( & can0, & msg);
}

void reader(App * self, int c) {
    //char valprint[10] = {
    //    0
    //};
	if(self -> mode == 0) {
		SCI_WRITE( & sci0, "\nControl - Volume: w/s/m - Background: q/a/z: - Tempo Mode: t - Key Mode: k - Play/Pause: p: - Toggle Slave/Leader: l");
		// Volume up
		if ((char) c == 'w') {
			SYNC(self->player_pointer, inc_volume, 123);
		}
		// Volume down
		if ((char) c == 's') {
			SYNC(self->player_pointer, dec_volume, 123);
		}
		// Mute
		if ((char) c == 'm') {
			SYNC(self->player_pointer, mute_volume, 123);
		}
		// Tempo
		if ((char) c == 't') {
			self->mode = 1;
			SCI_WRITE( & sci0, "\nInput mode Tempo");
		}
		// Key
		if ((char) c == 'k') {
			self->mode = 2;
			SCI_WRITE( & sci0, "\nInput mode Key");
		}
		// Pause/Play
		if ((char) c == 'p') {
			ASYNC(self->controller_pointer, pause_play, 123);
		}
		if ((char) c == 'l') {
			if(leader){
				SCI_WRITE( & sci0, "\nMode set to Slave");
				leader = false;
			}else{
				SCI_WRITE( & sci0, "\nMode set to Leader");
				leader = true;
			}
		}
	}
	/// Tempo/Key mode
	else if( self -> mode == 1 || self -> mode == 2){
		if(self -> mode == 1){
			SCI_WRITE( & sci0, "\nTempo mode");
		}else {
			SCI_WRITE( & sci0, "\nKey mode");
		}
		if ((char) c == 'e') {
			if(self -> mode == 1){
				SCI_WRITE( & sci0, "\nTempo set to: ");
				SCI_WRITE( & sci0, self->buffer);
				int tempo_change = atoi(self->buffer);
				if(tempo_change < 60 || tempo_change > 240){
					SCI_WRITE( & sci0, "\nTempo range is 60 - 240. Input a valid tempo.");
					return;
				}
				SYNC(self->controller_pointer, set_tempo, tempo_change);
			}else {
				SCI_WRITE( & sci0, "\nKey set to: ");
				SCI_WRITE( & sci0, self->buffer);
				int key_change = atoi(self->buffer);
				if(key_change < -5 || key_change > 5){
					SCI_WRITE( & sci0, "\nKey range is -5 - 5. Input a valid key.");
					return;
				}
				SYNC(self->controller_pointer, set_key, key_change);
			}
			for(int i = 0; i <20; i++){
				self->buffer[i] = 0;
			}
			self->buffer_pointer = 0;
			self->mode = 0;
			SCI_WRITE( & sci0, "\nControl - Volume: w/s/m - Background: q/a/z: ");
		}else {
			self->buffer[self->buffer_pointer] = c;
			SCI_WRITE( & sci0, "\nInputted char: ");
			sci_writechar(&sci0, c);
			self->buffer_pointer++;
		}
	}

    SCI_WRITE( & sci0, "\n");
}

//##################### PLAYER ####################

void play(Player * self, int c) {
	if(self->stop){
		self->stop = 0;
		return;
	}
	
    if (self -> flip == 0) {
            self -> flip = 1;
            * self -> spointer = self -> volume;
        } else {
            self -> flip = 0;
            * self -> spointer = 0x0;
        }
    AFTER(USEC(self -> period), self, play, 123);
}

//##################### CONTROLLER ####################

void calc_next_note(Controller * self, int c){
	if(self->flip == 1){
		// If there is a silence at the end of the note
		ASYNC(self->player_pointer, stop_play, 123);
		self->flip = 0;
		// TODO: Change after call to accomodate for different tempos
		AFTER(MSEC(50), self, calc_next_note, 123);
		self->note_index = (self->note_index + 1) % 32;
	}else{
		// Set the period
		int next_period = periods[notes[self->note_index]+10+self->key];
		SYNC(self->player_pointer, set_period, next_period);
		ASYNC(self->player_pointer, play, 123);
		self->flip = 1;
		float play_length = (1.0/((float)self->tempo/120.0)) * 450;
		if(lengths[self->note_index] == B){
			play_length = play_length*2;
		}
		if(lengths[self->note_index] == C){
			play_length = play_length/2;
		}
		if(self->paused){
			self->note_index = 0;
			self->flip = 0;
			return;
		}
		AFTER(MSEC((int)play_length), self, calc_next_note, 123);
	}
}



void startApp(App * self, int arg) {
    CANMsg msg;

   
    CAN_INIT( & can0);
    SCI_INIT( & sci0);
    SCI_WRITE( & sci0, "Hello, hello...\n");
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