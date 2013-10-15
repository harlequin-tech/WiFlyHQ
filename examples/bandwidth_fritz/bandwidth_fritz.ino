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
* WiFly Bandwidth monitor for Fritz Box routers.
* Tested with Fritz! Box 7340.
*
* Poll the fritz box every 5 seconds to get the transmit and receive data rates.
*
*/

#include <SoftwareSerial.h>
#include "WiFlyHQ.h"

SoftwareSerial wiflySerial(8,9);
WiFly wifly;

const char mySSID[] = "myssid";
const char myPassword[] = "my_wpa_password";

uint32_t lastCheck=0;		// Last bandwidth check time

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Starting"));
    Serial.print(F("Free memory: "));
    Serial.println(wifly.getFreeMemory(),DEC);

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
}

void loop()
{
    char date[32];
    uint32_t rxRate;
    uint32_t txRate;
    uint32_t now=millis();

    if ((now - lastCheck) > 5000) {
	if (getBandwidth(&rxRate, &txRate, date, sizeof(date))) {
	    Serial.print(date);
	    Serial.print(F(" - rxRate: ")); Serial.print(rxRate);
	    Serial.print(F(" txRate: ")); Serial.println(txRate);
	}

        lastCheck = now;
    }
}


// Get the current tx and rx bandwidth from the fritzbox router
bool getBandwidth(uint32_t *rxRate, uint32_t *txRate, char *date, int dateSize)
{
    if (!sendSoapReq("192.168.1.1", 49000, F("/upnp/control/WANIPConn1"), 
	F("urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1#GetAddonInfos"),
	F("<?xml version=\"1.0\" encodin=\"utf-8\"?>\n"
	  "<s:Envelope s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\" "
	  "xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\">\n"
	  "<s:Body>\n"
	  "<u:GetAddonInfos xmlns:u=\"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1\" />\n"
	  "</s:Body>\n</s:Envelope>"))) {
	return false;
    }

    uint32_t now=millis();

    if (date != NULL) {
	if (!wifly.match(F("DATE: "),5000)) {
	    Serial.println(F("failed to find DATE:"));
	    return false;
	}

	Serial.print(F("DATE: response took "));
	Serial.print(millis()-now);
	Serial.println(F(" msecs"));
	wifly.gets(date, dateSize);
    }

    if (!wifly.match(F("<NewByteSendRate>"))) {
        Serial.println(F("failed to find NewByteSendRate"));
	return false;
    }

    *txRate = wifly.parseInt();

    if (!wifly.match(F("<NewByteReceiveRate>"))) {
        Serial.println(F("failed to find NewByteReceiveRate"));
	return false;
    }
    *rxRate = wifly.parseInt();

    wifly.close();

    return true;
}


// Send a SOAP request
bool sendSoapReq(
    char *host, 
    uint16_t port, 
    const __FlashStringHelper *path, 
    const __FlashStringHelper *action, 
    const __FlashStringHelper *xmlreq)
{
    if (wifly.open(host, port)) {
        Serial.print(F("Connected to "));
	Serial.println(host);

	wifly.print(F("POST "));
	wifly.print(path);
	wifly.println(F(" HTTP/1.1"));
	wifly.print(F("HOST: "));
	wifly.print(host);
	wifly.print(':');
	wifly.println(port);

	// request current bandwidth
	wifly.print(F("SOAPACTION: \""));
	wifly.print(action);
	wifly.println('"');
	wifly.println(F("Content-Type: text/xml; charset=\"utf-8\""));
	wifly.print(F("Content-Length: "));
	wifly.println(strlen_P((const prog_char *)xmlreq));
	wifly.println();
	wifly.println(xmlreq);
	wifly.println();
    } else {
        Serial.println(F("Failed to connect"));
    }
}
