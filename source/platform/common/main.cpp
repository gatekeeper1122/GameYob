#include "platform/common/manager.h"
#include "platform/system.h"

int main(int argc, char* argv[]) {
    if(!systemInit(argc, argv)) {
        return 1;
    }

    mgrInit();

    while(systemIsRunning()) {
        mgrRun();
    }

    mgrExit();

    systemExit();
    return 0;
}
