#ifndef DOCIMPORTER_H
#define DOCIMPORTER_H

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
#include <QMap>
#include <QList>
#include "StreamAgent.h"

class HtmlImporter : public QObject
{
public:
	enum CharFormat { Italic, Bold, Underline, Strikeout, Super, Sub, Fixed }; // Index in bitset

	HtmlImporter(QObject *parent = 0);

	bool parse( const QString& path ); // return: false bei fehler
	const QString& getError() const { return d_error; }
	const QString& getInfo() const { return d_info; }
private:
	QString d_error;
	QString d_info;
	StreamAgent d_out;
};

#endif // DOCIMPORTER_H
