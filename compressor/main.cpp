/*
    This program is free software: you can redistribute it and/or modify
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

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QString>
#include <iostream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("compressor"));

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("source"), QStringLiteral("Source file."));
    parser.addPositionalArgument(QStringLiteral("destination"),
                                 QStringLiteral("Destination file."));
    QCommandLineOption decmpOpt(QStringList{QStringLiteral("d"), QStringLiteral("decompress")},
                                QStringLiteral("Decompress."));
    parser.addOption(decmpOpt);
    QCommandLineOption vrbsOpt(QStringList{QStringLiteral("v"), QStringLiteral("verbosity")},
                               QStringLiteral("Verbosity."),
                               QStringLiteral("verbosity value 0-4"));
    parser.addOption(vrbsOpt);
    QCommandLineOption blszOpt(QStringList{QStringLiteral("b"), QStringLiteral("blocksize")},
                               QStringLiteral("Block size. Ignores when decompress."),
                               QStringLiteral("blocksize value 1-9"));
    parser.addOption(blszOpt);
    QCommandLineOption wrkfOpt(QStringList{QStringLiteral("w"), QStringLiteral("workfactor")},
                               QStringLiteral("Work factor. Ignores when decompress."),
                               QStringLiteral("workfactor value 0-250"));
    parser.addOption(wrkfOpt);
    QCommandLineOption smallOpt(QStringList{QStringLiteral("s"), QStringLiteral("small")},
                                QStringLiteral("Small. Ignores when compress."),
                                QStringLiteral("small value zero or not zero"));
    parser.addOption(smallOpt);
    parser.addHelpOption();
    parser.process(app);

    using namespace std;

    //Check file arguments.
    const QStringList args = parser.positionalArguments();
    if (args.size() < 2)
    {
        cout << "Too few arguments!" << endl;
        return 1;
    }

    //Verbosity check.
    bool ok = false;
    const QString vrbsValue = parser.value(vrbsOpt);
    int vrbs = vrbsValue.toInt(&ok);
    if (!ok)
    {
        if (!vrbsValue.isEmpty())
        {
            cout << "Invalid verbosity value! Must be from 0 to 4." << endl;
            return 1;
        }
        else
            vrbs = 0;
    }
    else if (vrbs < 0 || vrbs > 4)
    {
        cout << "Invalid verbosity value range! Must be from 0 to 4." << endl;
        return 1;
    }

    int blsz = -1;
    int wrkf = -1;
    int small = -1;
    const bool decmp = parser.isSet(decmpOpt);
    if (!decmp)
    {
        //Block size and work factor check if compress.
        QString val = parser.value(blszOpt);
        blsz = val.toInt(&ok);
        if (!ok)
        {
            if (!val.isEmpty())
            {
                cout << "Invalid block size value! Must be from 1 to 9." << endl;
                return 1;
            }
            else
                blsz = 6;
        }
        else if (blsz < 1 || blsz > 9)
        {
            cout << "Invalid block size value range! Must be from 1 to 9." << endl;
            return 1;
        }

        val = parser.value(wrkfOpt);
        wrkf = val.toInt(&ok);
        if (!ok)
        {
            if (!val.isEmpty())
            {
                cout << "Invalid work factor value! Must be from 0 to 250." << endl;
                return 1;
            }
            else
                wrkf = 30;
        }
        else if (wrkf < 0 || wrkf > 250)
        {
            cout << "Invalid work factor value range! Must be from 0 to 250." << endl;
            return 1;
        }
    }
    else
    {
        //Small check if decompress.
        const QString smallVal = parser.value(smallOpt);
        small = smallVal.toInt(&ok);
        if (!ok)
        {
            if (!smallVal.isEmpty())
            {
                cout << "Invalid small value! Must be zero or not zero." << endl;
                return 1;
            }
            else
                small = 0;
        }
    }

    //Open files.
    QFile src(args.at(0));
    QFile dest(args.at(1));
    if (!src.open(QIODevice::ReadOnly) || !dest.open(QIODevice::WriteOnly))
    {
        //Files closes in destructor if necessary.
        cout << "Can't open source or destination file!" << endl;
        return 1;
    }

    //Compress or decompress.
    int ret = 0;
    if (decmp)
        ret = BZCompressor::decompress(&src, &dest, vrbs, small);
    else
        ret = BZCompressor::compress(&src, &dest, blsz, vrbs, wrkf);

    src.close();
    dest.close();

    //Success or failed compression.
    if (ret == BZ_STREAM_END)
    {
        cout << "Success!" << endl;
        return 0;
    }
    else
    {
        cout << "Failed!" << endl;
        return 1;
    }
}
