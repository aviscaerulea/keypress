# CLAUDE.md

## プロジェクト概要

@../README.md

## 詳細仕様

@SPEC.md

## 開発環境

### 必須ツール

- VisualStudio 2026 Community
- VC++ デスクトップ開発ワークロード

### ビルド環境の準備

PowerShell で VisualStudio の開発環境を有効化する。

```powershell
Enable-VSDev
```

### ビルド

```cmd
task build
```

手動でビルドする場合:

```cmd
cl /EHsc /W4 /O2 /GL /utf-8 /Fo"out/" /Fe"out/" keypress.cpp /link /LTCG /SUBSYSTEM:WINDOWS user32.lib
```

### 生成物

- `out/keypress.exe`: 実行ファイル
- `out/keypress.obj`: オブジェクトファイル

### クリーン

```cmd
task clean
```

## 実装上の注意点

### エントリポイント

- `wWinMain` を使用（`/SUBSYSTEM:WINDOWS`）
- コンソールウィンドウを表示しない

### エラー出力

- `AttachConsole(ATTACH_PARENT_PROCESS)` で親コンソールにアタッチ
- `freopen_s` で stderr をコンソール出力にリダイレクト
- Unicode 出力のため `_setmode(_fileno(g_stderr), _O_U16TEXT)` を設定

### キー入力送信

- `SendInput` API を使用
- INPUT 構造体の配列を作成
- キー押下→キー解放の順で INPUT を追加

### 文字列処理

- `std::wstring` を使用（Unicode 対応）
- キー名は大文字小文字を区別しない（内部で小文字に変換）
- トリム処理で空白を除去

## テストシナリオ

### 正常系

1. 単一キー: `keypress.exe "a"`
2. 修飾キー同時押し: `keypress.exe "shift+a"`
3. 複数修飾キー: `keypress.exe "ctrl+shift+a"`
4. 連続入力: `keypress.exe "a,b,c"`
5. スリープ挿入: `keypress.exe "a,(100),b"`
6. Win キー: `keypress.exe "win+d"`
7. 複合操作: `keypress.exe "ctrl+c,(500),alt+tab,(200),ctrl+v"`

### 異常系

1. 引数なし: `keypress.exe ""`
2. 不明なキー: `keypress.exe "invalid_key"`
3. 不正なスリープ: `keypress.exe "(abc)"`
4. 負のスリープ: `keypress.exe "(-100)"`

## 保守

### 新しいキーの追加

`init_keymap()` 関数に追加する。

```cpp
g_keymap[L"newkey"] = VK_NEWKEY;
```

### デバッグ

SUBSYSTEM:CONSOLE でビルドすることでコンソール出力を確認できる。

```cmd
cl /EHsc /W4 /Od /Zi /utf-8 keypress.cpp /link /SUBSYSTEM:CONSOLE user32.lib
```
