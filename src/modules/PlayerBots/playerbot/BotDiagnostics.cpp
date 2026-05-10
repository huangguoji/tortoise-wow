#include "BotDiagnostics.h"
#include "PlayerbotAIConfig.h"

namespace ai { namespace botdiag {
    __declspec(thread) const char* gLastPhaseTag     = nullptr;
    __declspec(thread) const char* gLastPhaseBotName = nullptr;

    bool IsActionLogEnabled()
    {
        return sPlayerbotAIConfig.enableActionLog;
    }
}}
