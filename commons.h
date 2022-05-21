/*
Здесь содержатся все общие элементы, а именно: Структура пакетов, общие заголовочные файлы, макросы и т.д.
Любое изменение здесь должно быть соответствующим образом обновлено во всех других файлах.

*/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include <dirent.h>

#define ERR -1
#define READY 0
#define OK 1
#define DONE 2
#define GET 3
#define PUT 4
#define PWD 5
#define LS 6
#define CD 7
#define CPWD 8
#define CLS 9
#define CCD 10
#define QUIT 11
#define GETARC 12
#define DATALEN 504
#define STRHELP "!pwd - Present working directory of client\n!ls - Show directory contents of client\n!cd path - Change client directory\npwd - Present working directory of server. May differ from client to client.\nls - Show server directory contents\ncd path - Change working server directory to that indicated by path\nget fileName - Download file with FileName at the client\nput fileName - Upload file with fileName at the server\nquit - Exit client, and delete related threadinfo\n\n"

//PACKET structure
struct PACKET{
	char data[DATALEN];
	int flag;
	int len;
	int commid;
};

struct PACKET * ntohp(struct PACKET *);
struct PACKET * htonp(struct PACKET *);

void sendFile(struct PACKET *, int, FILE *);

void recvFile(struct PACKET *, struct PACKET *, int, FILE *);