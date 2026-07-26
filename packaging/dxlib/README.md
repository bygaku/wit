# Witas — DxLib 用パッケージ

DxLib と一緒に使うための Witas ランタイムです。

```
include/                    公開ヘッダ (wit.h, wit_types.h)
lib/x64/Release/wit.lib     Release 用 (/MT)
lib/x64/Debug/wit.lib       Debug 用   (/MTd)
lib/x64/Debug/wit.pdb       Debug 用シンボル
```

**Visual Studio 2015 以降 / x64** 向けです。

`.lib` のファイル名は Debug と Release で同じで、**フォルダだけが違います。**

---

## 導入手順

DxLib の導入が終わっている前提です。プロジェクトのプロパティを開いてください。

### 1. インクルードパス

`構成プロパティ` → `C/C++` → `全般` → `追加のインクルードディレクトリ`

このパッケージの `include` フォルダを追加します。Debug / Release 共通です。

### 2. ライブラリパス

`構成プロパティ` → `リンカー` → `全般` → `追加のライブラリディレクトリ`

**構成ごとに別のフォルダを指定してください。**

- `Release` 構成 → `lib\x64\Release`
- `Debug` 構成 → `lib\x64\Debug`

### 3. 追加の依存ファイル

`構成プロパティ` → `リンカー` → `入力` → `追加の依存ファイル`

`wit.lib` を追加します。Debug / Release 共通です（フォルダで切り替わるため）。

### 4. ランタイムライブラリ

DxLib の導入時に設定済みのはずですが、念のため確認してください。

`構成プロパティ` → `C/C++` → `コード生成` → `ランタイムライブラリ`

- `Release` 構成 → `マルチスレッド (/MT)`
- `Debug` 構成 → `マルチスレッド デバッグ (/MTd)`

Witas はこの設定でビルドされています。`/MD` や `/MDd` にすると `LNK2038` が出ます。

---

## DxLib と併用するときの注意

### DxLib のサウンド関数は使わないでください

`LoadSoundMem` / `PlaySoundMem` などの DxLib のサウンド機能と Witas は、
同じオーディオデバイスを別々に開きます。音声再生はすべて Witas 側に寄せてください。

### DxLib を WASAPI 排他モードで動かさないでください

排他モードはデバイスを占有するため、Witas の初期化に失敗します。
DxLib の既定は DirectSound なので、明示的に切り替えていなければ問題ありません。

### 初期化と終了の順序

```cpp
#include "DxLib.h"
#include "wit.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    ChangeWindowMode(TRUE);
    if (DxLib_Init() < 0) return -1;

    WitInitParams params = {};
    if (WitInit("assets/game.wpb", &params) != WIT_RESULT_SUCCESS) {
        DxLib_End();
        return -1;
    }

    SetDrawScreen(DX_SCREEN_BACK);

    while (ScreenFlip() == 0 && ProcessMessage() == 0 && ClearDrawScreen() == 0) {
        WitSystem_Update();   // 毎フレーム呼ぶ

        if (CheckHitKey(KEY_INPUT_ESCAPE) != 0) break;
    }

    WitUninit();
    DxLib_End();
    return 0;
}
```

- `WitInit` は `DxLib_Init()` の**後**に呼んでください。
- `WitUninit` は `DxLib_End()` の**前**に呼んでください。
- `WitSystem_Update` はゲームループから**毎フレーム**呼んでください。
  これを忘れると再生終了の検出やストリーミングの補充が進みません。

Witas の API はすべてメインスレッドから呼ぶ前提です。
ゲーム側にコールバックを渡す API はないため、DxLib の関数が
オーディオスレッドから呼ばれることはありません。

---

## トラブルシューティング

### `LNK2038: 'RuntimeLibrary' の不一致が検出されました`

`ランタイムライブラリ` が `/MT` `/MTd` になっているか確認してください。
`Debug` 構成で `lib\x64\Release` を参照している場合も同じエラーが出ます。

### `LNK2019: 外部シンボル ... は未解決です`

`追加のライブラリディレクトリ` が構成に合っているか確認してください。

### `LNK1112: モジュールのコンピューター '<arch>' の種類が競合しています`

プロジェクトのプラットフォームが `x64` になっているか確認してください。

### `WitInit` が `WIT_RESULT_FILE_NOT_FOUND` を返す

`.wpb` のパスは実行ファイルからの相対パスです。
Visual Studio から実行する場合の作業ディレクトリはプロジェクトフォルダなので、
`exe` を直接起動したときとパスが変わります。