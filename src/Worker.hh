/*
 *  Worker.cpp - Is the owner of a Processor thread
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of liveMediaStreamer.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  David Cassany <david.cassany@i2cat.net>,
 *  		  Martin German <martin.german@i2cat.net>
 *            
 */

#ifndef _WORKER_HH
#define _WORKER_HH

#include <thread>
#include <map>
#include <chrono>
#include <mutex>
#include <queue>
#include <vector>

#include "Frame.hh"
#include "Types.hh"
#include "Jzon.h"
#include "Utils.hh"

#define MAX_SLAVE 16

class Runnable {
    
public:
    ~Runnable(){};
    bool processFrame();
    virtual void processEvent() = 0;
    virtual void removeFrames() = 0;
    virtual bool hasFrames() = 0;
    virtual bool isEnabled() = 0;
    virtual void stop() = 0;
    bool ready();
    void sleepUntilReady();
    int getId() {return id;};
    void setId(int id_) {id = id_;};
    bool operator()(const Runnable* lhs, const Runnable* rhs);
    std::chrono::system_clock::time_point getTime() const {return time;};
    
protected:
    virtual int64_t doProcessFrame(bool removeFrame = true) = 0;
    std::chrono::system_clock::time_point time;
    
private:
    int id;
};

struct RunnableLess : public std::binary_function<Runnable*, Runnable*, bool>                                                                                     
{
  bool operator()(const Runnable* lhs, const Runnable* rhs) const
  {
    return lhs->getTime() < rhs->getTime();
  }
};

class Worker {
    
public:
    Worker(Runnable *processor_);
    Worker();
    virtual ~Worker();
    
    bool start();
    bool isRunning();
    virtual void stop();
    virtual void enable();
    virtual void disable();
    bool isEnabled();
    bool addProcessor(int id, Runnable *processor);
    bool removeProcessor(int id);
    WorkerType getType(){return type;};
    std::priority_queue<Runnable*, std::vector<Runnable*>, RunnableLess> getProcessors(){return processors;};
    void getState(Jzon::Object &workerNode);
    
protected:
    virtual void process() = 0;

    std::priority_queue<Runnable*, std::vector<Runnable*>, RunnableLess> processors;
    std::thread thread;
    bool run;
    bool enabled;

    WorkerType type;
    std::mutex mtx;
};

class LiveMediaWorker : public Worker {
public:
    LiveMediaWorker();
    void enable() {};
    void disable() {};
    void stop();
private:
    void process();
};

#endif
