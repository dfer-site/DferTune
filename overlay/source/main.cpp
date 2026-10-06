#define TESLA_INIT_IMPL

#include "exception_wrap.hpp"
#include "tune.h"
#include "gui_error.hpp"
#include "gui_main.hpp"
#include "sdmc/sdmc.hpp"
#include "pm/pm.hpp"
#include "config/config.hpp"
#include "overlay_i18n.hpp"
#include "strings.hpp"

#include <tesla.hpp>

class SysTuneOverlay final : public tsl::Overlay {
  private:
    const char *msg = nullptr;
    Result fail     = 0;

  public:
    void initServices() override {
        // Check free system RAM before attempting to launch the sysmodule.
        // Uses the same svcGetSystemInfo calls as drawMemoryWidget in gui_main.cpp:
        //   type 0, subtype 2 = total system RAM
        //   type 1, subtype 2 = used  system RAM
        {
            u64 RAM_Total_system_u = 0, RAM_Used_system_u = 0;
            svcGetSystemInfo(&RAM_Total_system_u, 0, INVALID_HANDLE, 2);
            svcGetSystemInfo(&RAM_Used_system_u,  1, INVALID_HANDLE, 2);
            const u64 freeRamBytes = RAM_Total_system_u - RAM_Used_system_u;
            constexpr u64 kMinRequired = 5872026ULL; // 5.6 MB
            if (freeRamBytes < kMinRequired) {
                this->msg = "Not enough memory.";
                return;
            }
        }


        if (R_FAILED(pm::Initialize())) {
            this->msg  = "Failed pm::Initialize()";
            return;
        }
        Result rc = tuneInitialize();

        // not found can happen if the service isn't started
        // connection refused can happen is the service was terminated by pmshell
        if (R_VALUE(rc) == KERNELRESULT(NotFound) || R_VALUE(rc) == KERNELRESULT(ConnectionRefused)) {
            u64 pid = 0;
            const NcmProgramLocation programLocation{
                .program_id = 0x420000000000000F,
                .storageID  = NcmStorageId_None,
            };
            rc = pmshellInitialize();
            if (R_SUCCEEDED(rc)) {
                rc = pmshellLaunchProgram(0, &programLocation, &pid);
                pmshellExit();
            }
            if (R_FAILED(rc) || pid == 0) {
                this->fail = rc;
                this->msg  = "  Failed to\n"
                            "launch sysmodule";
                return;
            }

            /* The sysmodule has been launched, but its IPC port isn't
             * registered yet. Poll tuneInitialize() in short intervals
             * until either it succeeds, the budget is exhausted, or a
             * non-recoverable error code comes back.
             *
             * Budget : 300 ms total
             * Poll   : 10 ms (up to 30 attempts)
             *
             * On a fast/idle system this typically returns on the 1st
             * or 2nd retry (~10-20 ms). On a busy system the budget
             * gives the service ample time to publish before we fall
             * through to the "Something went wrong" path with the
             * last rc still in hand. */
            constexpr u64 kPollIntervalNs = 10'000'000ULL;   // 10 ms
            constexpr int kMaxAttempts    = 30;              // 300 ms total

            for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
                svcSleepThread(kPollIntervalNs);
                rc = tuneInitialize();
                if (R_SUCCEEDED(rc))
                    break;

                /* Only NotFound / ConnectionRefused mean "service not
                 * ready yet" — anything else is a real failure and
                 * additional polling won't change the outcome. */
                if (R_VALUE(rc) != KERNELRESULT(NotFound) &&
                    R_VALUE(rc) != KERNELRESULT(ConnectionRefused))
                    break;
            }
        }

        if (R_FAILED(rc)) {
            this->msg  = "Something went wrong:";
            this->fail = rc;
            return;
        }

        if (R_FAILED(sdmc::Open())) {
            this->msg  = "Failed sdmc::Open()";
            return;
        }

        config::ensure_language_config();
        reloadDferTuneTranslations();
        i18n::syncFromConfig();

        // initServices() runs right before Tesla creates the framebuffer, so this
        // is the moment to make the panel wider than Tesla's 448 px default.
        ult::DefaultFramebufferWidth = static_cast<u16>(config::get_overlay_width());

        u32 api = 0;
        if (R_FAILED(tuneGetApiVersion(&api)) || api != TUNE_API_VERSION) {
            /* The running sysmodule has a different API version (e.g. an old
             * build was loaded at boot).  Gracefully stop it and relaunch the
             * current version so the overlay can connect to a compatible IPC
             * endpoint.
             *
             * Steps:
             *  1. Ask the old sysmodule to quit via IPC.
             *  2. Close our IPC handle.
             *  3. Poll until the named port disappears (up to 500 ms).
             *  4. Relaunch via pmshell and poll until the port reappears.
             *  5. Re-check the API version; show an error only if it still
             *     doesn't match after the restart. */
            tuneQuit();
            tuneExit();

            /* Wait for the old sysmodule to fully terminate. */
            constexpr u64 kPollIntervalNs = 10'000'000ULL;  // 10 ms
            constexpr int kWaitAttempts   = 50;             // 500 ms total
            for (int i = 0; i < kWaitAttempts; ++i) {
                svcSleepThread(kPollIntervalNs);
                Result probe = tuneInitialize();
                if (R_VALUE(probe) == KERNELRESULT(NotFound) ||
                    R_VALUE(probe) == KERNELRESULT(ConnectionRefused)) {
                    tuneExit();
                    break;
                }
                tuneExit();
            }

            /* Relaunch the sysmodule. */
            u64 pid = 0;
            const NcmProgramLocation programLocation{
                .program_id = 0x420000000000000F,
                .storageID  = NcmStorageId_None,
            };
            rc = pmshellInitialize();
            if (R_SUCCEEDED(rc)) {
                rc = pmshellLaunchProgram(0, &programLocation, &pid);
                pmshellExit();
            }
            if (R_FAILED(rc) || pid == 0) {
                this->fail = rc;
                this->msg  = "  Failed to\n"
                            "relaunch sysmodule";
                return;
            }

            /* Poll until the new sysmodule's IPC port is ready. */
            constexpr int kRelaunchAttempts = 30;  // 300 ms total
            rc = KERNELRESULT(NotFound);
            for (int attempt = 0; attempt < kRelaunchAttempts; ++attempt) {
                svcSleepThread(kPollIntervalNs);
                rc = tuneInitialize();
                if (R_SUCCEEDED(rc))
                    break;
                if (R_VALUE(rc) != KERNELRESULT(NotFound) &&
                    R_VALUE(rc) != KERNELRESULT(ConnectionRefused))
                    break;
            }

            if (R_FAILED(rc)) {
                this->fail = rc;
                this->msg  = "Something went wrong:";
                return;
            }

            /* Final version check after restart. */
            api = 0;
            if (R_FAILED(tuneGetApiVersion(&api)) || api != TUNE_API_VERSION) {
                this->msg = "   Unsupported\n"
                            "DferTune version!";
            }
        }
    }

    void exitServices() override {
        sdmc::Close();
        pm::Exit();
        tuneExit();
    }

    std::unique_ptr<tsl::Gui> loadInitialGui() override {
        if (this->msg) {
            return std::make_unique<ErrorGui>(this->msg, this->fail);
        } else {
            return std::make_unique<MainGui>();
        }
    }
};

int main(int argc, char **argv) {
    return tsl::loop<SysTuneOverlay>(argc, argv);
}
