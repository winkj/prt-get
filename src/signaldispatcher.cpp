////////////////////////////////////////////////////////////////////////
// FILE:        signaldispatcher.cpp
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <iostream>
using namespace std;

#include "signaldispatcher.h"

SignalDispatcher* SignalDispatcher::m_instance = 0;

SignalDispatcher::SignalDispatcher()
{

}

SignalDispatcher* SignalDispatcher::instance()
{
    if ( m_instance == 0 ) {
        m_instance = new SignalDispatcher();
    }

    return m_instance;
}

void SignalDispatcher::dispatch( int signalNumber )
{
    map<int, SignalHandler*>::iterator it = 
        SignalDispatcher::instance()->m_signalHandlers.find( signalNumber );
    if ( it !=  SignalDispatcher::instance()->m_signalHandlers.end() ) {
        it->second->handleSignal( signalNumber );
    } else {
        cerr << "prt-get: caught signal " << signalNumber << endl;
    }
    exit( signalNumber );
}

void SignalDispatcher::registerHandler( SignalHandler* handler,
                                        int signalNumber )
{
    m_signalHandlers[signalNumber] = handler;
}

void SignalDispatcher::unregisterHandler( int signalNumber )
{
    m_signalHandlers.erase( signalNumber );
}
