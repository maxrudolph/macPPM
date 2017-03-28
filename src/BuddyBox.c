//
//  BuddyBox.c
//  BuddyBox
//
//  Created by Nicholas Robinson on 12/23/12.
//  Copyright (c) 2012 Nicholas Robinson. All rights reserved.
//

#include "BuddyBox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

void initializeBuddyBox(BuddyBox *bb, unsigned int sampleRate)
{
  int i;
  printf("BuddyBox:\tInitializing.\n");
  
  bb->active                      = 1;
  bb->negativeShift               = 0;
  bb->sampleRate                  = sampleRate;
  bb->input                       = SIGNAL_LOW;
  bb->lastInput                   = SIGNAL_LOW;
  bb->inputChannel                = 0;
  bb->inputChannelCount           = 8;
  bb->outputChannelCount          = 0;
  bb->inputSampleCount            = 0;
  bb->outputSampleCount           = 0;
  bb->elapsedInputSampleCounts    = 0;
  bb->maxElapsedInputSampleCount  = 0;
  bb->lastInputEdgeSampleCount    = 0;
  bb->inputSynchroFrameCount      = 0;
  bb->badInputFrameCount          = 0;
  bb->outputOverflowSampleCount   = 0;
  for (i = 0; i < MAX_CHANNELS; i++)
    bb->inputChannelBuffer[i]   = 0;
  for (i = 0; i < MAX_CHANNELS; i++)
    bb->outputChannelBuffer[i]  = 0;
  bb->minInputSample              = -1.0f;
  bb->maxInputSample              = 1.0f;
  for (i = 0; i < MAX_CHANNELS; i++)
    bb->inputChannelValues[i]  = 0.0f;
  allocateOutputOverflowBuffer(bb);
}

void allocateOutputOverflowBuffer(BuddyBox *bb)
{
  bb->outputOverflowBufferSize = OVERFLOW_SAMPLES;
  bb->outputOverflowBuffer = (float *) malloc(bb->outputOverflowBufferSize * sizeof(float));
  if(bb->outputOverflowBuffer == NULL)
    {
      printf("BuddyBox:\tCould Not Allocate Output Overflow Buffer.\n");
      exit(1);
    }
  memset(bb->outputOverflowBuffer, 0, bb->outputOverflowBufferSize * sizeof(float));
}

void readBufferIntoBuddyBoxInputChannelBuffer(BuddyBox *bb, float* buffer, unsigned int bufferSize)
{
  float localMinSample, localMaxSample;
  unsigned int i, localMaxElapsedCount, localNegativeShift;
  
  //detectBuddyBoxInputTimeout(bb, buffer, bufferSize);
  float min=0.0;
  float max=0.0;
  //for( i=0;i<bufferSize;i++){
  //  if(buffer[i] > max) max = buffer[i];
  //  if(buffer[i] < min) min = buffer[i];
  //}
  //printf("buffer min,max = %f,%f\n",min,max);fflush(stdout);
  localMinSample = 0.0f;
  localMaxSample = 0.0f;
  localMaxElapsedCount = 0;
  localNegativeShift = 0;
  int in_frame=0;
  int in_pulse=0;
  int pulse_start=0;
  int in_sep=0;
  float channel_durations[8];
  float avg_channel_durations[8] = {0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0};
  int channel=7;
  const float min_duration = CHANNEL_MIN_DURATION;
  const float max_duration = CHANNEL_MAX_DURATION;
  const float min_sep = 375.0;
  const float threshold = -0.9;
  int nread=0;
  //for (i = 0; bb->active && i < bufferSize; i++, bb->inputSampleCount++)
  for (i = bufferSize-1; bb->active && i >0 ; i--, bb->inputSampleCount++)
    {
      /* look for ppm reset code */
      if( in_pulse ){
	if(buffer[i] > threshold){//still in pulse
	  
	}else{//no longer in pulse
	  int pulse_duration = i-pulse_start;
	  float pulse_duration_ms = -pulse_duration*((float) 1.0e6/bb->sampleRate);
	  //printf("pulse duration %d,%f\n",pulse_duration,pulse_duration_ms);fflush(stdout);
	  in_pulse=0;	   
	  if( in_frame ){
	    if( pulse_duration_ms >= min_duration ){
	      // save pulse duration
	      channel_durations[channel] = pulse_duration_ms;
	      channel--;
	    }
	  }else if( pulse_duration_ms > 5000.0 ){//this is /was the ppm reset code
	    //printf("in frame now\n"); fflush(stdout);
	    in_frame=1;
	    channel=7;
	  }
	}
      }else{// not in pulse
	if(buffer[i] > threshold ){//entering pulse
	  in_pulse=1;
	  pulse_start = i;
	}	
      }
      if( channel < 0 ){
	//printf("pulse durations:");
	int j;
	for(j=0;j<8;j++){
	  channel_durations[j] = channel_durations[j] > CHANNEL_MAX_DURATION ? CHANNEL_MAX_DURATION : channel_durations[j];
	  channel_durations[j] = channel_durations[j] < CHANNEL_MIN_DURATION ? CHANNEL_MIN_DURATION : channel_durations[j];
	  bb->inputChannelValues[j] = (float) (channel_durations[j] - min_duration) / (max_duration - min_duration);

	}
	//printf("\n");
	in_frame=0;
	in_pulse=0;
	channel=0;
	nread++;
	return;
      }
    }

}
  
void detectBuddyBoxInputTimeout(BuddyBox *bb, float* buffer, unsigned int bufferSize)
{
  unsigned int i;
  for (i = 0; i < bufferSize; i++)
    if (isBuddyBoxRawInputAboveNoiseThreshold(buffer[i]))
      return;
  if (!isBuddyBoxInputCalibrating(bb))
    handleBuddyBoxInputTimeout(bb);
}

unsigned int isBuddyBoxRawInputAboveNoiseThreshold(float bufferSample)
{
  return (getBuddyBoxSampleMagnitude(bufferSample) > SIGNAL_NOISE_THRESHOLD);
}

void handleBuddyBoxInputTimeout(BuddyBox *bb)
{
  printf("BuddyBox:\tInput Timeout.\n");
  disconnectBuddyBox(bb);
}

float getBuddyBoxSampleMagnitude(float sample)
{
  return fabs(sample);
}

float getBuddyBoxLocalMinSample(float bufferSample, float localMinSample)
{
  return (bufferSample < localMinSample) ? bufferSample : localMinSample;
}

float getBuddyBoxLocalMaxSample(float bufferSample, float localMaxSample)
{
  return (bufferSample > localMaxSample) ? bufferSample : localMaxSample;
}

float getBuddyBoxLocalMaxElapsedInputSampleCount(BuddyBox *bb, unsigned int localMaxElapsedCount, unsigned int bufferSize)
{
  return (bb->elapsedInputSampleCounts > localMaxElapsedCount && bb->elapsedInputSampleCounts < bufferSize) ? bb->elapsedInputSampleCounts : localMaxElapsedCount;
}

unsigned int getBuddyBoxLocalNegativeShift(BuddyBox *bb, unsigned int localMaxElapsedCount, float bufferSample)
{
  return (localMaxElapsedCount > bb->maxElapsedInputSampleCount) ? (bufferSample > 0.0f) : bb->negativeShift;
}

void processBuddyBoxRawInput(BuddyBox *bb, float bufferSample)
{
  bb->lastInput = bb->input;
  bb->input = isBuddyBoxRawInputHigh(bb, bufferSample) ? SIGNAL_HIGH : SIGNAL_LOW;
}

unsigned int isBuddyBoxInputEdge(BuddyBox *bb, float bufferSample)
{
  return (bb->input != bb->lastInput);
}

unsigned int isBuddyBoxRawInputHigh(BuddyBox *bb, float bufferSample)
{
  if (bb->negativeShift)
    return (isBuddyBoxRawInputAboveNoiseThreshold(bufferSample) && bufferSample < (bb->maxInputSample + bb->minInputSample) / 2);
  else
    return (isBuddyBoxRawInputAboveNoiseThreshold(bufferSample) && bufferSample > (bb->maxInputSample + bb->minInputSample) / 2);
}

void processBuddyBoxInputEdge(BuddyBox *bb)
{
  updateBuddyBoxElapsedInputSampleCounts(bb);
  if (isBuddyBoxInputHigh(bb))
    processHighBuddyBoxInput(bb);
  bb->lastInputEdgeSampleCount = bb->inputSampleCount;
}

void updateBuddyBoxElapsedInputSampleCounts(BuddyBox *bb)
{
  if (bb->lastInputEdgeSampleCount > bb->inputSampleCount)
    bb->elapsedInputSampleCounts = bb->inputSampleCount + (MAXUINT - bb->lastInputEdgeSampleCount);
  else
    bb->elapsedInputSampleCounts = bb->inputSampleCount - bb->lastInputEdgeSampleCount;
}

unsigned int isBuddyBoxInputHigh(BuddyBox *bb)
{
  return (bb->input == SIGNAL_HIGH);
}

void processHighBuddyBoxInput(BuddyBox *bb)
{
  if (isBuddyBoxInputSynchroFrame(bb))
    processBuddyBoxInputSynchroFrame(bb);
  else
    {
      if (isBuddyBoxInputChannelValid(bb))
	storeBuddyBoxInputChannel(bb);
      else if (!isBuddyBoxInputCalibrating(bb))
	handleInvalidBuddyBoxInputChannel(bb);
      targetNextBuddyBoxInputChannel(bb);
    }
}

unsigned int isBuddyBoxInputSynchroFrame(BuddyBox *bb)
{
  //return (bb->inputChannel >= MIN_CHANNELS && (bb->elapsedInputSampleCounts > bb->maxElapsedInputSampleCount / 2));
  return 0;
}

void processBuddyBoxInputSynchroFrame(BuddyBox *bb)
{
  bb->inputSynchroFrameCount++;
  if (!isBuddyBoxInputCalibrating(bb))
    {
      if (!isBuddyBoxInputChannelCountValid(bb))
	handleInvalidBuddyBoxInputChannelCount(bb);
      else
	processBuddyBoxInputFrame(bb);
    }
  else if (isBuddyBoxInputChannelValid(bb))
    storeBuddyBoxInputChannelCount(bb);
  targetNextBuddyBoxInputFrame(bb);
}

unsigned int isBuddyBoxInputChannelCountValid(BuddyBox *bb)
{
  return (bb->inputChannel == bb->inputChannelCount);
}

void storeBuddyBoxInputChannelCount(BuddyBox *bb)
{
  bb->inputChannelCount = bb->inputChannel;
}

void handleInvalidBuddyBoxInputChannelCount(BuddyBox *bb)
{
  bb->badInputFrameCount++;
  if (!isBuddyBoxInputViable(bb))
    {
      printf("BuddyBox:\tInput Channel Count Changed from %d to %d.\n", bb->inputChannelCount, bb->inputChannel);
      disconnectBuddyBox(bb);
    }   
}

unsigned int isBuddyBoxInputViable(BuddyBox *bb)
{
  return (bb->badInputFrameCount < BAD_FRAME_THRESHOLD);
}

void targetNextBuddyBoxInputFrame(BuddyBox *bb)
{
  bb->inputChannel = 0;
}

unsigned int isBuddyBoxInputChannelValid(BuddyBox *bb)
{
  return (bb->inputChannel < MAX_CHANNELS);
}

void processBuddyBoxInputFrame(BuddyBox *bb)
{
  unsigned int i, chanelDuration;

  for (i = 0; i < bb->inputChannelCount; i++)
    {
      chanelDuration = bb->inputChannelBuffer[i] * MICROSECONDS_PER_SECOND / bb->sampleRate;
      if (chanelDuration < CHANNEL_MIN_DURATION)
	bb->inputChannelValues[i] = 0.0f;
      else if (chanelDuration > CHANNEL_MAX_DURATION)
	bb->inputChannelValues[i] = 1.0f;
      else
	bb->inputChannelValues[i] = (float) (chanelDuration - CHANNEL_MIN_DURATION) / (CHANNEL_MAX_DURATION - CHANNEL_MIN_DURATION);
    }
  bb->badInputFrameCount = 0;
}

void storeBuddyBoxInputChannel(BuddyBox *bb)
{
  bb->inputChannelBuffer[bb->inputChannel] = bb->elapsedInputSampleCounts;
}

void handleInvalidBuddyBoxInputChannel(BuddyBox *bb)
{
  bb->badInputFrameCount++;
  if (!isBuddyBoxInputViable(bb))
    {
      printf("BuddyBox:\tInvalid Input Channel Received.\n");
      disconnectBuddyBox(bb);
    }
  targetNextBuddyBoxInputFrame(bb);
}

void targetNextBuddyBoxInputChannel(BuddyBox *bb)
{
  bb->inputChannel++;
}

unsigned int isBuddyBoxInputCalibrating(BuddyBox *bb)
{
  //  return (bb->inputSynchroFrameCount < CALIBRATION_FRAMES);
  return ((unsigned int) 0);
}

void calibrateBuddyBoxInput(BuddyBox *bb, float localMinSample, float localMaxSample, unsigned int localMaxElapsedCount, unsigned int localNegativeShift)
{
  bb->minInputSample = localMinSample;
  bb->maxInputSample = localMaxSample;
  bb->maxElapsedInputSampleCount = localMaxElapsedCount;
  bb->negativeShift = localNegativeShift;
}

void setBuddyBoxOutputChannelValue(BuddyBox *bb, unsigned int channel, float channelValue)
{
  unsigned int channelDuration;
    
  channelDuration = channelValue / 1.0f * (CHANNEL_MAX_DURATION - CHANNEL_MIN_DURATION) + CHANNEL_MIN_DURATION;
  setBuddyBoxOutputChannelDuration(bb, channel, channelDuration);
}

void setBuddyBoxOutputChannelDuration(BuddyBox *bb, unsigned int channel, unsigned int channelDuration)
{
  if (channel < MAX_CHANNELS)
    {
      if (channelDuration > CHANNEL_MAX_DURATION)
	channelDuration = CHANNEL_MAX_DURATION;
      else if (channelDuration < CHANNEL_MIN_DURATION)
	channelDuration = CHANNEL_MIN_DURATION;
      bb->outputChannelBuffer[channel] = channelDuration * bb->sampleRate / MICROSECONDS_PER_SECOND;
    }
}

void writeBuddyBoxOutputChannelBufferIntoBuffer(BuddyBox *bb, float buffer[], unsigned int bufferSize)
{
  unsigned int bufferSampleCount;

  bufferSampleCount = writeBuddyBoxOverflowBufferIntoBuffer(bb, buffer);
  while (bufferSampleCount < bufferSize)
    bufferSampleCount += writeBuddyBoxOutputChannelBufferIntoBufferFrame(bb, buffer, bufferSize, bufferSampleCount);
}

unsigned int writeBuddyBoxOverflowBufferIntoBuffer(BuddyBox *bb, float* buffer)
{
  unsigned int i;

  for (i = 0; i < bb->outputOverflowSampleCount; i++)
    buffer[i] = bb->outputOverflowBuffer[i];
  bb->outputOverflowSampleCount = 0;
        
  return i;
}

unsigned int writeBuddyBoxOutputChannelBufferIntoBufferFrame(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int bufferSampleCount)
{
  unsigned int frameSampleCount;

  frameSampleCount = writeBuddyBoxOutputChannelBufferIntoBufferChannels(bb, buffer, bufferSize, bufferSampleCount);
  frameSampleCount += writeBuddyBoxOutputChannelBufferIntoBufferSynchro(bb, buffer, bufferSize, bufferSampleCount, frameSampleCount);
        
  return frameSampleCount;
}

unsigned int writeBuddyBoxOutputChannelBufferIntoBufferChannels(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int bufferSampleCount)
{
  unsigned int channel, channelsSampleCount;

  channelsSampleCount = 0;
  for (channel = 0; channel < bb->outputChannelCount; channel++)
    channelsSampleCount += writeBuddyBoxOutputChannelBufferIntoBufferChannel(bb, buffer, bufferSize, bufferSampleCount, channelsSampleCount, channel);
            
  return channelsSampleCount;
}

unsigned int writeBuddyBoxOutputChannelBufferIntoBufferChannel(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int bufferSampleCount, unsigned int channelsSampleCount, unsigned int channel)
{
  unsigned int channelSampleCount;
                
  channelSampleCount = writeBuddyBoxChannelSeperatorIntoBufferChannel(bb, buffer, bufferSize, bufferSampleCount + channelsSampleCount);
  channelSampleCount += writeBuddyBoxChannelDurationIntoBufferChannel(bb, buffer, bufferSize, bufferSampleCount + channelsSampleCount + channelSampleCount, bb->outputChannelBuffer[channel]);
                
  return channelSampleCount;
}

unsigned int writeBuddyBoxChannelSeperatorIntoBufferChannel(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int startBufferSample)
{
  return writeBuddyBoxSignalsToBufferOrOverflowBuffer(bb, buffer, bufferSize, startBufferSample, 0, SEPARATOR_DURATION * bb->sampleRate / MICROSECONDS_PER_SECOND, SIGNAL_HIGH_FLOAT);
}

unsigned int writeBuddyBoxSignalsToBufferOrOverflowBuffer(BuddyBox *bb, float *buffer, unsigned int bufferSize, unsigned int startBufferSample, unsigned int comparatorOffset, unsigned int endBufferSampleOffset, float signal)
{
  unsigned int i;

  for (i = 0; i + comparatorOffset < endBufferSampleOffset; i++)
    writeBuddyBoxSignalToBufferOrOverflowBuffer(bb, buffer, bufferSize, startBufferSample + i, signal);
                        
  return i;
}

void writeBuddyBoxSignalToBufferOrOverflowBuffer(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int bufferSample, float signal)
{
  if (bufferSample < bufferSize)
    writeBuddyBoxSignalToBuffer(bb, buffer, bufferSample, signal);
  else
    writeBuddyBoxSignalToOverflowBuffer(bb, bufferSample - bufferSize, signal);
}

void writeBuddyBoxSignalToBuffer(BuddyBox *bb, float buffer[], unsigned int bufferSample, float signal)
{
  buffer[bufferSample] = signal;
  bb->outputSampleCount++;
}

void writeBuddyBoxSignalToOverflowBuffer(BuddyBox *bb, unsigned int bufferSample, float signal)
{
  bb->outputOverflowBuffer[bufferSample] = signal;
  bb->outputOverflowSampleCount++;
}

unsigned int writeBuddyBoxChannelDurationIntoBufferChannel(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int startBufferSample, unsigned int endBufferSampleOffset)
{
  return writeBuddyBoxSignalsToBufferOrOverflowBuffer(bb, buffer, bufferSize, startBufferSample, 0, endBufferSampleOffset, SIGNAL_LOW_FLOAT);
}

unsigned int writeBuddyBoxOutputChannelBufferIntoBufferSynchro(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int bufferSampleCount, unsigned int frameSampleCount)
{
  unsigned int synchroSampleCount;
                
  synchroSampleCount = writeBuddyBoxChannelSeperatorIntoBufferChannel(bb, buffer, bufferSize, bufferSampleCount + frameSampleCount);
  synchroSampleCount += writeBuddyBoxSynchroIntoBufferSynchro(bb, buffer, bufferSize, bufferSampleCount + frameSampleCount + synchroSampleCount, frameSampleCount);
                
  return synchroSampleCount;
}

unsigned int writeBuddyBoxSynchroIntoBufferSynchro(BuddyBox *bb, float buffer[], unsigned int bufferSize, unsigned int startBufferSample, unsigned int comparatorOffset)
{
  return writeBuddyBoxSignalsToBufferOrOverflowBuffer(bb, buffer, bufferSize, startBufferSample, comparatorOffset, bb->sampleRate * FRAME_DURATION / MICROSECONDS_PER_SECOND, SIGNAL_LOW_FLOAT);
}

void disconnectBuddyBox(BuddyBox *bb)
{
  bb->active = 0;
  free(bb->outputOverflowBuffer);
  printf("BuddyBox:\tDisconnected.\n");
}
