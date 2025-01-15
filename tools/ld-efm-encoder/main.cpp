/************************************************************************

    main.cpp

    ld-efm-encoder - EFM data encoder
    Copyright (C) 2025 Simon Inns

    This file is part of ld-decode-tools.

    ld-process-efm is free software: you can redistribute it and/or
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
#include "efmencoder.h"

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
    // Positional argument to specify input WAV file
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Specify input WAV file"));

    // Positional argument to specify output audio file
    parser.addPositionalArgument("output", QCoreApplication::translate("main", "Specify output EFM file"));

    // Add the new optional argument for audio test data
    QCommandLineOption audio_test_data_option(QStringList() << "t" << "audio-testdata",
                                           QCoreApplication::translate("main", "Generate audio test data of (1 frame is 2x16-bit L&R samples)"),
                                           QCoreApplication::translate("main", "frames"));
    parser.addOption(audio_test_data_option);

    // Process the command line options and arguments given by the user
    parser.process(a);

    // Standard logging options
    processStandardDebugOptions(parser);

    // Get the filename arguments from the parser
    QString input_filename;
    QString output_filename;
    bool gen_audio_test_data = false;
    int32_t audio_test_frames = 0;
    QStringList positional_arguments = parser.positionalArguments();
    if (parser.isSet(audio_test_data_option)) {
        if (positional_arguments.count() != 1) {
            qWarning() << "You must specify the output EFM filename when using the audio-testdata option";
            return 1;
        }
        input_filename = "";
        output_filename = positional_arguments.at(0);
        gen_audio_test_data = true;
        audio_test_frames = parser.value(audio_test_data_option).toInt();
        if (audio_test_frames <= 0) {
            qWarning() << "Invalid frame size specified for audio-testdata - must be greater than 0";
            return 1;
        }
    } else {
        if (positional_arguments.count() != 2) {
            qWarning() << "You must specify the input WAV filename and the output EFM filename";
            return 1;
        }
        input_filename = positional_arguments.at(0);
        output_filename = positional_arguments.at(1);
    }

    // Perform the processing
    qInfo() << "Beginning EFM encoding of" << (parser.isSet(audio_test_data_option) ? "test data" : input_filename);
    EfmEncoder efm_encoder;

    if (!efm_encoder.encode(input_filename, output_filename, gen_audio_test_data, audio_test_frames)) {
        return 1;
    }

    // Quit with success
    return 0;
}