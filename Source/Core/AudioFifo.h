#pragma once
#include <juce_core/juce_core.h>
#include <array>

// Single-producer / single-consumer frame queue from the audio thread to the UI.
// DataType must be trivially copyable in practice (fixed-size arrays, no vectors):
// push() runs on the audio thread and must never allocate.
template <typename DataType, int Capacity = 1024>
class AudioFifo
{
public:
    AudioFifo() : abstractFifo(Capacity) {}

    void push(const DataType& data)
    {
        auto writeHandle = abstractFifo.write(1);
        if (writeHandle.blockSize1 > 0)
        {
            buffer[(size_t)writeHandle.startIndex1] = data;
        }
    }

    bool pull(DataType& data)
    {
        auto readHandle = abstractFifo.read(1);
        if (readHandle.blockSize1 > 0)
        {
            data = buffer[(size_t)readHandle.startIndex1];
            return true;
        }
        return false;
    }

    // Skips to the newest data if UI falls behind, discarding older values
    bool pullLatest(DataType& data)
    {
        bool foundData = false;
        while (pull(data))
            foundData = true;
        return foundData;
    }

    int getNumReady() const { return abstractFifo.getNumReady(); }

private:
    juce::AbstractFifo abstractFifo;
    std::array<DataType, Capacity> buffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFifo)
};
