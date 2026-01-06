#ifndef SAFETY_CORE_H
#define SAFETY_CORE_H

#include <stddef.h>
#include <stdatomic.h>
#include <stdbool.h>

/* --- 核心常量 --- */
#define STATUS_PASS      0
#define STATUS_DROP      1  
#define STATUS_FLAG      2
#define STATUS_TRUNCATE  3  

/* 阶段定义 */
#define STAGE_INPUT_PROMPT    0x01 
#define STAGE_OUTPUT_STREAM   0x02 
/* 新增：RAG 上下文阶段 (对应报告中的 Context Scanning) */
#define STAGE_RAG_CONTEXT     0x04 

/* 动作映射 */
#define ACTION_ALLOW        200
#define ACTION_BLOCK_L0     201
#define ACTION_BLOCK_L2     202
#define ACTION_TOXIC_DUMP   203

/* --- 策略配置 (RCU) --- */
typedef struct policy_config {
    int version;
    double threshold_strict;
    
    /* 阶段开关 */
    bool check_input_l0;
    bool check_input_l2;
    bool check_output_l0;
    bool check_output_l2;

    /* 新增：长上下文策略 */
    bool enable_anchor_scan;    // 是否开启 Head+Tail 锚点扫描
    size_t anchor_window_size;  // 锚点窗口大小 (如 1000 tokens)

    atomic_int ref_count;
} policy_config_t;

/* --- 数据结构 --- */

typedef struct safety_stream {
    char buffer[4096];
    size_t buffer_len;
    int stage;
    char *user_id;
    policy_config_t *policy_snapshot;
} safety_stream_t;

typedef struct safety_ctx {
    const char *text;
    size_t len;
    int stage;
    int action_code;
} safety_ctx_t;

/* --- API 原型 --- */

void policy_init(void);
void policy_update(int version, double strict);
policy_config_t* policy_acquire(void);
void policy_release(policy_config_t *p);

safety_stream_t* safety_stream_create(char *user_id, int stage);
void safety_stream_destroy(safety_stream_t *s);
int safety_stream_push(safety_stream_t *s, const char *token);

/* 新增：针对 RAG 场景的“分块并行扫描”接口 (对应报告方案 A) */
int safety_rag_check(const char *user_query, const char *long_context, char *user_id);

int filter_l0_rules(safety_ctx_t *ctx, policy_config_t *p);
int filter_l2_model(safety_ctx_t *ctx, policy_config_t *p);

#endif
