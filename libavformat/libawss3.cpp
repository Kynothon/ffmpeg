/*
 * Copyright (c) 2014 Lukasz Marek <lukasz.m.luki@gmail.com>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/core/auth/AWSCredentials.h>

extern "C" {

#include "libavutil/avstring.h"
#include "libavutil/opt.h"
#include "avformat.h"
#include "internal.h"
#include "url.h"

typedef struct {
    const AVClass *clazz;
    Aws::S3::S3Client *ctx;
    int dh;
    int fd;
    int64_t filesize;
    int trunc;
    int timeout;
    char *workgroup;
} LIBAWSContext;

static av_cold int libawss3_connect(URLContext *h){
    printf("-> %s\n", __func__);
    LIBAWSContext *libawsclient = (LIBAWSContext*) h->priv_data;


    printf("1\n");
   // If region is specified, use it
    Aws::SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Debug;  

    Aws::InitAPI(options);

    printf("2\n");
    Aws::Client::ClientConfiguration clientConfig;
    clientConfig.region = "us-east-1";

    clientConfig.endpointOverride = "http://cheyenne:9000";
    Aws::String accessKeyId = "51ZBTAYHRQGLLIX2NF7W";
    Aws::String secretKey = "caMUtCdHEock7LjL5xfiGQgjyFRuEwr77lyVghQ/";
    Aws::Auth::AWSCredentials AWSCredentials = Aws::Auth::AWSCredentials(accessKeyId, secretKey);
    
    Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy signingPolicy = 
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never;
    // Set up request
    // snippet-start:[s3.cpp.put_object.code]
    Aws::S3::S3Client s3_client(AWSCredentials ,clientConfig, signingPolicy, false);

    printf("3\n");

    libawsclient->ctx = NULL; //&s3_client;

    printf("<- %s\n", __func__);
    return 0;
}

static av_cold int libsmbc_close(URLContext *h)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    if (libawsc->fd >= 0) {
        libawsc->fd = -1;
    }
    return 0;
}

static av_cold int libsmbc_open(URLContext *h, const char *url, int flags)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    int access, ret;
    struct stat st;

    libawsc->fd = -1;
    libawsc->filesize = -1;

    if ((ret = libawss3_connect(h)) < 0)
        goto fail;

    return 0;
  fail:
    libsmbc_close(h);
    return ret;
}

static int64_t libsmbc_seek(URLContext *h, int64_t pos, int whence)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    int64_t newpos;

    if (whence == AVSEEK_SIZE) {
        if (libawsc->filesize == -1) {
            av_log(h, AV_LOG_ERROR, "Error during seeking: filesize is unknown.\n");
            return AVERROR(EIO);
        } else
            return libawsc->filesize;
    }

    return newpos;
}

static int libsmbc_read(URLContext *h, unsigned char *buf, int size)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    int bytes_read;

    /*
    if ((bytes_read = smbc_read(libawsc->fd, buf, size)) < 0) {
        int ret = AVERROR(errno);
        av_log(h, AV_LOG_ERROR, "Read error: %s\n", strerror(errno));
        return ret;
    }
    */

    return bytes_read ? bytes_read : AVERROR_EOF;
}

static int libsmbc_write(URLContext *h, const unsigned char *buf, int size)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    int bytes_written;

    /*
    if ((bytes_written = smbc_write(libawsc->fd, buf, size)) < 0) {
        int ret = AVERROR(errno);
        av_log(h, AV_LOG_ERROR, "Write error: %s\n", strerror(errno));
        return ret;
    }
    */

    return bytes_written;
}

static int libsmbc_open_dir(URLContext *h)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    int ret;

    if ((ret = libawss3_connect(h)) < 0)
        goto fail;

    /*
    if ((libawsc->dh = smbc_opendir(h->filename)) < 0) {
        ret = AVERROR(errno);
        av_log(h, AV_LOG_ERROR, "Error opening dir: %s\n", strerror(errno));
        goto fail;
    }
    */

    return 0;

  fail:
    libsmbc_close(h);
    return ret;
}

static int libsmbc_read_dir(URLContext *h, AVIODirEntry **next)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    AVIODirEntry *entry;
    struct smbc_dirent *dirent = NULL;
    char *url = NULL;
    int skip_entry;

    *next = entry = ff_alloc_dir_entry();
    if (!entry)
        return AVERROR(ENOMEM);

        /*
    do {
        skip_entry = 0;
    //    dirent = smbc_readdir(libawsc->dh);
        if (!dirent) {
            av_freep(next);
            return 0;
        }
        switch (dirent->smbc_type) {
        case SMBC_DIR:
            entry->type = AVIO_ENTRY_DIRECTORY;
            break;
        case SMBC_FILE:
            entry->type = AVIO_ENTRY_FILE;
            break;
        case SMBC_FILE_SHARE:
            entry->type = AVIO_ENTRY_SHARE;
            break;
        case SMBC_SERVER:
            entry->type = AVIO_ENTRY_SERVER;
            break;
        case SMBC_WORKGROUP:
            entry->type = AVIO_ENTRY_WORKGROUP;
            break;
        case SMBC_COMMS_SHARE:
        case SMBC_IPC_SHARE:
        case SMBC_PRINTER_SHARE:
            skip_entry = 1;
            break;
        case SMBC_LINK:
        default:
            entry->type = AVIO_ENTRY_UNKNOWN;
            break;
        }
    } while (skip_entry || !strcmp(dirent->name, ".") ||
             !strcmp(dirent->name, ".."));

    entry->name = av_strdup(dirent->name);
    if (!entry->name) {
        av_freep(next);
        return AVERROR(ENOMEM);
    }

    url = av_append_path_component(h->filename, dirent->name);
    if (url) {
        struct stat st;
        if (!smbc_stat(url, &st)) {
            entry->group_id = st.st_gid;
            entry->user_id = st.st_uid;
            entry->size = st.st_size;
            entry->filemode = st.st_mode & 0777;
            entry->modification_timestamp = INT64_C(1000000) * st.st_mtime;
            entry->access_timestamp =  INT64_C(1000000) * st.st_atime;
            entry->status_change_timestamp = INT64_C(1000000) * st.st_ctime;
        }
        av_free(url);
    }
    */

    return 0;
}

static int libsmbc_close_dir(URLContext *h)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    /*
    if (libawsc->dh >= 0) {
        smbc_closedir(libsmbc->dh);
        libsmbc->dh = -1;
    }
    libsmbc_close(h);
    */
    return 0;
}

static int libsmbc_delete(URLContext *h)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h->priv_data;
    int ret;
    struct stat st;

    if ((ret = libawss3_connect(h)) < 0)
        goto cleanup;

    /*
    if ((libsmbc->fd = smbc_open(h->filename, O_WRONLY, 0666)) < 0) {
        ret = AVERROR(errno);
        goto cleanup;
    }

    if (smbc_fstat(libsmbc->fd, &st) < 0) {
        ret = AVERROR(errno);
        goto cleanup;
    }

    smbc_close(libsmbc->fd);
    libsmbc->fd = -1;

    if (S_ISDIR(st.st_mode)) {
        if (smbc_rmdir(h->filename) < 0) {
            ret = AVERROR(errno);
            goto cleanup;
        }
    } else {
        if (smbc_unlink(h->filename) < 0) {
            ret = AVERROR(errno);
            goto cleanup;
        }
    }
*/
    ret = 0;

cleanup:
    libsmbc_close(h);
    return ret;
}

static int libsmbc_move(URLContext *h_src, URLContext *h_dst)
{
    LIBAWSContext *libawsc = (LIBAWSContext*) h_src->priv_data;
    int ret;

    if ((ret = libawss3_connect(h_src)) < 0)
        goto cleanup;

    /*
    if ((libsmbc->dh = smbc_rename(h_src->filename, h_dst->filename)) < 0) {
        ret = AVERROR(errno);
        goto cleanup;
    }
    */

    ret = 0;

cleanup:
    libsmbc_close(h_src);
    return ret;
}

#define OFFSET(x) offsetof(LIBAWSContext, x)
#define D AV_OPT_FLAG_DECODING_PARAM
#define E AV_OPT_FLAG_ENCODING_PARAM
static const AVOption options[] = {
    {"timeout",   "set timeout in ms of socket I/O operations",    OFFSET(timeout), AV_OPT_TYPE_INT, {.i64 = -1}, -1, INT_MAX, D|E },
    {"truncate",  "truncate existing files on write",              OFFSET(trunc),   AV_OPT_TYPE_INT, { .i64 = 1 }, 0, 1, E },
    {"workgroup", "set the workgroup used for making connections", OFFSET(workgroup), AV_OPT_TYPE_STRING, { 0 }, 0, 0, D|E },
    {NULL}
};

static const AVClass libawss3_context_class = {
    .class_name     = "libawss3",
    .item_name      = av_default_item_name,
    .option         = options,
    .version        = LIBAVUTIL_VERSION_INT,
};
}

URLProtocol ff_libawss3_protocol = {
    .name                = "s3",
    .url_open            = libsmbc_open,
    .url_read            = libsmbc_read,
    .url_write           = libsmbc_write,
    .url_seek            = libsmbc_seek,
    .url_close           = libsmbc_close,
    .priv_data_size      = sizeof(LIBAWSContext),
    .priv_data_class     = &libawss3_context_class,
    .flags               = URL_PROTOCOL_FLAG_NETWORK,
    .url_open_dir        = libsmbc_open_dir,
    .url_read_dir        = libsmbc_read_dir,
    .url_close_dir       = libsmbc_close_dir,
    .url_delete          = libsmbc_delete,
    .url_move            = libsmbc_move,
};

int tesss = 3;
const int tee = 4;
