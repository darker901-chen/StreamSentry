# SETUP — 從零到開工(你只跑指令,環境交給 Claude Code)

## 你實際要做的事,只有這些
1. 解壓縮這個 zip 到 `C:\dev\<PLUGIN_NAME>\`(名字定案後資料夾一起改名)。
2. 裝一個東西:**App Installer**(給你 winget 指令)。多數 Win11 已內建;沒有的話開
   Microsoft Store 搜 "App Installer" 按安裝。這是唯一需要你手動裝的前置。
3. 在專案資料夾開「**系統管理員** PowerShell」,跑 `claude`。
4. 把下面 §3 的開工 prompt 整段貼進去。之後就是回指令、看它做。

環境(CMake / Git / VS Build Tools / OBS)**由 Claude Code 跑腳本自動裝**,不用你一個個下載。

## 解壓後結構
```
<PLUGIN_NAME>/
├── CLAUDE.md              ← 憲法:Claude Code 每次自動讀
├── SPEC.md                ← v0.1 技術規格
├── SETUP.md               ← 本檔
├── scripts/
│   └── bootstrap.ps1      ← 環境自動安裝腳本(Claude Code 會執行)
├── reports/               ← agent 交接區(file-based handoff)
└── .claude/agents/
    ├── verifier.md            ← build + 自動測試,產證據報告
    ├── spec-guardian.md       ← 對憲法審 diff,PASS/FAIL
    └── scribe.md              ← 已驗證證據寫進 TESTING/CHANGELOG
```

## 自動化到哪、哪裡非你不可(誠實邊界)
- **Claude Code 自動做**:跑 bootstrap.ps1 裝 CMake/Git/VS Build Tools/OBS、
  初始化專案、build、跑單元測試、規格審查、寫報告。
- **只有兩點非人不可**(腳本會在該停的地方印出 [MANUAL] 提醒):
  1. 裝 VS Build Tools 時若彈出微軟的**授權同意 GUI**,你點一下同意(微軟強制,無法靜默)。
  2. **OBS 裡肉眼確認 filter 有沒有出現**——這是 M0 的驗收,GUI 的事只能人看。
- 全自動涵蓋 build、座標轉換單元測試(pure module)、規格合規;OBS 內實際行為
  (toast 真被蓋、fail-closed 真切黑)每個里程碑仍需你手動 10 分鐘,scribe 會在
  TESTING.md 幫你維護「這次該看什麼」。

## 多角色運作(對應 Release Guardian 的 hub-and-spoke)
- **主 session = hub**:計畫、實作、每個里程碑派工。
- **verifier / spec-guardian / scribe = spokes**:互不通訊,只透過 `reports/` 檔案交接。
- 每個里程碑循環:`實作 → verifier(VERIFIED?) → spec-guardian(PASS?) → scribe 記錄 → 你親眼看`。

## §3 開工 prompt(系統管理員 PowerShell 裡跑 claude,貼整段)
```
先讀 CLAUDE.md 和 SPEC.md,讀完用三句話跟我確認你理解的鐵律。

接著執行 Week 0 里程碑 M0,分兩階段:

[階段 A — 環境]
1. 執行 scripts\bootstrap.ps1(powershell -ExecutionPolicy Bypass -File
   scripts\bootstrap.ps1)自動安裝 CMake / Git / VS Build Tools(含 C++
   workload)/ OBS Studio。
2. 若腳本印出 [MANUAL],照它說的處理(例如點微軟授權同意、或關掉重開一個
   新的 PowerShell 讓環境變數生效),然後回報我目前狀態。
3. 逐一驗證 git、cmake、VS C++ toolchain 可用,把版本貼給我。環境沒全通
   之前不要進階段 B。

[階段 B — 骨架]
4. 以 https://github.com/obsproject/obs-plugintemplate 為基礎初始化本專案
   (git init;plugin id / CMake project / 顯示名稱都用本資料夾名)。
5. 在 Windows 完成建置,產出可載入的 .dll。
6. 實作最小 video filter:註冊進 filter 清單、properties 只有一個 enable
   checkbox、render 原樣 pass-through。
7. 告訴我 build 產物要複製到 OBS 哪個路徑、OBS log 看哪一行確認載入成功。
8. 用 template 內建 CI 設好 GitHub Actions:push tag → 自動編譯 → Release zip。

流程規則:
- 每階段動手前先列計畫等我確認。
- 完成一個里程碑後,先派 verifier 驗證、再派 spec-guardian 審查,兩者都過
  才找 scribe 記錄,然後停下來等我手動檢查。報告一律寫進 reports/。
- M0 不寫任何偵測或遮蔽邏輯。M0 完成定義:我在 OBS 的 filter 清單親眼看到
  這個 plugin。
先給我階段 A 的計畫。
```

## M0 之後
骨架在 OBS 裡亮起來 → 回 Claude.ai 找我拿 M1(render path:假 rect 驅動 plate +
fail-closed 路徑)的 prompt。別讓 Claude Code 自行往下衝功能(CLAUDE.md 第 5 條會擋)。

## 常見狀況
- **bootstrap 報找不到 winget**:裝 App Installer(見上面第 2 點)後重跑。
- **裝完 VS 後 cmake 找不到編譯器**:關掉 PowerShell 開新的,環境變數才更新;
  這點腳本也會提醒。
- **SmartScreen 警告自己 build 的 dll**:本機開發正常,發布時 README 處理。
- **Claude Code 想加規格外的東西**:指 CLAUDE.md 第 5 條拒絕,或直接跑 spec-guardian。
