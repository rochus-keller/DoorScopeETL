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

#include "HtmlImporter.h"
#include <QFile>
#include <QBuffer>
#include <QtDebug>
#include <private/qtexthtmlparser_p.h>
#include <bitset>
#include <QStack>
#include <QFileInfo>
#include <QDir>

HtmlImporter::HtmlImporter(QObject *parent)
	: QObject(parent)
{
	connect( &d_out, SIGNAL( log( QString, int ) ), parent, SLOT( onLog( QString, int ) ) );
}

struct Section
{
	Section():d_at(-1),d_level(0){}
	~Section()
	{
		for( int i = 0; i < d_subs.size(); i++ )
			delete d_subs[i];
	}
	QList<Section*> d_subs;
	QList<int> d_content;
	int d_at;
	int d_level;
};

struct Context
{
	Context():nextId(1){}
	QStack<int> trace; // Level
	quint32 nextId;
	StreamAgent* out;
	QDir path;
	QTextHtmlParser parser;
};

static inline void stdAtts( Context& ctx )
{
	ctx.out->writeString( "DoorScope", "Created By" );
	ctx.out->writeDate( QDateTime::currentDateTime(), "Created On" );
	ctx.out->writeString( "HTML Import", "Created Thru" );
}

static void readImg( Context& ctx, const QTextHtmlParserNode& n )
{
	// NOTE n.imageName ist noch kodiert und enthält z.B. %20
	const QUrl url = QUrl::fromEncoded( n.imageName.toUtf8() );
	QFileInfo path( url.toLocalFile() );
	if( path.isRelative() )
		path = ctx.path.absoluteFilePath( url.toLocalFile() );
	ctx.out->startFrame( "rt" );
	ctx.out->readImg( path.absoluteFilePath(), n.imageWidth, n.imageHeight, "ole" );
	/*
	if( n.imageWidth > 0.0 && n.imageHeight > 0.0 )
	{
		ctx.out->writeReal( n.imageWidth, "~width" );
		ctx.out->writeReal( n.imageHeight, "~height" );
	}
	*/
	ctx.out->endFrame();
}

static int f2i( QTextHTMLElements f )
{
	switch( f )
	{
	case Html_em:
	case Html_i:
	case Html_cite:
	case Html_var:
	case Html_dfn:
		return HtmlImporter::Italic;
	case Html_big:
	case Html_small:
	case Html_nobr: // unbekannt
		// ignorieren
		break;
	case Html_strong:
	case Html_b:
		return HtmlImporter::Bold;
	case Html_u:
		return HtmlImporter::Underline;
	case Html_s:
		return HtmlImporter::Strikeout;
	case Html_code:
	case Html_tt:
	case Html_kbd:
	case Html_samp:
		return HtmlImporter::Fixed;
	case Html_sub:
		return HtmlImporter::Sub;
	case Html_sup:
		return HtmlImporter::Super;
	}
	return -1;
}

static void writeFormat( Context& ctx, const std::bitset<8>& format )
{
	if( format.test( HtmlImporter::Italic ) )
		ctx.out->writeChar( 'i' );
	if( format.test( HtmlImporter::Bold ) )
		ctx.out->writeChar( 'b' );
	if( format.test( HtmlImporter::Underline ) )
		ctx.out->writeChar( 'u' );
	if( format.test( HtmlImporter::Strikeout ) )
		ctx.out->writeChar( 'k' );
	if( format.test( HtmlImporter::Super ) )
		ctx.out->writeChar( 'p' );
	if( format.test( HtmlImporter::Sub ) )
		ctx.out->writeChar( 's' );
	if( format.test( HtmlImporter::Fixed ) )
		ctx.out->writeChar( 'f' ); // RISK: neu gegenüber Doors
}

static QString simplify( QString str )
{
	if ( str.isEmpty() )
        return str;
    QString result;
	result.resize( str.size() );
    const QChar* from = str.data();
    const QChar* fromend = from + str.size();
    int outc = 0;
    QChar* to = result.data();

	// Ersetzte den Whitespace am Anfang durch einen Space
	while( from != fromend && from->isSpace() )
		from++;
    if( from != str.data() )
        to[ outc++ ] = QLatin1Char(' ');
    for(;from != fromend ;) 
	{
		// Ersetze den Whitespace im Rest des Strings durch einen Space inkl. dem Ende
        while( from != fromend && from->isSpace() )
            from++;
        while( from != fromend && !from->isSpace() )
            to[ outc++ ] = *from++;
        if( from != fromend )
            to[ outc++ ] = QLatin1Char(' ');
        else
            break;
    }
    result.truncate( outc );
    return result;
}

static void consumeFollowers( Context& ctx, int n, std::bitset<8>& format, int fidx = -1 )
{
	const int parent = ctx.parser.at(n).parent;
	n++;
	while( n < ctx.parser.count() && ctx.parser.at(n).id == Html_unknown )
	{
		// Setzte Format sofort zurück, sobald Scope des Formats endet
		if( fidx != -1 && parent != ctx.parser.at(n).parent )
			format.set( fidx, false );
		const QString str = simplify( ctx.parser.at(n).text );
		if( !str.isEmpty() )
		{
			ctx.out->startFrame( "rt" );
			writeFormat( ctx, format );
			ctx.out->writeString( str );
			ctx.out->endFrame();
		}
		n++;
	}
}

static void readFrag( Context& ctx, const QTextHtmlParserNode& n, std::bitset<8>& format )
{
	if( n.id == Html_img )
	{
		readImg( ctx, n );
		return;
	}else if( n.id == Html_br )
	{
		// TODO: setzt <br> das Format zurück?
		ctx.out->startFrame( "rt" );
		writeFormat( ctx, format );
		ctx.out->writeString( "\n" );
		ctx.out->endFrame();
		return;
	} // else

	const int fidx = f2i( n.id );
	if( fidx != -1 )
		format.set( fidx );

	const QString str = simplify( n.text );
	if( !str.isEmpty() )
	{
		ctx.out->startFrame( "rt" );
		writeFormat( ctx, format );
		ctx.out->writeString( str );
		ctx.out->endFrame();
	}

	for( int i = 0; i < n.children.size(); i++ )
	{
		readFrag( ctx, ctx.parser.at( n.children[i] ), format );
		consumeFollowers( ctx, n.children[i], format, fidx );
	}

	if( fidx != -1 )
		format.set( fidx, false );
}

static void readFrags( Context& ctx, const QTextHtmlParserNode& p )
{
	std::bitset<8> format;
	for( int i = 0; i < p.children.size(); i++ )
	{
		// Starte Neues frag für je imp und alle übrigen
		// img hat nie Children
		// a hat selten Children. Falls dann sup oder sub.
		// Hier sollte a einfach als Text behandelt werden, da wir sonst die Art der URL interpretieren müssen.
		readFrag( ctx, ctx.parser.at( p.children[i] ), format );
		consumeFollowers( ctx, p.children[i], format );
	}
}

static QString collectText( const QTextHtmlParser& parser, const QTextHtmlParserNode& p )
{
	// Hier verwenden wir normales QString::simplified(), da Formatierung ignoriert wird.
	QString text;
	if( p.id == Html_img )
		text += "<img>";
	text += p.text.simplified();
	for( int i = 0; i < p.children.size(); i++ )
	{
		text += collectText( parser, parser.at( p.children[i] ) );
		int n = p.children[i] + 1;
		while( n < parser.count() && parser.at(n).id == Html_unknown )
		{
			text += parser.at(n).text.simplified();
			n++;
		}
	}
	return text;
}

static QString coded( QString str )
{
	str.replace( QChar('&'), "&amp;" );
	str.replace( QChar('>'), "&gt;" );
	str.replace( QChar('<'), "&lt;" );
	return str;
}

static QString generateHtml( Context& ctx, const QTextHtmlParserNode& p )
{
	const QStringList blocked = QStringList() << "width" << "height" << "lang" << "class" << "size" << 
		"style" << "align" << "valign";

	QString html;
	if( p.id != Html_unknown && p.id != Html_font )
	{
		// qDebug() << p.tag << p.attributes; // TEST
		html += QString( "<%1" ).arg( p.tag );
		for( int i = 0; i < p.attributes.size() / 2; i++ )
		{
			const QString name = p.attributes[ 2 * i ].toLower();
			if( !blocked.contains( name ) )
				html += QString( " %1=\"%2\"" ).arg( name ).arg( p.attributes[ 2 * i + 1 ] );
		}
		html += ">";
	}
	html += coded( p.text ); 
	for( int i = 0; i < p.children.size(); i++ )
	{
		html += generateHtml( ctx, ctx.parser.at( p.children[i] ) );
		int n = p.children[i] + 1;
		while( n < ctx.parser.count() && ctx.parser.at(n).id == Html_unknown )
		{
			html += coded( ctx.parser.at(n).text ); 
			n++;
		}
	}
	if( p.id != Html_unknown && p.id != Html_font )
	{
		html += QString( "</%1>" ).arg( p.tag );
	}
	return html;
}

static void readParagraph( Context& ctx, const QTextHtmlParserNode& p )
{
	// TODO: falls nur einfacher Text enthalten, sollte nicht BML geschrieben werden.

	ctx.out->startEmbed();

	ctx.out->startFrame( "par" );
	const QString str = simplify( p.text );
	if( !str.isEmpty() )
	{
		ctx.out->startFrame( "rt" );
		ctx.out->writeString( str );
		ctx.out->endFrame(); // rt
	}
	readFrags( ctx, p );
	ctx.out->endFrame(); // par

	ctx.out->endEmbed( "Object Text" );
}

static int h2i( QTextHTMLElements f )
{
	switch( f )
	{
	case Html_h1:
		return 1;
	case Html_h2:
		return 2;
	case Html_h3:
		return 3;
	case Html_h4:
		return 4;
	case Html_h5:
		return 5;
	case Html_h6:
		return 6;
	default:
		return 0;
	}
}

static void dump( const QTextHtmlParser& parser, const QTextHtmlParserNode& p, int level )
{
	QByteArray indent( level, '\t' );
	QByteArray str = simplify( p.text ).toLatin1();
	qDebug( "%s %s %d    \"%s\"", indent.data(), p.tag.toLatin1().data(), p.id, str.data() );
	for( int i = 0; i < p.children.count(); i++ )
		dump( parser, parser.at( p.children[i] ), level + 1 );
}

static void dump2( const QTextHtmlParser& parser )
{
	for( int i = 0; i < parser.count(); i++ )
	{
		const QTextHtmlParserNode& p = parser.at( i );
		QByteArray str = simplify( p.text ).toLatin1();
		qDebug( "%d->%d %s id=%d  \"%s\"", i, p.parent, p.tag.toLatin1().data(), p.id, str.data() );
	}
}

static int findNumber( const QString& title )
{
	if( title.isEmpty() || !title[0].isDigit() )
		return -1;
	enum State { Digit, Dot };
	State s = Digit;
	int i = 0;
	while( i < title.size() )
	{
		switch( s )
		{
		case Digit:
			if( title[i].isDigit() )
				; // ignore
			else if( title[i] == QChar('.') )
				s = Dot;
			else if( title[i].isSpace() )
				return i + 1;
			else
				return i;
			break;
		case Dot:
			if( title[i].isDigit() )
				s = Digit;
			else if( title[i].isSpace() )
				return i + 1;
			else
				return -1; // Wenn nach einem Dot etwas anderes kommt als eine Zahl oder ein Space, breche die Übung ab.
			break;
		}
		i++;
	}
	return -1;
}

static void readHtmlNode( Context& ctx, const QTextHtmlParserNode& p )
{
	switch( p.id )
	{
	case Html_html:
	case Html_body:
	case Html_div:
	case Html_span:
	default:
		for( int i = 0; i < p.children.count(); i++ )
			readHtmlNode( ctx, ctx.parser.at( p.children[i] ) );
		break;
	case Html_p:
	case Html_address:
	case Html_a:
		if( !collectText( ctx.parser, p ).isEmpty() ) // RISK: teuer
		{
			ctx.out->startFrame( "obj" );
			ctx.out->writeInt( ctx.nextId++, "Absolute Number" );
			stdAtts( ctx );

			readParagraph( ctx, p );
			ctx.out->endFrame(); // obj
		}
		break;
	// case Html_center: // gibt es nicht
	case Html_ul:
	case Html_ol:
	case Html_table:
	case Html_dl:
    case Html_pre: 
		{
			ctx.out->startFrame( "obj" );
			ctx.out->writeInt( ctx.nextId++, "Absolute Number" );
			stdAtts( ctx );
			ctx.out->writeHtml( generateHtml( ctx, p ), "Object Text" );
			ctx.out->endFrame(); // obj
		}
		break;

	case Html_h1:
	case Html_h2:
	case Html_h3:
	case Html_h4:
	case Html_h5:
	case Html_h6:
		{
			const QString str = simplify( collectText( ctx.parser, p ) ); // ignoriere Formatierung
			if( !str.isEmpty() )
			{
				const int l = h2i( p.id );
				while( l <= ctx.trace.top() && ctx.trace.size() > 1 )
				{
					ctx.trace.pop();
					ctx.out->endFrame(); // obj
				}
				ctx.out->startFrame( "obj" );
				stdAtts( ctx );
				ctx.out->writeInt( ctx.nextId++, "Absolute Number" );
				const int split = findNumber( str );
				if( split != -1 )
				{
					ctx.out->writeString( str.left( split ), "~number" );
					ctx.out->writeString( str.mid( split ), "Object Heading" );
				}else
					ctx.out->writeString( str, "Object Heading" );
				ctx.trace.push( l );
			}
		}
		break;
	// case Html_hr: // Horizontal rule ignorieren
	// case Html_caption: // Deprecated

	case Html_dt:
	case Html_dd:
	case Html_li:
	case Html_tr:
	case Html_td:
	case Html_th:
	case Html_thead:
	case Html_tbody:
	case Html_tfoot:
		// NOTE: sollen hier nicht vorkommen
		break;
	case Html_title:
		// ignore
		break;
	}
}

static void structure( Context& ctx, const QTextHtmlParserNode& p, Section* parent )
{
	switch( p.id )
	{
	case Html_h1:
	case Html_h2:
	case Html_h3:
	case Html_h4:
	case Html_h5:
	case Html_h6:

		break;
	}

	for( int i = 0; i < p.children.count(); i++ )
	{
		const QTextHtmlParserNode& sub = ctx.parser.at( p.children[i] );
	}
}

static void findName( Context& ctx, const QString& name )
{
	for( int i = 0; i < ctx.parser.count(); i++ )
	{
		if( ctx.parser.at(i).id == Html_title )
		{
			QString str = simplify( ctx.parser.at(i).text );
			if( !str.isEmpty() )
			{
				ctx.out->writeString( str, "Name" );
				return;
			}
		}
	}
	ctx.out->writeString( name, "Name" );
}

bool HtmlImporter::parse( const QString& path )
{
	d_error.clear();

	QFile f( path );
	if( !f.open( QIODevice::ReadOnly ) )
	{
		d_error = tr("cannot open '%1' for reading").arg(path);
		return false;
	}

	Context ctx;
	ctx.out = &d_out;
	QFileInfo info( path );
	ctx.path = info.absoluteDir();
	ctx.parser.parse( QString::fromLatin1( f.readAll() ), 0 );
	if( ctx.parser.count() == 0 )
	{
		d_error = "HTML stream has no contents!";
		return false;
	}
	//Section root;
	//structure( ctx, ctx.parser.at(0), &root );

	try
	{
		const QString name = info.completeBaseName();
		d_out.open( name );
		d_out.writeString( "DoorScopeExport" );
		d_out.writeString( "0.3" );
		d_out.writeDate( QDateTime::currentDateTime() );
		d_out.startFrame( "mod" );

		d_out.writeString( QUuid::createUuid().toString(), "~moduleID" );
		stdAtts( ctx );
		d_out.writeString( name, "~modulePath" );
		findName( ctx, name );

		const QTextHtmlParserNode& n = ctx.parser.at(0);
		ctx.trace.push( 0 );
		readHtmlNode( ctx, n );

		for( int j = 1; j < ctx.trace.size(); j++ )
			ctx.out->endFrame(); // obj

		d_out.endFrame(); // mod
	}catch( std::exception& e )
	{
		d_error += e.what();
		d_out.close();
		return false;
	}
	d_out.close();
	return true;
}
