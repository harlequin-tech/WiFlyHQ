A library for the Roving Networks WiFly RN-XV.

The library provides functions for setting up and managing the WiFly module,
sending UDP packets, opening TCP connections and sending and receiving data
over the TCP connection.

Doxygen <a href="http://harlequin-tech.github.com/WiFlyHQ">documentation for WiFlyHQ</a>

Examples
--------

Example code to setup and use the hardware serial interface:

	Serial.begin(9600);
	wifly.begin(&Serial, NULL);

Setup and use a software serial interface:

	#include <SoftwareSerial.h>
	SoftwareSerial wifiSerial(8,9);

	wifiSerial.begin(9600);
	wifly.begin(&wifiSerial);

Join a WiFi network:

	wifly.setSSID("mySSID");
	wifly.setPassphrase("myWPApassword");
	wifly.enableDHCP();
	wifly.join();

Create an Ad Hoc WiFi network on channel 10:

	wifly.createAdhocNetwork("myssid", 10);

Send a UDP packet:

	wifly.setIpProtocol(WIFLY_PROTOCOL_UDP);
	wifly.sendto("Hello, world", "192.168.1.100", 2042);

Open a TCP connection and send some data, and close the connection:

	wifly.setIpProtocol(WIFLY_PROTOCOL_TCP);
	wifly.open("192.168.1.100",8042);
	wifly.println("Hello, world!");
	wifly.close();

Receive UDP or TCP data (assumes software serial interface):

	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}

Easy handling of multiple receive options with multiMatch_P():

        if (wifly.available() > 0) {
	    int match = wifly.multiMatch_P(100, 3,
			    F("button"), F("slider="), F("switch="));
	    switch (match) {
	    case 0: /* button */
		Serial.print(F("button: pressed"));
		break;
	    case 1: /* slider */
		int slider = wifly.parseInt();
		Serial.print(F("slider: "));
		Serial.println(slider);
		break;
	    case 2: /* switch */
		char ch = wifly.read();
		Serial.print(F("switch: "));
		Serial.println(ch);
		break;
	    default: /* timeout */
		break;
	    }
	}

Known Issues
------------

Limitations with WiFly RN-XV rev 2.32 firmware

1. Cannot determine the IP address of the TCP client that has connected.
2. Changing the local port does not take effect until after a save and reboot.
3. Closing a TCP connection may not work. Client may stay connected
   and send additional data. Packet analysis shows that no FIN is sent
   by the WiFly, but it still sends the *CLOS* message back to the
   arduino.
4. Only supports one TCP connection at a time, which means its easy
   for a web browser to lock out other users.
5. Changing the flush timeout (set comm time x) does not take affect until 
   after a save and reboot.

To do
-----

1. Add FTP support.
