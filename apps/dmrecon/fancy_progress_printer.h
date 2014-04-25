#ifndef DMRECON_FANCY_PROGRESS_PRINTER_H
#define DMRECON_FANCY_PROGRESS_PRINTER_H

#include "util/thread.h"
#include "dmrecon/dmrecon.h"

#include <set>
#include <string>

class FancyProgressPrinter;

class ProgressHandle
{
public:
    ProgressHandle(FancyProgressPrinter &progress_printer_,
                   mvs::Settings const &settings_);
    ~ProgressHandle();

    void setRecon(mvs::DMRecon const& recon_);

    void setDone()
    {
        this->done = true;
    }

private:
    FancyProgressPrinter & progress_printer;
    mvs::Settings const &settings;
    mvs::DMRecon const *recon;
    bool done;
};

/* -------------------------------------------------------------- */

class FancyProgressPrinter : public util::Thread
{
    friend class ProgressHandle;

public:
    void setBasePath(std::string _basePath);
    void setNumViews(int _numViews);

    template<class T>
    void addRefViews(T const& views);
    void addRefView(int viewID);

    virtual void* run();
    void stop();

private:
    enum ViewStatus
    {
        STATUS_IGNORED,
        STATUS_QUEUED,
        STATUS_IN_PROGRESS,
        STATUS_DONE,
        STATUS_FAILED
    };

private:
    void setStatus(int refViewNr, ViewStatus status);
    void insertRecon(mvs::DMRecon const *ptr);
    void eraseRecon(mvs::DMRecon const *ptr);

private:
    util::Mutex mutex;
    std::string basePath;
    bool isRunning;

    std::vector<ViewStatus> viewStatus;
    std::set<mvs::DMRecon const *> runningRecons;
};

/* -------------------------------------------------------------- */

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
    this->viewStatus.at(viewID) = STATUS_QUEUED;
}

template<class T>
inline void
FancyProgressPrinter::addRefViews(T const& views)
{
    util::MutexLock lock(this->mutex);
    for (typename T::const_iterator it = views.begin(); it != views.end(); ++it)
    {
        if (static_cast<std::size_t>(*it) < this->viewStatus.size())
            this->viewStatus[*it] = STATUS_QUEUED;
    }
}

inline void
FancyProgressPrinter::insertRecon(mvs::DMRecon const *ptr)
{
    util::MutexLock lock(mutex);
    runningRecons.insert(ptr);
}

inline void
FancyProgressPrinter::eraseRecon(mvs::DMRecon const *ptr)
{
    util::MutexLock lock(this->mutex);
    this->runningRecons.erase(ptr);
}

inline void
FancyProgressPrinter::stop()
{
    util::MutexLock lock(this->mutex);
    this->isRunning = false;
}

inline void
FancyProgressPrinter::setStatus (int refViewNr, ViewStatus status)
{
    viewStatus.at(refViewNr) = status;
}

inline
ProgressHandle::ProgressHandle(FancyProgressPrinter &progress_printer_,
                               const mvs::Settings &settings_)
    : progress_printer(progress_printer_), settings(settings_), done(false)
{
    this->progress_printer.setStatus(this->settings.refViewNr,
        FancyProgressPrinter::STATUS_IN_PROGRESS);
}

inline
void ProgressHandle::setRecon(const mvs::DMRecon &recon_)
{
    this->recon = &recon_;
    this->progress_printer.insertRecon(this->recon);
}

inline
ProgressHandle::~ProgressHandle()
{
    if (done)
        this->progress_printer.setStatus(settings.refViewNr,
            FancyProgressPrinter::STATUS_DONE);
    else
        this->progress_printer.setStatus(settings.refViewNr,
            FancyProgressPrinter::STATUS_FAILED);

    if (recon != NULL)
        this->progress_printer.eraseRecon(recon);
}

#endif
