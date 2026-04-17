# Road Data Format

`/road_data_format.json` をもとに、道路データの構造とサンプル内容を整理したメモです。

## 概要

- データ形式は JSON
- トップレベルのキーは `groups` / `intersections` / `roads` / `version`
- このサンプルファイルの `version` は `3`
- サンプル件数は `groups: 1`、`intersections: 8`、`roads: 10`

## トップレベル構造

```json
{
  "groups": [...],
  "intersections": [...],
  "roads": [...],
  "version": 3
}
```

## groups

道路や交差点をまとめるグループ情報です。

### フィールド

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `id` | string | グループ UUID |
| `locked` | boolean | 編集ロック状態 |
| `name` | string | グループ名 |
| `visible` | boolean | 表示状態 |

### サンプル

このファイルでは 1 件のみで、`Default` グループが使われています。

## intersections

交差点ノードの一覧です。各道路の `startIntersectionId` / `endIntersectionId` から参照されます。

### フィールド

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `entryDist` | number | 交差点進入距離らしき値。サンプルでは全件 `8.0` |
| `groupId` | string | 所属グループ UUID |
| `id` | string | 交差点 UUID |
| `name` | string | 交差点名 |
| `pos` | number[3] | 座標 `[x, y, z]` |
| `type` | string | ノード種別。サンプルでは全件 `intersection` |

### サンプル一覧

| Name | ID | Position | EntryDist |
| --- | --- | --- | --- |
| `Intersection 0` | `39d56a9d-0002-4c6c-9df4-68fbe08275c3` | `[19.009, 6.581, 59.325]` | `8.0` |
| `Intersection 1` | `70bbeeca-b402-4031-b07e-694092f07e6c` | `[217.523, 8.339, -150.649]` | `8.0` |
| `Intersection 2` | `e8a20037-05dd-4c95-942b-0c297c3f7c21` | `[-252.914, 5.333, 114.760]` | `8.0` |
| `Intersection 3` | `fbee295d-7d69-4efb-ad9a-208d3573e98f` | `[-148.932, 14.438, -43.456]` | `8.0` |
| `Intersection 4` | `2695d6eb-455d-49e9-be93-1425f74f8ff6` | `[-497.333, 4.000, -49.245]` | `8.0` |
| `Intersection 5` | `75d20551-0fdd-4baa-8236-01efe2deaa96` | `[-258.838, 11.369, -52.798]` | `8.0` |
| `Intersection 6` | `d736c38b-9d45-44e8-8ddd-c21befac8ced` | `[-209.648, 9.602, 6.739]` | `8.0` |
| `Intersection 7` | `bf3b79ff-e1eb-4971-a574-b1e6bd2a4fbf` | `[-210.521, 13.412, -90.419]` | `8.0` |

## roads

道路本体の配列です。始点・終点の交差点参照、デフォルトの車線幅、速度、摩擦、道路形状の制御点列を持ちます。

### フィールド

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `active` | number | 有効フラグ。サンプルでは全件 `1` |
| `bankAngle` | array | バンク角設定。サンプルでは全件空配列 |
| `closed` | boolean | 閉ループ道路かどうか。サンプルでは全件 `false` |
| `defaultFriction` | number | 既定摩擦値。サンプルでは全件 `0.15` |
| `defaultTargetSpeed` | number | 既定目標速度。サンプルでは全件 `40.0` |
| `defaultWidthLaneCenter` | number | 中央帯または中央レーン幅。サンプルでは `0.0` |
| `defaultWidthLaneLeft1` | number | 左 1 車線の既定幅。サンプルでは `4.0` |
| `defaultWidthLaneRight1` | number | 右 1 車線の既定幅。サンプルでは `4.0` |
| `endIntersectionId` | string | 終点交差点 UUID |
| `groupId` | string | 所属グループ UUID |
| `id` | string | 道路 UUID |
| `laneSections` | array | レーン断面の詳細設定。サンプルでは全件空配列 |
| `name` | string | 道路名 |
| `point` | array | 道路中心線の制御点列 |
| `roadType` | number | 道路タイプ。サンプルでは全件 `0` |
| `startIntersectionId` | string | 始点交差点 UUID |
| `verticalCurve` | array | 縦断曲線情報。サンプルでは全件空配列 |

### point 配列の要素

`roads[].point[]` は道路形状の制御点です。

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `curvatureRadius` | number | 曲率半径 |
| `pos` | number[3] | 制御点座標 `[x, y, z]` |
| `useCurvatureRadius` | number | 曲率半径を使うかどうかのフラグ。サンプルでは全件 `0` |

### サンプル上の共通傾向

- 全道路で `defaultTargetSpeed = 40.0`
- 全道路で `defaultFriction = 0.15`
- 全道路で左右 1 車線幅は `4.0`
- 全道路で `defaultWidthLaneCenter = 0.0`
- 全道路で `closed = false`
- 全道路で `roadType = 0`
- 全道路で `laneSections` / `bankAngle` / `verticalCurve` は空配列

### サンプル道路一覧

| Road | Start | End | Control Points | Width L/C/R | Speed | Friction |
| --- | --- | --- | --- | --- | --- | --- |
| `Road 0` | `Intersection 2` | `Intersection 0` | `8` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 1` | `Intersection 0` | `Intersection 1` | `4` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 0 Copy A` | `Intersection 6` | `Intersection 3` | `4` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 0 Copy B` | `Intersection 3` | `Intersection 0` | `5` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 4` | `Intersection 2` | `Intersection 4` | `5` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 0 Copy A Copy A` | `Intersection 4` | `Intersection 5` | `5` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 0 Copy A Copy B` | `Intersection 5` | `(未接続)` | `3` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 7` | `Intersection 5` | `Intersection 6` | `4` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 8` | `Intersection 3` | `Intersection 7` | `4` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |
| `Road 9` | `Intersection 2` | `Intersection 6` | `4` | `4.0 / 0.0 / 4.0` | `40.0` | `0.15` |

## 補足

- `endIntersectionId` が空文字の道路が 1 本あり、サンプル上では終点未接続の道路を表している可能性があります
- `laneSections`、`bankAngle`、`verticalCurve` はこのサンプルでは未使用ですが、将来的な拡張用フィールドの可能性があります
- 実運用上の意味づけは、エディタ側の Python や Viewer State 実装と照合するとさらに明確になります

## vNext 提案

以下は、現在の `road_data_format.json` を大きく壊さずに拡張する場合の提案です。

方針は次の通りです。

- `roads` が道路本体と道路端メタデータを持つ
- `intersections` は接続トポロジーと交差点形状ルールを持つ
- 停止線や進入位置は `intersections` ではなく `roads` 側に持つ
- 交差点は道路を読んで形状生成するが、道路端の最終値は所有しない

## roads の拡張案

### 追加したいキー

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `endMetadata` | object | 始点側 / 終点側の端点メタデータ |
| `metadataPoints` | array | 道路上の一般メタデータポイント列 |

### `endMetadata` の例

```json
{
  "start": {
    "entryOffset": 0.0,
    "stopLineOffset": -2.0,
    "crosswalkOffset": -5.0,
    "yieldType": "stop",
    "signalId": "signal_a"
  },
  "end": {
    "entryOffset": 1.0,
    "stopLineOffset": 0.5,
    "crosswalkOffset": -2.5,
    "yieldType": "yield",
    "signalId": ""
  }
}
```

### `endMetadata` のフィールド候補

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `entryOffset` | number | 道路端の接続基準点からの前後オフセット |
| `stopLineOffset` | number | 停止線の前後オフセット |
| `crosswalkOffset` | number | 横断歩道の前後オフセット |
| `yieldType` | string | `stop` / `yield` / `priority` など |
| `signalId` | string | 信号制御との関連 ID |

### `metadataPoints` の例

```json
[
  {
    "type": "speed",
    "u_coord": 0.25,
    "value": 50.0
  },
  {
    "type": "marking",
    "u_coord": 0.7,
    "value": "double_yellow"
  }
]
```

### `metadataPoints` のフィールド候補

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `type` | string | メタデータ種別 |
| `u_coord` | number | 道路上の正規化位置 `0.0 - 1.0` |
| `value` | any | 種別ごとの値 |

### `roads` の全体例

```json
{
  "id": "road_01",
  "name": "Road 01",
  "groupId": "group_default",
  "roadType": 0,
  "active": 1,
  "closed": false,
  "startIntersectionId": "intersection_a",
  "endIntersectionId": "intersection_b",
  "defaultTargetSpeed": 40.0,
  "defaultFriction": 0.15,
  "defaultWidthLaneCenter": 0.0,
  "defaultWidthLaneLeft1": 4.0,
  "defaultWidthLaneRight1": 4.0,
  "point": [
    {
      "pos": [0.0, 0.0, 0.0],
      "useCurvatureRadius": 0,
      "curvatureRadius": 0.0
    }
  ],
  "verticalCurve": [],
  "bankAngle": [],
  "laneSections": [],
  "endMetadata": {
    "start": {
      "entryOffset": 0.0,
      "stopLineOffset": -2.0,
      "crosswalkOffset": -5.0,
      "yieldType": "stop",
      "signalId": "signal_a"
    },
    "end": {
      "entryOffset": 1.0,
      "stopLineOffset": 0.5,
      "crosswalkOffset": -2.5,
      "yieldType": "yield",
      "signalId": ""
    }
  },
  "metadataPoints": [
    {
      "type": "speed",
      "u_coord": 0.25,
      "value": 50.0
    }
  ]
}
```

## intersections の拡張案

### 追加したい考え方

`intersections` は、道路端の個別値を持つのではなく、以下を持つとよいです。

- 接続トポロジー
- 交差点タイプ
- 交差点形状ルール
- 生成用の既定値

### 追加したいキー

| フィールド | 型 | 内容 |
| --- | --- | --- |
| `intersectionType` | string | `standard` / `roundabout` など |
| `connectedRoads` | array | 接続道路一覧 |
| `shapeRule` | string | 交差点生成方式 |
| `cornerRadius` | number | 標準交差点用のコーナーR |
| `outerRadius` | number | ラウンドアバウト外径 |
| `islandRadius` | number | ラウンドアバウト中央島半径 |
| `entryDistDefault` | number | 進入距離の既定値 |

### `connectedRoads` の例

```json
[
  {
    "roadId": "road_01",
    "roadEnd": "end"
  },
  {
    "roadId": "road_02",
    "roadEnd": "start"
  }
]
```

### `intersections` の全体例

```json
{
  "id": "intersection_a",
  "name": "Intersection A",
  "groupId": "group_default",
  "pos": [10.0, 0.0, 20.0],
  "type": "intersection",
  "intersectionType": "standard",
  "entryDist": 8.0,
  "entryDistDefault": 8.0,
  "cornerRadius": 6.0,
  "shapeRule": "auto_corner",
  "connectedRoads": [
    {
      "roadId": "road_01",
      "roadEnd": "end"
    },
    {
      "roadId": "road_02",
      "roadEnd": "start"
    }
  ]
}
```

ラウンドアバウトであれば、例えば次のように持てます。

```json
{
  "id": "roundabout_a",
  "name": "Roundabout A",
  "groupId": "group_default",
  "pos": [0.0, 0.0, 0.0],
  "type": "intersection",
  "intersectionType": "roundabout",
  "entryDist": 10.0,
  "outerRadius": 20.0,
  "islandRadius": 9.0,
  "shapeRule": "roundabout_auto",
  "connectedRoads": [
    {
      "roadId": "road_10",
      "roadEnd": "start"
    },
    {
      "roadId": "road_11",
      "roadEnd": "end"
    }
  ]
}
```

## 所有責務の整理

### `roads` が持つもの

- 道路線形
- 縦断
- バンク角
- 車線構成
- 幅員
- 停止線や横断歩道などの端点メタデータ
- 制限速度などの一般メタデータ

### `intersections` が持つもの

- 接続トポロジー
- 交差点タイプ
- 交差点形状ルール
- 交差点生成の既定値

### `intersections` が持たないほうがよいもの

- 各道路ごとの停止線位置
- 各道路ごとの進入位置微調整
- 各道路ごとの車線構成の最終値
- 各道路ごとの端点メタデータのコピー

## 移行しやすさ

既存フォーマットから移行するときは、次の順で進めるのが安全です。

1. 既存の `roads` / `intersections` は残す
2. `roads` に `endMetadata` と `metadataPoints` を追加する
3. `intersections` に `intersectionType` と `connectedRoads` を追加する
4. 停止線や進入位置は新規データから `roads.endMetadata` に保存する
5. 交差点生成側は `roads` を読む実装へ段階的に切り替える

この順なら、既存の `startIntersectionId` / `endIntersectionId` を活かしながら拡張できます。
