#ifndef HISTOWIDGET_H
#define HISTOWIDGET_H

#include <QWidget>

#include <QGLWidget>
#include <QGLFramebufferObject>
#include <QTimer>
#include <QTime>

class HistoWidget : public QWidget
{
	Q_OBJECT
public:
	explicit HistoWidget(QString forCaption, QWidget *parent = 0);
	void loadImage(QString fileName);

protected:
	/*QGLFramebufferObject *fbo;
	void paintGL();
	void resizeGL(int w, int h);*/
	void paintEvent(QPaintEvent *);

	QTimer *timer;
	QTime *time;

	QList<QList<int> > histogram;

	QString forCaption;
signals:

public slots:
	void clearArrays();
	void addArray(QList<int> array, int selection = -1, QString caption = "");
	void arraysFinish();

};

#endif // HISTOWIDGET_H
