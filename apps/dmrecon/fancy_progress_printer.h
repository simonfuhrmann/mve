#ifndef DMRECON_FANCY_PROGRESS_PRINTER_H
#define DMRECON_FANCY_PROGRESS_PRINTER_H

#include "util/thread.h"
#include "dmrecon/DMRecon.h"

#include <set>
#include <string>

class FancyProgressPrinter : public util::Thread
{
public:
    void setBasePath(std::string _basePath);
    void setNumViews(int _numViews);

    template<class T>
    void addRefViews(T const& views);

    void addRefView(int viewID);

    void insertRecon(mvs::DMRecon const *ptr);
    void eraseRecon(mvs::DMRecon const *ptr);

    virtual void* run();
    void stop();

private:
    enum ViewStatus {
        STATUS_IGNORED, STATUS_QUEUED, STATUS_IN_PROGRESS, STATUS_DONE
    };

    util::Mutex mutex;
    std::string basePath;
    bool isRunning;

    std::vector<ViewStatus> viewStatus;
    std::set<mvs::DMRecon const *> runningRecons;
};

inline void
FancyProgressPrinter::setBasePath(std::string _basePath)
{
    util::MutexLock lock(this->mutex);
    this->basePath = _basePath;
}

inline void
FancyProgressPrinter::setNumViews(int numViews)
{
    util::MutexLock lock(this->mutex);
    this->viewStatus.resize(numViews, STATUS_IGNORED);
}

inline void
FancyProgressPrinter::addRefView(int viewID)
{
    util::MutexLock lock(this->mutex);
    this->viewStatus[viewID] = STATUS_QUEUED;
}

template<class T>
inline void
FancyProgressPrinter::addRefViews(T const& views)
{
    util::MutexLock lock(this->mutex);
    for (typename T::const_iterator it = views.begin(); it != views.end(); ++it)
        this->viewStatus[*it] = STATUS_QUEUED;
}

inline void
FancyProgressPrinter::insertRecon(mvs::DMRecon const *ptr)
{
    util::MutexLock lock(mutex);
    runningRecons.insert(ptr);
    this->viewStatus[ptr->getRefViewNr()] = STATUS_IN_PROGRESS;
}

inline void
FancyProgressPrinter::eraseRecon(mvs::DMRecon const *ptr)
{
    util::MutexLock lock(this->mutex);
    this->runningRecons.erase(ptr);
    this->viewStatus[ptr->getRefViewNr()] = STATUS_DONE;
}

inline void
FancyProgressPrinter::stop()
{
    util::MutexLock lock(this->mutex);
    this->isRunning = false;
}


#endif
