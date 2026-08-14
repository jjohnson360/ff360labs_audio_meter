#pragma once
#include <juce_core/juce_core.h>
#include <array>

template <typename DataType>
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
        DataType temp;
        while (pull(temp))
        {
            data = temp;
            foundData = true;
        }
        return foundData;
    }

private:
    static constexpr int Capacity = 1024;
    juce::AbstractFifo abstractFifo;
    std::array<DataType, Capacity> buffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFifo)
};
