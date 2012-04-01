A library for the Roving Networks WiFly RN-XV.

The library provides functions for setting up and managing the WiFly module,
sending UDP packets, opening TCP connections and sending and receiving data
over the TCP connection.

Example code to setup and use the hardware serial interface:

	Serial.begin(9600);
	wifly.begin(&Serial, NULL);

Example code to setup and use a software serial interface:

	#include <SoftwareSerial.h>
	SoftwareSerial wifiSerial(8,9);

	wifiSerial.begin(9600);
	wifly.begin(&wifiSerial);

Example code to join a WiFi network:

	wifly.setSSID("mySSID");
	wifly.setPassphrase("myWPApassword");
	wifly.enableDHCP();
	wifly.join();

Example code to send a UDP packet:

	wifly.setIpProtocol(WIFLY_PROTOCOL_UDP);
	wifly.sendto("Hello, world", "192.168.1.100", 2042);

Example code to open a TCP connection and send some data, and close the connection:

	wifly.open("192.168.1.100",8042);
	wifly.println("Hello, world!");
	wifly.close();

Example code to receive UDP or TCP data (assumes software serial interface):

	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}
