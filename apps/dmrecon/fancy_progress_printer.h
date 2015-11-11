/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef DMRECON_FANCY_PROGRESS_PRINTER_H
#define DMRECON_FANCY_PROGRESS_PRINTER_H

#include <set>
#include <string>
#include <thread>
#include <mutex>

#include "dmrecon/dmrecon.h"

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

class FancyProgressPrinter
{
    friend class ProgressHandle;

public:
    void setBasePath(std::string _basePath);
    void setNumViews(std::size_t _numViews);

    template<class T>
    void addRefViews(T const& views);
    void addRefView(std::size_t viewID);

    void run();
    void stop();
    void print();
    void start();

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
    void setStatus(std::size_t refViewNr, ViewStatus status);
    void insertRecon(mvs::DMRecon const *ptr);
    void eraseRecon(mvs::DMRecon const *ptr);

private:
    std::thread thread;
    std::mutex mutex;
    std::string basePath;
    bool isRunning;

    std::vector<ViewStatus> viewStatus;
    std::set<mvs::DMRecon const *> runningRecons;
};

/* -------------------------------------------------------------- */

inline void
FancyProgressPrinter::setBasePath(std::string _basePath)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->basePath = _basePath;
}

inline void
FancyProgressPrinter::setNumViews(std::size_t numViews)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->viewStatus.resize(numViews, STATUS_IGNORED);
}

inline void
FancyProgressPrinter::addRefView(std::size_t viewID)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->viewStatus.at(viewID) = STATUS_QUEUED;
}

template<class T>
inline void
FancyProgressPrinter::addRefViews(T const& views)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    for (typename T::const_iterator it = views.begin(); it != views.end(); ++it)
    {
        if (static_cast<std::size_t>(*it) < this->viewStatus.size())
            this->viewStatus[*it] = STATUS_QUEUED;
    }
}

inline void
FancyProgressPrinter::insertRecon(mvs::DMRecon const *ptr)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    runningRecons.insert(ptr);
}

inline void
FancyProgressPrinter::eraseRecon(mvs::DMRecon const *ptr)
{
    std::lock_guard<std::mutex> lock(this->mutex);
    this->runningRecons.erase(ptr);
}

inline void
FancyProgressPrinter::stop()
{
    {
        std::lock_guard<std::mutex> lock(this->mutex);
        this->isRunning = false;
    }
    this->print();
}

inline void
FancyProgressPrinter::setStatus (std::size_t refViewNr, ViewStatus status)
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

    if (recon != nullptr)
        this->progress_printer.eraseRecon(recon);
}

#endif
