#ifndef ANCHOR_H
#define ANCHOR_H

#include <QObject>
#include <QMatrix4x4>
#include <QGLFramebufferObject>
#include "selection.h"

class Anchor : public QObject
{
Q_OBJECT

public:
	Anchor(QObject *parent);
	~Anchor();

	void setId(int id);
	int getId();

	QList<QVariant> getParameters();
	QVariant getParameter(int index);
	QList<QString> getParameterNames();
	QString getParameterName(int index);

	void setParameters(QList<QVariant> parameters, QList<QString> parameterNames);

	QMatrix4x4 getNoScalingMatrix();
	void setNoScalingMatrix(QMatrix4x4 noScalingMatrix);

	QMatrix4x4 getMatrix();
	void setMatrix(QMatrix4x4 matrix);

	void setPosition(QVector3D position);
	QVector3D getPosition();

	void setProjectedPosition(QVector2D projectedPosition);
	QVector2D getProjectedPosition();

	void setScreenPosition(QVector2D screenPosition);
	QVector2D getScreenPosition();

	void setScreenCenterPosition(QVector2D screenCenterPosition);
	QVector2D getScreenCenterPosition();

	void calculateZ(QMatrix4x4 matrix);
	qreal getZ();
	qreal getAlpha();

	void addSelection(Selection *selection);
	QList<Selection *> &getSelections();

	void setScreenshot(QGLFramebufferObject * screenshot);
	QGLFramebufferObject *getScreenshot();

	void setBaseScreenshot(QGLFramebufferObject * baseScreenshot);
	QGLFramebufferObject *getBaseScreenshot();

	inline void setCluster(int cluster) { this->cluster = cluster; }
	inline int getCluster() { return this->cluster; }
private:
	int cluster;

	static int idCounter;
	int id;

	QMatrix4x4 noScalingMatrix;
	QMatrix4x4 matrix;
	QVector3D position;
	QVector2D projectedPosition;
	QVector2D screenPosition;
	QVector2D screenCenterPosition;
	qreal z;

	QList<QVariant> parameters;
	QList<QString> parameterNames;
	QList<Selection *> selections;

	QGLFramebufferObject *baseScreenshot;
	QGLFramebufferObject *screenshot;
};

#endif // ANCHOR_H
