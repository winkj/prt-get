////////////////////////////////////////////////////////////////////////
// FILE:        configuration.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_

#include <string>
#include <list>
#include <utility>

class ArgParser;

/*!
  Configuration file class
*/
class Configuration
{
public:
    Configuration( const std::string& configFile, const ArgParser* parser );
    bool parse();

    bool writeLog() const;
    bool appendLog() const;
    bool removeLogOnSuccess() const;
    std::string logFilePattern() const;

    const std::list< std::pair<std::string, std::string> >& rootList() const;


    enum ReadmeMode { VERBOSE_README, COMPACT_README, NO_README };
    ReadmeMode readmeMode() const;

    std::string cacheFile() const;

    bool runScripts() const;
    bool preferHigher() const;
    bool useRegex() const;

    void addConfig(const std::string& line,
                   bool configSet,
                   bool configPrepend);

    std::string makeCommand() const;
    std::string addCommand() const;
    std::string removeCommand() const;
    std::string runscriptCommand() const;

private:
    std::string m_configFile;
    const ArgParser* m_parser;

    // config data
    std::string m_cacheFile;

    std::list< std::pair<std::string, std::string> > m_rootList;

    std::string m_logFilePattern;
    bool m_writeLog;
    bool m_appendLog;
    bool m_removeLogOnSuccess;

    ReadmeMode m_readmeMode;

    bool m_runScripts;
    bool m_preferHigher;
    bool m_useRegex;

    std::string m_makeCommand;
    std::string m_addCommand;
    std::string m_removeCommand;
    std::string m_runscriptCommand;


    void parseLine(const std::string& line, bool prepend=false);
};

#endif /* _CONFIGURATION_H_ */
