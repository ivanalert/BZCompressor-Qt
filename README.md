Compression library based on zlib with QIODevice interface.

Works like all QIODevice objects. Compresses when write to device and decompresses when read from
device.

Not QIODevice public members:

void setBlockSize(int value) - sets compression block size 1-9, more info in bzip2 documentation.

int blockSize() const - gets block size.

void setVerbosity(int value) - sets verbosity level 0-4.

int verbosity() const - gets verbosity level.

void setWorkFactor(int value) - sets compression work factor 0-250, more info in bzip2
documentation.

int workFactor() const - gets compression work factor.

void setSmall(int value) - sets decompression small, can be zero or non zero value. More
information in bzip2 documentation.

int state() const - gets compression/decompression state BZ_OK, BZ_STREAM_END etc. More info in
bzip2 documentation.

Not QIODevice public static members:

int compress(QIODevice *src, QIODevice *dest, int blockSize, int verbosity, int workFactor) -
compress src to dest at a time, with a certain blockSize, verbosity and workFactor.

int decompress(QIODevice *src, QIODevice *dest, int verbosity, int small) - decompress src to dest
at a time, with a certain verbosity and small.

Build static library and cli programm in linux:

Install bzip2 library dev package. In Ubuntu libbz2-dev.
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ../
make
