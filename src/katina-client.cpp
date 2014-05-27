/*
 * katina-client.cpp
 *
 *  Created on: 23 May 2014
 *      Author: oasookee@googlemail.com
 */


/*-----------------------------------------------------------------.
| Copyright (C) 2014 SooKee oasookee@googlemail.com               |
'------------------------------------------------------------------'

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.

http://www.gnu.org/licenses/gpl-2.0.html

'-----------------------------------------------------------------*/

// STACK TRACE
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <execinfo.h>
#include <cxxabi.h>

#include <katina/Katina.h>
#include <katina/log.h>
#include <katina/types.h>

using namespace oastats;
using namespace oastats::log;
using namespace oastats::types;

#ifndef REVISION
#define REVISION "not set"
#endif

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	1024	// max tokens resulting from Cmd_TokenizeString
//#define NET_ADDRSTRMAXLEN 48	// maximum length of an IPv6 address string including trailing '\0'
#define MAX_TIMEDEMO_DURATIONS	4096
#define	MAX_RELIABLE_COMMANDS	64			// max string commands buffered for restransmit

#define	MAX_QPATH			64		// max length of a quake game pathname
#ifdef PATH_MAX
#define MAX_OSPATH			PATH_MAX
#else
#define	MAX_OSPATH			256		// max length of a filesystem pathname
#endif

#define	MAX_CVAR_VALUE_STRING	256
#define	MAX_INFO_STRING		1024

typedef unsigned char byte;

typedef enum {qfalse, qtrue} qboolean;
typedef int fileHandle_t;

typedef enum
{
	NA_BAD = 0,					// an address lookup failed
	NA_BOT,
	NA_LOOPBACK,
	NA_BROADCAST,
	NA_IP,
	NA_IP6,
	NA_MULTICAST6,
	NA_UNSPEC
} netadrtype_t;

typedef enum
{
	NS_CLIENT,
	NS_SERVER
} netsrc_t;

struct netadr_t
{
	netadrtype_t	type;

	byte	ip[4];
	byte	ip6[16];

	unsigned short	port;
	unsigned long	scope_id;	// Needed for IPv6 link-local addresses
};

/*
Netchan handles packet fragmentation and out of order / duplicate suppression
*/

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
	qboolean	unsentFragments;
	int			unsentFragmentStart;
	int			unsentLength;
	byte		unsentBuffer[MAX_MSGLEN];
} netchan_t;

void Netchan_Init( int qport );
void Netchan_Setup( netsrc_t sock, netchan_t *chan, netadr_t adr, int qport );

void Netchan_Transmit( netchan_t *chan, int length, const byte *data );
void Netchan_TransmitNextFragment( netchan_t *chan );

qboolean Netchan_Process( netchan_t *chan, msg_t *msg );


/*
==============================================================

PROTOCOL

==============================================================
*/

#define	PROTOCOL_VERSION	71

#define	UPDATE_SERVER_NAME	""
// override on command line, config files etc.
#define MASTER_SERVER_NAME	"dpmaster.deathmask.net"

#define	PORT_MASTER			27950
#define	PORT_UPDATE			27951
#define	PORT_SERVER			27960
#define	NUM_SERVER_PORTS	4		// broadcast scan this many ports after
									// PORT_SERVER so a single machine can
									// run multiple servers


// the svc_strings[] array in cl_parse.c should mirror this
//
// server to client
//
enum svc_ops_e {
	svc_bad,
	svc_nop,
	svc_gamestate,
	svc_configstring,			// [short] [string] only in gamestate messages
	svc_baseline,				// only in gamestate messages
	svc_serverCommand,			// [string] to be executed by client game module
	svc_download,				// [short] size [size bytes]
	svc_snapshot,
	svc_EOF,

	// svc_extension follows a svc_EOF, followed by another svc_* ...
	//  this keeps legacy clients compatible.
	svc_extension,
	svc_voip,     // not wrapped in USE_VOIP, so this value is reserved.
};


//
// client to server
//
enum clc_ops_e {
	clc_bad,
	clc_nop,
	clc_move,				// [[usercmd_t]
	clc_moveNoDelta,		// [[usercmd_t]
	clc_clientCommand,		// [string] message
	clc_EOF,

	// clc_extension follows a clc_EOF, followed by another clc_* ...
	//  this keeps legacy servers compatible.
	clc_extension,
	clc_voip,   // not wrapped in USE_VOIP, so this value is reserved.
};

struct clientConnection_t
{
	int clientNum;
	int lastPacketSentTime;			// for retransmits during connection
	int lastPacketTime;				// for timeouts

	netadr_t serverAddress;
	int connectTime;				// for connection retransmits
	int connectPacketCount;			// for display on connection dialog
	char serverMessage[MAX_STRING_TOKENS];	// for display on connection dialog

	int challenge;					// from the server to use for connecting
	int checksumFeed;				// from the server for checksum calculations

	// these are our reliable messages that go to the server
	int reliableSequence;
	int reliableAcknowledge;		// the last one the server has executed
	char reliableCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int serverMessageSequence;

	// reliable messages received from server
	int serverCommandSequence;
	int lastExecutedServerCommand;		// last server command grabbed or executed with CL_GetServerCommand
	char serverCommands[MAX_RELIABLE_COMMANDS][MAX_STRING_CHARS];

	// file transfer from server
	fileHandle_t download;
	char downloadTempName[MAX_OSPATH];
	char downloadName[MAX_OSPATH];
//#ifdef USE_CURL
//	qboolean	cURLEnabled;
//	qboolean	cURLUsed;
//	qboolean	cURLDisconnected;
//	char		downloadURL[MAX_OSPATH];
//	CURL		*downloadCURL;
//	CURLM		*downloadCURLM;
//#endif /* USE_CURL */
	int sv_allowDownload;
	char sv_dlURL[MAX_CVAR_VALUE_STRING];
	int downloadNumber;
	int downloadBlock;	// block we are waiting for
	int downloadCount;	// how many bytes we got
	int downloadSize;	// how many bytes we got
	char downloadList[MAX_INFO_STRING]; // list of paks we need to download
	qboolean downloadRestart;	// if true, we need to do another FS_Restart because we downloaded a pak

	// demo information
	char		demoName[MAX_QPATH];
	qboolean	spDemoRecording;
	qboolean	demorecording;
	qboolean	demoplaying;
	qboolean	demowaiting;	// don't record until a non-delta message is received
	qboolean	firstDemoFrameSkipped;
	fileHandle_t	demofile;

	int			timeDemoFrames;		// counter of rendered frames
	int			timeDemoStart;		// cls.realtime before first frame
	int			timeDemoBaseTime;	// each frame will be at this time + frameNum * 50
	int			timeDemoLastFrame;// time the last frame was rendered
	int			timeDemoMinDuration;	// minimum frame duration
	int			timeDemoMaxDuration;	// maximum frame duration
	unsigned char	timeDemoDurations[ MAX_TIMEDEMO_DURATIONS ];	// log of frame durations

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t	netchan;
};

struct clientActive_t
{
	int timeoutcount;		// it requres several frames in a timeout condition
									// to disconnect, preventing debugging breaks from
									// causing immediate disconnects on continue
	clSnapshot_t snap;			// latest received from server

	int serverTime;			// may be paused during play
	int oldServerTime;		// to prevent time from flowing bakcwards
	int oldFrameServerTime;	// to check tournament restarts
	int serverTimeDelta;	// cl.serverTime = cls.realtime + cl.serverTimeDelta
									// this value changes as net lag varies
	qboolean extrapolatedSnapshot;	// set if any cgame frame has been forced to extrapolate
									// cleared when CL_AdjustTimeDelta looks at it
	qboolean newSnapshots;		// set on parse of any valid packet

	gameState_t gameState;			// configstrings
	cha mapname[MAX_QPATH];	// extracted from CS_SERVERINFO

	int parseEntitiesNum;	// index (not anded off) into cl_parse_entities[]

	int mouseDx[2], mouseDy[2];	// added to by mouse events
	int mouseIndex;
	int joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	// cgame communicates a few values to the client system
	int cgameUserCmdValue;	// current weapon to add to usercmd_t
	float cgameSensitivity;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t cmds[CMD_BACKUP];	// each mesage will send several old cmds
	int cmdNumber;			// incremented each frame, because multiple
									// frames may need to be packed into a single packet

	outPacket_t outPackets[PACKET_BACKUP];	// information about each packet we have sent out

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t viewangles;

	int serverId;			// included in each client message so the server
												// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t snapshots[PACKET_BACKUP];

	entityState_t entityBaselines[MAX_GENTITIES];	// for delta compression when not in previous frame

	entityState_t parseEntities[MAX_PARSE_ENTITIES];
};

class Klient
{
private:

	str server;

	clientActive_t		cl;
	clientConnection_t	clc;
	clientStatic_t		cls;

public:
	/*
	================
	CL_Connect_f

	================
	*/
	void CL_Connect_f(const str& server, const str serverString)
	{
		netadrtype_t family = NA_IP;

		// save arguments for reconnect
		//Q_strncpyz( cl_reconnectArgs, Cmd_Args(), sizeof( cl_reconnectArgs ) );

		this->server = server;

		// fire a message off to the motd server
		CL_RequestMotd();

		// clear any previous "server full" type messages
		clc.serverMessage[0] = 0;

		if ( com_sv_running->integer && !strcmp( server, "localhost" ) ) {
			// if running a local server, kill it
			SV_Shutdown( "Server quit" );
		}

		// make sure a local server is killed
		Cvar_Set( "sv_killserver", "1" );
		SV_Frame( 0 );

		noGameRestart = qtrue;
		CL_Disconnect( qtrue );
		Con_Close();

		Q_strncpyz( clc.servername, server, sizeof(clc.servername) );

		if (!NET_StringToAdr(clc.servername, &clc.serverAddress, family) ) {
			Com_Printf ("Bad server address\n");
			clc.state = CA_DISCONNECTED;
			return;
		}
		if (clc.serverAddress.port == 0) {
			clc.serverAddress.port = BigShort( PORT_SERVER );
		}

		serverString = NET_AdrToStringwPort(clc.serverAddress);

		Com_Printf( "%s resolved to %s\n", clc.servername, serverString);

		if( cl_guidServerUniq->integer )
			CL_UpdateGUID( serverString, strlen( serverString ) );
		else
			CL_UpdateGUID( NULL, 0 );

		// if we aren't playing on a lan, we need to authenticate
		// with the cd key
		if(NET_IsLocalAddress(clc.serverAddress))
			clc.state = CA_CHALLENGING;
		else
		{
			clc.state = CA_CONNECTING;

			// Set a client challenge number that ideally is mirrored back by the server.
			clc.challenge = ((rand() << 16) ^ rand()) ^ Com_Milliseconds();
		}

		Key_SetCatcher( 0 );
		clc.connectTime = -99999;	// CL_CheckForResend() will fire immediately
		clc.connectPacketCount = 0;

		// server connection string
		Cvar_Set( "cl_currentServerAddress", server );
	}

};

/**
 * katina /path/to/config [$HOME/.katina]
 */
int main(const int argc, const char* argv[])
{
	srand(time(0));
	log("KLIENT REVISION: " << REVISION);
	Klient klient;
	klient.start(str(argc == 2 ? argv[1] : "$HOME/.katina"));
}
