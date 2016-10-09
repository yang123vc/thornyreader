#include <android/log.h>
#include "CreBridge.h"

int main(int argc, char *argv[])
{
    __android_log_print(ANDROID_LOG_INFO, "axy.crengine", "crengine v1.0.0");
    CreBridge cre;
    return cre.main(argc, argv);
}
