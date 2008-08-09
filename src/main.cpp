/******************************************************************************\
 * Copyright (c) 2004-2008
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include <qapplication.h>
#include <qmessagebox.h>
#include <iostream>
#include "global.h"
#include "llconclientdlg.h"
#include "llconserverdlg.h"
#include "settings.h"


/* Implementation *************************************************************/
// these pointers are only used for the post-event routine
QApplication* pApp   = NULL;
QDialog* pMainWindow = NULL;


int main ( int argc, char** argv )
{
	std::string strArgument;
    double      rDbleArgument;

    /* check if server or client application shall be started */
    bool        bIsClient             = true;
    bool        bUseGUI               = true;
    bool        bUseServerLogging     = false;
    quint16     iPortNumber           = LLCON_PORT_NUMBER;
	std::string strIniFileName        = "";
    std::string strHTMLStatusFileName = "";
    std::string strServerName         = "";

	/* QT docu: argv()[0] is the program name, argv()[1] is the first
	   argument and argv()[argc()-1] is the last argument.
	   Start with first argument, therefore "i = 1" */
	for ( int i = 1; i < argc; i++ )
	{
		/* Server mode flag ------------------------------------------------------- */
		if ( GetFlagArgument ( argc, argv, i, "-s", "--server" ) )
		{
			bIsClient = false;
            cerr << "server mode chosen" << std::endl;
			continue;
		}

		/* Use GUI flag ----------------------------------------------------------- */
		if ( GetFlagArgument ( argc, argv, i, "-n", "--nogui" ) )
		{
			bUseGUI = false;
            cerr << "no GUI mode chosen" << std::endl;
			continue;
		}

		/* Use logging flag ------------------------------------------------------- */
		if ( GetFlagArgument ( argc, argv, i, "-l", "--log" ) )
		{
			bUseServerLogging = true;
            cerr << "logging enabled" << std::endl;
			continue;
		}

		/* Port number ------------------------------------------------------------ */
		if ( GetNumericArgument ( argc, argv, i, "-p", "--port",
                                  0, 65535, rDbleArgument ) )
		{
			iPortNumber = static_cast<quint16> ( rDbleArgument );
            cerr << "selected port number: " << iPortNumber << std::endl;
			continue;
		}

		/* HTML status file ------------------------------------------------------- */
		if ( GetStringArgument ( argc, argv, i, "-m", "--htmlstatus", strArgument ) )
		{
			strHTMLStatusFileName = strArgument;
            cerr << "HTML status file name: " << strHTMLStatusFileName << std::endl;
			continue;
		}

		if ( GetStringArgument ( argc, argv, i, "-a", "--servername", strArgument ) )
		{
			strServerName = strArgument;
            cerr << "server name for HTML status file: " << strServerName << std::endl;
			continue;
		}

		/* Initialization file ---------------------------------------------------- */
		if ( GetStringArgument ( argc, argv, i, "-i", "--inifile", strArgument ) )
		{
			strIniFileName = strArgument;
            cerr << "initialization file name: " << strIniFileName << std::endl;
			continue;
		}

		/* Help (usage) flag ------------------------------------------------------ */
		if ( ( !strcmp ( argv[i], "--help" ) ) ||
			 ( !strcmp ( argv[i], "-h" ) ) || ( !strcmp ( argv[i], "-?" ) ) )
		{
            const std::string strHelp = UsageArguments(argv);
            cerr << strHelp;
			exit ( 1 );
		}

		/* Unknown option --------------------------------------------------------- */
		cerr << argv[0] << ": ";
		cerr << "Unknown option '" << argv[i] << "' -- use '--help' for help"
			<< endl;

		exit ( 1 );
	}

    // Application object
    QApplication app ( argc, argv, bUseGUI );

    // init resources
    extern int qInitResources();
    qInitResources();

    try
    {
        if ( bIsClient )
        {
            // client
            // actual client object
            CClient Client;

            // load settings from init-file
            CSettings Settings ( &Client );


// TODO use QString

            Settings.Load ( strIniFileName.c_str() );

            // GUI object
            CLlconClientDlg ClientDlg ( &Client, 0 );

            // set main window
            pMainWindow = &ClientDlg;
            pApp = &app; // Needed for post-event routine

            // show dialog
            ClientDlg.show();
            app.exec();

            // save settings to init-file

// TODO use QString

            Settings.Save ( strIniFileName.c_str() );
        }
        else
        {
            // server
            // actual server object

// TODO use QString

            CServer Server ( bUseServerLogging, iPortNumber,
                             strHTMLStatusFileName.c_str(),
                             strServerName.c_str() );

            if ( bUseGUI )
            {
                // GUI object for the server
                CLlconServerDlg ServerDlg ( &Server, 0 );

                // set main window
                pMainWindow = &ServerDlg;
                pApp = &app; // needed for post-event routine

                // show dialog
                ServerDlg.show();
                app.exec();
            }
            else
            {
                // only start application without using the GUI
                qDebug() << CAboutDlg::GetVersionAndNameStr ( false );
                app.exec();
            }
        }
    }

    catch ( CGenErr generr )
    {
        // show generic error
        if ( bUseGUI )
        {
            QMessageBox::critical ( 0, APP_NAME, generr.GetErrorText(), "Quit", 0 );
        }
        else
        {
            qDebug() << generr.GetErrorText();
        }
    }

    return 0;
}


/******************************************************************************\
* Command Line Argument Parsing                                                *
\******************************************************************************/
std::string UsageArguments ( char **argv )
{
	return
		"Usage: " + std::string ( argv[0] ) + " [option] [argument]\n"
		"Recognized options:\n"
		"  -s, --server               start server\n"
		"  -n, --nogui                disable GUI (only avaiable for server)\n"
		"  -l, --log                  enable logging\n"
		"  -i, --inifile              initialization file name (only available for client)\n"
		"  -p, --port                 local port number (only avaiable for server)\n"
		"  -m, --htmlstatus           enable HTML status file, set file name (only avaiable for server)\n"
		"  -h, -?, --help             this help text\n"
		"Example: " + std::string ( argv[0] ) + " -l -inifile myinifile.ini\n";
}

bool GetFlagArgument ( int, char **argv, int &i,
                       std::string strShortOpt, std::string strLongOpt )
{
	if ( ( !strShortOpt.compare ( argv[i] ) ) || ( !strLongOpt.compare ( argv[i] ) ) )
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool GetStringArgument ( int argc, char **argv, int &i,
                         std::string strShortOpt, std::string strLongOpt,
                         std::string & strArg )
{
	if ( ( !strShortOpt.compare ( argv[i] ) ) || ( !strLongOpt.compare ( argv[i] ) ) )
	{
		if ( ++i >= argc )
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a string argument" << endl;
			exit ( 1 );
		}

		strArg = argv[i];

		return true;
	}
	else
	{
		return false;
	}
}

bool GetNumericArgument ( int argc, char **argv, int &i,
                          std::string strShortOpt, std::string strLongOpt,
                          double rRangeStart, double rRangeStop,
                          double & rValue)
{
	if ( ( !strShortOpt.compare ( argv[i] ) ) || ( !strLongOpt.compare ( argv[i] ) ) )
	{
		if ( ++i >= argc )
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a numeric argument between "
				<< rRangeStart << " and " << rRangeStop << endl;
			exit ( 1 );
		}

		char *p;
		rValue = strtod ( argv[i], &p );
		if ( *p || rValue < rRangeStart || rValue > rRangeStop )
		{
			cerr << argv[0] << ": ";
			cerr << "'" << strLongOpt << "' needs a numeric argument between "
				<< rRangeStart << " and " << rRangeStop << endl;
			exit ( 1 );
		}

		return true;
	}
	else
	{
		return false;
	}
}


/******************************************************************************\
* Window Message System                                                        *
\******************************************************************************/
void PostWinMessage ( const _MESSAGE_IDENT MessID, const int iMessageParam,
                      const int iChanNum )
{
    // first check if application is initialized
    if ( pApp != NULL )
    {
        CLlconEvent* LlconEv =
            new CLlconEvent ( MessID, iMessageParam, iChanNum );

        // Qt will delete the event object when done
        QCoreApplication::postEvent ( pMainWindow, LlconEv );
    }
}
