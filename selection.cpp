#include "selection.h"
#include <QQuaternion>
#include <QtDebug>

Selection::Selection()
{
	this->reset();
}

Selection::~Selection()
{
	if (this->selectionMask != 0)
	{
		delete this->selectionMask;
	}

	this->reset();
}

void Selection::reset()
{
	this->rendered = false;

	this->points.clear();
	/*this->parameters.clear();
	this->parameterNames.clear();*/

	//selection cannot delete integrated views, for the presentation manages them
	/*for (int i = 0; i < this->integratedViews.count(); i++)
	{
		delete this->integratedViews[i];
	}*/
	this->integratedViews.clear();
}

void Selection::recordPoint(SelectionPoint point)
{
	this->points << point;
	this->setRendered(false);
}

QList<SelectionPoint> &Selection::getPoints()
{
	return this->points;
}

/*void Selection::addParameter(qreal parameter, QString parameterName)
{
	this->parameters << parameter;
	this->parameterNames << parameterName;
}

void Selection::setParameters(QList<qreal> parameters, QList<QString> parameterNames)
{
	this->parameters = parameters;
	this->parameterNames = parameterNames;
}

qreal Selection::getParameter(int i)
{
	return this->parameters[i];
}

QString Selection::getParameterName(int i)
{
	return this->parameterNames[i];
}*/

void Selection::setRendered(bool rendered)
{
	this->rendered = rendered;
}

bool Selection::isRendered()
{
	return this->rendered;
}

qreal Selection::getMaximumDistance(QList<qreal> parameters)
{
	qreal max = 0.0;
	/*for (int i = 0; i < qMin(parameters.count(), this->parameters.count()); i++)
	{
		if (i == 0 || max < qAbs(parameters[i] - this->parameters[i]))
			max = qAbs(parameters[i] - this->parameters[i]);
	}*/

	/*QQuaternion q0(this->parameters[3], this->parameters[0], this->parameters[1], this->parameters[2]);
	QQuaternion q1(parameters[3], parameters[0], parameters[1], parameters[2]);

	QQuaternion q = (q0.normalized() - q1.normalized());

	max = q.length();*/


	return max;
}

void Selection::setSelectionMask(QGLFramebufferObject *selectionMask)
{
	this->selectionMask = selectionMask;
}

QGLFramebufferObject *Selection::getSelectionMask()
{
	return this->selectionMask;
}

IntegratedView* Selection::addIntegratedView(IntegratedView *view)
{
	this->integratedViews << view;
	return view;
}

QList<IntegratedView *> &Selection::getIntegratedViews()
{
	return this->integratedViews;
}
