////////////////////////////////////////////////////////////////////////
// FILE:        process.cpp
// AUTHORS:     Johannes Winkelmann, jw@tks6.net
//              Output redirection by Logan Ingalls, log@plutor.org
// COPYRIGHT:   (c) 2002 by Johannes Winkelmann
// ---------------------------------------------------------------------
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
////////////////////////////////////////////////////////////////////////

#include <list>
using namespace std;

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdio>

#include "process.h"

#include "stringhelper.h"
using namespace StringHelper;


/*!
  create a process
  \param app the application to execute
  \param arguments the arguments to be passed to \a app
  \param fdlog file descriptor to a log file
*/
Process::Process( const string& app, const string& arguments, int fdlog )
  : m_app( app ), m_arguments( arguments ), m_fdlog( fdlog )
{
}

/*!
  execute the process
  \return the exit status of the application
*/
int Process::execute()
{
    list<string> args;
    split( m_arguments, ' ', args, 0, false );

    const int argc = 1 + args.size() + 1; // app, args, NULL

    char** argv = new char*[argc];
    list<string>::iterator it = args.begin();

    int i = 0;
    argv[i] = const_cast<char*>( m_app.c_str() );
    for ( ; it != args.end(); ++it ) {
        ++i;
        argv[i] = const_cast<char*>( it->c_str() );
    }

    ++i;
    assert( i+1 == argc );
    argv[i] = NULL;
    int status = 0;

    if ( m_fdlog > 0 ) {
        status = execLog(argc, argv);
    } else {
        status = exec(argc, argv);
    }
    delete [] argv;

    return status;
}


int Process::execLog(const int argc, char** argv)
{
    int status = 0;
    int fdpipe[2];
    pipe( fdpipe );

    pid_t pid = fork();
    if ( pid == 0 ) {
        // child process
        close( fdpipe[0] );
	dup2( fdpipe[1], STDOUT_FILENO );
	dup2( fdpipe[1], STDERR_FILENO );

        execv( m_app.c_str(), argv );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        close( fdpipe[1] );

	char readbuf[1024];
	int bytes, wpval;

	while ( (wpval = waitpid ( pid, &status, WNOHANG )) == 0 ) {
	    while ( (bytes=read(fdpipe[0], readbuf, sizeof(readbuf)-1)) > 0 ) {
	        readbuf[bytes] = 0;
		printf("%s", readbuf);
                fflush(stdout);
                fflush(stderr);
                write( m_fdlog, readbuf, bytes );
	    }
	}
        close( fdpipe[0] );

        if ( wpval != pid ) {
            status = -1;
        }
    }

    return status;
}


int Process::exec(const int argc, char** argv)
{
    int status = 0;
    pid_t pid = fork();
    if ( pid == 0 ) {
        // child process
        execv( m_app.c_str(), argv );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        if ( waitpid ( pid, &status, 0 ) != pid ) {
            status = -1;
        }
    }
    return status;
}

/*!
  execute the process using the shell
  \return the exit status of the application

  \todo make shell exchangable
*/
int Process::executeShell()
{
    // TODO: make shell exchangable
    static const char SHELL[] = "/bin/sh";
    int status = 0;
    if ( m_fdlog > 0 ) {
        status = execShellLog(SHELL);
    } else {
        status = execShell(SHELL);
    }

    return status;
}


int Process::execShellLog(const char* SHELL)
{
    int status = 0;

    int fdpipe[2];
    pipe( fdpipe );

    pid_t pid = fork();
    if ( pid == 0 ) {
        // child process
        close( fdpipe[0] );
	dup2( fdpipe[1], STDOUT_FILENO );
	dup2( fdpipe[1], STDERR_FILENO );

        execl( SHELL, SHELL, "-c", (m_app + " " + m_arguments).c_str(), NULL );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        close( fdpipe[1] );

	char readbuf[1024];
	int bytes, wpval;

	while ( (wpval = waitpid ( pid, &status, WNOHANG )) == 0 ) {
	    while ( (bytes=read(fdpipe[0], readbuf, sizeof(readbuf)-1)) > 0 ) {
	        readbuf[bytes] = 0;
		printf("%s", readbuf);
                fflush(stdout);
                fflush(stderr);
                write( m_fdlog, readbuf, bytes );
	    }
	}
        close( fdpipe[0] );

        if ( wpval != pid ) {
            status = -1;
        }
    }

    return status;
}

int Process::execShell(const char* SHELL)
{
    int status = 0;

    pid_t pid = fork();
    if ( pid == 0 ) {
        execl( SHELL, SHELL, "-c", (m_app + " " + m_arguments).c_str(), NULL );
        _exit( EXIT_FAILURE );
    } else if ( pid < 0 ) {
        // fork failed
        status = -1;
    } else {
        // parent process
        if ( waitpid ( pid, &status, 0 ) != pid ) {
            status = -1;
        }
    }

    return status;
}
