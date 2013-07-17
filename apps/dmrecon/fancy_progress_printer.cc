#include "fancy_progress_printer.h"
#include "util/system.h"

char const *const ANSI_CURSOR_RESET = "\x1B[H";
char const *const ANSI_CLEAR_SCREEN = "\x1B[2J";

char const *const ANSI_STYLE_RESET = "\x1B[0m";
char const *const ANSI_STYLE_BOLD = "\x1B[1m";

char const *const ANSI_STYLE_BLACK = "\x1B[30m";
char const *const ANSI_STYLE_RED = "\x1B[31m";
char const *const ANSI_STYLE_GREEN = "\x1B[32m";
char const *const ANSI_STYLE_YELLOW = "\x1B[33m";
char const *const ANSI_STYLE_BLUE = "\x1B[34m";
char const *const ANSI_STYLE_MAGENTA = "\x1B[35m";
char const *const ANSI_STYLE_CYAN = "\x1B[36m";
char const *const ANSI_STYLE_WHITE = "\x1B[37m";

void*
FancyProgressPrinter::run()
{
    this->isRunning = true;
    while (this->isRunning)
    {
        {
            util::MutexLock lock(this->mutex);

            std::cout << ANSI_CURSOR_RESET << ANSI_CLEAR_SCREEN;
            std::cout << "Reconstructing " << this->basePath << "\n\n  ";

            for (std::vector<ViewStatus>::iterator it = this->viewStatus.begin();
                 it != this->viewStatus.end(); ++it)
            {
                switch (*it)
                {
                case STATUS_IGNORED:
                    std::cout << ANSI_STYLE_WHITE << '_';
                    break;
                case STATUS_QUEUED:
                    std::cout << ANSI_STYLE_WHITE << '.';
                    break;
                case STATUS_IN_PROGRESS:
                    std::cout << ANSI_STYLE_BOLD << ANSI_STYLE_YELLOW << '?';
                    break;
                case STATUS_DONE:
                    std::cout << ANSI_STYLE_BOLD << ANSI_STYLE_GREEN << '!';
                    break;
                }

                std::cout << ANSI_STYLE_RESET;
            }

            std::cout << "\n\n";

            for (std::set<mvs::DMRecon const*>::iterator it = this->runningRecons.begin();
                 it != this->runningRecons.end(); ++it)
            {
                mvs::Progress const &p = (*it)->getProgress();
                std::cout << "View #" << (*it)->getRefViewNr()
                          << ": filled " << p.filled
                          << " of " << (p.filled + p.queueSize) << '\n';
            }
        }

        util::system::sleep_sec(1.0f);
    }

    return NULL;
}
