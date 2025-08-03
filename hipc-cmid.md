### HIPC(CMIF) details:

service.h
```c
NX_INLINE Result serviceDispatchImpl(
    Service* s, u32 request_id,
    const void* in_data, u32 in_data_size,
    void* out_data, u32 out_data_size,
    SfDispatchParams disp
)
{
    // Make a copy of the service struct, so that the compiler can assume that it won't be modified by function calls.
    Service srv = *s;

    void* in = serviceMakeRequest(&srv, request_id, disp.context,
        in_data_size, disp.in_send_pid,
        disp.buffer_attrs, disp.buffers,
        disp.in_num_objects, disp.in_objects,
        disp.in_num_handles, disp.in_handles);

    if (in_data_size)
        __builtin_memcpy(in, in_data, in_data_size);

    Result rc = svcSendSyncRequest(disp.target_session == INVALID_HANDLE ? s->session : disp.target_session);
    if (R_SUCCEEDED(rc)) {
        void* out = NULL;
        rc = serviceParseResponse(&srv,
            out_data_size, &out,
            disp.out_num_objects, disp.out_objects,
            disp.out_handle_attrs, disp.out_handles);

        if (R_SUCCEEDED(rc) && out_data && out_data_size)
            __builtin_memcpy(out_data, out, out_data_size);
    }

    return rc;
}
```

cmif.h
```c
NX_INLINE Result cmifParseResponse(CmifResponse* res, void* base, bool is_domain, u32 size)
{
    HipcResponse hipc = hipcParseResponse(base);
    void* start = cmifGetAlignedDataStart(hipc.data_words, base);

    CmifOutHeader* hdr = NULL;
    u32* objects = NULL;
    if (is_domain) {
        CmifDomainOutHeader* domain_hdr = (CmifDomainOutHeader*)start;
        hdr = (CmifOutHeader*)(domain_hdr+1);
        objects = (u32*)((u8*)hdr + sizeof(CmifOutHeader) + size);
    }
    else
        hdr = (CmifOutHeader*)start;

    if (hdr->magic != CMIF_OUT_HEADER_MAGIC)
        return MAKERESULT(Module_Libnx, LibnxError_InvalidCmifOutHeader);
    if (R_FAILED(hdr->result))
        return hdr->result;

    *res = (CmifResponse){
        .data         = hdr+1,
        .objects      = objects,
        .copy_handles = hipc.copy_handles,
        .move_handles = hipc.move_handles,
    };

    return 0;
}
```

hipc.h

```c
NX_CONSTEXPR HipcResponse hipcParseResponse(void* base)
{
    // Parse header
    HipcHeader hdr = {};
    __builtin_memcpy(&hdr, base, sizeof(hdr));
    base = (u8*)base + sizeof(hdr);

    // Initialize response
    HipcResponse response = {};
    response.num_statics = hdr.num_send_statics;
    response.num_data_words = hdr.num_data_words;
    response.pid = HIPC_RESPONSE_NO_PID;

    // Parse special header
    if (hdr.has_special_header)
    {
        HipcSpecialHeader sphdr = {};
        __builtin_memcpy(&sphdr, base, sizeof(sphdr));
        base = (u8*)base + sizeof(sphdr);

        // Update response
        response.num_copy_handles = sphdr.num_copy_handles;
        response.num_move_handles = sphdr.num_move_handles;

        // Parse PID descriptor
        if (sphdr.send_pid) {
            response.pid = *(u64*)base;
            base = (u8*)base + sizeof(u64);
        }
    }

    // Copy handles
    response.copy_handles = (Handle*)base;
    base = response.copy_handles + response.num_copy_handles;

    // Move handles
    response.move_handles = (Handle*)base;
    base = response.move_handles + response.num_move_handles;

    // Send statics
    response.statics = (HipcStaticDescriptor*)base;
    base = response.statics + response.num_statics;

    // Data words
    response.data_words = (u32*)base;

    return response;
}
```