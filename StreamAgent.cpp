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

#include "StreamAgent.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QClipboard>
#include <QApplication>
#include <QMimeData>
#include <QSettings>
#include <Stream/Exceptions.h>
#include <QDir>
 
StreamAgent::StreamAgent(QObject *parent)
    : QObject(parent)
{
	d_outs.append( Slot() );
}

StreamAgent::~StreamAgent()
{
}

void StreamAgent::open( QString name )
{
	// close();
	d_outs.clear();
	d_outs.append( Slot() );

	QSettings set;
	QDir dir( set.value( "OutDir", QDir::currentPath() ).toString() );
	QFile* f = new QFile( dir.absoluteFilePath( name + ".dsdx" ) );
	if( !f->open( QIODevice::WriteOnly | QIODevice::Unbuffered ) )
	{
		delete f;
		onError( "StreamAgent::open: Cannot open file for writing" );
		return;
	}
	onStatus( QString( "Created stream %1" ).arg( f->fileName() ) );
	d_outs.back().d_out.setDevice( f, true );
}

void StreamAgent::close()
{
	if( d_outs.size() > 1 )
		onError( "StreamAgent::close: endEmbed missing from level " + QString::number( d_outs.size() ) );
	d_outs.clear();
	d_outs.append( Slot() );
	onStatus( "Closing stream" );
}

void StreamAgent::writeCell( const QByteArray& name, const Stream::DataCell& value )
{
	try
	{
		onTrace( QString( "WriteCell '%1' (%2) = %3" ).
			arg( QString::fromLatin1( name ) ).
			arg( QString::fromLatin1(Stream::DataCell::typePrettyName[value.getType()]) ).
			arg( value.toPrettyString() ) );
		if( name.isEmpty() )
		{
			d_outs.back().d_out.writeSlot( value );
		}else
		{
			d_outs.back().d_out.writeSlot( value, name.data(), true );
		}
	}catch( Stream::StreamException& e )
	{
		onError( QString( "StreamAgent::writeCell %1 %2" ).arg( e.getCode() ).arg( QString( e.getMsg() ) ) );
	}catch( std::exception& e )
	{
		onError( "StreamAgent::writeCell " + QString( e.what() ) );
	}catch( ... )
	{
		onError( "StreamAgent::writeCell: unknown exception" );
	}
}

void StreamAgent::loadImg( QString filePath, bool deleteAfterwards, QByteArray name )
{
	QImage img;
	onTrace( "LoadImg " + filePath );
	if( !img.load( filePath ) )
	{
		img.load( ":/DoorScopeEtl/img_placeholder.png" );
		writeCell( name, Stream::DataCell().setImage( img ) );
		onError( "StreamAgent::loadImg: cannot load image file" );
		return;
	}
	writeCell( name, Stream::DataCell().setImage( img ) );
	if( deleteAfterwards )
		QFile::remove( filePath );
}

void StreamAgent::readImg( QString filePath, int w, int h, QByteArray name )
{
	QImage img;
	onTrace( "LoadImg " + filePath );
	if( !img.load( filePath ) )
	{
		img.load( ":/DoorScopeEtl/img_placeholder.png" );
		writeCell( name, Stream::DataCell().setImage( img ) );
		onError( "StreamAgent::readImg: cannot load image file" );
		return;
	}
	if( w > 0 && h > 0 )
		img = img.scaled( QSize( w, h ), Qt::KeepAspectRatio, Qt::SmoothTransformation );
	writeCell( name, Stream::DataCell().setImage( img ) );
}

void StreamAgent::writeString( QString value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setString( value ) );
}

void StreamAgent::writeHtml( QString value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setHtml( value ) );
}

void StreamAgent::pasteString( QByteArray name )
{
	writeCell( name, Stream::DataCell().setString( QApplication::clipboard()->text() ) );
}

void StreamAgent::writeReal( double value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setDouble( value ) );
}

void StreamAgent::writeInt( int value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setInt32( value ) );
}

void StreamAgent::writeDate( QDateTime value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setDateTime( value ) );
}

void StreamAgent::writeChar( char value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setUInt8( value ) );
}

void StreamAgent::writeBool( bool value, QByteArray name )
{
	writeCell( name, Stream::DataCell().setBool( value ) );
}

void StreamAgent::startFrame( QByteArray name )
{
	try
	{
		onTrace( "StartFrame " + name );
		if( name.isNull() )
		{
			d_outs.back().d_out.startFrame();
			return;
		}else
		{
			const QByteArray n = name;
			d_outs.back().d_out.startFrame( n.data() );
		}
	}catch( Stream::StreamException& e )
	{
		onError( QString( "StreamAgent::startFrame %1 %2" ).arg( e.getCode() ).arg( QString( e.getMsg() ) ) );
	}catch( std::exception& e )
	{
		onError( "StreamAgent::startFrame " + QString( e.what() ) );
	}catch( ... )
	{
		onError( "StreamAgent::startFrame: unknown exception" );
	}
}

void StreamAgent::endFrame()
{
	try
	{
		onTrace( "EndFrame" );
		d_outs.back().d_out.endFrame();
	}catch( Stream::StreamException& e )
	{
		onError( QString( "StreamAgent::endFrame %1 %2" ).arg( e.getCode() ).arg( QString( e.getMsg() ) ) );
	}catch( std::exception& e )
	{
		onError( "StreamAgent::endFrame " + QString( e.what() ) );
	}catch( ... )
	{
		onError( "StreamAgent::endFrame: unknown exception" );
	}
}

void StreamAgent::startEmbed()
{
	try
	{
		d_outs.append( Slot() );
		onTrace( "StartEmbed" );
	}catch( std::exception& e )
	{
		onError( "StreamAgent::startEmbed " + QString( e.what() ) );
	}catch( ... )
	{
		onError( "StreamAgent::startEmbed: unknown exception" );
	}
}

void StreamAgent::endEmbed( QByteArray name )
{
	try
	{
		if( d_outs.size() <= 1 )
		{
			onError( "StreamAgent::endEmbed: startEmbed missmatch" );
			return;
		}
		onTrace( "EndEmbed " + name );
		const QByteArray bml = d_outs.back().d_out.getStream();
		d_outs.pop_back();
		writeCell( name, Stream::DataCell().setBml( bml ) );
	}catch( std::exception& e )
	{
		onError( "StreamAgent::endEmbed " + QString( e.what() ) );
	}catch( ... )
	{
		onError(  "StreamAgent::endEmbed: unknown exception" );
	}
}
