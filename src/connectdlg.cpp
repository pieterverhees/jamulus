/******************************************************************************\
 * Copyright (c) 2004-2013
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

#include "connectdlg.h"


/* Implementation *************************************************************/
CConnectDlg::CConnectDlg ( const bool bNewShowCompleteRegList,
                           QWidget* parent,
                           Qt::WindowFlags f )
    : QDialog ( parent, f ),
      strCentralServerAddress  ( "" ),
      strSelectedAddress       ( "" ),
      strSelectedServerName    ( "" ),
      bShowCompleteRegList     ( bNewShowCompleteRegList ),
      bServerListReceived      ( false ),
      bServerListItemWasChosen ( false )
{
    setupUi ( this );


    // Add help text to controls -----------------------------------------------
    // server list
    lvwServers->setWhatsThis ( tr ( "<b>Server List:</b> The server list shows "
        "a list of available servers which are registered at the central "
        "server. Select a server from the list and press the connect button to "
        "connect to this server. Alternatively, double click a server from "
        "the list to connect to it.<br>"
        "Note that it may take some time to retrieve the server list from the "
        "central server. If no valid central server address is specified in "
        "the settings, no server list will be available." ) );

    lvwServers->setAccessibleName ( tr ( "Server list view" ) );

    // server address
    QString strServAddrH = tr ( "<b>Server Address:</b> The IP address or URL "
        "of the server running the " ) + APP_NAME + tr ( " server software "
        "must be set here. An optional port number can be added after the IP "
        "address or URL using a comma as a separator, e.g, <i>" ) +
        DEFAULT_SERVER_ADDRESS + ":" +
        QString().setNum ( LLCON_DEFAULT_PORT_NUMBER ) + tr ( "</i>. A list of "
        "the most recent used server IP addresses or URLs is available for "
        "selection." );

    lblServerAddr->setWhatsThis ( strServAddrH );
    cbxServerAddr->setWhatsThis ( strServAddrH );

    cbxServerAddr->setAccessibleName        ( tr ( "Server address edit box" ) );
    cbxServerAddr->setAccessibleDescription ( tr ( "Holds the current server "
        "IP address or URL. It also stores old URLs in the combo box list." ) );


    // init server address combo box (max MAX_NUM_SERVER_ADDR_ITEMS entries)
    cbxServerAddr->setMaxCount     ( MAX_NUM_SERVER_ADDR_ITEMS );
    cbxServerAddr->setInsertPolicy ( QComboBox::NoInsert );

    // set up list view for connected clients (note that the last column size
    // must not be specified since this column takes all the remaining space)
    lvwServers->setColumnWidth ( 0, 165 );
    lvwServers->setColumnWidth ( 1, 65 );
    lvwServers->setColumnWidth ( 2, 60 );
    lvwServers->clear();

    // add invisible column which is used for sorting the list
    lvwServers->setColumnCount ( 5 );
    lvwServers->hideColumn ( 4 );

    // make sure the connect button has the focus
    butConnect->setFocus();


    // Connections -------------------------------------------------------------
    // list view
    QObject::connect ( lvwServers,
        SIGNAL ( itemSelectionChanged() ),
        this, SLOT ( OnServerListItemSelectionChanged() ) );

    QObject::connect ( lvwServers,
        SIGNAL ( itemDoubleClicked ( QTreeWidgetItem*, int ) ),
        this, SLOT ( OnServerListItemDoubleClicked ( QTreeWidgetItem*, int ) ) );

    QObject::connect ( lvwServers, // to get default return key behaviour working
        SIGNAL ( activated ( QModelIndex ) ),
        this, SLOT ( OnConnectClicked() ) );

    // combo boxes
    QObject::connect ( cbxServerAddr, SIGNAL ( editTextChanged ( const QString& ) ),
        this, SLOT ( OnServerAddrEditTextChanged ( const QString& ) ) );

    // buttons
    QObject::connect ( butCancel, SIGNAL ( clicked() ),
        this, SLOT ( close() ) );

    QObject::connect ( butConnect, SIGNAL ( clicked() ),
        this, SLOT ( OnConnectClicked() ) );

    // timers
    QObject::connect ( &TimerPing, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerPing() ) );

    QObject::connect ( &TimerReRequestServList, SIGNAL ( timeout() ),
        this, SLOT ( OnTimerReRequestServList() ) );
}

void CConnectDlg::Init ( const QString           strNewCentralServerAddr, 
                         const CVector<QString>& vstrIPAddresses )
{
    // take central server address string
    strCentralServerAddress = strNewCentralServerAddr;

    // load stored IP addresses in combo box
    cbxServerAddr->clear();

    for ( int iLEIdx = 0; iLEIdx < MAX_NUM_SERVER_ADDR_ITEMS; iLEIdx++ )
    {
        if ( !vstrIPAddresses[iLEIdx].isEmpty() )
        {
            cbxServerAddr->addItem ( vstrIPAddresses[iLEIdx] );
        }
    }
}

void CConnectDlg::showEvent ( QShowEvent* )
{
    // reset flags (on opening the connect dialg, we always want to request a
    // new updated server list per definition)
    bServerListReceived      = false;
    bServerListItemWasChosen = false;

    // clear current address and name
    strSelectedAddress    = "";
    strSelectedServerName = "";

    // clear server list view
    lvwServers->clear();

    // get the IP address of the central server (using the ParseNetworAddress
    // function) when the connect dialog is opened, this seems to be the correct
    // time to do it
    if ( NetworkUtil().ParseNetworkAddress ( strCentralServerAddress,
                                             CentralServerAddress ) )
    {
        // send the request for the server list
        emit ReqServerListQuery ( CentralServerAddress );

        // start timer, if this message did not get any respond to retransmit
        // the server list request message
        TimerReRequestServList.start ( SERV_LIST_REQ_UPDATE_TIME_MS );
    }
}

void CConnectDlg::hideEvent ( QHideEvent* )
{
    // if window is closed, stop timers
    TimerPing.stop();
    TimerReRequestServList.stop();
}

void CConnectDlg::OnTimerReRequestServList()
{
    // if the server list is not yet received, retransmit the request for the
    // server list
    if ( !bServerListReceived )
    {
        emit ReqServerListQuery ( CentralServerAddress );
    }
}

void CConnectDlg::SetServerList ( const CHostAddress&         InetAddr,
                                  const CVector<CServerInfo>& vecServerInfo )
{
    // set flag and disable timer for resend server list request
    bServerListReceived = true;
    TimerReRequestServList.stop();

    // first clear list
    lvwServers->clear();

    // add list item for each server in the server list
    const int iServerInfoLen = vecServerInfo.Size();

    for ( int iIdx = 0; iIdx < iServerInfoLen; iIdx++ )
    {
        // get the host address, note that for the very first entry which is
        // the central server, we have to use the receive host address
        // instead
        CHostAddress CurHostAddress;
        if ( iIdx > 0 )
        {
            CurHostAddress = vecServerInfo[iIdx].HostAddr;
        }
        else
        {
            // substitude the receive host address for central server
            CurHostAddress = InetAddr;
        }

        // create new list view item
        QTreeWidgetItem* pNewListViewItem = new QTreeWidgetItem ( lvwServers );

        // make the entry invisible (will be set to visible on successful ping
        // result) if the complete list of registered servers shall not be shown
        if ( !bShowCompleteRegList )
        {
            pNewListViewItem->setHidden ( true );
        }

        // server name (if empty, show host address instead)
        if ( !vecServerInfo[iIdx].strName.isEmpty() )
        {
            pNewListViewItem->setText ( 0, vecServerInfo[iIdx].strName );
        }
        else
        {
            // IP address and port (use IP number without last byte)
            // Definition: If the port number is the default port number, we do
            // not show it.
            if ( vecServerInfo[iIdx].HostAddr.iPort == LLCON_DEFAULT_PORT_NUMBER )
            {
                // only show IP number, no port number
                pNewListViewItem->setText ( 0, CurHostAddress.
                    toString ( CHostAddress::SM_IP_NO_LAST_BYTE ) );
            }
            else
            {
                // show IP number and port
                pNewListViewItem->setText ( 0, CurHostAddress.
                    toString ( CHostAddress::SM_IP_NO_LAST_BYTE_PORT ) );
            }
        }

        // the ping time shall be shown in bold font
        QFont CurPingTimeFont = pNewListViewItem->font( 3 );
        CurPingTimeFont.setBold ( true );
        pNewListViewItem->setFont ( 1, CurPingTimeFont );

        // server location (city and country)
        QString strLocation = vecServerInfo[iIdx].strCity;
        if ( ( !strLocation.isEmpty() ) &&
             ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry ) )
        {
            strLocation += ", ";
        }
        if ( vecServerInfo[iIdx].eCountry != QLocale::AnyCountry )
        {
            strLocation +=
                QLocale::countryToString ( vecServerInfo[iIdx].eCountry );
        }

// for debugging, plot address infos in connect dialog
// Do not enable this for official versions!
#if 0
strLocation += ", " + vecServerInfo[iIdx].HostAddr.InetAddr.toString() +
    ":" + QString().setNum ( vecServerInfo[iIdx].HostAddr.iPort );
#endif

        pNewListViewItem->setText ( 3, strLocation );

        // init the minimum ping time with a large number (note that this number
        // must fit in an integer type)
        pNewListViewItem->setText ( 4, "99999999" );

        // store host address
        pNewListViewItem->setData ( 0, Qt::UserRole,
            CurHostAddress.toString() );
    }

    // immediately issue the ping measurements and start the ping timer since
    // the server list is filled now
    OnTimerPing();
    TimerPing.start ( PING_UPDATE_TIME_SERVER_LIST_MS );
}

void CConnectDlg::OnServerListItemSelectionChanged()
{
    // get current selected item (we are only interested in the first selcted
    // item)
    QList<QTreeWidgetItem*> CurSelListItemList = lvwServers->selectedItems();

    // if an item is clicked/selected, copy the server name to the combo box
    if ( CurSelListItemList.count() > 0 )
    {
        // make sure no signals are send when we change the text
        cbxServerAddr->blockSignals ( true );
        {
            cbxServerAddr->setEditText ( CurSelListItemList[0]->text ( 0 ) );
        }
        cbxServerAddr->blockSignals ( false );
    }
}

void CConnectDlg::OnServerListItemDoubleClicked ( QTreeWidgetItem* Item,
                                                  int )
{
    // if a server list item was double clicked, it is the same as if the
    // connect button was clicked
    if ( Item != 0 )
    {
        OnConnectClicked();
    }
}

void CConnectDlg::OnServerAddrEditTextChanged ( const QString& )
{
    // in the server address combo box, a text was changed, remove selection
    // in the server list (if any)
    lvwServers->clearSelection();
}

void CConnectDlg::OnConnectClicked()
{
    // get the IP address to be used according to the following definitions:
    // - if the list has focus and a line is selected, use this line
    // - if the list has no focus, use the current combo box text
    QList<QTreeWidgetItem*> CurSelListItemList = lvwServers->selectedItems();

    if ( CurSelListItemList.count() > 0 )
    {
        // get host address from selected list view item as a string
        strSelectedAddress =
            CurSelListItemList[0]->data ( 0, Qt::UserRole ).toString();

        // store selected server name
        strSelectedServerName = CurSelListItemList[0]->text ( 0 );

        // set flag that a server list item was chosen to connect
        bServerListItemWasChosen = true;
    }
    else
    {
        strSelectedAddress = cbxServerAddr->currentText();
    }

    // tell the parent window that the connection shall be initiated
    done ( QDialog::Accepted );
}

void CConnectDlg::OnTimerPing()
{
    // send ping messages to the servers in the list
    const int iServerListLen = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        CHostAddress CurServerAddress;

        // try to parse host address string which is stored as user data
        // in the server list item GUI control element
        if ( NetworkUtil().ParseNetworkAddress (
                lvwServers->topLevelItem ( iIdx )->
                data ( 0, Qt::UserRole ).toString(),
                CurServerAddress ) )
        {
            // if address is valid, send ping
            emit CreateCLServerListPingMes ( CurServerAddress );
        }
    }
}

void CConnectDlg::SetPingTimeAndNumClientsResult ( CHostAddress& InetAddr,
                                                   const int     iPingTime,
                                                   const int     iPingTimeLEDColor,
                                                   const int     iNumClients )
{
    // apply the received ping time to the correct server list entry
    const int iServerListLen = lvwServers->topLevelItemCount();

    for ( int iIdx = 0; iIdx < iServerListLen; iIdx++ )
    {
        // compare the received address with the user data string of the
        // host address by a string compare
        if ( !lvwServers->topLevelItem ( iIdx )->
                data ( 0, Qt::UserRole ).toString().
                compare ( InetAddr.toString() ) )
        {
            // update the color of the ping time font
            switch ( iPingTimeLEDColor )
            {
            case MUL_COL_LED_GREEN:
                lvwServers->
                    topLevelItem ( iIdx )->setTextColor ( 1, Qt::darkGreen );
                break;

            case MUL_COL_LED_YELLOW:
                lvwServers->
                    topLevelItem ( iIdx )->setTextColor ( 1, Qt::darkYellow );
                break;

            case MUL_COL_LED_RED:
                lvwServers->
                    topLevelItem ( iIdx )->setTextColor ( 1, Qt::red );
                break;
            }

            // update ping text, take special care if ping time exceeds a
            // certain value
            if ( iPingTime > 500 )
            {
                lvwServers->topLevelItem ( iIdx )->setText ( 1, ">500 ms" );
            }
            else
            {
                lvwServers->topLevelItem ( iIdx )->
                    setText ( 1, QString().setNum ( iPingTime ) + " ms" );
            }

            // update number of clients text
            lvwServers->topLevelItem ( iIdx )->
                setText ( 2, QString().setNum ( iNumClients ) );

            // a ping time was received, set item to visible (note that we have
            // to check if the item is hidden, otherwise we get a lot of CPU
            // usage by calling "setHidden(false)" even if the item was already
            // visible)
            if ( lvwServers->topLevelItem ( iIdx )->isHidden() )
            {
                lvwServers->topLevelItem ( iIdx )->setHidden ( false );
            }

            // update minimum ping time column (invisible, used for sorting) if
            // the new value is smaller than the old value
            if ( lvwServers->topLevelItem ( iIdx )->text ( 4 ).toInt() > iPingTime )
            {
                // we pad to a total of 8 characters with zeros to make sure the
                // sorting is done correctly
                lvwServers->topLevelItem ( iIdx )->
                    setText ( 4, QString ( "%1" ).arg (
                    iPingTime, 8, 10, QLatin1Char ( '0' ) ) );

                // Update the sorting (lowest number on top).
                // Note that the sorting must be the last action for the current
                // item since the topLevelItem ( iIdx ) is then no longer valid.
                lvwServers->sortByColumn ( 4, Qt::AscendingOrder );
            }
        }
    }
}
