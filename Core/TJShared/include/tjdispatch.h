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
 
 #ifndef _TJ_DISPATCH_H
#define _TJ_DISPATCH_H

#include "tjsharedinternal.h"
#include "tjutil.h"
#include "tjthread.h"
#include <deque>

namespace tj {
	namespace shared {
		class EXPORTED Task: public virtual Object, public Runnable {
			friend class DispatchThread;
			friend class Dispatcher;

			public:
				virtual ~Task();
				bool IsRun() const;
				bool IsEnqueued() const;
				bool IsStalled() const;
				bool IsRunning() const;
				bool DidFail() const;
				virtual bool CanRun() const;
				virtual void Run() = 0;

			protected:
				virtual void OnReuse();
				virtual void OnAfterRun();
				Task();

				CriticalSection _lock;
				volatile int _flags;
				const static int KTaskEnqueued = 0x1;
				const static int KTaskRun = 0x2;
				const static int KTaskFailed = 0x4;
				const static int KTaskStalled = 0x8;
				const static int KTaskRunning = 0x01;
		};

		class EXPORTED Future: public Task {
			public:
				virtual ~Future();
				virtual void DependsOn(strong<Future> future);
				virtual void OnDependencyRan(strong<Future> dep);
				virtual bool CanRun() const;
				virtual bool WaitForCompletion(const Time& timeout = -1);

			protected:
				Future();
				virtual void OnAfterRun();

				
				std::set< weak<Future> > _dependent;
				volatile unsigned int _dependencies;
				Event _completed;
		};

		class DispatchThread;
		class Dispatcher;
		
		class EXPORTED SharedDispatcher {
			
			public:
				SharedDispatcher();
				~SharedDispatcher();
				static strong<Dispatcher> Instance();
				
			protected:
				static ref<Dispatcher> _instance;
			
		};

		class EXPORTED Dispatcher: public virtual Object {
			friend class DispatchThread;

			public:
				Dispatcher(int maxThreads = 0, Thread::Priority priority = Thread::PriorityNormal);
				virtual ~Dispatcher();
				virtual void Dispatch(strong<Task> t);
				virtual void Requeue(strong<Task> t);
				virtual void Stop();
				static strong<Dispatcher> CurrentInstance();
				static strong<Dispatcher> CurrentOrDefaultInstance();
				virtual unsigned int GetProcessedItemsCount() const;
				virtual unsigned int GetThreadCount() const;
				virtual void WaitForCompletion();

			private:
				static ref<Dispatcher> GetCurrent();
				virtual void DispatchTask(ref<Task> t);

				CriticalSection _lock;
				bool _accepting;
				std::deque< ref<Task> > _queue;
				std::set< ref<Task> > _stalled;
				Semaphore _queuedTasks;
				Event _taskFinished;
				std::set< ref<DispatchThread> > _threads;
				int _maxThreads;
				volatile int _busyThreads;
				volatile unsigned int _itemsProcessed;
				const Thread::Priority _defaultPriority;

				static ThreadLocal _currentDispatcher;
		};

		class EXPORTED DispatchThread: public Thread {
			public:
				DispatchThread(ref<Dispatcher> d);
				virtual ~DispatchThread();
				virtual void Run();

			private:
				Dispatcher* _dispatcher;
		};
	}
}

#endif
