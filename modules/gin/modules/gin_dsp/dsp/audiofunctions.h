/*
 ==============================================================================

 This file is part of the GIN library.
 Copyright (c) 2019 - Roland Rabien.

 ==============================================================================
 */


#pragma once

#include "audioutil.h"
#include "oscillators.h"

//==============================================================================
struct FuncState
{
    FuncState (double sr) : sampleRate (sr) {}
    virtual ~FuncState() = default;
    virtual void setSampleRate (double sr) { sampleRate = sr; }
    virtual void reset() {}
    double sampleRate = 44100.0;
};

//==============================================================================
struct OscState : public FuncState
{
    OscState (double sr) : FuncState (sr) {}

    float phase { Random::getSystemRandom().nextFloat() };

    inline void incPhase (float note)
    {
        if (note != lastNote)
        {
            lastNote = note;

            frequency = float (gin::getMidiNoteInHertz (note));
            const float period = 1.0f / frequency;
            const float periodInSamples = float (period * sampleRate);
            delta = 1.0f / periodInSamples;

            jassert (delta > 0);
        }
        phase += delta;
        if (phase > 1.0f)
            phase -= 1.0f;

        jassert (! std::isinf (phase));
    }

    void reset() override
    {
        phase = Random::getSystemRandom().nextFloat();
    }

    float lastNote = -1.0f, frequency = -1.0f, delta = -1.0f;
};

//==============================================================================
struct HPState : public FuncState
{
    HPState (double sr) : FuncState (sr) {}

    double process (double v, double freq, double res)
    {
        float q = 0.70710678118655f / (1.0f - (float (res) / 100.0f) * 0.99f);
        auto c = IIRCoefficients::makeHighPass (sampleRate, freq, q);

        filter.setCoefficients (c);
        return filter.processSingleSampleRaw (float (v));
    }

    void reset() override
    {
        filter.reset();
    }

    juce::IIRFilter filter;
};

//==============================================================================
struct BPState : public FuncState
{
    BPState (double sr) : FuncState (sr) {}

    double process (double v, double freq, double res)
    {
        float q = 0.70710678118655f / (1.0f - (float (res) / 100.0f) * 0.99f);
        auto c = IIRCoefficients::makeBandPass (sampleRate, freq, q);

        filter.setCoefficients (c);
        return filter.processSingleSampleRaw (float (v));
    }

    void reset() override
    {
        filter.reset();
    }

    juce::IIRFilter filter;
};

//==============================================================================
struct LPState : public FuncState
{
    LPState (double sr) : FuncState (sr) {}

    double process (double v, double freq, double res)
    {
        float q = 0.70710678118655f / (1.0f - (float (res) / 100.0f) * 0.99f);
        auto c = IIRCoefficients::makeLowPass (sampleRate, freq, q);

        filter.setCoefficients (c);
        return filter.processSingleSampleRaw (float (v));
    }

    void reset() override
    {
        filter.reset();
    }

    juce::IIRFilter filter;
};

//==============================================================================
struct NotchState : public FuncState
{
    NotchState (double sr) : FuncState (sr) {}

    double process (double v, double freq, double res)
    {
        float q = 0.70710678118655f / (1.0f - (float (res) / 100.0f) * 0.99f);
        auto c = IIRCoefficients::makeNotchFilter (sampleRate, freq, q);

        filter.setCoefficients (c);
        return filter.processSingleSampleRaw (float (v));
    }

    void reset() override
    {
        filter.reset();
    }

    juce::IIRFilter filter;
};

//==============================================================================
class AudioFunctionHost
{
public:
    // You must provide a set of lookup tables at the correct sample rate
    gin::BandLimitedLookupTables* lookupTables = nullptr;

protected:
    void addOscillatorFunctions (gin::EquationParser&);
    void addFilterFunctions (gin::EquationParser&);

    std::map<int, std::unique_ptr<FuncState>> funcStates;
    double sampleRate = 44100.0;

    template <class T>
    T* getFuncParams (int i, double sr)
    {
        auto itr = funcStates.find (i);
        if (itr != funcStates.end())
        {
            auto p = dynamic_cast<T*> (itr->second.get());
            jassert (p != nullptr);
            return p;
        }

        auto p = new T (sr);
        funcStates[i].reset (p);
        return p;
    }
};
