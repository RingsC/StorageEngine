/*
 * ha_test_main.cpp
 */

#include "ha_head.h"

void ha_BuildConfParms(rep_ha_t *p_ha_conf, char *pc_group, uint16 my_port)
{
	/* election conf */
	p_ha_conf->priority = 100;

	p_ha_conf->listen_port = my_port;

	p_ha_conf->gourp_list = pc_group;

	p_ha_conf->phrase1_timeout= 2;
	p_ha_conf->phrase2_timeout = 4;
	p_ha_conf->election_timeout = 8;

	p_ha_conf->min_leis = 0;
	p_ha_conf->min_votes = 0;

	p_ha_conf->heartbeat_timeout = 20;
	p_ha_conf->heartbeat_interval = 5;
	p_ha_conf->election_delay = 60;
	p_ha_conf->manual_start_election = 0;

	/* communication conf */
	p_ha_conf->first_reconn_time = 8;
	p_ha_conf->tmp_conn_life_time = 6;

	p_ha_conf->send_timeout = 5;
	p_ha_conf->recv_timeout = 5;

	p_ha_conf->tcp_ka_idle = 15;
	p_ha_conf->tcp_ka_interval = 1;
	p_ha_conf->tcp_ka_count = 3;

	p_ha_conf->election_count_2sites = 5;

	return;
}

int main(int argc, char *argv[])
{
	char *pc_my_site = argv[1];
	char *pc_my_ip = NULL;
	uint16 my_port;
	char *pc_group = argv[2];
	rep_ha_t ha_conf;
	ha_env_t *p_env = &g_ha_env;
	int ret = 0;

	initsocketenv();

	/* check and record argments */
	ha_Log("argc: %d", argc);
	for(int i = 0; i < argc; i++){
		ha_Log("argv[%d]: %s", i, argv[i]);
	}
	if(argc != 3){
		ha_Error("Wrong argments.");
		ret = -1;
		goto out;
	}

	/* get my site info */
	if(!ha_ParseIpPort(pc_my_site, &pc_my_site, &my_port)){
		ha_Error("Parse my addr failed");
		ret = -1;
		goto out;
	}

	/* init env */
	memset((char *)p_env, 0, sizeof(ha_env_t));
	ha_BuildConfParms(&ha_conf, pc_group, my_port);
	if(!ha_EnvInit(p_env, pc_my_site, &ha_conf, false)){
		ha_Error("init env failed.");
		ret = -1;
		goto out;
	}

	ha_SelectLoop((void *)p_env);

out:
	if(pc_my_ip != 0){
		free(pc_my_ip);
	}

	return ret;
}

