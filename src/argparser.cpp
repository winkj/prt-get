////////////////////////////////////////////////////////////////////////
// FILE:        argparser.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

using namespace std;

#include "argparser.h"

/*!
  Construct a ArgParser object
  \param argc argument count
  \param argv argument vector
*/
ArgParser::ArgParser( int argc, char** argv )
    : m_isCommandGiven( false ),
      m_isForced( false ),
      m_isTest( false ),
      m_isAlternateConfigGiven( false ),
      m_useCache( false ),
      m_calledAsPrtCache( false ),
      m_alternateConfigFile( "" ),
      m_pkgmkArgs( "" ),
      m_pkgaddArgs( "" ),
      m_pkgrmArgs( "" ),
      m_installRoot( "" ),
      m_ignore( "" ),
      m_argc( argc ),
      m_argv( argv ),
      m_verbose( 0 ),
      m_writeLog( false ),
      m_hasFilter( false ),
      m_noStdConfig( false ),
      m_nodeps( false ),
      m_all( false ),
      m_printPath( false ),
      m_execPreInstall( false ),
      m_execPostInstall( false ),
      m_preferHigher( false ),
      m_strictDiff( false ),
      m_useRegex(false),
      m_fullPath(false),
      m_recursive(false),
      m_printTree(false),
      m_depSort(false)
{
}


/*!
  \return true if an alternate configuration file is given
*/
bool ArgParser::isAlternateConfigGiven() const
{
    return m_isAlternateConfigGiven;
}


/*!
  \return true if a command is given
*/
bool ArgParser::isCommandGiven() const
{
    return m_isCommandGiven;
}


/*!
  \return a list of arguments not processed by ArgParser
*/
const list<char*>& ArgParser::otherArgs() const
{
    return m_otherArgs;
}


/*!
  \return what command was given
*/
ArgParser::Type ArgParser::commandType() const
{
    return m_commandType;
}


/*!
  \return addtional arguments to pkgmk
*/
const string& ArgParser::pkgmkArgs() const
{
    return m_pkgmkArgs;
}


/*!
  \return addtional arguments to pkgadd
*/
const string& ArgParser::pkgaddArgs() const
{
    return m_pkgaddArgs;
}


/*!
  \return the name of the alternative configuration file
*/
const string& ArgParser::alternateConfigFile() const
{
    return m_alternateConfigFile;
}


/*!
  parse the arguments
  \return true on success
*/
bool ArgParser::parse()
{
    const int commandCount = 35;
    string commands[commandCount] = { "list", "search", "dsearch",
                                      "info",
                                      "depends", "install", "depinst",
                                      "help", "isinst", "dup", "update",
                                      "quickdep", "diff", "quickdiff",
                                      "grpinst", "version", "cache",
                                      "path", "listinst", "printf", "readme",
                                      "dependent", "sysup", "current",
                                      "fsearch", "lock", "unlock",
                                      "listlocked", "cat", "ls", "edit",
                                      "remove", "deptree", "dumpconfig",
                                      "listorphans" };

    Type commandID[commandCount] = { LIST, SEARCH, DSEARCH, INFO,
                                     DEPENDS, INSTALL, DEPINST,
                                     HELP, ISINST, DUP, UPDATE,
                                     QUICKDEP, DIFF, QUICKDIFF,
                                     GRPINST, SHOW_VERSION, CREATE_CACHE,
                                     PATH, LISTINST, PRINTF, README,
                                     DEPENDENT, SYSUP, CURRENT,
                                     FSEARCH, LOCK, UNLOCK, LISTLOCKED,
                                     CAT, LS, EDIT, REMOVE, DEPTREE,
                                     DUMPCONFIG, LISTORPHANS };
    if ( m_argc < 2 ) {
        return false;
    }

    // if called from a symlink ending on prt-cache, use cached
    // access
    string app = m_argv[0];
    string::size_type pos = app.rfind( "/" );
    if ( pos != string::npos ) {
        app = app.substr( pos );
    }
    if ( app.find( "prt-cache" ) != string::npos ) {
        m_useCache = true;
        m_calledAsPrtCache = true;
    }

    for ( int i = 1; i < m_argc; ++i ) {
        if ( m_argv[i][0] == '-' ) {
            string s = m_argv[i];
            if ( s == "-v" ) {
                m_verbose += 1;
            } else if ( s == "-vv" ) {
                m_verbose += 2;
            } else if ( s == "--force" ) {
                m_isForced = true;
            } else if ( s == "--test" ) {
                m_isTest = true;
            } else if ( s == "--cache" ) {
                m_useCache = true;
            } else if ( s == "--nodeps" ) {
                m_nodeps = true;
            } else if ( s == "--all" ) {
                m_all = true;
            } else if ( s == "--path" ) {
                m_printPath = true;
            } else if ( s == "--log" ) {
                m_writeLog = true;
            } else if ( s == "--pre-install" ) {
                m_execPreInstall = true;
            } else if ( s == "--post-install" ) {
                m_execPostInstall = true;
            } else if ( s == "--install-scripts" ) {
                m_execPreInstall = true;
                m_execPostInstall = true;
            } else if ( s == "--no-std-config" ) {
                m_noStdConfig = true;
            } else if ( s == "--prefer-higher" || s == "-ph" ) {
                m_preferHigher = true;
            } else if ( s == "--strict-diff" || s == "-sd" ) {
                m_strictDiff = true;
            } else if ( s == "--regex" ) {
                m_useRegex = true;
            } else if ( s == "--full" ) {
                m_fullPath = true;
            } else if ( s == "--recursive" ) {
                m_recursive = true;
            } else if ( s == "--tree" ) {
                m_printTree = true;
            } else if ( s == "--depsort" ) {
                m_depSort = true;

            } else if ( s == "-f" ) {
                m_pkgaddArgs += " " + s;
            } else if ( s == "-fr" ) {
                m_pkgmkArgs += " -f";
            } else if ( s == "-if" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-uf" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-im" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-um" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-is" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-us" ) {
                m_pkgmkArgs += " " + s;
	    } else if ( s == "-kw" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-ns" ) {
                m_pkgmkArgs += " " + s;
            } else if ( s == "-fi" ) {
                m_pkgaddArgs += " -f";
            }

            // substrings
            else if ( s.substr( 0, 8 )  == "--margs=" ) {
                m_pkgmkArgs += " " + s.substr( 8 );
            } else if ( s.substr( 0, 8 ) == "--aargs=" ) {
                m_pkgaddArgs += " " + s.substr( 8 );
            } else if ( s.substr( 0, 8 ) == "--rargs=" ) {
                m_pkgrmArgs = s.substr( 8 );
            } else if ( s.substr( 0, 7 ) == "--sort=" ) {
                m_sortArgs = s.substr( 7 );
            } else if ( s.substr( 0, 9 ) == "--filter=" ) {
                m_filter = s.substr( 9 );
                m_hasFilter = true;
            } else if ( s.substr( 0, 9 ) == "--config=" ) {
                m_alternateConfigFile = s.substr( 9 );
                m_isAlternateConfigGiven = true;
            } else if ( s.substr( 0, 16 ) == "--config-append=" ) {
                m_configData.push_back(make_pair(m_argv[i]+16,
                                                 CONFIG_APPEND ) );
            } else if ( s.substr( 0, 17 ) == "--config-prepend=" ) {
                m_configData.push_back(make_pair(m_argv[i]+17,
                                                 CONFIG_PREPEND ) );
            } else if ( s.substr( 0, 13 ) == "--config-set=" ) {
                m_configData.push_back(make_pair(m_argv[i]+13, CONFIG_SET ) );
            } else if ( s.substr( 0, 15 ) == "--install-root=" ) {
                m_installRoot = s.substr(15);
            } else if ( s.substr( 0, 9 ) == "--ignore=" ) {
                m_ignore = s.substr(9);
            } else {
                m_unknownOption = s;
                return false;
            }
        } else {
            if (!m_isCommandGiven) {
                string s = m_argv[i];
                m_commandName = s;
                for ( int i = 0; i < commandCount; ++i ) {
                    if ( s == commands[i] ) {
                        m_isCommandGiven = true;
                        m_commandType = commandID[i];
                        break;
                    }
                }
                // first argument must be command
                if ( !m_isCommandGiven ) {
                    return false;
                }
            } else {
                m_otherArgs.push_back( m_argv[i] );
            }
        }
    }



    return m_isCommandGiven;
}


/*!
  \return true whether --force has been specified
*/
bool ArgParser::isForced() const
{
    return m_isForced;
}


/*!
  \return true whether --test has been specified
*/
bool ArgParser::isTest() const
{
    return m_isTest;
}


/*!
  \return the level of verbose: -v -> 1, -vv -> 2
*/
int ArgParser::verbose() const
{
    return m_verbose;
}


/*!
  \return whether --cache has been specified
*/
bool ArgParser::useCache() const
{
    return m_useCache;
}


/*!
  \return whether prt-get was called as 'prt-cache' or not
*/
bool ArgParser::wasCalledAsPrtCached() const
{
    return m_calledAsPrtCache;
}

/*!
  \return whether prt-get should write to a log file or not
*/
bool ArgParser::writeLog() const
{
    return m_writeLog;
}

/*!
  \return the --sort="..." string
*/
const string& ArgParser::sortArgs() const
{
    return m_sortArgs;
}

/*!
  \return whether there was a --filter argument
*/
bool ArgParser::hasFilter() const
{
    return m_hasFilter;
}


/*!
  \return whether there was a --no-std-config argument
 */
bool ArgParser::noStdConfig() const
{
    return m_noStdConfig;
}


/*!
  \return the --filter="..." string
*/
const string& ArgParser::filter() const
{
    return m_filter;
}

/*!
  \return whether there was a --nodeps argument
*/
bool ArgParser::nodeps() const
{
    return m_nodeps;
}

/*!
  \return whether there was a --all argument
*/
bool ArgParser::all() const
{
    return m_all;
}

bool ArgParser::printPath() const
{
    return m_printPath;
}

bool ArgParser::recursive() const
{
    return m_recursive;
}

bool ArgParser::printTree() const
{
    return m_printTree;
}

bool ArgParser::depSort() const
{
    return m_depSort;
}

const string& ArgParser::commandName() const
{
    return m_commandName;
}

const string& ArgParser::unknownOption() const
{
    return m_unknownOption;
}

bool ArgParser::execPreInstall() const
{
    return m_execPreInstall;
}

bool ArgParser::execPostInstall() const
{
    return m_execPostInstall;
}

const list< pair<char*, ArgParser::ConfigArgType> >
ArgParser::configData() const
{
    return m_configData;
}

const string& ArgParser::installRoot() const
{
    return m_installRoot;
}

const string& ArgParser::pkgrmArgs() const
{
    return m_pkgrmArgs;
}

bool ArgParser::preferHigher() const
{
    return m_preferHigher;
}

bool ArgParser::strictDiff() const
{
    return m_strictDiff;
}

bool ArgParser::useRegex() const
{
    return m_useRegex;
}

bool ArgParser::fullPath() const
{
    return m_fullPath;
}


const string& ArgParser::ignore() const
{
    return m_ignore;
}
