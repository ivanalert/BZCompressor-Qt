#include "bzcompressor.h"

const unsigned BZCompressor::CHUNK(16384);

BZCompressor::BZCompressor(QObject *parent)
    : QIODevice(parent), m_device(nullptr), m_blockSize(6), m_verbosity(0), m_workFactor(30),
      m_small(0), m_state(BZ_OK), m_buffer((char*)malloc(CHUNK)), m_end(false)
{

}

BZCompressor::BZCompressor(QIODevice *device, QObject *parent)
    : QIODevice(parent), m_device(device), m_blockSize(6), m_verbosity(0), m_workFactor(30),
      m_small(0), m_state(BZ_OK), m_buffer((char*)malloc(CHUNK)), m_end(false)
{

}

BZCompressor::~BZCompressor()
{
    close();
}

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
                m_state = compress(0, 0, BZ_FINISH);
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
    m_strm.avail_out = length;
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
            m_strm.avail_in = avail;
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
    //if (!src->open(QIODevice::ReadOnly) || !dest->open(QIODevice::WriteOnly))
    //    return BZ_IO_ERROR;

    do
    {
        qint64 avail = src->read(in, CHUNK);
        if (avail < 0)
        {
            BZ2_bzDecompressEnd(&strm);
            return BZ_IO_ERROR;
        }
        strm.avail_in = avail;

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
        m_state = compress((char*)data, len, BZ_RUN);
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
    m_strm.avail_in = length;
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
    qint64 have;
    char in[CHUNK];
    char out[CHUNK];

    bz_stream strm;
    ret = compressInit(&strm, blockSize, verbosity, workFactor);
    if (ret != BZ_OK)
        return ret;
    //if (!src->open(QIODevice::ReadOnly) || !dest->open(QIODevice::WriteOnly))
    //    return BZ_IO_ERROR;

    do
    {
        qint64 avail = src->read(in, CHUNK);
        if (avail < 0)
        {
            BZ2_bzCompressEnd(&strm);
            return BZ_IO_ERROR;
        }
        strm.avail_in = avail;

        action = src->atEnd() ? BZ_FINISH : BZ_RUN;
        strm.next_in = in;

        do
        {
            strm.avail_out = CHUNK;
            strm.next_out = out;

            ret = BZ2_bzCompress(&strm, action);
            Q_ASSERT(ret != BZ_SEQUENCE_ERROR);

            have = CHUNK - strm.avail_out;
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
    strm->next_in = 0;

    strm->avail_out = 0;
    strm->total_out_lo32 = 0;
    strm->total_out_hi32 = 0;
    strm->next_out = 0;

    strm->bzalloc = 0;
    strm->bzfree = 0;
    strm->opaque = 0;
}