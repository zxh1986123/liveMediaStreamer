/*
 *  Filter.hh - Filter base classes
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
 *            Marc Palau <marc.palau@i2cat.net>
 *            Gerard Castillo <gerard.castillo@i2cat.net>
 */

#include "Filter.hh"
#include "Utils.hh"
#include <iostream>
#include <thread>
#include <chrono>

#define WALL_CLOCK_THRESHOLD 100000 //us
#define SLOW_MODIFIER 1.10
#define FAST_MODIFIER 0.90

BaseFilter::BaseFilter()
{
    frameTimeMod = 1;
    bufferStateFrameTimeMod = 1;
}

BaseFilter::~BaseFilter()
{
    for (auto it : readers) {
        delete it.second;
    }

    for (auto it : writers) {
        delete it.second;
    }

    readers.clear();
    writers.clear();
    oFrames.clear();
    dFrames.clear();
    rUpdates.clear();
}

void BaseFilter::setFrameTime(size_t fTime)
{
    frameTime = std::chrono::microseconds(fTime);
}

Reader* BaseFilter::getReader(int id) 
{
    if (readers.count(id) <= 0) {
        return NULL;
    }

    return readers[id];
}

Reader* BaseFilter::setReader(int readerID, FrameQueue* queue, bool sharedQueue)
{
    if (readers.size() >= getMaxReaders() || readers.count(readerID) > 0 ) {
        return NULL;
    }

    Reader* r = new Reader(sharedQueue);
    readers[readerID] = r;

    return r;
}

int BaseFilter::generateReaderID()
{
    if (maxReaders == 1) {
        return DEFAULT_ID;
    }

    int id = rand();

    while (readers.count(id) > 0) {
        id = rand();
    }

    return id;
}

int BaseFilter::generateWriterID()
{
    if (maxWriters == 1) {
        return DEFAULT_ID;
    }

    int id = rand();

    while (writers.count(id) > 0) {
        id = rand();
    }

    return id;
}

bool BaseFilter::demandOriginFrames()
{
    bool newFrame;
    bool someFrame = false;
    QueueState qState;

    for (auto it : readers) {
        if (!it.second->isConnected()) {
            it.second->disconnect();
            //NOTE: think about readers as shared pointers
            delete it.second;
            readers.erase(it.first);
            continue;
        }

        oFrames[it.first] = it.second->getFrame(qState, newFrame, force);

        if (oFrames[it.first] != NULL) {
            if (newFrame) {
                rUpdates[it.first] = true;
            } else {
                rUpdates[it.first] = false;
            }

            someFrame = true;

        } else {
            rUpdates[it.first] = false;
        }

    }

    if (qState == SLOW) {
        bufferStateFrameTimeMod = SLOW_MODIFIER;
    } else {
        bufferStateFrameTimeMod = FAST_MODIFIER;
    }

    return someFrame;
}

bool BaseFilter::demandDestinationFrames()
{
    bool newFrame = false;
    for (auto it : writers){
        if (!it.second->isConnected()){
            it.second->disconnect();
            delete it.second;
            writers.erase(it.first);
            continue;
        }

        dFrames[it.first] = it.second->getFrame(true);
        newFrame = true;
    }

    return newFrame;
}

void BaseFilter::addFrames()
{
    for (auto it : writers){
        if (it.second->isConnected()){
            it.second->addFrame();
        }
    }
}

void BaseFilter::removeFrames()
{
    for (auto it : readers){
        if (rUpdates[it.first]){
            it.second->removeFrame();
        }
    }
}

bool BaseFilter::connect(BaseFilter *R, int writerID, int readerID, bool slaveQueue)
{
    Reader* r;
    FrameQueue *queue;

    utils::debugMsg("slaveQueue Value: " + std::to_string(slaveQueue));
    if (writers.size() < getMaxWriters() && writers.count(writerID) <= 0) {
        writers[writerID] = new Writer();
        utils::debugMsg("New writer created " + std::to_string(writerID));
    }

    if (slaveQueue) {
        if (writers.count(writerID) > 0 && !writers[writerID]->isConnected()) {
            utils::errorMsg("Writer " + std::to_string(writerID) + " null or not connected");
            return false;
        }
    } else {
        if (writers.count(writerID) > 0 && writers[writerID]->isConnected()) {
            utils::errorMsg("Writer " + std::to_string(writerID) + " null or already connected");
            return false;
        }
    }

    if (R->getReader(readerID) && R->getReader(readerID)->isConnected()){
        return false;
    }

    if (slaveQueue) {
        queue = writers[writerID]->getQueue();
    } else {
        queue = allocQueue(writerID);
        utils::debugMsg("New queue allocated for writer " + std::to_string(writerID));
    }

    if (!(r = R->setReader(readerID, queue, slaveQueue))) {
        utils::errorMsg("Could not set the queue to the reader");
        return false;
    }

    if (!slaveQueue) {
        writers[writerID]->setQueue(queue);
    }

    return writers[writerID]->connect(r);
}

bool BaseFilter::connectOneToOne(BaseFilter *R, bool slaveQueue)
{
    int writerID = generateWriterID();
    int readerID = R->generateReaderID();
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::connectManyToOne(BaseFilter *R, int writerID, bool slaveQueue)
{
    int readerID = R->generateReaderID();
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::connectManyToMany(BaseFilter *R, int readerID, int writerID, bool slaveQueue)
{
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::connectOneToMany(BaseFilter *R, int readerID, bool slaveQueue)
{
    int writerID = generateWriterID();
    return connect(R, writerID, readerID, slaveQueue);
}

bool BaseFilter::disconnectWriter(int writerId)
{
    bool ret;
    if (writers.count(writerId) <= 0) {
        return false;
    }

    ret = writers[writerId]->disconnect();
    if (ret){
        writers.erase(writerId);
    }
    return ret;
}

bool BaseFilter::disconnectReader(int readerId)
{
    bool ret;
    if (readers.count(readerId) <= 0) {
        return false;
    }

    ret = readers[readerId]->disconnect();
    if (ret){
        readers.erase(readerId);
    }
    return ret;
}

void BaseFilter::disconnectAll()
{
    for (auto it : writers) {
        it.second->disconnect();
    }

    for (auto it : readers) {
        it.second->disconnect();
    }
}

//TODO: Delete
// bool BaseFilter::disconnect(BaseFilter *R, int writerId, int readerId)
// {
//     if (writers.count(writerId) <= 0) {
//         return false;
//     }
// 
//     Reader *r = R->getReader(readerId);
// 
//     if (!r) {
//         return false;
//     }
// 
//     writers[writerId]->disconnect(r);
//     dFrames.erase(writerId);
//     R->oFrames.erase(readerId);
// 
//     return true;
// }

void BaseFilter::processEvent()
{
    eventQueueMutex.lock();

    while(newEvent()) {

        Event e = eventQueue.top();
        std::string action = e.getAction();
        Jzon::Node* params = e.getParams();
        Jzon::Object outputNode;

        if (action.empty() || eventMap.count(action) <= 0) {
            outputNode.Add("error", "Error while processing event. Wrong action...");
            e.sendAndClose(outputNode);
            eventQueue.pop();
            break;
        }

        eventMap[action](params, outputNode);
        e.sendAndClose(outputNode);

        eventQueue.pop();
    }

    eventQueueMutex.unlock();
}

bool BaseFilter::newEvent()
{
    if (eventQueue.empty()) {
        return false;
    }

    Event tmp = eventQueue.top();
    if (!tmp.canBeExecuted(std::chrono::system_clock::now())) {
        return false;
    }

    return true;
}

void BaseFilter::pushEvent(Event e)
{
    eventQueueMutex.lock();
    eventQueue.push(e);
    eventQueueMutex.unlock();
}

void BaseFilter::getState(Jzon::Object &filterNode)
{
    eventQueueMutex.lock();
    filterNode.Add("type", utils::getFilterTypeAsString(fType));
    filterNode.Add("workerId", workerId);
    doGetState(filterNode);
    eventQueueMutex.unlock();
}


bool BaseFilter::hasFrames()
{
	if (!demandOriginFrames() || !demandDestinationFrames()) {
		return false;
	}

	return true;
}

//TODO: delete it
// bool BaseFilter::deleteReader(int id)
// {
//     if (readers.count(id) <= 0) {
//         return false;
//     }
// 
//     if (readers[id]->isConnected()) {
//         return false;
//     }
// 
//     delete readers[id];
//     readers.erase(id);
// 
//     return true;
// }

void BaseFilter::updateTimestamp()
{
    if (frameTime.count() == 0) {
        timestamp = wallClock;
        return;
    }

    timestamp += frameTime;

    lastDiffTime = diffTime;
    diffTime = wallClock - timestamp;

    if (diffTime.count() > WALL_CLOCK_THRESHOLD || diffTime.count() < (-WALL_CLOCK_THRESHOLD) ) {
        // reset timestamp value in order to realign with the wall clock
        utils::warningMsg("Wall clock deviations exceeded! Reseting values!");
        timestamp = wallClock;
        diffTime = std::chrono::microseconds(0);
        frameTimeMod = 1;
    }

    if (diffTime.count() > 0 && lastDiffTime < diffTime) {
        // delayed and incrementing delay. Need to speed up
        frameTimeMod -= 0.01;
    }

    if (diffTime.count() < 0 && lastDiffTime > diffTime) {
        // advanced and incremeting advance. Need to slow down
        frameTimeMod += 0.01;
    }

    if (frameTimeMod < 0) {
        frameTimeMod = 0;
    }
}

MasterFilter::MasterFilter() :
    BaseFilter()
{
}

void MasterFilter::processAll()
{   
    //TODO: update slaves frames
    for (auto it : slaves){
        it.second->execute();
    }
}

bool MasterFilter::runningSlaves()
{
    bool running = false;
    for (auto it : slaves){
        running |= it.second->isRunning();
    }
    return running;
}

size_t MasterFilter::processFrame()
{
    size_t enlapsedTime;
    size_t frameTime_;
    
    wallClock = std::chrono::duration_cast<std::chrono::microseconds>
        (std::chrono::system_clock::now().time_since_epoch());
        
    processEvent();

    if (!demandOriginFrames() || !demandDestinationFrames()) {
            return RETRY;
    }
    
    processAll();
    
    runDoProcessFrame();
    
    while (runningSlaves()){
        std::this_thread::sleep_for(std::chrono::microseconds(RETRY));
    }

    removeFrames();

    if (frameTime.count() == 0){
        return RETRY;
    }
    
    enlapsedTime = (std::chrono::duration_cast<std::chrono::microseconds>
        (std::chrono::system_clock::now().time_since_epoch()) - wallClock).count();
        
    frameTime_ = frameTime.count()*frameTimeMod*bufferStateFrameTimeMod;

    if (enlapsedTime > frameTime_){
        return 0;
    }

    return frameTime_ - enlapsedTime;
}

SlaveFilter::SlaveFilter() :
    BaseFilter()
{
}

OneToOneFilter::OneToOneFilter(size_t fTime, FilterRole fRole_, bool force_) :
    BaseFilter(), MasterFilter(), SlaveFilter()
{
    fRole = fRole_;
    force = force_;
    frameTime = std::chrono::microseconds(fTime);
    maxReaders = maxWriters = 1;
}



bool OneToOneFilter::runDoProcessFrame()
{   
    if (doProcessFrame(oFrames.begin()->second, dFrames.begin()->second)) {
        updateTimestamp();
        dFrames.begin()->second->setPresentationTime(timestamp);
        addFrames();
        return true;
    }
    
    return false;
}


OneToManyFilter::OneToManyFilter(unsigned writersNum, size_t fTime, FilterRole fRole_, bool force_) :
    BaseFilter(), MasterFilter(), SlaveFilter()
{
    fRole = fRole_;
    force = force_;
    frameTime = std::chrono::microseconds(fTime);
    maxReaders = 1;
    maxWriters = writersNum;
}

size_t OneToManyFilter::processFrame()
{   
    if (doProcessFrame(oFrames.begin()->second, dFrames)) {
        updateTimestamp();

        for (auto it : dFrames) {
            it.second->setPresentationTime(timestamp);
        }

        addFrames();
        return true;
    }

    return false;
}

HeadFilter::HeadFilter(unsigned writersNum, size_t fTime, FilterRole fRole_) :
    BaseFilter(), MasterFilter(), SlaveFilter()
{
    fRole = fRole_;
    force = false;
    frameTime = std::chrono::microseconds(fTime);
    maxReaders = 0;
    maxWriters = writersNum;
}

void HeadFilter::pushEvent(Event e)
{
    std::string action = e.getAction();
    Jzon::Node* params = e.getParams();
    Jzon::Object outputNode;

    if (action.empty()) {
        return;
    }

    if (eventMap.count(action) <= 0) {
        return;
    }

    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}



TailFilter::TailFilter(unsigned readersNum, size_t fTime, FilterRole fRole_) :
    BaseFilter(), MasterFilter(), SlaveFilter()
{
    fRole = fRole_;
    force = false;
    frameTime = std::chrono::microseconds(fTime);
    maxReaders = readersNum;
    maxWriters = 0;
}

void TailFilter::pushEvent(Event e)
{
    std::string action = e.getAction();
    Jzon::Node* params = e.getParams();
    Jzon::Object outputNode;

    if (action.empty()) {
        return;
    }

    if (eventMap.count(action) <= 0) {
        return;
    }

    eventMap[action](params, outputNode);
    e.sendAndClose(outputNode);
}


ManyToOneFilter::ManyToOneFilter(unsigned readersNum, size_t fTime, FilterRole fRole_, bool force_) :
    BaseFilter(), MasterFilter(), SlaveFilter()
{
    fRole = fRole_;
    force = force_;
    frameTime = std::chrono::microseconds(fTime);
    maxReaders = readersNum;
    maxWriters = 1;
}

size_t ManyToOneFilter::processFrame()
{
    if (doProcessFrame(oFrames, dFrames.begin()->second)) {
        updateTimestamp();
        dFrames.begin()->second->setPresentationTime(timestamp);
        addFrames();
        return true;
    }

    return false;
}
