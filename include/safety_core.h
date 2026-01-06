#ifndef SAFETY_CORE_H
#define SAFETY_CORE_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>

/* --- 核心常量 --- */
#define STATUS_PASS      0
#define STATUS_DROP      1  // 输入阶段：整单拒绝
#define STATUS_FLAG      2
#define STATUS_TRUNCATE  3  // 输出阶段：切断流

/* 阶段定义 (对应 PDF Page 4 分层防护) */
#define STAGE_INPUT_PROMPT    0x01 // 用户输入 (Prompt Injection check)
#define STAGE_OUTPUT_STREAM   0x02 // 模型输出 (Toxic Generation check)

/* 动作映射 */
#define ACTION_ALLOW        200
#define ACTION_BLOCK_L0     201
#define ACTION_BLOCK_L2     202
#define ACTION_TOXIC_DUMP   203

/* --- 数据结构 --- */

/* 策略配置 (RCU 管理) */
typedef struct policy_config {
    int version;
    double threshold_strict;
    
    /* 阶段开关 - 允许针对不同阶段关闭特定检查 */
    bool check_input_l0;
    bool check_input_l2;
    bool check_output_l0;
    bool check_output_l2;

    atomic_int ref_count;
} policy_config_t;

/* * 流式会话状态机
 * 这个结构体将伴随一次 LLM 生成的全生命周期。
 */
typedef struct safety_stream {
    /* 缓冲区管理 */
    char buffer[4096];      // 累积未检查的文本
    size_t buffer_len;
    
    int stage;              // STAGE_INPUT or STAGE_OUTPUT
    char *user_id;
    
    /* 引用当前的策略快照，保证整个流的一致性 */
    policy_config_t *policy_snapshot;
} safety_stream_t;

/* 上下文 (用于单次调用) */
typedef struct safety_ctx {
    const char *text;       // 当前要检查的文本片段
    size_t len;
    int stage;
    int action_code;
} safety_ctx_t;


/* --- API 原型 --- */

/* RCU 管理 */
void policy_init(void);
void policy_update(int version, double strict);
policy_config_t* policy_acquire(void);
void policy_release(policy_config_t *p);

/* 流式处理 API */
safety_stream_t* safety_stream_create(char *user_id, int stage);
void safety_stream_destroy(safety_stream_t *s);

/* * 核心热路径函数 
 * 返回 STATUS_PASS (0) 则继续生成
 * 返回 STATUS_TRUNCATE (3) 则立即中断
 */
int safety_stream_push(safety_stream_t *s, const char *token);

/* 过滤器实现 (内部使用) */
int filter_l0_rules(safety_ctx_t *ctx, policy_config_t *p);
int filter_l2_model(safety_ctx_t *ctx, policy_config_t *p);

#endif
