/*
Contains server specific declarations and function prototypes..

*/


#include "commons.h"
#include <pthread.h>

#define C_PORT 5555
#define IP "127.0.0.1"
#define BACKLOG 20
#define OPTLEN 10
#define CLIENTS 2


//Threading specific declarations................................................
struct THREADINFO{
	pthread_t thread_ID;
	int sockfd;
	char curr_dir[DATALEN];
};

//Linked list structure.........................................................
struct LLNODE{
	struct THREADINFO threadinfo;
	struct LLNODE *next;
};


//Doubly linked list structure..................................................
struct LLIST{
	struct LLNODE *head, *tail;
	int size;
};

int server, sfd;
char data_buff[DATALEN];

struct THREADINFO thread_info[CLIENTS];
struct LLIST client_list;
pthread_mutex_t clientlist_mutex;
struct LLNODE *curr;

//Server specific function prototypes
void client_handler(void *);

void client_pwd(struct PACKET *, struct THREADINFO);
void client_ls(struct PACKET *, struct THREADINFO);
void client_cd(struct PACKET *, struct THREADINFO *);
void client_get(struct PACKET *, struct THREADINFO);
void client_put(struct PACKET *, struct THREADINFO);
//void client_quit(struct PACKET *, struct THREADINFO);
//void getFileNameFromPath(char *);

void list_init(struct LLIST *);
int list_insert(struct LLIST *, struct THREADINFO *);
int list_delete(struct LLIST *, struct THREADINFO *);
//void list_dump(struct LLIST *);
int compare(struct THREADINFO *, struct THREADINFO *);