/*
 * WiFlyHQ Example WebSocketClient.ino
 *
 * This sketch implements a simple websocket client that connects to a 
 * websocket echo server, sends a message, and receives the response.
 * Accepts a line of text via the serial monitor, sends it to the websocket
 * echo server, and receives the echo response.
 * See http://www.websocket.org for more information.
 *
 * This sketch is released to the public domain.
 *
 */

#include "Arduino.h"
#include <WiFlyHQ.h>
#include <SoftwareSerial.h>

/* Work around a bug with PROGMEM and PSTR where the compiler always
 * generates warnings.
 */
#undef PROGMEM 
#define PROGMEM __attribute__(( section(".progmem.data") )) 
#undef PSTR 
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];})) 

int getMessage(char *buf, int size);
void send(const char *data);
bool connect(const char *hostname, const char *path="/", uint16_t port=80);

SoftwareSerial wifiSerial(8,9);
WiFly wifly;

const char mySSID[] = "mySSID";
const char myPassword[] = "myPassword";

char server[] = "echo.websocket.org";

void setup()
{
    Serial.begin(115200);

    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
        Serial.println(F("Failed to start wifly"));
	wifly.terminal();
    }

    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	Serial.println(F("Joining network"));
	if (wifly.join(mySSID, myPassword, true)) {
	    wifly.save();
	    Serial.println(F("Joined wifi network"));
	} else {
	    Serial.println(F("Failed to join wifi network"));
	    wifly.terminal();
	}
    } else {
        Serial.println(F("Already joined network"));
    }

    if (!connect(server)) {
	Serial.print(F("Failed to connect to "));
	Serial.println(server);
	wifly.terminal();
    }

    Serial.println(F("Sending Hello World"));
    send("Hello, World!");
}

char inBuf[128];
char outBuf[128];
uint8_t outBufInd = 0;

void loop() 
{
    if (getMessage(inBuf, sizeof(inBuf)) > 0) {
	Serial.print(F("Received response: "));
	Serial.println(inBuf);
    }

    if (Serial.available()) {
        char ch = Serial.read();
        if (ch == '\r') {
	    /* Got a carriage return, send the message */
	    outBuf[outBufInd] = 0;	// null terminate the string
	    send(outBuf);
	    outBufInd = 0;
	    Serial.println();
	} else if (outBufInd < (sizeof(outBuf) - 1)) {
	    outBuf[outBufInd] = ch;
	    outBufInd++;
	    Serial.write(ch);		// echo input back to Serial monitor
	}
    }
}

/** See if there is a properly formatted message from the
 * server. A correct message starts with character 0, and ends
 * with character 255.
 * @param buf - buffer to store incoming message in
 * @param size - max size of message to store
 * @returns - size of the received message, or 0 if no message received
 */
int getMessage(char *buf, int size)
{
    int len = 0; 

    if (wifly.available() > 0) {
	if (wifly.read() == 0) {
	    /* read up to the end of the message (255) */
	    len = wifly.getsTerm(buf, size, 255);
	}
    }
    return len;
}

/** Send a message to the server */
void send(const char *data) 
{
    wifly.write((uint8_t)0);
    wifly.write(data);
    wifly.write((uint8_t)255);
}

/** Connect to a websocket server.
 */
bool connect(const char *hostname, const char *path, uint16_t port)
{
    if (!wifly.open(hostname, port)) {
        Serial.println(F("connect: failed to open TCP connection"));
	return false;
    }

    wifly.print(F("GET "));
    wifly.print(path);
    wifly.println(F(" HTTP/1.1"));
    wifly.println(F("Upgrade: WebSocket"));
    wifly.println(F("Connection: Upgrade"));
    wifly.print(F("Host: "));
    wifly.println(hostname);
    wifly.println(F("Origin: http://www.websocket.org"));
    wifly.println();

    /* Wait for the handshake response */
    if (wifly.match(F("HTTP/1.1 101"), 2000)) {
	wifly.flushRx(200);
	return true;
    }

    Serial.println(F("connect: handshake failed"));
    wifly.close();

    return false;
}
