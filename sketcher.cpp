#include "sketcher.h"
#include <QtDebug>

//Sketcher::Sketcher(QWidget * parent, const QGLWidget * shareWidget) : QGLWidget(parent, shareWidget)
Sketcher::Sketcher()
{
	this->reset();

	//this->setVisible(false);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2048, 8, 0, GL_RGBA, GL_FLOAT, 0);

	textureIndex = 3;
	((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT")) (textureIndex, texture, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);

	/*gl = new QGLContext(QGLFormat(QGL::DoubleBuffer | QGL::DepthBuffer | QGL::Rgba | QGL::AlphaChannel | QGL::IndirectRendering));
	gl->create();
	gl->makeCurrent();

	qDebug() << "valid: " << gl->isValid();*/
}
Sketcher::~Sketcher()
{
	this->reset();
}

/*void Sketcher::initializeGL()
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2048, 8, 0, GL_RGBA, GL_FLOAT, 0);

	qDebug() << "1";

	textureIndex = 3;
	((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT")) (textureIndex, texture, 0, false, 0, GL_READ_WRITE, GL_RGBA32F);

	qDebug() << "2";
}*/

void Sketcher::reset()
{
	for (int i = 0; i < this->selections.count(); i++)
	{
		delete this->selections[i];
	}
	this->selections.clear();

	/*for (int i = 0; i < this->anchors.count(); i++)
	{
		delete this->anchors[i];
	}
	this->anchors.clear();*/
}

Selection* Sketcher::createSelection(int width, int height)
{
	Selection *selection = new Selection();
	//selection->setParameters(parameters, parameterNames);

	QGLFramebufferObject *fbo = new QGLFramebufferObject(width, height);

	glBindTexture(GL_TEXTURE_2D, fbo->texture());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	selection->setSelectionMask(fbo);
	this->selections << selection;
	return selection;
}

QWidget *Sketcher::getLastIVWidget()
{
	return this->selections.last()->getIntegratedViews().last()->getWidget();
}

GLuint Sketcher::getLastTexture()
{
	return this->selections.last()->getSelectionMask()->texture();
}

void Sketcher::addPoint(SelectionPoint point)
{
	this->selections.last()->recordPoint(point);
}

void Sketcher::renderSelection(QGLShaderProgram *shader, Selection *selection)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	selection->getSelectionMask()->bind();

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glViewport(0, 0, selection->getSelectionMask()->size().width(), selection->getSelectionMask()->size().height());


	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(-1.0, +1.0, -1.0, +1.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glClearColor(0.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	shader->bind();
	shader->setUniformValue("texture", textureIndex);

	//glActiveTexture(GL_TEXTURE3);
	this->createSelectionTexture(selection);
	//glActiveTexture(GL_TEXTURE0);

	glDisable(GL_TEXTURE_2D);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_COLOR_MATERIAL);

	glEnable(GL_COLOR_MATERIAL);
	glColor4f(1.0, 0.0, 1.0, 1.0);
	glBegin(GL_QUADS);
		glVertex2f( 1.0, -1.0);
		glVertex2f(-1.0, -1.0);
		glVertex2f(-1.0,  1.0);
		glVertex2f( 1.0,  1.0);
	glEnd();
	glEnable(GL_COLOR_MATERIAL);

	shader->release();

	selection->getSelectionMask()->release();

	/*QImage p = selection->getSelectionMask()->toImage();
	p.save("c:/users/mindek/documents/a.png");*/

	selection->setRendered();


	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopClientAttrib();
	glPopAttrib();

	emit selectionRendered();
}

void Sketcher::renderLastSelection(QGLShaderProgram *shader)
{
	this->renderSelection(shader, this->selections.last());	
}

void Sketcher::createSelectionTexture(Selection *selection)
{
	QList<SelectionPoint> stroke = selection->getPoints();

	GLfloat *d = (GLfloat *) malloc(sizeof(GLfloat) * 2048 * 8 * 4);

	for (int j = 0; j <= stroke.count(); j++)
	{
		for (int k = 0; k < 4; k++)
		{
			qreal data;
			if (j == 0)
			{
				data = (qreal) stroke.count();
			}
			else
			{
				/*if (k == 0)
					qDebug() << stroke[j - 1].getPos();*/

				switch (k)
				{
				case 0:
					data = stroke[j - 1].getPos().x();
					break;
				case 1:
					data = stroke[j - 1].getPos().y();
					break;
				default:
					data = 0.0;
					break;
				}
			}

			d[k + 4 * (2048 * 0 + j)] = data;
		}
	}

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2048, 8, 0, GL_RGBA, GL_FLOAT, d);

	//qDebug() << "texture created with error = " << glGetError();

	free(d);
}

void Sketcher::setSelections(QList<Selection *> selections)
{
	this->selections = selections;
}

QList<Selection *> &Sketcher::getSelections()
{
	return this->selections;
}

qreal Sketcher::getMaximumDistance(QList<QVariant> parameters)
{
	return this->selections.last()->getMaximumDistance(parameters);
}
