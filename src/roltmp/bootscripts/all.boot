#all.boot
#boot script invoked by all rocs

#add login users
###cd "$CODA/VXWORKS_ppc/bootscripts"
###< loginadd.boot

#add IP hosts
cd "$CODA/VXWORKS_ppc/bootscripts"
< hostadd.boot


#set interrupt rate of the system clock
sysClkRateSet(100)

#set nfs authorization
nfsAuthUnixSet("clonfs1",2508,146)
nfsMount("clonfs1","/vol/logs/run_log","/nfs/usr/clas/logs/run_log")
