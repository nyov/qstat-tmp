/*
 * qstat
 * by Steve Jankowski
 *
 * Crysis protocol
 * Copyright 2012 Steven Hartland
 *
 * Licensed under the Artistic License, see LICENSE.txt for license terms
 */
#ifndef QSTAT_CRYSIS_H
#define QSTAT_CRYSIS_H

#include "qserver.h"

// Packet processing methods
query_status_t deal_with_crysis_packet( struct qserver *server, char *pkt, int pktlen );
query_status_t send_crysis_request_packet( struct qserver *server );

#endif

