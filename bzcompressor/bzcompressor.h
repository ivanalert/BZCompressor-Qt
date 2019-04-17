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

#ifndef BZCOMPRESSOR_H
#define BZCOMPRESSOR_H

#include <QIODevice>
#include <bzlib.h>

class BZCompressor : public QIODevice
{
    Q_OBJECT

public:
    explicit BZCompressor(QObject *parent = nullptr)
        : QIODevice(parent), m_buffer(reinterpret_cast<char*>(malloc(CHUNK)))
    {

    }

    explicit BZCompressor(QIODevice *device, QObject *parent = nullptr)
        : QIODevice(parent), m_device(device), m_buffer(reinterpret_cast<char*>(malloc(CHUNK)))
    {

    }

    ~BZCompressor() override
    {
        close();
    }

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

    void setBlockSize(int value) noexcept
    {
        m_blockSize = value;
    }

    int blockSize() const noexcept
    {
        return m_blockSize;
    }

    void setVerbosity(int value) noexcept
    {
        m_verbosity = value;
    }

    int verbosity() const noexcept
    {
        return m_verbosity;
    }

    void setWorkFactor(int value) noexcept
    {
        m_workFactor = value;
    }

    int workFactor() const noexcept
    {
        return m_workFactor;
    }

    void setSmall(int value) noexcept
    {
        m_small = value;
    }

    int small() const noexcept
    {
        return m_small;
    }

    int state() const noexcept
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

    QIODevice *m_device{nullptr};
    bz_stream m_strm;
    int m_blockSize{6};
    int m_verbosity{0};
    int m_workFactor{30};
    int m_small{0};
    int m_state{BZ_OK};
    bool m_end{false};

    QScopedPointer<char, QScopedPointerPodDeleter> m_buffer;

    static void resetStream(bz_stream *strm);
    static int compressInit(bz_stream *strm, int blockSize, int verbosity, int workFactor);
    static int decompressInit(bz_stream *strm, int verbosity, int small);

    static const unsigned CHUNK{16384};
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
