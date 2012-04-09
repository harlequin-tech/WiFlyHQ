/*
 * WiFlyHQ Example tcpclient.ino
 *
 * This sketch implements a simple TCP client that connects to a 
 * TCP server, sends any serial monitor input to the server and any
 * data received from the server to the TCP monitor, and disconnects
 * after 10 seconds.
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
    char buf[64];

    Serial.begin(115200);
    Serial.println("Starting");
    Serial.print("Free memory: ");
    Serial.println(wifly.getFreeMemory(),DEC);

    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
        Serial.println("Failed to start wifly");
	terminal();
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

    Serial.println("WiFly ready");

    Serial.print("MAC: ");
    Serial.println(wifly.getMAC(buf, sizeof(buf)));
    Serial.print("IP: ");
    Serial.println(wifly.getIP(buf, sizeof(buf)));
    Serial.print("Netmask: ");
    Serial.println(wifly.getNetmask(buf, sizeof(buf)));
    Serial.print("Gateway: ");
    Serial.println(wifly.getGateway(buf, sizeof(buf)));

    Serial.println("Set DeviceID");
    wifly.setDeviceID("Wifly-TCP");
    Serial.print("DeviceID: ");
    Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

    wifly.setIpProtocol(WIFLY_PROTOCOL_TCP);

    if (wifly.isConnected()) {
        Serial.println("Old connection active. Closing");
	wifly.close();
    }
}

uint32_t connectTime = 0;

void loop()
{
    int available;

    if (wifly.isConnected() == false) {
	Serial.println("Connecting");
	if (wifly.open("192.168.1.60",8042)) {
	    Serial.println("Connected");
	    connectTime = millis();
	} else {
	    Serial.println("Failed to open");
	}
    } else {
	available = wifly.available();
	if (available < 0) {
	    Serial.println("Disconnected");
	} else if (available > 0) {
	    Serial.write(wifly.read());
	} else {
	    /* Disconnect after 10 seconds */
	    if ((millis() - connectTime) > 10000) {
		Serial.println("Disconnecting");
		wifly.close();
	    }
	}

	/* Send data from the serial monitor to the TCP server */
	if (Serial.available()) {
	    wifly.write(Serial.read());
	}
    }
}

void terminal()
{
    while (1) {
	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}


	if (Serial.available()) { // Outgoing data
	    wifly.write(Serial.read());
	}
    }
}
