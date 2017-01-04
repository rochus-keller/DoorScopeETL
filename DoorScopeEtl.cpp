/*
* Copyright 2005-2017 Rochus Keller <mailto:me@rochus-keller.info>
*
* This file is part of the DoorScopeEtl application
* see <http://doorscope.ch/>).
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

#include "DoorScopeEtl.h"
#include <QTextEdit>
#include <QTcpServer>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QApplication>
#include <QInputDialog>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include "IpcProtocol.h"
#include "HtmlImporter.h"

static const int s_doorsDefaultPort = 5093;
static const char* s_version = "0.6.2";
static const char* s_date = "2017-01-03";

DoorScopeEtl::DoorScopeEtl(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	QSettings set;

	setWindowTitle( tr( "DoorScope Extract-Transform-Load (ETL)" ) );

	QMenu* file = menuBar()->addMenu ( tr( "&File" ) );
#ifdef _DEBUG
	file->addAction( tr( "&Test" ), this, SLOT( onTest() ) );
#endif
	file->addAction( tr( "&Open Html..." ), this, SLOT( onParseHtml() ) );
	file->addSeparator();
	file->addAction( tr( "&Quit" ), qApp, SLOT( quit() ), tr("CTRL+Q") );

	QMenu* settings = menuBar()->addMenu ( tr( "&Settings" ) );
	settings->addAction( tr( "Set &Port..." ), this, SLOT( onSetPort() ) );
	settings->addAction( tr( "Set &Output Directory..." ), this, SLOT( onSetOutDir() ) );

	QMenu* log = menuBar()->addMenu( tr( "&Log" ) );
	log->addAction( tr( "&Clear Log" ), this, SLOT( onClearLog() ), tr("CTRL+DEL") );
	d_logTrace = new QAction( tr( "Trace on/off" ), this );
	d_logTrace->setCheckable( true );
	d_logTrace->setChecked( set.value( "LogTrace", false ).toBool() );
	connect( d_logTrace, SIGNAL( triggered() ), this, SLOT( onLogTrace() ) );
	log->addAction( d_logTrace );
	d_logProto = new QAction( tr( "Log Protocol on/off" ), this );
	d_logProto->setCheckable( true );
	connect( d_logProto, SIGNAL( triggered() ), this, SLOT( onLogProto() ) );
	log->addAction( d_logProto );

	QMenu* info = menuBar()->addMenu( tr( "&?" ) );
	info->addAction( tr( "&About DoorScope ETL..." ), this, SLOT( onAbout() ) );

	d_log = new QTextEdit( this );
	d_log->setReadOnly( true );
	// d_log->setWordWrapMode( QTextOption::NoWrap );
	setCentralWidget( d_log );

	d_server = new QTcpServer( this );
	connect( d_server, SIGNAL( newConnection ()), this, SLOT( onNewConnection() ) );

	updatePort();

	onLog( "Output directory: " + set.value( "OutDir", QDir::currentPath() ).toString(), LogStatus );
	if( set.contains( "WindowSize" ) )
		resize( set.value( "WindowSize" ).toSize() );

	d_html = new HtmlImporter( this );
}

DoorScopeEtl::~DoorScopeEtl()
{

}

void DoorScopeEtl::resizeEvent( QResizeEvent * event )
{
	QSettings set;
	set.setValue( "WindowSize", size() );
}

void DoorScopeEtl::onSetOutDir()
{
	QSettings set;
	QString path = QFileDialog::getExistingDirectory( this, 
		tr("Select Output Directory - DoorScope ETL"), set.value( "OutDir" ).toString() );
	if( path.isEmpty() )
		return;
	set.setValue( "OutDir", path );
	onLog( "Output directory: " + path, LogStatus );
}

void DoorScopeEtl::onSetPort()
{
	bool ok;
	int port = QInputDialog::getInteger( this, tr( "Set Port - DoorScope ETL" ), tr( "Enter a new IPC port:" ),
		d_server->serverPort(), 1, 0xffff, 1, &ok );
	if( !ok )
		return;
	QSettings set;
	set.setValue( "IpcPort", port );
	updatePort();
}

void DoorScopeEtl::updatePort()
{
	QSettings set;

	if( d_server->isListening() )
		d_server->close();

	if( !d_server->listen( QHostAddress::Any, set.value( "IpcPort", s_doorsDefaultPort ).toInt() ) )
		onLog( d_server->errorString(), LogError );
	else
		onLog( "Listening on port " + QString::number( d_server->serverPort() ), LogStatus );
}

void DoorScopeEtl::onNewConnection()
{
	QTcpSocket* sock = d_server->nextPendingConnection(); 
	if( sock == 0 )
		return;
	IpcProtocol* p = new IpcProtocol( sock );
	connect( sock, SIGNAL( disconnected() ), sock, SLOT( deleteLater() ) );
	if( d_logProto->isChecked() )
		connect( sock, SIGNAL(readyRead()), this, SLOT(onData()) ); 
	else
		connect( sock, SIGNAL(readyRead()), p, SLOT(onData()) ); 
	connect( sock, SIGNAL(error(QAbstractSocket::SocketError)), p, SLOT(onError(QAbstractSocket::SocketError)));
	connect( &p->d_agent, SIGNAL( log( QString, int ) ), this, SLOT( onLog( QString, int ) ) );
}

void DoorScopeEtl::onTest()
{
	QString path = QFileDialog::getOpenFileName( this, tr("Parse Protocol Log File"), QString(), "*.log" ); 
	if( path.isNull() )
		return;
	QFile in( path );
	if( !in.open( QIODevice::ReadOnly ) )
		return;
	IpcProtocol p( 0 );
	connect( &p.d_agent, SIGNAL( log( QString, int ) ), this, SLOT( onLog( QString, int ) ) );
	p.parse( &in );
}

void DoorScopeEtl::onData()
{
	QTcpSocket* sock = (QTcpSocket*) sender();
	QFile out( d_logPath );
	out.open( QIODevice::Append );
	out.write( sock->readAll() );
}

void DoorScopeEtl::onLog( QString str, int kind )
{
	switch( kind )
	{
	case LogTrace:
		if( !d_logTrace->isChecked() )
			return;
		str = ">>> " + str;
		break;
	case LogStatus:
		str = "*** " + str;
		break;
	case LogError:
		str = "### " + str;
		break;
	}
#ifdef _DEBUG
	//QByteArray tmp = str.toLatin1();
	//qDebug( tmp.data() );
#endif
	d_log->append( str );
	d_log->moveCursor( QTextCursor::End );
}

void DoorScopeEtl::onClearLog()
{
	d_log->clear();
}

void DoorScopeEtl::onLogTrace()
{
	QSettings set;
	set.setValue( "LogTrace", d_logTrace->isChecked() );
}

void DoorScopeEtl::onLogProto()
{
	if( d_logProto->isChecked() )
	{
		d_logPath = QFileDialog::getSaveFileName( this, tr("Save Protocol Log File"), 
			d_logPath, "*.log" ); 
		if( d_logPath.isNull() )
			d_logProto->setChecked( false );
	}
}

void DoorScopeEtl::onAbout()
{
	QMessageBox::about( this, tr("About DoorScope ETL"), 
		tr("Version: %1  Date: %2\n"
		"DoorScope ETL can be used to extract and transform data to DSDX.\n" 
		"For version information see properties of the executable file.\n\n"

		"Author: Rochus Keller, rochus.keller@gmail.com\n"
		"Copyright (c) 2005-2017\n\n"

		"Terms of use:\n"
		"This version of DoorScope ETL is freeware, i.e. it can be used for free by anyone. "
		"The software and documentation are provided \"as is\", without warranty of any kind, "
		"expressed or implied, including but not limited to the warranties of merchantability, "
		"fitness for a particular purpose and noninfringement. In no event shall the author or copyright holders be "
		"liable for any claim, damages or other liability, whether in an action of contract, tort or otherwise, "
		"arising from, out of or in connection with the software or the use or other dealings in the software." ).
		arg( s_version ).arg( s_date ) );
}

void DoorScopeEtl::onParseHtml()
{
	QString filter;
	QString path = QFileDialog::getOpenFileName( this, tr("Import HTML Document"), d_lastPath,
		"HTML files (*.html *.htm)", 
		&filter, QFileDialog::DontUseNativeDialog ); 
	if( path.isNull() || filter.isNull() )
		return;
	QFileInfo info( path );
	d_lastPath = info.absolutePath();
	QApplication::setOverrideCursor( Qt::WaitCursor );

	if( !d_html->parse( path ) )
	{
		QApplication::restoreOverrideCursor();
		QMessageBox::critical( this, tr("Error importing document" ), d_html->getError() );
	}else
		QApplication::restoreOverrideCursor();
}
