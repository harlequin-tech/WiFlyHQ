/*-
 * Copyright (c) 2013 Darran Hunt (darran [at] hunt dot net dot nz)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
* WiFly Bandwidth monitor for routers supporting SNMP.
* Tested with Dlink DSL-526B.
*
* Should work with any router that supports SNMP and the IF-MIB
*
*/

#include <SoftwareSerial.h>
#include "WiFlyHQ.h"

SoftwareSerial wiflySerial(8,9);
WiFly wifly;


int snmp_req(uint8_t *buf, char *oid);
int snmp_oid(uint8_t *buf, char *oid);
int snmp_get(uint8_t *buf, char *oid);
uint32_t average(uint32_t data);
void init_average(void);

#define WF_MAX_MSG_SIZE 64
char buf[WF_MAX_MSG_SIZE];

char str[32];

/* SNMP Message */
struct snmp_s { 
    uint8_t buf[WF_MAX_MSG_SIZE];
    size_t size;
} snmp_ifInOctets;

uint32_t last_request = 0;
uint32_t last_update = 0;
uint32_t rxbytes_last=0;

uint8_t msglen = sizeof(buf);

#define NTOHS(value)    (((uint16_t)(value)) << 8 | (((uint16_t)(value)) >> 8))

// Convert network order 32bit value to local endian order
#define NTOHL(data) (((uint32_t)((uint8_t *)&data)[0]) << 24 | \
		     ((uint32_t)((uint8_t *)&data)[1]) << 16 | \
		     ((uint32_t)((uint8_t *)&data)[2]) << 8 | \
		     (uint32_t)((uint8_t *)&data)[3])


const char mySSID[] = "my_ssid";
const char myPassword[] = "my_wpa_password";

uint16_t ind=0; // input buffer index

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting");
    
    wiflySerial.begin(19200);
    if (!wifly.begin(&wiflySerial, &Serial)) {
        Serial.println(F("Failed to start wifly"));
	wifly.terminal();
    }

    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	/* Setup the WiFly to connect to a wifi network */
	Serial.println(F("Joining network"));
	wifly.setSSID(mySSID);
	wifly.setPassphrase(myPassword);
	wifly.enableDHCP();

	if (wifly.join()) {
	    Serial.println(F("Joined wifi network"));
	} else {
	    Serial.println(F("Failed to join wifi network"));
	    wifly.terminal();
	}
    } else {
        Serial.println(F("Already joined network"));
    }
    Serial.println(F("WiFly ready"));

    wifly.setDeviceID(F("WiFly-Bandwidth"));
    wifly.setHostIP(F("192.168.1.1"));		// Address of router
    wifly.setHostPort(161);			// SNMP UDP port
    wifly.setUartMode(WIFLY_UART_MODE_DATA_TRIGGER);
    wifly.save();

    /* Build an SNMP request for the IF_MIB ifInOctets count for interface 2 (eth0).
     * This is the total number of bytes that have been received
     */
    snmp_ifInOctets.size = snmp_req(snmp_ifInOctets.buf, (char *)"1.3.6.1.2.1.2.2.1.10.2");

    init_average();
}

void loop()
{
    uint32_t now = millis();

    do {
	if ((now - last_request) >= 2000) {
	    /* Send a query to the router to get the latest recevied byte count */
	    wifly.write(snmp_ifInOctets.buf, snmp_ifInOctets.size);
	    last_request = now;
	}

	/* Check for UDP data */
	if (wifly.available() > 0) {
	    uint8_t ch = wifly.read();

	    if (ind < sizeof(buf)) {
		buf[ind++] = ch;
	    }

	    if (ind == 2) {
		msglen = ch + 2;
		if (msglen > sizeof(buf)) {
		    Serial.println(F("Received message too long"));
		    ind = 0;
		    continue;
		}
	    }

	    if (ind >= msglen) {
		/* Got a full message, deal with it  */

		/* The data is the last part of the message, and from
		 * the IF-MIB file you can see that ifInOctets is a Counter32,
		 * so its 4 bytes in size.
		 * http://www.net-snmp.org/docs/mibs/IF-MIB.txt
		 * The data is in network-order (big-endian) so it has 
		 * to be converted to host-order (little-endian) using
		 * the NTOHL macro..
		 */
		uint32_t *rxbytesp = (uint32_t *)&buf[msglen-4];
		uint32_t rxbytes = NTOHL(*rxbytesp);

		uint32_t count = rxbytes - rxbytes_last;
		uint32_t kbps = count / (now - last_update);

		if (rxbytes_last == 0) {
		    /* first sample, just record it and don't try and display it */
		    rxbytes_last = rxbytes;
		} else {
		    rxbytes_last = rxbytes;

		    Serial.print(" Period: ");
		    Serial.print(now - last_update, DEC);
		    Serial.print(" bytes: ");
		    Serial.print(count,DEC);
		    Serial.print(" rate: ");
		    Serial.println(kbps, DEC);

		    kbps = (kbps * 1000) / 1024;	/* Adjust to Kbps */

		    Serial.print("kbps: ");
		    Serial.print(kbps, DEC);

		    if (kbps > 2048) {
			Serial.println("Bogus kbps, skipping");
			last_update = now;
			ind = 0;		// reset ind for next buffer
			msglen = sizeof(buf);
			continue;
		    }

		    /* Use the average */
		    kbps = average(kbps);
		    Serial.print(" average: ");
		    Serial.println(kbps, DEC);

		}
		last_update = now;
		ind = 0;		// reset ind for next buffer
		msglen = sizeof(buf);

	    }
	}
    } while (0);
}

#define SAMPLE_SIZE 10
static uint32_t sample[SAMPLE_SIZE];

void init_average()
{
    int ind;
    for (ind=0; ind<SAMPLE_SIZE; ind++) {
        sample[ind] = 0;
    }
}

/* Keeping a running averge of kbps 
 * to smooth the output
 */
uint32_t average(uint32_t data)
{
    static uint8_t head = 0;
    static uint8_t tail = 0;
    static uint8_t count = 0;
    static uint32_t total = 0;	/* Running total */

    total += data;
    total -= sample[tail++];
    if (tail >= SAMPLE_SIZE) {
	tail = 0;
    }
    sample[head++] = data;
    if (head >= SAMPLE_SIZE) {
	head = 0;
    }
    if (count < SAMPLE_SIZE) {
	count++;
    }

    /* Return the running average */
    return total / count;
}

/*
 * SNMP functions
 */

/* Build an SNMP request message for the specified OID */
int snmp_req(uint8_t *buf, char *oid)
{
    int len;

    buf[0]=0x30;	/* Universal Sequence */

    buf[2]=2;		/* INTEGER */
    buf[3]=1;		/* LEN: 1 */
    buf[4]=0;		/* VAL: 0 */

    buf[5]=4;		/* OCTET STRING */
    buf[6]=6;		/* LEN: 6 */
    buf[7]='p';	
    buf[8]='u';
    buf[9]='b';
    buf[10]='l';
    buf[11]='i';
    buf[12]='c';

    len = snmp_get(&buf[13], oid);
    buf[1] = len + 11;

    return len+13;
}

// build an snmp get request in the buffer for the specifie object ID
int snmp_get(uint8_t *buf, char *oid)
{
    int len = snmp_oid(&buf[20], oid);

    /* Request */
    buf[0] = 0xa0;	/* GET REQUEST */
    buf[1] = len+20;

    /* Request ID */
    buf[2] = 0x02;	/* INTEGER */
    buf[3] = 0x04;	/* LEN: 4 */
    buf[4] = 0x00;
    buf[5] = 0xfc;
    buf[6] = 0x5a;
    buf[7] = 0x96;

    /* Error Status */
    buf[8] = 0x02;
    buf[9] = 0x01;
    buf[10] = 0x00;	/* No error */

    /* Error Index */
    buf[11] = 0x02;
    buf[12] = 0x01;
    buf[13] = 0x00;	/* No index */

    buf[14] = 0x30;	/* Universal Sequence */
    buf[15] = len+6;
    buf[16] = 0x30;	/* Universal Sequence */
    buf[17] = len+4;

    buf[18] = 6;	/* Object data type */
    buf[19] = len;

    buf[len+20] = 5;	/* NULL */
    buf[len+21] = 0;

    return len+22;
}

/* Add the OID to the buffer
* returns: buffer length
*/
int snmp_oid(uint8_t *buf, char *oid)
{
    int ind=0;
    char *endp;

    /* Compress first two items into one byte */
    buf[0] = strtol(oid, &endp, 10) * 40 + strtol(endp+1, &endp, 10);

    ind = 1;
    while (*endp == '.') {
	buf[ind] = strtol(endp+1, &endp, 10);
	ind++;
    }

    return ind;
}
