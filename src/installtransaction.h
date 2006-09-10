////////////////////////////////////////////////////////////////////////
// FILE:        installtransaction.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _INSTALLTRANSACTION_H_
#define _INSTALLTRANSACTION_H_

#include <string>
#include <list>
#include <map>
#include <vector>
#include <utility>
using namespace std;

#include "depresolver.h"

class Repository;
class PkgDB;
class Package;
class ArgParser;
class Configuration;
/*!
  \class InstallTransaction
  \brief Transaction for installing/updating a list of packages
*/
class InstallTransaction
{
public:
    InstallTransaction( const list<char*>& names,
                        const Repository* repo,
                        PkgDB* pkgDB,
                        const Configuration* config );
    InstallTransaction( const list<string>& names,
                        const Repository* repo,
                        PkgDB* pkgDB,
                        const Configuration* config );

    static const std::string PKGMK_DEFAULT_COMMAND;
    static const std::string PKGADD_DEFAULT_COMMAND;
    static const std::string PKGRM_DEFAULT_COMMAND;


    /*! Result of an installation */
    enum InstallResult {
        SUCCESS,             /*!< yeah, success */
        NO_PACKAGE_GIVEN,    /*!< no package give to install */
        PACKAGE_NOT_FOUND,   /*!< package not found */
        PKGMK_EXEC_ERROR,    /*!< can't execute pkgmk */
        PKGMK_FAILURE,       /*!< error while pkgmk */
        PKGADD_EXEC_ERROR,   /*!< can't execute pkgadd */
        PKGDEST_ERROR,             /*!< can't change to PKGDEST */
        PKGADD_FAILURE,      /*!< error while pkgadd */
        CYCLIC_DEPEND,       /*!< cyclic dependencies found */
        LOG_DIR_FAILURE,     /*!< couldn't create log directory */
        LOG_FILE_FAILURE,    /*!< couldn't create log file */
        NO_LOG_FILE,         /*!< no log file specified */
        CANT_LOCK_LOG_FILE   /*!< can't create lock for log file */
    };

    enum State {
        EXEC_SUCCESS,
        FAILED,
        NONEXISTENT
    };
    struct InstallInfo {
        InstallInfo(bool hasReadme_) {
            hasReadme = hasReadme_;
            preState = NONEXISTENT;
            postState = NONEXISTENT;
        }
        State preState;
        State postState;
        bool hasReadme;
    };

    InstallResult install( const ArgParser* parser,
                           bool update,
                           bool group );
    InstallResult  calcDependencies();

    const list< pair<string, InstallInfo> >& installedPackages() const;
    const list<string>& alreadyInstalledPackages() const;
    const list<string>& ignoredPackages() const;


    const list<string>& dependencies() const;
    const list< pair<string,string> >& missing() const;
    const list< pair<string, InstallInfo> >& installError() const;

private:
    bool calculateDependencies();
    void checkDependecies( const Package* package, int depends=-1 );

    InstallResult installPackage( const Package* package,
                                  const ArgParser* parser,
                                  bool update,
                                  InstallInfo& info ) const;

    static string getPkgDest(const string& installRoot);

    PkgDB* m_pkgDB;
    DepResolver m_resolver;
    const Repository* m_repo;

    // packages to be installed
    list< pair<string, const Package*> > m_packages;

    // boolean used to implement lazy initialization
    bool m_depCalced;

    // packages< pair<name, hasReadme> > installed by this transaction
    list< pair<string, InstallInfo> > m_installedPackages;

    // packages which were requested to be installed which where already
    list<string> m_alreadyInstalledPackages;

    // packages which are required by the transaction, but ignored by
    // the user
    list<string> m_ignoredPackages;

    list<string> m_depNameList;
    vector<string> m_depList;

    // packages requested to be installed not found in the ports tree
    list< pair<string, string> > m_missingPackages;

    // packages where build/installed failed
    list< pair<string, InstallInfo> > m_installErrors;

    /// prt-get itself
    const Configuration* m_config;

};

#endif /* _INSTALLTRANSACTION_H_ */
