
#include "rtsp.h"

int main(int argc, char **argv)
{
    Rtsp2File rf(argv[1], argv[2]);

    rf.run();
    
    return 0;
}