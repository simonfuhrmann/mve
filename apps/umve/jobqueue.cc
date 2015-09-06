/*
 * Copyright (C) 2015, Simon Fuhrmann
 * TU Darmstadt - Graphics, Capture and Massively Parallel Computing
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms
 * of the BSD 3-Clause license. See the LICENSE.txt file for details.
 */

#include <sstream>
#include <iostream>
#include <QThreadPool>

#include "util/string.h"
#include "jobqueue.h"

JobQueue*
JobQueue::get (void)
{
    static JobQueue* instance = new JobQueue();
    return instance;
}

/* ---------------------------------------------------------------- */

JobQueue::JobQueue (void)
{
    this->jobs_list = new QListWidget();
    this->dock = nullptr;
    //QThreadPool::globalInstance()->setMaxThreadCount(2);

    /* Header of the list. */
    QPushButton* fake_but = new QPushButton(tr("Add fake job"));

    /* Main layout, header and scroll area. */
    QBoxLayout* main_layout = new QVBoxLayout();
    main_layout->setSpacing(0);
    main_layout->setContentsMargins(0, 0, 0, 0);
    //main_layout->addWidget(fake_but, 0); // Uncomment for "add fake job" button
    //main_layout->addLayout(label_layout);
    main_layout->addWidget(this->jobs_list, 1);
    this->setLayout(main_layout);

    /* Connect and setup timer. */
    this->connect(fake_but, SIGNAL(clicked()), this, SLOT(add_fake_job()));
    this->update_timer = new QTimer(this);

    this->connect(this->update_timer, SIGNAL(timeout()),
        this, SLOT(on_update()));
    this->connect(this->jobs_list, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
        this, SLOT(on_item_activated(QListWidgetItem*)));

    this->update_timer->start(1000); // Every sec
}

/* ---------------------------------------------------------------- */

void
JobQueue::add_job (JobProgress* job)
{
    std::stringstream list_label;
    list_label << job->get_name() << "\n" << "Starting...";

    JobQueueEntry entry;
    entry.progress = job;
    entry.item = new QListWidgetItem(QIcon(":/images/icon_exec.svg"), "");
    //entry.item->setData(Qt::UserRole, (int)this->jobs.size());
    this->jobs.push_back(entry);
    this->update_job(this->jobs.back());
    this->jobs_list->addItem(entry.item);
}

/* ---------------------------------------------------------------- */

struct FakeJob : public JobProgress
{
    int i;

    FakeJob (void) : i(5) {}
    char const* get_name (void) { return "Fake Job"; }
    bool is_completed (void) { i = (i <= 0 ? 0 : i-1); return !i; }
    bool has_progress (void) { return false; }
    float get_progress (void) { return 0.0f; }
    char const* get_message (void) { return "Faking..."; }
    void cancel_job (void) { i = 0; }
};

void
JobQueue::add_fake_job (void)
{
    this->add_job(new FakeJob);
}

/* ---------------------------------------------------------------- */

void
JobQueue::on_update (void)
{
    /* Erase completed jobs. */
    for (JobQueueList::iterator i = this->jobs.begin(); i != this->jobs.end();)
    {
        if (i->finished == 10)
        {
            delete i->progress;
            delete i->item;
            i = this->jobs.erase(i);
            continue;
        }
        else
        {
            i += 1;
        }
    }

    /* Update job list. */
    for (std::size_t i = 0; i < this->jobs.size(); ++i)
        this->update_job(this->jobs[i]);

    /* Update labels. */
    if (this->dock)
    {
        int atc = QThreadPool::globalInstance()->activeThreadCount();
        int mtc = QThreadPool::globalInstance()->maxThreadCount();
        this->dock->setWindowTitle(tr("Jobs (%1/%2)").arg(atc).arg(mtc));
    }
}

/* ---------------------------------------------------------------- */

void
JobQueue::on_item_activated (QListWidgetItem* item)
{
    //std::size_t id = (std::size_t)item->data(Qt::UserRole).toInt();
    //std::cout << "Item " << item << " was selected!" << std::endl;
    for (std::size_t i = 0; i < this->jobs.size(); ++i)
        if (this->jobs[i].item == item)
        {
            this->jobs[i].progress->cancel_job();
            break;
        }
}

/* ---------------------------------------------------------------- */

void
JobQueue::update_job (JobQueueEntry& job)
{
    std::stringstream list_label;
    list_label << job.progress->get_name();
    if (job.progress->has_progress())
    {
        float progress = job.progress->get_progress() * 100.0f;
        list_label << " (" << util::string::get_fixed(progress, 2) << "%)";
    }
    list_label << "\n" << job.progress->get_message();
    job.item->setText(list_label.str().c_str());

    if (job.progress->is_completed())
        job.finished += 1;
}
