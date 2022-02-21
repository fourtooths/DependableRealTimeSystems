#include "TinyTimber.h"

#include "sciTinyTimber.h"

#include "canTinyTimber.h"

#include <stdlib.h>

#include <stdio.h>

/* ###################### PogChamp Manual ##########################
 * w - Volume up
 * s - Volume down
 * q - Background up
 * a - Background down
 * z - Toggle background
 * m - Toggle mute
 * */

typedef struct {
    Object super;
    int count;
    char c;
    int * spointer;
    int volume;
    int period;
    int flip;
    int muted;
    int mutevalue;
    int backgroundpnt;
    int deadline;
    Timer timer;
    int32_t max;
    int32_t sum;
    int countt;
}
App;

App app = {
    initObject(),
    0,
    'X',
    (int * ) 0x4000741C,
    0x5,
    500,
    0,
    0,
    0,
    0,
    USEC(100),
    initTimer(),
    0,
    0,
    0,
};

typedef struct {
    Object super;
    int background_loop_range;
    int deadline;
    Timer timer;
    int32_t max;
    int32_t sum;
    int count;
}
Background;

Background background = {
    initObject(),
    13500,
    USEC(1300),
    initTimer(),
    0,
    0,
    0,

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
    char valprint[10] = {
        0
    };
    SCI_WRITE( & sci0, "\nControl - Volume: w/s/m - Background: q/a/z: ");
    // Volume up
    if ((char) c == 'w') {
        if (self -> muted == 1) {
            SCI_WRITE( & sci0, "Volume Muted");
            return;
        }
        int cvol = self -> volume;
        if (cvol != 0x14) {
            self -> volume = cvol + 1;
            snprintf(valprint, 10, "%d", self -> volume);
            SCI_WRITE( & sci0, "\nVolume increased to: ");
            SCI_WRITE( & sci0, valprint);
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
            snprintf(valprint, 10, "%d", self -> volume);
            SCI_WRITE( & sci0, "Volume decreased to: ");
            SCI_WRITE( & sci0, valprint);
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
        Background * back = (Background * ) self -> backgroundpnt;
        int background_lr = SYNC(back, read_background_lr, NULL);
        background_lr += 500;
        SYNC(back, set_background_lr, background_lr);
        snprintf(valprint, 10, "%d", background_lr);
        SCI_WRITE( & sci0, "\nBackground increased to: ");
        SCI_WRITE( & sci0, valprint);
        SCI_WRITE( & sci0, "\0");
    }
    if ((char) c == 'a') {
        Background * back = (Background * ) self -> backgroundpnt;
         int background_lr = SYNC(back, read_background_lr, NULL);
        background_lr -= 500;
        SYNC(back, set_background_lr, background_lr);
        snprintf(valprint, 10, "%d", background_lr);
        SCI_WRITE( & sci0, "\nBackground decreased to: ");
        SCI_WRITE( & sci0, valprint);
        SCI_WRITE( & sci0, "\0");
    }
    if ((char) c == 'z') {
        Background * back = (Background * ) self -> backgroundpnt;
        if (self -> deadline > 0) {
            self -> deadline = USEC(0);
            SYNC(back, set_background_deadline, 0);
            } else {
            self -> deadline = USEC(100);
            SYNC(back, set_background_deadline, 1300);
        }
    }

    SCI_WRITE( & sci0, "\n");
}

void set_background_lr(Background * self, int lr){
    self->background_loop_range = lr;
}

int read_background_lr(Background * self, int lr){
    return self->background_loop_range;
}

int set_background_deadline(Background * self, int dl){
    self->deadline = dl;
}

void play(App * self, int c) {
    /*char valprint[10] = {
        0
    };
    Time time1 = CURRENT_OFFSET();
    
	for (int i = 0; i < 100; i++) {
    */
    if (self -> flip == 0) {
            self -> flip = 1;
            * self -> spointer = self -> volume;
        } else {
            self -> flip = 0;
            * self -> spointer = 0x0;
        }
    //}
   /* Time time2 = CURRENT_OFFSET();
    Time value = time2 - time1;
    if (self -> countt < 500) {
        self -> countt = self -> countt + 1;
        self -> sum = self -> sum + value;
        if (value > self -> max) {
            self -> max = value;
        }
    } else if (self -> countt == 500) {
        self -> max = USEC_OF(self -> max);

        self -> countt += 1;
        int average = self -> sum / 500;
        average = USEC_OF(average);
        SCI_WRITE( & sci0, "\nMax was found at: ");
        snprintf(valprint, 10, "%d", (int) self -> max);
        SCI_WRITE( & sci0, valprint);
        SCI_WRITE( & sci0, "\nAverage was found at: ");
        snprintf(valprint, 10, "%d", average);
        SCI_WRITE( & sci0, valprint);
    }*/
    SEND(USEC(self -> period), self -> deadline, self, play, 123);
}
void background_task(Background * self, int c) {
    /*char valprint[10] = {
        0
    };
    //Time time1 = CURRENT_OFFSET();
    T_RESET(&self->timer);
    */
	for (int i = 0; i < self -> background_loop_range; i++) {}
    /*Time time2 = CURRENT_OFFSET();
    //Time value = T_SAMPLE( & self -> timer);
    Time value = time2 - time1;
    if (self -> count < 500) {
        self -> count = self -> count + 1;
        self -> sum = self -> sum + value;
        if (value > self -> max) {
            self -> max = value;
        }
    } else if (self -> count == 500) {
        self -> max = USEC_OF((self -> max));
        self -> count += 1;
        int average = self -> sum / 500;
        average = USEC_OF((average));
        SCI_WRITE( & sci0, "\nMax was found at: ");
        snprintf(valprint, 10, "%d", (int) self -> max);
        SCI_WRITE( & sci0, valprint);

        SCI_WRITE( & sci0, "\nAverage was found at: ");
        snprintf(valprint, 10, "%d", average);
        SCI_WRITE( & sci0, valprint);

    }*/
    SEND(USEC(1300), self -> deadline, self, background_task, 123);
}

void startApp(App * self, int arg) {
    CANMsg msg;

    self -> backgroundpnt = (int) & background;
    CAN_INIT( & can0);
    SCI_INIT( & sci0);
    SCI_WRITE( & sci0, "Hello, hello...\n");
    play(self, 123);
    SYNC(&background, background_task, NULL);
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