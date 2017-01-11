# ThreadPool
Simple C++11 thread pool.

## Setup / Usage

```cpp

#include "thread_pool.h"
#include <iostream>

class ApplyFoo {
	public:
	int operator()(int answer) const {
		return answer;
	}
};

int main (int argc, char * argv[]) {
    // create thread pool with 2 threads.
    tp::ThreadPool tp(2);
	
    // enqueue and store future
    auto result = tp.enqueue(ApplyFoo(), 42);
	
    // get result from future
    std::cout << result.get() << std::endl;	
}

```

## Running Tests With CMake

```
mkdir build
cd build
cmake .. 
make
make test
```

## Known Issues and TODOs
* ~~DONE an api call for waiting until all tasks done.~~
* TODO work stealing.
* TODO map/reduce.





