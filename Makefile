datapublisher: datapublisher.c
	gcc datapublisher.c ../gp_functions.c lib/libpaho-mqtt3/libpaho-mqtt3as/libpaho-mqtt3as.o -lpthread -lssl -o datapublisher