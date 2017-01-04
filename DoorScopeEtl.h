#ifndef DOORSCOPEETL_H
#define DOORSCOPEETL_H

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

#include <QtGui/QMainWindow>
#include <QAction>

class QTcpServer;
class QTextEdit;
class HtmlImporter;

class DoorScopeEtl : public QMainWindow
{
	Q_OBJECT

public:
	DoorScopeEtl(QWidget *parent = 0, Qt::WFlags flags = 0);
	~DoorScopeEtl();

	enum LogKind { LogTrace = 0, LogStatus = 1, LogError = 2 };
public slots:
	void onLog( QString, int kind );
protected slots:
	void onNewConnection();
	void onSetPort();
	void onClearLog();
	void onLogTrace();
	void onSetOutDir(); 
	void onData();
	void onTest();
	void onLogProto();
	void onAbout();
	void onParseHtml();
protected:
	void updatePort();
	// Overrides
	void resizeEvent( QResizeEvent * event );
private:
	QTcpServer* d_server;
	QTextEdit* d_log;
	QAction* d_logTrace;
	QAction* d_logProto;
	QString d_logPath;
	HtmlImporter* d_html;
	QString d_lastPath;
};

#endif // DOORSCOPEETL_H
