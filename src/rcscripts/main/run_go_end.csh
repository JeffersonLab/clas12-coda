#!/bin/csh

#  run_go_end.csh

#  Coda script to perform go tasks

#  E.Wolin, 3-oct-97

#  do go tasks


#  smartsockets broadcast
rc_transition -a clasrun -d run_control -file    go  >& /dev/null &


run_log_go -a clasrun >>& $CLON_LOG/run_log/run_log_go.log

# set trigger configuration file for go
#############################download_trig_config -mode go -nogui

#  done
exit(0)
