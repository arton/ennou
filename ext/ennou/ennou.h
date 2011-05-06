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

#if !defined(ENNOUT_HEADER)
#define ENNOUT_HEADER

typedef struct EnnouIO
{
    size_t requestContentLength;
    HTTP_REQUEST_ID requestId;
} ennou_io_t;

#endif /* ENNOUT_HEADER */
