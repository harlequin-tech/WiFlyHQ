/*-
 * Copyright (c) 2012,2013 Darran Hunt (darran [at] hunt dot net dot nz)
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

/**
 * @file WiFly RN-XV Library
 */

#include "WiFlyHQ.h"

/* For free memory check */
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

#undef DEBUG

#ifdef DEBUG
#define DPRINT(item) debug.print(item)
#define DPRINTLN(item) debug.println(item)
#else
#define DPRINT(item)
#define DPRINTLN(item)
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

/* WiFi data rates */
#define WIFLY_RATE_1MBPS	0
#define WIFLY_RATE_2MBPS	1
#define WIFLY_RATE_5_5MBPS	2	/* 5.5 MBps */
#define WIFLY_RATE_6MBPS	8
#define WIFLY_RATE_9MBPS	9
#define WIFLY_RATE_11MBPS	3
#define WIFLY_RATE_12MBPS	10
#define WIFLY_RATE_18MBPS	11
#define WIFLY_RATE_24MBPS	12	/* Default */
#define WIFLY_RATE_36MBPS	13
#define WIFLY_RATE_48MBPS	14
#define WIFLY_RATE_54MBPS	15

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
const prog_char resp_Protocol[] PROGMEM = "PROTO=";
const prog_char req_GetAdhoc[] PROGMEM = "get adhoc\r";
const prog_char resp_Beacon[] PROGMEM = "Beacon=";
const prog_char resp_Probe[] PROGMEM = "Probe=";
const prog_char resp_Reboot[] PROGMEM = "Reboot=";
const prog_char resp_Join[] PROGMEM = "Join=";
const prog_char resp_Rate[] PROGMEM = "Rate=";
const prog_char resp_Power[] PROGMEM = "TxPower=";
const prog_char resp_Replace[] PROGMEM = "Replace=";

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
    { req_GetIP,	resp_Protocol }, /* 20 */
    { req_GetAdhoc,	resp_Beacon },   /* 21 */
    { req_GetAdhoc,	resp_Probe },    /* 22 */
    { req_GetAdhoc,	resp_Reboot },   /* 23 */
    { req_GetWLAN,	resp_Join },	 /* 24 */
    { req_GetWLAN,	resp_Rate },	 /* 25 */
    { req_GetWLAN,	resp_Power },	 /* 26 */
    { req_GetOpt,	resp_Replace },	 /* 27 */
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
    WIFLY_GET_PROTOCOL	= 20,
    WIFLY_GET_BEACON	= 21,
    WIFLY_GET_PROBE	= 22,
    WIFLY_GET_REBOOT	= 23,
    WIFLY_GET_JOIN	= 24,
    WIFLY_GET_RATE	= 25,
    WIFLY_GET_POWER	= 26,
    WIFLY_GET_REPLACE	= 27,
} e_wifly_requests;

/**
 * Convert a unsigned int to a string
 * @param val the value to convert to a string
 * @param base format for string; either DEC for decimal or
 *             HEX for hexidecimal
 * @param buf the buffer to write the string to
 * @param size the size of the buffer
 * @returns number of characters written to the buffer
 *          not including the null terminator (i.e. size of the string)
 **/
static int simple_utoa(uint32_t val, uint8_t base, char *buf, int size)
{
    char tmpbuf[16];
    int ind=0;
    uint32_t nval;
    int fsize=0;

    if (base == DEC) {
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
    bool gotX = false;

    while ((ch=*buf++) != 0) {
	if (ch >= '0' && ch <= '9') {
	    res = (res << 4) + ch - '0';
	} else if (ch >= 'a' && ch <= 'f') {
	    res = (res << 4) + ch - 'a' + 10;
	} else if (ch >= 'A' && ch <= 'F') {
	    res = (res << 4) + ch - 'A' + 10;
	} else if ((ch == 'x') && !gotX) {
	    /* Ignore 0x at start */
	    gotX = true;
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

/**
 * Convert an IPAdress to an ASCIIZ string
 * @param addr - the IP Address to convert
 * @param buf - the buffer to write the result to
 * @param size - the size of the buffer
 * @returns pointer to the result
 */
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

/**
 * Convert a dotquad IP address string to an IPAddress.
 * E.g. "192.168.1.100" -> { 192, 168, 1, 100 }.
 * @param buf - the string to convert
 * @returns the IPAddress form of the string
 */
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

    lastPort = 0;
    lastHost[0] = 0;

    if (!setopt(PSTR("set u m 1"), (char *)NULL)) {
	debug.println(F("Failed to turn off echo"));
    }
    if (!setopt(PSTR("set sys printlvl 0"), (char *)NULL)) {
	debug.println(F("Failed to turn off sys print"));
    }
    if (!setopt(PSTR("set comm remote 0"), (char *)NULL)) {
	debug.println(F("Failed to set comm remote"));
    }

    /* update connection status */
    getConnection();

    DPRINT(F("tcp status: ")); DPRINT(status.tcp); DPRINT("\n\r");
    DPRINT(F("assoc status: ")); DPRINT(status.assoc); DPRINT("\n\r");
    DPRINT(F("authen status: ")); DPRINT(status.authen); DPRINT("\n\r");
    DPRINT(F("dns status: ")); DPRINT(status.dnsServer); DPRINT("\n\r");
    DPRINT(F("dns found status: ")); DPRINT(status.dnsFound); DPRINT("\n\r");
    DPRINT(F("channel status: ")); DPRINT(status.channel); DPRINT("\n\r");

    dhcpMode = getDHCPMode();
    dhcp = !((dhcpMode == WIFLY_DHCP_MODE_OFF) || (dhcpMode == WIFLY_DHCP_MODE_SERVER));

    replaceChar = getSpaceReplace();
}

/**
 * Start the WiFly device, set it up to handle commands, obtain
 * some initial status (TCP connection status, WiFi association, etc).
 * @param serialdev - the serial stream to use to talk to the WiFly.
 * @param debugPrint - optional debug stream for errors and status.
 * @retval true - WiFly ready for use
 * @retval false - failed to initialise WiFly
 */
boolean WiFly::begin(Stream *serialdev, Stream *debugPrint)
{
    debug.begin(debugPrint);
    serial = serialdev;

    if (!enterCommandMode()) {
	debug.println(F("Failed to enter command mode"));
	return false;
    }

    init();

    if (!exitCommandMode()) {
	debug.println(F("Failed to exit command mode"));
	return false;
    }

    return true;
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

/**
 * Flush the incoming data from the WiFly.
 * @param timeout - the number of milliseconds to wait for additional data to flush. Default is 500msecs.
 */
void WiFly::flushRx(int timeout)
{
    char ch;
    DPRINT(F("flush\n\r"));
    while (readTimeout(&ch,timeout));
    DPRINT(F("flushed\n\r"));
}

/**
 * Write a byte to the WiFly.
 * @param byte - the byte to write.
 * @return the number of bytes written (1).
 */
size_t WiFly::write(uint8_t byte)
{
    if (dbgInd < dbgMax) {
	dbgBuf[dbgInd++] = byte;
    }
    return serial->write(byte);
}

/* Read-ahead for checking for TCP stream close 
 * A circular buffer is used to keep read-ahead bytes and
 * feed them back to the user.
 */
static char peekBuf[8];
static uint8_t peekHead = 0;	/* head of buffer; new characters stored here */
static uint8_t peekTail = 0;	/* Tail of buffer; characters read from here */
static uint8_t peekCount = 0;	/* Number of characters in peek buffer */

/**
 * Return the next byte that a read() would return, but leave the
 * byte in the receive buffer.
 * @returns the next byte that would be read 
 * @retval -1 - no data in receive buffer
 */
int WiFly::peek()
{
    if (peekCount == 0) {
       return serial->peek();
    } else {
       return peekBuf[peekTail];
    }
}

/** Check for a state change on the stream
 * @param str progmem string to check stream input for
 * @param peeked true if the caller peeked the first char of the string,
 *               false if the caller read the character already
 * @retval true - the state change was matched
 * @retval false - state change not matched
 * @note A side effect of this function is that it will store
 *       the unmatched string in the read-ahead peek buffer since
 *       it has to read the characters from the WiFly to check for 
 *       the match.  The peek buffer is used to feed those characters
 *       to the user ahead of reading any more characters from the WiFly.
 */
boolean WiFly::checkStream(const prog_char *str, boolean peeked)
{
    char next;

#ifdef DEBUG
    debug.print(F("checkStream: "));
    debug.println((const __FlashStringHelper *)str);
#endif

    if (peekCount > 0) {
	uint8_t ind=0;
	if (peeked) {
	    str++;
	    ind = 1;
	}
	for (; ind<peekCount; ind++) {
	    uint8_t pind = peekTail + ind;
	    next = pgm_read_byte(str++);

	    if (next == '\0') {
		/* Done - got a match */
		peekCount = 0; // discard peeked bytes
		peekTail = 0;
		peekHead = 0;
		return true;
	    }

	    if (pind > sizeof(peekBuf)) {
		pind = 0;
	    }
	    if (peekBuf[pind] != next) {
		/* Not a match */
		return false;
	    }
	    /* peeked characters match */
	}

	/* string matched so far, keep reading */
    } else if (!peeked) {
	/* Already read and matched the first character before being called */
	str++;
    }

    next = pgm_read_byte(str++);
    while (readTimeout(&peekBuf[peekHead]),50) {
	if (peekBuf[peekHead] != next) {
	    if (++peekHead >= sizeof(peekBuf)) {
		peekHead = 0;
	    }
	    peekCount++;
	    if (peekCount > sizeof(peekBuf)) {
		debug.println(F("ERROR peek.1 buffer overlow"));
	    }
	    break;
	}
	if (++peekHead >= sizeof(peekBuf)) {
	    peekHead = 0;
	}
	peekCount++;
	if (peekCount > sizeof(peekBuf)) {
	    debug.println(F("ERROR peek.2 buffer overlow"));
	}
	next = pgm_read_byte(str++);
	if (next == '\0') {
	    /* Done - got a match */
	    peekCount = 0; // discard peeked bytes
	    peekTail = 0;
	    peekHead = 0;
	    return true;
	}
    }

    return false;
}

/** Check for stream close, if its closed
 * we will quickly receive *CLOS* from the WiFly
 * @param peeked - set to true if first character of *CLOS* was peeked, or false if it has been read.
 * @retval true - stream closed
 * @retval false - stream not closed
 */
boolean WiFly::checkClose(boolean peeked)
{
    if (checkStream(PSTR("*CLOS*"), peeked)) {
	connected = false;
	DPRINTLN(F("Stream closed"));
	return true;
    }
    return false;
}

/** Check for stream open.
 * @param peeked - set to true if first character of *OPEN* was peeked, or false if it has been read.
 * @retval true - stream opened
 * @retval false - stream not opened
 */
boolean WiFly::checkOpen(boolean peeked)
{
    if (checkStream(PSTR("*OPEN*"), peeked)) {
	connected = true;
	DPRINTLN(F("Stream opened"));
	return true;
    }
    return false;
}

/** Read the next byte from the WiFly.
 * @returns the byte read
 * @retval -1 - nothing in the receive buffer to read
 */
int WiFly::read()
{
    int data = -1;

    /* Any data in peek buffer? */
    if (peekCount) {
	data = (uint8_t)peekBuf[peekTail++];
	if (peekTail >= sizeof(peekBuf)) {
	    peekTail = 0;
	}
	peekCount--;
    } else {
	data = serial->read();
	/* TCP connected? Check for close */
	if (connected && data == '*') {
	    if (checkClose(false)) {
		return -1;
	    } else {
		data = (uint8_t)peekBuf[peekTail++];
		if (peekTail >= sizeof(peekBuf)) {
		    peekTail = 0;
		}
		peekCount--;
	    }
	}
    }

    return data;
}


/** Check to see if data is available to be read.
 * @returns the number of bytes that are available to read.
 * @retval 0 - no data available
 * @retval -1 - active TCP connection was closed,
 */
int WiFly::available()
{
    int count;

    count = serial->available();
    if (count > 0) {
	if (debugOn) {
	    debug.print(F("available: peek = "));
	    debug.println((char)serial->peek());
	}
	/* Check for TCP stream closure */
	if (serial->peek() == '*') {
	    if (connected) {
		if (checkClose(true)) {
		    return -1;
		} else {
		    return peekCount + serial->available();
		}
	    } else {
		checkOpen(true);
		return peekCount + serial->available();
	    }
	}
    }

    return count+peekCount;
}

void WiFly::flush()
{
   serial->flush();
}
  

/** Hex dump a string */
void WiFly::dump(const char *str)
{
    while (*str) {
	debug.print(*str,HEX);
	debug.print(' ');
	str++;
    }
    debug.println();
}

/** Send a string to the WiFly */
void WiFly::send(const char *str)
{
    DPRINT(F("send: ")); DPRINT(str); DPRINT("\n\r");
    print(str);
    //serial->print(str);
}

/** Send a character to the WiFly */
void WiFly::send(const char ch)
{
    write(ch);
    //serial->write(ch);
}

/** Send a string from PROGMEM to the WiFly */
void WiFly::send_P(const prog_char *str)
{
    DPRINT(F("send_P: "));
    DPRINTLN((const __FlashStringHelper *)str);

    print((const __FlashStringHelper *)str);
}

/**
 * Start a capture of all the characters recevied from the WiFly.
 * @param size - the size of the capture buffer. This will be malloced.
 */
void WiFly::dbgBegin(int size)
{
    if (dbgBuf != NULL) {
	free(dbgBuf);
    }
    dbgBuf = (char *)malloc(size);
    dbgInd = 0;
    dbgMax = size;
}

/** Stop debug capture and free buffer */
void WiFly::dbgEnd()
{
    if (dbgBuf != NULL) {
	free(dbgBuf);
	dbgBuf = NULL;
    }
    dbgInd = 0;
    dbgMax = 0;
}

/** Do a hex and ASCII dump of the capture buffer, and free the buffer.  */
void WiFly::dbgDump()
{
    int ind;

    if (dbgBuf == NULL) {
	return;
    }

    if (dbgInd > 0) {
	debug.println(F("debug dump"));
	for (ind=0; ind<dbgInd; ind++) {
	    debug.print(ind);
	    debug.print(F(": "));
	    debug.print(dbgBuf[ind],HEX);
	    if (isprint(dbgBuf[ind])) {
		debug.print(' ');
		debug.print(dbgBuf[ind]);
	    }
	    debug.println();
	}
    }
    free(dbgBuf);
    dbgBuf = NULL;
    dbgMax = 0;
}

/** Read the next character from the WiFly serial interface.
 * Waits up to timeout milliseconds to receive the character.
 * @param chp pointer to store the read character in
 * @param timeout the number of milliseconds to wait for a character
 * @retval true - character read
 * @retval false - timeout reached, character not read
 */
boolean WiFly::readTimeout(char *chp, uint16_t timeout)
{
    uint32_t start = millis();
    char ch;

    static int ind=0;

    while (millis() - start < timeout) {
	if (serial->available() > 0) {
	    ch = serial->read();
	    *chp = ch;
	    if (dbgInd < dbgMax) {
		dbgBuf[dbgInd++] = ch;
	    }
	    if (debugOn) {
		debug.print(ind++);
		debug.print(F(": "));
		debug.print(ch,HEX);
		if (isprint(ch)) {
		    debug.print(' ');
		    debug.print(ch);
		}
		debug.println();
	    }
	    return true;
	}
    }

    if (debugOn) {
	debug.println(F("readTimeout - timed out"));
    }

    return false;
}

static char prompt[16];
static boolean gotPrompt = false;

/** Scan the input data for the WiFLy prompt.  This is a string starting with a '<' and
 * ending with a '>'. Store the prompt for future use.
 */
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
				DPRINT(F("setPrompt: ")); DPRINT(prompt); DPRINT("\n\r");
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

/** See if the prompt is somewhere in the string */
boolean WiFly::checkPrompt(const char *str)
{
    if (strstr(str, prompt) != NULL) {
	return true;
    } else {
	return false;
    }
}

/**
 * Read characters from the WiFly and match them against the
 * string. Ignore any leading characters that don't match. Keep
 * reading, discarding the input, until the string is matched
 * or until no characters are received for the timeout duration.
 * @param str The string to match
 * @param timeout fail if no data received for this period (in milliseconds).
 * @retval true - a match was found
 * @retval false - no match found, timeout reached
 */
boolean WiFly::match(const char *str, uint16_t timeout)
{
    const char *match = str;
    char ch;

#ifdef DEBUG
    if (debugOn) {
	debug.print(F("match: "));
	debug.println(str);
    }
#endif

    if ((match == NULL) || (*match == '\0')) {
	return true;
    }

    /* find first character */
    while (readTimeout(&ch,timeout)) {
	if (ch == *match) {
	    match++;
	} else {
	    match = str;
	    if (ch == *match) {
		match++;
	    }
	}
	if (*match == '\0') {
	    DPRINT(F("match: true\n\r"));
	    return true;
	}
    }

    DPRINT(F("match: false\n\r"));
    return false;
}

/**
 * Read characters from the WiFly and match them against the
 * progmem string. Ignore any leading characters that don't match. Keep
 * reading, discarding the input, until the string is matched
 * or until no characters are received for the timeout duration.
 * @param str The string to match, in progmem.
 * @param timeout fail if no data received for this period (in milliseconds).
 * @retval true - a match was found
 * @retval false - no match found, timeout reached
 */
boolean WiFly::match_P(const prog_char *str, uint16_t timeout)
{
    const prog_char *match = str;
    char ch, ch_P;

    if (debugOn) {
	debug.print(F("match_P: "));
	debug.println((const __FlashStringHelper *)str);
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
	    if (ch == pgm_read_byte(match)) {
		match++;
	    }
	}

	ch_P = pgm_read_byte(match);
	if (ch_P == '\0') {
	    DPRINT(F("match_P: true\n\r"));
	    return true;
	}
    }

    DPRINT(F("match_P: false\n\r"));
    return false;
}

/**
 * Read characters from the WiFly and match them against the set of
 * progmem strings. Ignore any leading characters that don't match. Keep
 * reading, discarding the input, until one of the strings is matched
 * or until no characters are received for the timeout duration.<br>
 * Example: res = multiMatch_P(500, 3, F("first="), F("second="), F("closed"));<br>
 * Will return 0 if "first=" is matched, 1 if "second=" is matched, 2 if "closed" is
 * matched, or -1 if nothing is matched and no data is received for 500 milliseconds.
 * @param timeout - fail if no data received for this period (in milliseconds).
 * @param count - the number of strings in the str array
 * @param ... A list of count progmem strings to match
 * @returns the index of the matching string
 * @retval -1 - no match found, timeout reached
 */
int WiFly::multiMatch_P(uint16_t timeout, uint8_t count, ...)
{
    const prog_char *str[20];
    int ind;
    va_list ap;
    va_start(ap, count);

    if (count > 20) {
	count = 20;
    }

    for (ind=0; ind<count; ind++) {
	str[ind] = va_arg(ap, const prog_char *);
    }
    va_end(ap);

    return multiMatch_P(str, count, timeout);
}

/**
 * Read characters from the WiFly and match them against the set of
 * progmem strings. Ignore any leading characters that don't match. Keep
 * reading, discarding the input, until one of the strings is matched
 * or until no characters are received for the timeout duration.
 * @param str - the array of progmem strings to match
 * @param count - the number of strings in the str array
 * @param timeout - fail if no data received for this period (in milliseconds).
 * @returns the index of the matching string
 * @retval -1 - no match found, timeout reached
 */
int8_t WiFly::multiMatch_P(const prog_char *str[], uint8_t count, uint16_t timeout)
{
    struct {
	bool active;
	const prog_char *str;
    } match[count];
    char ch, ch_P;
    uint8_t ind;

    for (ind=0; ind<count; ind++) {
	match[ind].active = false;
	match[ind].str = str[ind];
	if (debugOn) {
	    debug.print(F("multiMatch_P: "));
	    debug.print(ind);
	    debug.print(' ');
	    debug.println((const __FlashStringHelper *)str[ind]);
	}
    }

    while (readTimeout(&ch,timeout)) {
	for (ind=0; ind<count; ind++) {
	    ch_P = pgm_read_byte(match[ind].str);
	    if (ch == ch_P) {
		match[ind].str++;
	    } else {
		/* Restart match */
		match[ind].str = str[ind];
		/* unmatched character might be the start of the string */
		if (ch == pgm_read_byte(match[ind].str)) {
		    match[ind].str++;
		}
	    }

	    if (pgm_read_byte(match[ind].str) == '\0') {
		/* Got a match */
		DPRINTLN(F("multiMatch_P: true"));
		return ind;
	    }
	}
    }

    DPRINTLN(F("multiMatch_P: failed"));
    return -1;
}

/**
 * Read characters from the WiFly and match them against the
 * flash string. Ignore any leading characters that don't match. Keep
 * reading, discarding the input, until the string is matched
 * or until no characters are received for the timeout duration.
 * @param str The string to match (in flash memory)
 * @timeout fail if no data received for this period (in milliseconds).
 * @retval true - a match was found
 * @retval false - no match found, timeout reached
 */
boolean WiFly::match(const __FlashStringHelper *str, uint16_t timeout)
{
    return match_P((const prog_char *)str, timeout);
}

/**
 * Read characters from the WiFly until the prompt string is seen.
 * Characters are discarded until the prompt is found.
 * Fails if no data is received for the timeout duration.
 * @timeout fail if no data received for this period (in milliseconds).
 * @retval true - prompt found
 * @retval false - prompt not found, timeout reached
 * @Note: The first time this function is called it will set the prompt string.
 */
boolean WiFly::getPrompt(uint16_t timeout)
{
    boolean res;

    if (!gotPrompt) {
	DPRINT(F("setPrompt\n\r"));

	res = setPrompt();
	if (!res) {
	    debug.println(F("setPrompt failed"));
	}
    } else {
	DPRINT(F("getPrompt \"")); DPRINT(prompt); DPRINT("\"\n\r");
	res = match(prompt, timeout);
    }
    return res;
}

/** Put the WiFly into command mode */
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
    DPRINT(F("Check in command mode\n\r"));
    serial->write('\r');
    if (getPrompt()) {
	inCommandMode = true;
	DPRINT(F("Already in command mode\n\r"));
	return true;
    }

    for (retry=0; retry<5; retry++) {
	DPRINT(F("send $$$ ")); DPRINT(retry); DPRINT("\n\r");
	delay(250);
	send_P(PSTR("$$$"));
	delay(250);
	if (match_P(PSTR("CMD\r\n"), 500)) {
	    inCommandMode = true;
	    return true;
	}
    }

    return false;
}

/** Take the WiFly out of command mode */
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
	debug.println(F("Failed to exit\n\r"));
	return false;
    }
}

int WiFly::getsTerm(char *buf, int size, char term, uint16_t timeout)
{
    char ch;
    int ind=0;

    DPRINTLN(F("getsTerm:"));

    while (readTimeout(&ch, timeout)) {
	if (ch == term) {
	    if (buf) {
		buf[ind] = 0;
	    }
	    return ind;
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

/**
 * Read characters into the buffer until a carriage-return and newline is reached.
 * If the buffer is too small, the remaining characters in the line are discarded.
 * @param buf - the buffer to read into. If this is NULL then all characters in the line are discarded.
 * @param size - the size of the buffer (max number of characters it can store)
 * @param timeout - the number of milliseconds to wait for a new character to arrive
 * @returns the number of characters read into the buffer.
 * @retval 0 - timeout reading newline
 * @note The buffer will be null terminated, and in effect can hold size-1 characters.
 */
int WiFly::gets(char *buf, int size, uint16_t timeout)
{
    char ch;
    int ind=0;

    DPRINTLN(F("gets:"));

    while (readTimeout(&ch, timeout)) {
	if (ch == '\r') {
	    readTimeout(&ch, timeout);
	    if (ch == '\n') {
		if (buf) {
		    buf[ind] = 0;
		}
		return ind;
	    }
	    if (buf) {
		if (ind < (size-2)) {
		    buf[ind++] = '\r';
		    buf[ind++] = ch;
		} else if (ind < (size - 1)) {
		    buf[ind++] = '\r';
		}
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
	DPRINT(F("Already in command mode\n\r"));
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
	debug.println(F("getCon: failed to start"));
	return 0;
    }
    //dbgBegin(256);

    DPRINT(F("getCon\n\r"));
    DPRINT(F("show c\n\r"));
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

    if (status.tcp == WIFLY_TCP_CONNECTED) {
	connected = true;
	if (debugOn) debug.println(F("getCon: TCP connected"));
    } else {
	connected = false;
	if (debugOn) debug.println(F("getCon: TCP disconnected"));
    }

    return res;
}

/** Get local IP address */
char *WiFly::getIP(char *buf, int size)
{
    char *chp = buf;
    if (getopt(WIFLY_GET_IP, buf, size)) {
	/* Trim off port */
	while (*chp && *chp != ':') {
	    chp++;
	}
    }
    *chp = '\0';

    return buf;
}

/** Get local port */
uint16_t WiFly::getPort()
{
    char buf[22];
    uint8_t ind;

    if (getopt(WIFLY_GET_IP, buf, sizeof(buf))) {
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

/** Get remote IP address */
char *WiFly::getHostIP(char *buf, int size)
{
    char *chp = buf;
    if (getopt(WIFLY_GET_HOST, buf, size)) {
	/* Trim off port */
	while (*chp && *chp != ':') {
	    chp++;
	}
    }
    *chp = '\0';

    return buf;
}

/** Get remote port */
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

uint8_t WiFly::getJoin()
{
    return getopt(WIFLY_GET_JOIN);
}

char *WiFly::getDeviceID(char *buf, int size)
{
    return getopt(WIFLY_GET_DEVICEID, buf, size);
}

uint32_t WiFly::getopt(int opt, uint8_t base)
{
    char buf[11];

    if (!getopt(opt, buf, sizeof(buf))) {
	return 0;
    }

    if (base == DEC) {
	return atou(buf);
    } else {
	return atoh(buf);
    }
}

uint8_t WiFly::getIpFlags()
{
    return getopt(WIFLY_GET_IP_FLAGS, HEX);
}

uint32_t WiFly::getBaud()
{
    return getopt(WIFLY_GET_BAUD);
}

/**
 * This command puts the module to sleep. You can wake
 * the module by sending characters over the UART or by
 * using the wake timer (supplied in seconds).
 */
boolean WiFly::sleep(uint16_t seconds)
{
    if (seconds != NULL) {
        if(!setopt(PSTR("set sys wake"), seconds)) {
            return false;
        }
    }
    if (!startCommand()) {
        return false;
    }
    send_P(PSTR("sleep\r"));
    inCommandMode = false;
    return true;
}

/**
 * This command sets the real-time clock by synchronizing
 * with the time server specified with the time server
 * (set time) parameters. This command sends a UDP time
 * server request packet.
 */
boolean WiFly::time()
{
    if (!startCommand()) {
        return false;
    }
    send_P(PSTR("time\r"));
    finishCommand();
    return true;
}

char *WiFly::getTime(char *buf, int size)
{
    return getopt(WIFLY_GET_TIME, buf, size);
}

uint32_t WiFly::getRTC()
{
    return getopt(WIFLY_GET_RTC);
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

	if (match(hostname, 5000)) {
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
    return getopt(WIFLY_GET_UPTIME);
}

uint8_t WiFly::getTimezone()
{
    return getopt(WIFLY_GET_ZONE);
}

uint8_t WiFly::getUartMode()
{
    return getopt(WIFLY_GET_UART_MODE, HEX);
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

const static struct {
    uint8_t protocol;
    char name[6];
} protmap[] __attribute__((__progmem__)) = {
    { WIFLY_PROTOCOL_UDP,	 "UDP," },
    { WIFLY_PROTOCOL_TCP, 	 "TCP," },
    { WIFLY_PROTOCOL_SECURE, 	 "SECUR" },
    { WIFLY_PROTOCOL_TCP_CLIENT, "TCP_C" },
    { WIFLY_PROTOCOL_HTTP, 	 "HTTP," },
    { WIFLY_PROTOCOL_RAW, 	 "RAW," },
    { WIFLY_PROTOCOL_SMTP, 	 "SMTP," }
};

uint8_t WiFly::getProtocol()
{
    char buf[50];
    int8_t prot=0;

    if (!getopt(WIFLY_GET_PROTOCOL, buf, sizeof(buf))) {
	return -1;
    }

    for (uint8_t ind=0; ind < (sizeof(protmap)/sizeof(protmap[0])); ind++) {
	  if (strstr_P(buf, protmap[ind].name) != NULL) {
	      prot |= pgm_read_byte(&protmap[ind].protocol);
	  }
    }

    return prot;
}

uint16_t WiFly::getFlushTimeout()
{
    return getopt(WIFLY_GET_FLUSHTIMEOUT);
}

uint16_t WiFly::getFlushSize()
{
    return getopt(WIFLY_GET_FLUSHSIZE);
}

uint8_t WiFly::getFlushChar()
{
    return getopt(WIFLY_GET_FLUSHCHAR, HEX);
}

int8_t WiFly::getRSSI()
{
    return -(int8_t)getopt(WIFLY_GET_RSSI);
}


const prog_char res_AOK[] PROGMEM = "AOK\r\n";
const prog_char res_ERR[] PROGMEM = "ERR: ";

/* Get the result from a set operation
 * Should be AOK or ERR
 */
boolean WiFly::getres(char *buf, int size)
{
    const prog_char *setResult[] = {
	PSTR("ERR: "),
	PSTR("AOK\r\n")
    };
    int8_t res;

    DPRINTLN(F("getres"));

    res = multiMatch_P(setResult, 2);

    if (res == 1) {
	return true;
    } else if (res == 0) {
	gets(buf, size);
	debug.print(F("ERR: "));
	debug.println(buf);
    } else {
	/* timeout */
	DPRINTLN(F("timeout"));
	strncpy_P(buf, PSTR("<timeout>"), size);
    }
    return false;
}

/** 
 * Set an option to an unsigned integer value
 * @param opt the set command to use (in Flash)
 * @param value the value to set
 * @param base the number format; DEC or HEX
 * @returns true on success, false on failure
 */
boolean WiFly::setopt(const prog_char *opt, const uint32_t value, uint8_t base)
{
    char buf[11];
    simple_utoa(value, base, buf, sizeof(buf));
    return setopt(opt, buf);
}


/**
 * Set an option, confirm ok status
 * The value to set can either be in RAM or in Flash.
 * @param cmd the set command to use (string in Flash)
 * @param buf the value to set the option to (string in RAM). Set to NULL if not used.
 * @param buf_p the value to set the option to (string in Flash). Set to NULL if not used.
 * @param spacesub set to true to have spaces replaced with the 
 *        current replacement character
 * @returns true on success, false on failure
 */
boolean WiFly::setopt(const prog_char *cmd, const char *buf, const __FlashStringHelper *buf_P, bool spacesub)
{
    char rbuf[16];
    boolean res;

    if (!startCommand()) {
	return false;
    }

    send_P(cmd);
    if (buf_P != NULL) {
	send(' ');
	if (spacesub) {
	    const prog_char *str = (const prog_char *)buf_P;
	    char ch;
	    while ((ch = pgm_read_byte(str++)) != 0) {
		if (ch == ' ') {
		    write(replaceChar);
		} else {
		    write(ch);
		}
	    }
	} else {
	    send_P((const prog_char *)buf_P);
	}
    } else if (buf != NULL) {
	send(' ');
	if (spacesub) {
	    const char *str = buf;
	    char ch;
	    /* spaces must be replaced */
	    while ((ch = *str++) != 0) {
		if (ch == ' ') {
		    write(replaceChar);
		} else {
		    write(ch);
		}
	    }
	} else {
	    send(buf);
        }
    }
    send('\r');

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

/** Reboots the WiFly.
 * @note Depending on the shield, this may also reboot the Arduino.
 */
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

/** Restore factory default settings */
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

boolean WiFly::setDeviceID(const __FlashStringHelper *buf)
{
    return setopt(PSTR("set o d"), NULL, buf);
}

boolean WiFly::setDeviceID(const char *buf)
{
    return setopt(PSTR("set o d"), buf);
}

bool WiFly::setJoin(uint8_t join)
{
    return setopt(PSTR("set wlan join"), join);
}

/** 
 * Set the WiFly's IP address.
 * @param buf IPv4 address in a null-terminated string.
 *            e.g "192.168.1.10"
 * @retval true address set
 * @retval false failed to set address
 */
boolean WiFly::setIP(const char *buf)
{
    return setopt(PSTR("set ip address"), buf);
}

/** 
 * Set the WiFly's IP address.
 * @param buf IPv4 address in a null-terminated flash string.
 *            e.g F("192.168.1.10")
 * @retval true address set
 * @retval false failed to set address
 */
boolean WiFly::setIP(const __FlashStringHelper *buf)
{
    return setopt(PSTR("set ip address"), NULL, buf);
}

/** Set local port */
boolean WiFly::setPort(const uint16_t port)
{
    return setopt(PSTR("set ip localport"), port);
}

boolean WiFly::setHostIP(const __FlashStringHelper *buf)
{
    return setopt(PSTR("set ip host"), NULL, buf);
}

boolean WiFly::setHostIP(const char *buf)
{
    return setopt(PSTR("set ip host"), buf);
}

boolean WiFly::setHostPort(const uint16_t port)
{
    return setopt(PSTR("set ip remote"), port);
}

boolean WiFly::setHost(const char *buf, uint16_t port)
{
    bool res;

    if (!startCommand()) {
	debug.println(F("SetHost: failed to start command"));
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

boolean WiFly::setNetmask(const __FlashStringHelper *buf)
{
    return setopt(PSTR("set ip netmask"), NULL, buf);
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

boolean WiFly::setProtocol(const uint8_t protocol)
{
    return setopt(PSTR("set ip protocol"), protocol, HEX);
}

boolean WiFly::setIpProtocol(const uint8_t protocol)
{
    return setProtocol(protocol);
}

boolean WiFly::setIpFlags(const uint8_t flags)
{
    return setopt(PSTR("set ip flags"), flags, HEX);
}

/** Set NTP server IP address */
boolean WiFly::setTimeAddress(const char *buf)
{
    return setopt(PSTR("set time address"), buf);
}

/** Set NTP server port */
boolean WiFly::setTimePort(const uint16_t port)
{
    return setopt(PSTR("set time port"), port);
}

/** Set timezone for calculating local time based on NTP time. */
boolean WiFly::setTimezone(const uint8_t zone)
{
    return setopt(PSTR("set time zone"), zone);
}

/** Set the NTP update period */
boolean WiFly::setTimeEnable(const uint16_t period)
{
    return setopt(PSTR("set time enable"), period);
}

boolean WiFly::setUartMode(const uint8_t mode)
{
    /* Always set NOECHO, need to keep echo off for library to function correctly */
    return setopt(PSTR("set uart mode"), mode | WIFLY_UART_MODE_NOECHO, HEX);
}

/** Set the UDP broadcast time interval.
 * @param seconds the number of seconds between broadcasts.
 *                Set this to zero to disable broadcasts.
 * @return true if sucessful, else false.
 */
boolean WiFly::setBroadcastInterval(const uint8_t seconds)
{
    return setopt(PSTR("set broadcast interval"), seconds, HEX);
}

/**
 * Enable the UDP auto-pair functionality.
 * The WiFly will automatically set the Host IP and port
 * to match the sender of the last UDP packet.
 */
boolean WiFly::enableUdpAutoPair()
{
   udpAutoPair = true;
   setHostIP(F("0.0.0.0"));
   setIpFlags(getIpFlags() | WIFLY_FLAG_UDP_AUTO_PAIR);
   disableHostRestore();

   return true;
}

boolean WiFly::disableUdpAutoPair()
{
   udpAutoPair = false;
   setIpFlags(getIpFlags() & ~WIFLY_FLAG_UDP_AUTO_PAIR);

   return true;
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
    return setopt(PSTR("set comm time"), timeout);
}

/** Set the comms flush character. 0 disables the feature.
 * A packet will be sent whenever this character is sent
 * to the WiFly. Used for auto connect mode or UDP packet sending.
 * @param flushChar send a packet when this character is sent.
 *        Set to 0 to disable character based flush.
 */
boolean WiFly::setFlushChar(const char flushChar)
{
    return setopt(PSTR("set comm match"), (uint8_t)flushChar, HEX);
}

/** Set the comms flush size.
 * A packet will be sent whenever this many characters are sent.
 * @param size number of characters to buffer before sending a packet
 */
boolean WiFly::setFlushSize(uint16_t size)
{
    if (size > 1460) {
	/* Maximum size */
	size = 1460;
    }

    return setopt(PSTR("set comm size"), size);
}

/** Set the WiFly IO function option */
boolean WiFly::setIOFunc(const uint8_t func)
{
    return setopt(PSTR("set sys iofunc"), func, HEX);
}

/**
 * Enable data trigger mode.  This mode will automatically send a new packet based on several conditions:
 * 1. If no characters are sent to the WiFly for at least the flushTimeout period.
 * 2. If the character defined by flushChar is sent to the WiFly.
 * 3. If the number of characters sent to the WiFly reaches flushSize.
 * @param flushTimeout Send a packet if no more characters are sent within this many milliseconds. Set
 *                     to 0 to disable.
 * @param flushChar Send a packet when this character is sent to the WiFly. Set to 0 to disable.
 * @param flushSize Send a packet when this many characters have been sent to the WiFly.
 * @returns true on success, else false.
 * @note as of 2.32 firmware, the flushTimeout parameter does not take affect until after a save and reboot.
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
    return setopt(PSTR("set wlan hide 1"), (char *)NULL);
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
    return setopt(PSTR("set wlan ssid"), buf, NULL, true);
}

/** Set the WiFi channel.
 * @param channel the wifi channel from 0 to 13. 0 means auto channel scan.
 * @returns true if successful, else false.
 */
boolean WiFly::setChannel(uint8_t channel)
{
    if (channel > 13) {
	channel = 13;
    }
    return setopt(PSTR("set wlan chan"), channel);
}

/** Set WEP key */
boolean WiFly::setKey(const char *buf)
{
    boolean res;

    if ((buf[1] == 'x') || (buf[1] == 'X')) {
	/* Skip over the 0x leader */
	buf += 2;
    }

    res = setopt(PSTR("set wlan key"), buf);

    hide();	/* hide the key */
    return res;
}


/**
 * Set WPA passphrase.
 * Spaces are automatically replaced with the current space substitution 
 * character ('$' is the default).
 * @Note: If your passphrase contains a '$' then use setSpaceReplace() to
 *        change the replacement character to something you're not using.
 */
boolean WiFly::setPassphrase(const char *buf)
{
    boolean res;
    res = setopt(PSTR("set wlan phrase"), buf, NULL, true);

    hide();	/* hide the key */
    return res;
}

/**
 * Set the space replacement character in WPA passphrase.
 * Default is '$'.
 */
boolean WiFly::setSpaceReplace(char ch)
{
    char buf[2] = { ch, 0 };

    replaceChar = ch;
    return setopt(PSTR("set opt replace"), buf);
}

char WiFly::getSpaceReplace(void)
{
    return (char)getopt(WIFLY_GET_REPLACE, HEX);
}

/* data rates to register setting */
const static struct {
    uint32_t rate;
    uint8_t setting;
} rateMap[] __attribute__((__progmem__)) = {
    {  1000000, WIFLY_RATE_1MBPS   },
    {  2000000, WIFLY_RATE_2MBPS   },
    {  5500000, WIFLY_RATE_5_5MBPS },
    {  6000000, WIFLY_RATE_6MBPS   },
    {  9000000, WIFLY_RATE_9MBPS   },
    { 11000000, WIFLY_RATE_11MBPS  },
    { 12000000, WIFLY_RATE_12MBPS  },
    { 18000000, WIFLY_RATE_18MBPS  },
    { 24000000, WIFLY_RATE_24MBPS  },
    { 36000000, WIFLY_RATE_36MBPS  },
    { 48000000, WIFLY_RATE_48MBPS  },
    { 54000000, WIFLY_RATE_54MBPS  }
};

/**
 * Set WiFi data rate
 * @param rate the data rate to set in bits per second.
 *             valid values are 1000000, 2000000, 5500000, 
 *             6000000, 9000000, 11000000, 12000000, 18000000,
 *             24000000, 36000000, 48000000, 54000000.
 * @returns true on success, false on failure.
 * @note rates are rounded up to the nearest valid value
 */
boolean WiFly::setRate(uint32_t rate)
{
    uint8_t setting = WIFLY_RATE_54MBPS;

    for (uint8_t ind=0; ind < (sizeof(rateMap)/sizeof(rateMap[0])); ind++) {
	if (rate <= pgm_read_dword(&rateMap[ind].rate)) {
	    setting = pgm_read_byte(&rateMap[ind].setting);
	    break;
	}
    }
    return setopt(PSTR("set wlan rate"), setting);
}

/**
 * Return the current WiFi data rate in bits/sec
 * @returns current data rate in bits/sec
 */
uint32_t WiFly::getRate()
{
    uint8_t rate = getopt(WIFLY_GET_RATE);

    debug.print("rate: "); debug.println(rate);

    for (uint8_t ind=0; ind < (sizeof(rateMap)/sizeof(rateMap[0])); ind++) {
	if (rate == pgm_read_byte(&rateMap[ind].setting)) {
	    return pgm_read_dword(&rateMap[ind].rate);
	}
    }

    return 0;	/* Unknown */
}

/**
 * Set the transmit power level.
 * @param dBm power level from 1 to 12 dBm
 * @returns true on success, false on failure.
 * @ Note: a setting of 0 means max power which is 12 dBm.
 */
boolean WiFly::setTxPower(uint8_t dBm)
{
    if (dBm > 12) {
	dBm = 12;
    }
    return setopt(PSTR("set wlan tx"), dBm);
}

/**
 * Get the current transmit power.
 * @returns tx power in dBm
 */
uint8_t WiFly::getTxPower()
{
    uint8_t power = getopt(WIFLY_GET_POWER);
    if (power == 0) {
	/* 0 means max power, or 12 dBm */
	power = 12;
    }
    return power;
}


/**
 * Set the ad hoc beacon period in milliseconds.
 * The beacon is a management frame needed to keep the network alive.
 * Default is 100 milliseconds.
 * @param msecs the number of milliseconds between beacon
 * @returns true on success, false on failure.
 */
boolean WiFly::setAdhocBeacon(const uint16_t msecs)
{
    return setopt(PSTR("set adhoc beacon"), msecs);
}

/**
 * Set the ad hoc network probe period.  When this number of seconds
 * passes since last receiving a beacon then the network is declared lost.
 * Default is 5 seconds..
 * @param secs the number of seconds in the probe period.
 * @returns true on success, false on failure.
 */
boolean WiFly::setAdhocProbe(const uint16_t secs)
{
    return setopt(PSTR("set adhoc probe"), secs);
}

uint16_t WiFly::getAdhocBeacon()
{
    return getopt(WIFLY_GET_BEACON);
}

uint16_t WiFly::getAdhocProbe()
{
    return getopt(WIFLY_GET_PROBE);
}

uint16_t WiFly::getAdhocReboot()
{
    return getopt(WIFLY_GET_REBOOT);
}

/** join a wireless network */
boolean WiFly::join(const char *ssid, uint16_t timeout)
{
    int8_t res;
    const prog_char *joinResult[] = {
	PSTR("FAILED"),
	PSTR("Associated!")
    };

    if (!startCommand()) {
	return false;
    }

    send_P(PSTR("join "));
    if (ssid != NULL) {
	send(ssid);
    }
    send_P(PSTR("\r"));

    res = multiMatch_P(joinResult,2,timeout);
    flushRx(100);
    if (res == 1) {
        status.assoc = 1;
	if (dhcp) {
	    // need some time to complete DHCP request
	    match_P(PSTR("GW="), 15000);
	    flushRx(100);
	}
	gets(NULL,0);
	finishCommand();
	return true;
    }

    finishCommand();
    return false;
}

/** join a wireless network */
boolean WiFly::join(uint16_t timeout)
{
    char ssid[64];
    getSSID(ssid, sizeof(ssid));

    return join(ssid);
}

/**
 * Join a wireless network.
 * @param ssid The SSID of the network to join
 * @param password The WPA passphrase or WEP hex key.
 * @param dhcp set to true to enable DHCP
 * @param mode WIFLY_MODE_WPA for WPA security, or WIFLY_MODE_WEP for WEP security.
 * @param timeout number of milliseconds to try connecting before timing out.
 * @return true on success, false on failure.
 */
boolean WiFly::join(const char *ssid, const char *password, bool dhcp, uint8_t mode, uint16_t timeout)
{
    setSSID(ssid);
    if (mode == WIFLY_MODE_WPA) {
	setPassphrase(password);
    } else {
	setKey(password);
    }

    if (dhcp) {
	enableDHCP();
    }

    return join(ssid, timeout);
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
    getConnection();
    return (status.assoc == 1);
}

boolean WiFly::setBaud(uint32_t baud)
{
    char buf[16];
    simple_utoa(baud, 10, buf, sizeof(buf));
    DPRINT(F("set baud ")); DPRINT(buf); DPRINT("\n\r");

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


/** Send final chunk, end of HTTP message */
void WiFly::sendChunkln()
{
    serial->println('0');
    serial->println();
}

/**
 * Send a string as an HTTP chunk with newline
 * An HTTP chunk is the length of the string in HEX followed
 * by the string.
 * @param str the string to send
 */
void WiFly::sendChunkln(const char *str)
{
    serial->println(strlen(str)+2,HEX);
    serial->println(str);
    serial->println();
}

/**
 * Send a progmame string as an HTTP chunk with newline
 * An HTTP chunk is the length of the string in HEX followed
 * by the string.
 * @param str the string to send
 */
void WiFly::sendChunkln(const __FlashStringHelper *str)
{
    serial->println(strlen_P((const prog_char *)str)+2,HEX);
    serial->println(str);
    serial->println();
}

/**
 * Send a string as an HTTP chunk without a newline
 * An HTTP chunk is the length of the string in HEX followed
 * by the string.
 * @param str the string to send
 */
void WiFly::sendChunk(const char *str)
{
    serial->println(strlen(str),HEX);
    serial->println(str);
}

/**
 * Send a progmem string as an HTTP chunk without a newline.
 * An HTTP chunk is the length of the string in HEX followed
 * by the string.
 * @param str the string to send
 */
void WiFly::sendChunk(const __FlashStringHelper *str)
{
    serial->println(strlen_P((const prog_char *)str),HEX);
    serial->println(str);
}

/**
 * Ping the specified host. Return true if a ping response
 * is received, else false.
 * @param host the host or IP address to ping
 * @retval true - received ping response
 * @retval false - no response from host
 */
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
 * Create an Adhoc WiFi network.
 * The WiFly is assigned IP address 169.254.1.1.
 * @param ssid the SSID to use for the network
 * @param channel the WiFi channel to use; 1 to 13.
 * @retval true - successfully create Ad Hoc network
 * @retval false - failed
 * @note the WiFly is rebooted as the final step of this command.
 */
boolean WiFly::createAdhocNetwork(const char *ssid, uint8_t channel)
{
    startCommand();
    setDHCP(WIFLY_DHCP_MODE_OFF);
    setIP(F("169.254.1.1"));
    setNetmask(F("255.255.0.0"));

    setJoin(WIFLY_WLAN_JOIN_ADHOC);
    setSSID(ssid);
    setChannel(channel);
    save();
    finishCommand();
    reboot();
    return true;
}

/**
 * Open a TCP connection.
 * If there is already an open connection then that is closed first.
 * @param addr - the IP address or hostname to connect. A DNS lookup will be peformed
 *               for the hostname.
 * @param port - the TCP port to connect to
 * @param block - true = wait for the connection to complete
 *                false = start the connection and return. Use openComplete() 
 *                        to determine the result.
 * @retval true - success, the connection is open
 * @retval false - failed, or connection already in progress
 */
boolean WiFly::open(const char *addr, uint16_t port, boolean block)
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
    debug.print(F("open ")); debug.print(addr); debug.print(' '); debug.println(buf);
    send_P(PSTR("open "));
    send(addr);
    send(" ");
    send(buf);
    send_P(PSTR("\r"));

    if (!getPrompt()) {
	debug.println(F("Failed to get prompt"));
	debug.println(F("WiFly has crashed and will reboot..."));
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
		DPRINT(F("Connected\n\r"));
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
		debug.print(F("Failed to connect: ")); debug.println(buf);
		finishCommand();
		return false;
	    }

	default:
	    if (debugOn) {
		debug.print(F("Unexpected char: "));
		debug.print(ch,HEX);
		if (isprint(ch)) {
		    debug.print(' ');
		    debug.print(ch);
		}
		debug.println();
	    }
	    break;
	}
    }

    debug.println(F("<timeout>"));
    finishCommand();
    return false;
}

/**
 * Open a TCP connection.
 * If there is already an open connection then that is closed first.
 * @param addr - the IP address to connect to
 * @param port - the TCP port to connect to
 * @param block - true = wait for the connection to complete
 *                false = start the connection and return. Use openComplete() 
 *                        to determine the result.
 * @retval true - success, the connection is open
 * @retval false - failed, or connection already in progress
 */
boolean WiFly::open(IPAddress addr, uint16_t port, boolean block)
{
    char buf[16];
    return open(iptoa(addr, buf, sizeof(buf)), port, block);
}

/**
 * Check to see if there is a tcp connection.
 * @retval true connected
 * @retval false not connected
 */
boolean WiFly::isConnected()
{
    /* Check for a connect or disconnect */
    available();

    return connected;
}

boolean WiFly::isInCommandMode()
{
    return inCommandMode;
}

/** Internal UPD sendto function */
boolean WiFly::sendto(
    const uint8_t *data,
    uint16_t size,
    const __FlashStringHelper *flashData,
    const char *host,
    uint16_t port)
{
    bool restore = true;

    if (!startCommand()) {
	debug.println(F("sendto: failed to start command"));
	return false;
    }

    if (udpAutoPair || (port != lastPort) || (strcmp(host, lastHost) != 0)) {
	setHost(host,port);
	if (!restoreHost) {
	    /* Keep a copy of this for reference for the next call */
	    lastPort = port;
	    strncpy(lastHost, host, sizeof(lastHost));
	    restore = false;
	}
#ifdef DEBUG
	debug.print(F("sendto: set host and port "));
	debug.print(host); debug.print(':'); debug.println(port);
#endif
    } else {
	/* same host and port, no need to restore */
	restore = false;
#ifdef DEBUG
	debug.print(F("sendto: same host and port "));
	debug.print(host); debug.print(':'); debug.println(port);
#endif
    }

    finishCommand();

    if (data) {
	write(data, size);
    } else if (flashData) {
	print(flashData);
    }

    /* Restore original host and port */
    if (restore) {
	setHost(lastHost,lastPort);
#ifdef DEBUG
	debug.print(F("sendto: restored "));
	debug.print(lastHost); debug.print(':'); debug.println(lastPort);
#endif
    } else if (udpAutoPair) {
       setHostIP(F("0.0.0.0"));
    }

#ifdef DEBUG
    debug.print(F("sendto: send "));
    if (data) {
	debug.println((char *)data);
    } else if (flashData) {
	debug.println(flashData);
    }
#endif

    return true;
}

/**
 * Send binary data as a UDP packet to a host.
 * @param data - pointer to an array of data to send
 * @param size - then number of bytes of data to send
 * @param host - the IP or hostname to send the packet to. If this is a hostname 
 *               then a DNS lookup will be performed to find the IP address.
 * @param port - the UDP port to send the packet to
 * @retval true - packet send successfully
 * @retval false - failed to send packet
 */
boolean WiFly::sendto(const uint8_t *data, uint16_t size, const char *host, uint16_t port)
{
    return sendto(data, size, NULL, host, port);
}

/**
 * Send binary data as a UDP packet to a host.
 * @param data - pointer to an array of data to send
 * @param size - then number of bytes of data to send
 * @param host - the IP address to send the packet to.
 * @param port - the UDP port to send the packet to
 * @retval true - packet send successfully
 * @retval false - failed to send packet
 */
boolean WiFly::sendto(const uint8_t *data, uint16_t size, IPAddress host, uint16_t port)
{
    char buf[16];

    return sendto(data, size, iptoa(host, buf, sizeof(buf)) , port);
}

/**
 * Send a string as a UDP packet to a host.
 * @param data - the null terminated string to send
 * @param host - the IP or hostname to send the packet to. If this is a hostname 
 *               then a DNS lookup will be performed to find the IP address.
 * @param port - the UDP port to send the packet to
 * @retval true - packet send successfully
 * @retval false - failed to send packet
 */
boolean WiFly::sendto(const char *data, const char *host, uint16_t port)
{
    return sendto((uint8_t *)data, strlen(data), host, port);
}

/**
 * Send a string as a UDP packet to a host.
 * @param data - the null terminated string to send
 * @param host - the IP address to send the packet to.
 * @param port - the UDP port to send the packet to
 * @retval true - packet send successfully
 * @retval false - failed to send packet
 */
boolean WiFly::sendto(const char *data, IPAddress host, uint16_t port)
{
    return sendto((uint8_t *)data, strlen(data), host, port);
}

/**
 * Send a string as a UDP packet to a host.
 * @param data - the null terminated flash string to send
 * @param host - the IP or hostname to send the packet to. If this is a hostname 
 *               then a DNS lookup will be performed to find the IP address.
 * @param port - the UDP port to send the packet to
 * @retval true - packet send successfully
 * @retval false - failed to send packet
 */
boolean WiFly::sendto(const __FlashStringHelper *flashData, const char *host, uint16_t port)
{
    return sendto(NULL, 0, flashData, host, port);
}

/**
 * Send a string as a UDP packet to a host.
 * @param data - the null terminated flash string to send
 * @param host - the IP address to send the packet to.
 * @param port - the UDP port to send the packet to
 * @retval true - packet send successfully
 * @retval false - failed to send packet
 */
boolean WiFly::sendto(const __FlashStringHelper *flashData, IPAddress host, uint16_t port)
{
    char buf[16];

    return sendto(NULL, 0, flashData, iptoa(host, buf, sizeof(buf)), port);
}

/**
 * Preserve the IP and Port set via setIP() and setPort() when using
 * sendto() function.
 */
void WiFly::enableHostRestore()
{
    restoreHost = true;
    getHostIP(lastHost, sizeof(lastHost));
    lastPort = getHostPort();
    debug.print(F("enableHostRestore: stored "));
    debug.print(lastHost); debug.print(':'); debug.println(lastPort);
}

/**
 * Don't preserve the IP and Port set via setIP() and setPort() when using
 * sendto() function. The IP and Port will be left set by the last sendto() call.
 */
void WiFly::disableHostRestore()
{
    restoreHost = false;
}

/**
 * Check to see if the non-blocking open has completed.
 * When this returns true the open has finished with either
 * success or failure. You can use isConnected() to see
 * if the open was successful.
 * @retval true - the open operation has completed
 * @retval false - the open is still in progress
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
		DPRINT(F("Connected\n\r"));
		connected = true;
		connecting = false;
		/* successful connection exits command mode */
		inCommandMode = false;
		DPRINT(F("openComplete: true\n\r"));
		return true;
	    } else {
		/* Failed to connected */
		connecting = false;
		finishCommand();
		DPRINT(F("openComplete: true\n\r"));
		return true;
	    }
	    break;
	case 'C': {
		buf[0] = ch;
		gets(&buf[1], sizeof(buf)-1);
		debug.print(F("Failed to connect: ")); debug.println(buf);
		connecting = false;
		finishCommand();
		DPRINT(F("openComplete: true\n\r"));
		return true;
	    }

	default:
	    buf[0] = ch;
	    gets(&buf[1], sizeof(buf)-1);
	    debug.print(F("Unexpected resp: "));
	    debug.println(buf);
	    connecting = false;
	    finishCommand();
	    DPRINT(F("openComplete: true\n\r"));
	    return true;
	}
    }

    DPRINT(F("openComplete: false\n\r"));
    return false;
}

/**
 * Start a simple terminal that connects the debug stream
 * to the WiFly.
 * Useful for debugging and manually setting and reading
 * WiFly options.
 */
void WiFly::terminal()
{
    debug.println(F("Terminal ready"));
    while (1) {
	if (serial->available() > 0) {
	    debug.write(serial->read());
	}

	if (debug.available()) { // Outgoing data
	    serial->write(debug.read());
	}
    }
}

/**
 * Close the TCP connection
 * @retval true - connection closed
 * @retval false - failed to close
 */
boolean WiFly::close()
{
    if (!connected) {
	return true;
    }

    flushRx();

    startCommand();
    send_P(PSTR("close\r"));

    if (match_P(PSTR("*CLOS*"))) {
	finishCommand();
	debug.println(F("close: got *CLOS*"));
	connected = false;
	return true;
    } else {
	debug.println(F("close: failed, no *CLOS*"));
    }

    /* Check connection state */
    getConnection();

    finishCommand();

    return !connected;
}


WFDebug::WFDebug()
{
    debug = NULL;
}

void WFDebug::begin(Stream *debugPrint)
{
    debug = debugPrint;
}

size_t WFDebug::write(uint8_t data)
{
    if (debug != NULL) {
	return debug->write(data);
    }

    return 0;
}
