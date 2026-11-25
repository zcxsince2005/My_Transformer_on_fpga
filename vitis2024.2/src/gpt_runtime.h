#ifndef GPT_RUNTIME_H
#define GPT_RUNTIME_H

int gpt_generate(const char *prompt, unsigned int max_new_tokens, char *out_text, unsigned int out_cap);

#endif
