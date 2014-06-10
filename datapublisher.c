#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datapublisher.h"
#include "lib/libpaho-mqtt3/MQTTAsync.h"
#include "../gp_functions.h"

#define ADDRESS     "tcp://10.0.0.57:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "Buoy_1/NaI/status"
#define PAYLOAD     "0"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTAsync_token deliveredtoken; 

int finished = 0;

void connlost(void *context, char *cause)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;

	printf("\nConnection lost\n");
	printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
 		finished = 1;
	}
}


void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	finished = 1;
}


void onSend(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	printf("Message with token value %d delivery confirmed\n", response->token);

	opts.onSuccess = onDisconnect;
	opts.context = client;

	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(-1);	
	}
}


void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	printf("Successful connection\n");
	
	opts.onSuccess = onSend;
	opts.context = client;

	pubmsg.payload = PAYLOAD;
	pubmsg.payloadlen = strlen(PAYLOAD);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	deliveredtoken = 0;

	if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
 		exit(-1);	
	}
}

int init_lcc(LCC **lccs, int *lccs_len)
{
	char tmpaddress[32+1] = "";
	char tmpstr[32+1] = "";
	
	if (readconf(CFG_FILE_PATH, "lcc.address", tmpaddress, 0) != 0) {
		*lccs = malloc(sizeof(LCC));
		if (*lccs == NULL) {
			fprintf(stderr, "datapublisher: error: init_lcc: malloc: ");
			perror(NULL);
			return 1;
		}
		
		strcpy((*lccs)->address, tmpaddress);
		*lccs_len = 1;
	}
	else if (readconf(CFG_FILE_PATH, "lcc.0.address", tmpaddress, 0) != 0) {
		int len=1, i=0;
	
		sprintf(tmpstr, "lcc.%d.address", len);
		while (readconf(CFG_FILE_PATH, tmpstr, tmpaddress, 0) != 0) {
			len++;
			sprintf(tmpstr, "lcc.%d.address", len);
		}
		
		*lccs = malloc(len * sizeof(LCC));
		if (*lccs == NULL) {
			fprintf(stderr, "datapublisher: error: init_lcc: malloc: ");
			perror(NULL);
			return 1;
		}
		
		for (i=0; i<len; i++) {
			sprintf(tmpstr, "lcc.%d.address", i);
			readconf(CFG_FILE_PATH, tmpstr, tmpaddress, 0);
			strcpy((*lccs+i)->address, tmpaddress);
		}
		*lccs_len = len;
	}
	else {
		fprintf (stderr, "datapublisher: error: init_lcc: no LCCs found in config file.\n");
		return 1;
	}
	
	return 0;
}

int main(int argc, char* argv[])
{
	LCC *lccs = NULL;
	int lccs_len = 0;
	
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_token token;
	int rc;
	
	if (init_lcc(&lccs, &lccs_len) != 0) {
		exit(EXIT_FAILURE);
	}
	
	//fprintf(stderr, "%s\n", lccs[1].address);
	
	MQTTAsync_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	MQTTAsync_setCallbacks(client, NULL, connlost, NULL, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(-1);	
	}

	printf("Waiting for publication of %s\n"
         "on topic %s for client with ClientID: %s\n",
         PAYLOAD, TOPIC, CLIENTID);
	while (!finished)
		#if defined(WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	MQTTAsync_destroy(&client);
 	return rc;
}
