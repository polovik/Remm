/* $Id: icedemo.c 4217 2012-07-27 17:24:12Z nanang $ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pjlib.h>
#include <pjlib-util.h>
#include <pjnath.h>

#define THIS_FILE   "client.c"

/* For this demo app, configure longer STUN keep-alive time
 * so that it does't clutter the screen output.
 */
#define KA_INTERVAL 300

/* This is our global variables */
static struct app_t {
	/* Command line options are stored here */
	struct options {
		unsigned comp_cnt;
		pj_str_t ns;
		int max_host;
		pj_bool_t regular;
		pj_str_t stun_srv;
		pj_str_t turn_srv;
		pj_bool_t turn_tcp;
		pj_str_t turn_username;
		pj_str_t turn_password;
		pj_bool_t turn_fingerprint;
		const char *log_file;
	} opt;

	/* Our global variables */
	pj_caching_pool cp;
	pj_pool_t *pool;
	pj_thread_t *thread;
	pj_bool_t thread_quit_flag;
	pj_ice_strans_cfg ice_cfg;
	pj_ice_strans *icest;
	FILE *log_fhnd;

	/* Variables to store parsed remote ICE info */
	struct rem_info {
		char ufrag[80];
		char pwd[80];
		unsigned comp_cnt;
		pj_sockaddr def_addr[PJ_ICE_MAX_COMP];
		unsigned cand_cnt;
		pj_ice_sess_cand cand[PJ_ICE_ST_MAX_CAND];
	} rem;
} icedemo;

/* Utility to display error messages */
static void icedemo_perror(const char *title, pj_status_t status) {
	char errmsg[PJ_ERR_MSG_SIZE];

	pj_strerror(status, errmsg, sizeof(errmsg));
	PJ_LOG(1, (THIS_FILE, "%s: %s", title, errmsg));
}

/* Utility: display error message and exit application (usually
 * because of fatal error.
 */
static void err_exit(const char *title, pj_status_t status) {
	if (status != PJ_SUCCESS) {
		icedemo_perror(title, status);
	}
	PJ_LOG(3, (THIS_FILE, "Shutting down.."));

	if (icedemo.icest)
		pj_ice_strans_destroy(icedemo.icest);

	pj_thread_sleep(500);

	icedemo.thread_quit_flag = PJ_TRUE;
	if (icedemo.thread) {
		pj_thread_join(icedemo.thread);
		pj_thread_destroy(icedemo.thread);
	}

	if (icedemo.ice_cfg.stun_cfg.ioqueue)
		pj_ioqueue_destroy(icedemo.ice_cfg.stun_cfg.ioqueue);

	if (icedemo.ice_cfg.stun_cfg.timer_heap)
		pj_timer_heap_destroy(icedemo.ice_cfg.stun_cfg.timer_heap);

	pj_caching_pool_destroy(&icedemo.cp);

	pj_shutdown();

	if (icedemo.log_fhnd) {
		fclose(icedemo.log_fhnd);
		icedemo.log_fhnd = NULL;
	}

	exit(status != PJ_SUCCESS);
}

#define CHECK(expr)	status=expr; \
			if (status!=PJ_SUCCESS) { \
			    err_exit(#expr, status); \
			}

/*
 * This function checks for events from both timer and ioqueue (for
 * network events). It is invoked by the worker thread.
 */
static pj_status_t handle_events(unsigned max_msec, unsigned *p_count) {
	enum {
		MAX_NET_EVENTS = 1
	};
	pj_time_val max_timeout = { 0, 0 };
	pj_time_val timeout = { 0, 0 };
	unsigned count = 0, net_event_count = 0;
	int c;

	max_timeout.msec = max_msec;

	/* Poll the timer to run it and also to retrieve the earliest entry. */
	timeout.sec = timeout.msec = 0;
	c = pj_timer_heap_poll(icedemo.ice_cfg.stun_cfg.timer_heap, &timeout);
	if (c > 0)
		count += c;

	/* timer_heap_poll should never ever returns negative value, or otherwise
	 * ioqueue_poll() will block forever!
	 */
	pj_assert(timeout.sec >= 0 && timeout.msec >= 0);
	if (timeout.msec >= 1000)
		timeout.msec = 999;

	/* compare the value with the timeout to wait from timer, and use the 
	 * minimum value. 
	 */
	if (PJ_TIME_VAL_GT(timeout, max_timeout))
		timeout = max_timeout;

	/* Poll ioqueue. 
	 * Repeat polling the ioqueue while we have immediate events, because
	 * timer heap may process more than one events, so if we only process
	 * one network events at a time (such as when IOCP backend is used),
	 * the ioqueue may have trouble keeping up with the request rate.
	 *
	 * For example, for each send() request, one network event will be
	 *   reported by ioqueue for the send() completion. If we don't poll
	 *   the ioqueue often enough, the send() completion will not be
	 *   reported in timely manner.
	 */
	do {
		c = pj_ioqueue_poll(icedemo.ice_cfg.stun_cfg.ioqueue, &timeout);
		if (c < 0) {
			pj_status_t err = pj_get_netos_error();
			pj_thread_sleep(PJ_TIME_VAL_MSEC(timeout));
			if (p_count)
				*p_count = count;
			return err;
		} else if (c == 0) {
			break;
		} else {
			net_event_count += c;
			timeout.sec = timeout.msec = 0;
		}
	} while (c > 0 && net_event_count < MAX_NET_EVENTS);

	count += net_event_count;
	if (p_count)
		*p_count = count;

	return PJ_SUCCESS;

}

/*
 * This is the worker thread that polls event in the background.
 */
static int icedemo_worker_thread(void *unused) {
	PJ_UNUSED_ARG(unused);

	while (!icedemo.thread_quit_flag) {
		handle_events(500, NULL);
	}

	return 0;
}

/*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about incoming data. By "data" it means application
 * data such as RTP/RTCP, and not packets that belong to ICE signaling (such
 * as STUN connectivity checks or TURN signaling).
 */
static void cb_on_rx_data(pj_ice_strans *ice_st, unsigned comp_id, void *pkt,
		pj_size_t size, const pj_sockaddr_t *src_addr, unsigned src_addr_len) {
	char ipstr[PJ_INET6_ADDRSTRLEN + 10];

	PJ_UNUSED_ARG(ice_st);
	PJ_UNUSED_ARG(src_addr_len);
	PJ_UNUSED_ARG(pkt);

	// Don't do this! It will ruin the packet buffer in case TCP is used!
	//((char*)pkt)[size] = '\0';

	PJ_LOG(3, (THIS_FILE, "Component %d: received %d bytes data from %s: \"%.*s\"", comp_id, size, pj_sockaddr_print(
			   src_addr, ipstr, sizeof(ipstr), 3), (unsigned) size, (char*) pkt));
}

/*
 * This is the callback that is registered to the ICE stream transport to
 * receive notification about ICE state progression.
 */
static void cb_on_ice_complete(pj_ice_strans *ice_st, pj_ice_strans_op op,
								   pj_status_t status) {
	const char *opname = (op == PJ_ICE_STRANS_OP_INIT ? "initialization" :
								 (op == PJ_ICE_STRANS_OP_NEGOTIATION ?
								 "negotiation" : "unknown_op"));

	if (status == PJ_SUCCESS) {
		PJ_LOG(3, (THIS_FILE, "ICE %s successful", opname));
	} else {
		char errmsg[PJ_ERR_MSG_SIZE];

		pj_strerror(status, errmsg, sizeof(errmsg));
		PJ_LOG(1, (THIS_FILE, "ICE %s failed: %s", opname, errmsg));
		pj_ice_strans_destroy(ice_st);
		icedemo.icest = NULL;
	}
}

/* log callback to write to file */
static void log_func(int level, const char *data, int len) {
	pj_log_write(level, data, len);
	if (icedemo.log_fhnd) {
		if (fwrite(data, len, 1, icedemo.log_fhnd) != 1)
			return;
	}
}

/*
 * This is the main application initialization function. It is called
 * once (and only once) during application initialization sequence by 
 * main().
 */
static pj_status_t icedemo_init(void) {
	pj_status_t status;

	if (icedemo.opt.log_file) {
		icedemo.log_fhnd = fopen(icedemo.opt.log_file, "a");
		pj_log_set_log_func(&log_func);
	}

	/* Initialize the libraries before anything else */
	CHECK(pj_init());
	CHECK(pjlib_util_init());
	CHECK(pjnath_init());

	/* Must create pool factory, where memory allocations come from */
	pj_caching_pool_init(&icedemo.cp, NULL, 0);

	/* Init our ICE settings with null values */
	pj_ice_strans_cfg_default(&icedemo.ice_cfg);

	icedemo.ice_cfg.stun_cfg.pf = &icedemo.cp.factory;

	/* Create application memory pool */
	icedemo.pool = pj_pool_create(&icedemo.cp.factory, "client", 512, 512, NULL);

	/* Create timer heap for timer stuff */
	CHECK(pj_timer_heap_create(icedemo.pool, 100, &icedemo.ice_cfg.stun_cfg.timer_heap));

	/* and create ioqueue for network I/O stuff */
	CHECK(pj_ioqueue_create(icedemo.pool, 16, &icedemo.ice_cfg.stun_cfg.ioqueue));

	/* something must poll the timer heap and ioqueue, 
	 * unless we're on Symbian where the timer heap and ioqueue run
	 * on themselves.
	 */
	CHECK(pj_thread_create(icedemo.pool, "client", &icedemo_worker_thread, NULL, 0, 0, &icedemo.thread));

	icedemo.ice_cfg.af = pj_AF_INET();

	/* Create DNS resolver if nameserver is set */
	if (icedemo.opt.ns.slen) {
		CHECK(pj_dns_resolver_create(&icedemo.cp.factory, "resolver", 0, icedemo.ice_cfg.stun_cfg.timer_heap, icedemo.ice_cfg.stun_cfg.ioqueue, &icedemo.ice_cfg.resolver));

		CHECK(pj_dns_resolver_set_ns(icedemo.ice_cfg.resolver, 1, &icedemo.opt.ns, NULL));
	}

	/* -= Start initializing ICE stream transport config =- */

	/* Maximum number of host candidates */
	if (icedemo.opt.max_host != -1)
		icedemo.ice_cfg.stun.max_host_cands = icedemo.opt.max_host;

	/* Nomination strategy */
	if (icedemo.opt.regular)
		icedemo.ice_cfg.opt.aggressive = PJ_FALSE;
	else
		icedemo.ice_cfg.opt.aggressive = PJ_TRUE;

	/* Configure STUN/srflx candidate resolution */
	if (icedemo.opt.stun_srv.slen) {
		char *pos;

		/* Command line option may contain port number */
		if ((pos = pj_strchr(&icedemo.opt.stun_srv, ':')) != NULL) {
			icedemo.ice_cfg.stun.server.ptr = icedemo.opt.stun_srv.ptr;
			icedemo.ice_cfg.stun.server.slen = (pos - icedemo.opt.stun_srv.ptr);

			icedemo.ice_cfg.stun.port = (pj_uint16_t) atoi(pos + 1);
		} else {
			icedemo.ice_cfg.stun.server = icedemo.opt.stun_srv;
			icedemo.ice_cfg.stun.port = PJ_STUN_PORT;
		}

		/* For this demo app, configure longer STUN keep-alive time
		 * so that it does't clutter the screen output.
		 */
		icedemo.ice_cfg.stun.cfg.ka_interval = KA_INTERVAL;
	}

	/* Configure TURN candidate */
	if (icedemo.opt.turn_srv.slen) {
		char *pos;

		/* Command line option may contain port number */
		if ((pos = pj_strchr(&icedemo.opt.turn_srv, ':')) != NULL) {
			icedemo.ice_cfg.turn.server.ptr = icedemo.opt.turn_srv.ptr;
			icedemo.ice_cfg.turn.server.slen = (pos - icedemo.opt.turn_srv.ptr);

			icedemo.ice_cfg.turn.port = (pj_uint16_t) atoi(pos + 1);
		} else {
			icedemo.ice_cfg.turn.server = icedemo.opt.turn_srv;
			icedemo.ice_cfg.turn.port = PJ_STUN_PORT;
		}

		/* TURN credential */
		icedemo.ice_cfg.turn.auth_cred.type = PJ_STUN_AUTH_CRED_STATIC;
		icedemo.ice_cfg.turn.auth_cred.data.static_cred.username =
				icedemo.opt.turn_username;
		icedemo.ice_cfg.turn.auth_cred.data.static_cred.data_type =
				PJ_STUN_PASSWD_PLAIN;
		icedemo.ice_cfg.turn.auth_cred.data.static_cred.data =
				icedemo.opt.turn_password;

		/* Connection type to TURN server */
		if (icedemo.opt.turn_tcp)
			icedemo.ice_cfg.turn.conn_type = PJ_TURN_TP_TCP;
		else
			icedemo.ice_cfg.turn.conn_type = PJ_TURN_TP_UDP;

		/* For this demo app, configure longer keep-alive time
		 * so that it does't clutter the screen output.
		 */
		icedemo.ice_cfg.turn.alloc_param.ka_interval = KA_INTERVAL;
	}

	/* -= That's it for now, initialization is complete =- */
	return PJ_SUCCESS;
}

/*
 * Create ICE stream transport instance, invoked from the menu.
 */
static void icedemo_create_instance(void) {
	pj_ice_strans_cb icecb;
	pj_status_t status;

	if (icedemo.icest != NULL) {
		puts("ICE instance already created, destroy it first");
		return;
	}

	/* init the callback */
	pj_bzero(&icecb, sizeof(icecb));
	icecb.on_rx_data = cb_on_rx_data;
	icecb.on_ice_complete = cb_on_ice_complete;

	/* create the instance */
	status = pj_ice_strans_create("client", /* object name  */
								  &icedemo.ice_cfg, /* settings	    */
								  icedemo.opt.comp_cnt, /* comp_cnt	    */
								  NULL, /* user data    */
								  &icecb, /* callback	    */
								  &icedemo.icest); /* instance ptr */
	if (status != PJ_SUCCESS)
		icedemo_perror("error creating ice", status);
	else
		PJ_LOG(3, (THIS_FILE, "ICE instance successfully created"));
}

/* Utility to nullify parsed remote info */
static void reset_rem_info(void) {
	pj_bzero(&icedemo.rem, sizeof(icedemo.rem));
}

/*
 * Stop/destroy ICE session.
 * Destroy ICE stream transport instance, invoked from the menu.
 */
static void icedemo_destroy_instance(int signum) {
	pj_status_t status;

	if (icedemo.icest == NULL) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE instance, create it first"));
		exit(0);
	}

	if (!pj_ice_strans_has_sess(icedemo.icest)) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE session, initialize first"));
		exit(0);
	}

	status = pj_ice_strans_stop_ice(icedemo.icest);
	if (status != PJ_SUCCESS)
		icedemo_perror("error stopping session", status);
	else
		PJ_LOG(3, (THIS_FILE, "ICE session stopped"));

	reset_rem_info();

	if (icedemo.icest == NULL) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE instance, create it first"));
		exit(0);
	}

	pj_ice_strans_destroy(icedemo.icest);
	icedemo.icest = NULL;

	reset_rem_info();

	PJ_LOG(3, (THIS_FILE, "ICE instance destroyed"));

	exit(0);
}

/*
 * Create ICE session, invoked from the menu.
 */
static void icedemo_init_session(unsigned rolechar) {
	pj_ice_sess_role role = (
			pj_tolower((pj_uint8_t) rolechar) == 'o' ?
					PJ_ICE_SESS_ROLE_CONTROLLING : PJ_ICE_SESS_ROLE_CONTROLLED);
	pj_status_t status;

	if (icedemo.icest == NULL) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE instance, create it first"));
		return;
	}

	if (pj_ice_strans_has_sess(icedemo.icest)) {
		PJ_LOG(1, (THIS_FILE, "Error: Session already created"));
		return;
	}

	status = pj_ice_strans_init_ice(icedemo.icest, role, NULL, NULL);
	if (status != PJ_SUCCESS)
		icedemo_perror("error creating session", status);
	else
		PJ_LOG(3, (THIS_FILE, "ICE session created"));

	reset_rem_info();
}

/*
 * Start ICE negotiation! This function is invoked from the menu.
 */
static void icedemo_start_nego(void) {
	pj_str_t rufrag, rpwd;
	pj_status_t status;

	if (icedemo.icest == NULL) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE instance, create it first"));
		return;
	}

	if (!pj_ice_strans_has_sess(icedemo.icest)) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE session, initialize first"));
		return;
	}

	if (icedemo.rem.cand_cnt == 0) {
		PJ_LOG(1, (THIS_FILE, "Error: No remote info, input remote info first"));
		return;
	}

	PJ_LOG(3, (THIS_FILE, "Starting ICE negotiation.."));

	status = pj_ice_strans_start_ice(icedemo.icest,
									 pj_cstr(&rufrag, icedemo.rem.ufrag),
									 pj_cstr(&rpwd, icedemo.rem.pwd), icedemo.rem.cand_cnt,
									 icedemo.rem.cand);
	if (status != PJ_SUCCESS)
		icedemo_perror("Error starting ICE", status);
	else
		PJ_LOG(3, (THIS_FILE, "ICE negotiation started"));
}

/*
 * Send application data to remote agent.
 */
static void icedemo_send_data(unsigned comp_id, const char *data) {
	pj_status_t status;

	if (icedemo.icest == NULL) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE instance, create it first"));
		return;
	}

	if (!pj_ice_strans_has_sess(icedemo.icest)) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE session, initialize first"));
		return;
	}

	/*
	 if (!pj_ice_strans_sess_is_complete(icedemo.icest)) {
	 PJ_LOG(1,(THIS_FILE, "Error: ICE negotiation has not been started or is in progress"));
	 return;
	 }
	 */

	if (comp_id < 1 || comp_id > pj_ice_strans_get_running_comp_cnt(icedemo.icest)) {
		PJ_LOG(1, (THIS_FILE, "Error: invalid component ID"));
		return;
	}

	status = pj_ice_strans_sendto(icedemo.icest, comp_id, data, strlen(data),
								  &icedemo.rem.def_addr[comp_id - 1],
								  pj_sockaddr_get_len(&icedemo.rem.def_addr[comp_id - 1]));
	if (status != PJ_SUCCESS)
		icedemo_perror("Error sending data", status);
	else
		PJ_LOG(3, (THIS_FILE, "Data sent"));
}

/*
 * And here's the main()
 */
int main(int argc, char *argv[]) {
	pj_status_t status;

	icedemo.opt.comp_cnt = 1;	//	Component count
	icedemo.opt.max_host = -1;	//	max number of host candidates
//	icedemo.opt.ns; // nameserver to activate DNS SRV resolution
	icedemo.opt.stun_srv = pj_str("stun.ekiga.net");
//	icedemo.opt.turn_srv; //
//	icedemo.opt.turn_tcp;
//	icedemo.opt.turn_username;
//	icedemo.opt.turn_password;
//	icedemo.opt.turn_fingerprint
//	icedemo.opt.regular
//	icedemo.opt.log_file

    // Register signal and signal handler
    signal(SIGINT, icedemo_destroy_instance);

    printf("==============icedemo_init\n");
	status = icedemo_init();
	if (status != PJ_SUCCESS)
		return 1;

	sleep(2);
	printf("==============icedemo_create_instance\n");
	icedemo_create_instance();
	sleep(2);
	printf("==============icedemo_init_session\n");
#ifdef CLIENT
	icedemo_init_session('a');
#elif SERVER
	icedemo_init_session('o');
#else
	assert(0);
#endif
	sleep(2);
	printf("==============icedemo_show_ice\n");

	if (icedemo.icest == NULL) {
		PJ_LOG(1, (THIS_FILE, "Error: No ICE instance, create it first"));
		return;
	}
	if (pj_ice_strans_sess_is_complete(icedemo.icest))
		puts("negotiation complete");
	else if (pj_ice_strans_sess_is_running(icedemo.icest))
		puts("negotiation is in progress");
	else if (pj_ice_strans_has_sess(icedemo.icest))
		puts("session ready");
	else
		puts("session not created");
	if (!pj_ice_strans_has_sess(icedemo.icest)) {
		puts("Create the session first to see more info");
		return 1;
	}
	pj_ice_sess_cand cand[PJ_ICE_ST_MAX_CAND];
	char ipaddr[PJ_INET6_ADDRSTRLEN];
	/* Get default candidate for the component */
	status = pj_ice_strans_get_def_cand(icedemo.icest, 1, &cand[0]);
	if (status != PJ_SUCCESS)
		return -status;
	unsigned j, cand_cnt;
	cand_cnt = PJ_ARRAY_SIZE(cand);
	status = pj_ice_strans_enum_cands(icedemo.icest, 1, &cand_cnt, cand);
	if (status != PJ_SUCCESS)
		return -status;
	/* And encode the candidates as SDP */
	for (j = 0; j < cand_cnt; ++j) {
		printf("a=candidate:%.*s %u UDP %u %s %u typ ", (int)cand[j].foundation.slen,
				cand[j].foundation.ptr, (unsigned)cand[j].comp_id, cand[j].prio,
				pj_sockaddr_print(&cand[j].addr, ipaddr, sizeof(ipaddr), 0),
				(unsigned)pj_sockaddr_get_port(&cand[j].addr));

		printf("%s\n", pj_ice_get_cand_type_name(cand[j].type), 0, 0, 0, 0, 0);
	}

	printf("==============Enter remote info:\n");
	/*
v=0
o=- 3414953978 3414953978 IN IP4 localhost
s=ice
t=0 0
a=ice-ufrag:515f007c
a=ice-pwd:5bd062c2
m=audio 51756 RTP/AVP 0
c=IN IP4 91.149.128.89
a=candidate:Sc0a80a5f 1 UDP 1862270975 91.149.128.89 51756 typ srflx

a=candidate:Sc0a80a6a 1 UDP 1862270975 91.149.128.89 36645 typ srflx
a=candidate:Hc0a80a6a 1 UDP 1694498815 192.168.10.106 36645 typ host
a=candidate:Hc0a801ec 1 UDP 1694498815 192.168.1.236 36645 typ host
*/
	char foundation[32], transport[12], type[32];
	int comp_id, prio, port;
	int cnt = fscanf(stdin, "a=candidate:%s %d %s %d %s %d typ %s", foundation,
						&comp_id, transport, &prio, ipaddr, &port, type);
	if (cnt != 7) {
		PJ_LOG(1, (THIS_FILE, "error: Invalid ICE candidate line"));
		return 1;
	}

	pj_ice_sess_cand *rem_cand;
	reset_rem_info();
	strcpy(icedemo.rem.ufrag, "515f007c");
	strcpy(icedemo.rem.pwd, "5bd062c2");

	rem_cand = &icedemo.rem.cand[0];
	pj_bzero(rem_cand, sizeof(*rem_cand));
	rem_cand->type = PJ_ICE_CAND_TYPE_SRFLX;
	rem_cand->comp_id = (pj_uint8_t) 1;
	pj_strdup2(icedemo.pool, &rem_cand->foundation, foundation);
	rem_cand->prio = prio;
	int af = pj_AF_INET();
	pj_str_t tmpaddr = pj_str(ipaddr);
	pj_sockaddr_init(af, &rem_cand->addr, NULL, 0);
	status = pj_sockaddr_set_str_addr(af, &rem_cand->addr, &tmpaddr);
	if (status != PJ_SUCCESS) {
		PJ_LOG(1, (THIS_FILE, "Error: invalid IP address '%s'", tmpaddr.ptr));
		return 1;
	}
	pj_sockaddr_set_port(&rem_cand->addr, (pj_uint16_t) port);
	icedemo.rem.cand_cnt = 1;
	icedemo.rem.comp_cnt = 1;
	if (icedemo.rem.cand_cnt == 0 || icedemo.rem.ufrag[0] == 0
		|| icedemo.rem.pwd[0] == 0 || icedemo.rem.comp_cnt == 0) {
		PJ_LOG(1, (THIS_FILE, "Error: not enough info"));
		return 1;
	}
	pj_sockaddr_init(af, &icedemo.rem.def_addr[0], NULL, 0);
	status = pj_sockaddr_set_str_addr(af, &icedemo.rem.def_addr[0], &tmpaddr);
	if (status != PJ_SUCCESS) {
		PJ_LOG(1, (THIS_FILE, "Invalid IP address in c= line"));
		return 1;
	}
	pj_sockaddr_set_port(&icedemo.rem.def_addr[0], (pj_uint16_t) port);

	PJ_LOG(3, (THIS_FILE, "Done, %d remote candidate(s) added", icedemo.rem.cand_cnt));

	printf("==============icedemo_start_nego\n");
	icedemo_start_nego();

	printf("==============Main LOOP\n");
	while (1) {
		sleep(1);
	}


//	if (strcmp(cmd, "send") == 0 || strcmp(cmd, "x") == 0) {
//
//		char *comp = strtok(NULL, SEP);
//
//		if (!comp) {
//			PJ_LOG(1, (THIS_FILE, "Error: component ID required"));
//		} else {
//			char *data = comp + strlen(comp) + 1;
//			if (!data)
//				data = "";
//			icedemo_send_data(atoi(comp), data);
//		}
//
//	}

	err_exit("Quitting..", PJ_SUCCESS);
	return 0;
}
