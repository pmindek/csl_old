#include "selectionpoint.h"

SelectionPoint::SelectionPoint()
{
	this->pos = QPointF(0.0, 0.0);
	this->pressure = 0.0;
	this->data = 0.0;
}

SelectionPoint::SelectionPoint(QPointF pos, qreal pressure, qreal data)
{
	this->pos = pos;
	this->pressure = pressure;
	this->data = data;
}

QPointF SelectionPoint::getPos()
{
	return this->pos;
}

qreal SelectionPoint::getPressure()
{
	return this->pressure;
}

qreal SelectionPoint::getData()
{
	return this->data;
}

void SelectionPoint::setX(qreal x)
{
	this->pos.setX(x);
}

void SelectionPoint::setY(qreal y)
{
	this->pos.setY(y);
}

void SelectionPoint::setPressure(qreal pressure)
{
	this->pressure = pressure;
}

void SelectionPoint::setData(qreal data)
{
	this->data = data;
}
