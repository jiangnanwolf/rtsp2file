#include "crash_handler.h"
#include "rtsp.h"

int main(int argc, char **argv)
{
    crash_handler_register();

    Rtsp2File rf(argv[1], argv[2]);

    rf.run();
    
    return 0;
}