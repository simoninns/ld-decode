/************************************************************************

    main.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-efm-encoder is free software: you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

************************************************************************/

#include <QCoreApplication>
#include <QDebug>
#include <QtGlobal>
#include <QCommandLineParser>
#include <QThread>

#include "logging.h"
#include "efm_processor.h"

int main(int argc, char *argv[])
{
    // Set 'binary mode' for stdin and stdout on windows
    setBinaryMode();
    // Install the local debug message handler
    setDebug(true);
    qInstallMessageHandler(debugOutputHandler);

    QCoreApplication a(argc, argv);

    // Set application name and version
    QCoreApplication::setApplicationName("ld-efm-encoder");
    QCoreApplication::setApplicationVersion(QString("Branch: %1 / Commit: %2").arg(APP_BRANCH, APP_COMMIT));
    QCoreApplication::setOrganizationDomain("domesday86.com");

    // Set up the command line parser
    QCommandLineParser parser;
    parser.setApplicationDescription(
        "ld-efm-encoder - EFM data encoder\n"
        "\n"
        "(c)2025 Simon Inns\n"
        "GPLv3 Open-Source - github: https://github.com/happycube/ld-decode");
    parser.addHelpOption();
    parser.addVersionOption();

    // Add the standard debug options --debug and --quiet
    addStandardDebugOptions(parser);

    // -- Positional arguments --
    // Positional argument to specify input data file
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Specify input audio data file"));

    // Positional argument to specify output audio file
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Specify output EFM file"));

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Standard logging options
    processStandardDebugOptions(parser);

    // Get the filename arguments from the parser
    QString input_filename;
    QString output_filename;

    QStringList positional_arguments = parser.positionalArguments();

    if (positional_arguments.count() != 2) {
        qWarning() << "You must specify the input data filename and the output EFM filename";
        return 1;
    }
    input_filename = positional_arguments.at(0);
    output_filename = positional_arguments.at(1);

    // Perform the processing
    EfmProcessor efm_processor;

    if (!efm_processor.process(input_filename, output_filename)) {
        return 1;
    }

    // Quit with success
    return 0;
}