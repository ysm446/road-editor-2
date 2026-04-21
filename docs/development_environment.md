# Development Environment

このページは、Road Editor 2 をこのリポジトリ上で開発するために必要な環境をまとめたものです。  
「必須のもの」と「あると便利なもの」を分けて整理しています。

## 前提

- OS: Windows
- 文字コード: UTF-8
- ビルド方式: CMake + Qt 6 + MinGW 64-bit

## 必須

### 1. Git

リポジトリの取得と更新に使います。

- `git` コマンドが通ること
- GitHub から clone / pull できること

### 2. Qt 6.11 系

このプロジェクトは Qt Widgets / OpenGL / OpenGLWidgets を使っています。  
Qt Online Installer などで、少なくとも次を入れてください。

- `Qt 6.11.x`
- `MinGW 64-bit` 向け Qt
- `Tools > MinGW 13.1.0 64-bit`

このリポジトリでは、現在次のパスを前提にしたコマンドを使っています。

- `C:/qt/6.11.0/mingw_64`
- `C:/qt/Tools/mingw1310_64/bin`

Qt のバージョン番号が違う場合は、`CMAKE_PREFIX_PATH` を自分の環境に合わせて読み替えてください。

### 3. CMake 3.20 以上

構成生成とビルド実行に使います。

- `cmake` コマンドが通ること

### 4. MinGW 64-bit ツールチェーン

コンパイラは Qt 同梱の MinGW を使う前提です。

- `g++`
- `mingw32-make`

PowerShell からビルドするときは、毎回先に次を実行してください。

```powershell
$env:PATH='C:\qt\Tools\mingw1310_64\bin;' + $env:PATH
```

これを入れないと、`cc1plus.exe` の依存 DLL が見つからずビルドに失敗することがあります。

### 5. OpenGL 4.1 対応 GPU / ドライバ

ビューポート描画に必要です。

- OpenGL 4.1 Core Profile が使えること

## 任意だが推奨

### Visual Studio Code / Visual Studio / Qt Creator

どれも必須ではありません。  
このプロジェクトのビルドは CMake で完結しているので、エディタは好みで構いません。

おすすめの使い分け:

- Visual Studio Code: 軽めに編集したいとき
- Visual Studio: コード参照や補完を重視したいとき
- Qt Creator: Qt プロジェクトとして見たいとき

注意:

- Visual Studio を入れていても、このリポジトリでは基本的に MSVC ではなく MinGW 構成を使います

### Python

現時点ではアプリ本体のビルドには必須ではありません。

使い道:

- 補助スクリプト
- JSON やデータ加工
- 将来のツール拡張

つまり、Python は「あると便利」ですが、「このアプリをビルドするための必須要件」ではありません。

## セットアップ手順

### 1. リポジトリ取得

```powershell
git clone <repository-url>
cd road-editor-2
```

### 2. 初回 configure

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MAKE_PROGRAM="C:/qt/Tools/mingw1310_64/bin/mingw32-make.exe" `
  -DCMAKE_PREFIX_PATH="C:/qt/6.11.0/mingw_64"
```

### 3. ビルド

```powershell
$env:PATH='C:\qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake --build build
```

### 4. 起動

```powershell
build/bin/RoadEditor2.exe
```

または:

```powershell
start.bat
```

## 開発中の注意

### 新しいソースを追加したとき

このリポジトリでは `CMakeLists.txt` が `GLOB_RECURSE` を使っています。  
そのため、新しい `.cpp` / `.h` を増やしたときは再 configure が必要です。

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MAKE_PROGRAM="C:/qt/Tools/mingw1310_64/bin/mingw32-make.exe" `
  -DCMAKE_PREFIX_PATH="C:/qt/6.11.0/mingw_64"
```

### 日本語テキスト

- すべて UTF-8 で扱う
- PowerShell で読むときも UTF-8 を明示する
- 日本語が崩れて見えたら、まず文字コードを疑う

### 実行中でリンク失敗する場合

`RoadEditor2.exe: Permission denied` が出る場合は、アプリが起動したままのことが多いです。  
アプリを閉じてから再ビルドしてください。

## 現時点で入っていると安心なもの一覧

- Git
- CMake 3.20+
- Qt 6.11.x
- Qt MinGW 64-bit
- MinGW 13.1.0 64-bit
- OpenGL 4.1 対応 GPU ドライバ
- 好みのエディタ
- Python 3.x（任意）
