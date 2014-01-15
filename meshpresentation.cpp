#include "meshpresentation.h"

MeshPresentation::MeshPresentation(int width, int height, QWidget *parent) : Presentation(width, height, parent)
{
}

void MeshPresentation::renderClusters(GLfloat *projectionMatrix, GLfloat *modelviewMatrix)
{
	for (int i = 0; i < this->clusters.count(); i++)
	{
		this->setup3DMatrix(projectionMatrix, modelviewMatrix);
		qreal ox = this->anchors[this->clusters[i][0]]->getPosition().x();
		qreal oy = this->anchors[this->clusters[i][0]]->getPosition().y();
		qreal oz = this->anchors[this->clusters[i][0]]->getPosition().z();

		glLineWidth(1.0);
		glBegin(GL_LINES);
			glColor4f(0.0, 0.2, 0.35, 0.0);
			//glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0.3, 143.8, 0.0);
			glColor4f(0.0, 0.2, 0.35, 0.3);
			glVertex3f(ox, oy, oz);
		glEnd();

		glPointSize(7.0);
		glColor4f(1.0, 1.0, 1.0, 1.0);
		glBegin(GL_POINTS);
			glVertex3f(ox, oy, oz);
		glEnd();

		this->setup2DMatrix();
		qreal sx = this->anchors[this->clusters[i][0]]->getScreenCenterPosition().x();
		qreal sy = this->anchors[this->clusters[i][0]]->getScreenCenterPosition().y();

		glLineWidth(1.5);
		glColor4f(1.0, 1.0, 1.0, 0.3);
		glBegin(GL_LINES);
		for (int j = 0; j <= 32; j++)
		{
			qreal x = sx + 30.0 * cos((qreal) j / 32.0 * 2.0 * PI);
			qreal y = sy + 30.0 * sin((qreal) j / 32.0 * 2.0 * PI);
			glVertex2f(x, y);
		}
		glEnd();
	}
}

void MeshPresentation::renderAnchor(Anchor *anchor, bool is2D)
{
	qreal distance = 1.0 - qAbs(this->currentParameters[0].toReal() - anchor->getParameters()[0].toReal());

	qreal ox = anchor->getPosition().x();
	qreal oy = anchor->getPosition().y();
	qreal oz = anchor->getPosition().z();

	qreal x = anchor->getScreenPosition().x();
	qreal y = anchor->getScreenPosition().y();

	if (!is2D && anchor->getCluster() < 0)
	{
		glBegin(GL_LINES);
			glColor4f(1.0, 1.0, 1.0, 0.0);
			//glVertex3f(0.0, 0.0, 0.0);
			glVertex3f(0.3, 143.8, 0.0);
			glColor4f(1.0, 1.0, 1.0, 0.3);
			glVertex3f(ox, oy, oz);
		glEnd();
	}

	this->setup2DMatrix();

	glColor4f(1.0, 1.0, 1.0, 1.0);
	glPointSize(PRSUtil::mix(9.0, 20.0, distance));
	glBegin(GL_POINTS);
		glVertex2f(x, y);
	glEnd();

	if (this->thumbnailAnchor == anchor)
		glColor4f(0.3, 0.3, 0.3, 1.0);
	else
		glColor4f(0.0, 0.0, 0.0, PRSUtil::mix(0.0, 1.0, distance));

	glPointSize(PRSUtil::mix(6.0, 17.0, distance));
	glBegin(GL_POINTS);
		glVertex2f(x, y);
	glEnd();
}

