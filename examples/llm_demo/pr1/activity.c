
/*
 *  POK Copyright (c) 2007-2025 POK team
 *  llama2.c runq.c (c) https://github.com/karpathy/llama2.c (MIT license)
 *  POK LLM Demo (c) 2026 Sergey Staroletov
 */

#include <core/semaphore.h>
#include <core/thread.h>
#include <core/time.h>  
#include <libc/stdio.h>
#include <libc/stdlib.h>
#include <libc/string.h> 
#include <libm.h>
#include <types.h>

//generated from .bin files of quantized model and tokenizer.bin from llama2.c
#include "model_weights.h"
#include "tokenizer_data.h"


#define DEB(MSG) printf("\n\n>>>>> %s\n\n", MSG); while(1) {}

int GS = 0;

// ----------------------------------------------------------------------------
// from runq.c 

typedef struct {
    int dim;
    int hidden_dim;
    int n_layers;
    int n_heads;
    int n_kv_heads;
    int vocab_size;
    int seq_len;
} Config;

typedef struct {
    int8_t* q;
    float* s;
} QuantizedTensor;

typedef struct {
    QuantizedTensor *q_tokens;
    //float* token_embedding_table;
    float* rms_att_weight;
    float* rms_ffn_weight;
    QuantizedTensor *wq;
    QuantizedTensor *wk;
    QuantizedTensor *wv;
    QuantizedTensor *wo;
    QuantizedTensor *w1;
    QuantizedTensor *w2;
    QuantizedTensor *w3;
    float* rms_final_weight;
    QuantizedTensor *wcls;
} TransformerWeights;

typedef struct {
    float *x;
    float *xb;
    float *xb2;
    float *hb;
    float *hb2;
    QuantizedTensor xq;
    QuantizedTensor hq;
    float *q;
    float *k;
    float *v;
    float *att;
    float *logits;
    float* key_cache;
    float* value_cache;
} RunState;

typedef struct {
    Config config;
    TransformerWeights weights;
    RunState state;
    // new deadline fields
    uint64_t deadline_us;      // 0 = no deadline
    int deadline_exceeded;     
} Transformer;


void* pok_malloc(size_t size) {
    return malloc(size);
}

void* pok_calloc(size_t nmemb, size_t size) {
    void* ptr = pok_malloc(nmemb * size);
    if (ptr) memset(ptr, 0, nmemb * size);
    return ptr;
}

void pok_free(void* ptr) {
    free(ptr);
}


void swap(void *a, void *b, size_t size) {
    char temp[size];
    memcpy(temp, a, size);
    memcpy(a, b, size);
    memcpy(b, temp, size);
}

void qsort(void *base, size_t nmemb, size_t size, 
              int (*compar)(const void *, const void *)) {
    if (nmemb < 2) return;
    char *base_ptr = (char *)base;
    void *pivot = base_ptr + (nmemb / 2) * size;
    size_t i = 0;
    size_t j = nmemb - 1;
    while (i <= j) {
        while (compar(base_ptr + i * size, pivot) < 0) i++;
        while (compar(base_ptr + j * size, pivot) > 0) j--;
        if (i <= j) {
            swap(base_ptr + i * size, base_ptr + j * size, size);
            if (pivot == base_ptr + i * size) pivot = base_ptr + j * size;
            else if (pivot == base_ptr + j * size) pivot = base_ptr + i * size;
            i++;
            if (j > 0) j--;
        }
    }
    if (j + 1 > 1) qsort(base_ptr, j + 1, size, compar);
    if (nmemb - i > 1) qsort(base_ptr + i * size, nmemb - i, size, compar);
}

void *bsearch(const void *key, const void *base, size_t nmemb, size_t size,
                 int (*compar)(const void *, const void *)) {
    const char *base_ptr = (const char *)base;
    size_t low = 0;
    size_t high = nmemb;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        const void *mid_ptr = base_ptr + (mid * size);
        int res = compar(key, mid_ptr);
        if (res == 0) {
            return (void *)mid_ptr; 
        } else if (res < 0) {
            high = mid;     
        } else {
            low = mid + 1; 
        }
    }
    return NULL; 
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest;
    while (*ptr != '\0') {
        ptr++;
    }
    while (*src != '\0') {
        *ptr = *src;
        ptr++;
        src++;
    }
    *ptr = '\0';
    return dest;
}



// ----------------------------------------------------------------------------
//  RunState 

void malloc_run_state(RunState* s, Config* p) {
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    s->x = pok_calloc(p->dim, sizeof(float));
    s->xb = pok_calloc(p->dim, sizeof(float));
    s->xb2 = pok_calloc(p->dim, sizeof(float));
    s->hb = pok_calloc(p->hidden_dim, sizeof(float));
    s->hb2 = pok_calloc(p->hidden_dim, sizeof(float));
    s->xq.q = pok_calloc(p->dim, sizeof(int8_t));
    s->xq.s = pok_calloc(p->dim, sizeof(float));
    s->hq.q = pok_calloc(p->hidden_dim, sizeof(int8_t));
    s->hq.s = pok_calloc(p->hidden_dim, sizeof(float));
    s->q = pok_calloc(p->dim, sizeof(float));
    s->k = pok_calloc(kv_dim, sizeof(float));
    s->v = pok_calloc(kv_dim, sizeof(float));
    s->att = pok_calloc(p->n_heads * p->seq_len, sizeof(float));
    s->logits = pok_calloc(p->vocab_size, sizeof(float));
    s->key_cache = pok_calloc(p->n_layers * p->seq_len * kv_dim, sizeof(float));
    s->value_cache = pok_calloc(p->n_layers * p->seq_len * kv_dim, sizeof(float));
    if (!s->x || !s->xb || !s->xb2 || !s->hb || !s->hb2 || !s->q
     || !s->k || !s->v || !s->att || !s->logits || !s->key_cache
     || !s->value_cache || !s->xq.q || !s->xq.s || !s->hq.q || !s->hq.s) {
        DEB("runq_pok: malloc_run_state failed!\n");
    }
}

void free_run_state(RunState* s) {
    pok_free(s->x); pok_free(s->xb); pok_free(s->xb2);
    pok_free(s->hb); pok_free(s->hb2);
    pok_free(s->xq.q); pok_free(s->xq.s);
    pok_free(s->hq.q); pok_free(s->hq.s);
    pok_free(s->q); pok_free(s->k); pok_free(s->v);
    pok_free(s->att); pok_free(s->logits);
    pok_free(s->key_cache); pok_free(s->value_cache);
}

// ----------------------------------------------------------------------------
// Quant 

void dequantize(QuantizedTensor *qx, float* x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = qx->q[i] * qx->s[i / GS];
    }
}

void quantize(QuantizedTensor *qx, float* x, int n) {
    int num_groups = n / GS;
    float Q_MAX = 127.0f;
    for (int group = 0; group < num_groups; group++) {
        float wmax = 0.0;
        for (int i = 0; i < GS; i++) {
            float val = fabs(x[group * GS + i]);
            if (val > wmax) wmax = val;
        }
        float scale = wmax / Q_MAX;
        qx->s[group] = scale;
        for (int i = 0; i < GS; i++) {
            float quant_value = x[group * GS + i] / scale;
            int8_t quantized = (int8_t) round(quant_value);
            qx->q[group * GS + i] = quantized;
        }
    }
}

QuantizedTensor *init_quantized_tensors(void **ptr, int n, int size_each) {
    void *p = *ptr;
    QuantizedTensor *res = pok_malloc(n * sizeof(QuantizedTensor));
    for(int i=0; i<n; i++) {
        res[i].q = (int8_t*)p;
        p = (int8_t*)p + size_each;
        res[i].s = (float*)p;
        p = (float*)p + size_each / GS;
    }
    *ptr = p;
    return res;
}


static void get_token_embedding(TransformerWeights* w, int vocab_size, int dim,
                                int token, float* out) {
    QuantizedTensor* qt = w->q_tokens;
    int offset = token * dim;
    for (int i = 0; i < dim; i++) {
        out[i] = (float)(qt->q[offset + i]) * qt->s[(offset + i) / GS];
    }
}

void memory_map_weights(TransformerWeights *w, Config* p, void* ptr, uint8_t shared_classifier) {
    int head_size = p->dim / p->n_heads;
    float* fptr = (float*) ptr;
    w->rms_att_weight = fptr;
    fptr += p->n_layers * p->dim;
    w->rms_ffn_weight = fptr;
    fptr += p->n_layers * p->dim;
    w->rms_final_weight = fptr;
    fptr += p->dim;

    ptr = (void*)fptr;
    w->q_tokens = init_quantized_tensors(&ptr, 1, p->vocab_size * p->dim);

    w->wq = init_quantized_tensors(&ptr, p->n_layers, p->dim * (p->n_heads * head_size));
    w->wk = init_quantized_tensors(&ptr, p->n_layers, p->dim * (p->n_kv_heads * head_size));
    w->wv = init_quantized_tensors(&ptr, p->n_layers, p->dim * (p->n_kv_heads * head_size));
    w->wo = init_quantized_tensors(&ptr, p->n_layers, (p->n_heads * head_size) * p->dim);

    w->w1 = init_quantized_tensors(&ptr, p->n_layers, p->dim * p->hidden_dim);
    w->w2 = init_quantized_tensors(&ptr, p->n_layers, p->hidden_dim * p->dim);
    w->w3 = init_quantized_tensors(&ptr, p->n_layers, p->dim * p->hidden_dim);

    w->wcls = shared_classifier ? w->q_tokens : init_quantized_tensors(&ptr, 1, p->dim * p->vocab_size);
}



void build_transformer_from_memory(Transformer *t, const void *model_data, size_t model_size) {
    // header as in .bin
    const uint8_t *ptr = (const uint8_t *)model_data;

    uint32_t magic = *(uint32_t*)ptr; ptr += 4;
    if (magic != 0x616b3432) {
        DEB("runq_pok: bad magic number\n");
    }
    int version = *(int*)ptr; ptr += 4;
    if (version != 2) {
        DEB("runq_pok: bad version\n");
    }

    memcpy(&t->config, ptr, sizeof(Config)); ptr += sizeof(Config);
    uint8_t shared_classifier = *ptr; ptr += 1;
    int group_size = *(int*)ptr; ptr += 4;
    GS = group_size;

    ptr = (const uint8_t *)model_data;
    ptr += 256;

    // ptr now points to weights
    memory_map_weights(&t->weights, &t->config, (void*)ptr, shared_classifier);
    malloc_run_state(&t->state, &t->config);

    t->deadline_us = 0;
    t->deadline_exceeded = 0;
}


void free_transformer(Transformer* t) {
    pok_free(t->weights.q_tokens);
    pok_free(t->weights.wq);
    pok_free(t->weights.wk);
    pok_free(t->weights.wv);
    pok_free(t->weights.wo);
    pok_free(t->weights.w1);
    pok_free(t->weights.w2);
    pok_free(t->weights.w3);
    if(t->weights.wcls != t->weights.q_tokens) pok_free(t->weights.wcls);
    free_run_state(&t->state);
}

// ----------------------------------------------------------------------------

void rmsnorm(float* o, float* x, float* weight, int size) {
    float ss = 0.0f;
    for (int j = 0; j < size; j++) ss += x[j] * x[j];
    ss /= size; ss += 1e-5f; ss = 1.0f / sqrtf(ss);
    for (int j = 0; j < size; j++) o[j] = weight[j] * (ss * x[j]);
}

void softmax(float* x, int size) {
    float max_val = x[0];
    for (int i = 1; i < size; i++) if (x[i] > max_val) max_val = x[i];
    float sum = 0.0f;
    for (int i = 0; i < size; i++) { x[i] = expf(x[i] - max_val); sum += x[i]; }
    for (int i = 0; i < size; i++) x[i] /= sum;
}

void matmul(float* xout, QuantizedTensor *x, QuantizedTensor *w, int n, int d) {
    int i;
    for (i = 0; i < d; i++) {
        float val = 0.0f;
        int32_t ival = 0;
        int in = i * n;
        int j;
        for (j = 0; j <= n - GS; j += GS) {
            for (int k = 0; k < GS; k++) {
                ival += ((int32_t) x->q[j + k]) * ((int32_t) w->q[in + j + k]);
            }
            val += ((float) ival) * w->s[(in + j) / GS] * x->s[j / GS];
            ival = 0;
        }
        xout[i] = val;
    }
}

// ----------------------------------------------------------------------------

float* forward(Transformer* transformer, int token, int pos) {
    Config* p = &transformer->config;
    TransformerWeights* w = &transformer->weights;
    RunState* s = &transformer->state;
    float *x = s->x;
    int dim = p->dim;
    int kv_dim = (p->dim * p->n_kv_heads) / p->n_heads;
    int kv_mul = p->n_heads / p->n_kv_heads;
    int hidden_dim = p->hidden_dim;
    int head_size = dim / p->n_heads;

    //memcpy(x, w->token_embedding_table + token*dim, dim * sizeof(float));
    get_token_embedding(w, p->vocab_size, dim, token, x);//fix

    for(int l = 0; l < p->n_layers; l++) {
        // attention rmsnorm
        rmsnorm(s->xb, x, w->rms_att_weight + l*dim, dim);

        // qkv matmuls
        quantize(&s->xq, s->xb, dim);
        matmul(s->q, &s->xq, w->wq + l, dim, dim);
        matmul(s->k, &s->xq, w->wk + l, dim, kv_dim);
        matmul(s->v, &s->xq, w->wv + l, dim, kv_dim);

        // RoPE
        for (int i = 0; i < dim; i+=2) {
            int head_dim = i % head_size;
            float freq = 1.0f / powf(10000.0f, head_dim / (float)head_size);
            float val = pos * freq;
            float fcr = cosf(val);
            float fci = sinf(val);
            int rotn = i < kv_dim ? 2 : 1;
            for (int v = 0; v < rotn; v++) {
                float* vec = v == 0 ? s->q : s->k;
                float v0 = vec[i];
                float v1 = vec[i+1];
                vec[i]   = v0 * fcr - v1 * fci;
                vec[i+1] = v0 * fci + v1 * fcr;
            }
        }

        // KV cache
        int loff = l * p->seq_len * kv_dim;
        float* key_cache_row = s->key_cache + loff + pos * kv_dim;
        float* value_cache_row = s->value_cache + loff + pos * kv_dim;
        memcpy(key_cache_row, s->k, kv_dim * sizeof(float));
        memcpy(value_cache_row, s->v, kv_dim * sizeof(float));

        // multihead attention
        int h;
        for (h = 0; h < p->n_heads; h++) {
            float* q = s->q + h * head_size;
            float* att = s->att + h * p->seq_len;
            for (int t = 0; t <= pos; t++) {
                float* k = s->key_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                float score = 0.0f;
                for (int i = 0; i < head_size; i++) score += q[i] * k[i];
                score /= sqrtf(head_size);
                att[t] = score;
            }
            softmax(att, pos + 1);
            float* xb = s->xb + h * head_size;
            memset(xb, 0, head_size * sizeof(float));
            for (int t = 0; t <= pos; t++) {
                float* v = s->value_cache + loff + t * kv_dim + (h / kv_mul) * head_size;
                float a = att[t];
                for (int i = 0; i < head_size; i++) xb[i] += a * v[i];
            }
        }

        // result of attention
        quantize(&s->xq, s->xb, dim);
        matmul(s->xb2, &s->xq, w->wo + l, dim, dim);
        for (int i = 0; i < dim; i++) x[i] += s->xb2[i];

        // ffn rmsnorm
        rmsnorm(s->xb, x, w->rms_ffn_weight + l*dim, dim);
        quantize(&s->xq, s->xb, dim);
        matmul(s->hb, &s->xq, w->w1 + l, dim, hidden_dim);
        matmul(s->hb2, &s->xq, w->w3 + l, dim, hidden_dim);

        // SwiGLU
        for (int i = 0; i < hidden_dim; i++) {
            float val = s->hb[i];
            val *= (1.0f / (1.0f + expf(-val)));
            val *= s->hb2[i];
            s->hb[i] = val;
        }

        quantize(&s->hq, s->hb, hidden_dim);
        matmul(s->xb, &s->hq, w->w2 + l, hidden_dim, dim);
        for (int i = 0; i < dim; i++) x[i] += s->xb[i];

        // check deadline after layer
        pok_time_t ns;
        pok_time_get(&ns); 
        if (transformer->deadline_us > 0 && ns >= transformer->deadline_us) {
            transformer->deadline_exceeded = 1;
            return s->logits;
        }
    }

    // final rmsnorm
    rmsnorm(x, x, w->rms_final_weight, dim);
    quantize(&s->xq, x, dim);
    matmul(s->logits, &s->xq, w->wcls, dim, p->vocab_size);
    return s->logits;
}

// ----------------------------------------------------------------------------

typedef struct {
    char *str;
    int id;
} TokenIndex;

typedef struct {
    char** vocab;
    float* vocab_scores;
    TokenIndex *sorted_vocab;
    int vocab_size;
    unsigned int max_token_length;
    unsigned char byte_pieces[512];
} Tokenizer;

void build_tokenizer_from_memory(Tokenizer* t, const void* data, int vocab_size) {
    const unsigned char* ptr = (const unsigned char*)data;

    t->vocab_size = vocab_size;
    t->vocab = (char**)malloc(vocab_size * sizeof(char*));
    t->vocab_scores = (float*)malloc(vocab_size * sizeof(float));
    t->sorted_vocab = NULL; 

    for (int i = 0; i < 256; i++) {
        t->byte_pieces[i * 2] = (unsigned char)i;
        t->byte_pieces[i * 2 + 1] = '\0';
    }

    //  max_token_length (int, 4 байта)
    t->max_token_length = *(int*)ptr;
    ptr += sizeof(int);

    // each token info
    for (int i = 0; i < vocab_size; i++) {
        // score (float, 4 bytes)
        t->vocab_scores[i] = *(float*)ptr;
        ptr += sizeof(float);

        // (int, 4 bytes)
        int len = *(int*)ptr;
        ptr += sizeof(int);

        // string (len bytes)
        t->vocab[i] = (char*)malloc(len + 1);
        memcpy(t->vocab[i], ptr, len);
        t->vocab[i][len] = '\0';
        ptr += len;
    }
}


int compare_tokens(const void *a, const void *b) {
    return strcmp(((TokenIndex*)a)->str, ((TokenIndex*)b)->str);
}


int str_lookup(char *str, TokenIndex *sorted_vocab, int vocab_size) {
    // efficiently find the perfect match for str in vocab, return its index or -1 if not found
    TokenIndex tok = { .str = str }; // acts as the key to search for
    TokenIndex *res = bsearch(&tok, sorted_vocab, vocab_size, sizeof(TokenIndex), compare_tokens);
    return res != NULL ? res->id : -1;
}

void encode(Tokenizer* t, char *text, int8_t bos, int8_t eos, int *tokens, int *n_tokens) {
    // encode the string text (input) into an upper-bound preallocated tokens[] array
    // bos != 0 means prepend the BOS token (=1), eos != 0 means append the EOS token (=2)
    if (text == NULL) { 
      printf("cannot encode NULL text\n"); 
      return;
    }

    if (t->sorted_vocab == NULL) {
        // lazily malloc and sort the vocabulary
        t->sorted_vocab = malloc(t->vocab_size * sizeof(TokenIndex));
        for (int i = 0; i < t->vocab_size; i++) {
            t->sorted_vocab[i].str = t->vocab[i];
            t->sorted_vocab[i].id = i;
        }
        qsort(t->sorted_vocab, t->vocab_size, sizeof(TokenIndex), compare_tokens);
    }

    // create a temporary buffer that will store merge candidates of always two consecutive tokens
    // *2 for concat, +1 for null terminator +2 for UTF8 (in case max_token_length is 1)
    char* str_buffer = malloc((t->max_token_length*2 +1 +2) * sizeof(char));
    size_t str_len = 0;

    // start at 0 tokens
    *n_tokens = 0;

    // add optional BOS (=1) token, if desired
    if (bos) tokens[(*n_tokens)++] = 1;

    // add_dummy_prefix is true by default
    // so prepend a dummy prefix token to the input string, but only if text != ""
    // TODO: pretty sure this isn't correct in the general case but I don't have the
    // energy to read more of the sentencepiece code to figure out what it's doing
    if (text[0] != '\0') {
        int dummy_prefix = str_lookup(" ", t->sorted_vocab, t->vocab_size);
        tokens[(*n_tokens)++] = dummy_prefix;
    }

    // process the raw (UTF-8) byte sequence of the input string
    for (char *c = text; *c != '\0'; c++) {

        if ((*c & 0xC0) != 0x80) {
            // this byte must be either a leading byte (11...) or an ASCII char (0x...)
            // => reset our location, as we're starting a new UTF-8 codepoint
            str_len = 0;
        }

        // append the current byte to the buffer
        str_buffer[str_len++] = *c; // ++ is post-increment, incremented after this line
        str_buffer[str_len] = '\0';

        // while the next character is a continuation byte, continue appending
        // but if there are too many of them, just stop to avoid overruning str_buffer size.
        if ((*(c+1) & 0xC0) == 0x80 && str_len < 4) {
            continue;
        }

        // ok c+1 is not a continuation byte, so we've read in a full codepoint
        int id = str_lookup(str_buffer, t->sorted_vocab, t->vocab_size);

        if (id != -1) {
            // we found this codepoint in vocab, add it as a token
            tokens[(*n_tokens)++] = id;
        } else {
            // byte_fallback encoding: just encode each byte as a token
            // +3 is here because the first 3 vocab elements are <unk>, <s>, </s>
            // so the individual bytes only start at index 3
            for (int i=0; i < str_len; i++) {
                tokens[(*n_tokens)++] = (unsigned char)str_buffer[i] + 3;
            }
        }
        str_len = 0; // protect against a sequence of stray UTF8 continuation bytes
    }

    // merge the best consecutive pair each iteration, according the scores in vocab_scores
    while (1) {
        float best_score = -1e10;
        int best_id = -1;
        int best_idx = -1;

        for (int i = 0; i < (*n_tokens - 1); i++) {
          char *s1 = t->vocab[tokens[i]];
          char *s2 = t->vocab[tokens[i+1]];
          char *dst = str_buffer;
          while (*s1) *dst++ = *s1++;
          while (*s2) *dst++ = *s2++;
          *dst = '\0';

          int id = str_lookup(str_buffer, t->sorted_vocab, t->vocab_size);
          if (id != -1 && t->vocab_scores[id] > best_score) {
              best_score = t->vocab_scores[id];
              best_id = id;
              best_idx = i;
          }
        }

        if (best_idx == -1) {
            break; // we couldn't find any more pairs to merge, so we're done
        }

        // merge the consecutive pair (best_idx, best_idx+1) into new token best_id
        tokens[best_idx] = best_id;
        // delete token at position best_idx+1, shift the entire sequence back 1
        for (int i = best_idx+1; i < (*n_tokens-1); i++) {
            tokens[i] = tokens[i+1];
        }
        (*n_tokens)--; // token length decreased
    }

    // add optional EOS (=2) token, if desired
    if (eos) tokens[(*n_tokens)++] = 2;

    free(str_buffer);
}



char* decode(Tokenizer* t, int prev_token, int token) {
    char *piece = t->vocab[token];
    // following BOS (1) token, sentencepiece decoder strips any leading whitespace
    if (prev_token == 1 && piece[0] == ' ') {
        piece++;
    }

    //token starts from "<0x", then 2 hex-symbol and '>',
    if (piece[0] == '<' && piece[1] == '0' && piece[2] == 'x') {
        char c1 = piece[3];
        char c2 = piece[4];
        if (piece[5] == '>' && piece[6] == '\0') {
            unsigned char byte_val = 0;
            if (c1 >= '0' && c1 <= '9') byte_val = (c1 - '0') << 4;
            else if (c1 >= 'A' && c1 <= 'F') byte_val = (c1 - 'A' + 10) << 4;
            else if (c1 >= 'a' && c1 <= 'f') byte_val = (c1 - 'a' + 10) << 4;
            else goto not_byte;

            if (c2 >= '0' && c2 <= '9') byte_val |= (c2 - '0');
            else if (c2 >= 'A' && c2 <= 'F') byte_val |= (c2 - 'A' + 10);
            else if (c2 >= 'a' && c2 <= 'f') byte_val |= (c2 - 'a' + 10);
            else goto not_byte;

            return (char*)t->byte_pieces + byte_val * 2;
        }
    }
not_byte: //:)
    return piece;
}


// ----------------------------------------------------------------------------

typedef struct {
    float prob;
    int index;
} ProbIndex;

typedef struct {
    int vocab_size;
    ProbIndex* probindex;
    float temperature;
    float topp;
    unsigned long long rng_state;
} Sampler;

int sample_argmax(float* probabilities, int n) {
    int max_i = 0;
    float max_p = probabilities[0];
    for (int i = 1; i < n; i++) if (probabilities[i] > max_p) { max_i = i; max_p = probabilities[i]; }
    return max_i;
}

int sample_mult(float* probabilities, int n, float coin) {
    float cdf = 0.0f;
    for (int i = 0; i < n; i++) { cdf += probabilities[i]; if (coin < cdf) return i; }
    return n - 1;
}

int compare_probindex(const void* a, const void* b) {
    ProbIndex* a_ = (ProbIndex*) a;
    ProbIndex* b_ = (ProbIndex*) b;
    if (a_->prob > b_->prob) return -1;
    if (a_->prob < b_->prob) return 1;
    return 0;
}

int sample_topp(float* probabilities, int n, float topp, ProbIndex* probindex, float coin) {
    int n0 = 0;
    const float cutoff = (1.0f - topp) / (n - 1);
    for (int i = 0; i < n; i++) {
        if (probabilities[i] >= cutoff) {
            probindex[n0].index = i;
            probindex[n0].prob = probabilities[i];
            n0++;
        }
    }
    qsort(probindex, n0, sizeof(ProbIndex), compare_probindex);
    float cumulative_prob = 0.0f;
    int last_idx = n0 - 1;
    for (int i = 0; i < n0; i++) {
        cumulative_prob += probindex[i].prob;
        if (cumulative_prob > topp) { last_idx = i; break; }
    }
    float r = coin * cumulative_prob;
    float cdf = 0.0f;
    for (int i = 0; i <= last_idx; i++) {
        cdf += probindex[i].prob;
        if (r < cdf) return probindex[i].index;
    }
    return probindex[last_idx].index;
}

void build_sampler(Sampler* sampler, int vocab_size, float temperature, float topp, unsigned long long rng_seed) {
    sampler->vocab_size = vocab_size;
    sampler->temperature = temperature;
    sampler->topp = topp;
    sampler->rng_state = rng_seed;
    sampler->probindex = pok_malloc(sampler->vocab_size * sizeof(ProbIndex));
}

void free_sampler(Sampler* sampler) { pok_free(sampler->probindex); }

unsigned int random_u32(unsigned long long *state) {
    *state ^= *state >> 12;
    *state ^= *state << 25;
    *state ^= *state >> 27;
    return (*state * 0x2545F4914F6CDD1Dull) >> 32;
}
float random_f32(unsigned long long *state) {
    return (random_u32(state) >> 8) / 16777216.0f;
}

int sample(Sampler* sampler, float* logits) {
    int next;
    if (sampler->temperature == 0.0f) {
        next = sample_argmax(logits, sampler->vocab_size);
    } else {
        for (int q=0; q<sampler->vocab_size; q++) logits[q] /= sampler->temperature;
        softmax(logits, sampler->vocab_size);
        float coin = random_f32(&sampler->rng_state);
        if (sampler->topp <= 0 || sampler->topp >= 1) {
            next = sample_mult(logits, sampler->vocab_size, coin);
        } else {
            next = sample_topp(logits, sampler->vocab_size, sampler->topp, sampler->probindex, coin);
        }
    }
    return next;
}

// ----------------------------------------------------------------------------
// Generation 

void generate(Transformer *transformer, Tokenizer *tokenizer, Sampler *sampler,
              char *prompt, int steps) {
    if (prompt == NULL) prompt = "";

    int num_prompt_tokens = 0;
    int* prompt_tokens = (int*)pok_malloc((strlen(prompt)+3) * sizeof(int));
    encode(tokenizer, prompt, 1, 0, prompt_tokens, &num_prompt_tokens); // assume encode exists

    int next;
    int token = prompt_tokens[0];
    int pos = 0;

    transformer->deadline_exceeded = 0;

    while (pos < steps) {
        float* logits = forward(transformer, token, pos);

        if (transformer->deadline_exceeded) {
            // need to stop the phrase 
            if (pos > 0 && token != '.' && token != '!' && token != '?') {
                int dot_token = str_lookup(".", tokenizer->sorted_vocab, tokenizer->vocab_size);
                if (dot_token != -1) {
                    char* piece = decode(tokenizer, token, dot_token);
                    printf("[%s]", piece);
                }
            }

            printf("\n[TRUNCATED at %d tokens due to deadline]\n", pos);
            break;
        }

        if (pos < num_prompt_tokens - 1) {
            next = prompt_tokens[pos + 1];
        } else {
            next = sample(sampler, logits);
        }
        pos++;
        if (next == 1) break; // BOS

        char* piece = decode(tokenizer, token, next);
        printf("%s", piece);
        token = next;
    }
    printf("\n");
    pok_free(prompt_tokens);
}

// ----------------------------------------------------------------------------
void set_deadline(Transformer* t, uint64_t deadline_us) {
    t->deadline_us = deadline_us;
}

static Transformer g_transformer;
static Tokenizer g_tokenizer;
static Sampler g_sampler;
static char g_prompt_buffer[256];
static char g_response_buffer[256];

// Demo variables
extern uint8_t sem_raw_empty;    
extern uint8_t sem_raw_full;     
extern uint8_t sem_est_empty;    
extern uint8_t sem_est_full;

static int temp;
static int humidity;
static int pressure;
static int wind_speed;
#define MAXSTR 15

static char temp_val[MAXSTR];
static char humidity_val[MAXSTR];
static char wind_val[MAXSTR];
static char pressure_val[MAXSTR];

//#define MYDELAY 10000
#define MYDELAY 8333 //my mac m4 qemu ~time

#define SEC(X) (X*MYDELAY)
#define NSEC(X) (X*MYDELAY*1000)



void *llm_job() {
 
    printf("[LLM init] Building transformer...");
    build_transformer_from_memory(&g_transformer, model_weights, model_weights_size);
    printf("done!\n");

    printf("[LLM init] Building transformer...");
    build_tokenizer_from_memory(&g_tokenizer, tokenizer_data, g_transformer.config.vocab_size);
    printf("done!\n");

    printf("[LLM init] Building sampler...");
    build_sampler(&g_sampler, g_transformer.config.vocab_size, 0.9f, 0.9f, 42);
    printf(" init done!\n");
  
    pok_time_t ns, ns1;

    while (1) {

        pok_sem_wait(sem_est_full, 0);   

        pok_time_get(&ns);
        g_transformer.deadline_us = ns + NSEC(10); 
        g_transformer.deadline_exceeded = 0;

        g_prompt_buffer[0] = 0;
        strcat(g_prompt_buffer, "Temperature is ");
        strcat(g_prompt_buffer, temp_val);
        strcat(g_prompt_buffer, " and pressure is ");
        strcat(g_prompt_buffer, pressure_val);
        strcat(g_prompt_buffer, " and wind is ");
        strcat(g_prompt_buffer, wind_val);
        strcat(g_prompt_buffer, " and humidity is ");
        strcat(g_prompt_buffer, humidity_val);
        strcat(g_prompt_buffer, ". What to do?");
              
        printf("[LLM] generation: \n");
        generate(&g_transformer, &g_tokenizer, &g_sampler, g_prompt_buffer, 256);

        pok_time_get(&ns1);
        printf("[LLM] time for generate: %lld\n", (ns1 - ns));
        pok_sem_signal(sem_est_empty);   // ok 

        pok_thread_sleep(SEC(5));
    }
  return (0);
}



//Exponential Moving Average (EMA)
float EMA(float start, float *current_ema, int i, int max) {
    float smoothing = 0.3;    
    float base_temp = start; 
    float noise = ((float)rand() / RAND_MAX * 4.0) - 2.0;
    float sine_wave = sin(i * 2.0 * 3.1415 / max) * 5.0;
    float raw_val = base_temp + sine_wave + noise;
    if (i == 0) {
        *current_ema = raw_val;
    } else {
        *current_ema = (raw_val - *current_ema) * smoothing + *current_ema;
    }    
    return raw_val;
}


void *sensor_job() {
    float ema_t = 0;
    float ema_h = 0;
    float ema_w = 0;
    float ema_p = 0;
    
    int i = 0;
    int max = 60;

    while (1) {
      pok_sem_wait(sem_raw_empty, 0);   // wait

      printf("\n");
      temp = EMA(20, &ema_t, i, max);
      printf("[Sensor] get temperature %d C\n", temp);
      humidity = EMA(50, &ema_h, i, max);
      if (humidity < 0) humidity = 0;
      if (humidity > 100) humidity = 100;
      printf("[Sensor] get humidity %d %%\n", humidity);
      pressure = EMA(730, &ema_p, i, max);
      printf("[Sensor] get pressure %d mm\n", pressure);
      wind_speed = EMA(5, &ema_w, i, max);
      if (wind_speed < 0) wind_speed = 0;
      printf("[Sensor] get wind speed %d m/s\n", wind_speed);
      printf("\n");
      
      pok_sem_signal(sem_raw_full);    // ok

      i++;
      if (i > max) i = 0;
      pok_thread_sleep(SEC(1));
    }
  return (0);
}


void *estimator_job() {
    while (1) {
      pok_sem_wait(sem_raw_full, 0);   // wait from sensor

      strcpy(temp_val, "unknown"); 
      if (temp < -10) strcpy(temp_val, "chilly"); 
      else
        if (temp < 0) strcpy(temp_val, "cold snap"); 
          else
            if (temp < 10) strcpy(temp_val, "cold"); 
              else
                if (temp > 10 && temp <= 20) strcpy(temp_val, "fine"); 
                  else
                    if (temp > 20 && temp < 30) strcpy(temp_val, "hot"); 
                      else
                       strcpy(temp_val, "scorching");
      
      strcpy(pressure_val, "unknown"); 
      if (pressure >= 732 && pressure <= 752) strcpy(pressure_val, "comfortable"); 
        else
          if (pressure < 732) strcpy(pressure_val, "insufficient"); 
            else
              if (pressure > 752) strcpy(pressure_val, "pressing");

      strcpy(humidity_val, "unknown"); 
      if (humidity >= 40 && humidity <= 60) strcpy(humidity_val, "comfortable"); 
        else
          if (humidity < 40) strcpy(humidity_val, "dry"); 
            else
              if (humidity > 60) strcpy(humidity_val, "wet");

      strcpy(wind_val, "unknown");
      if (wind_speed < 2) strcpy(wind_val, "calm"); 
          else 
            if (wind_speed < 6) strcpy(wind_val, "breezy"); 
              else 
                if (wind_speed < 9) strcpy(wind_val, "moderate"); 
                    else 
                      if (wind_speed < 12) strcpy(wind_val, "windy"); 
                        else 
                          if (wind_speed < 18) strcpy(wind_val, "strong"); 
                            else 
                              strcpy(wind_val, "gale");        


        printf("\n[Estimator] classified temp=%d pressure=%d humidity=%d wind_speed=%d \n\n", temp, pressure, humidity, wind_speed);
        pok_sem_signal(sem_raw_empty);   // ok, sensor 

        pok_sem_wait(sem_est_empty, 0);  // wait LLM

        pok_sem_signal(sem_est_full);    // ok, LLM  

      pok_thread_sleep(SEC(1));
    }

}