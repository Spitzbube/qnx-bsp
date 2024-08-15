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

#ifndef _OMXIL_CORE_H_
#define _OMXIL_CORE_H_

#define OMX_MAX_CMP_NUM        16
#define OMX_CORE_MAX_CMP_ROLES 8
#define OMX_COMP_MAX_INST      16

typedef OMX_ERRORTYPE (*qnx_omx_component_init)(OMX_HANDLETYPE hComponent);

typedef struct _omx_components_info
{
  char*                         name;// Component name
  qnx_omx_component_init        fn_ptr;// create instance fn ptr
  void*                         inst[OMX_COMP_MAX_INST];// Instance handle
  void*                so_lib_handle;// So Library handle
  const char* roles[OMX_CORE_MAX_CMP_ROLES];// roles played
}omx_component_info;

typedef OMX_ERRORTYPE (*omx_get_component_info)(omx_component_info *pInfo);


#endif //_OMXIL_CORE_H_


