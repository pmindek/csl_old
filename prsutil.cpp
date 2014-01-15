#include "prsutil.h"

PRSUtil::PRSUtil()
{
}

QQuaternion PRSUtil::matrixToQuaternion(QMatrix4x4 matrix)
{
	qreal *r = matrix.data();

	qreal r11 = r[0];
	qreal r12 = r[4];
	qreal r13 = r[8];
	qreal r21 = r[1];
	qreal r22 = r[5];
	qreal r23 = r[9];
	qreal r31 = r[2];
	qreal r32 = r[6];
	qreal r33 = r[10];

	qreal q0 = ( r11 + r22 + r33 + 1.0f) / 4.0f;
	qreal q1 = ( r11 - r22 - r33 + 1.0f) / 4.0f;
	qreal q2 = (-r11 + r22 - r33 + 1.0f) / 4.0f;
	qreal q3 = (-r11 - r22 + r33 + 1.0f) / 4.0f;
	if (q0 < 0.0f) q0 = 0.0f;
	if (q1 < 0.0f) q1 = 0.0f;
	if (q2 < 0.0f) q2 = 0.0f;
	if (q3 < 0.0f) q3 = 0.0f;
	q0 = sqrt(q0);
	q1 = sqrt(q1);
	q2 = sqrt(q2);
	q3 = sqrt(q3);
	if(q0 >= q1 && q0 >= q2 && q0 >= q3) {
		q0 *= +1.0f;
		q1 *= SIGN(r32 - r23);
		q2 *= SIGN(r13 - r31);
		q3 *= SIGN(r21 - r12);
	} else if(q1 >= q0 && q1 >= q2 && q1 >= q3) {
		q0 *= SIGN(r32 - r23);
		q1 *= +1.0f;
		q2 *= SIGN(r21 + r12);
		q3 *= SIGN(r13 + r31);
	} else if(q2 >= q0 && q2 >= q1 && q2 >= q3) {
		q0 *= SIGN(r13 - r31);
		q1 *= SIGN(r21 + r12);
		q2 *= +1.0f;
		q3 *= SIGN(r32 + r23);
	} else if(q3 >= q0 && q3 >= q1 && q3 >= q2) {
		q0 *= SIGN(r21 - r12);
		q1 *= SIGN(r31 + r13);
		q2 *= SIGN(r32 + r23);
		q3 *= +1.0f;
	}

	return QQuaternion(q0, q1, q2, q3).normalized();
}

QMatrix4x4 PRSUtil::columnMajorToMatrix(float *matrix)
{
	qreal values[16];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			values[i * 4 + j] = (qreal) matrix[j * 4 + i];
	return QMatrix4x4(values);
}

QMatrix4x4 PRSUtil::columnMajorToMatrix(qreal *matrix)
{
	qreal values[16];
	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			values[i * 4 + j] = matrix[j * 4 + i];
	return QMatrix4x4(values);
}

///

QVector3D PRSUtil::translationFromMatrix(QMatrix4x4 matrix)
{
	return QVector3D(
				matrix.data()[12],
				matrix.data()[13],
				matrix.data()[14]
				);
}

QVector3D PRSUtil::scalingFromMatrix(QMatrix4x4 matrix)
{
	  return QVector3D(
				sqrt(matrix.data()[0] * matrix.data()[0] + matrix.data()[4] * matrix.data()[4] + matrix.data()[ 8] * matrix.data()[ 8]),
				sqrt(matrix.data()[1] * matrix.data()[1] + matrix.data()[5] * matrix.data()[5] + matrix.data()[ 9] * matrix.data()[ 9]),
				sqrt(matrix.data()[2] * matrix.data()[2] + matrix.data()[6] * matrix.data()[6] + matrix.data()[10] * matrix.data()[10])
				);
}

QMatrix4x4 PRSUtil::rotationFromMatrix(QMatrix4x4 matrix, QVector3D scaling)
{
	return QMatrix4x4(
				matrix.data()[0] / scaling.x(), matrix.data()[4] / scaling.y(), matrix.data()[ 8] / scaling.z(), 0.0,
				matrix.data()[1] / scaling.x(), matrix.data()[5] / scaling.y(), matrix.data()[ 9] / scaling.z(), 0.0,
				matrix.data()[2] / scaling.x(), matrix.data()[6] / scaling.y(), matrix.data()[10] / scaling.z(), 0.0,
				0.0,                            0.0,                            0.0,                             1.0
				);
}

///

qreal PRSUtil::mix(qreal a0, qreal a1, qreal x)
{
	if (x < 0.0)
		x = 0.0;
	if (x > 1.0)
		x = 1.0;
	return a0 * (1.0 - x) + a1 * x;
}

QVector2D  PRSUtil::mix(QVector2D a0, QVector2D a1, qreal x)
{
	return QVector2D(PRSUtil::mix(a0.x(), a1.x(), x),
					 PRSUtil::mix(a0.y(), a1.y(), x));
}

QVector3D  PRSUtil::mix(QVector3D a0, QVector3D a1, qreal x)
{
	return QVector3D(PRSUtil::mix(a0.x(), a1.x(), x),
					 PRSUtil::mix(a0.y(), a1.y(), x),
					 PRSUtil::mix(a0.z(), a1.z(), x));
}

QVector4D  PRSUtil::mix(QVector4D a0, QVector4D a1, qreal x)
{
	return QVector4D(PRSUtil::mix(a0.x(), a1.x(), x),
					 PRSUtil::mix(a0.y(), a1.y(), x),
					 PRSUtil::mix(a0.z(), a1.z(), x),
					 PRSUtil::mix(a0.w(), a1.w(), x));
}

QMatrix4x4 PRSUtil::mix(QMatrix4x4 a0, QMatrix4x4 a1, qreal x)
{
	//method 1
	/*qreal values[16];
	QQuaternion rotation = QQuaternion::slerp(matrixToQuaternion(a0), matrixToQuaternion(a1), x);

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			values[i * 4 + j] = PRSUtil::mix(a0.data()[j * 4 + i], a1.data()[j * 4 + i], x);

	QMatrix4x4 identity;
	identity.setToIdentity();
	identity.rotate(rotation);

	values[0] = identity.data()[0];
	values[1] = identity.data()[4];
	values[2] = identity.data()[8];

	values[4] = identity.data()[1];
	values[5] = identity.data()[5];
	values[6] = identity.data()[9];

	values[8] = identity.data()[2];
	values[9] = identity.data()[6];
	values[10] = identity.data()[10];

	QMatrix4x4 matrix = QMatrix4x4(values);
*/

	//method 2
	/*values[0] = 1.0 - 2 * rotation.y() * rotation.y() - 2.0 * rotation.z() * rotation.z();
	values[1] = 2.0 * rotation.x() * rotation.y() - 2.0 * rotation.z() * rotation.scalar();
	values[2] = 2.0 * rotation.x() * rotation.z() + 2.0 * rotation.y() * rotation.scalar();

	values[4] = 2.0 * rotation.x() * rotation.y() + 2.0 * rotation.z() * rotation.scalar();
	values[5] = 1.0 - 2.0 * rotation.x() * rotation.x() - 2.0 * rotation.z() * rotation.z();
	values[6] = 2.0 * rotation.y() * rotation.z() - 2.0 * rotation.x() * rotation.scalar();

	values[7] = 2.0 * rotation.x() * rotation.z() - 2.0 * rotation.y() * rotation.scalar();
	values[8] = 2.0 * rotation.y() * rotation.z() + 2.0 * rotation.x() * rotation.scalar();
	values[10] = 1.0 - 2.0 * rotation.x() * rotation.x() - 2.0 * rotation.y() * rotation.y();*/

	//method 3
	QVector3D translation = PRSUtil::mix(
								PRSUtil::translationFromMatrix(a0),
								PRSUtil::translationFromMatrix(a1),
								x);

	QVector3D scaling = PRSUtil::mix(
								PRSUtil::scalingFromMatrix(a0),
								PRSUtil::scalingFromMatrix(a1),
								x);

	QQuaternion rotation = QQuaternion::slerp(
								PRSUtil::matrixToQuaternion(PRSUtil::rotationFromMatrix(a0, scaling)),
								PRSUtil::matrixToQuaternion(PRSUtil::rotationFromMatrix(a1, scaling)),
								x);

	QMatrix4x4 matrix;
	matrix.setToIdentity();
	matrix.translate(translation);
	matrix.rotate(rotation);
	//matrix.scale(scaling);

	return matrix;
}

QRect PRSUtil::mix(QRect a0, QRect a1, qreal x)
{
	return QRect(PRSUtil::mix(a0.left(), a1.left(), x),
				 PRSUtil::mix(a0.top(), a1.top(), x),
				 PRSUtil::mix(a0.width(), a1.width(), x),
				 PRSUtil::mix(a0.height(), a1.height(), x));
}

QVariant PRSUtil::mix(QVariant a0, QVariant a1, qreal x)
{
	QVariant result;
	switch (a0.type())
	{
	default:
	case QVariant::Double:
		result = PRSUtil::mix(a0.toFloat(), a1.toFloat(), x);
		break;
	case QVariant::Int:
		result = (int) PRSUtil::mix((qreal) a0.toInt(), (qreal) a1.toInt(), x);
		break;
	case QVariant::Bool:
		result = (x >= 0.5);
		break;
	case QVariant::Vector2D:
		result = QVariant::fromValue(qVariantFromValue(PRSUtil::mix(qvariant_cast<QVector2D> (a0), qvariant_cast<QVector2D> (a1), x)));
		break;
	case QVariant::Vector3D:
		result = QVariant::fromValue(qVariantFromValue(PRSUtil::mix(qvariant_cast<QVector3D> (a0), qvariant_cast<QVector3D> (a1), x)));
		break;
	case QVariant::Vector4D:
		result = QVariant::fromValue(qVariantFromValue(PRSUtil::mix(qvariant_cast<QVector4D> (a0), qvariant_cast<QVector4D> (a1), x)));
		break;
	case QVariant::Matrix4x4:
		result = QVariant::fromValue(qVariantFromValue(PRSUtil::mix(qvariant_cast<QMatrix4x4> (a0), qvariant_cast<QMatrix4x4> (a1), x)));
		break;
	case QVariant::Rect:
		result = QVariant::fromValue(qVariantFromValue(PRSUtil::mix(a0.toRect(), a1.toRect(), x)));
		break;
	}

	return result;
}

QRectF PRSUtil::fitToSize(QSizeF size, QSizeF boundingSize)
{
	qreal ratio;
	QSize newSize;

	if (size.width() > boundingSize.width())
	{
		if (size.height() > boundingSize.height())
		{
			if (boundingSize.width() / boundingSize.height() > size.width() / size.height())
			{
				ratio = boundingSize.height() / size.height();
			}
			else
			{
				ratio = boundingSize.width() / size.width();
			}
		}
		else
		{
			ratio = boundingSize.width() / size.width();
		}
	}
	else
	{
		if (size.height() > boundingSize.height())
		{
			ratio = boundingSize.height() / size.height();
		}
		else
		{
			ratio = 1.0;
		}
	}

	newSize = QSize(size.width() * ratio, size.height() * ratio);
	return QRectF(boundingSize.width() / 2.0 - newSize.width() / 2.0,
				  boundingSize.height() / 2.0 - newSize.height() / 2.0,
				  newSize.width(),
				  newSize.height());

	/*qreal w = 0;
	qreal h = 0;

	if (boundingSize.width() / boundingSize.height() > size.width() / size.height())
	{
		w = (size.width() / size.height()) * boundingSize.height();
		h = boundingSize.height();
	}
	else
	{
		w = boundingSize.width();
		h = boundingSize.width() / (size.width() / size.height());
	}

	return QSizeF(w, h);*/
}


QRectF PRSUtil::fitRatioToRect(QSizeF ratio, QRectF boundingRect)
{
	qreal brRatio = boundingRect.width() / boundingRect.height();
	qreal inRatio = ratio.width() / ratio.height();

	QSize outSize;

	if (brRatio > inRatio)
	{
		outSize.setHeight(boundingRect.height());
		outSize.setWidth(inRatio * boundingRect.height());
	}
	else
	{
		outSize.setWidth(boundingRect.width());
		outSize.setHeight(boundingRect.width() / inRatio);
	}

	return QRectF((boundingRect.left() + boundingRect.right()) / 2.0 - outSize.width() / 2.0,
				  (boundingRect.top() + boundingRect.bottom()) / 2.0 - outSize.height() / 2.0,
				  outSize.width(),
				  outSize.height());
}
