# Road Editor 2

Road Editor 2 は、道路線形・交差点・Terrain を編集するスタンドアローンの道路ネットワーク編集ツールです。  
Qt 6 / C++17 / OpenGL 4.1 を使って実装しています。

## 現在できること

- 道路ネットワーク JSON の読み込み・保存
- Environment JSON の読み込み・保存
- 道路中心線、路面メッシュ、交差点メッシュの表示
- Select / Edit / Vertical / Bank / Lane の各編集モード
- 交差点、ソケット、レーン断面の編集
- Heightmap と Terrain texture のインポート
- Terrain への吸着
- Undo / Redo

## 動作環境

- Windows
- Qt 6.11 系 + MinGW 64-bit
- CMake 3.20 以上
- OpenGL 4.1 対応 GPU

依存ライブラリの `glm` と `nlohmann/json` は、CMake の `FetchContent` で取得されます。

## ビルド

初回設定:

```powershell
cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_MAKE_PROGRAM="C:/qt/Tools/mingw1310_64/bin/mingw32-make.exe" `
  -DCMAKE_PREFIX_PATH="C:/qt/6.11.0/mingw_64"
```

ビルド:

```powershell
$env:PATH='C:\qt\Tools\mingw1310_64\bin;' + $env:PATH
cmake --build build
```

このリポジトリでは `CMakeLists.txt` が `GLOB_RECURSE` を使っているため、新しい `.cpp` / `.h` を追加したあとには再度 `cmake -S . -B build ...` を実行してください。

## 起動

```powershell
build/bin/RoadEditor2.exe
```

または `start.bat` から起動できます。  
起動時は何もロードされていない空の状態で始まります。

## 基本操作

| 入力 | 動作 |
|---|---|
| `Alt + 左ドラッグ` | オービット |
| `Alt + 中ドラッグ` | パン |
| `ホイール` | ズーム |
| `1` | Select モード |
| `2` | Edit モード |
| `3` | Vertical モード |
| `4` | Bank モード |
| `5` | Lane モード |
| `R` | 道路作成 |
| `I` | 交差点作成 |
| `W` | Select モードの移動ギズモ切替 |
| `C` | Terrain へ吸着 |
| `Ctrl + Z / Ctrl + Y` | Undo / Redo |

## データ

- 道路ネットワーク JSON: `version = 4`
- サンプル道路: [data/sample_road.json](data/sample_road.json)
- フォーマット例: [docs/road_data_format.json](docs/road_data_format.json)
- 仕様メモ: [docs/road_data_format_spec.md](docs/road_data_format_spec.md)

## 開発向けドキュメント

- 開発環境のセットアップ: [docs/development_environment.md](docs/development_environment.md)
- 既存メモ: [CLAUDE.md](CLAUDE.md)
