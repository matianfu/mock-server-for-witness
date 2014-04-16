mock-server-for-witness
=======================

a simple program used with tcpserver (ucspi-tcp) to provide a mock server for wifi-ble reader test.

compile the program using:

	$ gcc formatter.c -o formatter

run the application:

	$ tcpserver 192.168.1.120 2000 ./formatter


