/*
 * Direct3D 8
 *
 * Copyright 2005 Oliver Stieber
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 */

#include "config.h"
#include "wine/port.h"
#include "initguid.h"
#include "d3d8_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

HRESULT WINAPI D3D8GetSWInfo(void) {
    FIXME("(void): stub\n");
    return 0;
}

void WINAPI DebugSetMute(void) {
    /* nothing to do */
}

IDirect3D8 * WINAPI DECLSPEC_HOTPATCH Direct3DCreate8(UINT sdk_version)
{
    struct d3d8 *object;

    TRACE("sdk_version %#x.\n", sdk_version);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return NULL;

    if (!d3d8_init(object))
    {
        WARN("Failed to initialize d3d8.\n");
        heap_free(object);
        return NULL;
    }

    TRACE("Created d3d8 object %p.\n", object);

    return &object->IDirect3D8_iface;
}

/* FIXME: We should probably use libvkd3d-shader for validation. */
HRESULT WINAPI ValidateVertexShader(const DWORD *vs_code, const DWORD *declaration,
        const D3DCAPS8 *caps, BOOL return_error, char **errors)
{
    const char *message = "";
    SIZE_T message_size;
    HRESULT hr = E_FAIL;

    TRACE("vs_code %p, declaration %p, caps %p, return_error %#x, errors %p.\n",
            vs_code, declaration, caps, return_error, errors);

    if (!vs_code)
    {
        message = "Invalid code pointer.\n";
        goto done;
    }

    switch (*vs_code)
    {
        case D3DVS_VERSION(1, 1):
        case D3DVS_VERSION(1, 0):
            break;

        default:
            message = "Unsupported shader version.\n";
            goto done;
    }

    if (caps && *vs_code > caps->VertexShaderVersion)
    {
        message = "Shader version not supported by caps.\n";
        goto done;
    }

    hr = S_OK;

done:
    if (!return_error)
        message = "";
    message_size = strlen(message) + 1;
    if (errors && (*errors = heap_alloc(message_size)))
        memcpy(*errors, message, message_size);

    return hr;
}

/***********************************************************************
 *              ValidatePixelShader (D3D8.@)
 *
 * PARAMS
 * toto       result?
 */
HRESULT WINAPI ValidatePixelShader(DWORD* pixelshader, DWORD* reserved1, BOOL boolean, DWORD* toto)
{
  HRESULT ret;
  static BOOL warned;

  if (TRACE_ON(d3d8) || !warned) {
      FIXME("(%p %p %d %p): stub\n", pixelshader, reserved1, boolean, toto);
      warned = TRUE;
  }

  if (!pixelshader)
      return E_FAIL;

  if (reserved1)
      return E_FAIL;

  switch(*pixelshader) {
        case 0xFFFF0100:
        case 0xFFFF0101:
        case 0xFFFF0102:
        case 0xFFFF0103:
        case 0xFFFF0104:
            ret=S_OK;
            break;
        default:
            WARN("Invalid shader version token %#x.\n", *pixelshader);
            ret=E_FAIL;
        }
  return ret;
}

void d3d8_resource_cleanup(struct d3d8_resource *resource)
{
    wined3d_private_store_cleanup(&resource->private_store);
}

HRESULT d3d8_resource_free_private_data(struct d3d8_resource *resource, const GUID *guid)
{
    struct wined3d_private_data *entry;

    wined3d_mutex_lock();
    entry = wined3d_private_store_get_private_data(&resource->private_store, guid);
    if (!entry)
    {
        wined3d_mutex_unlock();
        return D3DERR_NOTFOUND;
    }

    wined3d_private_store_free_private_data(&resource->private_store, entry);
    wined3d_mutex_unlock();

    return D3D_OK;
}

HRESULT d3d8_resource_get_private_data(struct d3d8_resource *resource, const GUID *guid,
        void *data, DWORD *data_size)
{
    const struct wined3d_private_data *stored_data;
    DWORD size_in;
    HRESULT hr;

    wined3d_mutex_lock();
    stored_data = wined3d_private_store_get_private_data(&resource->private_store, guid);
    if (!stored_data)
    {
        hr = D3DERR_NOTFOUND;
        goto done;
    }

    size_in = *data_size;
    *data_size = stored_data->size;
    if (!data)
    {
        hr = D3D_OK;
        goto done;
    }
    if (size_in < stored_data->size)
    {
        hr = D3DERR_MOREDATA;
        goto done;
    }

    if (stored_data->flags & WINED3DSPD_IUNKNOWN)
        IUnknown_AddRef(stored_data->content.object);
    memcpy(data, stored_data->content.data, stored_data->size);
    hr = D3D_OK;

done:
    wined3d_mutex_unlock();
    return hr;
}

void d3d8_resource_init(struct d3d8_resource *resource)
{
    resource->refcount = 1;
    wined3d_private_store_init(&resource->private_store);
}

HRESULT d3d8_resource_set_private_data(struct d3d8_resource *resource, const GUID *guid,
        const void *data, DWORD data_size, DWORD flags)
{
    HRESULT hr;

    wined3d_mutex_lock();
    hr = wined3d_private_store_set_private_data(&resource->private_store, guid, data, data_size, flags);
    wined3d_mutex_unlock();
    return hr;
}
