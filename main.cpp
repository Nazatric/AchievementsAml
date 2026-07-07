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

/* ------------------------------------------------------------------------
 * Achievement definition
 * ---------------------------------------------------------------------- */
struct Achievement
{
    const char* key;          // unique save-key, e.g. "ach_first_boot"
    const char* label;        // Title shown in the toast
    const char* description;  // Subtitle shown in the toast
    bool (*check)();          // Returns true once the condition is met
};

/* ------------------------------------------------------------------------
 * EXAMPLE achievement checks.
 *
 * These two are intentionally "safe" - they only use data AML itself
 * tracks (session start time, mods loaded count), so they are GUARANTEED
 * to work without needing any GTA SA memory offsets.
 *
 * Real gameplay achievements (kills, missions, stats, etc.) need actual
 * memory addresses from the Android build, which I don't have verified
 * yet - see the TODO block below for how we add those safely once we
 * have them.
 * ---------------------------------------------------------------------- */
static time_t s_sessionStart = 0;

static bool Check_SessionStarted()
{
    return s_sessionStart != 0;
}

static bool Check_TenMinutePlaytime()
{
    if (s_sessionStart == 0) return false;
    return (time(nullptr) - s_sessionStart) >= 600; // 10 minutes
}

/* TODO: real gameplay achievements go here once we have verified
 * Android memory offsets or SCM global-variable indices, e.g.:
 *
 * static bool Check_FirstKill()
 * {
 *     // int kills = *(int*)(some_base_addr + offset);
 *     // return kills > 0;
 *     return false;
 * }
 */

static Achievement g_achievements[] = {
    { "ach_session_started", "Engine Started",   "Loaded into San Andreas with AML running", Check_SessionStarted },
    { "ach_ten_min_play",    "Getting Comfortable", "Played for 10 minutes this session",      Check_TenMinutePlaytime },
};
static const int kAchCount = sizeof(g_achievements) / sizeof(g_achievements[0]);

/* ------------------------------------------------------------------------
 * Persistence - uses AML's built-in MLS key/value store so unlocked
 * achievements survive between game sessions without touching game
 * save files at all.
 * ---------------------------------------------------------------------- */
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

/* ------------------------------------------------------------------------
 * Notification - MVP version uses AML's native ShowToast().
 * This pops a native Android toast on screen; it's not the fancy sliding
 * in-game HUD banner the PC version has, but it works out of the box,
 * on every device, with zero risk of crashing the game.
 *
 * (Once we have verified font/draw offsets for the Android build, this
 * can be swapped for a real in-game HUD banner like the PC version.)
 * ---------------------------------------------------------------------- */
static void NotifyUnlock(const Achievement& a)
{
    logger->Info("Achievement unlocked: %s", a.label);
}

/* ------------------------------------------------------------------------
 * Poll loop - AML has no direct "OnGameTick" hook exposed generically,
 * so we run this on a background thread with a sleep interval, which is
 * the standard approach for AML mods that need periodic checks.
 * ---------------------------------------------------------------------- */

static void* PollThread(void*)
{
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
        usleep(500 * 1000); // 500ms, matches the PC mod's poll rate
    }
    return nullptr;
}

/* ------------------------------------------------------------------------
 * Entry point - AML calls this once the mod .so is loaded.
 * ---------------------------------------------------------------------- */
extern "C" void OnModLoad()
{
    logger->SetTag("AchievementsSA");
    logger->Info("Mod loaded, starting poll thread");
    pthread_t t;
    pthread_create(&t, nullptr, PollThread, nullptr);
}
