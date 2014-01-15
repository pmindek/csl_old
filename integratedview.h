#ifndef INTEGRATEDVIEW_H
#define INTEGRATEDVIEW_H

#include <QWidget>
#include <QVector2D>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGLWidget>
#include <QGLFramebufferObject>
#include <QEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>

class IntegratedView : public QWidget
{
	Q_OBJECT
public:
	explicit IntegratedView(QGLFramebufferObject *givenFbo, QString id, QWidget *embeddeWidget = 0, QWidget *parent = 0);
	~IntegratedView();
	QVector2D aspectRatio();
	QGLFramebufferObject *getFbo();
	void injectEvent(QEvent *event, QPoint pos);

	inline QWidget *getWidget() { return this->embeddedWidget; }

	void setId(QString id);
	QString getId() const;
	QRect getCenteredRect(QSize size);
	QRect getCenteredRect(int width, int height);

	static QEvent *changeEventPosition(QEvent *event, QPoint pos);

private:
	QGraphicsScene *scene;
	QGraphicsView *view;
	QGLFramebufferObject *fbo;

	QWidget *embeddedWidget;
	QString id;

signals:
	void updated();
	void eventReceived();
	
public slots:
	void render();

};

#endif // INTEGRATEDVIEW_H
