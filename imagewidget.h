#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include <QImage>

class ImageWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ImageWidget(QWidget *parent = 0);
	void setTitle(QString title);
	void setImage(QImage image);

protected:
	void paintEvent(QPaintEvent *);

	QImage image;
	QString title;

signals:

public slots:

};

#endif // HISTOWIDGET_H
