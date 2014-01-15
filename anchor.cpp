#include "anchor.h"

int Anchor::idCounter = 0;

Anchor::Anchor(QObject *parent) :
	QObject(parent)
{
	this->id = (++idCounter);
	this->selections.clear();

	this->baseScreenshot = 0;
	this->screenshot = 0;
}

Anchor::~Anchor()
{
	if (this->baseScreenshot != 0)
	{
		this->baseScreenshot->release();
		delete this->baseScreenshot;
	}
	if (this->screenshot != 0)
	{
		this->screenshot->release();
		delete this->screenshot;
	}
}

void Anchor::setId(int id)
{
	this->id = id;
	if (this->id > this->idCounter)
		this->idCounter = this->id;
}

int Anchor::getId()
{
	return this->id;
}

QMatrix4x4 Anchor::getNoScalingMatrix()
{
	return this->noScalingMatrix;
}

void Anchor::setNoScalingMatrix(QMatrix4x4 noScalingMatrix)
{
	this->noScalingMatrix = noScalingMatrix;
}

QMatrix4x4 Anchor::getMatrix()
{
	return this->matrix;
}

void Anchor::setMatrix(QMatrix4x4 matrix)
{
	this->matrix = matrix;

	QVector4D point = matrix.inverted().map(QVector4D(0.0, 0.0, -1.0, 1.0));

	this->position = point.toVector3D();
}

QList<QVariant> Anchor::getParameters()
{
	return this->parameters;
}

QList<QString> Anchor::getParameterNames()
{
	return this->parameterNames;
}

void Anchor::setParameters(QList<QVariant> parameters, QList<QString> parameterNames)
{
	this->parameters = parameters;
	this->parameterNames = parameterNames;
}


void Anchor::setPosition(QVector3D position)
{
	//qDebug() << this->id << "CHANGED POSITION TO " << position;
	this->position = position;
}

QVector3D Anchor::getPosition()
{
	return this->position;
}

void Anchor::setProjectedPosition(QVector2D projectedPosition)
{
	this->projectedPosition = projectedPosition;
}

QVector2D Anchor::getProjectedPosition()
{
	return this->projectedPosition;
}

void Anchor::setScreenPosition(QVector2D screenPosition)
{
	this->screenPosition = screenPosition;
}

QVector2D Anchor::getScreenPosition()
{
	return this->screenPosition;
}

void Anchor::setScreenCenterPosition(QVector2D screenCenterPosition)
{
	this->screenCenterPosition = screenCenterPosition;
}

QVector2D Anchor::getScreenCenterPosition()
{
	return this->screenCenterPosition;
}


void Anchor::calculateZ(QMatrix4x4 matrix)
{
	QVector4D point = matrix.map(QVector4D(this->position.x(), this->position.y(), this->position.z(), 1.0));
	this->z = point.toVector3D().z();
}

qreal Anchor::getZ()
{
	return this->z;
}

qreal Anchor::getAlpha()
{
	qreal max = 50;
	return this->z > 0 ? 0.0 : (this->z < -max ? 0.0 : 1.0 - ((-this->z) / max));
}

void Anchor::addSelection(Selection *selection)
{
	this->selections << selection;
}

QList<Selection *> &Anchor::getSelections()
{
	return this->selections;
}

void Anchor::setScreenshot(QGLFramebufferObject * screenshot)
{
	this->screenshot = screenshot;
}

QGLFramebufferObject *Anchor::getScreenshot()
{
	return this->screenshot;
}

void Anchor::setBaseScreenshot(QGLFramebufferObject * baseScreenshot)
{
	this->baseScreenshot = baseScreenshot;
}

QGLFramebufferObject *Anchor::getBaseScreenshot()
{
	return this->baseScreenshot;
}
