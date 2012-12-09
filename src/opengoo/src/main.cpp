/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QRect>
#include <QDebug>
#include <QTime>
#include <QXmlStreamWriter>
#include <cstdio>

#ifndef Q_OS_WIN32
#include "backtracer.h"
#endif
#ifdef Q_OS_WIN32
#include "backtracer_win32.h"
#endif

#include "flags.h"
#include "mainwidget.h"

#include "soundsystem.h"
#include "publicclass.h"

#include <logger.h>
#include <consoleappender.h>

struct OGGameConfig
{
    int screen_width;
    int screen_height;
    QString language;
    bool fullscreen;
};

void gooMessageHandler(QtMsgType, const QMessageLogContext &, const QString&);
void ReadConfig(OGGameConfig* config);

static const QString GAMEDIR = QDir::homePath() + "/.OpenGOO";

int main(int argc, char *argv[])
{
    #ifndef Q_OS_WIN32
    BackTracer(SIGSEGV);
    BackTracer(SIGFPE);
    #endif
    #ifdef Q_OS_WIN32
    AddVectoredExceptionHandler(0, UnhandledException2);
    #endif

    ConsoleAppender * con_apd;
    con_apd  = new ConsoleAppender(LoggerEngine::LevelInfo,stdout,"%d - <%l> - %m%n");
    LoggerEngine::addAppender(con_apd);
    con_apd  = new ConsoleAppender(LoggerEngine::LevelDebug     |
                                   LoggerEngine::LevelWarn      |
                                   LoggerEngine::LevelCritical  |
                                   LoggerEngine::LevelError     |
                                   LoggerEngine::LevelException |
                                   LoggerEngine::LevelFatal,stdout,"%d - <%l> - %m [%f:%i]%n");
    LoggerEngine::addAppender(con_apd);
    qInstallMessageHandler(gooMessageHandler);

    //intialize randseed
    qsrand(QTime::currentTime().toString("hhmmsszzz").toUInt());

    //Check for the run parameters
    for (int i=1; i<argc; i++) {
        QString arg(argv[i]);
        //Check for debug Option
        if (!arg.compare("-debug", Qt::CaseInsensitive)) {
            flag |= DEBUG;
            logWarn("DEBUG MODE ON");
        }
        else if (!arg.compare("-opengl", Qt::CaseInsensitive)) {
            flag |= OPENGL | FPS;
        }
        else if (!arg.compare("-text", Qt::CaseInsensitive)) {
            flag |= ONLYTEXT | DEBUG;
        }
        else if (!arg.compare("-fps",Qt::CaseInsensitive)){
            flag |= FPS;
        }
    }
    if (flag == STANDARD) logWarn("STD MODE");
    if (flag & OPENGL) {
        logWarn("OPENGL ACTIVATED");
        //QApplication::setGraphicsSystem("opengl");
    }


    QApplication a(argc, argv);

    //CHECK FOR GAME DIR IN HOME DIRECTORY
    QDir dir(GAMEDIR);
    //If the game dir doesn't exist create it
    if (!dir.exists()) {
        if (flag & DEBUG) logWarn("Game dir doesn't exist!");
        dir.mkdir(GAMEDIR);
        dir.cd(GAMEDIR);
        //create subdir for user levels and progressions.
        dir.mkdir("userLevels");
        dir.mkdir("userProgression");
        dir.mkdir("debug");
    }
    else if (flag & DEBUG) logWarn("Game dir exist!");

    OGGameConfig config;

    ReadConfig(&config);

    MainWidget w(QRect(50, 50, config.screen_width, config.screen_height));

    if (config.fullscreen) { w.setWindowState(Qt::WindowFullScreen); }

    w.show();
    return a.exec();
}

void gooMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

void ReadConfig(OGGameConfig* config)
{
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGTH = 600;
    QDomDocument domDoc;
// Default settings
    config->screen_width = SCREEN_WIDTH;
    config->screen_height = SCREEN_HEIGTH;
    config->fullscreen = true;

    QFile file("./resources/config.xml");

    if (file.open(QIODevice::ReadOnly))
    {
        if (domDoc.setContent(&file))
        {
            QDomElement domElement = domDoc.documentElement();

            if (domElement.tagName() != "config")
            {
                logWarn("File config.xml is corrupted");
                file.close();

                return;
            }

            for(QDomNode n = domElement.firstChild(); !n.isNull(); n = n.nextSibling())
            {
                QDomElement domElement = n.toElement();

                if (domElement.tagName() == "param")
                {
                    QString attribute = domElement.attribute("name", "");

                    if (attribute == "screen_width")
                    {
                        config->screen_width = domElement.attribute("value", "").toInt();
                    }
                    else if (attribute == "screen_height")
                    {
                        config->screen_height = domElement.attribute("value", "").toInt();
                    }
                    else if (attribute == "language")
                    {
                        config->language = domElement.attribute("value", "");
                    }
                    else if (attribute == "fullscreen")
                    {
                        if (domElement.attribute("value", "") == "true")
                        {
                            config->fullscreen = true;
                        }
                        else { config->fullscreen = false; }
                    }
                }
            }
        }
        else
        {
            logWarn("File config.xml has incorrect xml format");
        }

        file.close();
    }
    else
    {
        logWarn("File config.xml not found");
        file.open(QIODevice::WriteOnly);
        QString xmlData;
        QTextStream out(&file);
        QXmlStreamWriter stream(&xmlData);
        stream.setAutoFormatting(true);
        stream.writeStartDocument();
        stream.writeStartElement("config");

        stream.writeStartElement("param");
        stream.writeAttribute("name", "screen_width");
        stream.writeAttribute("value",  QString::number(SCREEN_WIDTH));
        stream.writeEndElement(); // end screen_width

        stream.writeStartElement("param");
        stream.writeAttribute("name", "screen_height");
        stream.writeAttribute("value",  QString::number(SCREEN_HEIGTH));
        stream.writeEndElement(); // end screen_height

        stream.writeStartElement("param");
        stream.writeAttribute("name", "fullscreen");

        if (config->fullscreen) { stream.writeAttribute("value",  "true"); }
        else { stream.writeAttribute("value",  "false"); }

        stream.writeEndElement(); // end fullscreen

        stream.writeEndElement(); // end config

        stream.writeEndDocument();
        out << xmlData;
        file.close();
    }
}
