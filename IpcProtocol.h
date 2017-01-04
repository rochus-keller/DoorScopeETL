#ifndef IPCPROTOCOL_H
#define IPCPROTOCOL_H

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
#include <QTcpSocket>
#include "StreamAgent.h"

class IpcProtocol : public QObject
{
	Q_OBJECT

public:
	IpcProtocol(QObject *parent);
	~IpcProtocol();

	StreamAgent d_agent;
	enum { s_maxParam = 3 };
	void parse( QIODevice* );
public slots:
	void onError(QAbstractSocket::SocketError);
	void onData();
protected:
	void errorClose( QIODevice*, QString );
	void execute(QIODevice*);
	void consume( QIODevice* );
	void evaluate( QIODevice* );
private:
	enum State 
	{
		Idle,
		ReadCode,
		ReadNum,
		ReadVal
	};
	int d_state;
	int d_command;
	int d_num;
	int d_pn;
	QVariant d_param[s_maxParam];
	QByteArray d_buf;
};

#endif // IPCPROTOCOL_H
