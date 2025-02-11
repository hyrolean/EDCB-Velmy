#include "StdAfx.h"
#include "SendUDP.h"
#include <process.h>

//マルチメディアタイマーを有効にして割込み精度を上げるかどうか(0:しない 1:する)
#define USE_MM_INTERVAL_LOCK 0
//パケット送信前に送信可能になるまでselectで待機させるかどうか(0:しない 1:する)
#define USE_SELECT_SEND 1

#if USE_MM_INTERVAL_LOCK
#include <MMSystem.h>
#pragma comment(lib, "WinMM.lib")
#endif

#define SEND_BUFF (188)


CSendUDP::CSendUDP(void)
{
    m_hSendThread = NULL;
    m_hSendStopEvent = _CreateEvent(FALSE, FALSE, NULL);

	m_hCriticalEvent = _CreateEvent(FALSE, TRUE, NULL);

	WSAData wsaData;
	WSAStartup(MAKEWORD(2,0), &wsaData);

	WCHAR strExePath[512] = L"";
	GetModuleFileName(NULL, strExePath, 512);

	WCHAR szPath[_MAX_PATH];	// パス
	WCHAR szFile[_MAX_PATH];	// ファイル
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_tsplitpath_s( strExePath, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_tmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );
	_tmakepath_s(  szFile, _MAX_PATH, L"", L"", szFname, szExt );

	Format(m_strIniPath, L"%s%s.ini",szPath,szFname);

}

CSendUDP::~CSendUDP(void)
{
	if( m_hSendThread != NULL ){
		::SetEvent(m_hSendStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hSendThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hSendThread, 0xffffffff);
		}
		CloseHandle(m_hSendThread);
		m_hSendThread = NULL;
	}
	if( m_hSendStopEvent != NULL ){
		CloseHandle(m_hSendStopEvent);
		m_hSendStopEvent = NULL;
	}

	if( m_hCriticalEvent == NULL ){
		::WaitForSingleObject(m_hCriticalEvent, 300);
		::CloseHandle(m_hCriticalEvent);
		m_hCriticalEvent = NULL;
	}

	for( int i=0; i<(int)m_TSBuff.size(); i++ ){
		SAFE_DELETE(m_TSBuff[i])
	}
	m_TSBuff.clear();

	CloseUpload();

	WSACleanup();

}

BOOL CSendUDP::StartUpload( vector<NW_SEND_INFO>* List )
{
	if( SockList.size() > 0 ){
		return FALSE;
	}

	m_uiWait = GetPrivateProfileInt( L"Set", L"SendWait", 4, m_strIniPath.c_str() );

	for( int i=0; i<(int)List->size(); i++ ){
		SOCKET_DATA Item;
		Item.sock = socket(AF_INET, SOCK_DGRAM, 0);
		if( Item.sock == INVALID_SOCKET ){
			CloseUpload();
			return FALSE;
		}
		//ノンブロッキングモードへ
		ULONG x = 1;
		ioctlsocket(Item.sock,FIONBIO, &x);

		if( (*List)[i].broadcastFlag == FALSE ){
			Item.addr.sin_family = AF_INET;
			Item.addr.sin_port = htons((WORD)(*List)[i].port);
			string strA="";
			WtoA((*List)[i].ipString, strA);
			Item.addr.sin_addr.S_un.S_addr = inet_addr(strA.c_str());

			//DWORD dwHost = inet_addr("127.0.0.1");
			//setsockopt(Item.sock, IPPROTO_IP,IP_MULTICAST_IF,(char *)&dwHost, sizeof(DWORD));
		}else{
			BOOL b=1;
			Item.addr.sin_family = AF_INET;
			Item.addr.sin_port = htons((WORD)(*List)[i].port);
			string strA="";
			WtoA((*List)[i].ipString, strA);
			Item.addr.sin_addr.S_un.S_addr = inet_addr(strA.c_str());
			setsockopt(Item.sock,SOL_SOCKET, SO_BROADCAST, (char *)&b, sizeof(b));
		}
		SockList.push_back(Item);
	}

	if( m_hSendThread == NULL ){
		m_uiSendSize = GetPrivateProfileInt( L"Set", L"SendSize", 128, m_strIniPath.c_str() )*188;
		//解析スレッド起動
		ResetEvent(m_hSendStopEvent);
		m_hSendThread = (HANDLE)_beginthreadex(NULL, 0, SendThread, (LPVOID)this, CREATE_SUSPENDED, NULL);
		SetThreadPriority( m_hSendThread, THREAD_PRIORITY_HIGHEST );
		ResumeThread(m_hSendThread);

	}

	return TRUE;
}

BOOL CSendUDP::CloseUpload()
{
	if( m_hSendThread != NULL ){
		::SetEvent(m_hSendStopEvent);
		// スレッド終了待ち
		if ( ::WaitForSingleObject(m_hSendThread, 15000) == WAIT_TIMEOUT ){
			::TerminateThread(m_hSendThread, 0xffffffff);
		}
		CloseHandle(m_hSendThread);
		m_hSendThread = NULL;
	}

	for( int i=0; i<(int)SockList.size(); i++ ){
		closesocket(SockList[i].sock);
	}
	SockList.clear();

	if( WaitForSingleObject( m_hCriticalEvent, 500 ) == WAIT_OBJECT_0 ){
		for( int i=0; i<(int)m_TSBuff.size(); i++ ){
			SAFE_DELETE(m_TSBuff[i])
		}
		m_TSBuff.clear();
	}
	if( m_hCriticalEvent != NULL ){
		SetEvent(m_hCriticalEvent);
	}

	return TRUE;
}

void CSendUDP::SendData(BYTE* pbBuff, DWORD dwSize)
{
	if( m_hSendThread != NULL ){
		TS_DATA* pItem = new TS_DATA;
		pItem->data = new BYTE[dwSize];
		ZeroMemory( pItem->data, dwSize );
		memcpy( pItem->data, pbBuff, dwSize );
		pItem->size = dwSize;

		if( WaitForSingleObject( m_hCriticalEvent, 500 ) == WAIT_OBJECT_0 ){
			m_TSBuff.push_back(pItem);
		}else{
			delete pItem;
		}
		if( m_hCriticalEvent != NULL ){
			SetEvent(m_hCriticalEvent);
		}
	}
}

UINT WINAPI CSendUDP::SendThread(LPVOID pParam)
{
	CSendUDP* pSys = (CSendUDP*)pParam;

	BYTE* pbSendBuff = new BYTE[pSys->m_uiSendSize];
	DWORD dwSendSize = 0;

	auto dur = [](DWORD s=0, DWORD e=GetTickCount()) -> DWORD {
		return (e>=s) ? e-s : s + 0xFFFFFFFF-e + 1 ;
	};

#if USE_MM_INTERVAL_LOCK
	class mm_interval_lock {
		DWORD period_;
	public:
		mm_interval_lock(DWORD period) : period_(period) {
			timeBeginPeriod(period_);
		}
		~mm_interval_lock() {
			timeEndPeriod(period_);
		}
	};
	mm_interval_lock interlock(10);
#endif


#if USE_SELECT_SEND
	auto doSelectSend = [](SOCKET sock, UINT wait, bool *sendOk=nullptr, bool *excepted=nullptr) {
		fd_set fdSend, fdExcept ;
		ZeroMemory(&fdSend,sizeof fdSend) ;
		FD_ZERO(&fdSend);
		FD_SET(sock,&fdSend);
		fdExcept = fdSend ;
		timeval time_limit ;
		time_limit.tv_sec = 0 ;
		time_limit.tv_usec = wait*1000 ;
		auto r=select(sock+1,NULL,&fdSend,&fdExcept,&time_limit) ;
		if(sendOk) *sendOk = FD_ISSET(sock, &fdSend) ? true : false ;
		if(excepted) *excepted = FD_ISSET(sock, &fdExcept) ? true : false ;
		return r ;
	};
#endif

	DWORD totalSleep = 0;
	auto doSleep = [&](UINT wait) {
		if(totalSleep<wait) {
			DWORD s = dur() ;
			DWORD t = wait-totalSleep ;
			Sleep(t);
			DWORD elap = dur(s) ;
			if(elap<t) totalSleep=0;
			else totalSleep = elap-t;
		}else
			totalSleep-=wait;
	};


	while(1){
		if( ::WaitForSingleObject(pSys->m_hSendStopEvent, 0) != WAIT_TIMEOUT ){
			//キャンセルされた
			break;
		}

		TS_DATA* pData = NULL;

		if( WaitForSingleObject( pSys->m_hCriticalEvent, 500 ) == WAIT_OBJECT_0 ){
			if(pSys->m_TSBuff.size()>500){
				for( int i=(int)pSys->m_TSBuff.size()-500; i>=0; i-- ){
					SAFE_DELETE(pSys->m_TSBuff[i]);
					vector<TS_DATA*>::iterator itr;
					itr = pSys->m_TSBuff.begin();
					advance(itr,i);
					pSys->m_TSBuff.erase( itr );
				}
			}
			if( pSys->m_TSBuff.size() != 0 ){
				pData = pSys->m_TSBuff[0];
				pSys->m_TSBuff.erase( pSys->m_TSBuff.begin() );
			}
		}else{
			continue ;
		}
		if( pSys->m_hCriticalEvent != NULL ){
			SetEvent(pSys->m_hCriticalEvent);
		}

		if( pData != NULL && pSys->SockList.size() > 0 ){
			dwSendSize = 0;

			DWORD dwRead = 0 ;
			while(dwRead < pData->size){
				int iSendSize = 0;
				if( dwRead+pSys->m_uiSendSize < pData->size ){
					iSendSize = pSys->m_uiSendSize;
				}else{
					iSendSize = pData->size-dwRead;
				}
#if USE_SELECT_SEND
				for( int i=(int)pSys->SockList.size()-1; i>=0 ; i-- ){
					doSelectSend(pSys->SockList[i].sock, pSys->m_uiWait);
				}
#endif
				for( int i=(int)pSys->SockList.size()-1; i>=0 ; i-- ){
					if( sendto(pSys->SockList[i].sock, (char*)pData->data+dwRead, iSendSize, 0, (struct sockaddr *)&pSys->SockList[i].addr, sizeof(pSys->SockList[i].addr)) == 0){
						closesocket(pSys->SockList[i].sock);
						vector<SOCKET_DATA>::iterator itr;
						itr = pSys->SockList.begin();
						advance(itr, i);
						pSys->SockList.erase(itr);
						//break;
					}
				}
#if !USE_SELECT_SEND
				if(pSys->m_TSBuff.size()<390) {
					doSleep(pSys->m_uiWait);
				}else
					totalSleep=0;
#endif
				dwRead+=iSendSize;
			}
		}else if( pSys->m_TSBuff.empty() ){
			doSleep(1);
		}

		SAFE_DELETE(pData);
	}

	SAFE_DELETE_ARRAY(pbSendBuff);

	return TRUE;
}