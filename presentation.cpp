#include "presentation.h"
#include <cmath>
#include <QtDebug>

Presentation::Presentation(int width, int height, QWidget *parent) : QWidget(parent)
{
	this->setVisible(false);
	this->resize(width, height);

	this->inputFbo = 0;
	this->depthFbo = 0;
	this->outputFbo = new QGLFramebufferObject(width, height);

	this->anchorsVisible = true;
	this->selectionsVisible = true;
	this->integratedViewsVisible = true;

	glGenTextures(1, &selectionMasksArray);
	glBindTexture(GL_TEXTURE_2D_ARRAY, selectionMasksArray);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	this->activeSelectionsCount = 0;

	this->sketcher = new Sketcher();

	this->interpolationTimer = new QTimer(this);
	connect(this->interpolationTimer, SIGNAL(timeout()), this, SLOT(animate()));

	this->slideTimer = new QTimer(this);
	connect(this->slideTimer, SIGNAL(timeout()), this, SLOT(animateSlide()));

	this->maximizeTimer = new QTimer(this);
	connect(this->maximizeTimer, SIGNAL(timeout()), this, SLOT(animateMaximize()));

	this->activeSelections.clear();
	this->activeIVRects.clear();
	this->activeIntegratedViews.clear();

	this->lastActiveAnchor = 0;
	this->activeAnchor = 0;
	this->hoverAnchor = 0;
	this->thumbnailAnchor = 0;
	this->interpolationAnimation = 1.0;
	this->thumbnailDisplayed = false;
	this->thumbnailDelay = 700;
	this->thumbnailAnchorTimer = new QTimer();
	connect(this->thumbnailAnchorTimer, SIGNAL(timeout()), this, SLOT(displayThumbnail()));

	this->selectionDisplayShader = 0;
	this->stripeShader = 0;

	connect(this->sketcher, SIGNAL(selectionRendered()), this, SLOT(renderSelectionsToActiveAnchorScreenshot()));

	this->integratedViewFbo = 0;

	this->slideAnimation = 0;

	this->integratedViews.clear();

	this->maximizedIntegratedView = false;
	this->currentViewIndex = -1;

	this->flagViewChanged = false;
	this->ivInvalid = true;
	this->animationWanted = false;
	this->autoActivateSelections = false;
	this->autoActivateSnapshots = false;

	this->anchors2D = false;
}

Presentation::~Presentation()
{
	delete this->sketcher;
}

void Presentation::reset()
{
	this->activeIntegratedViews.clear();
	this->activeSelections.clear();
	this->activeIVRects.clear();

	this->activeSelectionsCount = 0;

	this->lastActiveAnchor = 0;
	this->activeAnchor = 0;
	this->thumbnailAnchor = 0;

	for (int i = 0; i < this->sketcher->getSelections().count(); i++)
	{
		delete this->sketcher->getSelections()[i];
	}
	this->sketcher->getSelections().clear();

	for (int i = 0; i < this->anchors.count(); i++)
	{
		delete this->anchors[i];
	}
	this->anchors.clear();

	emit updated();	
}

void Presentation::setAnchors2D(bool anchors2D)
{
	this->anchors2D = anchors2D;
}

void Presentation::initializeGL()
{

}

bool activeIVRectsLessThan(const Presentation::ActiveRect &s1, const Presentation::ActiveRect &s2)
{
	return s1.index < s2.index;
}

bool ivLessThan(IntegratedView *s1, IntegratedView *s2)
{
	return s1->getId() < s2->getId();
}

void Presentation::prepareIntegratedViewFbo(qreal animation, int position)
{
	//animation goes from -1 to 1
	//-1 is the same as if currentViewIndex was decreased
	//1 is the same as if currentViewIndex was increased
	//0 is no animation

	float animationR = 0.0;

	if (animation != 0)
	{
		animation = pow(qAbs(animation), 0.5) * (animation / qAbs(animation));
		animationR = pow(qAbs(animation), 2.0) * (animation / qAbs(animation));
	}


	//index of the active integrated view
	int viewIndex = this->currentViewIndex;

	//horizontal strip of integrated views
	horizontal = position == StripeTop || position == StripeBottom;
	//true = one view in the middle, false = two views in the middle
	inpair = true;

	//spacing between integrated views
	spacing = 10.0;
	//margin for the blurring
	margin = 10.0;
	//bounding box of one integrated view
	boundingSizeX = qMin(this->outputFbo->width() / 3.5, this->outputFbo->height() / 3.5);
	boundingSizeY = qMin(this->outputFbo->width() / 3.5, this->outputFbo->height() / 3.5);


	//center of the strip
	if (horizontal)
	{
		integratedViewX = this->outputFbo->width() / 2.0;
		integratedViewY = position == StripeTop ? boundingSizeY / 2 + spacing : this->outputFbo->height() - boundingSizeY / 2 - spacing;
	}
	else
	{
		integratedViewX = position == StripeLeft ? boundingSizeX / 2 + spacing : this->outputFbo->width() - boundingSizeX / 2 - spacing;
		integratedViewY = this->outputFbo->height() / 2.0;
	}

	//length of the strip will be visibleLength + 2 * fadingLength
	//visibleLength = (horizontal ? this->outputFbo->width() / 2.0 : this->outputFbo->height() / 2.0);
	//fadingLength = visibleLength / 2.0;
	visibleLength = (horizontal ? this->outputFbo->width() / 2.5 : this->outputFbo->height() / 2.5);
	fadingLength = visibleLength / 1.5;



	stripeLength = visibleLength + 2 * fadingLength;
	stripeWidth = horizontal ? stripeLength : boundingSizeX + margin * 2;
	stripeHeight = horizontal ? boundingSizeY + margin * 2 : stripeLength;


	//create new fbo for the integrated views
	/*if (this->integratedViewFbo != 0)
		delete this->integratedViewFbo;*/
	if (this->integratedViewFbo == 0)
		this->integratedViewFbo = new QGLFramebufferObject((int) stripeWidth, (int) stripeHeight);


	//store screen space sizes of all integrated views
	sizes.clear();
	for (int i = 0; i < this->activeIntegratedViews.count(); i++)
	{
		qreal w = 0;
		qreal h = 0;

		QVector2D ratio = this->activeIntegratedViews[i]->aspectRatio();

		if (boundingSizeX / boundingSizeY > ratio.x() / ratio.y())
		{
			w = (ratio.x() / ratio.y()) * boundingSizeY;
			h = boundingSizeY;
		}
		else
		{
			w = boundingSizeX;
			h = boundingSizeX / (ratio.x() / ratio.y());
		}

		sizes << QSizeF(w, h);
	}

	this->integratedViewFbo->bind();
	glClearColor(1.0, 1.0, 1.0, 0.25);
	glClear(GL_COLOR_BUFFER_BIT);


	qreal animationLength;
	QRectF displayRect;

	if (animation == 0.0)
		animationLength = 0.0;
	else if (animation < 0.0)
	{
		if (this->currentViewIndex > 0)
		{
			if (horizontal)
			{
				if (inpair)
				{
					animationLength = sizes[this->currentViewIndex].width() / 2.0 +
									  sizes[this->currentViewIndex - 1].width() / 2.0 +
									  spacing;
				}
				else
				{
					animationLength = sizes[this->currentViewIndex].width() + spacing;
				}
			}
			else
			{
				if (inpair)
				{
					animationLength = sizes[this->currentViewIndex].height() / 2.0 +
									  sizes[this->currentViewIndex - 1].height() / 2.0 +
									  spacing;
				}
				else
				{
					animationLength = sizes[this->currentViewIndex].height() + spacing;
				}
			}
		}
		else
		{
			animationLength = 0.0;
		}
	}
	else if (animation > 0.0)
	{
		if (this->currentViewIndex < this->activeIntegratedViews.count() - 1)
		{
			if (horizontal)
			{
				if (inpair)
				{
					animationLength = sizes[this->currentViewIndex].width() / 2.0 +
									  sizes[this->currentViewIndex + 1].width() / 2.0 +
									  spacing;
				}
				else
				{
					animationLength = sizes[this->currentViewIndex + 1].width() + spacing;
				}
			}
			else
			{
				if (inpair)
				{
					animationLength = sizes[this->currentViewIndex].height() / 2.0 +
									  sizes[this->currentViewIndex + 1].height() / 2.0 +
									  spacing;
				}
				else
				{
					animationLength = sizes[this->currentViewIndex + 1].height() + spacing;
				}
			}
		}
		else
		{
			animationLength = 0.0;
		}
	}

	glMatrixMode(GL_PROJECTION);
	//glPushMatrix();
	glLoadIdentity();
	glOrtho(0, stripeWidth - 1, stripeHeight - 1, 0, +1, -1);
	glMatrixMode(GL_MODELVIEW);
	//glPushMatrix();
	glViewport(0, 0, (int) stripeWidth, (int) stripeHeight);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	this->activeIVRects.clear();
	ActiveRect activeRect;

	for (int i = 0; i < 2; i++)
	{
		viewIndex = this->currentViewIndex;

		if (viewIndex < 0 || viewIndex >= this->activeIntegratedViews.count())
			break;

		if (horizontal)
		{
			if (inpair)
			{
				displayRect.setLeft(stripeLength / 2.0 - sizes[viewIndex].width() / 2.0 - spacing + spacing * i - animationLength * animation);
			}
			else
			{
				displayRect.setLeft(stripeLength / 2.0 - sizes[viewIndex].width() - spacing / 2.0 - spacing + spacing * i - animationLength * animation);
			}
			displayRect.setWidth(0);
		}
		else
		{
			if (inpair)
			{
				displayRect.setTop(stripeLength / 2.0 - sizes[viewIndex].height() / 2.0 - spacing + spacing * i - animationLength * animation);
			}
			else
			{
				displayRect.setTop(stripeLength / 2.0 - sizes[viewIndex].height() - spacing / 2.0 - spacing + spacing * i - animationLength * animation);
			}
			displayRect.setHeight(0);
		}

		viewIndex -= i;

		while ((i == 0 && viewIndex < this->activeIntegratedViews.count() && displayRect.left() < stripeLength)
			   ||
			   (i == 1 && viewIndex >= 0 && displayRect.right() > 0))
		{
			//set parameters of the display rectangle, other than the left edge, which is already set up

			if (horizontal)
			{
				displayRect.setLeft((i ==  0 ? (displayRect.left() + displayRect.width() + spacing) :
											  (displayRect.left() - sizes[viewIndex].width() - spacing))
									);
				displayRect.setTop(stripeHeight / 2.0 - sizes[viewIndex].height() / 2.0);
			}
			else
			{
				displayRect.setLeft(stripeWidth / 2.0 - sizes[viewIndex].width() / 2.0);
				displayRect.setTop((i ==  0 ? (displayRect.top() + displayRect.height() + spacing) :
											  (displayRect.top() - sizes[viewIndex].height() - spacing))
									);
			}

			displayRect.setWidth(sizes[viewIndex].width());
			displayRect.setHeight(sizes[viewIndex].height());


			//store displayed rect so it can be used for interaction
			activeRect.index = viewIndex;
			activeRect.integratedView = this->activeIntegratedViews.at(viewIndex);
			activeRect.rect = displayRect;
			activeRect.rect.moveLeft(activeRect.rect.left() + integratedViewX - stripeWidth / 2);
			activeRect.rect.moveTop(activeRect.rect.top() + integratedViewY - stripeHeight / 2);
			this->activeIVRects << activeRect;

			if (animationR == 0.0)
			{
				if (viewIndex == this->currentViewIndex)
					glColor4f(1.0, 1.0, 1.0, 1.0);
				else
					glColor4f(1.0, 1.0, 1.0, 0.4);
			}
			else
			{
				if (viewIndex == this->nextViewIndex)
					glColor4f(1.0, 1.0, 1.0, PRSUtil::mix(0.4, 1.0, (animationR > 0.0 ? animationR : -animationR)));
				else if (viewIndex == this->currentViewIndex)
					glColor4f(1.0, 1.0, 1.0, PRSUtil::mix(1.0, 0.4, (animationR > 0.0 ? animationR : -animationR)));
				else
					glColor4f(1.0, 1.0, 1.0, 0.4);
			}

			glBindTexture(GL_TEXTURE_2D, this->activeIntegratedViews.at(viewIndex)->getFbo()->texture());
			glBegin(GL_QUADS);
				glTexCoord2f(0.0, 1.0); glVertex2f(displayRect.left(), displayRect.top());
				glTexCoord2f(0.0, 0.0); glVertex2f(displayRect.left(), displayRect.bottom());
				glTexCoord2f(1.0, 0.0); glVertex2f(displayRect.right(), displayRect.bottom());
				glTexCoord2f(1.0, 1.0); glVertex2f(displayRect.right(), displayRect.top());
			glEnd();

			viewIndex += 1 - i * 2;
		}
	}

	qSort(this->activeIVRects.begin(), this->activeIVRects.end(), activeIVRectsLessThan);

	/*glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);*/

	this->integratedViewFbo->release();
}

void Presentation::clearSelectionDisplayShaders()
{
	this->selectionDisplayShaders.clear();
}

void Presentation::addSelectionDisplayShader(QGLShaderProgram *shader)
{
	this->selectionDisplayShaders << shader;
	this->selectionDisplayShader = shader;
}

void Presentation::setSelectionDisplayShader(int index)
{
	this->selectionDisplayShader = this->selectionDisplayShaders[index];
}

void Presentation::setStripeShader(QGLShaderProgram *shader)
{
	this->stripeShader = shader;
}

void Presentation::resizeFbo(int width, int height)
{
	this->inputFbo = 0;
	if (this->outputFbo != 0 && width > 0 && height > 0)
	{
		delete this->outputFbo;
		this->outputFbo = new QGLFramebufferObject(width, height);

		if (this->integratedViewFbo != 0)
		{
			delete this->integratedViewFbo;
			this->integratedViewFbo = 0;
		}

		//qDebug() << "after INTERNAL resizing, error = " << glGetError();
	}

	if (this->maximizedIntegratedView && this->currentViewIndex >= 0 && this->currentViewIndex < this->activeIntegratedViews.count())
		this->maximizedRect = PRSUtil::fitToSize(this->activeIntegratedViews[this->currentViewIndex]->size(), this->outputFbo->size()).toRect();
}

void Presentation::setInputFbo(QGLFramebufferObject *inputFbo)
{
	this->inputFbo = inputFbo;
}

void Presentation::setDepthFbo(QGLFramebufferObject *depthFbo)
{
	this->depthFbo = depthFbo;
}

QGLFramebufferObject *Presentation::getOutputFbo()
{
	return this->outputFbo;
}

bool anchorsCompare(Anchor *a0, Anchor *a1)
{
	return a0->getZ() < a1->getZ();
}

void Presentation::copyFbo(QGLFramebufferObject *from, QGLFramebufferObject *to)
{
	//qDebug() << "copying" << from->width() << from->height() << "to this one:" << to->width() << to->height();
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	to->bind();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, +1.0, -1.0, +1.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, to->width(), to->height());
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	
	glBindTexture(GL_TEXTURE_2D, from->texture());
	glBegin(GL_QUADS);
		glTexCoord2f(1.0, 0.0); glVertex2f( 1.0, -1.0);
		glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
		glTexCoord2f(0.0, 1.0); glVertex2f(-1.0,  1.0);
		glTexCoord2f(1.0, 1.0); glVertex2f( 1.0,  1.0);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	to->release();

	glPopClientAttrib();
	glPopAttrib();
}

QGLFramebufferObject *Presentation::imageToFbo(QImage image)
{
	QGLFramebufferObject *fbo = new QGLFramebufferObject(image.size());

	glBindTexture(GL_TEXTURE_2D, fbo->texture());
	//glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER,GL_LINEAR);

	QPainter painter(fbo);
	painter.drawImage(QRect(QPoint(0, 0), image.size()), image);
	painter.end();

	painter.drawLine(0,0,image.size().width(), image.size().height());

	return fbo;
}

void Presentation::renderIntegratedViews()
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, this->outputFbo->width() - 1.0, this->outputFbo->height() - 1.0, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, this->outputFbo->width(), this->outputFbo->height());
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	qreal length = visibleLength + 2 * fadingLength;
	
	glBindTexture(GL_TEXTURE_2D, this->integratedViewFbo->texture());

	if (this->stripeShader != 0 && this->stripeShader->isLinked())
		this->stripeShader->bind();

	glBegin(GL_QUAD_STRIP);
	if (this->horizontal)
	{
		glColor4f(1.0, 1.0, 1.0, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex2f(integratedViewX - visibleLength / 2.0 - fadingLength, integratedViewY - boundingSizeY / 2.0 - margin);
		glTexCoord2f(0.0, 0.0); glVertex2f(integratedViewX - visibleLength / 2.0 - fadingLength, integratedViewY + boundingSizeY / 2.0 + margin);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glTexCoord2f(fadingLength / length, 1.0); glVertex2f(integratedViewX - visibleLength / 2.0, integratedViewY - boundingSizeY / 2.0 - margin);
		glTexCoord2f(fadingLength / length, 0.0); glVertex2f(integratedViewX - visibleLength / 2.0, integratedViewY + boundingSizeY / 2.0 + margin);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glTexCoord2f(1.0 - fadingLength / length, 1.0); glVertex2f(integratedViewX + visibleLength / 2.0, integratedViewY - boundingSizeY / 2.0 - margin);
		glTexCoord2f(1.0 - fadingLength / length, 0.0); glVertex2f(integratedViewX + visibleLength / 2.0, integratedViewY + boundingSizeY / 2.0 +margin);

		glColor4f(1.0, 1.0, 1.0, 0.0);
		glTexCoord2f(1.0, 1.0); glVertex2f(integratedViewX + visibleLength / 2.0 + fadingLength, integratedViewY - boundingSizeY / 2.0 - margin);
		glTexCoord2f(1.0, 0.0); glVertex2f(integratedViewX + visibleLength / 2.0 + fadingLength, integratedViewY + boundingSizeY / 2.0 + margin);
	}
	else
	{
		glColor4f(1.0, 1.0, 1.0, 0.0);
		glTexCoord2f(0.0, 1.0); glVertex2f(integratedViewX - boundingSizeX / 2.0 - margin, integratedViewY - visibleLength / 2.0 - fadingLength);
		glTexCoord2f(1.0, 1.0); glVertex2f(integratedViewX + boundingSizeX / 2.0 + margin, integratedViewY - visibleLength / 2.0 - fadingLength);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glTexCoord2f(0.0, 1.0 - fadingLength / length); glVertex2f(integratedViewX - boundingSizeX / 2.0 - margin, integratedViewY - visibleLength / 2.0);
		glTexCoord2f(1.0, 1.0 - fadingLength / length); glVertex2f(integratedViewX + boundingSizeX / 2.0 + margin, integratedViewY - visibleLength / 2.0);

		glColor4f(1.0, 1.0, 1.0, 1.0);
		glTexCoord2f(0.0, fadingLength / length); glVertex2f(integratedViewX - boundingSizeX / 2.0 - margin, integratedViewY + visibleLength / 2.0);
		glTexCoord2f(1.0, fadingLength / length); glVertex2f(integratedViewX + boundingSizeX / 2.0 + margin, integratedViewY + visibleLength / 2.0);

		glColor4f(1.0, 1.0, 1.0, 0.0);
		glTexCoord2f(0.0, 0.0); glVertex2f(integratedViewX - boundingSizeX / 2.0 - margin, integratedViewY + visibleLength / 2.0 + fadingLength);
		glTexCoord2f(1.0, 0.0); glVertex2f(integratedViewX + boundingSizeX / 2.0 + margin, integratedViewY + visibleLength / 2.0 + fadingLength);
	}
	glEnd();

	if (this->stripeShader != 0 && this->stripeShader->isLinked())
		this->stripeShader->release();

	if (this->maximizeTimer->isActive() || this->maximizedIntegratedView)
	{
		/*qDebug() << "view index = " << this->currentViewIndex;
		for (int i = 0; i < this->activeIVRects.count(); i++)
		{
			qDebug() << i << " ! " << this->activeIVRects[i].index;
		}*/

		float maxAnim = pow(this->maximizeAnimation, 0.42);
		float maxAnimR = pow(this->maximizeAnimation, 3.0);
		QRect maximizingRect = PRSUtil::mix(this->activeIVRects[this->currentViewIndex - this->activeIVRects[0].index].rect.toRect(),
											this->maximizedRect,
											maxAnim);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glDisable(GL_TEXTURE_2D);
		glColor4f(1.0, 1.0, 1.0, 0.6 * maxAnimR);
		glBegin(GL_QUADS);
			glVertex2f(0, 0);
			glVertex2f(this->outputFbo->width(), 0);
			glVertex2f(this->outputFbo->width(), this->outputFbo->height());
			glVertex2f(0, this->outputFbo->height());
		glEnd();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, this->activeIntegratedViews[this->currentViewIndex]->getFbo()->texture());
		glColor4f(1.0, 1.0, 1.0, maxAnim);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2f(maximizingRect.left(),  maximizingRect.top());
			glTexCoord2f(1.0, 1.0); glVertex2f(maximizingRect.right(), maximizingRect.top());
			glTexCoord2f(1.0, 0.0); glVertex2f(maximizingRect.right(), maximizingRect.bottom());
			glTexCoord2f(0.0, 0.0); glVertex2f(maximizingRect.left(),  maximizingRect.bottom());
		glEnd();

		if (this->slideTimer->isActive())
		{
			glBindTexture(GL_TEXTURE_2D, this->activeIntegratedViews[this->nextViewIndex]->getFbo()->texture());
			glColor4f(1.0, 1.0, 1.0, maxAnim * qAbs(this->slideAnimation));
			glBegin(GL_QUADS);
				glTexCoord2f(0.0, 1.0); glVertex2f(maximizingRect.left(),  maximizingRect.top());
				glTexCoord2f(1.0, 1.0); glVertex2f(maximizingRect.right(), maximizingRect.top());
				glTexCoord2f(1.0, 0.0); glVertex2f(maximizingRect.right(), maximizingRect.bottom());
				glTexCoord2f(0.0, 0.0); glVertex2f(maximizingRect.left(),  maximizingRect.bottom());
			glEnd();
		}
	}

	glPopClientAttrib();
	glPopAttrib();
}

void Presentation::renderSelections(Anchor *anchor, int x, int y, int width, int height, bool all, bool preview)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, +1.0, -1.0, +1.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(x, y, width, height);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);

	if (this->selectionDisplayShader != 0 && this->selectionDisplayShader->isLinked())
		this->selectionDisplayShader->bind();

	this->selectionDisplayShader->setUniformValue("size", QSizeF(width, height));
	this->selectionDisplayShader->setUniformValue("preview", 1);
	this->selectionDisplayShader->setUniformValue("showPreview", (preview ? 1 : 0));

	for (int i = 0; i < this->sketcher->getSelections().count(); i++)
	{
		int show = 0;
		//does the selection belong to the selected anchor?
		for (int j = 0; j < anchor->getSelections().count(); j++)
		{
			if (this->sketcher->getSelections().at(i) == anchor->getSelections().at(j))
			{
				show = 1;
				break;
			}
		}

		//or in case it doesn't, does it belong to the last selection before the interpolation animation?
		if (show == 0 && this->lastActiveAnchor != 0 && this->interpolationTimer->isActive())
		{
			for (int j = 0; j < this->lastActiveAnchor->getSelections().count(); j++)
			{
				if (this->sketcher->getSelections().at(i) == this->lastActiveAnchor->getSelections().at(j))
				{
					show = 2;
					break;
				}
			}
		}

		bool selectionIsFocused = this->currentViewIndex >= 0 &&
								this->currentViewIndex < this->activeIntegratedViews.count() &&
								this->sketcher->getSelections().at(i)->getIntegratedViews().contains(this->activeIntegratedViews[this->currentViewIndex]);

		bool selectionWillBeFocused = this->nextViewIndex >= 0 &&
								   this->nextViewIndex < this->activeIntegratedViews.count() &&
								   this->sketcher->getSelections().at(i)->getIntegratedViews().contains(this->activeIntegratedViews[this->nextViewIndex]);

		qreal selectionAlpha = (this->activeSelections.isEmpty() || all) ?
									0.5 : //0.2 :
									(this->activeSelections.contains(this->sketcher->getSelections().at(i)) ?
										0.5 : 0.1);

		float animation = this->slideAnimation;
		if (animation != 0)
		{
			animation = pow(qAbs(animation), 2.0f) * (animation / qAbs(animation));
		}

		if (animation == 0.0)
		{
			if (selectionIsFocused)
				selectionAlpha = 1.0;
		}
		else
		{
			if (selectionIsFocused)
				selectionAlpha = PRSUtil::mix(1.0, 0.5, (animation > 0.0 ? animation : -animation));
			else if (selectionWillBeFocused)
				selectionAlpha = PRSUtil::mix(0.5, 1.0, (animation > 0.0 ? animation : -animation));
		}
		


		switch (show)
		{
		case 0:
			continue;
			break;
		case 1:
			glColor4f(1.0, 1.0, 1.0, this->interpolationAnimation * selectionAlpha);
			break;
		case 2:
			glColor4f(1.0, 1.0, 1.0, (1.0 - this->interpolationAnimation) * selectionAlpha);
			break;
		default:
			continue;
			break;
		}

		((PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, this->sketcher->getSelections().at(i)->getSelectionMask()->texture());
		((PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, anchor->getScreenshot()->texture());
		((PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);

		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2f(-1.0,  1.0);
			glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
			glTexCoord2f(1.0, 0.0); glVertex2f( 1.0, -1.0);
			glTexCoord2f(1.0, 1.0); glVertex2f( 1.0,  1.0);
		glEnd();		
	}

	if (this->selectionDisplayShader != 0 && this->selectionDisplayShader->isLinked())
		this->selectionDisplayShader->release();
	//glUseProgram(0);

	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPopClientAttrib();
	glPopAttrib();
}

void Presentation::displayThumbnail()
{
	this->thumbnailAnchorTimer->stop();
	this->thumbnailDisplayed = true;
	emit this->updated();
}

void Presentation::updateIntegratedViews(PRSShaderEnhancer *shaderEnhancer)
{
	if (!this->animationWanted && !this->ivInvalid)
		return;

	this->ivInvalid = false;


	GLuint *data = shaderEnhancer->getHistogramData();
	QList<int> list;

	emit this->arraysReset();

	int selectionCount = shaderEnhancer->getActiveSelectionsCount();
	int parameterCount = shaderEnhancer->getParameterCount();
	int histogramSize = shaderEnhancer->getHistogramSize();

	QStringList parameterCaptions = shaderEnhancer->getParameterCaptions();

	for (int i = 0; i < selectionCount; i++)
	{
		for (int j = 0; j < parameterCount; j++)
		{
			list.clear();
			for (int k = 0; k < histogramSize; k++)
			{
				list << data[histogramSize * parameterCount * i + histogramSize * j + k];
			}
			emit this->arrayReceived(list, i, parameterCaptions.count() > j ? parameterCaptions[j] : tr("(untitled)"));
		}
	}
	emit this->arraysFinished();


	//shaderEnhancer->clearStorage();

	delete data;

	//emit this->updated();
}

void Presentation::renderClusters(GLfloat *projectionMatrix, GLfloat *modelviewMatrix)
{
}

void Presentation::renderAnchor(Anchor *anchor, bool is2D)
{
	qreal x = anchor->getPosition().x();
	qreal y = anchor->getPosition().y();
	qreal z = anchor->getPosition().z();

	if (!is2D)
	{
		glBegin(GL_LINES);
			glColor4f(1.0, 1.0, 1.0, 0.0);
			//glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0.0, 140.0, 0.0);
			glColor4f(1.0, 1.0, 1.0, 0.3);
			glVertex3f(x, y, z);
		glEnd();
	}

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glPointSize(17.0);
	glBegin(GL_POINTS);
		glVertex3f(x, y, z);
	glEnd();

	/*if (this->thumbnailAnchor == this->anchors[i])
		glColor4f(1.0, 0.9, 0.8, 1.0);
	else
		glColor4f(1.0, 1.0, 1.0, this->anchors2D ? 1.0 : this->anchors[i]->getAlpha());*/

	if (this->thumbnailAnchor == anchor)
		glColor4f(0.3, 0.3, 0.3, 1.0);
	else
		glColor4f(0.0, 0.0, 0.0, is2D ? 1.0 : anchor->getAlpha());

	glPointSize(14.0);
	glBegin(GL_POINTS);
		glVertex3f(x, y, z);
	glEnd();
}

QList<int> Presentation::regionQuery(int p, float eps)
{
	QList<int> region;
	for (int i = 0; i < this->anchors.count(); i++)
	{
		if (i == p)
		{
			region << p;
			continue;
		}

		if (QVector2D(this->anchors[i]->getProjectedPosition() - this->anchors[p]->getProjectedPosition()).length() <= eps)
			region << i;
	}

	return region;
}

void Presentation::clusterAnchors()
{
	QList<Anchor *> unvisited;

	for (int i = 0; i < this->anchors.count(); i++)
	{
		unvisited << this->anchors[i];
		this->anchors[i]->setCluster(-1);
	}

	float eps = 5.0;
	int minPts = 2;
	int c = 0;


	for (int i = 0; i < this->clusters.count(); i++)
	{
		this->clusters[i].clear();
	}
	this->clusters.clear();

	for (int i = 0; i < this->anchors.count(); i++)
	{
		//qDebug() << "clustering anchor" << i;
		int ui = unvisited.indexOf(this->anchors[i]);
		if (ui >= 0)
		{
			unvisited.removeAt(ui);
			QList<int> neighborPts = regionQuery(i, eps);
			//qDebug() << i << "has these neighbors:" << neighborPts;
			if (neighborPts.count() < minPts)
			{
				this->anchors[i]->setCluster(-2);
				this->anchors[i]->setScreenPosition(this->anchors[i]->getProjectedPosition());
			}
			else
			{
				//qDebug() << i << "is clustered as" << c;
				this->anchors[i]->setCluster(c);
				this->clusters << QList<int>();
				this->clusters[c] << i;

				for (int j = 0; j < neighborPts.count(); j++)
				{
					int p = neighborPts[j];
					int ui = unvisited.indexOf(this->anchors[p]);
					if (ui >= 0)
					{
						unvisited.removeAt(ui);
						QList<int> neighborPts0 = regionQuery(p, eps);
						if (neighborPts0.count() >= minPts);
						{
							neighborPts.append(neighborPts0);
						}
					}
					if (this->anchors[p]->getCluster() < 0)
					{
						//qDebug() << p << "is clustered as" << c;
						this->anchors[p]->setCluster(c);
						this->clusters[c] << p;
					}
				}

				c++;
			}
		}
	}

	//qDebug() << clusters;

	for (int i = 0; i < this->clusters.count(); i++)
	{
		//qDebug() << "!!!!!cluster" << i << "has" << this->clusters[i].count() << "items";

		QVector2D center = QVector2D(0.0, 0.0);
		for (int j = 0; j < this->clusters[i].count(); j++)
		{
			center += this->anchors[this->clusters[i][j]]->getProjectedPosition();
		}
		center /= (qreal) this->clusters[i].count();

		//qDebug() << "it's center is" << center;

		for (int j = 0; j < this->clusters[i].count(); j++)
		{
			qreal rad = 30.0;
			qreal x, y;
			qreal zo = (qreal) j / (qreal) (this->clusters[i].count());

			x = center.x() + rad * cos(zo * 2.0 * PI);
			y = center.y() + rad * sin(zo * 2.0 * PI);

			this->anchors[this->clusters[i][j]]->setScreenCenterPosition(center);
			this->anchors[this->clusters[i][j]]->setScreenPosition(QVector2D(x, y));
		}
	}
}

void Presentation::setup2DMatrix()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, this->outputFbo->width() - 1.0, 0.0, this->outputFbo->height() - 1.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, this->outputFbo->size().width(), this->outputFbo->size().height());
	glLoadIdentity();
}

void Presentation::setup3DMatrix(GLfloat *projectionMatrix, GLfloat *modelviewMatrix)
{
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projectionMatrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(modelviewMatrix);
}

void Presentation::render()
{
	GLfloat projectionMatrix[16];
	glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix);
	GLfloat modelviewMatrix[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelviewMatrix);

	this->render(projectionMatrix, modelviewMatrix);
}

void Presentation::render(GLfloat *projectionMatrix, GLfloat *modelviewMatrix)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	if (!this->activeIntegratedViews.isEmpty())
	{
		this->prepareIntegratedViewFbo(this->slideAnimation, this->currentViewPosition);
	}

	this->outputFbo->bind();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0, +1.0, -1.0, +1.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, this->outputFbo->size().width(), this->outputFbo->size().height());
	glLoadIdentity();

	glClearColor(1.0, 1.0, 0.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_LIGHTING);

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glBindTexture(GL_TEXTURE_2D, this->inputFbo->texture());
	glBegin(GL_QUADS);
		glTexCoord2f(1.0, 0.0); glVertex2f( 1.0, -1.0);
		glTexCoord2f(0.0, 0.0); glVertex2f(-1.0, -1.0);
		glTexCoord2f(0.0, 1.0); glVertex2f(-1.0,  1.0);
		glTexCoord2f(1.0, 1.0); glVertex2f( 1.0,  1.0);
	glEnd();

	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_DEPTH_TEST);

	//render anchors
	if (this->anchorsVisible)
	{
		if (this->anchors2D)
		{
			this->setup2DMatrix();
		}
		else
		{
			this->setup3DMatrix(projectionMatrix, modelviewMatrix);
		}


		if (!this->anchors2D)
		{
			//sort labels by z coordinate in the eye space
			QMatrix4x4 matrix = PRSUtil::columnMajorToMatrix(modelviewMatrix);

			for (int i = 0; i < this->anchors.count(); i++)
				this->anchors[i]->calculateZ(matrix);
			qStableSort(this->anchors.begin(), this->anchors.end(), anchorsCompare);
		}

		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
		glGetIntegerv(GL_VIEWPORT, viewport);
		for (int i = 0; i < this->anchors.count(); i++)
		{
			GLdouble wx, wy, wz;
			gluProject(this->anchors[i]->getPosition().x(), this->anchors[i]->getPosition().y(), this->anchors[i]->getPosition().z(),
					   modelview, projection, viewport,
					   &wx, &wy, &wz);

			this->anchors[i]->setProjectedPosition(QVector2D(wx, wy));
		}

		this->clusterAnchors();

		glEnable(GL_POINT_SMOOTH);
		glLineWidth(1.0);
		glEnable(GL_LINE_SMOOTH);

		this->renderClusters(projectionMatrix, modelviewMatrix);

		for (int i = 0; i < this->anchors.count(); i++)
		{
			if (this->anchors2D)
			{
				this->setup2DMatrix();
			}
			else
			{
				this->setup3DMatrix(projectionMatrix, modelviewMatrix);
			}

			renderAnchor(this->anchors[i], this->anchors2D);
		}
	}
	
	bool enableThumbnail = true;
	//render selections
	/*if (this->hoverAnchor != 0)
	{
		float diff = 0.0;
		for (int i = 0; i < 16; i++)
		{
			diff += qAbs(this->currentMatrix.data()[i] - this->hoverAnchor->getNoScalingMatrix().data()[i]);
		}

		//if (this->hoverAnchor->getNoScalingMatrix() == this->currentMatrix)
		if (diff < 0.5)
		{
			this->renderSelections(this->hoverAnchor, 0, 0, this->outputFbo->width(), this->outputFbo->height(), false, true);
			enableThumbnail = false;
		}
	}*/

	if (this->activeAnchor != 0)
	{
		if (this->selectionsVisible)
			this->renderSelections(this->activeAnchor, 0, 0, this->outputFbo->width(), this->outputFbo->height());
	
		if (this->activeIntegratedViews.count() > 0 && this->integratedViewsVisible)
		{
			this->renderIntegratedViews();
		}
	}

	/*glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, this->outputFbo->size().width() - 1.0, this->outputFbo->size().height() - 1.0, 0.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, this->outputFbo->size().width(), this->outputFbo->size().height());
	glLoadIdentity();

	glDisable(GL_TEXTURE_2D);
	glColor4f(1.0, 0.0, 0.0, 0.5);
	glBegin(GL_QUADS);
	for (int i = 0; i < this->activeIVRects.count(); i++)
	{
		glVertex2f(this->activeIVRects[i].rect.left(), this->activeIVRects[i].rect.top());
		glVertex2f(this->activeIVRects[i].rect.left(), this->activeIVRects[i].rect.bottom());
		glVertex2f(this->activeIVRects[i].rect.right(), this->activeIVRects[i].rect.bottom());
		glVertex2f(this->activeIVRects[i].rect.right(), this->activeIVRects[i].rect.top());
	}
	glEnd();*/



	//render thumbnail of the mouse hovered anchor
	if (enableThumbnail && this->thumbnailDisplayed && this->thumbnailAnchor != 0 && this->thumbnailAnchor->getScreenshot() != 0)
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, this->outputFbo->size().width() - 1.0, 0.0, this->outputFbo->size().height() - 1.0, -1.0, +1.0);
		glMatrixMode(GL_MODELVIEW);
		glViewport(0, 0, this->outputFbo->size().width(), this->outputFbo->size().height());
		glLoadIdentity();

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);

		GLint viewport[4];
		GLdouble modelview[16];
		GLdouble projection[16];

		glGetIntegerv(GL_VIEWPORT, viewport);

		qreal winX, winY, winZ;
		qreal objX, objY, objZ;

		objX = this->thumbnailAnchor->getPosition().x();
		objY = this->thumbnailAnchor->getPosition().y();
		objZ = this->thumbnailAnchor->getPosition().z();


		if (!this->anchors2D)
		{
			for (int i = 0; i < 16; i++)
			{
				modelview[i] = (GLdouble) modelviewMatrix[i];
				projection[i] = (GLdouble) projectionMatrix[i];
			}

			gluProject(objX, objY, objZ, modelview, projection, viewport, &winX, &winY, &winZ);
		}
		else
		{
			winX = objX;
			winY = objY;
			winZ = objZ;
		}

		qreal w = 160;
		qreal h = 160;

		if (this->thumbnailAnchor->getScreenshot()->width() > this->thumbnailAnchor->getScreenshot()->height())
		{
			w = 160;
			h = this->thumbnailAnchor->getScreenshot()->height() * 160 / this->thumbnailAnchor->getScreenshot()->width();
		}
		else
		{
			w = this->thumbnailAnchor->getScreenshot()->width() * 160 / this->thumbnailAnchor->getScreenshot()->height();
			h = 160;
		}

		if (winX < w / 2)
			winX = w / 2;
		if (winX > this->outputFbo->size().width() - w / 2)
			winX = this->outputFbo->size().width() - w / 2;
		if (winY < -10)
			winY = -10;
		if (winY > this->outputFbo->size().height() - h - 10)
			winY = this->outputFbo->size().height() - h - 10;

		glDisable(GL_LIGHTING);

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, this->thumbnailAnchor->getScreenshot()->texture());
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0, 1.0); glVertex2f(winX - w / 2, winY + h + 10);
			glTexCoord2f(1.0, 1.0); glVertex2f(winX + w / 2, winY + h + 10);
			glTexCoord2f(1.0, 0.0); glVertex2f(winX + w / 2, winY + 10);
			glTexCoord2f(0.0, 0.0); glVertex2f(winX - w / 2, winY + 10);
		glEnd();

		glDisable(GL_TEXTURE_2D);
		glLineWidth(1.0);
		glColor4f(1.0, 1.0, 1.0, 0.7);
		glBegin(GL_LINE_STRIP);
			glVertex2f(winX - w / 2, winY + h + 10);
			glVertex2f(winX + w / 2, winY + h + 10);
			glVertex2f(winX + w / 2, winY + 10);
			glVertex2f(winX - w / 2, winY + 10);
			glVertex2f(winX - w / 2, winY + h + 10);
		glEnd();

		//"drop shadow"
		{
			qreal size = 5.0;
			qreal opacity = 0.2;
			glBegin(GL_QUADS);
				//top
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX - w / 2, winY + h + 10 + size);
				glVertex2f(winX + w / 2, winY + h + 10 + size);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX + w / 2, winY + h + 10);
				glVertex2f(winX - w / 2, winY + h + 10);

				//bottom
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX - w / 2, winY + 10 - size);
				glVertex2f(winX + w / 2, winY + 10 - size);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX + w / 2, winY + 10);
				glVertex2f(winX - w / 2, winY + 10);

				//left
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX - w / 2 - size, winY + h + 10);
				glVertex2f(winX - w / 2 - size, winY + 10);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX - w / 2, winY + 10);
				glVertex2f(winX - w / 2, winY + h + 10);

				//right
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX + w / 2 + size, winY + h + 10);
				glVertex2f(winX + w / 2 + size, winY + 10);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX + w / 2, winY + 10);
				glVertex2f(winX + w / 2, winY + h + 10);

				//topleft
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX - w / 2, winY + h + 10 + size);
				glVertex2f(winX - w / 2 - size, winY + h + 10 + size);
				glVertex2f(winX - w / 2 - size, winY + h + 10);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX - w / 2, winY + h + 10);

				//topright
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX + w / 2, winY + h + 10 + size);
				glVertex2f(winX + w / 2 + size, winY + h + 10 + size);
				glVertex2f(winX + w / 2 + size, winY + h + 10);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX + w / 2, winY + h + 10);

				//bottomleft
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX - w / 2, winY + 10 - size);
				glVertex2f(winX - w / 2 - size, winY + 10 - size);
				glVertex2f(winX - w / 2 - size, winY + 10);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX - w / 2, winY + 10);

				//bottomright
				glColor4f(0.0, 0.0, 0.0, 0.0);
				glVertex2f(winX + w / 2, winY + 10 - size);
				glVertex2f(winX + w / 2 + size, winY + 10 - size);
				glVertex2f(winX + w / 2 + size, winY + 10);
				glColor4f(0.0, 0.0, 0.0, opacity);
				glVertex2f(winX + w / 2, winY + 10);
			glEnd();
		}
	}

	this->outputFbo->release();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopClientAttrib();
	glPopAttrib();
}

Anchor* Presentation::addAnchor(QMatrix4x4 matrix, QMatrix4x4 noScalingMatrix, QList<QVariant> parameters, QList<QString> parameterNames)
{
	QGLFramebufferObject *baseScreenshot = new QGLFramebufferObject(this->inputFbo->width(), this->inputFbo->height());
	QGLFramebufferObject *screenshot = new QGLFramebufferObject(this->inputFbo->width(), this->inputFbo->height());

	this->copyFbo(this->inputFbo, baseScreenshot);
	this->copyFbo(this->inputFbo, screenshot);

	Anchor *anchor = new Anchor(this);
	anchor->setMatrix(matrix);
	anchor->setNoScalingMatrix(noScalingMatrix);
	anchor->setParameters(parameters, parameterNames);
	anchor->setBaseScreenshot(baseScreenshot);
	anchor->setScreenshot(screenshot);

	this->anchors << anchor;

	emit updated();

	return anchor;
}

Anchor* Presentation::addAnchor(QVector3D position, QList<QVariant> parameters, QList<QString> parameterNames)
{
	QGLFramebufferObject *baseScreenshot = new QGLFramebufferObject(this->inputFbo->width(), this->inputFbo->height());
	QGLFramebufferObject *screenshot = new QGLFramebufferObject(this->inputFbo->width(), this->inputFbo->height());

	this->copyFbo(this->inputFbo, baseScreenshot);
	this->copyFbo(this->inputFbo, screenshot);

	Anchor *anchor = new Anchor(this);
	anchor->setPosition(position);
	anchor->setParameters(parameters, parameterNames);
	anchor->setBaseScreenshot(baseScreenshot);
	anchor->setScreenshot(screenshot);

	this->anchors << anchor;

	emit updated();

	return anchor;
}

Sketcher *Presentation::getSketcher()
{
	return this->sketcher;
}

Selection *Presentation::createSelection(QMatrix4x4 matrix, QMatrix4x4 noScalingMatrix, QList<QVariant> parameters, QList<QString> parameterNames)
{
	Selection *selection = this->sketcher->createSelection(this->outputFbo->width(), this->outputFbo->height());

	if (this->activeAnchor == 0)
	{
		this->lastActiveAnchor = this->activeAnchor;
		this->activeAnchor = this->addAnchor(matrix, noScalingMatrix, parameters, parameterNames);
	}

	this->activeAnchor->addSelection(selection);

	emit updated();

	return selection;
}

QPointF Presentation::invertY(QPointF point)
{
	return QPointF(point.x(), this->outputFbo->height() - point.y());
}

Selection *Presentation::createSelection(QVector3D position, QList<QVariant> parameters, QList<QString> parameterNames)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);
	glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

	Selection *selection = this->sketcher->createSelection(this->outputFbo->width(), this->outputFbo->height());

	if (this->activeAnchor == 0)
	{
		this->lastActiveAnchor = this->activeAnchor;
		this->activeAnchor = this->addAnchor(position, parameters, parameterNames);
	}

	this->activeAnchor->addSelection(selection);

	glPopClientAttrib();
	glPopAttrib();

	emit updated();

	return selection;
}


bool Presentation::isIntegratedViewRegistered(QString id)
{
	for (int i = 0; i < this->integratedViews.count(); i++)
	{
		if (this->integratedViews[i]->getId() == id)
			return true;
	}
	return false;
}

void Presentation::registerIntegratedView(QGLFramebufferObject *givenFbo, QWidget *integratedView, QString id)
{
	qDebug() << "registering" << id;

	if (this->isIntegratedViewRegistered(id))
		return;
	qDebug() << "done";

	IntegratedView * iv = new IntegratedView(givenFbo, id, integratedView, this);
	this->integratedViews << iv;
	connect(iv, SIGNAL(updated()), this, SLOT(defferedUpdate()));

	//These slots of the integratedView will be triggered to transfer data from the presentation; the presentation will emit
	//respective signals to transfer the data.
	if (integratedView->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("clearArrays()")) >= 0)
		connect(this, SIGNAL(arraysReset()), integratedView, SLOT(clearArrays()));
	if (integratedView->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("addArray(QList<int>, int, QString)")) >= 0)
		connect(this, SIGNAL(arrayReceived(QList<int>, int, QString)), integratedView, SLOT(addArray(QList<int>, int, QString)));
	if (integratedView->metaObject()->indexOfSlot(QMetaObject::normalizedSignature("arraysFinish()")) >= 0)
		connect(this, SIGNAL(arraysFinished()), integratedView, SLOT(arraysFinish()));

	//IntegratedView() makes the context of its parent (this) current, so we need to make the Presentation's parent current, so
	//the registration of the integrated view widgets can be done in initializeGL of the parent widget
}

bool Presentation::addIntegratedView(Selection *selection, QString id)
{
	for (int i = 0; i < this->integratedViews.count(); i++)
	{
		if (this->integratedViews[i]->getId() == id)
		{
			selection->addIntegratedView(this->integratedViews[i]);
			return true;
		}
	}

	return false;
}

void Presentation::defferedUpdate()
{
	emit updated();
}

//mark that the view has changed. next time the current parameters change (setCurrentParameter{s}()) and flagViewChanged is true, changeView() is called (this is done by checkIfViewChanged() method).
void Presentation::viewChanged()
{
	this->flagViewChanged = true;
}

void Presentation::checkIfViewChanged()
{
	if (this->flagViewChanged)
	{
		this->flagViewChanged = false;
		this->changeView();
	}
}

//apply the operations when the view changes
void Presentation::changeView()
{
	int matchedSnapshot = -1;

	if (this->autoActivateSnapshots)
	{
		for (int i = 0; i < this->anchors.size(); i++)
		{
			bool match = true;
			for (int j = 0; j < qMin(this->currentParameters.size(), this->anchors[i]->getParameters().size()); j++)
			{
				qDebug() << "matching" << this->anchors[i]->getParameterName(j) << ":" << this->currentParameters[j] << this->anchors[i]->getParameter(j);
				if (!PRSUtil::equal(this->currentParameters[j], this->anchors[i]->getParameter(j), this->anchors[i]->getParameterName(j)))
				{
					qDebug() << "nope";
					match = false;
					break;
				}
				qDebug() << "yep";
			}

			if (match)
			{
				matchedSnapshot = i;
				break;
			}
		}
	}

	qDebug() << "match:" << matchedSnapshot;

	if (matchedSnapshot >= 0)
	{
		//auto-select the contextual snapshot
		this->lastActiveAnchor = this->activeAnchor;
		this->activeAnchor = this->anchors[matchedSnapshot];
		this->thumbnailAnchor = 0;

		if (this->autoActivateSelections)
		{
			this->beginSelection();
			this->selectAllSelections();
			this->endSelection();
		}
	}
	else
	{
		//deselect selected snapshot
		this->lastActiveAnchor = 0;
		this->activeAnchor = 0;
		this->maximizedIntegratedView = false;

		this->unselectSelections();
	}

	this->slideTimer->stop();
	this->maximizeTimer->stop();
}

void Presentation::unselectSelections()
{
	this->activeSelections.clear();
	this->activeIntegratedViews.clear();
	this->activeIVRects.clear();

	this->activeSelectionsCount = 0;
}

void Presentation::beginSelection()
{
	this->unselectSelections();

	this->activeSelectionsCount = 0;
	this->firstSelectionAt = true;
}

void Presentation::endSelection()
{
	QSet<IntegratedView *> ivSet;
	ivSet.clear();

	//find integrated views common for all the selections
	this->activeIntegratedViews.clear();
	for (int i = 0; i < this->activeSelections.count(); i++)
	{
		for (int j = 0; j < this->activeSelections[i]->getIntegratedViews().count(); j++)
		{
			ivSet << this->activeSelections[i]->getIntegratedViews()[j];
		}
	}

	this->activeIntegratedViews = QList<IntegratedView *>::fromSet(ivSet);

	qSort(this->activeIntegratedViews.begin(), this->activeIntegratedViews.end(), ivLessThan);

	//set the middle integrated view as the middle one visually as well
	this->currentViewIndex = this->activeIntegratedViews.count() / 2;

	//find the position of the integration view strip
	QPoint delta = this->selectionAt - this->lastSelectionAt;


	if (qAbs(delta.x()) > qAbs(delta.y()))
	{
		if (delta.x() <= 0)
			this->currentViewPosition = StripeLeft;
		else
			this->currentViewPosition = StripeRight;
	}
	else
	{
		if (delta.y() < 0)
			this->currentViewPosition = StripeBottom;
		else
			this->currentViewPosition = StripeTop;
	}


	//generate a texture array of the selection masks
	this->selectionMasksSize = this->outputFbo->size();
	this->activeSelectionsCount = this->activeSelections.count();

	//qDebug() << this->selectionMasksSize;
	QVector<uchar> data;
	for (int i = 0; i < this->activeSelections.count(); i++)
	{
		QImage image = this->activeSelections[i]->getSelectionMask()->toImage().scaled(this->selectionMasksSize).mirrored();

		//qDebug() << "s" << i << image.size();
		data.reserve(data.size() + image.byteCount());
		for (int j = 0; j < image.byteCount(); j++)
		{
			data.append(image.bits()[j]);
		}
	}
	//qDebug() << data.size() << "bytes of selections";

	//return;

	//if (false) //todo
	{
	glBindTexture(GL_TEXTURE_2D_ARRAY, selectionMasksArray);

	((PFNGLTEXIMAGE3DPROC)wglGetProcAddress("glTexImage3D"))
	/*glTexImage3D*/(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA,
				 this->selectionMasksSize.width(), this->selectionMasksSize.height(), this->activeSelections.count(),
				 0, GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	}

	data.clear();

	this->ivInvalid = true;

	//qDebug() << "texture ready" << rand() % 100;
}

void Presentation::selectAllSelections()
{
	if (this->activeAnchor != 0)
		this->activeSelections =  this->activeAnchor->getSelections();
}

void Presentation::selectLastSelection()
{
	if (this->activeAnchor != 0)
	{
		this->activeSelections << this->getSketcher()->getSelections().last();
	}
}


bool Presentation::selectSelection(int x, int y)
{
	if (this->activeAnchor == 0)
		return false;

	if (this->firstSelectionAt)
	{
		this->firstSelectionAt = false;
		this->lastSelectionAt = QPoint(x, y);
	}
	else
	{
		this->lastSelectionAt = this->selectionAt;
	}
	this->selectionAt = QPoint(x, y);

	if (x < 0 || x >= this->outputFbo->width() ||
		y < 0 || y >= this->outputFbo->height())
		return false;


	uchar pixel[4];

	Selection *selected = 0;
	int selectedIntensity = 0;

	for (int i = 0; i < this->activeAnchor->getSelections().count(); i++)
	{
		this->activeAnchor->getSelections().at(i)->getSelectionMask()->bind();

		int transformedX = (int) ((qreal) x / (qreal) this->outputFbo->width() * (qreal) this->activeAnchor->getSelections().at(i)->getSelectionMask()->width());
		int transformedY = (int) ((qreal) y / (qreal) this->outputFbo->height() * (qreal) this->activeAnchor->getSelections().at(i)->getSelectionMask()->height());

		glReadBuffer(GL_AUX0);
		glReadPixels(transformedX, transformedY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &pixel);

		if (selectedIntensity < pixel[0])
		{
			selectedIntensity = pixel[0];
			selected = this->activeAnchor->getSelections().at(i);
		}
	}

	if (selectedIntensity > 0)
	{
		if (!this->activeSelections.contains(selected))
			this->activeSelections << selected;

		return true;
	}
	else
	{
		return false;
	}
}

int Presentation::anchorOnPosition(int x, int y)
{
	qreal distance;

	for (int i = this->anchors.count() - 1; i >= 0; i--)
	{
		distance = QVector2D(QVector2D(x, y) - this->anchors[i]->getScreenPosition()).length();

		if (distance < 7)
		{
			this->hoverAnchor = this->anchors[i];
			return this->anchors[i]->getId();
		}
	}

	this->hoverAnchor = 0;
	return -1;
}

int Presentation::findAnchor(int anchorId)
{
	for (int i = 0; i < this->anchors.count(); i++)
	{
		if (this->anchors[i]->getId() == anchorId)
		{
			return i;
		}
	}

	return -1;
}

void Presentation::animateToAnchor(QMatrix4x4 matrix, QList<QVariant> parameters, int anchorId)
{
	if (this->interpolationTimer->isActive())
		return;

	int index = this->findAnchor(anchorId);
	if (index < 0)
		return;
	if (this->activeAnchor == this->anchors[index])
		return;

	this->lastActiveAnchor = this->activeAnchor;
	this->activeAnchor = this->anchors[index];
	this->thumbnailAnchor = 0;

	/*GLfloat modelview[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
	qreal values[16];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			values[i * 4 + j] = (qreal) modelview[j * 4 + i];
	this->matrixFrom =  QMatrix4x4(values);*/

	this->matrixFrom = matrix;
	this->matrixTo = this->anchors[index]->getNoScalingMatrix();

	this->parametersFrom = parameters;
	this->parametersTo = this->anchors[index]->getParameters();

	this->unselectSelections();

	this->interpolationAnimation = 0.0;
	this->interpolationTimer->start(10);
}

void Presentation::animateToAnchorParameters(QList<QVariant> parameters, int anchorId)
{
	int index = this->findAnchor(anchorId);
	if (index < 0)
		return;
	if (this->activeAnchor == this->anchors[index])
		return;

	this->lastActiveAnchor = this->activeAnchor;
	this->activeAnchor = this->anchors[index];
	this->thumbnailAnchor = 0;

	this->parametersFrom = parameters;
	this->parametersTo = this->anchors[index]->getParameters();

	this->unselectSelections();

	this->interpolationAnimation = 0.0;
	this->interpolationTimer->start(10);
}

void Presentation::animateToAnchorParameters(int anchorId)
{
	int index = this->findAnchor(anchorId);
	if (index < 0)
		return;
	if (this->activeAnchor == this->anchors[index])
		return;

	this->lastActiveAnchor = this->activeAnchor;
	this->activeAnchor = this->anchors[index];
	this->thumbnailAnchor = 0;

	this->parametersFrom = this->currentParameters;
	this->parametersTo = this->anchors[index]->getParameters();

	this->unselectSelections();

	this->interpolationAnimation = 0.0;
	this->interpolationTimer->start(10);
}

bool Presentation::showAnchorThumbnail(int anchorId)
{
	Anchor *lastThumbnailAnchor = this->thumbnailAnchor;

	int index = this->findAnchor(anchorId);
	if (index < 0)
	{
		this->thumbnailAnchor = 0;
	}
	else
	{
		this->thumbnailAnchor = this->anchors[index];
	}

	this->thumbnailDisplayed = false;
	this->thumbnailAnchorTimer->start(this->thumbnailDelay);

	return this->thumbnailAnchor != lastThumbnailAnchor;
}

bool Presentation::injectEvent(QEvent *event, QPoint pos)
{
	/*QPoint pos;

	if (event->type() == QEvent::Wheel)
		pos = ((QWheelEvent *) event)->pos();
	if (event->type() == QEvent::MouseButtonDblClick ||
		event->type() == QEvent::MouseButtonPress ||
		event->type() == QEvent::MouseButtonRelease ||
		event->type() == QEvent::MouseMove)
		pos = ((QMouseEvent *) event)->pos();*/

	if (this->maximizedIntegratedView)
	{
		if (pos.x() >= this->maximizedRect.left() &&
			pos.x() <= this->maximizedRect.right() &&
			pos.y() >= this->maximizedRect.top() &&
			pos.y() <= this->maximizedRect.bottom())
		{
			QPoint newPos(
						(int) ((qreal) (pos.x() - this->maximizedRect.x()) / (qreal) this->maximizedRect.width() * (qreal) this->activeIntegratedViews[this->currentViewIndex]->width()),
						(int) ((qreal) (pos.y() - this->maximizedRect.y()) / (qreal) this->maximizedRect.height() * (qreal) this->activeIntegratedViews[this->currentViewIndex]->height())
						);
			this->activeIntegratedViews[this->currentViewIndex]->injectEvent(event, newPos);
			return true;
		}
	}
	else
	{
		for (int i = 0; i < this->activeIVRects.count(); i++)
		{
			if (pos.x() >= this->activeIVRects[i].rect.left() &&
				pos.x() <= this->activeIVRects[i].rect.right() &&
				pos.y() >= this->activeIVRects[i].rect.top() &&
				pos.y() <= this->activeIVRects[i].rect.bottom())
			{
				QPoint newPos(
							(int) ((qreal) (pos.x() - this->activeIVRects[i].rect.x()) / (qreal) this->activeIVRects[i].rect.width() * (qreal) this->activeIVRects[i].integratedView->width()),
							(int) ((qreal) (pos.y() - this->activeIVRects[i].rect.y()) / (qreal) this->activeIVRects[i].rect.height() * (qreal) this->activeIVRects[i].integratedView->height())
							);
				this->activeIVRects[i].integratedView->injectEvent(event, newPos);
				return true;
			}
		}
	}

	return false;

	/*
	if (this->activeVisualization &&
		this->activeSelection != 0 &&
		pos.x() >= this->activeVisualizationRect.left() &&
		pos.x() <= this->activeVisualizationRect.right() &&
		pos.y() >= this->activeVisualizationRect.top() &&
		pos.y() <= this->activeVisualizationRect.bottom())
	{
		QPoint newPos(
					(int) ((qreal) (pos.x() - this->activeVisualizationRect.x()) / (qreal) this->activeVisualizationRect.width() * (qreal) this->activeSelection->getIntegratedViews().first()->width()),
					(int) ((qreal) (pos.y() - this->activeVisualizationRect.y()) / (qreal) this->activeVisualizationRect.height() * (qreal) this->activeSelection->getIntegratedViews().first()->height())
					);

		//this->activeSelection->getIntegratedViews().first()->injectEvent(IntegratedView::changeEventPosition(event, newPos), newPos);
		this->activeSelection->getIntegratedViews().first()->injectEvent(event, newPos);

		return true;
	}
	else
	{
		return false;
	}*/
}

void Presentation::addSelectionPoint(SelectionPoint p)
{
	this->getSketcher()->addPoint(p);
}

void Presentation::renderLastSelection(QGLShaderProgram *shader)
{
	this->getSketcher()->renderLastSelection(shader);
}

void Presentation::setEasingCurve(QEasingCurve::Type type)
{
	this->easingCurve.setType(type);
}

void Presentation::animate()
{
	this->interpolationAnimation += 0.04;
	if (this->interpolationAnimation >= 1.0)
	{
		this->interpolationTimer->stop();
		this->interpolationAnimation = 1.0;

		if (this->autoActivateSelections)
		{
			this->beginSelection();
			this->selectAllSelections();
			this->endSelection();
		}
	}

	QMatrix4x4 matrix = PRSUtil::mix(matrixFrom, matrixTo, easingCurve.valueForProgress(this->interpolationAnimation));

	QList<QVariant> interpolated;

	for (int i = 0; i < this->parametersFrom.count(); i++)
	{
		interpolated << PRSUtil::mix(this->parametersFrom[i], this->parametersTo[i], easingCurve.valueForProgress(this->interpolationAnimation));
	}

	/*

	GLfloat values[16];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			values[i * 4 + j] = (GLfloat) matrix.data()[j * 4 + i];

	glLoadMatrixf(values);
	*/

	//qDebug() << "emitting params" << interpolated;
	emit parametersChanged(interpolated);
	//qDebug() << "emitting matrix" << matrix;
	emit matrixChanged(matrix);
	//emit updated();
}

void Presentation::renderSelectionsToAnchorScreenshot(Anchor *anchor)
{
	if (anchor->getBaseScreenshot() == 0)
		return;
	if (anchor->getScreenshot() == 0)
	{
		QGLFramebufferObject *fbo = new QGLFramebufferObject(anchor->getBaseScreenshot()->size());

		anchor->setScreenshot(fbo);
	}

	this->copyFbo(anchor->getBaseScreenshot(), anchor->getScreenshot());
	anchor->getScreenshot()->bind();
	this->renderSelections(anchor, 0, 0, anchor->getScreenshot()->width(), anchor->getScreenshot()->height(), true);
	anchor->getScreenshot()->release();
}

void Presentation::renderSelectionsToActiveAnchorScreenshot()
{
	/*
	this->copyFbo(this->activeAnchor->getBaseScreenshot(), this->activeAnchor->getScreenshot());
	this->activeAnchor->getScreenshot()->bind();
	this->renderSelections(this->activeAnchor);
	this->activeAnchor->getScreenshot()->release();*/
	this->renderSelectionsToAnchorScreenshot(this->activeAnchor);
}

void Presentation::setThumbnailDelay(int msec)
{
	this->thumbnailDelay = msec;
}

int Presentation::getThumbnailDelay()
{
	return this->thumbnailDelay;
}

void Presentation::slidePrev()
{
	if (this->slideTimer->isActive() || this->activeIntegratedViews.isEmpty() || this->currentViewIndex <= 0)
		return;

	this->slideAnimation = 0.0;
	this->slideAdd = false;
	this->nextViewIndex = this->currentViewIndex - 1;
	this->slideTimer->start(10);
}

void Presentation::slideNext()
{
	if (this->slideTimer->isActive() || this->activeIntegratedViews.isEmpty() || this->currentViewIndex >= this->activeIntegratedViews.count() - 1)
		return;

	this->slideAnimation = 0.0;
	this->slideAdd = true;
	this->nextViewIndex = this->currentViewIndex + 1;
	this->slideTimer->start(10);
}

void Presentation::maximizeIntegratedView()
{
	if (this->maximizeTimer->isActive() || this->activeIntegratedViews.isEmpty() || this->currentViewIndex < 0 || this->currentViewIndex >= this->activeIntegratedViews.count())
		return;

	this->maximizedRect = PRSUtil::fitToSize(this->activeIntegratedViews[this->currentViewIndex]->size(), this->outputFbo->size()).toRect();

	this->maximizedIntegratedView = false;
	this->maximizeAnimation = 0.0;
	this->maximizeAdd = true;
	this->maximizeTimer->start(10);
}

void Presentation::minimizeIntegratedView()
{
	if (this->maximizeTimer->isActive() || this->activeIntegratedViews.isEmpty() || this->currentViewIndex < 0 || this->currentViewIndex >= this->activeIntegratedViews.count())
		return;

	this->maximizedIntegratedView = true;
	this->maximizeAnimation = 1.0;
	this->maximizeAdd = false;
	this->maximizeTimer->start(10);
}

void Presentation::maximizeMinimizeIntegratedView()
{
	if (this->maximizedIntegratedView)
		this->minimizeIntegratedView();
	else
		this->maximizeIntegratedView();
}


void Presentation::animateSlide()
{
	this->slideAnimation += (this->slideAdd ? 0.05 : -0.05);

	this->maximizedRect = PRSUtil::mix(
				PRSUtil::fitToSize(this->activeIntegratedViews[this->currentViewIndex]->size(), this->outputFbo->size()).toRect(),
				PRSUtil::fitToSize(this->activeIntegratedViews[this->nextViewIndex]->size(), this->outputFbo->size()).toRect(),
				qAbs(this->slideAnimation));

	if ((this->slideAdd && this->slideAnimation > 1.0) || (!this->slideAdd && this->slideAnimation < -1.0))
	{
		this->slideAnimation = 0.0;
		this->slideTimer->stop();
		this->currentViewIndex = this->nextViewIndex;
	}

	emit updated();
}

void Presentation::animateMaximize()
{
	this->maximizeAnimation += (this->maximizeAdd ? 0.05 : -0.05);
	if (this->maximizeAdd && this->maximizeAnimation > 1.0)
	{
		this->maximizeAnimation = 1.0;
		this->maximizeTimer->stop();
		this->maximizedIntegratedView = true;
	}
	if (!this->maximizeAdd && this->maximizeAnimation < 0.0)
	{
		this->maximizeAnimation = 0.0;
		this->maximizeTimer->stop();
		this->maximizedIntegratedView = false;
	}

	emit updated();
}

bool Presentation::save(QString directory)
{

	QDir dir(directory);

	//if the directory exists, remove everything from it
	if (dir.exists())
	{
		foreach (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files))
		{
			if (!info.isDir())
			{
				QFile::remove(info.absoluteFilePath());
			}
		}
		dir.rmdir(directory);
	}
	QDir().mkdir(directory);

	//save selection masks
	for (int i = 0; i < this->sketcher->getSelections().count(); i++)
	{
		this->sketcher->getSelections().at(i)->getSelectionMask()->toImage().save(dir.absoluteFilePath("selection_" + QString::number(i) + ".png"));
	}

	//save presentation details
	QFile output(directory + "/" + "presentation.xml");
	if (!output.open(QIODevice::WriteOnly))
		return false;
	QXmlStreamWriter writer(&output);
	writer.setAutoFormatting(true);
	writer.writeStartDocument();

	//root element
	writer.writeStartElement("presentation");

	//save anchors
	writer.writeStartElement("anchors");
	for (int i = 0; i < this->anchors.count(); i++)
	{
		this->anchors[i]->getBaseScreenshot()->toImage().save(dir.absoluteFilePath("base_screenshot_" + QString::number(this->anchors[i]->getId()) + ".png"));
		//this->anchors[i]->getScreenshot()->toImage().save(dir.absoluteFilePath("screenshot_" + QString::number(this->anchors[i]->getId()) + ".png"));

		writer.writeStartElement("anchor");
		writer.writeAttribute("id", QString::number(this->anchors[i]->getId()));
			writer.writeStartElement("position");
			writer.writeAttribute("x", QString::number(this->anchors[i]->getPosition().x()));
			writer.writeAttribute("y", QString::number(this->anchors[i]->getPosition().y()));
			writer.writeAttribute("z", QString::number(this->anchors[i]->getPosition().z()));
			writer.writeEndElement();

			writer.writeStartElement("matrix");
				for (int j = 0; j < 4; j++)
				{
					for (int k = 0; k < 4; k++)
					{
						writer.writeTextElement("m" + QString::number(k + 1) + QString::number(j + 1), QString::number(this->anchors[i]->getMatrix().data()[k + j * 4]));
					}
				}
			writer.writeEndElement();

			writer.writeStartElement("selections");
				for (int j = 0; j < this->anchors[i]->getSelections().count(); j++)
				{
					Selection *searchFor = this->anchors[i]->getSelections()[j];
					int selectionId = -1;
					for (int k = 0; k < this->sketcher->getSelections().count(); k++)
					{
						if (this->sketcher->getSelections()[k] == searchFor)
						{
							selectionId = k;
							break;
						}
					}
					if (selectionId >= 0)
						writer.writeTextElement("selection", QString::number(selectionId));
				}
			writer.writeEndElement();

			writer.writeStartElement("parameters");
				for (int j = 0; j < this->anchors[i]->getParameters().count(); j++)
				{
					writer.writeStartElement("parameter");
						writer.writeAttribute("name", this->anchors[i]->getParameterNames()[j]);
						writer.writeCharacters(QString::number(this->anchors[i]->getParameters()[j].toReal()));
					writer.writeEndElement();
				}
			writer.writeEndElement();
		writer.writeEndElement();
	}
	//end of anchors
	writer.writeEndElement();

	//save recorded strokes
	writer.writeStartElement("strokes");
	for (int i = 0; i < this->sketcher->getSelections().count(); i++)
	{
		writer.writeStartElement("stroke");
		writer.writeAttribute("selection_id", QString::number(i));

		QList<SelectionPoint> stroke = this->sketcher->getSelections()[i]->getPoints();

		for (int j = 0; j < stroke.count(); j++)
		{
			writer.writeStartElement("point");
			writer.writeAttribute("x", QString::number(stroke[j].getPos().x()));
			writer.writeAttribute("y", QString::number(stroke[j].getPos().y()));
			writer.writeAttribute("pressure", QString::number(stroke[j].getPressure()));
			writer.writeAttribute("data", QString::number(stroke[j].getData()));
			writer.writeEndElement();
		}

		writer.writeEndElement();
	}

	//end of strokes
	writer.writeEndElement();

	//save integrated views for every selection
	writer.writeStartElement("views");
	for (int i = 0; i < this->sketcher->getSelections().count(); i++)
	{
		writer.writeStartElement("view");
		writer.writeAttribute("selection_id", QString::number(i));

		QList<IntegratedView *> integratedViews = this->sketcher->getSelections()[i]->getIntegratedViews();

		for (int j = 0; j < integratedViews.count(); j++)
		{
			writer.writeTextElement("name", integratedViews[j]->getId());
		}

		writer.writeEndElement();
	}

	//end of strokes
	writer.writeEndElement();

	//end of the root
	writer.writeEndElement();

	writer.writeEndDocument();
	output.close();
	return true;
}

bool Presentation::load(QString directory)
{
	this->reset();

	//load selections from selection_*.png files
	QList<Selection *> selections;
	QDir dir(directory);
	if (dir.exists())
	{
		foreach (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::Files))
		{
			QRegExp re("^selection_[0-9]+\\.png$");
			QString name = info.fileName();
			if (name.contains(re))
			{
				QGLFramebufferObject *fbo = this->imageToFbo(QImage(info.absoluteFilePath()));
				glBindTexture(GL_TEXTURE_2D, fbo->texture());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

				Selection *selection = new Selection();
				selection->setSelectionMask(fbo);

				selections << selection;
			}
		}

		this->sketcher->setSelections(selections);
	}
	else
	{
		return false;
	}

	Anchor *anchor;
	QMatrix4x4 matrix;
	QList<QVariant> parameters;
	QList<QString> parameterNames;

	//parse anchors.xml
	QFile input(directory + "/" + "presentation.xml");
	if (!input.open(QIODevice::ReadOnly))
		return false;
	QXmlStreamReader reader(&input);
	int token;

	enum ReadingWhat {Nothing = 0,
					  PresentationNode,
						AnchorsNode,
						  AnchorNode,    //id
						  PositionNode,  //x, y, z
						  MatrixNode,
							MatrixElementNode,
						  SelectionsNode,
							SelectionNode,
						  ParametersNode,
							ParameterNode,
						StrokesNode,
						  StrokeNode,    //selection_id
							PointNode,   //x, y, pressure, data
						ViewsNode,
						  ViewNode,      //selection_id
							NameNode
					 };

	ReadingWhat reading = Nothing;
	QRegExp matrixElementRE("^m([0-9])([0-9])$");
	int matrixRow;
	int matrixColumn;
	int selectionId;

	while (!reader.atEnd())
	{
		token = reader.readNext();

		if (token == QXmlStreamReader::StartElement)
		{
			//presentation
			if (reading == Nothing && reader.name() == "presentation")
			{
				reading = PresentationNode;
			}
			//anchors
			else if (reading == PresentationNode && reader.name() == "anchors")
			{
				reading = AnchorsNode;
			}
			else if (reading == AnchorsNode && reader.name() == "anchor")
			{
				reading = AnchorNode;

				anchor = new Anchor(this);
				matrix.setToIdentity();

				//find id attribute and assign its value to the newly created anchor
				for (int i = 0; i < reader.attributes().count(); i++)
				{
					if (reader.attributes().at(i).name() == "id")
					{
						anchor->setId(reader.attributes().at(i).value().toString().toInt());
						break;
					}
				}

				anchor->setBaseScreenshot(this->imageToFbo(QImage(directory + "/base_screenshot_" + QString::number(anchor->getId()) + ".png" )));
				//anchor->setScreenshot(this->imageToFbo(QImage(directory + "/screenshot_" + QString::number(anchor->getId()) + ".png" )));
			}
			else if (reading == AnchorNode && reader.name() == "position")
			{
				reading = PositionNode;

				//find x, y, and z coordinates and assign their values as position to the newly created anchor
				QVector3D position = QVector3D(0.0, 0.0, 0.0);
				for (int i = 0; i < reader.attributes().count(); i++)
				{
					if (reader.attributes().at(i).name() == "x")
					{
						position.setX(reader.attributes().at(i).value().toString().toFloat());
					}
					if (reader.attributes().at(i).name() == "y")
					{
						position.setY(reader.attributes().at(i).value().toString().toFloat());
					}
					if (reader.attributes().at(i).name() == "z")
					{
						position.setZ(reader.attributes().at(i).value().toString().toFloat());
					}
				}
				anchor->setPosition(position);
			}
			else if (reading == AnchorNode && reader.name() == "matrix")
			{
				reading = MatrixNode;
			}
			else if (reading == MatrixNode && reader.name().toString().contains(matrixElementRE))
			{
				reading = MatrixElementNode;
				matrixColumn = matrixElementRE.cap(1).toInt();
				matrixRow = matrixElementRE.cap(2).toInt();
			}
			else if (reading == AnchorNode && reader.name() == "selections")
			{
				reading = SelectionsNode;
			}
			else if (reading == SelectionsNode && reader.name() == "selection")
			{
				reading = SelectionNode;
			}
			else if (reading == AnchorNode && reader.name() == "parameters")
			{
				reading = ParametersNode;
			}
			else if (reading == ParametersNode && reader.name() == "parameter")
			{
				for (int i = 0; i < reader.attributes().count(); i++)
				{
					if (reader.attributes().at(i).name() == "name")
					{
						parameterNames << reader.attributes().at(i).value().toString();
						break;
					}
				}

				reading = ParameterNode;
			}
			//strokes
			else if (reading == PresentationNode && reader.name() == "strokes")
			{
				reading = StrokesNode;
			}
			else if (reading == StrokesNode && reader.name() == "stroke")
			{
				reading = StrokeNode;

				//find selection_id attribute and remember it for all point nodes
				for (int i = 0; i < reader.attributes().count(); i++)
				{
					if (reader.attributes().at(i).name() == "selection_id")
					{
						selectionId = reader.attributes().at(i).value().toString().toInt();
						break;
					}
				}
			}
			else if (reading == StrokeNode && reader.name() == "point")
			{
				reading = PointNode;

				//find x, y, pressure and data and assign their values to the selection with id selectionId
				SelectionPoint selectionPoint;
				for (int i = 0; i < reader.attributes().count(); i++)
				{
					if (reader.attributes().at(i).name() == "x")
					{
						selectionPoint.setX(reader.attributes().at(i).value().toString().toFloat());
					}
					if (reader.attributes().at(i).name() == "y")
					{
						selectionPoint.setY(reader.attributes().at(i).value().toString().toFloat());
					}
					if (reader.attributes().at(i).name() == "pressure")
					{
						selectionPoint.setPressure(reader.attributes().at(i).value().toString().toFloat());
					}
					if (reader.attributes().at(i).name() == "data")
					{
						selectionPoint.setData(reader.attributes().at(i).value().toString().toFloat());
					}
				}
				/*qDebug() << "selectionId" << selectionId;
				this->sketcher->getSelections()[selectionId]->recordPoint(selectionPoint);*/
			}
			//views
			else if (reading == PresentationNode && reader.name() == "views")
			{
				reading = ViewsNode;
			}
			else if (reading == ViewsNode && reader.name() == "view")
			{
				reading = ViewNode;

				//find selection_id attribute and remember it for all name nodes
				for (int i = 0; i < reader.attributes().count(); i++)
				{
					if (reader.attributes().at(i).name() == "selection_id")
					{
						selectionId = reader.attributes().at(i).value().toString().toInt();
						break;
					}
				}
			}
			else if (reading == ViewNode && reader.name() == "name")
			{
				reading = NameNode;
			}
		}

		if (token == QXmlStreamReader::EndElement)
		{
			//views
			if (reading == NameNode && reader.name() == "name")
			{
				reading = ViewNode;
			}
			else if (reading == ViewNode && reader.name() == "view")
			{
				reading = ViewsNode;
			}
			else if (reading == ViewsNode && reader.name() == "views")
			{
				reading = PresentationNode;
			}
			//strokes
			if (reading == PointNode && reader.name() == "point")
			{
				reading = StrokeNode;
			}
			else if (reading == StrokeNode && reader.name() == "stroke")
			{
				reading = StrokesNode;
			}
			else if (reading == StrokesNode && reader.name() == "strokes")
			{
				reading = PresentationNode;
			}
			//anchors
			else if (reading == SelectionNode && reader.name() == "selection")
			{
				reading = SelectionsNode;
			}
			else if (reading == SelectionsNode && reader.name() == "selections")
			{
				reading = AnchorNode;
			}
			else if (reading == ParameterNode && reader.name() == "parameter")
			{
				reading = ParametersNode;
			}
			else if (reading == ParametersNode && reader.name() == "parameters")
			{
				reading = AnchorNode;

				anchor->setParameters(parameters, parameterNames);
				parameters.clear();
				parameterNames.clear();
			}
			else if (reading == MatrixElementNode && reader.name().toString().contains(matrixElementRE))
			{
				reading = MatrixNode;
			}
			else if (reading == MatrixNode && reader.name() == "matrix")
			{
				reading = AnchorNode;

				//we are finished with the loading of the matrix, so we can assign it to the newly created anchor
				if (!this->anchors2D)
					anchor->setMatrix(matrix);
			}
			else if (reading == PositionNode && reader.name() == "position")
			{
				reading = AnchorNode;
			}
			else if (reading == AnchorNode && reader.name() == "anchor")
			{
				reading = AnchorsNode;

				//we are finished with the loading of the anchor, so we can add it to the presentation
				this->anchors << anchor;
			}
			else if (reading == AnchorsNode && reader.name() == "anchors")
			{
				reading = PresentationNode;
			}
			//presentation
			else if (reading == PresentationNode && reader.name() == "presentation")
			{
				reading = Nothing;
			}
		}

		if (token == QXmlStreamReader::Characters)
		{
			if (reading == MatrixElementNode)
			{
				//store respective element to the matrix
				matrix.data()[(matrixColumn - 1) + 4 * (matrixRow - 1)] = reader.text().toString().toFloat();
			}
			else if (reading == SelectionNode)
			{
				//add selection with index stored in the selection node to the newly created anchor
				anchor->addSelection(this->sketcher->getSelections()[reader.text().toString().toInt()]);
			}
			else if (reading == NameNode)
			{
				//add integrated view with loaded text ID to the desired selection (specified as selectionId in the parent view node)
				this->addIntegratedView(this->sketcher->getSelections()[selectionId], reader.text().toString());
			}
			else if (reading == ParameterNode)
			{
				parameters << reader.text().toString().toDouble();
			}
		}

		/*qDebug() << "++++++++++++++++++";
		qDebug() << "token" << token;
		qDebug() << "name" << reader.name();
		qDebug() << "text" << reader.text();
		qDebug() << "reading" << reading;*/
	}
	if (reader.hasError())
	{
		return false;
	}

	//now we render all selections to all anchor screenshots
	for (int i = 0; i < this->anchors.count(); i++)
	{
		this->renderSelectionsToAnchorScreenshot(this->anchors[i]);
	}

	input.close();
	return true;
}

void Presentation::setAnchorsVisible(bool anchorsVisible)
{
	this->anchorsVisible = anchorsVisible;
	emit updated();
}

void Presentation::setSelectionsVisible(bool selectionsVisible)
{
	this->selectionsVisible = selectionsVisible;
	emit updated();
}

void Presentation::setIntegratedViewsVisible(bool integratedViewsVisible)
{
	this->integratedViewsVisible = integratedViewsVisible;
	emit updated();
}

bool Presentation::isIntegratedViewsVisible()
{
	return this->integratedViewsVisible;
}


GLuint Presentation::getSelectionMasksArray()
{
	/*QList<GLuint> samplers;
	for (int i = 0; i < 32 && i < this->activeSelectionsCount; i++)
	{
		samplers << this->activeSelections[0]->getSelectionMask()->texture();
	}

	return samplers;*/
	return this->selectionMasksArray;
}

int Presentation::getActiveSelectionsCount()
{
	return this->activeSelectionsCount;
}

void Presentation::setAnimationWanted(bool animationWanted)
{
	this->animationWanted = animationWanted;
}

bool Presentation::isAnimationWanted()
{
	return animationWanted;
}

void Presentation::setAutoActivateSelections(bool autoActivateSelections)
{
	this->autoActivateSelections = autoActivateSelections;
}

bool Presentation::isAutoActivateSelections()
{
	return this->autoActivateSelections;
}

void Presentation::setAutoActivateSnapshots(bool autoActivateSnapshots)
{
	this->autoActivateSnapshots = autoActivateSnapshots;
}

bool Presentation::isAutoActivateSnapshots()
{
	return this->autoActivateSnapshots;
}


bool Presentation::isMaximizedIntegratedView()
{
	return maximizedIntegratedView;
}

/*
void Presentation::prepareRenderingShader(QGLShaderProgram *shader)
{
	//host context must be current!
	glBindTexture(GL_TEXTURE_2D_ARRAY, this->selectionMasksArray);
	shader->setUniformValue("PRSSelections", 1);
	shader->setUniformValue("PRSSelectionCount", this->activeSelectionsCount);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	shader->setUniformValue("PRSScreenWidth", viewport[2]);
	shader->setUniformValue("PRSScreenHeight", viewport[3]);
}
*/
