/*
    This file is part of BZCompressor.

    BZCompressor is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "bzcompressor.h"

bool BZCompressor::open(QIODevice::OpenMode mode)
{
    if (!isOpen() && m_device && m_device->open(mode))
    {
        if (mode & QIODevice::WriteOnly)
            m_state = compressInit(&m_strm, m_blockSize, m_verbosity, m_workFactor);
        else if (mode & QIODevice::ReadOnly)
            m_state = decompressInit(&m_strm, m_verbosity, m_small);
        else
            m_state = BZ_SEQUENCE_ERROR;

        if (m_state == BZ_OK)
            QIODevice::open(mode);
        else
            m_device->close();
    }

    return isOpen();
}

void BZCompressor::close()
{
    if (isOpen())
    {
        if (openMode() & QIODevice::WriteOnly)
        {
            QIODevice::close();
            if (!m_end)
                m_state = compress(reinterpret_cast<char*>(0), 0, BZ_FINISH);
            BZ2_bzCompressEnd(&m_strm);
        }
        else
        {
            QIODevice::close();
            BZ2_bzDecompressEnd(&m_strm);
        }

        m_device->close();
    }
}

qint64 BZCompressor::readData(char *data, qint64 maxlen)
{
    if (!m_end)
    {
        qint64 have;
        m_state = decompress(data, maxlen, have);
        if (m_state != BZ_OK)
            m_end = true;

        return have;
    }

    return -1;
}

int BZCompressor::decompress(char *data, qint64 length, qint64 &have)
{
    int ret = BZ_OK;
    have = 0;

    //Not update if out not full.
    //Potential truncation!
    m_strm.avail_out = static_cast<decltype(m_strm.avail_out)>(length);
    m_strm.next_out = data;

    do
    {
        if (m_strm.avail_in <= 0)
        {
            qint64 avail = m_device->read(m_buffer.data(), CHUNK);
            if (avail < 0)
            {
                ret = BZ_IO_ERROR;
                have = -1;
                setErrorString("error reading device");
                break;
            }
            //Potential truncation!
            m_strm.avail_in = static_cast<decltype(m_strm.avail_in)>(avail);
            m_strm.next_in = m_buffer.data();

            if (avail == 0)
            {
                if (m_device->isSequential())
                    ret = BZ_OK;
                else
                {
                    ret = BZ_UNEXPECTED_EOF;
                    setErrorString("invalid end of compressed stream");
                }
                break;
            }
        }

        ret = BZ2_bzDecompress(&m_strm);
        Q_ASSERT(ret != BZ_SEQUENCE_ERROR);
        switch (ret)
        {
        case BZ_DATA_ERROR:
            have = -1;
            setErrorString("invalid compress block");
            return ret;
        case BZ_DATA_ERROR_MAGIC:
            have = -1;
            setErrorString("invalid start stream");
            return ret;
        case BZ_MEM_ERROR:
            have = -1;
            setErrorString("can't allocate memory");
            return ret;
        }

        have = length - m_strm.avail_out;
    }
    while (m_strm.avail_out != 0 && ret != BZ_STREAM_END);

    return ret;
}

//Static.
int BZCompressor::decompress(QIODevice *src, QIODevice *dest, int verbosity, int small)
{
    int ret;
    qint64 have;
    char in[CHUNK];
    char out[CHUNK];

    bz_stream strm;
    ret = decompressInit(&strm, verbosity, small);
    if (ret != BZ_OK)
        return ret;

    do
    {
        qint64 avail = src->read(in, CHUNK);
        if (avail < 0)
        {
            BZ2_bzDecompressEnd(&strm);
            return BZ_IO_ERROR;
        }
        //Potential truncation!
        strm.avail_in = static_cast<decltype(strm.avail_in)>(avail);

        //End of file but not compressed stream.
        if (strm.avail_in == 0)
        {
            BZ2_bzDecompressEnd(&strm);
            return BZ_UNEXPECTED_EOF;
        }

        strm.next_in = in;

        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out;

            ret = BZ2_bzDecompress(&strm);
            Q_ASSERT(ret != BZ_SEQUENCE_ERROR);
            switch (ret)
            {
            case BZ_DATA_ERROR:
            case BZ_DATA_ERROR_MAGIC:
            case BZ_MEM_ERROR:
                BZ2_bzDecompressEnd(&strm);
                return ret;
            }

            have = CHUNK - strm.avail_out;
            if (dest->write(out, have) != have)
            {
                BZ2_bzDecompressEnd(&strm);
                return BZ_IO_ERROR;
            }
        }
        while (strm.avail_out == 0);
    }
    while(ret != BZ_STREAM_END);

    BZ2_bzDecompressEnd(&strm);

    return ret;
}

qint64 BZCompressor::writeData(const char *data, qint64 len)
{
    if (!m_end)
    {
        m_state = compress(const_cast<char*>(data), len, BZ_RUN);
        if (m_state == BZ_RUN_OK)
            return len;
        else
        {
            m_end = true;
            return -1;
        }
    }

    return -1;
}

int BZCompressor::compress(char *data, qint64 length, int action)
{
    int ret = BZ_RUN_OK;
    //Potential truncation!
    m_strm.avail_in = static_cast<decltype(m_strm.avail_in)>(length);
    m_strm.next_in = data;

    do
    {
        m_strm.avail_out = CHUNK;
        m_strm.next_out = m_buffer.data();

        ret = BZ2_bzCompress(&m_strm, action);
        Q_ASSERT(ret != BZ_SEQUENCE_ERROR);

        qint64 have = CHUNK - m_strm.avail_out;
        if (m_device->write(m_buffer.data(), have) != have)
        {
            ret = BZ_IO_ERROR;
            setErrorString("error writing device");
            break;
        }
    }
    while (m_strm.avail_out == 0);
    Q_ASSERT(m_strm.avail_in == 0);

    return ret;
}

//Static.
int BZCompressor::compress(QIODevice *src, QIODevice *dest, int blockSize, int verbosity,
                           int workFactor)
{
    int ret, action;
    char in[CHUNK];
    char out[CHUNK];

    bz_stream strm;
    ret = compressInit(&strm, blockSize, verbosity, workFactor);
    if (ret != BZ_OK)
        return ret;

    do
    {
        qint64 avail = src->read(in, CHUNK);
        if (avail < 0)
        {
            BZ2_bzCompressEnd(&strm);
            return BZ_IO_ERROR;
        }
        //Potential truncation!
        strm.avail_in = static_cast<decltype(strm.avail_in)>(avail);

        action = src->atEnd() ? BZ_FINISH : BZ_RUN;
        strm.next_in = in;

        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out;

            ret = BZ2_bzCompress(&strm, action);
            Q_ASSERT(ret != BZ_SEQUENCE_ERROR);

            qint64 have = CHUNK - strm.avail_out;
            if (dest->write(out, have) != have)
            {
                BZ2_bzCompressEnd(&strm);
                return BZ_IO_ERROR;
            }
        }
        while (strm.avail_out == 0);
        Q_ASSERT(strm.avail_in == 0);
    }
    while (action != BZ_FINISH);
    Q_ASSERT(ret == BZ_STREAM_END);

    BZ2_bzCompressEnd(&strm);

    return ret;
}

//Static.
void BZCompressor::resetStream(bz_stream *strm)
{
    strm->avail_in = 0;
    strm->total_in_lo32 = 0;
    strm->total_in_hi32 = 0;
    strm->next_in = reinterpret_cast<decltype(strm->next_in)>(0);

    strm->avail_out = 0;
    strm->total_out_lo32 = 0;
    strm->total_out_hi32 = 0;
    strm->next_out = reinterpret_cast<decltype(strm->next_out)>(0);

    strm->bzalloc = reinterpret_cast<decltype(strm->bzalloc)>(0);
    strm->bzfree = reinterpret_cast<decltype(strm->bzfree)>(0);
    strm->opaque = reinterpret_cast<decltype(strm->opaque)>(0);;
}
