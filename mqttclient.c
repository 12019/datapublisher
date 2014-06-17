#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datapublisher.h"
#include "mqttclient.h"
#include "lib/libpaho-mqtt3/MQTTAsync.h"

volatile MQTTAsync_token deliveredtoken; 

int mqtt_finished = 0;

char* mqtt_payload = NULL;
char* mqtt_topic = NULL;

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
 		mqtt_finished = 1;
	}
}

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	printf("Successful disconnection\n");
	mqtt_finished = 1;
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
	mqtt_finished = 1;
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

	pubmsg.payload = mqtt_payload;
	pubmsg.payloadlen = strlen(mqtt_payload);
	pubmsg.qos = QOS;
	pubmsg.retained = 0;
	deliveredtoken = 0;

	if ((rc = MQTTAsync_sendMessage(client, mqtt_topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
 		exit(-1);	
	}
}

int mqtt_client_send(LCC lcc, char* topic, void* payload)
{
	mqtt_finished = 0;
	deliveredtoken = 1;
	
	MQTTAsync client;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_token token;
	int rc;
	
	
	mqtt_payload = (char*) payload;
	mqtt_topic = topic;
	fprintf(stderr, "hey0\n");
	MQTTAsync_create(&client, lcc.address, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	fprintf(stderr, "hey1\n");
	MQTTAsync_setCallbacks(client, NULL, connlost, NULL, NULL);
	
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
	conn_opts.context = client;
	fprintf(stderr, "%s\n", lcc.address);
	
	rc = MQTTAsync_connect(client, &conn_opts);fprintf(stderr, "hey2\n");
	if (rc != MQTTASYNC_SUCCESS) {
		printf("Failed to start connect, return code %d\n", rc);
		exit(-1);	
	}

	printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", mqtt_payload, mqtt_topic, CLIENTID);
		 
	while (!mqtt_finished)
		usleep(10000L);
				
	MQTTAsync_destroy(&client);
	
 	return rc;
}
