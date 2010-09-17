/*
 *  Copyright (C) 2006, 2007, 2008, 2009, 2010 Stephen F. Booth <me@sbooth.org>
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    - Neither the name of Stephen F. Booth nor the names of its 
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <AudioToolbox/AudioToolbox.h>
#include <vector>
#include <map>


// ========================================
// Forward declarations
// ========================================
class AudioDecoder;
class CARingBuffer;
class DecoderStateData;
class PCMConverter;


// ========================================
// Constants
// ========================================
#define kActiveDecoderArraySize 8


// ========================================
// Enums
// ========================================
enum {
	eAudioPlayerFlagIsSeeking				= 1 << 0,
	eAudioPlayerFlagMuteOutput				= 1 << 1
};

// ========================================
// An audio player class
// The player primarily uses two threads:
//  1) A decoding thread, which reads audio via an AudioDecoder instance and stores it in the ring buffer.
//     The audio is stored as deinterleaved, normalized [-1, 1) native floating point data in 64 bits (AKA doubles)
//  2) A rendering thread, which reads audio from the ring buffer and performs conversion to the required output format.
//     Sample rate conversion is done using Apple's AudioConverter API.
//     Final conversion to the stream's format is done using PCMConverter.
// ========================================
class AudioPlayer
{
	
public:
	
	// ========================================
	// Creation/Destruction
	AudioPlayer();
	~AudioPlayer();
	
	// ========================================
	// Playback Control
	void Play();
	void Pause();
	inline void PlayPause()							{ IsPlaying() ? Pause() : Play(); }
	void Stop();
	
	inline bool IsPlaying()							{ return mIsPlaying; }
	CFURLRef GetPlayingURL();

	// ========================================
	// Playback Properties
	SInt64 GetCurrentFrame();
	SInt64 GetTotalFrames();
	inline SInt64 GetRemainingFrames()				{ return GetTotalFrames() - GetCurrentFrame(); }
	
	CFTimeInterval GetCurrentTime();
	CFTimeInterval GetTotalTime();
	inline CFTimeInterval GetRemainingTime()		{ return GetTotalTime() - GetCurrentTime(); }
	
	// ========================================
	// Seeking
	bool SeekForward(CFTimeInterval secondsToSkip = 3);
	bool SeekBackward(CFTimeInterval secondsToSkip = 3);

	bool SeekToTime(CFTimeInterval timeInSeconds);
	bool SeekToFrame(SInt64 frame);
	
	bool SupportsSeeking();
	
	// ========================================
	// Player Parameters
	bool GetMasterVolume(Float32& volume);
	bool SetMasterVolume(Float32 volume);

	bool GetVolumeForChannel(UInt32 channel, Float32& volume);
	bool SetVolumeForChannel(UInt32 channel, Float32 volume);

	// ========================================
	// Device Management
	CFStringRef CreateOutputDeviceUID();
	bool SetOutputDeviceUID(CFStringRef deviceUID);
	
	AudioDeviceID GetOutputDeviceID()				{ return mOutputDeviceID; }
	bool SetOutputDeviceID(AudioDeviceID deviceID);

	bool GetOutputDeviceSampleRate(Float64& sampleRate);
	bool SetOutputDeviceSampleRate(Float64 sampleRate);

	bool OutputDeviceIsHogged();

	bool StartHoggingOutputDevice();
	bool StopHoggingOutputDevice();

	// ========================================
	// Stream Management
	bool GetOutputStreams(std::vector<AudioStreamID>& streams);
	
	bool GetOutputStreamVirtualFormat(AudioStreamID streamID, AudioStreamBasicDescription& virtualFormat);
	bool SetOutputStreamVirtualFormat(AudioStreamID streamID, const AudioStreamBasicDescription& virtualFormat);

	bool GetOutputStreamPhysicalFormat(AudioStreamID streamID, AudioStreamBasicDescription& physicalFormat);
	bool SetOutputStreamPhysicalFormat(AudioStreamID streamID, const AudioStreamBasicDescription& physicalFormat);

	// ========================================
	// Playlist Management
	// The player will take ownership of decoder
	bool Enqueue(CFURLRef url);
	bool Enqueue(AudioDecoder *decoder);
	
	bool ClearQueuedDecoders();

private:

	// ========================================
	// AudioHardware Utilities (for non-mixable audio)
	bool OpenOutput();
	bool CloseOutput();

	bool StartOutput();
	bool StopOutput();

	bool OutputIsRunning();
	bool ResetOutput();
	
	// ========================================
	// Other Utilities
	void StopActiveDecoders();
	
	DecoderStateData * GetCurrentDecoderState();
	DecoderStateData * GetDecoderStateStartingAfterTimeStamp(SInt64 timeStamp);

	bool CreateConvertersAndConversionBuffers();
	
	bool BuildVirtualFormatsCache();
	bool AddVirtualFormatPropertyListeners();
	bool RemoveVirtualFormatPropertyListeners();

	// ========================================
	// Data Members
	AudioDeviceID						mOutputDeviceID;
	AudioDeviceIOProcID					mOutputDeviceIOProcID;
	UInt32								mOutputDeviceBufferFrameSize;
	std::vector<AudioStreamID>			mOutputDeviceStreamIDs;
	std::map<AudioStreamID, AudioStreamBasicDescription> mStreamVirtualFormats;
	
	CARingBuffer						*mRingBuffer;
	AudioStreamBasicDescription			mRingBufferFormat;
//	AudioChannelLayout					mRingBufferChannelLayout;

	PCMConverter						**mOutputConverters;
	AudioConverterRef					mSampleRateConverter;
	AudioBufferList						*mSampleRateConversionBuffer;
	AudioBufferList						*mOutputBuffer;

	volatile uint32_t					mFlags;

	bool								mIsPlaying;

	CFMutableArrayRef					mDecoderQueue;
	DecoderStateData					*mActiveDecoders [kActiveDecoderArraySize];

	pthread_mutex_t						mMutex;
	
	pthread_t							mDecoderThread;
	semaphore_t							mDecoderSemaphore;
	bool								mKeepDecoding;
	
	pthread_t							mCollectorThread;
	semaphore_t							mCollectorSemaphore;
	bool								mKeepCollecting;

	int64_t								mFramesDecoded;
	int64_t								mFramesRendered;
	int64_t								mFramesRenderedLastPass;

public:

	// ========================================
	// Callbacks- for internal use only
	OSStatus Render(AudioDeviceID			inDevice,
					const AudioTimeStamp	*inNow,
					const AudioBufferList	*inInputData,
					const AudioTimeStamp	*inInputTime,
					AudioBufferList			*outOutputData,
					const AudioTimeStamp	*inOutputTime);
	
	OSStatus AudioObjectPropertyChanged(AudioObjectID						inObjectID,
										UInt32								inNumberAddresses,
										const AudioObjectPropertyAddress	inAddresses[]);

	OSStatus FillSampleRateConversionBuffer(AudioConverterRef				inAudioConverter,
											UInt32							*ioNumberDataPackets,
											AudioBufferList					*ioData,
											AudioStreamPacketDescription	**outDataPacketDescription);
	
	// ========================================
	// Thread entry points
	void * DecoderThreadEntry();
	void * CollectorThreadEntry();

};
