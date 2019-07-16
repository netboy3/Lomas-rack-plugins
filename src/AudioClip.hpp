#include "dep/dr_wav/dr_wav.h"
#include "Interpolation.hpp"
#define WAVEFORM_RESOLUTION 64

struct AudioClip
{
	AudioClip(){}

	unsigned int getSampleCount() { return sampleCount_; }
	
	unsigned int getChannelCount() { return channels_; }
	
	unsigned int getSampleRate() { return sampleRate_; }

	bool isLoaded() { return sampleCount_ > 10; }

	float getSeconds() { return (float)sampleCount_ / (float)sampleRate_; }
	
	float getSample(float index)
	{
		int x1 = floorf(index);
		int x0 = x1 - 1;
		int x2 = x1 + 1;
		int x3 = x2 + 1;
		float t = index - x1;

		return BSpline(left_channel_[x0], left_channel_[x1], left_channel_[x2], left_channel_[x3], t);
	}

	void load(const std::string &path)
    {
		drwav_uint64 totalSampleCount = 0;
		
        float *pSampleData = drwav_open_and_read_file_f32(path.c_str(), &channels_, &sampleRate_, &totalSampleCount);

		if (pSampleData == NULL)
			return;

		left_channel_.clear();
		
		// This so we don't have to clamp on interolation samplepos - 1
		left_channel_.push_back(0);

		for (size_t i = 0; i < totalSampleCount; i += channels_)
			left_channel_.push_back(pSampleData[i]);
		
		sampleCount_ = left_channel_.size();

		// This so we don't have to clamp on interolation samplepos + 2
		left_channel_.push_back(0);
		left_channel_.push_back(0);

		drwav_free(pSampleData);

		calculateWaveform();
    }

	void unLoad()
	{
		left_channel_.clear();
		left_channel_.push_back(0);
		sampleCount_ = 0;
	}

	void calculateWaveform()
	{
		int samplesPerSlice = sampleCount_ / WAVEFORM_RESOLUTION;
		int slice = 0;
		float acumulator = 0;
		int counter = 0;
		for (unsigned int i = 0; i < sampleCount_; i++)
		{
			counter++;
			acumulator = acumulator + abs(left_channel_[i]);
			if (counter >= samplesPerSlice)
			{
				waveform_[slice] = acumulator / samplesPerSlice;
				counter = 0;
				acumulator = 0;
				slice++;
			}
		}
		waveform_[WAVEFORM_RESOLUTION - 1] = 0;
	}

	void startRec(unsigned int sampleRate)
	{
		sampleRate_ = sampleRate;
		left_channel_.clear();
		left_channel_.push_back(0);
		sampleCount_ = 1;
	}

	// Returns true until max recording time.
	bool rec(float sample)
	{
		left_channel_.push_back(clamp(sample, -0.5f, 0.5f));
		sampleCount_++;
		if (sampleCount_ >= maxRecordSamples)
		{
			stopRec();
			return false;
		}
		return true;
	}

	void stopRec()
	{
		// We didnt add on startRec. Is fine like this.
		
		// This so we don't have to clamp on interolation samplepos + 2
		left_channel_.push_back(0);
		left_channel_.push_back(0);
	}

	void getData(float* data)
	{
		for (size_t i = 0; i < sampleCount_ + 2; i++)
			data[i] = left_channel_[i];
	}
	
	const unsigned int maxRecordSamples = 44100 * 10;
	float waveform_[WAVEFORM_RESOLUTION] = {0, 0, 0, 0};

    private:

	std::vector<float> left_channel_;
	unsigned int sampleCount_ = 0;
	unsigned int channels_ = 0;
	unsigned int sampleRate_ = 0;
	
};


