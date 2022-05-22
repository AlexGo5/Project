/*
Содержит объявления, специфичные для клиента, и прототипы функций..

*/

#include "commons.h"

//#define IPSERVER "127.0.0.1"  //Это значение определяет IP-адрес, к которому клиент пытается подключиться 
#define PORTSERVER 5555			  //Порт, на котором сервер работает с указанным выше IP-адресом. Должен быть заранее известен.
#define U_INPUTLEN 1024		  //Длина пользовательского ввода

//Structure of user commands
struct command{
	int id;
	char path[DATALEN];
	char fileName[DATALEN];
};

size_t size_sockaddr = sizeof(struct sockaddr), size_packet = sizeof(struct PACKET);

struct command * getUserCommand(char *);

void getCurrentWorkingDir();
void listContentsDir();

void server_pwd(int);
void server_ls(int);
void server_cd(struct command *, int);
void server_get(struct command *, int);
void server_put(struct command *, int);
//void client_QUIT(struct command *, int);