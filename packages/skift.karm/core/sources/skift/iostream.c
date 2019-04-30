/* Copyright © 2018-2019 N. Van Bossuyt.                                      */
/* This code is licensed under the MIT License.                               */
/* See: LICENSE.md                                                            */

#include <skift/cstring.h>
#include <skift/__plugs__.h>
#include <skift/__printf__.h>

#include <skift/math.h>
#include <skift/iostream.h>

// IO Stream constructor and destructor ------------------------------------- //

iostream_t *iostream(iostream_flag_t flags)
{
    iostream_t *stream = MALLOC(iostream_t);

    stream->flags = flags;

    stream->read = NULL;
    stream->write = NULL;
    stream->tell = NULL;
    stream->seek = NULL;
    stream->fstat = NULL;
    stream->ioctl = NULL;
    stream->close = NULL;

    stream->fd = -1;
    stream->p = NULL;

    if (flags & IOSTREAM_BUFFERED_WRITE)
    {
        stream->write_buffer = malloc(IOSTREAM_BUFFER_SIZE);
        stream->write_used = 0;
    }
    else
    {
        stream->write_buffer = NULL;
        stream->write_used = 0;
    }

    if (flags & IOSTREAM_BUFFERED_READ)
    {
        stream->read_buffer = malloc(IOSTREAM_BUFFER_SIZE);
        stream->read_used = 0;
    }
    else
    {
        stream->read_buffer = NULL;
        stream->read_used = 0;
    }

    return stream;
}

void iostream_delete(iostream_t *stream)
{
    if (stream->close != NULL)
    {
        stream->close(stream);
    }

    free(iostream);
}

iostream_t *iostream_open(const char *path, iostream_flag_t flags)
{
    int fd = __plug_iostream_open(path, flags);

    if (fd < 0)
    {
        return NULL;
    }

    iostream_t *stream = iostream(flags);
    if (stream == NULL)
    {
        __plug_iostream_close(fd);
    }

    stream->fd = fd;

    return stream;
}

void iostream_close(iostream_t *stream)
{
    if (stream != NULL)
    {
        iostream_flush(stream);

        if (stream->close != NULL)
        {
            stream->close(stream);
            return; 
        }

        if (stream->fd != -1)
        {
            __plug_iostream_close(stream->fd);
            return; 
        }
    }
}

// IO Stream generic io operation ------------------------------------------- //

int iostream_read(iostream_t *stream, void *buffer, uint size)
{

    if (stream->read != NULL)
    {
        return stream->read(stream, buffer, size);
    }

    if (stream->fd != -1)
    {
        return __plug_iostream_read(stream->fd, buffer, size);
    }

    return -1;
}

static int iostream_write_no_buffered(iostream_t *stream, const void *buffer, uint size)
{
    if (stream->write != NULL)
    {
        return stream->write(stream, buffer, size);
    }

    if (stream->fd != -1)
    {
        return __plug_iostream_write(stream->fd, buffer, size);
    }

    return -1;
}

static int iostream_write_buffered(iostream_t *stream, const void *buffer, uint size)
{
    int data_left = size;

    char *data_to_write = (char *)buffer;

    while (data_left > 0)
    {
        // Append the data to the buffer
        int free_space = IOSTREAM_BUFFER_SIZE - stream->write_used;
        int data_added = min(free_space, data_left);

        memcpy(((char *)(stream->write_buffer)) + stream->write_used, data_to_write, data_added);
        stream->write_used += data_added;
        data_left -= data_added;
        data_to_write += data_added;

        // flush the data if buffer is full
        if (stream->write_used == IOSTREAM_BUFFER_SIZE)
        {
            iostream_flush(stream);
        }
    }

    return size;
}

int iostream_write(iostream_t *stream, const void *buffer, uint size)
{
    if (stream != NULL)
    {
        if (stream->write_buffer != NULL)
        {
            return iostream_write_buffered(stream, buffer, size);
        }
        else
        {
            return iostream_write_no_buffered(stream, buffer, size);
        }
    }
    else
    {
        return -1;
    }
}

int iostream_flush(iostream_t *stream)
{
    if (stream != NULL)
    {
        if (stream->write_buffer != NULL)
        {
            iostream_write_no_buffered(stream, stream->write_buffer, stream->write_used);
            stream->write_used = 0;
        }

        return 0;
    }
    else
    {
        return -1;
    }
}

int iostream_tell(iostream_t *stream, iostream_whence_t whence)
{
    if (stream != NULL)
    {
        if (stream->tell != NULL)
        {
            return stream->tell(stream, whence);
        }

        if (stream->fd != -1)
        {
            return __plug_iostream_tell(stream->fd, whence);
        }
    }

    return 0;
}

int iostream_seek(iostream_t *stream, int offset, iostream_whence_t whence)
{
    if (stream != NULL)
    {
        if (stream->seek != NULL)
        {
            return stream->seek(stream, offset, whence);
        }

        if (stream->fd != -1)
        {
            return __plug_iostream_seek(stream->fd, offset, whence);
        }
    }

    return 0;
}

int iostream_fstat(iostream_t *stream, iostream_stat_t *stat)
{
    if (stream != NULL)
    {
        if (stream->fstat != NULL)
        {
            return stream->fstat(stream, stat);
        }

        if (stream->fd != -1)
        {
            return __plug_iostream_fstat(stream->fd, stat);
        }
    }

    return -1;
}

int iostream_ioctl(iostream_t *stream, int request, void *arg)
{
    if (stream != NULL)
    {
        if (stream->ioctl != NULL)
        {
            return stream->ioctl(stream, request, arg);
        }

        if (stream->fd != -1)
        {
            return __plug_iostream_ioctl(stream->fd, request, arg);
        }
    }

    return -1;
}

int iostream_putchar(iostream_t *stream, int c)
{
    if (iostream_write(stream, &c, 1) == 1)
    {
        return (int)(uchar)c;
    }
    else
    {
        return -1;
    }
}

int iostream_getchar(iostream_t *stream)
{
    uchar c;

    if (iostream_read(stream, &c, 1) == 1)
    {
        return (int)c;
    }
    else
    {
        return -1;
    }
}

int iostream_puts(iostream_t* stream, const char* string)
{
    return iostream_write(stream, string, strlen(string));
}

// int iostream_gets(iostream_t* stream, char* string, int n)
// {
// 
//     return 0;
// }

void iostream_printf_append(printf_info_t *info, char c)
{
    iostream_t* stream = info->p;
    iostream_write(stream, &c, 1);
}

int iostream_printf(iostream_t* stream, const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    int result = iostream_vprintf(stream, fmt, va);

    va_end(va);

    return result;
}

int iostream_vprintf(iostream_t* stream, const char* fmt, va_list va)
{

    printf_info_t info = (printf_info_t)
    {
        .format = fmt,
        .append = iostream_printf_append,
        .p = (void*)stream,
        .max_n = -1,
    };

    return __printf(&info, va);
}

// TODO: int iostream_scanf(iostream_t *stream, const char *fmt, ...)
//       {
//
//       }