#include "gpt_tokenizer.h"

#include "gpt_vocab.h"

int gpt_encode_char(char c) {
    unsigned int v = (unsigned char)c;
    if (v < GPT_VOCAB_SIZE) {
        return (int)v;
    }
    return GPT_UNK_TOKEN;
}

char gpt_decode_token(int token) {
    if (token < 0 || token >= GPT_VOCAB_SIZE) {
        return '?';
    }
    return GPT_ID_TO_CHAR[token];
}
