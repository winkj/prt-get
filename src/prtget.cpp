////////////////////////////////////////////////////////////////////////
// FILE:        prtget.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <algorithm>
#include <set>
#include <iomanip>
#include <cstdio>
#include <cassert>
using namespace std;

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "prtget.h"
#include "repository.h"
#include "argparser.h"
#include "installtransaction.h"
#include "configuration.h"

#include "stringhelper.h"
#include "versioncomparator.h"
#include "file.h"
#include "process.h"
#include "datafileparser.h"
using namespace StringHelper;


using VersionComparator::COMP_RESULT;
using VersionComparator::GREATER;
using VersionComparator::LESS;
using VersionComparator::EQUAL;
using VersionComparator::UNDEFINED;

const string PrtGet::CONF_FILE = SYSCONFDIR"/prt-get.conf";
const string PrtGet::DEFAULT_CACHE_FILE = LOCALSTATEDIR"/lib/pkg/prt-get.cache";

/*!
  Create a PrtGet object
  \param parser the argument parser to be used
*/
PrtGet::PrtGet( const ArgParser* parser )
    : m_repo( 0 ),
      m_config( 0 ),
      m_parser( parser ),
      m_cacheFile( DEFAULT_CACHE_FILE ),
      m_returnValue( PG_OK ),
      m_currentTransaction( 0 )
{
    if ( m_parser->wasCalledAsPrtCached() ) {
        m_appName = "prt-cache";
    } else {
        m_appName = "prt-get";
    }


    m_pkgDB = new PkgDB(m_parser->installRoot());
    readConfig();

    m_useRegex = m_config->useRegex() || m_parser->useRegex();
}

/*! destruct PrtGet object */
PrtGet::~PrtGet()
{
    if ( m_config ) {
        delete m_config;
    }
    if ( m_repo ) {
        delete m_repo;
    }

    delete m_pkgDB;
}


/*! print version and exit */
void PrtGet::printVersion()
{
    cout << m_appName << " " << VERSION
         << " by Johannes Winkelmann, jw@tks6.net" << endl;
}

/*! print version, usage and exit */
void PrtGet::printUsage()
{
    printVersion();
    cout << "Usage: " << m_appName << " <command> [options]" << endl;

    cout << "where commands are:" << endl;

    cout << "\nINFORMATION" << endl;
    cout << "  help                       show this help" << endl;
    cout << "  version                    show the current version" << endl;
    cout << "  list     [<filter>]        show a list of available ports"
         << endl;
    cout << "  printf   <format>          print formatted list of available"
         << " ports"
         << endl;
    cout << "  listinst [<filter>][--depsort]  show a list of installed ports"
         << endl;
    cout << "  listorphans                list of ports with no "
         << "packages depending on them" << endl;
    cout << "  info     <port>            show info about a port" << endl;
    cout << "  path     <port>            show path of a port" << endl;
    cout << "  readme   <port>            show a port's readme file "
         << "(if it exists)" << endl;
    cout << "  dup                        Find duplicate ports" << endl;
    cout << "  isinst   <port1 port2...>  print whether ports are installed"
         << endl;
    cout << "  current  <port>            print installed version of port"
         << endl;

    cout << "\nDIFFERENCES / CHECK FOR UPDATES" << endl;
    cout << "  diff     <port1 port2...>  list outdated packages (or check "
         << "args for change)" << endl;
    cout << "  quickdiff                  same as diff but simple format"
         << endl;
    cout << "          where opt can be:" << endl;
    cout << "    --all            display locked ports too"
         << endl;
    cout << "    --prefer-higher  prefer higher installed "
         << "versions over lower ports"
         << endl;
    cout << "    --strict-diff    override prefer higher "
         << "configuration setting"
         << endl;

    cout << "\nDEPENDENCIES" << endl;
    cout << "  depends   <port1 port2...>  show dependencies for these ports"
         << endl;
    cout << "  quickdep  <port1 port2...>  same as 'depends' but simple format"
         << endl;
    cout << "  deptree   <port>            show dependencies tree for <port>"
         << endl;
    cout << "  dependent [opt] <port>      show installed packages which "
         << "depend on 'port'"
         << endl;
    cout << "          where opt can be:" << endl;
    cout << "                --all    list all dependent packages, not "
         << "only installed" << endl;
    cout << "                --recursive    print recursive listing" << endl;
    cout << "                --tree         print recursive tree listing"
         << endl;

    cout << "\nSEARCHING" << endl;
    cout << "  search  <expr>     show port names containing 'expr'" << endl;
    cout << "  dsearch <expr>     show ports containing 'expr' in the "
         << "name or description" << endl;
    cout << "  fsearch <pattern>  show file names in footprints matching "
         << "'pattern'" << endl;

    cout << "\nINSTALL, UPDATE and REMOVAL" << endl;
    cout << "  install [opt] <port1 port2...>    install ports" << endl;
    cout << "  update  [opt] <port1 port2...>    update ports" << endl;
    cout << "  grpinst [opt] <port1 port2...>    install ports, stop on error"
         << endl;
    cout << "  depinst [opt] <port1 port2...>    install ports and their dependencies"
         << endl;
    cout << "  remove [opt] <port1 port2...>     remove ports"
         << endl;
    cout << "          where opt can be:" << endl;
    cout << "                -f, -fi             force installation" << endl;
    cout << "                -fr                 force rebuild" << endl;
    cout << "                -uf                 update footprint" << endl;
    cout << "                -if                 ignore footprint" << endl;
    cout << "                -um                 update md5sum" << endl;
    cout << "                -im                 ignore md5sum" << endl;
    cout << "                --margs=<string>    pass 'string' to pkgmk"
         << endl;
    cout << "                --aargs=<string>    pass 'string' to pkgadd"
         << endl;
    cout << "                --rargs=<string>    pass 'string' to pkgrm"
         << endl;
    cout << "                --test              test mode" << endl;
    cout << "                --log               write log file"<< endl;
    cout << "                --ignore=<package1,package2,...>" << endl
         << "                                    Don't install/update those packages"<< endl;
    cout << "                --pre-install       execute pre-install script"
         << endl;
    cout << "                --post-install      execute post-install script"
         << endl;
    cout << "                --install-scripts   execute "
         << "pre-install and post-install script"
         << endl;

    cout << "\nSYSTEM UPDATE " << endl;
    cout << "  sysup [opt]                       update all outdated ports"
         << endl;
    cout << "          where opt can be:" << endl;
    cout << "                --nodeps            don't sort by dependencies"
         << endl;
    cout << "                --test              test mode" << endl;
    cout << "                --log               write log file"<< endl;
    cout << "                --prefer-higher     prefer higher installed "
         << "versions over lower ones in ports tree"
         << endl;
    cout << "                --strict-diff       override prefer higher "
         << "configuration setting"
         << endl;

    cout << "  lock <port1 port2...>             lock current version "
         << "of packages"
         << endl;
    cout << "  unlock <port1 port2...>           unlock packages"
         << endl;
    cout << "  listlocked                        list locked packages"
         << endl;

    cout << "\nFILE OPERATIONS " << endl;

    cout << "  ls <port>                         print a listing of the port's"
         << " directory" << endl;
    cout << "  cat <port> <file>                 print out 'port/file'"
         << endl;
    cout << "  edit <port> <file>                edit 'port/file'" << endl;

    cout << "\nGENERAL OPTIONS" << endl;
    cout << "                -v                 Show version in listing"
         << endl;
    cout << "                -vv                Show version and decription "          << "in listing\n" << endl;
    cout << "                --path             Print path to port if appropriate (search, list, depends)\n" << endl;
    cout << "                --cache             Use a cache file" << endl;
    cout << "                --config=<file>     Use alternative "
         << "configuration file" << endl;
    cout << "                --install-root=..   Use alternative "
         << "install root directory" << endl;




    cout << "                --no-std-config     Don't parse "
         << "default configuration file" << endl;
    cout << "                --config-prepend=.. Prepend '..' to"
         << " configuration" << endl;
    cout << "                --config-append=..  Append '..' "
         << "to configuration" << endl;
    cout << "                --config-set=..     Set configuration "
         << "data '..',\n"
         << "                                       overriding config file"
         << endl;

}


/*! print list of duplicate packages in the repository */
void PrtGet::listShadowed()
{
    if ( m_parser->useCache() ) {
        cout << m_appName << ": command 'dup' can't work on a cache" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

    initRepo( true );

    string format = "%p1 %v1 > %p2 %v2\n";
    if (m_parser->otherArgs().size() > 0)
        format = *(m_parser->otherArgs().begin());
    else if (m_parser->verbose() > 0)
        format = "* %n\n  %p1 %v1 preceeds over \n  %p2 %v2\n";

    string output;
    Package* p1;
    Package* p2;

    list<pair<Package*, Package*> >::const_iterator it =
        m_repo->shadowedPackages().begin();
    for ( ; it != m_repo->shadowedPackages().end(); ++it ) {
        output = format;
        p1 = it->second;
        p2 = it->first;

        StringHelper::replaceAll(output, "%n",  p1->name());
        StringHelper::replaceAll(output, "%p1", p1->path() + "/" + p1->name());
        StringHelper::replaceAll(output, "%p2", p2->path() + "/" + p2->name());
        StringHelper::replaceAll(output, "%v1", p1->versionReleaseString());
        StringHelper::replaceAll(output, "%v2", p2->versionReleaseString());

        StringHelper::replaceAll(output, "\\n", "\n");
        cout << output;
    }
}

/*!
  find ports matching a pattern in repository

  \sa Repository::getMatchingPackages()
*/
void PrtGet::listPackages()
{
    string arg = "*";
    assertMaxArgCount(1);

    if ( m_parser->otherArgs().size() == 1 ) {
        arg = *(m_parser->otherArgs().begin());
    }

    initRepo();
    list<Package*> packages;
    m_repo->getMatchingPackages( arg, packages );
    if ( packages.size() ) {
        list<Package*>::iterator it = packages.begin();
        for ( ; it != packages.end(); ++it ) {
            if ( m_parser->printPath() ) {
                cout << (*it)->path() << "/";
            }
            cout << (*it)->name();
            if ( m_parser->verbose() > 0 ) {
                cout << " " << (*it)->version() << "-" << (*it)->release();
            }
            if ( m_parser->verbose() > 1 && !(*it)->description().empty() ) {
                cout << ": " << (*it)->description();
            }

            cout << endl;
        }
    } else {
        cout << "No matching packages found"  << endl;
    }
}

/*!
  search repository for a certain pattern (which is read by the argument
  parser.

  \sa Repository::searchMatchingPackages()
*/
void PrtGet::searchPackages( bool searchDesc )
{
    assertExactArgCount(1);

    initRepo();
    string arg = *(m_parser->otherArgs().begin());
    list<Package*> packages;
    m_repo->searchMatchingPackages( arg, packages, searchDesc );
    if ( packages.size() ) {
        list<Package*>::iterator it = packages.begin();
        for ( ; it != packages.end(); ++it ) {
            if ( m_parser->printPath()) {
                cout << (*it)->path() << "/";
            }
            cout << (*it)->name();

            if ( m_parser->verbose() > 0 ) {
                cout << " " << (*it)->version() << "-" << (*it)->release();
            }
            if ( m_parser->verbose() > 1 && !(*it)->description().empty() ) {
                cout << ": " << (*it)->description();
            }


            cout << endl;
        }
    } else {
        m_returnValue = PG_GENERAL_ERROR;
        cout << "No matching packages found"  << endl;
    }
}

/*! print info for a package */
void PrtGet::printInfo()
{
    assertExactArgCount(1);

    initRepo();
    string arg = *(m_parser->otherArgs().begin());
    const Package* p = m_repo->getPackage( arg );
    if ( p ) {
        cout << "Name:         " << p->name() << "\n"
             << "Path:         " << p->path() << "\n"
             << "Version:      " << p->version() << "\n"
             << "Release:      " << p->release() << endl;

        if ( !p->description().empty() ) {
            cout << "Description:  " << p->description() << endl;
        }
        if ( !p->url().empty() ) {
            cout << "URL:          " << p->url() << endl;
        }
        if ( !p->packager().empty() ) {
            cout << "Packager:     " << p->packager() << endl;
        }
        if ( !p->maintainer().empty() ) {
            cout << "Maintainer:   " << p->maintainer() << endl;
        }

        if ( !p->dependencies().empty() ) {
            cout << "Dependencies: " << p->dependencies() << endl;
        }

        // TODO: don't hardcode file names
        string filesString = "";
        if ( p->hasReadme() ) {
            filesString += "README ";
        }
        if ( p->hasPreInstall() ) {
            filesString += "pre-install ";
        }
        if ( p->hasPostInstall() ) {
            filesString += "post-install ";
        }

        if ( filesString.length() > 0 ) {
            filesString = StringHelper::stripWhiteSpace( filesString );
            StringHelper::replaceAll( filesString, " ", "," );
            cout << "Files:        " << filesString << endl;
        }

        if ( m_parser->verbose() > 0 && p->hasReadme()) {
            cout << "\n-- README ------" << endl;
            readme();
        }

    } else {
        cerr << "Package '" << arg << "' not found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }
}


/*!
  initialize repository
  \sa Repository::initFromCache()
  \sa Repository::initFromFS()
 */
void PrtGet::initRepo( bool listDuplicate )
{
    if ( !m_repo ) {
        m_repo = new Repository(m_useRegex);

        if ( m_parser->useCache() ) {
            if (m_config->cacheFile() != "") {
                m_cacheFile = m_config->cacheFile();
            }

            Repository::CacheReadResult result =
                m_repo->initFromCache( m_cacheFile );
            if ( result == Repository::ACCESS_ERR  ) {
                cerr << "Can't open cache file: " << m_cacheFile << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            } else if ( result == Repository::FORMAT_ERR ) {
                cerr << "warning: your cache file "
                     << m_cacheFile << " was made with an "
                     << "older version "
                     << "of prt-get."
                     << "\nPlease regenerate it using"
                     << "\n  prt-get cache" << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            }

            struct stat cacheStat;
            struct stat confStat;
            stat( m_cacheFile.c_str(), &cacheStat );
            stat( CONF_FILE.c_str(), &confStat );
            if ( confStat.st_ctime > cacheStat.st_ctime ) {
                cerr << "Error: "
                     << "Configuration changed after generating cache"
                     << endl;
                cerr << "regenerate cache using 'prt-get cache'" << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            }

            if ( !m_parser->wasCalledAsPrtCached() ) {
                cout << m_appName << ": using cache" << endl;
            }

        } else {
            m_repo->initFromFS( m_config->rootList(), listDuplicate );
        }
    }
}

/*! print whether a package is installed or not */
void PrtGet::isInstalled()
{
    assertMinArgCount(1);

    const list<char*>& l = m_parser->otherArgs();
    list<char*>::const_iterator it = l.begin();
    for ( ; it != l.end(); ++it ) {
        bool isAlias = false;
        string aliasName;

        if ( m_pkgDB->isInstalled( *it, true, &isAlias, &aliasName  ) ) {
            if (isAlias) {
                cout << *it << " is provided by package "
                     << aliasName
                     << endl;
            } else {
                cout << "package " << *it << " is installed" << endl;
            }
        } else {
            cout << "package " << *it << " is not installed" << endl;
            m_returnValue = PG_GENERAL_ERROR;
        }
    }
}


/*! list installed packages */
void PrtGet::listInstalled()
{
    assertMaxArgCount(1);

    string arg = "*";
    if ( m_parser->otherArgs().size() == 1 ) {
        arg = *(m_parser->otherArgs().begin());
    }

    map<string, string> l;
    m_pkgDB->getMatchingPackages( arg, l, m_useRegex );
    map<string, string>::iterator it = l.begin();

    if ( l.empty() && m_parser->otherArgs().size() > 0 ) {
        cerr << m_appName << ": No matching packages found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

    if (m_parser->depSort()) {
	// sort by dependency, without injecting missing ones
	// calcDependencies chokes on the full list, so go through the
	// ports one by one
	
	initRepo();
	map<string, string>::iterator mit;
	string name;
	while (!l.empty()) {
	    mit = l.begin();
	    name = mit->first;
	    l.erase(mit);
	
	    InstallTransaction trans( name, m_repo, m_pkgDB, m_config );
	    InstallTransaction::InstallResult result = trans.calcDependencies();
	    const list<string>& depRef = trans.dependencies();
	    list<string>::const_iterator it = depRef.begin();


	    for (; it != depRef.end(); ++it) {
		if (l.find(*it) != l.end()) {
		    cout << *it << endl;
		    l.erase(*it);
		}	
	    }
	    cout << name << endl;
        }
	
    } else {
	for ( ; it != l.end(); ++it ) {
	    if ( m_parser->verbose() > 1 ) {
		// warning: will slow down the process...
		initRepo();
	    }
	    cout <<  it->first.c_str();
	    if ( m_parser->verbose() > 0 ) {
		cout << " " << it->second.c_str();
	    }
	    if ( m_parser->verbose() > 1 ) {
		const Package* p = m_repo->getPackage( it->first );
		if ( p ) {
		    cout << " " << p->description();
		}
	    }

	    cout << endl;
	}
    }
}

/*!
   install package
   \param update whether this is an update operation
   \param group whether it's a group install (stop on error)

*/
void PrtGet::install( bool update, bool group, bool dependencies )
{
    assertMinArgCount(1);

    // this can be done without initRepo()
    const list<char*>& args = m_parser->otherArgs();
    list<char*>::const_iterator it = args.begin();

    if ( args.size() == 1 ) {
        for ( ; it != args.end(); ++it ) {
            string s = *it;
            if ( !update && m_pkgDB->isInstalled( s ) ) {
                cout << "package " << s << " is installed" << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            } else if ( update && !m_pkgDB->isInstalled( s ) ) {
                // can't upgrade
                cout << "package " << s << " is not installed" << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            }
        }
    }

    initRepo();

    if (dependencies) {
        // calc dependencies
        InstallTransaction depTransaction( m_parser->otherArgs(),
                                           m_repo, m_pkgDB, m_config );
        InstallTransaction::InstallResult result =
            depTransaction.calcDependencies();

        // TODO: code duplication with printDepends!
        if ( result == InstallTransaction::CYCLIC_DEPEND ) {
            cerr << "prt-get: cyclic dependencies found" << endl;
            m_returnValue = PG_GENERAL_ERROR;
            return;
        } else if ( result == InstallTransaction::PACKAGE_NOT_FOUND ) {
            warnPackageNotFound(depTransaction);
            m_returnValue = PG_GENERAL_ERROR;
            return;
        }
        const list<string>& depRef = depTransaction.dependencies();
        list<string>::const_iterator it = depRef.begin();

        list<string> deps;
        for (; it != depRef.end(); ++it) {
            if (!m_pkgDB->isInstalled(*it)) {
                deps.push_back(*it);
            }
        }

        InstallTransaction transaction( deps, m_repo, m_pkgDB, m_config );
        executeTransaction( transaction, update, group );
    } else {
        InstallTransaction transaction( m_parser->otherArgs(),
                                        m_repo, m_pkgDB, m_config );
        executeTransaction( transaction, update, group );
    }
}

void PrtGet::executeTransaction( InstallTransaction& transaction,
                                 bool update, bool group )
{
    m_currentTransaction = &transaction;

    string command[] = { "install", "installed" };
    if ( update ) {
        command[0] = "update";
        command[1] = "updated";
    }

    if ( m_parser->isTest() ) {
        cout << "*** " << m_appName << ": test mode" << endl;
    }

    InstallTransaction::InstallResult result =
        transaction.install( m_parser, update, group );
    bool failed = false;
    // TODO: use switch
    if ( result == InstallTransaction::PACKAGE_NOT_FOUND ) {
        cout << m_appName << ": package(s) not found" << endl;
    } else if ( result == InstallTransaction::PKGMK_EXEC_ERROR ) {
        cout << m_appName << " couldn't excecute pkgmk "
             << "(or alternative command). "
             << "Make sure it's installed properly" << endl;
    } else if ( result == InstallTransaction::PKGMK_FAILURE ) {
        cout << m_appName << ": error while " << command[0] << endl;
    } else if ( result == InstallTransaction::NO_PACKAGE_GIVEN ) {
        cout << m_appName << ": no package specified for "
             << command[0] << endl;
    } else if ( result == InstallTransaction::PKGADD_EXEC_ERROR ) {
        cout << m_appName << " couldn't excecute pkgadd. "
             << "Make sure it's installed properly" << endl;
    } else if ( result == InstallTransaction::PKGDEST_ERROR ) {
        cout << m_appName << ": error changing to PKGDEST directory  "
             << transaction.getPkgmkPackageDir() << endl;
        failed = true;
    } else if ( result == InstallTransaction::PKGADD_FAILURE ) {
        cout << m_appName << ": error while pkgadding " << endl;
    } else if ( result == InstallTransaction::LOG_DIR_FAILURE ) {
        cout << m_appName << ": can't create log file directory " << endl;
    } else if ( result == InstallTransaction::LOG_FILE_FAILURE ) {
        cout << m_appName << ": can't create log file" << endl;
        failed = true;
    } else if ( result == InstallTransaction::NO_LOG_FILE ) {
        cout << m_appName << ": no log file specified, but logging enabled"
             << endl;
        failed = true;
    } else if ( result == InstallTransaction::CANT_LOCK_LOG_FILE ) {
        cout << m_appName << ": can't create lock file for the log file. "
             << "\nMaybe there's another instance of prt-get using the same "
             << "file."
             << "\nIf this is a stale not, please remove "
            // TODO: file name of lock file
             << endl;
        failed = true;
    } else if ( result != InstallTransaction::SUCCESS ) {
        cout << m_appName << ": Unknown error " << result << endl;
        failed = true;
    }

    if ( !failed ) {
        evaluateResult( transaction, update );
        if ( m_parser->isTest() ) {
            cout << "\n*** " << m_appName << ": test mode end" << endl;
        }
    } else {
        m_returnValue = PG_INSTALL_ERROR;
    }

    m_currentTransaction = 0;
}

/*!
  print dependency listing
  \param simpleListing Whether it should be in a simple format
*/
void PrtGet::printDepends( bool simpleListing )
{
    assertMinArgCount(1);

    initRepo();

    InstallTransaction transaction( m_parser->otherArgs(),
                                    m_repo, m_pkgDB, m_config );
    InstallTransaction::InstallResult result = transaction.calcDependencies();
    if ( result == InstallTransaction::CYCLIC_DEPEND ) {
        cerr << "prt-get: cyclic dependencies found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    } else if ( result == InstallTransaction::PACKAGE_NOT_FOUND ) {
        warnPackageNotFound(transaction);
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

    const list<string>& deps = transaction.dependencies();
    if ( simpleListing ) {
        if ( deps.size() > 0 ) {
            list<string>::const_iterator it = deps.begin();
            for ( ; it != deps.end(); ++it ) {
                cout << *it << " ";
            }
            cout << endl;
        }
    } else {
        if ( deps.size() > 0 ) {
            cout << "-- dependencies ([i] = installed)" << endl;
            list<string>::const_iterator it = deps.begin();

            bool isAlias;
            string provider;
            for ( ; it != deps.end(); ++it ) {
                isAlias = false;
                if ( m_pkgDB->isInstalled( *it, true, &isAlias, &provider ) ) {
                    cout << "[i] ";
                } else {
                    cout << "[ ] ";
                }
                if (m_parser->printPath() > 0) {
                    cout << m_repo->getPackage(*it)->path() << "/";
                }
                cout << *it;

                if (isAlias) {
                    cout << " (provided by " << provider << ")";
                }
                cout << endl;
            }
        } else {
            cout << "No dependencies found" << endl;
        }

        const list< pair<string, string> >& missing = transaction.missing();
        if ( missing.size() ) {
            list< pair<string, string> >::const_iterator mit = missing.begin();
            cout << endl << "-- missing packages" << endl;
            for ( ; mit != missing.end(); ++mit ) {
                cout << mit->first;
                if ( !mit->second.empty() ) {
                    cout << " from " << mit->second;
                }
                cout << endl;
            }
        }
    }
}

/*! read the config file */
void PrtGet::readConfig()
{
    string fName = CONF_FILE;
    if ( m_parser->isAlternateConfigGiven() ) {
        fName = m_parser->alternateConfigFile();
    }

    if ( m_config ) {
        return; // don't initialize twice
    }
    m_config = new Configuration( fName, m_parser );

    if (!m_parser->noStdConfig()) {
        if ( !m_config->parse() ) {
            cerr << "Can't read config file " << fName
                 << ". Exiting" << endl;
            m_returnValue = PG_GENERAL_ERROR;
            return;
        }
    }

    const list< pair<char*, ArgParser::ConfigArgType> >& configData =
        m_parser->configData();
    list< pair<char*, ArgParser::ConfigArgType> >::const_iterator it =
        configData.begin();
    for (; it != configData.end(); ++it) {
        m_config->addConfig(it->first,
                            it->second == ArgParser::CONFIG_SET,
                            it->second == ArgParser::CONFIG_PREPEND);
    }
}

/*!
  print a simple list of port which are installed in a different version
  than they are in the repository
*/
void PrtGet::printQuickDiff()
{
    initRepo();

    const map<string, string>& installed = m_pkgDB->installedPackages();
    map<string, string>::const_iterator it = installed.begin();
    const Package* p = 0;
    COMP_RESULT result;
    for ( ; it != installed.end(); ++it ) {
        if ( !m_locker.isLocked( it->first ) ) {
            p = m_repo->getPackage( it->first );
            if ( p ) {
                result = compareVersions(p->versionReleaseString(),
                                         it->second);
                if (result == GREATER) {
                    cout <<  it->first.c_str() << " ";
                }
                // we don't care about undefined diffs here
            }
        }
    }
    cout << endl;
}


void PrtGet::printFormattedDiffLine(const string& name,
                                    const string& versionInstalled,
                                    const string& versionPortsTree,
                                    bool isLocked)
{
    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout <<  name;

    cout.width( 20 );
    cout.fill( ' ' );
    cout << versionInstalled;

    string locked = "";
    if ( isLocked ) {
        locked = "locked";
    }
    cout.width( 20 );
    cout.fill( ' ' );
    cout << versionPortsTree << locked << endl;
}
/*!
  print an overview of port which are installed in a different version
  than they are in the repository
*/
void PrtGet::printDiff()
{
    initRepo();
    map< string, string > l;
    if ( m_parser->otherArgs().size() > 0 ) {
        expandWildcardsPkgDB( m_parser->otherArgs(), l );
    }
    if ( l.size() < m_parser->otherArgs().size() ) {
        cerr << "prt-get: no matching installed packages found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

#if 0
    // const list<char*>& l = m_parser->otherArgs();
    // list<char*>::const_iterator checkIt = l.begin();

    // check whether ports to be checked are installed
    list< string >::iterator checkIt = l.begin();
    for ( ; checkIt != l.end(); ++checkIt ) {
        if ( ! m_pkgDB->isInstalled( *checkIt )  ) {
            cerr << "Port not installed: " << *checkIt << endl;
            m_returnValue = PG_GENERAL_ERROR;
            return;
        }
    }
#endif

    const map<string, string>& installed = m_pkgDB->installedPackages();
    map<string, string>::const_iterator it = installed.begin();
    const Package* p = 0;
    int count = 0;
    COMP_RESULT result;
    for ( ; it != installed.end(); ++it ) {

        p = m_repo->getPackage( it->first );
        if ( p ) {
            if ( l.size() && l.find( it->first ) == l.end() ) {
                continue;
            }

            result = compareVersions( p->versionReleaseString(),
                                      it->second );
            if (result  == GREATER ) {
                if ( !m_locker.isLocked( it->first )  ||
                     m_parser->otherArgs().size() > 0 ||
                     m_parser->all() ) {


                    ++count;
                    if ( count == 1 ) {
                        cout << "Differences between installed packages "
                             << "and ports tree:\n" << endl;
                        printFormattedDiffLine("Port",
                                               "Installed",
                                               "Available in the ports tree",
                                               false);
                        cout << endl;
                    }

                    printFormattedDiffLine(it->first,
                                           it->second,
                                           p->versionReleaseString(),
                                           m_locker.isLocked( it->first ));
                }
            } else if (result == UNDEFINED) {
                m_undefinedVersionComp.push_back(make_pair(p, it->second));
            }
        }
    }

    if (m_undefinedVersionComp.size()) {
        cout << "\n\n" << "Undecidable version differences (use --strict-diff)"
             << endl << endl;
        printFormattedDiffLine("Port",
                               "Installed",
                               "Available in the ports tree",
                               false);
        cout << endl;

        list< pair< const Package*, string > >::iterator it =
            m_undefinedVersionComp.begin();
        const Package* p;
        for (; it != m_undefinedVersionComp.end(); ++it) {
            p = it->first;
            printFormattedDiffLine(p->name(),
                                   it->second,
                                   p->versionReleaseString(),
                                   false);
        }
    }


    if ( count == 0 ) {
        cout << "No differences found" << endl;
    }
}

/*! print path to a port */
void PrtGet::printPath()
{
    assertExactArgCount(1);

    initRepo();
    string arg = *(m_parser->otherArgs().begin());
    const Package* p = m_repo->getPackage( arg );
    if ( p ) {
        cout << p->path() << "/" << p->name() << endl;
    } else {
        cerr << "Package '" << arg << "' not found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }
}


/*! helper method to print the result of an InstallTransaction */
void PrtGet::evaluateResult( InstallTransaction& transaction,
                             bool update,
                             bool interrupted )
{
    int errors = 0;

    // TODO: this is a duplicate, it's in install() as well
    string command[] = { "install", "installed" };
    if ( update ) {
        command[0] = "update";
        command[1] = "updated";
    }

    const list<string>& ignored = transaction.ignoredPackages();
    if ( ignored.size() ) {
        cout << endl << "-- Packages ignored" << endl;
        list<string>::const_iterator iit = ignored.begin();

        for ( ; iit != ignored.end(); ++iit ) {
            cout << *iit << endl;
        }
    }

    const list< pair<string, string> >& missing = transaction.missing();
    if ( missing.size() ) {
        ++errors;
        cout << endl << "-- Packages not found" << endl;
        list< pair<string, string> >::const_iterator mit = missing.begin();

        for ( ; mit != missing.end(); ++mit ) {
            cout << mit->first;
            if ( mit->second != "" ) {
                cout << " from " << mit->second;
            }
            cout << endl;
        }
    }

    const list< pair<string, InstallTransaction::InstallInfo> >& error =
        transaction.installError();
    if ( error.size() ) {
        ++errors;
        cout << endl << "-- Packages where "
             << command[0] << " failed" << endl;
        list< pair<string, InstallTransaction::InstallInfo> >::const_iterator
            eit = error.begin();

        for ( ; eit != error.end(); ++eit ) {
            cout << eit->first;
            reportPrePost(eit->second);
            cout << endl;
        }
    }

    const list<string>& already = transaction.alreadyInstalledPackages();
    if ( already.size() ) {
        cout << endl << "-- Packages installed before this run (ignored)"
             << endl;
        list<string>::const_iterator ait = already.begin();

        bool isAlias;
        string provider;
        for ( ; ait != already.end(); ++ait ) {
            isAlias = false;
            cout << *ait;
            m_pkgDB->isInstalled(*ait, true, &isAlias, &provider);

            if (isAlias) {
                cout << " (provided by " << provider << ")";
            }
            cout << endl;
        }
    }


    const list< pair<string, InstallTransaction::InstallInfo> >& inst =
        transaction.installedPackages();
    if ( inst.size() ) {
        cout << endl << "-- Packages " << command[1] << endl;
        list< pair<string, InstallTransaction::InstallInfo> >::const_iterator
            iit = inst.begin();

        bool atLeastOnePackageHasReadme = false;

        for ( ; iit != inst.end(); ++iit ) {
	    if (m_parser->printPath()) {
		// TODO: avoid lookup by tuning
		// InstallTransaction::installedPackages()
		const Package* p = m_repo->getPackage(iit->first);
		if (p) {
		    cout << p->path() << "/";
		}
	    }
            cout << iit->first;
            if ( iit->second.hasReadme ) {
                if ( m_config->readmeMode() ==
                     Configuration::COMPACT_README ) {
                    cout << " (README)";
                }
                atLeastOnePackageHasReadme = true;
            }
            reportPrePost(iit->second);
            cout << endl;
        }


        // readme's
        if ( atLeastOnePackageHasReadme ) {
            if ( m_config->readmeMode() == Configuration::VERBOSE_README ) {
                cout << endl << "-- " << command[1]
                     << " packages with README files:" << endl;
                iit = inst.begin();
                for ( ; iit != inst.end(); ++iit ) {
                    if ( iit->second.hasReadme ) {
                        cout << iit->first;
                        cout << endl;
                    }
                }
            }
        }
    }
    if ( m_undefinedVersionComp.size() ) {
        cout << endl
             << "-- Packages with undecidable version "
             << "difference (use --strict-diff)"
             << endl;
        list< pair<const Package*, string> >::const_iterator uit =
            m_undefinedVersionComp.begin();
        const Package * p;
        for ( ; uit != m_undefinedVersionComp.end(); ++uit ) {
            p = uit->first;
            cout << p->name() << " ("
                 << uit->second
                 << " vs "
                 << p->versionReleaseString() << ")" << endl;
        }
    }

    cout << endl;

    if ( errors == 0 && !interrupted ) {
        cout << "prt-get: " << command[1] << " successfully" << endl;
    } else {
        m_returnValue = PG_PARTIAL_INSTALL_ERROR;
    }
}

void PrtGet::reportPrePost(const InstallTransaction::InstallInfo& info) {
    if (info.preState != InstallTransaction::NONEXISTENT) {
        string preString = "failed";
        if (info.preState == InstallTransaction::EXEC_SUCCESS) {
            preString = "ok";
        }
        cout << " [pre: " << preString << "]";
    }
    if ( info.postState != InstallTransaction::NONEXISTENT) {
        string postString = "failed";
        if (info.postState == InstallTransaction::EXEC_SUCCESS){
            postString = "ok";
        }
        cout << " [post: " << postString << "]";
    }

}

/*! create a cache */
void PrtGet::createCache()
{
    if ( m_parser->wasCalledAsPrtCached() ) {
        cerr << m_appName << ": Can't create cache from cache. "
             << "Use prt-get instead" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

    initRepo();
    if (m_config->cacheFile() != "") {
        m_cacheFile = m_config->cacheFile();
    }

    Repository::WriteResult result = m_repo->writeCache( m_cacheFile );
    if ( result == Repository::DIR_ERR ) {
        cerr << "Can't create cache directory " << m_cacheFile << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }
    if ( result == Repository::FILE_ERR ) {
        cerr << "Can't open cache file " << m_cacheFile << " for writing"
             << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

}

/*!
  \return true if v1 is greater than v2
 */
COMP_RESULT PrtGet::compareVersions( const string& v1, const string& v2 )
{
    if (v1 == v2) {
        return EQUAL;
    }


    if (m_parser->preferHigher() ||
        (m_config->preferHigher() && !m_parser->strictDiff())) {

        COMP_RESULT result = VersionComparator::compareVersions(v1, v2);
        return result;
    }

    if (v1 != v2)
        return GREATER;
    return LESS;
}

int PrtGet::returnValue() const
{
    return m_returnValue;
}


/*! print a list of packages available in the repository */
void PrtGet::printf()
{
    map<string, string> sortedOutput;

    assertExactArgCount(1);

    initRepo();
    string filter = m_parser->useRegex() ? "." : "*";
    if ( m_parser->hasFilter() ) {
        filter = m_parser->filter();
    }
    list<Package*> packages;
    m_repo->getMatchingPackages( filter, packages );
    list<Package*>::const_iterator it = packages.begin();

    const string formatString = *(m_parser->otherArgs().begin());
    string sortString =
        StringHelper::stripWhiteSpace( m_parser->sortArgs() );
    sortString += "%n"; // make it unique

    for ( ; it != packages.end(); ++it ) {
        string output = formatString;
        string sortkey = sortString;

        const Package* p = *it;

        StringHelper::replaceAll( output, "%n", p->name() );
        StringHelper::replaceAll( output, "%u", p->url() );
        StringHelper::replaceAll( output, "%p", p->path() );
        StringHelper::replaceAll( output, "%v", p->version() );
        StringHelper::replaceAll( output, "%r", p->release() );
        StringHelper::replaceAll( output, "%d", p->description() );
        StringHelper::replaceAll( output, "%e", p->dependencies() );
        StringHelper::replaceAll( output, "%P", p->packager() );
        StringHelper::replaceAll( output, "%M", p->maintainer() );

        StringHelper::replaceAll( output, "\\t", "\t" );
        StringHelper::replaceAll( output, "\\n", "\n" );

        StringHelper::replaceAll( sortkey, "%n", p->name() );
        StringHelper::replaceAll( sortkey, "%u", p->url() );
        StringHelper::replaceAll( sortkey, "%p", p->path() );
        StringHelper::replaceAll( sortkey, "%v", p->version() );
        StringHelper::replaceAll( sortkey, "%r", p->release() );
        StringHelper::replaceAll( sortkey, "%d", p->description() );
        StringHelper::replaceAll( sortkey, "%e", p->dependencies() );
        StringHelper::replaceAll( sortkey, "%P", p->packager() );
        StringHelper::replaceAll( sortkey, "%M", p->maintainer() );

        string isInst = "no";
        if ( m_pkgDB->isInstalled( p->name() ) ) {
            string ip = p->name() + "-" +
                m_pkgDB->getPackageVersion( p->name() );
            if ( ip == p->name() + "-" + p->versionReleaseString() ) {
                isInst = "yes";
            } else {
                isInst = "diff";
            }
        }
        StringHelper::replaceAll( output, "%i", isInst );
        StringHelper::replaceAll( sortkey, "%i", isInst );

        string isLocked = m_locker.isLocked( p->name() ) ? "yes" : "no";
        StringHelper::replaceAll( output, "%l", isLocked );
        StringHelper::replaceAll( sortkey, "%l", isLocked );

        string hasReadme = p->hasReadme() ? "yes" : "no";
        StringHelper::replaceAll( output, "%R", hasReadme );
        StringHelper::replaceAll( sortkey, "%R", hasReadme );

        string hasPreInstall = p->hasPreInstall() ? "yes" : "no";
        StringHelper::replaceAll( output, "%E", hasPreInstall );
        StringHelper::replaceAll( sortkey, "%E", hasPreInstall );

        string hasPostInstall = p->hasPostInstall() ? "yes" : "no";
        StringHelper::replaceAll( output, "%O", hasPostInstall );
        StringHelper::replaceAll( sortkey, "%O", hasPostInstall );

        sortedOutput[sortkey] = output;
    }

    map<string, string>::iterator sortIt = sortedOutput.begin();
    for ( ; sortIt != sortedOutput.end(); ++sortIt ) {
        if ( StringHelper::stripWhiteSpace(sortIt->second).length() > 0) {
            cout << sortIt->second;
        }
    }
}

void PrtGet::readme()
{
    assertExactArgCount(1);

    initRepo();
    string arg = *(m_parser->otherArgs().begin());
    const Package* p = m_repo->getPackage( arg );
    if ( p ) {
        string file = p->path() + "/" + p->name() + "/README";
        printFile(file);
    } else {
        cerr << "Package '" << arg << "' not found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }
}

bool PrtGet::printFile(const string& file)
{
    if (!File::fileExists(file)) {
        return false;
    }

    char* pager = getenv("PAGER");
    if (pager) {
        Process proc(pager, file);
        proc.executeShell();
    } else {
        FILE* fp = fopen( file.c_str(), "r" );
        char buf[255];
        if ( fp ) {
            while ( fgets( buf, 255, fp ) ) {
                cout << buf;
            }
            fclose( fp );
        }
    }

    return true;
}

void PrtGet::printDependent()
{
    assertExactArgCount(1);

    initRepo();
    string arg = *(m_parser->otherArgs().begin());

    if (m_parser->printTree()) {
        cout << arg << endl;
        printDependent(arg, 2);
    } else {
        printDependent(arg, 0);
    }
}

void PrtGet::printDependent(const string& dep, int level)
{
    map<string, Package*>::const_iterator it = m_repo->packages().begin();
    static map<string, bool> shownMap;

    set<const Package*> dependent;
    for ( ; it != m_repo->packages().end(); ++it ) {

        // TODO: is the following line needed?
        const Package* p = it->second;
        if ( p && p->dependencies().find( dep ) != string::npos ) {
            list<string> tokens;
            StringHelper::split( p->dependencies(), ',', tokens );
            list<string>::iterator it = find( tokens.begin(),
                                              tokens.end(),
                                              dep );
            if ( it != tokens.end() ) {
                dependent.insert( p );
            }
        }
    }

    // - there are two modes, tree and non-tree recursive mode; in
    // tree mode, packages are shown multiple times, in non tree
    // recursive mode they're only printed the first time; this is not
    // necessarily optimal for rebuilding:
    //
    // a -> b -> d
    //  \     ^
    //   > c /
    //
    // trying to rebuild 'd' before 'c' might possibly fail
    string indent = "";
    if (m_parser->printTree()) {
        for (int i = 0; i < level; ++i) {
            indent += " ";
        }
    }
    set<const Package*>::iterator it2 = dependent.begin();
    for ( ; it2 != dependent.end(); ++it2 ) {
        const Package* p = *it2;

        if (m_parser->recursive() && !m_parser->printTree()) {
            if (shownMap[p->name()]) {
                continue;
            }
            shownMap[p->name()] = true;
        }

        if ( m_parser->all() || m_pkgDB->isInstalled( p->name() ) ) {

            cout << indent << p->name();
            if ( m_parser->verbose() > 0 ) {
                cout << " " << p->versionReleaseString();
            }
            if ( m_parser->verbose() > 1 ) {
                cout << ":  " << p->description();
            }

            cout << endl;

            if (m_parser->recursive()) {
                printDependent( p->name(), level+2 );
            }
        }
    }
}

void PrtGet::listOrphans()
{
    initRepo();
    map<string, string> installed = m_pkgDB->installedPackages();
    map<string, bool> required;
    map<string, string>::iterator it = installed.begin();

    for (; it != installed.end(); ++it) {
        list<string> tokens;
        const Package* p = m_repo->getPackage(it->first);
        if (p) {
            StringHelper::split( p->dependencies(), ',', tokens );
            list<string>::iterator lit = tokens.begin();
            for (; lit != tokens.end(); ++lit) {
                required[*lit] = true;
            }
        }
    }

    // - we could store the package pointer in another map to avoid
    // another getPackage lockup, but it seems better to optimized for
    // memory since it's only used when called with -vv

    it = installed.begin();
    for (; it != installed.end(); ++it) {
        if (!required[it->first]) {
            cout << it->first;
            if ( m_parser->verbose() > 0 ) {
                cout << " " << it->second;
            }
            if ( m_parser->verbose() > 1 ) {
                const Package* p = m_repo->getPackage(it->first);
                if (p) {
                    cout << ":  " << p->description();
                }
            }
            cout << endl;
        }
    }
}


void PrtGet::warnPackageNotFound(InstallTransaction& transaction)
{
    cerr << "The package '";
    cerr << transaction.missing().begin()->first;
    cerr << "' could not be found: " << endl;
}

void PrtGet::sysup()
{
    // TODO: refactor getDifferentPackages from diff/quickdiff
    initRepo();

    list<string>* target;
    list<string> packagesToUpdate;
    list<string> sortedList;

    const map<string, string>& installed = m_pkgDB->installedPackages();
    map<string, string>::const_iterator it = installed.begin();
    const Package* p = 0;
    COMP_RESULT result;
    for ( ; it != installed.end(); ++it ) {
        if ( !m_locker.isLocked( it->first ) ) {
            p = m_repo->getPackage( it->first );
            if ( p ) {
                result = compareVersions( p->versionReleaseString(),
                                          it->second );
                if (result  == GREATER ) {
                    packagesToUpdate.push_back( it->first );
                } else if (result  == UNDEFINED ) {
                    m_undefinedVersionComp.push_back(make_pair(p, it->second));
                }
            }
        }
    }

    if ( packagesToUpdate.empty() ) {
        cout << "System is up to date" << endl;
        return;
    }

    if ( m_parser->nodeps() ) {
        target = &packagesToUpdate;
    } else {
        // sort by dependency

        // TODO: refactor code from printDepends
        InstallTransaction depTrans( packagesToUpdate,
                                     m_repo, m_pkgDB, m_config );
        InstallTransaction::InstallResult result = depTrans.calcDependencies();
        if ( result == InstallTransaction::CYCLIC_DEPEND ) {
            cerr << "cyclic dependencies" << endl;
            m_returnValue = PG_GENERAL_ERROR;
            return;
        } else if ( result == InstallTransaction::PACKAGE_NOT_FOUND ) {
            warnPackageNotFound(depTrans);
            m_returnValue = PG_GENERAL_ERROR;
            return;
        }

        const list<string>& deps = depTrans.dependencies();
        if ( deps.size() > 0 ) {
            list<string>::const_iterator it = deps.begin();
            for ( ; it != deps.end(); ++it ) {
                if ( find( packagesToUpdate.begin(),
                           packagesToUpdate.end(), *it ) !=
                     packagesToUpdate.end() ) {;
                     sortedList.push_back( *it );
                }
            }
        }

        target = &sortedList;
    }

    InstallTransaction transaction( *target,
                                    m_repo, m_pkgDB, m_config );
    executeTransaction( transaction, true, false );
}


void PrtGet::expandWildcardsPkgDB( const list<char*>& in,
                                   map<string, string>& target )
{
    list<char*>::const_iterator it = in.begin();
    for ( ; it != in.end(); ++it ) {
        map<string, string> l;
        m_pkgDB->getMatchingPackages( *it, l, m_useRegex );
        map<string, string>::iterator iit = l.begin();
        for ( ; iit != l.end(); ++iit ) {
            target[iit->first] = iit->second;
        }
    }
}

void PrtGet::expandWildcardsRepo( const list<char*>& in, list<string>& target )
{
    list<char*>::const_iterator it = in.begin();

    for ( ; it != in.end(); ++it ) {
        list<Package*> l;
        m_repo->getMatchingPackages( *it, l );
        list<Package*>::iterator iit = l.begin();
        for ( ; iit != l.end(); ++iit ) {
            target.push_back( (*iit)->name() );
        }
    }
}


void PrtGet::current()
{
    assertExactArgCount(1);

    const map<string, string>& installed = m_pkgDB->installedPackages();
    map<string, string>::const_iterator it = installed.begin();
    string search = *(m_parser->otherArgs().begin());

    for ( ; it != installed.end(); ++it ) {
        if ( it->first == search ) {
            cout << it->second.c_str() << endl;
            return;
        }
    }

    cout << "Package " << search << " not installed" << endl;
    m_returnValue = 1;
}

SignalHandler::HandlerResult PrtGet::handleSignal( int signal )
{
    // TODO: second argument could also be true:
    // TODO: kill installtransaction

    cout << "prt-get: interrupted" << endl;
    if ( m_currentTransaction ) {
        evaluateResult( *m_currentTransaction, false, true );
    }
}

/*!
  find files matching a pattern in repository

  \sa Repository::getMatchingPackages()
*/
void PrtGet::fsearch()
{
    assertMinArgCount(1);

    string arg = "*";
    if ( m_parser->otherArgs().size() == 1 ) {
        arg = *(m_parser->otherArgs().begin());
    }

    initRepo();
    const map<string, Package*>& packages = m_repo->packages();
    map<string, Package*>::const_iterator it = packages.begin();
    bool first = true;
    for ( ; it != packages.end(); ++it ) {
        list<string> matches;
        string fp =
            it->second->path() + "/" +
            it->second->name() + "/" + ".footprint";
        if ( File::grep( fp, arg, matches,
                         m_parser->fullPath(),
                         m_useRegex)) {
            if ( matches.size() > 0 ) {
                if ( first ) {
                    first = false;
                } else {
                    cout << endl;
                }
                cout << "Found in "
                     << it->second->path() << "/"
                     << it->first << ":" << endl;
                list<string>::iterator it = matches.begin();
                for ( ; it != matches.end(); ++it ) {
                    cout << "  " << *it << endl;
                }
            }
        }
    }

    if ( first ) {
        m_returnValue = PG_GENERAL_ERROR;
    }
}

void PrtGet::setLock( bool lock )
{
    assertMinArgCount(1);

    if ( lock ) {
        initRepo();
    }

    const list<char*>& args = m_parser->otherArgs();
    list<char*>::const_iterator it = args.begin();
    for ( ; it != args.end(); ++it ) {
        if ( lock ) {
            if (m_pkgDB->isInstalled( *it )) {
                if (!m_locker.lock( *it )) {
                    cerr << "Already locked: " << *it << endl;
                    m_returnValue = PG_GENERAL_ERROR;
                }
            } else {
                cerr << "Package '" << *it << "' not found" << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            }

        } else {
            if ( !m_locker.unlock( *it ) ) {
                cerr << "Not locked previously: " << *it << endl;
                m_returnValue = PG_GENERAL_ERROR;
                return;
            }
        }
    }
								
    if (!m_locker.store()) {
        cerr << "Failed to write lock data" << endl;
        m_returnValue = PG_GENERAL_ERROR;
    }
}

void PrtGet::listLocked()
{
    // shares some code with listInstalled
    if ( m_locker.openFailed() ) {
        cerr << "Failed to open lock data file" << endl;
        m_returnValue = PG_GENERAL_ERROR;
    }

    const map<string, string>& l = m_pkgDB->installedPackages();

    if ( l.empty() ) {
        return;
    }

    if ( m_parser->verbose() > 1 ) {
        // warning: will slow down the process...
        initRepo();
    }


    const vector<string>& lockedPackages = m_locker.lockedPackages();
    vector<string>::const_iterator it = lockedPackages.begin();
    for ( ; it != lockedPackages.end(); ++it ) {
        cout << *it;
        if ( m_parser->verbose() > 0 ) {
            cout << " " << m_pkgDB->getPackageVersion(*it);
        }
        if ( m_parser->verbose() > 1 ) {
            const Package* p = m_repo->getPackage( *it );
            if ( p ) {
                cout << ": " << p->description();
            }
        }

        cout << endl;

    }
}


void PrtGet::edit()
{
    assertMinArgCount(1);
    assertMaxArgCount(2);

    char* editor = getenv("EDITOR");
    if (editor) {
        initRepo();

        list<char*>::const_iterator it = m_parser->otherArgs().begin();
        string arg = *it;
        const Package* p = m_repo->getPackage( arg );
        if ( p ) {
            string fileName = "Pkgfile";
            if (++it != m_parser->otherArgs().end()) {
                fileName = *it;
            }
            string file = p->path() + "/" + p->name() + "/" + fileName;
            Process proc(editor, file);
            m_returnValue = proc.executeShell();
            if (m_returnValue) {
                cerr << "error while execution the editor" << endl;
            }
        } else {
            cerr << "Package '" << arg << "' not found" << endl;
            m_returnValue = PG_GENERAL_ERROR;
            return;
        }

    } else {
        cerr << "Environment variable EDITOR not set" << endl;;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

}

void PrtGet::ls()
{
    assertExactArgCount(1);

    initRepo();

    list<char*>::const_iterator it = m_parser->otherArgs().begin();
    string arg = *it;
    const Package* p = m_repo->getPackage( arg );
    if ( p ) {
        string dirname = p->path() + "/" + p->name();
        DIR* dir = opendir(dirname.c_str());
        struct dirent* entry;
        vector<string> files;
        while (entry = readdir(dir)) {
            string dName = entry->d_name;
            if (dName != "." && dName != "..") {
                files.push_back(dName);
            }
        }
        closedir(dir);

        sort(files.begin(), files.end());
        vector<string>::iterator fit = files.begin();
        for (; fit != files.end(); ++fit) {
            if (m_parser->printPath()) {
                cout << p->path() + "/" +p->name() + "/";
            }
            cout << *fit << endl;
        }
    } else {
        cerr << "Package '" << arg << "' not found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }
}

void PrtGet::cat()
{
    assertMinArgCount(1);
    assertMaxArgCount(2);

    initRepo();

    list<char*>::const_iterator it = m_parser->otherArgs().begin();
    string arg = *it;
    const Package* p = m_repo->getPackage( arg );
    if ( p ) {
        string fileName = "Pkgfile";
        if (++it != m_parser->otherArgs().end()) {
            fileName = *it;
        }
        string file = p->path() + "/" + p->name() + "/" + fileName;
        if (!printFile(file)) {
            cerr << "File '" << *it << "' not found" << endl;
            m_returnValue = PG_GENERAL_ERROR;
            return;
        }
    } else {
        cerr << "Package '" << arg << "' not found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }
}

void PrtGet::remove()
{
    assertMinArgCount(1);

    list<string> removed;
    list<string> failed;
    list<string> notInstalled;

    if ( m_parser->isTest() ) {
        cout << "*** " << m_appName << ": test mode" << endl;
    }

    string command = InstallTransaction::PKGRM_DEFAULT_COMMAND;
    if (m_config->removeCommand() != "") {
        command = m_config->removeCommand();
    }

    const list<char*>& args = m_parser->otherArgs();
    list<char*>::const_iterator it = args.begin();
    for ( ; it != args.end(); ++it ) {
        if (m_pkgDB->isInstalled(*it)) {
            // TODO: prettify
            string args = "";
            if (m_parser->installRoot() != "") {
                args = "-r " + m_parser->installRoot() + " ";
            }
            args += (m_parser->pkgrmArgs() + " " + *it);

            Process proc(command, args);
            if (m_parser->isTest() || proc.executeShell() == 0) {
                removed.push_back(*it);
                if (m_locker.isLocked(*it)) {
                    m_locker.unlock(*it);
                    m_locker.store();
                }
            } else {
                failed.push_back(*it);
            }
        } else {
            notInstalled.push_back(*it);
        }
    }

    if ( removed.size() ) {
        cout << endl << "-- Packages removed"
             << endl;
        list<string>::const_iterator it = removed.begin();

        for ( ; it != removed.end(); ++it ) {
            cout << *it << endl;
        }
    }

    if ( failed.size() ) {
        cout << endl << "-- Packages where removal failed"
             << endl;
        list<string>::const_iterator it = failed.begin();

        for ( ; it != failed.end(); ++it ) {
            cout << *it << endl;
        }
    }

    if ( notInstalled.size() ) {
        cout << endl << "-- Packages which were not installed"
             << endl;
        list<string>::const_iterator it = notInstalled.begin();

        for ( ; it != notInstalled.end(); ++it ) {
            cout << *it << endl;
        }
    }

    if ( m_parser->isTest() ) {
        cout << "*** " << m_appName << ": test mode end" << endl;
    }



}

void PrtGet::assertMaxArgCount(int count)
{
    if ( m_parser->otherArgs().size() > count ) {
        argCountFailure(count, "at most");
    }
}

void PrtGet::assertExactArgCount(int count)
{
    if ( m_parser->otherArgs().size() != count ) {
        argCountFailure(count, "exactly");
    }
}

void PrtGet::assertMinArgCount(int count)
{
     if ( m_parser->otherArgs().size() < count ) {
         argCountFailure(count, "at least");
     }
}

void PrtGet::argCountFailure(int count, const string& specifier)
{
    cerr << m_appName << " "
         << m_parser->commandName() << " takes " << specifier << " "
         << count << (count > 1 ? " arguments" : " argument") << endl;
    exit(PG_ARG_ERROR);
}


void PrtGet::printDependTree()
{
    assertExactArgCount(1);

    initRepo();

    list<char*>::const_iterator it = m_parser->otherArgs().begin();
    string arg = *it;
    const Package* p = m_repo->getPackage( arg );
    if (!p) {
        cerr << "Package '" << arg << "' not found" << endl;
        m_returnValue = PG_GENERAL_ERROR;
        return;
    }

    if (p->dependencies().length() > 0) {

        cout << "-- dependencies ([i] = installed";
        if (!m_parser->all()) {
            cout << ", '-->' = seen before";
        }
        cout << ")" << endl;
        if ( m_pkgDB->isInstalled( *it ) ) {
            cout << "[i] ";
        } else {
            cout << "[ ] ";
        }
        cout << p->name() << endl;
        printDepsLevel(2, p);
    }

}

void PrtGet::printDepsLevel(int indent, const Package* package)
{
    static map<string, bool> shownMap;

    list<string> deps;
    StringHelper::split(package->dependencies(), ',', deps);
    list<string>::iterator it = deps.begin();
    bool isAlias = false;
    string aliasName = "";

    for (; it != deps.end(); ++it) {
        if ( m_pkgDB->isInstalled( *it, true, &isAlias, &aliasName ) ) {
            cout << "[i] ";
        } else {
            cout << "[ ] ";
        }
        for (int i = 0; i < indent; ++i) {
            cout << " ";
        }
        cout << *it;
        if (isAlias) {
            cout << " (provided by " << aliasName << ")";
        }
        const Package* p = m_repo->getPackage( *it );
        if (p) {
            if (p->dependencies().length() > 0) {
                map<string, bool>::iterator shownIt = shownMap.find(*it);
                if (shownIt != shownMap.end()) {
                    cout << " -->" << endl;;
                } else {
                    cout << endl;
                    printDepsLevel(indent+2, p);
                    if (!m_parser->all()) {
                        shownMap[*it] = true;
                    }
                }
            } else {
                cout << endl;
            }
        } else {
            cout << " (not found in ports tree)" << endl;
        }
    }
}

void PrtGet::dumpConfig()
{

    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "Alias file: " << PkgDB::ALIAS_STORE << endl;

    if (!m_parser->noStdConfig()) {
        string fName = CONF_FILE;
        if ( m_parser->isAlternateConfigGiven() ) {
            fName = m_parser->alternateConfigFile();
        }
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Configuration file: " << fName << endl;
    }

    if (m_config->cacheFile() != "") {
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Cache file: " << m_config->cacheFile() << endl;
    }
    if (m_config->makeCommand() != "") {
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Make command file: " << m_config->makeCommand() << endl;
    }
    if (m_config->addCommand() != "") {
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Add command: " << m_config->addCommand() << endl;
    }
    if (m_config->removeCommand() != "") {
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Remove command: " << m_config->removeCommand() << endl;
    }
    if (m_config->runscriptCommand() != "") {
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Runscript command: " << m_config->runscriptCommand() << endl;
    }

    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "Run scripts: " <<(m_config->runScripts() ? "yes" : "no" )
         << endl;

    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "Keep higher version:" <<(m_config->preferHigher() ? "yes" : "no" )
         << endl;

    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "Readme mode:  ";
    switch (m_config->readmeMode()) {
        case Configuration::VERBOSE_README:
            cout << "verbose";
            break;
        case Configuration::COMPACT_README:
            cout << "compact";
            break;
        case Configuration::NO_README:
            cout << "off";
            break;
    }
    cout << endl;

    cout << endl;

    if (m_config->logFilePattern() != "") {
        cout.setf( ios::left, ios::adjustfield );
        cout.width( 20 );
        cout.fill( ' ' );
        cout << "Log file: " << m_config->logFilePattern() << endl;
    }
    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "  Write log: " << (m_config->writeLog() ? "yes" : "no" ) << endl;
    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "  Append log: " <<(m_config->appendLog() ? "yes" : "no" ) << endl;

    cout << endl;
    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "Pkgmk settings: " << m_config->logFilePattern() << endl;
    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "  Package dir: " << InstallTransaction::getPkgmkPackageDir() 
         << endl;
    
    cout.setf( ios::left, ios::adjustfield );
    cout.width( 20 );
    cout.fill( ' ' );
    cout << "  Compression mode: " 
         << InstallTransaction::getPkgmkCompressionMode() << endl;


    cout << endl;
    list< pair<string, string> >::const_iterator it =
        m_config->rootList().begin();
    cout << "Port "
         << (m_config->rootList().size() == 1 ? "directory" : "directories")
         << ": " << endl;
    for (; it != m_config->rootList().end(); ++it) {
        cout << " " << it->first;
        if (it->second != "") {
            cout << " (" << it->second << ")";
        }
        cout << endl;
    }
}
