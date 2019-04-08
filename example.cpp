#include <iostream>
// #include "unixutil.h"	/* for socklen_t, EAFNOSUPPORT */
	#include <sys/time.h>
	#include <sys/wait.h>
#include <unistd.h>
       #include <fcntl.h>

#include <memory.h>

	#include <sys/param.h>
	#if defined(__sun)
		#include <fcntl.h>
		#include <sys/sockio.h>
	#elif (defined(BSD) && BSD >= 199306) || defined (__FreeBSD_kernel__)
		#include <ifaddrs.h>
		/* Do not move or remove the include below for "sys/socket"!
		 * Will break FreeBSD builds. */
		#include <sys/socket.h>
	#endif
	#include <arpa/inet.h>  /* for inet_pton() */
	#include <net/if.h>
	#include <netinet/in.h>

	/*! This typedef makes the code slightly more WIN32 tolerant.
	 * On WIN32 systems, SOCKET is unsigned and is not a file
	 * descriptor. */
	typedef int SOCKET;

	/*! INVALID_SOCKET is unsigned on win32. */
	#define INVALID_SOCKET (-1)

	/*! select() returns SOCKET_ERROR on win32. */
	#define SOCKET_ERROR (-1)

typedef unsigned short ushort;
typedef unsigned long ulong;

int sock_make_blocking(SOCKET sock)
{
#ifdef _WIN32
	u_long val = 0;
	return ioctlsocket(sock, FIONBIO, &val);
#else
	int val;

	val = fcntl(sock, F_GETFL, 0);
	// SOCK_NONBLOCK
	// if (fcntl(sock, F_SETFL, val & ~O_NONBLOCK) == -1) {
	if (fcntl(sock,  F_SETFL, val & ~SOCK_NONBLOCK) == -1) {
		return -1;
	}
#endif
	return 0;
}


int sock_make_no_blocking(SOCKET sock)
{
#ifdef _WIN32
	u_long val = 1;
	return ioctlsocket(sock, FIONBIO, &val);
#else /* _WIN32 */
	int val;

	val = fcntl(sock, F_GETFL, 0);

	// if (fcntl(sock, F_SETFL, val | O_NONBLOCK) == -1) {
	if (fcntl(sock,  F_SETFL, val | SOCK_NONBLOCK) == -1) {
		return -1;
	}
#endif /* _WIN32 */
	return 0;
}


bool upnp_discover(ushort Tries)
{
	SOCKET              Sock;
	struct sockaddr_in  Addr;
	char                Buffer[1450],
		*Begin = NULL,
		*End = NULL;
	int                 i = 0,
		t = 0,
		Ret = 0,
		TrueLen = sizeof(bool);
	bool                True = true;
	ulong               One = 1,
		Zero = 0;
	
	// WSADATA wsaData;
	// WSAStartup(MAKEWORD(2, 2), &wsaData);

	
	// Sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	Sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (Sock < 0) {
		std::cout << "socket err\n";
		return false;
	}

/*
	this.MULTICAST_ADDR = '239.255.255.250';
	this.MULTICAST_PORT = 1900;
*/

#define upnp_broadcast_port 1900
#define upnp_broadcast_ip "239.255.255.250"

#define upnp_search_request "M-SEARCH * HTTP/1.1\r\nHOST: 239.255.255.250:1900\r\nST: upnp:rootdevice\r\nMAN: 'ssdp:discover'\r\nMX: 3\r\n\r\n"


	memset(&Addr, 0, sizeof(Addr));
	Addr.sin_family = AF_INET;
	Addr.sin_port = htons(upnp_broadcast_port);
	Addr.sin_addr.s_addr = inet_addr(upnp_broadcast_ip);

	int so_broadcast = 1;
	Ret = setsockopt(Sock, SOL_SOCKET, SO_BROADCAST, (char *)&so_broadcast, sizeof so_broadcast);
	std::cout << Ret << " setsockopt\n";

	for (i = 0; i < Tries; ++i)
	{
		memset(&Buffer, 0, sizeof(Buffer));

		strcpy(Buffer, upnp_search_request);
		// strcpy_s(Buffer, upnp_search_request);

		auto sended = sendto(Sock, Buffer, strlen(Buffer), 0, (struct sockaddr*)&Addr, sizeof(Addr));
		std::cout << strlen(Buffer)  << " buffer " << sended << " sended\n";

		for (t = 0; t < 10; ++t)
		{

			// ioctlsocket(Sock, FIONBIO, &One);
			sock_make_no_blocking(Sock);

			memset(&Buffer, 0, sizeof(Buffer));
			Ret = recvfrom(Sock, Buffer, (sizeof(Buffer) - 1), 0, NULL, NULL);
			if (Ret == SOCKET_ERROR)
			{
				std::cout << "err\n";
				sleep(1);
				continue;
			}
			else {
				Begin = strstr(Buffer, "http://");
				if (Begin != NULL)
				{
					End = strchr(Begin, '\r');
					if (End != NULL)
					{
						std::cout << Buffer << "\n";
						/*
						*End = '\0';
						// strncpy(Device->Location, Begin, (sizeof(Device->Location) - 1));
						// upnp_parse_url(Device);
						closesocket(Sock);
						return TRUE;
						*/
						return true;
					}
				}
			}
		}

		sock_make_blocking(Sock);
		// ioctlsocket(Sock, FIONBIO, &Zero);
	}
	// closesocket(Sock);
	close(Sock);
	return false;
}

int main()
{
	upnp_discover(2);
    std::cout << "Hello World!\n"; 
}

