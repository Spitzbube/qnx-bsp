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

#ifndef _OMXIL_LIST_H_
#define _OMXIL_LIST_H_

    typedef struct _BufferList BufferList;


    /*
     * The main structure for buffer management.
     *
     *   pBufHdr     - An array of pointers to buffer headers with valid data.
     *   pAllocHdr   - An array of pointers to all allocated buffer headers.
     *                 The size of the array is set dynamically using the nBufferCountActual value
     *                   send by the client.
     *   pShmBufs    - An array of pointers storing the physical address of the buffers
                         allocated and stored in the corresponding pAllocHdr indices.
     *   nListEnd    - Marker to the boundary of the array. This points to the last index of the
     *                   pBufHdr array.
     *   nSizeOfList - Count of valid data in the list.
     *   nAllocSize  - Size of the allocated list. This is equal to (nListEnd + 1) in most of
     *                   the times. When the list is freed this is decremented and at that
     *                   time the value is not equal to (nListEnd + 1). This is because
     *                   the list is not freed from the end and hence we cannot decrement
     *                   nListEnd each time we free an element in the list. When nAllocSize is zero,
     *                   the list is completely freed and the other paramaters of the list are
     *                   initialized.
     *                 If the client crashes before freeing up the buffers, this parameter is
     *                   checked (for nonzero value) to see if there are still elements on the list.
     *                   If yes, then the remaining elements are freed.
     *    nWritePos  - The position where the next buffer would be written. The value is incremented
     *                   after the write. It is wrapped around when it is greater than nListEnd.
     *    nReadPos   - The position from where the next buffer would be read. The value is incremented
     *                   after the read. It is wrapped around when it is greater than nListEnd.
     *    eDir       - Type of BufferList.
     *                            OMX_DirInput  =  Input  Buffer List
     *                           OMX_DirOutput  =  Output Buffer List
     */
    struct _BufferList{
        OMX_BUFFERHEADERTYPE **pBufHdr;
        OMX_BUFFERHEADERTYPE **pAllocHdr;
        void **pShmBufs;
        OMX_U32 nListEnd;
        OMX_U32 nSizeOfList;
        OMX_U32 nAllocSize;
        OMX_U32 nWritePos;
        OMX_U32 nReadPos;
        OMX_BOOL bAllocated;
        OMX_DIRTYPE eDir;
    };


    /*
     * Allocates a new entry in a BufferList.
     * Finds the position where memory has to be allocated.
     * Actual allocation happens in the caller function.
     */
#define ListAllocate(_pH, _nIndex)              \
    if (_pH.nListEnd == -1){                     \
        _pH.nListEnd = 0;                         \
        _pH.nWritePos = 0;                        \
    }                                         \
    else                                         \
        _pH.nListEnd++;                              \
    _pH.nAllocSize++;                            \
    _nIndex = _pH.nListEnd




    /*
     * Sets an entry in the BufferList.
     * The entry set is a BufferHeader.
     * The nWritePos value is incremented after the write.
     * It is wrapped around when it is greater than nListEnd.
     */
#define ListSetEntry(_pH, _pB)                  \
    if (_pH.nSizeOfList < (_pH.nListEnd + 1)){   \
        _pH.nSizeOfList++;                        \
        _pH.pBufHdr[_pH.nWritePos++] = _pB;       \
        if (_pH.nReadPos == -1)                   \
            _pH.nReadPos = 0;                      \
        if (_pH.nWritePos > _pH.nListEnd)         \
            _pH.nWritePos = 0;                     \
    }


    /*
     * Gets an entry from the BufferList
     * The entry is a BufferHeader
     * The nReadPos value is not incremented after the read.
     */
#define ListPeekEntry(_pH, _pB)                  \
    if (_pH.nSizeOfList > 0){                    \
        _pB = _pH.pBufHdr[_pH.nReadPos];        \
    }

    /*
     * Increment nReadPos
     */
#define ListFlushEntry(_pH)                      \
    if (_pH.nSizeOfList > 0){                    \
        _pH.nSizeOfList--;                       \
        _pH.nReadPos++;                          \
        if (_pH.nReadPos > _pH.nListEnd)         \
            _pH.nReadPos = 0;                    \
    }


    /*
     * Gets an entry from the BufferList
     * The entry is a BufferHeader
     * The nReadPos value is incremented after the read.
     * It is wrapped around when it is greater than nListEnd.
     */
#define ListGetEntry(_pH, _pB)                  \
    if (_pH.nSizeOfList > 0){                    \
        _pH.nSizeOfList--;                        \
        _pB = _pH.pBufHdr[_pH.nReadPos++];        \
        if (_pH.nReadPos > _pH.nListEnd)          \
            _pH.nReadPos = 0;                      \
    }


    /*
     * Flushes all entries from the BufferList structure.
     * The nSizeOfList gives the number of valid entries in the list.
     * The nReadPos value is incremented after the read.
     * It is wrapped around when it is greater than nListEnd.
     */
#define ListFlushEntries(_pH, _pC)              \
    while (_pH.nSizeOfList > 0){                \
        _pH.nSizeOfList--;                       \
        if (_pH.eDir == OMX_DirInput)            \
            _pC->pCallbacks->EmptyBufferDone(_pC->hSelf,_pC->pAppData,_pH.pBufHdr[_pH.nReadPos++]);\
        else if (_pH.eDir == OMX_DirOutput)      \
            _pC->pCallbacks->FillBufferDone(_pC->hSelf,_pC->pAppData,_pH.pBufHdr[_pH.nReadPos++]);\
        if (_pH.nReadPos > _pH.nListEnd)         \
            _pH.nReadPos = 0;                     \
    }


    /*
     * Frees the memory allocated for BufferList entries
     *   by comparing with client supplied buffer header.
     * The nAllocSize value gives the number of allocated (i.e. not free'd) entries in the list.
     * When nAllocSize is zero, the list is completely freed
     *   and the other paramaters of the list are initialized.
     */
#define ListFreeBuffer(_pH, _pB, _pP, _nIndex)                       \
    for (_nIndex = 0; _nIndex <= _pH.nListEnd; _nIndex++){           \
        if (_pH.pAllocHdr[_nIndex] == _pB){                            \
            _pH.nAllocSize--;                                         \
            if (_pH.pAllocHdr[_nIndex]){                                \
                if (_pH.pAllocHdr[_nIndex]->pBuffer && _pH.bAllocated)                     \
                    munmap(_pH.pAllocHdr[_nIndex]->pBuffer, _pH.pAllocHdr[_nIndex]->nAllocLen);               \
                _pH.pAllocHdr[_nIndex]->pBuffer = NULL;               \
                OMX_BUFFERHEADERTYPE *bufhdr = (OMX_BUFFERHEADERTYPE *)_pH.pAllocHdr[_nIndex]; \
                free(bufhdr);                                 \
                _pH.pAllocHdr[_nIndex] = NULL;                           \
            }                                                      \
            if (_pH.nAllocSize == 0){                                 \
                _pH.nWritePos = -1;                                    \
                _pH.nReadPos = -1;                                     \
                _pH.nListEnd = -1;                                     \
                _pH.nSizeOfList = 0;                                   \
                _pP->bPopulated = OMX_FALSE;                           \
            }                                                      \
            break;                                                    \
        }                                                         \
    }


    /*
     * Frees the memory allocated for BufferList entries.
     * This is called in case the client crashes suddenly before freeing all the component buffers.
     * The nAllocSize parameter is
     *   checked (for nonzero value) to see if there are still elements on the list.
     * If yes, then the remaining elements are freed.
     */
#define ListFreeAllBuffers(_pH, _nIndex)                             \
    for (_nIndex = 0; _nIndex <= _pH.nListEnd; _nIndex++){           \
        if (_pH.pAllocHdr[_nIndex]){                                   \
            _pH.nAllocSize--;                                         \
            if (_pH.pAllocHdr[_nIndex]->pBuffer)                        \
                _pH.pAllocHdr[_nIndex]->pBuffer = NULL;                  \
            OMX_BUFFERHEADERTYPE *bufhdr = (OMX_BUFFERHEADERTYPE *)_pH.pAllocHdr[_nIndex]; \
            free(bufhdr);                                    \
            _pH.pAllocHdr[_nIndex] = NULL;                              \
            if (_pH.nAllocSize == 0){                                 \
                _pH.nWritePos = -1;                                    \
                _pH.nReadPos = -1;                                     \
                _pH.nListEnd = -1;                                     \
                _pH.nSizeOfList = 0;                                   \
            }                                                      \
        }                                                         \
    }



    /*
     * Loads the parameters of the buffer header.
     * When the list has nBufferCountActual elements allocated
     *   then the bPopulated value of port definition is set to true.
     */
#define LoadBufferHeader(_pList, _pBufHdr, _pAppPrivate, _nSizeBytes, _nPortIndex,    \
        _ppBufHdr, _pPortDef)     \
    _pBufHdr->nAllocLen = _nSizeBytes;                                                \
    _pBufHdr->pAppPrivate = _pAppPrivate;                                             \
    if (_pList.eDir == OMX_DirInput){                                                 \
        _pBufHdr->nInputPortIndex = _nPortIndex;                                       \
        _pBufHdr->nOutputPortIndex = OMX_NOPORT;                                       \
    }                                                                              \
    else{                                                                             \
        _pBufHdr->nInputPortIndex = OMX_NOPORT;                                        \
        _pBufHdr->nOutputPortIndex = _nPortIndex;                                      \
    }                                                                              \
    _ppBufHdr = _pBufHdr;                                                             \
    if (_pList.nListEnd == (_pPortDef->nBufferCountActual - 1))                       \
        _pPortDef->bPopulated = OMX_TRUE;


#endif //_OMXIL_LIST_H_



