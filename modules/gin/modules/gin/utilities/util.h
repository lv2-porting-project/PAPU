/*==============================================================================

 Copyright 2018 by Roland Rabien
 For more information visit www.rabiensoftware.com

 ==============================================================================*/

#pragma once

//==============================================================================
inline float calculateRMS (const float* values, int n)
{
    float rms = 0;

    for (int i = 0; i < n; i++)
        rms += values[i] * values[i];

    return std::sqrt ((1.0f / n) * rms);
}

inline float calculateMedian (const float* values, int n)
{
    Array<float> f;
    f.insertArray (0, values, n);
    f.sort();

    if (f.size() % 2 == 0)
        return (f[f.size() / 2] + f[f.size() / 2 - 1]) / 2.0f;
    return f[f.size()/2];
}

//==============================================================================
// Fisher-Yates Shuffle
template <typename T>
void shuffleArray (Random& r, T array)
{
    const int n = array.size();
    for (int i = n - 1; i >= 1; i--)
    {
        int j = r.nextInt (i + 1);
        array.swap (i, j);
    }
}

//==============================================================================
// Based on reference implementation of Perlin Noise by Ken Perlin
// http://mrl.nyu.edu/~perlin/paper445.pdf
template <class T>
class PerlinNoise
{
public:
    PerlinNoise()
    {
        p = { 151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
            8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
            35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168, 68,175,74,165,71,
            134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
            55,46,245,40,244,102,143,54, 65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
            18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186, 3,64,52,217,226,
            250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
            189,28,42,223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,
            43,172,9,129,22,39,253, 19,98,108,110,79,113,224,232,178,185, 112,104,218,246,
            97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145,235,249,14,239,
            107,49,192,214, 31,181,199,106,157,184, 84,204,176,115,121,50,45,127, 4,150,254,
            138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180 };

        p.addArray (p);
    }

    PerlinNoise (unsigned int seed)
    {
        Random r (seed);

        for (int i = 0; i <= 255; i++)
            p.add (i);

        shuffleArray (r, p);

        p.addArray (p);
    }

    T noise (T x, T y = 0, T z = 0)
    {
        int X = (int) std::floor (x) & 255;
        int Y = (int) std::floor (y) & 255;
        int Z = (int) std::floor (z) & 255;

        x -= std::floor (x);
        y -= std::floor (y);
        z -= std::floor (z);

        T u = fade (x);
        T v = fade (y);
        T w = fade (z);

        int A  = p[X] + Y;
        int AA = p[A] + Z;
        int AB = p[A + 1] + Z;
        int B  = p[X + 1] + Y;
        int BA = p[B] + Z;
        int BB = p[B + 1] + Z;

        // Add blended results from 8 corners of cube
        T res = lerp (w,
                      lerp (v, lerp (u, grad(p[AA], x, y, z),
                                     grad (p[BA], x-1, y, z)),
                            lerp (u, grad (p[AB], x, y-1, z),
                                  grad (p[BB], x-1, y-1, z))),
                      lerp (v, lerp (u, grad (p[AA+1], x, y, z-1),
                                     grad (p[BA+1], x-1, y, z-1)),
                            lerp (u, grad (p[AB+1], x, y-1, z-1),
                                  grad (p[BB+1], x-1, y-1, z-1))));

        return T ((res + 1.0) / 2.0);
    }

private:
    T fade (T t)
    {
        return t * t * t * (t * (t * 6 - 15) + 10);
    }

    T lerp (T t, T a, T b)
    {
        return a + t * (b - a);
    }

    T grad (int hash, T x, T y, T z)
    {
        int h = hash & 15;
        T u = h < 8 ? x : y, v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    Array<int> p;
};

//==============================================================================
// Keeps a rolling average of a series of numbers
class RollingAverage
{
public:
    RollingAverage (int numVals_)
      : numVals (numVals_)
    {
    }

    double average (double nextValue)
    {
        return (nextValue + numVals * currAvg) / (double)(numVals + 1);
    }

    double getAverage()
    {
        return currAvg;
    }

    void setAverage (double avg)
    {
        currAvg = avg;
    }

private:
    int numVals = 0;
    double currAvg = 0.0;
};

//==============================================================================
// Returns the next colour in a set where the hues differ by the golden ratio.
// Good for coming up with a random set of colours
Colour goldenRatioColor (int idx);

//==============================================================================
// Async Download. Doesn't have the main thread pause the URL::downloadToFile has
class AsyncDownload : private Thread,
                      private AsyncUpdater
{
public:
    AsyncDownload (String url_, std::function<void (AsyncDownload*, juce::MemoryBlock, bool)> cb_)
      : Thread ("AsyncDownload"), url (url_), cb (cb_)
    {
        startThread();
    }

    ~AsyncDownload() override
    {
        stopThread (100);
    }

    void run() override
    {
        ok = URL (url).readEntireBinaryStream (data);
        triggerAsyncUpdate();
    }

    void handleAsyncUpdate() override
    {
        if (cb)
            cb (this, data, ok);
    }

    String url;
    std::function<void (AsyncDownload*, juce::MemoryBlock, bool)> cb;
    bool ok = false;
    juce::MemoryBlock data;
};

//==============================================================================
// Time Profiler -- get a quick idea how long something takes
class TimeProfiler
{
public:
    TimeProfiler (const String& name_) :
      name (name_), start (Time::getMillisecondCounterHiRes()) {}

    ~TimeProfiler()
    {
        DBG (name + String::formatted (" %.2fs", (Time::getMillisecondCounterHiRes() - start) / 1000.0));
    }

private:
    String name;
    double start;
};

//==============================================================================
template <typename T>
inline bool almostEqual (T a, T b, T precision = T (0.00001))
{
    return std::abs (a - b) < precision;
}

int versionStringToInt (const String& versionString);

//==============================================================================
void delayedLambda (std::function<void ()> callback, int delayMS);
