#ifndef SELECTION_H
#define SELECTION_H

#include <QList>
#include <QList>
#include <QString>
#include <QGLFramebufferObject>
#include "selectionpoint.h"
#include "integratedview.h"

class Selection
{
public:
	Selection();
	~Selection();

	void reset();
	void recordPoint(SelectionPoint point);
	QList<SelectionPoint> &getPoints();
	/*void addParameter(qreal parameter, QString parameterName = "");
	void setParameters(QList<QVariant> parameters, QList<QString> parameterNames = QList<QString>());
	qreal getParameter(int i);
	QString getParameterName(int i);*/
	void setRendered(bool rendered = true);
	bool isRendered();

	qreal getMaximumDistance(QList<QVariant> parameters);

	IntegratedView *addIntegratedView(IntegratedView *view);
	QList<IntegratedView *> &getIntegratedViews();

	void setSelectionMask(QGLFramebufferObject *selectionMask);
	QGLFramebufferObject *getSelectionMask();

private:
	bool rendered;

	QList<SelectionPoint> points;
	/*QList<QVariant> parameters;
	QList<QString> parameterNames;*/
	QList<IntegratedView *> integratedViews;

	QGLFramebufferObject *selectionMask;
};

#endif // SELECTION_H
