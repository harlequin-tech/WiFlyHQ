/*
 * WiFlyHQ Example httpclient_progmem.ino
 *
 * This sketch implements a simple Web client that connects to a 
 * web server, sends a GET, and then sends the result to the 
 * Serial monitor.
 *
 * This example uses PROGMEM to reduce the amount of RAM needed
 * for the sketch. Most of the strings are stored in flash rather than RAM.
 *
 * This sketch is released to the public domain.
 *
 */

#include <avr/pgmspace.h>
#include <WiFlyHQ.h>

#include <SoftwareSerial.h>
SoftwareSerial wifiSerial(8,9);

//#include <AltSoftSerial.h>
//AltSoftSerial wifiSerial(8,9);

WiFly wifly;

/* Change these to match your WiFi network */
const char mySSID[] = "myssid";
const char myPassword[] = "my-wpa-password";

//const char site[] = "arduino.cc";
//const char site[] = "www.google.co.nz";
const char site[] = "hunt.net.nz";

void terminal();

void setup()
{
    char buf[32];

    Serial.begin(115200);
    Serial.println(F("Starting"));
    Serial.print(F("Free memory: "));
    Serial.println(wifly.getFreeMemory(),DEC);

    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
        Serial.println(F("Failed to start wifly"));
	terminal();
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
	    terminal();
	}
    } else {
        Serial.println(F("Already joined network"));
    }

    //terminal();

    Serial.print(F("MAC: "));
    Serial.println(wifly.getMAC(buf, sizeof(buf)));
    Serial.print(F("IP: "));
    Serial.println(wifly.getIP(buf, sizeof(buf)));
    Serial.print(F("Netmask: "));
    Serial.println(wifly.getNetmask(buf, sizeof(buf)));
    Serial.print(F("Gateway: "));
    Serial.println(wifly.getGateway(buf, sizeof(buf)));
    Serial.print(F("SSID: "));
    Serial.println(wifly.getSSID(buf, sizeof(buf)));

    wifly.setDeviceID("Wifly-WebClient");
    Serial.print(F("DeviceID: "));
    Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

    if (wifly.isConnected()) {
        Serial.println(F("Old connection active. Closing"));
	wifly.close();
    }

    if (wifly.open(site, 80)) {
        Serial.print(F("Connected to "));
	Serial.println(site);

	/* Send the request */
	wifly.println("GET / HTTP/1.0");
	wifly.println();
    } else {
        Serial.println(F("Failed to connect"));
    }
}

void loop()
{
    if (wifly.available() > 0) {
	char ch = wifly.read();
	Serial.write(ch);
	if (ch == '\n') {
	    /* add a carriage return */ 
	    Serial.write('\r');
	}
    }

    if (Serial.available() > 0) {
	wifly.write(Serial.read());
    }
}

/* Connect the WiFly serial to the serial monitor. */
void terminal()
{
    while (1) {
	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}


	if (Serial.available() > 0) {
	    wifly.write(Serial.read());
	}
    }
}
