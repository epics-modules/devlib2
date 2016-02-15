#!../../bin/linux-x86_64/explore

## You may have to change explore to something else
## everywhere it appears in this file

#< envPaths

## Register all support components
dbLoadDatabase("../../dbd/explore.dbd",0,0)
explore_registerRecordDeviceDriver(pdbbase) 

## Load record instances
##dbLoadRecords("../../db/explore.db","")

iocInit()
