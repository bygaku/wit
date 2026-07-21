# Wit

学生のゲーム開発のための、軽量なサウンドミドルウェア。

Wit は、ゲームプログラミングを学ぶ学生に向けた軽量なオーディオミドルウェアです。
既存のサウンドミドルウェアのようなプロ向けツールに対し、学習コストと機能をあえて絞り、
とっつきやすい代替、エントリーモデルとなることを目指しています。Wit は次の 2 つで構成されます。

- **Witas**（Wit Audio System） — ゲームに組み込む C/C++ オーディオランタイム。
- **WitStudio** — Witas が読み込むオーディオデータを作成する GUI オーサリングツール。

バージョン: 1.0.0（beta.2）
ライセンス: MIT（`LICENSE` を参照）。同梱しているサードパーティ製コンポーネントは
それぞれ独自のライセンスに従います（`THIRD_PARTY_LICENSES.txt` を参照）。

## ターゲット
チームにサウンドの担当者がいない、ゲーム開発をしている学生

## パッケージの内容

```
wit-sdk-0.9.0/
├─ include/                 # ランタイムライブラリの公開ヘッダ
│   ├─ wit.h
│   └─ wit_types.h
├─ lib/
│   └─ x64/
│       ├─ Debug/   wit.lib (+ wit.pdb)
│       └─ Release/ wit.lib
├─ studio/                  # WitStudio.exe
├─ LICENSE
├─ THIRD_PARTY_LICENSES.txt
└─ README.md
```

## Witas について

Witas は x64（Windows, MSVC / VS2022）向けの静的ライブラリとして提供されます。
C++17 対応のコンパイラが必要です。

1. `include/` をプロジェクトのインクルードディレクトリに追加します。
2. ビルド構成に合わせて `.lib` をリンクします。
    - Debug ビルド   → `lib/x64/Debug/wit.lib`
    - Release ビルド → `lib/x64/Release/wit.lib`
3. 公開ヘッダをインクルードします。

   ```cpp
   #include <wit.h>
   ```

## WitStudio について

`studio/WitStudio.exe` を実行します。必要な Qt は同じフォルダに同梱
されているため、Qt を別途インストールする必要はありません。`WitStudio.exe` と、
その隣にある DLL 群・`platforms/` フォルダは必ず一緒に置いてください。分離すると
起動しません。

プロジェクトをビルドすると `wit_assets/` が生成されます。
`wit_assets/` 内にランタイムで読み込むための `.wpb` / `.wccb` / `.wwb` が生成されます。

### CueCollection (.wccb)

1つのプロジェクトに複数の `CueColelction` を追加できます。

### Cue

1つの `CueColelction` に対して複数の `Cue` を追加できます。
`Cue` には登録した `Waveform` の鳴らし方を設定できます。

### Waveform (.wwb)

1つの `Cue` に対して最大 16 個の `Waveform` を追加できます。
`Waveform` に登録する音声ファイルの最大の文字数は 63 文字です。

## 補足

- WitStudio は、Witas ランタイムが再生時に読み込むバイナリのオーディオデータ
  （RIFF 形式のチャンクファイル）を生成します。
- Witas ランタイムは Qt に依存しません。Qt に依存するのは WitStudio だけです。

## ライセンスについて

Wit 自身のコードは MIT ライセンスです。WitStudio は Qt 6 を LGPLv3 のもとで同梱し、
オーディオバックエンドには miniaudio（パブリックドメイン / MIT-0）を使用しています。
詳細と、同梱している Qt バイナリに適用される義務については
`THIRD_PARTY_LICENSES.txt` に記載しています。