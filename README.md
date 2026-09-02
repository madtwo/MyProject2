# MyProject2 — 六向重力解谜 (GravityShift)

UE 5.8 C++ 项目。玩法逻辑全部在 C++ 插件里，蓝图只是挂资产的空壳（这是设计，不是缺失）。

## 代码分类

| 目录 | 内容 |
|------|------|
| `Plugins/GravityShiftCore/Source/` | **核心 C++ 源码**：`GSGravityManager`（六向重力状态机 + G/R 调试键 Tick 轮询）、`GSGravityBlock`/`GSGravityBreakerBlock`（方块）、`GSDemoRoom`（演示房，自带子 Actor 管理器） |
| `Plugins/GravityShiftCore/Content/Python/` | 插件安装脚本：一键生成蓝图壳资产（`install_gravityshift_blueprints.py`） |
| `Content/` | 关卡与资产：`测试案例.umap`（测试关：棚子、三个测试方块、GravityShift_Manager、DemoRoom）、`GravityShift/` 下蓝图壳 |
| `Config/` | 引擎配置（DefaultEngine.ini 等） |
| `MyProject2.uproject` | 工程入口 |
| `HANDOVER_GravityShift.md` | 交接/施工记录文档 |

**不入库（已 gitignore）**：`Binaries/`、`Intermediate/`、`Saved/`、`DerivedDataCache/`、`.vs/` —— 全是编译器和编辑器生成物，克隆后右键 uproject 编译即可还原（增量编译约 14 秒）。

## 操作说明

PIE 内按 **G** 切换重力方向（六向循环），**R** 重置方块。按键在 `GSGravityManager::Tick` 里轮询 `IsInputKeyDown` 实现（BindKey 在关卡摆放实例上不可靠，详见 HANDOVER 第 11 节）。
