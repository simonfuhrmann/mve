#ifndef UMVE_ADDIN_AABB_CREATOR_HEADER
#define UMVE_ADDIN_AABB_CREATOR_HEADER

#include <QFormLayout>
#include <QDoubleSpinBox>

#include "scene_addins/addin_base.h"

class AddinAABBCreator : public AddinBase
{
    Q_OBJECT

public:
    AddinAABBCreator (void);
    QWidget* get_sidebar_widget (void);

private slots:
    void on_create_clicked (void);

private:
    QFormLayout* layout;
    QDoubleSpinBox* spins[6];
};

#endif /* UMVE_ADDIN_AABB_CREATOR_HEADER */
