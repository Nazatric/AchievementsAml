#include "amlmod.h"
#include "logger.h"
#include "iaml.h"

#include <string>
#include <vector>
#include <ctime>
#include <pthread.h>
#include <unistd.h>

MYMOD(net.nazaqat.achievementsaml, AchievementsSA, 0.1.0, Nazaqat)
NEEDGAME(com.rockstargames.gtasa)

struct Achievement
{
    const char* key;
    const char* label;
    const char* description;
    bool (*check)();
};

static time_t s_sessionStart = 0;

static bool Check_SessionStarted()
{
    return s_sessionStart != 0;
}

static bool Check_TenMinutePlaytime()
{
    if (s_sessionStart == 0) return false;
    return (time(nullptr) - s_sessionStart) >= 600;
}

static Achievement g_achievements[] = {
    { "ach_session_started_v2", "Mod Loaded",   "AML successfully loaded AchievementsSA", Check_SessionStarted },
    { "ach_ten_min_play_v2",    "Getting Comfortable", "Played for 10 minutes this session",      Check_TenMinutePlaytime },
};
static const int kAchCount = sizeof(g_achievements) / sizeof(g_achievements[0]);

static bool IsUnlocked(const Achievement& a)
{
    int32_t val = 0;
    if (aml->MLSGetInt(a.key, &val))
        return val != 0;
    return false;
}

static void SetUnlocked(const Achievement& a)
{
    aml->MLSSetInt(a.key, 1);
    aml->MLSSaveFile();
}

static void NotifyUnlock(const Achievement& a)
{
    aml->ShowToast(true, "Achievement Unlocked: %s\n%s", a.label, a.description);
    logger->Info("Unlocked: %s", a.label);
}

static void* PollThread(void*)
{
    aml->GetJNIEnvironment();

    s_sessionStart = time(nullptr);

    while (true)
    {
        for (int i = 0; i < kAchCount; i++)
        {
            Achievement& a = g_achievements[i];
            if (IsUnlocked(a)) continue;

            if (a.check())
            {
                SetUnlocked(a);
                NotifyUnlock(a);
            }
        }
        usleep(500 * 1000);
    }
    return nullptr;
}

extern "C" void OnModLoad()
{
    logger->SetTag("AchievementsSA");
    logger->Info("Mod loaded, starting poll thread");
    pthread_t t;
    pthread_create(&t, nullptr, PollThread, nullptr);
}
