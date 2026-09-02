# GravityShift UE5.8 移交文档

> 接手 AI 第一件事:把这份文档从头到尾读一遍。再去看 `C:\Users\20625\.zcode\skills\ue-nocode\SKILL.md` 和新增的 `C:\Users\20625\.zcode\skills\ue-cpp-build-cnpath\SKILL.md`(中文路径编译排雷)。
> 然后确认编辑器窗口标题(工作簿 SKILL.md 第 0 步:端口在监听 ≠ 目标项目在运行)。

## 0. 一句话现状

C++ 插件 `GravityShiftCore` **已编译通过**(修了 4 个真实 C++ 错误);测试关卡 `测试案例.umap` **已摆好并 PIE 验收**(Manager + 三种方块 + 测试房,第 10 节);**G/R 六向重力切换与重置已修好并实测通过**(Tick 轮询方案,第 11 节);代码已同步 GitHub(第 12 节)。**下一轮工作从规范包 P04 起**(第 9 节)。第 5 节的编辑器会话快照是历史信息,接手时以实际进程为准。

## 1. 环境事实(本机,2026-09-01 验证)

- 引擎:`D:\UE_5.8`(UE 5.8.2,中文本地化,VibeUE 引擎级已装)
- **当前项目:`D:\UE\MyProject2`(英文路径,从原 `D:\UE\我的项目2` 复制而来)**
  - 为什么复制:见 SKILL `ue-cpp-build-cnpath`——中文项目路径下 C++ 编译必崩(cl.exe 把 UTF-8 响应文件当 GBK 读,路径全乱码)。原项目一字未动,继续做重力系统就用 MyProject2。
- 原项目 `D:\UE\我的项目2` 仍保留(棚子、测试案例.umap、MCP/Python/VibeUE 配置都在),作为参考/回退。
- 参照规范:`D:\下载\UE58_GravityShift_AgentKit_v1\UE58_GravityShift_AgentKit_v1`(P00–P10 阶段、资产契约 `agent/asset_contracts.yaml`、Profile 数据 `data/DT_BlockProfiles.csv`)
- 插件源包:`D:\下载\GravityShift_UE58_ImportPack_v2\GravityShift_UE58_ImportPack_v2`(写包的 AI 说"静态检查通过但没在真引擎编译过"——果然一编就错,已修,见第 4 节)

## 2. MCP 连接方式

- 端点:`http://127.0.0.1:8000/mcp`(HTTP transport,协议 2025-06-18)
- 编辑器必须开着(本项目里 `bAutoStartServer=True` 在 `Config/DefaultEditorPerProjectUserSettings.ini` 的 `[ModelContextProtocol]` 段,但**复制项目后第一次启动没自动起**——上一轮是手动用控制台命令 `ModelContextProtocol.StartServer` 拉起的。原因没查清,猜测是 Saved 配置没复制导致;每次开新项目都先 `netstat :8000` 确认,没起就用兜底通道见下条)
- 兜底通道(不需要 MCP):`C:\Users\20625\.zcode\skills\ue-nocode\reference\ue_pyexec.py`,走引擎原生 Python 远程执行(组播 239.0.0.1:6766)。本项目 `Config/DefaultEngine.ini` 已配 `bRemoteExecution=True`,复制项目时一并带过来了,实测可用。
- MCP 客户端助手:`C:\Users\20625\.zcode\skills\ue-nocode\reference\ue.py`(子命令 `tools`/`toolsets`/`describe`/`call`/`py`/`raw`,多行代码走文件)。`mcp_http.py` 是最小握手客户端。
- 验证三步:HTTP 握手 → `tools/list`(应 10 个顶层工具,含 VibeUE 的 `execute_python_code`)→ `list_toolsets`(应 82 个工具集,30 个 VibeUE 服务)

## 3. 已完成 ✅

1. **插件安装**:`Plugins/GravityShiftCore` 已复制进 `D:\UE\MyProject2\Plugins\`,`MyProject2.uproject` 已启用 `GravityShiftCore`(外加 `PythonScriptPlugin`、`EditorScriptingUtilities`)
2. **编译通过**:`MyProject2Editor` target,Win64 Development。产物:
   - `D:\UE\MyProject2\Binaries\Win64\UnrealEditor-MyProject2.dll`
   - `D:\UE\MyProject2\Plugins\GravityShiftCore\Binaries\Win64\UnrealEditor-GravityShiftCore.dll`
   - 编译命令(关键:必须用引擎内置 dotnet + `-NoUBA`,见 SKILL `ue-cpp-build-cnpath`):
     ```
     $env:DOTNET_ROOT="D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64"
     $env:DOTNET_MULTILEVEL_LOOKUP="0"
     & "D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe" `
       "D:\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" `
       "MyProject2Editor" Win64 Development `
       "-project=D:\UE\MyProject2\MyProject2.uproject" -WaitMutex -NoUBA
     ```
3. **蓝图生成**(跑 `install_gravityshift_blueprints.py` + `validate_gravityshift_install.py`,都 PASS):
   - `/Game/GravityShift/Core/BP_GravityManager` ✅
   - `/Game/GravityShift/Components/BPC_GravityBody` ✅
   - `/Game/GravityShift/Components/BPC_Breakable` ✅
   - `/Game/GravityShift/Components/BPC_Resettable` ✅
   - `/Game/GravityShift/Interactions/BP_SurfaceModifierPanel` ✅
   - `/Game/GravityShift/Tests/BP_GravityDemoRoom` ✅
   - `/Game/GravityShift/Blocks/BP_BlockBase` ❌ **没建出来**(见第 6 节)
   - `/Game/GravityShift/NativeStarter/Blocks/BP_GS_BlockBase` ✅(安全版,原生父类)
4. **上一轮手搓的棚子碰撞修复**(在原项目里,测试案例.umap):网格 `CubeGridToolOutput_C388EDCD` 的 `body_setup.collision_trace_flag` 改成了 `CTF_USE_COMPLEX_AS_SIMPLE`(原来是 `USE_SIMPLE_AND_COMPLEX` 但 agg_geom 全 0,所以看得见走得穿)。复杂碰撞精确,门洞保持敞开。**这个修复在 MyProject2 里同样有效**(网格资产是同一份,Content 已复制)。
5. **三种方块区分材质**已建好:`/Game/GravityShift/Materials/M_Block_Static`(灰)、`M_Block_Gravity`(蓝)、`M_Block_Breaker`(橙)

## 4. 修过的 4 个 C++ 错误(已在源码里改掉,改的位置都在 `D:\UE\MyProject2\Plugins\GravityShiftCore\Source\`)

1. **UHT 拒绝 `Units="J"` 和 `Units="1/s"`**:UE 不认识这两个计量单位。`J`(焦耳)整段删掉;`1/s` 改成 `Hz`。涉及文件:`GravityShiftProfiles.h`、`GravityShiftTypes.h`、`GSBreakableComponent.h`、`GSGravityBodyComponent.h`、`GSGravityMathLibrary.h`
2. **`#include "Engine/PrimaryDataAsset.h"` 不存在**:UE5 里 `UPrimaryDataAsset` 在 `Engine/DataAsset.h`。改 `GravityShiftProfiles.h` 第 4 行。
3. **lambda 非法捕获静态局部变量**:`GSGravityDemoRoom.cpp` 第 23 行 `auto CreateWall = [this, &CubeMesh](...)`,`CubeMesh` 是 `static ConstructorHelpers::FObjectFinder`,静态存储期不能捕获。改成 `[this]`,lambda 内直接用 `CubeMesh`(静态局部可不捕获直接访问)。
4. **变量遮蔽类成员(C4458 当 error)**:`GSGravityManager.cpp` 第 245 行局部变量 `AActor* Owner` 遮蔽了 `AActor::Owner`。改名 `OwnerActor`。

**还有 2 个非阻塞但要注意的**:链接器偶尔报 `LNK1201`(写不了 PDB)/`LNK1136`(lib 损坏),根因是 UBA(Unreal Build Accelerator)的文件层在 `C:\ProgramData\Epic\UnrealBuildAccelerator\` 上 `SetFileInformationByHandle Access is denied`。**加 `-NoUBA` 编译就干净通过**。不排除本机某杀软/权限在拦 UBA,但 `-NoUBA` 已规避。

## 5. 当前编辑器会话状态

> 本节是 2026-09-01 第二轮的会话快照,PID/关卡状态已过时;接手时先确认编辑器是否在运行、`netstat :8000` 是否在监听,再核对当前打开的关卡。

- 编辑器进程 PID 44204,窗口标题应是 `MyProject2 - 虚幻编辑器`(开之前先 `Get-Process -Id 44204 | MainWindowTitle` 核对)
- MCP 在 8000 监听中(手动 `ModelContextProtocol.StartServer` 拉起的)
- **当前关卡 = `/Temp/Untitled_1`(空关卡,不是测试案例!)**——MyProject2 默认开空关卡。上一轮的 `setup_level.py` 把 Manager 摆到了这个 Untitled 里(还摆了两份,因为脚本跑了两遍中途崩),**测试方块没摆成**(因为 `block_cls` 加载失败,见下条)。Untitled 是临时的,可以直接抛弃不用管那两份 Manager。

## 6. 下一轮要做的事(按顺序)

### 6.1 打开测试案例关卡
```
# 在编辑器里:File > Open Level > D:\UE\MyProject2\Content\测试案例.umap
# 或 MCP:
python ue.py py 'import unreal; unreal.EditorAssetLibrary.sync_load_asset("/Game/测试案例.测试案例"); unreal.EditorLevelUtils.load_level("/Game/测试案例.测试案例")'
```
这个关卡里有:PlayerStart (0,0,92)、Floor (0,0,-0.5)、**棚子** (-3400,3100,0,网格 CubeGridToolOutput_C388EDCD,碰撞已修)。PlayerStart 在棚子内部三面墙+屋顶的开口处。

### 6.2 重跑安装器,补出原生 `BP_BlockBase`
上一轮删了旧壳 `BP_BlockBase`(Actor 父类空壳)但**没重跑安装器**,所以契约路径 `/Game/GravityShift/Blocks/BP_BlockBase` 是空的。重跑:
```
python ue.py py "@D:/UE/MyProject2/Plugins/GravityShiftCore/Content/Python/install_gravityshift_blueprints.py"
```
现在旧壳没了,安装器会直接在契约路径建出**原生父类版本**(父类 `GSGravityBlock`)。

### 6.3 摆三种方块 + Manager(脚本在 `D:\UE\.workbuddy\tmp\ue_scripts\setup_level.py`)
那个脚本逻辑对的(按 `data/DT_BlockProfiles.csv` 的参数:Static/Gravity/GravityBreaker),只是 `block_cls` 之前用错了路径。**改对路径**就行:脚本里 `load_bp_class("/Game/GravityShift/Blocks/BP_BlockBase")`(注意第 6.2 步跑完后这个路径才有)。摆位:
- 三方块在 PlayerStart 前方 +X 800、横向 ±350,贴地(Static z=52,Gravity/Breaker 从 z=200 落下)
- Manager 摆在 (-300,0,400) 附近(label `GravityShift_Manager`)
- 测试房 `BP_GravityDemoRoom` 摆在 (15000,0,330) 远处(自带地板顶面落在 z=0,自包含)
- 方块属性赋值用 `actor.set_editor_property("bSimulatePhysics",...)` 等(属性名见 `GSGravityBlock.h`:`bSimulatePhysics`/`bGravityAffected`/`GravityScale`/`MassOverrideKg`/`bUseCCD`/`bBreakable`)
- 赋值后调 `actor.apply_block_profile()`(UFUNCTION)把内联 Profile 应用到组件

### 6.4 PIE 验收
```
python ue.py call EditorToolset.EditorAppToolset StartPIE '{"options":{"bSimulate":false}}'
# 等 6~8s
python ue.py py @verify_pie.py   # 自己写:查 Manager 是否注册了 3 个 GravityBody,GetGravityDirection,调 RequestNextGravity 看方块横移
python ue.py call EditorToolset.EditorAppToolset StopPIE '{}'
```
玩家在 PIE 里按 G 循环六向重力、按 R 恢复。要送键的话用 `unreal.InputRecordingBlueprintLibrary` 或直接调 Manager 的 `RequestNextGravity()`/`ResetPuzzleState()`(都是 UFUNCTION)。

### 6.5 存盘
```
python ue.py py 'import unreal; unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()'
```

## 7. 已知坑(详见 SKILL `ue-cpp-build-cnpath`)

1. **中文项目路径下 C++ 编译必崩**——cl.exe 把 UTF-8 响应文件当 GBK 读,所有中文路径变乱码,C1083。解决:复制项目到英文路径。
2. **UBA 文件层 Access denied** 导致 LNK1136/LNK1201。解决:`-NoUBA`。
3. **UBT 5.8 要 .NET 10**,系统只有 6/8。必须用引擎内置 dotnet(`D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe`),设 `DOTNET_ROOT` 指向它。`Build.bat` 内部会做这事,但 `Build.bat` 经 cmd 转中文参数会乱码,所以**直接调 dotnet.exe 跑 UBT dll** 最稳。
4. **复制项目后 MCP 不自动起**——`bAutoStartServer=True` 配置在,但首次没起。兜底:控制台命令 `ModelContextProtocol.StartServer`(Python 里 `unreal.SystemLibrary.execute_console_command(None, "ModelContextProtocol.StartServer")`)。`ue_pyexec.py` 组播通道作为完全不依赖 MCP 的兜底。
5. **MCP 顶层工具 `execute_python_code` 异常时会把 stdout 吞掉**,只回 error_message。重逻辑要么拆步,要么整体 try/except。
6. **蓝图蓝图类加载**:用 `unreal.load_asset(path).generated_class()`,**别用** `unreal.load_class(None, path)`——后者在本环境偶尔返回 None(已踩)。
7. `StaticMeshComponent` 没有 `collision_enabled`/`use_ccd` 属性,要用方法 `get_collision_enabled()`/`set_use_ccd(bool)`。
8. `HitResult` 在本版本不是 subscriptable,要用 `.to_dict()`。

## 8. 关键路径速查

| 用途 | 路径 |
|---|---|
| 项目(做重力系统用这个) | `D:\UE\MyProject2` |
| 项目(原版,棚子/测试案例母本) | `D:\UE\我的项目2` |
| 引擎 | `D:\UE_5.8` |
| 引擎内置 dotnet | `D:\UE_5.8\Engine\Binaries\ThirdParty\DotNet\10.0\win-x64\dotnet.exe` |
| UBT dll | `D:\UE_5.8\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll` |
| 测试关卡 | `D:\UE\MyProject2\Content\测试案例.umap` |
| 插件源码 | `D:\UE\MyProject2\Plugins\GravityShiftCore\Source\GravityShiftCore\` |
| 插件 Python 脚本 | `D:\UE\MyProject2\Plugins\GravityShiftCore\Content\Python\` |
| 规范包 | `D:\下载\UE58_GravityShift_AgentKit_v1\UE58_GravityShift_AgentKit_v1\` |
| 导入包 | `D:\下载\GravityShift_UE58_ImportPack_v2\GravityShift_UE58_ImportPack_v2\` |
| MCP CLI 助手 | `C:\Users\20625\.zcode\skills\ue-nocode\reference\ue.py` |
| UE 无代码工作簿 | `C:\Users\20625\.zcode\skills\ue-nocode\SKILL.md` |
| 中文路径编译排雷(新) | `C:\Users\20625\.zcode\skills\ue-cpp-build-cnpath\SKILL.md` |
| 临时排雷脚本 | `D:\UE\.workbuddy\tmp\ue_scripts\` |

## 9. 还没碰的部分(规范包里的后续阶段)

按 `D:\下载\UE58_GravityShift_AgentKit_v1\...\agent\phase_prompts\` 的 P04 起:
- P04 玩家任意方向行走(Character Movement 完整重力,目前插件**没实现**——README 写明这版只做了物块系统)
- P05 落地响应、P06 冲击破坏、P07 表面交互、P08 重置 UI、P09 关卡测试、P10 稳定

本插件包只覆盖:六向重力切换(G 键)+ 物块物理 + 表面装置 + 冲击破坏 + 状态恢复。玩家自身六向行走要单开。

## 10. 接手轮完成记录(2026-09-01 第二轮接手 ✅ 全部完成)

第 6 节五步全部做完并 PIE 实测验收:

- ✅ 6.1 测试案例.umap 已打开(当前关卡);Untitled_1 已抛弃(未保存切换,无弹窗)
- ✅ 6.2 安装器重跑 PASS:`/Game/GravityShift/Blocks/BP_BlockBase` 已建出(原生父类 GSGravityBlock)
- ✅ 6.3 摆位完成:Manager(-300,0,400)+ 三方块(Static 灰 800,-350,52 / Gravity 蓝 800,0 从 z=200 落地 / GravityBreaker 橙 800,350,CCD+可破坏)+ 测试房(15000,0,330)。三方块与出生点都在棚子包围盒内(棚子 X -3400~3100 / Y -2300~3200,已核实)
- ✅ 6.4 PIE 实测验收:
  - 自定义重力下落:Gravity/Breaker 从 z=199 落到 49(Static sim=False 不动)
  - 六向切换:`request_next_gravity(m, unreal.GSGravityChangeReason.SCRIPTED)` → ACCEPTED,重力 (0,0,-1)→(1,0,0),两个物理块横移 +130cm
  - 重置:`reset_puzzle_state()` → 重力回 -Z(reason=RESET),方块回原位重新下落
- ✅ 6.5 关卡已保存

**本轮新踩的 API 坑(已可用)**:
1. `RequestNextGravity(InstigatorActor, Reason)` 两个参数都必填;python 侧枚举**不带 E 前缀**:`unreal.GSGravityChangeReason.SCRIPTED`(成员 SCRIPTED 大写)——`unreal.EGSGravityChangeReason` 不存在,原始 int 也不接受
2. `setup_level.py` 的"删除旧壳"名单必须剔除 `BP_BlockBase`(它现在是原生资产,删了方块就摆不出来)——本轮已改
3. PIE 中方块会持续被自定义重力推动,编辑器关卡里的摆位每次 PIE 后不变(PIE 世界独立)

## 11. G/R 按键修复记录(2026-09-01 第三轮 ✅ 验收通过)

**用户报告**:PIE 里按 G/R 无反应;打开测试块蓝图编辑器看不到东西(后者=设计使然,见上)。

**根因**:`GSGravityManager::BeginPlay` 里的 `EnableInput(PC)+BindKey(G/R)` 对**关卡摆放的实例**不生效——绑定和键都正常,但推到 PlayerController 输入栈的 InputComponent 会丢(DemoRoom 里子 Actor 组件生成的原生实例却一直正常,原因未深究)。实测:关卡实例 revision 恒 0,DemoRoom 实例每次按键都 +1;Python 里手动 `enable_input(pc)` 一次立刻修好。

**修复(改的是 C++,已重编译)**:`GSGravityManager` 弃用 BindKey,改为 **Tick 轮询 PlayerInput 键状态 + 边缘检测**:
- 构造器 `PrimaryActorTick.bCanEverTick = true`
- `Tick()`:`PC->IsInputKeyDown(DebugNextGravityKey/DebugResetKey)` 上升沿 → `HandleDebugNextGravity()`/`HandleDebugReset()`
- 边缘检测天然免疫按键长按重复;只要键进了游戏视口就一定可见,与输入栈时序无关
- BeginPlay 里保留一条 LOG(打印轮询键名),方便以后排查
- 编译:`dotnet.exe UBT.dll MyProject2Editor Win64 Development -project=D:\UE\MyProject2\MyProject2.uproject -WaitMutex -NoUBA`(14s 通过)

**PIE 真实按键验收(OS 级注入,2026-09-01)**:
- G:两个 Manager 同步 rev 0→1,Z-→X+,两物理方块横移到 x≈2950(+2156cm)✅
- R:rev→2,轴回 Z-,reason=RESET,方块回 (800,±350) 落地 ✅

**验收方法论(重要)**:PIE 里模拟真实按键必须 PowerShell `SetProcessDPIAware()` + `SetCursorPos`(CUA screen 坐标即物理像素)+ 点击聚焦视口 + `keybd_event`(VK: G=0x47 R=0x52 W=0x57);CUA `cursor_position` 回读光标位置确认落点。**关卡加载弹的"消息日志"窗口会抢键盘焦点**——按键前必须关掉它(PostMessage WM_CLOSE 到窗口句柄最稳)或先点一下游戏画面。`ke <键> Down` 控制台命令会触发 InputComponent 处理器但不更新 is_input_key_down 状态表,别用它做唯一证据。

## 12. GitHub 同步记录(2026-09-02 ✅)

- 仓库:**https://github.com/madtwo/MyProject2**(私有)
- 入库内容与分类见根目录 `README.md`;标准 UE .gitignore 排除 Binaries/Intermediate/Saved/DerivedDataCache/.vs(克隆后右键 uproject 重编译即可,增量约 14s)
- 初始提交 291 文件 / 约 130MB,无单文件 >50MB(未用 LFS)
- **提交方式**:当时 FlClash 未开、github.com:443 与 ssh.github.com:443 均被重置,git 协议不可用;走 api.github.com REST 直传(blobs→tree→commit→ref)。空仓库上 git-data API 会 409,先经 Contents API 放 README 引导,因此**远端历史 = 引导提交 + 全量提交,与本地历史(单根提交)内容相同但 sha 不同**。开代理后执行一次 `git fetch origin && git reset --hard origin/main` 对齐,之后正常 `git push` 即可(本地分支已设好跟踪)
- 推送脚本固化在 `C:\Users\20625\.zcode\skills\ue-nocode\reference\push_via_api.py`;令牌读取、网络封锁面、流程坑详见 ue-nocode SKILL 的"项目同步 GitHub"章节
- 令牌来源:Windows 凭证管理器 `GitHub - https://api.github.com/madtwo`(gh 时代的 OAuth token,repo 权限)
- 最后同步位置:远端 main head `db9d68a`(= 本条 §12 的内容),与本地提交 `65d55ae` 内容逐字节一致;此后本地若无新提交,即无未推送内容
