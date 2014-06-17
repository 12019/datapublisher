#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datapublisher.h"
#include "mqttclient.h"
#include "lib/libpaho-mqtt3/MQTTClient.h"

int mqtt_client_send(LCC lcc, char* topic, void* payload)
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;	
	
	int rc;
	char* mqtt_payload = NULL;
	char* mqtt_topic = NULL;
	
	mqtt_payload = (char*) payload;
	mqtt_topic = topic;
	
	MQTTClient_create(&client, lcc.address, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	
	rc = MQTTClient_connect(client, &conn_opts);
	if (rc != MQTTCLIENT_SUCCESS) {
		printf("Failed to start connect, return code %d\n", rc);
		exit(-1);	
	}

	pubmsg.payload = mqtt_payload;
	pubmsg.payloadlen = strlen(mqtt_payload);
	pubmsg.qos = MQTT_QOS;
	pubmsg.retained = 0;

	MQTTClient_publishMessage(client, mqtt_topic, &pubmsg, &token);
	printf("Waiting for publication of %s\non topic %s for client with ClientID: %s\n", mqtt_payload, mqtt_topic, MQTT_CLIENTID);	
	rc = MQTTClient_waitForCompletion(client, token, MQTT_TIMEOUT);
	printf("Message with delivery token %d delivered\n", token);

	MQTTClient_disconnect(client, MQTT_TIMEOUT);
	MQTTClient_destroy(&client);				
	
 	return rc;
}
