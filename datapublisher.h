#define CFG_FILE_PATH "datapublisher.cfg"

#define INOTIFY_EVENT_SIZE  ( sizeof (struct inotify_event) )
#define INOTIFY_EVENT_BUF_LEN     ( 1024 * ( INOTIFY_EVENT_SIZE + 16 ) )

typedef struct lcc
{
	char address[32+1];
} LCC;
