/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "gb_apu/Gb_Apu.h"
#include "gb_apu/Multi_Buffer.h"

//==============================================================================
/**
*/
class PAPUAudioProcessorEditor;
class PAPUAudioProcessor : public gin::GinProcessor
{
public:
    //==============================================================================
    PAPUAudioProcessor();
    ~PAPUAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    void processBlock (AudioSampleBuffer&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    
    static const char* paramPulse1OL;
    static const char* paramPulse1OR;
    static const char* paramPulse1Duty;
    static const char* paramPulse1A;
    static const char* paramPulse1R;
    static const char* paramPulse1Tune;
    static const char* paramPulse1Fine;
    static const char* paramPulse1Sweep;
    static const char* paramPulse1Shift;
    
    static const char* paramPulse2OL;
    static const char* paramPulse2OR;
    static const char* paramPulse2Duty;
    static const char* paramPulse2A;
    static const char* paramPulse2R;
    static const char* paramPulse2Tune;
    static const char* paramPulse2Fine;

    static const char* paramNoiseOL;
    static const char* paramNoiseOR;
    static const char* paramNoiseShift;
    static const char* paramNoiseStep;
    static const char* paramNoiseRatio;
    static const char* paramNoiseA;
    static const char* paramNoiseR;
    
    static const char* paramOutput;

    void setEditor (PAPUAudioProcessorEditor* editor_)
    {
        ScopedLock sl (editorLock);
        editor = editor_;
    }
    
private:
    void runUntil (int& done, AudioSampleBuffer& buffer, int pos);
    void runOscs (int curNote, bool trigger);
    
    int lastNote = -1, velocity = 0;
    double pitchBend = 0;
    Array<int> noteQueue;
    
    LinearSmoothedValue<float> outputSmoothed;
    CriticalSection editorLock;
    PAPUAudioProcessorEditor* editor = nullptr;
    
    Gb_Apu apu;
    Stereo_Buffer buf;
    
    blip_time_t time = 0;
    
    blip_time_t clock() { return time += 4; }
    
    void writeReg (int reg, int value, bool force);
    
    std::map<int, int> regCache;
        
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PAPUAudioProcessor)
};

