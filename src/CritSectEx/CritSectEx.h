// kate: auto-insert-doxygen true; backspace-indents true; indent-width 5; keep-extra-spaces true; replace-tabs false; tab-indents true; tab-width 5;
/**
	@file CritSectEx.h
	A fast CriticalSection like class with timeout (reimplemented from and) inspired by
	@n
	http://www.codeproject.com/KB/threads/CritSectEx.aspx
	originally released under the CPOL license (http://www.codeproject.com/info/cpol10.aspx)
	for MS Windows only; extended and ported to Mac OS X & linux by RJVB
	@n
	This file includes only RJVB's MutexEx version which I provide under No License At All.
 */

#ifdef SWIG

%module CritSectEx
%{
#	if !(defined(WIN32) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(SWIGWIN))
#		include "msemul.h"
#	endif
#	include "CritSectEx.h"
%}
%feature("autodoc","3");

%init %{
	init_HRTime();
%}

#endif //SWIG

#ifndef _CRITSECTEX_H

#pragma once

// snip MSWin code

#include <stdio.h>
#include <stdlib.h>

// #ifndef CRITSECT
// #	define CRITSECT	CritSectEx
// #endif

#include "msemul.h"
//#if !defined(__windows__)
#	ifdef __cplusplus
#		include <cstdlib>
#		include <exception>
		typedef class cseAssertFailure : public std::exception{
		public:
			const char *msg;
			const int errcode;
			cseAssertFailure( const char *s, int errcode=errno )
				: errcode(errcode)
			{
				msg = s;
			}
			virtual const char* what() const throw()
			{
				return msg;
			}
			virtual const int code() const throw()
			{
				return errcode;
			}
		} cseAssertFailure;
#	endif
//#endif


#if /*defined(WIN32) || */ defined(_MSC_VER)
#	define InlDebugBreak()	{ __asm { int 3 }; }
#	pragma intrinsic(_WriteBarrier)
#	pragma intrinsic(_ReadWriteBarrier)
#elif (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 1)
#	define _WriteBarrier()		__sync_synchronize()
#	define _ReadWriteBarrier()	__sync_synchronize()
#endif

#if defined(DEBUG)
// snip MSWin code
#		include <assert.h>
#		ifndef ASSERT
#			define ASSERT(x) assert(x)
#		endif // ASSERT
#		ifndef VERIFY
#			define VERIFY(x) ASSERT(x)
#		endif
#else // DEBUG
#	ifndef ASSERT
#		define ASSERT(x)
#	endif // ASSERT
#	ifndef VERIFY
#		define VERIFY(x) (x)
#	endif
#endif // DEBUG

#ifndef STR
#	define STR(name)	# name
#endif
#ifndef STRING
#	define STRING(name)	STR(name)
#endif

#define EXCEPTION_FAILED_CRITSECTEX_SIGNAL	0xC20A018E

#ifdef __cplusplus
#	include <typeinfo>
#endif

#ifdef __cplusplus
	__forceinline static void cseAssertExInline(bool expected, const char *fileName, int linenr, const char *title="CritSectEx malfunction", const char *arg=NULL) throw(cseAssertFailure)
#else
	__forceinline static void cseAssertExInline(void *expected, const char *fileName, int linenr, const char *title, const char *arg)
#endif
	{
		if( !(expected) ){
// snip MSWin code
		  const char *larg = (arg)? arg : "";;
			if( !title ){
				title = "CritSectEx malfunction";
			}
// snip MSWin code
		  char msgBuf[1024];
			// error handling. Do whatever is necessary in your implementation.
			snprintf( msgBuf, sizeof(msgBuf),
					"fatal CRITSECT malfunction at '%s':%d%s%s)",
					fileName, linenr, (arg)? " with arg=" : "", larg
			);
			fprintf( stderr, "%s %s (errno=%d=%s)\n", msgBuf, title, errno, strerror(errno) );
			fflush(stderr);
#	ifdef __cplusplus
			throw cseAssertFailure(msgBuf);
#	endif
		}
	}
#ifdef __cplusplus
	extern void cseAssertEx(bool, const char *, int, const char*, const char*);
	extern void cseAssertEx(bool, const char *, int, const char*);
	extern void cseAssertEx(bool, const char *, int);
#endif

#if defined(i386) || defined(__i386__) || defined(__x86_64__)
#	define INTEL_CPU
#endif

#if defined(__GNUC__) && (defined(INTEL_CPU) || defined(_MSEMUL_H))

#	define CRITSECTGCC

#else // WIN32?
#	ifdef DEBUG
#		include "timing.h"
#	endif
#endif // CRITSECTGCC

#ifdef __cplusplus
static inline void _InterlockedSetTrue( volatile long &atomic )
{
	if /*while*/( !atomic ){
		if( !_InterlockedIncrement(&atomic) ){
			YieldProcessor();
		}
	}
}

static inline void _InterlockedSetFalse( volatile long &atomic )
{
	while( atomic ){
		if( atomic > 0 ){
			if( _InterlockedDecrement(&atomic) ){
				YieldProcessor();
			}
		}
		else{
			if( _InterlockedIncrement(&atomic) ){
				YieldProcessor();
			}
		}
	}
}
#endif //__cplusplus

#ifndef _MSEMUL_H
/**
	set the referenced state variable to True in an atomic operation
	(which avoids changing the state while another thread is reading it)
 */
static inline void _InterlockedSetTrue( volatile long *atomic )
{
	if /*while*/( !*atomic ){
		if( !_InterlockedIncrement(atomic) ){
			YieldProcessor();
		}
	}
}

/**
	set the referenced state variable to False in an atomic operation
	(which avoids changing the state while another thread is reading it)
 */
static inline void _InterlockedSetFalse( volatile long *atomic )
{
	if /*while*/( *atomic ){
		if( _InterlockedDecrement(atomic) ){
			YieldProcessor();
		}
	}
}
#endif //_MSEMUL_H

#if defined(__cplusplus)
#if defined(MUTEXEX_CAN_TIMEOUT) && defined(__APPLE__)
#	define __MUTEXEX_CAN_TIMEOUT__
#endif

/**
	A critical section class API-compatible with Vladislav Gelfer's CritSectEx
	This class uses a simple platform-specific mutex except where native mutexes
	don't provide a timed wait. In that case (OS X), the msemul layer is used to
	emulate CreateSemaphore/ReleaseSemaphore given that a mutex is a semaphore
	with starting value 1. Note however that this imposes the limits that come
	with pthread's sem_open et al (semaphores count to the limit of open files).
 */
class MutexEx {
	// Declare all variables volatile, so that the compiler won't
	// try to optimise something important away.
	struct membervars {
#if defined(__windows__) || defined(__MUTEXEX_CAN_TIMEOUT__)
		volatile HANDLE	hMutex;
		volatile DWORD		bIsLocked;
#else
		pthread_mutex_t	mMutex, *hMutex;
		int				iMutexLockError;
		volatile DWORD		bIsLocked;
#endif
// #ifdef DEBUG
		volatile long	hLockerThreadId;
		volatile bool	bUnlocking;
// #endif
		volatile bool	bTimedOut;
		size_t scopesUnlocked, scopesLocked;
	} m;

private:
	// disable copy constructor and assignment
	MutexEx(const MutexEx&);
	void operator = (const MutexEx&);

	__forceinline void PerfLock(DWORD dwTimeout)
	{
#ifdef DEBUG
		if( m.bIsLocked ){
			fprintf( stderr, "Thread %lu attempting to lock mutex of thread %ld\n",
				GetCurrentThreadId(), m.hLockerThreadId
			);
		}
#endif
#if defined(__windows__) || defined(__MUTEXEX_CAN_TIMEOUT__)
		switch( WaitForSingleObject( m.hMutex, dwTimeout ) ){
			case WAIT_ABANDONED:
			case WAIT_FAILED:
				cseAssertExInline(false, __FILE__, __LINE__);
				break;
			case WAIT_TIMEOUT:
				m.bTimedOut = true;
				break;
			default:
#	ifdef DEBUG
				m.hLockerThreadId = (long) GetCurrentThreadId();
#	endif
				m.bIsLocked += 1;
				break;
		}
#elif defined(MUTEXEX_CAN_TIMEOUT)
		{ struct timespec timeout;
			clock_gettime( CLOCK_REALTIME, &timeout );
			{ time_t sec = (time_t) (dwMilliseconds/1000);
				timeout.tv_sec += sec;
				timeout.tv_nsec += (long) ( (dwMilliseconds- sec*1000)* 1000000 );
				while( timeout.tv_nsec > 999999999 ){
					timeout.tv_sec += 1;
					timeout.tv_nsec -= 1000000000;
				}
			}
			errno = 0;
			if( (m.iMutexLockError = pthread_mutex_timedlock( m.hMutex, &timeout )) ){
				if( errno == ETIMEDOUT ){
					m.bTimedOut = true;
				}
				else{
					cseAssertExInline(false, __FILE__, __LINE__, "pthread_mutex_timedlock failed");
				}
			}
#	ifdef DEBUG
			m.hLockerThreadId = (long) GetCurrentThreadId();
#	endif
			m.bIsLocked += 1;
		}
#else
		// attempt to lock m.hMutex;
		if( (m.iMutexLockError = pthread_mutex_lock(m.hMutex)) ){
			if( errno == ETIMEDOUT ){
				m.bTimedOut = true;
			}
			else{
				cseAssertExInline(false, __FILE__, __LINE__, "pthread_mutex_lock failed");
			}
		}
#	ifdef DEBUG
		fprintf( stderr, "Mutex of thread %ld locked (recurs.lock=%ld) by thread %lu at t=%gs\n",
			m.hLockerThreadId, m.bIsLocked+1, GetCurrentThreadId(), HRTime_tic()
		);
		m.hLockerThreadId = (long) GetCurrentThreadId();
#	endif
		m.bIsLocked += 1;
#endif
	}

	__forceinline void PerfUnlock()
	{
//		if( m.bIsLocked ){
// snip MSWin code
#if defined(__MUTEXEX_CAN_TIMEOUT__)
		ReleaseSemaphore(m.hMutex, 1, NULL);
#else
		// release m.hMutex
		m.iMutexLockError = pthread_mutex_unlock(m.hMutex);
#endif
		if( m.bIsLocked > 0 ){
			m.bIsLocked -= 1;
#ifdef DEBUG
			if( !m.bUnlocking ){
				fprintf( stderr, "Mutex of thread %ld unlocked (recurs.lock=%ld) by thread %lu at t=%gs\n",
					m.hLockerThreadId, m.bIsLocked, GetCurrentThreadId(), HRTime_toc()
				);
			}
#endif
		}
#ifdef DEBUG
		m.hLockerThreadId = -1;
#endif
//		}
	}

public:
	volatile unsigned long lockCounter;
#ifdef CRITSECTEX_ALLOWSHARED
	void *operator new(size_t size)
	{ extern void *MSEreallocShared( void* ptr, size_t N, size_t oldN );
		return MSEreallocShared( NULL, size, 0 );
	}
	void operator delete(void *p)
	{ extern void MSEfreeShared(void *ptr);
		MSEfreeShared(p);
	}
#endif //CRITSECTEX_ALLOWSHARED
	// Constructor/Destructor
	MutexEx(DWORD dwSpinMax=0)
	{
		memset(&this->m, 0, sizeof(this->m));
// snip MSWin code
#if defined(__MUTEXEX_CAN_TIMEOUT__)
		m.hMutex = CreateSemaphore( NULL, 1, -1, NULL );
		cseAssertExInline( (m.hMutex!=NULL), __FILE__, __LINE__);
#else
		// create a pthread_mutex_t
		cseAssertExInline( (pthread_mutex_init(&m.mMutex, NULL) == 0), __FILE__, __LINE__);
		m.hMutex = &m.mMutex;
		m.scopesUnlocked = m.scopesLocked = 0;
#endif
		lockCounter = 0;
#ifdef DEBUG
		m.hLockerThreadId = -1;
		init_HRTime();
#endif
	}

	virtual ~MutexEx()
	{
#if defined(__windows__) || defined(__MUTEXEX_CAN_TIMEOUT__)
		// should not be done when m.bIsLocked == TRUE ?!
		CloseHandle(m.hMutex);
#else
		// delete the m.hMutex
		m.iMutexLockError = pthread_mutex_destroy(m.hMutex);
		m.hMutex = NULL;
		if( m.scopesLocked || m.scopesUnlocked ){
			fprintf( stderr, "MutexEx: %lu scopes were destroyed still locked, %lu were already unlocked\n", 
				m.scopesLocked, m.scopesUnlocked );
		}
#endif
	}

	// Lock/Unlock
	__forceinline bool Lock(bool& bUnlockFlag, DWORD dwTimeout = INFINITE)
	{
		PerfLock(dwTimeout);
		bUnlockFlag = !m.bTimedOut;
		return true;
	}

	__forceinline void Unlock(bool bUnlockFlag)
	{
		if( bUnlockFlag ){
#ifdef DEBUG
			m.bUnlocking = true;
#endif
			PerfUnlock();
#ifdef DEBUG
			m.bUnlocking = false;
#endif
		}
	}

	__forceinline bool TimedOut() const { return m.bTimedOut; }
	__forceinline bool IsLocked() const { return (bool) m.bIsLocked; }
	__forceinline DWORD SpinMax()	const { return 0; }
	operator bool () const { return (bool) m.bIsLocked; }

	// Some extra
	void SetSpinMax(DWORD dwSpinMax)
	{
	}
	void AllocateKernelSemaphore()
	{
	}

	// Scope
	class Scope {

		// disable copy constructor and assignment
		Scope(const Scope&);
		void operator = (const Scope&);

		MutexEx *m_pCs;
		bool m_bLocked;
		bool m_bUnlockFlag;

		void InternalUnlock()
		{
			if( m_bUnlockFlag ){
				ASSERT(m_pCs);
				m_pCs->PerfUnlock();
			}
		}

		__forceinline void InternalLock(DWORD dwTimeout)
		{
			ASSERT(m_pCs);
			m_bLocked = m_pCs->Lock(m_bUnlockFlag, dwTimeout);
		}

		__forceinline void InternalLock(MutexEx &cs, DWORD dwTimeout)
		{
			m_bUnlockFlag = false;
			m_pCs = &cs;
			ASSERT(m_pCs);
			m_bLocked = m_pCs->Lock(m_bUnlockFlag, dwTimeout);
		}

		__forceinline void InternalLock(MutexEx *cs, DWORD dwTimeout)
		{
			m_bUnlockFlag = false;
			m_pCs = cs;
			ASSERT(m_pCs);
			m_bLocked = m_pCs->Lock(m_bUnlockFlag, dwTimeout);
		}

	public:
		bool verbose;
		__forceinline Scope()
			:m_pCs(NULL)
			,m_bLocked(false)
			,m_bUnlockFlag(false)
			,verbose(false)
		{
		}
		__forceinline Scope(MutexEx &cs, DWORD dwTimeout = INFINITE)
		{
			verbose = false;
			if( dwTimeout ){
				InternalLock(cs, dwTimeout);
			}
			else{
				m_pCs = &cs, m_bLocked = m_bUnlockFlag = false;
			}
		}
		__forceinline Scope(MutexEx *cs, DWORD dwTimeout = INFINITE)
			:m_pCs(NULL)
			,m_bLocked(false)
			,m_bUnlockFlag(false)
			,verbose(false)
		{
			if( cs && dwTimeout ){
				InternalLock(cs, dwTimeout);
			}
			else{
				m_pCs = cs, m_bLocked = m_bUnlockFlag = false;
			}
		}
		__forceinline ~Scope()
		{
			if( m_pCs && verbose ){
				if( m_bLocked ){
					m_pCs->m.scopesLocked += 1;
				}
				else{
					m_pCs->m.scopesUnlocked += 1;
				}
			}
			InternalUnlock();
		}

		bool Lock(MutexEx &cs, DWORD dwTimeout = INFINITE)
		{
			if (&cs == m_pCs)
				return Lock(dwTimeout);

#ifdef DEBUG
			fprintf( stderr, "InternalUnlock before InternalLock!\n" );
#endif
			InternalUnlock();
			InternalLock(cs, dwTimeout);
			return m_bLocked;
		}
		bool Lock(DWORD dwTimeout = INFINITE)
		{
			ASSERT(m_pCs);
			if (!m_bLocked)
				InternalLock(dwTimeout);
			return m_bLocked;
		}
		void Unlock()
		{
			InternalUnlock();
			m_bUnlockFlag = false;
			m_bLocked = false;
		}

		__forceinline bool TimedOut()
		{
			if( m_pCs ){
				return m_pCs->TimedOut();
			}
			else{
				return false;
			}
		}
		__forceinline bool IsLocked() const { return m_bLocked; }
		__forceinline MutexEx *Parent()
		{
			return m_pCs;
		}
		operator bool () const { return m_bLocked; }
	};
	friend class Scope;
};
#undef __MUTEXEX_CAN_TIMEOUT__
#endif

#define _CRITSECTEX_H
#endif // !_CRITSECTEX_H
