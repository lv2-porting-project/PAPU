/*
 ==============================================================================

 This file is part of the GIN library.
 Copyright (c) 2019 - Roland Rabien.

 ==============================================================================
 */


#include "audioutil.h"

//==============================================================================
void applyGain (AudioSampleBuffer& buffer, LinearSmoothedValue<float>& gain)
{
    if (gain.isSmoothing())
    {
        if (float** w = buffer.getArrayOfWritePointers())
        {
            for (int s = 0; s < buffer.getNumSamples(); s++)
            {
                float g = gain.getNextValue();
                for (int c = 0; c < buffer.getNumChannels(); c++)
                    w[c][s] *= g;
            }
        }

    }
    else
    {
        buffer.applyGain (gain.getTargetValue());
    }
}

void applyGain (AudioSampleBuffer& buffer, int channel, LinearSmoothedValue<float>& gain)
{
    if (gain.isSmoothing())
    {
        if (float* w = buffer.getWritePointer (channel))
            for (int s = 0; s < buffer.getNumSamples(); s++)
                w[s] *= gain.getNextValue();
    }
    else
    {
        buffer.applyGain (channel, 0, buffer.getNumSamples(), gain.getTargetValue());
    }
}

//==============================================================================
LevelTracker::LevelTracker (float decayPerSecond) : decayRate (decayPerSecond)
{
}

void LevelTracker::trackBuffer (AudioSampleBuffer& buffer)
{
    for (int i = 0; i < buffer.getNumChannels(); i++)
        trackBuffer (buffer.getReadPointer (0), buffer.getNumSamples());
}

void LevelTracker::trackBuffer (const float* buffer, int numSamples)
{
    Range<float> range = FloatVectorOperations::findMinAndMax (buffer, numSamples);
    float v1 = std::fabs (range.getStart());
    float v2 = std::fabs (range.getEnd());

    float peakDB = Decibels::gainToDecibels (jmax (v1, v2));

    if (peakDB > 0)
        clip = true;

    const float time = (Time::getMillisecondCounter() / 1000.0f);

    if (getLevel() < peakDB)
    {
        peakLevel = peakDB;
        peakTime = time;
    }
}

float LevelTracker::getLevel()
{
    const float hold = 50.0f / 1000.0f;

    const float elapsed = (Time::getMillisecondCounter() / 1000.0f) - peakTime;

    if (elapsed < hold)
        return peakLevel;

    return peakLevel - (decayRate * (elapsed - hold));
}

//==============================================================================
String getMidiMessageType (const MidiMessage& msg)
{
    if (msg.isNoteOn()) return "Note On";
    if (msg.isNoteOff()) return "Note Off";
    if (msg.isSysEx()) return "SysEx";
    if (msg.isProgramChange()) return "Program Change";
    if (msg.isPitchWheel()) return "Pitch Wheel";
    if (msg.isAftertouch()) return "Aftertouch";
    if (msg.isChannelPressure()) return "Channel Pressure";
    if (msg.isController()) return "Controller";
    if (msg.isAllNotesOff()) return "All Notes Off";
    if (msg.isAllSoundOff()) return "All Sound Off";
    if (msg.isMetaEvent()) return "Meta";
    if (msg.isTrackMetaEvent()) return "Track Meta";
    if (msg.isEndOfTrackMetaEvent()) return "End of Track Meta";
    if (msg.isTrackNameEvent()) return "Track Name";
    if (msg.isTextMetaEvent()) return "Text Meta";
    if (msg.isTempoMetaEvent()) return "Tempo Meta";
    if (msg.isTimeSignatureMetaEvent()) return "Time Sig";
    if (msg.isKeySignatureMetaEvent()) return "Key Sig";
    if (msg.isMidiChannelMetaEvent()) return "Midi Channel";
    if (msg.isActiveSense()) return "Active Sense";
    if (msg.isMidiStart()) return "Midi Start";
    if (msg.isMidiStop()) return "Midi Stop";
    if (msg.isMidiClock()) return "Midi Clock";
    if (msg.isSongPositionPointer()) return "Song Pos";
    if (msg.isQuarterFrame()) return "Quarter Frame";
    if (msg.isFullFrame()) return "Full Frame";
    if (msg.isMidiMachineControlMessage()) return "MMC";

    return "Unknown";
}
