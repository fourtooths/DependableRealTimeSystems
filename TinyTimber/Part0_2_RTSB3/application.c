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
	char buf[20];
	int pointer;
	int last[3];
	int lpointer;
	char nbuf[3];
	int npointer;
}
App;

App app = {
    initObject(),
    0,
    'X',
	{0},
	0,
	{0},
	-1,
	{0},
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
	
    SCI_WRITE( & sci0, "Rcv: ");
    self->buf[self->pointer] = c;
    self->pointer++;
    SCI_WRITECHAR( & sci0, c);
    SCI_WRITE( & sci0, "\nBuffer: ");
    SCI_WRITE( & sci0, self->buf);
    SCI_WRITE( & sci0, "\n");
    if ((char) c == 'e') {
        SCI_WRITE( & sci0, "\n e was found\n");
        self->buf[self->pointer] = '\0';
        self->lpointer++;
        self->last[self->lpointer % 3] = atoi(self->buf);
        self->pointer = 0;
        for (int i = 0; i < 20; i++) {
            self->buf[i] = 0;
        }
        int sum = 0;
        int median = 0;
        char mbuf[20];
        if (self->lpointer == 0) {
            sum = self->last[0];
            median = sum;
            snprintf(mbuf, 20, "%d", self->last[0]);
            SCI_WRITE( & sci0, mbuf);
            SCI_WRITE( & sci0, "\n");
        } else if (self->lpointer == 1) {
            sum = self->last[0] + self->last[1];
            median = sum / 2;
            snprintf(mbuf, 20, "%d", self->last[0]);
            SCI_WRITE( & sci0, mbuf);
            SCI_WRITE( & sci0, " - ");
            snprintf(mbuf, 20, "%d", self->last[1]);
            SCI_WRITE( & sci0, mbuf);
            SCI_WRITE( & sci0, "\n");
        } else {
            sum = self->last[0] + self->last[1] + self->last[2];
            median = 0;
            if (self->last[0] > self->last[1]) {
                median = self->last[0];
                if (self->last[2] < median){
                    median = self->last[2];
                }
            } else {
            median = self->last[1];
            if (self->last[2] < median){
                median = self->last[2];
            }
        }

        snprintf(mbuf, 20, "%d", self->last[0]);
        SCI_WRITE( & sci0, mbuf);
        SCI_WRITE( & sci0, " - ");
        snprintf(mbuf, 20, "%d", self->last[1]);
        SCI_WRITE( & sci0, mbuf);
        SCI_WRITE( & sci0, " - ");
        snprintf(mbuf, 20, "%d", self->last[2]);
        SCI_WRITE( & sci0, mbuf);
        SCI_WRITE( & sci0, "\n");
    }

    SCI_WRITE( & sci0, "\n Sum: ");
    snprintf(mbuf, 20, "%d", sum);
    SCI_WRITE( & sci0, mbuf);
    SCI_WRITE( & sci0, "\n Median: ");
    snprintf(mbuf, 20, "%d", median);
    SCI_WRITE( & sci0, mbuf);
    SCI_WRITE( & sci0, "\n Rcv: ");

} else if ((char) c == 'F') {
    for (int i = 0; i < 20; i++) {
        self->buf[i] = 0;
    }
    self->lpointer = -1;
    self->pointer = 0;
    SCI_WRITE( & sci0, "The three history has been yooted... \n");
}
}


void readerb(App * self, int c) {
    if ((char) c != 'e') {
        SCI_WRITE( & sci0, "\nChar written: ");
        SCI_WRITECHAR( & sci0, c);
        SCI_WRITE( & sci0, "\nKey: ");
        self->nbuf[self->npointer] = c;
        self->npointer++;
    }
    if ((char) c == 'e') {
        char mbuf[10];
        self->nbuf[self->npointer] = '\0';
        int key = atoi(self->nbuf);
        SCI_WRITE( & sci0, "\n");
        SCI_WRITE( & sci0, "\nCalculated periods:\n ");
        SCI_WRITE( & sci0, mbuf);
        SCI_WRITE( & sci0, "\n");
        for (int i = 0; i < 32; i++) {
            int index = brotherjohn[i] + key + 10;
            snprintf(mbuf, 10, "%d", period[index]);
            SCI_WRITE( & sci0, mbuf);
            SCI_WRITE( & sci0, " ");
        }
        SCI_WRITE( & sci0, "\nKey: ");
        self->nbuf[0] = 0;
        self->nbuf[1] = 0;
        self->nbuf[2] = 0;
        self->npointer = 0;
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

    for (int i = 0; i < 20; i++) {
        self->buf[i] = 0;
    }
}

int main() {
    INSTALL( & sci0, sci_interrupt, SCI_IRQ0);
    INSTALL( & can0, can_interrupt, CAN_IRQ0);
    TINYTIMBER( & app, startApp, 0);
    return 0;
}