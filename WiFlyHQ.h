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

/* Release history
 *
 * Version  Date         Description
 * 0.1      25-Mar-2012  First release.
 *
 */

#ifndef _WIFLYHQ_H_
#define _WIFLYHQ_H_

#include <Arduino.h>
#include <Stream.h>
#include <avr/pgmspace.h>
#include <IPAddress.h>

/* IP Protocol bits */
#define WIFLY_PROTOCOL_UDP		0x01
#define WIFLY_PROTOCOL_TCP		0x02
#define WIFLY_PROTOCOL_SECURE		0x04
#define WIFLY_PROTOCOL_TCP_CLIENT	0x08
#define WIFLY_PROTOCOL_HTTP		0x10

/* IP Flag bits */
#define WIFLY_FLAG_TCP_KEEP		0x01	/* Keep TCP connection alive when wifi lost */
#define WIFLY_FLAG_TCP_NODELAY		0x02
#define WIFLY_FLAG_TCP_RETRY		0x04
#define WIFLY_FLAG_UDP_RETRY		0x08
#define WIFLY_FLAG_DNS_CACHING		0x10
#define WIFLY_FLAG_ARP_CACHING		0x20
#define WIFLY_FLAG_UDP_AUTO_PAIR	0x40
#define WIFLY_FLAG_ADD_TIMESTAMP	0x80

/* UART mode bits */
#define WIFLY_UART_MODE_NOECHO		0x01
#define WIFLY_UART_MODE_DATA_TRIGGER	0x02
#define WIFLY_UART_MODE_SLEEP_RX_BREAK	0x08
#define WIFLY_UART_MODE_RX_BUFFER	0x10

/* DHCP modes */
#define WIFLY_DHCP_MODE_OFF		0x00	/* No DHCP, static IP mode */
#define WIFLY_DHCP_MODE_ON		0x01	/* get IP, Gateway, and DNS from AP */
#define WIFLY_DHCP_MODE_AUTOIP		0x02	/* Used with Adhoc networks */
#define WIFLY_DHCP_MODE_CACHE		0x03	/* Use previous DHCP address based on lease */
#define WIFLY_DHCP_MODE_SERVER		0x04	/* Server DHCP IP addresses? */

#define WIFLY_DEFAULT_TIMEOUT		500	/* 500 milliseconds */

class WFDebug : public Print {
public:
    WFDebug();
    void begin(Print *debugPrint);
    virtual size_t write(uint8_t byte);
private:
    Print *debug;
};

class WiFly : public Stream {
public:
    WiFly();
    
    boolean begin(Stream *serialdev, Print *debugPrint = NULL);
    
    char *getSSID(char *buf, int size);
    char *getDeviceID(char *buf, int size);    
    char *getIP(char *buf, int size);
    char *getNetmask(char *buf, int size);
    char *getGateway(char *buf, int size);
    char *getDNS(char *buf, int size);
    char *getMAC(char *buf, int size);
    int8_t getDHCPMode();

    uint16_t getConnection();
    int8_t getRSSI();

    boolean setDeviceID(const char *buf);
    boolean setBaud(uint32_t baud);
    uint32_t getBaud();
    uint8_t getUartMode();
    uint8_t getIpFlags();

    uint8_t getFlushChar();
    uint16_t getFlushSize();
    uint16_t getFlushTimeout();

    char *getHostIP(char *buf, int size);
    uint16_t getHostPort();

    boolean setSSID(const char *buf);
    boolean setIP(const char *buf);
    boolean setNetmask(const char *buf);
    boolean setGateway(const char *buf);
    boolean setDNS(const char *buf);
    boolean setChannel(const char *buf);
    boolean setKey(const char *buf);
    boolean setPassphrase(const char *buf);
    boolean setSpaceReplace(const char *buf);
    boolean setDHCP(const uint8_t mode);

    boolean setHostIP(const char *buf);
    boolean setHostPort(const uint16_t port);
    boolean setHost(const char *buf, uint16_t port);

    boolean setIpProtocol(const uint8_t protocol);
    boolean setIpFlags(const uint8_t flags);
    boolean setUartMode(const uint8_t mode);

    boolean setBroadcastInterval(const uint8_t seconds);

    boolean setTimeAddress(const char *buf);
    boolean setTimePort(const uint16_t port);
    boolean setTimezone(const uint8_t zone);
    boolean setTimeEnable(const uint16_t enable);

    boolean setFlushTimeout(const uint16_t timeout);
    boolean setFlushChar(const char flushChar);
    boolean setFlushSize(const uint16_t size);
    boolean enableDataTrigger(const uint16_t flushtime=10, const char flushChar=0, const uint16_t flushSize=64);
    boolean disableDataTrigger();

    boolean setIOFunc(const uint8_t func);

    char *getTime(char *buf, int size);
    uint32_t getUptime();
    uint8_t getTimezone();
    uint32_t getRTC();

    bool getHostByName(const char *hostname, char *buf, int size);
    boolean ping(const char *host);

    boolean enableDHCP();
    boolean disableDHCP();
    
    boolean join(const char *ssid);
    boolean join(void);
    boolean leave();
    boolean isAssociated();

    boolean save();
    boolean reboot();
    boolean factoryRestore();

    boolean sendto(const uint8_t *data, uint16_t size, const char *host, uint16_t port);
    boolean sendto(const uint8_t *data, uint16_t size, IPAddress host, uint16_t port);
    boolean sendto(const char *data, const char *host, uint16_t port);
    boolean sendto(const char *data, IPAddress host, uint16_t port);
    boolean sendto(const __FlashStringHelper *data, const char *host, uint16_t port);
    boolean sendto(const __FlashStringHelper *data, IPAddress host, uint16_t port);

    void enableHostRestore();
    void disableHostRestore();

    boolean open(const char *addr, int port=80, boolean block=true);
    boolean open(IPAddress addr, int port=80, boolean block=true);
    boolean close();
    boolean openComplete();
    boolean isConnected();
    boolean isInCommandMode();
    
    virtual size_t write(uint8_t byte);
    virtual int read();
    virtual int available();
    virtual void flush();
    virtual int peek();

    char *iptoa(IPAddress addr, char *buf, int size);
    IPAddress atoip(char *buf);
    boolean isDotQuad(const char *addr);

    void print_P(const prog_char *str);
    void println_P(const prog_char *str);

    int getFreeMemory();
  
    using Print::write;

    void dbgBegin(int size=256);
    void dbgDump();
    boolean debugOn;
  private:
    void init(void);

    void dump(const char *str);

    boolean sendto(
	const uint8_t *data,
	uint16_t size,
	const __FlashStringHelper *flashData,
	const char *host,
	uint16_t port);

    void send_P(const prog_char *str);
    void send(const char *str);
    void send(const char ch);
    boolean enterCommandMode();
    boolean exitCommandMode();
    boolean setPrompt();
    boolean getPrompt(boolean strict=false, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean checkPrompt(const char *str);
    boolean match(const char *str, boolean strict=false, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean match_P(const prog_char *str, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    int getResponse(char *buf, int size, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean readTimeout(char *ch, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    int gets(char *buf, int size, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean startCommand();
    boolean finishCommand();
    char *getopt(int opt, char *buf, int size);
    boolean setopt(const prog_char *cmd, const char *buf);
    boolean getres(char *buf, int size);
    void flushRx(int timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean checkClose(boolean peeked);

    boolean hide();

    boolean inCommandMode;
    int  exitCommand;
    boolean dhcp;
    bool restoreHost;

    boolean connected;
    boolean connecting;
    struct {
	uint8_t tcp;
	uint8_t assoc;
	uint8_t authen;
	uint8_t dnsServer;
	uint8_t dnsFound;
	uint8_t channel;
    } status;

    Stream *serial;	/* Serial interface to WiFly */
    
    WFDebug debug;	/* Internal debug channel. */

    /*  for dbgDump() */
    char *dbgBuf;
    int dbgInd;
    int dbgMax;
};

#endif
