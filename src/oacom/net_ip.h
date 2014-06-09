#pragma once
#ifndef _OACOM_NET_IP_H_
#define _OACOM_NET_IP_H_
/*
 * net_ip.h
 *
 *  Created on: 26 May 2014
 *      Author: oasookee@gmail.com
 */

#include <katina/types.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "q_shared.h"
#include "net_chan.h"

namespace oacom {

using namespace katina::types;

#define NET_ENABLEV4            0x01
#define NET_ENABLEV6            0x02
// if this flag is set, always attempt ipv6 connections instead of ipv4 if a v6 address is found.
#define NET_PRIOV6              0x04
// disables ipv6 multicast support if set.
#define NET_DISABLEMCAST        0x08

#define NET_ADDRSTRMAXLEN 48	// maximum length of an IPv6 address string including trailing '\0'
#define	PORT_ANY			-1

#define	PORT_MASTER			27950
#define	PORT_UPDATE			27951
#define	PORT_SERVER			27960
#define	NUM_SERVER_PORTS	4		// broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers
/*
====================
NET_ErrorString
====================
*/
char *NET_ErrorString( void );

//static void NetadrToSockadr( netadr_t *a, sockaddr *s );


//static void SockadrToNetadr( sockaddr *s, netadr_t *a );

//static struct addrinfo *SearchAddrInfo(addrinfo *hints, sa_family_t family);

/*
=============
Sys_StringToSockaddr
=============
*/
//static bool Sys_StringToSockaddr(const char *s, sockaddr *sadr, int sadr_len, sa_family_t family);

/*
=============
Sys_SockaddrToString
=============
*/
//static void Sys_SockaddrToString(char *dest, int destlen, sockaddr *input);

/*
=============
Sys_StringToAdr
=============
*/
bool Sys_StringToAdr( const char *s, netadr_t *a, netadrtype_t family );

/*
===================
NET_CompareBaseAdrMask

Compare without port, and up to the bit number given in netmask.
===================
*/
bool NET_CompareBaseAdrMask(netadr_t a, netadr_t b, int netmask);
/*
===================
NET_CompareBaseAdr

Compares without the port
===================
*/
bool NET_CompareBaseAdr (netadr_t a, netadr_t b);

const char	*NET_AdrToString (netadr_t a);

const char	*NET_AdrToStringwPort (netadr_t a);

bool	NET_CompareAdr (netadr_t a, netadr_t b);

bool	NET_IsLocalAddress( netadr_t adr );

/*
==================
NET_GetPacket

Receive one packet
==================
*/
#ifdef _DEBUG
int	recvfromCount;
#endif

bool NET_GetPacket(netadr_t *net_from, msg_t *net_message, fd_set *fdr);

/*
==================
Sys_SendPacket
==================
*/
void Sys_SendPacket( int length, const void *data, netadr_t to );

/*
==================
Sys_IsLANAddress

LAN clients will have their rate var ignored
==================
*/
bool Sys_IsLANAddress( netadr_t adr );

/*
==================
Sys_ShowIP
==================
*/
void Sys_ShowIP(void);


//=============================================================================


/*
====================
NET_IPSocket
====================
*/
int NET_IPSocket(const char* net_interface, int port, int* err);

/*
====================
NET_IP6Socket
====================
*/
int NET_IP6Socket(const char* net_interface, int port, sockaddr_in6* bindto, int* err);

/*
====================
NET_SetMulticast
Set the current multicast group
====================
*/
void NET_SetMulticast6(void);

/*
====================
NET_JoinMulticast
Join an ipv6 multicast group
====================
*/
void NET_JoinMulticast6(void);

void NET_LeaveMulticast6();

/*
====================
NET_OpenSocks
====================
*/
void NET_OpenSocks( int port );

/*
=====================
NET_AddLocalAddress
=====================
*/
static void NET_AddLocalAddress(char *ifname, sockaddr *addr, sockaddr *netmask);

static void NET_GetLocalAddress(void);

/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP( void );

/*
====================
NET_Config
====================
*/
void NET_Config( bool enableNetworking );

/*
====================
NET_Init
====================
*/
void NET_Init( void );

/*
====================
NET_Shutdown
====================
*/
void NET_Shutdown( void );

/*
====================
NET_Event

Called from NET_Sleep which uses select() to determine which sockets have seen action.
====================
*/

void NET_Event(fd_set *fdr);

/*
====================
NET_Sleep

Sleeps msec or until something happens on the network
====================
*/
void NET_Sleep(int msec);

/*
====================
NET_Restart_f
====================
*/
void NET_Restart_f( void );

} // oacom

#endif /* _OACOM_NET_IP_H_ */
