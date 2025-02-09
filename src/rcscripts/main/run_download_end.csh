#!/bin/csh

#  run_download_end.csh

#  Coda script, will be run in the end of Download transition

###(echo " "; echo starting: `date`; echo " "; run_log_begin -a clasrun; echo ending: `date`; echo " ";) >>& $CLON_LOG/run_log/run_download_end.log &
###(echo " "; echo starting: `date`; echo " "; sleep 10; echo ending: `date`; echo " ";) >>& $CLON_LOG/run_log/run_download_end.log &

#(echo " "; echo starting: `date`; echo " "; /usr/clas12/release/pro/epics/css_share/detectors/RICH/scripts/rich-lvcycle-daq.sh; set script_status = $status; echo ending: `date`; echo " ";)



echo " "
echo starting: `date`
echo " "

/usr/clas12/release/pro/epics/css_share/detectors/RICH/scripts/rich-lvcycle-daq.sh
##set status = 0

set script_status = $status

echo " "
echo ending: `date`
echo " "

if ($script_status > 0) then
    echo "ERROR in run_download_end.csh: script_status=$script_status"
    exit(1)
else
    echo "INFO: run_download_end.csh executed successfully, script_status=$script_status"
    exit(0)
endif

