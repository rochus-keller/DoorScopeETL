#ifndef STREAMX_H
#define STREAMX_H

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

#include <QObject>
#include <Stream/DataWriter.h>
#include <QMap>
#include <QLinkedList>

class StreamAgent : public QObject
{
    Q_OBJECT    
public:
    StreamAgent(QObject *parent = 0);
	~StreamAgent();

	void onError( QString msg ) { log( msg, 2 ); }
	void onStatus( QString msg ) { log( msg, 1 ); }
	void onTrace( QString msg ) { log( msg, 0 ); }
	void readImg( QString filePath, int w = 0, int h = 0, QByteArray name = QByteArray() ); 
signals:
	void log( QString, int kind );
public slots:
	void open( QString name );
	void close();

	// name darf empty sein
	// Doors-Typen: string|int|char|bool|OleAutoObj, real, Date
	// DOORS data is stored in ANSI format
	void writeString( QString val, QByteArray name = QByteArray() ); 
	void writeHtml( QString val, QByteArray name = QByteArray() ); 
	void writeInt( int val, QByteArray name = QByteArray() ); 
	void writeChar( char val, QByteArray name = QByteArray() ); 
	void writeBool( bool val, QByteArray name = QByteArray() ); 
	void loadImg( QString filePath, bool deleteAfterwards = true, QByteArray name = QByteArray() ); 
	void writeReal( double val, QByteArray name = QByteArray() ); 
	void writeDate( QDateTime val, QByteArray name = QByteArray() );  

	void pasteString( QByteArray name = QByteArray() ); 

	void startFrame( QByteArray name = QByteArray() ); 
	void endFrame(); 

	void startEmbed(); 
	void endEmbed( QByteArray name = QByteArray() ); 
private:
	void writeCell( const QByteArray& name, const Stream::DataCell& value );

	struct Slot
	{
		Stream::DataWriter d_out;
		Slot():d_out(0) {}
	};
	QLinkedList<Slot> d_outs;
};

#endif // STREAMX_H
