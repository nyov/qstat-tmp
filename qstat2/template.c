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

#include <stdio.h>
#include <string.h>

#ifdef __hpux
#include <sys/types.h>
#include <netinet/in.h>
#endif
#ifdef _AIX
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#endif

#include "qstat.h"

#ifdef _WIN32
#include <time.h>
#define strcasecmp	stricmp
#define strncasecmp	strnicmp
#endif

#ifdef __hpux
#define STATIC static
#else
#define STATIC
#endif


extern int hostname_lookup;
extern int get_player_info;
extern int get_server_rules;
extern int num_servers_total;
extern int num_servers_timed_out;
extern int num_servers_down;
extern int num_players_total;

static char *server_template;
static char *header_template;
static char *trailer_template;
static char *player_template;

static void display_server_var( struct qserver *server, int var);
static void display_player_var( struct player *player, int var, struct qserver *server);
static void display_generic_var( int var);
static int parse_var( char *varname, int *varlen);
static int read_template( char *filename, char **template);
static void display_string( char *str);


#define VARIABLE_CHAR '$'

static char *variable_option;
static int if_skip;
static int if_level;
static int if_skip_save;
static int if_level_save;
static int html_mode;

struct vardef {
    char *var;
    int varcode;
    int options;
};

#define NO_OPTIONS 0
#define OPTIONS_OK 1
#define EXPR	   2

struct vardef variable_defs[] = {
#define V_HOSTNAME	1
	{"HOSTNAME", V_HOSTNAME, NO_OPTIONS },
#define V_SERVERNAME	2
	{"SERVERNAME", V_SERVERNAME, NO_OPTIONS },
#define V_PING		3
	{"PING", V_PING, NO_OPTIONS },
#define V_PLAYERS	4
	{"PLAYERS", V_PLAYERS, NO_OPTIONS | EXPR },
#define V_MAXPLAYERS	5
	{"MAXPLAYERS", V_MAXPLAYERS, NO_OPTIONS },
#define V_MAP		6
	{"MAP", V_MAP, NO_OPTIONS },
#define V_GAME		7
	{"GAME", V_GAME, NO_OPTIONS | EXPR },
#define V_RETRIES	8
	{"RETRIES", V_RETRIES, NO_OPTIONS },
#define V_IPADDR	9
	{"IPADDR", V_IPADDR, NO_OPTIONS },
#define V_PORT		10
	{"PORT", V_PORT, NO_OPTIONS },
#define V_ARG		11
	{"ARG", V_ARG, NO_OPTIONS },
#define V_QSTATURL	12
	{"QSTATURL", V_QSTATURL, NO_OPTIONS },
#define V_QSTATVERSION	13
	{"QSTATVERSION", V_QSTATVERSION, NO_OPTIONS },
#define V_QSTATAUTHOR	14
	{"QSTATAUTHOR", V_QSTATAUTHOR, NO_OPTIONS },
#define V_QSTATAUTHOREMAIL	15
	{"QSTATAUTHOREMAIL", V_QSTATAUTHOREMAIL, NO_OPTIONS },
#define V_TYPE	16
	{"TYPE", V_TYPE, NO_OPTIONS },
#define V_RULE	17
	{"RULE", V_RULE, OPTIONS_OK | EXPR },
#define V_ALLRULES	18
	{"ALLRULES", V_ALLRULES, NO_OPTIONS },
#define V_PLAYERTEMPLATE	19
	{"PLAYERTEMPLATE", V_PLAYERTEMPLATE, NO_OPTIONS },
#define V_PLAYERNAME	20
	{"PLAYERNAME", V_PLAYERNAME, NO_OPTIONS },
#define V_FRAGS		21
	{"FRAGS", V_FRAGS, NO_OPTIONS },
#define V_PLAYERPING	22
	{"PLAYERPING", V_PLAYERPING, NO_OPTIONS },
#define V_CONNECTTIME	23
	{"CONNECTTIME", V_CONNECTTIME, NO_OPTIONS },
#define V_SKIN		24
	{"SKIN", V_SKIN, NO_OPTIONS },
#define V_SHIRTCOLOR	25
	{"SHIRTCOLOR", V_SHIRTCOLOR, NO_OPTIONS },
#define V_PANTSCOLOR	26
	{"PANTSCOLOR", V_PANTSCOLOR, NO_OPTIONS },
#define V_PLAYERIP	27
	{"PLAYERIP", V_PLAYERIP, NO_OPTIONS },
#define V_IF	28
	{"IF", V_IF, OPTIONS_OK },
#define V_ENDIF	29
	{"ENDIF", V_ENDIF, NO_OPTIONS },
#define V_HTML	35
	{"HTML", V_HTML, NO_OPTIONS },
#define V_FLAG	36
	{"FLAG", V_FLAG, OPTIONS_OK | EXPR },
#define V_UP	37
	{"UP", V_UP, NO_OPTIONS | EXPR },
#define V_DOWN	38
	{"DOWN", V_DOWN, NO_OPTIONS | EXPR },
#define V_IFNOT	39
	{"IFNOT", V_IFNOT, OPTIONS_OK },
#define V_TIMEOUT	40
	{"TIMEOUT", V_TIMEOUT, NO_OPTIONS | EXPR },
#define V_NOW	41
	{"NOW", V_NOW, NO_OPTIONS },
#define V_TOTALSERVERS	42
	{"TOTALSERVERS", V_TOTALSERVERS, NO_OPTIONS },
#define V_TOTALUP	43
	{"TOTALUP", V_TOTALUP, NO_OPTIONS },
#define V_TOTALNOTUP	44
	{"TOTALNOTUP", V_TOTALNOTUP, NO_OPTIONS },
#define V_TOTALPLAYERS	45
	{"TOTALPLAYERS", V_TOTALPLAYERS, NO_OPTIONS },
#define V_TEAMNUM	49
	{"TEAMNUM", V_TEAMNUM, NO_OPTIONS },
#define V_BACKSLASH	50
	{"\\", V_BACKSLASH, NO_OPTIONS },
#define V_HOSTNOTFOUND	51
	{"HOSTNOTFOUND", V_HOSTNOTFOUND, NO_OPTIONS | EXPR },
#define V_MESH	52
	{"MESH", V_MESH, NO_OPTIONS },
#define V_ISEMPTY	56
	{"ISEMPTY", V_ISEMPTY, NO_OPTIONS | EXPR },
#define V_ISFULL	57
	{"ISFULL", V_ISFULL, NO_OPTIONS | EXPR },
#define V_PACKETLOSS	58
	{"PACKETLOSS", V_PACKETLOSS, NO_OPTIONS },
#define V_ISTEAM	59
	{"ISTEAM", V_ISTEAM, NO_OPTIONS | EXPR },
#define V_TEAMNAME	60
	{"TEAMNAME", V_TEAMNAME, NO_OPTIONS },
#define V_DEFAULTTYPE	61
	{"DEFAULTTYPE", V_DEFAULTTYPE, NO_OPTIONS },
#define V_NOWINT	62
	{"NOWINT", V_NOWINT, NO_OPTIONS },
#define V_TYPESTRING	62
	{"TYPESTRING", V_TYPESTRING, NO_OPTIONS },
#define V_FACE		63
	{"FACE", V_FACE, NO_OPTIONS },
#define V_SOLDIEROFFORTUNE		64
	{"SOLDIEROFFORTUNE", V_SOLDIEROFFORTUNE, NO_OPTIONS },
#define V_COLORNUMBERS	65
	{"COLORNUMBERS", V_COLORNUMBERS, NO_OPTIONS },
#define V_COLORNAMES	66
	{"COLORNAMES", V_COLORNAMES, NO_OPTIONS },
#define V_COLORRGB	67
	{"COLORRGB", V_COLORRGB, NO_OPTIONS },
#define V_TIMESECONDS	68
	{"TIMESECONDS", V_TIMESECONDS, NO_OPTIONS },
#define V_TIMECLOCK	69
	{"TIMECLOCK", V_TIMECLOCK, NO_OPTIONS },
#define V_TIMESTOPWATCH	70
	{"TIMESTOPWATCH", V_TIMESTOPWATCH, NO_OPTIONS },
};

int
read_qserver_template( char *filename)
{
    return read_template( filename, &server_template);
}

int
read_header_template( char *filename)
{
    return read_template( filename, &header_template);
}

int
read_trailer_template( char *filename)
{
    return read_template( filename, &trailer_template);
}

int
read_player_template( char *filename)
{
    return read_template( filename, &player_template);
}

STATIC int
read_template( char *filename, char **template)
{
    FILE *file;
    int length, rc;

    file= fopen( filename, "r");
    if ( file == NULL)  {
	perror( filename);
	return -1;
    }

    fseek( file, 0, SEEK_END);
    length= ftell( file);
    fseek( file, 0, SEEK_SET);

    *template= (char*)malloc( length+1);
    rc= fread( *template, 1, length, file);
    if ( rc == 0 && length > 0)  {
	perror( filename);
	fclose( file);
	free( *template);
	*template= NULL;
	return -1;
    }
    (*template)[length]= '\0';
    return 0;
}

int
have_server_template()
{
    return server_template != NULL;
}

int
have_header_template()
{
    return header_template != NULL;
}

int
have_trailer_template()
{
    return trailer_template != NULL;
}

void
template_display_server( struct qserver *server)
{
    char *t= server_template;
    int var, varlen;

    if_level= 0;
    if_skip= 0;
    for ( ; *t; t++)  {
	if ( *t != VARIABLE_CHAR)  {
	    if ( ! if_skip)
		putchar( *t);
	    continue;
	}
	var= parse_var( t, &varlen);
	if ( var == -1)  {
	    if ( ! if_skip)
		putchar( VARIABLE_CHAR);
	    continue;
	}
	if ( var == V_BACKSLASH)  {
	    t+= 2;
	    if ( *t == '\r')  {
		if ( *++t == '\n')
		    t++;
	    }
	    else if ( *t == '\n')
		t++;
	    t--;
	    continue;
	}
	if ( (var == V_IF || var == V_IFNOT) && variable_option != NULL)  {
	    int truth= (var==V_IF)?1:0;
	    if ( !if_skip && is_true( server, NULL, variable_option) == truth)
		if_level++;
	    else
		if_skip++;
	}
	else if ( var == V_ENDIF)  {
	    if ( if_skip)
		if_skip--;
	    else if ( if_level)
		if_level--;
	}
	if ( ! if_skip)
	    display_server_var( server, var);
	t+= varlen;
    }
}

void
template_display_players( struct qserver *server)
{
    struct player *player;
    for ( player= server->players; player != NULL; player= player->next)
	template_display_player( server, player);
}

void
template_display_player( struct qserver *server, struct player *player)
{
    char *t= player_template;
    int var, varlen;

    if ( player_template == NULL)
	return;

    if_level= 0;
    if_skip= 0;
    for ( ; *t; t++)  {
	if ( *t != VARIABLE_CHAR)  {
	    if ( ! if_skip)
		putchar( *t);
	    continue;
	}
	var= parse_var( t, &varlen);
	if ( var == -1)  {
	    if ( ! if_skip)
		putchar( VARIABLE_CHAR);
	    continue;
	}
	if ( var == V_BACKSLASH)  {
	    t+= 2;
	    if ( *t == '\r')  {
		if ( *++t == '\n')
		    t++;
	    }
	    else if ( *t == '\n')
		t++;
	    t--;
	    continue;
	}
	if ( (var == V_IF || var == V_IFNOT) && variable_option != NULL)  {
	    int truth= (var==V_IF)?1:0;
	    if ( !if_skip && is_true( server, player, variable_option) == truth)
		if_level++;
	    else
		if_skip++;
	}
	else if ( var == V_ENDIF)  {
	    if ( if_skip)
		if_skip--;
	    else if ( if_level)
		if_level--;
	}
	if ( ! if_skip)
	    display_player_var( player, var, server);
	t+= varlen;
    }
}

void
template_display_header()
{
    char *t= header_template;
    int var, varlen;

    if_level= 0;
    if_skip= 0;
    for ( ; *t; t++)  {
	if ( *t != VARIABLE_CHAR)  {
	    putchar( *t);
	    continue;
	}
	var= parse_var( t, &varlen);
	if ( var == -1)  {
	    putchar( VARIABLE_CHAR);
	    continue;
	}
	if ( var == V_BACKSLASH)  {
	    t+= 2;
	    if ( *t == '\r')  {
		if ( *++t == '\n')
		    t++;
	    }
	    else if ( *t == '\n')
		t++;
	    t--;
	    continue;
	}
	display_generic_var( var);
	t+= varlen;
    }
}

void
template_display_trailer()
{
    char *t= trailer_template;
    int var, varlen;

    if_level= 0;
    if_skip= 0;
    for ( ; *t; t++)  {
	if ( *t != VARIABLE_CHAR)  {
	    putchar( *t);
	    continue;
	}
	var= parse_var( t, &varlen);
	if ( var == -1)  {
	    putchar( VARIABLE_CHAR);
	    continue;
	}
	if ( var == V_BACKSLASH)  {
	    t+= 2;
	    if ( *t == '\r')  {
		if ( *++t == '\n')
		    t++;
	    }
	    else if ( *t == '\n')
		t++;
	    t--;
	    continue;
	}
	display_generic_var( var);
	t+= varlen;
    }
}

STATIC void
display_server_var( struct qserver *server, int var)
{
    char *game;
    int full_data= 1;
    if ( server->server_name == DOWN || server->server_name == TIMEOUT ||
		server->server_name == SYSERROR || 
		server->error != NULL || server->type->master)
	full_data= 0;

    switch( var)  {
    case V_HOSTNAME:
	fputs( (hostname_lookup) ? server->host_name : server->arg, stdout);
	break;
    case V_SERVERNAME:
	display_string( xform_name( server->server_name, server));
	break;
    case V_PING:
	if ( server->server_name != TIMEOUT && server->server_name != DOWN &&
		server->server_name != HOSTNOTFOUND)
	    fprintf( stdout, "%d", server->ping_total/server->n_requests);
	break;
    case V_PLAYERS:
	if ( full_data)
	    fprintf( stdout, "%d", server->num_players);
	break;
    case V_MAXPLAYERS:
	if ( full_data)
	    fprintf( stdout, "%d", server->max_players);
	break;
    case V_MAP:
	if ( full_data)
	    fputs( (server->map_name) ? server->map_name : "?", stdout);
	break;
    case V_GAME:
	if ( full_data)  {
	    game= get_qw_game( server);
	    fputs( (game) ? game : "", stdout);
	}
	break;
    case V_RETRIES:
	if ( server->server_name != TIMEOUT && server->server_name != DOWN &&
		server->server_name != HOSTNOTFOUND)
	    fprintf( stdout, "%d", server->n_retries);
	break;
    case V_IPADDR:
	{ unsigned long ipaddr= ntohl(server->ipaddr);
	fprintf( stdout, "%u.%u.%u.%u", (ipaddr>>24)&0xff,
		(ipaddr>>16)&0xff, (ipaddr>>8)&0xff, ipaddr&0xff);
	}
	break;
    case V_PORT:
	fprintf( stdout, "%hu", server->port);
	break;
    case V_ARG:
	fputs( server->arg, stdout);
	break;
    case V_TYPE:
	fputs( server->type->game_name, stdout);
	break;
    case V_TYPESTRING:
	fputs( server->type->type_string, stdout);
	break;
    case V_RULE:
	{
	struct rule *rule;
	if ( variable_option == NULL)
	    break;
	for ( rule= server->rules; rule != NULL; rule= rule->next)
	    if ( strcasecmp( rule->name, variable_option) == 0)
		display_string( rule->value);
	}
	break;
    case V_ALLRULES:
	{
	struct rule *rule;
	for ( rule= server->rules; rule != NULL; rule= rule->next)  {
	    if ( rule != server->rules)
		fputs( ", ", stdout);
	    display_string( rule->name);
	    putchar( '=');
	    display_string( rule->value);
	}
	}
	break;
    case V_PLAYERTEMPLATE:
	if_level_save= if_level;
	if_skip_save= if_skip;
	template_display_players( server);
	if_level= if_level_save;
	if_skip= if_skip_save;
	break;
    default:
	display_generic_var( var);
    }
}

STATIC void
display_player_var( struct player *player, int var, struct qserver *server)
{
    switch( var)  {
    case V_PLAYERNAME:
	display_string( xform_name( player->name, server));
	break;
    case V_FRAGS:
	fprintf( stdout, "%d", player->frags);
	break;
    case V_PLAYERPING:
	fprintf( stdout, "%d", player->ping);
	break;
    case V_CONNECTTIME:
	fputs( play_time(player->connect_time,0), stdout);
	break;
    case V_SKIN:
	display_string( player->skin);
	break;
    case V_MESH:
	display_string( player->mesh);
	break;
    case V_FACE:
	display_string( player->face);
	break;
    case V_SHIRTCOLOR:
	fputs( quake_color(player->shirt_color), stdout);
	break;
    case V_PANTSCOLOR:
	fputs( quake_color(player->pants_color), stdout);
	break;
    case V_PLAYERIP:
	if ( player->address)
	    fputs( player->address, stdout);
	break;
    case V_TEAMNUM:
	fprintf( stdout, "%d", player->team);
	break;
    case V_PACKETLOSS:
	fprintf( stdout, "%d", player->packet_loss);
	break;
    case V_TEAMNAME:
	if ( player->team_name)
	    fprintf( stdout, "%s", player->team_name);
	break;
    case V_COLORNUMBERS:
	color_names= 0;
	break;
    case V_COLORNAMES:
	color_names= 1;
	break;
    case V_COLORRGB:
	color_names= 2;
	break;
    case V_TIMESECONDS:
	time_format= SECONDS;
	break;
    case V_TIMECLOCK:
	time_format= CLOCK_TIME;
	break;
    case V_TIMESTOPWATCH:
	time_format= STOPWATCH_TIME;
	break;
    default:
	display_generic_var( var);
    }
}

STATIC void
display_generic_var( int var)
{
    switch( var)  {
    case V_QSTATURL:
	fputs( "http://www.activesw.com/people/steve/qstat.html", stdout);
	break;
    case V_QSTATVERSION:
	fputs( qstat_version, stdout);
	break;
    case V_QSTATAUTHOR:
	fputs( "Steve Jankowski", stdout);
	break;
    case V_QSTATAUTHOREMAIL:
	fputs( "steve@activesw.com", stdout);
	break;
    case V_HTML:
	html_mode^= 1;
	break;
    case V_NOW:  {
	time_t now= time(0);
	char *now_string= ctime(&now);
	now_string[strlen(now_string)-1]= '\0';
	fputs( now_string, stdout);
	break;
	}
    case V_NOWINT:
	fprintf( stdout, "%u", time(0));
	break;
    case V_TOTALSERVERS:
	fprintf( stdout, "%d", num_servers_total);
	break;
    case V_TOTALUP:
	fprintf( stdout, "%d", num_servers_total - num_servers_timed_out -
		num_servers_down);
	break;
    case V_TOTALNOTUP:
	fprintf( stdout, "%d", num_servers_timed_out + num_servers_down);
	break;
    case V_TOTALPLAYERS:
	fprintf( stdout, "%d", num_players_total);
	break;
    case V_DEFAULTTYPE:
	fprintf( stdout, "%s", default_server_type->game_name);
	break;
    default: break;
    }
}

STATIC int
parse_var( char *varname, int *varlen)
{
    char *v= ++varname, *colon= NULL;
    int i, quote= 0;

    if ( variable_option != NULL)  {
	free( variable_option);
	variable_option= NULL;
    }

    if ( *v == '(')  {
	v++;
	varname++;
	quote++;
    }
    else if ( *v == '\\')  {
	*varlen= 1;
	return V_BACKSLASH;
    }

    for ( ; *v; v++)
	if ( (!quote && !isalpha( *v)) || (quote && (*v == ')' || *v == ':')))
	    break;
    if ( v-varname == 0)
	return -1;

    *varlen= v-varname;
    if ( *v == ':')
	colon= v;
    else if ( quote && *v == ')')
	v++;

    for ( i= 0; i < sizeof(variable_defs)/sizeof(struct vardef); i++)
	if ( strncasecmp( varname, variable_defs[i].var, *varlen) == 0)  {
	    if ( colon != NULL && ((variable_defs[i].options & OPTIONS_OK) || quote))  {
		for ( v++; *v; v++)  {
		    if ( (!quote && !isalpha( *v) && *v != '*' && *v != '_') ||
			(quote==1 && *v == ')')) break;
		    if ( *v == '(')
			quote++;
		    else if ( *v == ')')
			quote--;
		}
		variable_option= (char*)malloc( v-colon+1);
		strncpy( variable_option, colon+1, v-colon-1);
		variable_option[v-colon-1]= '\0';
		if ( quote && *v == ')')
		    v++;
	    }
	    *varlen= v-varname+quote;
	    return variable_defs[i].varcode;
	}

    return -1;
}

int
is_true( struct qserver *server, struct player *player, char *expr)
{
    int i, len, arglen;
    char *arg= NULL, *lparen, *rparen;
    server_type *t;

    if ( (lparen= strchr( expr, '(')) != NULL)  {
	if ( (rparen= strchr( lparen, ')')) != NULL)  {
	    len= lparen - expr;
	    arg= lparen + 1;
	    arglen= rparen - lparen - 1;
	}
    }
    else
	len= strlen( expr);

    for ( i= 0; i < sizeof(variable_defs)/sizeof(struct vardef); i++)
	if ( strncasecmp( expr, variable_defs[i].var, len) == 0)  {
	    if ( !(variable_defs[i].options & EXPR))  {
		fprintf( stderr, "unsupported IF expression \"%s\"\n", expr);
		return 1;
	    }
	    switch ( variable_defs[i].varcode)  {
	    case V_GAME:  {
		char *g= get_qw_game( server);
		if ( g == NULL || *g == '\0')
		    return 0;
		else
		    return 1;
	    }
	    case V_PLAYERS: return server->num_players > 0;
	    case V_ISEMPTY: return server->num_players == 0;
	    case V_ISFULL: return server->max_players ? server->num_players >= server->max_players : 0;
	    case V_ISTEAM: return player ? player->number == TRIBES_TEAM : 0;
	    case V_RULE:  {
		struct rule *rule;
		if ( arg == NULL)
		    return 0;
		for ( rule= server->rules; rule != NULL; rule= rule->next)
		    if ( strncasecmp( rule->name, arg, arglen) == 0 &&
				strlen( rule->name) == arglen)
			return 1;
		return 0;
	    }
	    case V_FLAG:  {
		if ( strncmp( "-H", arg, arglen) == 0)
		    return hostname_lookup;
		if ( strncmp( "-P", arg, arglen) == 0)
		    return get_player_info;
		if ( strncmp( "-R", arg, arglen) == 0)
		    return get_server_rules;
		return 0;
	    }
	    case V_UP:
		return server->server_name != DOWN &&
			server->server_name != TIMEOUT &&
			server->server_name != HOSTNOTFOUND;
	    case V_DOWN:
		return server->server_name == DOWN;
	    case V_TIMEOUT:
		return server->server_name == TIMEOUT;
	    case V_HOSTNOTFOUND:
		return server->server_name == HOSTNOTFOUND;
	    default: return 0;
	    }
	}

    t= &types[0];
    for ( ; t->id; t++)
	if ( strncasecmp( expr, t->template_var, len) == 0)
	    return server->type->id == t->id;

    fprintf( stderr, "bad IF expression \"%s\"\n", expr);
    return 0;
}

STATIC void
display_string( char *str)
{
    if ( str == NULL)
	return;
    if ( ! html_mode)  {
	fputs( str, stdout);
	return;
    }

    for ( ; *str; str++)
	switch ( *str) {
	case '<': fputs( "&lt;", stdout); break;
	case '>': fputs( "&gt;", stdout); break;
	case '&': fputs( "&amp;", stdout); break;
	default: putchar( *str);
	}
}

