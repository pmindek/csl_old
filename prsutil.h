#ifndef PRSUTIL_H
#define PRSUTIL_H

#include <QtGlobal>
#include <QVector3D>
#include <QQuaternion>
#include <QMatrix4x4>
#include <QRect>
#include <QSizeF>
#include <cmath>

#include <QtOpenGL>
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glu.h>

#define PI 3.14159265358979323846264338327950

#define SIGN(x) ((x) < 0.0 ? -1.0 : 1.0)

#define GL_READ_ONLY 0x88B8
#define GL_WRITE_ONLY 0x88B9
#define GL_READ_WRITE 0x88BA

#define GL_RED_INTEGER 0x8D94
#define GL_RGBA8UI 0x8D7C
#define GL_LUMINANCE16UI 0x8D7A
#define GL_LUMINANCE_INTEGER 0x8D9C

#define GL_R32UI 0x8236
#define GL_RGBA32UI 0x8D70
#define GL_RGBA32F 0x8814
#define GL_R32F 0x822E
#define GL_TEXTURE_WRAP_R 0x8072

#define GL_TEXTURE_2D_ARRAY 35866
#define GL_TEXTURE_3D 0x806F

typedef void (APIENTRY * PFNGLBINDIMAGETEXTUREEXTPROC) (GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format);
typedef void (APIENTRY * PFNGLMEMORYBARRIEREXTPROC) (GLbitfield barriers);
typedef void (APIENTRY * PFNGLTEXIMAGE3DPROC) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);


class PRSUtil
{
public:
	PRSUtil();

	//conversion methods
	static QQuaternion matrixToQuaternion(QMatrix4x4 matrix);
	static QMatrix4x4 columnMajorToMatrix(float *matrix);
	static QMatrix4x4 columnMajorToMatrix(qreal *matrix);

	//matrix decomposition methods
	static QVector3D translationFromMatrix(QMatrix4x4 matrix);
	static QVector3D scalingFromMatrix(QMatrix4x4 matrix);
	static QMatrix4x4 rotationFromMatrix(QMatrix4x4 matrix, QVector3D scaling);

	//interpolation methods
	static qreal mix(qreal a0, qreal a1, qreal x);
	static QVector2D mix(QVector2D a0, QVector2D a1, qreal x);
	static QVector3D mix(QVector3D a0, QVector3D a1, qreal x);
	static QVector4D mix(QVector4D a0, QVector4D a1, qreal x);
	static QMatrix4x4 mix(QMatrix4x4 a0, QMatrix4x4 a1, qreal x);
	static QRect mix(QRect a0, QRect a1, qreal x);
	static QVariant mix(QVariant a0, QVariant a1, qreal x);

	//other methods
	static QRectF fitToSize(QSizeF size, QSizeF boundingSize);
	static QRectF fitRatioToRect(QSizeF ratio, QRectF boundingRect);
};

#endif // PRSUTIL_H
