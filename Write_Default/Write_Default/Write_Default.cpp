// Write_Default.cpp : DLL �A�v���P�[�V�����p�ɃG�N�X�|�[�g�����֐����`���܂��B
//

#include "stdafx.h"

#include "Write_PlugIn.h"
#include "WriteMain.h"
#include "SettingDlg.h"
#include <tchar.h>
#include <map>
using namespace std;

extern map<DWORD, CWriteMain*> g_List;
DWORD g_nextID = 1;

extern HINSTANCE g_instance;

#define PLUGIN_NAME L"�f�t�H���g 188�o�C�gTS�o�� PlugIn"

DWORD GetNextID()
{
	DWORD nextID = 0xFFFFFFFF;

	map<DWORD, CWriteMain*>::iterator itr;
	itr = g_List.find(g_nextID);
	if( itr == g_List.end() ){
		nextID = g_nextID;
		g_nextID++;
		if( g_nextID == 0 || g_nextID == 0xFFFFFFFF){
			g_nextID = 1;
		}
	}else{
		for( DWORD i=1; i<0xFFFFFFFF; i++ ){
			itr = g_List.find(g_nextID);
			if( itr == g_List.end() ){
				nextID = g_nextID;
				g_nextID++;
				if( g_nextID == 0 || g_nextID == 0xFFFFFFFF){
					g_nextID = 1;
				}
				break;
			}else{
				g_nextID++;
			}
			if( g_nextID == 0 || g_nextID == 0xFFFFFFFF){
				g_nextID = 1;
			}
		}
	}

	return nextID;
}


//PlugIn�̖��O���擾����
//name��NULL���͕K�v�ȃT�C�Y��nameSize�ŕԂ�
//�ʏ�nameSize=256�ŌĂяo��
//�߂�l
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// name                     [OUT]����
// nameSize                 [IN/OUT]name�̃T�C�Y(WCHAR�P��)
BOOL WINAPI GetPlugInName(
	WCHAR* name,
	DWORD* nameSize
	)
{
	if( name == NULL ){
		if( nameSize == NULL ){
			return FALSE;
		}else{
			*nameSize = (DWORD)wcslen(PLUGIN_NAME)+1;
		}
	}else{
		if( nameSize == NULL ){
			return FALSE;
		}else{
			if( *nameSize < (DWORD)wcslen(PLUGIN_NAME)+1 ){
				*nameSize = (DWORD)wcslen(PLUGIN_NAME)+1;
				return FALSE;
			}else{
				wcscpy_s(name, *nameSize, PLUGIN_NAME);
			}
		}
	}
	return TRUE;
}

//PlugIn�Őݒ肪�K�v�ȏꍇ�A�ݒ�p�̃_�C�A���O�Ȃǂ�\������
//�����F
// parentWnd                [IN]�e�E�C���h�E
void WINAPI Setting(
	HWND parentWnd
	)
{
	WCHAR dllPath[MAX_PATH] = L"";
	GetModuleFileName(g_instance, dllPath, MAX_PATH);

	wstring iniPath = dllPath;
	iniPath += L".ini";

	CSettingDlg dlg;

	auto itos = [](int i) { wstring s; Format(s,L"%d",i); return s; };

	dlg.size = itos(GetPrivateProfileInt(L"SET", L"Size", DEFAULT_BUFFER_SIZE, iniPath.c_str()));
	dlg.packet = itos(GetPrivateProfileInt(L"SET", L"Packet", DEFAULT_BUFFER_PACKET, iniPath.c_str()));
	dlg.reserve = GetPrivateProfileInt(L"SET", L"Reserve", 1, iniPath.c_str()) ? true : false ;
	dlg.priority = GetPrivateProfileInt(L"SET", L"Priority", 0, iniPath.c_str()) ;
	dlg.flush = GetPrivateProfileInt(L"SET", L"Flush", 0, iniPath.c_str()) ? true : false ;
    dlg.shrink = GetPrivateProfileInt(L"SET", L"Shrink", 0, iniPath.c_str()) ? true : false ;

	if( dlg.CreateSettingDialog(g_instance, parentWnd) == IDOK ){
		WritePrivateProfileString(L"SET", L"Size", dlg.size.c_str(), iniPath.c_str());
		WritePrivateProfileString(L"SET", L"Packet", dlg.packet.c_str(), iniPath.c_str());
		WritePrivateProfileString(L"SET", L"Reserve", itos(dlg.reserve).c_str(), iniPath.c_str());
		WritePrivateProfileString(L"SET", L"Priority", itos(dlg.priority).c_str(), iniPath.c_str());
		WritePrivateProfileString(L"SET", L"Flush", itos(dlg.flush).c_str(), iniPath.c_str());
		WritePrivateProfileString(L"SET", L"Shrink", itos(dlg.shrink).c_str(), iniPath.c_str());
	}

//  MessageBox(parentWnd, PLUGIN_NAME, L"Write PlugIn", MB_OK);
}

//�����ۑ��Ή��̂��߃C���X�^���X��V�K�ɍ쐬����
//�����Ή��ł��Ȃ��ꍇ�͂��̎��_�ŃG���[�Ƃ���
//�߂�l
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id               [OUT]����ID
BOOL WINAPI CreateCtrl(
	DWORD* id
	)
{
	if( id == NULL ){
		return FALSE;
	}

	CWriteMain* main = new CWriteMain;
	*id = GetNextID();
	g_List.insert(pair<DWORD, CWriteMain*>(*id, main));

	return TRUE;
}

//�C���X�^���X���폜����
//�߂�l
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id               [IN]����ID
BOOL WINAPI DeleteCtrl(
	DWORD id
	)
{
	map<DWORD, CWriteMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	delete itr->second;

	g_List.erase(itr);

	return TRUE;
}

//�t�@�C���ۑ����J�n����
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id                   [IN]����ID
// fileName             [IN]�ۑ��t�@�C���t���p�X�i�K�v�ɉ����Ċg���q�ς�����ȂǍs���j
// overWriteFlag        [IN]����t�@�C�������ݎ��ɏ㏑�����邩�ǂ����iTRUE�F����AFALSE�F���Ȃ��j
// createSize           [IN]���͗\�z�e�ʁi188�o�C�gTS�ł̗e�ʁB�����^�掞�ȂǑ����Ԗ���̏ꍇ��0�B�����Ȃǂ̉\��������̂Ŗڈ����x�j
BOOL WINAPI StartSave(
	DWORD id,
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize
	)
{
	map<DWORD, CWriteMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->_StartSave(fileName, overWriteFlag, createSize);
}

//�t�@�C���ۑ����I������
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id                   [IN]����ID
BOOL WINAPI StopSave(
	DWORD id
	)
{
	map<DWORD, CWriteMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->_StopSave();
}

//���ۂɕۑ����Ă���t�@�C���p�X���擾����i�Đ���o�b�`�����ɗ��p�����j
//filePath��NULL���͕K�v�ȃT�C�Y��filePathSize�ŕԂ�
//�ʏ�filePathSize=512�ŌĂяo��
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id                   [IN]����ID
// filePath             [OUT]�ۑ��t�@�C���t���p�X
// filePathSize         [IN/OUT]filePath�̃T�C�Y(WCHAR�P��)
BOOL WINAPI GetSaveFilePath(
	DWORD id,
	WCHAR* filePath,
	DWORD* filePathSize
	)
{
	map<DWORD, CWriteMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->_GetSaveFilePath(filePath, filePathSize);
}

//�ۑ��pTS�f�[�^�𑗂�
//�󂫗e�ʕs���Ȃǂŏ����o�����s�����ꍇ�AwriteSize�̒l������
//�ēx�ۑ���������Ƃ��̑��M�J�n�n�_�����߂�
//�߂�l�F
// TRUE�i�����j�AFALSE�i���s�j
//�����F
// id                   [IN]����ID
// data                 [IN]TS�f�[�^
// size                 [IN]data�̃T�C�Y
// writeSize            [OUT]�ۑ��ɗ��p�����T�C�Y
BOOL WINAPI AddTSBuff(
	DWORD id,
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	map<DWORD, CWriteMain*>::iterator itr;
	itr = g_List.find(id);
	if( itr == g_List.end() ){
		return FALSE;
	}

	return itr->second->_AddTSBuff(data, size, writeSize);
}
