/*-
 * Copyright (c) 2012 Darran Hunt (darran [at] hunt dot net dot nz)
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
 * @file WiFly RN-XV Library
 */

#include "WiFlyHQ.h"

/* For free memory check */
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

#undef DEBUG

#ifdef DEBUG
#define DPRINT(item) \
    if (debug && debugOn) { \
	debug->print(item); \
    }
#else
#define DPRINT(item)
#endif

#define WIFLY_STATUS_TCP_MASK 		0x000F
#define WIFLY_STATUS_TCP_OFFSET		0
#define WIFLY_STATUS_ASSOC_MASK 	0x0001
#define WIFLY_STATUS_ASSOC_OFFSET	4
#define WIFLY_STATUS_AUTHEN_MASK 	0x0001
#define WIFLY_STATUS_AUTHEN_OFFSET	5
#define WIFLY_STATUS_DNS_SERVER_MASK	0x0001
#define WIFLY_STATUS_DNS_SERVER_OFFSET	6
#define WIFLY_STATUS_DNS_FOUND_MASK	0x0001
#define WIFLY_STATUS_DNS_FOUND_OFFSET	7
#define WIFLY_STATUS_CHAN_MASK 		0x000F
#define WIFLY_STATUS_CHAN_OFFSET	9

#define WIFLY_TCP_IDLE		0
#define WIFLY_TCP_CONNECTED	1
#define WIFLY_TCP_NOIP		3
#define WIFLY_TCP_CONNECTING	4

/* Work around a bug with PROGMEM and PSTR where the compiler always
 * generates warnings.
 */
#undef PROGMEM 
#define PROGMEM __attribute__(( section(".progmem.data") )) 
#undef PSTR 
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];})) 

/* Request and response strings in PROGMEM */
const prog_char req_GetIP[] PROGMEM = "get ip\r";
const prog_char resp_IP[] PROGMEM = "IP=";
const prog_char resp_NM[] PROGMEM = "NM=";
const prog_char resp_GW[] PROGMEM = "GW=";
const prog_char resp_Host[] PROGMEM = "HOST=";
const prog_char resp_DHCP[] PROGMEM = "DHCP=";
const prog_char req_GetMAC[] PROGMEM = "get mac\r";
const prog_char resp_MAC[] PROGMEM = "Mac Addr=";
const prog_char req_GetWLAN[] PROGMEM = "get wlan\r";
const prog_char resp_SSID[] PROGMEM = "SSID=";
const prog_char resp_Chan[] PROGMEM = "Chan=";
const prog_char req_GetOpt[] PROGMEM = "get opt\r";
const prog_char resp_DeviceID[] PROGMEM = "DeviceId=";
const prog_char req_GetUart[] PROGMEM = "get u\r";
const prog_char resp_Baud[] PROGMEM = "Baudrate=";
const prog_char req_GetTime[] PROGMEM = "get time\r";
const prog_char resp_Zone[] PROGMEM = "Zone=";
const prog_char req_ShowTime[] PROGMEM = "show time\r";
const prog_char resp_Uptime[] PROGMEM = "UpTime=";
const prog_char resp_Time[] PROGMEM = "Time=";
const prog_char req_GetDNS[] PROGMEM = "get dns\r";
const prog_char resp_DNSAddr[] PROGMEM = "Address=";
const prog_char req_ShowTimeT[] PROGMEM = "show t t\r";
const prog_char resp_RTC[] PROGMEM = "RTC=";
const prog_char resp_Mode[] PROGMEM = "Mode=";
const prog_char req_GetComm[] PROGMEM = "get comm\r";
const prog_char resp_FlushTimeout[] PROGMEM = "FlushTimer=";
const prog_char resp_FlushChar[] PROGMEM = "MatchChar=";
const prog_char resp_FlushSize[] PROGMEM = "FlushSize=";
const prog_char req_GetRSSI[] PROGMEM = "show rssi\r";
const prog_char resp_RSSI[] PROGMEM = "RSSI=(-";
const prog_char resp_Flags[] PROGMEM = "FLAGS=0x";

/* Request and response for specific info */
static struct {
    const prog_char *req;
    const prog_char *resp;
} requests[] = {
    { req_GetIP,	resp_IP },	 /* 0 */
    { req_GetIP,	resp_NM },	 /* 1 */
    { req_GetIP,	resp_GW },	 /* 2 */
    { req_GetMAC,	resp_MAC },	 /* 3 */
    { req_GetWLAN,	resp_SSID },	 /* 4 */
    { req_GetOpt,	resp_DeviceID }, /* 5 */
    { req_GetUart,	resp_Baud }, 	 /* 6 */
    { req_ShowTime,	resp_Time }, 	 /* 7 */
    { req_ShowTime,	resp_Uptime }, 	 /* 8 */
    { req_GetTime,	resp_Zone }, 	 /* 9 */
    { req_GetDNS,	resp_DNSAddr },	 /* 10 */
    { req_ShowTimeT,	resp_RTC },	 /* 11 */
    { req_GetIP,	resp_DHCP },	 /* 12 */
    { req_GetUart,	resp_Mode },	 /* 13 */
    { req_GetComm,	resp_FlushTimeout }, /* 14 */
    { req_GetComm,	resp_FlushChar }, /* 15 */
    { req_GetComm,	resp_FlushSize }, /* 16 */
    { req_GetRSSI,	resp_RSSI },	 /* 17 */
    { req_GetIP,	resp_Flags },	 /* 18 */
    { req_GetIP,	resp_Host },	 /* 19 */
};

/* Request indices, must match table above */
typedef enum {
    WIFLY_GET_IP	= 0,
    WIFLY_GET_NETMASK	= 1,
    WIFLY_GET_GATEWAY	= 2,
    WIFLY_GET_MAC	= 3,
    WIFLY_GET_SSID	= 4,
    WIFLY_GET_DEVICEID	= 5,
    WIFLY_GET_BAUD	= 6,
    WIFLY_GET_TIME	= 7,
    WIFLY_GET_UPTIME	= 8,
    WIFLY_GET_ZONE	= 9,
    WIFLY_GET_DNS	= 10,
    WIFLY_GET_RTC	= 11,
    WIFLY_GET_DHCP	= 12,
    WIFLY_GET_UART_MODE	= 13,
    WIFLY_GET_FLUSHTIMEOUT = 14,
    WIFLY_GET_FLUSHCHAR	= 15,
    WIFLY_GET_FLUSHSIZE	= 16,
    WIFLY_GET_RSSI	= 17,
    WIFLY_GET_IP_FLAGS	= 18,
    WIFLY_GET_HOST	= 19,
} e_wifly_requests;

/** Convert a unsigned int to a string */
static int simple_utoa(uint32_t val, uint8_t base, char *buf, int size)
{
    char tmpbuf[16];
    int ind=0;
    uint32_t nval;
    int fsize=0;

    if (base == 10) {
	do {
	    nval = val / 10;
	    tmpbuf[ind++] = '0' + val - (nval * 10);
	    val = nval;
	} while (val);
    } else {
	do {
	    nval = val & 0x0F;
	    tmpbuf[ind++] = nval + ((nval < 10) ? '0' : 'A');
	    val >>= 4;
	} while (val);
	tmpbuf[ind++] = 'x';
	tmpbuf[ind++] = '0';
    }

    ind--;

    do {
	buf[fsize++] = tmpbuf[ind];
    } while ((ind-- > 0) && (fsize < (size-1)));
    buf[fsize] = '\0';

    return fsize;
}

/** Simple hex string to uint32_t */
static uint32_t atoh(char *buf)
{
    uint32_t res=0;
    char ch;

    while ((ch=*buf++) != 0) {
	if (ch >= '0' && ch <= '9') {
	    res = (res << 4) + ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
	    res = (res << 4) + ch - 'a';
	} else if (ch >= 'A' && ch <= 'F') {
	    res = (res << 4) + ch - 'A';
	} else {
	    break;
	}
    }

    return res;
}

/** Simple ASCII to unsigned int */
static uint32_t atou(const char *buf)
{
    uint32_t res=0;

    while (*buf) {
	if ((*buf < '0') || (*buf > '9')) {
	    break;
	}
	res = res * 10 + *buf - '0';
	buf++;
    }

    return res;
}

/** Convert an IPAdress to an ASCIIZ string */
char *WiFly::iptoa(IPAddress addr, char *buf, int size)
{
    uint8_t fsize=0;
    uint8_t ind;

    for (ind=0; ind<3; ind++) {
	fsize += simple_utoa(addr[ind], 10, &buf[fsize], size-fsize);
	if (fsize < (size-1)) {
	    buf[fsize++] = '.';
	}
    }
    simple_utoa(addr[ind], 10, &buf[fsize], size-fsize);
    return buf;
}

/** Convert a dotquad string to an IPAddress */
IPAddress WiFly::atoip(char *buf)
{
    IPAddress ip;

    for (uint8_t ind=0; ind<3; ind++) {
	ip[ind] = atou(buf);
	while (*buf >= '0'  && *buf <= '9') {
	    buf++;
	}
	if (*buf == '\0') break;
    }

    return ip;
}

WiFly::WiFly()
{
    debug = NULL;
    inCommandMode = false;
    exitCommand = 0;
    connected = false;
    connecting = false;
    dhcp = true;
    restoreHost = true;
#ifdef DEBUG
    debugOn = true;
#else
    debugOn = false;
#endif

    dbgBuf = NULL;
    dbgInd = 0;
    dbgMax = 0;

}

/**
 * Get WiFly ready to handle commands, and determine
 * some initial status.
 */
void WiFly::init()
{
    int8_t dhcpMode=0;

    if (!setopt(PSTR("set u m 1"), NULL)) {
	eprint_P(PSTR("Failed to turn off echo\n\r"));
    }
    if (!setopt(PSTR("set sys printlvl 0"), NULL)) {
	eprint_P(PSTR("Failed to turn off sys print\n\r"));
    }
    if (!setopt(PSTR("set comm remote 0"), NULL)) {
	eprint_P(PSTR("Failed to set comm remote\n\r"));
    }

    /* update connection status */
    getConnection();

    DPRINT("tcp status: "); DPRINT(status.tcp); DPRINT("\n\r");
    DPRINT("assoc status: "); DPRINT(status.assoc); DPRINT("\n\r");
    DPRINT("authen status: "); DPRINT(status.authen); DPRINT("\n\r");
    DPRINT("dns status: "); DPRINT(status.dnsServer); DPRINT("\n\r");
    DPRINT("dns found status: "); DPRINT(status.dnsFound); DPRINT("\n\r");
    DPRINT("channel status: "); DPRINT(status.channel); DPRINT("\n\r");

    dhcpMode = getDHCPMode();
    dhcp = !((dhcpMode == WIFLY_DHCP_MODE_OFF) || (dhcpMode == WIFLY_DHCP_MODE_SERVER));

    if (status.tcp == WIFLY_TCP_CONNECTED) {
	connected = true;
    }
}

boolean WiFly::begin(Stream *serialdev, Print *debugPrint)
{

    serial = serialdev;
    debug = debugPrint;

    if (!enterCommandMode()) {
	eprint_P(PSTR("Failed to enter command mode\n\r"));
	return false;
    }

    init();

    if (!exitCommandMode()) {
	eprint_P(PSTR("Failed to exit command mode\n\r"));
	return false;
    }

    return true;
}

void WiFly::print_P(const prog_char *str)
{
    char ch;
    while ((ch=pgm_read_byte(str++)) != 0) {
	serial->write(ch);
    }
}

void WiFly::println_P(const prog_char *str)
{
    print_P(str);
    serial->println();
}

/**
 * Return number of bytes of memory available.
 * @returns number of bytes of free memory
 */
int WiFly::getFreeMemory()
{
    int free;

    if ((int)__brkval == 0)
	free = ((int)&free) - ((int)&__bss_end);
    else
	free = ((int)&free) - ((int)__brkval);

    return free;
}

/* Error print via debug stream */
void WiFly::eprint(const char ch)
{
    if (debug) {
	debug->print(ch);
    }
}

void WiFly::eprint(const char *str)
{
    if (debug) {
	debug->print(str);
    }
}

void WiFly::eprintln(const char *str)
{
    if (debug) {
	debug->println(str);
    }
}

void WiFly::eprint(const int data)
{
    if (debug) {
	debug->print(data);
    }
}

void WiFly::eprint(const char data, int base)
{
    if (debug) {
	debug->print(data,base);
    }
}

void WiFly::eprint(const unsigned char data, int base)
{
    if (debug) {
	debug->print(data,base);
    }
}
void WiFly::eprint(const int data, int base)
{
    if (debug) {
	debug->print(data,base);
    }
}

void WiFly::eprint(const unsigned int data, int base)
{
    if (debug) {
	debug->print(data,base);
    }
}
void WiFly::eprint(const long data, int base)
{
    if (debug) {
	debug->print(data,base);
    }
}

void WiFly::eprint(const unsigned long data, int base)
{
    if (debug) {
	debug->print(data,base);
    }
}


void WiFly::eprintln(const int data)
{
    if (debug) {
	debug->println(data);
    }
}

void WiFly::eprintln(void)
{
    if (debug) {
	debug->println();
    }
}

void WiFly::eprint_P(const prog_char *str)
{
    char ch;
    if (debug) {
	while ((ch=pgm_read_byte(str++)) != 0) {
	    debug->write(ch);
	}
    }
}

void WiFly::eprintln_P(const prog_char *str)
{
    eprint_P(str);
    eprintln();
}

void WiFly::flushRx(int timeout)
{
    char ch;
    DPRINT("flush\n\r");
    while (readTimeout(&ch,timeout));
    DPRINT("flushed\n\r");
}

size_t WiFly::write(uint8_t byte)
{
   return serial->write(byte);
}

/* Read-ahead for checking for TCP stream close */
static char peekBuf[6];
static uint8_t peekInd = 0;	/* Current read-ahead index */
static uint8_t peekOut = 0;	/* Feed read-ahead from here */
const prog_char resp_Close[] PROGMEM = "*CLOS*";

int WiFly::peek()
{
    if (!peekInd) {
       return serial->peek();
    } else {
       return peekBuf[peekOut];
    }
}

/* Check for stream close, if its closed
 * we will quickly receive *CLOS* from the WiFly
 */
boolean WiFly::checkClose(boolean peeked)
{
    /* Already read the first character? */
    if (!peeked) {
	peekBuf[0] = '*';
	peekInd = 1;
    } else {
	peekInd = 0;
    }

    peekOut = 0;

    while (readTimeout(&peekBuf[peekInd]),50) {
	if (peekBuf[peekInd] != pgm_read_byte(&resp_Close[peekInd])) {
	    /* Not a close */
	    peekInd++;
	    break;
	} else {
	    if (peekInd >= 5) {
		/* Done - got a close */
		peekInd = 0;
		connected = false;
		eprint_P(PSTR("Closed\n\r"));
		return true;
	    }
	}
	peekInd++;
    }

    return false;
}

int WiFly::read()
{
    int data = -1;

    /* Any data in peek buffer? */
    if (peekInd) {
	data = (uint8_t)peekBuf[peekOut++];
	if (peekOut >= peekInd) {
	    /* Finished catching up */
	    peekInd = 0;
	    peekOut = 0;
	}
    } else {
	data = serial->read();
	/* TCP connected? Check for close */
	if (connected && data == '*') {
	    if (checkClose(false)) {
		return -1;
	    } else {
		data = (uint8_t)peekBuf[peekOut++];
		if (peekOut >= peekInd) {
		    /* Finished catching up */
		    peekInd = 0;
		    peekOut = 0;
		}
	    }
	}
    }

    return data;
}


int WiFly::available()
{
    int count;

    count = serial->available();
    if (count > 0) {
	/* Check for TCP stream closure */
	if (connected && (serial->peek() == '*')) {
	    if (checkClose(true)) {
		return -1;
	    } else {
		return peekInd;
	    }
	}
    }

    return count+peekInd;
}

void WiFly::flush()
{
   serial->flush();
}
  

void WiFly::dump(const char *str)
{
    if (debug) {
	while (*str) {
	    eprint(*str,HEX);
	    eprint(' ');
	    str++;
	}
	eprintln();
    }
}

/* Send a string to the WiFly */
void WiFly::send(const char *str)
{
    DPRINT("send: "); DPRINT(str); DPRINT("\n\r");

    serial->print(str);
}

/* Send a character to the WiFly */
void WiFly::send(const char ch)
{
    serial->write(ch);
}

/* Send a string from PROGMEM to the WiFly */
void WiFly::send_P(const prog_char *str)
{
    uint8_t ch;

    DPRINT("send_P: ");

    while ((ch=pgm_read_byte(str++)) != 0) {
	serial->write(ch);
	DPRINT((char)ch);
    }
    DPRINT("\n\r");
}

void WiFly::dbgBegin(int size)
{
    if (dbgBuf != NULL) {
	free(dbgBuf);
    }
    dbgBuf = (char *)malloc(size);
    dbgInd = 0;
    dbgMax = size;
}

void WiFly::dbgDump()
{
    int ind;

    if (dbgBuf == NULL) {
	return;
    }

    eprint_P(PSTR("debug dump\n\r"));

    for (ind=0; ind<dbgInd; ind++) {
	debug->print(ind);
	debug->print(": ");
	debug->print(dbgBuf[ind],HEX);
	if (isprint(dbgBuf[ind])) {
	    debug->print(" ");
	    debug->print(dbgBuf[ind]);
	}
	debug->println();
    }
    free(dbgBuf);
    dbgBuf = NULL;
    dbgMax = 0;
}

boolean WiFly::readTimeout(char *ch, uint16_t timeout)
{
    uint32_t start = millis();

    static int ind=0;

    while (millis() - start < timeout) {
	if (serial->available() > 0) {
	    *ch = serial->read();
	    if (dbgInd < dbgMax) {
		dbgBuf[dbgInd++] = *ch;
	    }
	    if (debugOn) {
		eprint(ind++);
		eprint_P(PSTR(": "));
		eprint(*ch,HEX);
		if (isprint(*ch)) {
		    eprint(' ');
		    eprint(*ch);
		}
		eprintln();
	    }
	    return true;
	}
    }

    if (debugOn) {
	eprint_P(PSTR("readTimeout - timed out\n\r"));
    }

    return false;
}

static char prompt[16];
static boolean gotPrompt = false;

boolean WiFly::setPrompt()
{
    char ch;

    while (readTimeout(&ch,500)) {
	if (ch == '<') {
	    uint8_t ind = 1;
	    prompt[0] = ch;
	    while (ind < (sizeof(prompt)-4)) {
		if (readTimeout(&ch,500)) {
		    prompt[ind++] = ch;
		    if (ch == '>') {
			if (readTimeout(&ch,500)) {
			    if (ch == ' ') {
				prompt[ind++] = ch;
				//prompt[ind++] = '\r';
				//prompt[ind++] = '\n';
				prompt[ind] = 0;
				DPRINT("setPrompt: "); DPRINT(prompt); DPRINT("\n\r");
				gotPrompt = true;
				gets(NULL,0);
				return true;
			    } else {
				/* wrong character */
				return false;
			    }
			} else {
			    /* timeout */
			    return false;
			}
		    }
		} else {
		    /* timeout */
		    return false;
		}
	    }

	    return false;
	}
    }

    return false;
}

int WiFly::getResponse(char *buf, int size, uint16_t timeout)
{
    int count=0;

    for (count=0; count<(size-1) && readTimeout(&buf[count], timeout); count++) {
    }

    buf[count] = 0;

    return count;
}

/* See if the prompt is somwhere in the string */
boolean WiFly::checkPrompt(const char *str)
{
    if (strstr(str, prompt) != NULL) {
	return true;
    } else {
	return false;
    }
}

/* Reach characters from the WiFly and match them against the
 * string. Ignore any leading characters that don't match.
 */
boolean WiFly::match(const char *str, boolean strict, uint16_t timeout)
{
    const char *match = str;
    char ch;
    boolean sticky=false;

#ifdef DEBUG
    if (debug && debugOn) {
	eprint_P(PSTR("match: "));
	eprintln(str);
    }
#endif

    if ((match == NULL) || (*match == '\0')) {
	return true;
    }

    /* find first character */
    while (readTimeout(&ch,timeout)) {
	if (ch == *match) {
	    match++;
	    sticky = true;
	} else {
	    if (sticky && strict) {
		return false;
	    } 

	    match = str;
	}
	if (*match == '\0') {
	    DPRINT("match: true\n\r");
	    return true;
	}
    }

    DPRINT("match: false\n\r");
    return false;
}

/* Reach characters from the WiFly and match them against the
 * PROGMEM string. Ignore any leading characters that don't match.
 */
boolean WiFly::match_P(const prog_char *str, uint16_t timeout)
{
    const prog_char *match = str;
    char ch, ch_P;

    if (debug && debugOn) {
	eprint_P(PSTR("match_P: "));
	ch_P = pgm_read_byte(match);
	while (ch_P != '\0') {
	    debug->write(ch_P);
	    match++;
	    ch_P = pgm_read_byte(match);
	}
	debug->println();
	match = str;
    }

    ch_P = pgm_read_byte(match);
    if (ch_P == '\0') {
	/* Null string always matches */
	return true;
    }

    while (readTimeout(&ch,timeout)) {
	if (ch == ch_P) {
	    match++;
	} else {
	    /* Restart match */
	    match = str;
	}

	ch_P = pgm_read_byte(match);
	if (ch_P == '\0') {
	    DPRINT("match_P: true\n\r");
	    return true;
	}
    }

    DPRINT("match_P: false\n\r");
    return false;
}

boolean WiFly::getPrompt(boolean strict, uint16_t timeout)
{
    boolean res;

    if (!gotPrompt) {
	DPRINT("setPrompt\n\r");

	res = setPrompt();
	if (!res) {
	    eprint_P(PSTR("setPrompt failed\n\r"));
	}
    } else {
	DPRINT("getPrompt \""); DPRINT(prompt); DPRINT("\"\n\r");
	res = match(prompt, strict, timeout);
    }
    return res;
}

boolean WiFly::enterCommandMode()
{
    uint8_t retry;

    if (inCommandMode) {
	return true;
    }

    delay(250);
    send_P(PSTR("$$$"));
    delay(250);
    if (match_P(PSTR("CMD\r\n"), 500)) {
	/* Get the prompt */
	if (gotPrompt) {
	    inCommandMode = true;
	    return true;
	} else {
	    for (retry=0; retry < 5; retry++) {
		serial->write('\r');
		if (getPrompt()) {
		    inCommandMode = true;
		    return true;
		}
	    }
	}
    }

    /* See if we're already in command mode */
    DPRINT("Check in command mode\n\r");
    serial->write('\r');
    if (getPrompt()) {
	inCommandMode = true;
	DPRINT("Already in command mode\n\r");
	return true;
    }

    for (retry=0; retry<5; retry++) {
	DPRINT("send $$$ "); DPRINT(retry); DPRINT("\n\r");
	send_P(PSTR("$$$"));
	delay(250);
	if (match_P(PSTR("CMD\r\n"), 500)) {
	    inCommandMode = true;
	    return true;
	}
    }

    return false;
}

boolean WiFly::exitCommandMode()
{
    if (!inCommandMode) {
	return true;
    }

    send_P(PSTR("exit\r"));

    if (match_P(PSTR("EXIT\r\n"), 500)) {
	inCommandMode = false;
	return true;
    } else {
	eprint_P(PSTR("Failed to exit\n\r"));
	return false;
    }
}

/* Get data to end of line */
int WiFly::gets(char *buf, int size, uint16_t timeout)
{
    char ch;
    int ind=0;

    if (debugOn) {
	DPRINT("gets: \n\r");
    }

    while (readTimeout(&ch, timeout)) {
	if (ch == '\r') {
	    readTimeout(&ch, timeout);
	    if (ch == '\n') {
		if (buf) {
		    buf[ind] = 0;
		}
		return ind;
	    }
	}
	/* Truncate to buffer size */
	if ((ind < (size-1)) && buf) {
	    buf[ind++] = ch;
	}
    }

    if (buf) {
	buf[ind] = 0;
    }
    return 0;
}

/* Get the WiFly ready to receive a command. */
boolean WiFly::startCommand()
{
    if (!inCommandMode) {
	if (!enterCommandMode()) {
	    return false;
	}
	/* If we're already in command mode, then we don't exit it in finishCommand().
	 * This is an optimisation to avoid switching in and out of command mode 
	 * when using several commands to implement another command.
	 */
    } else {
	DPRINT("Already in command mode\n\r");
    }
    exitCommand++;
    return true;
}

/* Finished with command */
boolean WiFly::finishCommand()
{
    if (--exitCommand == 0) {
	return exitCommandMode();
    }
    return true;
}

/* Get the value of an option */
char *WiFly::getopt(int opt, char *buf, int size)
{
    if (startCommand()) {
	send_P(requests[opt].req);

	if (match_P(requests[opt].resp, 500)) {
	    gets(buf, size);
	    getPrompt();
	    finishCommand();
	    return buf;
	}

	finishCommand();
    }
    return (char *)"<error>";
}

/* Get WiFly connection status */
uint16_t WiFly::getConnection()
{
    char buf[16];
    uint16_t res;
    int len;

    if (!startCommand()) {
	eprint_P(PSTR("getCon: failed to start\n\r"));
	return 0;
    }
    //dbgBegin(256);

    DPRINT("getCon\n\r");
    DPRINT("show c\n\r");
    send_P(PSTR("show c\r"));
    len = gets(buf, sizeof(buf));

    if (checkPrompt(buf)) {
	/* Got prompt first */
	len = gets(buf, sizeof(buf));
    } else {
	getPrompt();
    }

    if (len <= 4) {
	res = (uint16_t)atoh(buf);
    } else {
	res = (uint16_t)atoh(&buf[len-4]);
    }

    status.tcp = (res >> WIFLY_STATUS_TCP_OFFSET) & WIFLY_STATUS_TCP_MASK;
    status.assoc = (res >> WIFLY_STATUS_ASSOC_OFFSET) & WIFLY_STATUS_ASSOC_MASK;
    status.authen = (res >> WIFLY_STATUS_AUTHEN_OFFSET) & WIFLY_STATUS_AUTHEN_MASK;
    status.dnsServer = (res >> WIFLY_STATUS_DNS_SERVER_OFFSET) & WIFLY_STATUS_DNS_SERVER_MASK;
    status.dnsFound = (res >> WIFLY_STATUS_DNS_FOUND_OFFSET) & WIFLY_STATUS_DNS_FOUND_MASK;
    status.channel = (res >> WIFLY_STATUS_CHAN_OFFSET) & WIFLY_STATUS_CHAN_MASK;

    finishCommand();

    return res;
}

char *WiFly::getIP(char *buf, int size)
{
    return getopt(WIFLY_GET_IP, buf, size);
}

char *WiFly::getHostIP(char *buf, int size)
{
    if (getopt(WIFLY_GET_HOST, buf, size)) {
	/* Trim off port */
	while (*buf && *buf != ':') {
	    buf++;
	}
    }
    *buf = '\0';
    return buf;
}

uint16_t WiFly::getHostPort()
{
    char buf[22];
    uint8_t ind;

    if (getopt(WIFLY_GET_HOST, buf, sizeof(buf))) {
	/* Trim off IP */
	for (ind=0;  buf[ind]; ind++) {
	    if (buf[ind] == ':') {
		ind++;
		break;
	    }
	}
	return (uint16_t)atou(&buf[ind]);
    }
    return 0;
}

char *WiFly::getNetmask(char *buf, int size)
{
    return getopt(WIFLY_GET_NETMASK, buf, size);
}

char *WiFly::getGateway(char *buf, int size)
{
    return getopt(WIFLY_GET_GATEWAY, buf, size);
}

char *WiFly::getDNS(char *buf, int size)
{
    return getopt(WIFLY_GET_DNS, buf, size);
}

char *WiFly::getMAC(char *buf, int size)
{
    return getopt(WIFLY_GET_MAC, buf, size);
}

char *WiFly::getSSID(char *buf, int size)
{
    return getopt(WIFLY_GET_SSID, buf, size);
}

char *WiFly::getDeviceID(char *buf, int size)
{
    return getopt(WIFLY_GET_DEVICEID, buf, size);
}

uint8_t WiFly::getIpFlags()
{
    char buf[3];

    if (!getopt(WIFLY_GET_IP_FLAGS, buf, sizeof(buf))) {
	return 0;
    }

    return (uint8_t)atoh(buf);
}

uint32_t WiFly::getBaud()
{
    char buf[16];

    if (!getopt(WIFLY_GET_BAUD, buf, sizeof(buf))) {
	return 0;
    }

    return atou(buf);
}

char *WiFly::getTime(char *buf, int size)
{
    return getopt(WIFLY_GET_TIME, buf, size);
}

uint32_t WiFly::getRTC()
{
    char buf[16];

    return atou(getopt(WIFLY_GET_RTC, buf, sizeof(buf)));
}

/**
 * Do a DNS lookup to find the ip address of the specified hostname 
 * @param hostname - host to lookup
 * @param buf - buffer to return the ip address in
 * @param size - size of the buffer
 * @return true on success, false on failure
 */
bool WiFly::getHostByName(const char *hostname, char *buf, int size)
{
    if (startCommand()) {
	send_P(PSTR("lookup "));
	send(hostname);
	send("\r");

	if (match(hostname, false, 5000)) {
	    char ch;
	    readTimeout(&ch);	// discard '='
	    gets(buf, size);
	    getPrompt();
	    finishCommand();
	    return true;
	}

	getPrompt();
	finishCommand();
    }

    /* lookup failed */
    return false;
}

uint32_t WiFly::getUptime()
{
    char buf[16];

    if (!getopt(WIFLY_GET_UPTIME, buf, sizeof(buf))) {
	return 0;
    }

    return atou(buf);
}

uint8_t WiFly::getTimezone()
{
    char buf[16];

    if (!getopt(WIFLY_GET_ZONE, buf, sizeof(buf))) {
	return 0;
    }

    return (uint8_t)atou(buf);
}

uint8_t WiFly::getUartMode()
{
    char buf[5];

    if (!getopt(WIFLY_GET_UART_MODE, buf, sizeof(buf))) {
	return 0;
    }

    return (uint8_t)atoh(&buf[2]);
}

int8_t WiFly::getDHCPMode()
{
    char buf[16];
    int8_t mode;

    if (!getopt(WIFLY_GET_DHCP, buf, sizeof(buf))) {
	return -1;
    }

    if (strncmp_P(buf, PSTR("OFF"), 3) == 0) {
	mode = 0;
    } else if (strncmp_P(buf, PSTR("ON"), 2) == 0) {
	mode = 1;
    } else if (strncmp_P(buf, PSTR("AUTOIP"), 6) == 0) {
	mode = 2;
    } else if (strncmp_P(buf, PSTR("CACHE"), 5) == 0) {
	mode = 3;
    } else if (strncmp_P(buf, PSTR("SERVER"), 6) == 0) {
	mode = 4;
    } else {
	mode = -1;	// unknown
    }
    
    return mode;
}

uint16_t WiFly::getFlushTimeout()
{
    char buf[16];

    if (!getopt(WIFLY_GET_FLUSHTIMEOUT, buf, sizeof(buf))) {
	return 0;
    }

    return (uint16_t)atou(buf);
}

uint16_t WiFly::getFlushSize()
{
    char buf[5];

    if (!getopt(WIFLY_GET_FLUSHSIZE, buf, sizeof(buf))) {
	return 0;
    }

    return (uint16_t)atou(buf);
}

uint8_t WiFly::getFlushChar()
{
    char buf[5];

    if (!getopt(WIFLY_GET_FLUSHCHAR, buf, sizeof(buf))) {
	return 0;
    }

    return (uint8_t)atoh(&buf[2]);
}

int8_t WiFly::getRSSI()
{
    char buf[5];

    if (!getopt(WIFLY_GET_RSSI, buf, sizeof(buf))) {
	return 0;
    }

    return -(int8_t)atou(buf);
}


const prog_char res_AOK[] PROGMEM = "AOK\r\n";
const prog_char res_ERR[] PROGMEM = "ERR: ";

/* Get the result from a set operation
 * Should be AOK or ERR
 */
boolean WiFly::getres(char *buf, int size)
{
    char ch;

    DPRINT("getres\n\r");

    while (readTimeout(&ch,500)) {
	switch (ch) {
	case 'A':
	    if (match_P(res_AOK+1)) {
		return true;
	    } else {
		eprint_P(PSTR("Failed to get AOK\n\r"));
		strncpy_P(buf, PSTR("<no AOK>"), size);
		return false;
	    }
	    break;
	case 'E':
	    if (match_P(res_ERR+1)) {
		gets(buf, size);
		eprint_P(PSTR("ERR: ")); eprintln(buf);
		return false;
	    }
	default:
	    break;
	}
    }

    strncpy_P(buf, PSTR("<timeout>"), size);
    return false;
}

/* Set an option, confirm ok status */
boolean WiFly::setopt(const prog_char *cmd, const char *buf)
{
    char rbuf[16];
    boolean res;

    if (!startCommand()) {
	return false;
    }

    send_P(cmd);
    if (buf != NULL) {
	send_P(PSTR(" "));
	send(buf);
    }
    send_P(PSTR("\r"));

    res = getres(rbuf, sizeof(rbuf));
    getPrompt();

    finishCommand();
    return res;
}

/* Save current configuration */
boolean WiFly::save()
{
    bool res = false;

    if (!startCommand()) {
	return false;
    }
    send_P(PSTR("save\r"));
    if (match_P(PSTR("Storing"))) {
	getPrompt();
	res = true;
    }

    finishCommand();
    return res;
}

boolean WiFly::reboot()
{
    if (!startCommand()) {
	return false;
    }
    send_P(PSTR("reboot\r"));
    if (!match_P(PSTR("*Reboot*"))) {
	finishCommand();
	return false;
    }

    delay(5000);
    inCommandMode = false;
    exitCommand = 0;
    init();
    return true;

}

boolean WiFly::factoryRestore()
{
    bool res = false;

    if (!startCommand()) {
	return false;
    }
    send_P(PSTR("factory RESTORE\r"));
    if (match_P(PSTR("Set Factory Defaults"))) {
	getPrompt();
	res = true;
    }

    finishCommand();
    return res;
}

boolean WiFly::setDeviceID(const char *buf)
{
    return setopt(PSTR("set o d"), buf);
}

boolean WiFly::setIP(const char *buf)
{
    return setopt(PSTR("set ip address"), buf);
}

boolean WiFly::setHostIP(const char *buf)
{
    return setopt(PSTR("set ip host"), buf);
}

boolean WiFly::setHostPort(const uint16_t port)
{
    char str[6];

    simple_utoa(port, 10, str, sizeof(str));
    return setopt(PSTR("set ip remote"), str);
}

boolean WiFly::setHost(const char *buf, uint16_t port)
{
    bool res;

    if (!startCommand()) {
	eprintln_P(PSTR("SetHost: failed to start command"));
	return false;
    }

    res = setHostIP(buf);
    res = res && setHostPort(port);

    finishCommand();

    return res;
}

boolean WiFly::setNetmask(const char *buf)
{
    return setopt(PSTR("set ip netmask"), buf);
}

boolean WiFly::setGateway(const char *buf)
{
    return setopt(PSTR("set ip gateway"), buf);
}

boolean WiFly::setDNS(const char *buf)
{
    return setopt(PSTR("set dns address"), buf);
}

boolean WiFly::setDHCP(const uint8_t mode)
{
    char buf[2];

    if (mode > 9) {
	return false;
    }

    buf[0] = '0' + mode;
    buf[1] = 0;

    return setopt(PSTR("set ip dhcp"), buf);
}

boolean WiFly::setIpProtocol(const uint8_t protocol)
{
    char hex[5];

    simple_utoa(protocol, 16, hex, sizeof(hex));

    return setopt(PSTR("set ip protocol"), hex);
}

boolean WiFly::setIpFlags(const uint8_t protocol)
{
    char hex[5];

    simple_utoa(protocol, 16, hex, sizeof(hex));

    return setopt(PSTR("set ip protocol"), hex);
}

boolean WiFly::setTimeAddress(const char *buf)
{
    return setopt(PSTR("set time address"), buf);
}

boolean WiFly::setTimePort(const uint16_t port)
{
    char str[6];

    simple_utoa(port, 10, str, sizeof(str));
    return setopt(PSTR("set time port"), str);
}

boolean WiFly::setTimezone(const uint8_t zone)
{
    char str[4];

    simple_utoa(zone, 10, str, sizeof(str));
    return setopt(PSTR("set time zone"), str);
}

boolean WiFly::setTimeEnable(const uint16_t protocol)
{
    char str[6];

    simple_utoa(protocol, 10, str, sizeof(str));

    return setopt(PSTR("set time enable"), str);
}

boolean WiFly::setUartMode(const uint8_t mode)
{
    char hex[5];

    /* Need to keep echo off for library to function correctly */
    simple_utoa(mode | WIFLY_UART_MODE_NOECHO, 16, hex, sizeof(hex));

    return setopt(PSTR("set uart mode"), hex);
}

boolean WiFly::setBroadcastInterval(const uint8_t seconds)
{
    char hex[5];

    simple_utoa(seconds, 16, hex, sizeof(hex));

    return setopt(PSTR("set broadcast interval"), hex);
}

/**
 * Set comms flush timeout. When using data trigger mode,
 * this timer defines how long the WiFly will wait for
 * the next character from the sketch before sending what
 * it has collected so far as a packet.
 * @param timeout number of milliseconds to wait before
 *                flushing the packet.
 * @note the flush timeout change does not actually work
 *       unless the config is saved and the wifly is rebooted.
 */
boolean WiFly::setFlushTimeout(const uint16_t timeout)
{
    char str[6];

    simple_utoa(timeout, 10, str, sizeof(str));
    return setopt(PSTR("set comm time"), str);
}

boolean WiFly::setFlushChar(const char flushChar)
{
    char str[5];

    simple_utoa((uint8_t)flushChar, 16, str, sizeof(str));
    return setopt(PSTR("set comm match"), str);
}

boolean WiFly::setFlushSize(const uint16_t size)
{
    char str[6];
    uint16_t lsize = size;

    if (lsize > 1460) {
	/* Maximum size */
	lsize = 1460;
    }

    simple_utoa(lsize, 10, str, sizeof(str));
    return setopt(PSTR("set comm size"), str);
}

boolean WiFly::setIOFunc(const uint8_t func)
{
    char str[5];

    simple_utoa(func, 16, str, sizeof(str));
    return setopt(PSTR("set sys iofunc"), str);
}

/**
 * Enable data trigger mode.  This mode will automatically send a new packet based on several conditions:
 * 1. If no characters are send to the WiFly for at least the flushTimeout period.
 * 2. If the character defined by flushChar is sent to the WiFly.
 * 3. If the numebr of characters sent to the WiFly reaches flushSize.
 * @param flushTimeout Send a packet if no more characters are sent within this many milliseconds. Set
 *                     to 0 to disable.
 * @param flushChar Send a packet when this character is sent to the WiFly. Set to 0 to disable.
 * @param flushSize Send a packet when this many characters have been sent to the WiFly.
 * @returns true on success, else false.
 */
boolean WiFly::enableDataTrigger(const uint16_t flushTimeout, const char flushChar, const uint16_t flushSize)
{
    bool res=true;

    res = res && setUartMode(getUartMode() | WIFLY_UART_MODE_DATA_TRIGGER);
    res = res && setFlushTimeout(flushTimeout);
    res = res && setFlushChar(flushChar);
    res = res && setFlushSize(flushSize);

    return res;
}

boolean WiFly::disableDataTrigger()
{
    bool res=true;

    res = res && setUartMode(getUartMode() & ~WIFLY_UART_MODE_DATA_TRIGGER);
    res = res && setFlushTimeout(10);
    res = res && setFlushChar(0);
    res = res && setFlushSize(64);

    return res;
}

/** Hide passphrase and key */
boolean WiFly::hide()
{
    return setopt(PSTR("set wlan hide 1"), NULL);
}

boolean WiFly::disableDHCP()
{
    return setDHCP(0);
}

boolean WiFly::enableDHCP()
{
    return setDHCP(1);
}

boolean WiFly::setSSID(const char *buf)
{
    return setopt(PSTR("set wlan ssid"), buf);
}

boolean WiFly::setChannel(const char *buf)
{
    return setopt(PSTR("set wlan chan"), buf);
}

/** Set WEP key */
boolean WiFly::setKey(const char *buf)
{
    boolean res;
    res = setopt(PSTR("set wlan key"), buf);

    hide();	/* hide the key */
    return res;
}

/**
 * Set WPA passphrase.
 * Use '$' instead of spaces.
 * Use setSpaceReplacement() to change the replacement character.
 */
boolean WiFly::setPassphrase(const char *buf)
{
    boolean res;
    res = setopt(PSTR("set wlan phrase"), buf);

    hide();	/* hide the key */
    return res;
}

/**
 * Set the space replacement character in WPA passphrase.
 * Default is '$'.
 */
boolean WiFly::setSpaceReplace(const char *buf)
{
    return setopt(PSTR("set opt replace"), buf);
}

/** join a wireless network */
boolean WiFly::join(const char *ssid)
{
    if (!startCommand()) {
	return false;
    }

    send_P(PSTR("join "));
    if (ssid != NULL) {
	send(ssid);
    }
    send_P(PSTR("\r"));

    if (match_P(PSTR("Associated!"),10000)) {
	flushRx(100);
        status.assoc = 1;
	if (dhcp) {
	    // need some time to complete DHCP request
	    match_P(PSTR("GW="), 15000);
	}
	gets(NULL,0);
	finishCommand();
	return true;
    }

    finishCommand();
    return false;
}

/** join a wireless network */
boolean WiFly::join(void)
{
    char ssid[64];
    getSSID(ssid, sizeof(ssid));

    return join(ssid);
}

/** leave the wireless network */
boolean WiFly::leave()
{
    send_P(PSTR("leave\r"));

    /* Don't care about result, it either succeeds with a
     * "DeAuth" reponse and prompt, or fails with just a
     * prompt because we're already de-associated.
     * So discard the result.
     */
    flushRx(100);

    status.assoc = 0;
    return true;
}

/** Check to see if the WiFly is connected to a wireless network */
boolean WiFly::isAssociated()
{
    return (status.assoc == 1);
}

boolean WiFly::setBaud(uint32_t baud)
{
    char buf[16];
    simple_utoa(baud, 10, buf, sizeof(buf));
    DPRINT("set baud "); DPRINT(buf); DPRINT("\n\r");

    /* Go into command mode, since "set uart instant" will exit command mode */
    startCommand();
    if (setopt(PSTR("set u i"), buf)) {
	//serial->begin(baud);		// Sketch will need to do this
	return true;
    }
    return false;
}

/** See if the string is a valid dot quad IP address */
boolean WiFly::isDotQuad(const char *addr)
{
    uint32_t value;

    for (uint8_t ind=0; ind<3; ind++) {
	value  = atou(addr);
	if (value > 255) {
	    return false;
	}
	while (*addr >= '0'  && *addr <= '9') {
	    addr++;
	}
	if (ind == 3) {
	    /* Got a match if this is the end of the string */
	    return *addr == '\0';
	}
	if (*addr != '.') {
	    return false;
	}
    }

    return false;
}

boolean WiFly::ping(const char *host)
{
    char ip[16];
    const char *addr = host;

    if (!isDotQuad(host)) {
	/* do a DNS lookup to get the IP address */
	if (!getHostByName(host, ip, sizeof(ip))) {
	    return false;
	}
	addr = ip;
    }

    startCommand();
    send_P(PSTR("ping "));
    send(addr);
    send('\r');

    match_P(PSTR("Ping try"));
    gets(NULL,0);
    if (!getPrompt()) {
	finishCommand();
	return false;
    }

    if (match_P(PSTR("64 bytes"), 5000)) {
	gets(NULL,0);
	gets(NULL,0, 5000);
	finishCommand();
	return true;
    }

    finishCommand();
    return false;
}

/**
 * Open a TCP connection
 * If there is already an open connection then that is closed first.
 * @param addr - the IP address or hostname to connect 
 * @param port - the TCP port to connect to
 * @param block - true = wait for the connection to complete
 *                false = start the connection, but use openComplete() 
 *                        to determine the result.
 * @returns true - success
 *          false - failed, or connection already in progress
 */
boolean WiFly::open(const char *addr, int port, boolean block)
{
    char buf[20];
    char ch;

    if (connecting) {
	/* an open is already in progress */
	return false;
    }

    startCommand();

    /* Already connected? Close the connection first */
    if (connected) {
	close();
    }

    simple_utoa(port, 10, buf, sizeof(buf));
    DPRINT("open "); DPRINT(addr); DPRINT(" "); DPRINT(buf); DPRINT("\n\r");
    eprint("open "); eprint(addr); eprint(" "); eprint(buf); eprintln();
    send_P(PSTR("open "));
    send(addr);
    send(" ");
    send(buf);
    send_P(PSTR("\r"));

    if (!getPrompt()) {
	eprint_P(PSTR("Failed to get prompt\n\r"));
	eprint_P(PSTR("WiFly has crashed and will reboot...\n\r"));
	while (1); /* wait for the reboot */
	return false;
    }

    if (!block) {
	/* Non-blocking connect, user will poll for result */
	connecting = true;
	return true;
    }

    /* Expect "*OPEN*" or "Connect FAILED" */

    while (readTimeout(&ch,10000)) {
	switch (ch) {
	case '*':
	    if (match_P(PSTR("OPEN*"))) {
		DPRINT("Connected\n\r");
		connected = true;
		/* successful connection exits command mode */
		inCommandMode = false;
		return true;
	    } else {
		finishCommand();
		return false;
	    }
	    break;
	case 'C': {
		buf[0] = ch;
		gets(&buf[1], sizeof(buf)-1);
		eprint_P(PSTR("Failed to connect: ")); eprintln(buf);
		finishCommand();
		return false;
	    }

	default:
	    if (debugOn) {
		eprint_P(PSTR("Unexpected char: "));
		eprint(ch,HEX);
		if (isprint(ch)) {
		    eprint(" ");
		    eprint(ch);
		}
		eprintln();
	    }
	    break;
	}
    }

    eprint_P(PSTR("<timeout>\n\r"));
    finishCommand();
    return false;
}

boolean WiFly::open(IPAddress addr, int port, boolean block)
{
    char buf[16];
    return open(iptoa(addr, buf, sizeof(buf)), port, block);
}

/** Check to see if there is a tcp connection. */
boolean WiFly::isConnected()
{
    return connected;
}

boolean WiFly::isInCommandMode()
{
    return inCommandMode;
}

boolean WiFly::sendto(
    const uint8_t *data,
    uint16_t size,
    const __FlashStringHelper *flashData,
    const char *host,
    uint16_t port)
{
    char lastHost[16];
    uint16_t lastPort;
    bool restore = restoreHost;

    if (!startCommand()) {
	eprintln_P(PSTR("sendto: failed to start command"));
	return false;
    }

    getHostIP(lastHost, sizeof(lastHost));
    lastPort = getHostPort();

    if ((port != lastPort) && (strcmp(host, lastHost) != 0)) {
	setHostIP(host);
	setHostPort(port);
    } else {
	/* same host and port, no need to restore */
	restore = false;
    }

    finishCommand();

    if (data) {
	write(data, size);
    } else if (flashData) {
	print(flashData);
    }

    /* Restore original host and port */
    if (restore) {
	eprint("Restoring host "); eprint(lastHost), eprint(":"), eprintln(lastPort);
	setHost(lastHost,lastPort);
    }

    return true;
}

boolean WiFly::sendto(const uint8_t *data, uint16_t size, const char *host, uint16_t port)
{
    return sendto(data, size, NULL, host, port);
}

boolean WiFly::sendto(const uint8_t *data, uint16_t size, IPAddress host, uint16_t port)
{
    char buf[16];

    return sendto(data, size, iptoa(host, buf, sizeof(buf)) , port);
}

boolean WiFly::sendto(const char *data, const char *host, uint16_t port)
{
    return sendto((uint8_t *)data, strlen(data), host, port);
}

boolean WiFly::sendto(const char *data, IPAddress host, uint16_t port)
{
    return sendto((uint8_t *)data, strlen(data), host, port);
}

boolean WiFly::sendto(const __FlashStringHelper *flashData, const char *host, uint16_t port)
{
    return sendto(NULL, 0, flashData, host, port);
}

boolean WiFly::sendto(const __FlashStringHelper *flashData, IPAddress host, uint16_t port)
{
    char buf[16];

    return sendto(NULL, 0, flashData, iptoa(host, buf, sizeof(buf)), port);
}

void WiFly::enableHostRestore()
{
    restoreHost = true;
}

void WiFly::disableHostRestore()
{
    restoreHost = false;
}

/**
 * Check to see if the non-blocking open has completed.
 * When this returns true the open has finished with either
 * success or failure. You can use isConnected() to see
 * if the open was successful.
 */
boolean WiFly::openComplete()
{
    char buf[20];

    if (!connecting) {
	return true;
    }

    if (serial->available()) {
	char ch = serial->read();
	switch (ch) {
	case '*':
	    if (match_P(PSTR("OPEN*"))) {
		DPRINT("Connected\n\r");
		connected = true;
		connecting = false;
		/* successful connection exits command mode */
		inCommandMode = false;
		DPRINT("openComplete: true\n\r");
		return true;
	    } else {
		/* Failed to connected */
		connecting = false;
		finishCommand();
		DPRINT("openComplete: true\n\r");
		return true;
	    }
	    break;
	case 'C': {
		buf[0] = ch;
		gets(&buf[1], sizeof(buf)-1);
		eprint_P(PSTR("Failed to connect: ")); eprintln(buf);
		connecting = false;
		finishCommand();
		DPRINT("openComplete: true\n\r");
		return true;
	    }

	default:
	    buf[0] = ch;
	    gets(&buf[1], sizeof(buf)-1);
	    if (debug) {
		eprint_P(PSTR("Unexpected resp: "));
		eprintln(buf);
	    }
	    connecting = false;
	    finishCommand();
	    DPRINT("openComplete: true\n\r");
	    return true;
	}
    }

    DPRINT("openComplete: false\n\r");
    return false;
}

/** Close the TCP connection */
boolean WiFly::close()
{
    if (!connected) {
	return true;
    }

    startCommand();
    send_P(PSTR("close\r"));

    if (match_P(PSTR("*CLOS*"))) {
	finishCommand();
	connected = false;
	return true;
    }

    finishCommand();

    return false;
}
