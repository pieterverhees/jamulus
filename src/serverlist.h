/******************************************************************************\
 * Copyright (c) 2004-2013
 *
 * Author(s):
 *  Volker Fischer
 *

Currently, if you want to run a private server, you have to open the firewall of
your computer at the correct ports and introduce a port forwarding at your
router to get it work. Using a central server simplifies the process. The user
who wants to run a server just registers his server a the central server
and a mechanism implemented in the protocol opens the firewall similar to STUN.

REQUIREMENTS:

The client sets the URL of the central server and can get a list of all
currently activated and registered private servers. If the user clicks on
the server of his choice, he gets connected to this server.

The server list must be available in both cases: if the client is connected to
the central server or not.

The server list contains the name of the server, an optional topic, an optional
location, the number of connected users and a ping time which is updated as
long as the server list is visible (similar to the ping measurement in the
general settings dialog). Additional information may be also present in the list
like reliability of the server, etc.

CONNECTION PROCESS:

The private server contacts the central server and registers through some
protocol mechanism.

If a client requests the server list from the central server, the central server
sends the IP address of the client to each registered private servers so that
they can immediately send a "firewall opening" UDP packet to this IP address.
If the client now sends ping messages to each of the private servers in the
list, the firewalls and routers are prepared for receiving UDP packets from this
IP address and will tunnel it through. Note: this mechanism will not work in a
private network.

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

#if !defined ( SERVERLIST_HOIJH8OUWEF_WFEIOBU_3_43445KJIUHF1912__INCLUDED_ )
#define SERVERLIST_HOIJH8OUWEF_WFEIOBU_3_43445KJIUHF1912__INCLUDED_

#include <QObject>
#include <QLocale>
#include <QList>
#include <QTimer>
#include <QMutex>
#include "global.h"
#include "util.h"
#include "protocol.h"


/* Classes ********************************************************************/
class CServerListEntry : public CServerInfo
{
public:
    CServerListEntry() :
        CServerInfo ( CHostAddress(),
                      0,
                      "",
                      "",
                      QLocale::AnyCountry,
                      "",
                      0,
                      false ) { UpdateRegistration(); }

    CServerListEntry ( const CHostAddress&     NHAddr,
                       const quint16           NLocPort,
                       const QString&          NsName,
                       const QString&          NsTopic,
                       const QLocale::Country& NeCountry,
                       const QString&          NsCity,
                       const int               NiMaxNumClients,
                       const bool              NbPermOnline)
        : CServerInfo ( NHAddr,
                        NLocPort,
                        NsName,
                        NsTopic,
                        NeCountry,
                        NsCity,
                        NiMaxNumClients,
                        NbPermOnline ) { UpdateRegistration(); }

    CServerListEntry ( const CHostAddress&    NHAddr,
                       const CServerCoreInfo& NewCoreServerInfo )
        : CServerInfo ( NHAddr,
                        NewCoreServerInfo.iLocalPortNumber,
                        NewCoreServerInfo.strName,
                        NewCoreServerInfo.strTopic,
                        NewCoreServerInfo.eCountry,
                        NewCoreServerInfo.strCity,
                        NewCoreServerInfo.iMaxNumClients,
                        NewCoreServerInfo.bPermanentOnline )
        { UpdateRegistration(); }

    void UpdateRegistration() { RegisterTime.start(); }

public:
    // time on which the entry was registered
    QTime RegisterTime;
};

class CServerListManager : public QObject
{
    Q_OBJECT

public:
    CServerListManager ( const quint16  iNPortNum,
                         const QString& sNCentServAddr,
                         const QString& strServerInfo,
                         const int      iNumChannels,
                         const bool     bNCentServPingServerInList,
                         CProtocol*     pNConLProt );

    // the update has to be called if any change to the server list
    // properties was done
    void Update();

    void SetEnabled ( const bool bState ) { bEnabled = bState; }
    bool GetEnabled() const { return bEnabled; }

    void SetCentralServerAddress ( const QString sNCentServAddr );
    QString GetCentralServerAddress() { return strCentralServerAddress; }

    void SetUseDefaultCentralServerAddress ( const bool bNUDCSeAddr )
        { bUseDefaultCentralServerAddress = bNUDCSeAddr; }

    bool GetUseDefaultCentralServerAddress()
        { return bUseDefaultCentralServerAddress; }

    bool GetIsCentralServer() const { return bIsCentralServer; }

    void CentralServerRegisterServer ( const CHostAddress&    InetAddr,
                                       const CServerCoreInfo& ServerInfo );

    void CentralServerUnregisterServer ( const CHostAddress& InetAddr );

    void CentralServerQueryServerList ( const CHostAddress& InetAddr );

    void SlaveServerUnregister() { SlaveServerRegisterServer ( false ); }

    // set server infos -> per definition the server info of this server is
    // stored in the first entry of the list, we assume here that the first
    // entry is correctly created in the constructor of the class
    void SetServerName ( const QString& strNewName )
        { ServerList[0].strName = strNewName; }

    QString GetServerName() { return ServerList[0].strName; }

    void SetServerCity ( const QString& strNewCity )
        { ServerList[0].strCity = strNewCity; }

    QString GetServerCity() { return ServerList[0].strCity; }

    void SetServerCountry ( const QLocale::Country eNewCountry )
        { ServerList[0].eCountry = eNewCountry; }

    QLocale::Country GetServerCountry() { return ServerList[0].eCountry; }

protected:
    void SlaveServerRegisterServer ( const bool bIsRegister );

    QTimer                  TimerPollList;
    QTimer                  TimerRegistering;
    QTimer                  TimerPingServerInList;
    QTimer                  TimerPingCentralServer;

    QMutex                  Mutex;

    QList<CServerListEntry> ServerList;

    quint16                 iPortNumber;
    QString                 strCentralServerAddress;
    int                     iNumPredefinedServers;
    bool                    bEnabled;
    bool                    bIsCentralServer;
    bool                    bUseDefaultCentralServerAddress;
    bool                    bCentServPingServerInList;

    CHostAddress            SlaveCurCentServerHostAddress;

    CProtocol*              pConnLessProtocol;

public slots:
    void OnTimerPollList();
    void OnTimerPingServerInList();
    void OnTimerPingCentralServer();
    void OnTimerRegistering() { SlaveServerRegisterServer ( true ); }
};

#endif /* !defined ( SERVERLIST_HOIJH8OUWEF_WFEIOBU_3_43445KJIUHF1912__INCLUDED_ ) */
