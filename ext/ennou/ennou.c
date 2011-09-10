/*
 * ENNOu - http server for rack on windows HTTP Server API
 * Copyright(c) 2011 arton
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
 * $Id:$
 */
#define Ennou_VERSION  "1.1.1"

/* for windows */
#define UNICODE
#define _WIN32_WINNT 0x0600
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <http.h>

/* for ruby */
#include "ruby.h"

#include "ennou.h"

#define DECLARE_INFO(sname)    sname info; memset(&info, 0, sizeof(sname))
#define DECLARE_PROPERTY(name) rb_define_module_function(ennou, #name "=", set_##name, 1); \
                               rb_define_module_function(ennou, #name, get_##name, 0)
#define ASSIGN_STRING(name) name = rb_str_freeze(rb_str_new2(#name)); \
    rb_define_const(ennou, #name, name)
#define ASSIGN_STRING2(name,value) name = rb_str_freeze(rb_str_new2(#value)); \
    rb_define_const(ennou, #name, name)

#define STRING_IO_MAX 100000;

typedef enum bool_t {
    false = 0,
    true = 1,
} bool;

static VALUE ennou;
static VALUE ennou_io;
static VALUE server_class;
static VALUE utf16_enc;
static VALUE stringio;
static VALUE tempfile;
static VALUE EMPTY_STRING;

static VALUE REQUEST_METHOD;
static VALUE SCRIPT_NAME;
static VALUE PATH_INFO;
static VALUE QUERY_STRING;
static VALUE REQUEST_PATH;
static VALUE SERVER_NAME;
static VALUE SERVER_PORT;
static VALUE SERVER_PROTOCOL;

static VALUE AUTH_TYPE;
static VALUE REMOTE_ADDR;
static VALUE REMOTE_HOST;
static VALUE REMOTE_IDENT;
static VALUE REMOTE_USER;
static VALUE SERVER_SOFTWARE;
static VALUE URL_SCHEME;
static VALUE HTTP;
static VALUE HTTPS;

static ID HTTP_RESPONSE_HEADER_IDS[HttpHeaderResponseMaximum];
static const char* HTTP_RESPONSE_HEADER_STRS[] = {
    "cache-control",
    "connection",
    "date",
    "keep-alive",
    "pragma",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "via",
    "warning",
    "allow",
    "content-length",
    "content-type",
    "content-encoding",
    "content-language",
    "content-location",
    "content-md5",
    "content-range",
    "expires",
    "last-modified",
    "accept-ranges",
    "age",
    "etag",
    "location",
    "proxy-authenticate",
    "retry-after",
    "server",
    "set-cookie",
    "vary",
    "www-authenticate",
};
static VALUE HTTP_HEADER_IDS[HttpHeaderRequestMaximum];
static const char* HTTP_HEADER_STRS[] = {
    "CACHE_CONTROL",
    "CONNECTION",
    "DATE",
    "KEEP_ALIVE",
    "PRAGMA",
    "TRAILER",
    "TRANSFER_ENCODING",
    "UPGRADE",
    "VIA",
    "WARNING",
    "ALLOW",
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "CONTENT_ENCODING",
    "CONTENT_LANGUAGE",
    "CONTENT_LOCATION",
    "CONTENT_MD5",
    "CONTENT_RANGE",
    "EXPIRES",
    "LAST_MODIFIED",
    "ACCEPT",
    "ACCEPT_CHARSET",
    "ACCEPT_ENCODING",
    "ACCEPT_LANGUAGE",
    "AUTHORIZATION",
    "COOKIE",
    "EXPECT",
    "FROM",
    "HOST",
    "IF_MATCH",
    "IF_MODIFIED_SINCE",
    "IF_NONE_MATCH",
    "IF_RANGE",
    "IF_UNMODIFIED_SINCE",
    "MAX_FORWARDS",
    "PROXY_AUTHORIZATION",
    "REFERER",
    "RANGE",
    "TE",
    "TRANSLATE",
    "USER_AGENT",
};
static VALUE HTTP_VERB_VALUES[HttpVerbMaximum];
static const char* HTTP_VERB_STRS[] = {
    "UNPARSED",
    "UNKNOWN",
    "INVALID",
    "OPTIONS",
    "GET",
    "HEAD",
    "POST",
    "PUT",
    "DELETE",
    "TRACE",
    "CONNECT",
    "TRACK",
    "MOVE",
    "COPY",
    "PROPFIND",
    "PROPPATCH",
    "MKCOL",
    "LOCK",
    "UNLOCK",
    "SEARCH",
};

static ID id_group_id;
static ID id_handle_id;
static ID id_event_id;
static ID id_break_id;
static ID id_status_id;
static ID id_headers_id;
static ID id_body_id;
static ID id_server_id;
static ID id_wrote_id;
static ID id_content_length_id;
static ID id_input_id;
static ID id_controller_id;
static ID id_scriptname_id;
static ID id_str_encode;
static ID id_bytesize;
static ID id_downcase;
static ID id_encoding;
static ID id_binmode;
static ID id_set_encoding;
static ID id_open;
static ID id_write;

static HTTP_SERVER_SESSION_ID session_id;

static void uninit_ennou(VALUE);

static void query_property(HTTP_SERVER_PROPERTY prop, PVOID info, ULONG len)
{
    ULONG optlen = 0;
    ULONG stat = HttpQueryServerSessionProperty(session_id, prop, info, len, &optlen);
    if (stat != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpQueryServerSessionProperty for %d (%d)", prop, stat);
    }
}

static void set_property(HTTP_SERVER_PROPERTY prop, PVOID info, ULONG len)
{
    ULONG stat = HttpSetServerSessionProperty(session_id, prop, info, len);
    if (stat != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpSetServerSessionProperty for %d (%d)", prop, stat);
    }
}

static VALUE set_timeout(VALUE self, VALUE v)
{
    DECLARE_INFO(HTTP_TIMEOUT_LIMIT_INFO);
    info.Flags.Present = 1;
    info.EntityBody = FIX2INT(rb_ary_entry(v, 0));
    info.DrainEntityBody = FIX2INT(rb_ary_entry(v, 1));
    info.RequestQueue = FIX2INT(rb_ary_entry(v, 2));
    info.IdleConnection = FIX2INT(rb_ary_entry(v, 3));    
    info.HeaderWait = FIX2INT(rb_ary_entry(v, 4));
    info.MinSendRate = FIX2INT(rb_ary_entry(v, 5));    
    set_property(HttpServerTimeoutsProperty, &info, sizeof(info));
    return Qnil;
}

static VALUE get_timeout(VALUE self)
{
    VALUE ret;
    DECLARE_INFO(HTTP_TIMEOUT_LIMIT_INFO);
    query_property(HttpServerTimeoutsProperty, &info, sizeof(info));
    ret = rb_ary_new2(5);
    if (info.Flags.Present)
    {
        rb_ary_push(ret, INT2FIX(info.EntityBody));
        rb_ary_push(ret, INT2FIX(info.DrainEntityBody));
        rb_ary_push(ret, INT2FIX(info.RequestQueue));
        rb_ary_push(ret, INT2FIX(info.IdleConnection));
        rb_ary_push(ret, INT2FIX(info.HeaderWait));
        rb_ary_push(ret, INT2FIX(info.MinSendRate));
    }
    return ret;
}

static VALUE set_bandwidth(VALUE self, VALUE v)
{
    return Qnil;
}

static VALUE get_bandwidth(VALUE self)
{
    return Qnil;    
}

static VALUE set_logging(VALUE self, VALUE v)
{
    DECLARE_INFO(HTTP_LOGGING_INFO);
    info.Flags.Present = 1;
    info.LoggingFlags = HTTP_LOGGING_FLAG_USE_UTF8_CONVERSION;
    
    return Qnil;
}

static VALUE get_logging(VALUE self)
{
    return Qnil;
}

static VALUE set_authentication(VALUE self, VALUE v)
{
    return Qnil;
}

static VALUE get_authentication(VALUE self)
{
    return Qnil;
}

static VALUE set_channelbind(VALUE self, VALUE v)
{
    return Qnil;    
}

static VALUE get_channelbind(VALUE self)
{
    return Qnil;
}

static PCWSTR to_wchar(VALUE u)
{
#if U_DEBUG    
    int i;
#endif    
    VALUE u16 = rb_funcall(u, id_str_encode, 1, utf16_enc);
    VALUE len = rb_funcall(u16, id_bytesize, 0);
    ULONG ul = NUM2ULONG(len);
    LPBYTE p = ALLOC_N(char, ul + 2);
    memcpy(p, StringValuePtr(u16), ul);
    *(p + ul) = *(p + ul + 1) = 0;
#if U_DEBUG
    for (i = 0; i < ul; i++)
    {
        printf("%02X(%c)\n", *(p + i), *(p + i));
    }
#endif    
    return (PCWSTR)p;
}

static PCSTR to_utf8(PCWSTR w, int wlen)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, w, wlen, NULL, 0, NULL, NULL);
    PSTR p = ALLOC_N(char, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, w, wlen, p, len, NULL, NULL);
    *(p + len) = '\0';
    return p;
}

static int each_resp_header(VALUE key, VALUE value, HTTP_RESPONSE* resp)
{
    ULONG ulen;
    VALUE vlen;
    int len, i;
    ID id;
    if (NIL_P(value)) return ST_CONTINUE;
    Check_Type(value, T_STRING);
    vlen = rb_funcall(value, id_bytesize, 0);
    if (!FIXNUM_P(vlen)) return ST_CONTINUE;
    ulen = FIX2ULONG(vlen);
    if (ulen > USHRT_MAX) return ST_CONTINUE;
    if (SYMBOL_P(key))
    {
        id = SYM2ID(key);
    }
    else
    {
        Check_Type(key, T_STRING);
        id = rb_intern_str(rb_funcall(key, id_downcase, 0));
    }
    for (i = 0; i < HttpHeaderResponseMaximum; i++)
    {
        if (id == HTTP_RESPONSE_HEADER_IDS[i])
        {
            resp->Headers.KnownHeaders[i].RawValueLength = ulen;
            resp->Headers.KnownHeaders[i].pRawValue = StringValuePtr(value);
            break;
        }
    }
    return ST_CONTINUE;
}

static void set_resp_headers(HTTP_RESPONSE* resp, VALUE headers)
{
    rb_hash_foreach(headers, each_resp_header, (VALUE)resp);
}

#define WAIT_TICK  500

static ULONG wait_io(VALUE self, ULONG stat, LPOVERLAPPED ov, const char* func, ULONGLONG timeout)
{
    if (stat != ERROR_IO_PENDING)
    {
        CloseHandle(ov->hEvent);
        rb_raise(rb_eSystemCallError, "call %s (%d)", func, stat);
    }
    for (; timeout > GetTickCount64();)
    {
        VALUE exc;
        DWORD dwError;
        if (!RTEST(rb_ivar_get(self, id_break_id)))
        {
            stat = rb_w32_wait_events(&ov->hEvent, 1, WAIT_TICK);
            if (stat == WAIT_TIMEOUT) continue;
            if (stat == WAIT_OBJECT_0) break;
        }
        if (stat == WAIT_OBJECT_0 + 1 || RTEST(rb_ivar_get(self, id_break_id)))
        {
            dwError = WSAEINTR;
            exc = rb_eInterrupt;
        }
        else
        {
            dwError = GetLastError();
            exc = rb_eSystemCallError;
        }
        CloseHandle(ov->hEvent);
        rb_raise(exc, "wait %s (%d)", func, dwError);
    }
    return stat;
}

static void required_flush(VALUE self, VALUE stat, VALUE headers, VALUE body, bool disc, bool moredata)
{
    HTTP_RESPONSE resp;
    HTTP_DATA_CHUNK chunk;
    ennou_io_t* ennoup;
    OVERLAPPED over;
    ULONG ret, flags;
    
    if (NIL_P(stat) || NIL_P(headers) || NIL_P(body)) return;
    
    memset(&resp, 0, sizeof(resp));
    memset(&chunk, 0, sizeof(chunk));
    chunk.DataChunkType = HttpDataChunkFromMemory;
    chunk.FromMemory.pBuffer = StringValuePtr(body);
    chunk.FromMemory.BufferLength = NUM2ULONG(rb_funcall(body, id_bytesize, 0));
    resp.StatusCode = FIX2INT(stat);
    resp.EntityChunkCount = (chunk.FromMemory.BufferLength) ? 1 : 0;
    resp.pEntityChunks = &chunk;
    flags = (moredata) ? HTTP_SEND_RESPONSE_FLAG_MORE_DATA : 0;
    flags |= (disc) ? HTTP_SEND_RESPONSE_FLAG_DISCONNECT : 0;
    resp.Flags = 0;
    set_resp_headers(&resp, headers);
    if (resp.Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength)
    {
        rb_ivar_set(self, id_content_length_id, Qtrue);
    }
    memset(&over, 0, sizeof(over));
    over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    Data_Get_Struct(self, ennou_io_t, ennoup);
    ret = HttpSendHttpResponse((HANDLE)NUM2LL(rb_ivar_get(rb_ivar_get(self, id_server_id), id_handle_id)),
                               ennoup->requestId,
                               flags,
                               &resp,
                               NULL,
                               NULL,
                               NULL,
                               0,
                               &over,
                               NULL);
    if (ret != NO_ERROR)
    {
        ret = wait_io(self, ret, &over, "HttpSendHttpResponse", -1);
    }
    CloseHandle(over.hEvent);
    if (!moredata)
    {
        rb_ivar_set(self, id_wrote_id, Qnil);
    }
}

static void resp_write_header_and_data(VALUE self, VALUE body, bool disc, bool moredata)
{
    VALUE status = rb_ivar_get(self, id_status_id);
    VALUE headers = rb_ivar_get(self, id_headers_id);
    if (NIL_P(status)) status = INT2FIX(200);
    if (NIL_P(headers)) headers = rb_hash_new();
    if (NIL_P(body))
    {
        body = rb_str_new("", 0);
        status = INT2FIX(204);
    }
    required_flush(self, status, headers, body, disc, moredata);
}

static void resp_flush(VALUE self, bool disc)
{
    resp_write_header_and_data(self, rb_ivar_get(self, id_body_id), disc, false);
 }

static void resp_finish(VALUE self, bool disc)
{
    ennou_io_t* ennoup;
    OVERLAPPED over;
    ULONG ret;
    VALUE wrote = rb_ivar_get(self, id_wrote_id);
    if (NIL_P(wrote)) return;
    if (wrote == Qfalse)
    {
        resp_flush(self, disc);
        return;
    }
    if (NIL_P(rb_ivar_get(self, id_content_length_id)))
    {
        disc = true;
    }
    memset(&over, 0, sizeof(over));
    over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    Data_Get_Struct(self, ennou_io_t, ennoup);
    ret = HttpSendResponseEntityBody((HANDLE)NUM2LL(rb_ivar_get(rb_ivar_get(self, id_server_id), id_handle_id)),
                                     ennoup->requestId,
                                     (disc) ? HTTP_SEND_RESPONSE_FLAG_DISCONNECT : 0,
                                     0,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     &over,
                                     NULL);
    if (ret != NO_ERROR && ret != ERROR_CONNECTION_INVALID)
    {
        wait_io(self, ret, &over, "HttpSendResponseEntityBody(close)", -1);
    }
    CloseHandle(over.hEvent);
    rb_ivar_set(self, id_wrote_id, Qnil);
}

#define BUFFSIZE 4096

static VALUE req_input(VALUE self)
{
    ennou_io_t* ennoup;
    OVERLAPPED over;
    DWORD bytesRead;
    ULONG ret;
    HANDLE queue = (HANDLE)NUM2LL(rb_ivar_get(rb_ivar_get(self, id_server_id), id_handle_id));
    volatile VALUE input = rb_ivar_get(self, id_input_id);
    if (input != Qnil) return input;
    memset(&over, 0, sizeof(over));
    Data_Get_Struct(self, ennou_io_t, ennoup);
    over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ennoup->requestContentLength)
    {
        char* buff;
        size_t read;
        size_t len = ennoup->requestContentLength;
        VALUE reqbody = rb_external_str_new_with_enc(NULL, len, rb_ascii8bit_encoding());
        buff = StringValuePtr(reqbody);
        for (read = 0; read < len;)
        {
            ret = HttpReceiveRequestEntityBody(queue, ennoup->requestId,
                                               0, 
                                               buff + read, len - read,
                                               NULL, &over);
            if (ret == ERROR_HANDLE_EOF) break;
            if (ret != NO_ERROR)
            {
                ret = wait_io(self, ret, &over, "HttpReceiveRequestEntityBody", -1);
                if (ret == WAIT_TIMEOUT)
                {
                    CloseHandle(over.hEvent);
                    return Qnil;
                }
            }
            if (!GetOverlappedResult(queue, &over, &bytesRead, FALSE))
            {
                CloseHandle(over.hEvent);                
                rb_raise(rb_eSystemCallError, "retrieve request body (%d)",  GetLastError());
            }
            read += bytesRead;
            ResetEvent(over.hEvent);
        }
        if (read < len)
        {
            rb_str_set_len(reqbody, read);
        }
        rb_ivar_set(self, id_input_id,
                    input = rb_class_new_instance(1, &reqbody, stringio));
        rb_funcall(input, id_set_encoding, 2, rb_enc_from_encoding(rb_ascii8bit_encoding()), Qnil);
    }
    else
    {
        VALUE args[2];
        VALUE rbuff = rb_external_str_new_with_enc(NULL, BUFFSIZE, rb_ascii8bit_encoding());
        char* buff = StringValuePtr(rbuff);
        args[0] = rb_str_new("ennou", 5);
        args[1] = rb_hash_new();
        rb_hash_aset(args[1], ID2SYM(id_encoding), rb_enc_from_encoding(rb_ascii8bit_encoding()));
        rb_ivar_set(self, id_input_id, 
                    input = rb_class_new_instance(2, args, tempfile));
        rb_funcall(input, id_binmode, 0);
        for (;;)
        {
            rb_str_set_len(rbuff, BUFFSIZE);
            ret = HttpReceiveRequestEntityBody(queue, ennoup->requestId,
                                               0, 
                                               buff, BUFFSIZE,
                                               NULL, &over);
            if (ret == ERROR_HANDLE_EOF) break;
            if (ret != NO_ERROR)
            {
                ret = wait_io(self, ret, &over, "HttpReceiveRequestEntityBody", -1);
                if (ret == WAIT_TIMEOUT)
                {
                    CloseHandle(over.hEvent);
                    return Qnil;
                }
            }
            if (!GetOverlappedResult(queue, &over, &bytesRead, FALSE))
            {
                CloseHandle(over.hEvent);                
                rb_raise(rb_eSystemCallError, "retrieve request body (%d)",  GetLastError());
            }
            if (rb_str_strlen(rbuff) != bytesRead)
            {
                rb_str_set_len(rbuff, bytesRead);
            }
            rb_funcall(input, id_write, 1, rbuff);
            ResetEvent(over.hEvent);
        }
        rb_funcall(input, id_open, 0);
    }
    CloseHandle(over.hEvent);
    return input;
}

/*
 *  call-seq:
 *    io.close
 *
 *    flush response. the connection may remain
 *    if the connection header variable isn't set close.
 */
static VALUE resp_close(VALUE self)
{
    resp_finish(self, false);
    return Qnil;
}

/*
 *  call-seq:
 *    io.disconnect
 *
 *    flush response and disconnect from client.
 */
static VALUE resp_disconnect(VALUE self)
{
    resp_finish(self, true);    
    return Qnil;    
}

/*
 *  call-seq:
 *     io.lump(status, headers, body)
 *
 *  send response using status, headers and body.
 *
 */
static VALUE resp_lump(VALUE self, VALUE stat, VALUE headers, VALUE body)
{
    FIX2INT(stat); // check fixnum range
    Check_Type(headers, T_HASH);
    Check_Type(body, T_STRING);
    required_flush(self, stat, headers, body, false, false);
    return Qnil;
}

static VALUE resp_status(VALUE self, VALUE stat)
{
    int n = FIX2INT(stat); // check fixnum range
    rb_ivar_set(self, id_status_id, stat);
    required_flush(self, stat, rb_ivar_get(self, id_headers_id), rb_ivar_get(self, id_body_id), false, false);
    return stat;
}

static VALUE resp_headers(VALUE self, VALUE headers)
{
    Check_Type(headers, T_HASH);
    rb_ivar_set(self, id_headers_id, headers);
    required_flush(self, rb_ivar_get(self, id_status_id), headers, rb_ivar_get(self, id_body_id), false, false);
    return headers;
}

static VALUE resp_body(VALUE self, VALUE body)
{
    Check_Type(body, T_STRING);
    rb_ivar_set(self, id_body_id, body);
    required_flush(self, rb_ivar_get(self, id_status_id), rb_ivar_get(self, id_headers_id), body, false, false);
    return body;
}

static VALUE resp_write_data(VALUE self, VALUE buff)
{
    ennou_io_t* ennoup;
    OVERLAPPED over;
    ULONG ret;
    HTTP_DATA_CHUNK chunk;
    memset(&chunk, 0, sizeof(chunk));
    chunk.DataChunkType = HttpDataChunkFromMemory;
    chunk.FromMemory.pBuffer = StringValuePtr(buff);
    chunk.FromMemory.BufferLength = NUM2ULONG(rb_funcall(buff, id_bytesize, 0));
    memset(&over, 0, sizeof(over));
    over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    Data_Get_Struct(self, ennou_io_t, ennoup);
    ret = HttpSendResponseEntityBody((HANDLE)NUM2LL(rb_ivar_get(rb_ivar_get(self, id_server_id), id_handle_id)),
                                     ennoup->requestId,
                                     HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
                                     1,
                                     &chunk,
                                     NULL,
                                     NULL,
                                     0,
                                     &over,
                                     NULL);
    if (ret != NO_ERROR)
    {
        wait_io(self, ret, &over, "HttpSendResponseEntityBody", -1);
    }
    CloseHandle(over.hEvent);
    return Qnil;    
}

static VALUE resp_write(VALUE self, VALUE buff)
{
    VALUE wrote;
    Check_Type(buff, T_STRING);
    wrote = rb_ivar_get(self, id_wrote_id);
    if (NIL_P(wrote)) rb_raise(rb_eRuntimeError, "IO already closed");
    if (wrote == Qfalse)
    {
        resp_write_header_and_data(self, buff, false, true);
        rb_ivar_set(self, id_wrote_id, Qtrue);
    }
    else
    {
        resp_write_data(self, buff);
    }
    return Qnil;
}

static VALUE server_add(VALUE self, VALUE uri)
{
    PSTR uri8;
    PSTR script;
    VALUE gid = rb_ivar_get(self, id_group_id);
    PCWSTR uri16 = to_wchar(uri);
    ULONG ret = HttpAddUrlToUrlGroup(NUM2LL(gid),
                                     uri16,
                                     0, 0);
    free((LPVOID)uri16);
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpAddUrlToUrlGroup for %s (%d)",
                 StringValueCStr(uri), ret);
    }
    uri8 = StringValueCStr(uri);
    script = strrchr(uri8, '/');
    if (!script || !*(script + 1))
    {
        rb_ivar_set(self, id_scriptname_id, EMPTY_STRING);
    }
    else
    {
        VALUE vscript = rb_str_new2(script);
        rb_ivar_set(self, id_scriptname_id, rb_str_freeze(vscript));
    }
    return uri;
}

static VALUE server_remove(VALUE self, VALUE uri)
{
    VALUE gid = rb_ivar_get(self, id_group_id);
    PCWSTR uri16 = to_wchar(uri);
    ULONG ret = HttpRemoveUrlFromUrlGroup(NUM2LL(gid),
                                          uri16,
                                          HTTP_URL_FLAG_REMOVE_ALL);
    free((LPVOID)uri16);
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpRemoveUrlFromUrlGroup for %s (%d)",
                 StringValueCStr(uri), ret);
    }
    return uri;
}

static VALUE server_set_script(VALUE self, VALUE script)
{
    rb_ivar_set(self, id_scriptname_id, script);
    return script;
}

static VALUE server_get_script(VALUE self)
{
    return rb_ivar_get(self, id_scriptname_id);
}

static VALUE server_break(VALUE self)
{
    rb_ivar_set(self, id_break_id, Qtrue);
    return self;
}

static VALUE server_wait(VALUE self, VALUE secs)
{
    ennou_io_t* ennoup;
    VALUE evt, result, resp, reqbody, env = rb_hash_new();
    HANDLE queue = (HANDLE)NUM2LL(rb_ivar_get(self, id_handle_id));
    BYTE buff[3000];
    char protobuff[64];
    HTTP_REQUEST* req = (HTTP_REQUEST*)buff;
    OVERLAPPED over;
    ULONG ret;
    PCSTR url;
    int i;
    double to = NUM2DBL(secs) * 1000;

    memset(&over, 0, sizeof(over));
    evt = rb_ivar_get(self, id_event_id);
    if (NIL_P(evt))
    {
        over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        rb_ivar_set(self, id_event_id, LL2NUM((__int64)over.hEvent));
    }
    else
    {
        over.hEvent = (HANDLE)NUM2LL(evt);
        ResetEvent(over.hEvent);
    }
    ret = HttpReceiveHttpRequest(queue, HTTP_NULL_ID, 0,
                                 req, sizeof(buff),
                                 NULL, &over);
    if (ret != NO_ERROR)
    {
        ret = wait_io(self, ret, &over, "HttpReceiveHttpRequest",
                      GetTickCount64() + (ULONGLONG)to);
        if (ret == WAIT_TIMEOUT)
        {
            ret = CancelIo(queue);
            return Qnil;
        }
    }
    for (i = 0; i < HttpHeaderRequestMaximum; i++)
    {
        if (req->Headers.KnownHeaders[i].RawValueLength > 0)
        {
            rb_hash_aset(env, HTTP_HEADER_IDS[i],
                         rb_str_new(req->Headers.KnownHeaders[i].pRawValue,
                                    req->Headers.KnownHeaders[i].RawValueLength));
        }
    }
    if (req->Verb == HttpVerbUnknown)
    {
        rb_hash_aset(env, REQUEST_METHOD,
                     rb_str_new(req->pUnknownVerb, req->UnknownVerbLength));
    }
    else
    {
        rb_hash_aset(env, REQUEST_METHOD, HTTP_VERB_VALUES[req->Verb]);
    }
    sprintf(protobuff, "%s/%d.%d", "HTTP",
            req->Version.MajorVersion, req->Version.MinorVersion);
    rb_hash_aset(env, SERVER_PROTOCOL, rb_str_new2(protobuff));
    rb_hash_aset(env, URL_SCHEME, (req->pSslInfo) ? HTTPS : HTTP);
    if (req->CookedUrl.HostLength)
    {
        char* chrp;
        url = to_utf8(req->CookedUrl.pHost, req->CookedUrl.HostLength / 2);
        chrp = strchr(url, ':');
        if (chrp)
        {
            rb_hash_aset(env, SERVER_NAME, rb_str_new(url, (size_t)chrp - (size_t)url));
            rb_hash_aset(env, SERVER_PORT, rb_str_new2(chrp + 1));
        }
        else
        {
            rb_hash_aset(env, SERVER_NAME, rb_str_new2(url));
            rb_hash_aset(env, SERVER_PORT, rb_str_new2("80"));
        }   
        free((LPVOID)url);
    }
    if (req->CookedUrl.AbsPathLength)
    {
        VALUE script = rb_ivar_get(self, id_scriptname_id);
        url = to_utf8(req->CookedUrl.pAbsPath, req->CookedUrl.AbsPathLength / 2);
        if (!strncmp(url, StringValueCStr(script), rb_str_strlen(script)))
        {
            rb_hash_aset(env, SCRIPT_NAME, script);
            rb_hash_aset(env, PATH_INFO, rb_str_new2(url + rb_str_strlen(script)));
        }
        else
        {
            rb_hash_aset(env, SCRIPT_NAME, EMPTY_STRING);
            rb_hash_aset(env, PATH_INFO, rb_str_new2(url));
        }
        free((LPVOID)url);
    }
    else
    {
        rb_hash_aset(env, SCRIPT_NAME, EMPTY_STRING);
        rb_hash_aset(env, PATH_INFO, EMPTY_STRING);
    }
    if (req->CookedUrl.QueryStringLength > 2)
    {
        url = to_utf8(req->CookedUrl.pQueryString + 1, req->CookedUrl.QueryStringLength / 2 - 1);
        rb_hash_aset(env, QUERY_STRING, rb_str_new2(url));
        free((LPVOID)url);
    }
    else
    {
        rb_hash_aset(env, QUERY_STRING, EMPTY_STRING);
    }
    result = rb_ary_new2(2);
    rb_ary_push(result, env);
    resp = Data_Make_Struct(ennou_io, ennou_io_t, 0, 0, ennoup);
    ennoup->requestId = req->RequestId;
    rb_ivar_set(resp, id_server_id, self);
    rb_ivar_set(resp, id_wrote_id, Qfalse);
    rb_ivar_set(resp, id_content_length_id, Qnil);
    reqbody = Qnil;
    if (!(req->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS))
    {
        reqbody = rb_enc_str_new("", 0, rb_ascii8bit_encoding());
    }
    else if (req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength > 0
             && req->Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength < 7)
    {
        ennoup->requestContentLength = atoi(req->Headers.KnownHeaders[HttpHeaderContentLength].pRawValue);
        if (ennoup->requestContentLength < 0)
        {
            ennoup->requestContentLength = 0;
        }
    }
    if (!NIL_P(reqbody))
    {
        VALUE siop = rb_class_new_instance(1, &reqbody, stringio);
        rb_funcall(siop, id_set_encoding, 2, rb_enc_from_encoding(rb_ascii8bit_encoding()), Qnil);
        rb_ivar_set(resp, id_input_id, siop);
    }
    rb_ary_push(result, resp);
    return result;
}

static VALUE server_close(VALUE self)
{
    ULONG ret;
    VALUE gid = rb_ivar_get(self, id_group_id);
    VALUE handle = rb_ivar_get(self, id_handle_id);
    VALUE evt = rb_ivar_get(self, id_event_id);
    if (!NIL_P(evt))
    {
        CloseHandle((HANDLE)NUM2LL(evt));
    }
    ret = HttpShutdownRequestQueue((HANDLE)NUM2LL(handle));
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpShutdownRequestQueue %d", ret);
    }
    ret = HttpCloseUrlGroup(NUM2LL(gid));
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpCloseUrlGroup %d", ret);
    }
    rb_ivar_set(self, id_group_id, Qnil);
    ret = HttpCloseRequestQueue((HANDLE)NUM2LL(handle));
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpCloseRequestQueue %d", ret);
    }
    rb_ivar_set(self, id_handle_id, Qnil);
    return Qnil;
}

static VALUE server_initialize(int argc, VALUE* argv, VALUE svr)
{
    ULONG ret;
    HTTP_URL_GROUP_ID gid;
    HANDLE queue;
    VALUE qname, multi;
    PCWSTR cqname;
    HTTPAPI_VERSION httpapi_version = HTTPAPI_VERSION_2;
    DECLARE_INFO(HTTP_BINDING_INFO);

    rb_scan_args(argc, argv, "11", &qname, &multi);
    Check_Type(qname, T_STRING);
    ret = HttpCreateUrlGroup(session_id, &gid, 0);
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "call HttpCreateUrlGroup %d", ret);
    }
    rb_ivar_set(svr, id_group_id, LL2NUM(gid));
    rb_ivar_set(svr, id_controller_id, Qfalse);
    rb_ivar_set(svr, id_break_id, Qfalse);
    rb_ivar_set(svr, id_event_id, Qnil);
    cqname = to_wchar(qname);
    if (RTEST(multi))
    {
        ret = HttpCreateRequestQueue(httpapi_version, cqname, NULL,
                                  HTTP_CREATE_REQUEST_QUEUE_FLAG_OPEN_EXISTING, &queue);
        if (ret != NO_ERROR)
        {
            rb_ivar_set(svr, id_controller_id, Qtrue);
            ret = HttpCreateRequestQueue(httpapi_version, cqname, NULL,
                                      HTTP_CREATE_REQUEST_QUEUE_FLAG_CONTROLLER, &queue);
        }
    }
    else
    {
        ret = HttpCreateRequestQueue(httpapi_version, cqname, NULL,
                                     0, &queue);
    }
    free((LPVOID)cqname);
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eFatal, "can't create request queue: %d", stat);
    }
    rb_ivar_set(svr, id_handle_id, LL2NUM((__int64)queue));
    info.Flags.Present = 1;
    info.RequestQueueHandle = queue;
    ret = HttpSetUrlGroupProperty(gid, HttpServerBindingProperty, &info, sizeof(info));
    if (ret != NO_ERROR)
    {
        rb_raise(rb_eSystemCallError, "Binding Queue %d", ret);
    }
    return svr;
}

static VALUE server_is_controller(VALUE self)
{
    return rb_ivar_get(self, id_controller_id);
}

static VALUE server_s_open(int argc, VALUE* argv, VALUE klass)
{
    VALUE svr = rb_class_new_instance(argc, argv, klass);
    if (rb_block_given_p())
    {
        return rb_ensure(rb_yield, svr, server_close, svr);
    }
    return svr;
}

static VALUE no_initialize(int argc, VALUE* argv, VALUE self)
{
    rb_raise(rb_eRuntimeError, "Ennou can't instantiate explicitly");
}

void Init_ennou()
{
    int i;
    char buff[64];
    ULONG stat;
    HTTPAPI_VERSION httpapi_version = HTTPAPI_VERSION_2;
    VALUE symgid = ID2SYM(rb_intern("group_id"));
    ennou = rb_define_module("Ennou");
    EMPTY_STRING = rb_str_freeze(rb_str_new2(""));
    rb_define_const(ennou, "EMPTY_STRING", EMPTY_STRING);
    ASSIGN_STRING(REQUEST_METHOD);
    ASSIGN_STRING(SCRIPT_NAME);
    ASSIGN_STRING(PATH_INFO);
    ASSIGN_STRING(QUERY_STRING);
    ASSIGN_STRING(REQUEST_PATH);
    ASSIGN_STRING(SERVER_NAME);
    ASSIGN_STRING(SERVER_PORT);
    ASSIGN_STRING(SERVER_PROTOCOL);
    ASSIGN_STRING(AUTH_TYPE);
    ASSIGN_STRING(REMOTE_ADDR);
    ASSIGN_STRING(REMOTE_HOST);
    ASSIGN_STRING(REMOTE_IDENT);
    ASSIGN_STRING(REMOTE_USER);
    ASSIGN_STRING(SERVER_SOFTWARE);
    ASSIGN_STRING(URL_SCHEME);
    ASSIGN_STRING2(HTTPS, https);
    ASSIGN_STRING2(HTTP, http);

    for (i = 0; i < HttpHeaderResponseMaximum; i++)
    {
        HTTP_RESPONSE_HEADER_IDS[i] = rb_intern(HTTP_RESPONSE_HEADER_STRS[i]);
    }

    for (i = 0; i < HttpHeaderRequestMaximum; i++)
    {
        if (i == HttpHeaderContentLength || i == HttpHeaderContentType)
        {
            strcpy(buff, HTTP_HEADER_STRS[i]);
        }
        else
        {
            sprintf(buff, "HTTP_%s", HTTP_HEADER_STRS[i]);
        }   
        HTTP_HEADER_IDS[i] = rb_str_freeze(rb_str_new2(buff));
        rb_define_const(ennou, buff, HTTP_HEADER_IDS[i]);
    }
    for (i = 0; i < HttpVerbMaximum; i++)
    {
        HTTP_VERB_VALUES[i] = rb_str_freeze(rb_str_new2(HTTP_VERB_STRS[i]));
        rb_define_const(ennou, HTTP_VERB_STRS[i], HTTP_VERB_VALUES[i]);
    }
    
    id_group_id = rb_intern("@group_id");
    id_handle_id = rb_intern("@handle");
    id_event_id = rb_intern("@event");
    id_break_id = rb_intern("@break");
    id_status_id = rb_intern("@status");
    id_headers_id = rb_intern("@headers");
    id_body_id = rb_intern("@body");
    id_server_id = rb_intern("@server");
    id_wrote_id = rb_intern("@wrote");
    id_content_length_id = rb_intern("@content_length");
    id_input_id = rb_intern("@input");
    id_controller_id = rb_intern("@controller");
    id_scriptname_id = rb_intern("@scriptname");
    id_str_encode = rb_intern("encode");
    id_bytesize = rb_intern("bytesize");
    id_downcase = rb_intern("downcase");
    id_encoding = rb_intern("encoding");
    id_binmode = rb_intern("binmode");
    id_set_encoding = rb_intern("set_encoding");
    id_open = rb_intern("open");
    id_write = rb_intern("write");
    utf16_enc = rb_const_get_from(rb_cEncoding, rb_intern("UTF_16LE"));
    
    rb_define_const(ennou, "VERSION", rb_str_new2(Ennou_VERSION));

    server_class = rb_define_class_under(ennou, "Server", rb_cObject);
    rb_define_singleton_method(server_class, "open", server_s_open, -1);
    rb_define_method(server_class, "initialize", server_initialize, -1);
    rb_define_method(server_class, "controller?", server_is_controller, 0);
    rb_define_method(server_class, "close", server_close, 0);
    rb_define_method(server_class, "wait", server_wait, 1);
    rb_define_method(server_class, "break", server_break, 0);
    rb_define_method(server_class, "add", server_add, 1);
    rb_define_method(server_class, "remove", server_remove, 1);
    rb_define_method(server_class, "script=", server_set_script, 1);
    rb_define_method(server_class, "script", server_get_script, 0);
    rb_mod_attr(1, &symgid, server_class);

    ennou_io = rb_define_class_under(ennou, "EnnouIO", rb_cObject);
    rb_define_method(ennou_io, "initialize", no_initialize, -1);
    rb_define_method(ennou_io, "close", resp_close, 0);
    rb_define_method(ennou_io, "disconnect", resp_disconnect, 0);
    rb_define_method(ennou_io, "input", req_input, 0);
    rb_define_method(ennou_io, "status=", resp_status, 1);
    rb_define_method(ennou_io, "headers=", resp_headers, 1);
    rb_define_method(ennou_io, "body=", resp_body, 1);
    rb_define_method(ennou_io, "lump", resp_lump, 3);
    rb_define_method(ennou_io, "write", resp_write, 1);

    stat = HttpInitialize(httpapi_version, HTTP_INITIALIZE_SERVER, 0);
    if (stat != NO_ERROR)
    {
        rb_raise(rb_eFatal, "can't initialize http server: %d", stat);
    }
    stat = HttpCreateServerSession(httpapi_version, &session_id, 0);
    if (stat != NO_ERROR)
    {
        rb_raise(rb_eFatal, "can't create http server session: %d", stat);
    }
    rb_set_end_proc(uninit_ennou, Qnil);

    DECLARE_PROPERTY(timeout);
    DECLARE_PROPERTY(bandwidth);
    DECLARE_PROPERTY(logging);
    DECLARE_PROPERTY(authentication);
    DECLARE_PROPERTY(channelbind);

    rb_require("stringio");
    stringio = rb_const_get(rb_cObject, rb_intern("StringIO"));
    rb_require("tempfile");
    tempfile = rb_const_get(rb_cObject, rb_intern("Tempfile"));
}

static void uninit_ennou(VALUE v)
{
    HttpCloseServerSession(session_id);
}
