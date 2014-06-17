datapublisher: datapublisher.c mqttclient.c
	gcc datapublisher.c mqttclient.c ../gp_functions.c lib/libpaho-mqtt3/libpaho-mqtt3cs/libpaho-mqtt3cs.o -lpthread -lssl -o datapublisher
