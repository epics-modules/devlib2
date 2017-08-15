
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <epicsGetopt.h>
#include <epicsMMIO.h>
#include <devLibPCI.h>
#include <devLibPCIImpl.h>

int verbose=0;

static const
epicsPCIID allids[] = {
    DEVPCI_DEVICE_ANY(),
    DEVPCI_END
};

static
int usage(int argc, char *argv[])
{
    fprintf(stderr, "Usage: %s [-h] [-v] [-B <bar#>] [-S <addr>] [-C <count>] [-W <width>] <pci_address> <read|write> [file]\n", argv[0]);
    return 1;
}

int main(int argc, char *argv[])
{
    int opt, bar=0, nargs, ret;
    const epicsPCIDevice *dev = NULL;
    volatile void *base;
    int width = 32;
    epicsUInt32 len = 0,
                start = 0,
                count = 0;
    FILE *io = stdout;

    while ((opt=getopt(argc, argv, "hvB:S:C:d:"))!=-1) {
        switch(opt) {
        case 'v':
            verbose = 1;
            break;
        case 'B':
            bar = atoi(optarg);
            break;
        case 'S':
            start = atoi(optarg);
            break;
        case 'C':
            count = atoi(optarg);
            break;
        case 'd':
            devPCIDebug = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Unknown argument '%c'\n", opt);
        case 'h':
            return usage(argc, argv);
        }
    }
    nargs = argc-optind;

    if(bar<0 || bar>6) {
        fprintf(stderr, "Invalid bar %d\n", bar);
        return 1;
    }

    if(nargs<2) {
        return usage(argc, argv);
    }

    devLibPCIRegisterBaseDefault();

    ret = devPCIFindSpec(allids, argv[optind], &dev, 0);
    if(ret) {
        fprintf(stderr, "Failed to find '%s'\n", argv[optind]);
        return 1;
    } else if(verbose) {
        fprintf(stderr, "Found '%s'\n", argv[optind]);
    }

    ret = devPCIToLocalAddr(dev, bar, &base, 0);
    if(ret) {
        fprintf(stderr, "Failed to map bar %d\n", bar);
        return 1;
    } else if(verbose) {
        fprintf(stderr, "Mapped bar %d to %p\n", bar, base);
    }

    ret = devPCIBarLen(dev, bar, &len);
    if(ret) {
        fprintf(stderr, "Failed to find length of bar %d\n", bar);
        return 1;
    } else if(verbose) {
        fprintf(stderr, "bar %d length %u\n", bar, (unsigned)len);
    }

    if(count>0 && count<len)
        len = count;

    width /= 8;
    if(width<=0 || width>4) {
        fprintf(stderr, "Invalid width %d\n", width*8);
        return 1;
    }

    if(strcmp("read", argv[optind+1])==0) {
        epicsUInt32 i = start, end = start+len;
        if(nargs>=3) {
            io = fopen(argv[optind+2], "wb");
            if(!io) {
                fprintf(stderr, "Failed to open '%s' for input\n", argv[optind+2]);
                return 1;
            }
        }

        if(verbose) {
            fprintf(stderr, "Read %u bytes\n", (unsigned)len);
        }

        for(; i<end; i+=4) {
            epicsUInt32 val = le_ioread32(((volatile char*)base)+i);
            fwrite(&val, 4, 1, io);
        }

        return 0;
    } else {
        fprintf(stderr, "Unknown command '%s'\n", argv[optind+1]);
        return 1;
    }
}

