# Issue #160 游戏适配资料调查

调查日期：2026-11-16。本文是适配前期资料，不包含游戏 ROM、提取的游戏文本或未经核验的 ROM 哈希。

## 结论摘要

| 系列/游戏 | 目标平台与已知版本范围 | 资料与适配判断 |
|---|---|---|
| 火焰纹章 | GBA 三作：封印之剑（日本版）、烈火之剑（日本《烈火之剑》/海外《Blazing Blade》）、圣魔之光石（日本/北美/欧洲/澳洲） | 该系列文本多、版本差异显著。先逐 ROM 确认消息表/对话状态、区域和修订，再决定能否共用翻译索引；每个正式 ROM revision 应独立记录 CRC/SHA-256。 |
| 黄金太阳 | 《Golden Sun: The Lost Age》之前作《Golden Sun》；两作均有日本、北美、欧洲等发行区域（需以各发行版 ROM 清单进一步核实修订） | 两作应视为两个独立适配目标。对话量大，需先验证 ROM 内文本编码/压缩方式、消息 ID 和分页状态。 |
| 逆转裁判 | GBA 原作《逆転裁判》及《逆転裁判2》《逆転裁判3》主要为日本 GBA 发行；《Ace Attorney》海外首发属于后续 DS 移植，不是 GBA 海外版 | 重点是日文 GBA 原版。DS 版不能直接套用 GBA ROM adapter；若目标是中文体验，应先明确文本来源和授权数据来源。 |
| 宝可梦 | GBA 第三世代：Ruby、Sapphire、Emerald、FireRed、LeafGreen；Ruby/Sapphire 与 FireRed/LeafGreen 各有双版本，另有 Emerald 单版本 | 至少五个主系列发行目标，存在区域版本与语言差异。各游戏文本脚本、指针和运行时状态机不同，不应以单一“Pokemon adapter”概括。先选定具体游戏/语言并采集 ROM revision。 |
| Stardew Valley | 现代游戏；发现 RG34XXSP + 64-bit KNULLI + PortMaster 适配 | 社区仓库 `cdale11/sv-port` 是 Stardew Valley Mainline PortMaster 移植的设备修正版，要求用户拥有完整 Windows 或 Linux Steam 主线游戏文件（.NET 6 布局）；不属于本项目现有 GBA/mGBA 核内 adapter 架构。若以后支持，应设计独立的 Linux/PortMaster 集成路径和文本注入方案。 |
| A Short Hike | 现代游戏 | 截至调查日，GitHub 精确仓库搜索未发现 RG34XX 或 PortMaster 适配项目。当前没有可确认的设备版路径；需另查 ARM/Linux 可执行版本及运行时/渲染兼容性，不能按 GBA adapter 路线处理。 |

## GBA 游戏版本范围和采集要求

下面列出的是需要纳入调查的商业发行目标，不等同于已穷举每一个 ROM dump/revision。发行地区、语言、ROM 修订和汉化补丁必须分别登记；同一标题的不同 ROM 不应未经验证共用地址或 hash。

### 火焰纹章

1. **封印之剑**（ファイアーエムブレム 封印の剣 / *The Binding Blade*）：GBA，日本发行。此次调查没有确认官方海外 GBA 发行版。
2. **烈火之剑**（ファイアーエムブレム 烈火の剣 / *The Blazing Blade*）：GBA，日本版；海外版以 *Fire Emblem* 为标题，至少北美和欧洲发行。不同语言版须独立验证。
3. **圣魔之光石**（聖魔の光石 / *The Sacred Stones*）：GBA，日本、北美、欧洲、澳洲发行。

适配前采集：每个合法取得的 ROM 的规范标题、区域/语言、ROM header 游戏代码、revision、文件长度、CRC32/SHA-256；在 mGBA 中记录消息 ID、当前页和对话结束状态所在 RAM 地址，并用多场景验证。地区文本和脚本差异不可只靠同名游戏推断。

### 黄金太阳

1. **黄金太阳**（黄金の太陽 開かれし封印 / *Golden Sun*）。
2. **黄金太阳：失落的时代**（黄金の太陽 失われし時代 / *Golden Sun: The Lost Age*）。

这两作剧情与 ROM 数据结构分别处理。启动适配前需补齐各地区/语言发行清单与 ROM revision 清单；然后定位文本资源、消息编号、字形编码和分页事件。系列同名不意味着脚本 ID 跨作兼容。

### 逆转裁判

GBA 的《逆転裁判》系列包括：

1. **逆転裁判**（2001）。
2. **逆転裁判2**（2002）。
3. **逆転裁判3**（2004）。

GBA 原版以日本市场为主。英语《Phoenix Wright: Ace Attorney》是后来 Nintendo DS 版的海外发行，不能列作 GBA ROM 的英语地区版。还需单独确认各 GBA 作的消息存储格式和游戏状态访问方法；文字推进、证物/法庭分支会使单纯 `(textId,page)` 映射面临较多分支覆盖工作。

### 宝可梦（GBA 第三世代）

- **Ruby / Sapphire**：双版本；需按地区和语言登记发行版。
- **Emerald**：增强版，独立脚本和 ROM 身份。
- **FireRed / LeafGreen**：双版本重制；文本及脚本与 Ruby 系列不同。

范围说明：此处将 issue 中的“宝可梦”解释为 GBA 游戏，因 issue 要求 GBA 全版本资料。不可从某一作推演另一作的 RAM 地址或文本 ID。还应在后续清单中明确是否纳入各地区、各语言及 revision；日本原版、英语版及其他语言版应分别核 hash。

## RG34XX 现代游戏适配调查

### Stardew Valley

发现项目：[`cdale11/sv-port`](https://github.com/cdale11/sv-port)。项目说明其为 Stardew Valley Mainline PortMaster port 的 RG34XXSP/KNULLI 修正版，当前 README 标示 v1.0.0。

关键限制：

- 目标为 **Anbernic RG34XXSP、64 位 KNULLI、已更新 PortMaster**；不能据此推断 RG34XX（非 SP）或其他固件均可运行。
- 需要用户自有的完整 Windows 或 Linux Steam 主线游戏安装，采用 .NET 6 文件布局；移植包不含商业游戏数据。
- 第一次启动要准备/缓存，初始化可需几十秒；农场复杂度影响帧率。Steam overlay、成就、云存档和 Steam 邀请多人不可用。
- 项目是原生游戏移植/运行方案，与 RetroBilingual 当前 mGBA core hook 不同。双语适配要先确定可维护的文本 API/文件替换或运行时注入点，再讨论端侧 HUD。

### A Short Hike

GitHub 仓库搜索（`RG34XX "A Short Hike"`、`"A Short Hike" PortMaster`）未发现对应项目；PortMaster 目录搜索也未确认该游戏条目。因此当前结论是“未找到公开、可复现的 RG34XX 适配”，而不是证明绝对不存在。下一步需核对 PortMaster 最新目录、上游 Linux/ARM 构建和 RG34XX 图形/API 兼容性。游戏本身不是 GBA ROM，不能使用现有 GBA RAM adapter 直接适配。

## 对本仓库的影响与推荐后续

本次只补充调查记录，不新增 adapter、ROM hash 或翻译数据。仓库当前参考 adapter 为《塞尔达传说：缩小帽》，架构基于 mGBA GBA core 与游戏 RAM 反射。建议实施优先级：

1. 先选定一个具体 GBA 游戏、地区和 ROM revision，建立可重复的合法本地 ROM 测试样本记录（不提交 ROM）。
2. 对该 ROM 做 RAM/文本结构逆向验证，填入精确 hash、游戏代码、地址、有效场景和页状态证据。
3. 以独立 adapter 开发；同系列其他版本只有完成地址与脚本映射验证后才声明兼容。
4. Stardew Valley 作为单独的 PortMaster/现代 Linux 适配研究项目；A Short Hike 先做上游运行时可行性调查。

## 来源

- mGBA 官方站点与发行信息：https://mgba.io/
- 火焰纹章发行背景：
  - https://en.wikipedia.org/wiki/Fire_Emblem_(video_game) （页面现名/内容为 *The Blazing Blade*）
  - https://en.wikipedia.org/wiki/Fire_Emblem_(2003_video_game)
  - https://en.wikipedia.org/wiki/Fire_Emblem:_The_Sacred_Stones
- 黄金太阳：
  - https://en.wikipedia.org/wiki/Golden_Sun
  - https://en.wikipedia.org/wiki/Golden_Sun:_The_Lost_Age
- 逆转裁判：https://en.wikipedia.org/wiki/Phoenix_Wright:_Ace_Attorney
- 宝可梦第三世代背景：https://en.wikipedia.org/wiki/Pok%C3%A9mon_(video_game_series)
- Stardew Valley RG34XXSP/KNULLI 移植：https://github.com/cdale11/sv-port
- PortMaster 游戏目录：https://portmaster.games/games/
- A Short Hike 搜索复核：https://github.com/search?q=RG34XX+%22A+Short+Hike%22&type=repositories 及 https://github.com/search?q=%22A+Short+Hike%22+PortMaster&type=repositories

网页资料用于确定发行和移植线索；ROM revision 的完整列表、校验值、消息地址和兼容性结论仍须基于合法取得的具体文件和实际模拟器验证后补录。
