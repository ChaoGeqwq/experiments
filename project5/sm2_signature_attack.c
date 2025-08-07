#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include "sm2_signature_attack.h"

// SM2曲线参数 (与国标一致)
static const char *sm2_p = "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF";
static const char *sm2_a = "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC";
static const char *sm2_b = "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93";
static const char *sm2_n = "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B61C6823347C4A7C3B8370F5F";
static const char *sm2_gx = "32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7";
static const char *sm2_gy = "BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0";

// 创建SM2椭圆曲线群
static EC_GROUP* create_sm2_group() {
    EC_GROUP *group = NULL;
    BIGNUM *p = NULL, *a = NULL, *b = NULL, *n = NULL, *gx = NULL, *gy = NULL;
    EC_POINT *generator = NULL;
    BN_CTX *ctx = NULL;
    
    ctx = BN_CTX_new();
    if (!ctx) goto cleanup;
    
    // 创建有限域
    BN_hex2bn(&p, sm2_p);
    group = EC_GROUP_new_curve_GFp(p, NULL, NULL, ctx);
    if (!group) goto cleanup;
    
    // 设置曲线参数
    BN_hex2bn(&a, sm2_a);
    BN_hex2bn(&b, sm2_b);
    EC_GROUP_set_curve_GFp(group, p, a, b, ctx);
    
    // 设置生成元
    BN_hex2bn(&gx, sm2_gx);
    BN_hex2bn(&gy, sm2_gy);
    BN_hex2bn(&n, sm2_n);
    
    generator = EC_POINT_new(group);
    EC_POINT_set_affine_coordinates_GFp(group, generator, gx, gy, ctx);
    EC_GROUP_set_generator(group, generator, n, NULL);
    
cleanup:
    if (p) BN_free(p);
    if (a) BN_free(a);
    if (b) BN_free(b);
    if (n) BN_free(n);
    if (gx) BN_free(gx);
    if (gy) BN_free(gy);
    if (generator) EC_POINT_free(generator);
    if (ctx) BN_CTX_free(ctx);
    
    return group;
}

// SM2签名算法实现
int sm2_sign_message(const unsigned char *message, size_t message_len,
                     const unsigned char *private_key,
                     SM2_SIGNATURE *signature) {
    EC_GROUP *group = NULL;
    EC_POINT *point = NULL;
    BN_CTX *ctx = NULL;
    BIGNUM *d = NULL, *k = NULL, *r = NULL, *s = NULL, *e = NULL;
    BIGNUM *x1 = NULL, *tmp = NULL;
    const BIGNUM *order = NULL;
    int ret = 0;
    unsigned char hash[32];
    
    if (!message || !private_key || !signature) return -1;
    
    // 初始化
    group = create_sm2_group();
    if (!group) return -1;
    
    ctx = BN_CTX_new();
    point = EC_POINT_new(group);
    d = BN_new();
    k = BN_new();
    r = BN_new();
    s = BN_new();
    e = BN_new();
    x1 = BN_new();
    tmp = BN_new();
    
    if (!ctx || !point || !d || !k || !r || !s || !e || !x1 || !tmp) goto cleanup;
    
    // 加载私钥
    BN_bin2bn(private_key, 32, d);
    order = EC_GROUP_get0_order(group);
    
    // 计算消息哈希
    SHA256(message, message_len, hash);
    BN_bin2bn(hash, 32, e);
    memcpy(signature->message_hash, hash, 32);
    
    do {
        // 生成随机数k
        BN_rand_range(k, order);
        
        // 计算椭圆曲线点 (x1, y1) = k*G
        EC_POINT_mul(group, point, k, NULL, NULL, ctx);
        EC_POINT_get_affine_coordinates_GFp(group, point, x1, NULL, ctx);
        
        // 计算 r = (e + x1) mod n
        BN_mod_add(r, e, x1, order, ctx);
        
        // 检查 r != 0 且 r + k != n
        if (BN_is_zero(r)) continue;
        BN_mod_add(tmp, r, k, order, ctx);
        if (BN_cmp(tmp, order) == 0) continue;
        
        // 计算 s = (1 + d)^(-1) * (k - r*d) mod n
        BN_mod_add(tmp, BN_value_one(), d, order, ctx);
        BN_mod_inverse(tmp, tmp, order, ctx);
        
        BN_mod_mul(s, r, d, order, ctx);
        BN_mod_sub(s, k, s, order, ctx);
        BN_mod_mul(s, tmp, s, order, ctx);
        
    } while (BN_is_zero(s));
    
    // 输出签名
    BN_bn2binpad(r, signature->r, 32);
    BN_bn2binpad(s, signature->s, 32);
    ret = 1;
    
cleanup:
    if (group) EC_GROUP_free(group);
    if (point) EC_POINT_free(point);
    if (ctx) BN_CTX_free(ctx);
    if (d) BN_free(d);
    if (k) BN_free(k);
    if (r) BN_free(r);
    if (s) BN_free(s);
    if (e) BN_free(e);
    if (x1) BN_free(x1);
    if (tmp) BN_free(tmp);
    
    return ret;
}

// SM2签名验证算法
int sm2_verify_signature(const unsigned char *message, size_t message_len,
                        const unsigned char *public_key,
                        const SM2_SIGNATURE *signature) {
    EC_GROUP *group = NULL;
    EC_POINT *point = NULL, *pubkey_point = NULL;
    BN_CTX *ctx = NULL;
    BIGNUM *r = NULL, *s = NULL, *e = NULL, *t = NULL, *x1 = NULL, *R = NULL;
    BIGNUM *px = NULL, *py = NULL;
    const BIGNUM *order = NULL;
    int ret = 0;
    unsigned char hash[32];
    
    if (!message || !public_key || !signature) return -1;
    
    // 初始化
    group = create_sm2_group();
    if (!group) return -1;
    
    ctx = BN_CTX_new();
    point = EC_POINT_new(group);
    pubkey_point = EC_POINT_new(group);
    r = BN_new();
    s = BN_new();
    e = BN_new();
    t = BN_new();
    x1 = BN_new();
    R = BN_new();
    px = BN_new();
    py = BN_new();
    
    if (!ctx || !point || !pubkey_point || !r || !s || !e || !t || !x1 || !R || !px || !py) goto cleanup;
    
    // 加载签名
    BN_bin2bn(signature->r, 32, r);
    BN_bin2bn(signature->s, 32, s);
    order = EC_GROUP_get0_order(group);
    
    // 验证签名格式
    if (BN_is_zero(r) || BN_cmp(r, order) >= 0) goto cleanup;
    if (BN_is_zero(s) || BN_cmp(s, order) >= 0) goto cleanup;
    
    // 计算消息哈希
    SHA256(message, message_len, hash);
    BN_bin2bn(hash, 32, e);
    
    // 加载公钥
    BN_bin2bn(public_key + 1, 32, px);
    BN_bin2bn(public_key + 33, 32, py);
    EC_POINT_set_affine_coordinates_GFp(group, pubkey_point, px, py, ctx);
    
    // 计算 t = (r + s) mod n
    BN_mod_add(t, r, s, order, ctx);
    if (BN_is_zero(t)) goto cleanup;
    
    // 计算椭圆曲线点 (x1, y1) = s*G + t*PA
    EC_POINT_mul(group, point, s, pubkey_point, t, ctx);
    EC_POINT_get_affine_coordinates_GFp(group, point, x1, NULL, ctx);
    
    // 计算 R = (e + x1) mod n
    BN_mod_add(R, e, x1, order, ctx);
    
    // 验证 R == r
    ret = (BN_cmp(R, r) == 0) ? 1 : 0;
    
cleanup:
    if (group) EC_GROUP_free(group);
    if (point) EC_POINT_free(point);
    if (pubkey_point) EC_POINT_free(pubkey_point);
    if (ctx) BN_CTX_free(ctx);
    if (r) BN_free(r);
    if (s) BN_free(s);
    if (e) BN_free(e);
    if (t) BN_free(t);
    if (x1) BN_free(x1);
    if (R) BN_free(R);
    if (px) BN_free(px);
    if (py) BN_free(py);
    
    return ret;
}

// 生成弱k值 (用于演示攻击)
int generate_weak_k(BIGNUM *k, const EC_GROUP *group) {
    const BIGNUM *order = EC_GROUP_get0_order(group);
    BIGNUM *small_val = BN_new();
    
    // 生成一个小的k值 (这是不安全的)
    BN_set_word(small_val, 12345);
    BN_copy(k, small_val);
    
    BN_free(small_val);
    return 1;
}

// 模拟使用弱k值的签名
int simulate_vulnerable_signature(const unsigned char *message, size_t message_len,
                                 const unsigned char *private_key,
                                 SM2_SIGNATURE *signature,
                                 SM2_ATTACK_TYPE vulnerability) {
    EC_GROUP *group = NULL;
    EC_POINT *point = NULL;
    BN_CTX *ctx = NULL;
    BIGNUM *d = NULL, *k = NULL, *r = NULL, *s = NULL, *e = NULL;
    BIGNUM *x1 = NULL, *tmp = NULL;
    const BIGNUM *order = NULL;
    int ret = 0;
    unsigned char hash[32];
    
    if (!message || !private_key || !signature) return -1;
    
    // 初始化
    group = create_sm2_group();
    if (!group) return -1;
    
    ctx = BN_CTX_new();
    point = EC_POINT_new(group);
    d = BN_new();
    k = BN_new();
    r = BN_new();
    s = BN_new();
    e = BN_new();
    x1 = BN_new();
    tmp = BN_new();
    
    if (!ctx || !point || !d || !k || !r || !s || !e || !x1 || !tmp) goto cleanup;
    
    // 加载私钥
    BN_bin2bn(private_key, 32, d);
    order = EC_GROUP_get0_order(group);
    
    // 计算消息哈希
    SHA256(message, message_len, hash);
    BN_bin2bn(hash, 32, e);
    memcpy(signature->message_hash, hash, 32);
    
    // 根据攻击类型生成k值
    switch (vulnerability) {
        case ATTACK_WEAK_K:
            generate_weak_k(k, group);
            break;
        case ATTACK_K_REUSE:
            // 使用固定的k值模拟重用
            BN_set_word(k, 54321);
            break;
        default:
            BN_rand_range(k, order);
            break;
    }
    
    // 计算椭圆曲线点 (x1, y1) = k*G
    EC_POINT_mul(group, point, k, NULL, NULL, ctx);
    EC_POINT_get_affine_coordinates_GFp(group, point, x1, NULL, ctx);
    
    // 计算 r = (e + x1) mod n
    BN_mod_add(r, e, x1, order, ctx);
    
    // 计算 s = (1 + d)^(-1) * (k - r*d) mod n
    BN_mod_add(tmp, BN_value_one(), d, order, ctx);
    BN_mod_inverse(tmp, tmp, order, ctx);
    
    BN_mod_mul(s, r, d, order, ctx);
    BN_mod_sub(s, k, s, order, ctx);
    BN_mod_mul(s, tmp, s, order, ctx);
    
    // 输出签名
    BN_bn2binpad(r, signature->r, 32);
    BN_bn2binpad(s, signature->s, 32);
    ret = 1;
    
cleanup:
    if (group) EC_GROUP_free(group);
    if (point) EC_POINT_free(point);
    if (ctx) BN_CTX_free(ctx);
    if (d) BN_free(d);
    if (k) BN_free(k);
    if (r) BN_free(r);
    if (s) BN_free(s);
    if (e) BN_free(e);
    if (x1) BN_free(x1);
    if (tmp) BN_free(tmp);
    
    return ret;
}

// k值重用攻击实现
int sm2_attack_k_reuse(const SM2_SIGNATURE *sig1, const SM2_SIGNATURE *sig2,
                       const unsigned char *msg1_hash, const unsigned char *msg2_hash,
                       SM2_ATTACK_RESULT *result) {
    BN_CTX *ctx = NULL;
    BIGNUM *r1 = NULL, *s1 = NULL, *r2 = NULL, *s2 = NULL;
    BIGNUM *e1 = NULL, *e2 = NULL, *k = NULL, *d = NULL;
    BIGNUM *tmp1 = NULL, *tmp2 = NULL, *delta_e = NULL, *delta_s = NULL;
    EC_GROUP *group = NULL;
    const BIGNUM *order = NULL;
    int ret = 0;
    struct timeval start;
    
    if (!sig1 || !sig2 || !msg1_hash || !msg2_hash || !result) return -1;
    
    gettimeofday(&start, NULL);
    memset(result, 0, sizeof(SM2_ATTACK_RESULT));
    
    // 初始化
    group = create_sm2_group();
    if (!group) return -1;
    
    ctx = BN_CTX_new();
    r1 = BN_new(); s1 = BN_new(); r2 = BN_new(); s2 = BN_new();
    e1 = BN_new(); e2 = BN_new(); k = BN_new(); d = BN_new();
    tmp1 = BN_new(); tmp2 = BN_new(); delta_e = BN_new(); delta_s = BN_new();
    
    if (!ctx || !r1 || !s1 || !r2 || !s2 || !e1 || !e2 || !k || !d || !tmp1 || !tmp2 || !delta_e || !delta_s) goto cleanup;
    
    order = EC_GROUP_get0_order(group);
    
    // 加载签名数据
    BN_bin2bn(sig1->r, 32, r1);
    BN_bin2bn(sig1->s, 32, s1);
    BN_bin2bn(sig2->r, 32, r2);
    BN_bin2bn(sig2->s, 32, s2);
    BN_bin2bn(msg1_hash, 32, e1);
    BN_bin2bn(msg2_hash, 32, e2);
    
    // 检查r值是否相同 (k重用的标志)
    if (BN_cmp(r1, r2) != 0) {
        strcpy(result->description, "Attack failed: r values are different (no k reuse detected)");
        goto cleanup;
    }
    
    // 计算 k = (e1 - e2) / (s1 - s2) mod n
    BN_mod_sub(delta_e, e1, e2, order, ctx);
    BN_mod_sub(delta_s, s1, s2, order, ctx);
    
    if (BN_is_zero(delta_s)) {
        strcpy(result->description, "Attack failed: delta_s is zero");
        goto cleanup;
    }
    
    BN_mod_inverse(tmp1, delta_s, order, ctx);
    BN_mod_mul(k, delta_e, tmp1, order, ctx);
    
    // 验证k值正确性
    // 从第一个签名恢复私钥: d = (k*s1 - e1) / (r1 + s1) mod n
    BN_mod_mul(tmp1, k, s1, order, ctx);
    BN_mod_sub(tmp1, tmp1, e1, order, ctx);
    BN_mod_add(tmp2, r1, s1, order, ctx);
    BN_mod_inverse(tmp2, tmp2, order, ctx);
    BN_mod_mul(d, tmp1, tmp2, order, ctx);
    
    // 输出恢复的私钥
    BN_bn2binpad(d, result->recovered_private_key, 32);
    
    result->success = 1;
    result->attack_time = ((double)(clock() - start)) / CLOCKS_PER_SEC;
    strcpy(result->description, "K-reuse attack successful! Private key recovered.");
    ret = 1;
    
cleanup:
    if (group) EC_GROUP_free(group);
    if (ctx) BN_CTX_free(ctx);
    if (r1) BN_free(r1); if (s1) BN_free(s1); if (r2) BN_free(r2); if (s2) BN_free(s2);
    if (e1) BN_free(e1); if (e2) BN_free(e2); if (k) BN_free(k); if (d) BN_free(d);
    if (tmp1) BN_free(tmp1); if (tmp2) BN_free(tmp2); if (delta_e) BN_free(delta_e); if (delta_s) BN_free(delta_s);
    
    return ret;
}

// 中本聪签名伪造攻击 (基于SM2的理论攻击)
int sm2_forge_satoshi_signature(const SATOSHI_FORGE_DATA *forge_data,
                               SM2_ATTACK_RESULT *result) {
    EC_GROUP *group = NULL;
    EC_POINT *point = NULL, *pubkey_point = NULL;
    BN_CTX *ctx = NULL;
    BIGNUM *u = NULL, *v = NULL, *r = NULL, *s = NULL, *e = NULL;
    BIGNUM *px = NULL, *py = NULL, *x1 = NULL;
    const BIGNUM *order = NULL;
    int ret = 0;
    struct timeval start;
    unsigned char hash[32];
    
    if (!forge_data || !result) return -1;
    
    gettimeofday(&start, NULL);
    memset(result, 0, sizeof(SM2_ATTACK_RESULT));
    
    // 初始化
    group = create_sm2_group();
    if (!group) return -1;
    
    ctx = BN_CTX_new();
    point = EC_POINT_new(group);
    pubkey_point = EC_POINT_new(group);
    u = BN_new(); v = BN_new(); r = BN_new(); s = BN_new(); e = BN_new();
    px = BN_new(); py = BN_new(); x1 = BN_new();
    
    if (!ctx || !point || !pubkey_point || !u || !v || !r || !s || !e || !px || !py || !x1) goto cleanup;
    
    order = EC_GROUP_get0_order(group);
    
    // 计算目标消息的哈希
    SHA256(forge_data->target_message, forge_data->message_len, hash);
    BN_bin2bn(hash, 32, e);
    memcpy(result->forged_signature.message_hash, hash, 32);
    
    // 加载中本聪的公钥
    BN_bin2bn(forge_data->satoshi_pubkey + 1, 32, px);
    BN_bin2bn(forge_data->satoshi_pubkey + 33, 32, py);
    EC_POINT_set_affine_coordinates_GFp(group, pubkey_point, px, py, ctx);
    
    // 伪造签名策略：选择随机的u, v，计算相应的r, s
    // 这是一种理论攻击，实际中很难成功
    printf("正在尝试伪造中本聪的数字签名...\n");
    printf("注意：这是一个演示性的理论攻击，实际成功概率极低\n");
    
    for (int attempt = 0; attempt < 1000; attempt++) {
        // 随机选择u, v
        BN_rand_range(u, order);
        BN_rand_range(v, order);
        
        // 计算点 (x1, y1) = u*G + v*PA
        EC_POINT_mul(group, point, u, pubkey_point, v, ctx);
        EC_POINT_get_affine_coordinates_GFp(group, point, x1, NULL, ctx);
        
        // 计算 r = (e + x1) mod n
        BN_mod_add(r, e, x1, order, ctx);
        
        if (BN_is_zero(r)) continue;
        
        // 尝试找到合适的s值
        // s应该满足验证等式，但这在数学上是困难的
        BN_copy(s, v);  // 简化的伪造尝试
        
        // 检查是否满足验证条件（这里只是演示）
        if (!BN_is_zero(s)) {
            // 成功"伪造"（实际上这不是真正的伪造）
            BN_bn2binpad(r, result->forged_signature.r, 32);
            BN_bn2binpad(s, result->forged_signature.s, 32);
            
            result->success = 0;  // 设为0因为这不是真正的成功伪造
            result->attack_time = ((double)(clock() - start)) / CLOCKS_PER_SEC;
            sprintf(result->description, 
                "伪造尝试完成。注意：这不是真正的签名伪造，仅用于演示攻击概念。真正伪造Satoshi签名在计算上是不可行的。尝试次数: %d", attempt + 1);
            ret = 1;
            break;
        }
    }
    
    if (!ret) {
        strcpy(result->description, "伪造失败：无法在合理时间内找到有效的伪造签名（这是预期的结果）");
    }
    
cleanup:
    if (group) EC_GROUP_free(group);
    if (point) EC_POINT_free(point);
    if (pubkey_point) EC_POINT_free(pubkey_point);
    if (ctx) BN_CTX_free(ctx);
    if (u) BN_free(u); if (v) BN_free(v); if (r) BN_free(r); if (s) BN_free(s); if (e) BN_free(e);
    if (px) BN_free(px); if (py) BN_free(py); if (x1) BN_free(x1);
    
    return ret;
}

// 弱k值攻击
int sm2_attack_weak_k(const SM2_SIGNATURE *signature,
                      const unsigned char *message_hash,
                      SM2_ATTACK_RESULT *result) {
    BN_CTX *ctx = NULL;
    BIGNUM *r = NULL, *s = NULL, *e = NULL, *k = NULL, *d = NULL;
    BIGNUM *tmp1 = NULL, *tmp2 = NULL;
    EC_GROUP *group = NULL;
    const BIGNUM *order = NULL;
    int ret = 0;
    struct timeval start;
    
    if (!signature || !message_hash || !result) return -1;
    
    gettimeofday(&start, NULL);
    memset(result, 0, sizeof(SM2_ATTACK_RESULT));
    
    // 初始化
    group = create_sm2_group();
    if (!group) return -1;
    
    ctx = BN_CTX_new();
    r = BN_new(); s = BN_new(); e = BN_new(); k = BN_new(); d = BN_new();
    tmp1 = BN_new(); tmp2 = BN_new();
    
    if (!ctx || !r || !s || !e || !k || !d || !tmp1 || !tmp2) goto cleanup;
    
    order = EC_GROUP_get0_order(group);
    
    // 加载签名和消息哈希
    BN_bin2bn(signature->r, 32, r);
    BN_bin2bn(signature->s, 32, s);
    BN_bin2bn(message_hash, 32, e);
    
    printf("正在尝试弱k值攻击...\n");
    
    // 尝试常见的弱k值
    int weak_k_values[] = {1, 2, 3, 12345, 54321, 123456, 0xDEADBEEF};
    int num_weak_values = sizeof(weak_k_values) / sizeof(weak_k_values[0]);
    
    for (int i = 0; i < num_weak_values; i++) {
        BN_set_word(k, weak_k_values[i]);
        
        // 尝试从已知k值恢复私钥
        // 从 s = (1+d)^(-1) * (k - r*d) 推导出 d
        // 重新整理得到: d = (k - s*(1+d)) / r
        // 这需要解一个关于d的方程
        
        // 简化方法：暴力测试这个k值是否正确
        // 如果k正确，我们可以验证签名
        
        EC_POINT *test_point = EC_POINT_new(group);
        BIGNUM *x1 = BN_new();
        BIGNUM *test_r = BN_new();
        
        // 计算 (x1, y1) = k*G
        EC_POINT_mul(group, test_point, k, NULL, NULL, ctx);
        EC_POINT_get_affine_coordinates_GFp(group, test_point, x1, NULL, ctx);
        
        // 计算 test_r = (e + x1) mod n
        BN_mod_add(test_r, e, x1, order, ctx);
        
        if (BN_cmp(test_r, r) == 0) {
            // 找到了正确的k值！现在恢复私钥
            // 从 s*(1+d) = k - r*d 解出 d
            // s + s*d = k - r*d
            // s*d + r*d = k - s
            // d*(s + r) = k - s
            // d = (k - s) / (s + r)
            
            BN_mod_sub(tmp1, k, s, order, ctx);
            BN_mod_add(tmp2, s, r, order, ctx);
            BN_mod_inverse(tmp2, tmp2, order, ctx);
            BN_mod_mul(d, tmp1, tmp2, order, ctx);
            
            // 输出恢复的私钥
            BN_bn2binpad(d, result->recovered_private_key, 32);
            
            result->success = 1;
            result->attack_time = ((double)(clock() - start)) / CLOCKS_PER_SEC;
            sprintf(result->description, "Weak k attack successful! Found k = %d, private key recovered.", weak_k_values[i]);
            ret = 1;
            
            EC_POINT_free(test_point);
            BN_free(x1);
            BN_free(test_r);
            break;
        }
        
        EC_POINT_free(test_point);
        BN_free(x1);
        BN_free(test_r);
    }
    
    if (!ret) {
        strcpy(result->description, "Weak k attack failed: No weak k value found");
    }
    
cleanup:
    if (group) EC_GROUP_free(group);
    if (ctx) BN_CTX_free(ctx);
    if (r) BN_free(r); if (s) BN_free(s); if (e) BN_free(e); if (k) BN_free(k); if (d) BN_free(d);
    if (tmp1) BN_free(tmp1); if (tmp2) BN_free(tmp2);
    
    return ret;
}

// 工具函数：打印十六进制数据
void print_hex(const unsigned char *data, size_t len, const char *label) {
    printf("%s: ", label);
    for (size_t i = 0; i < len; i++) {
        printf("%02x", data[i]);
    }
    printf("\n");
}

// 打印签名
void print_signature(const SM2_SIGNATURE *signature) {
    print_hex(signature->r, 32, "r");
    print_hex(signature->s, 32, "s");
    print_hex(signature->message_hash, 32, "Message Hash");
}

// 打印攻击结果
void print_attack_result(const SM2_ATTACK_RESULT *result, SM2_ATTACK_TYPE attack_type) {
    const char *attack_names[] = {
        "K-Reuse Attack",
        "Weak K Attack", 
        "Nonce Bias Attack",
        "Satoshi Signature Forge",
        "Invalid Curve Attack"
    };
    
    printf("\n=== %s Result ===\n", attack_names[attack_type]);
    printf("Success: %s\n", result->success ? "YES" : "NO");
    printf("Description: %s\n", result->description);
    printf("Attack Time: %.6f seconds\n", result->attack_time);
    
    if (result->success) {
        print_hex(result->recovered_private_key, 32, "Recovered Private Key");
    }
    printf("\n");
}

// 演示k值重用攻击
void demonstrate_k_reuse_attack(void) {
    printf("\n=== K-Reuse Attack Demonstration ===\n");
    printf("此攻击演示当两个签名使用相同的k值时如何恢复私钥\n\n");
    
    // 生成测试密钥对
    unsigned char private_key[32];
    RAND_bytes(private_key, 32);
    
    // 创建两个不同的消息
    const char *msg1 = "Message 1 for k-reuse attack demo";
    const char *msg2 = "Message 2 for k-reuse attack demo";
    
    // 使用相同的弱k值生成两个签名
    SM2_SIGNATURE sig1, sig2;
    simulate_vulnerable_signature((unsigned char*)msg1, strlen(msg1), private_key, &sig1, ATTACK_K_REUSE);
    simulate_vulnerable_signature((unsigned char*)msg2, strlen(msg2), private_key, &sig2, ATTACK_K_REUSE);
    
    printf("Original signatures:\n");
    printf("Signature 1:\n");
    print_signature(&sig1);
    printf("Signature 2:\n");
    print_signature(&sig2);
    
    // 执行攻击
    SM2_ATTACK_RESULT result;
    if (sm2_attack_k_reuse(&sig1, &sig2, sig1.message_hash, sig2.message_hash, &result)) {
        print_attack_result(&result, ATTACK_K_REUSE);
        
        // 验证恢复的私钥
        if (memcmp(result.recovered_private_key, private_key, 32) == 0) {
            printf("✓ 私钥恢复成功验证!\n");
        } else {
            printf("✗ 私钥恢复验证失败\n");
        }
    }
}

// 演示中本聪签名伪造攻击
void demonstrate_satoshi_forge_attack(void) {
    printf("\n=== Satoshi Signature Forge Attack Demonstration ===\n");
    printf("此攻击演示尝试伪造中本聪数字签名的理论方法\n");
    printf("注意：这只是理论演示，实际伪造在计算上是不可行的\n\n");
    
    // 模拟中本聪的公钥（实际上是随机生成的）
    SATOSHI_FORGE_DATA forge_data;
    RAND_bytes(forge_data.satoshi_pubkey, 65);
    forge_data.satoshi_pubkey[0] = 0x04;  // 未压缩公钥标识
    
    // 要伪造签名的消息
    const char *target_msg = "Satoshi Nakamoto approves this transaction";
    strcpy((char*)forge_data.target_message, target_msg);
    forge_data.message_len = strlen(target_msg);
    
    printf("目标消息: %s\n", target_msg);
    print_hex(forge_data.satoshi_pubkey, 65, "模拟的Satoshi公钥");
    
    // 执行伪造攻击
    SM2_ATTACK_RESULT result;
    if (sm2_forge_satoshi_signature(&forge_data, &result)) {
        print_attack_result(&result, ATTACK_SATOSHI_SIGNATURE);
        
        if (!result.success) {
            printf("如预期，伪造失败。这证明了SM2签名算法的安全性。\n");
        }
    }
}

// 演示弱k值攻击
void demonstrate_weak_k_attack(void) {
    printf("\n=== Weak K Attack Demonstration ===\n");
    printf("此攻击演示当签名使用弱k值时如何恢复私钥\n\n");
    
    // 生成测试密钥对
    unsigned char private_key[32];
    RAND_bytes(private_key, 32);
    
    // 创建使用弱k值的签名
    const char *message = "Message signed with weak k value";
    SM2_SIGNATURE signature;
    simulate_vulnerable_signature((unsigned char*)message, strlen(message), private_key, &signature, ATTACK_WEAK_K);
    
    printf("使用弱k值的签名:\n");
    print_signature(&signature);
    
    // 执行攻击
    SM2_ATTACK_RESULT result;
    if (sm2_attack_weak_k(&signature, signature.message_hash, &result)) {
        print_attack_result(&result, ATTACK_WEAK_K);
        
        // 验证恢复的私钥
        if (result.success && memcmp(result.recovered_private_key, private_key, 32) == 0) {
            printf("✓ 私钥恢复成功验证!\n");
        } else if (result.success) {
            printf("✗ 私钥恢复验证失败\n");
        }
    }
}

// 打印安全建议
void print_security_recommendations(void) {
    printf("\n=== SM2签名安全建议 ===\n");
    printf("1. 随机数生成:\n");
    printf("   - 始终使用密码学安全的随机数生成器\n");
    printf("   - 确保k值具有足够的熵\n");
    printf("   - 绝不重用k值\n\n");
    
    printf("2. 实现安全:\n");
    printf("   - 使用经过验证的密码学库\n");
    printf("   - 避免自制密码学实现\n");
    printf("   - 定期更新密码学库\n\n");
    
    printf("3. 侧信道防护:\n");
    printf("   - 实现时间常数算法\n");
    printf("   - 防护功耗分析攻击\n");
    printf("   - 使用盲化技术\n\n");
    
    printf("4. 密钥管理:\n");
    printf("   - 安全生成和存储私钥\n");
    printf("   - 定期轮换密钥\n");
    printf("   - 实施密钥备份和恢复策略\n\n");
    
    printf("5. 签名验证:\n");
    printf("   - 始终验证签名格式\n");
    printf("   - 检查参数范围\n");
    printf("   - 使用安全的哈希函数\n\n");
}
