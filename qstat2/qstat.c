/*
 * qstat 2.3g
 * by Steve Jankowski
 * steve@activesw.com
 * http://www.activesw.com/people/steve/qstat.html
 *
 * Thanks to Per Hammer for the OS/2 patches (per@mindbend.demon.co.uk)
 * Thanks to John Ross Hunt for the OpenVMS Alpha patches (bigboote@ais.net)
 * Thanks to Scott MacFiggen for the quicksort code (smf@activesw.com)
 *
 * Inspired by QuakePing by Len Norton
 *
 * Copyright 1996,1997,1998,1999 by Steve Jankowski
 *
 *   Permission granted to use this software for any purpose you desire
 *   provided that existing copyright notices are retained verbatim in all
 *   copies and derived works.
 */

#define VERSION "2.3g"
char *qstat_version= VERSION;

/* OS/2 defines */
#ifdef __OS2__
#define BSD_SELECT
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>

#ifdef unix
#define _ISUNIX
#endif
#ifdef __unix
#define _ISUNIX
#endif
#ifdef VMS
#define _ISUNIX
#endif
#ifdef _AIX
#define _ISUNIX
#endif


#ifdef _ISUNIX
#include <unistd.h>
#include <sys/socket.h>
#ifndef VMS
#include <sys/param.h>
#endif
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>

#ifndef F_SETFL
#include <fcntl.h>
#endif

#ifdef __hpux
extern int h_errno;
#endif

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#ifndef INADDR_NONE
#define INADDR_NONE ~0
#endif
#define sockerr()	errno
#endif /* _ISUNIX */

#ifdef _WIN32
#define FD_SETSIZE 256
#include <winsock.h>
#include <sys/timeb.h>
#define close(a) closesocket(a)
int gettimeofday(struct timeval *now, void *blah)
{
    struct timeb timeb;
    ftime( &timeb);
    now->tv_sec= timeb.time;
    now->tv_usec= (unsigned int)timeb.millitm * 1000;
}
#define sockerr()	WSAGetLastError()
#define strcasecmp      stricmp
#define strncasecmp     strnicmp
#ifndef EADDRINUSE
#define EADDRINUSE	WSAEADDRINUSE
#endif
#endif /* _WIN32 */

#ifdef __OS2__
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <utils.h>
 
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define close(a)        soclose(a)
#endif /* __OS2__ */

#define QUERY_PACKETS
#include "qstat.h"


#ifndef FD_SETSIZE
#define FD_SETSIZE 64
#endif

/* Values set by command-line arguments
 */

int hostname_lookup= 0;		/* set if -H was specified */
int new_style= 1;		/* unset if -old was specified */
int n_retries= DEFAULT_RETRIES;
int retry_interval= DEFAULT_RETRY_INTERVAL;
int master_retry_interval= DEFAULT_RETRY_INTERVAL*4;
int get_player_info= 0;
int get_server_rules= 0;
int up_servers_only= 0;
int no_full_servers= 0;
int no_empty_servers= 0;
int no_header_display= 0;
int raw_display= 0;
char *raw_delimiter= "\t";
int player_address= 0;
int hex_player_names= 0;
int max_simultaneous= MAXFD_DEFAULT;
int raw_arg= 0;
int progress= 0;
int num_servers_total= 0;
int num_players_total= 0;
int num_servers_returned= 0;
int num_servers_timed_out= 0;
int num_servers_down= 0;
server_type* default_server_type= NULL;
int sort= 0;
int first_sort_key= SORT_PING;
int second_sort_key= SORT_GAME;
int show_errors= 0;

#define DEFAULT_COLOR_NAMES_RAW		0
#define DEFAULT_COLOR_NAMES_DISPLAY	1
int color_names= -1;

#define SECONDS 0
#define CLOCK_TIME 1
#define STOPWATCH_TIME 2
#define DEFAULT_TIME_FMT_RAW		SECONDS
#define DEFAULT_TIME_FMT_DISPLAY	CLOCK_TIME
int time_format= -1;

struct qserver *servers= NULL;
struct qserver **connmap= NULL;
int connected= 0;
time_t run_timeout= 0;
time_t start_time;
int waiting_for_masters;

char *DOWN= "DOWN";
char *SYSERROR= "SYSERROR";
char *TIMEOUT= "TIMEOUT";
char *MASTER= "MASTER";
char *SERVERERROR= "ERROR";
char *HOSTNOTFOUND= "HOSTNOTFOUND";

int display_prefix= 0;
char *current_filename;
int current_fileline;


/* MODIFY HERE
 * Change these functions to display however you want
 */
void
display_server(
    struct qserver *server
)
{
    char name[100], prefix[8];

    if ( raw_display)  {
	raw_display_server( server);
	return;
    }
    if ( have_server_template())  {
	template_display_server( server);
	return;
    }

    sprintf( prefix, "%-4s", server->type->type_prefix);

    if ( server->server_name == DOWN)  {
	if ( ! up_servers_only)
	    printf( "%s%-16s %10s\n", (display_prefix)?prefix:"",
		(hostname_lookup) ? server->host_name : server->arg, DOWN);
	return;
    }
    if ( server->server_name == TIMEOUT)  {
	if ( server->flags & FLAG_BROADCAST && server->n_servers)
	    printf( "%s%-16s %d servers\n", (display_prefix)?prefix:"",
		server->arg, server->n_servers);
	else if ( ! up_servers_only)
	    printf( "%s%-16s no response\n", (display_prefix)?prefix:"",
		(hostname_lookup) ? server->host_name : server->arg);
	return;
    }

    if ( server->type->master)  {
	display_qwmaster(server);
	return;
    }

    if ( no_full_servers && server->num_players >= server->max_players)
	return;

    if ( no_empty_servers && server->num_players == 0)
	return;

    if ( server->error != NULL)  {
	printf( "%s%-22s ERROR <%s>\n",
	    (display_prefix)?prefix:"",
	    (hostname_lookup) ? server->host_name : server->arg,
	    server->error);
	return;
    }

    if ( new_style)  {
	char *game= get_qw_game( server);
	printf( "%s%-22s %2d/%2d %*s %6d / %1d  %*s %s\n",
	    (display_prefix)?prefix:"",
	    (hostname_lookup) ? server->host_name : server->arg,
	    server->num_players, server->max_players,
	    (server->type->id == HL_SERVER) ? 12 : 8, (server->map_name) ? server->map_name : "?",
	    server->ping_total/server->n_requests,
	    server->n_retries,
	    (server->type->id == QW_SERVER || server->type->id == Q2_SERVER || server->type->id == Q3_SERVER) ? 9 : 0, game,
	    xform_name(server->server_name, server));
	if ( get_server_rules)
	    server->type->display_rule_func( server);
	if ( get_player_info)
	    server->type->display_player_func( server);
    }
    else  {
	sprintf( name, "\"%s\"", server->server_name);
	printf( "%-16s %10s map %s at %22s %d/%d players %d ms\n", 
	    (hostname_lookup) ? server->host_name : server->arg,
	    name, server->map_name,
	    server->address, server->num_players, server->max_players,
	    server->ping_total/server->n_requests);
    }
}

void
display_qwmaster( struct qserver *server)
{
    char *prefix;
    prefix= server->type->type_prefix;

    if ( server->error != NULL)
	printf( "%s %-17s ERROR <%s>\n", prefix,
		(hostname_lookup) ? server->host_name : server->arg,
		server->error);
    else
	printf( "%s %-17s %d servers %6d / %1d\n", prefix,
	    (hostname_lookup) ? server->host_name : server->arg,
	    server->n_servers,
	    server->ping_total/server->n_requests,
	    server->n_retries);
}

void
display_header()
{
    if ( ! no_header_display)
	printf( "%-16s %8s %8s %15s    %s\n", "ADDRESS", "PLAYERS", "MAP",
		"RESPONSE TIME", "NAME");
}

void
display_server_rules( struct qserver *server)
{
    struct rule *rule;
    int printed= 0;
    rule= server->rules;
    for ( ; rule != NULL; rule= rule->next)  {
	if ( (server->type->id != Q_SERVER &&
		server->type->id != H2_SERVER) || ! is_default_rule( rule)) {
	    printf( "%c%s=%s", (printed)?',':'\t', rule->name, rule->value);
	    printed++;
	}
    }
    if ( printed)
	puts("");
}

void
display_q_player_info( struct qserver *server)
{
    char fmt[128];
    struct player *player;

    strcpy( fmt, "\t#%-2d %3d frags %9s ");

    if ( color_names)
	strcat( fmt, "%9s:%-9s ");
    else
	strcat( fmt, "%2d:%-2d ");
    if ( player_address)
	strcat( fmt, "%22s ");
    else
	strcat( fmt, "%s");
    strcat( fmt, "%s\n");

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		player->number,
		player->frags,
		play_time(player->connect_time,1),
		quake_color(player->shirt_color),
		quake_color(player->pants_color),
		(player_address)?player->address:"",
		xform_name( player->name, server));
    }
}

void
display_qw_player_info( struct qserver *server)
{
    char fmt[128];
    struct player *player;

    strcpy( fmt, "\t#%-6d %3d frags %6s@%-5s %8s");

    if ( color_names)
	strcat( fmt, "%9s:%-9s ");
    else
	strcat( fmt, "%2d:%-2d ");
    strcat( fmt, "%s\n");

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		player->number,
		player->frags,
		play_time(player->connect_time,0),
		ping_time(player->ping),
		player->skin ? player->skin : "",
		quake_color(player->shirt_color),
		quake_color(player->pants_color),
		xform_name( player->name, server));
    }
}

void
display_q2_player_info( struct qserver *server)
{
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( "\t%3d frags %8s  %s\n", 
		player->frags,
		ping_time(player->ping),
		xform_name( player->name, server));
    }
}

void
display_unreal_player_info( struct qserver *server)
{
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( "\t%3d frags team#%-3d %7s %s\n", 
		player->frags,
		player->team,
		ping_time(player->ping),
		xform_name( player->name, server));
    }
}

void
display_shogo_player_info( struct qserver *server)
{
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( "\t%3d frags %8s %s\n", 
		player->frags,
		ping_time(player->ping),
		xform_name( player->name, server));
    }
}

void
display_halflife_player_info( struct qserver *server)
{
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( "\t%3d frags %8s %s\n", 
		player->frags,
		play_time( player->connect_time,1),
		xform_name( player->name, server));
    }
}

void
display_tribes_player_info( struct qserver *server)
{
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( "\t%4d score team#%d %8s %s\n", 
		player->frags,
		player->team,
		ping_time( player->ping),
		xform_name( player->name, server));
    }
}

void
display_bfris_player_info( struct qserver *server)
{
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
      printf("\tid: %d, ship: %d, team: %s, ping: %d, score: %d, kills: %d, name: %s\n",
	     player->number,
	     player->ship,
	     player->team_name,
	     player->ping,
	     player->score,
	     player->frags,
	     xform_name( player->name, server));
    }
}

char *
get_qw_game( struct qserver *server)
{
    struct rule *rule;
    if ( server->type->game_rule == NULL || *server->type->game_rule == '\0')
	return "";
    rule= server->rules;
    for ( ; rule != NULL; rule= rule->next)
	if ( strcmp( rule->name, server->type->game_rule) == 0)  {
	    if ( server->type->id == Q3_SERVER &&
			strcmp( rule->value, "baseq3") == 0)
		return "";
	    return rule->value;
	}
    rule= server->rules;
    for ( ; rule != NULL; rule= rule->next)
	if ( strcmp( rule->name, "game") == 0)
	    return rule->value;
    return "";
}

/* Raw output for web master types
 */

#define RD raw_delimiter

void
raw_display_server( struct qserver *server)
{
    char *prefix;
    int server_type= server->type->id;
    prefix= server->type->type_prefix;

    if ( server->server_name == DOWN)  {
	if ( ! up_servers_only)
	    printf( "%s" "%.*s%.*s" "%s%s" "%s%s\n\n",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup)?server->host_name:server->arg,
		RD, DOWN);
	return;
    }
    if ( server->server_name == TIMEOUT)  {
	if ( server->flags & FLAG_BROADCAST && server->n_servers)
	    printf( "%s" "%.*s%.*s" "%s%s" "%s%d\n", prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, server->arg,
		RD, server->n_servers);
	else if ( ! up_servers_only)
	    printf( "%s" "%.*s%.*s" "%s%s" "%s%s\n\n",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup)?server->host_name:server->arg,
		RD, TIMEOUT);
	return;
    }

    if ( server->error != NULL)  {
        printf( "%s" "%.*s%.*s" "%s%s" "%s%s" "%s%s",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup) ? server->host_name : server->arg,
		RD, "ERROR",
		RD, server->error);
    }
    else if ( server_type == Q_SERVER || server_type == H2_SERVER)  {
        printf( "%s" "%.*s%.*s" "%s%s" "%s%s" "%s%s" "%s%d" "%s%s" "%s%d" "%s%d" "%s%d" "%s%d",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup) ? server->host_name : server->arg,
		RD, xform_name( server->server_name, server),
		RD, server->address,
		RD, server->protocol_version,
		RD, server->map_name,
		RD, server->max_players,
		RD, server->num_players,
		RD, server->ping_total/server->n_requests,
		RD, server->n_retries
        );
    }
    else if ( server_type == QW_SERVER || server_type == Q2_SERVER ||
		server_type == HW_SERVER || server_type == HL_SERVER ||
		server_type == SIN_SERVER || server_type == Q3_SERVER ||
		server_type == BFRIS_SERVER || server_type == UN_SERVER ||
		server_type == SHOGO_SERVER || server_type == KINGPIN_SERVER ||
		server_type == HERETIC2_SERVER)  {
        printf( "%s" "%.*s%.*s" "%s%s" "%s%s" "%s%s" "%s%d" "%s%d" "%s%d" "%s%d",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup) ? server->host_name : server->arg,
		RD, xform_name( server->server_name, server),
		RD, (server->map_name) ? server->map_name : "?",
		RD, server->max_players,
		RD, server->num_players,
		RD, server->ping_total/server->n_requests,
		RD, server->n_retries
        );
    }
    else if ( server_type == TRIBES_SERVER)  {
	printf( "%s" "%.*s%.*s" "%s%s" "%s%s" "%s%s" "%s%d" "%s%d",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup) ? server->host_name : server->arg,
		RD, xform_name( server->server_name, server),
		RD, (server->map_name) ? server->map_name : "?",
		RD, server->num_players,
		RD, server->max_players
	);
    }
    else if ( server->type->master)  {
        printf( "%s" "%.*s%.*s" "%s%s" "%s%d",
		prefix,
		raw_arg, RD, raw_arg, server->arg,
		RD, (hostname_lookup) ? server->host_name : server->arg,
		RD, server->n_servers
	);
    }
    else  {
	printf( "Unknown server type %d (prefix %s)\n", server_type,
		prefix);
    }
    puts("");

    if ( server->type->master || server->error != NULL)  {
	puts("");
	return;
    }

    if ( get_server_rules)
	server->type->display_raw_rule_func( server);
    if ( get_player_info)
	server->type->display_raw_player_func( server);
    puts("");
}

void
raw_display_server_rules( struct qserver *server)
{
    struct rule *rule;
    int printed= 0;
    rule= server->rules;
    for ( ; rule != NULL; rule= rule->next)  {
	printf( "%s%s=%s", (printed)?RD:"", rule->name, rule->value);
	printed++;
    }
    if ( server->missing_rules)
	printf( "%s?", (printed)?RD:"");
    puts("");
}

void
raw_display_q_player_info( struct qserver *server)
{
    char fmt[128];
    struct player *player;

    strcpy( fmt, "%d" "%s%s" "%s%s" "%s%d" "%s%s");
    if ( color_names)
	strcat( fmt, "%s%s" "%s%s");
    else
	strcat( fmt, "%s%d" "%s%d");

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		player->number,
		RD, xform_name( player->name, server),
		RD, player->address,
		RD, player->frags,
		RD, play_time(player->connect_time,1),
		RD, quake_color(player->shirt_color),
		RD, quake_color(player->pants_color)
	);
	puts("");
    }
}

void
raw_display_qw_player_info( struct qserver *server)
{
    char fmt[128];
    struct player *player;

    strcpy( fmt, "%d" "%s%s" "%s%d" "%s%s");
    if ( color_names)
	strcat( fmt, "%s%s" "%s%s");
    else
	strcat( fmt, "%s%d" "%s%d");
    strcat( fmt, "%s%d" "%s%s");

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		player->number,
		RD, xform_name( player->name, server),
		RD, player->frags,
		RD, play_time(player->connect_time,1),
		RD, quake_color(player->shirt_color),
		RD, quake_color(player->pants_color),
		RD, player->ping,
		RD, player->skin ? player->skin : ""
	);
	puts("");
    }
}

void
raw_display_q2_player_info( struct qserver *server)
{
    static char fmt[24] = "%s" "%s%d" "%s%d";
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		xform_name( player->name, server),
		RD, player->frags,
		RD, player->ping
	);
	puts("");
    }
}

void
raw_display_unreal_player_info( struct qserver *server)
{
    static char fmt[28]= "%s" "%s%d" "%s%d" "%s%d" "%s%s" "%s%s" "%s%s";
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		xform_name( player->name, server),
		RD, player->frags,
		RD, player->ping,
		RD, player->team,
		RD, player->skin ? player->skin : "",
		RD, player->mesh ? player->mesh : "",
		RD, player->face ? player->face : ""
	);
	puts("");
    }
}

void
raw_display_halflife_player_info( struct qserver *server)
{
    static char fmt[24]= "%s" "%s%d" "%s%s";
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		xform_name( player->name, server),
		RD, player->frags,
		RD, play_time( player->connect_time,1)
	);
	puts("");
    }
}

void
raw_display_tribes_player_info( struct qserver *server)
{
    static char fmt[24]= "%s" "%s%d" "%s%d" "%s%d" "%s%d";
    struct player *player;

    player= server->players;
    for ( ; player != NULL; player= player->next)  {
	printf( fmt,
		xform_name( player->name, server),
		RD, player->frags,
		RD, player->ping,
		RD, player->team,
		RD, player->packet_loss
	);
	puts("");
    }
}

void
raw_display_bfris_player_info( struct qserver *server)
{
  static char fmt[] = "%d" "%s%d" "%s%s" "%s%d" "%s%d" "%s%d" "%s%s";
  struct player *player;

  player= server->players;
  for ( ; player != NULL; player= player->next)  {
    printf( fmt,
	    player->number,
	    RD, player->ship,
	    RD, player->team_name,
	    RD, player->ping,
	    RD, player->score,
	    RD, player->frags,
	    RD, xform_name( player->name, server)
	    );
    puts("");
  }
}

void
display_progress()
{
    fprintf( stderr, "\r%d/%d (%d timed out, %d down)",
	num_servers_returned+num_servers_timed_out,
	num_servers_total,
	num_servers_timed_out,
	num_servers_down);
}

/* ----- END MODIFICATION ----- Don't need to change anything below here. */


void set_non_blocking( int fd);

/* Misc flags
 */

char * NO_SERVER_RULES= NULL;
int NO_PLAYER_INFO= 0xffff;
struct timeval packet_recv_time;
int server_types= 0;
static int one= 1;
static int little_endian;
static int big_endian;
unsigned int swap_long( void *);
char * strndup( char *string, int len);
#define FORCE 1

/* Print an error message and the program usage notes
 */

void
usage( char *msg, char **argv, char *a1)
{
    server_type *type;

    if ( msg)
	fprintf( stderr, msg, a1);

    printf( "Usage: %s [options ...]\n", argv[0]);
    printf( "\t[-default server-type] [-f file] [host[:port]] ...\n");
    printf( "Where host is an IP address or host name\n");
    type= &types[0];
    for ( ; type->id != Q_UNKNOWN_TYPE; type++)
	printf( "%s\t\tquery %s server\n", type->type_option,
		type->game_name);
    printf( "-default\tset default server type:");
    type= &types[0];
    for ( ; type->id != Q_UNKNOWN_TYPE; type++)
	printf( " %s", type->type_string);
    puts("");
    printf( "-f\t\tread hosts from file\n");
    printf( "-R\t\tfetch and display server rules\n");
    printf( "-P\t\tfetch and display player info\n");
    printf( "-sort\t\tsort servers: p=by-ping, g=by-game \n");
    printf( "-u\t\tonly display servers that are up\n");
    printf( "-nf\t\tdo not display full servers\n");
    printf( "-ne\t\tdo not display empty servers\n");
    printf( "-cn\t\tdisplay color names instead of numbers\n");
    printf( "-ncn\t\tdisplay color numbers instead of names\n");
    printf( "-hc\t\tdisplay colors in #rrggbb format\n");
    printf( "-tc\t\tdisplay time in clock format (DhDDmDDs)\n");
    printf( "-tsw\t\tdisplay time in stop-watch format (DD:DD:DD)\n");
    printf( "-ts\t\tdisplay time in seconds\n");
    printf( "-pa\t\tdisplay player address\n");
    printf( "-hpn\t\tdisplay player names in hex\n");
    printf( "-old\t\told style display\n");
    printf( "-progress\tdisplay progress meter (text only)\n");
    printf( "-retry\t\tnumber of retries, default is %d\n", DEFAULT_RETRIES);
    printf( "-interval\tinterval between retries, default is %.2lf seconds\n",
	DEFAULT_RETRY_INTERVAL / 1000.0);
    printf( "-mi\t\tinterval between master server retries, default is %.2lf seconds\n",
	(DEFAULT_RETRY_INTERVAL*4) / 1000.0);
    printf( "-timeout\ttotal time in seconds before giving up\n");
    printf( "-maxsim\t\tset maximum simultaneous queries\n");
    printf( "-errors\t\tdisplay errors\n");
    printf( "-raw\t\toutput in raw format using delimiter\n");
    printf( "-Th,-Ts,-Tp,-Tt\toutput templates: header, server, player, and trailer\n");
    printf( "-H\t\tresolve host names\n");
    printf( "-Hcache\t\thost name cache file\n");
    printf( "qstat version %s\n", VERSION);
    exit(0);
}

struct server_arg  {
    server_type *type;
    char *arg;
    char *outfilename;
};

server_type*
find_server_type_id( int type_id)
{
    server_type *type= &types[0];
    for ( ; type->id != Q_UNKNOWN_TYPE; type++)
	if ( type->id == type_id)
	    return type;
    return NULL;
}

server_type*
find_server_type_string( char* type_string)
{
    server_type *type= &types[0];
    char *t= type_string;
    while ( *t) *t++= tolower( *t);

    for ( ; type->id != Q_UNKNOWN_TYPE; type++)
	if ( strcmp( type->type_string, type_string) == 0)
	    return type;
    return NULL;
}

server_type*
find_server_type_option( char* option, int *outfile)
{
    server_type *type= &types[0];

    char *comma= strchr( option, ',');
    if ( comma)  {
	*comma++= '\0';
	if ( strcmp( comma, "outfile") != 0)
	    return NULL;
	if ( outfile) *outfile= 1;
    }
    else if ( outfile)
	*outfile= 0;

    for ( ; type->id != Q_UNKNOWN_TYPE; type++)
	if ( strcmp( type->type_option, option) == 0)
	    return type;
    return NULL;
}

void
add_server_arg( char *arg, int type, char *outfilename,
	struct server_arg **args, int *n, int *max)
{
    if ( *n == *max)  {
	if ( *max == 0)  {
	    *max= 4;
	    *args= malloc(sizeof(struct server_arg) * (*max));
	}
	else  {
	    (*max)*= 2;
	    *args= (struct server_arg*) realloc( *args,
		sizeof(struct server_arg) * (*max));
	}
    }
    (*args)[*n].type= find_server_type_id( type);
    (*args)[*n].arg= arg;
    (*args)[*n].outfilename= outfilename;
    (*n)++;
}


main( int argc, char *argv[])
{
    int pktlen, rc, maxfd, fd;
    long pkt_data[PACKET_LEN/sizeof(long)];
    char *pkt= (char*)&pkt_data[0];
    fd_set read_fds;
    struct timeval timeout;
    int arg, timeout_milli, n_files, i;
    struct qserver *server;
    char **files, *outfilename;
    struct server_arg *server_args= NULL;
    int n_server_args= 0, max_server_args= 0;
 
#ifdef _WIN32
    WORD version= MAKEWORD(1,1);
    WSADATA wsa_data;
    if ( WSAStartup(version,&wsa_data) != 0)  {
	fprintf( stderr, "Could not open winsock\n");
	exit(1);
    }
#endif

    if ( argc == 1)
	usage(NULL,argv,NULL);

    files= (char **) malloc( sizeof(char*) * (argc/2));
    n_files= 0;

    default_server_type= find_server_type_id( Q_SERVER);
    little_endian= ((char*)&one)[0];
    big_endian= !little_endian;

    for ( arg= 1; arg < argc; arg++)  {
	if ( argv[arg][0] != '-')
	    break;
	outfilename= NULL;
	if ( strcmp( argv[arg], "-f") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -f\n", argv,NULL);
	    files[n_files++]= argv[arg];
	}
	else if ( strcmp( argv[arg], "-retry") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -retry\n", argv,NULL);
	    n_retries= atoi( argv[arg]);
	    if ( n_retries <= 0)  {
		fprintf( stderr, "retries must be greater than zero\n");
		exit(1);
	    }
	}
	else if ( strcmp( argv[arg], "-interval") == 0)  {
	    double value= 0.0;
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -interval\n", argv,NULL);
	    sscanf( argv[arg], "%lf", &value);
	    if ( value < 0.1)  {
		fprintf( stderr, "retry interval must be greater than 0.1\n");
		exit(1);
	    }
	    retry_interval= (int)(value * 1000);
	}
	else if ( strcmp( argv[arg], "-mi") == 0)  {
	    double value= 0.0;
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -mi\n", argv,NULL);
	    sscanf( argv[arg], "%lf", &value);
	    if ( value < 0.1)  {
		fprintf( stderr, "interval must be greater than 0.1\n");
		exit(1);
	    }
	    master_retry_interval= (int)(value * 1000);
	}
	else if ( strcmp( argv[arg], "-H") == 0)
	    hostname_lookup= 1;
	else if ( strcmp( argv[arg], "-u") == 0)
	    up_servers_only= 1;
	else if ( strcmp( argv[arg], "-nf") == 0)
	    no_full_servers= 1;
	else if ( strcmp( argv[arg], "-ne") == 0)
	    no_empty_servers= 1;
	else if ( strcmp( argv[arg], "-nh") == 0)
	    no_header_display= 1;
	else if ( strcmp( argv[arg], "-old") == 0)
	    new_style= 0;
	else if ( strcmp( argv[arg], "-P") == 0)
	    get_player_info= 1;
	else if ( strcmp( argv[arg], "-R") == 0)
	    get_server_rules= 1;
	else if ( strcmp( argv[arg], "-raw") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -raw\n", argv,NULL);
	    raw_delimiter= argv[arg];
	    raw_display= 1;
	}
	else if ( strcmp( argv[arg], "-ncn") == 0)  {
	    color_names= 0;
 	}
	else if ( strcmp( argv[arg], "-cn") == 0)  {
	    color_names= 1;
 	}
	else if ( strcmp( argv[arg], "-hc") == 0)  {
	    color_names= 2;
 	}
	else if ( strcmp( argv[arg], "-tc") == 0)  {
	    time_format= CLOCK_TIME;
	}
	else if ( strcmp( argv[arg], "-tsw") == 0)  {
	    time_format= STOPWATCH_TIME;
	}
	else if ( strcmp( argv[arg], "-ts") == 0)  {
	    time_format= SECONDS;
	}
	else if ( strcmp( argv[arg], "-pa") == 0)  {
	    player_address= 1;
	}
	else if ( strcmp( argv[arg], "-hpn") == 0)  {
	    hex_player_names= 1;
	}
	else if ( strncmp( argv[arg], "-maxsimultaneous", 7) == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -maxsimultaneous\n", argv,NULL);
	    max_simultaneous= atoi(argv[arg]);
	    if ( max_simultaneous <= 0)
		usage( "value for -maxsimultaneous must be > 0\n", argv,NULL);
	    if ( max_simultaneous > FD_SETSIZE)
		max_simultaneous= FD_SETSIZE;
 	}
	else if ( strcmp( argv[arg], "-raw-arg") == 0)  {
	    raw_arg= 1000;
 	}
	else if ( strcmp( argv[arg], "-timeout") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -timeout\n", argv,NULL);
	    run_timeout= atoi( argv[arg]);
	    if ( run_timeout <= 0)
		usage( "value for -timeout must be > 0\n", argv,NULL);
	}
	else if ( strcmp( argv[arg], "-progress") == 0)  {
	    progress= 1;
 	}
	else if ( strcmp( argv[arg], "-Hcache") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -Hcache\n", argv,NULL);
	    if ( hcache_open( argv[arg], 0) == -1)
		return 1;
	}
	else if ( strcmp( argv[arg], "-default") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -default\n", argv,NULL);
	    default_server_type= find_server_type_string( argv[arg]);
	    if ( default_server_type == NULL)  {
		char opt[256], *o= &opt[0];
		sprintf( opt, "-%s", argv[arg]);
		while ( *o) *o++= tolower(*o);
		default_server_type= find_server_type_option( opt, NULL);
	    }
	    if ( default_server_type == NULL)  {
		fprintf( stderr, "unknown server type \"%s\"\n", argv[arg]);
		usage( NULL, argv,NULL);
	    }
	}
	else if ( strncmp( argv[arg], "-Tserver", 3) == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for %s\n", argv, argv[arg]);
	    if ( read_qserver_template( argv[arg]) == -1)
		return 1;
	}
	else if ( strncmp( argv[arg], "-Theader", 3) == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for %s\n", argv, argv[arg]);
	    if ( read_header_template( argv[arg]) == -1)
		return 1;
	}
	else if ( strncmp( argv[arg], "-Ttrailer", 3) == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for %s\n", argv, argv[arg]);
	    if ( read_trailer_template( argv[arg]) == -1)
		return 1;
	}
	else if ( strncmp( argv[arg], "-Tplayer", 3) == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for %s\n", argv, argv[arg]);
	    if ( read_player_template( argv[arg]) == -1)
		return 1;
	}
	else if ( strcmp( argv[arg], "-sort") == 0)  {
	    arg++;
	    if ( arg >= argc)
		usage( "missing argument for -sort\n", argv, NULL);
	    if ( argv[arg][0] == 'p')
		first_sort_key= SORT_PING;
	    else if ( argv[arg][0] == 'g')
		first_sort_key= SORT_GAME;
	    else
		usage( "Unknown sort key \"%s\", use p or g\n", argv,
			argv[arg]);
	    sort++;
	}
	else if ( strcmp( argv[arg], "-errors") == 0)  {
	    show_errors++;
	}
	else  {
	    int outfile;
	    server_type *type;
	    arg++;
	    if ( arg >= argc)  {
		fprintf( stderr, "missing argument for \"%s\"\n", argv[arg-1]);
		return 1;
	    }
	    type= find_server_type_option( argv[arg-1], &outfile);
	    if ( type == NULL)  {
		fprintf( stderr, "unknown option \"%s\"\n", argv[arg-1]);
		return 1;
	    }
	    outfilename= NULL;
	    if ( outfile)  {
		outfilename= strchr( argv[arg], ',');
		if ( outfilename == NULL)  {
		    fprintf( stderr, "missing file name for \"%s,outfile\"\n",
			argv[arg-1]);
		    return 1;
		}
		*outfilename++= '\0';
	    }
	    add_server_arg( argv[arg], type->id, outfilename,
		&server_args, &n_server_args, &max_server_args);
	}
    }

    start_time= time(0);

    for ( i= 0; i < n_files; i++)
	add_file( files[i]);

    for ( ; arg < argc; arg++)
	add_qserver( argv[arg], default_server_type, NULL);

    for ( i= 0; i < n_server_args; i++)
	add_qserver( server_args[i].arg, server_args[i].type,
		server_args[i].outfilename);

    free( server_args);

    if ( servers == NULL)
	exit(1);

    connmap= (struct qserver**) calloc( 1, sizeof(struct qserver*) *
	( max_simultaneous + 10));

    if ( color_names == -1)
	color_names= ( raw_display) ? DEFAULT_COLOR_NAMES_RAW :
		DEFAULT_COLOR_NAMES_DISPLAY;

    if ( time_format == -1)
	time_format= ( raw_display) ? DEFAULT_TIME_FMT_RAW :
		DEFAULT_TIME_FMT_DISPLAY;

    if ( (server_types & MASTER_SERVER) || count_bits(server_types) > 1)
	display_prefix= 1;

    if ( new_style && ! raw_display && ! have_server_template())
	display_header();
    else if ( have_header_template())
	template_display_header();

    q_serverinfo.length= htons( q_serverinfo.length);
    h2_serverinfo.length= htons( h2_serverinfo.length);
    q_player.length= htons( q_player.length);

/*
    save_retry_interval= retry_interval;
    if ( server_types & MASTER_SERVER)
	retry_interval*= 2;
    if ( server_types & Q2_MASTER)
	retry_interval*= 2;
    if ( server_types & Q3_MASTER)
	retry_interval*= 2;
*/

    bind_sockets();

    while (connected)  {
	FD_ZERO( &read_fds);
	maxfd= set_fds( &read_fds);

	if ( progress)
	    display_progress();
	get_next_timeout( &timeout);
	rc= select( maxfd+1, &read_fds, NULL, NULL, &timeout);

	if (rc == 0)  {
	    if ( run_timeout && time(0)-start_time >= run_timeout)
		break;
	    send_packets();
	    bind_sockets();
	    continue;
	}
	if (rc == SOCKET_ERROR)  {
	    perror("select");
	    hcache_update_file();
	    return -1;
	}

	gettimeofday( &packet_recv_time, NULL);
#ifdef unix
	fd= 3;
	for ( ; rc; rc--)  {
	    struct sockaddr_in addr;
	    int addrlen= sizeof(addr);
	    while ( fd <= maxfd && ! FD_ISSET( fd, &read_fds))
		fd++;
	    if ( fd > maxfd)
		break;
	    server= connmap[fd++];
	    if ( server->flags & FLAG_BROADCAST)
		pktlen= recvfrom( server->fd, pkt, sizeof(pkt_data), 0,
			(struct sockaddr*)&addr, &addrlen);
	    else
	        pktlen= recv( server->fd, pkt, sizeof(pkt_data), 0);
	    if (pktlen == SOCKET_ERROR)  {
		if ( connection_refused())  {
		    server->server_name= DOWN;
		    num_servers_down++;
		    cleanup_qserver( server, 1);
		    if ( ! connected)
			bind_sockets();
		}
		continue;
	    }
	    if ( server->flags & FLAG_BROADCAST)  {
		struct qserver *broadcast= server;
		/* create new server and init */
		server= add_qserver_byaddr( ntohl(addr.sin_addr.s_addr),
			ntohs(addr.sin_port), server->type, NULL);
		server->packet_time1= broadcast->packet_time1;
		server->packet_time2= broadcast->packet_time2;
		server->ping_total= broadcast->ping_total;
		server->n_requests= broadcast->n_requests;
		server->n_packets= broadcast->n_packets;
		broadcast->n_servers++;
	    }

	    server->type->packet_func( server, pkt, pktlen);
	}
#endif /* unix */

#ifdef _WIN32
	for ( server= servers; server != NULL; server= server->next)  {
	    struct sockaddr_in addr;
	    struct qserver *broadcast;
	    int addrlen= sizeof(addr);

	    if ( server->fd == -1)
	        continue;
	    if ( ! FD_ISSET( server->fd, &read_fds))
		continue;
	    if ( server->flags & FLAG_BROADCAST)
		pktlen= recvfrom( server->fd, pkt, sizeof(pkt_data), 0,
			(struct sockaddr*)&addr, &addrlen);
	    else
	        pktlen= recv( server->fd, pkt, sizeof(pkt_data), 0);
	    if ( pktlen == SOCKET_ERROR)  {
		if ( connection_refused())  {
		    server->server_name= DOWN;
		    num_servers_down++;
		    cleanup_qserver( server, 1);
		    if ( ! connected)
			bind_sockets();
		}
		continue;
	    }

	    broadcast= NULL;
	    if ( server->flags & FLAG_BROADCAST)  {
		broadcast= server;
		/* create new server and init */
		server= add_qserver_byaddr( ntohl(addr.sin_addr.s_addr),
			ntohs(addr.sin_port), server->type, NULL);
		server->packet_time1= broadcast->packet_time1;
		server->packet_time2= broadcast->packet_time2;
		server->ping_total= broadcast->ping_total;
		server->n_requests= broadcast->n_requests;
		server->n_packets= broadcast->n_packets;
		broadcast->n_servers++;
	    }

	    server->type->packet_func( server, pkt, pktlen);
	    if ( broadcast)
		server= broadcast;
	}
#endif /* _WIN32 */

	if ( run_timeout && time(0)-start_time >= run_timeout)
	    break;
	if ( ! connected)
	    bind_sockets();
    }

    hcache_update_file();

    if ( progress)  {
	display_progress();
	fputs( "\n", stderr);
    }

    if ( sort)  {
	struct qserver **array, *server;
	array= (struct qserver **) malloc( sizeof(struct qserver *) *
		num_servers_total);
	server= servers;
	for ( i= 0; server != NULL; i++)  {
	    array[i]= server;
	    server= server->next;
	}
	qstatsort( array, num_servers_total);
	if ( progress)
	    fprintf( stderr, "\n");
	for ( i= 0; i < num_servers_total; i++)
	    display_server( array[i]);
	free( array);
    }
    else  {
	struct qserver *server;
	server= servers;
	for ( ; server; server= server->next)
	    if ( server->server_name == HOSTNOTFOUND)
		display_server( server);
    }

    if ( have_trailer_template())
	template_display_trailer();

    return 0;
}


void
add_file( char *filename)
{
    FILE *file;
    char name[200];
    server_type* type;

    if ( strcmp( filename, "-") == 0)  {
	file= stdin;
	current_filename= NULL;
    }
    else  {
	file= fopen( filename, "r");
	current_filename= filename;
    }
    current_fileline= 1;

    if ( file == NULL)  {
	perror( filename);
	return;
    }
    for ( ; fscanf( file, "%s", name) == 1; current_fileline++)  {
	type= find_server_type_string( name);
	if ( type == NULL)
	    add_qserver( name, default_server_type, NULL);
	else if ( fscanf( file, "%s", name) == 1)
	    add_qserver( name, type, NULL);
    }

    if ( file != stdin)
	fclose(file);

    current_fileline= 0;
}

void
print_file_location()
{
    if ( current_fileline != 0)
	fprintf( stderr, "%s:%d: ", current_filename?current_filename:"<stdin>",
		current_fileline);
}

int
add_qserver( char *arg, server_type* type, char *outfilename)
{
    struct sockaddr_in addr;
    struct hostent *ent= NULL;
    struct hostent temp_ent;
    struct hostent *name_ent= NULL;
    struct qserver *server;
    int i, port, flags= 0;
    char **a, *colon= NULL, *arg_copy, *s, *hostname= NULL;
    unsigned long ipaddr;
 
    if ( run_timeout && time(0)-start_time >= run_timeout)
	exit(0);

    port= type->default_port;

    if ( outfilename && strcmp( outfilename, "-") != 0)  {
	FILE *outfile= fopen( outfilename, "r+");
	if ( outfile == NULL && (errno == EACCES || errno == EISDIR ||
		errno == ENOSPC || errno == ENOTDIR))  {
	    perror( outfilename);
	    return -1;
	}
	if ( outfile)
	    fclose(outfile);
    }

    arg_copy= strdup(arg);

    colon= strchr( arg, ':');
    if ( colon != NULL)  {
	sscanf( colon+1, "%d", &port);
	*colon= '\0';
    }

    if ( *arg == '+')  {
	flags|= FLAG_BROADCAST;
	arg++;
    }

    ipaddr= inet_addr(arg);
    if ( ipaddr == INADDR_NONE)  {
	if ( strcmp( arg, "255.255.255.255") != 0)
	    ipaddr= htonl( hcache_lookup_hostname(arg));
    }
    else if ( hostname_lookup && !(flags&FLAG_BROADCAST))
	hostname= hcache_lookup_ipaddr( ntohl(ipaddr));

    if ( ipaddr == INADDR_NONE && strcmp( arg, "255.255.255.255") != 0)  {
	print_file_location();
	fprintf( stderr, "%s: %s\n", arg, strherror(h_errno));
	server= (struct qserver *) calloc( 1, sizeof( struct qserver));
	init_qserver( server);
	server->arg= arg_copy;
	server->server_name= HOSTNOTFOUND;
	server->error= strdup( strherror(h_errno));
	server->port= port;
	server->type= type;
	server->next= servers;
	servers= server;
	server_types|= type->id;
        return -1;
    }

    server= servers;
    while ( server != NULL)  {
	if ( server->ipaddr == ipaddr && server->port == port)
	    return 0;
	server= server->next;
    }

    server= (struct qserver *) calloc( 1, sizeof( struct qserver));
    server->arg= arg_copy;
    if ( hostname && colon)  {
	server->host_name= malloc( strlen(hostname) + strlen(colon+1)+2);
	sprintf( server->host_name, "%s:%s", hostname, colon+1);
    }
    else
	server->host_name= strdup((hostname)?hostname:arg);

    server->ipaddr= ipaddr;
    server->port= port;
    server->type= type;
    server->outfilename= outfilename;
    server->flags= flags;
    init_qserver( server);

    if ( server->type->master)
	waiting_for_masters++;

    if ( num_servers_total % 10 == 0)
	hcache_update_file();

    server->next= servers;
    servers= server;

    server_types|= type->id;

    return 0;
}

struct qserver *
add_qserver_byaddr( unsigned long ipaddr, unsigned short port,
	server_type* type, int *new_server)
{
    char arg[36];
    struct qserver *server;
    struct hostent *name_ent= NULL;
    char *hostname= NULL;

    if ( run_timeout && time(0)-start_time >= run_timeout)
	exit(0);

    if ( new_server)
	*new_server= 0;
    ipaddr= htonl(ipaddr);
    server= servers;
    while ( server != NULL)  {
	if ( server->ipaddr == ipaddr && server->port == port)
	    return server;
	server= server->next;
    }

    if ( new_server)
	*new_server= 1;

    server= (struct qserver *) calloc( 1, sizeof( struct qserver));
    server->ipaddr= ipaddr;
    ipaddr= ntohl(ipaddr);
    sprintf( arg, "%d.%d.%d.%d:%hu", ipaddr>>24, (ipaddr>>16)&0xff,
	(ipaddr>>8)&0xff, ipaddr&0xff, port);
    server->arg= strdup(arg);

    if ( hostname_lookup)
	hostname= hcache_lookup_ipaddr( ipaddr);
    if ( hostname)  {
	server->host_name= malloc( strlen(hostname) + 6 + 2);
	sprintf( server->host_name, "%s:%hu", hostname, port);
    }
    else
	server->host_name= strdup( arg);

    server->port= port;
    server->type= type;
    init_qserver( server);

    if ( num_servers_total % 10 == 0)
	hcache_update_file();

    server->next= servers;
    servers= server;
    return server;
}

void
add_servers_from_masters()
{
    struct qserver *server;
    unsigned long ipaddr, i;
    unsigned short port;
    int n_servers, new_server;
    char *pkt;
    server_type* server_type;
    FILE *outfile;

    for ( server= servers; server != NULL; server= server->next)  {
	if ( !server->type->master || server->master_pkt == NULL)
	    continue;
	pkt= server->master_pkt;
	server_type= find_server_type_id( server->type->master);
	outfile= NULL;
	if ( server->outfilename)  {
	    if ( strcmp( server->outfilename, "-") == 0)
		outfile= stdout;
	    else
	        outfile= fopen( server->outfilename, "w");
	    if ( outfile == NULL)  {
		perror( server->outfilename);
		continue;
	    }
	}
	n_servers= 0;
	for ( i= 0; i < server->master_pkt_len; i+= 6)  {
	    memcpy( &ipaddr, &pkt[i], 4);
	    memcpy( &port, &pkt[i+4], 2);
	    ipaddr= ntohl( ipaddr);
	    port= ntohs( port);
	    new_server= 1;
	    if ( outfile)
		fprintf( outfile, "%s %d.%d.%d.%d:%hu\n",
			server_type->type_string,
			(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
			(ipaddr>>8)&0xff, ipaddr&0xff, port);
	    else
		add_qserver_byaddr( ipaddr, port, server_type, &new_server);
	    n_servers+= new_server;
	}
	free( server->master_pkt);
	server->master_pkt= NULL;
	server->master_pkt_len= 0;
	server->n_servers= n_servers;
	if ( outfile)
	    fclose( outfile);
    }
    if ( hostname_lookup)
	hcache_update_file();
    bind_sockets();
}

void
init_qserver( struct qserver *server)
{
    server->server_name= NULL;
    server->map_name= NULL;
    server->game= NULL;
    server->num_players= 0;
    server->fd= -1;
    if ( server->flags & FLAG_BROADCAST)  {
	server->retry1= 1;
	server->retry2= 1;
    }
    else  {
	server->retry1= n_retries;
	server->retry2= n_retries;
    }
    server->n_retries= 0;
    server->ping_total= 0;
    server->n_packets= 0;
    server->n_requests= 0;

    server->n_servers= 0;
    server->master_pkt_len= 0;
    server->master_pkt= NULL;
    server->error= NULL;

    server->saved_data = NULL;
    server->saved_data_size = 0;

    server->next_rule= (get_server_rules) ? "" : NO_SERVER_RULES;
    server->next_player_info= (get_player_info) ? 0 : NO_PLAYER_INFO;

    server->n_player_info= 0;
    server->players= NULL;
    server->n_rules= 0;
    server->rules= NULL;
    server->missing_rules= 0;

    num_servers_total++;
}

/* Functions for binding sockets to Quake servers
 */
int
bind_qserver( struct qserver *server)
{
    struct sockaddr_in addr;
    static int one= 1;

    if ( server->type->id == UN_MASTER || server->type->id == BFRIS_SERVER)
	server->fd= socket( AF_INET, SOCK_STREAM, 0);
    else
	server->fd= socket( AF_INET, SOCK_DGRAM, 0);

    if ( server->fd == INVALID_SOCKET) {
	if ( sockerr() == EMFILE)  {
	    server->fd= -1;
	    return -2;
	}
        perror( "socket" );
	server->server_name= SYSERROR;
        return -1;
    }

    addr.sin_family = AF_INET;
    if ( server->type->id == Q2_MASTER)
	addr.sin_port = htons(26500);
    else
	addr.sin_port = htons(0);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    memset( &(addr.sin_zero), 0, sizeof(addr.sin_zero) );

    if ( bind( server->fd, (struct sockaddr *)&addr,
		sizeof(struct sockaddr)) == SOCKET_ERROR) {
	if ( sockerr() != EADDRINUSE)  {
	    perror( "bind" );
	    server->server_name= SYSERROR;
	}
	close(server->fd);
	server->fd= -1;
        return -1;
    }

    if ( server->flags & FLAG_BROADCAST)
	setsockopt( server->fd, SOL_SOCKET, SO_BROADCAST, (char*)&one,
		sizeof(one));

    if ( server->type->id != Q2_MASTER &&
		!(server->flags & FLAG_BROADCAST))  {
	addr.sin_family= AF_INET;
	addr.sin_port= htons(server->port + (server->type->id==UN_SERVER));
	addr.sin_addr.s_addr= server->ipaddr;
	memset( &(addr.sin_zero), 0, sizeof(addr.sin_zero) );

	if ( connect( server->fd, (struct sockaddr *)&addr, sizeof(addr)) ==
		SOCKET_ERROR)  {
	    if ( server->type->id == UN_MASTER)  {
		if ( connection_refused())  {
		/*  server->fd= -2; */
		/* set up for connect retry */
		}
	    }
	    perror( "connect");
	    server->server_name= SYSERROR;
	    close(server->fd);
	    server->fd= -1;
	    return -1;
	}
    }

    if ( server->type->id == UN_MASTER || server->type->id == BFRIS_SERVER)
	set_non_blocking( server->fd);

#ifdef unix
    connmap[server->fd]= server;
#endif

    return 0;
}

void
bind_sockets()
{
    struct qserver *server= servers;
    int rc;

    for ( ;	server != NULL && connected < max_simultaneous;
		server= server->next)  {
	if ( server->server_name == NULL && server->fd == -1)  {
	    if ( waiting_for_masters && !server->type->master)
		continue;
	    if ( (rc= bind_qserver( server)) == 0)  {
		server->type->status_query_func( server);
		connected++;
	    }
	    else if ( rc == -2)
		break;
	}
    }
}


/* Functions for sending packets
 */
void
send_packets()
{
    struct qserver *server= servers;
    struct timeval now;
    int rc;
    int interval;

    gettimeofday( &now, NULL);

    for ( ; server != NULL; server= server->next)  {
	if ( server->fd == -1)
	    continue;
	if ( server->type->id & MASTER_SERVER)
	    interval= master_retry_interval;
	else
	    interval= retry_interval;
	if ( server->server_name == NULL || server->type->id == UN_SERVER ||
		server->type->id == SHOGO_SERVER)  {
	    if ( server->retry1 != n_retries &&
		    time_delta( &now, &server->packet_time1) <
			(interval*(n_retries-server->retry1+1)))
		continue;
	    if ( ! server->retry1)  {
		cleanup_qserver( server, 0);
		continue;
	    }
	    server->type->status_query_func( server);
	    continue;
	}
	if ( server->next_rule != NO_SERVER_RULES)  {
	    if ( server->retry1 != n_retries &&
		    time_delta( &now, &server->packet_time1) <
			(interval*(n_retries-server->retry1+1)))
		continue;
	    if ( ! server->retry1)  {
		server->next_rule= NULL;
		server->missing_rules= 1;
		cleanup_qserver( server, 0);
		continue;
	    }
	    send_rule_request_packet( server);
	}
	if ( server->next_player_info < server->num_players &&
		server->type->player_packet)  {
	    if ( server->retry2 != n_retries &&
		    time_delta( &now, &server->packet_time2) <
			(interval*(n_retries-server->retry2+1)))
		continue;
	    if ( ! server->retry2)  {
		server->next_player_info++;
		if ( server->next_player_info >= server->num_players)  {
		    cleanup_qserver( server, 0);
		    continue;
		}
		server->retry2= n_retries;
	    }
	    send_player_request_packet( server);
	}
    }
}

int
send_broadcast( struct qserver *server, char *pkt, int pktlen)
{
    struct sockaddr_in addr;
    addr.sin_family= AF_INET;
    addr.sin_port= htons(server->port);
    addr.sin_addr.s_addr= server->ipaddr;
    memset( &(addr.sin_zero), 0, sizeof(addr.sin_zero));
    return sendto( server->fd, (const char*) pkt, pktlen, 0,
		(struct sockaddr *) &addr, sizeof(addr));
}

/* server starts sending data immediately, so we need not do anything */
void
send_bfris_request_packet( struct qserver *server)
{

  if ( server->retry1 == n_retries || server->flags & FLAG_BROADCAST)  {
    gettimeofday( &server->packet_time1, NULL);
    server->n_requests++;
  }
  else
    server->n_retries++;

  server->retry1--;
  server->n_packets++;
}


/* First packet for a normal Quake server
 */
void
send_qserver_request_packet( struct qserver *server)
{
    int rc;
    if ( server->flags & FLAG_BROADCAST)
	rc= send_broadcast( server, server->type->status_packet,
		server->type->status_len);
    else
	rc= send( server->fd, server->type->status_packet,
		server->type->status_len, 0);

    if ( rc == SOCKET_ERROR)  {
	unsigned long ipaddr= ntohl(server->ipaddr);
	fprintf( stderr,
		"Error on %d.%d.%d.%d, skipping ...\n",
		(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
		(ipaddr>>8)&0xff, ipaddr&0xff);
	perror( "send");
	cleanup_qserver( server, 1);
	return;
    }
    if ( server->retry1 == n_retries || server->flags & FLAG_BROADCAST)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

/* First packet for a QuakeWorld server
 */
void
send_qwserver_request_packet( struct qserver *server)
{
    int rc;

    if ( server->flags & FLAG_BROADCAST)
	rc= send_broadcast( server, server->type->status_packet,
		server->type->status_len);
    else
	rc= send( server->fd, server->type->status_packet,
		server->type->status_len, 0);

    if ( rc == SOCKET_ERROR)  {
	unsigned long ipaddr= ntohl(server->ipaddr);
	fprintf( stderr,
		"Error on %d.%d.%d.%d, skipping ...\n",
		(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
		(ipaddr>>8)&0xff, ipaddr&0xff);
	perror( "send");
	cleanup_qserver( server, 1);
	return;
    }
    if ( server->retry1 == n_retries || server->flags & FLAG_BROADCAST)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

/* First packet for an Unreal server
 */
void
send_unreal_request_packet( struct qserver *server)
{
    int rc;

    rc= send( server->fd, server->type->status_packet,
	server->type->status_len, 0);

    if ( rc == SOCKET_ERROR)
	perror( "send");
    if ( server->retry1 == n_retries)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

/* First packet for an Unreal master
 */
void
send_unrealmaster_request_packet( struct qserver *server)
{
    int rc;

    rc= send( server->fd, server->type->status_packet,
	server->type->status_len, 0);

    if ( rc == SOCKET_ERROR)
	perror( "send");
    if ( server->retry1 == n_retries)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

/* First packet for a QuakeWorld master server
 */
void
send_qwmaster_request_packet( struct qserver *server)
{
    int rc= 0;

    if ( server->type->id != Q2_MASTER)  {
	if ( server->type->id == HL_MASTER)
	    memcpy( server->type->master_packet+1, server->master_query_tag, 3);
	rc= send( server->fd, server->type->master_packet,
		server->type->master_len, 0);
    }
    else if ( server->type->id == Q2_MASTER)  {
	struct sockaddr_in addr;
	addr.sin_family= AF_INET;
	addr.sin_port= htons(server->port);
	addr.sin_addr.s_addr= server->ipaddr;
	memset( &(addr.sin_zero), 0, sizeof(addr.sin_zero));
	rc= sendto( server->fd, server->type->master_packet,
		server->type->master_len, 0,
		(struct sockaddr *) &addr, sizeof(addr));
    }

    if ( rc == SOCKET_ERROR)
	perror( "send");
    if ( server->retry1 == n_retries)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

/* First packet for a Tribes server
 */
void
send_tribes_request_packet( struct qserver *server)
{
    int rc;

    if ( get_player_info || get_server_rules)
    rc= send( server->fd, server->type->player_packet,
	server->type->player_len, 0);
    else
    rc= send( server->fd, server->type->status_packet,
	server->type->status_len, 0);

    if ( rc == SOCKET_ERROR)
	perror( "send");
    if ( server->retry1 == n_retries)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

void
send_rule_request_packet( struct qserver *server)
{
    int rc, len;

    if ( server->type->id == Q_SERVER)  {
	strcpy( (char*)q_rule.data, server->next_rule);
	len= Q_HEADER_LEN + strlen((char*)q_rule.data) + 1;
	q_rule.length= htons( (short)len);
    }
    else
	len= server->type->rule_len;

    rc= send( server->fd, (const char *) server->type->rule_packet,
	len, 0);
    if ( rc == SOCKET_ERROR)
	perror( "send");
    if ( server->retry1 == n_retries)  {
	gettimeofday( &server->packet_time1, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry1--;
    server->n_packets++;
}

void
send_player_request_packet( struct qserver *server)
{
    int rc;

    if ( server->type->id == Q_SERVER)
	q_player.data[0]= server->next_player_info;
    rc= send( server->fd, (const char *) server->type->player_packet,
	server->type->player_len, 0);
    if ( rc == SOCKET_ERROR)
	perror( "send");
    if ( server->retry2 == n_retries)  {
	gettimeofday( &server->packet_time2, NULL);
	server->n_requests++;
    }
    else
	server->n_retries++;
    server->retry2--;
    server->n_packets++;
}

/* Functions for figuring timeouts and when to give up
 */
void
cleanup_qserver( struct qserver *server, int force)
{
    int close_it= force;
    if ( server->server_name == NULL)  {
	close_it= 1;
	if ( server->type->id & MASTER_SERVER && server->master_pkt != NULL)
	    server->server_name= MASTER;
	else  {
	    server->server_name= TIMEOUT;
	    num_servers_timed_out++;
	}
    }
    else if ( server->type->flags & TF_SINGLE_QUERY)
	close_it= 1;
    else if ( server->next_rule == NO_SERVER_RULES &&
		server->next_player_info >= server->num_players)
	close_it= 1;

    if ( close_it)  {

	if ( server->saved_data) {
	    free(server->saved_data);
	    server->saved_data = NULL;
	    server->saved_data_size = 0;
	}

	if ( server->fd != -1)  {
	    close( server->fd);
#ifdef unix
	    connmap[server->fd]= NULL;
#endif
	    server->fd= -1;
	    connected--;
	}
	if ( server->server_name != TIMEOUT)  {
	    num_servers_returned++;
	    if ( server->server_name != DOWN)
		num_players_total+= server->num_players;
	}
	if ( server->server_name == TIMEOUT || server->server_name == DOWN)
	    server->ping_total= 999999;
	if ( server->type->master)  {
	    waiting_for_masters--;
	    if ( waiting_for_masters == 0)
		add_servers_from_masters();
	}
	if ( ! sort)
	    display_server( server);
    }
}

void
get_next_timeout( struct timeval *timeout)
{
    struct qserver *server= servers;
    struct timeval now;
    int diff1, diff2, diff, smallest= retry_interval+master_retry_interval;
    int interval;

    gettimeofday( &now, NULL);
    for ( ; server != NULL; server= server->next)  {
	if ( server->fd == -1)
	    continue;
	if ( server->type->id & MASTER_SERVER)
	    interval= master_retry_interval;
	else
	    interval= retry_interval;
	diff2= 0xffff;
	diff1= 0xffff;
	if ( server->server_name == NULL)
	    diff1= interval*(n_retries-server->retry1+1) -
		time_delta( &now, &server->packet_time1);
	else  {
	    if ( server->next_rule != NULL)
		diff1= interval*(n_retries-server->retry1+1) -
			time_delta( &now, &server->packet_time1);
	    if ( server->next_player_info < server->num_players)
		diff2= interval*(n_retries-server->retry2+1) -
			time_delta( &now, &server->packet_time2);
	}
	diff= (diff1<diff2)?diff1:diff2;
	if ( diff < smallest)
	    smallest= diff;
    }
    if ( smallest < 10)
	smallest= 10;
    timeout->tv_sec= smallest / 1000;
    timeout->tv_usec= (smallest % 1000) * 1000;
}

int
set_fds( fd_set *fds)
{
    struct qserver *server= servers;
    int maxfd= 1, i;
#ifdef unix
    for ( i= 3; i < max_simultaneous+10; i++)
	if ( connmap[i] != NULL)  {
	    FD_SET( connmap[i]->fd, fds);
	    maxfd= connmap[i]->fd;
	}
#endif /* unix */

#ifdef _WIN32
    while ( server != NULL)  {
	if ( server->fd != -1)  {
	    FD_SET( server->fd, fds);
	    if ( server->fd > maxfd)
		maxfd= server->fd;
	}
	server= server->next;
    }
#endif /* _WIN32 */

    return maxfd;
}


/* Functions for handling response packets
 */

/* Packet from normal Quake server
 */
void
deal_with_q_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    struct q_packet *pkt= (struct q_packet *)rawpkt;
    int rc;

    if ( ntohs( pkt->length) != pktlen)  {
	fprintf( stderr, "%s Ignoring bogus packet; length %d != %d\n",
		server->arg, ntohs( pkt->length), pktlen);
	cleanup_qserver(server,FORCE);
	return;
    }

    rawpkt[pktlen]= '\0';

    switch ( pkt->op_code)  {
    case Q_CCREP_ACCEPT:
    case Q_CCREP_REJECT:
	return;
    case Q_CCREP_SERVER_INFO:
	server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time1);
	rc= server_info_packet( server, pkt, pktlen-Q_HEADER_LEN);
	break;
    case Q_CCREP_PLAYER_INFO:
	server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time2);
	rc= player_info_packet( server, pkt, pktlen-Q_HEADER_LEN);
	break;
    case Q_CCREP_RULE_INFO:
	server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time1);
	rc= rule_info_packet( server, pkt, pktlen-Q_HEADER_LEN);
	break;
    case Q_CCREQ_CONNECT:
    case Q_CCREQ_SERVER_INFO:
    case Q_CCREQ_PLAYER_INFO:
    case Q_CCREQ_RULE_INFO:
    default:
	return;
    }

    if ( rc == -1)
	fprintf( stderr, "%s error on packet opcode %x\n", server->arg,
		(int)pkt->op_code);

    cleanup_qserver( server, (rc == -1) ? FORCE : 0);
}

/* Packet from QuakeWorld server
 */
void
deal_with_qw_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    char *pkt= &rawpkt[5];
    char *key, *value, *end;
    int len, rc, complete= 0;

    server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time1);

    if ( (rawpkt[0] != '\377' || rawpkt[1] != '\377' ||
	    rawpkt[2] != '\377' || rawpkt[3] != '\377') && show_errors)  {
	unsigned long ipaddr= ntohl(server->ipaddr);
	fprintf( stderr,
		"Odd packet from QW server %d.%d.%d.%d, processing ...\n",
		(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
		(ipaddr>>8)&0xff, ipaddr&0xff);
	print_packet( rawpkt, pktlen);
    }

    if ( rawpkt[4] == 'n')  {
	if ( server->type->id != QW_SERVER)
	    server->type= find_server_type_id( QW_SERVER);
	deal_with_q1qw_packet( server, rawpkt, pktlen);
	return;
    }
    else if ( rawpkt[4] == '\377' && rawpkt[5] == 'n')  {
	if ( server->type->id != HW_SERVER)
	    server->type= find_server_type_id( HW_SERVER);
	deal_with_q1qw_packet( server, rawpkt, pktlen);
	return;
    }
    else if ( strncmp( &rawpkt[4], "print\n\\", 7) == 0)  {
	rawpkt[pktlen]= '\0';
	deal_with_q2_packet( server, rawpkt+10, pktlen-10);
	return;
    }
    else if ( strncmp( &rawpkt[4], "print\n", 6) == 0)  {
	/* work-around for occasional bug in Quake II status packets
	*/
	char *c, *p;
	rawpkt[pktlen]= '\0';
	p= c= &rawpkt[10];
	while ( *p != '\\' && (c= strchr( p, '\n')))
	    p= c+1;
	if ( *p == '\\' && c != NULL)  {
	    deal_with_q2_packet( server, p, pktlen-(p-rawpkt));
	    return;
	}
    }
    else if ( server->type->id == HL_SERVER)  {
	deal_with_halflife_packet( server, rawpkt, pktlen);
	return;
    }
    else if ( strncmp( &rawpkt[4], "statusResponse\n", 15) == 0)  {
    	/* quake3 response */
	rawpkt[pktlen]= '\0';
    	deal_with_q2_packet( server, rawpkt + 19, pktlen - 19);
	return;
    }

    if ( show_errors)  {
	unsigned long ipaddr= ntohl(server->ipaddr);
	fprintf( stderr,
		"Odd packet from QW server %d.%d.%d.%d, skipping ...\n",
		(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
		(ipaddr>>8)&0xff, ipaddr&0xff);
	print_packet( rawpkt, pktlen);
    }

    cleanup_qserver( server, 0);
}

void
deal_with_q1qw_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    char *key, *value, *end;
    struct rule *rule;
    struct player *player= NULL;
    int len, rc, complete= 0;
    int number, frags, connect_time, ping;
    char *pkt= &rawpkt[5];

    if ( server->type->id == HW_SERVER)
	pkt= &rawpkt[6];

    while ( *pkt && pkt-rawpkt < pktlen)  {
	if ( *pkt == '\\')  {
	    pkt++;
	    end= strchr( pkt, '\\');
	    if ( end == NULL)
		break;
	    *end= '\0';
	    key= pkt;
	    pkt+= strlen(pkt)+1;
	    end= strchr( pkt, '\\');
	    if ( end == NULL)
		end= strchr( pkt, '\n');
	    value= (char*) malloc(end-pkt+1);
	    memcpy( value, pkt, end-pkt);
	    value[end-pkt]= '\0';
	    pkt= end;
	    if ( strcmp( key, "hostname") == 0)
		server->server_name= value;
	    else if  ( strcmp( key, "map") == 0)
		server->map_name= value;
	    else if  ( strcmp( key, "maxclients") == 0)  {
		server->max_players= atoi(value);
		free( value);
	    }
	    else if ( get_server_rules || strncmp( key, "*game", 5) == 0)  {
		rule= (struct rule *) malloc( sizeof( struct rule));
		rule->name= strdup(key);
		rule->value= value;
		rule->next= server->rules;
		server->rules= rule;
		server->n_rules++;
		if ( strcmp( key, "*gamedir") == 0)
		    server->game= value;
	    }
	}
	else if ( *pkt == '\n')  {
	    pkt++;
	    if ( *pkt == '\0')
		break;
	    rc= sscanf( pkt, "%d %d %d %d %n", &number, &frags, &connect_time,
		&ping, &len);
	    if ( rc != 4)  {
		char *nl;	/* assume it's an error packet */
		server->error= malloc( pktlen+1);
	        nl= strchr( pkt, '\n');
		if ( nl != NULL)  {
		    strncpy( server->error, pkt, nl-pkt);
		    server->error[nl-pkt]= '\0';
		}
		else
		    strcpy( server->error, pkt);
		server->server_name= SERVERERROR;
		complete= 1;
		break;
	    }
	    if ( get_player_info)  {
		player= (struct player *) calloc( 1, sizeof( struct player));
		player->number= number;
		player->frags= frags;
		player->connect_time= connect_time * 60;
		player->ping= ping;
	    }
	    else
		player= NULL;

	    pkt+= len;

	    if ( *pkt != '"') break;
	    pkt++;
	    end= strchr( pkt, '"');
	    if ( end == NULL) break;
	    if ( player != NULL)  {
		player->name= (char*) malloc(end-pkt+1);
		memcpy( player->name, pkt, end-pkt);
		player->name[end-pkt]= '\0';
	    }
	    pkt= end+2;

	    if ( *pkt != '"') break;
	    pkt++;
	    end= strchr( pkt, '"');
	    if ( end == NULL) break;
	    if ( player != NULL)  {
		player->skin= (char*) malloc(end-pkt+1);
		memcpy( player->skin, pkt, end-pkt);
		player->skin[end-pkt]= '\0';
	    }
	    pkt= end+2;

	    if ( player != NULL)  {
		sscanf( pkt, "%d %d%n", &player->shirt_color,
			&player->pants_color, &len);
		player->next= server->players;
		server->players= player;
	    }
	    else
		sscanf( pkt, "%*d %*d%n", &len);
	    pkt+= len;

	    server->num_players++;
	}
	else
	    pkt++;
	complete= 1;
    }

    if ( !complete)
	print_packet( rawpkt, pktlen);
    else if ( server->server_name == NULL)
	server->server_name= strdup("");

    cleanup_qserver( server, 0);
}

void
deal_with_q2_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    char *key, *value, *end;
    struct rule *rule;
    struct player *player= NULL;
    int len, rc, complete= 0;
    int frags, ping;
    char *pkt= rawpkt;

/*	print_packet( rawpkt, pktlen);
*/

    while ( *pkt && pkt-rawpkt < pktlen)  {
	if ( *pkt == '\\')  {
	    pkt++;
	    end= strchr( pkt, '\\');
	    if ( end == NULL)
		break;
	    *end= '\0';
	    key= pkt;
	    pkt+= strlen(pkt)+1;
	    end= strchr( pkt, '\\');
	    if ( end == NULL)
		end= strchr( pkt, '\n');
	    value= (char*) malloc(end-pkt+1);
	    memcpy( value, pkt, end-pkt);
	    value[end-pkt]= '\0';
	    pkt= end;
	    if ( strcmp( key, "hostname") == 0 ||
			strcmp( key, "sv_hostname") == 0)
		server->server_name= value;
	    else if  ( strcmp( key, "mapname") == 0 ||
		    (strcmp( key, "map") == 0 && server->map_name == NULL))  {
		if ( server->map_name != NULL)
		    free( server->map_name);
		server->map_name= value;
	    }
	    else if  ( strcmp( key, "maxclients") == 0 || 
	    		strcmp( key, "sv_maxclients") == 0)  {
		server->max_players= atoi(value);
		free(value);
	    }
	    else if ( get_server_rules || strncmp( key, "game", 4) == 0)  {
		rule= (struct rule *) malloc( sizeof( struct rule));
		rule->name= strdup(key);
		rule->value= value;
		rule->next= server->rules;
		server->rules= rule;
		server->n_rules++;
		if ( strcmp( key, "game") == 0)
		    server->game= value;
	    }
	}
	else if ( *pkt == '\n')  {
	    pkt++;
	    if ( *pkt == '\0')
		break;
	    rc= sscanf( pkt, "%d %d %n", &frags, &ping, &len);
	    if ( rc != 2)  {
		char *nl;	/* assume it's an error packet */
		server->error= malloc( pktlen+1);
	        nl= strchr( pkt, '\n');
		if ( nl != NULL)
		    strncpy( server->error, pkt, nl-pkt);
		else
		    strcpy( server->error, pkt);
		server->server_name= SERVERERROR;
		complete= 1;
		break;
	    }
	    if ( get_player_info)  {
		player= (struct player *) calloc( 1, sizeof( struct player));
		player->number= 0;
		player->connect_time= -1;
		player->frags= frags;
		player->ping= ping;
	    }
	    else
		player= NULL;

	    pkt+= len;

	    if ( *pkt != '"') break;
	    pkt++;
	    end= strchr( pkt, '"');
	    if ( end == NULL) break;
	    if ( player != NULL)  {
		player->name= (char*) malloc(end-pkt+1);
		memcpy( player->name, pkt, end-pkt);
		player->name[end-pkt]= '\0';
	    }
	    pkt= end+1;

	    if ( player != NULL)  {
		player->skin= NULL;
		player->shirt_color= -1;
		player->pants_color= -1;
		player->next= server->players;
		server->players= player;
	    }
	    server->num_players++;
	}
	else
	    pkt++;
	complete= 1;
    }

    if ( !complete)
	print_packet( rawpkt, pktlen);
    else if ( server->server_name == NULL)
	server->server_name= strdup("");

    cleanup_qserver( server, 0);
}

/* Packet from QuakeWorld master server
 */
void
deal_with_qwmaster_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    unsigned int ipaddr;
    unsigned short port;
    int i;

/*  print_packet( rawpkt, pktlen);
*/

    server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time1);

    if ( rawpkt[0] == QW_NACK)  {
	server->error= strdup( &rawpkt[2]);
	server->server_name= SERVERERROR;
	cleanup_qserver( server, 1);
	return;
    }

    if ( *((unsigned int*)rawpkt) == 0xffffffff)  {
	rawpkt+= 4;	/* QW 1.5 */
	pktlen-= 4;
    }

    if ( rawpkt[0] == QW_SERVERS && rawpkt[1] == QW_NEWLINE)  {
	rawpkt+= 2;
	pktlen-= 2;
    }
    else if ( rawpkt[0] == HL_SERVERS && rawpkt[1] == 0x0d)  {
	memcpy( server->master_query_tag, rawpkt+2, 3);
	rawpkt+= 6;
	pktlen-= 6;
    }
    else if ( strncmp( rawpkt, "servers", 7) == 0)  {
	rawpkt+= 8;
	pktlen-= 8;
    }
    else if ( strncmp( rawpkt, "getserversResponse", 18) == 0)  {
    	char *p, *q, *s;
	unsigned int ipaddr;
	unsigned short port;
	static int q3m_debug= 0;

	rawpkt+= 18;
	pktlen-= 18;

if ( q3m_debug) printf( "q3m pktlen %d lastchar %x\n", pktlen, (unsigned int)rawpkt[pktlen-1]);
	q = server->master_pkt= (char*)realloc( server->master_pkt,
		server->master_pkt_len + pktlen+1);
	p = rawpkt;
	rawpkt[pktlen] = 0;

	while (p < &rawpkt[pktlen-6]) {
	    memcpy( server->master_pkt + server->master_pkt_len, &p[1], 4);
	    memcpy( server->master_pkt + server->master_pkt_len + 4, &p[5], 2);
	    server->master_pkt_len += 6;
	    p++;
	    while (*p != '\\')
		p++;
	}
	server->n_servers= server->master_pkt_len / 6;
	server->next_player_info= -1;
if ( q3m_debug) printf( "q3m %d servers\n", server->n_servers);
	server->retry1= 0;
	return;
    }
    else if ( show_errors)  {
	unsigned int ipaddr= ntohl(server->ipaddr);
	fprintf( stderr,
		"Odd packet from QW master %d.%d.%d.%d, processing ...\n",
		(ipaddr>>24)&0xff, (ipaddr>>16)&0xff,
		(ipaddr>>8)&0xff, ipaddr&0xff);
	print_packet( rawpkt, pktlen);
    }

    server->master_pkt= (char*)realloc( server->master_pkt,
	server->master_pkt_len+pktlen+1);
    rawpkt[pktlen]= '\0';
    memcpy( server->master_pkt+server->master_pkt_len, rawpkt, pktlen+1);
    server->master_pkt_len+= pktlen;

    server->n_servers= server->master_pkt_len / 6;

    if ( server->type->flags & TF_MASTER_MULTI_RESPONSE)  {
	server->next_player_info= -1;
	server->retry1= 0;
    }
    else if ( server->type->id == HL_MASTER)  {
	if ( server->master_query_tag[0] == 0 &&
		server->master_query_tag[1] == 0 &&
		server->master_query_tag[2] == 0)  {
	    server->server_name= MASTER;
	    cleanup_qserver( server, 1);
	    bind_sockets();
	}
	else  {
	    server->retry1++;
	    send_qwmaster_request_packet( server);
	}
    }
    else  {
	server->server_name= MASTER;
	cleanup_qserver( server, 0);
	bind_sockets();
    }
}


/* Packet from Tribes master server
 */
void
deal_with_tribesmaster_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    unsigned char *upkt= (unsigned char*) rawpkt;
    int packet_number= upkt[2];
    int n_packets= upkt[3];
    unsigned char *p;
    char *mpkt;
    int len;
    unsigned int ipaddr;
    unsigned short port;

    if ( memcmp( rawpkt, tribes_master_response, sizeof(tribes_master_response)) != 0)  {
	printf( "Odd packet from Tribes master server\n");
	print_packet( rawpkt, pktlen);
    }

    /* 0x1006
       01		packet number
       08		# packets
       02
       0000
       66
       0d		length of following string
       "Tribes Master"
       3c		length of following string
       "Check out the Starsiege demo now!   www.starsiegeplayers.com"
       0035
       06 d143 4764 812c
       06 d1e2 8df3 616d
       06 1804 6d50 616d
       06 d81c 6dc0 616d
*/
/* 0x1006
   02
   08
   02 0000
   66
   00 3f
   06 cf88 344c 1227
*/

/* printf( "packet_number %d n_packets %d\n", packet_number, n_packets);
*/

    len= upkt[8];
    if ( len > 0)  {
	p= (unsigned char*) rawpkt+9;
/* printf( "%.*s\n", len, p);
*/
	p+= len;
	len= upkt[8+len+1];
/* printf( "%.*s\n", len, p+1);
*/
	p+= len+1;
	p+= 2;
    }
    else
	p= (unsigned char*) rawpkt+10;

    if ( server->master_pkt == NULL)  {
	server->master_pkt= (char*)malloc( n_packets*64*6);
	mpkt= server->master_pkt;
    }
    else
	mpkt= server->master_pkt + server->n_servers*6;

    while ( (char*)p < rawpkt+pktlen)  {
	if ( *p != 0x6) printf( "*p %u\n", (unsigned)*p);
	memcpy( mpkt, p+1, sizeof(ipaddr));
	if ( 0)  {
	    mpkt[4]= p[5];
	    mpkt[5]= p[6];
	}
	else  {
	    mpkt[5]= p[5];
	    mpkt[4]= p[6];
	}
/*
    printf( "%08x:%hu %u.%u.%u.%u:%hu\n", ipaddr, port, ipaddr>>24,
	(ipaddr>>16)&0xff, (ipaddr>>8)&0xff, ipaddr&0xff, port);
*/
	p+= 7;
	mpkt+= 6;
    }
/*
if (  (char*)p != rawpkt+pktlen)
printf( "%x %x\n", p, rawpkt+pktlen);
*/
    server->master_pkt_len= mpkt - server->master_pkt;
    server->n_servers= server->master_pkt_len / 6;
    server->server_name= MASTER;
    server->next_player_info= -1;

    if ( packet_number >= n_packets)
	cleanup_qserver( server, 1);
    else
	cleanup_qserver( server, 0);
}

int
server_info_packet( struct qserver *server, struct q_packet *pkt, int datalen)
{
    int off= 0;

    /* ignore duplicate packets */
    if ( server->server_name != NULL)
	return 0;

    server->address= strdup((char*)&pkt->data[off]);
    off+= strlen(server->address) + 1;
    if ( off >= datalen)
	return -1;

    server->server_name= strdup((char*)&pkt->data[off]);
    off+= strlen(server->server_name) + 1;
    if ( off >= datalen)
	return -1;

    server->map_name= strdup((char*)&pkt->data[off]);
    off+= strlen(server->map_name) + 1;
    if ( off > datalen)
	return -1;

    server->num_players= pkt->data[off++];
    server->max_players= pkt->data[off++];
    server->protocol_version= pkt->data[off++];

    server->retry1= n_retries;

    if ( get_server_rules)
	send_rule_request_packet( server);
    if ( get_player_info)
	send_player_request_packet( server);

    return 0;
}

int
player_info_packet( struct qserver *server, struct q_packet *pkt, int datalen)
{
    char *name, *address;
    int off, colors, frags, connect_time, player_number;
    struct player *player, *last;

    off= 0;
    player_number= pkt->data[off++];
    name= (char*) &pkt->data[off];
    off+= strlen(name)+1;
    if ( off >= datalen)
	return -1;

    colors= pkt->data[off+3];
    colors= (colors<<8) + pkt->data[off+2];
    colors= (colors<<8) + pkt->data[off+1];
    colors= (colors<<8) + pkt->data[off];
    off+= sizeof(colors);

    frags= pkt->data[off+3];
    frags= (frags<<8) + pkt->data[off+2];
    frags= (frags<<8) + pkt->data[off+1];
    frags= (frags<<8) + pkt->data[off];
    off+= sizeof(frags);

    connect_time= pkt->data[off+3];
    connect_time= (connect_time<<8) + pkt->data[off+2];
    connect_time= (connect_time<<8) + pkt->data[off+1];
    connect_time= (connect_time<<8) + pkt->data[off];
    off+= sizeof(connect_time);

    address= (char*) &pkt->data[off];
    off+= strlen(address)+1;
    if ( off > datalen)
	return -1;

    last= server->players;
    while ( last != NULL && last->next != NULL)  {
	if ( last->number == player_number)
	     return 0;
	last= last->next;
    }
    if ( last != NULL && last->number == player_number)
	return 0;

    player= (struct player *) calloc( 1, sizeof(struct player));
    player->number= player_number;
    player->name= strdup( name);
    player->address= strdup( address);
    player->connect_time= connect_time;
    player->frags= frags;
    player->shirt_color= colors>>4;
    player->pants_color= colors&0xf;
    player->next= NULL;

    if ( last == NULL)
	server->players= player;
    else
	last->next= player;

    server->next_player_info++;
    server->retry2= n_retries;
    if ( server->next_player_info < server->num_players)
	send_player_request_packet( server);

    return 0;
}

int
rule_info_packet( struct qserver *server, struct q_packet *pkt, int datalen)
{
    int off= 0;
    struct rule *rule, *last;
    char *name, *value;

    /* Straggler packet after we've already given up fetching rules */
    if ( server->next_rule == NULL)
	return 0;

    if ( ntohs(pkt->length) == Q_HEADER_LEN)  {
	server->next_rule= NULL;
	return 0;
    }

    name= (char*)&pkt->data[off];
    off+= strlen( name)+1;
    if ( off >= datalen)
	return -1;

    value= (char*)&pkt->data[off];
    off+= strlen( value)+1;
    if ( off > datalen)
	return -1;

    last= server->rules;
    while ( last != NULL && last->next != NULL)  {
	if ( strcmp( last->name, name) == 0)
	     return 0;
	last= last->next;
    }
    if ( last != NULL && strcmp( last->name, name) == 0)
	return 0;

    rule= (struct rule *) malloc( sizeof( struct rule));
    rule->name= strdup( name);
    rule->value= strdup( value);
    rule->next= NULL;

    if ( last == NULL)
	server->rules= rule;
    else
	last->next= rule;

    server->n_rules++;
    server->next_rule= rule->name;
    server->retry1= n_retries;
    send_rule_request_packet( server);

    return 0;
}

void
add_rule( struct qserver *server, char *key, char *value) 
{
    struct rule *rule;
    for ( rule= server->rules; rule; rule= rule->next)
	if ( strcmp( rule->name, key) == 0)
	    return;

    rule= (struct rule *) malloc( sizeof( struct rule));
    rule->name= strdup(key);
    rule->value= strdup(value);
    rule->next= server->rules;
    server->rules= rule;
    server->n_rules++;
}

void
add_nrule( struct qserver *server, char *key, char *value, int len) 
{
    struct rule *rule;
    for ( rule= server->rules; rule; rule= rule->next)
	if ( strcmp( rule->name, key) == 0)
	    return;

    rule= (struct rule *) malloc( sizeof( struct rule));
    rule->name= strdup(key);
    rule->value= strndup(value,len);
    rule->next= server->rules;
    server->rules= rule;
    server->n_rules++;
}

struct player *
add_player( struct qserver *server, int player_number)
{
    struct player *player;
    for ( player= server->players; player; player= player->next)
	if ( player->number == player_number)
	    return NULL;

    player= (struct player *) calloc( 1, sizeof( struct player));
    player->number= player_number;
    player->next= server->players;
    server->players= player;
    return player;
}

void
deal_with_unreal_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    char *term= NULL, *s, *key, *value;
    struct player *player= NULL;
    int i, id_major, id_minor, final=0;

    if ( server->server_name == NULL)
	server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time1);
    else
	gettimeofday( &server->packet_time1, NULL);

    rawpkt[pktlen]= '\0';

    s= rawpkt;
    while ( *s)  {
	while ( *s == '\\') s++;
	if ( !*s) break;
	key= s;
	while ( *s && *s != '\\') s++;
	if ( !*s) break;
	*s++= '\0';
	while ( *s == '\\') s++;
	if ( !*s)
	    value= NULL;
	else
	    value= s;
	while ( *s && *s != '\\') s++;
	if ( *s)
	    *s++= '\0';

	if ( value == NULL)  {
	    if ( strcmp( key, "final") == 0)
		final= 1;
	    break;
	}
	
	if ( strcmp( key, "mapname") == 0 && !server->map_name)
	    server->map_name= strdup( value);
	else if ( strcmp( key, "hostname") == 0 && !server->server_name)
	    server->server_name= strdup( value);
	else if ( strcmp( key, "maxplayers") == 0)
	    server->max_players= atoi( value);
	else if ( strcmp( key, "numplayers") == 0)
	    server->num_players= atoi( value);
	else if ( strcmp( key, server->type->game_rule) == 0 && !server->game) {
	    server->game= strdup( value);
	    add_rule( server, key, value);
	}
	else if ( strcmp( key, "queryid") == 0)
	    sscanf( value, "%d.%d", &id_major, &id_minor);
	else if ( strcmp( key, "final") == 0)  {
	    final= 1;
	    break;
	}
	else if ( strncmp( key, "player_", 7) == 0)  {
	    player= add_player( server, atoi(key+7));
	    if ( player) 
		player->name= strdup( value);
	}
	else if ( player && strncmp( key, "frags_", 6) == 0)
	    player->frags= atoi( value);
	else if ( player && strncmp( key, "team_", 5) == 0)
	    player->team= atoi( value);
	else if ( player && strncmp( key, "skin_", 5) == 0)
	    player->skin= strdup( value);
	else if ( player && strncmp( key, "mesh_", 5) == 0)
	    player->mesh= strdup( value);
	else if ( player && strncmp( key, "ping_", 5) == 0)
	    player->ping= atoi( value);
	else if ( player && strncmp( key, "face_", 5) == 0)
	    player->face= strdup( value);
	else if ( !player)
	    add_rule( server, key, value);
    }

    if ( final)
	cleanup_qserver( server, 1);
    if ( server->num_players < 0 && id_minor >= 3)
	cleanup_qserver( server, 1);
}

int
deal_with_unrealmaster_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    if ( pktlen == 0)  {
	cleanup_qserver( server, 1);
	return 0;
    }
    print_packet( rawpkt, pktlen);
puts( "--");
    return 0;
}

int
deal_with_halflife_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    char *pkt;
    if ( pktlen < 5)  {
	cleanup_qserver( server, 1);
	return 0;
    }

    /* 'info' response */
    if ( rawpkt[4] == 'C')  {
	if ( server->server_name != NULL)
	    return 0;
	pkt= &rawpkt[5];
	server->address= strdup( pkt);
	pkt+= strlen(pkt)+1;
	server->server_name= strdup( pkt);
	pkt+= strlen(pkt)+1;
	server->map_name= strdup( pkt);
	pkt+= strlen(pkt)+1;

	if ( *pkt)
	    add_rule( server, "gamedir", pkt);
	if ( *pkt && strcmp( pkt, "valve") != 0)
	    add_rule( server, "game", pkt);
	pkt+= strlen(pkt)+1;
	if ( *pkt)
	    add_rule( server, "gamename", pkt);
	pkt+= strlen(pkt)+1;

	server->num_players= (unsigned int)pkt[0];
	server->max_players= (unsigned int)pkt[1];

	if ( get_player_info && server->num_players)  {
	    server->next_player_info= server->num_players-1;
	    send_player_request_packet( server);
	}
	if ( get_server_rules)  {
	    server->next_rule= "";
	    server->retry1= n_retries;
	    send_rule_request_packet( server);
	}
    }
    /* 'players' response */
    else if ( rawpkt[4] == 'D' && server->players == NULL)  {
	unsigned int n= 0, temp;
	float *ingame= (float*) &temp;
	struct player *player;
	server->num_players= (unsigned int)rawpkt[5];
	pkt= &rawpkt[6];
	rawpkt[pktlen]= '\0';
	while (1)  {
	    if ( *pkt != n+1)
		break;
	    n++;
	    pkt++;
	    player= (struct player*) calloc( 1, sizeof(struct player));
	    player->name= strdup( pkt);
	    pkt+= strlen(pkt)+1;
	    memcpy( &player->frags, pkt, 4);
	    pkt+= 4;
	    memcpy( &temp, pkt, 4);
	    pkt+= 4;
	    if ( big_endian)  {
		player->frags= swap_long( &player->frags);
		temp= swap_long( &temp);
	    }
	    player->connect_time= *ingame;
	    player->next= server->players;
	    server->players= player;
/*printf( "%d %08x %f (%08x) %s\n", n, player->frags, *ingame, temp,
player->name);
*/
	}
	server->num_players= n;
/*	print_packet( rawpkt, pktlen);
*/
	server->next_player_info= server->num_players;
    }
    /* 'rules' response */
    else if ( rawpkt[4] == 'E' && server->next_rule != NULL)  {
	int n= 0;
	struct rule *rule;
/*	printf( "got HL rules packet\n");
	print_packet( rawpkt, pktlen);
*/
	n= (unsigned)rawpkt[5] + (unsigned)rawpkt[6]*256;
	pkt= &rawpkt[7];
	while ( n)  {
	    rule= (struct rule*) malloc( sizeof(struct rule));
	    rule->name= strdup( pkt);
	    pkt+= strlen(pkt)+1;
	    rule->value= strdup( pkt);
	    pkt+= strlen(pkt)+1;
	    rule->next= server->rules;
	    server->rules= rule;
	    server->n_rules++;
	    n--;
	}
	server->next_rule= NULL;
    }

    cleanup_qserver( server, 0);
    return 0;
}


static int tribes_debug= 0;

void
deal_with_tribes_packet( struct qserver *server, char *rawpkt, int pktlen)
{
    unsigned char *pkt, *end;
    int len, pnum, ping, packet_loss, n_teams, t;
    struct player *player;
    struct player **teams= NULL;
    char buf[24];

    if ( server->server_name == NULL)
	server->ping_total+= time_delta( &packet_recv_time,
		&server->packet_time1);
    else
	gettimeofday( &server->packet_time1, NULL);

if ( tribes_debug)     print_packet( rawpkt, pktlen);

    if ( pktlen < sizeof( tribes_info_reponse))  {
	cleanup_qserver( server, 1);
	return;
    }
    if ( get_player_info || get_server_rules)  {
	if ( strncmp( rawpkt, tribes_players_reponse,
			sizeof( tribes_players_reponse)) != 0)  {
	    cleanup_qserver( server, 1);
	    return;
	}
    }
    else if ( strncmp( rawpkt, tribes_info_reponse,
		sizeof( tribes_info_reponse)) != 0)  {
	cleanup_qserver( server, 1);
	return;
    }

    pkt= (unsigned char*) &rawpkt[sizeof( tribes_info_reponse)];

    len= *pkt;		/* game name: "Tribes" */
    add_nrule( server, "gamename", (char*)pkt+1, len);
    pkt+= len+1;
    len= *pkt;		/* version */
    add_nrule( server, "version", (char*)pkt+1, len);
    pkt+= len+1;
    len= *pkt;		/* server name */
    server->server_name= strndup( (char*)pkt+1, len);
    pkt+= len+1;
    add_rule( server, "dedicated", *pkt?"1":"0");
    pkt++;		/* flag: dedicated server */
    add_rule( server, "needpass", *pkt?"1":"0");
    pkt++;		/* flag: password on server */
    server->num_players= *pkt++;
    server->max_players= *pkt++;

    if ( !get_player_info && !get_server_rules)  {
	cleanup_qserver( server, 0);
	return;
    }

    sprintf( buf, "%u", (unsigned int)pkt[0] + (unsigned int)pkt[1]*256);
    add_rule( server, "cpu", buf);
    pkt++;		/* cpu speed, lsb */
    pkt++;		/* cpu speed, msb */

    len= *pkt;		/* Mod (game) */
    add_nrule( server, "mods", (char*)pkt+1, len);
    pkt+= len+1;

    len= *pkt;		/* game (mission): "C&H" */
    add_nrule( server, "game", (char*)pkt+1, len);
    pkt+= len+1;

    len= *pkt;		/* Mission (map) */
    server->map_name= strndup( (char*)pkt+1, len);
    pkt+= len+1;

    len= *pkt;		/* description (contains Admin: and Email: ) */
if ( tribes_debug) printf( "%.*s\n", len, pkt+1);
    pkt+= len+1;

    n_teams= *pkt++;		/* number of teams */
    if ( n_teams == 255)  {
	cleanup_qserver( server, 1);
	return;
    }
    sprintf( buf, "%d", n_teams);
    add_rule( server, "numteams", buf);

    len= *pkt;		/* first title */
if ( tribes_debug) printf( "%.*s\n", len, pkt+1);
    pkt+= len+1;

    len= *pkt;		/* second title */
if ( tribes_debug) printf( "%.*s\n", len, pkt+1);
    pkt+= len+1;

    if ( n_teams > 1)  {
	teams= (struct player**) calloc( 1, sizeof(struct player*) * n_teams);
	for ( t= 0; t < n_teams; t++)  {
	    teams[t]= (struct player*) calloc(1, sizeof(struct player));
	    teams[t]->number= TRIBES_TEAM;
	    teams[t]->team= t;
	    len= *pkt;		/* team name */
	    teams[t]->name= strndup( (char*)pkt+1, len);
if ( tribes_debug) printf( "team#0 <%.*s>\n", len, pkt+1);
	    pkt+= len+1;

	    len= *pkt;		/* team score */
if ( len <= 2 && tribes_debug) printf( "%s score len %d\n", server->arg, len);
	    if ( len > 2)  {
		strncpy( buf, (char*)pkt+1+3, len-3);
		buf[len-3]= '\0';
	    }
	    else
		buf[0]= '\0';
	    teams[t]->frags= atoi( buf);
if ( tribes_debug) printf( "team#0 <%.*s>\n", len-3, pkt+1+3);
	    pkt+= len+1;
	}
    }
    else  {
	len= *pkt;		/* DM team? */
if ( tribes_debug) printf( "%.*s\n", len, pkt+1);
	pkt+= len+1;
	pkt++;
	n_teams= 0;
    }

    pnum= 0;
    while ( (char*)pkt < (rawpkt+pktlen))  {
	ping= (unsigned int)*pkt << 2;
	pkt++;
	packet_loss= *pkt;
	pkt++;
if ( tribes_debug) printf( "player#%d, team #%d\n", pnum, (int)*pkt);
	pkt++;
	len= *pkt;
	if ( (char*)pkt+len > (rawpkt+pktlen))
	    break;
	player= (struct player*) calloc( 1, sizeof(struct player));
	player->team= pkt[-1];
	if ( n_teams && player->team < n_teams)
	    player->team_name= teams[player->team]->name;
	else if ( player->team == 255 && n_teams)
	    player->team_name= "Unknown";
	player->ping= ping;
	player->packet_loss= packet_loss;
	player->name= strndup( (char*)pkt+1, len);
if ( tribes_debug) 	printf( "player#%d, name %.*s\n", pnum, len, pkt+1);
	pkt+= len+1;
	len= *pkt;
if ( tribes_debug) printf( "player#%d, info <%.*s>\n", pnum, len, pkt+1);
	end= (unsigned char*) strchr( (char*)pkt+9, 0x9);
	if ( end)  {
	    strncpy( buf, (char*)pkt+9, end-(pkt+9));
	    buf[end-(pkt+9)]= '\0';
	    player->frags= atoi( buf);
if ( tribes_debug) printf( "player#%d, score <%.*s>\n", pnum, end-(pkt+9), pkt+9);
	}
	player->next= server->players;
	server->players= player;

	pkt+= len+1;
	pnum++;
    }

    for ( t= n_teams; t;)  {
	t--;
	teams[t]->next= server->players;
	server->players= teams[t];
    }
    free( teams);

    cleanup_qserver( server, 0);
}

/* postions of map name, player name (in player substring), zero-based */
#define BFRIS_MAP_POS 18
#define BFRIS_PNAME_POS 11
void
deal_with_bfris_packet( struct qserver *server, char *rawpkt, int pktlen)
{

  int i, player_data_pos, nplayers;
  char *player_data, player_data_current;

  server->ping_total += time_delta( &packet_recv_time,
				    &server->packet_time1);

  /* add to the data previously saved */
  if (! server->saved_data)
    server->saved_data = malloc(pktlen);
  else
    server->saved_data = realloc(server->saved_data,server->saved_data_size + pktlen);

  memcpy(server->saved_data + server->saved_data_size, rawpkt, pktlen);
  server->saved_data_size += pktlen;

  /* after we get the server portion of the data, server->game != NULL */
  if (!server->game) {

    /* server data goes up to map name */
    if (server->saved_data_size <= BFRIS_MAP_POS)
      return;

    /* see if map name is complete */
    player_data_pos=0;
    for (i=BFRIS_MAP_POS; i<server->saved_data_size; i++) {
      if (server->saved_data[i] == '\0') {
	player_data_pos = i+1;
	/* data must extend beyond map name */
	if (server->saved_data_size <= player_data_pos)
	  return;
	break;
      }
    }

    /* did we find beginning of player data? */
    if (!player_data_pos)
      return;

    /* now we can go ahead and fill in server data */
    server->map_name = strdup((char*)server->saved_data + BFRIS_MAP_POS);
    server->max_players = server->saved_data[12];
    server->protocol_version = server->saved_data[11];

    /* save game type */
    switch (server->saved_data[13] & 15) {
    case 0: server->game = "FFA"; break;
    case 5: server->game = "Rover"; break;
    case 6: server->game = "Occupation"; break;
    case 7: server->game = "SPAAL"; break;
    case 8: server->game = "CTF"; break;
    default:
      server->game = "unknown"; break;
    }
    add_rule(server,server->type->game_rule,server->game);

    if (get_server_rules) {
      char buf[24];

      /* server revision */
      sprintf(buf,"%d",(unsigned int)server->saved_data[11]);
      add_rule(server,"Revision",buf);

      /* latency */
      sprintf(buf,"%d",(unsigned int)server->saved_data[10]);
      add_rule(server,"Latency",buf);

      /* player allocation */
      add_rule(server,"Allocation",server->saved_data[13] & 16 ? "Automatic" : "Manual");

    }

  }

  /* If we got this far, we know the data saved goes at least to the start of
     the player information, and that the server data is taken care of.
  */

  /* start of player data */
  player_data_pos = BFRIS_MAP_POS + strlen((char*)server->saved_data+BFRIS_MAP_POS) + 1;

  /* ensure all player data have arrived */
  nplayers = 0;
  while (server->saved_data[player_data_pos] != '\0') {

    player_data_pos += BFRIS_PNAME_POS;

    /* does player data extend to player name? */
    if (server->saved_data_size <= player_data_pos + 1)
      return;

    /* does player data extend to end of player name? */
    for (i=0; player_data_pos + i < server->saved_data_size; i++) {

      if (server->saved_data_size == player_data_pos + i + 1)
	return;

      if (server->saved_data[player_data_pos + i] == '\0') {
	player_data_pos += i + 1;
	nplayers++;
	break;
      }
    }
  }
  /* all player data are complete */

  server->num_players = nplayers;

  if (get_player_info) {

    /* start of player data */
    player_data_pos = BFRIS_MAP_POS + strlen((char*)server->saved_data+BFRIS_MAP_POS) + 1;

    for (i=0; i<nplayers; i++) {
      struct player *player;
      player= add_player( server, server->saved_data[player_data_pos]);

      player->ship = server->saved_data[player_data_pos + 1]; 
      player->ping = server->saved_data[player_data_pos + 2];
      player->frags = server->saved_data[player_data_pos + 3];
      player->team = server->saved_data[player_data_pos + 4];
      switch (player->team) {
      case 0: player->team_name = "silver"; break;
      case 1: player->team_name = "red"; break;
      case 2: player->team_name = "blue"; break;
      case 3: player->team_name = "green"; break;
      case 4: player->team_name = "purple"; break;
      case 5: player->team_name = "yellow"; break;
      case 6: player->team_name = "cyan"; break;
      default:
	player->team_name = "unknown"; break;
      }
      player->room = server->saved_data[player_data_pos + 5];

      /* score is little-endian integer */
      player->score =
	server->saved_data[player_data_pos+7] +
	(server->saved_data[player_data_pos+8] << 8) +
	(server->saved_data[player_data_pos+9] << 16) +
	(server->saved_data[player_data_pos+10] << 24);

	  /* for archs with > 4-byte int */
      if (player->score & 0x80000000)
	player->score = -(~(player->score)) - 1;


      player_data_pos += BFRIS_PNAME_POS;
      player->name = strdup((char*)server->saved_data + player_data_pos);

      player_data_pos += strlen(player->name) + 1;
    }

  }

  server->server_name = "BFRIS Server";
  cleanup_qserver(server, 1);
  return;
}

/* Misc utility functions
 */

char *
strndup( char *string, int len)
{
    char *result;
    result= (char*) malloc( len+1);
    memcpy( result, string, len);
    result[len]= '\0';
    return result;
}

unsigned int
swap_long( void *l)
{
    unsigned char *b= (unsigned char *) l;
    return b[0] + (b[1]<<8) + (b[2]<<16) + (b[3]<<24);
}

char *
xform_name( char *string, struct qserver *server)
{
    static char _q[1024];
    unsigned char *s= (unsigned char*) string;
    char *q= _q;
    int is_server_name= (string == server->server_name);

    if ( s == NULL)  {
	q[0]= '?';
	q[1]= '\0';
	return _q;
    }

    if ( hex_player_names && !is_server_name)  {
	for ( ; *s; s++, q+= 2)
	    sprintf( q, "%02x", *s);
	*q= '\0';
	return _q;
    }

    if ( server->type->id == Q3_SERVER)  {
	for ( ; *s; s++)  {
	    if ( isprint(*s))  {
		*q++= *s;
		continue;
	    }
	    if ( *s == '\033')
		s++;
	    else if ( *s == 0x80)
		*q++= '(';
	    else if ( *s == 0x81)
		*q++= '=';
	    else if ( *s == 0x82)
		*q++= ')';
	    else if ( *s == 0x10 || *s == 0x90)
		*q++= '[';
	    else if ( *s == 0x11 || *s == 0x91)
		*q++= ']';
	    else if ( *s >= 0x92 && *s <= 0x9a) 
		*q++= *s - 98;
	    else if ( *s >= 0xa0 && *s <= 0xe0)
		*q++= *s - 128;
	    else if ( *s >= 0xe1 && *s <= 0xfa)
		*q++= *s - 160;
	    else if ( *s >= 0xfb && *s <= 0xfe)
		*q++= *s - 128;
	}
        *q= '\0';
    }
    else if ( !is_server_name)  {
	for ( ; *s; s++)  {
	    if ( isprint(*s))  {
		*q++= *s;
		continue;
	    }

	    if ( *s >= 0xa0)
		*q++= *s & 0x7f;
	    else if ( *s >= 0x92 && *s < 0x9c)
		*q++= '0' + (*s - 0x92);
	    else if ( *s >= 0x12 && *s < 0x1c)
		*q++= '0' + (*s - 0x12);
	    else if ( *s == 0x90 || *s == 0x10)
		*q++= '[';
	    else if ( *s == 0x91 || *s == 0x11)
		*q++= ']';
	    else if ( *s == 0xa || *s == 0xc || *s == 0xd)
		*q++= ']';
	}
	*q= '\0';
    }
    else
	strcpy( _q, string);

    return _q;

}

int
is_default_rule( struct rule *rule)
{
    if ( strcmp( rule->name, "sv_maxspeed") == 0)
	return strcmp( rule->value, Q_DEFAULT_SV_MAXSPEED) == 0;
    if ( strcmp( rule->name, "sv_friction") == 0)
	return strcmp( rule->value, Q_DEFAULT_SV_FRICTION) == 0;
    if ( strcmp( rule->name, "sv_gravity") == 0)
	return strcmp( rule->value, Q_DEFAULT_SV_GRAVITY) == 0;
    if ( strcmp( rule->name, "noexit") == 0)
	return strcmp( rule->value, Q_DEFAULT_NOEXIT) == 0;
    if ( strcmp( rule->name, "teamplay") == 0)
	return strcmp( rule->value, Q_DEFAULT_TEAMPLAY) == 0;
    if ( strcmp( rule->name, "timelimit") == 0)
	return strcmp( rule->value, Q_DEFAULT_TIMELIMIT) == 0;
    if ( strcmp( rule->name, "fraglimit") == 0)
	return strcmp( rule->value, Q_DEFAULT_FRAGLIMIT) == 0;
    return 0;
}

char *
strherror( int h_err)
{
    static char msg[100];
    switch (h_err)  {
    case HOST_NOT_FOUND:	return "host not found";
    case TRY_AGAIN:		return "try again";
    case NO_RECOVERY:		return "no recovery";
    case NO_ADDRESS:		return "no address";
    default:	sprintf( msg, "%d", h_err); return msg;
    }
}

int
time_delta( struct timeval *later, struct timeval *past)
{
    if ( later->tv_usec < past->tv_usec)  {
	later->tv_sec--;
	later->tv_usec+= 1000000;
    }
    return (later->tv_sec - past->tv_sec) * 1000 +
	(later->tv_usec - past->tv_usec) / 1000;
}

int
connection_refused()
{
#ifdef unix
    return errno == ECONNREFUSED;
#endif

#ifdef _WIN32
    return WSAGetLastError() == WSAECONNABORTED;
#endif
}

void
set_non_blocking( int fd)
{
    int one= 1;
#ifdef unix
#ifdef O_NONBLOCK
    fcntl( fd, F_SETFL, O_NONBLOCK);
#else
    fcntl( fd, F_SETFL, O_NDELAY);
#endif
#endif

#ifdef _WIN32
    ioctlsocket( fd, FIONBIO, (unsigned long*)&one);
#endif
}

void
print_packet( char *buf, int buflen)
{
    int i;
    for ( i= 0; i < buflen; i++)  {
	if ( buf[i] == ' ') fprintf( stderr, " 20 ");
	else if ( isprint( buf[i])) fprintf( stderr, "%c", buf[i]);
	else fprintf( stderr, " %02x ", (unsigned char)buf[i]);
    }
    fprintf(stderr,"\n");
}

char *
quake_color( int color)
{
    static char *colors[] = {
	"White",	/* 0 */
	"Brown",	/* 1 */
	"Lavender",	/* 2 */
	"Khaki",	/* 3 */
	"Red",		/* 4 */
	"Lt Brown",	/* 5 */
	"Peach",	/* 6 */
	"Lt Peach",	/* 7 */
	"Purple",	/* 8 */
	"Dk Purple",	/* 9 */
	"Tan",		/* 10 */
	"Green",	/* 11 */
	"Yellow",	/* 12 */
	"Blue",		/* 13 */
	"Blue",		/* 14 */
	"Blue"		/* 15 */
    };

    static char *rgb_colors[] = {
	"#ffffff",	/* 0 */
	"#8b4513",	/* 1 */
	"#e6e6fa",	/* 2 */
	"#f0e68c",	/* 3 */
	"#ff0000",	/* 4 */
	"#deb887",	/* 5 */
	"#eecbad",	/* 6 */
	"#ffdab9",	/* 7 */
	"#9370db",	/* 8 */
	"#5d478b",	/* 9 */
	"#d2b48c",	/* 10 */
	"#00ff00",	/* 11 */
	"#ffff00",	/* 12 */
	"#0000ff",	/* 13 */
	"#0000ff",	/* 14 */
	"#0000ff"	/* 15 */
    };

    if ( color_names)  {
        if ( color_names == 1)
	    return colors[color&0xf];
	else
	    return rgb_colors[color&0xf];
    }
    else
	return (char*)color;
}

char *
play_time( int seconds, int show_seconds)
{
    static char time_string[24];
    if ( time_format == CLOCK_TIME)  {
	time_string[0]= '\0';
	if ( seconds/3600)
	    sprintf( time_string, "%2dh", seconds/3600);
	else
	    strcat( time_string, "   ");
	if ( (seconds%3600)/60 || seconds/3600)
	    sprintf( time_string+strlen(time_string), "%2dm",
		(seconds%3600)/60);
	else if ( ! show_seconds)
	    sprintf( time_string+strlen(time_string), " 0m");
	else
	    strcat( time_string, "   ");
	if ( show_seconds)
	    sprintf( time_string+strlen(time_string), "%2ds", seconds%60);
    }
    else if ( time_format == STOPWATCH_TIME)  {
	if ( show_seconds)
	    sprintf( time_string, "%02d:%02d:%02d", seconds/3600,
		(seconds%3600)/60, seconds % 60);
	else
	    sprintf( time_string, "%02d:%02d", seconds/3600,
		(seconds%3600)/60);
    }
    else
	sprintf( time_string, "%d", seconds);

    return time_string;
}

char *
ping_time( int ms)
{
    static char time_string[24];
    if ( ms < 1000)
	sprintf( time_string, "%dms", ms);
    else if ( ms < 1000000)
	sprintf( time_string, "%ds", ms/1000);
    else
	sprintf( time_string, "%dm", ms/1000/60);
    return time_string;
}

int
count_bits( int n)
{
    int b= 0;
    for ( ; n; n>>=1)
	if ( n&1)
	    b++;
    return b;
}


/*
 * Quick sort
 */
void quicksort( struct qserver **foo, int i, int j);
int  qpartition( struct qserver *foo[],int i,int j);

void
qstatsort(struct qserver **array, int size)
{
    quicksort( array, 0, size-1);
}

void
quicksort( struct qserver **array, int i, int j)
{
    int q= 0;
  
    if ( i < j) {
	q = qpartition(array,i,j);
	quicksort(array,i,q);
	quicksort(array,q+1,j);
    }
}


int
server_compare( struct qserver *one, struct qserver *two)
{
    int rc;

    if ( first_sort_key == SORT_GAME)
	if ( (rc= strcmp_withnull( one->game, two->game)) == 0)  {
	    if ( one->n_requests == 0)
		return two->n_requests;
	    else if ( two->n_requests == 0)
		return -1;
	    return one->ping_total/one->n_requests -
			two->ping_total/two->n_requests;
	}
	else
	    return rc;

    if ( first_sort_key == SORT_PING)  {
	if ( one->n_requests == 0)
	    return two->n_requests;
	else if ( two->n_requests == 0)
	    return -1;
	if ( one->ping_total/one->n_requests == two->ping_total/two->n_requests)
	    return strcmp_withnull( one->game, two->game);
	else
	    return one->ping_total/one->n_requests -
			two->ping_total/two->n_requests;
    }
    return 0;
}

int
qpartition(struct qserver **array, int a, int b)
{
    /* this is our comparison point. when we are done
       splitting this array into 2 parts, we want all the
       elements on the left side to be less then or equal
       to this, all the elements on the right side need to
       be greater then or equal to this
    */
    struct qserver *z;

    /* indicies into the array to sort. Used to calculate a partition
       point
    */
    int i = a-1;
    int j = b+1;

    /* temp pointer used to swap two array elements */
    struct qserver * tmp = NULL;

    z = array[a];

    while (1) {

        /* move the right indice over until the value of that array
           elem is less than or equal to z. Stop if we hit the left
           side of the array (ie, j == a);
        */
	do {
	    j--;
	} while( j > a && server_compare(array[j],z) > 0);

        /* move the left indice over until the value of that
           array elem is greater than or equal to z, or until
           we hit the right side of the array (ie i == j)
        */
	do {
	    i++;
	} while( i <= j && server_compare(array[i],z) < 0);

        /* if i is less then j, we need to switch those two array
           elements, if not then we are done partitioning this array
           section
        */
	if(i < j) {
	    tmp = array[i];
	    array[i] = array[j];
	    array[j] = tmp;
	}
	else
	    return j;
    }
}

int
strcmp_withnull( char *one, char *two)
{
    if ( one == NULL && two == NULL)
	return 0;
    if ( one != NULL && two == NULL)
	return -1;
    if ( one == NULL)
	return 1;
    return strcasecmp( one, two);
}

