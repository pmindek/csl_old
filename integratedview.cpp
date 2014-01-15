#include "integratedview.h"
#include <QtDebug>
#include <QHBoxLayout>
#include <QApplication>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneEvent>
#include <QTime>

IntegratedView::IntegratedView(QGLFramebufferObject *givenFbo, QString id, QWidget *embeddeWidget, QWidget *parent) :
	QWidget(parent)
{
	this->id = id;

	this->setVisible(false);

	this->embeddedWidget = embeddeWidget;
	this->setFixedSize(this->embeddedWidget->size());

	//connect(this, SIGNAL(eventReceived()), this->embeddedWidget, SLOT(update()));

	QHBoxLayout *layout = new QHBoxLayout();
	layout->setMargin(0);
	layout->addWidget(this->embeddedWidget);

	QWidget *w = new QWidget();
	w->setFixedSize(this->embeddedWidget->size());
	w->setLayout(layout);

	this->scene = new QGraphicsScene(this);

	//connect(this->scene, SIGNAL(changed(QList<QRectF>)), this->embeddedWidget, SLOT(update()));


	this->view = new QGraphicsView(this->scene);
	//this->view->adjustSize();
	this->view->setFixedSize(this->embeddedWidget->size());

	//this->view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers | QGL::IndirectRendering)));
	
	this->view->setViewport(new QWidget());

	this->view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);  //QGraphicsView::FullViewportUpdate);
	this->view->setRenderHint(QPainter::Antialiasing, true);

	this->scene->addWidget(w);

	this->view->setAttribute(Qt::WA_DontShowOnScreen);
	this->view->show();
	this->view->close();

	this->view->move(0, 0);
	this->view->setInteractive(true);

	/*QGLFramebufferObjectFormat *format = new QGLFramebufferObjectFormat();
	format->setSamples(4);
	format->setAttachment(QGLFramebufferObject::CombinedDepthStencil);
	this->fbo = new QGLFramebufferObject(this->embeddedWidget->size(), *format);*/

	//this->fbo = new QGLFramebufferObject(this->embeddedWidget->size()/*, QGLFramebufferObject::CombinedDepthStencil*/);
	this->fbo = givenFbo;

	glBindTexture(GL_TEXTURE_2D, this->fbo->texture());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	connect(this->scene, SIGNAL(changed(QList<QRectF>)), this, SLOT(render()));
}

IntegratedView::~IntegratedView()
{
	this->close();
}

QVector2D IntegratedView::aspectRatio()
{
	return QVector2D(this->embeddedWidget->width(), this->embeddedWidget->height());
}

QGLFramebufferObject *IntegratedView::getFbo()
{
	return this->fbo;
}

void IntegratedView::injectEvent(QEvent *event, QPoint pos)
{
	/*QWidget *receiver = this->embeddedWidget;
	QWidget *child;
	QPoint p = pos;

	while (receiver->childAt(p) != 0)
	{
		child = receiver->childAt(p);
		p = child->mapFrom(receiver, p);
		receiver = child;
	}

	Application::postEvent(receiver, IntegratedView::changeEventPosition(event, p));*/

	QEvent::Type type = event->type();

	if (type == QEvent::Wheel)
	{
		QGraphicsSceneWheelEvent wheelEvent(QEvent::GraphicsSceneWheel);
		wheelEvent.setDelta(((QWheelEvent *) event)->delta());
		wheelEvent.setOrientation(((QWheelEvent *) event)->orientation());
		wheelEvent.setPos(pos);
		wheelEvent.setScenePos(pos);
		wheelEvent.setScreenPos(pos);
		wheelEvent.setAccepted(false);
		wheelEvent.setButtons(((QWheelEvent *) event)->buttons());
		wheelEvent.setModifiers(((QWheelEvent *) event)->modifiers());
		wheelEvent.setWidget(0);

		QApplication::sendEvent(this->scene, &wheelEvent);
	}
	else if (type == QEvent::KeyPress)
	{
		qDebug() << "key event";
		//QKeyEvent *keyEvent = new QKeyEvent(event);
		//this->scene->sendEvent()
		//QApplication::sendEvent(this->scene, event);
		QKeyEvent *ev = new QKeyEvent(QEvent::KeyPress,
										  ((QKeyEvent *) event)->key(),
										  ((QKeyEvent *) event)->modifiers(),
										  ((QKeyEvent *) event)->text());

		QApplication::sendEvent(this->scene, ev);
	}
	else
	{
		switch (event->type())
		{
		case QEvent::MouseMove:
			type = QEvent::GraphicsSceneMouseMove;
			break;
		case QEvent::MouseButtonPress:
			type = QEvent::GraphicsSceneMousePress;
			break;
		case QEvent::MouseButtonRelease:
			type = QEvent::GraphicsSceneMouseRelease;
			break;
		case QEvent::MouseButtonDblClick:
			type = QEvent::GraphicsSceneMouseDoubleClick;
			break;
		}

		QGraphicsSceneMouseEvent mouseEvent(type);
		mouseEvent.setPos(pos);
		mouseEvent.setScenePos(pos);
		mouseEvent.setScreenPos(pos);
		mouseEvent.setButtonDownPos(((QMouseEvent *) event)->button(), pos);
		mouseEvent.setButtonDownScenePos(((QMouseEvent *) event)->button(), pos);
		mouseEvent.setButtonDownScreenPos(((QMouseEvent *) event)->button(), pos);
		mouseEvent.setAccepted(false);
		mouseEvent.setButton(((QMouseEvent *) event)->button());
		mouseEvent.setButtons(((QMouseEvent *) event)->buttons());
		mouseEvent.setModifiers(((QMouseEvent *) event)->modifiers());
		mouseEvent.setWidget(0);

		QApplication::sendEvent(this->scene, &mouseEvent);
	}

	emit eventReceived();
}

void IntegratedView::render()
{
	QPainter *p = new QPainter(this->fbo);

	//p->fillRect(QRectF(0,0,this->fbo->width(), this->fbo->height()), QColor(255, 0, 0, 255));
	this->view->render(p, QRectF(0, 0, this->fbo->width(), this->fbo->height()), QRect(0, 0, this->fbo->width(), this->fbo->height()));
	//this->scene->render(p, QRectF(0, 0, this->fbo->width(), this->fbo->height()), QRect(0, 0, this->fbo->width(), this->fbo->height()));

	delete p;

	emit updated();
}

QEvent *IntegratedView::changeEventPosition(QEvent *event, QPoint pos)
{
	if (event->type() == QEvent::Wheel)
	{
		return new QWheelEvent(pos,
							   ((QWheelEvent *) event)->delta(),
							   ((QWheelEvent *) event)->buttons(),
							   ((QWheelEvent *) event)->modifiers(),
							   ((QWheelEvent *) event)->orientation());
	}
	else
	if (event->type() == QEvent::MouseButtonDblClick ||
		event->type() == QEvent::MouseButtonPress ||
		event->type() == QEvent::MouseButtonRelease ||
		event->type() == QEvent::MouseMove)
	{
		return new QMouseEvent(((QMouseEvent *) event)->type(),
							   pos,
							   ((QMouseEvent *) event)->button(),
							   ((QMouseEvent *) event)->buttons(),
							   ((QMouseEvent *) event)->modifiers());
	}
	else
		return 0;
}

void IntegratedView::setId(QString id)
{
	this->id = id;
}

QString IntegratedView::getId() const
{
	return this->id;
}

QRect IntegratedView::getCenteredRect(QSize size)
{
	QSize fullSize = this->size();

	return QRect(size.width() / 2 - fullSize.width() / 2, size.height() / 2 - fullSize.height() / 2, fullSize.width(), fullSize.height());
}

QRect IntegratedView::getCenteredRect(int width, int height)
{
	return this->getCenteredRect(QSize(width, height));
}
