#include <android/log.h>
#include "thornyreader.h"
#include "CreBridge.h"

int main(int argc, char *argv[])
{
    __android_log_print(ANDROID_LOG_INFO, "axy.crengine", "crengine v%s", THORNYREADER_VERSION);
    CreBridge cre;
    return cre.main(argc, argv);
}
