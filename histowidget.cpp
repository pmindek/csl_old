#include "histowidget.h"
#include <cmath>
#include <QtDebug>

HistoWidget::HistoWidget(QString forCaption, QWidget *parent) :
    QWidget(parent)
{
	this->setFixedSize(256, 180);

	timer = new QTimer(this);
	time = new QTime();

	connect(timer, SIGNAL(timeout()), this, SLOT(update()));
	timer->start(10);

	time->start();

	this->forCaption = forCaption;
}

void HistoWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);

	p.fillRect(0, 0, this->width(), this->height(), QColor(255, 255, 255, 255));


	//qDebug() << max;

	for (int i = 0; i < this->histogram.count(); i++)
	{
		int max = 0;
		for (int j = 0; j < this->histogram[i].count(); j++)
		{
			if (j == 0 || max < this->histogram[i][j])
				max = this->histogram[i][j];
		}

		if (forCaption == "0")
		{
			if (this->histogram.count() == 1)
				p.setPen(QColor(255, 0, 0, 64));
			else
				p.setPen(QColor(255 - (int) (((qreal) i / (qreal) (this->histogram.count() - 1)) * 255.0), 0, 0, 64));
		}
		else
		{
			if (this->histogram.count() == 1)
				p.setPen(QColor(0, 0, 255, 64));
			else
				p.setPen(QColor(0, 0, 255 - (int) (((qreal) i / (qreal) (this->histogram.count() - 1)) * 255.0), 64));
		}

		for (int j = 0; j < this->histogram[i].count(); j++)
		{
			p.drawLine(j, this->height(), j, this->height() - (int)


					   (
						   pow((qreal) this->histogram[i][j] / (qreal) max, 0.1)

						   * (qreal) this->height())


					   );
		}
	}

	//max = 10000;
				/*int index;
				for (int i = 0; i < this->width(); i++)
				{
					index = (int) floor((qreal) i / (qreal) this->width() * (qreal) this->histogram.count());
					p.drawLine(i, this->height(), i, this->height() - (int) ((qreal) this->histogram[index] / (qreal) max * (qreal) this->height()));
				}*/



	/*p.drawImage(0, 0, this->fbo->toImage());*/


	/*this->makeCurrent();
	p.beginNativePainting();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, 1.0, -1.0, 1.0, 1.0, -1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, this->width(), this->height());
	glLoadIdentity();

	glRotatef((qreal) (time->elapsed() % 10000) / 10000.0 * 360.0, 0.0, 0.0, 1.0);

	glClearColor(0.7, 0.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBegin(GL_QUADS);
		glVertex2f( 0.1,  0.1);
		glVertex2f(-0.1,  0.1);
		glVertex2f(-0.1, -0.1);
		glVertex2f( 0.1, -0.1);
	glEnd();

	p.endNativePainting();*/
}

void HistoWidget::clearArrays()
{
	for (int i = 0; i < this->histogram.count(); i++)
	{
		this->histogram[i].clear();
	}
	this->histogram.clear();
	update();
}

void HistoWidget::addArray(QList<int> array, int selection, QString caption)
{
	if (caption == forCaption)
		this->histogram << array;

	//qDebug() << array.size() << "sel" << selection << "cap" << caption;
	//qDebug() << array;


	/*for (int i = 0; i < this->histogram.length(); i++)
		qDebug() << i << "~" << this->histogram[i];*/

	update();
}

void HistoWidget::arraysFinish()
{
	//qDebug() << "forCaption" << forCaption << "has" << histogram.size();
	update();
}
