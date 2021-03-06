/*
Contains server specific function implementations..

*/

#include "server.h"

char addr_str[INET_ADDRSTRLEN];

int main(int argc, char *argv[])
{

	struct PACKET *np, *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	struct sockaddr_in serv_addr, client_addr;
	int sin_size;

	// получение локального IP
	char *IP = (char *)malloc(100);
	IP = getIP();

	/* initialize linked list */
	list_init(&client_list);

	/* initiate mutex */
	pthread_mutex_init(&clientlist_mutex, NULL);

	// Creating control socket
	if ((server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("error creating control socket : Returning Error No:%d\n", errno);
		return errno;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(C_PORT);
	serv_addr.sin_addr.s_addr = inet_addr(IP);
	memset(&(serv_addr.sin_zero), 0, 8);

	if (bind(server, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("Error binding control socket to port %d : Returning error No:%d\n", C_PORT, errno);
		return errno;
	}

	if (listen(server, BACKLOG) == -1)
	{
		printf("Error listening at control port : Returning Error No:%d\n", errno);
		return errno;
	}

	printf("\nServer started at port:%d\n", C_PORT);

	// int numClients;
	// input(numClients);
	sin_size = sizeof(struct sockaddr_in);
	while (1)
	{
		if ((sfd = accept(server, (struct sockaddr *)&client_addr, (socklen_t *)&sin_size)) == -1)
		{
			printf("Error accepting connections : Returning Error No:%d\n\n", errno);
			return errno;
		}
		if (client_list.size == CLIENTS)
		{
			fprintf(stderr, "Connection full. Request rejected !!\n");
			hp->flag = ERR;
			np = htonp(hp);
			send(sfd, np, sizeof(struct PACKET), 0);
			continue;
		}
		hp->flag = OK;
		np = htonp(hp);
		send(sfd, np, sizeof(struct PACKET), 0);
		void *addr;
		addr = &(client_addr.sin_addr);
		inet_ntop(AF_INET, addr, addr_str, sizeof(addr_str));
		printf("Connection request received from:[%d]%s\n", sfd, addr_str);
		fflush(stdout);

		struct THREADINFO threadinfo;
		threadinfo.sockfd = sfd;
		char cwd[DATALEN];
		getcwd(cwd, DATALEN);
		strcpy(threadinfo.curr_dir, cwd);
		pthread_mutex_lock(&clientlist_mutex);
		list_insert(&client_list, &threadinfo);
		pthread_mutex_unlock(&clientlist_mutex);

		pthread_create(&threadinfo.thread_ID, NULL, client_handler, (void *)&threadinfo);
	}

	return 0;
}

// void input(int *n)
// {
// 	while(1){
// 		printf("Input number of clients:\n");
// 		scanf("%d", n);
// 		if(*n < 1 || 100<*n)
// 		   printf("\nIncorrect number of clients!\n\n");
// 		else
// 		   return;
// 	}
// }

void client_handler(void *fd)
{
	int bytes, option;
	struct PACKET *np = (struct PACKET *)malloc(sizeof(struct PACKET)), *hp;

	struct THREADINFO threadinfo = *(struct THREADINFO *)fd;

	while (1)
	{
		bytes = recv(threadinfo.sockfd, np, sizeof(struct PACKET), 0);
		if (bytes == 0)
		{
			printf("Connection closed by client at:[%d]%s\n", threadinfo.sockfd, addr_str);
			pthread_mutex_lock(&clientlist_mutex);
			list_delete(&client_list, &threadinfo);
			pthread_mutex_unlock(&clientlist_mutex);
			break;
		}

		hp = ntohp(np);
		printf("[%d]command #%d\n", threadinfo.sockfd, hp->commid);
		switch (hp->commid)
		{
		case GET:
			client_get(hp, threadinfo);
			break;
		case PUT:
			client_put(hp, threadinfo);
			break;
		case LS:
			client_ls(hp, threadinfo);
			break;
		case CD:
			client_cd(hp, &threadinfo);
			break;
		case PWD:
			client_pwd(hp, threadinfo);
			break;
		case GETARC:
			client_get_arc(hp, threadinfo);
			break;
		/*case QUIT:
			client_quit(hp, threadinfo);
			break;*/
		default:
			// fprintf(stderr,"Corrupt packet received..\n\n");
			break;
		}
	}
}

char *getIP()
{
	const char *google_dns_server = "8.8.8.8";
	int dns_port = 53;

	struct sockaddr_in serv;

	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	// Socket could not be created
	if (sock < 0)
	{
		perror("Socket error");
		return NULL;
	}

	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(google_dns_server);
	serv.sin_port = htons(dns_port);

	int err = connect(sock, (const struct sockaddr *)&serv, sizeof(serv));

	struct sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (struct sockaddr *)&name, &namelen);

	char *buffer = (char *)malloc(100);
	char *p = inet_ntop(AF_INET, &name.sin_addr, buffer, 100);

	if (p != NULL)
	{
		// printf("Local ip is : %s \n", buffer);
		return buffer;
	}
	else
	{
		// Some error
		printf("Error number : %d . Error message : %s \n", errno, strerror(errno));
	}

	close(sock);

	return NULL;
}

// pwd Command...............................................................................................
void client_pwd(struct PACKET *hp, struct THREADINFO t)
{
	int sent;
	struct PACKET *np;

	strcpy(hp->data, t.curr_dir);
	hp->flag = OK;
	hp->len = strlen(hp->data);
	np = htonp(hp);
	sent = send(t.sockfd, np, sizeof(struct PACKET), 0);
}

void correct_send(int sfd, struct PACKET *np, struct PACKET *hp)
{

	struct PACKET *hpx = (struct PACKET *)malloc(sizeof(struct PACKET));
	struct PACKET *npx = (struct PACKET *)malloc(sizeof(struct PACKET));

	while (1)
	{
		send(sfd, np, sizeof(struct PACKET), 0);
		recv(sfd, np, sizeof(struct PACKET), 0);
		hpx = ntohp(np);
		if (hp->flag == hpx->flag && hp->len == hpx->len)
		{
			if (hp->flag == OK)
				hpx->flag = END;
			else
			    hpx->flag = hp->flag;
			npx = htonp(hpx);
			send(sfd, npx, sizeof(struct PACKET), 0);
			break;
		}
		else
		{
			hpx->flag = OK;
			npx = htonp(hpx);
			send(sfd, npx, sizeof(struct PACKET), 0);
		}
	}
	free(hpx);
	free(npx);
}

// ls Command.................................................................................................
void client_ls(struct PACKET *hp, struct THREADINFO t)
{

	struct PACKET *np;
	int sent;
	// char cwd[DATALEN];
	DIR *dir;
	struct dirent *e;

	if ((dir = opendir(t.curr_dir)) != NULL)
	{
		while ((e = readdir(dir)) != NULL)
		{
			strcpy(hp->data, e->d_name);
			// strcat(hp->data, "\n");
			hp->flag = OK;
			hp->len = strlen(hp->data);
			np = htonp(hp);
			correct_send(t.sockfd, np, hp);
		}
		hp->flag = DONE;
		strcpy(hp->data, "");
		hp->len = strlen(hp->data);
		np = htonp(hp);
		correct_send(t.sockfd, np, hp);
	}
	else
		hp->flag = ERR;

	if (hp->flag == ERR)
	{
		strcpy(hp->data, "Error processing 'ls' command at server..!!\n\n");
		hp->len = strlen(hp->data);
		np = htonp(hp);
		correct_send(t.sockfd, np, hp);
		// sent = send(t.sockfd, np, sizeof(struct PACKET), 0);
		//  exit(1);
	}
}

// cd Command.........................................................................................
void client_cd(struct PACKET *hp, struct THREADINFO *t)
{
	struct PACKET *np;
	int sent;

	char temp[DATALEN];
	getcwd(temp, DATALEN);

	chdir(t->curr_dir);

	if (chdir(hp->data) < 0)
	{
		hp->flag = ERR;
		strcpy(hp->data, "Error changing directory, probably non-existent\n");
		hp->len = strlen(hp->data);
		np = htonp(hp);
		sent = send(t->sockfd, np, sizeof(struct PACKET), 0);
	}

	else
	{
		getcwd(t->curr_dir, DATALEN);
		chdir(temp);
		client_pwd(hp, *t);
	}
}

// get Command..................................................................................................
void client_get(struct PACKET *hp, struct THREADINFO t)
{
	struct PACKET *np;
	int sent, total_sent = 0;
	size_t read;
	FILE *in;
	char path[496];
	strcpy(path, t.curr_dir);
	strcat(path, "/");
	strcat(path, hp->data);
	printf("File:%s\n", path);
	in = fopen(path, "r+b");

	if (!in)
	{
		hp->flag = ERR;
		hp->len = strlen(hp->data);
		np = htonp(hp);
		sent = send(t.sockfd, np, sizeof(struct PACKET), 0);

		fprintf(stderr, "Error opening file:%s\n\n", hp->data);
		return;
	}

	hp->flag = OK;
	hp->len = strlen(hp->data);
	np = htonp(hp);
	sent = send(t.sockfd, np, sizeof(struct PACKET), 0);

	sendFile(hp, t.sockfd, in);

	fclose(in);
}

void client_get_arc(struct PACKET *hp, struct THREADINFO t)
{
	int sent, total_sent = 0;
	size_t read;
	FILE *in;
	char path[496];
	char *name;
	name = hp->data;
	strcpy(path, t.curr_dir);
	strcat(path, "/");
	strcat(path, name);
	printf("File:%s\n", path);

	// in = fopen(path, "r+b");

	// if (!in)
	// {

	// 	hp->flag = ERR;
	// 	strcpy(hp->data, "Error opening file in server!");
	// 	hp->len = strlen(hp->data);
	// 	np = htonp(hp);
	// 	sent = send(t.sockfd, np, sizeof(struct PACKET), 0);

	// 	fprintf(stderr, "Error opening file:%s", hp->data);
	// 	return;
	// }
	// fclose(in);

	char newName[256];
	int i = 0;
	for (; name[i] != '.' && name[i] != '\0'; i++)
		newName[i] = name[i];
	newName[i] = '\0';

	char tarCommand[256] = "tar -cf ";
	strcat(tarCommand, newName);	//"tar -cf name"
	strcat(tarCommand, ".tar.gz "); //"tar -cf name.tar.gz "
	strcat(tarCommand, name);		//"tar -cf name.tar.gz path"
	// printf("%s", tarCommand);
	system(tarCommand);

	strcat(newName, ".tar.gz");

	in = fopen(newName, "r+b");

	if (!in)
	{
		fprintf(stderr, "Error opening file:%s", hp->data);
		return;
	}

	sendFile(hp, t.sockfd, in);

	fclose(in);

	strcpy(tarCommand, "rm ");
	strcat(tarCommand, newName);
	// printf("%s", tarCommand);
	system(tarCommand);
}

// put Command...................................................................................................
void client_put(struct PACKET *hp, struct THREADINFO t)
{
	struct PACKET *np = (struct PACKET *)malloc(sizeof(struct PACKET));
	int bytes, total_recv = 0;
	// getFileNameFromPath(hp->data);
	FILE *out;
	char path[496];
	strcpy(path, t.curr_dir);
	strcat(path, "/");
	strcat(path, hp->data);
	printf("File:%s\n", path);
	out = fopen(path, "wb");
	if (!out)
	{

		hp->flag = ERR;
		strcpy(hp->data, "Error creating file in server!");
		hp->len = strlen(hp->data);
		np = htonp(hp);
		bytes = send(t.sockfd, np, sizeof(struct PACKET), 0);

		fprintf(stderr, "Error creating file:%s", hp->data);
		// return;
	}
	else
	{

		hp->flag = READY;
		// strcpy(hp->data,"");
		hp->len = strlen(hp->data);
		np = htonp(hp);

		bytes = send(t.sockfd, np, sizeof(struct PACKET), 0);

		// Once file is created at server
		recvFile(hp, np, t.sockfd, out);
	}

	fclose(out);
}

// For comparing if two threads in arguments are the same..........................
int compare(struct THREADINFO *a, struct THREADINFO *b)
{
	return a->sockfd - b->sockfd;
}

// Initializing given list.........................................................
void list_init(struct LLIST *ll)
{
	ll->head = ll->tail = NULL;
	ll->size = 0;
}

// Insert operation on list.........................................................
int list_insert(struct LLIST *ll, struct THREADINFO *thr_info)
{
	if (ll->size == CLIENTS)
		return -1;
	if (ll->head == NULL)
	{
		ll->head = (struct LLNODE *)malloc(sizeof(struct LLNODE));
		ll->head->threadinfo = *thr_info;
		ll->head->next = NULL;
		ll->tail = ll->head;
	}
	else
	{
		ll->tail->next = (struct LLNODE *)malloc(sizeof(struct LLNODE));
		ll->tail->next->threadinfo = *thr_info;
		ll->tail->next->next = NULL;
		ll->tail = ll->tail->next;
	}
	ll->size++;
	return 0;
}

// Delete operation on list...........................................................
int list_delete(struct LLIST *ll, struct THREADINFO *thr_info)
{
	struct LLNODE *curr, *temp;
	if (ll->head == NULL)
		return -1;
	if (compare(thr_info, &ll->head->threadinfo) == 0)
	{
		temp = ll->head;
		ll->head = ll->head->next;
		if (ll->head == NULL)
			ll->tail = ll->head;
		free(temp);
		ll->size--;
		return 0;
	}
	for (curr = ll->head; curr->next != NULL; curr = curr->next)
	{
		if (compare(thr_info, &curr->next->threadinfo) == 0)
		{
			temp = curr->next;
			if (temp == ll->tail)
				ll->tail = curr;
			curr->next = curr->next->next;
			free(temp);
			ll->size--;
			return 0;
		}
	}
	return -1;
}
