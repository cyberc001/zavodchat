#include <emscripten/emscripten.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rnnoise.h"

DenoiseState *denoise;
size_t frame_ln;
float* frame_buf;

struct buffer {
	float* mem;
	size_t mem_ln;
	// end points to first free bucket after data
	size_t begin, end;
} buf_in, buf_out;


void buf_init(struct buffer* buf, size_t mem_ln)
{
	buf->mem_ln = mem_ln;
	buf->mem = malloc(mem_ln * sizeof(float));
	buf->begin = buf->end = 0;
}
void buf_adv_begin(struct buffer* buf, size_t amt)
{
	buf->begin += amt;
	if(buf->begin == buf->mem_ln)
		buf->begin = 0;
}
void buf_adv_end(struct buffer* buf, size_t amt)
{
	buf->end += amt;
	if(buf->end == buf->mem_ln)
		buf->end = 0;
}

size_t buf_size(struct buffer* buf)
{
	return buf->end >= buf->begin ? buf->end - buf->begin
		: buf->mem_ln - buf->begin + buf->end;
}
void buf_print(struct buffer* buf)
{
	printf("buffer b=%lu e=%lu size=%lu data= ", buf->begin, buf->end, buf_size(buf));
	for(size_t i = 0; i < buf->mem_ln; ++i)
		printf("%f ", buf->mem[i]);
	printf("\n");
}

void buf_push(struct buffer* buf, const float* in, size_t in_ln)
{
	if(buf->mem_ln - buf_size(buf) - 1 < in_ln){
		printf("ERROR: buffer overflow, in_ln=%lu\n", in_ln);
		buf_print(buf);
		return;
	}

	if(buf->end > buf->begin){
		size_t rem = buf->mem_ln - buf->end;
		if(in_ln > rem){
			memcpy(buf->mem + buf->end, in, rem * sizeof(float));
			in_ln -= rem;
			in += rem;
			buf->end = 0;
		} else {
			memcpy(buf->mem + buf->end, in, in_ln * sizeof(float));
			buf_adv_end(buf, in_ln);
			return;
		}
	}

	// buf->end <= buf->begin
	memcpy(buf->mem + buf->end, in, in_ln * sizeof(float));
	buf_adv_end(buf, in_ln);
}
void buf_pop(struct buffer* buf, float* out, size_t out_ln)
{
	if(buf_size(buf) < out_ln){
		printf("ERROR: buffer underflow, out_ln=%lu\n", out_ln);
		buf_print(buf);
		return;
	}

	if(buf->begin > buf->end){
		size_t rem = buf->mem_ln - buf->begin;
		if(out_ln > rem){
			memcpy(out, buf->mem + buf->begin, rem * sizeof(float));
			out_ln -= rem;
			out += rem;
			buf->begin = 0;
		} else {
			memcpy(out, buf->mem + buf->begin, out_ln * sizeof(float));
			buf_adv_begin(buf, out_ln);
			return;
		}
	}

	// buf->begin <= buf->end
	memcpy(out, buf->mem + buf->begin, out_ln * sizeof(float));
	buf_adv_begin(buf, out_ln);
}

int main()
{
	printf("creating denoise state\n");
	denoise = rnnoise_create(NULL);
	frame_ln = rnnoise_get_frame_size();
	frame_buf = malloc(sizeof(float) * frame_ln);
	printf("frame ln %lu\n", frame_ln);

	buf_init(&buf_in, frame_ln * 2);
	buf_init(&buf_out, frame_ln * 2);	

	return 0;
}

// Assume that always (data_ln < frame_ln)
EMSCRIPTEN_KEEPALIVE void process(float* data, size_t data_ln)
{
	//printf("process() begin\n");
	//buf_print(&buf_in);

	buf_push(&buf_in, data, data_ln);
	while(buf_size(&buf_in) >= frame_ln){
		buf_pop(&buf_in, frame_buf, frame_ln);
		//rnnoise_process_frame(denoise, frame_buf, frame_buf);
		buf_push(&buf_out, frame_buf, frame_ln);
	}

	if(buf_size(&buf_out) >= data_ln)
		buf_pop(&buf_out, data, data_ln);

	//printf("process() end\n");
	return;
}
