/****************************************************************************
*																			*
* CriticalSection.h 														*
*																			*
* Create by :																*
* Kingfisher	 															*
* 																			*
* Description: 																*
* 封装Win32临界量对象和互斥量内核对象										*
****************************************************************************/

#pragma once
#ifdef WIN32
#include <windows.h>

class CCriSec
{
public:
    CCriSec()		{::InitializeCriticalSection(&m_crisec);}
    ~CCriSec()		{::DeleteCriticalSection(&m_crisec);}

    void Lock()		{::EnterCriticalSection(&m_crisec);}
    void Unlock()	{::LeaveCriticalSection(&m_crisec);}

private:
    CCriSec(const CCriSec& cs);
    CCriSec operator = (const CCriSec& cs);

private:
    CRITICAL_SECTION    m_crisec;
};

#else
#include <pthread.h>

class CCriSec
{
public:
    CCriSec()		{pthread_mutex_init(&m_sect, nullptr);}
    ~CCriSec()		{pthread_mutex_destroy(&m_sect);}

    void Lock()		{pthread_mutex_lock(&m_sect);}
    void Unlock()	{pthread_mutex_unlock(&m_sect);}

private:
    CCriSec(const CCriSec& cs);
    CCriSec operator = (const CCriSec& cs);

private:
    pthread_mutex_t    m_sect;
};
#endif

template<class CLockObj> class CLocalLock
{
public:
	CLocalLock(CLockObj& obj) : m_lock(obj) {m_lock.Lock();}
	~CLocalLock() {m_lock.Unlock();}
private:
	CLockObj& m_lock;
};

typedef CLocalLock<CCriSec>		CCriSecLock;
