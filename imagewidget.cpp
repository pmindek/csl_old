#include "imagewidget.h"
#include <QPainter>
#include <QtDebug>

ImageWidget::ImageWidget(QWidget *parent) :
    QWidget(parent)
{
}

void ImageWidget::setTitle(QString title)
{
	this->title = title;
}

void ImageWidget::setImage(QImage image)
{
	this->image = image;
	this->setFixedSize(this->image.size());
	this->update();
}

void ImageWidget::paintEvent(QPaintEvent *)
{
	QPainter p(this);

	qDebug() << "painting" << image.size() << "image on" << this->size() << "widget";

	p.fillRect(0, 0, this->width(), this->height(), QColor(255, 255, 255, 255));

	p.drawImage(0, 0, this->image);

	/*p.setPen(QColor(0,0,0,255));
	QFont f = p.font();
	f.setPixelSize(40);
	p.setFont(f);
	p.drawText(0, 0, this->width(), this->height(), Qt::AlignTop | Qt::AlignHCenter, this->title);*/
}
