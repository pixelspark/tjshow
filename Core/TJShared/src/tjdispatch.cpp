/* This file is part of TJShow. TJShow is free software: you 
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later 
 * version.
 * 
 * TJShow is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with TJShow.  If not, see <http://www.gnu.org/licenses/>. */
 
 #include "../include/tjdispatch.h"
#include "../include/tjlog.h"
using namespace tj::shared;

/** Task **/
Task::~Task() {
}

Task::Task(): _flags(0) {
}

void Task::OnReuse() {
	ThreadLock lock(&_lock);
	_flags = 0;
}

bool Task::IsRun() const {
	return (_flags & KTaskRun)!=0;
}

bool Task::IsRunning() const {
	return (_flags & KTaskRunning) != 0;
}

bool Task::CanRun() const {
	return !IsRun();
}

bool Task::IsEnqueued() const {
	return (_flags & KTaskEnqueued) != 0;
}

bool Task::DidFail() const {
	return (_flags & KTaskFailed) != 0;
}

bool Task::IsStalled() const {
	return (_flags & KTaskStalled) != 0;
}

void Task::OnAfterRun() {
}

/** Future **/
Future::Future(): _dependencies(0) {
}

Future::~Future() {
}

bool Future::WaitForCompletion(const Time& timeout) {
	{
		ThreadLock lock(&_lock);
		if(IsRun()) {
			return true;
		}
		 
		if(!IsEnqueued() && !IsRunning()) {
			Throw(L"Cannot wait for the completion of this future, because it is not enqueued in a dispatcher and not already running", ExceptionTypeError);
		}
	}
	return _completed.Wait(timeout);
}

void Future::DependsOn(strong<Future> of) {
	if(IsEnqueued()) {
		Throw(L"Cannot change dependencies of future when the future is already enqueued", ExceptionTypeError);
	}
	ThreadLock lock(&_lock);

	{
		ThreadLock oflock(&(of->_lock));
		if(of->IsRun()) {
			// Dependency already satisfied
		}
		else {
			ref<Future> thisFuture(this);
			of->_dependent.insert(weak<Future>(thisFuture));
			++_dependencies;
		}
	}
}

void Future::OnDependencyRan(strong<Future> f) {
	ThreadLock lock(&_lock);
	--_dependencies;
	if(_dependencies==0) {
		ref<Dispatcher> disp = Dispatcher::CurrentInstance();
		if(disp) {
			disp->Requeue(ref<Task>(this));
		}
		else {
			Throw(L"Future ran in dispatcher thread, but dispatcher cannot be obtained to complete operation", ExceptionTypeError);
		}
	}
}

void Future::OnAfterRun() {
	{
		ThreadLock lock(&_lock);
		std::set< weak<Future> >::iterator it = _dependent.begin();
		while(it!=_dependent.end()) {
			ref<Future> dep = *it;
			if(dep) {
				dep->OnDependencyRan(ref<Future>(this));
			}
			++it;
		}
	}

	// No need to retain this list anymore
	_dependent.clear();
	_completed.Signal();
}

bool Future::CanRun() const {
	return _dependencies==0;
}

/** Dispatcher **/
ThreadLocal Dispatcher::_currentDispatcher;

Dispatcher::Dispatcher(int maxThreads, Thread::Priority prio): _maxThreads(maxThreads), _busyThreads(0), _defaultPriority(prio), _itemsProcessed(0), _accepting(true) {
	// TODO: if maxThreads=0, limit the maximum number of threads to the number of cores in the system * a load factor
	if(maxThreads==0) {
		_maxThreads = 2;
	}
}

Dispatcher::~Dispatcher() {
	// The destruction of _threads will cause a WaitForCompletion on each of them; please call Stop/WaitForCompletion first!
}

unsigned int Dispatcher::GetProcessedItemsCount() const {
	return _itemsProcessed;
}

unsigned int Dispatcher::GetThreadCount() const {
	return _threads.size();
}

strong<Dispatcher> Dispatcher::CurrentInstance() {
	ref<Dispatcher> di = GetCurrent();
	if(!di) {
		Throw(L"There is no current dispatcher; please instantiate a DispatcherBlock object first!", ExceptionTypeError);
	}
	return di;
}

strong<Dispatcher> Dispatcher::CurrentOrDefaultInstance() {
	ref<Dispatcher> di = GetCurrent();
	if(!di) {
		di = SharedDispatcher::Instance();
	}
	return di;
}

ref<Dispatcher> Dispatcher::GetCurrent() {
	Dispatcher* dsp = reinterpret_cast<Dispatcher*>(_currentDispatcher.GetValue());
	if(dsp!=0) {
		return ref<Dispatcher>(dsp);
	}
	return null;
}

void Dispatcher::Stop() {
	// Send quit messages (null tasks)
	{
		ThreadLock lock(&_lock);
		_accepting = false;
		// Dispatch the same quit task as many times as there are threads; this assumes
		// that threads do not take any new task after receiving a quit task
		std::set< ref<DispatchThread> >::iterator tit = _threads.begin();
		while(tit!=_threads.end()) {
			DispatchTask(null);
			++tit;
		}
	}
	
	_threads.clear(); // ~DispatcherThread will wait for completion
}

void Dispatcher::Requeue(strong<Task> t) {
	ThreadLock taskLock(&(t->_lock));
	if(!t->CanRun() || !t->IsStalled()) {
		Throw(L"Dispatcher::Requeue called with a task that still cannot run or is not currently stalled; not changing anything!", ExceptionTypeWarning);
	}
	
	// Remove from 'stalled' set
	t->_flags &= (~Task::KTaskStalled);
	std::set<ref<Task> >::iterator it = _stalled.find(ref<Task>(t));
	if(it!=_stalled.end()) {
		_stalled.erase(it);
		Dispatch(t);
	}
	else {
		Throw(L"Cannot requeue a stalled task that is not stalled in this dispatcher", ExceptionTypeError);
	}
}

void Dispatcher::DispatchTask(ref<Task> t) {
	if(t && t->IsEnqueued()) {
		Throw(L"Task is already enqueued in (another?) dispatcher!", ExceptionTypeError);
	}

	ThreadLock lock(&_lock);
	if(t) {
		ThreadLock taskLock(&(t->_lock));
		++_itemsProcessed;
		if(t->CanRun()) {
			t->_flags |= Task::KTaskEnqueued;
			_queue.push_back(t);
			_queuedTasks.Release();
		}
		else {
			t->_flags |= Task::KTaskStalled;
			_stalled.insert(t);
		}
		
	}
	else {
		// A null task is always enqueued; it stops the executing dispatcher thread
		_queue.push_front(t);
		_queuedTasks.Release();
	}
}

void Dispatcher::WaitForCompletion() {
	while(true) {
		{
			ThreadLock lock(&_lock);
			if(_queue.size()==0) {
				return;
			}
		}
		
		_taskFinished.Wait();
	}
}

void Dispatcher::Dispatch(strong<Task> t) {
	ThreadLock lock(&_lock);
	if(!_accepting) {
		Throw(L"Dispatcher is stopping, and does not accept new tasks anymore!", ExceptionTypeError);
	}
	
	int numThreads = _threads.size();

	/* If there are no threads yet, or the number of busy threads is equal to the number of 
	available threads (i.e. all threads are busy) create a thread (if the total number of threads
	is still below the maximum number of threads */
	if((numThreads<1) || ((_busyThreads>=numThreads) && (int(_maxThreads)>numThreads))) {
		// Create a new response thread
		ref<DispatchThread> wrt = GC::Hold(new DispatchThread(this));
		_threads.insert(wrt);
		wrt->Start();
		wrt->SetPriority(_defaultPriority);
	}
	
	DispatchTask(t);
}

/** DispatchThread **/
DispatchThread::DispatchThread(ref<Dispatcher> d): _dispatcher(d.GetPointer()) {
}

DispatchThread::~DispatchThread() {
	WaitForCompletion();
}

void DispatchThread::Run() {
	#ifdef TJ_OS_WIN
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
	#endif

	while(true) {
		Semaphore& queueSemaphore = _dispatcher->_queuedTasks;

		// Wait for a task to appear in the queue; if there is a task, wake up and execute it
		if(queueSemaphore.Wait()) {
			ref<Task> task;

			{
				ThreadLock lock(&(_dispatcher->_lock));	
				std::deque< ref<Task> >::iterator it = _dispatcher->_queue.begin();
				if(it==_dispatcher->_queue.end()) {
					continue;
				}

				task = *it;
				_dispatcher->_queue.pop_front();
				if(task) {
					++(_dispatcher->_busyThreads);
				}
			}

			try {
				if(task) {
					{
						ThreadLock taskLock(&(task->_lock));
						if(!task->CanRun()) {
							Throw(L"Task from queue, but cannot run!", ExceptionTypeError);
						}
						task->_flags &= (~Task::KTaskEnqueued);
						task->_flags |= Task::KTaskRunning;
					}
					
					Dispatcher::_currentDispatcher.SetValue(reinterpret_cast<void*>(_dispatcher));
					task->Run();
					
					{
						ThreadLock taskLock(&(task->_lock));
						task->OnAfterRun();
						Dispatcher::_currentDispatcher.SetValue(0);
						task->_flags &= (~Task::KTaskRunning);
						task->_flags |= Task::KTaskRun;
						_dispatcher->_taskFinished.Signal();
					}
				}
				else {
					Log::Write(L"TJShared/DispatchThread", L"Bailing out");
					return;
				}
			}
			catch(const Exception& e) {
				if(task) {
					task->_flags |= Task::KTaskFailed;
				}
				Log::Write(L"TJShared/DispatcherThread", L"Error occurred when processing client request: "+e.GetMsg());
			}
			catch(...) {
				if(task) {
					task->_flags |= Task::KTaskFailed;
				}
				Log::Write(L"TJShared/DispatcherThread", L"Unknown error occurred when processing client request");
			}

			{
				ThreadLock lock(&(_dispatcher->_lock));	
				--(_dispatcher->_busyThreads);
			}
		}
	}

	Log::Write(L"TJShared/DispatchThread", L"Exiting");
	
	#ifdef TJ_OS_WIN
		CoUninitialize();
	#endif
}

/** SharedDispatcher **/
ref<Dispatcher> SharedDispatcher::_instance;

SharedDispatcher::SharedDispatcher() {
	if(_instance) {
		Throw(L"A shared dispatcher already exists; please instantiate only one SharedDispatcher per process (preferably in the main thread)", ExceptionTypeError);
	}
	_instance = GC::Hold(new Dispatcher());
}

SharedDispatcher::~SharedDispatcher() {
	_instance->WaitForCompletion();
	_instance->Stop();
}

strong<Dispatcher> SharedDispatcher::Instance() {
	if(!_instance) {
		Throw(L"There is no shared dispatcher; programmers: please instantiate a SharedDispatcher object in the main thread first", ExceptionTypeError);
	}
	return _instance;
}
