#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/inotify.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "datapublisher.h"

#include "../gp_functions.h"
#include "../msg_queue.h"

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
	char dp_station_nr[8 + 1] = "";
	
	LCC *lccs = NULL;
	int lccs_len = 0;
	
	/*int in_fd = 0;
	int in_wd = 0;
	int in_len = 0;
	char in_buffer[INOTIFY_EVENT_BUF_LEN];*/
	
	int msg_id = 0;
	int msg_rc = 0;
	
	//char tmpstr[32+1] = "";
	//struct msqid_ds	msg_queue;
	
	// get station number
	if (readconf(CFG_FILE_PATH, "stationnr", dp_station_nr, 0) == 0) {
		fprintf(stderr, "datapublisher: error: main: readconf: could not gain station number\n");
		exit(EXIT_FAILURE);
	}
	
	// initialize LCCs
	if (init_lcc(&lccs, &lccs_len) != 0) {
		fprintf(stderr, "datapublisher: error: main: init_lcc: could not initiate LCCs\n");
		exit(EXIT_FAILURE);
	}
	
	// initialize message queue
	msg_id = msgget(10021, IPC_CREAT | 0666);
	if (msg_id < 0) {
		fprintf(stderr, "datapublisher: error: msgget: ");
		perror(NULL);
	}
	
	/*msgctl(msg_id, IPC_STAT, &msg_queue);
	while(msg_queue.msg_qnum == 0) {
		msgctl(msg_id, IPC_STAT, &msg_queue);
		usleep(100000L);
	}
	fprintf(stderr, "%d\n", msg_queue.msg_qnum);*/
	
	while (1) {
		msg_rc = msgrcv(msg_id, &dataMsg, sizeof(MsgType), 0, 0);
		if (msg_rc < 0) {
			fprintf(stderr, "datapublisher: error: msgrcv: ");
			perror(NULL);
		}
		else {
			char payload[256 + 1] = "";
			char topic[64 + 1] = "";
			
			//printf("rcv: %s %s %s %s\n", dataMsg.date, dataMsg.type, dataMsg.origin, dataMsg.value);
			
			sprintf(payload, "{\"date\": \"%s\", \"value\": %s}", dataMsg.date, dataMsg.value);
			sprintf(topic, "%s/%s/doserate", dp_station_nr, dataMsg.origin + 18);
			
			mqtt_client_send(lccs[0], topic, payload);
			//usleep(1000000L);
		}
	}
	
	/*in_fd = inotify_init();
	if (in_fd < 0) {
		fprintf(stderr, "datapublisher: error: inotify_init: ");
		perror(NULL);
	}
	
	in_wd = inotify_add_watch(in_fd, "/usbstick/datatxt/DET/monthly", IN_CLOSE_WRITE);
	if (in_wd < 0) {
		fprintf(stderr, "datapublisher: error: inotify_add_watch: ");
		perror(NULL);
	}
	
	in_len = read(in_fd, in_buffer, INOTIFY_EVENT_BUF_LEN);
	if (in_len < 0) {
		fprintf(stderr, "datapublisher: error: read: ");
		perror(NULL);
	}
	else {
		int i = 0;
		while (i < in_len) {
			struct inotify_event *event = (struct inotify_event*) &in_buffer[i];     
			if (event->len) {
				if (event->mask & IN_CLOSE_WRITE) {
					if (strcmp(event->name + 7, "1min.csv") == 0) {
						printf("file %s has been written and closed.\n", event->name);
					}
				}
			}
			i += INOTIFY_EVENT_SIZE + event->len;
		}
	}*/
	
	

	//MQTTAsync_destroy(&client);
 	return 0;
}
