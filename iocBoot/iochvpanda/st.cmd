#!../../bin/linux-x86_64/explore


dbLoadDatabase("../../dbd/explore.dbd",0,0)
explore_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("BASE","a:0.0")
epicsEnvSet("P","TST:PANDA:")
dbLoadRecords("hv-panda.db","DEV=$(BASE),P=$(P)")
dbLoadRecords("hv-panda-spi.db","DEV=$(BASE),P=$(P)")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=0,P=$(P)CH0:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=20,P=$(P)CH1:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=40,P=$(P)CH2:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=60,P=$(P)CH3:")

iocInit()
