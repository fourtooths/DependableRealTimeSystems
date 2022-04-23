#include "TinyTimber.h"

#include "sciTinyTimber.h"

#include "canTinyTimber.h"

#include <stdlib.h>

#include <stdio.h>

#include <sioTinyTimber.h>

#include <sioTinyTimber.c>

// ################## DEFINITIONS ################

#define true 1
#define false 0
#define bool int
#define MUTE (uchar)'m'
#define VOLUP (uchar)'w'
#define VOLDOWN (uchar)'s'
#define TEMPO (uchar)'t'
#define KEY (uchar)'k'
#define PAUSE (uchar)'p'
#define RESTART (uchar)'r'
#define POS (uchar)1
#define NEG (uchar)0

// ##################### OBJECTS ####################

typedef struct
{
    Object super;
    int volume;
    int period;
    int stop;
    int flip;
    int *spointer;
    int volume_restore;
    int muted;

} Player;

Player player = {
    initObject(),
    0x10,
    0,
    0,
    0,
    (int *)0x4000741C,
};

typedef struct
{
    Object super;
    int tempo;
    int key;
    int note_index;
    Player *player_pointer;
    int flip;
    bool paused;
    bool restart;
    int tempo_change;
    int led_on;
} Controller;

Controller controller = {
    initObject(),
    120,
    0,
    0,
    &player,
    0,
    true,
    false,
    0,
    0,
};

typedef struct {
    uchar node_id;
    bool is_leader;
    int n_nodes;
} NodeState;

typedef struct
{
    Object super;
    int count;
    char c;
    Controller *controller_pointer;
    Player *player_pointer;
    int mode;
    int buffer_pointer;
    char buffer[20];
    Time last_time;
    int bounce;
    Timer timer;
    Time times[3];
    int time_pointer;
    NodeState node_state;
} App;

App app = {
    initObject(),
    0,
    'X',
    &controller,
    &player,
    0,
    0,
    {0},
    0,
    0,
    initTimer(),
    {0},
    0,
    {0, true, 0},
};

// ################## METHOD DEFINITIONS #############
void reader(App *, int);
void leader_reader(App *, int);
void slave_reader(App *, int);
void receiver(App *, int);
void leader_receiver(App *, int);
void slave_receiver(App *, int);
void button(App *, int);
void button_held(App *self, int count);
void button_interval(App *self, int c);
void bounce_refusal(App *self, int c);
void tap_tempo(App *self, int c);

void inc_volume(Player *self, int c);
void dec_volume(Player *self, int c);
void mute_volume(Player *self, int c);

void set_key(Controller *self, int c);
void set_tempo(Controller *self, int c);
void pause_play(Controller *self, int c);
void restart(Controller *self, int c);
void tap_set_tempo(Controller *self, int c);
void led_timeout(Controller *self, int c);

Serial sci0 = initSerial(SCI_PORT0, &app, reader);
Can can0 = initCan(CAN_PORT0, &app, receiver);
SysIO sio0 = initSysIO(SIO_PORT0, &app, button);

// ############## USER BUTTON METHODS ###############
void button(App *self, int c) {
    if (self->bounce == 1) {
        return;
    } else {
        self->bounce = 1;
        ASYNC(self, bounce_refusal, 123);
    }
    ASYNC(self, button_held, 0);
    ASYNC(self, button_interval, 123);
}

void bounce_refusal(App *self, int c) {
    if (c == 1) {
        self->bounce = 0;
    } else {
        AFTER(MSEC(100), self, bounce_refusal, 1);
    }
}

void button_held(App *self, int count) {
    char valprint[50] = {0};
    if (count == 100) {
        SCI_WRITE(&sci0, "\nEntered Hold Mode");
    }
    if (count == 200) {
        SCI_WRITE(&sci0, "\nReset Key and Tempo");
        ASYNC(self->controller_pointer, set_tempo, 120);
        ASYNC(self->controller_pointer, set_key, 0);
    }
    if (SIO_READ(&sio0) == 1) {
        if (count >= 100) {
            SCI_WRITE(&sci0, "\nHeld for ");
            int output = count / 100;
            snprintf(valprint, 50, "%d", output);
            SCI_WRITE(&sci0, valprint);
            SCI_WRITE(&sci0, " second(s)");
            snprintf(valprint, 50, "%d", count);
            SCI_WRITE(&sci0, valprint);
        }
        return;
    }

    count += 1;
    AFTER(MSEC(10), self, button_held, count);
}

void button_interval(App *self, int c) {
    char valprint[50] = {
        0};
    Time time = T_SAMPLE(&self->timer);

    SCI_WRITE(&sci0, "\nDifference: ");
    snprintf(valprint, 50, "%ld", (time - self->last_time) / 100);
    self->time_pointer++;
    self->times[self->time_pointer % 3] = (time - self->last_time);
    SCI_WRITE(&sci0, valprint);
    self->last_time = time;
    ASYNC(self, tap_tempo, 123);
}

void tap_tempo(App *self, int c) {
    char valprint[50] = {
        0};
    // MSEC(1000) used becuase of the timestamps being in base timeunit
    if (abs(self->times[0] - self->times[1]) > MSEC(100)) {
        return;
    }
    if (abs(self->times[1] - self->times[2]) > MSEC(100)) {
        return;
    }
    if (abs(self->times[0] - self->times[2]) > MSEC(100)) {
        return;
    }
    SCI_WRITE(&sci0, "\nValues: ");
    snprintf(valprint, 50, "%ld", self->times[0]);
    SCI_WRITE(&sci0, valprint);
    SCI_WRITE(&sci0, " : ");
    snprintf(valprint, 50, "%ld", self->times[1]);
    SCI_WRITE(&sci0, valprint);
    SCI_WRITE(&sci0, " : ");
    snprintf(valprint, 50, "%ld", self->times[2]);
    SCI_WRITE(&sci0, valprint);
    SCI_WRITE(&sci0, " --- \n ");

    SCI_WRITE(&sci0, "\nTap Tempo Detected ");
    Time time_average = ((self->times[0] + self->times[1] + self->times[2]) / 3) / 100;
    SCI_WRITE(&sci0, "\nTest1: ");
    snprintf(valprint, 50, "%ld", time_average);
    SCI_WRITE(&sci0, valprint);
    // self->times[0] = 0;
    // self->times[1] = 0;
    // self->times[2] = 0;
    ASYNC(self->controller_pointer, tap_set_tempo, (int)(60000 / time_average));
    SCI_WRITE(&sci0, " Set the tempo to: ");
    snprintf(valprint, 50, "%d", (int)(60000 / time_average));
    SCI_WRITE(&sci0, valprint);
    SCI_WRITE(&sci0, "\n ");
}

// ################ RECEIVER ##################
void receiver(App *self, int unused) {
    if (self->node_state.is_leader) {
        leader_receiver(self, unused);
    } else {
        slave_receiver(self, unused);
    }
    SCI_WRITE(&sci0, "\n");
}

void leader_receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    switch (msg.buff[0]) {
    /*case MUTE:
        SCI_WRITE(&sci0, "\nMute msg received: ");
        break;
    case VOLUP:
        SCI_WRITE(&sci0, "\nVolume Up msg received: ");
        break;
    case VOLDOWN:
        SCI_WRITE(&sci0, "\nVolume Down msg received: ");
        break;*/
    case TEMPO:
        SCI_WRITE(&sci0, "\nTempo msg received: ");
        break;
    case KEY:
        SCI_WRITE(&sci0, "\nKey msg received: ");
        break;
    case PAUSE:
        SCI_WRITE(&sci0, "\nPause msg received: ");
        break;
    }
}

void slave_receiver(App *self, int unused) {
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
    switch (msg.buff[0]) {
    /*case MUTE:
        SCI_WRITE(&sci0, "\nMute msg received: ");
        ASYNC(self->player_pointer, mute_volume, 123);
        break;
    case VOLUP:
        SCI_WRITE(&sci0, "\nVolume Up msg received: ");
        ASYNC(self->player_pointer, inc_volume, 123);
        break;
    case VOLDOWN:
        SCI_WRITE(&sci0, "\nVolume Down msg received: ");
        ASYNC(self->player_pointer, dec_volume, 123);
        break;*/
    case TEMPO:
        SCI_WRITE(&sci0, "\nTempo msg received: ");
        ASYNC(self->controller_pointer, set_tempo, (int)msg.buff[1]);
        break;
    case KEY:
        SCI_WRITE(&sci0, "\nKey msg received: ");
        int number = (int)msg.buff[2];
        if (msg.buff[1] == NEG) {
            number *= -1;
        }
        ASYNC(self->controller_pointer, set_key, number);
        break;
    case PAUSE:
        SCI_WRITE(&sci0, "\nPause msg received: ");
        ASYNC(self->controller_pointer, pause_play, 123);
        break;
    }
}

// ################ DATA LISTS ################
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
    0};
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
    506};

typedef enum {
    A,
    B,
    C
} Beat;

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
    B};

// ################ HELPER METHODS #####################

void set_period(Player *self, int c) {
    self->period = c;
}

void send_can_msg(uchar command, uchar argone, uchar argtwo) {
    CANMsg msg;
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 3;
    msg.buff[0] = command;
    msg.buff[1] = argone;
    msg.buff[2] = argtwo;
    CAN_SEND(&can0, &msg);
}

void inc_volume(Player *self, int c) {
    if (self->muted) {
        SCI_WRITE(&sci0, "\nVolume muted");
        return;
    }
    char valprint[10] = {
        0};
    if (self->volume == 20) {
        SCI_WRITE(&sci0, "\nVolume already capped at 20");
    } else {
        self->volume++;
        SCI_WRITE(&sci0, "\nVolume at:");
        snprintf(valprint, 10, "%d", self->volume);
        SCI_WRITE(&sci0, valprint);
    }
}

void dec_volume(Player *self, int c) {
    if (self->muted) {
        SCI_WRITE(&sci0, "\nVolume muted");
        return;
    }
    char valprint[10] = {
        0};
    if (self->volume == 0) {
        SCI_WRITE(&sci0, "\nVolume already muted at 0");
    } else {
        self->volume--;
        SCI_WRITE(&sci0, "\nVolume at:");
        snprintf(valprint, 10, "%d", self->volume);
        SCI_WRITE(&sci0, valprint);
    }
}

void mute_volume(Player *self, int mute_flag) {
    if (self->muted) {
        SCI_WRITE(&sci0, "\nUnmuted");
        self->volume = self->volume_restore;
        self->muted = 0;
        if (mute_flag == 1) {
            SIO_WRITE(&sio0, 0); // turn LED on if slave
        }
    } else {
        SCI_WRITE(&sci0, "\nMuted");
        self->volume_restore = self->volume;
        self->volume = 0;
        self->muted = 1;
        if (mute_flag == 1) {
            SIO_WRITE(&sio0, 1); // turn LED off if slave
        }
    }
}

void stop_play(Player *self, int c) {
    self->stop = 1;
}

void set_key(Controller *self, int c) {

    self->key = c;
}

void set_tempo(Controller *self, int c) {
    self->tempo = c;
    self->tempo_change = 1;
}

void calc_next_note(Controller *self, int c);

void pause_play(Controller *self, int c) {
    if (self->paused) {
        SCI_WRITE(&sci0, "\nUnpaused");
        self->paused = false;
        self->led_on = 1;
        ASYNC(self, calc_next_note, 1);
        ASYNC(self, led_timeout, 0);
    } else {
        SCI_WRITE(&sci0, "\nPaused");
        self->paused = true;
        ASYNC(self->player_pointer, stop_play, 123);
    }
}

void restart(Controller *self, int c) {
    SCI_WRITE(&sci0, "\nRestarting");
    ASYNC(self->player_pointer, stop_play, 123);

    SIO_WRITE(&sio0, 1);
    self->note_index = 0;
    self->flip = 0;

    self->led_on = 1;
    ASYNC(self, calc_next_note, 1);
    ASYNC(self, led_timeout, 0);
}

void tap_set_tempo(Controller *self, int c) {
    if (c <= 300 && c >= 30) {
        ASYNC(self, set_tempo, c);
    }
}

// #################### READER ####################

void reader(App *self, int c) {
    if (self->node_state.is_leader) {
        leader_reader(self, c);
    } else {
        slave_reader(self, c);
    }
    SCI_WRITE(&sci0, "\n");
}

void leader_reader(App *self, int c) {
    if (self->mode == 0) {
        SCI_WRITE(&sci0, "\nControl - Volume: w/s/m - Background: q/a/z: - Tempo Mode: t - Key Mode: k - Play/Pause: p: - Toggle Slave/Leader: l");
        // Volume up
        if ((char)c == 'w') {
            SCI_WRITE(&sci0, "\nVolume Up key read: ");
            ASYNC(self->player_pointer, inc_volume, 123);
        }
        // Volume down
        if ((char)c == 's') {
            SCI_WRITE(&sci0, "\nVolume Down key read: ");
            ASYNC(self->player_pointer, dec_volume, 123);
        }
        // Mute
        if ((char)c == 'm') {
            SCI_WRITE(&sci0, "\nMute key read: ");
            ASYNC(self->player_pointer, mute_volume, 0);
        }
        // Tempo
        if ((char)c == 't') {
            self->mode = 1;
            SCI_WRITE(&sci0, "\nInput mode Tempo");
        }
        // Key
        if ((char)c == 'k') {
            self->mode = 2;
            SCI_WRITE(&sci0, "\nInput mode Key");
        }
        // Pause/Play
        if ((char)c == 'p') {
            ASYNC(self->controller_pointer, pause_play, 123);
            send_can_msg(PAUSE, 0, 0);
        }

        // Restart
        if ((char)c == 'r') {
            ASYNC(self->controller_pointer, restart, 123);
            send_can_msg(RESTART, 0, 0);
        }

        // leader/slave
        if ((char)c == 'l') {
            SCI_WRITE(&sci0, "\nMode set to Slave");
            self->node_state.is_leader = false;
        }
    }
    /// Tempo/Key mode
    else if (self->mode == 1 || self->mode == 2) {
        if (self->mode == 1) {
            SCI_WRITE(&sci0, "\nTempo mode");
        } else {
            SCI_WRITE(&sci0, "\nKey mode");
        }
        if ((char)c == 'e') {
            if (self->mode == 1) {
                int tempo_change = atoi(self->buffer);
                if (tempo_change < 60 || tempo_change > 240) {
                    SCI_WRITE(&sci0, "\nTempo range is 60 - 240. Input a valid tempo.");
                    for (int i = 0; i < 20; i++) {
                        self->buffer_pointer = 0;
                        self->buffer[i] = 0;
                        tempo_change = 0;
                    }
                    return;
                }
                SCI_WRITE(&sci0, "\nTempo set to: ");
                SCI_WRITE(&sci0, self->buffer);
                SYNC(self->controller_pointer, set_tempo, tempo_change);
                send_can_msg(TEMPO, (uchar)tempo_change, 0);
            } else {
                int key_change = atoi(self->buffer);
                if (key_change < -5 || key_change > 5) {
                    SCI_WRITE(&sci0, "\nKey range is -5 - 5. Input a valid key.");
                    self->buffer_pointer = 0;
                    for (int i = 0; i < 20; i++) {
                        self->buffer[i] = 0;
                    }
                    return;
                }
                SCI_WRITE(&sci0, "\nKey set to: ");
                SCI_WRITE(&sci0, self->buffer);
                SYNC(self->controller_pointer, set_key, key_change);
                int sign = NEG;
                if (c >= 0) {
                    sign = POS;
                }
                send_can_msg(KEY, sign, (uchar)key_change);
            }
            for (int i = 0; i < 20; i++) {
                self->buffer[i] = 0;
            }
            self->buffer_pointer = 0;
            self->mode = 0;
            SCI_WRITE(&sci0, "\nControl - Volume: w/s/m - Background: q/a/z: - Tempo Mode: t - Key Mode: k - Play/Pause: p: - Toggle Slave/Leader: l");
        } else {
            self->buffer[self->buffer_pointer] = c;
            SCI_WRITE(&sci0, "\nInputted char: ");
            sci_writechar(&sci0, c);
            self->buffer_pointer++;
        }
    }
}

void slave_reader(App *self, int c) {
    if (self->mode == 0) {
        SCI_WRITE(&sci0, "\nControl - Volume: w/s/m - Background: q/a/z: - Tempo Mode: t - Key Mode: k - Play/Pause: p: - Toggle Slave/Leader: l");
        // Volume up
        if ((char)c == 'w') {
            SCI_WRITE(&sci0, "\nVolume Up key read: ");
            ASYNC(self->player_pointer, inc_volume, 123);
        }
        // Volume down
        if ((char)c == 's') {
            SCI_WRITE(&sci0, "\nVolume Down key read: ");
            ASYNC(self->player_pointer, dec_volume, 123);
        }
        // Mute
        if ((char)c == 'm') {
            SCI_WRITE(&sci0, "\nMute key read: ");
            ASYNC(self->player_pointer, mute_volume, 1);
        }
    }
}

//##################### PLAYER ####################

void play(Player *self, int c) {
    if (self->stop) {
        self->stop = 0;
        return;
    }

    if (self->flip == 0) {
        self->flip = 1;
        *self->spointer = self->volume;
    } else {
        self->flip = 0;
        *self->spointer = 0x0;
    }
    AFTER(USEC(self->period), self, play, 123);
}

//##################### CONTROLLER ################

void led_timeout(Controller *self, int c) {
    self->led_on = 1;
    if (self->tempo_change) {
        self->tempo_change = 0;
        self->led_on = 0;
        return;
    }
    float play_length = (1.0 / ((float)self->tempo / 120.0)) * 500.0;
    if (c) {
        SIO_WRITE(&sio0, c);
        AFTER(MSEC((int)play_length) / 2, self, led_timeout, 0);
    } else {
        SIO_WRITE(&sio0, c);
        AFTER(MSEC((int)play_length) / 2, self, led_timeout, 1);
    }
}

void calc_next_note(Controller *self, int c) {
    if (self->flip == 1) {
        // If there is a silence at the end of the note
        ASYNC(self->player_pointer, stop_play, 123);
        self->flip = 0;
        AFTER(MSEC(50), self, calc_next_note, 123);
        self->note_index = (self->note_index + 1) % 32;
    } else {
        // Turn on the LED if led is off when it should be on
        if (!self->led_on) {
            ASYNC(self, led_timeout, 0);
        }
        // Set the period
        int next_period = periods[notes[self->note_index] + 10 + self->key];
        SYNC(self->player_pointer, set_period, next_period);
        ASYNC(self->player_pointer, play, 123);
        self->flip = 1;
        float play_length = (1.0 / ((float)self->tempo / 120.0)) * 500.0;
        if (lengths[self->note_index] == B) {
            play_length = play_length * 2.0;
        }
        if (lengths[self->note_index] == C) {
            play_length = play_length / 2.0;
        }
        if (self->paused) {
            SIO_WRITE(&sio0, 1);
            self->note_index = 0;
            self->flip = 0;
            return;
        }

        AFTER(MSEC((int)play_length - 50), self, calc_next_note, 123);
    }
}

void startApp(App *self, int arg) {
    CANMsg msg;

    CAN_INIT(&can0);
    SCI_INIT(&sci0);
    SIO_INIT(&sio0);
    SIO_TOGGLE(&sio0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
    msg.msgId = 1;
    msg.nodeId = 1;
    msg.length = 6;
    msg.buff[0] = 'H';
    msg.buff[1] = 'e';
    msg.buff[2] = 'l';
    msg.buff[3] = 'l';
    msg.buff[4] = 'o';
    msg.buff[5] = 0;
    CAN_SEND(&can0, &msg);
    SCI_WRITE(&sci0, "\nControl - Volume: w/s/m - Background: q/a/z: - Tempo Mode: t - Key Mode: k - Play/Pause: p: - Toggle Slave/Leader: l");
}

int main() {
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio0, sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 123);

    return 0;
}