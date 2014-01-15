#ifndef SKETCHER_H
#define SKETCHER_H

#include <QGLContext>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QList>
#include <QString>
#include <QPainter>
#include "selectionpoint.h"
#include "selection.h"
#include "anchor.h"

#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA

#define GL_RGBA32F 0x8814
#define GL_R32F 0x822E


typedef void (APIENTRY * PFNGLBINDIMAGETEXTUREEXTPROC) (GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format);
typedef void (APIENTRY * PFNGLMEMORYBARRIEREXTPROC) (GLbitfield barriers);

class Sketcher : public QObject
{
Q_OBJECT

public:
	//Sketcher(QWidget * parent = 0, const QGLWidget * shareWidget = 0);
	Sketcher();
	~Sketcher();

	void reset();
	Selection *createSelection(int width, int height);
	QWidget *getLastIVWidget();
	GLuint getLastTexture();
	void addPoint(SelectionPoint point);
	void renderSelection(QGLShaderProgram *shader, Selection *selection);
	void renderLastSelection(QGLShaderProgram *shader);

	qreal getMaximumDistance(QList<qreal> parameters);

	void setSelections(QList<Selection *> selections);
	QList<Selection *> &getSelections();

/*
protected:
	void initializeGL();*/

private:
	void createSelectionTexture(Selection *selection);

	GLuint texture;
	GLuint textureIndex;

	QList<Selection *> selections;
	QList<Anchor *> anchors;

signals:
	void selectionRendered();
};

#endif // SKETCHER_H
