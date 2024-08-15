/*
 * Copyright 2022, QNX Software Systems.
 * Copyright 2022, Texas Instruments Incorporated - http://www.ti.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef OMXIL_QUEUE_H_
#define OMXIL_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *name;
    uint16_t readIndex;
    uint16_t writeIndex;
    uint16_t maxIndex;
    void** item;
} qfifo_t;

qfifo_t *CreateQueue( uint32_t nbuf, const char *name );
bool QueueCopy( qfifo_t *dst, qfifo_t *src );
uint32_t QueueGetSize( qfifo_t *q );
bool QueueEmpty( qfifo_t *q );
bool QueueFull( qfifo_t *q );
bool QueuePut( qfifo_t *q, void *item );
void *QueueGet( qfifo_t *q);
void *QueuePeek( qfifo_t *q);
void DestroyQueue( qfifo_t *bufHdrQ );

#ifdef __cplusplus
}
#endif

#endif // OMXIL_QUEUE_H_




