#ifndef PRESENTATION_H
#define PRESENTATION_H

#include <QObject>
#include <QWidget>
#include <QList>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QString>
#include <QGLFramebufferObject>
#include <QGLShaderProgram>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEventTransition>

#include "anchor.h"
#include "sketcher.h"

#include "prsutil.h"
#include "integratedview.h"
#include "prsshaderenhancer.h"

class Presentation : public QWidget
{
	Q_OBJECT
public:	
	explicit Presentation(int width, int height, QWidget * parent = 0);
	~Presentation();

	inline QString getVersion() { return "0.9.1"; }

	inline void setCurrentParameter(qreal parameter) { QList<qreal> parameters; parameters << parameter; this->currentParameters = parameters; }
	inline void setCurrentParameters(QList<qreal> parameters) { this->currentParameters = parameters; }

	void reset();

	void setAnchors2D(bool anchors2D);

	void clearSelectionDisplayShaders();
	void addSelectionDisplayShader(QGLShaderProgram *shader);
	void setSelectionDisplayShader(int index);
	void setStripeShader(QGLShaderProgram *shader);

	void resizeFbo(int width, int height);
	void setInputFbo(QGLFramebufferObject *inputFbo);
	void setDepthFbo(QGLFramebufferObject *depthFbo);
	QGLFramebufferObject *getOutputFbo();
	Sketcher *getSketcher();

	void copyFbo(QGLFramebufferObject *from, QGLFramebufferObject *to);
	QGLFramebufferObject *imageToFbo(QImage image);

	QList<int> regionQuery(int p, float eps);
	void clusterAnchors();
	void render();
	void render(GLfloat *projectionMatrix, GLfloat *modelviewMatrix);

	QPointF invertY(QPointF point);
	Anchor *addAnchor(QMatrix4x4 matrix, QMatrix4x4 noScalingMatrix, QList<qreal> parameters = QList<qreal>(), QList<QString> parameterNames = QList<QString>());
	Anchor *addAnchor(QVector3D position, QList<qreal> parameters = QList<qreal>(), QList<QString> parameterNames = QList<QString>());
	Selection *createSelection(QMatrix4x4 matrix, QMatrix4x4 noScalingMatrix, QList<qreal> parameters, QList<QString> parameterNames);
	Selection *createSelection(QVector3D position, QList<qreal> parameters, QList<QString> parameterNames);
	bool isIntegratedViewRegistered(QString id);
	void registerIntegratedView(QGLFramebufferObject *givenFbo, QWidget *integratedView, QString id);
	bool addIntegratedView(Selection *selection, QString id);

	void unselectSelections();
	void beginSelection();
	void endSelection();
	void selectAllSelections();
	void selectLastSelection();
	bool selectSelection(int x, int y);
	int anchorOnPosition(int x, int y);
	void animateToAnchor(QMatrix4x4 matrix, QList<qreal> parameters, int anchorId);
	void animateToAnchorParameters(QList<qreal> parameters, int anchorId);
	void animateToAnchorParameters(int anchorId);
	bool showAnchorThumbnail(int anchorId);
	bool injectEvent(QEvent *event, QPoint pos);

	void addSelectionPoint(SelectionPoint p);
	void renderLastSelection(QGLShaderProgram *shader);

	bool save(QString directory);
	bool load(QString directory);

	void setThumbnailDelay(int msec);
	int getThumbnailDelay();

	void slidePrev();
	void slideNext();

	void maximizeIntegratedView();
	void minimizeIntegratedView();
	void maximizeMinimizeIntegratedView();

	//void prepareRenderingShader(QGLShaderProgram *shader);

	void setAnchorsVisible(bool anchorsVisible);
	void setSelectionsVisible(bool selectionsVisible);
	void setIntegratedViewsVisible(bool integratedViewsVisible);
	bool isIntegratedViewsVisible();

	GLuint getSelectionMasksArray();
	int getActiveSelectionsCount();

	void setAnimationWanted(bool animationWanted);
	bool isAnimationWanted();

	void setAutoActivateSelections(bool autoActivateSelections);
	bool isAutoActivateSelections();

	bool isMaximizedIntegratedView();


	//types
	enum StripPosition {StripeTop = 0, StripeBottom, StripeLeft, StripeRight};
	struct ActiveRect
	{
		int index;
		QRectF rect;
		IntegratedView *integratedView;
	};

	inline void setProjectionMatrix(GLfloat projectionMatrix[16])
	{
		for (int i = 0; i < 16; i++)
			this->projectionMatrix[i] = projectionMatrix[i];
	}

	inline void setModelviewMatrix(GLfloat modelviewMatrix[16])
	{
		for (int i = 0; i < 16; i++)
			this->modelviewMatrix[i] = modelviewMatrix[i];
	}

	inline void setCurrentMatrix(QMatrix4x4 currentMatrix)
	{
		this->currentMatrix = currentMatrix;
	}

	inline QList<GLuint> getScreenshots()
	{
		QList<GLuint> screenshots;

		for (int i = 0; i < this->anchors.count(); i++)
		{
			screenshots << this->anchors[i]->getScreenshot()->texture();
		}

		return screenshots;
	}

protected:
	void setup2DMatrix();
	void setup3DMatrix(GLfloat *projectionMatrix, GLfloat *modelviewMatrix);

	virtual void renderClusters(GLfloat *projectionMatrix, GLfloat *modelviewMatrix);
	virtual void renderAnchor(Anchor *anchor, bool is2D);

	void initializeGL();
	int findAnchor(int anchorId);
	void renderIntegratedViews();
	void renderSelections(Anchor *anchor, int integratedViewX, int integratedViewY, int width, int height, bool all = false, bool preview = false);
	void renderSelectionsToAnchorScreenshot(Anchor *anchor);

	void prepareIntegratedViewFbo(qreal animation, int position);

//private:
	QList<QList<int> > clusters;

	QList<qreal> currentParameters;
	QMatrix4x4 currentMatrix;

	GLdouble projectionMatrix[16];
	GLdouble modelviewMatrix[16];

	QGLFramebufferObject *inputFbo;
	QGLFramebufferObject *depthFbo;
	QGLFramebufferObject *outputFbo;
	QGLFramebufferObject *integratedViewFbo;

	QList<Anchor *> anchors;
	Sketcher *sketcher;
	QGLWidget *gl;

	QTimer *interpolationTimer;
	qreal interpolationAnimation;
	QMatrix4x4 matrixFrom, matrixTo;
	QList<qreal> parametersFrom, parametersTo;

	QTimer *maximizeTimer;
	qreal maximizeAnimation;
	QRect maximizedRect;
	bool maximizeAdd;
	bool maximizedIntegratedView;

	QTimer *slideTimer;
	qreal slideAnimation;
	bool slideAdd;

	Anchor *lastActiveAnchor;
	Anchor *activeAnchor;
	Anchor *hoverAnchor;
	Anchor *thumbnailAnchor;
	QTimer *thumbnailAnchorTimer;
	bool thumbnailDisplayed;

	QList<QGLShaderProgram *> selectionDisplayShaders;
	QGLShaderProgram * selectionDisplayShader;
	QGLShaderProgram *stripeShader;

	//selected selections
	QList<Selection *> activeSelections;
	//integrated views common for all selected selections
	QList<IntegratedView *> activeIntegratedViews;
	//rectangles of the active integrated views
	QList<ActiveRect> activeIVRects;

	int currentViewIndex;
	int nextViewIndex;
	int currentViewPosition;
	
	int thumbnailDelay;

	bool horizontal;
	bool inpair;
	qreal spacing;
	qreal margin;
	qreal boundingSizeX;
	qreal boundingSizeY;
	qreal integratedViewX;
	qreal integratedViewY;
	qreal visibleLength;
	qreal fadingLength;
	QList<QSizeF> sizes;
	qreal stripeLength;
	qreal stripeWidth;
	qreal stripeHeight;

	bool firstSelectionAt;
	QPoint lastSelectionAt;
	QPoint selectionAt;

	QList<IntegratedView *> integratedViews;

	QSize selectionMasksSize;
	GLuint selectionMasksArray;
	int activeSelectionsCount;

	bool selectionsVisible;
	bool anchorsVisible;

	bool ivInvalid; //if true, integrated views will be updated next time
	bool animationWanted;
	bool autoActivateSelections; //automatically activate all selections when switched to an anchor

	bool anchors2D;

	bool integratedViewsVisible;

signals:
	void parametersChanged(QList<qreal>);
	void matrixChanged(QMatrix4x4);
	void updated();

	//data transfer bgetween presentation and integrated views
	void arraysReset();
	void arrayReceived(QList<int>, int, QString);
	void arraysFinished();

public slots:
	void animate();
	void animateSlide();
	void animateMaximize();
	void viewChanged();
	void renderSelectionsToActiveAnchorScreenshot();
	void defferedUpdate();
	void displayThumbnail();
	void updateIntegratedViews(PRSShaderEnhancer *shaderEnhancer);
};

#endif // PRESENTATION_H
