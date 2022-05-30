/*
Содержит реализации, которые характерны как и для сервера, так и для клиента.
*/

#include "commons.h"

// Correcting packet bit storage for usage in host
struct PACKET *ntohp(struct PACKET *np)
{
	struct PACKET *hp = (struct PACKET *)malloc(sizeof(struct PACKET));

	hp->len = ntohs(np->len);
	hp->flag = ntohs(np->flag);
	hp->commid = ntohs(np->commid);
	memcpy(hp->data, np->data, DATALEN);

	return hp;
}

// Correcting packet bit storage for usage in network
struct PACKET *htonp(struct PACKET *hp)
{
	struct PACKET *np = (struct PACKET *)malloc(sizeof(struct PACKET));

	np->len = htons(hp->len);
	np->flag = htons(hp->flag);
	np->commid = htons(hp->commid);
	memcpy(np->data, hp->data, DATALEN);

	return np;
}

// Common function for sending file
void sendFile(struct PACKET *hp, int sfd, FILE *fp)
{
	// size_t read;
	struct PACKET *np, *hpx = (struct PACKET *)malloc(sizeof(struct PACKET)), *npx = (struct PACKET *)malloc(sizeof(struct PACKET));
	;
	// char fName[DATALEN];
	// strcpy(fName, hp->data);
	int sent, total_sent = 0, error = 0, j = 0, bytes;
	int flag;
	while (!feof(fp))
	{
		memset(hp->data, '\0', sizeof(char) * DATALEN);
		hp->len = fread(hp->data, 1, DATALEN - 1, fp);
		hp->flag = OK;
		np = htonp(hp);
		while (1)
		{
			flag = 1;
			sent = send(sfd, np, sizeof(struct PACKET), 0);
			if (sent != sizeof(struct PACKET))
			{
				flag = 0;
			}
			if ((bytes = recv(sfd, npx, sizeof(struct PACKET), 0)) <= 0)
			{
				flag = 0;
			}
			hpx = ntohp(npx);
			if (flag && hp->len == hpx->len)
			{
				total_sent += hp->len;
				break;
			}
			else
			{
				sent = send(sfd, np, sizeof(struct PACKET), 0);
				if (sent != sizeof(struct PACKET))
				{
					error++;
					goto outsideLoop;
				}
			}
		}
		hp->flag = END;
		np = htonp(hp);
		sent = send(sfd, np, sizeof(struct PACKET), 0);
		if (sent != sizeof(struct PACKET))
		{
			error++;
			goto outsideLoop;
		}
		j++;
	}
outsideLoop:
	if (error)
	{
		hp->flag = ERR;
		strcpy(hp->data, "");
		hp->len = 0;
		np = htonp(hp);
		// sent = send(sfd, np, sizeof(struct PACKET), 0);
	}
	else
	{
		hp->flag = DONE;
		strcpy(hp->data, "");
		hp->len = 0;
		np = htonp(hp);
		// sent = send(sfd, np, sizeof(struct PACKET), 0);
	}
	while (1)
	{
		flag = 1;
		sent = send(sfd, np, sizeof(struct PACKET), 0);
		if (sent != sizeof(struct PACKET))
		{
			flag = 0;
		}
		if ((bytes = recv(sfd, npx, sizeof(struct PACKET), 0)) <= 0)
		{
			flag = 0;
		}
		hpx = ntohp(npx);
		if (flag && hp->len == hpx->len)
		{
			sent = send(sfd, np, sizeof(struct PACKET), 0);
			break;
		}
		else
		{
			hpx->flag = OK;
			npx = htonp(hpx);
			sent = send(sfd, np, sizeof(struct PACKET), 0);
		}
	}
	fprintf(stderr, "%d bytes read.\n%d packets sent\n", total_sent, j);
	fflush(stderr);
}

// Common function for receiving file
int recvFile(struct PACKET *hp, struct PACKET *np, int sfd, FILE *out)
{
	int bytes, total_bytes = 0, j = 0, sent;

	struct PACKET *npx = (struct PACKET *)malloc(sizeof(struct PACKET));
	;
	struct PACKET *hpx = (struct PACKET *)malloc(sizeof(struct PACKET));
	// if((bytes = recv(sfd, np, sizeof(struct PACKET), 0)) <= 0){
	// 	fprintf(stderr, "Error receiving file !!\n");
	// 	return -1;
	// }
	// j++;
	// hp = ntohp(np);
	while (hp->flag != DONE && hp->flag != ERR)
	{
		int flag = 1;
		while (1)
		{
			flag = 1;
			if ((bytes = recv(sfd, np, sizeof(struct PACKET), 0)) <= 0)
			{
				// fprintf(stderr, "Error receiving file !!\n");
				flag = 0;
			}

			sent = send(sfd, np, sizeof(struct PACKET), 0);
			if (sent != sizeof(struct PACKET))
			{
				flag = 0;
			}

			if ((bytes = recv(sfd, npx, sizeof(struct PACKET), 0)) <= 0)
			{
				// fprintf(stderr, "Error receiving file !!\n");
				flag = 0;
			}

			hpx = ntohp(npx);

			if (flag && (hpx->flag == END || hpx->flag == DONE || hpx->flag == ERR))
			{
				break;
			}
		}

		j++;
		hp = ntohp(np);

		if (hp->len > 0)
		{
			total_bytes += fwrite(hp->data, 1, hp->len, out);
		}
	}
	if (hp->flag == ERR)
	{
		fprintf(stderr, "Error sending file !!\n");
	}
	fprintf(stderr, "%d bytes written\n%d packets received\n", total_bytes, j);
	fflush(stderr);
	return total_bytes;
}