AICS Core (AI Content Safety Engine)

> "Talk is cheap. Show me the code." - Linus Torvalds

这是基于 **《2026 中国区 AI 内容安全计划》** 重构的高性能核心引擎 (MVP)。

本项目实现了一个C11 标准的、零依赖的 AI 安全网关内核，专注于解决高并发下的延迟抖动和流式截断问题。

最新版本已适配 "Long Context (长上下文)" 性能优化方案。🏛 核心架构原则

1. **Good Taste (好品味)**: 拒绝复杂的嵌套 `if-else`，采用扁平化的函数指针流水线。
  
2. **Never Break Userspace (用户至上)**: 使用 RCU (Read-Copy-Update) 模式管理策略配置，确保在密钥轮换、阈值热更新时，推理线程**零锁、零等待**。
  
3. **Pragmatism (实用主义)**:
  
  * **流式乐观门控**: 让 LLM 一边生成一边检查，只在缓冲区积攒到一定程度或发现高风险时才介入。
    
  * **锚点扫描 (Anchor Scanning)**: 针对 RAG/长文档场景，自动仅扫描 Head/Tail 窗口，解决 $O(n^2)$ 延迟瓶颈。
    

📂 目录结构

* `include/safety_core.h`: 核心 API 契约 (State Machine & RCU definitions)。
  
* `src/pipeline.c`: 流式处理主循环与 RAG 分块逻辑。
  
* `src/policy_rcu.c`: 线程安全的配置管理。
  
* `src/filters.c`: 安全过滤器实现 (L0/L2, Anchor Logic)。
  

🚀 快速开始

你需要一个支持 C11 atomics 的 GCC/Clang 编译器。 # 编译并运行压力测试 (Torture Test) make test

如果看到 `All Tests Passed`，说明内核逻辑在并发压力下是稳固的。✅ MVP 功能清单 (当前状态)

* $$x$$**流式状态机**: 支持 `safety_stream_push`，实现 Token 级的实时截断。
  
* $$x$$**RAG 分块扫描**: 新增 `safety_rag_check`，区分 Query (高危) 与 Context (低危) 处理逻辑。
  
* $$x$$**锚点优化**: 当输入超过 2000 tokens 时，L2 自动降级为扫描首尾各 1000 tokens。
  
* $$x$$**RCU 并发管理**: 支持多线程读取下的策略热更新，无锁设计。
  
* $$x$$**成本短路**: L0 命中即终止，跳过 L2。
  

🚧 路线图 (Roadmap)

本 MVP 包含核心逻辑的完整实现，但以下组件使用 **Mock (模拟)** 实现，需在生产环境中替换：

* **真实模型接入**: 目前 `filters.c` 使用字符串匹配模拟。需对接 ONNX Runtime / TensorRT C++ API。
  
* **向量数据库集成**: L1 阶段的向量检索需对接 Milvus/Faiss。
  
* **持久化层**: 异步日志目前仅打印到 stdout，需对接 Kafka 生产者客户端。
  
* **网络网关**: 需封装为 gRPC/HTTP 服务以供业务方调用。
  

📜 许可证

GPL v2.0
