#include "../include/safety_core.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define L2_CHECK_THRESHOLD 50 // 攒够 50 字符才跑一次模型

/* --- 流生命周期管理 --- */

safety_stream_t* safety_stream_create(char *user_id, int stage) {
    safety_stream_t *s = malloc(sizeof(safety_stream_t));
    s->buffer_len = 0;
    s->buffer[0] = '\0';
    s->stage = stage;
    s->user_id = user_id; // 实际应深拷贝
    
    // 绑定当前策略版本，确保流的一致性
    s->policy_snapshot = policy_acquire();
    return s;
}

void safety_stream_destroy(safety_stream_t *s) {
    if (s) {
        policy_release(s->policy_snapshot);
        free(s);
    }
}

/* --- 核心推流函数 --- */

int safety_stream_push(safety_stream_t *s, const char *token) {
    // 1. 防溢出保护
    if (s->buffer_len + strlen(token) >= sizeof(s->buffer) - 1) {
        // 缓冲区满了。策略选择：
        // A) 强制运行一次 L2 并清空
        // B) 滑动窗口
        // 这里为了简单，我们选择清空前半部分 (Reset)
        s->buffer_len = 0; 
        s->buffer[0] = '\0';
    }

    // 2. 拼接 Token
    strcat(s->buffer, token);
    s->buffer_len += strlen(token);

    // 构造临时 Context
    safety_ctx_t ctx = {
        .text = s->buffer,
        .len = s->buffer_len,
        .stage = s->stage,
        .action_code = ACTION_ALLOW
    };

    // 3. 运行 L0 (Hot Path - Run Every Time)
    // 这是 "实时流式安全" 的第一道防线
    int status = filter_l0_rules(&ctx, s->policy_snapshot);
    if (status != STATUS_PASS) {
        return status; // 立即截断!
    }

    // 4. 运行 L2 (Speculative - Run Batch)
    // 只有当缓冲区够大时才运行，这就是 "性能优化"
    if (s->buffer_len > L2_CHECK_THRESHOLD) {
        status = filter_l2_model(&ctx, s->policy_snapshot);
        if (status != STATUS_PASS) {
            return status; // 延迟截断
        }
        
        // 如果 L2 通过了，我们可以安全地清空缓冲区吗？
        // 不一定。可能这就是 "kill" 的前半段。
        // 但为了演示 Speculative Window，我们假设检查通过即可重置窗口。
        // 实际生产中会使用 "Overlap Window" (重叠窗口)。
        s->buffer_len = 0;
        s->buffer[0] = '\0';
    }

    // 5. 乐观放行
    // 还没到 L2 阈值，且 L0 没报错 -> 直接发给用户
    return STATUS_PASS;
}
