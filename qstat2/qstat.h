/*
 * qstat.h
 * by Steve Jankowski
 * steve@webmethods.com
 * http://www.activesw.com/people/steve/qstat.html
 *
 * Copyright 1996,1997,1998,1999 by Steve Jankowski
 */

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
#include <sys/time.h>
#endif
#ifdef _WIN32
#include <winsock.h>
#endif

/* Various magic numbers.
 */

#define Q_DEFAULT_PORT			26000
#define HEXEN2_DEFAULT_PORT		26900
#define Q2_DEFAULT_PORT			27910
#define Q3_DEFAULT_PORT			27960
#define Q2_MASTER_DEFAULT_PORT		27900
#define Q3_MASTER_DEFAULT_PORT		27950
#define QW_DEFAULT_PORT			27500
#define QW_MASTER_DEFAULT_PORT		27000
#define HW_DEFAULT_PORT			26950
#define UNREAL_DEFAULT_PORT		7777
#define UNREAL_MASTER_DEFAULT_PORT	28900
#define HALFLIFE_DEFAULT_PORT		27015
#define HL_MASTER_DEFAULT_PORT		27010
#define SIN_DEFAULT_PORT		22450
#define SHOGO_DEFAULT_PORT		27888
#define TRIBES_DEFAULT_PORT		28001
#define TRIBES_MASTER_DEFAULT_PORT	28000
#define BFRIS_DEFAULT_PORT		44001
#define KINGPIN_DEFAULT_PORT		31510
#define HERETIC2_DEFAULT_PORT		28910
#define SOF_DEFAULT_PORT		28910
#define GAMESPY_MASTER_DEFAULT_PORT	28900

#define Q_UNKNOWN_TYPE 0
#define MASTER_SERVER 0x40000000

#define Q_SERVER 1
#define QW_SERVER (1<<1)
#define QW_MASTER ((1<<2)|MASTER_SERVER)
#define H2_SERVER (1<<3)
#define Q2_SERVER (1<<4)
#define Q2_MASTER ((1<<5)|MASTER_SERVER)
#define HW_SERVER (1<<6)
#define UN_SERVER (1<<7)
#define UN_MASTER ((1<<8)|MASTER_SERVER)
#define HL_SERVER (1<<9)
#define SIN_SERVER (1<<11)
#define SHOGO_SERVER (1<<12)
#define HL_MASTER ((1<<13)|MASTER_SERVER)
#define TRIBES_SERVER (1<<14)
#define TRIBES_MASTER ((1<<15)|MASTER_SERVER)
#define Q3_SERVER (1<<16) 
#define Q3_MASTER ((1<<17)|MASTER_SERVER)
#define BFRIS_SERVER (1<<18)
#define KINGPIN_SERVER (1<<19)
#define HERETIC2_SERVER (1<<20)
#define SOF_SERVER (1<<21)
#define GAMESPY_MASTER ((1<<22)|MASTER_SERVER)
#define GAMESPY_PROTOCOL_SERVER (1<<23)

#define TF_SINGLE_QUERY		1
#define TF_OUTFILE		2
#define TF_MASTER_MULTI_RESPONSE	4
#define TF_TCP_CONNECT		8
#define TF_QUERY_ARG		16
#define TF_QUERY_ARG_REQUIRED	32

#define TRIBES_TEAM	-1

struct qserver;
struct q_packet;

typedef void (*DisplayFunc)( struct qserver *);
typedef void (*QueryFunc)( struct qserver *);
typedef void (*PacketFunc)( struct qserver *, char *rawpkt, int pktlen);

/* Output and formatting functions
 */
 
void display_server( struct qserver *server);
void display_qwmaster( struct qserver *server);
void display_server_rules( struct qserver *server);
void display_player_info( struct qserver *server);
void display_q_player_info( struct qserver *server);
void display_qw_player_info( struct qserver *server);
void display_q2_player_info( struct qserver *server);
void display_unreal_player_info( struct qserver *server);
void display_shogo_player_info( struct qserver *server);
void display_halflife_player_info( struct qserver *server);
void display_tribes_player_info( struct qserver *server);
void display_bfris_player_info( struct qserver *server);
 
void raw_display_server( struct qserver *server);
void raw_display_server_rules( struct qserver *server);
void raw_display_player_info( struct qserver *server);
void raw_display_q_player_info( struct qserver *server);
void raw_display_qw_player_info( struct qserver *server);
void raw_display_q2_player_info( struct qserver *server);
void raw_display_unreal_player_info( struct qserver *server);
void raw_display_halflife_player_info( struct qserver *server);
void raw_display_tribes_player_info( struct qserver *server);
void raw_display_bfris_player_info( struct qserver *server);
 
void send_server_request_packet( struct qserver *server);
void send_qserver_request_packet( struct qserver *server);
void send_qwserver_request_packet( struct qserver *server);
void send_unreal_request_packet( struct qserver *server);
void send_tribes_request_packet( struct qserver *server);
void send_qwmaster_request_packet( struct qserver *server);
void send_bfris_request_packet( struct qserver *server);
void send_player_request_packet( struct qserver *server);
void send_rule_request_packet( struct qserver *server);
void send_gamespy_master_request( struct qserver *server);

void deal_with_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_q_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_qw_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_q1qw_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_q2_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_qwmaster_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_unreal_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_tribes_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_tribesmaster_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_bfris_packet( struct qserver *server, char *pkt, int pktlen);
void deal_with_gamespy_master_response( struct qserver *server, char *pkt, int pktlen);

typedef struct _server_type  {
    int id;
    char *type_prefix;
    char *type_string;
    char *type_option;
    char *game_name;
    int master;
    unsigned short default_port;
    int flags;
    char *game_rule;
    char *template_var;
    char *status_packet;
    int status_len;
    char *player_packet;
    int player_len;
    char *rule_packet;
    int rule_len;
    char *master_packet;
    int master_len;
    DisplayFunc display_player_func;
    DisplayFunc display_rule_func;
    DisplayFunc display_raw_player_func;
    DisplayFunc display_raw_rule_func;
    QueryFunc status_query_func;
    QueryFunc player_query_func;
    QueryFunc rule_query_func;
    PacketFunc packet_func;
} server_type;

extern server_type types[];
extern server_type* default_server_type;

#ifdef QUERY_PACKETS
#undef QUERY_PACKETS

/* QUAKE */
struct q_packet  {
    unsigned char flag1;
    unsigned char flag2;
    unsigned short length;
    unsigned char op_code;
    unsigned char data[19];
};
#define Q_HEADER_LEN	5

/*
struct {
    unsigned char flag1;
    unsigned char flag2;
    unsigned short length;
    unsigned char op_code;
    char name[6];
    unsigned char version;
};
*/

#define Q_FLAG1			0x80
#define Q_FLAG2			0x00
#define Q_CCREQ_SERVER_INFO	0x02
#define Q_CCREQ_PLAYER_INFO	0x03
#define Q_CCREQ_RULE_INFO	0x04

/* The \003 below is the protocol version */
#define Q_SERVERINFO_LEN	12
struct q_packet q_serverinfo =
{ Q_FLAG1, Q_FLAG2, Q_SERVERINFO_LEN, Q_CCREQ_SERVER_INFO, "QUAKE\000\003" };

struct q_packet q_rule = {Q_FLAG1,Q_FLAG2, 0, Q_CCREQ_RULE_INFO, ""};
struct q_packet q_player = {Q_FLAG1,Q_FLAG2, 6, Q_CCREQ_PLAYER_INFO, ""};

/* QUAKE WORLD */
struct {
    char prefix[4];
    char command[7];
} qw_serverstatus =
{ '\377', '\377', '\377', '\377', 's', 't', 'a', 't', 'u', 's', '\n' };

/* QUAKE3 */
struct {
    char prefix[4];
    char command[10];
} q3_serverstatus =
{ '\377', '\377', '\377', '\377', 'g', 'e', 't', 's', 't', 'a', 't', 'u', 's', '\n' };

/* HEXEN WORLD */
struct {
    char prefix[5];
    char command[7];
} hw_serverstatus =
{ '\377', '\377', '\377', '\377', '\377', 's', 't', 'a', 't', 'u', 's', '\n' };

/* HEXEN 2 */
/* The \004 below is the protocol version */
#define H2_SERVERINFO_LEN	14
struct q_packet h2_serverinfo =
{ Q_FLAG1, Q_FLAG2, H2_SERVERINFO_LEN, Q_CCREQ_SERVER_INFO, "HEXENII\000\004" };

/* UNREAL */
char unreal_serverstatus[8] = { '\\', 's','t','a','t','u','s', '\\' };
char unreal_masterlist[22] = "\\list\\\\gamename\\unreal";

/* HALF LIFE */
char hl_ping[9] =
{ '\377', '\377', '\377', '\377', 'p', 'i', 'n', 'g', '\0' };
char hl_rules[10] =
{ '\377', '\377', '\377', '\377', 'r', 'u', 'l', 'e', 's', '\0' };
char hl_info[9] =
{ '\377', '\377', '\377', '\377', 'i', 'n', 'f', 'o', '\0' };
char hl_players[12] =
{ '\377', '\377', '\377', '\377', 'p', 'l', 'a', 'y', 'e', 'r', 's', '\0' };
char hl_details[12] =
{ '\377', '\377', '\377', '\377', 'd', 'e', 't', 'a', 'i', 'l', 's', '\0' };

/* QUAKE WORLD MASTER */
#define QW_GET_SERVERS    'c'
char qw_masterquery[] = { QW_GET_SERVERS, '\n', '\0' };

/* QUAKE 2 MASTER */
char q2_masterquery[] = { 'q', 'u', 'e', 'r', 'y', '\n', '\0' };

/* QUAKE 3 MASTER */
char *q3_masterquery = "\377\377\377\377getservers %s empty full demo\n";
char q3_masterquery_buf[80];
char *q3_masterquery_default_version= "45";

/* HALF-LIFE MASTER */
char hl_masterquery[4] = { 'e', '\0', '\0', '\0' };

/* TRIBES */
char tribes_info[] = { '`', '*', '*' };
char tribes_players[] = { 'b', '*', '*' };
/*  This is what the game sends to get minimal status
{ '\020', '\03', '\377', 0, (unsigned char)0xc5, 6 };
*/
char tribes_info_reponse[] = { 'a', '*', '*', 'b' };
char tribes_players_reponse[] = { 'c', '*', '*', 'b' };
char tribes_masterquery[] = { 0x10, 0x3, '\377', 0, 0x2 };
char tribes_master_response[] = { 0x10, 0x6 };

/* GAMESPY */
char gamespy_master_request_prefix[] = "\\list\\\\gamename\\";
char gamespy_master_validate[] = "\\gamename\\gamespy2\\gamever\\020109017\\location\\5\\validate\\12345678\\final\\";

server_type types[] = { 
{
    /* QUAKE */
    Q_SERVER,			/* id */
    "QS",			/* type_prefix */
    "qs",			/* type_string */
    "-qs",			/* type_option */
    "Quake",			/* game_name */
    0,				/* master */
    Q_DEFAULT_PORT,		/* default_port */
    0,				/* flags */
    "*gamedir",			/* game_rule */
    "QUAKE",			/* template_var */
    (char*) &q_serverinfo,	/* status_packet */
    Q_SERVERINFO_LEN,		/* status_len */
    (char*) &q_player,		/* player_packet */
    Q_HEADER_LEN+1,		/* player_len */
    (char*) &q_rule,		/* rule_packet */
    sizeof( q_rule),		/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qserver_request_packet,/* status_query_func */
    send_player_request_packet,	/* rule_query_func */
    send_rule_request_packet,	/* player_query_func */
    deal_with_q_packet,		/* packet_func */
},
{
    /* HEXEN 2 */
    H2_SERVER,			/* id */
    "H2S",			/* type_prefix */
    "h2s",			/* type_string */
    "-h2s",			/* type_option */
    "Hexen II",			/* game_name */
    0,				/* master */
    HEXEN2_DEFAULT_PORT,	/* default_port */
    0,				/* flags */
    "*gamedir",			/* game_rule */
    "HEXEN2",			/* template_var */
    (char*) &h2_serverinfo,	/* status_packet */
    H2_SERVERINFO_LEN,		/* status_len */
    (char*) &q_player,		/* player_packet */
    Q_HEADER_LEN+1,		/* player_len */
    (char*) &q_rule,		/* rule_packet */
    sizeof( q_rule),		/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qserver_request_packet,/* status_query_func */
    send_player_request_packet,	/* rule_query_func */
    send_rule_request_packet,	/* player_query_func */
    deal_with_q_packet,		/* packet_func */
},
{
    /* QUAKE WORLD */
    QW_SERVER,			/* id */
    "QWS",			/* type_prefix */
    "qws",			/* type_string */
    "-qws",			/* type_option */
    "QuakeWorld",		/* game_name */
    0,				/* master */
    QW_DEFAULT_PORT,		/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "*gamedir",			/* game_rule */
    "QUAKEWORLD",		/* template_var */
    (char*) &qw_serverstatus,	/* status_packet */
    sizeof( qw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_qw_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_qw_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* HEXEN WORLD */
    HW_SERVER,			/* id */
    "HWS",			/* type_prefix */
    "hws",			/* type_string */
    "-hws",			/* type_option */
    "HexenWorld",		/* game_name */
    0,				/* master */
    HW_DEFAULT_PORT,		/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "*gamedir",			/* game_rule */
    "HEXENWORLD",		/* template_var */
    (char*) &hw_serverstatus,	/* status_packet */
    sizeof( hw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_qw_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_qw_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* QUAKE 2 */
    Q2_SERVER,			/* id */
    "Q2S",			/* type_prefix */
    "q2s",			/* type_string */
    "-q2s",			/* type_option */
    "Quake II",			/* game_name */
    0,				/* master */
    Q2_DEFAULT_PORT,		/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gamedir",			/* game_rule */
    "QUAKE2",			/* template_var */
    (char*) &qw_serverstatus,	/* status_packet */
    sizeof( qw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* QUAKE 3 */
    Q3_SERVER,			/* id */
    "Q3S",			/* type_prefix */
    "q3s",			/* type_string */
    "-q3s",			/* type_option */
    "Quake III: Arena",		/* game_name */
    0,				/* master */
    Q3_DEFAULT_PORT,		/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gamename",			/* game_rule */
    "QUAKE3",			/* template_var */
    (char*) &q3_serverstatus,	/* status_packet */
    sizeof( q3_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* UNREAL */
    UN_SERVER,			/* id */
    "UNS",			/* type_prefix */
    "uns",			/* type_string */
    "-uns",			/* type_option */
    "Unreal",			/* game_name */
    0,				/* master */
    UNREAL_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gametype",			/* game_rule */
    "UNREAL",			/* template_var */
    (char*) &unreal_serverstatus,	/* status_packet */
    sizeof( unreal_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_unreal_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_unreal_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_unreal_request_packet,	/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_unreal_packet,	/* packet_func */
},
{
    /* HALF LIFE */
    HL_SERVER,			/* id */
    "HLS",			/* type_prefix */
    "hls",			/* type_string */
    "-hls",			/* type_option */
    "Half-Life",		/* game_name */
    0,				/* master */
    HALFLIFE_DEFAULT_PORT,	/* default_port */
    0,				/* flags */
    "game",			/* game_rule */
    "HALFLIFE",			/* template_var */
    (char*) &hl_details,		/* status_packet */
    sizeof( hl_details),		/* status_len */
    (char*) &hl_players,	/* player_packet */
    sizeof( hl_players),	/* player_len */
    (char*) &hl_rules,		/* rule_packet */
    sizeof( hl_rules),		/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_halflife_player_info,/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_halflife_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    send_rule_request_packet,	/* rule_query_func */
    send_player_request_packet,	/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* SIN */
    SIN_SERVER,			/* id */
    "SNS",			/* type_prefix */
    "sns",			/* type_string */
    "-sns",			/* type_option */
    "Sin",			/* game_name */
    0,				/* master */
    SIN_DEFAULT_PORT,		/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gamedir",			/* game_rule */
    "SIN",			/* template_var */
    (char*) &qw_serverstatus,	/* status_packet */
    sizeof( qw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* SHOGO */
    SHOGO_SERVER,		/* id */
    "SGS",			/* type_prefix */
    "sgs",			/* type_string */
    "-sgs",			/* type_option */
    "Shogo: Mobile Armor Division",	/* game_name */
    0,				/* master */
    SHOGO_DEFAULT_PORT,		/* default_port */
    0,				/* flags */
    "",				/* game_rule */
    "SHOGO",			/* template_var */
    (char*) &unreal_serverstatus,	/* status_packet */
    sizeof( unreal_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_unreal_request_packet,	/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_unreal_packet,	/* packet_func */
},
{
    /* TRIBES */
    TRIBES_SERVER,		/* id */
    "TBS",			/* type_prefix */
    "tbs",			/* type_string */
    "-tbs",			/* type_option */
    "Tribes",			/* game_name */
    0,				/* master */
    TRIBES_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "game",			/* game_rule */
    "TRIBES",			/* template_var */
    (char*) &tribes_info,	/* status_packet */
    sizeof( tribes_info),	/* status_len */
    (char*) &tribes_players,	/* player_packet */
    sizeof( tribes_players),	/* player_len */
    (char*) &tribes_players,	/* rule_packet */
    sizeof( tribes_players),	/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_tribes_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_tribes_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_tribes_request_packet,	/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_tribes_packet,	/* packet_func */
},
{
    /* BFRIS */
    BFRIS_SERVER,		/* id */
    "BFS",			/* type_prefix */
    "bfs",			/* type_string */
    "-bfs",			/* type_option */
    "BFRIS",			/* game_name */
    0,				/* master */
    BFRIS_DEFAULT_PORT,		/* default_port */
    0,				/* flags */
    "Rules",			/* game_rule */
    "BFRIS",			/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_bfris_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_bfris_player_info,/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_bfris_request_packet,	/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_bfris_packet,	/* packet_func */
},
{
    /* KINGPIN */
    KINGPIN_SERVER,		/* id */
    "KPS",			/* type_prefix */
    "kps",			/* type_string */
    "-kps",			/* type_option */
    "Kingpin",			/* game_name */
    0,				/* master */
    KINGPIN_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gamedir",			/* game_rule */
    "KINGPIN",			/* template_var */
    (char*) &qw_serverstatus,	/* status_packet */
    sizeof( qw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* HERETIC II */
    HERETIC2_SERVER,		/* id */
    "HRS",			/* type_prefix */
    "hrs",			/* type_string */
    "-hrs",			/* type_option */
    "Heretic II",		/* game_name */
    0,				/* master */
    HERETIC2_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gamedir",			/* game_rule */
    "HERETIC2",			/* template_var */
    (char*) &qw_serverstatus,	/* status_packet */
    sizeof( qw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* SOLDIER OF FORTUNE */
    SOF_SERVER,			/* id */
    "SFS",			/* type_prefix */
    "sfs",			/* type_string */
    "-sfs",			/* type_option */
    "Soldier of Fortune",	/* game_name */
    0,				/* master */
    SOF_DEFAULT_PORT,		/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gamedir",			/* game_rule */
    "SOLDIEROFFORTUNE",		/* template_var */
    (char*) &qw_serverstatus,	/* status_packet */
    sizeof( qw_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_q2_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_q2_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_qwserver_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qw_packet,	/* packet_func */
},
{
    /* GAMESPY PROTOCOL */
    GAMESPY_PROTOCOL_SERVER,	/* id */
    "GPS",			/* type_prefix */
    "gps",			/* type_string */
    "-gps",			/* type_option */
    "Gamespy Protocol",		/* game_name */
    0,				/* master */
    0,				/* default_port */
    TF_SINGLE_QUERY,		/* flags */
    "gametype",			/* game_rule */
    "GAMESPYPROTOCOL",		/* template_var */
    (char*) &unreal_serverstatus,	/* status_packet */
    sizeof( unreal_serverstatus),	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    NULL,			/* master_packet */
    0,				/* master_len */
    display_unreal_player_info,	/* display_player_func */
    display_server_rules,	/* display_rule_func */
    raw_display_unreal_player_info,	/* display_raw_player_func */
    raw_display_server_rules,	/* display_raw_rule_func */
    send_unreal_request_packet,	/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_unreal_packet,	/* packet_func */
},


/* --- MASTER SERVERS --- */
{
    /* QUAKE WORLD MASTER */
    QW_MASTER,			/* id */
    "QWM",			/* type_prefix */
    "qwm",			/* type_string */
    "-qwm",			/* type_option */ /* ## also "-qw" */
    "QuakeWorld Master",	/* game_name */
    QW_SERVER,			/* master */
    QW_MASTER_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY|TF_OUTFILE,	/* flags */
    "",				/* game_rule */
    "QWMASTER",			/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    (char*) &qw_masterquery,	/* master_packet */
    sizeof( qw_masterquery),	/* master_len */
    display_qwmaster,		/* display_player_func */
    NULL,	/* display_rule_func */
    NULL,	/* display_raw_player_func */
    NULL,	/* display_raw_rule_func */
    send_qwmaster_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qwmaster_packet,	/* packet_func */
},
{
    /* QUAKE 2 MASTER */
    Q2_MASTER,			/* id */
    "Q2M",			/* type_prefix */
    "q2m",			/* type_string */
    "-q2m",			/* type_option */ /* ## also "-qw" */
    "Quake II Master",		/* game_name */
    Q2_SERVER,			/* master */
    Q2_MASTER_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY|TF_OUTFILE,	/* flags */
    "",				/* game_rule */
    "Q2MASTER",			/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    q2_masterquery,		/* master_packet */
    sizeof( q2_masterquery),	/* master_len */
    display_qwmaster,		/* display_player_func */
    NULL,	/* display_rule_func */
    NULL,	/* display_raw_player_func */
    NULL,	/* display_raw_rule_func */
    send_qwmaster_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qwmaster_packet,	/* packet_func */
},
{
    /* QUAKE 3 MASTER */
    Q3_MASTER,			/* id */
    "Q3M",			/* type_prefix */
    "q3m",			/* type_string */
    "-q3m",			/* type_option */
    "Quake III Master",		/* game_name */
    Q3_SERVER,			/* master */
    Q3_MASTER_DEFAULT_PORT,	/* default_port */
    TF_OUTFILE | TF_QUERY_ARG,	/* flags */
    "",				/* game_rule */
    "Q3MASTER",			/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    q3_masterquery_buf,		/* master_packet */
    sizeof( q3_masterquery_buf),/* master_len */
    display_qwmaster,		/* display_player_func */
    NULL,	/* display_rule_func */
    NULL,	/* display_raw_player_func */
    NULL,	/* display_raw_rule_func */
    send_qwmaster_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qwmaster_packet,	/* packet_func */
},
{
    /* HALF-LIFE MASTER */
    HL_MASTER,			/* id */
    "HLM",			/* type_prefix */
    "hlm",			/* type_string */
    "-hlm",			/* type_option */ /* ## also "-qw" */
    "Half-Life Master",		/* game_name */
    HL_SERVER,			/* master */
    HL_MASTER_DEFAULT_PORT,	/* default_port */
    TF_SINGLE_QUERY|TF_OUTFILE, /* flags */
    "",				/* game_rule */
    "HLMASTER",			/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    (char*) &hl_masterquery,	/* master_packet */
    sizeof( hl_masterquery),	/* master_len */
    display_qwmaster,		/* display_player_func */
    NULL,	/* display_rule_func */
    NULL,	/* display_raw_player_func */
    NULL,	/* display_raw_rule_func */
    send_qwmaster_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_qwmaster_packet,	/* packet_func */
},
{
    /* TRIBES MASTER */
    TRIBES_MASTER,		/* id */
    "TBM",			/* type_prefix */
    "tbm",			/* type_string */
    "-tbm",			/* type_option */
    "Tribes Master",		/* game_name */
    TRIBES_SERVER,		/* master */
    TRIBES_MASTER_DEFAULT_PORT,	/* default_port */
    TF_OUTFILE,			/* flags */
    "",				/* game_rule */
    "TRIBESMASTER",		/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    (char*) &tribes_masterquery,/* master_packet */
    sizeof( tribes_masterquery),/* master_len */
    display_qwmaster,		/* display_player_func */
    NULL,	/* display_rule_func */
    NULL,	/* display_raw_player_func */
    NULL,	/* display_raw_rule_func */
    send_qwmaster_request_packet,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_tribesmaster_packet,/* packet_func */
},
{
    /* GAMESPY MASTER */
    GAMESPY_MASTER,		/* id */
    "GSM",			/* type_prefix */
    "gsm",			/* type_string */
    "-gsm",			/* type_option */
    "Gamespy Master",		/* game_name */
    GAMESPY_PROTOCOL_SERVER,	/* master */
    GAMESPY_MASTER_DEFAULT_PORT,		/* default_port */
    TF_OUTFILE | TF_TCP_CONNECT | TF_QUERY_ARG | TF_QUERY_ARG_REQUIRED,	/* flags */
    "",				/* game_rule */
    "GAMESPYMASTER",		/* template_var */
    (char*) &gamespy_master_request_prefix,	/* status_packet */
    sizeof( gamespy_master_request_prefix)-1,	/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    (char*) &gamespy_master_validate,/* master_packet */
    sizeof( gamespy_master_validate)-1,/* master_len */
    display_qwmaster,		/* display_player_func */
    NULL,	/* display_rule_func */
    NULL,	/* display_raw_player_func */
    NULL,	/* display_raw_rule_func */
    send_gamespy_master_request,/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    deal_with_gamespy_master_response,/* packet_func */
},
{
    Q_UNKNOWN_TYPE,		/* id */
    "",				/* type_prefix */
    "",				/* type_string */
    "",				/* type_option */
    "",				/* game_name */
    0,				/* master */
    0,				/* default_port */
    0,				/* flags */
    "",				/* game_rule */
    "",				/* template_var */
    NULL,			/* status_packet */
    0,				/* status_len */
    NULL,			/* player_packet */
    0,				/* player_len */
    NULL,			/* rule_packet */
    0,				/* rule_len */
    (char*) NULL,		/* master_packet */
    0,				/* master_len */
    NULL,			/* display_player_func */
    NULL,			/* display_rule_func */
    NULL,			/* display_raw_player_func */
    NULL,			/* display_raw_rule_func */
    NULL,			/* status_query_func */
    NULL,			/* rule_query_func */
    NULL,			/* player_query_func */
    NULL,			/* packet_func */
}
};

#endif /* QUERY_PACKETS */

/* Structures for keeping information about Quake servers, server
 * rules, and players.
 */

struct player;

#define FLAG_BROADCAST	0x1

struct qserver {
    char *arg;
    char *host_name;
    unsigned long ipaddr;
    int flags;
    server_type * type;
    int fd;
    char *outfilename;
    char *query_arg;
    unsigned short port;
    int retry1;
    int retry2;
    int n_retries;
    struct timeval packet_time1;
    struct timeval packet_time2;
    int ping_total;		/* average is ping_total / n_requests */
    int n_requests;
    int n_packets;

    int n_servers;
    int master_pkt_len;
    char *master_pkt;
    char master_query_tag[4];
    char *error;

    char *server_name;
    char *address;
    char *map_name;
    char *game;
    int max_players;
    int num_players;
    int protocol_version;

    unsigned char *saved_data;
    int saved_data_size;

    int next_player_info;
    int n_player_info;
    struct player *players;

    char *next_rule;
    int n_rules;
    struct rule *rules;
    struct rule **last_rule;
    int missing_rules;

    struct qserver *next;
};

struct player  {
    int number;
    char *name;
    int frags;
    int team;		/* Unreal and Tribes only */
    char *team_name;	/* Tribes, BFRIS only, do not free()  */
    int connect_time;
    int shirt_color;
    int pants_color;
    char *address;
    int ping;
    int packet_loss;	/* Tribes only */
    char *skin;
    char *mesh;		/* Unreal only */
    char *face;		/* Unreal only */
    int score;		/* BFRIS only */
    int ship;		/* BFRIS only */
    int room;		/* BFRIS only */
    struct player *next;
};

struct rule  {
    char *name;
    char *value;
    struct rule *next;
};

extern char *qstat_version;
extern char *DOWN;
extern char *SYSERROR;
extern char *TIMEOUT;
extern char *MASTER;
extern char *SERVERERROR;
extern char *HOSTNOTFOUND;

#define DEFAULT_RETRIES			3
#define DEFAULT_RETRY_INTERVAL		500	/* milli-seconds */
#define MAXFD_DEFAULT			20

#define SORT_GAME		1
#define SORT_PING		2
extern int first_sort_key;
extern int second_sort_key;

#define SECONDS 0
#define CLOCK_TIME 1
#define STOPWATCH_TIME 2
#define DEFAULT_TIME_FMT_RAW            SECONDS
#define DEFAULT_TIME_FMT_DISPLAY        CLOCK_TIME
extern int time_format;

extern int color_names;


/* Definitions for the original Quake network protocol.
 */

#define PACKET_LEN 0xffff

/* Quake packet formats and magic numbers
 */
struct qheader  {
    unsigned char flag1;
    unsigned char flag2;
    unsigned short length;
    unsigned char op_code;
};

#define Q_NET_PROTOCOL_VERSION	3
#define HEXEN2_NET_PROTOCOL_VERSION	4

#define Q_CCREQ_CONNECT		0x01
#define Q_CCREP_ACCEPT		0x81
#define Q_CCREP_REJECT		0x82

#define Q_CCREP_SERVER_INFO	0x83

#define Q_CCREP_PLAYER_INFO	0x84

#define Q_CCREP_RULE_INFO	0x85

#define Q_DEFAULT_SV_MAXSPEED	"320"
#define Q_DEFAULT_SV_FRICTION	"4"
#define Q_DEFAULT_SV_GRAVITY	"800"
#define Q_DEFAULT_NOEXIT	"0"
#define Q_DEFAULT_TEAMPLAY	"0"
#define Q_DEFAULT_TIMELIMIT	"0"
#define Q_DEFAULT_FRAGLIMIT	"0"


/* Definitions for the QuakeWorld network protocol
 */


/*
#define QW_GET_SERVERS    'c'
*/
#define QW_SERVERS        'd'
#define HL_SERVERS        'f'
/*
HL master: send 'a', master responds with a small 'l' packet containing
	the text "Outdated protocol"
HL master: send 'e', master responds with a small 'f' packet
HL master: send 'g', master responds with a small 'h' packet containing
	name of master server
HL master: send 'i', master responds with a small 'j' packet
*/
#define QW_GET_USERINFO   'o'
#define QW_USERINFO       'p'
#define QW_GET_SEENINFO   'u'
#define QW_SEENINFO       'v'
#define QW_NACK           'm'
#define QW_NEWLINE        '\n'
#define QW_RULE_SEPARATOR '\\'

#define QW_REQUEST_LENGTH 20

int is_default_rule( struct rule *rule);
char *xform_name( char*, struct qserver *server);
char *quake_color( int color);
char *play_time( int seconds, int show_seconds);
char *ping_time( int ms);
char *get_qw_game( struct qserver *server);


/* Query status and packet handling functions
 */

void cleanup_qserver( struct qserver *server, int force);
 
int server_info_packet( struct qserver *server, struct q_packet *pkt,
        int datalen);
int player_info_packet( struct qserver *server, struct q_packet *pkt,
        int datalen);
int rule_info_packet( struct qserver *server, struct q_packet *pkt,
	int datalen);
 
int time_delta( struct timeval *later, struct timeval *past);
char * strherror( int h_err);
int connection_refused();
 
void add_file( char *filename);
int add_qserver( char *arg, server_type* type, char *outfilename, char *query_arg);
struct qserver* add_qserver_byaddr( unsigned long ipaddr, unsigned short port,
	server_type* type, int *new_server);
void init_qserver( struct qserver *server);
int bind_qserver( struct qserver *server);
void bind_sockets();
void send_packets();
struct qserver * find_server_by_address( unsigned int ipaddr, unsigned short port);
void add_server_to_hash( struct qserver *server);


 
int set_fds( fd_set *fds);
void get_next_timeout( struct timeval *timeout);

void print_packet( char *buf, int buflen);


/*
 * Output template stuff
 */

int read_qserver_template( char *filename);
int read_header_template( char *filename);
int read_trailer_template( char *filename);
int read_player_template( char *filename);
int have_server_template();
int have_header_template();
int have_trailer_template();

void template_display_server( struct qserver *server);
void template_display_header();
void template_display_trailer();
void template_display_players( struct qserver *server);
void template_display_player( struct qserver *server, struct player *player);



/*
 * Host cache stuff
 */

int hcache_open( char *filename, int update);
void hcache_write( char *filename);
void hcache_invalidate();
void hcache_validate();
unsigned long hcache_lookup_hostname( char *hostname);
char * hcache_lookup_ipaddr( unsigned long ipaddr);
void hcache_write_file( char *filename);
void hcache_update_file();

