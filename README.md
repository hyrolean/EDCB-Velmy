EpgDataCap_Bon
==============
**BonDriver based multifunctional EPG software**

Documents are stored in the 'Document' directory.
Configuration files are stored in the 'ini' directory.

**このForkについて**

自分で使うために自由に改造しています。
プルリクエストはしないので、必要ならつまみ食いしてください。

## ■変更点(by hyrolean)
- Windows10(20H1/20H2)上で発生するUDP送信時のもたつきの現象を修正
- 開発環境をVS2015へ移行
- バッチファイルに渡されたファイル名に%文字が含まれる場合に、ファイル名が化ける現象を修正
- 高速チャンネルスキャンに対応(BonCtrl.ini の \[CHSCAN\] セクションに FastScan=1 を指定することによって機能 **※1**)
- Write_Default.dll プラグインのバッファリングを改善(録画時のドロップ発生率を低減) **MOD2**

  **※1:高速チャンネルスキャンの動作には、FastScanに対応したBonDriver([BonDriver_PTx-ST](https://github.com/hyrolean/BonDriver_PTx-ST_mod)など)が別途必要**
