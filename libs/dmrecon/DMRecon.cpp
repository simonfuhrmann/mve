#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <set>
#include <ctime>

#include "DMRecon.h"
#include "GlobalViewSelection.h"
#include "math/vector.h"
#include "mve/image.h"
#include "mve/imagetools.h"
#include "Settings.h"
#include "util/fs.h"
#include "util/string.h"

MVS_NAMESPACE_BEGIN


DMRecon::DMRecon(mve::Scene::Ptr _scene, Settings const& _settings)
    :
    scene(_scene),
    settings(_settings)
{
    mve::Scene::ViewList const& mve_views(scene->get_views());
    std::size_t refViewNr = settings.refViewNr;

    /* Check if master image exists */
    if (refViewNr >= mve_views.size() ||
        (mve_views[refViewNr].get() == NULL) ||
        (!mve_views[refViewNr]->is_camera_valid()))
    {
        std::cerr<<"ERROR: Invalid master view."<<std::endl;
        throw std::invalid_argument("Invalid master view");
    }

    /* Check for meaningful scale factor */
    if (settings.scale < 0.f)
        throw std::invalid_argument("Invalid scale factor.");

    /* Fetch bundle file. */
    try
    {
        this->bundle = this->scene->get_bundle();
    }
    catch (std::exception& e)
    {
        throw std::runtime_error(std::string("Error reading bundle file: ")
              + e.what());
    }

    /* Create SingleView Pointer List from mve views */
    views.resize(mve_views.size());
    for (std::size_t i = 0; i < mve_views.size(); ++i)
    {
        if (!mve_views[i].get() || !mve_views[i]->is_camera_valid())
            continue;
        mvs::SingleViewPtr sView(new mvs::SingleView(mve_views[i]));
        views[i] = sView;
    }
    SingleViewPtr refV = views[refViewNr];

    /* Prepare reconstruction */
    refV->loadColorImage(this->settings.imageEmbedding);
    refV->prepareRecon(settings.scale);
    mve::ImageBase::Ptr scaled_img = refV->getScaledImg();
    this->width = scaled_img->width();
    this->height = scaled_img->height();

    /* Create log file and write out some general MVS information */
    if (!settings.logPath.empty()) {
        // Create directory if necessary
        std::string logfn(settings.logPath);
        std::size_t pos = logfn.find_last_of("/");
        if (pos != logfn.length() - 1)
            logfn += "/";
        if (!util::fs::dir_exists(logfn.c_str()))
            if (!util::fs::mkdir(logfn.c_str()))
                if (!util::fs::dir_exists(logfn.c_str()))
                    throw std::runtime_error("Error creating directory: " + logfn);

        // Build log file name and open file
        std::string name(refV->createFileName(settings.scale));
        name += ".log";
        logfn += name;
        std::cout << "Creating log file at " << logfn
                  << std::endl;
        log.open(logfn.c_str());
        if (!log.good())
            throw std::runtime_error("Cannot open log file");
    }
    log << "MULTI-VIEW STEREO LOG FILE" << std::endl;
    log << "--------------------------" << std::endl;
    log << std::endl;
    log << "Data main path is " << scene->get_path() << std::endl;
    log << std::setw(20) << "Master image: "
        << std::setw(5) << settings.refViewNr << std::endl;
    log << std::setw(20) << "Global VS maximum: "
        << std::setw(5) << settings.globalVSMax << std::endl;
    log << std::setw(20) << "Use color scale: "
        << std::setw(5) << (settings.useColorScale ? "true" : "false")
        << std::endl;
}

DMRecon::~DMRecon()
{
    /* Close log file if it is open */
    if (log.is_open())
        log.close();
}

void DMRecon::start()
{
    progress.start_time = std::time(0);

    analyzeFeatures();
    globalViewSelection();
    processFeatures();
    processQueue();

    if (progress.cancelled) {
        progress.status = RECON_CANCELLED;
        return;
    }

    progress.status = RECON_SAVING;
    SingleViewPtr refV(views[settings.refViewNr]);
    if (settings.writePlyFile) {
        refV->saveReconAsPly(settings.plyPath, settings.scale);
    }
    refV->writeReconImages(settings.scale);
    progress.status = RECON_IDLE;

    // Output percentage of filled pixels
    {
        std::size_t nrPix = this->width * this->height;
        float percent = (float) progress.filled / (float) nrPix;
        std::cout << "Filled " << progress.filled << " pixels, i.e. "
                  << util::string::get_fixed(percent * 100.f, 1)
                  << " %." << std::endl;
        log << "Filled " << progress.filled << " pixels, i.e. "
              << util::string::get_fixed(percent * 100.f, 1)
              << " %." << std::endl;
    }

    // Output required time to process the image
    {
        size_t mvs_time = std::time(0) - progress.start_time;
        std::cout << "MVS took " << mvs_time << " seconds." << std::endl;
        log << "MVS took " << mvs_time << " seconds." << std::endl;

    }
}

/**  Attach features visible in reference view to other views where
     they are visible */
void DMRecon::analyzeFeatures()
{
    progress.status = RECON_FEATURES;

    SingleViewPtr refV = views[settings.refViewNr];

    mve::BundleFile::FeaturePoints const & features = bundle->get_points();

    for (std::size_t i = 0; i < features.size() && !progress.cancelled; ++i)
    {
        if (!features[i].contains_view_id(settings.refViewNr))
            continue;
        math::Vec3f featurePos(features[i].pos);
        if (!refV->pointInFrustum(featurePos))
            continue;
        for (std::size_t j = 0; j < features[i].refs.size(); ++j)
        {
            std::size_t id = features[i].refs[j].img_id;
            if (views[id] != NULL && views[id]->pointInFrustum(featurePos))
                views[id]->addFeature(i);
        }
    }
}

void DMRecon::globalViewSelection()
{
    progress.status = RECON_GLOBALVS;

    if (progress.cancelled)  return;
    // Initialize global view selection
    GlobalViewSelection globalVS(views, bundle->get_points(), settings);

    // Perform global view selection
    globalVS.performVS();

    // Load selected images + logging and output
    neighViews = globalVS.getSelectedIDs();
    std::stringstream ss;
    ss << "Global view selection took the following views: " << std::endl;
    IndexSet::const_iterator citID;
    for (citID = neighViews.begin(); citID != neighViews.end()
         && !progress.cancelled; ++citID)
    {
        ss << *citID << " ";
        views[*citID]->loadColorImage(this->settings.imageEmbedding);
        views[*citID]->createImagePyramid();
    }
    ss << std::endl;
    std::cout << ss.str();
    log << ss.str();
}

void DMRecon::processFeatures()
{
    progress.status = RECON_FEATURES;
    if (progress.cancelled)  return;
    SingleViewPtr refV = views[settings.refViewNr];
    mve::BundleFile::FeaturePoints const & features = bundle->get_points();

    /* select features that should be processed:
       take features only from master view for the moment */
    std::cout<<"Started to process "<<features.size()<<" features."<<std::endl;
    log<<"Started to process "<<features.size()<<" features."<<std::endl;
    size_t success = 0, processed = 0;

    for (size_t i = 0; i < features.size() && !progress.cancelled; ++i)
    {
        bool useFeature = false;
        if (features[i].contains_view_id(settings.refViewNr))
            useFeature = true;
        else {
            IndexSet::const_iterator id = neighViews.begin();
            while (id != neighViews.end()) {
                if (features[i].contains_view_id(*id)) {
                    useFeature = true;
                    break;
                }
                ++id;
            }
        }
        if (!useFeature)
            continue;
        math::Vec3f featPos(features[i].pos);
        if (!refV->pointInFrustum(featPos)) {
            continue;
        }
        math::Vec2f pixPosF = refV->worldToScreen(featPos);
        std::size_t x = round(pixPosF[0]);
        std::size_t y = round(pixPosF[1]);
        float initDepth = (featPos - refV->camPos).norm();
        PatchOptimization patch(views, settings, x, y, initDepth,
            0.f, 0.f, neighViews, IndexSet());
        patch.doAutoOptimization();
        ++processed;
        float conf = patch.computeConfidence();
        size_t index = y * this->width + x;
        if (conf == 0) {
            continue;
        }

        // optimization was successful:
        ++success;
        float depth = patch.getDepth();
        math::Vec3f normal = patch.getNormal();
        if (refV->confImg->at(index) < conf) {
            if (refV->confImg->at(index) <= 0) {
                ++progress.filled;
            }
            refV->depthImg->at(index) = depth;
            refV->normalImg->at(index, 0) = normal[0];
            refV->normalImg->at(index, 1) = normal[1];
            refV->normalImg->at(index, 2) = normal[2];
            refV->dzImg->at(index, 0) = patch.getDzI();
            refV->dzImg->at(index, 1) = patch.getDzJ();
            refV->confImg->at(index) = conf;
            QueueData tmpData;
            tmpData.confidence = conf;
            tmpData.depth = depth;
            tmpData.dz_i = patch.getDzI();
            tmpData.dz_j = patch.getDzJ();
            tmpData.localViewIDs = patch.getLocalViewIDs();
            tmpData.x = x;
            tmpData.y = y;
            prQueue.push(tmpData);
        }
    }
    std::cout << "Processed " << processed << " features, from which "
              << success << " succeeded optimization." << std::endl;
    log << "Processed " << processed << " features, from which "
          << success << " succeeded optimization." << std::endl;
}

void
DMRecon::processQueue()
{
    progress.status = RECON_QUEUE;
    if (progress.cancelled)  return;

    SingleViewPtr refV = this->views[settings.refViewNr];

    std::cout << "Process queue ..." << std::endl;
    log << "Process queue ..." << std::endl;
    size_t count = 0, lastStatus = 1;
    progress.queueSize = prQueue.size();
    std::cout << "Count: " << std::setw(8) << count
              << "  filled: " << std::setw(8) << progress.filled
              << "  Queue: " << std::setw(8) << progress.queueSize
              << std::endl;
    log << "Count: " << std::setw(8) << count
          << "  filled: " << std::setw(8) << progress.filled
          << "  Queue: " << std::setw(8) << progress.queueSize
          << std::endl;
    lastStatus = progress.filled;

    while (!prQueue.empty() && !progress.cancelled)
    {
        progress.queueSize = prQueue.size();
        if ((progress.filled % 1000 == 0) && (progress.filled != lastStatus))
        {
            std::cout << "Count: " << std::setw(8) << count
                      << "  filled: " << std::setw(8) << progress.filled
                      << "  Queue: " << std::setw(8) << progress.queueSize
                      << std::endl;
            log << "Count: " << std::setw(8) << count
                  << "  filled: " << std::setw(8) << progress.filled
                  << "  Queue: " << std::setw(8) << progress.queueSize
                  << std::endl;
            lastStatus = progress.filled;
        }
        QueueData tmpData = prQueue.top();
        prQueue.pop();
        ++count;
        float x = tmpData.x;
        float y = tmpData.y;
        std::size_t index = y * this->width + x;
        if (refV->confImg->at(index) > tmpData.confidence) {
            continue ;
        }
        PatchOptimization patch(views, settings, x, y, tmpData.depth,
            tmpData.dz_i, tmpData.dz_j, neighViews, tmpData.localViewIDs);
        patch.doAutoOptimization();
        tmpData.confidence = patch.computeConfidence();
        if (tmpData.confidence == 0) {
            continue;
        }

        float new_depth = patch.getDepth();
        tmpData.depth = new_depth;
        tmpData.dz_i = patch.getDzI();
        tmpData.dz_j = patch.getDzJ();
        math::Vec3f normal = patch.getNormal();
        tmpData.localViewIDs = patch.getLocalViewIDs();
        if (refV->confImg->at(index) <= 0) {
            ++progress.filled;
        }
        if (refV->confImg->at(index) < tmpData.confidence) {
            refV->depthImg->at(index) = tmpData.depth;
            refV->normalImg->at(index, 0) = normal[0];
            refV->normalImg->at(index, 1) = normal[1];
            refV->normalImg->at(index, 2) = normal[2];
            refV->dzImg->at(index, 0) = tmpData.dz_i;
            refV->dzImg->at(index, 1) = tmpData.dz_j;
            refV->confImg->at(index) = tmpData.confidence;

            // left
            tmpData.x = x - 1; tmpData.y = y;
            index = tmpData.y * this->width + tmpData.x;
            if (refV->confImg->at(index) < tmpData.confidence - 0.05f ||
                refV->confImg->at(index) == 0.f)
            {
                prQueue.push(tmpData);
            }
            // right
            tmpData.x = x + 1; tmpData.y = y;
            index = tmpData.y * this->width + tmpData.x;
            if (refV->confImg->at(index) < tmpData.confidence - 0.05f ||
                refV->confImg->at(index) == 0.f)
            {
                prQueue.push(tmpData);
            }
            // top
            tmpData.x = x; tmpData.y = y - 1;
            index = tmpData.y * this->width + tmpData.x;
            if (refV->confImg->at(index) < tmpData.confidence - 0.05f ||
                refV->confImg->at(index) == 0.f)
            {
                prQueue.push(tmpData);
            }
            // bottom
            tmpData.x = x; tmpData.y = y + 1;
            index = tmpData.y * this->width + tmpData.x;
            if (refV->confImg->at(index) < tmpData.confidence - 0.05f ||
                refV->confImg->at(index) == 0.f)
            {
                prQueue.push(tmpData);
            }
        }
    }
}


MVS_NAMESPACE_END
