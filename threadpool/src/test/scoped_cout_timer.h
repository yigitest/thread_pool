#ifndef YY_SCOPED_TIMER_H
#define YY_SCOPED_TIMER_H

#include <chrono>
#include <iostream>

class ScopedCoutTimer {

	public:	
	
	ScopedCoutTimer()
		: t0(std::chrono::high_resolution_clock::now()){
	}

	~ScopedCoutTimer(void) {
		auto t1 = std::chrono::high_resolution_clock::now();
		auto nanos = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
		std::cout << nanos << "ms." << std::endl;
	}

	private:
	std::chrono::high_resolution_clock::time_point t0;
};


#endif // YY_SCOPED_TIMER_H