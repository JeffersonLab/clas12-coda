
/* et2hipo.h */


#ifdef	__cplusplus
extern "C" {
#endif

int et_bridge_config_init_hipo(et_bridgeconfig *config);
int et_bridge_config_destroy_hipo(et_bridgeconfig sconfig);
int et_bridge_config_setmodefrom_hipo(et_bridgeconfig config, int val);
int et_bridge_config_getmodefrom_hipo(et_bridgeconfig config, int *val);
int et_bridge_config_setmodeto_hipo(et_bridgeconfig config, int val);
int et_bridge_config_getmodeto_hipo(et_bridgeconfig config, int *val);
int et_bridge_config_setchunkfrom_hipo(et_bridgeconfig config, int val);
int et_bridge_config_getchunkfrom_hipo(et_bridgeconfig config, int *val);
int et_bridge_config_setchunkto_hipo(et_bridgeconfig config, int val);
int et_bridge_config_getchunkto_hipo(et_bridgeconfig config, int *val);
int et_bridge_config_settimeoutfrom_hipo(et_bridgeconfig config, struct timespec val);
int et_bridge_config_gettimeoutfrom_hipo(et_bridgeconfig config, struct timespec *val);
int et_bridge_config_settimeoutto_hipo(et_bridgeconfig config, struct timespec val);
int et_bridge_config_gettimeoutto_hipo(et_bridgeconfig config, struct timespec *val);
int et_bridge_config_setfunc_hipo(et_bridgeconfig config, ET_SWAP_FUNCPTR func);
int et_bridge_CODAswap_hipo(et_event *src_ev, et_event *dest_ev, int bytes, int same_endian);
int et_events_bridge_hipo(et_sys_id id_from, et_sys_id id_to,
		          et_att_id att_from, et_att_id att_to,
		          et_bridgeconfig bconfig,
		          int num, int *ntransferred);

#ifdef  __cplusplus
}
#endif
