#include "../include/safety_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ... 原有的 safety_stream 代码保持不变 ... */
#define L2_CHECK_THRESHOLD 50

safety_stream_t* safety_stream_create(char *user_id, int stage) {
    safety_stream_t *s = malloc(sizeof(safety_stream_t));
    s->buffer_len = 0;
    s->buffer[0] = '\0';
    s->stage = stage;
    s->user_id = user_id; 
    s->policy_snapshot = policy_acquire();
    return s;
}

void safety_stream_destroy(safety_stream_t *s) {
    if (s) {
        policy_release(s->policy_snapshot);
        free(s);
    }
}

int safety_stream_push(safety_stream_t *s, const char *token) {
    if (s->buffer_len + strlen(token) >= sizeof(s->buffer) - 1) {
        s->buffer_len = 0; 
        s->buffer[0] = '\0';
    }
    strcat(s->buffer, token);
    s->buffer_len += strlen(token);

    safety_ctx_t ctx = {
        .text = s->buffer, .len = s->buffer_len,
        .stage = s->stage, .action_code = ACTION_ALLOW
    };

    int status = filter_l0_rules(&ctx, s->policy_snapshot);
    if (status != STATUS_PASS) return status;

    if (s->buffer_len > L2_CHECK_THRESHOLD) {
        status = filter_l2_model(&ctx, s->policy_snapshot);
        if (status != STATUS_PASS) return status;
        s->buffer_len = 0;
        s->buffer[0] = '\0';
    }
    return STATUS_PASS;
}

/* --- 新增：RAG 场景专用接口 (Chunk-Parallel Scanning) --- */
int safety_rag_check(const char *user_query, const char *long_context, char *user_id) {
    policy_config_t *policy = policy_acquire();
    int final_status = STATUS_PASS;

    /* 步骤 1: 扫描 User Query (高危，必须全量扫描) */
    safety_ctx_t query_ctx = {
        .text = user_query,
        .len = strlen(user_query),
        .stage = STAGE_INPUT_PROMPT,
        .action_code = ACTION_ALLOW
    };
    
    if (filter_l0_rules(&query_ctx, policy) == STATUS_DROP ||
        filter_l2_model(&query_ctx, policy) == STATUS_DROP) {
        final_status = STATUS_DROP;
        goto cleanup;
    }

    /* 步骤 2: 扫描 Long Context (低危，使用 Anchor Scanning) */
    /* 在真实实现中，这可以与步骤 1 并行执行 (Parallel) */
    safety_ctx_t context_ctx = {
        .text = long_context,
        .len = strlen(long_context),
        .stage = STAGE_RAG_CONTEXT,
        .action_code = ACTION_ALLOW
    };
    
    // 注意：这里的 filter_l2_model 会自动触发 Anchor Logic (如果长度超标)
    if (filter_l2_model(&context_ctx, policy) == STATUS_DROP) {
        // 对于 Context 中的风险，我们可能选择 DROP，或者只 WARNING
        // 这里为了安全选择 DROP
        final_status = STATUS_DROP;
        goto cleanup;
    }

cleanup:
    policy_release(policy);
    return final_status;
}
