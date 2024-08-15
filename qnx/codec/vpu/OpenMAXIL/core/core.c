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

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <errno.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include "log.h"
#include "core.h"

/*
 * It is set to 1 after OMX_Init is called
 */
static int initialized = 0;

static omx_component_info core[OMX_MAX_CMP_NUM];
static int size_of_core = 0;
static const char *DEFAULT_LIB_PATH = "/lib/dll/omxil";

/*
 * search name in component table for index
 */
static int get_cmp_index(char *cmp_name)
{
    int rc = -1;
    int i=0;

    LOG(LOG_DEBUG2, "In %s for %s", __func__, cmp_name);
    for(i=0; i< size_of_core; i++)
    {
        LOG(LOG_DEBUG2, "%s: cmp_name = %s , core[i].name = %s ,count = %d",__func__, cmp_name, core[i].name, i);

        if(!strcmp(cmp_name, core[i].name))
        {
            rc = i;
            break;
        }
    }
    LOG(LOG_DEBUG2, "%s returning index %d",__func__, rc);
    return rc;
}

/*
 * Gets the index to store the next handle for specified component name
 */
static int get_comp_handle_index(char *cmp_name)
{
    unsigned i=0,j=0;
    int rc = -1;
    for(i=0; i< size_of_core; i++)
    {
        if(!strcmp(cmp_name, core[i].name))
        {
            for(j=0; j< OMX_COMP_MAX_INST; j++)
            {
                if(NULL == core[i].inst[j])
                {
                    rc = j;
                    LOG(LOG_DEBUG2,"free handle slot exists %d", rc);
                    return rc;
                }
            }
            break;
        }
    }
    return rc;
}

/*
 * Check if the component handle already exists or not
 */
static int is_cmp_handle_exists(OMX_HANDLETYPE inst)
{
    unsigned i=0,j=0;
    int rc = -1;

    if(NULL == inst)
        return rc;

    for(i=0; i< size_of_core; i++)
    {
        for(j=0; j< OMX_COMP_MAX_INST; j++)
        {
            if(inst == core[i].inst[j])
            {
                rc = i;
                return rc;
            }
        }
    }
    return rc;
}

/*
 * Load components from $OMXIL_COMPONENT_PATH(default /usr/lib/omxil)
 */
static OMX_ERRORTYPE load_comp_libs()
{
    const char *lib_dir;
    DIR *dir;
    struct dirent *ent;
    OMX_ERRORTYPE ret = OMX_ErrorUndefined;
    void *lib_hdl = NULL;
    omx_get_component_info fn_get_info = NULL;
    const char *libcomp_prefix = "omxil_comp";
#if defined (DEBUG_MODE)
    const char *libcomp_suffix = "_g.so";
#else
    const char *libcomp_suffix = ".so";
#endif
    char fullpath[120];
    int  namelen, dirlen;

    if((lib_dir = getenv( "OMXIL_COMPONENT_PATH" )) == NULL )
    {
        lib_dir = DEFAULT_LIB_PATH;
    }
    LOG(LOG_DEBUG2, "Loading the library from path %s", lib_dir );

    if ((dir = opendir(lib_dir)) != NULL)
    {
        while ((ent = readdir(dir)) != NULL)
        {
            LOG(LOG_DEBUG2, "Loading the library %s", ent->d_name);
            //validate library name
            namelen = strlen(ent->d_name);
            if( (strncmp(libcomp_prefix, ent->d_name, strlen(libcomp_prefix))) == 0 &&
#if defined (DEBUG_MODE)
                    (strncmp(libcomp_suffix, &(ent->d_name[namelen - 5]), 5)) == 0)
#else
                    (strncmp(libcomp_suffix, &(ent->d_name[namelen - 3]), 3)) == 0)
#endif
            {

                dirlen = strlen(lib_dir);
                if(dirlen + namelen + 1 > 120)
                {
                    LOG(LOG_ERROR, "Error: path name too long %d", dirlen);
                    break;
                }

                memset(fullpath, 0, 120);
                memcpy(fullpath, lib_dir, dirlen);
                fullpath[dirlen] = '/';
                memcpy(fullpath + dirlen + 1, ent->d_name, namelen);
                LOG(LOG_DEBUG1, "Loading the library %s", ent->d_name);
                if ((lib_hdl = dlopen(fullpath, RTLD_NOW)) == NULL)
                  LOG(LOG_ERROR, "dlopen of '%s' failed err='%s'", fullpath, strerror(errno));
                else
                {
                    fn_get_info = dlsym(lib_hdl, "QCompGetInfo");
                    if(fn_get_info != NULL)
                    {
                        core[size_of_core].so_lib_handle = lib_hdl;
                        ret = (*(fn_get_info))(&(core[size_of_core++]));
                        LOG(LOG_DEBUG2, "Get list of components(%d)", size_of_core);
                    }
                }
            }
        }
        closedir (dir);
    }
    else
    {
        LOG(LOG_ERROR, "Could load the omxil components err='%s' put them in '%s' or set the OMXIL_COMPONENT_PATH environment variable", strerror(errno), lib_dir);
        ret = OMX_ErrorNotReady;
    }

    return ret;
}

/*
 * DeInit component
 */
static OMX_ERRORTYPE deinit_component(OMX_IN OMX_HANDLETYPE hComp)
{
    OMX_ERRORTYPE eRet = OMX_ErrorBadParameter;
    LOG(LOG_DEBUG2,"OMXCORE: deinit_component %p", hComp);

    if(hComp)
    {
        // call the deinit fuction first
        eRet = ((OMX_COMPONENTTYPE*)hComp)->ComponentDeInit(hComp);
    }
    return eRet;
}

/*
 * Clears the component handle from the component table
 */
static void clear_cmp_handle(OMX_HANDLETYPE inst)
{
    unsigned i = 0,j=0;

    if(NULL == inst)
        return;

    for(i=0; i< size_of_core; i++)
    {
        for(j=0; j< OMX_COMP_MAX_INST; j++)
        {
            if(inst == core[i].inst[j])
            {
                // destroy the component.
                free(inst);
                core[i].inst[j] = NULL;
                return;
            }
        }
    }
    return;
}

/*
 * Init
 */
OMX_ERRORTYPE OMX_Init()
{
    OMX_ERRORTYPE err;

    if(initialized == 0)
    {
        initialized = 1;
    }
    else
    {
        LOG(LOG_INFO, "OMXILCore:%s OMX Core already initialized ", __func__);
        return OMX_ErrorNone;
    }

    if ((err = omxil_init_slog2())== -1)
    {
        LOG(LOG_ERROR, "OMXILCore:%s failed to init slog2", __func__);
        return OMX_ErrorUndefined;
    }

    if((err = load_comp_libs()) != OMX_ErrorNone)
    {
        LOG(LOG_ERROR, "OMXILCore:%s failed to load component libs", __func__);
    }

    return err;
}

/*
 * In this function the Deinit function for each component loader is performed
 */
OMX_ERRORTYPE OMX_Deinit()
{
    int i, err;

    if(initialized) {
        for(i = 0; i < size_of_core; i++) {
            LOG(LOG_DEBUG2, " Unloading the dynamic library for %s", core[i].name);
            err = dlclose(core[i].so_lib_handle);
            if(err)
                LOG(LOG_ERROR, "Error %d in dlclose of lib %s", err,core[i].name);
            core[i].so_lib_handle = NULL;
        }
        size_of_core = 0;
        initialized = 0;
    }

    return OMX_ErrorNone;
}

/*
 * Search component name in componenet info table
 * Init component if found.
 */
OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE* pHandle,
        OMX_STRING cComponentName,
        OMX_PTR pAppData,
        OMX_CALLBACKTYPE* pCallBacks)
{

    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    int cmp_index = -1;
    int hnd_index = -1;

    LOG(LOG_DEBUG2, "In %s for %s", __func__, cComponentName);

    if(pHandle == NULL)
    {
        LOG(LOG_ERROR, "OMXILCore:%s Invalide input parameter pHandle(%p)", __func__, pHandle);
        return OMX_ErrorBadParameter;
    }

    cmp_index = get_cmp_index(cComponentName);
    if(cmp_index >= 0 && core[cmp_index].fn_ptr)
    {
        // Construct the component requested
        // Function returns the opaque handle
        void* hComp = malloc(sizeof(OMX_COMPONENTTYPE));
        if(hComp)
        {
            //Init component
            eRet = (*(core[cmp_index].fn_ptr))(hComp);
            if(eRet != OMX_ErrorNone)
            {
                LOG(LOG_ERROR,"Component not created succesfully");
                free(hComp);
                return eRet;
            }

            ((OMX_COMPONENTTYPE*)hComp)->SetCallbacks(hComp, pCallBacks, pAppData);

            hnd_index = get_comp_handle_index(cComponentName);
            if(hnd_index >= 0)
            {
                core[cmp_index].inst[hnd_index] = (OMX_HANDLETYPE) hComp;
                *pHandle = (OMX_HANDLETYPE)hComp;
                LOG(LOG_DEBUG2,"Component(%p) Successfully created", hComp);
            }
            else
            {
                LOG(LOG_ERROR,"OMX_GetHandle:NO free slot available to store Component Handle");
                free(hComp);
                eRet = OMX_ErrorInsufficientResources;
            }
        }
        else
        {
            eRet = OMX_ErrorInsufficientResources;
            LOG(LOG_ERROR,"Component Creation failed");
        }
    }
    else
    {
        eRet = OMX_ErrorNotImplemented;
        LOG(LOG_ERROR,"ERROR: Already another instance active  ;rejecting");
    }

    LOG(LOG_DEBUG2, "Out of %s", __func__);
    return eRet;
}

/*
 * Free component resources
 */
OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    int i = 0;
    LOG(LOG_DEBUG2,"OMXCORE API :  Free Handle %p", hComponent);

    //Check if there is active instance
    if((i = is_cmp_handle_exists(hComponent)) >=0)
    {
        //Delete the component
        if ((eRet = deinit_component(hComponent)) == OMX_ErrorNone)
        {
            clear_cmp_handle(hComponent);
        }
        else
        {
            LOG(LOG_DEBUG2," OMX_FreeHandle failed on %p", hComponent);
            return eRet;
        }
    }
    else
    {
        LOG(LOG_ERROR, "OMXCORE Warning: Free Handle called with no active instances");
    }
    return OMX_ErrorNone;

}

/*
 * Get comonent name from index
 */
OMX_ERRORTYPE OMX_ComponentNameEnum(
        OMX_STRING cComponentName,
        OMX_U32 nNameLength,
        OMX_U32 nIndex)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    size_t nNameLen;

    LOG(LOG_DEBUG2, "OMXCORE%s %p %d %d", __func__, cComponentName
            ,(unsigned)nNameLength
            ,(unsigned)nIndex);
    if(nIndex < size_of_core)
    {
        nNameLen = strlen(core[nIndex].name);
        if(nNameLength <= nNameLen) {
            LOG(LOG_ERROR, "OMXCORE%s Not enough space for cComponentName(%d, %d)", __func__,nNameLength, nNameLen);
            eRet = OMX_ErrorBadParameter;
        }
        else
            strcpy(cComponentName, core[nIndex].name);
    }
    else
    {
        eRet = OMX_ErrorNoMore;
    }
    return eRet;

}

/*
 * The implementation of this function is described in the OpenMAX spec
 */
OMX_ERRORTYPE OMX_SetupTunnel(
        OMX_HANDLETYPE hOutput,
        OMX_U32 nPortOutput,
        OMX_HANDLETYPE hInput,
        OMX_U32 nPortInput)
{

    /* Not supported right now */
    LOG(LOG_INFO, "OMXCORE:%s Not implemented", __func__);
    return OMX_ErrorNotImplemented;

}

/*
 * OMX_GetRolesOfComponent standard function
 */
OMX_ERRORTYPE OMX_GetRolesOfComponent (
        OMX_STRING compName,
        OMX_U32 *pNumRoles,
        OMX_U8 **roles)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned i,j,numofroles = 0;;

    LOG(LOG_DEBUG2,"GetRolesOfComponent %s",compName);
    if (roles == NULL)
    {
        if (pNumRoles == NULL)
        {
            LOG(LOG_ERROR,"%s:%d ERROR: Both Roles and numRoles Invalid", __func__, __LINE__);
            eRet = OMX_ErrorBadParameter;
        }
        else
        {
            *pNumRoles = 0;
            for(i=0; i< size_of_core; i++)
            {
                if(!strcmp(compName,core[i].name))
                {
                    for(j=0; (j<OMX_CORE_MAX_CMP_ROLES) && core[i].roles[j];j++)
                    {
                        (*pNumRoles)++;
                    }
                    break;
                }
            }

        }
        return eRet;
    }

    if(pNumRoles)
    {
        if (*pNumRoles == 0)
        {
            return OMX_ErrorBadParameter;
        }

        numofroles = *pNumRoles;
        *pNumRoles = 0;
        for(i=0; i< size_of_core; i++)
        {
            if(!strcmp(compName,core[i].name))
            {
                for(j=0; (j<OMX_CORE_MAX_CMP_ROLES) && core[i].roles[j];j++)
                {
                    if(roles && roles[*pNumRoles])
                    {
                        // Hopefully, client code provided a OMX_MAX_STRINGNAME_SIZE (128bytes) allocated buffer.
                        strcpy((char *)roles[*pNumRoles], core[i].roles[j]);
                    }
                    (*pNumRoles)++;
                    if (numofroles == *pNumRoles)
                    {
                        break;
                    }
                }
                break;
            }
        }
    }
    else
    {
        LOG(LOG_ERROR,"ERROR: Both Roles and numRoles Invalid");
        eRet = OMX_ErrorBadParameter;
    }
    return eRet;
}

/*
 * This function searches in all the component loaders any component
 * supporting the requested role
 */
OMX_ERRORTYPE OMX_GetComponentsOfRole (
        OMX_STRING role,
        OMX_U32 *pNumComps,
        OMX_U8  **compNames)
{
    OMX_ERRORTYPE eRet = OMX_ErrorNone;
    unsigned i,j,namecount=0;

    LOG(LOG_DEBUG2," Inside OMX_GetComponentsOfRole ");

    if( initialized == 0 ) {
      LOG(LOG_ERROR, "you need to call OMX_Init() at least once before calling this function");
      return OMX_ErrorInvalidState;
    }

    /*If CompNames is NULL then return*/
    if (compNames == NULL)
    {
        if (pNumComps == NULL)
        {
            eRet = OMX_ErrorBadParameter;
        }
        else
        {
            *pNumComps          = 0;
            for (i=0; i<size_of_core;i++)
            {
                for(j=0; j<OMX_CORE_MAX_CMP_ROLES && core[i].roles[j] ; j++)
                {
                    if(!strcmp(role,core[i].roles[j]))
                    {
                        (*pNumComps)++;
                    }
                }
            }
        }
        return eRet;
    }

    if(pNumComps)
    {
        namecount = *pNumComps;

        if (namecount == 0)
        {
            return OMX_ErrorBadParameter;
        }

        *pNumComps          = 0;

        for (i=0; i<size_of_core;i++)
        {
            for(j=0; j<OMX_CORE_MAX_CMP_ROLES && core[i].roles[j] ; j++)
            {
                if(!strcmp(role,core[i].roles[j]))
                {
                    // Hopefully, client code provided a OMX_MAX_STRINGNAME_SIZE (128bytes) allocated buffer.
                    strcpy((char *)compNames[*pNumComps], core[i].name);
                    (*pNumComps)++;
                    break;
                }
            }
            if (*pNumComps == namecount)
            {
                break;
            }
        }
    }
    else
    {
        eRet = OMX_ErrorBadParameter;
    }

    LOG(LOG_DEBUG2," Leaving OMX_GetComponentsOfRole ");
    return eRet;

}

/*
 * The implementation of this function is described in the OpenMAX spec
 */
OMX_ERRORTYPE OMX_GetContentPipe(
        OMX_HANDLETYPE *hPipe,
        OMX_STRING szURI)
{
    (void) hPipe, (void) szURI;
    /* Not supported right now */
    LOG(LOG_INFO, "OMXCORE:%s Not implemented", __func__);
    return OMX_ErrorNotImplemented;
}

