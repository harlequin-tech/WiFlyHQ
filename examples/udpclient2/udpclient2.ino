/*
 * WiFlyHQ Example udpclient2.ino
 *
 * This sketch implements a simple UDP client that sends UDP packets
 * to two UDP servers.
 *
 * This sketch is released to the public domain.
 *
 */


#include <SoftwareSerial.h>
SoftwareSerial wifiSerial(8,9);

//#include <AltSoftSerial.h>
//AltSoftSerial wifiSerial(8,9);

#include <WiFlyHQ.h>

/* Change these to match your WiFi network */
const char mySSID[] = "myssid";
const char myPassword[] = "my-wpa-password";

void terminal();

WiFly wifly;

void setup()
{
    char buf[32];

    Serial.begin(115200);
    Serial.println("Starting");
    Serial.print("Free memory: ");
    Serial.println(wifly.getFreeMemory(),DEC);

    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
        Serial.println("Failed to start wifly");
	terminal();
    }

    if (wifly.getFlushTimeout() != 10) {
        Serial.println("Restoring flush timeout to 10msecs");
        wifly.setFlushTimeout(10);
	wifly.save();
	wifly.reboot();
    }

    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	/* Setup the WiFly to connect to a wifi network */
	Serial.println("Joining network");
	wifly.setSSID(mySSID);
	wifly.setPassphrase(myPassword);
	wifly.enableDHCP();

	if (wifly.join()) {
	    Serial.println("Joined wifi network");
	} else {
	    Serial.println("Failed to join wifi network");
	    terminal();
	}
    } else {
        Serial.println("Already joined network");
    }

    /* Setup for UDP packets, sent automatically */
    wifly.setIpProtocol(WIFLY_PROTOCOL_UDP);
    wifly.setHost("192.168.1.60", 8042);	// Send UDP packet to this server and port

    Serial.print("MAC: ");
    Serial.println(wifly.getMAC(buf, sizeof(buf)));
    Serial.print("IP: ");
    Serial.println(wifly.getIP(buf, sizeof(buf)));
    Serial.print("Netmask: ");
    Serial.println(wifly.getNetmask(buf, sizeof(buf)));
    Serial.print("Gateway: ");
    Serial.println(wifly.getGateway(buf, sizeof(buf)));

    wifly.setDeviceID("Wifly-UDP");
    Serial.print("DeviceID: ");
    Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

    Serial.println("WiFly ready");
}

uint32_t count=0;
uint8_t tick=0;
uint32_t lastSend = 0;	/* Last time message was sent */

void loop()
{
    /* Send a message every 500 milliseconds */
    if ((millis() - lastSend) > 500) {
        count++;
	Serial.print("Sending message ");
	Serial.println(count);

	if (tick == 0) {
	    wifly.setHost("192.168.1.60", 8042);
	    tick++;
	} else {
	    wifly.setHost("192.168.1.104", 8043);
	    tick = 0;
	}

	wifly.print("Hello, count=");
	wifly.println(count);
	lastSend = millis();
    }

    if (Serial.available()) {
        /* if the user hits 't', switch to the terminal for debugging */
        if (Serial.read() == 't') {
	    terminal();
	}
    }

}

void terminal()
{
    Serial.println("Terminal ready");
    while (1) {
	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}

	if (Serial.available()) {
	    wifly.write(Serial.read());
	}
    }
}
