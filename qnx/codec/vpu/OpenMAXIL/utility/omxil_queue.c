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

#include <stdlib.h> // calloc
#include <inttypes.h>
#include <stdbool.h>

#include "log.h" // LOG
#include "omxil_queue.h"

qfifo_t *CreateQueue( uint32_t nbuf, const char *name )
{
    qfifo_t *pfifo = calloc(1, sizeof(qfifo_t));
    if( pfifo ) {
        pfifo->maxIndex = nbuf + 1;
        pfifo->item = (void **)calloc( pfifo->maxIndex, sizeof(void *) );
        if( pfifo->item == NULL ) {
            LOG( LOG_ERROR, "CreateQueue memory failure for item (%zu)",
                    pfifo->maxIndex * sizeof(void *));
            free(pfifo);
            return NULL;
        }

        pfifo->name = name;
    }
    return pfifo;
}

bool QueueCopy( qfifo_t *dst, qfifo_t *src )
{
    void *it;
    while( ( it = QueueGet( src ) ) ) {
        if( !QueuePut( dst, it ) ) {
            return false;
        }
    }
    return true;
}

inline uint32_t QueueGetSize( qfifo_t *q )
{
    return (q->maxIndex - 1);
}

inline bool QueueEmpty( qfifo_t *q )
{
    return (q->readIndex == q->writeIndex);
}

inline bool QueueFull( qfifo_t *q )
{
    return ( ( ( q->writeIndex + 1 ) % q->maxIndex ) == q->readIndex );
}

bool QueuePut( qfifo_t *q, void *item )
{
    bool result = false;

    //check if buf header queue is full
    if( QueueFull( q ) ) {
        LOG( LOG_WARNING, "QueuePut(%s) queue is full(writeIndex %d,readIndex %d)",
                               q->name, q->writeIndex, q->readIndex);
        return result;
    }

    q->item[q->writeIndex] = item;
    q->writeIndex = (q->writeIndex + 1) % q->maxIndex;
    result = true;

    return result;
}

void *QueueGet( qfifo_t *q)
{
    void *ritem = NULL;

    //check if buf header queue is empty
    if( QueueEmpty( q ) ) {
        return NULL;
    }

    ritem = q->item[q->readIndex];
    q->item[q->readIndex] = NULL;
    q->readIndex = ( q->readIndex + 1 ) % q->maxIndex;

    return ritem;
}

void *QueuePeek( qfifo_t *q)
{
    //check if buf header queue is empty
    if( QueueEmpty( q ) ) {
        return NULL;
    }

    return q->item[q->readIndex];
}


void DestroyQueue( qfifo_t *q )
{
    free( q->item);
    free( q );
}

