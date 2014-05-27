#pragma once
#ifndef NET_CHAN_H_
#define NET_CHAN_H_
/*
 * net_chan.h
 *
 *  Created on: 25 May 2014
 *      Author: oasookee@gmail.com
 */

#include <katina/types.h>

#include "q_shared.h"
#include "msg.h"

namespace oacom {

using namespace oastats::types;

#define	MAX_MSGLEN 16384		// max length of a message, which may

typedef enum {
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

typedef enum {
	NA_BAD = 0,					// an address lookup failed
	NA_BOT,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IP6,
	NA_MULTICAST6,
	NA_UNSPEC
} netadrtype_t;

typedef struct {
	netadrtype_t	type;

	byte	ip[4];
	byte	ip6[16];

	unsigned short	port;
	unsigned long	scope_id;	// Needed for IPv6 link-local addresses
} netadr_t;

typedef struct {
	netsrc_t	sock;

	int			dropped;			// between last packet and previous

	netadr_t	remoteAddress;
	int			qport;				// qport value to write when transmitting

	// sequencing variables
	int			incomingSequence;
	int			outgoingSequence;

	// incoming fragment assembly buffer
	int			fragmentSequence;
	int			fragmentLength;
	byte		fragmentBuffer[MAX_MSGLEN];

	// outgoing fragment buffer
	// we need to space out the sending of large fragmented messages
	bool		unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	byte		unsentBuffer[MAX_MSGLEN];
} netchan_t;

void Netchan_Init(int port);

/*
==============
Netchan_Setup

called to open a channel to a remote system
==============
*/
void Netchan_Setup(netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

/*
=================
Netchan_TransmitNextFragment

Send one fragment of the current message
=================
*/
void Netchan_TransmitNextFragment( netchan_t *chan );


/*
===============
Netchan_Transmit

Sends a message to a connection, fragmenting if necessary
A 0 length will still generate a packet.
================
*/
void Netchan_Transmit( netchan_t *chan, int length, const byte *data );

/*
=================
Netchan_Process

Returns qfalse if the message should not be processed due to being
out of order or a fragment.

Msg must be large enough to hold MAX_MSGLEN, because if this is the
final fragment of a multi-part message, the entire thing will be
copied out.
=================
*/
bool Netchan_Process( netchan_t *chan, msg_t *msg );


bool NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, msg_t *net_message);


void NET_SendLoopPacket (netsrc_t sock, int length, const void *data, netadr_t to);

static void NET_QueuePacket( int length, const void *data, netadr_t to,
	int offset );

void NET_FlushPacketQueue(void);

void NET_SendPacket( netsrc_t sock, int length, const void *data, netadr_t to );

/*
===============
NET_OutOfBandPrint

Sends a text message in an out-of-band datagram
================
*/
void NET_OutOfBandPrint( netsrc_t sock, netadr_t adr, const char *format, ... );

/*
===============
NET_OutOfBandPrint

Sends a data message in an out-of-band datagram (only used for "connect")
================
*/
void NET_OutOfBandData( netsrc_t sock, netadr_t adr, byte *format, int len );

/*
=============
NET_StringToAdr

Traps "localhost" for loopback, passes everything else to system
return 0 on address not found, 1 on address found with port, 2 on address found without port.
=============
*/
int NET_StringToAdr( const char *s, netadr_t *a, netadrtype_t family );

} // oacom

#endif /* NET_CHAN_H_ */
