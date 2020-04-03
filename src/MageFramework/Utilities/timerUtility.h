#pragma once
#include <chrono>

#define TIME_POINT std::chrono::high_resolution_clock::time_point

namespace TimerUtil
{
	static TIME_POINT startTime;
	inline void initTimer()
	{
		std::cout << "Starting Timer \n";
		startTime = std::chrono::high_resolution_clock::now();
	};

	inline float getTimeElapsedSinceStart()
	{
		TIME_POINT currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::milli>(currentTime - startTime).count();
		return deltaTime;
	};

	inline float getTimeElapsedSinceStart(std::chrono::high_resolution_clock::time_point _start)
	{
		TIME_POINT currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::milli>(currentTime - _start).count();
		return deltaTime;
	}

	// Reference: https://en.wikipedia.org/wiki/Halton_sequence
	inline float haltonSequenceAt(int index, int base)
	{
		float f = 1.0f;
		float r = 0.0f;

		while (index > 0)
		{
			f = f / float(base);
			r += f * (index%base);
			index = int(std::floor(index / base));
		}

		return r;
	}
} 