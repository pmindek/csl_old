#ifndef PRSShaderEnhancer_H
#define PRSShaderEnhancer_H

#include <QtOpenGL>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QStringList>
#include <QGLShaderProgram>

#include "gl/glext.h"

#include "prsutil.h"

class PRSShaderEnhancer
{
public:
	PRSShaderEnhancer();
	~PRSShaderEnhancer();

	QString enhance(QString shader, QString version);

	void init(QGLShaderProgram *shader, QGLShaderProgram *clearShader = 0);
	void clearStorage();
	void use(GLuint what, GLuint mouseX, GLuint mouseY);
	void unuse();
	void setPower(GLfloat power);


	void prepareShader(GLuint unit, GLuint selectionMasksArray, int activeSelectionsCount);
	GLuint *getHistogramData();
	GLuint getHistogramStorage();
	GLuint getFragmentStorage();
	int getParameterCount();
	int getActiveSelectionsCount();
	int getHistogramSize();

	QStringList getParameterCaptions();

private:
	QString insertCoreFunctionality(QString shader);

	int parameterCount;
	int activeSelectionsCount;

	QGLShaderProgram *shader, *clearShader;

	GLuint histogramStorage;
	GLuint fragmentStorage;

	QString version;

	int histogramX, histogramY;

	QStringList parameterCaptions;
};

#endif // PRSShaderEnhancer_H
