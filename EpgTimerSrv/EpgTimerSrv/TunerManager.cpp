#include "StdAfx.h"
#include "TunerManager.h"


CTunerManager::CTunerManager(void)
{
}


CTunerManager::~CTunerManager(void)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
}

BOOL CTunerManager::FindBonFileName(wstring src, wstring& dllName)
{
	wstring buff = src;
	size_t pos = buff.rfind(L")");
	if( pos == string::npos ){
		dllName = src;
		return FALSE;
	}

	int count = 1;
	for( size_t i=pos-1; i>=0; i-- ){
		if(buff.compare(i,1,L")") == 0 ){
			count++;
		}else if(buff.compare(i,1,L"(") == 0){
			count--;
		}
		if( count == 0 ){
			dllName = buff.substr(0, i);
			break;
		}
	}

	return TRUE;
}

//�`���[�i�[�ꗗ�̓ǂݍ��݂��s��
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
BOOL CTunerManager::ReloadTuner()
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		SAFE_DELETE(itr->second);
	}
	this->tunerMap.clear();

	wstring path = L"";
	GetSettingPath(path);

	wstring srvIniPath = L"";
	GetEpgTimerSrvIniPath(srvIniPath);

	wstring searchKey = path;
	searchKey += L"\\*.ChSet4.txt";

	WIN32_FIND_DATA findData;
	HANDLE find;

	//�w��t�H���_�̃t�@�C���ꗗ�擾
	find = FindFirstFile( searchKey.c_str(), &findData);
	if ( find == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}
	do{
		if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
			//�{���Ɋg���qDLL?
			if( IsExt(findData.cFileName, L".txt") == TRUE ){
				wstring chSetPath = L"";
				Format(chSetPath, L"%s\\%s", path.c_str(), findData.cFileName);

				wstring bonFileName = L"";
				wstring buff = findData.cFileName;

				FindBonFileName(buff, bonFileName);

				bonFileName += L".dll";

				WORD count = (WORD)GetPrivateProfileInt(bonFileName.c_str(), L"Count", 0, srvIniPath.c_str());
				if( count != 0 ){
					//�J�E���g0�ȏ�̂��̂������p
					WORD priority = (WORD)GetPrivateProfileInt(bonFileName.c_str(), L"Priority", 0, srvIniPath.c_str());
					BOOL epgCapFlag = (BOOL)GetPrivateProfileInt(bonFileName.c_str(), L"GetEpg", 1, srvIniPath.c_str());
					WORD EPGcount = (WORD)GetPrivateProfileInt(bonFileName.c_str(), L"EPGCount", count, srvIniPath.c_str());
					if(EPGcount==0)	EPGcount = count;

					for( WORD i=1; i<=count; i++ ){
						TUNER_INFO* item = new TUNER_INFO;
						item->bonID = priority;
						item->tunerID = i;
						if(EPGcount<i){		//	EPGCount�𒴂��Ă�����EPG�擾�Ɏg�p���Ȃ�
							item->epgCapFlag = 0;
						} else {
							item->epgCapFlag = epgCapFlag;
						}
						item->bonFileName = bonFileName;
						item->chUtil.ParseText(chSetPath.c_str());
						item->chSet4FilePath = chSetPath;
						DWORD key = ((DWORD)item->bonID)<<16 | item->tunerID;
						this->tunerMap.insert(pair<DWORD, TUNER_INFO*>(key, item));
					}
				}
			}
		}
	}while(FindNextFile(find, &findData));

	FindClose(find);

	return TRUE;
}


//�`���[�i�[��ID�ꗗ���擾����B
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// idList			[OUT]�`���[�i�[��ID�ꗗ
BOOL CTunerManager::GetEnumID(
	vector<DWORD>* idList
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		idList->push_back(itr->first);
	}
	return TRUE;
}

//�`���[�i�[�\�񐧌���擾����
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// ctrlMap			[OUT]�`���[�i�[�\�񐧌�̈ꗗ
BOOL CTunerManager::GetEnumTunerBank(
	map<DWORD, CTunerBankCtrl*>* ctrlMap
	)
{
	if( ctrlMap == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		CTunerBankCtrl* ctrl = new CTunerBankCtrl;
		ctrl->SetTunerInfo( itr->second->bonID, itr->second->tunerID, itr->second->bonFileName, itr->second->chSet4FilePath);
		ctrlMap->insert(pair<DWORD, CTunerBankCtrl*>(itr->first, ctrl));
	}
	return TRUE;
}

//�w��T�[�r�X���T�|�[�g���Ă��Ȃ��`���[�i�[�ꗗ���擾����
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// ONID				[IN]�m�F�������T�[�r�X��ONID
// TSID				[IN]�m�F�������T�[�r�X��TSID
// SID				[IN]�m�F�������T�[�r�X��SID
// idList			[OUT]�`���[�i�[��ID�ꗗ
BOOL CTunerManager::GetNotSupportServiceTuner(
	WORD ONID,
	WORD TSID,
	WORD SID,
	vector<DWORD>* idList
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		multimap<LONGLONG, CH_DATA4>::iterator itrCh;
		LONGLONG key = _Create64Key(ONID, TSID, SID);
		itrCh = itr->second->chUtil.chList.find(key);
		if( itrCh == itr->second->chUtil.chList.end() ||
			!itrCh->second.useViewFlag ){
				idList->push_back(itr->first);
		}

	}
	return TRUE;
}

BOOL CTunerManager::GetSupportServiceTuner(
	WORD ONID,
	WORD TSID,
	WORD SID,
	vector<DWORD>* idList
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		multimap<LONGLONG, CH_DATA4>::iterator itrCh;
		LONGLONG key = _Create64Key(ONID, TSID, SID);
		itrCh = itr->second->chUtil.chList.find(key);
		if( itrCh != itr->second->chUtil.chList.end() &&
			itrCh->second.useViewFlag ){
				idList->push_back(itr->first);
		}

	}
	return TRUE;
}

BOOL CTunerManager::GetCh(
	DWORD tunerID,
	WORD ONID,
	WORD TSID,
	WORD SID,
	DWORD* space,
	DWORD* ch
	)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		multimap<LONGLONG, CH_DATA4>::iterator itrCh;
		LONGLONG key = _Create64Key(ONID, TSID, SID);
		itrCh = itr->second->chUtil.chList.find(key);
		if( itrCh != itr->second->chUtil.chList.end() ){
			if( space != NULL ){
				*space = itrCh->second.space;
			}
			if( ch != NULL ){
				*ch = itrCh->second.ch;
			}
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CTunerManager::GetEnumEpgCapTuner(
	vector<DWORD>* idList
	)
{
	if( idList == NULL ){
		return FALSE;
	}
	map<DWORD, TUNER_INFO*>::iterator itr;
	for( itr = this->tunerMap.begin(); itr != this->tunerMap.end(); itr++ ){
		if( itr->second->epgCapFlag == TRUE ){
			idList->push_back(itr->first);
		}
	}
	return TRUE;
}

BOOL CTunerManager::IsSupportService(
	DWORD tunerID,
	WORD ONID,
	WORD TSID,
	WORD SID
	)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return FALSE;
	}

	multimap<LONGLONG, CH_DATA4>::iterator itrCh;
	LONGLONG key = _Create64Key(ONID, TSID, SID);
	itrCh = itr->second->chUtil.chList.find(key);
	if( itrCh == itr->second->chUtil.chList.end() ){
		return FALSE;
	}
	return TRUE;
}

BOOL CTunerManager::GetBonFileName(
	DWORD tunerID,
	wstring& bonFileName
	)
{
	map<DWORD, TUNER_INFO*>::iterator itr;
	itr = this->tunerMap.find(tunerID);
	if( itr == this->tunerMap.end() ){
		return FALSE;
	}
	bonFileName = itr->second->bonFileName;

	return TRUE;
}
