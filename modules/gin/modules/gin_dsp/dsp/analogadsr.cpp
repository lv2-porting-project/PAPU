/*
 ==============================================================================

 This file is part of the GIN library.
 Copyright (c) 2019 - Roland Rabien.

 ==============================================================================
 */


#include "adsr.h"
#include <cmath>

void AnalogADSR::noteOn()
{
    calculateRelease();

    if (attack == 0.0f)
        state = State::decay;
    else
        state = State::attack;

    output = attack == 0.0 ? 1.0f : 0.0f;
}

void AnalogADSR::noteOff()
{
    if (output > 0.0f)
        state = State::release;
    else
        state = State::idle;
}

void AnalogADSR::setAttack (float a)
{
    if (! almostEqual (attack, a))
    {
        attack = a;
        calculateAttack();
    }
}

void AnalogADSR::setDecay (float d)
{
    if (! almostEqual (decay, d))
    {
        decay = d;
        calculateDecay();
    }
}

void AnalogADSR::setRelease (float r)
{
    if (! almostEqual (release, r))
    {
        release = r;
        calculateRelease();
    }
}

void AnalogADSR::setSustainLevel (float s)
{
    if (! almostEqual (sustain, s))
    {
        sustain = s;
        calculateDecay();
        if (state != State::release)
            calculateRelease();
    }
}

void AnalogADSR::calculateAttack()
{
    float samples = float (sampleRate * attack);
    float tco = std::exp (-0.5f);

    attackCoeff = std::exp (-std::log ((1.0f + tco) / tco) / float (samples));
    attackOffset = (1.0f + tco) * (1.0f - attackCoeff);
}

void AnalogADSR::calculateDecay()
{
    float samples = float (sampleRate * decay);
    float tco = std::exp (-5.0f);

    decayCoeff = std::exp (-std::log ((1.0f + tco) / tco) / samples);
    decayOffset = (sustain - tco) * (1.0f - decayCoeff);
}

void AnalogADSR::calculateRelease()
{
    float samples = float (sampleRate * release);
    float tco = std::exp (-5.0f);

    releaseCoeff = std::exp (-std::log ((1.0f + tco) / tco) / samples);
    releaseOffset = -tco * (1.0f - releaseCoeff);
}

void AnalogADSR::process (AudioSampleBuffer& buffer)
{
    process (buffer, 0, buffer.getNumSamples());
}

void AnalogADSR::process (AudioSampleBuffer& buffer, int startSample, int numSamples)
{
    float* d = buffer.getWritePointer (0, startSample);
    for (int i = 0; i < numSamples; i++)
    {
        switch (state)
        {
            case State::idle:
                break;
            case State::attack:
                output = attackOffset + output * attackCoeff;
                if (output >= 1.0 || attack == 0.0f)
                {
                    output = 1.0;
                    state = State::decay;
                }
                break;
            case State::decay:
                output = decayOffset + output * decayCoeff;
                if (output <= sustain)
                {
                    state = State::sustain;
                }
                break;
            case State::sustain:
                break;
            case State::release:
                output = releaseOffset + output * releaseCoeff;
                if (output <= 0.0 || release == 0.0)
                {
                    output = 0.0;
                    state = State::idle;
                }
                break;
        }
        *d++ = output;
    }
}

float AnalogADSR::process()
{
    switch (state)
    {
        case State::idle:
            break;
        case State::attack:
            output = attackOffset + output * attackCoeff;
            if (output >= 1.0 || attack == 0.0f)
            {
                output = 1.0;
                state = State::decay;
            }
            break;
        case State::decay:
            output = decayOffset + output * decayCoeff;
            if (output <= sustain)
            {
                state = State::sustain;
            }
            break;
        case State::sustain:
            break;
        case State::release:
            output = releaseOffset + output * releaseCoeff;
            if (output <= 0.0 || release == 0.0)
            {
                output = 0.0;
                state = State::idle;
            }
            break;
    }
    return output;
}

void AnalogADSR::reset()
{
    state = State::idle;
    output = 0.0f;
}
