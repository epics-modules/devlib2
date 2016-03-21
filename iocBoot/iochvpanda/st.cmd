#!../../bin/linux-x86/explore


dbLoadDatabase("../../dbd/explore.dbd",0,0)
explore_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet("BASE","a:0.0")
dbLoadRecords("hv-panda.db","DEV=$(BASE),P-TST:PANDA:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=0,P-TST:PANDA:CH0:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=20,P-TST:PANDA:CH1:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=40,P-TST:PANDA:CH2:")
dbLoadRecords("hv-panda-chan.db","DEV=$(BASE),OFFSET=60,P-TST:PANDA:CH3:")

iocInit()
