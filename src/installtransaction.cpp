////////////////////////////////////////////////////////////////////////
// FILE:        installtransaction.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <unistd.h>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <list>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
using namespace std;


#include "installtransaction.h"
#include "repository.h"
#include "pkgdb.h"
#include "stringhelper.h"
#include "argparser.h"
#include "process.h"
#include "configuration.h"

#ifdef USE_LOCKING
#include "lockfile.h"
#endif

using namespace StringHelper;


const string InstallTransaction::PKGMK_DEFAULT_COMMAND =  "/usr/bin/pkgmk";
const string InstallTransaction::PKGADD_DEFAULT_COMMAND = "/usr/bin/pkgadd";
const string InstallTransaction::PKGRM_DEFAULT_COMMAND =  "/usr/bin/pkgrm";

/*!
 Create a nice InstallTransaction
 \param names a list of port names to be installed
 \param repo the repository to look for packages
 \param pkgDB the pkgDB with already installed packages
*/
InstallTransaction::InstallTransaction( const list<string>& names,
                                        const Repository* repo,
                                        PkgDB* pkgDB,
                                        const Configuration* config )
    : m_repo( repo ),
      m_pkgDB( pkgDB ),
      m_depCalced( false ),
      m_config( config )
{
    list<string>::const_iterator it = names.begin();
    for ( ; it != names.end(); ++it ) {
        m_packages.push_back( make_pair( *it, m_repo->getPackage( *it ) ) );
    }

}

/*!
 Create a nice InstallTransaction
 \param names a list of port names to be installed
 \param repo the repository to look for packages
 \param pkgDB the pkgDB with already installed packages
*/
InstallTransaction::InstallTransaction( const list<char*>& names,
                                        const Repository* repo,
                                        PkgDB* pkgDB,
                                        const Configuration* config )
    : m_repo( repo ),
      m_pkgDB( pkgDB ),
      m_depCalced( false ),
      m_config( config )
{
    list<char*>::const_iterator it = names.begin();
    for ( ; it != names.end(); ++it ) {
        m_packages.push_back( make_pair( *it, m_repo->getPackage( *it ) ) );
    }

}



/*!
 Create a nice InstallTransaction
 \param names a list of port names to be installed
 \param repo the repository to look for packages
 \param pkgDB the pkgDB with already installed packages
*/
InstallTransaction::InstallTransaction( const string& name,
                                        const Repository* repo,
                                        PkgDB* pkgDB,
                                        const Configuration* config )
    : m_repo( repo ),
      m_pkgDB( pkgDB ),
      m_depCalced( false ),
      m_config( config )
{
    m_packages.push_back( make_pair( name, m_repo->getPackage( name ) ) );

}

/*!
  \return packages where building/installation failed
*/
const list< pair<string, InstallTransaction::InstallInfo> >&
InstallTransaction::installError() const
{
    return m_installErrors;
}

/*!
  install (commit) a transaction
  \param parser the argument parser
  \param update whether this is an update operation
  \param group whether this is a group transaction (stops transaction on error)
  \return returns an InstallResult telling whether installation worked
*/
InstallTransaction::InstallResult
InstallTransaction::install( const ArgParser* parser,
                             bool update, bool group )
{
    if ( m_packages.empty() ) {
        return NO_PACKAGE_GIVEN;
    }

    list<string> ignoredPackages;
    StringHelper::split(parser->ignore(), ',', ignoredPackages);

    list< pair<string, const Package*> >::iterator it = m_packages.begin();
    for ( ; it != m_packages.end(); ++it ) {
        const Package* package = it->second;

        if (find(ignoredPackages.begin(),
                 ignoredPackages.end(),
                 it->first) != ignoredPackages.end() ) {
            m_ignoredPackages.push_back(it->first);
            continue;
        }

        if ( package == NULL ) {
            m_missingPackages.push_back( make_pair( it->first, string("") ) );
            if ( group ) {
                return PACKAGE_NOT_FOUND;
            }
            continue;
        }

        // consider aliases here, but don't show them specifically
        if ( !update && m_pkgDB->isInstalled( package->name(), true ) ) {
            // ignore
            m_alreadyInstalledPackages.push_back( package->name() );
            continue;
        }

        InstallTransaction::InstallResult result;
        InstallInfo info( package->hasReadme() );
        if ( parser->isTest() ||
             (result = installPackage( package, parser, update, info )) == SUCCESS) {

            m_installedPackages.push_back( make_pair( package->name(), info));
        } else {

            // log failures are critical
            if ( result == LOG_DIR_FAILURE ||
                 result == LOG_FILE_FAILURE ||
                 result == NO_LOG_FILE ||
                 result == CANT_LOCK_LOG_FILE ||

                 // or pkgdest
                 result == PKGDEST_ERROR ) {
                return result;
            }

            m_installErrors.push_back( make_pair(package->name(), info) );
            if ( group ) {
                return PKGMK_FAILURE;
            }
        }
    }

    return SUCCESS;
}

/*!
  Install a single package
  \param package the package to be installed
  \param parser the argument parser to be used
  \param update whether this is an update transaction
  \param info store pre and post install information
*/
InstallTransaction::InstallResult
InstallTransaction::installPackage( const Package* package,
                                    const ArgParser* parser,
                                    bool update,
                                    InstallTransaction::InstallInfo& info )
    const
{

    InstallTransaction::InstallResult result = SUCCESS;
#ifdef USE_LOCKING
    LockFile lockFile;
#endif

    int fdlog = -1;
    string logFile = "";
    string timestamp;

    string commandName = "prt-get";
    if ( parser->wasCalledAsPrtCached() ) {
        commandName = "prt-cache";
    }

    // - initial information about the package to be build
    string message;
    message = commandName + ": ";
    if (update) {
        message += "updating ";
    } else {
        message += "installing ";
    }
    message += package->path() + "/" + package->name();
    cout << message << endl;

    if ( m_config->writeLog() ) {
        logFile = m_config->logFilePattern();
        if ( logFile == "" ) {
            return NO_LOG_FILE;
        }

        StringHelper::replaceAll( logFile, "%n", package->name() );
        StringHelper::replaceAll( logFile, "%p", package->path() );
        StringHelper::replaceAll( logFile, "%v", package->version() );
        StringHelper::replaceAll( logFile, "%r", package->release() );

#ifdef USE_LOCKING
        lockFile.setFile( logFile );
        if ( !lockFile.lockWrite() ) {
            cout << "here" << logFile << endl;
            return CANT_LOCK_LOG_FILE;
        }
#endif

        size_t pos = logFile.find_last_of( "/" );
        if ( pos != string::npos ) {
            if ( !Repository::createOutputDir( logFile.substr( 0, pos ) ) ) {
                return LOG_DIR_FAILURE;
            }
        }

        if ( !m_config->appendLog() ) {
            unlink( logFile.c_str() );
        }

        fdlog = open( logFile.c_str(),
		          O_APPEND | O_WRONLY | O_CREAT, 0666 );

        if ( fdlog == -1 ) {
            return LOG_FILE_FAILURE;
        }

        write( fdlog, message.c_str(), message.length());
        write( fdlog, "\n", 1);

        time_t startTime;
        time(&startTime);
        timestamp = ctime(&startTime);
        timestamp = commandName + ": starting build " + timestamp;
        write( fdlog, timestamp.c_str(), timestamp.length());
    }

    string pkgdir = package->path() + "/" + package->name();
    chdir( pkgdir.c_str() );

    string runscriptCommand = "sh";
    if (m_config->runscriptCommand() != "") {
        runscriptCommand = m_config->runscriptCommand();
    }

    // -- pre-install
    struct stat statData;
    if ((parser->execPreInstall() || m_config->runScripts()) &&
        stat((pkgdir + "/" + "pre-install").c_str(), &statData) == 0) {
        Process preProc( runscriptCommand,
                         pkgdir + "/" + "pre-install",
                         fdlog );
        if (preProc.executeShell()) {
            info.preState = FAILED;
        } else {
            info.preState = EXEC_SUCCESS;
        }
    }

    // -- build
    string cmd = PKGMK_DEFAULT_COMMAND;
    if (m_config->makeCommand() != "") {
        cmd = m_config->makeCommand();
    }

    string args = "-d " + parser->pkgmkArgs();
    Process makeProc( cmd, args, fdlog );
    if ( makeProc.executeShell() ) {
        result = PKGMK_FAILURE;
    } else {
        // -- update
        string pkgdest = getPkgmkPackageDir();
        if ( pkgdest != "" ) {
            // TODO: don't manipulate pkgdir
            pkgdir = pkgdest;
            string message = "prt-get: Using PKGMK_PACKAGE_DIR: " + pkgdir;
            if (parser->verbose() > 0) {
                cout << message << endl;
            }
            if ( m_config->writeLog() ) {
                write( fdlog, message.c_str(), message.length() );
                write( fdlog, "\n", 1 );
            }
        }

        // the following chdir is a noop if usePkgDest() returns false
        if ( chdir( pkgdir.c_str() ) != 0 ) {
            result = PKGDEST_ERROR;
        } else {
            cmd = PKGADD_DEFAULT_COMMAND;
            if (m_config->addCommand() != "") {
                cmd = m_config->addCommand();
            }

            args = "";
            if (parser->installRoot() != "") {
                args = "-r " + parser->installRoot() + " ";
            }


            if ( update ) {
                args += "-u ";
            }
            if ( !parser->pkgaddArgs().empty() ) {
                args += parser->pkgaddArgs() + " ";
            }
            args +=
                package->name()    + "#" +
                package->version() + "-" +
                package->release() + ".pkg.tar." + getPkgmkCompressionMode();


            // - inform the user about what's happening
            string fullCommand = commandName + ": " + cmd + " " + args;
            string summary;
            if (update) {
                string from = m_pkgDB->getPackageVersion(package->name());
                string to = package->version() + "-" + package->release();
                if (from ==  to) {
                    summary = commandName + ": " + "reinstalling " +
                        package->name() + " " + to;
                } else {
                    summary = commandName + ": " + "updating " +
                        package->name() + " from " + from + " to " + to;
                }
            } else {
                summary = commandName + ": " + "installing " +
                    package->name() + " " +
                    package->version() + "-" + package->release();
            }

            // - print and log
            cout << summary << endl;
            if (parser->verbose() > 0) {
                cout << fullCommand << endl;
            }
            if ( m_config->writeLog() ) {
                time_t endTime;
                time(&endTime);
                timestamp = ctime(&endTime);
                timestamp = commandName + ": build done " + timestamp;

                write( fdlog, summary.c_str(), summary.length() );
                write( fdlog, "\n", 1 );
                write( fdlog, fullCommand.c_str(), fullCommand.length() );
                write( fdlog, "\n", 1 );
                write( fdlog, timestamp.c_str(), timestamp.length());
                write( fdlog, "\n", 1 );
            }

            Process installProc( cmd, args, fdlog );
            if ( installProc.executeShell() ) {
                result = PKGADD_FAILURE;
            } else {
                // exec post install
                if ((parser->execPostInstall()  || m_config->runScripts() ) &&
                    stat((package->path() + "/" + package->name() +
                          "/" + "post-install").c_str(), &statData)
                    == 0) {
                    // Work around the pkgdir variable change
                    Process postProc( runscriptCommand,
                                      package->path() + "/" + package->name()+
                                      "/" + "post-install",
				      fdlog );
                    if (postProc.executeShell()) {
                        info.postState = FAILED;
                    } else {
                        info.postState = EXEC_SUCCESS;
                    }
                }
            }
        }
    }

    if ( m_config->writeLog() ) {

#ifdef USE_LOCKING
        lockFile.unlock();
#endif

        // Close logfile
        close ( fdlog );

        if (m_config->removeLogOnSuccess() && !m_config->appendLog() &&
            result == SUCCESS) {
            unlink(logFile.c_str());
        }
    }
    return result;
}

/*!
  Calculate dependencies for this transaction
  \return true on success
*/
bool InstallTransaction::calculateDependencies()
{
    if ( m_depCalced ) {
        return true;
    }
    m_depCalced = true;
    if ( m_packages.empty() ) {
        return false;
    }

    list< pair<string, const Package*> >::const_iterator it =
        m_packages.begin();
    for ( ; it != m_packages.end(); ++it ) {
        const Package* package = it->second;
        if ( package ) {
            checkDependecies( package );
        }
    }
    list<int> indexList;
    if ( ! m_resolver.resolve( indexList ) ) {
        m_depCalced = false;
        return false;
    }

    list<int>::iterator lit = indexList.begin();
    for ( ; lit != indexList.end(); ++lit ) {
        m_depNameList.push_back( m_depList[*lit] );
    }

    return true;
}

/*!
  recursive method to calculate dependencies
  \param package package for which we want to calculate dependencies
  \param depends index if the package \a package depends on (-1 for none)
*/
void InstallTransaction::checkDependecies( const Package* package,
                                           int depends )
{
    int index = -1;
    bool newPackage = true;
    for ( unsigned int i = 0; i < m_depList.size(); ++i ){
        if ( m_depList[i] == package->name() ) {
            index = i;
            newPackage = false;
            break;
        }
    }


    if ( index == -1 ) {
        index = m_depList.size();
        m_depList.push_back( package->name() );
    }

    if ( depends != -1 ) {
        m_resolver.addDependency( index, depends );
    } else {
        // this just adds index to the dependency resolver
        m_resolver.addDependency( index, index );
    }

    if ( newPackage ) {
        if ( !package->dependencies().empty() ) {
            list<string> deps;
            split( package->dependencies(), ',', deps );
            list<string>::iterator it = deps.begin();
            for ( ; it != deps.end(); ++it ) {
                string dep = *it;
                if ( !dep.empty() ) {
                    string::size_type pos = dep.find_last_of( '/' );
                    if ( pos != string::npos && (pos+1) < dep.length() ) {
                        dep = dep.substr( pos + 1 );
                    }
                    const Package* p = m_repo->getPackage( dep );
                    if ( p ) {
                        checkDependecies( p, index );
                    } else {
                        m_missingPackages.
                            push_back( make_pair( dep, package->name() ) );
                    }
                }
            }
        }
    }
}


/*!
  This method returns a list of packages which should be installed to
  meet the requirements for the packages to be installed. Includes
  the packages to be installed. The packages are in the correct order,
  packages to be installed first come first :-)

  \return a list of packages required for the transaction
*/
const list<string>& InstallTransaction::dependencies() const
{
    return m_depNameList;
}


/*!
  This method returns a list of packages which could not be installed
  because they could not be found in the ports tree. The return value is
  a pair, \a pair.first is package name and \a pair.second is the package
  requiring \a pair.first.

  \return packages missing in the ports tree
*/
const list< pair<string, string> >& InstallTransaction::missing() const
{
    return m_missingPackages;
}


/*!
  \return packages which were requested to be installed but are already
  installed
*/
const list<string>& InstallTransaction::alreadyInstalledPackages() const
{
    return m_alreadyInstalledPackages;
}


/*!
  \return the packages which were installed in this transaction
*/
const list< pair<string, InstallTransaction::InstallInfo> >&
InstallTransaction::installedPackages() const
{
    return m_installedPackages;
}


/*!
  calculate dependendencies for this package
*/
InstallTransaction::InstallResult
InstallTransaction::calcDependencies( )
{
    if ( m_packages.empty() ) {
        return NO_PACKAGE_GIVEN;
    }

    bool validPackages = false;
    list< pair<string, const Package*> >::iterator it = m_packages.begin();
    for ( ; it != m_packages.end(); ++it ) {
        if ( it->second ) {
            validPackages = true;
        } else {
            // Note: moved here from calculateDependencies
            m_missingPackages.push_back( make_pair( it->first, string("") ) );
        }
    }
    if ( !validPackages ) {
        return PACKAGE_NOT_FOUND;
    }

    if ( !calculateDependencies() ) {
        return CYCLIC_DEPEND;
    }
    return SUCCESS;
}


/*
 * getPkgDest assumes that you're in the build directory already
 */
string InstallTransaction::getPkgmkSetting(const string& setting)
{
    string value = "";
    value = getPkgmkSettingFromFile(setting, "/etc/pkgmk.conf");
    if (value.size() == 0) {
        value = getPkgmkSettingFromFile(setting, "/usr/bin/pkgmk");
    }

    return value;
}

string InstallTransaction::getPkgmkSettingFromFile(const string& setting, const string& fileName)
{
    FILE* fp = fopen(fileName.c_str(), "r");
    if (!fp)
        return "";

    string candidate;
    string s;
    char line[256];
    while (fgets(line, 256, fp)) {
        s = StringHelper::stripWhiteSpace(line);
        if (StringHelper::startsWith(s, setting + "=")) {
            candidate = s;
        }
    }
    fclose(fp);

    string value = "";
    if (candidate.length() > 0) {
        string cmd = "eval " + candidate + " && echo $" + setting;
        FILE* p = popen(cmd.c_str(), "r");
        if (p) {
            fgets(line, 256, p);
            value = StringHelper::stripWhiteSpace(line);
            fclose(p);
        }
    }

    return value;
}

const list<string>& InstallTransaction::ignoredPackages() const
{
    return m_ignoredPackages;
}

string InstallTransaction::getPkgmkPackageDir()
{
    return getPkgmkSetting("PKGMK_PACKAGE_DIR");
}

string InstallTransaction::getPkgmkCompressionMode()
{
    string value = getPkgmkSetting("PKGMK_COMPRESSION_MODE");

    return value.size() ? value : "gz";
}
