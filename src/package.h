////////////////////////////////////////////////////////////////////////
// FILE:        package.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#ifndef _PACKAGE_H_
#define _PACKAGE_H_

#include <string>

struct PackageData;

/*!
  \class Package
  \brief representation of a package

  Representation of a package from the crux ports tree
*/
class Package
{
public:
    Package( const std::string& name,
             const std::string& path );

    Package( const std::string& name,
             const std::string& path,
             const std::string& version,
             const std::string& release,
             const std::string& description,
             const std::string& dependencies,
             const std::string& url,
             const std::string& packager,
             const std::string& maintainer,
             const std::string& hasReadme,
             const std::string& hasPreInstall,
             const std::string& hasPostInstall );

    ~Package();

    const std::string& name() const;
    const std::string& path() const;
    const std::string& version() const;
    const std::string& release() const;
    const std::string& description() const;
    const std::string& dependencies() const;
    const std::string& url() const;
    const std::string& packager() const;
    const std::string& maintainer() const;
    const bool hasReadme() const;
    const bool hasPreInstall() const;
    const bool hasPostInstall() const;

    void setDependencies( const std::string& dependencies );


private:
    void load() const;
    
    static void expandShellCommands(std::string& input, 
                                    const time_t& timeNow,
                                    const struct utsname unameBuf);

    mutable PackageData* m_data;
    mutable bool m_loaded;

  };

struct PackageData
{
    PackageData( const std::string& name_,
                 const std::string& path_,
                 const std::string& version_="",
                 const std::string& release_="",
                 const std::string& description_="",
                 const std::string& dependencies_="",
                 const std::string& url_="",
                 const std::string& packager="",
                 const std::string& maintainer="",
                 const std::string& hasReadme_="",
                 const std::string& hasPreInstall_="",
                 const std::string& hasPostInstall_="");

    std::string name;
    std::string path;
    std::string version;
    std::string release;
    std::string description;
    std::string depends;
    std::string url;
    std::string packager;
    std::string maintainer;

    bool hasReadme;
    bool hasPreInstall;
    bool hasPostInstall;
};

#endif /* _PACKAGE_H_ */
