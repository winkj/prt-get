////////////////////////////////////////////////////////////////////////
// FILE:        main.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <signal.h>
#include <iostream>
using namespace std;

#include "argparser.h"
#include "prtget.h"
#include "signaldispatcher.h"

int main( int argc, char** argv )
{
    ArgParser argParser( argc, argv );
    if ( !argParser.parse() ) {
        if (argParser.unknownOption().size() > 0){
            cerr << "prt-get: Unknown option: " << argParser.unknownOption()
                 << endl;
        } else if ( !argParser.isCommandGiven() ) {
            if (argParser.commandName() != "") {
                cerr << "prt-get: Unknown command '"
                     << argParser.commandName() << "'. "
                     << "try prt-get help for more information" << endl;
            } else {
                cerr << "prt-get: no command given. "
                     << "try prt-get help for more information" << endl;
            }
        }

        exit( -1 );
    }
    if ( argParser.verbose() > 2 ) {
        cerr << "prt-get: can't specify both -v and -vv. " << endl;
        exit( -1 );
    }

    PrtGet prtGet( &argParser );

    signal( SIGHUP, SignalDispatcher::dispatch );
    signal( SIGINT, SignalDispatcher::dispatch );
    signal( SIGQUIT, SignalDispatcher::dispatch );
    signal( SIGILL, SignalDispatcher::dispatch );

    SignalDispatcher::instance()->registerHandler( &prtGet, SIGINT );
    SignalDispatcher::instance()->registerHandler( &prtGet, SIGHUP );
    SignalDispatcher::instance()->registerHandler( &prtGet, SIGQUIT );
    SignalDispatcher::instance()->registerHandler( &prtGet, SIGILL );


    ArgParser::Type command = argParser.commandType();
    switch ( command )
    {
        case ArgParser::HELP:
            prtGet.printUsage();
            break;
        case ArgParser::SHOW_VERSION:
            prtGet.printVersion();
            break;
        case ArgParser::LIST:
            prtGet.listPackages();
            break;
        case ArgParser::DUP:
            prtGet.listShadowed();
            break;
        case ArgParser::SEARCH:
            prtGet.searchPackages();
            break;
        case ArgParser::DSEARCH:
            prtGet.searchPackages( true );
            break;
        case ArgParser::INFO:
            prtGet.printInfo();
            break;
        case ArgParser::ISINST:
            prtGet.isInstalled();
            break;
        case ArgParser::INSTALL:
            prtGet.install();
            break;
        case ArgParser::DEPINST:
            prtGet.install(false, true, true);
            break;
        case ArgParser::GRPINST:
            prtGet.install( false, true );
            break;
        case ArgParser::DEPENDS:
            prtGet.printDepends();
            break;
        case ArgParser::QUICKDEP:
            prtGet.printDepends( true );
            break;
        case ArgParser::UPDATE:
            prtGet.install( true );
            break;
        case ArgParser::DIFF:
            prtGet.printDiff();
            break;
        case ArgParser::QUICKDIFF:
            prtGet.printQuickDiff();
            break;
        case ArgParser::CREATE_CACHE:
            prtGet.createCache();
            break;
        case ArgParser::PATH:
            prtGet.printPath();
            break;
        case ArgParser::LISTINST:
            prtGet.listInstalled();
            break;
        case ArgParser::PRINTF:
            prtGet.printf();
            break;
        case ArgParser::README:
            prtGet.readme();
            break;
        case ArgParser::DEPENDENT:
            prtGet.printDependent();
            break;
        case ArgParser::SYSUP:
            prtGet.sysup();
            break;
        case ArgParser::CURRENT:
            prtGet.current();
            break;
        case ArgParser::FSEARCH:
            prtGet.fsearch();
            break;
        case ArgParser::LOCK:
            prtGet.setLock( true );
            break;
        case ArgParser::UNLOCK:
            prtGet.setLock( false );
            break;
        case ArgParser::LISTLOCKED:
            prtGet.listLocked();
            break;
        case ArgParser::CAT:
            prtGet.cat();
            break;
        case ArgParser::LS:
            prtGet.ls();
            break;
        case ArgParser::EDIT:
            prtGet.edit();
            break;
        case ArgParser::REMOVE:
            prtGet.remove();
            break;
        case ArgParser::DEPTREE:
            prtGet.printDependTree();
            break;
        case ArgParser::DUMPCONFIG:
            prtGet.dumpConfig();
            break;
        default:
            cerr << "unknown command" << endl;
            break;
    }


    return prtGet.returnValue();
}
