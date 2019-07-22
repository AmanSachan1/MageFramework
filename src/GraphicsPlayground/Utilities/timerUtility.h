#pragma once
#include <chrono>

namespace TimerUtil
{
	static std::chrono::high_resolution_clock::time_point startTime;
	inline void initTimer()
	{
		std::cout << "Starting Timer \n";
		startTime = std::chrono::high_resolution_clock::now();
	};

	inline float getTimeElapsedSinceStart()
	{
		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - startTime);
		return deltaTime.count();
	};

	inline float getTimeElapsedSinceStart(std::chrono::high_resolution_clock::time_point _start)
	{
		std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
		std::chrono::duration<float> deltaTime = std::chrono::duration_cast<std::chrono::duration<float>>(currentTime - _start);
		return deltaTime.count();
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