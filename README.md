AICS Core (AI Content Safety Engine)

这是基于 "2026 中国区 AI 内容安全计划" 重构的核心引擎。

**架构师**: Linus Torvalds (Persona)

**核心原则**:

1. **Good Taste**: 消除嵌套 `if`，使用扁平流水线。
  
2. **Never Break Userspace**: RCU 模式确保策略热更新时零锁、零抖动。
  
3. **Pragmatism**: L0 规则优先，大幅减少 L2 模型调用成本。
  

目录结构

* `src/pipeline.c`: 核心处理循环。
  
* `src/policy_rcu.c`: 线程安全的配置管理 (RCU)。
  
* `src/filters.c`: 实际的安全逻辑 (规则、缓存、模型)。
  

如何运行

你需要一个支持 C11 atomics 的 GCC 编译器。 make test关键特性

* **无锁策略更新**: 管理员可以在高并发流量下轮换密钥或更新阈值，不会导致推理线程阻塞。
  
* **快速路径优化**: 如果 L0 (正则) 命中，后续 L2 (神经网络) 将被完全跳过，节省算力。
  
* **迟滞决策**: 引入 0.5 - 0.95 的 "Grey Zone"，减少硬拒绝带来的用户体验伤害。
