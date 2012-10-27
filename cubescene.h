#ifndef CUBESCENE_H
#define CUBESCENE_H

#include <QSize>
#include <QString>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

#include <QProcess>
#include <QDebug>

#include "cubecellitem.h"

#define DEFAULT_CELL_WIDTH 120
#define MIN_PAD 15
#define MAX_ROW_COL_SIZE 16

#define COL_NUM 3
#define ROW_NUM 5

#define GRID_COLOR 120, 120, 120
#define GRID_WIDTH 1

#define WHITE_CELL_COLOR "White"
#define WHITE_CELL_POS -1
#define WHITE_CELL_IDX (WHITE_CELL_POS * 2)

#define THUMNAIL_X_PAD 6
#define THUMNAIL_CELL_POS -2
#define THUMNAIL_CELL_IDX (THUMNAIL_CELL_POS * 2)

#define STARTBUTTON_X_PAD 6
#define STARTBUTTON_CELL_POS -3
#define STARTBUTTON_CELL_IDX (STARTBUTTON_CELL_POS * 2)

#define BACKGROUND_FILE "gnu_tux-320x240.png"

#include <QThread>

class AdbExecutor
{
public:
	AdbExecutor() {};
	AdbExecutor(const char *c) { args << c; };

	void addArg(const char *a) { args << a; };
	void addArg(const QString &a) { args << a; };
	void addArg(const QStringList &a) { args << a; };

	bool exitSuccess(void) { return exitCode == 0; };

	void clear(void) {
		args.clear();
		error.clear();
		output.clear();
		exitCode = 0;
	}

	int run(bool waitForFinished = true) {
		cmd = "adb";

		qDebug() << "Exec: " << cmd << " " << args;
		p.start(cmd, args);

		if (waitForFinished) {
			return wait();
		}

		return 0;
	}

	int wait() {
		p.waitForFinished();

		output = p.readAllStandardOutput();
		error = p.readAllStandardError();

		return p.exitCode();
	}

	void printErrorInfo() {
		qDebug() << "Process return:" << exitCode;
		qDebug() << error;
	}

	bool outputEqual (const char *str) {
		return output.startsWith(str);
	}

	QByteArray error;
	QByteArray output;

private:
	int exitCode;
	QProcess p;
	QString cmd;
	QStringList args;
};

class FbReader : public QThread
{
	Q_OBJECT

public:
	FbReader(QObject * parent);

	bool supportCompress();
	bool setCompress(bool value);
	int getScreenInfo(int &, int &, int &);

protected:
	int screenCap(QByteArray &bytes);
	void parseFbData(const QByteArray &bytes);
	void run();

signals:
    void newFbReceived(QByteArray *bytes);

private:
	char *buf;
	bool do_compress;
};

class CubeScene : public QGraphicsScene
{
	Q_OBJECT

public:
    CubeScene(QObject * parent = 0);
    ~CubeScene();

    void loadImage (const QString &file);

    void initialize (void);

    void startPlay(void);

    void moveAllCell(const QPoint &pos, int off_row, int off_col);

    void moveCell(const QPoint &pos, int row, int col);

    void checkAllCell(void);

    void setBgVisible(bool visible) { bg_mask->setVisible(visible); };
    bool getBgVisible(void)         { return bg_mask->isVisible(); };

    QSize getSize(void) { return QSize(cube_width, cube_height); }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void keyReleaseEvent(QKeyEvent * event);

    void drawGrid (int row, int col);
    QPoint getCellPos(int row, int col);
    QPoint scenePosToVirtual(QPointF pos);
    void sendTap(QPoint pos);
    void sendEvent(QPoint pos);
    QStringList newEventCmd (int type, int code, int value);
    void sendVirtualClick(QPoint pos);
    void sendVirtualKey(int key);
    void startFbReader();
    void stopFbReader();

public slots:
    void updateSceen(QByteArray *bytes);

private:
    CubeCellItem *bg_mask;
    CubeCellItem *b_items[MAX_ROW_COL_SIZE][MAX_ROW_COL_SIZE];
    CubeCellItem *b_curr_items[MAX_ROW_COL_SIZE][MAX_ROW_COL_SIZE];
    int m_white_row;
    int m_white_col;

    int fb_width;
    int fb_height;
    int pixel_format;
    int cube_width;
    int cube_height;
    int cell_width;
    int cell_height;
    int row_size;
    int col_size;
    int x_pad;
    int y_pad;
    int v_width;
    int v_height;

    QPixmap pixmap;
    FbReader *reader;
};

#endif // CUBESCENE_H
