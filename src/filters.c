#include "../include/safety_core.h"
#include <string.h>
#include <stdio.h>

/* --- L0: 规则 (Regex/Bloom Filter) --- */
int filter_l0_rules(safety_ctx_t *ctx, policy_config_t *policy) {
    // 1. 根据阶段检查开关
    if (ctx->stage == STAGE_INPUT_PROMPT && !policy->check_input_l0) return STATUS_PASS;
    if (ctx->stage == STAGE_OUTPUT_STREAM && !policy->check_output_l0) return STATUS_PASS;

    // 2. 简单的关键词匹配 (模拟 AC 自动机)
    // 注意：在流式输出中，这里检查的是 "accumulated buffer"
    
    if (strstr(ctx->text, "rm -rf") != NULL) {
        ctx->action_code = ACTION_BLOCK_L0;
        return STATUS_DROP;
    }
    
    // 模拟针对 Output 的特定规则
    if (ctx->stage == STAGE_OUTPUT_STREAM) {
        if (strstr(ctx->text, "KILL ALL HUMANS") != NULL) {
            ctx->action_code = ACTION_BLOCK_L0;
            return STATUS_TRUNCATE;
        }
    }

    return STATUS_PASS;
}

/* --- L2: 模型 (Heavy Inference) --- */
int filter_l2_model(safety_ctx_t *ctx, policy_config_t *policy) {
    // 开关检查
    if (ctx->stage == STAGE_INPUT_PROMPT && !policy->check_input_l2) return STATUS_PASS;
    if (ctx->stage == STAGE_OUTPUT_STREAM && !policy->check_output_l2) return STATUS_PASS;

    // 模拟模型推理成本
    // 只有当文本长度足够长，或者包含特定语义时才拦截
    
    double score = 0.0;
    
    // 模拟检测 Jailbreak 意图 (Input)
    if (ctx->stage == STAGE_INPUT_PROMPT) {
        if (strstr(ctx->text, "DAN Mode") != NULL) score = 0.99;
    }
    
    // 模拟检测有毒内容 (Output)
    if (ctx->stage == STAGE_OUTPUT_STREAM) {
        if (strstr(ctx->text, "instructions to build a bomb") != NULL) score = 0.98;
    }

    if (score > policy->threshold_strict) {
        ctx->action_code = ACTION_BLOCK_L2;
        // 如果是流，返回 TRUNCATE；如果是 Prompt，返回 DROP
        return (ctx->stage == STAGE_OUTPUT_STREAM) ? STATUS_TRUNCATE : STATUS_DROP;
    }

    return STATUS_PASS;
}
