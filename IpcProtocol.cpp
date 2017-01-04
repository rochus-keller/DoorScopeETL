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

#include "IpcProtocol.h"

enum ParamType 
{
	ParamNone,
	ParamString,	// UInt SPACE String SPACE
	ParamInt,		// Int SPACE
	ParamChar,		// Char SPACE
	ParamBool,		// Bool SPACE
	ParamReal,		// Double SPACE
	ParamDate		// UInt SPACE String SPACE (2009-02-21 oder 2009-02-21 14:23:12)
};
static bool s_hasNum[] =
{
	false,	// ParamNone
	true,	// ParamString
	false,	// ParamInt
	false,	// ParamChar
	false,	// ParamBool
	false,	// ParamReal
	true,	// ParamDate
};

struct Command
{
	const char* name;
	ParamType param[IpcProtocol::s_maxParam];
};
static const Command s_cmds[] =
{
	{ "OpenStream", ParamString, ParamNone, ParamNone },		// 0
	{ "CloseStream", ParamNone, ParamNone, ParamNone },			// 1
	{ "StringVal", ParamString, ParamNone, ParamNone },			// 2
	{ "StringValName", ParamString, ParamString, ParamNone },	// 3
	{ "IntVal", ParamInt, ParamNone, ParamNone },				// 4
	{ "IntValName", ParamInt, ParamString, ParamNone },			// 5
	{ "BoolVal", ParamBool, ParamNone, ParamNone },				// 6
	{ "BoolValName", ParamBool, ParamString, ParamNone },		// 7
	{ "CharVal", ParamChar, ParamNone, ParamNone },				// 8
	{ "CharValName", ParamChar, ParamString, ParamNone },		// 9
	{ "RealVal", ParamReal, ParamNone, ParamNone },				// 10
	{ "RealValName", ParamReal, ParamString, ParamNone },		// 11
	{ "DateVal", ParamDate, ParamNone, ParamNone },				// 12
	{ "DateValName", ParamDate, ParamString, ParamNone },		// 13
	{ "LoadImg", ParamString, ParamBool, ParamNone },			// 14, path, delete
	{ "LoadImgName", ParamString, ParamBool, ParamString },		// 15, path, delete, name
	{ "StartFrame", ParamNone, ParamNone, ParamNone },			// 16
	{ "StartFrameName", ParamString, ParamNone, ParamNone },	// 17
	{ "EndFrame", ParamNone, ParamNone, ParamNone },			// 18
	{ "StartEmbed", ParamNone, ParamNone, ParamNone },			// 19
	{ "EndEmbed", ParamNone, ParamNone, ParamNone },			// 20
	{ "EndEmbedName", ParamString, ParamNone, ParamNone },		// 21
	{ "PasteString", ParamNone, ParamNone, ParamNone },			// 22
	{ "PasteStringName", ParamString, ParamNone, ParamNone },	// 23
	{ 0, ParamNone, ParamNone, ParamNone },
};
static const int s_maxCommand = 23;

IpcProtocol::IpcProtocol(QObject *parent)
	: QObject(parent), d_state( Idle )
{

}

IpcProtocol::~IpcProtocol()
{

}

void IpcProtocol::onError(QAbstractSocket::SocketError)
{
	QTcpSocket* sock = (QTcpSocket*) sender();
	d_agent.onError( sock->errorString() );
}

void IpcProtocol::onData()
{
	QIODevice* sock = (QIODevice*) sender();
	parse( sock );
}

void IpcProtocol::parse(QIODevice* sock)
{
	char ch;
	bool ok;
	while( sock->isOpen() && sock->bytesAvailable() )
	{
		sock->getChar( &ch );
		switch( d_state )
		{
		case Idle:
			if( ch != ' ' )
			{
				d_state = ReadCode;
				d_buf.clear();
				d_buf.append( ch );
			}
			break;
		case ReadCode:
			if( ch != '|' )
				d_buf.append( ch );
			else
			{
				d_command = d_buf.toUInt( &ok );
				if( !ok || d_command > s_maxCommand )
				{
					errorClose( sock, "Invalid command " + d_buf );
				}
				// d_agent.onTrace( s_cmds[ d_command ].name ); // TEST

				if( s_cmds[ d_command ].param[0] == ParamNone )
				{
					execute(sock);
				}else
				{
					d_pn = 0;
					d_buf.clear();
					if( s_hasNum[s_cmds[ d_command ].param[d_pn]] )
						d_state = ReadNum;
					else
						d_state = ReadVal;
				}
			}
			break;
		case ReadNum:
			if( ch != '|' )
				d_buf.append( ch );
			else
			{
				d_num = d_buf.toUInt( &ok );
				if( !ok )
				{
					errorClose( sock, "Invalid string count " + d_buf );
				}else
				{
					d_state = ReadVal;
					d_buf.clear();
				}
			}
			break;
		case ReadVal:
			{
				bool doit = false;
				if( s_hasNum[ s_cmds[d_command].param[d_pn] ] )
				{
					//if( d_num == 158 )
					//	qDebug( "hit" ); // TEST
					if( d_num > 0 )
					{
						// Lese weiteres Zeichen des Strings
						d_buf.append( ch );
						// DOORS sendet anscheinend die Strings als UTF-8 über das Netz (nirgends dokumentiert!)
						// Die Anzahl bezieht sich aber auf den Originalstring, d.h. es kommen mehr Bytes als gezählt!
						if( false ) // quint8(ch) & 0x80 )
						{
							// NOTE: hat nicht recht funktioniert. 
							// Ermittle daher im DOORS-Script neu die Anzahl Bytes vom UTF-String.

							// Erstes Byte gesetzt, also kodiert: 1xxxxxxx
							if( quint8(ch) & 0xC0 == 0xC0 ) // Erste beide Bytes gesetzt, also ein Start-Byte
								d_num--;
							// else
							//	Ein Folgebyte; wurde bereits mit dem Startbyte gezählt
							// 110xxxxx 10xxxxxx
							// 1110xxxx 10xxxxxx 10xxxxxx
							// 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
						}else
							d_num--; // ASCII-Zeichen
					}else if( d_num == 0 ) 
					{
						// Lese auf den String folgendes Trennzeichen
						if( ch != '|' )
						{
							errorClose( sock, "Expecting BAR after string: " + d_buf );
						}
						d_num--;
						doit = true;
					}else
						Q_ASSERT( d_num < 0 );
				}else
				{
					if( ch != '|' )
					{
						// Lese weitere Zeichen des Werts
						d_buf.append( ch );
					}else
					{
						doit = true;
					}
				}
				if( doit )
				{
					evaluate( sock );
				}
			}
			break;
		}
	}
}
void IpcProtocol::evaluate(QIODevice* sock)
{
	// Speichere den Wert als Param
	consume( sock );
	d_pn++;
	if( d_pn >= s_maxParam || s_cmds[ d_command ].param[d_pn] == ParamNone )
	{
		// Es kommen keine weiteren Params, führe aus
		execute(sock);
	}else
	{
		// Es kommen noch weitere Params
		d_buf.clear();
		if( s_hasNum[s_cmds[ d_command ].param[d_pn]] )
			d_state = ReadNum;
		else
			d_state = ReadVal;
	}
}

void IpcProtocol::errorClose( QIODevice* sock, QString msg )
{
	d_agent.onError( msg );
	sock->close();
	d_state = Idle;
}

void IpcProtocol::execute(QIODevice* sock)
{
	switch( d_command )
	{
	case 0: // OpenStream
		d_agent.open( d_param[0].toString() );
		break;
	case 1: // CloseStream
		d_agent.close();
		break;
	case 2: // StringVal
		d_agent.writeString( d_param[0].toString() );
		break;
	case 3: // StringValName
		d_agent.writeString( d_param[0].toString(), d_param[1].toByteArray() );
		break;
	case 4: // IntVal
		d_agent.writeInt( d_param[0].toInt() );
		break;
	case 5: // IntValName
		d_agent.writeInt( d_param[0].toInt(), d_param[1].toByteArray() );
		break;
	case 6: // BoolVal
		d_agent.writeBool( d_param[0].toBool() );
		break;
	case 7: // BoolValName
		d_agent.writeBool( d_param[0].toBool(), d_param[1].toByteArray() );
		break;
	case 8: // CharVal
		d_agent.writeChar( d_param[0].toChar().toLatin1() );
		break;
	case 9: // CharValName
		d_agent.writeChar( d_param[0].toChar().toLatin1(), d_param[1].toByteArray() );
		break;
	case 10: // RealVal
		d_agent.writeReal( d_param[0].toDouble() );
		break;
	case 11: // RealValName
		d_agent.writeReal( d_param[0].toDouble(), d_param[1].toByteArray() );
		break;
	case 12: // DateVal
		d_agent.writeDate( d_param[0].toDateTime() );
		break;
	case 13: // DateValName
		d_agent.writeDate( d_param[0].toDateTime(), d_param[1].toByteArray() );
		break;
	case 14: // LoadImg
		d_agent.loadImg( d_param[0].toString(), d_param[1].toBool() );
		break;
	case 15: // LoadImgName
		d_agent.loadImg( d_param[0].toString(), d_param[1].toBool(), d_param[2].toByteArray() );
		break;
	case 16: // StartFrame
		d_agent.startFrame();
		break;
	case 17: // StartFrameName
		d_agent.startFrame( d_param[0].toByteArray() );
		break;
	case 18: // EndFrame
		d_agent.endFrame();
		break;
	case 19: // StartEmbed
		d_agent.startEmbed();
		break;
	case 20: // EndEmbed
		d_agent.endEmbed();
		break;
	case 21: // EndEmbedName
		d_agent.endEmbed( d_param[0].toByteArray() );
		break;
	case 22: // PasteString
		d_agent.pasteString();
		break;
	case 23: // PasteStringName
		d_agent.pasteString( d_param[0].toByteArray() );
		break;
	}
	d_state = Idle;
}

void IpcProtocol::consume( QIODevice* sock )
{
	bool ok;
	switch( s_cmds[ d_command ].param[d_pn] )
	{
	case ParamString:
		d_param[d_pn] = QString::fromUtf8( d_buf ); 
		break;
	case ParamInt:
		d_param[d_pn] = d_buf.toInt( &ok );
		if( !ok )
		{
			errorClose( sock, "invalid integer " + d_buf );
		}
		break;
	case ParamChar:
		if( d_buf.size() == 1 )
		{
			d_param[d_pn] = QChar( d_buf[0] );
		}else
			errorClose( sock, "invalid char " + d_buf );
		break;
	case ParamBool:
		if( d_buf == "0" )
			d_param[d_pn] = false;
		else if( d_buf == "1" )
			d_param[d_pn] = true;
		else
			errorClose( sock, "invalid bool " + d_buf );
		break;
	case ParamReal:
		// Als String der form "3.141593"
		d_param[d_pn] = d_buf.toDouble( &ok );
		if( !ok )
		{
			errorClose( sock, "invalid real " + d_buf );
		}
		break;
	case ParamDate:
		{
			// Doors kann keine Dates direkt übergeben.
			// ISO-Format wird auch nicht unterstützt. stringOf kann Time nicht formatieren.
			// Daher stringOf( date, "yyyy-MM-dd" ) ergibt "2009-02-21 14:23:12" oder "2009-02-21"
			// je nachdem includesTime(date) true oder false; siehe auch dateAndTime(date)
			QDateTime dt;
			dt = QDateTime::fromString( d_buf, "yyyy-MM-dd h:m:s" );
			if( !dt.isValid() )
				dt = QDateTime::fromString( d_buf, "yyyy-MM-dd" );
			if( !dt.isValid() )
				dt = QDateTime::fromString( d_buf, "h:m:s" );
			if( !dt.isValid() )
			{
				errorClose( sock, "invalid date " + d_buf );
			}
			d_param[d_pn] = dt;
		}
		break;
	default:
		errorClose( sock, "invalid parameter type" );
		break;
	}
}
