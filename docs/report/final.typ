#import "theme.typ": *

#show: project-report

#set page(width: 210mm, height: 297mm, margin: 0pt, fill: cover, numbering: none)

#image("assets/hero.png", width: 100%)
#place(top + right, dx: -14pt, dy: 14pt, image("assets/decor/eye_icon.svg", width: 36pt))
#v(16pt)
#pad(x: 18pt)[
  #text(size: 10pt, fill: rgb("#CFC7BF"))[Computer Graphics Final Project 2026]
  #v(7pt)
  #text(size: 32pt, weight: "bold", fill: rgb("#F4F3F1"))[Future's Gaze]
  #linebreak()
  #text(size: 24pt, weight: "bold", fill: accent)[未來的視線]
  #v(12pt)
  #rulebar()
  #v(12pt)
  #text(size: 11pt, fill: rgb("#E8E2DB"))[
    一個以 OpenGL 固定管線完成的互動式 AI 未來世界：中央的 Prediction Core
    不是單純的眼球，而是一套正在觀看、預測、記憶與忽略的具體系統。
  ]
  #v(16pt)
  #text(size: 10pt, fill: rgb("#D8D0C8"))[學號：114598085  ／  姓名：葉騏緯]
]

#pagebreak()
#set page(
  width: 210mm,
  height: 297mm,
  margin: (x: 19mm, y: 18mm),
  fill: paper,
  numbering: "1",
)

= Project Setup

runtime 僅依賴 OpenGL、FreeGLUT 與 `stb_image.h` (已 vendor 入 repo)。

#table(
  columns: (1fr, 1fr, 2.1fr),
  inset: 6pt,
  stroke: 0.35pt + rgb("#C8C1BA"),
  [依賴], [最低版本], [用途],
  [C++ compiler], [C++20], [建議 clang 17+、GCC 13+、Apple Clang 15+ 或 MSVC 19.38+。],
  [CMake], [3.24], [使用 `CMakePresets.json` 管理 dev / release / observer 等 preset。],
  [Ninja], [any], [Linux、macOS、MSYS2 的主要建置 generator。],
  [OpenGL], [legacy], [需支援 fixed pipeline lighting 與 stencil buffer。],
  [FreeGLUT], [3.x], [提供視窗、滑鼠與鍵盤 callback。],
)

== 安裝依賴

#codeblock[
  ```bash
  # Ubuntu / Debian
  sudo apt install clang ninja-build cmake libgl-dev freeglut3-dev

  # Arch / Manjaro
  sudo pacman -S clang ninja cmake mesa freeglut

  # macOS Homebrew
  brew install ninja cmake freeglut

  # Windows MSYS2 clang64
  pacman -S mingw-w64-clang-x86_64-clang \
            mingw-w64-clang-x86_64-cmake \
            mingw-w64-clang-x86_64-ninja \
            mingw-w64-clang-x86_64-freeglut
  ```
]

== Build 與 Run

#codeblock[
  ```bash
  # Release build
  cmake --preset release
  cmake --build --preset release

  # 執行
  ./build/release/future_gaze

  # 若要跑測試
  cmake --preset dev
  cmake --build --preset dev
  ctest --preset dev
  ```
]

Visual Studio 使用者可改用 `release-vs` preset。macOS 的 `FindGLUT` 會優先選擇系統 `GLUT.framework`，`CMAKE_PREFIX_PATH` 對此無效；需改用 `-DGLUT_INCLUDE_DIR="$(brew --prefix freeglut)/include"` 與 `-DGLUT_glut_LIBRARY="$(brew --prefix freeglut)/lib/libglut.dylib"` 顯式覆寫。執行後視窗預設為 `1280 × 720`，可自由縮放。若 OBJ 模型不在 repo 中，依 `scripts/README.md` 執行 fetch + convert 腳本重新生成。

= User Guide

操作設計分成兩層：一般模式控制相機；Gaze 模式把左鍵拖曳改成控制中央 Prediction Core 的視線。這讓觀眾可以先把整個場景當作一座雕塑觀看，再進入「被 AI 注視後，世界如何變形」的互動層。

== Camera

#table(
  columns: (1fr, 2fr),
  inset: 6pt,
  stroke: 0.35pt + rgb("#C8C1BA"),
  [輸入], [效果],
  [滑鼠左鍵拖曳], [以餐桌與巨眼為中心環繞相機，觀察桌面、桌下與 AI 配角。],
  [滑鼠中鍵或右鍵拖曳], [平移相機，微調截圖構圖或靠近某個物件。],
  [滾輪], [縮放視角，拉近虹膜電路、Robonaut 金屬材質或桌面陰影。],
  [`R`], [重置相機到預設位置，回到全景構圖。],
)

== Gaze

#table(
  columns: (1fr, 2fr),
  inset: 6pt,
  stroke: 0.35pt + rgb("#C8C1BA"),
  [輸入], [效果],
  [`G`], [切換 Gaze control mode。開啟後，左鍵拖曳不再轉相機，而是轉動巨眼視線。],
  [Gaze 模式下左鍵拖曳], [改變 Prediction Core 的 yaw / pitch，觸發預見、眷戀、盲點三種區域權重。],
  [`H`], [讓巨眼視線回到 resting direction，三區域權重也回到初始狀態。],
  [`V`], [顯示或關閉 gaze-debug overlay，可看見視線方向、盲點方向與區域邊界。],
  [`Esc`], [離開程式。],
)

#img("assets/gaze_debug.png", [Debug overlay：`V` 讓不可見的視線計算顯形，方便確認三種觀看法則的邊界與方向。])

= 螢幕截圖與故事主線

多年以後，當那些系統已經被遺忘，人們仍然記得那張晚餐桌是如何靜止著，宛如時間在離席瞬間凝固，拒絕繼續往前流動。

椅子沒有推回原位，杯中的水仍在晃動。空氣記得剛才有人的手臂掠過，尚未決定要如何復原。桌面的摺痕記得每一頓曾在上面吃過的飯。盤子壓著一封信，沒有人知道它還要被壓多久。一顆眼球就懸浮在那邊。虹膜裡走著電路紋，機械光圈等待一個足以定格的瞬間，資料校準環以不可見的速度旋轉，後方的線纜沒入場景邊緣，連接進某個尚未到來的地方——那裡已經知道今晚接下來會發生什麼。

它開始觀看。

#img("assets/hero.png", [序幕：一套 AI 預測系統正在觀看一張留下人類情感殘痕的晚餐桌。])

== 第一幕：預見

#twocol(
  image-card("assets/foresight_zone.png", [預見區], [結果已經侵入原因。被系統確信的未來，先於現在取得體積。]),
  image-card(
    "assets/prediction_core.png",
    [觀測核心],
    [Prediction Core 是預見的源頭——電路虹膜將桌面轉譯為機率地圖，讓每個物件先看見自己的下一秒。],
  ),
)

視線落在桌面，酒杯的輪廓開始多出幾個自己。

杯身仍在桌上，切片向上張開，如同一列按照機率排列的透明證人。最高機率的倒落路徑最厚重，像一塊已經落下的石頭；較不可能的軌跡則薄如紙張，在桌面幾毫米處顫抖，等待某個從未到來的確認。桌面已經長出水漬，地面已經排好碎片的輪廓——後果先於原因抵達，穩穩在那裡等待剩下的事情發生。

越被系統確信的未來，越像實體。越不被預期的可能，只能以薄片、邊緣或微弱形體停留在空間裡，等待被否決。

#pagebreak()

== 第二幕：眷戀

#img("assets/longing_zone.png", [眷戀區：被愛過的物件，不再具有唯一形狀。])

當視線轉動，那張普通的椅子開始顯示它同時是多張椅子。

同一件物件，因為曾被不同的人愛過，而擁有了不同的幾何面。朝老人的方向，椅面凹陷成一個清楚的人形痕跡，如同某個在那裡坐了數十年的人仍然在場；朝孩子的方向，椅腳與桌布恰好拼成一座廢棄堡壘的入口；從另一側，那些結構暴露為狹薄的細片，是某次爭吵後被推離、從未完全回到原位的角度。從任何一個位置看，椅子的背面都是另一個人的正面。

這種現象蔓延到桌上其他物件。那束花已經開始凋謝：朝送花的人，花展開被剪下的瞬間，還沒離開花園時的重量；朝收花的人，花展開第一眼見到它時心裡某個難以命名的移動。兩個形狀疊在一起，像兩張曝光不足的底片，彼此都干擾了對方的輪廓。蛋糕少了一角，那個缺口朝不同方向展開不同的切割方式——有人記得自己切的，有人記得被切給自己，兩種記憶讓缺口同時向兩個方向傾斜，使那個缺口再也沒辦法只是一個缺口。

巨眼把每段記憶都具象化成幾何事實，讓所有私人正面同時存在。隨著視線繼續掃過桌面，共享的物件逐漸失去它們的公共版本：沒有任何一個角度能讓椅子只是一張椅子，沒有任何一個位置能讓花束只是一束花。人們仍然圍坐在同一張桌子旁，但他們所面對的，已經是完全不同的餐桌。

== 第三幕：盲點

#img("assets/blindspot_zone.png", [盲點區：只有不被觀看的東西，仍然保持完整。])

桌下有一封從未被拆開的信，信封上的字跡仍清晰，要看見它只需要把視線從桌面移開，而這件事從未發生過。幾毫米外，有人無意間掉落的照片，正面朝下貼著地板，沒有人知道那張臉屬於誰，連照片本身似乎也已遺忘，它只是一張照片，習慣了任何朝向。

在最靠牆的那根桌腳邊，有一株從地板裂縫長出的植物，已長到第七片葉子，在這裡生長了三年，從沒有人坐上這張椅子之前就開始了，在晚餐之前，在那封信被放到盤子下方之前，在有人愛過這張椅子又停止愛它之前。它緩慢但持續地往有光的方向延伸，對桌上發生的任何事都無動於衷。在桌腳與地板接合的角落，一隻蜘蛛已完成一張六邊形的網，精確計算了每一根絲的張力，對任何視線的存在毫無感知。

巨眼的視線從來到達不了這裡。椅子仍是椅子，信仍是信，植物只在乎光的方向。時間以正常的速度流動，沒有切片展開，沒有記憶在物件身上聚集，沒有任何計算決定它們下一秒的位置。

整個場景裡，只有這一角落的東西仍以現在式存在。植物明天還會再長出一片葉子，蜘蛛明天還會修補那張網，信還是不會被拆開。這些事情會繼續發生，與任何人的觀看完全無關。

#pagebreak()

== 技術補景

#grid(
  columns: (1fr, 1fr),
  column-gutter: 10pt,
  row-gutter: 10pt,
  image-card("assets/robonaut_close.png", [Robonaut 2], [NASA 外部 OBJ，經材質重評級與程序動畫處理。]),
  image-card("assets/ingenuity_orbit.png", [Ingenuity Orbit], [無人機以 sin / cos 繞 Prediction Core 公轉。]),

  image-card("assets/shadow_floor.png", [Floor Shadow], [地板上的 planar shadow 讓桌椅與機器角色落地。]),
  image-card(
    "assets/texture_closeup.png",
    [Texture Close-up],
    [桌面俯視：木桌 (wood)、桌布 (cloth)、盤面 (marble)、紙件 (paper) 四張程序貼圖同框。],
  ),
)

= Checklist

#wide-table(
  [作業要求],
  [實作證據],
  [截圖],
  [Texture Mapping ≥ 4],
  [自製 `cloth.png`、`wood.png`、`metal.png`、`circuit.png`、`marble.png`、`paper.png` 六張程序貼圖，分別用於桌布、木桌、金屬、虹膜電路、盤面與紙件。],
  [`texture_closeup.png`, `prediction_core.png`],
  [AI 物件使用貼圖],
  [`Prediction Core` 的 iris circuit ring 使用 `assets/textures/circuit.png`，把 AI 運算感直接放在主體物件上。],
  [`prediction_core.png`],
  [外部 OBJ 模型],
  [載入 NASA Robonaut 2 與 NASA Ingenuity Mars Helicopter OBJ，兩者都不是課堂範例模型。],
  [`robonaut_close.png`, `ingenuity_orbit.png`],
  [OBJ 必須有動畫],
  [Robonaut 有巡邏、追視與 dance pivots；Ingenuity 有繞眼球公轉與傾斜擺動。],
  [`robonaut_close.png`, `ingenuity_orbit.png`],
  [Self Rotation],
  [`scene_animation.cpp` 的 `self_rotation` 讓 `circuit_ring` 以 `t * 42°` 自轉。],
  [`prediction_core.png`],
  [Orbit Motion],
  [`orbit_drone` 使用 `cos(a)` / `sin(a)` 計算 Ingenuity 環繞 Prediction Core 的位置。],
  [`ingenuity_orbit.png`],
  [Custom Animation ≥ 2],
  [`gaze_future` 展開酒杯未來切片；`gaze_memory` 展開椅子記憶形狀；另有 Robonaut procedural dance。],
  [`foresight_zone.png`, `longing_zone.png`],
  [Light ≥ 2],
  [Renderer 設定 `GL_LIGHT0` key、`GL_LIGHT1` fill、`GL_LIGHT2` rim，形成主光、補光與輪廓光。],
  [`hero.png`],
  [Shadow 可見],
  [`DrawPlanarShadowOnGround` 與 `DrawPlanarShadowOnTable` 分別投影到地板與桌面，並用 stencil 裁切。],
  [`shadow_floor.png`],
  [AI Future World 主題],
  [巨眼預測核心、Robonaut 服務機器人、Ingenuity 觀測 drone、gaze-debug HUD 與三種視線變形共同構成 AI 未來世界。],
  [`hero.png`, `gaze_debug.png`],
)

= 技術挑戰

本專案的所有 rendering 都在 OpenGL 固定管線下完成，意即沒有 vertex shader、沒有 geometry shader、沒有 framebuffer object、沒有 GLSL。這個限制從根本上封死了許多現代做法，每一個視覺效果都必須在 CPU 端用數學或場景圖結構來替代 GPU 的能力。以下五個挑戰依困難程度排列；其中 P4(三視線幾何效果)是整個作品最核心的技術賭注，在本節佔最多篇幅。

== 1. OBJ 材質重評級

=== 問題背景

NASA Robonaut 2 與 Ingenuity 的 OBJ 模型附帶數十種 MTL 材質定義，這些 MTL 來自離線或 PBR 工作流程：`Ns`(shininess)的值域以 0–1 歸一化設計，反射強度用貼圖 mask 疊加；但 OpenGL 固定管線的 Phong 模型要求不同的語義——`GL_SHININESS` 是 0–128 的浮點數，`GL_SPECULAR` 期望直接代表高光亮度，不存在 metallic 或 roughness 通道。若直接把 MTL 的數值對應進去，Robonaut 的白色裝甲在場景中呈深灰，金屬關節完全無反光，整個人形角色消失在桌邊光線裡。

=== 嘗試過的方法

第一個方案是全域提高 ambient light 強度：場景變亮了，但所有物件的立體感一起消失，模型看起來像被平光打亮的紙片。第二個方案是在 MTL 載入後把所有 `Ka`、`Kd`、`Ks` 乘上一個常數係數，結果是金屬與塑膠材質都一起變得過飽和，失去材質差異。這兩條路都證明「全域補償」無法重現 PBR 材質的分層語意。

=== 最終解法

最終採用的做法是依 material name substring 進行分類，將同一個 OBJ 的材質族群映射到不同的 Phong 參數組合。每個 material name 在載入時被比對幾個關鍵詞，分配到對應的材質語義桶：

#codeblock[
  ```text
  分類規則(substring match，依序優先)：
  含 "visor" / "lens" / "glass" → 高 specular (0.9), shininess 96, 低 diffuse
  含 "joint" / "metal" / "arm"  → 中 specular (0.6), shininess 48, diffuse 0.5
  含 "armor" / "white" / "body" → 低 specular (0.3), shininess 24, diffuse 0.85
  含 "cable" / "wire" / "dark"  → 零 specular, shininess 8,  diffuse 0.3
  fallback                       → specular 0.4, shininess 32, diffuse 0.65
  ```
]

同時，rim light(`GL_LIGHT2`)刻意設定較高的 specular contribution，讓 Robonaut 的輪廓在深色背景中維持分離感。這不是單純「調亮」，而是把 PBR 設計意圖重新以固定管線語言表達。

=== 成效

#techbox[結果][
  Robonaut 在三種光源下呈現清晰的材質層次：白色裝甲柔和漫反射、金屬關節有可辨識的高光點、玻璃頭罩在 rim light 下留有一條細亮邊。材質識別度在三公尺場景深度之外仍能維持。
]

== 2. Planar Shadow Projection 數學推導

=== 問題背景與技術選型

Shadow map 在固定管線下技術上可行：先把場景從光源視角渲染成深度貼圖(只需 `glGenTextures` + `glCopyTexImage2D`)，再在第二 pass 用投影矩陣把光源視角座標重投影到螢幕空間，逐片元做深度比對。但這條路在沒有 fragment shader 的情況下極度彆扭——固定管線的 texture combiners 表達能力有限，深度比對只能靠 `GL_TEXTURE_COMPARE_MODE` 搭配特定硬體擴充，跨平台行為不一致，且最終效果品質仍受 texture projector 精度侷限，實作與除錯成本遠超收益。

場景的陰影接收面都是平整幾何：地板 ($y = 0$)與桌面 ($y = h_"table"$)。Planar shadow projection 正是針對「接收面為已知平面」這個條件量身設計，它不需要深度貼圖，只需要一個 $4 times 4$ 矩陣，把所有 caster 幾何投影壓平到目標平面上再重畫。計算量極小，邏輯完全透明，在固定管線下反而是最乾淨的方案。

=== 從幾何出發的推導

以齊次座標表示：平面 $bold(p) = (a, b, c, d)$ 滿足平面方程 $bold(p) dot bold(v) = 0$，光源 $bold(l) = (l_x, l_y, l_z, l_w)$ ($l_w = 1$ 點光，$l_w = 0$ 方向光)。對 caster 上任意頂點 $bold(v)$，從光源出發穿過該頂點的射線為：

$ bold(r)(t) = bold(l) + t (bold(v) - bold(l)) $

代入平面方程 $bold(p) dot bold(r)(t) = 0$ 求解 $t$：

$ bold(p) dot bold(l) + t (bold(p) dot bold(v) - bold(p) dot bold(l)) = 0 $
$ t = frac(bold(p) dot bold(l), bold(p) dot bold(l) - bold(p) dot bold(v)) $

代回射線得到投影頂點，整理後各分量為：

$ r_i = frac((bold(p) dot bold(l)) v_i - (bold(p) dot bold(v)) l_i, bold(p) dot bold(l) - bold(p) dot bold(v)) $

分子即為線性映射 $S bold(v)$，令 $"dot" = bold(p) dot bold(l)$，矩陣元素為：

$ S_(i,j) = "dot" · delta_(i,j) - l_i p_j $

其中 $delta_(i,j)$ 是 Kronecker delta。分母 $bold(p) dot bold(l) - bold(p) dot bold(v)$ 對應齊次座標的 $w$ 分量，GPU 在光柵化時自動做透視除法，因此直接把 $S bold(v)$ 送進去即可。對應 `Renderer::MakeShadowMatrix` 的核心：

#codeblock[
  ```cpp
  matrix[col * 4 + row] = ((row == col) ? dot : 0.0f) - light[row] * plane[col];
  ```
]

=== Render Pass 與 Stencil

整幀渲染順序為：先正常畫完場景 (`scene_root_->Draw()`)，再呼叫 shadow pass，避免半透明陰影蓋在物件本體上。每個 shadow pass 內部分兩段：

#codeblock[
  ```text
  -- Pass A: mark receiver region into stencil buffer --
  glClear(STENCIL_BUFFER_BIT)            -- reset stencil to 0
  glStencilFunc(ALWAYS, ref=1, 0xFF)
  glStencilOp(REPLACE, REPLACE, REPLACE) -- write 1 on any fragment
  glColorMask(FALSE, ...)                -- suppress color output
  glDepthMask(FALSE)
  DrawReceiverBox(center, scale)         -- mark stencil=1 where surface is

  -- Pass B: draw projected shadow, clipped to stencil == 1 --
  glColorMask(TRUE, ...)
  glStencilFunc(EQUAL, ref=1, 0xFF)      -- pass only where stencil == 1
  glStencilOp(KEEP, KEEP, INCR)          -- increment on write (no double-darken)
  glDisable(LIGHTING | TEXTURE_2D | CULL_FACE)
  glEnable(BLEND)
  glBlendFunc(SRC_ALPHA, ONE_MINUS_SRC_ALPHA)
  glDepthMask(FALSE)
  glColor4f(0, 0, 0, shadow_alpha)
  glMultMatrixf(shadow_matrix S)
  DrawShadow(caster_root, min_y, max_y)  -- skip casters outside y range
  ```
]

Pass A 每次呼叫都重新 clear 並標記——stencil 不是全域預計算的，而是每個 shadow pass 獨立維護。Pass B 的 `INCR` 防止同一像素被重疊的投影三角形雙重壓暗：第一個三角形把 stencil 從 1 累加到 2，第二個三角形在 `EQUAL 1` 的條件下失敗，不再疊加黑色。桌面與地板各呼叫一次 `DrawShadowPass`，以不同的 `receiver_center/scale` 和 `min/max_caster_y` 分別裁切各自的接收範圍。

=== 成效

#techbox[結果][
  地板接收 Robonaut 與桌椅的陰影；桌面接收桌上物件(酒杯、花束、蛋糕)的投影，且陰影不外洩到桌緣之外。兩套 planar shadow 在同一幀內以正確的前後順序呈現，視覺上讓所有物件確實「落地」。
]

== 3. Gaze Zone 指數平滑混合

=== 問題背景

三種視線效果(預見、眷戀、盲點)以 120° yaw sector 分配：

$ "sector" = floor\( (("yaw" + 60°) mod 360°) / 120° \) mod 3 $

最直觀的做法是把 `sector` 直接映射成一個二值開關，進入某個 sector 就啟動對應效果。但這會讓酒杯切片在視線越過邊界時瞬間彈出，椅子記憶面板瞬間收回——整個場景像是 UI 選項切換，而不是視線真的改寫了物件的存在方式。

=== 解法推導：指數衰減混合

對每個 zone $i \in \{0, 1, 2\}$ 維護一個連續權重 $w_i \in [0, 1]$。每幀更新時，令 target 為：

$ "target"_i = cases(1 & "if" i = "sector", 0 & "otherwise") $

以指數衰減步進逼近：

$ alpha = 1 - exp(-Delta t / tau), quad tau = 0.26 "s" $
$ w_i arrow.l w_i + ("target"_i - w_i) · alpha $

選擇 $tau = 0.26 "s"$ 的依據是對「醒來」速度的主觀調校：$3 tau approx 0.78 "s"$ 後權重到達 95% 目標值，這個速度對 30 fps 的幀率夠順滑、對人眼感知不會顯得遲鈍。若 $tau$ 太小(< 0.1 s)效果接近硬切；太大(> 0.6 s)視線轉動後系統才慢慢跟上，互動感消失。

最後加一個 snap 閾值，避免 $w_i$ 在接近 0 或 1 時留下浮點殘差持續計算：

#codeblock[
  ```text
  sector ← floor( normalize(yaw + 60°) / 120° ) mod 3
  for each zone i ∈ {0, 1, 2}:
    target ← (i == sector) ? 1.0 : 0.0
    w[i]   ← w[i] + (target - w[i]) * α
    if |w[i] - target| < 0.002:
      w[i] ← target          // 消除浮點殘差
  ```
]

=== 三向混合的語意

值得注意的是，在 sector 邊界附近，$w_0 + w_1 + w_2$ 短暫不等於 1(例如從 foresight 轉往 longing 時，foresight 還沒完全衰減到 0)。這個「重疊期」是刻意保留的：酒杯切片淡出同時椅子記憶片段淡入，場景在短暫的一兩秒裡同時展示兩種視線的殘像，讓觀眾感受到視線正在「重新詮釋」同一個桌面，而不是切換頻道。

#pagebreak()

== 4. 固定管線下的三視線幾何效果

這是整個作品最核心的技術挑戰。預見、眷戀、盲點三種效果要求同一個場景中，同一組物件依視線權重呈現截然不同的幾何外觀——而且這個「外觀」必須與 stencil shadow、材質、場景圖生命週期完整整合。

=== 問題背景：GPU 端無法在 submit 後修改頂點

固定管線沒有 vertex shader，意即 CPU 把頂點送進 GPU 之後，每個頂點的世界座標就固定了——GPU 端不能根據視線方向或任何 uniform 變數在繪製期間重新計算位置。若要讓酒杯「切片展開」或椅子「記憶面板浮現」，所有幾何位置的決策都必須在 CPU 端、draw call 之前完成。

另一個可能的做法是每幀用 `glBegin/glEnd` 即時建構動態頂點；但這繞過了整個 scene graph 的 TfHandle 管理機制，無法與 shadow pass 的 caster filter(`min/max_caster_y`)、stencil receiver 標記等功能整合，維護成本極高，且動態建構的 mesh 難以複用材質與貼圖狀態。

=== 解法：預分配 Fragment 節點 + CPU Scene Graph

核心策略是「預先存在但預設不可見」：程式在場景初始化時就建立所有可能出現的幾何節點，將它們藏在原始物件的 rest 位置(縮到零尺寸或移出視野)，再在每幀 Tick 中根據 zone 權重調整各節點的 transform。

每個節點透過 `TfHandle`(scene graph 的 transform handle)在 CPU 端計算一個 $4 times 4$ 矩陣，`glLoadMatrixf` 送進去。這樣一來，GPU 看到的頂點數量不變，只是每個節點的 model transform 在每幀被重新計算。

=== gaze_future：預見切片的展開

所有 future 節點在 `SceneAnimation::Bind` 時依名稱前綴收集，並按名稱中的 substring 分為五種 kind：Slice(切片本體)、Trajectory(倒落軌跡線)、Stain(桌面水漬)、Shard(地板碎片)、BranchProp(環境分支道具)。各節點的 rest transform 是場景初始化時的位置；當 `w_foresight = 0` 時，各 kind 保留一個不為零的最小 scale，使幾何確實存在於場景中但肉眼難以察覺，而不是縮到零。

slot 交錯靠一個純 weight 縮放公式實現：

$ "lead"_k = "Clamp01"(w dot (1.16 - k dot 0.055)) $

slot $k$ 越大，係數 $(1.16 - k dot 0.055)$ 越小，需要更高的 $w$ 才能讓 $"lead"_k$ 達到 1.0——這就製造了高 slot 編號的節點「更慢展開」的效果，不需要額外的時間延遲計時器。

Slice 的每幀 transform 計算如下($"tremor" = 0.94 + 0.06 sin(3t + k)$)：

#codeblock[
  ```text
  position.y  += 0.065 * k * lead               -- rise with slot index
  scale.xz     = rest.scale.xz * (0.06 + 0.94 * lead)   -- thin at rest
  scale.y      = rest.scale.y  * (0.20 + 1.40 * lead) * tremor  -- stretch beyond rest
  euler.y     += t * 20 + k * 18 * lead         -- slow Y-spin per slice
  ```
]

$Y$ 方向的 scale 係數最終可達 $1.40$，高於 rest 的 1.4 倍，製造切片被「拉長」的感覺而不只是「縮放回原尺寸」。Stain 在 $X Z$ 平面展開($Y$ 不動)；Shard 在 $X Z$ 小幅散開同時整體縮放；BranchProp 從以 anchor 為基準的 tucked 位置 lerp 回 rest 位置，並附帶三軸正弦擺盪。

=== gaze_memory：眷戀形狀的展開

memory 節點同樣在 Bind 時依名稱 substring 分為五種 kind：Panel、Figure、Fort、Argument、Relic。每種 kind 各有獨立的動畫邏輯，但都共享同一個呼吸因子：

$ "breathe"(t) = (0.96 + 0.04 sin(1.8 t + "target" + "slot")) dot w $

所有 kind 的呼吸頻率相同($1.8$)，相位用 `target + slot` 錯開，讓同一個 scene 裡不同節點的振盪不同步，看起來像各自獨立的脈動而不是齊步擺動。

各 kind 的展開方式如下：

#codeblock[
  ```text
  Panel:    scale.y  *= breathe
            scale.z   = rest.z * (0.25 + 0.75 * w)   -- unfurl in depth
            pos.y    += 0.20 * w                       -- lift
            euler.y  += 10 * sin(0.55t + target) * w  -- gentle sway

  Figure:   pos  = lerp(tucked=(0,0.58,0.05), rest.pos, w)
            scale *= breathe

  Fort:     pos  = lerp(tucked=(0,0.56,0.04), rest.pos, w)
            scale *= breathe
            euler.z += 3 * sin(1.2t + slot) * w       -- tilt wobble

  Argument: pos  = lerp(tucked=(0,0.56,0.05), rest.pos, w)
            scale *= breathe
            euler.y += 9 * sin(1.1t + slot) * w       -- spin wobble

  Relic:    open  = Clamp01(w * (1.10 - slot * 0.055))  -- slot-staggered
            pulse = 0.985 + 0.015 * sin(1.3t + slot)
            pos   = lerp(anchor + (0,-0.22,-0.18), rest.pos, open)
            scale = rest.scale * (0.10 + 0.90*open) * pulse
  ```
]

Figure、Fort、Argument 都從各自固定的 tucked 點向 rest position lerp；Relic 則從以 `kMemoryAnchors[target]` 為基準的偏移點展開，並帶有與 Slice 相同的 slot 錯開機制。

=== 成效

#techbox[整體架構成效][
  觀眾看到的不是 UI 選單切換，而是同一個晚餐桌被三種觀看方式改寫。預見、眷戀、盲點不再只是敘事概念，而是可以逐幀量測、可以還原成位移插值的技術事實。場景沒有告訴你任何事：它讓你親手轉動視線，看世界如何因為被看而改變形狀。全部效果以固定管線實現，GPU 側不需要任何非標準擴充。
]

#pagebreak()

== 5. 自製 TF Tree 與 Robonaut 程序動畫

=== 問題背景：沒有 rig 就得自己建樹

NASA Robonaut 2 的 OBJ 只有幾何資料，沒有骨架、蒙皮權重或關鍵幀動畫。要讓它的身體沿橢圓巡邏、頭部即時追視 Ingenuity、各關節隨音樂律動，需要一套能在 CPU 端管理節點父子關係、並把 world transform 正確傳遞到每個關節的系統。現成的 assimp / Bullet 不進 runtime，因此從零手搓了 `TfTree`。

=== TfTree：Generational Handle + Dirty-Bit Cache

`TfTree` 以 flat `std::vector<Node>` 儲存所有節點，避免指標式樹的快取不友善問題。對外暴露的 `TfHandle` 是一個 generational index：

#codeblock[
  ```cpp
  struct TfHandle {
      uint32_t index;       // slot in nodes[]
      uint32_t generation;  // increments when slot is reused
  };
  ```
]

當節點被銷毀時，slot 回收進 `free_list_` 並遞增 `generation`；舊 handle 再被使用時 generation 不匹配，`NodeFor()` 返回 `nullptr`，完全杜絕懸空 handle 的 silent misbehavior，不需要引用計數。

World matrix 採 lazy evaluation：每次 `SetLocal()` 把自己與所有後代標記 `world_dirty = true`；`WorldMatrix()` 被呼叫時才從根遞迴計算並快取：

$ M_"world" = M_"parent world" times M_"local" $

Local matrix 的分解順序為 $T dot R_Y dot R_X dot R_Z dot S$（對應 `LocalMatrixFromTransform` 的串接），最終以 `glLoadMatrixf(world_matrix.data())` 送進固定管線。

=== Ingenuity Orbit Motion

Ingenuity 的位置以 Lissajous-like 軌道計算（$r = 3.0$，$omega = 0.55 "rad/s"$）：

$ bold("pos")(t) = bold("eye") + (3 cos(omega t),; 0.40 sin(2 omega t),; 3 sin(omega t)) $

$Y$ 分量用 $2 omega$ 製造上下擺盪，視覺上像直升機維持懸停。機身三軸旋轉依軌道相位計算：

#codeblock[
  ```text
  euler.y = -a * (180/pi) + 90   -- face forward along orbit
  euler.z = 14 * cos(2a)         -- roll bank on curve
  euler.x =  6 * sin(2a)         -- pitch nod on bob
  ```
]

位置最終透過 `ResolveSensorPosition` 過物理碰撞修正，確保 Ingenuity 不穿入 eye safety zone。

=== Robonaut 巡邏：Spring-Damper on Ellipse

巡邏目標點沿橢圓以 $omega_p = 0.08 "rad/s"$ 移動（$R_x = 3.05$，$R_z = 1.95$，$C_z = 0.62$）：

$ x_t = R_x cos(omega_p t), quad z_t = C_z + R_z sin(omega_p t) $

身體質心以 spring-damper 追蹤目標點（$k = 2.0$，$c = 2.0$，$v_"max" = 1.5$）：

$ a_x = k (x_t - x) - c v_x, quad v_x arrow.l v_x + a_x Delta t $

阻尼比 $zeta = c slash (2 sqrt(k)) = 2 slash (2 sqrt(2)) approx 0.71$，欠阻尼，Robonaut 在抵達目標前有輕微超衝，看起來像有慣性的機械角色在煞車。速度超過 $v_"max"$ 時方向保持、量值截斷。最後透過 `MoveKinematic` 做物理碰撞掃描防止穿模。

=== 頭部追視：SmoothDampAngleDeg

`robonaut_track` 每幀計算身體到 Ingenuity 的 yaw 目標角：

$ "yaw"_"target" = "atan2"("to"_x, "to"_z) times (180 slash pi) + 180° $

角度平滑用自製的 `SmoothDampAngleDeg`（smooth time = 0.7 s）：先把角度差規範化到 $(-180°, +180°]$ 取最短弧，再呼叫 `SmoothDamp`。`SmoothDamp` 是臨界阻尼跟隨器的多項式近似（$omega = 2 slash tau$）：

$ "exp"_"approx"(x) approx 1 slash (1 + x + 0.48 x^2 + 0.235 x^3), quad x = omega Delta t $

頭部的 yaw 完全由 `robonaut_track` 負責；pitch 由 `robonaut_procedural_dance` 的 head joint 單獨驅動（$10° sin(2.65 t + 1.5)$），兩個 animator channel 各管一個自由度，互不干擾。

=== Procedural Dance

`robonaut_procedural_dance` 以 $"beat" = 2.65 t$ 驅動全身律動。身體根節點做三維小幅位移與傾斜：

#codeblock[
  ```text
  bounce   = 0.5 + 0.5 * sin(2 * beat)
  pos.y   += 0.030 * bounce              -- vertical hop
  pos.x   += 0.010 * sin(0.4 * beat)    -- lateral sway
  pos.z   += 0.015 * sin(beat + 0.65)   -- fore-aft shift
  euler.x += 2.0  * sin(beat + 0.6)     -- nod
  euler.z += 3.0  * sin(0.5 * beat)     -- lean
  ```
]

各 dance joint（center、head、雙臂肩肘腕共 10 個）再各自疊加角度偏移，center joint 還有 $+Y$ 位移讓上半身隨節拍起伏。Dance 是常駐 animator channel，與 gaze 效果並行運行，讓 Robonaut 無論處於哪個 gaze zone 都保有持續的機械生命感。

= 素材來源

本專案所有外部素材均為 public domain 或 CC0，可自由使用與修改，無需署名。OBJ 模型皆由 `scripts/fetch_assets.py` 下載並以 `scripts/convert_glb_to_obj.py` 轉換為正規化 OBJ 格式進入 runtime。

== NASA 模型（Public Domain）

NASA 媒體依 NASA 使用政策為 public domain，可用於任何目的。

#table(
  columns: (1.2fr, 1.4fr, 2fr),
  inset: 6pt,
  stroke: 0.35pt + rgb("#C8C1BA"),
  [模型], [原始格式], [場景角色],
  [Robonaut 2], [GLB → OBJ], [主要 AI 角色，人形服務機器人，擺放於晚餐桌旁。],
  [Ingenuity Mars Helicopter], [GLB → OBJ], [Orbit 主角，自主旋翼無人機，繞 Prediction Core 公轉。],
  [Infrared Camera], [GLB → OBJ], [預見區，桌面預測投影器 / 感測鏡頭。],
  [Astronaut Glove], [GLB → OBJ], [眷戀區，空椅旁的人類殘痕，暗示曾經在場的人。],
  [Crew Lock Bag], [GLB → OBJ], [盲點區，桌下被忽略的收納袋，保持完整不被特效影響。],
)

來源：NASA / NASA-3D-Resources GitHub repository。

== Poly Pizza 模型（CC0）

以下模型來自 Poly Pizza 平台，均為 CC0 授權。

#table(
  columns: (1.4fr, 1.1fr, 2fr),
  inset: 6pt,
  stroke: 0.35pt + rgb("#C8C1BA"),
  [模型], [作者], [場景角色],
  [Dirty Plate], [Kay Lousberg], [預見區，事故鏈中的滑動髒盤。],
  [Bowl Dirty], [Kay Lousberg], [預見區，被撞歪的髒碗，酒杯事故的第二分支。],
  [Stew], [Kay Lousberg], [預見區，湯即將灑出的具體後果。],
  [Cutting Board], [Kay Lousberg], [預見區，被推移的砧板。],
  [Chopsticks], [Quaternius], [預見區，被撞散的餐具。],
  [Present], [CreativeTrio], [眷戀區，記憶祭壇的禮物盒。],
  [Wall Corkboard], [CreativeTrio], [眷戀區，私人記憶的照片牆介面。],
  [Mug With Office Tool], [CreativeTrio], [眷戀區，椅旁日常細節。],
  [Closed Umbrella], [CreativeTrio], [眷戀區，爭吵後留下的離席痕跡。],
  [Zz Plant], [Isa Lousberg], [盲點區，桌下盆栽，完整但不被 AI 解釋。],
  [Cyberpunk Platform], [Poly Pizza], [預見背景，科幻預測實驗室平台。],
  [Sci Fi Floor Tile], [Poly Pizza], [預見背景，Prediction Core 計算區地磚。],
  [Sci Fi Wall 3], [Poly Pizza], [預見背景，科幻牆片。],
  [Curtains Double], [Poly Pizza], [眷戀背景，私人記憶角落窗簾。],
  [Lamp With Shade], [Poly Pizza], [眷戀背景，暖色懷舊立燈。],
  [Candles], [Poly Pizza], [眷戀背景，柔光蠟燭群。],
)

== 自製程序貼圖（CC0）

全部 6 張貼圖由 `scripts/gen_textures.py` 以程序算法生成，512×512 PNG，CC0 釋出。

#table(
  columns: (0.8fr, 1.2fr, 2fr),
  inset: 6pt,
  stroke: 0.35pt + rgb("#C8C1BA"),
  [檔案], [用途], [技術],
  [`wood.png`], [桌面、桌腳], [正弦波木紋條紋],
  [`cloth.png`], [椅座、椅背], [週期性編織格紋],
  [`circuit.png`], [巨眼虹膜電路環（AI 物件必要項）], [PCB 走線加焊盤隨機分佈],
  [`marble.png`], [鞏膜球、瓷盤], [正弦波大理石紋],
  [`metal.png`], [虹膜框、縫隙環、線纜], [水平磨砂拉絲],
  [`paper.png`], [桌上信封], [白紙纖維噪點],
)

== 報告裝飾素材

`decor/eye_icon.svg`：Wikimedia Commons，File:Eye Icon.svg，作者 Public Domain Nouns，CC0。

#v(12pt)
#align(center)[
  #pill[OpenGL fixed pipeline]
  #h(6pt)
  #pill[Prediction Core]
  #h(6pt)
  #pill[Robonaut 2]
  #h(6pt)
  #pill[Ingenuity Orbit]
]
