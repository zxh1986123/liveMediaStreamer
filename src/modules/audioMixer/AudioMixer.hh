/*
 *  AudioMixer - Audio mixer structure
 *  Copyright (C) 2014  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of media-streamer.
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
 *  Authors:  Marc Palau <marc.palau@i2cat.net>
 */
 
#ifndef _AUDIO_MIXER_HH
#define _AUDIO_MIXER_HH

#include "../../Frame.hh"
#include "../../Filter.hh"

class AudioMixer : public ManyToOneFilter {
    
    public:
        AudioMixer(int inputChannels);
        AudioMixer(int inputChannels, int frameChannels, int sampleRate);
        FrameQueue *allocQueue();
        bool doProcessFrame(std::map<int, Frame*> orgFrames, Frame *dst);

    private:
        void mixNonEmptyFrames(std::map<int, Frame*> orgFrames, std::vector<int> filledFramesIds, Frame *dst, int totalFrames); 
        void applyMixAlgorithm(std::vector<float> &fSamples, int frameNumber);
        void applyGainToChannel(std::vector<float> &fSamples, float gain);
        void sumValues(std::vector<float> fSamples, std::vector<float> &mixedSamples); 
        
        int frameChannels;
        int sampleRate;
        SampleFmt sampleFormat;
        std::map<int,float> gains;
        float masterGain;

        //Vectors as attributes in order to improve memory management
        std::vector<float> samples;
        std::vector<float> mixedSamples;
};


#endif