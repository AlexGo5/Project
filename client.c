/*
Здесь определены все функции, связанные с использованием приложений на стороне клиента.

*/

#include "client.h"

struct PACKET *np;

int main()
{
	struct sockaddr_in serv_addr;

	int server, x;
	struct command *uComm;

	// Creating client socket

	if ((server = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("error creating control socket : Returning Error No:%d\n", errno);
		return errno;
	}

	char IPSERVER[100];
	printf("\nEnter the IP of the server\n");
	fgets(IPSERVER, 100, stdin);

	memset((char *)&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(IPSERVER);
	serv_addr.sin_port = htons(PORTSERVER);

	// connection attempt

	if ((x = connect(server, (struct sockaddr *)&serv_addr, size_sockaddr)) < 0)
	{
		fprintf(stderr, "Error connecting to server at %s:%d : Returning Error No:%d\n", IPSERVER, PORTSERVER, errno);
		return errno;
	}

	struct PACKET *hp;
	np = (struct PACKET *)malloc(sizeof(struct PACKET));
	recv(server, np, sizeof(struct PACKET), 0);
	hp = ntohp(np);
	if (hp->flag == ERR)
	{
		printf("\nAccess denied. Server is full.\n");
		return 0;
	}
	printf("FTP Client started : \n\n"); // User message
	char userInput[U_INPUTLEN];
	while (1)
	{
		printf("\nftp>");
		fgets(userInput, U_INPUTLEN, stdin); // User input
		char *pos;
		if (userInput[0] == '\n')
			goto def;
		if ((pos = strchr(userInput, '\n')) != NULL)
			*pos = '\0';
		uComm = getUserCommand(userInput); // For parsing user input and entering information into the command structure

		switch (uComm->id)
		{
		case CPWD:
			getCurrentWorkingDir(); // For client side pwd
			break;
		case CLS:
			listContentsDir(); // For client side ls
			break;
		case CCD: // For client side cd
			if (chdir(uComm->path) < 0)
			{
				fprintf(stderr, "Error changing directory, probably non-existent..!!\n");
			}
			else
			{
				printf("Current working client directory is :\n");
				getCurrentWorkingDir();
			}
			break;
		case PWD:
			server_pwd(server); // For server side pwd
			break;
		case LS:
			server_ls(server); // For server side ls
			break;
		case CD:
			server_cd(uComm, server); // For server side cd
			break;
		case GET:
			server_get(uComm, server); // For get operation
			break;
		case PUT:
			server_put(uComm, server); // For put operation
			break;
		case GETARC:
			server_getArc(uComm, server);
			break;
		case HELP:
			break;
		case QUIT:
			// client_QUIT(uComm, server);
			goto outsideLoop;
		default:
		def:
			fprintf(stderr, "Incorrect command..!!\n");
			break;
		}
	}
outsideLoop: // label for breaking out of loop

	close(server); // letting server know that socket is closing
	printf("\n\nDONE !!\n");

	fflush(stdout);
	return 0;
}

// Client side pwd............................................................................
void getCurrentWorkingDir()
{
	char dir[DATALEN];
	if (!getcwd(dir, DATALEN))
		fprintf(stderr, "Error getting current directory..!!\n");
	else
		printf("%s\n", dir);
}

// Client side ls.............................................................................
void listContentsDir()
{
	char cwd[DATALEN];
	DIR *dir;
	struct dirent *e;
	if (!getcwd(cwd, DATALEN))
	{
		fprintf(stderr, "Error .. !!\n");
		exit(1);
	}
	else
	{
		if ((dir = opendir(cwd)) != NULL)
		{
			while ((e = readdir(dir)) != NULL)
			{
				printf("\n%s", e->d_name);
			}
		}
		else
			fprintf(stderr, "Error opening: %s !!\n", cwd);
	}
	printf("\n");
	fflush(stdout);
}

// Server side pwd..............................................................................
void server_pwd(int sfd)
{
	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	hp->flag = OK;
	hp->commid = PWD;
	strcpy(hp->data, "");
	hp->len = strlen(hp->data);

	np = htonp(hp);
	int sent, bytes;

	if ((sent = send(sfd, np, size_packet, 0)) < 0)
	{
		fprintf(stderr, "Error sending packets !!\n");
		return;
	}

	if ((bytes = recv(sfd, np, size_packet, 0)) <= 0)
	{
		fprintf(stderr, "Error receiving packet..!!\n");
		return;
	}

	hp = ntohp(np);
	printf("Current Server directory :\n%s\n", hp->data);
}

struct PACKET* correct_receive(int sfd, struct PACKET *np, struct PACKET *hp)
{
	while (1)
	{
		recv(sfd, np, sizeof(struct PACKET), 0);
		send(sfd, np, sizeof(struct PACKET), 0);
		recv(sfd, np, sizeof(struct PACKET), 0);
		hp = ntohp(np);
		if (hp->flag == END || hp->flag == DONE || hp->flag == ERR)
			break;
	}
	return hp;
}

// Server side ls...............................................................................
void server_ls(int sfd)
{
	int bytes, sent;
	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	hp->flag = OK;
	strcpy(hp->data, "");
	hp->len = strlen(hp->data);
	hp->commid = LS;

	np = htonp(hp);
	if ((sent = send(sfd, np, size_packet, 0)) < 0)
	{
		fprintf(stderr, "Error sending packets !!\n");
		return;
	}

	// if ((bytes = recv(sfd, np, size_packet, 0)) <= 0)
	// {
	// 	fprintf(stderr, "Error receiving packet !!\n");
	// 	return;
	// }
	// hp = ntohp(np);
	do
	{
		hp = correct_receive(sfd, np, hp);
		printf("%s\n", hp->data);
		//printf("%d\n", hp->flag);
	}while (hp->flag != DONE && hp->flag != ERR);
}

// Server side cd................................................................................
void server_cd(struct command *cmd, int sfd)
{
	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	int bytes, sent;

	hp->flag = OK;
	hp->commid = CD;
	strcpy(hp->data, cmd->path);
	hp->len = strlen(hp->data);

	np = htonp(hp);

	if ((sent = send(sfd, np, size_packet, 0)) < 0)
	{
		fprintf(stderr, "Error sending packets !!\n");
		return;
	}

	if ((bytes = recv(sfd, np, size_packet, 0)) <= 0)
	{
		fprintf(stderr, "Error receiving packet..!!\n");
		return;
	}

	hp = ntohp(np);
	printf("Current Server directory :\n%s\n", hp->data);
}

// GET Operation........................................................................................
void server_get(struct command *cmd, int sfd)
{

	FILE *out = fopen(cmd->fileName, "wb");
	if (!out)
	{
		fprintf(stderr, "Error creating file. See if required permissions are satisfied !!\n");
		return;
	}

	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	int sent, bytes;

	hp->commid = GET;
	hp->flag = OK;
	strcpy(hp->data, cmd->path);
	hp->len = strlen(hp->data);

	np = htonp(hp);

	if ((sent = send(sfd, np, size_packet, 0)) < 0)
	{
		fprintf(stderr, "Error sending packets !!\n");
		return;
	}

	if ((bytes = recv(sfd, np, sizeof(struct PACKET), 0)) <= 0)
	{
		fprintf(stderr, "Error receiving packets !!\n");
		fclose(out);
		char tarCommand[256] = "";
		strcpy(tarCommand, "rm ");
		strcat(tarCommand, cmd->fileName);
		system(tarCommand);
		printf("\nThis file doesn't exist\n");
		return;
	}

	hp = ntohp(np);
	if (hp->flag == ERR)
	{
		fclose(out);
		char tarCommand[256] = "";
		strcpy(tarCommand, "rm ");
		strcat(tarCommand, cmd->fileName);
		system(tarCommand);
		printf("\nThis file doesn't exist\n");
		return;
	}
	else if (!recvFile(hp, np, sfd, out))
	{
		fclose(out);
		char tarCommand[256] = "";
		strcpy(tarCommand, "rm ");
		strcat(tarCommand, cmd->fileName);
		system(tarCommand);
		printf("\nThis file doesn't exist\n");
	}
	else
		fclose(out);
}

void server_getArc(struct command *cmd, int sfd)
{

	char *name = cmd->fileName;
	int i = 0;
	for (; name[i] != '.' && name[i] != '\0'; i++)
		;
	name[i] = '\0';
	strcat(name, ".tar.gz");

	FILE *out = fopen(name, "wb");
	if (!out)
	{
		fprintf(stderr, "Error creating file. See if required permissions are satisfied !!\n");
		return;
	}

	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	int sent, bytes;

	hp->commid = GETARC;
	hp->flag = OK;
	strcpy(hp->data, cmd->path);
	hp->len = strlen(hp->data);

	np = htonp(hp);

	if ((sent = send(sfd, np, size_packet, 0)) < 0)
	{
		fprintf(stderr, "Error sending packets !!\n");
		return;
	}

	if (!recvFile(hp, np, sfd, out))
	{
		fclose(out);
		char tarCommand[256] = "";
		strcpy(tarCommand, "rm ");
		strcat(tarCommand, name);
		system(tarCommand);
		printf("\nThis file doesn't exist\n");
	}
	else
		fclose(out);
}

// PUT Operation.........................................................................................
void server_put(struct command *cmd, int sfd)
{
	printf("%s\n", cmd->path);
	FILE *in = fopen(cmd->path, "r+b");
	if (!in)
	{
		fprintf(stderr, "Error opening file. See if pemissions are satisfied. Сheck the existence of the file!!\n");
		return;
	}

	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));
	int sent, bytes;

	hp->commid = PUT;
	hp->flag = OK;
	strcpy(hp->data, cmd->fileName);
	hp->len = strlen(hp->data);

	np = htonp(hp);

	if ((sent = send(sfd, np, size_packet, 0)) <= 0)
	{
		fprintf(stderr, "Error sending packets !!\n");
		return;
	}

	if ((bytes = recv(sfd, np, size_packet, 0)) <= 0)
	{
		fprintf(stderr, "Error receiving confirmation packets !!\n");
		return;
	}

	hp = ntohp(np);
	// strcpy(hp->data, cmd->fileName)
	if (hp->flag == READY)
	{
		sendFile(hp, sfd, in);
	}
	else
	{
		fprintf(stderr, "Error creating file at server !!\n");
	}
	fclose(in);
}

// User Input Analysis...............................................................................
struct command *getUserCommand(char *input)
{
	char n[DATALEN + 2];
	strcpy(n, "");
	strcat(n, " ");
	strcat(n, input);
	strcat(n, " ");
	char *option, *last;
	struct command *cmd = (struct command *)malloc(sizeof(struct command));
	strcpy(cmd->path, "");
	strcpy(cmd->fileName, "");
	cmd->id = -2;
	option = strtok(input, " \t");
	if (!strcmp(option, "!pwd"))
	{
		cmd->id = CPWD;
	}

	else if (!strcmp(option, "!ls"))
	{
		cmd->id = CLS;
	}

	else if (!strcmp(option, "!cd"))
	{
		cmd->id = CCD;
		option = strtok(NULL, " ");
		if (!option)
		{
			cmd->id = ERR;
			return cmd;
		}
		strcpy(cmd->path, option);
	}

	else if (!strcmp(option, "pwd"))
	{
		cmd->id = PWD;
	}

	else if (!strcmp(option, "ls"))
	{
		cmd->id = LS;
	}

	else if (!strcmp(option, "cd"))
	{
		cmd->id = CD;
		option = strtok(NULL, " ");
		if (!option)
		{
			cmd->id = ERR;
			return cmd;
		}
		strcpy(cmd->path, option);
	}

	else if (!strcmp(option, "get"))
	{
		cmd->id = GET;
		option = strtok(NULL, " ");
		if (!option)
		{
			cmd->id = ERR;
			return cmd;
		}
		strcpy(cmd->path, option);
		while (option != NULL)
		{
			last = option;
			option = strtok(NULL, "/");
		}
		strcpy(cmd->fileName, last);
	}
	else if (!strcmp(option, "getArc"))
	{
		cmd->id = GETARC;
		option = strtok(NULL, " ");
		if (!option)
		{
			cmd->id = ERR;
			return cmd;
		}
		strcpy(cmd->path, option);
		while (option != NULL)
		{
			last = option;
			option = strtok(NULL, "/");
		}

		strcpy(cmd->fileName, last);
	}
	else if (!strcmp(option, "put"))
	{
		cmd->id = PUT;
		option = strtok(NULL, " ");
		if (!option)
		{
			cmd->id = ERR;
			return cmd;
		}
		strcpy(cmd->path, option);
		while (option != NULL)
		{
			last = option;
			option = strtok(NULL, "/");
		}
		strcpy(cmd->fileName, last);
	}
	else if (!strcmp(option, "help"))
	{
		printf("%s", STRHELP);
		cmd->id = HELP;
	}
	else if (!strcmp(option, "quit"))
	{
		cmd->id = QUIT;
	}

	return cmd;
}

void client_QUIT(struct command *cmd, int sfd)
{
	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));

	hp->flag = QUIT;
	hp->commid = QUIT;
	strcpy(hp->data, "");
	hp->len = 0;

	np = htonp(hp);

	int sent = send(sfd, np, size_packet, 0);
}