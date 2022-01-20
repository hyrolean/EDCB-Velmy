#pragma once

#include <memory>
#include <deque>

#include "../../Common/Util.h"
#include "../../Common/StringUtil.h"

// ����o�b�t�@�e�ʂ� DEFAULT_BUFFER_SIZE �~ DEFAULT_BUFFER_PACKET �̃o�C�g�e��
// �K��l 385024 bytes * 32 packets = 12320768 bytes = �ő� 94 Mbits
// �� �n�f�W��16Mbps�̃r�b�g���[�g�ōő��6�b�㕪�̃o�b�t�@�ۏ�e�ʂƂ��Čv��
// �� �����l�� 385024 bytes * 2 packets = 770048 bytes �̍ŏ��e�ʂŎn�܂�A
//    �e�ʂ�����Ȃ���� �ő� 94 Mbits �܂Ŏ����Ő�����U�����
#define DEFAULT_BUFFER_SIZE 385024
#define DEFAULT_BUFFER_PACKET 32

class CWriteMain
{
public:
	CWriteMain(void);
	~CWriteMain(void);

	//�t�@�C���ۑ����J�n����
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	//�����F
	// fileName             [IN]�ۑ��t�@�C���t���p�X�i�K�v�ɉ����Ċg���q�ς�����ȂǍs���j
	// overWriteFlag        [IN]����t�@�C�������ݎ��ɏ㏑�����邩�ǂ����iTRUE�F����AFALSE�F���Ȃ��j
	// createSize           [IN]���͗\�z�e�ʁi188�o�C�gTS�ł̗e�ʁB�����^�掞�ȂǑ����Ԗ���̏ꍇ��0�B�����Ȃǂ̉\��������̂Ŗڈ����x�j
	BOOL _StartSave(
		LPCWSTR fileName,
		BOOL overWriteFlag,
		ULONGLONG createSize
		);

	//�t�@�C���ۑ����I������
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	BOOL _StopSave(
		);

	//���ۂɕۑ����Ă���t�@�C���p�X���擾����i�Đ���o�b�`�����ɗ��p�����j
	//filePath��NULL���͕K�v�ȃT�C�Y��filePathSize�ŕԂ�
	//�ʏ�filePathSize=512�ŌĂяo��
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	//�����F
	// filePath             [OUT]�ۑ��t�@�C���t���p�X
	// filePathSize         [IN/OUT]filePath�̃T�C�Y(WCHAR�P��)
	BOOL _GetSaveFilePath(
		WCHAR* filePath,
		DWORD* filePathSize
		);

	//�ۑ��pTS�f�[�^�𑗂�
	//�󂫗e�ʕs���Ȃǂŏ����o�����s�����ꍇ�AwriteSize�̒l������
	//�ēx�ۑ���������Ƃ��̑��M�J�n�n�_�����߂�
	//�߂�l�F
	// TRUE�i�����j�AFALSE�i���s�j
	//�����F
	// data                 [IN]TS�f�[�^
	// size                 [IN]data�̃T�C�Y
	// writeSize            [OUT]�ۑ��ɗ��p�����T�C�Y
	BOOL _AddTSBuff(
		BYTE* data,
		DWORD size,
		DWORD* writeSize
		);

protected: // MPWM (Massive Packet Writing Moderator) MOD Fixed by hyrolean
	class PACKET {
		BYTE *data_;
		size_t size_;
		size_t wrote_;
	public:
		PACKET(size_t packet_size) {
			data_ = new BYTE[packet_size];
			size_ = packet_size;
			wrote_ = 0;
		}
		~PACKET() {
			delete [] data_ ;
		}
		size_t write(BYTE *buffer, size_t buffer_size) {
			if(wrote_>=size_) return 0;
			size_t w = min(buffer_size , size_ - wrote_);
			memcpy(&data_[wrote_], buffer, w);
			wrote_ += w ;
			return w;
		}
		void clear() { wrote_ = 0 ; }
		BYTE *data() { return data_; }
		size_t size() { return size_; }
		size_t wrote() { return wrote_; }
		bool full() { return wrote_ >= size_ ; }
	};
	vector< shared_ptr<PACKET> > packets ;
	vector<int> emptyIndices;
	deque<int> queueIndices;
	int pushingIndex;

	HANDLE writerThread;
	HANDLE pusherEvent, writerEvent;
	BOOL writerFailed, writerTerminated;
	CRITICAL_SECTION critical;

	BOOL WriterWriteOnePacket();
	unsigned int WriterThreadProcMain () ;
	static unsigned int __stdcall WriterThreadProc (PVOID pv) ;

protected:
	HANDLE file;
	wstring savePath;
	ULONGLONG reserveSize;

	size_t bufferSize;
	size_t maxPackets;
protected:
	BOOL GetNextFileName(wstring filePath, wstring& newPath);

};

