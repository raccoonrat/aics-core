#include "../include/safety_core.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* 模拟简单的 token 计数 (假设 1 char = 1 token 以简化演示) */
#define COUNT_TOKENS(s, l) (l)

/* --- L0: 规则 --- */
int filter_l0_rules(safety_ctx_t *ctx, policy_config_t *policy) {
    // 快速检查，不做任何裁剪 (因为正则很快)
    if (strstr(ctx->text, "rm -rf") != NULL) {
        ctx->action_code = ACTION_BLOCK_L0;
        return STATUS_DROP;
    }
    return STATUS_PASS;
}

/* --- L2: 模型 (支持 Anchor Scanning) --- */
int filter_l2_model(safety_ctx_t *ctx, policy_config_t *policy) {
    if (ctx->stage == STAGE_INPUT_PROMPT && !policy->check_input_l2) return STATUS_PASS;

    const char *scan_target = ctx->text;
    char *temp_buffer = NULL;

    // --- 锚点扫描逻辑 (Anchor Scanning) ---
    // 如果策略开启，且文本长度超过两倍窗口，则只取首尾
    if (policy->enable_anchor_scan && ctx->len > (policy->anchor_window_size * 2)) {
        size_t window = policy->anchor_window_size;
        
        // 分配临时缓冲区: Head + "..." + Tail + \0
        temp_buffer = malloc(window * 2 + 5); 
        if (temp_buffer) {
            // Copy Head
            memcpy(temp_buffer, ctx->text, window);
            // Copy Tail
            memcpy(temp_buffer + window, ctx->text + ctx->len - window, window);
            temp_buffer[window * 2] = '\0';
            
            // 指向新缓冲区进行扫描
            scan_target = temp_buffer;
            
            // printf("[Debug] Anchor Scan Active: Original %zu -> Scanned %zu\n", ctx->len, window*2);
        }
    }

    // --- 模拟模型推理 ---
    double score = 0.0;
    
    // 即使在长文本中，如果 "DAN Mode" 出现在开头或结尾，我们也能抓住它
    if (strstr(scan_target, "DAN Mode") != NULL) score = 0.99;
    
    // 清理临时资源
    if (temp_buffer) free(temp_buffer);

    if (score > policy->threshold_strict) {
        ctx->action_code = ACTION_BLOCK_L2;
        return STATUS_DROP;
    }

    return STATUS_PASS;
}
