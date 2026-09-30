
/* ipc.h */

#ifdef  __cplusplus
extern "C" {
#endif

  int epics_json_msg_sender_init(const char *expid, const char *session, const char *send_unique_id, const char *send_topic, const char *recv_unique_id, const char *recv_topic);
  int epics_json_msg_send(const char *caname, const char *catype, int nelem, void *data);
  int epics_json_msg_close();
  int send_daq_message_to_epics(const char *caname, const char *catype, int nelem, void *data);
  int send_control_message(const char *command, const char *option);

#ifdef  __cplusplus
}
#endif
