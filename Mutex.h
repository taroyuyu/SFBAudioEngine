/*
 *  Copyright (C) 2010, 2011, 2012, 2013 Stephen F. Booth <me@sbooth.org>
 *  All Rights Reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *
 *    - Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    - Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *    - Neither the name of Stephen F. Booth nor the names of its 
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <pthread.h>

/*! @file Mutex.h @brief A \c pthread_mutex_t wrapper */

/*! @brief \c SFBAudioEngine's encompassing namespace */
namespace SFB {

	/*! @brief A wrapper around a pthread mutex */
	class Mutex
	{
	public:
		/*!
		 * @brief Create a new \c Mutex
		 * @throws std::runtime_exception
		 */
		Mutex();

		/*! @brief Destroy this \c Mutex*/
		virtual ~Mutex();

		/*! @cond */

		/*! @internal This class is non-copyable */
		Mutex(const Mutex& rhs) = delete;

		/*! @internal This class is non-assignable */
		Mutex& operator=(const Mutex& rhs) = delete;

		/*! @endcond */

		/*!
		 * @brief Lock this \c Mutex.
		 * @return \c true if the lock was obtained, \c false otherwise
		 * @throws std::runtime_exception
		 */
		bool Lock();

		/*!
		 * @brief Unlock this \c Mutex
		 * @throws std::runtime_exception
		 */
		void Unlock();

		/*!
		 * @brief Attempt to lock this \c Mutex
		 * @return \c true if the lock is held by the calling thread, \c false otherwise
		 * @throws std::runtime_exception
		 */
		bool TryLock();

		/*!
		 * @brief Attempt to lock this \c Mutex
		 * @param acquiredLock \c true if the lock was acquired in the call, \c false otherwise
		 * @return \c true if the lock is held by the calling thread, \c false otherwise
		 * @throws std::runtime_exception
		 */
		bool TryLock(bool& acquiredLock);

		/*!
		 * @brief Determine if the calling thread owns the \c Mutex
		 * @return \c true if the calling thread owns the \c Mutex, \c false otherwise
		 */
		inline bool Owned() const { return pthread_equal(mOwner, pthread_self()); }

	protected:
		pthread_mutex_t mMutex;		/*!< @brief The pthread mutex */
		pthread_t mOwner;			/*!< @brief The thread that owns the mutex */

	public:

		/*! @brief A scope based wrapper around \c Mutex::Lock() */
		class Locker
		{
		public:
			/*!
			 * @brief Create a new \c Mutex::Locker()
			 * On creation this class calls \c Mutex::Lock().
			 * On destruction, if the lock was acquired \c Mutex::Unlock() is called.
			 * @param mutex The \c Mutex to lock
			 * @throws std::runtime_exception
			 */
			Locker(Mutex& mutex);

			/*! Destroy this \c Mutex::Locker */
			~Locker();

		private:
			Mutex& mMutex;		/*!< The associated \c Mutex */
			bool mReleaseLock;	/*!< Whether the destructor should call \c Mutex::Unlock() */
		};

		/*! @brief A scope based wrapper around \c Mutex::TryLock() */
		class Tryer
		{
		public:
			/*!
			 * @brief Create a new \c Mutex::Tryer()
			 * On creation this class calls \c Mutex::TryLock().
			 * On destruction, if the lock was acquired \c Mutex::Unlock() is called.
			 * @param mutex The \c Mutex to attempt to lock
			 * @throws std::runtime_exception
			 */
			Tryer(Mutex& mutex);

			/*! @brief Destroy this \c Mutex::Tryer */
			~Tryer();

			/*!
			 * @brief Determine if the mutex is owned and locked by the calling thread
			 * @return true if the mutex is owned and locked, false otherwise
			 */
			inline operator bool() const { return mLocked; }

		private:
			Mutex& mMutex;		/*!< The associated \c Mutex */
			bool mLocked;		/*!< Whether the mutex is locked */
			bool mReleaseLock;	/*!< Whether the destructor should call \c Mutex::Unlock() */
		};
	};
	
}