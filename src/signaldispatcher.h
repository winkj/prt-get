////////////////////////////////////////////////////////////////////////
// FILE:        signaldispatcher.h
// AUTHOR:      Johannes Winkelmann, jw@tks6.net
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify  
//  it under the terms of the GNU General Public License as published by  
//  the Free Software Foundation; either version 2 of the License, or     
//  (at your option) any later version.                                   
////////////////////////////////////////////////////////////////////////

#ifndef _SIGNALDISPATCHER_H_
#define _SIGNALDISPATCHER_H_

#include <map>

/*!
  signal handler for the SignalDispatcher class. Implement this class to 
  receive signals
  \brief SignalHandler for SignalDispatcher
*/
class SignalHandler
{
public:
    /*! Result of a handlSignal() call */
    enum HandlerResult { 
        SIGNAL_NOT_HANDLED, /*!< not handled */
        EXIT,               /*!< signal handled, exit now */
        CONTINUE            /*!< signal handled, don't exit */
    }; 
    virtual HandlerResult handleSignal( int signalNumber ) = 0;
};

/*!
  dispatches signals. Singleton, use the instance() method to access 
  the instance of this class. Register your SignalHandler to handle signals

  \brief Dispatch unix signals
*/
class SignalDispatcher
{
public:
    static SignalDispatcher* instance();
    static void dispatch( int signalNumber );
    
    void registerHandler( SignalHandler* handler, int signalNumber );
    void unregisterHandler( int signalNumber );    
    
protected:
    SignalDispatcher();
    
private:
    static SignalDispatcher* m_instance;
    std::map<int, SignalHandler*> m_signalHandlers;
    
};

#endif /* _SIGNALDISPATCHER_H_ */
