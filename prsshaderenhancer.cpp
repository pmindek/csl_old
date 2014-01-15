#include "prsshaderenhancer.h"

PRSShaderEnhancer::PRSShaderEnhancer()
{
	this->histogramStorage = 0;
	this->fragmentStorage = 0;

	this->histogramX = 256;
	this->histogramY = 256;

	parameterCaptions << "0";
	parameterCaptions << "1";
}

PRSShaderEnhancer::~PRSShaderEnhancer()
{
	if (this->histogramStorage != 0)
		glDeleteTextures(1, &histogramStorage);

	if (this->fragmentStorage != 0)
		glDeleteTextures(1, &fragmentStorage);
}


QString PRSShaderEnhancer::insertCoreFunctionality(QString shader)
{
	QRegExp removeVersion("#version.*\\n");
	removeVersion.setMinimal(true);
	shader.remove(removeVersion);


	/*QStringList params;
	for (int i = 0; i < this->variables.count()  + this->maxVectorDim; i++)
	{
		params << "float param" + QString::number(i);
	}
	QString variablesParameters = params.join(", ");

	QString variablesArray = "";
	for (int i = 0; i < this->variables.count() + this->maxVectorDim; i++)
	{
		variablesArray += "	fv[" + QString::number(i) + "] = param" + QString::number(i) + ";\n";
	}*/


	QString codeFragment =
		"#version " + this->version + "\n"
		//"#extension GL_NV_gpu_shader5 : enable\n"
		"#extension GL_ARB_gpu_shader5 : enable\n"
		"#extension GL_EXT_gpu_shader4 : enable\n"
		"#extension GL_EXT_shader_image_load_store : enable\n"
		"\n"
		"uniform sampler2DArray PRSSelections;\n"
		"uniform int PRSSelectionCount;\n"
		"uniform int PRSScreenWidth;\n"
		"uniform int PRSScreenHeight;\n"
		"coherent uniform layout(size1x32) uimage2D PRSHistogram;\n"
		"\n"
		"int PRSTemp_int;\n"
		"float PRSTemp_float;\n"
		"\n"
		"float prsFragmentDOI()\n"
		"{\n"
		"	vec4 q = vec4(0);\n"
		"	for (int i = 0; i < PRSSelectionCount; i++)\n"
		"	{\n"
		"		q += texture2DArray(PRSSelections, vec3(gl_FragCoord.xy / vec2(PRSScreenWidth, PRSScreenHeight), float(i)));\n"
		"	}\n"
		"	return clamp(q, 0., 1.);\n"
		"}\n"
		"\n"
		"void prsExportData(float data, int count, int which)\n"
		"{\n"
		"	for (int ijk = 0; ijk < PRSSelectionCount; ijk++)\n"
		"	{\n"
		"		PRSTemp_float = texture2DArray(PRSSelections, vec3(gl_FragCoord.xy / vec2(PRSScreenWidth, PRSScreenHeight), float(ijk))).r;\n"
		"		if (PRSTemp_float <= 0.0) continue;\n"
		"	\n;"
		"		PRSTemp_int = int(floor(data * 255.0));\n"
		"		imageAtomicAdd(PRSHistogram, ivec2(PRSTemp_int, ijk * count + which), int(255.0 * PRSTemp_float));\n"
		"	}\n"
		"}\n";



	shader.insert(0, codeFragment);

	return shader;
}

QString PRSShaderEnhancer::enhance(QString shader, QString version)
{
	this->version = version;

	QString enhanced = this->insertCoreFunctionality(shader);

	return enhanced;
}

void PRSShaderEnhancer::init(QGLShaderProgram *shader, QGLShaderProgram *clearShader)
{
	this->shader = shader;
	this->clearShader = clearShader;

	this->parameterCount = 2;

	glGenTextures(1, &histogramStorage);
	glBindTexture(GL_TEXTURE_2D, histogramStorage);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, histogramX, histogramY, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, 0);
	((PFNGLBINDIMAGETEXTUREEXTPROC)wglGetProcAddress("glBindImageTextureEXT")) (4, histogramStorage, 0, false, 0, GL_READ_WRITE, GL_R32UI);
	shader->bind();
	shader->setUniformValue("PRSHistogram", 4);
	shader->release();

	if (clearShader != 0)
	{
		clearShader->bind();
		clearShader->setUniformValue("PRSHistogram", 4);
		clearShader->setUniformValue("PRSHistogramWidth", this->histogramX);
		clearShader->setUniformValue("PRSHistogramHeight", this->histogramY);
		clearShader->release();
	}
}

void PRSShaderEnhancer::clearStorage()
{
	if (clearShader == 0)
		return;

	glPushMatrix();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glViewport(0, 0, 1, 1);
	glOrtho(-10.0, 10.0, -10.0, 10.0, -1.0, +1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	this->clearShader->bind();
	glBegin(GL_POINTS);
		glVertex2d(1,0);
	glEnd();
	this->clearShader->release();

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	/*GLuint *d = (GLuint *) malloc(sizeof(GLuint) * histogramX * histogramY);
	for (int i = 0; i < histogramX * histogramY; i++)
	{
		d[i] = 0;
	}

	qDebug() << glGetError() << "CLEAR STORAGE";
	glBindTexture(GL_TEXTURE_2D, histogramStorage);
	qDebug() << glGetError() << "glTexImage2D";
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, histogramX, histogramY, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, d);
	qDebug() << glGetError() << "glTexImage2D";

	free(d);*/
}

void PRSShaderEnhancer::use(GLuint what, GLuint mouseX, GLuint mouseY)
{
	this->shader->setUniformValue("SA_USE", what);
	this->shader->setUniformValue("SA_X", mouseX);
	this->shader->setUniformValue("SA_Y", mouseY);
}

void PRSShaderEnhancer::unuse()
{
	this->shader->setUniformValue("SA_USE", 0);
}

void PRSShaderEnhancer::setPower(GLfloat power)
{
	this->shader->setUniformValue("SA_POWER", power);
}






GLuint PRSShaderEnhancer::getHistogramStorage()
{
	return this->histogramStorage;
}

GLuint PRSShaderEnhancer::getFragmentStorage()
{
	return this->fragmentStorage;
}

void PRSShaderEnhancer::prepareShader(GLuint unit, GLuint selectionMasksArray, int activeSelectionsCount)
{
	((PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0 + unit);

	//host context must be current!

	this->activeSelectionsCount = activeSelectionsCount;

	glBindTexture(GL_TEXTURE_2D_ARRAY, selectionMasksArray);
	shader->setUniformValue("PRSSelections", unit);
	//shader->setUniformValueArray("PRSSelectionsArray", selectionMasksArray.toVector(), qMin(selectionMasksArray.count(), 32) );
	shader->setUniformValue("PRSSelectionCount", this->activeSelectionsCount);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	shader->setUniformValue("PRSScreenWidth", viewport[2]);
	shader->setUniformValue("PRSScreenHeight", viewport[3]);

	((PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTexture"))(GL_TEXTURE0);
}

GLuint *PRSShaderEnhancer::getHistogramData()
{
	GLuint *d = (GLuint *) malloc(sizeof(GLuint) * histogramX * histogramY);
	glBindTexture(GL_TEXTURE_2D, this->histogramStorage);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, d);

	return d;
}

int PRSShaderEnhancer::getParameterCount()
{
	return this->parameterCount;
}

int PRSShaderEnhancer::getActiveSelectionsCount()
{
	return this->activeSelectionsCount;
}

int PRSShaderEnhancer::getHistogramSize()
{
	return this->histogramX;
}

QStringList PRSShaderEnhancer::getParameterCaptions()
{
	return this->parameterCaptions;
}
