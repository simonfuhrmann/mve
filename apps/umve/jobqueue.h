/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#ifndef JOB_QUEUE_HEADER
#define JOB_QUEUE_HEADER

#include <QBoxLayout>
#include <QDockWidget>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <vector>

struct JobProgress
{
    virtual ~JobProgress (void) {}
    virtual char const* get_name (void) = 0;
    virtual char const* get_message (void) = 0;
    virtual bool is_completed (void) = 0;
    virtual bool has_progress (void) = 0;
    virtual float get_progress (void) = 0;
    virtual void cancel_job (void) = 0;
};

/* ---------------------------------------------------------------- */

struct JobQueueEntry
{
    JobProgress* progress;
    QListWidgetItem* item;
    int finished;

    JobQueueEntry (void) : progress(nullptr), item(nullptr), finished(0) {}
};

/* ---------------------------------------------------------------- */

class JobQueue : public QWidget
{
    Q_OBJECT

/* Singleton implementation. */
public:
    static JobQueue* get (void);

protected slots:
    void on_update (void);
    void on_item_activated(QListWidgetItem* item);
    void add_fake_job (void);
    void update_job (JobQueueEntry& job);

protected:
    JobQueue (void);

private:
    typedef std::vector<JobQueueEntry> JobQueueList;

    QListWidget* jobs_list;
    QDockWidget* dock;
    QTimer* update_timer;
    JobQueueList jobs;

public:
    /* Note: job object is deleted when queue entry is removed. */
    void add_job (JobProgress* job);

    /* QT stuff. */
    QSize sizeHint (void) const;
    void set_dock_widget (QDockWidget* dock);

    bool is_empty (void) const;
};

/* -------------------------- Implementation ---------------------- */

inline QSize
JobQueue::sizeHint (void) const
{
    return QSize(175, 0);
}

inline void
JobQueue::set_dock_widget (QDockWidget* dock)
{
    this->dock = dock;
}

inline bool
JobQueue::is_empty (void) const
{
    bool empty = true;
    for (std::size_t i = 0; i < this->jobs.size(); ++i)
        if (this->jobs[i].finished == 0)
            empty = false;
    return empty;
}

#endif /* JOB_QUEUE_HEADER */
