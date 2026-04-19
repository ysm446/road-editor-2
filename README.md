# Road Editor 2

スタンドアローンの道路ネットワーク編集ツール。Qt 6 + OpenGL 4.1 で実装。

Houdini 製道路ツールをベースに、スタンドアローンアプリとして作り直したもの。
JSON 形式の道路データを読み込み、3D ビューポートで中心線・路面メッシュを表示、制御点をインタラクティブに編集できる。

## 機能

- JSON 道路データの読み込み・保存
- 3D ビューポート（オービットカメラ）
- 路面メッシュ生成（Lambert 陰影付き）
- 制御点の選択・ドラッグ編集
- Undo / Redo（50ステップ）

## 必要環境

- Qt 6.11+ (MinGW 64-bit)
- CMake 3.20+
- OpenGL 4.1 対応 GPU

依存ライブラリ（glm, nlohmann/json）はビルド時に FetchContent で自動取得されます。

## ビルド

PowerShell からビルドする場合は、先に MinGW の `bin` を `PATH` に追加してください。
これを入れないと `g++` 内部の `cc1plus.exe` が必要 DLL を読めず、`cmake --build build`
が途中で失敗することがあります。

```bash
cmake -S . -B build -G "MinGW Makefiles" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_MAKE_PROGRAM="C:/qt/Tools/mingw1310_64/bin/mingw32-make.exe" \
  -DCMAKE_PREFIX_PATH="C:/qt/6.11.0/mingw_64"

$env:PATH='C:\qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake --build build
```

`cmake --build build --config Release -- -j4` でもよいですが、このリポジトリの現在の
構成では上の 2 行をそのまま使うのが安全です。

## 起動

`start.bat` をダブルクリック、または:

```
build/bin/RoadEditor2.exe
```

起動時に `docs/road_data_format.json` が自動読み込みされます。
File > Open で別のファイルを開けます。

## 操作

| 操作 | 内容 |
|---|---|
| Alt + 左ドラッグ | 回転 |
| Alt + 中ドラッグ | パン |
| スクロール | ズーム |
| 左クリック | 制御点を選択（オレンジ表示） |
| 左ドラッグ | 制御点を水平移動 |
| Ctrl+Z / Ctrl+Y | Undo / Redo |

## データ形式

JSON バージョン 3。`docs/road_data_format.json` を参照。
