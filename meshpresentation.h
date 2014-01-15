#ifndef MESHPRESENTATION_H
#define MESHPRESENTATION_H

#include "presentation.h"

class MeshPresentation : public Presentation
{
	Q_OBJECT
public:	
	explicit MeshPresentation(int width, int height, QWidget * parent = 0);

protected:
	virtual void renderClusters(GLfloat *projectionMatrix, GLfloat *modelviewMatrix);
	virtual void renderAnchor(Anchor *anchor, bool is2D);
};

#endif // MESHPRESENTATION_H
