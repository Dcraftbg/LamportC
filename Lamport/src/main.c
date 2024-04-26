#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "fileutils.h"
typedef struct String {
    char* buf;
    size_t len;
    size_t cap;
} String;

void die(const char* msg) {
    fprintf(stderr, "%s", msg);
    exit(1);
}
void String_reserve(String* t, size_t extra) {
    void* bo = t->buf;
    size_t ncap;
    if (t->len + extra > t->cap) {
        ncap = t->len * 2 + extra;
        t->buf = realloc(t->buf, ncap);
        if (!t->buf) {
            free(bo);
            die("Ran out of memory");
        }    
        t->cap = ncap;
    }
}
void String_push(String* t, char c) {
    String_reserve(t, 1);
    t->buf[t->len] = c;
    t->len += 1;
}
void String_extend(String* t, char* other, size_t len) {
    String_reserve(t, len);
    memcpy(t->buf + t->len, other, len);
    t->len += len;
}
void String_drop(String* t) {
    free(t->buf);
    t->buf = NULL;
    t->cap = 0;
    t->len = 0;
}

typedef struct u256 {
   uint64_t is[4];
} u256;
bool u256_neq(u256 a, u256 b) {
    return a.is[0] != b.is[0] ||
           a.is[1] != b.is[1] ||
           a.is[2] != b.is[2] ||
           a.is[3] != b.is[3];
}
typedef struct {
   u256 pairs[2][256];
} Key;
typedef struct {
   u256 data[256];
} Signature;
void Signature_drop(Signature* sig) {
    free(sig);
}
void log_base64(FILE* sink, uint8_t* bytes, size_t len);
// NOTE: I don't care that this is completely inaccurate
uint64_t rand_u64(void) {
   return (uint64_t)rand() << 32 ^ (uint64_t)rand() << 31 ^ (uint64_t)rand();
}
u256 rand_u256(void) {
   u256 v;
   for(size_t i = 0; i < 4; ++i) {
        v.is[i] = rand_u64();
   }
   return v;
}
Key* key_new(void) {
    void* res = malloc(sizeof(Key));
    assert(res && "Ran out of memory");
    return res;
}
Key* key_gen_private(void) {
   Key* k = key_new();
   for(size_t p = 0; p < 2; ++p) {
        for(size_t i = 0; i < 256; ++i) {
           k->pairs[p][i] = rand_u256();
        } 
   }
   return k;
}
char* encode_base64(uint8_t* bytes, size_t len);
bool key_dump_file(const char* path, Key* key) {
   String s = { 0 };
   char* buf = NULL;
   int e = 0;
   if(!path || !key) return false;
   buf = encode_base64((uint8_t*) & key->pairs[0], sizeof(key->pairs[0]));
   String_extend(&s, buf, strlen(buf));
   free(buf);
   String_push(&s, '\n');
   buf = encode_base64((uint8_t*) & key->pairs[1], sizeof(key->pairs[1]));
   String_extend(&s, buf, strlen(buf));
   free(buf);
   if (write_whole_file(path, s.buf, s.len)) {
       String_drop(&s);
       return false;
   }
   String_drop(&s);
   return true;
}
void key_drop(Key* key) {
   free(key);
}
const char* base64_lookup = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
//                                      buf[4]
void encode_u24_base64(uint32_t v, size_t s, char* buf) {
    size_t at = 0;
    while(s--) {
        buf[at] = base64_lookup[(v & 0xFE0000) >> 18];
        v = (v << 6)&0xFFFFFF;
        at++;
    }
    for(;at<4;++at) {
        buf[at] = '=';
    }
}
char* encode_base64(uint8_t* bytes, size_t len) {
    size_t i = 0;
    size_t bl = ((len+2) / 3) * 4;
    size_t at = 0;
    char* obuf = malloc(bl+1);
    char* head = obuf;
    uint32_t buf = 0;
    char cbuf[4];
    assert(obuf && "Ran out of memory");
    obuf[bl] = '\0';
    for (i = 0; i < len; i += 3, head += 4) {
        buf = 0;
        for (at = 0; i + at < len && at < 3; ++at) {
            ((uint8_t*)&buf)[2 - at] = bytes[i + at];
        }
        encode_u24_base64(buf, at + 1, (char*)&cbuf);
        memcpy(head, cbuf, 4);
    }
    return obuf;
}
void log_base64(FILE* sink, uint8_t* bytes, size_t len) {
    size_t i=0;
    uint32_t buf=0;
    char cbuf[4];
    size_t at = 0;
    for(i=0;i<len; i+=3) {
        buf = 0;
        for(at=0;i + at < len && at < 3;++at) {
           ((uint8_t*)&buf)[2-at] = bytes[i+at];
        }
        encode_u24_base64(buf,at+1,(char*)&cbuf);
        fwrite(cbuf,1,4,sink);
    }
    fputc('\n',sink);
}
#define EDEC_NOT_ENOUGH_MEM 1
#define EDEC_INVALID_PADDING 2
#define EDEC_INVALID_DIGIT 3
#define EDEC_UNEXPECTED_CONT 4
int decode_base64_char(char c) {
    return
        c >= 'A' && c <= 'Z' ? c - 'A' :
        c >= 'a' && c <= 'z' ? c - 'a' + 26 :
        c >= '0' && c <= '9' ? c - '0' + 52 :
        c == '+' ? 62 :
        c == '/' ? 63 :
        -1;
}
#include <string.h>
int decode_base64(char* code, size_t len, uint8_t* res, size_t res_cap) {
    uint32_t view_dec;
    size_t req_mem;
    uint32_t v;
    int n;
    char* _v;
    size_t rest;
    if (len % 4 != 0) return EDEC_INVALID_PADDING;

    req_mem = (len + 3) / 4 * 3; 
    _v = code + len - 4;
    // Save up a bit of required memory for the check. This is dumb but yeah. 
    while (_v < code + len && *_v != '=') _v++; 
    rest = ((size_t)_v - (size_t)(code + len - 3));
    req_mem -= 3-((rest * 3) / 4);
    if (res_cap < req_mem)
        return EDEC_NOT_ENOUGH_MEM;
    while (len-4) {
        v = 0;
        for (size_t i = 0; i < 4; ++i) {
            if ((n = decode_base64_char(code[i])) < 0) 
                return EDEC_INVALID_DIGIT;
            v = (v << 6) | n;
        }
        // ----------------------
        char c = ((char*)&v)[0];
        ((char*)&v)[0] = ((char*)&v)[2];
        ((char*)&v)[2] = c;
        // Swapping the bytes in a 24 bit integer^

        memcpy(res, &v, 3); // TODO: I think its safe to say that this is 3
        code += 4;
        len  -= 4;
        res += 3;
        res_cap -= 3;
    }
    v = 0;
    for (size_t i = 0; i < 4; ++i) {
        n = 0;
        if (code[i] == '=') {
            while (code[i] == '=' && i < 4) {
                v = (v << 6);
                i++;
            }
            if (i != 4) return EDEC_UNEXPECTED_CONT;
            break;
        }
        else {
            if ((n = decode_base64_char(code[i])) < 0)
                return EDEC_INVALID_DIGIT;
        }
        v = (v << 6) | n;
    }
    // ----------------------
    char c = ((char*)&v)[0];
    ((char*)&v)[0] = ((char*)&v)[2];
    ((char*)&v)[2] = c;
    // Swapping the bytes in a 24 bit integer^
    memcpy(res, &v, res_cap > 3 ? 3 : res_cap);
    return 0;
#if 0
    size_t resto = res_cap % 3;
    size_t res_cap_round = (res_cap + 2) / 3;
    size_t clen = 0;
    char* chunk = NULL;
    int ccv = 0;
    char tmp = 0;
    // Trim the '=' sign on the right and assert its equal to resto
    while (len > 0 && code[len] == '=' && 3 - resto) { len--; resto++; }
    if(3-resto != 3) 
        return EDEC_INVALID_PADDING;
    clen = len / 4;
    if (clen > res_cap_round) return EDEC_NOT_ENOUGH_MEM;
    for (size_t i = 0; i < clen; ++i, res += 3) {
        chunk = (char*)((uint32_t*)code + i);
        uint32_t v=0;
        for (size_t j = 0; j < 4; ++j) {
            if (i+1 == clen && j > resto) {
                if (chunk[j] != '=') 
                    return EDEC_INVALID_DIGIT;
                continue;
            }
            else if (chunk[j] == '=') {
                __debugbreak();
                return EDEC_INVALID_DIGIT;
            }
            ccv = decode_base64_char(chunk[j]);
            if (ccv < 0) 
                return EDEC_INVALID_DIGIT;
            v = (v << 6) | ccv;
            // v = (v >> 6) | (ccv << 14);
        }
        tmp = ((char*)&v)[2];
        ((char*)&v)[2] = ((char*)&v)[0];
        ((char*)&v)[0] = tmp;
        memcpy(res, ((char*)&v), 3);
    }
    return 0;
#endif
}
#include <Windows.h>
static int test_base64_tf(void) {
    size_t size = 0;
    char* lorem = read_whole_file("lorem.txt", &size);
    ULONGLONG startT, elapsedT;
    char* encode;
    size_t encode_len;
    char* buftmp;
    int e = 0;
    if (!lorem) {
        fprintf(stderr, "[ERROR] Failed to load lorem.txt");
        return 1;
    }
    startT = GetTickCount64();
    encode = encode_base64(lorem, size);
    encode_len = (size / 3) * 4;
    elapsedT = GetTickCount64() - startT;
    buftmp = malloc(size);
    assert(buftmp);
    if (e = decode_base64(encode, encode_len, buftmp, size)) {
        fprintf(stderr, "[ERROR] Failed to decode encoding %d\n", e);
        return 1;
    }
    if (e = memcmp(lorem, buftmp, size)) {
        fprintf(stderr, "[ERROR] Test failed. Decoding and Encoding is incorrect %d\n", e);
        return 1;
    }
    free(buftmp);
    free(encode);
    free(lorem);
    return 0;
}

const char* strtrimr(const char* a) {
    char* head = a;
    char c = 0;
    while (*head++);
    head--;
    do {
        (c = *head--);
    } while (head > a && isspace(c));
    return head;
}
Key* key_load(const char* path) {
    size_t size = 0;
    Key* key = NULL;
    char* buf = read_whole_file(path, &size);
    char* head = buf;
    size_t len = 0;
    size_t len2 = 0;
    int e = 0;
    if (!buf) {
        fprintf(stderr, "[ERROR] Failed to load key %s from file system\n",path);
        return NULL;
    }
    key = key_new();
    if (!(head = strchr(buf, '\n'))) {
        fprintf(stderr, "[ERROR] Key %s has invalid syntax\n", path);
        goto DEFER_ERR;
    }
    len  = (size_t)head - (size_t)buf;
    head += 1;
    len2 = (size_t)strtrimr(head)-(size_t)head + 1;
    if (e=decode_base64(buf, len, (uint8_t*)&key->pairs[0], sizeof(key->pairs[0]))) {
        fprintf(stderr, "[ERROR] Failed to decode key pair 0. Decoding error %d\n",e);
        fprintf(stderr, "[INFO] Key: %.*s\n", (int)len, buf);
        goto DEFER_ERR;
    }
    if (e = decode_base64(head, len2, (uint8_t*)&key->pairs[1], sizeof(key->pairs[1]))) {
        fprintf(stderr, "[ERROR] Failed to decode key pair 1. Decoding error %d\n", e);
        goto DEFER_ERR;
    }
DEFER:
    free(buf);
    return key;
DEFER_ERR:
    if(key) key_drop(key);
    key = NULL;
    goto DEFER;
}

#include <sha256.h>
Key* Key_make_public(Key* priv) {
   SHA256_CTX ctx;
   Key* k = key_new();
   size_t j, i;
   for (j = 0; j < 2; ++j) {
       for (i = 0; i < 256; ++i) {
           sha256_init(&ctx);
           sha256_update(&ctx, (BYTE*)&priv->pairs[j][i], sizeof(u256));
           sha256_final(&ctx, (BYTE*)&k->pairs[j][i]);
       }
   }
   return k;
}
int regen_pub_priv(void) {
    Key* key = key_load("priv.key");
    if (!key) {
        return 1;
    }
    printf("[INFO] Successfully loaded key!\n");
    Key* pub = Key_make_public(key);
    if (!key_dump_file("pub.key", pub)) {
        fprintf(stderr, "[ERROR] Failed to generate pub.key\n");
        return 1;
    }
    printf("[INFO] Successfully generated public key!\n");
    key_drop(pub);
    key_drop(key);
    return 0;
}

Signature* Signature_new() {
    Signature* v = malloc(sizeof(Signature));
    assert(v && "Ran out of memory");
    return v;
}
Signature* sign_message(Signature* sig, const char* msg, Key* pub) {
    SHA256_CTX ctx = { 0 };
    u256 hash = { 0 };
    size_t i, j;
    sha256_init(&ctx);
    sha256_update(&ctx, msg, strlen(msg));
    sha256_final(&ctx, (BYTE*)&hash);
    for (i = 0; i < 4; ++i) {
        uint64_t v = hash.is[i];
        for (j = 0; j < sizeof(v) * 8; ++j) {
            bool bit = (v & ((uint64_t)1 << j)) >> j;
            sig->data[i*64+j] = pub->pairs[bit][i*64];
        }
    }
    return sig;
}
bool Signature_validate(Signature* sig, Key* key, const char* msg) {
    SHA256_CTX ctx = { 0 };
    u256 hash = { 0 };
    size_t i, j;
    sha256_init(&ctx);
    sha256_update(&ctx, msg, strlen(msg));
    sha256_final(&ctx, (BYTE*)&hash);
    for (i = 0; i < 4; ++i) {
        uint64_t v = hash.is[i];
        for (j = 0; j < sizeof(v) * 8; ++j) {
            bool bit = (v & ((uint64_t)1 << j)) >> j;
            if (u256_neq(sig->data[i*64+j], key->pairs[bit][i * 64])) {   
                return false;
            }
        }
    }
    return true;
}

int main(void) {
    srand(time(NULL));
    const char* kp = "pub.key";
    Key* key = NULL;
    if (!file_exists(kp)) {
        Key* k = key_gen_private();
        key = Key_make_public(k);
        key_drop(k);
        if (!key_dump_file(kp, key)) { // Simple caching for the key
            fprintf(stderr, "[ERROR] Failed to generate %s\n", kp);
            return 1;
        }
        printf("[INFO] Generated %s (cached)\n", kp);
    }
    else {
        key = key_load(kp);
        if (!key) {
            fprintf(stderr, "[ERROR] Failed to load public key %s!\n",kp);
            return 1;
        }
    }
    const char* msg1 = "Hello, I completely agree with this statement: I like pancakes";
    const char* msg2 = "Hello, I completely agree with this statement: I DON'T like pancakes";
    Signature* sig = Signature_new();
    sign_message(sig, msg1, key);
    printf("[INFO] Signed message \"%s\"!\n",msg1);
    printf("[INFO] Did we sign \"%s\"?\n", msg1);
    printf("[INFO] Signature was %s\n", Signature_validate(sig, key, msg1) ? "VALID" : "INVALID");
    printf("[INFO] Did we sign \"%s\"?\n", msg2);
    printf("[INFO] Signature was %s\n", Signature_validate(sig, key, msg2) ? "VALID" : "INVALID");
    
    Signature_drop(sig);
    key_drop(key);
    return 0;
} 

