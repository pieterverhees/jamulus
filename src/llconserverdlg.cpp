/******************************************************************************\
 * Copyright (c) 2004-2006
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

#include "llconserverdlg.h"


/* Implementation *************************************************************/
CLlconServerDlg::CLlconServerDlg ( CServer* pNServP, QWidget* parent )
    : pServer ( pNServP ), QDialog ( parent ),
    BitmCubeGreen ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL ),
    BitmCubeRed ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL ),
    BitmCubeYellow ( LED_WIDTH_HEIGHT_SIZE_PIXEL, LED_WIDTH_HEIGHT_SIZE_PIXEL )
{
    /* set text for version and application name */
    TextLabelNameVersion->setText ( QString ( APP_NAME ) +
        tr ( " server " ) + QString ( VERSION ) );

    /* Create bitmaps */
    BitmCubeGreen.fill ( QColor ( 0, 255, 0 ) );
    BitmCubeRed.fill ( QColor ( 255, 0, 0 ) );
    BitmCubeYellow.fill ( QColor ( 255, 255, 0 ) );

    // set up list view for connected clients
    ListViewClients->setColumnWidth ( 0, 170 );

// TODO QT4

//    ListViewClients->setColumnAlignment ( 1, Qt::AlignLeft );
    ListViewClients->setColumnWidth ( 1, 150 );
//    ListViewClients->setColumnAlignment ( 2, Qt::AlignCenter );
//    ListViewClients->setColumnAlignment ( 3, Qt::AlignCenter );
//    ListViewClients->setColumnAlignment ( 4, Qt::AlignRight );
//    ListViewClients->setColumnAlignment ( 5, Qt::AlignRight );
//    ListViewClients->setColumnAlignment ( 6, Qt::AlignRight );
    ListViewClients->clear();

    /* insert items in reverse order because in Windows all of them are
       always visible -> put first item on the top */
    vecpListViewItems.Init(MAX_NUM_CHANNELS);
    for ( int i = MAX_NUM_CHANNELS - 1; i >= 0; i-- )
    {
        vecpListViewItems[i] = new CServerListViewItem ( ListViewClients );
#ifndef _WIN32
        vecpListViewItems[i]->setVisible ( false );
#endif
    }

    // Init timing jitter text label
    TextLabelResponseTime->setText ( "" );


    /* Main menu bar -------------------------------------------------------- */

// TODO QT4

    pMenu = new QMenuBar ( this );
//    pMenu->insertItem ( tr ( "&?" ), new CLlconHelpMenu ( this ) );
    pMenu->addMenu ( new CLlconHelpMenu ( this ) );
    //pMenu->setSeparator ( QMenuBar::InWindowsStyle );

    // Now tell the layout about the menu
    layout()->setMenuBar ( pMenu );


    /* connections ---------------------------------------------------------- */
    // timers
    QObject::connect ( &Timer, SIGNAL ( timeout() ), this, SLOT ( OnTimer() ) );


    /* timers --------------------------------------------------------------- */
    // start timer for GUI controls
    Timer.start ( GUI_CONTRL_UPDATE_TIME );
}

void CLlconServerDlg::OnTimer()
{
    CVector<CHostAddress>   vecHostAddresses;
    CVector<std::string>    vecsName;
    CVector<int>            veciJitBufSize;
    CVector<int>            veciNetwOutBlSiFact;
    CVector<int>            veciNetwInBlSiFact;
    double                  dCurTiStdDev;

    ListViewMutex.lock();

    pServer->GetConCliParam ( vecHostAddresses, vecsName, veciJitBufSize,
        veciNetwOutBlSiFact, veciNetwInBlSiFact );

    /* fill list with connected clients */
    for ( int i = 0; i < MAX_NUM_CHANNELS; i++ )
    {
        if ( !( vecHostAddresses[i].InetAddr == QHostAddress ( (quint32) 0 ) ) )
        {
            // IP, port number
            vecpListViewItems[i]->setText ( 0, QString().sprintf ( "%s : %d",
                vecHostAddresses[i].InetAddr.toString().toLatin1(),
                vecHostAddresses[i].iPort ) /* IP, port */);

            // name
            vecpListViewItems[i]->setText ( 1, vecsName[i].c_str() );

            // jitter buffer size (polling for updates)
            vecpListViewItems[i]->setText ( 4,
                QString().setNum ( veciJitBufSize[i] ) );

            // in/out network block sizes
            vecpListViewItems[i]->setText ( 5,
                QString().setNum (
                double ( veciNetwInBlSiFact[i] * MIN_BLOCK_DURATION_MS), 'f', 2 ) );
            vecpListViewItems[i]->setText(6,
                QString().setNum (
                double ( veciNetwOutBlSiFact[i] * MIN_BLOCK_DURATION_MS), 'f', 2 ) );

#ifndef _WIN32
            vecpListViewItems[i]->setVisible ( true );
#endif
        }
        else
        {
#ifdef _WIN32
            // remove text for Windows version
            vecpListViewItems[i]->setText ( 0, "" );
            vecpListViewItems[i]->setText ( 1, "" );
            vecpListViewItems[i]->setText ( 4, "" );
            vecpListViewItems[i]->setText ( 5, "" );
#else
            vecpListViewItems[i]->setVisible ( false );
#endif
        }
    }

    ListViewMutex.unlock();

    // response time (if available)
    if ( pServer->GetTimingStdDev ( dCurTiStdDev ) )
    {
        TextLabelResponseTime->setText ( QString().
            setNum ( dCurTiStdDev, 'f', 2 ) + " ms" );
    }
    else
    {
        TextLabelResponseTime->setText ( "---" );
    }
}

void CLlconServerDlg::customEvent ( QEvent* Event )
{
    if ( Event->type() == QEvent::User + 11 )
    {
        ListViewMutex.lock();

        const int iMessType = ( (CLlconEvent*) Event )->iMessType;
        const int iStatus = ( (CLlconEvent*) Event )->iStatus;
        const int iChanNum = ( (CLlconEvent*) Event )->iChanNum;

        switch(iMessType)
        {
        case MS_JIT_BUF_PUT:
            vecpListViewItems[iChanNum]->SetLight ( 0, iStatus );
            break;

        case MS_JIT_BUF_GET:
            vecpListViewItems[iChanNum]->SetLight ( 1, iStatus );
            break;
        }

        ListViewMutex.unlock();
    }
}
