////////////////////////////////////////////////////////////////////////
// FILE:        package.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////


#include <iostream>
#include <cstdio>
#include <sys/stat.h>
#include <sys/utsname.h>
using namespace std;

#include "package.h"
#include "stringhelper.h"
using namespace StringHelper;



/*!
  Create a package, which is not yet fully initialized, This is interesting
  in combination with the lazy initialization
*/
Package::Package( const string& name,
                  const string& path )
    : m_loaded( false )
{
    m_data = new PackageData( name, path );
}

/*!
  Create a fully initialized package. Most interesting when created from a
  cache file
*/
Package::Package( const string& name,
                  const string& path,
                  const string& version,
                  const string& release,
                  const string& description,
                  const string& dependencies,
                  const string& url,
                  const string& packager,
                  const string& maintainer,
                  const string& hasReadme,
                  const string& hasPreInstall,
                  const string& hasPostInstall)
    : m_loaded( true )
{
    m_data = new PackageData( name, path, version, release,
                              description, dependencies, url,
                              packager, maintainer, hasReadme,
                              hasPreInstall, hasPostInstall );

}

Package::~Package()
{
    delete m_data;
}

/*! \return the name of this package */
const string& Package::name() const
{
    return m_data->name;
}

/*! \return the path to this package */
const string& Package::path() const
{
    return m_data->path;
}

/*! \return the version of this package */
const string& Package::version() const
{
    load();
    return m_data->version;
}

/*! \return the release number of this package */
const string& Package::release() const
{
    load();
    return m_data->release;
}

/*! \return the description field of this package */
const string& Package::description() const
{
    load();
    return m_data->description;
}

/*! \return the dependency line of this package */
const string& Package::dependencies() const
{
    load();
    return m_data->depends;
}

/*! \return the url of this package */
const string& Package::url() const
{
    load();
    return m_data->url;
}

/*! \return the packager of this package */
const string& Package::packager() const
{
    load();
    return m_data->packager;
}
/*! \return the maintainer of this package */
const string& Package::maintainer() const
{
    load();
    return m_data->maintainer;
}

/*! \return whether or not this package has a readme file */
const bool Package::hasReadme() const
{
    load();
    return m_data->hasReadme;
}

/*! \return a typically formatted version-release string */
string Package::versionReleaseString() const
{
    load();
    return m_data->versionReleaseString;
}

const bool Package::hasPreInstall() const
{
    return m_data->hasPreInstall;
}

const bool Package::hasPostInstall() const
{
    return m_data->hasPostInstall;
}

/*!
  load from Pkgfile
*/
void Package::load() const
{
    if ( m_loaded ) {
        return;
    }
    m_loaded = true;

    string fileName = m_data->path + "/" + m_data->name + "/Pkgfile";

    // c IO is about four times faster then fstream :-(
    FILE* fp = fopen( fileName.c_str(), "r" );
    if ( fp == NULL ) {
        return;
    }


    const int length = BUFSIZ;
    char input[length];
    string line;

    time_t timeNow;
    time(&timeNow);

    struct utsname unameBuf;
    if (uname(&unameBuf) != 0) {
        unameBuf.release[0] = '\0';
    }



    while ( fgets( input, length, fp ) ) {

        line = stripWhiteSpace( input );

        if ( line.substr( 0, 8 ) == "version=" ) {
            m_data->version = getValueBefore( getValue( line, '=' ), '#' );
            m_data->version = stripWhiteSpace( m_data->version );

            expandShellCommands(m_data->version, timeNow, unameBuf);
        } else if ( line.substr( 0, 8 ) == "release=" ) {
            m_data->release = getValueBefore( getValue( line, '=' ), '#' );
            m_data->release = stripWhiteSpace( m_data->release );
        } else if ( line[0] == '#' ) {
            while ( !line.empty() &&
                    ( line[0] == '#' || line[0] == ' ' || line[0] == '\t' ) ) {
                line = line.substr( 1 );
            }
            string::size_type pos = line.find( ':' );
            if ( pos != string::npos ) {
                if ( startwith_nocase( line, "desc" ) ) {
                    m_data->description =
                        stripWhiteSpace( getValue( line, ':' ) );

                } else if ( startwith_nocase( line, "pack" ) ) {
                    m_data->packager =
                        stripWhiteSpace( getValue( line, ':' ) );
                } else if ( startwith_nocase( line, "maint" ) ) {
                    m_data->maintainer =
                        stripWhiteSpace( getValue( line, ':' ) );
                } else if ( startwith_nocase( line, "url" ) ) {
                    m_data->url = stripWhiteSpace( getValue( line, ':' ) );
                } else if ( startwith_nocase( line, "dep" ) ) {
                    string depends = stripWhiteSpace( getValue( line, ':' ) );

                    StringHelper::replaceAll( depends, " ", "," );
                    StringHelper::replaceAll( depends, ",,", "," );

                    // TODO: decide which one to use
#if 0
                    // remove commented out packages
                    list<string> deps = StringHelper::split( depends, ',' );
                    list<string>::iterator it = deps.begin();
                    for ( ; it != deps.end(); ++it ) {
                        if ( (*it)[0] == '#' ) {
                            cerr << "Commented dep: " << *it << endl;
                        } else {
                            if ( it != deps.begin() ) {
                                m_data->depends += ",";
                            }
                            m_data->depends += *it;
                        }
                    }
#else
                    m_data->depends = depends;
#endif

                }
            }
        }
    }
    fclose( fp );

    m_data->generateVersionReleaseString();

    string file = m_data->path + "/" + m_data->name + "/README";
    struct stat buf;
    if ( stat( file.c_str(), &buf ) != -1) {
        m_data->hasReadme = true;
    }
    file = m_data->path + "/" + m_data->name + "/pre-install";
    if ( stat( file.c_str(), &buf ) != -1) {
        m_data->hasPreInstall = true;
    }
    file = m_data->path + "/" + m_data->name + "/post-install";
    if ( stat( file.c_str(), &buf ) != -1) {
        m_data->hasPostInstall = true;
    }

}

void Package::setDependencies( const std::string& dependencies )
{
    m_data->depends = dependencies;
}



PackageData::PackageData( const string& name_,
                          const string& path_,
                          const string& version_,
                          const string& release_,
                          const string& description_,
                          const string& dependencies_,
                          const string& url_,
                          const string& packager_,
                          const string& maintainer_,
                          const string& hasReadme_,
                          const string& hasPreInstall_,
                          const string& hasPostInstall_ )
    : name( name_ ),
      path( path_ ),
      version( version_ ),
      release( release_ ),
      description( description_ ),
      depends( dependencies_ ),
      url( url_ ),
      packager( packager_ ),
      maintainer( maintainer_ )

{
    generateVersionReleaseString();
    
    hasReadme = ( stripWhiteSpace( hasReadme_ ) == "yes" );
    hasPreInstall = ( stripWhiteSpace( hasPreInstall_ ) == "yes" );
    hasPostInstall = ( stripWhiteSpace( hasPostInstall_ ) == "yes" );
}

void PackageData::generateVersionReleaseString()
{
    versionReleaseString = version + "-" + release;
}


void Package::expandShellCommands(std::string& input,
                                  const time_t& timeNow,
                                  const struct utsname unameBuf)
{
    // TODO: consider dropping either of the tagsets, depending on feedback

    static const int TAG_COUNT = 2;
    string startTag[TAG_COUNT] = { "`", "$(" };
    string endTag[TAG_COUNT] = { "`", ")" };

    for (int i = 0; i < TAG_COUNT; ++i) {
        string::size_type pos;
        while ((pos = input.find(startTag[i])) != string::npos) {

            if (unameBuf.release) {
                input = replaceAll(input,
                                   startTag[i] + "uname -r" + endTag[i],
                                   unameBuf.release);
            }

            pos = input.find(startTag[i] + "date");
            if (pos != string::npos) {
                // NOTE: currently only works for one date pattern
                string::size_type startpos, endpos;
                endpos = input.find(endTag[i], pos+1);
                startpos = input.find('+', pos+1);

                string format = input.substr(startpos+1, endpos-startpos-1);

                // support date '+...' and date "+..."
                int len = format.length();
                if (format[len-1] == '\'' || format[len-1] == '"') {
                    format = format.substr(0, len-1);
                }
                char timeBuf[32];
                strftime(timeBuf, 32, format.c_str(), localtime(&timeNow));

                input = input.substr(0, pos) + timeBuf +
                    input.substr(endpos+1);
            }
        }
    }
}
