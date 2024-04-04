#include "DispatcherTest.h"


int _tmain(int argc, _TCHAR* argv[])
{	
	JobDispatcher<TestWorkerThread> workerService(TEST_WORKER_THREAD);

	workerService.RunWorkerThreads();


	//for (auto& t : gTestObjects)
	//	std::cout << "TestCount: " << t->GetTestCount() << std::endl;

	getchar();
	return 0;
}