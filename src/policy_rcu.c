#include "../include/safety_core.h"
#include <stdlib.h>
#include <stdio.h>

static _Atomic(policy_config_t *) GlobalPolicy;

static policy_config_t* create_policy_obj(int version, double strict) {
    policy_config_t *p = malloc(sizeof(policy_config_t));
    if (!p) return NULL;
    p->version = version;
    p->threshold_strict = strict;
    
    p->check_input_l0 = true;
    p->check_input_l2 = true;
    p->check_output_l0 = true;
    p->check_output_l2 = true;

    /* 默认开启长文本优化 */
    p->enable_anchor_scan = true;
    p->anchor_window_size = 1000; // 对应报告建议: Head(1000) + Tail(1000)
    
    atomic_init(&p->ref_count, 1);
    return p;
}

void policy_init(void) {
    policy_config_t *p = create_policy_obj(1, 0.90);
    atomic_store(&GlobalPolicy, p);
    printf("[Init] Policy System Ready (v1) with Anchor Scanning\n");
}

policy_config_t* policy_acquire(void) {
    policy_config_t *p;
    while (1) {
        p = atomic_load(&GlobalPolicy);
        atomic_fetch_add(&p->ref_count, 1);
        return p;
    }
}

void policy_release(policy_config_t *p) {
    if (!p) return;
    if (atomic_fetch_sub(&p->ref_count, 1) == 1) {
        free(p);
    }
}

void policy_update(int version, double strict) {
    policy_config_t *new_pol = create_policy_obj(version, strict);
    policy_config_t *old_pol = atomic_exchange(&GlobalPolicy, new_pol);
    printf("[Admin] Policy Updated v%d -> v%d\n", old_pol->version, version);
    policy_release(old_pol);
}
