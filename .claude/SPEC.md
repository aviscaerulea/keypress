# keypress 技術仕様書

## 概要

keypress はコマンドライン引数で指定されたキー操作を即座に実行する超軽量 Windows ツール。SendInput API を使用し、単一実行ファイルとして動作する。GUI・コンソールウィンドウは一切表示しない。

## 技術スタック

### API とライブラリ

- **SendInput**: キー入力送信
  - `user32.lib`
  - INPUT 構造体の配列でキー押下・解放を制御
- **VkKeyScanW**: 文字→仮想キーコード変換
  - 任意の文字（記号含む）をキー入力に変換
  - 現在のキーボードレイアウトに依存
- **AttachConsole**: 親コンソールへのアタッチ
  - SUBSYSTEM:WINDOWS でも stderr 出力を実現

### ビルド環境

- 言語: C++
- コンパイラ: Visual Studio 2026 以降（C++ デスクトップ開発ワークロード）
- ビルドツール: PowerShell 7+, Task
- ターゲット: Windows 10/11（x64）

## アーキテクチャ

### 実行フロー

```
1. wWinMain エントリポイント
2. AttachConsole で親コンソールにアタッチ（stderr 用）
3. freopen_s で stderr をコンソール出力にリダイレクト
4. キーマップ初期化（init_keymap）
5. lpCmdLine から引数文字列を取得・トリム
6. ダブルクォートの除去
7. `,` で分割してトークン配列に
8. 各トークンを順次処理（process_token）
   ├─ (数字のみ) → Sleep()
   ├─ `+` を含む → 同時押し（send_keys）
   ├─ キーマップに存在 → 単一キー送信（send_keys）
   └─ 上記以外 → 文字列入力（send_string → send_char）
9. トークン間に 20ms の間隔を挿入
10. 正常終了（コード 0）またはエラー終了（コード 1）
```

### 定数

```cpp
constexpr int KEY_INTERVAL_MS = 20;  // 連続キー入力の間隔（ミリ秒）
```

### 終了コード

| コード | 説明 |
|--------|------|
| 0 | 成功 |
| 1 | エラー |

### エラーメッセージ

| メッセージ | 原因 |
|------------|------|
| `Error: 不明なキー名: xxx` | 同時押し指定で未知のキー名を使用 |
| `Error: 入力できない文字: 'x'` | VkKeyScanW で変換不能な文字 |
| `Error: 文字 'x' の入力に失敗しました` | SendInput の送信失敗 |
| `Warning: キー入力の一部が失敗しました` | SendInput の部分的な失敗 |
| `Warning: スリープ時間が大きすぎます` | stoi のオーバーフロー |

## トークン解釈機構

### 判定優先順位

1. **スリープ**: 正規表現 `^\([0-9]+\)$` にマッチ → `Sleep(ms)`
2. **同時押し**: `+` を含む → キー名を仮想キーコードに変換して `SendInput`
3. **単一キー**: キーマップに存在 or 1 文字の英数字 → `SendInput`
4. **文字列入力**: 上記以外 → `VkKeyScanW` で 1 文字ずつ入力

### スリープ判定

```cpp
static std::wregex pattern(L"^\\([0-9]+\\)$");
```

- `(500)` → スリープ 500ms
- `(500s)` → スリープではなく `(` `5` `0` `0` `s` `)` を文字列入力
- `500` → スリープではなく `5` `0` `0` を文字列入力

### 同時押し機構

1. `+` で分割してキー名配列に
2. 各キー名を仮想キーコードに変換
3. 全キーを順方向に押下
4. 全キーを逆順に解放
5. SendInput に一括送信

### 文字列入力機構（VkKeyScan 方式）

キーマップに存在しない複数文字のトークンは、各文字を `VkKeyScanW` で仮想キーコードと修飾状態に変換して送信する。

1. `VkKeyScanW(ch)` で仮想キーコードと修飾フラグを取得
2. 必要な修飾キー（Shift, Ctrl, Alt）を押下
3. キーを押下・解放
4. 修飾キーを逆順に解放
5. 文字間に 20ms の間隔を挿入

## キーマップ

### 修飾キー

| キー名 | 仮想キーコード |
|--------|----------------|
| shift | VK_SHIFT |
| ctrl | VK_CONTROL |
| alt | VK_MENU |
| win | VK_LWIN |

### 特殊キー

| キー名 | 仮想キーコード | 説明 |
|--------|----------------|------|
| enter | VK_RETURN | Enter キー |
| esc | VK_ESCAPE | Escape キー |
| tab | VK_TAB | Tab キー |
| space | VK_SPACE | Space キー |
| backspace | VK_BACK | Backspace キー |
| delete | VK_DELETE | Delete キー |
| insert | VK_INSERT | Insert キー |
| home | VK_HOME | Home キー |
| end | VK_END | End キー |
| pageup | VK_PRIOR | PageUp キー |
| pagedown | VK_NEXT | PageDown キー |
| up | VK_UP | 上矢印キー |
| down | VK_DOWN | 下矢印キー |
| left | VK_LEFT | 左矢印キー |
| right | VK_RIGHT | 右矢印キー |
| printscreen | VK_SNAPSHOT | PrintScreen キー |
| scrolllock | VK_SCROLL | ScrollLock キー |
| pause | VK_PAUSE | Pause キー |
| numlock | VK_NUMLOCK | NumLock キー |
| capslock | VK_CAPITAL | CapsLock キー |

### ファンクションキー

- `f1`〜`f24`（VK_F1〜VK_F24）

### アルファベット・数字

- `a`〜`z`: 大文字小文字区別なし、`towupper` で仮想キーコードに変換
- `0`〜`9`: 文字コードがそのまま仮想キーコード

## 制約事項

### キー入力対象

- アクティブウィンドウまたはフォーカスを持つアプリケーションにキー入力が送られる
- 特定のウィンドウを指定してキー入力を送ることはできない

### 権限

- 管理者権限で実行されているアプリケーションには SendInput が効かない場合がある
- UIPI（User Interface Privilege Isolation）の制約を受ける

### キーボードレイアウト

- `VkKeyScanW` は現在のキーボードレイアウトに依存
- US 配列と JIS 配列で記号の入力結果が異なる

### 未対応

- NumPad のキー（テンキー）
- マウス操作
- ウィンドウ指定送信

## セキュリティ

### 入力検証

- キー名のバリデーション（キーマップ照合）
- スリープ時間の数値検証（正規表現マッチ）
- 入力不能文字の検出（VkKeyScanW の戻り値チェック）

### 権限モデル

- 通常ユーザ権限で動作
- 管理者権限アプリへの入力は UIPI により制限される

## メンテナンス

### 新しいキーの追加

`init_keymap()` 関数にエントリを追加する。

```cpp
g_keymap[L"newkey"] = VK_NEWKEY;
```

### デバッグ

SUBSYSTEM:CONSOLE でビルドすることでコンソール出力を確認できる。

```cmd
cl /EHsc /W4 /Od /Zi /utf-8 keypress.cpp /link /SUBSYSTEM:CONSOLE user32.lib
```

### ドキュメント

- `README.md`: ユーザ向け仕様書
- `SPEC.md`: 開発者向け技術仕様書
- `.claude/CLAUDE.md`: プロジェクト開発規則
