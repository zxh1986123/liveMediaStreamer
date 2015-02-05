/*
 *  AudioDecoderLibav - A libav-based audio decoder
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
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

#ifndef _AUDIO_DECODER_LIBAV_HH
#define _AUDIO_DECODER_LIBAV_HH

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libswresample/swresample.h>
}

#include "../../AudioFrame.hh"
#include "../../FrameQueue.hh"
#include "../../Filter.hh"


class AudioDecoderLibav : public OneToOneFilter {

public:
    AudioDecoderLibav(FilterRole fRole_ = MASTER);
    ~AudioDecoderLibav();
    bool doProcessFrame(Frame *org, Frame *dst);
    FrameQueue* allocQueue(int wId);
    bool configure(SampleFmt sampleFormat, int channels, int sampleRate);

private:

    void initializeEventMap();
    bool resample(AVFrame* src, AudioFrame* dst);
    void checkInputParams(int sampleFormat, int channels, int sampleRate);
    bool inputConfig();
    bool outputConfig();
    bool reconfigureDecoder(AudioFrame* frame);
    void configEvent(Jzon::Node* params, Jzon::Object &outputNode);
    void doGetState(Jzon::Object &filterNode);



    AVCodec             *codec;
    AVCodecContext      *codecCtx;
    AVFrame             *inFrame;
    AVPacket            pkt;
    int                 gotFrame;
    SwrContext          *resampleCtx;
    AVCodecID           codecID;
    AVSampleFormat      inLibavSampleFmt;
    AVSampleFormat      outLibavSampleFmt;

    ACodecType          fCodec;
    SampleFmt           inSampleFmt;
    SampleFmt           outSampleFmt;
    int                 inChannels;
    int                 outChannels;
    int                 inSampleRate;
    int                 outSampleRate;
    unsigned int        bytesPerSample;
    unsigned char       *auxBuff[1];

};

#endif
