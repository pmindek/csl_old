#ifndef SELECTIONPOINT_H
#define SELECTIONPOINT_H

#include <QPointF>

class SelectionPoint
{
public:
	SelectionPoint();
	SelectionPoint(QPointF pos, qreal pressure = 0.0, qreal data = 0.0);
	QPointF getPos();
	qreal getPressure();
	qreal getData();

	void setX(qreal x);
	void setY(qreal y);
	void setPressure(qreal pressure);
	void setData(qreal data);

private:
	QPointF pos;
	qreal pressure;
	qreal data;
};


#endif // SELECTIONPOINT_H
