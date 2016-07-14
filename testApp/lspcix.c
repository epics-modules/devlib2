
#include <stdlib.h>
#include <stdio.h>

#include <devLibPCI.h>
#include <devLibPCIImpl.h>

int main(int argc, char *argv[])
{
    int lvl=2;
    if(argc>1)
        lvl =atoi(argv[1]);
    if(argc>2)
        devPCIDebug = atoi(argv[2]);
    devLibPCIRegisterBaseDefault();
    devPCIShow(lvl, 0, 0, 0);
    return 0;
}
