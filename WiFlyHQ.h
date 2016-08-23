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

/* Release history
 *
 * Version  Date         Description
 * 0.1      25-Mar-2012  First release.
 * 0.2      09-Apr-2012  Added features to support http servers.
 *                       - added an httpserver.ino example.
 *                       - added sendChunk() and sendChunkln() to send chunked HTTP bodies.
 *                       - added terminal() method for simple terminal access via debug stream
 *                       - replaced getFreeMemory() with simpler version that works with 0 bytes
 *                       - turned peek buffer into a circular buffer to fix bug with detecting
 *                         *CLOS* and *OPEN* after a partial match.
 *                       - Added new TCP connection detection via *OPEN* match from available().
 *                         isConnected() can now be polled until a client connects.
 *                       - made the match() function public, handy for matching text in a stream.
 *                       - Added a getProtocol() function to get current set of protocols.
 * 0.3      21-Apr-2012  Added createAdhocNetwork() to create an Ad Hoc WiFi network.
 *              Optimised the setopt() and getopt() function so they handle
 *              integer conversions and refactored all of the set and get functions.
 *              Added a multMatch_P() function to match serial data against multiple
 *              progmem strings.
 *              Added failure detection to the join() function to quickly detect
 *              a failure rather than relying on a timeout.
 *              Added setJoin() and getJoin() function for access to the wlan join parameter.
 *              Refactored getres() to use the new multiMatch_P() function.
 *
 */

/**
 * @mainpage WiFlyHQ WiFly RN-XV Arduino library
 * 
 * This library provides functions for setting up and managing the WiFly module,
 * sending UDP packets, opening TCP connections and sending and receiving data
 * over the TCP connection.
 * 
 * @author Harlequin-Tech
 */ 

/**
 * @file WiFlyHQ.h
 *
 * @brief The WiFly class definition.
 */

#ifndef _WIFLYHQ_H_
#define _WIFLYHQ_H_

#include <Arduino.h>
#include <Stream.h>
#include <avr/pgmspace.h>
#include <IPAddress.h>

#if (ARDUINO >= 103)
typedef const char PROGMEM prog_char;
#endif

/* IP Protocol bits */
#define WIFLY_PROTOCOL_UDP        0x01
#define WIFLY_PROTOCOL_TCP        0x02
#define WIFLY_PROTOCOL_SECURE     0x04
#define WIFLY_PROTOCOL_TCP_CLIENT 0x08
#define WIFLY_PROTOCOL_HTTP       0x10    /* HTTP Client mode */
#define WIFLY_PROTOCOL_RAW        0x20
#define WIFLY_PROTOCOL_SMTP       0x40

/* IP Flag bits */
#define WIFLY_FLAG_TCP_KEEP       0x01    /* Keep TCP connection alive when wifi lost */
#define WIFLY_FLAG_TCP_NODELAY    0x02
#define WIFLY_FLAG_TCP_RETRY      0x04
#define WIFLY_FLAG_UDP_RETRY      0x08
#define WIFLY_FLAG_DNS_CACHING    0x10
#define WIFLY_FLAG_ARP_CACHING    0x20
#define WIFLY_FLAG_UDP_AUTO_PAIR  0x40
#define WIFLY_FLAG_ADD_TIMESTAMP  0x80

/* UART mode bits */
#define WIFLY_UART_MODE_NOECHO         0x01
#define WIFLY_UART_MODE_DATA_TRIGGER   0x02
#define WIFLY_UART_MODE_SLEEP_RX_BREAK 0x08
#define WIFLY_UART_MODE_RX_BUFFER      0x10

/* DHCP modes */
#define WIFLY_DHCP_MODE_OFF      0x00    /* No DHCP, static IP mode */
#define WIFLY_DHCP_MODE_ON       0x01    /* get IP, Gateway, and DNS from AP */
#define WIFLY_DHCP_MODE_AUTOIP   0x02    /* Used with Adhoc networks */
#define WIFLY_DHCP_MODE_CACHE    0x03    /* Use previous DHCP address based on lease */
#define WIFLY_DHCP_MODE_SERVER   0x04    /* Server DHCP IP addresses? */

/* WLAN Join modes */
#define WIFLY_WLAN_JOIN_MANUAL   0x00    /* Don't auto-join a network */
#define WIFLY_WLAN_JOIN_AUTO     0x01    /* Auto-join network set in SSID, passkey, and channel. */
#define WIFLY_WLAN_JOIN_ANY      0x02    /* Ignore SSID and join strongest network using passkey. */
#define WIFLY_WLAN_JOIN_ADHOC    0x04    /* Create an Adhoc network using SSID, Channel, IP and NetMask */

#define WIFLY_DEFAULT_TIMEOUT    500    /* 500 milliseconds */

#define WIFLY_MODE_WPA           0    
#define WIFLY_MODE_WEP_128       1
#define WIFLY_MODE_WEP_64        2

class WFDebug : public Stream {
public:
    WFDebug();
    void begin(Stream *debugPrint);

    virtual size_t write(uint8_t byte);
    virtual int read() { return debug->read(); }
    virtual int available() { return debug->available(); }
    virtual void flush() { return debug->flush(); }
    virtual int peek() { return debug->peek(); }

    using Print::write;
private:
    Stream *debug;
};

class WiFly : public Stream {
public:
    WiFly();
    
    boolean begin(Stream *serialdev, Stream *debugPrint = NULL);
    
    char *getSSID(char *buf, int size);
    uint8_t getJoin();
    char *getDeviceID(char *buf, int size);    
    char *getIP(char *buf, int size);
    uint16_t getPort();
    char *getNetmask(char *buf, int size);
    char *getGateway(char *buf, int size);
    char *getDNS(char *buf, int size);
    char *getMAC(char *buf, int size);
    int8_t getDHCPMode();
    uint32_t getRate();
    uint8_t getTxPower();

    uint16_t getConnection();
    int8_t getRSSI();

    bool setJoin(uint8_t join);
    boolean setDeviceID(const char *buf);
    boolean setDeviceID(const __FlashStringHelper *buf);
    boolean setBaud(uint32_t baud);
    uint32_t getBaud();
    uint8_t getUartMode();
    uint8_t getIpFlags();
    uint8_t getProtocol();

    uint8_t getFlushChar();
    uint16_t getFlushSize();
    uint16_t getFlushTimeout();

    char getSpaceReplace(void);

    char *getHostIP(char *buf, int size);
    uint16_t getHostPort();

    boolean setSSID(const char *buf);
    boolean setIP(const char *buf);
    boolean setIP(const __FlashStringHelper *buf);
    boolean setPort(const uint16_t port);
    boolean setNetmask(const char *buf);
    boolean setNetmask(const __FlashStringHelper *buf);
    boolean setGateway(const char *buf);
    boolean setDNS(const char *buf);
    boolean setChannel(uint8_t channel);
    boolean setAuth(uint8_t mode);
    boolean setKey(const char *buf);
    boolean setPassphrase(const char *buf);
    boolean setSpaceReplace(char ch);
    boolean setDHCP(const uint8_t mode);
    boolean setRate(uint32_t rate);
    boolean setTxPower(uint8_t dBm);

    boolean setHostIP(const char *buf);
    boolean setHostIP(const __FlashStringHelper *buf);
    boolean setHostPort(const uint16_t port);
    boolean setHost(const char *buf, uint16_t port);

    boolean setProtocol(const uint8_t protocol);
    boolean setIpProtocol(const uint8_t protocol);    /* obsolete */
    boolean setIpFlags(const uint8_t flags);
    boolean setUartMode(const uint8_t mode);

    boolean setBroadcastInterval(const uint8_t seconds);

    boolean setTimeAddress(const char *buf);
    boolean setTimePort(const uint16_t port);
    boolean setTimezone(const uint8_t zone);
    boolean setTimeEnable(const uint16_t enable);

    boolean setAdhocBeacon(const uint16_t msecs);
    boolean setAdhocProbe(const uint16_t secs);
    uint16_t getAdhocBeacon();
    uint16_t getAdhocProbe();
    uint16_t getAdhocReboot();

    boolean setFlushTimeout(const uint16_t timeout);
    boolean setFlushChar(const char flushChar);
    boolean setFlushSize(uint16_t size);
    boolean enableDataTrigger(const uint16_t flushtime=10, const char flushChar=0, const uint16_t flushSize=64);
    boolean disableDataTrigger();
    boolean enableUdpAutoPair();
    boolean disableUdpAutoPair();

    boolean setIOFunc(const uint8_t func);
    
    boolean sleep(uint16_t seconds = 0);
    
    boolean time();
    char *getTime(char *buf, int size);
    uint32_t getUptime();
    uint8_t getTimezone();
    uint32_t getRTC();

    bool getHostByName(const char *hostname, char *buf, int size);
    boolean ping(const char *host);

    boolean enableDHCP();
    boolean disableDHCP();
    
    boolean createAdhocNetwork(const char *ssid, uint8_t channel);
    boolean join(const char *ssid, uint16_t timeout=20000);
    boolean join(uint16_t timeout=20000);
    boolean join(const char *ssid, const char *password, bool dhcp=true, uint8_t mode=WIFLY_MODE_WPA, uint16_t timeout=20000);
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

    boolean open(const char *addr, uint16_t port=80, boolean block=true, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean open(IPAddress addr, uint16_t port=80, boolean block=true, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
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

    void sendChunk(const char *str);
    void sendChunk(const __FlashStringHelper *str);
    void sendChunkln(const char *str);
    void sendChunkln(const __FlashStringHelper *str);
    void sendChunkln(void);

    int getFreeMemory();
    void terminal();
  
    using Print::write;

    void dbgBegin(int size=256);
    void dbgDump();
    void dbgEnd();
    boolean debugOn;

    boolean match(const char *str, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean match(const __FlashStringHelper *str, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    int multiMatch_P(uint16_t timeout, uint8_t count, ...);
    int gets(char *buf, int size, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    int getsTerm(char *buf, int size, char term, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    void flushRx(int timeout=WIFLY_DEFAULT_TIMEOUT);

    boolean setFtpDefaults(void);
    boolean setFtpAddress(const char *addr);
    boolean setFtpPort(uint16_t port);
    boolean setFtpDirectory(const char *dir);
    boolean setFtpUser(const char *user);
    boolean setFtpPassword(const char *password);
    boolean setFtpFilename(const char *filename);
    boolean setFtpTimer(uint16_t msecs);
    boolean setFtpMode(uint8_t mode);

    boolean ftpGet(
    const char *addr,
    const char *dir,
    const char *user,
    const char *password,
    const char *filename);

  private:
    void init(void);

    void dump(const char *str);

    boolean sendto(
    const uint8_t *data,
    uint16_t size,
    const __FlashStringHelper *flashData,
    const char *host,
    uint16_t port);

    boolean match_P(const prog_char *str, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    int8_t multiMatch_P(const prog_char *str[], uint8_t count, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);

    void send_P(const prog_char *str);
    void send(const char *str);
    void send(const char ch);
    boolean enterCommandMode();
    boolean exitCommandMode();
    boolean setPrompt();
    boolean getPrompt(uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean checkPrompt(const char *str);
    int getResponse(char *buf, int size, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean readTimeout(char *ch, uint16_t timeout=WIFLY_DEFAULT_TIMEOUT);
    boolean startCommand();
    boolean finishCommand();
    char *getopt(int opt, char *buf, int size);
    uint32_t getopt(int opt, uint8_t base=DEC);
    boolean setopt(const prog_char *cmd, const char *buf=NULL, const __FlashStringHelper *buf_P=NULL, bool spacesub=false);
    boolean setopt(const prog_char *opt, const uint32_t value, uint8_t base=DEC);
    boolean getres(char *buf, int size);

    boolean checkStream(const prog_char *str, boolean peeked);
    boolean checkClose(boolean peeked);
    boolean checkOpen(boolean peeked);

    boolean hide();

    boolean inCommandMode;
    int  exitCommand;
    boolean dhcp;
    bool restoreHost;
    bool restoreHostStored;
    char lastHost[32];
    uint16_t lastPort;

    boolean tcpMode;
    boolean udpAutoPair;

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

    Stream *serial;    /* Serial interface to WiFly */
    
    WFDebug debug;    /* Internal debug channel. */

    char replaceChar;    /* The space replacement character */

    /*  for dbgDump() */
    char *dbgBuf;
    int dbgInd;
    int dbgMax;
};

#endif
