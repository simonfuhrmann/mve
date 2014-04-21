#include <QtGui>
#include <QtCore>
#include <QPushButton>
#include <QGroupBox>
#include <QDebug>
#include <QColorDialog>
#include <QMenuBar>
#include <QAction>
#include <iostream>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QGraphicsView>

#include <chrono>

#include "scenemanager.h"
#include "sceneoverview.h"
#include "sky_keying_plugin.h"
#include "kuwahara.h"


// Returns the best sky color match for the current view, to be used as default color.
QColor getSkyColor() {
    // TODO: Implement
    return QColor(1, 3, 3, 7);
}

SkyKeyingPlugin::SkyKeyingPlugin (QWidget* parent) : MainWindowTab(parent) {
    QVBoxLayout *vbox = new QVBoxLayout(this);

    // Create new toolbar with icons
    QToolBar *toolbar = new QToolBar(this);
    QAction *colorButton = new QAction(QIcon::fromTheme("", QIcon("://img/chroma.png")), tr("Select chroma key"), this);
    QAction *previousButton = new QAction(QIcon::fromTheme("go-previous", QIcon("img/prev.png")), tr("Previous view"), this);
    QAction *nextButton = new QAction(QIcon::fromTheme("go-next", QIcon("img/next.png")), tr("Next view"), this);
    QAction *applyButton = new QAction(QIcon::fromTheme("tools-check-spelling", QIcon("img/folder-drag-accept.png")), tr("Apply"), this);
    this->spinbox = new QSpinBox();
    this->colorDialog = new QColorDialog();

    // Create image
    this->image = new QGraphicsView;
    QGraphicsScene *scene = new QGraphicsScene();

    scene->addPixmap(QPixmap());
    image->setScene(scene);

    /************************************
     * Default values
     ************************************/

    // Maximum threshold can be 255 (RGB)
    spinbox->setMaximum(255);
    spinbox->setMinimum(0);
    spinbox->setToolTip(tr("Threshold"));
    spinbox->setValue(30);
    colorDialog->setCurrentColor(getSkyColor());
    colorDialog->setModal(1);

    // Add icons, tooltips, parent information to toolbar
    toolbar->addAction(previousButton);
    toolbar->addAction(nextButton);
    toolbar->addAction(colorButton);
    toolbar->addWidget(spinbox);
    toolbar->addAction(applyButton);

    /************************************
     * Alignment, signals/slots
     ************************************/
    toolbar->setGeometry(10, 10, 200, 30);
    vbox->addWidget(toolbar);
    vbox->addWidget(this->image);

    connect(colorButton, SIGNAL(triggered()), colorDialog, SLOT(show()));
    connect(applyButton, SIGNAL(triggered()), this, SLOT(apply()));
    connect(spinbox, SIGNAL(valueChanged(int)), this, SLOT(apply()));
    connect(&SceneManager::get(), SIGNAL(view_selected(mve::View::Ptr)), this, SLOT(receiveViewPointer(mve::View::Ptr)));
}

/*
 * This function will both receive the view pointer that SceneManager emits,
 * and handle it by calling transformImage() on the view pointer.
 */
void SkyKeyingPlugin::receiveViewPointer(mve::View::Ptr viewPtr) {
    //std::size_t view_id = viewPtr->get_id();
    //std::cout << "View pointer received!" << std::endl;
    transformViewPointer(viewPtr);
}

/*
 * This function will transform a view pointer into an
 * RGB32 image that is usable for Qt.
 * After this is done, displayImage() will be called with the result.
 */
void SkyKeyingPlugin::transformViewPointer(mve::View::Ptr viewPtr) {
    if (viewPtr != NULL) {
        // TODO: Should we support more than "undistorted"?
        mve::ByteImage::Ptr img = viewPtr->get_byte_image("undistorted");
        this->currentImagePointer = img;
        mve::FloatImage::Ptr depthMapPointer = viewPtr->get_float_image("depth-L0");
        this->depthMapPointer = depthMapPointer;
        if (img != NULL) {
            std::size_t imageWidth = img->width();
            std::size_t imageHeight = img->height();
            //std::cout << imageWidth << ", " << imageHeight << std::endl;
            QImage *qImg = new QImage(imageWidth, imageHeight, QImage::Format_RGB32);
            #pragma omp parallel for collapse(2)
            for (std::size_t y = 0; y < imageHeight; ++y)
                for (std::size_t x = 0; x < imageWidth; ++x) {
                    unsigned int r = (unsigned int)(img->at(x, y, 0));
                    unsigned int g = (unsigned int)(img->at(x, y, 1));
                    unsigned int b = (unsigned int)(img->at(x, y, 2));
                    unsigned int rgb = r << 16 | g  << 8 | b;
                    qImg->setPixel(x, y, rgb);
                }
            //std::cout << "View pointer transformed!" << std::endl;
            displayImage(qImg);
        }
    }
}

/*
 * Takes an image and displays it in UMVE.
 */
void SkyKeyingPlugin::displayImage(QImage *qImg) {
    //std::cout << "Displaying image..." << std::endl;
    QGraphicsScene *scene = new QGraphicsScene();
    QPixmap pixmap = QPixmap::fromImage(*qImg);
    scene->addPixmap(pixmap);
    this->image->setScene(scene);
}


/*
 * Applies the current settings (color and threshold) to the image displayed in UMVE.
 */
void SkyKeyingPlugin::apply() {
    std::size_t threshold = this->spinbox->value();
    std::size_t imageWidth = this->image->scene()->width();
    std::size_t imageHeight = this->image->scene()->height();


    //mve::ByteImage::Ptr img = this->currentImagePointer;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    mve::ByteImage::Ptr imgKuwahara = smooth_kuwahara<uint8_t>(this->currentImagePointer, 5);
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    typedef std::chrono::duration<int,std::milli> millisecs_t;
    millisecs_t duration(std::chrono::duration_cast<millisecs_t>(end - start));
    std::cout << duration.count() << " ms\n";



    mve::FloatImage::Ptr dep = this->depthMapPointer;
    QImage *qImg = new QImage(imageWidth, imageHeight, QImage::Format_RGB32);
    #pragma omp parallel for collapse(2)
    for (std::size_t y = 0; y < imageHeight; ++y)
        for (std::size_t x = 0; x < imageWidth; ++x) {
            // Keep original colors if possible
            unsigned int r = (unsigned int)(imgKuwahara->at(x, y, 0)/* * dep->at(x, y, 0)*/);
            unsigned int g = (unsigned int)(imgKuwahara->at(x, y, 1)/* * dep->at(x, y, 0)*/);
            unsigned int b = (unsigned int)(imgKuwahara->at(x, y, 2)/* * dep->at(x, y, 0)*/);

            if (r < threshold || g < threshold || b < threshold) {
//                r = 0;
//                g = 0;
//                b = 0;
            }
            unsigned int rgb = r << 16 | g  << 8 | b;
            qImg->setPixel(x, y, rgb);
        }
    displayImage(qImg);
}

SkyKeyingPlugin::~SkyKeyingPlugin() {
}

QString SkyKeyingPlugin::get_title() {
    return tr("Sky Keying");
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(SkyKeyingPlugin, SkyKeyingPlugin)
#endif
