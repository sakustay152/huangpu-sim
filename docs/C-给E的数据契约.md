# C → E 数据契约（SFML 版）

> 维护：组员 C（`player/GoalTracker`）。对接：组员 E（SFML UI）。
> 原则：**E 只读、C 只算**。E 不自己算总资产，也不判结局；结局归 D 的 `EndingJudge`。
> 快照入口：`GoalTracker::snapshot(const GameContext&) const` → `GoalSnapshot`（纯只读，不改任何状态）。

## 1. GoalSnapshot 字段（逐项核对 getter）

| 字段 | 来源 | 类型 | 刷新时机 | 备注 |
| --- | --- | --- | --- | --- |
| `cash` | `Player::wallet().get()` | int（元） | 交易 / 事件 / 周结算 / 炒股后 | 可能为负（融资爆仓走 `Wallet::charge`，全游戏唯一负债入口） |
| `inventoryValue` | `TradeSystem::estimatePortfolioValue(ctx)` | int（元） | 背包变动后 | 按「各地最高卖价 × 数量」估值，不是进货成本；**E 不要自己重算** |
| `totalAssets` | `cash + inventoryValue` | int（元） | 同上 | 与 `EndingJudge` 的 `totalAssetsMin` 条件同口径（`Condition.cpp` 里就是这两项相加） |
| `targetMoney` | `ConfigManager::balance().targetMoney` | int（元） | 配置加载后固定 | 默认 `12000`；**只是 UI 目标线**，具体结局阈值在 `data/endings.json` |
| `studyScore` | `Player::studyScore()` | int（0–100） | 学习 / 周结算后 | `addStudy` 与 `StudySystem` 都会 clamp 到 0–100；≥85 奖学金 |
| `passScore` | `balance().passScore` | int | 配置固定 | 默认 `60`；期末低于它 → `failed = true` |
| `maxAffinity` | `NpcSystem::maxAffinity()` | int（0–100） | 社交 / 事件后 | 所有 NPC 中的最高好感，不是某一个 NPC |
| `week` | `TimeSystem::week()` | int | 每次行动 / 周结算后 | 从 1 开始 |
| `weeks` | `balance().weeks` | int | 配置固定 | 默认 `18`（16 教学周 + 2 考试周） |
| `actionsLeft` | `TimeSystem::actionsLeft()` | int | 每次行动后 | 0 = 本周行动点耗尽，需要周结算 |
| `apPerWeek` | `TimeSystem::apPerWeek()` | int | 配置固定 | 默认 `6` |
| `stamina` | `Player::energy().get()` | int | 打工 / 炒股 / 熬夜学习 / 休息后 | 范围 `0 … staminaMax`；低于阈值收益减半 |
| `staminaMax` | `Player::energy().max()` | int | 配置固定 | 默认 `100` |
| `slotsUsed` | `Player::inventory().usedSlots()` | int（格） | 交易 / 事件后 | = Σ(数量 × 每件占格) |
| `slotsMax` | `Player::inventory().capacity()` | int（格） | 配置固定 | 默认 `30` |
| `failed` | `Player::study().failed()` | bool | 期末周结算后 | `true` = 已挂科（补考费已扣），结局判定用 `not:failed` |

补充：`cash` / `inventoryValue` / `totalAssets` 三者恒满足 `totalAssets == cash + inventoryValue`，UI 可以只用 `totalAssets` 画进度条，用另两项做明细。

## 2. 交易结果（买卖按钮）

`TradeSystem::buy/sell` 返回 `TradeResult`：

| 字段 | 类型 | 用途 |
| --- | --- | --- |
| `ok` | bool | 成功与否 |
| `msg` | std::string | **可直接显示**的完整中文提示（已含数量/金额/原因） |
| `amount` | int | 成交总额（成功时） |
| `code` | `TradeError` 枚举 | 失败原因，用于图标/音效/分类提示，不做文案拼接 |
| `unitPrice` | int | 单价（失败时也给报价） |
| `maxQty` | int | 本次最多可成交数量（失败时用于「最多 X 件」） |

`TradeError`：`None / InvalidCount / UnknownGoods / WrongLocation / NotEnoughCash / NotEnoughSlot / QuotaExceeded / NotOwned`。

## 3. 刷新时机与事件

快照是**拉取式**：任何事件都不会替你保存快照，UI 收到事件后重新调一次 `snapshot(ctx)` 即可（读一次很便宜，也可每帧读）。

| 事件 | 发布者 | UI 该刷什么 |
| --- | --- | --- |
| `TradeCompletedEvent` | `TradeSystem` | 现金 / 背包 / 总资产 / 交易流水 |
| `ActionTakenEvent` | `Session` | 行动点 / 体力 / 时间 / 全部目标数值 |
| `WeekStartedEvent` | `WeekSettlement`、`GameContext::initNewGame` | 周数 / 行动点 / 行情 / 周报 |
| `EndingDecidedEvent` | `Game` | 切结局画面（此时不要再用快照推断结局） |

## 4. 禁止事项

1. **禁止** UI 直接改 `Player` / `Market` / `TimeSystem` 的状态；所有修改走各自公开方法（如 `wallet().add`、`inventory().add`、`addStudy`）。
2. **禁止** UI 持有 `GameContext` 指针跨屏/跨场景长期保存；只在当前帧/当前回调里取 `GameContext&`。
3. **禁止** UI 自己写「现金 + 库存估值」公式或遍历背包算总资产 —— 一律用 `snapshot()`，否则会和结局判定漂移。
4. **禁止** UI 用 `totalAssets >= targetMoney` 自行判结局；结局只由 `EndingJudge::judge()` 决定（阈值在 `data/endings.json`）。
5. **禁止** UI 调 `StudySystem::setScore()` 改分数（那是旧档迁移/测试用）；加分走 `Player::addStudy()`。
6. **禁止**缓存 `GoalSnapshot` 跨帧/跨屏当存档用；需要历史数据请让 C 加接口，不要在 UI 里攒状态。
