# keypress

**コマンドラインからキー入力を自動化する、超軽量 Windows ツール**

CLI からグローバルホットキーの押下などを自動化できる。
実行時にウィンドウは一切表示しない。

## 特徴

- 単一実行ファイル、ランタイム依存なし
- 修飾キー（Shift, Ctrl, Alt, Win）の同時押しに対応
- アルファベット、数字、ファンクションキー、特殊キーを網羅
- ミリ秒単位のスリープ挿入で複合操作を記述可能
- 文字列の連続入力に対応（VkKeyScanW による記号文字サポート）
- バックグラウンド実行（GUI・コンソールウィンドウなし）
- SendInput API による確実なキー入力送信
- 連続キー入力間に 20ms の間隔を自動挿入（同時押し誤認防止）

## 使い方

```
keypress.exe "操作1,操作2,..."
```

```powershell
# グローバルホットキー
keypress.exe "shift+ctrl+alt+a"

# デスクトップ表示
keypress.exe "win+d"

# コピー → タブ切替 → ペースト
keypress.exe "ctrl+c,(500),alt+tab,(200),ctrl+v"

# 文字列を入力
keypress.exe "hello"

# 修飾キー付きキーとスリープ
keypress.exe "shift+a,(100),ctrl+b"

# エラー出力をキャプチャ
keypress.exe "invalid_key" 2>&1 | Out-File error.log
```

## 引数形式

| 記法 | 説明 | 例 |
|------|------|----|
| `+` | 同時押し | `shift+a`, `ctrl+alt+delete` |
| `,` | 操作の区切り | `a,b,c` |
| `(数値)` | ミリ秒スリープ | `(500)` |
| 文字列 | 1 文字ずつ入力 | `hello`, `500` |

## 対応キー

### 修飾キー

- `shift`, `ctrl`, `alt`, `win`

### アルファベット

- `a`〜`z`（大文字小文字区別なし）

### 数字

- `0`〜`9`

### ファンクションキー

- `f1`〜`f24`

### 特殊キー

- `enter`, `esc`, `tab`, `space`, `backspace`, `delete`
- `insert`, `home`, `end`, `pageup`, `pagedown`
- `up`, `down`, `left`, `right`
- `printscreen`, `scrolllock`, `pause`, `numlock`, `capslock`

## 終了コード

| コード | 説明 |
|--------|------|
| 0 | 成功 |
| 1 | エラー（不正な引数、不明なキー名、入力不能文字など） |

## インストール

[Scoop](https://scoop.sh/) でインストールできる。

```powershell
scoop bucket add aviscaerulea https://github.com/aviscaerulea/scoop-bucket
scoop install keypress
```

## 動作要件

### 実行

- Windows 10/11（x64）
- 追加のソフトウェアは不要

### ビルド

- [Visual Studio](https://visualstudio.microsoft.com/) 2026 以降（C++ デスクトップ開発ワークロード）
- [PowerShell 7+](https://github.com/PowerShell/PowerShell)（pwsh）
- [Task](https://taskfile.dev/)

## ビルド方法

```powershell
task build
```

手動でビルドする場合:

```powershell
Enable-VSDev
cl /EHsc /W4 /O2 /GL /utf-8 /Fo"out/" /Fe"out/" keypress.cpp /link /LTCG /SUBSYSTEM:WINDOWS user32.lib
```

## 動作の仕組み

1. `wWinMain` エントリポイントで起動（ウィンドウなし）
2. `AttachConsole` で親コンソールにアタッチし stderr を有効化
3. 引数文字列を `,` で分割してトークン化
4. 各トークンを判定:
   - `(数字のみ)` → `Sleep()` でスリープ
   - `+` を含む → 同時押しとして `SendInput` で送信
   - キーマップに存在 → 単一キーとして `SendInput` で送信
   - 上記以外 → `VkKeyScanW` で 1 文字ずつキー入力
5. 連続するキー操作間に 20ms の間隔を自動挿入

## なぜ AutoHotKey ではないのか

主な目的がグローバルホットキー押下に限定されており、キー操作の複雑なスクリプティングは不要なため、VC++ で高速軽量に動作する単一実行ファイルとして作成した。

## ライセンス

MIT
