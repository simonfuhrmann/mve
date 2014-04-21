#ifndef SKY_KEYING_TAB_HEADER
#define SKY_KEYING_TAB_HEADER

#include <QWidget>
#include <QPushButton>
#include <QGraphicsView>
#include <QSpinBox>
#include <QColorDialog>
#include "mainwindowtab.h"
#include "mve/bilateral.h"
#include "mve/scene.h"
#include "mve/bundle.h"
#include "mve/image.h"

class SkyKeyingPlugin : public MainWindowTab {
    Q_OBJECT
#if QT_VERSION >= 0x050000
    Q_PLUGIN_METADATA(IID MainWindowTab_iid)
#endif
    Q_INTERFACES(MainWindowTab)

    public:
        SkyKeyingPlugin(QWidget* parent = NULL);
        virtual ~SkyKeyingPlugin();
        virtual QString get_title();
    private:
        void transformViewPointer(mve::View::Ptr);
        void displayImage(QImage *qImg);
        QGraphicsView *image;
        QSpinBox *spinbox;
        QColorDialog *colorDialog;
        mve::ByteImage::Ptr currentImagePointer;
        mve::FloatImage::Ptr depthMapPointer;
    private slots:
        void receiveViewPointer(mve::View::Ptr);
        void apply();
};

#endif /* SKY_KEYING_TAB_HEADER */
