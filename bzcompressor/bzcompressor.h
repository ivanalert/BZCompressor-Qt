#ifndef BZCOMPRESSOR_H
#define BZCOMPRESSOR_H

#include <QIODevice>
#include <bzlib.h>

class BZCompressor : public QIODevice
{
    Q_OBJECT

public:
    explicit BZCompressor(QObject *parent = nullptr);
    explicit BZCompressor(QIODevice *device, QObject *parent = nullptr);
    ~BZCompressor() override;

    // QIODevice interface
    bool isSequential() const override
    {
        return true;
    }

    bool open(OpenMode mode) override;
    void close() override;

    bool atEnd() const override
    {
        return m_end && QIODevice::atEnd();
    }

    qint64 bytesAvailable() const override
    {
        if (openMode() & QIODevice::ReadOnly)
        {
            qint64 result = QIODevice::bytesAvailable();
            if (result <= 0)
                result = m_strm.avail_in + m_device->bytesAvailable();
            return result;
        }

        return 0;
    }

    qint64 bytesToWrite() const override
    {
        if (openMode() & QIODevice::WriteOnly)
        {
            qint64 result = QIODevice::bytesToWrite();
            if (result <= 0)
                result = m_strm.avail_in + m_device->bytesToWrite();
            return result;
        }

        return 0;
    }

    void setBlockSize(int value)
    {
        m_blockSize = value;
    }

    int blockSize() const
    {
        return m_blockSize;
    }

    void setVerbosity(int value)
    {
        m_verbosity = value;
    }

    int verbosity() const
    {
        return m_verbosity;
    }

    void setWorkFactor(int value)
    {
        m_workFactor = value;
    }

    int workFactor() const
    {
        return m_workFactor;
    }

    void setSmall(int value)
    {
        m_small = value;
    }

    int small() const
    {
        return m_small;
    }

    int state() const
    {
        return m_state;
    }

    static int compress(QIODevice *src, QIODevice *dest, int blockSize, int verbosity,
                        int workFactor);
    static int decompress(QIODevice *src, QIODevice *dest, int verbosity, int small);

protected:
    // QIODevice interface
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    int compress(char *data, qint64 length, int action);
    int decompress(char *data, qint64 length, qint64 &have);

    QIODevice *m_device;
    bz_stream m_strm;
    int m_blockSize;
    int m_verbosity;
    int m_workFactor;
    int m_small;
    int m_state;
    bool m_end;

    QScopedPointer<char, QScopedPointerPodDeleter> m_buffer;

    static void resetStream(bz_stream *strm);
    static int compressInit(bz_stream *strm, int blockSize, int verbosity, int workFactor);
    static int decompressInit(bz_stream *strm, int verbosity, int small);

    static const unsigned CHUNK;
};

inline int BZCompressor::compressInit(bz_stream *strm, int blockSize, int verbosity,
                                      int workFactor)
{
    resetStream(strm);
    return BZ2_bzCompressInit(strm, blockSize, verbosity, workFactor);
}

inline int BZCompressor::decompressInit(bz_stream *strm, int verbosity, int small)
{
    resetStream(strm);
    return BZ2_bzDecompressInit(strm, verbosity, small);
}

#endif // BZCOMPRESSOR_H
