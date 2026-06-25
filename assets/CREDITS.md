# Asset Credits & Licenses

外部資產出處與授權。所有外部模型皆為 public domain / CC0，
並記錄出處以利報告引用。

## robonaut2
- 角色: 主要 AI 外部 OBJ — 人形服務機器人。NASA 真實工程機體, 辨識度最高且與晚餐桌『人 vs 機器』敘事最契合。
- 授權: NASA media — public domain (see NASA usage guidelines)
- 出處: NASA / NASA-3D-Resources (Robonaut 2)
- 來源: https://raw.githubusercontent.com/nasa/NASA-3D-Resources/master/3D%20Models/Robonaut%202/Robonaut%202.glb
- 格式: glb -> convert to OBJ
- 檔案: Robonaut 2.glb
- sha256: `21739a4f7632cb8f34d12fd988900cb132c83d1447c007dd2ed170282b17b5b8`

## ingenuity
- 角色: Orbit 主角 — 自主旋翼無人機。真實火星直升機, 讀作 surveillance drone, 旋翼造型適合繞巨眼公轉。
- 授權: NASA media — public domain (see NASA usage guidelines)
- 出處: NASA / NASA-3D-Resources (Ingenuity Mars Helicopter)
- 來源: https://raw.githubusercontent.com/nasa/NASA-3D-Resources/master/3D%20Models/Ingenuity%20Mars%20Helicopter/Ingenuity%20Mars%20Helicopter.glb
- 格式: glb -> convert to OBJ
- 檔案: Ingenuity Mars Helicopter.glb
- sha256: `9e5a5398ab335eae7d46335085301ce46fb7de052300b645debe8bac6d39f2b5`

## textures (P3)

全部 6 張貼圖由 `scripts/gen_textures.py` 以程序生成，屬自製素材，CC0 釋出。

| 檔案 | 用途 | 技術 |
|------|------|------|
| `wood.png` | 桌面 + 桌腳 | 正弦波木紋條紋 |
| `cloth.png` | 椅座 + 椅背 | 週期性編織格紋 |
| `circuit.png` | 巨眼虹膜電路環 (AI 物件必要項) | PCB 走線 + 焊盤隨機分佈 |
| `marble.png` | 鞏膜球 + 瓷盤 | 正弦波大理石紋 |
| `metal.png` | 虹膜框 + 縫隙環 + 線纜 | 水平磨砂拉絲 |
| `paper.png` | 桌上信封 | 白紙纖維噪點 |

- 授權: CC0 (自製，無版權限制)
- 生成腳本: `scripts/gen_textures.py`
- 尺寸: 512×512 PNG

## quaternius_scifi
- 角色: 風格化備援 — 若需要更乾淨 game-ready 拓樸的 sci-fi 機器人/面板。原生 OBJ。僅在 NASA 機體拓樸不利著色時啟用。
- 授權: CC0
- 出處: Quaternius (CC0)
- 來源: https://quaternius.com/  (Sci-Fi / Animated Robot packs)
- 格式: native OBJ/FBX/glTF
- 檔案: (manual download)
- sha256: `—`
