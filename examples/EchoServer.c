#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include "../WuHost.h"


#include <sys/types.h>

#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>

int guard(int n, char * err) { if (n == -1) { perror(err); exit(1); } return n; }

int main(int argc, char ** argv) {
	const char * hostAddr = "127.0.0.1";
	const char * port = "1234";
	const char * dstHostAddr = "127.0.0.1";
	const char * dstPort = "1234";
	int32_t maxClients = 2;

	if (argc == 5) {
		hostAddr = argv[1];
		port = argv[2];
		dstHostAddr = argv[3];
		dstPort = argv[4];
	} else {
		printf("Usage: ip port dstip dstport\n");
		return 1;
	}


    int udpsock;
    struct sockaddr_in6 addr;

    udpsock = guard(socket(AF_INET6, SOCK_DGRAM, 0),"sock creation fialed??");
	
 
	
	int no = 0;     
	setsockopt(udpsock, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(no)); 
	int yes = 1;
	setsockopt(udpsock, SOL_SOCKET, SO_REUSEADDR, (void *)&yes, sizeof(yes)); 
		
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(atoi(dstPort));
    inet_pton(AF_INET6, dstHostAddr, &addr.sin6_addr);
    
	guard(connect(udpsock, (struct sockaddr *)&addr, sizeof(addr)),"connect fail");

	int flags = guard(fcntl(udpsock, F_GETFL), "could not get flags on TCP listening socket");
	guard(fcntl(udpsock, F_SETFL, flags | O_NONBLOCK), "could not set TCP listening socket to be non-blocking");

	WuHost * host = NULL;

	int32_t status = WuHostCreate(hostAddr, port, maxClients, & host);
	if (status != WU_OK) {
		printf("failed to create host\n");
		return 1;
	}
	printf("Started listening\n");
	for (;;) {
		WuEvent evt;
		while (WuHostServe(host, & evt, 50)) {
			switch (evt.type) {
			case WuEvent_ClientJoin:
				{
					WuAddress claddr = WuClientGetAddress(evt.client);
					printf("%s%s: client join: %i.%i.%i.%i\n",hostAddr,port,          
						(claddr.host >> 24) & 0xFF,
						(claddr.host >> 16) & 0xFF,
						(claddr.host >> 8) & 0xFF,
						 claddr.host & 0xFF);
					break;
				}
			case WuEvent_ClientLeave:
				{
					WuAddress claddr = WuClientGetAddress(evt.client);
					printf("%s%s: client leave: %i.%i.%i.%i\n",hostAddr,port,          
						(claddr.host >> 24) & 0xFF,
						(claddr.host >> 16) & 0xFF,
						(claddr.host >> 8) & 0xFF,
						 claddr.host & 0xFF);
					WuHostRemoveClient(host, evt.client);
					break;
				}
			case WuEvent_TextData:
				{
					const char * text = (const char * ) evt.data;
					int32_t length = evt.length;
					WuHostSendText(host, evt.client, text, length);

					break;
				}
			case WuEvent_BinaryData:
				{
					//WuHostSendBinary(host, evt.client, evt.data, evt.length);
					send(udpsock,evt.data,evt.length,0);
					break;
				}
			default:
				break;
			}
		}
	}

    close(udpsock);

	return 0;
}
