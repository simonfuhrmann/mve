#include <gtest/gtest.h>

#include <cstdio>
#include <iomanip>
#include <sstream>
#include <string>

#include "mve/bundle_io.h"
#include "mve/scene.h"
#include "util/exception.h"
#include "util/file_system.h"

namespace {

/* Cleanup mechanism. Unlinks files on destruction. */
class OnScopeExit final
{
public:
    ~OnScopeExit (void);
    void unlink (std::string const& str);

private:
    void unlink_recursive (std::string const& str);

private:
    std::vector<std::string> paths;
};

std::string create_scene_on_disk (std::size_t const view_count,
    mve::Bundle::Ptr const bundle, OnScopeExit* const clean_up);
mve::Scene::Ptr scene_with_dirty_bundle_and_view (OnScopeExit* const clean_up);
void make_dirty (mve::View::Ptr view);
void make_a_clean_view_dirty (mve::Scene::Ptr scene);
mve::Scene::ViewList load_views_from (std::string const& scene_path);
mve::Bundle::Ptr load_bundle_from (std::string const& scene_path);
mve::Bundle::Ptr make_bundle (std::size_t const camera_count);
bool views_match (mve::Scene::ViewList const& lhs,
    mve::Scene::ViewList const& rhs);
bool bundle_cameras_match (mve::Bundle::Ptr lhs, mve::Bundle::ConstPtr rhs);

} // anonymous namespace

//== Test the initial state of a created scene =================================

TEST(SceneTest, ACreatedSceneIsInitialyClean)
{
    OnScopeExit clean_up;

    std::string scene_path = create_scene_on_disk(0, nullptr, &clean_up);
    mve::Scene::Ptr scene = mve::Scene::create(scene_path);
    ASSERT_TRUE(scene != nullptr);
    EXPECT_FALSE(scene->is_dirty());
}

TEST(SceneTest, TheInitialPathOfACreatedSceneIsThePathItWasCreatedWith)
{
    OnScopeExit clean_up;

    std::string scene_path = create_scene_on_disk(0, make_bundle(0), &clean_up);
    mve::Scene::Ptr scene = mve::Scene::create(scene_path);
    EXPECT_EQ(scene_path, scene->get_path());
}

TEST(SceneTest, TheInitialViewsOfACreatedSceneMatchWithThatSceneOnDisk)
{
    using ViewList = mve::Scene::ViewList;
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_without_views = mve::Scene::create(
        create_scene_on_disk(0, make_bundle(5), &clean_up));
    EXPECT_EQ(scene_without_views->get_views().size(), std::size_t(0));

    mve::Scene::Ptr scene_with_views = mve::Scene::create(
        create_scene_on_disk(73, make_bundle(23), &clean_up));
    ViewList views_on_disk = load_views_from(scene_with_views->get_path());
    EXPECT_TRUE(views_match(views_on_disk, scene_with_views->get_views()));
}

TEST(SceneTest, TheInitialBundleOfACreatedSceneMatchesWithThatSceneOnDisk)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_empty_bundle = mve::Scene::create(
        create_scene_on_disk(0, make_bundle(0), &clean_up));
    EXPECT_TRUE(bundle_cameras_match(
        load_bundle_from(scene_with_empty_bundle->get_path()),
        scene_with_empty_bundle->get_bundle()));

    mve::Scene::Ptr scene_with_non_empty_bundle = mve::Scene::create(
        create_scene_on_disk(3, make_bundle(23), &clean_up));
    EXPECT_TRUE(bundle_cameras_match(
        load_bundle_from(scene_with_non_empty_bundle->get_path()),
        scene_with_non_empty_bundle->get_bundle()));
}

//== Creating a scene with missing files or directories ========================

TEST(SceneTest, CreateSceneThrowsAnExceptionIfTheDirectoryDoesNotExist)
{
    std::string not_a_directory = std::tmpnam(nullptr);
    EXPECT_THROW(mve::Scene::create(not_a_directory), util::Exception);
}

TEST(SceneTest, CreateSceneThrowsAnExceptionIfTheViewsSubdirectoryDoesNotExist)
{
    OnScopeExit clean_up;

    std::string dir_with_no_views_subdir;
    {
        dir_with_no_views_subdir = std::tmpnam(nullptr);
        util::fs::mkdir(dir_with_no_views_subdir.c_str());
        clean_up.unlink(dir_with_no_views_subdir);
        std::string bundle_file = util::fs::join_path(
            dir_with_no_views_subdir, "synth_0.out");
        mve::save_mve_bundle(make_bundle(0), bundle_file);
    }
    EXPECT_THROW(mve::Scene::create(dir_with_no_views_subdir), util::Exception);
}

TEST(SceneTest, CreatingASceneOnADirectoryWithNoBundleFileMakesGetBundleThrow)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_missing_bundle =
        mve::Scene::create(create_scene_on_disk(0, nullptr, &clean_up));
    EXPECT_THROW(scene_missing_bundle->get_bundle(), util::Exception);
}

//== Test loading into an existing scene =======================================

TEST(SceneTest, WhenLoadIsCalledOnASceneItsPathUpdatesAccordingly)
{
    OnScopeExit clean_up;

    std::string directory = create_scene_on_disk(0, make_bundle(3), &clean_up);
    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(13, make_bundle(3), &clean_up));
    scene->load_scene(directory);
    EXPECT_EQ(directory, scene->get_path());
}

TEST(SceneTest, WhenLoadIsCalledOnASceneItsViewsUpdateAccordingly)
{
    using Views = mve::Scene::ViewList;
    OnScopeExit clean_up;

    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(13, make_bundle(3), &clean_up));
    std::string path = create_scene_on_disk(9, make_bundle(4), &clean_up);
    scene->load_scene(path);
    Views views_from_disk = load_views_from(path);
    EXPECT_TRUE(views_match(views_from_disk, scene->get_views()));
}

TEST(SceneTest, WhenLoadIsCalledOnASceneItsBundleUpdatesAccordingly)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(13, make_bundle(0), &clean_up));
    std::string path = create_scene_on_disk(0, make_bundle(5), &clean_up);
    scene->load_scene(path);
    mve::Bundle::Ptr bundle_from_disk = load_bundle_from(path);
    EXPECT_TRUE(bundle_cameras_match(bundle_from_disk, scene->get_bundle()));
}

//== Loading a scene with missing files or directories =========================

TEST(SceneTest, LoadThrowsAnExceptionIfTheDirectoryDoesNotExist)
{
    OnScopeExit clean_up;

    std::string not_a_directory = std::tmpnam(nullptr);
    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(0, make_bundle(0), &clean_up));
    EXPECT_THROW(scene->load_scene(not_a_directory), util::Exception);
}

TEST(SceneTest, LoadThrowsAnExceptionIfTheViewsSubdirectoryDoesNotExist)
{
    OnScopeExit clean_up;

    std::string directory_with_no_views_subdir;
    {
        directory_with_no_views_subdir = std::tmpnam(nullptr);
        util::fs::mkdir(directory_with_no_views_subdir.c_str());
        clean_up.unlink(directory_with_no_views_subdir);
        std::string bundle_file = util::fs::join_path(
            directory_with_no_views_subdir, "synth_0.out");
        mve::save_mve_bundle(make_bundle(0), bundle_file);
    }
    std::string not_a_directory = std::tmpnam(nullptr);
    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(0, make_bundle(0), &clean_up));
    EXPECT_THROW(scene->load_scene(directory_with_no_views_subdir),
        util::Exception);
}

TEST(SceneTest, LoadingFromADirectoryWithNoBundleFileMakesGetBundleThrow)
{
    OnScopeExit clean_up;

    std::string directory_missing_bundle_file =
        create_scene_on_disk(0, nullptr, &clean_up);
    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(0, make_bundle(0), &clean_up));
    scene->load_scene(directory_missing_bundle_file);
    EXPECT_THROW(scene->get_bundle(), util::Exception);
}

//== Test saving onto disk =====================================================

TEST(SceneTest, WhenSaveIsCalledOnASceneTheSceneOnDiskUpdatesAccordingly)
{
    using ViewList = mve::Scene::ViewList;
    OnScopeExit clean_up;

    mve::Scene::Ptr dirty_scene = scene_with_dirty_bundle_and_view(&clean_up);
    dirty_scene->save_scene();

    mve::Bundle::Ptr loaded_bundle = load_bundle_from(dirty_scene->get_path());
    ViewList loaded_views = load_views_from(dirty_scene->get_path());
    EXPECT_TRUE(bundle_cameras_match(loaded_bundle, dirty_scene->get_bundle()));
    EXPECT_TRUE(views_match(loaded_views, dirty_scene->get_views()));
}

TEST(SceneTest, WhenSaveBundleIsCalledOnASceneOnlyTheBundleIsUpdatedOnDisk)
{
    using ViewList = mve::Scene::ViewList;
    OnScopeExit clean_up;

    mve::Scene::Ptr dirty_scene = scene_with_dirty_bundle_and_view(&clean_up);
    dirty_scene->save_bundle();

    mve::Bundle::Ptr loaded_bundle = load_bundle_from(dirty_scene->get_path());
    ViewList loaded_views = load_views_from(dirty_scene->get_path());
    EXPECT_TRUE(bundle_cameras_match(loaded_bundle, dirty_scene->get_bundle()));
    EXPECT_FALSE(views_match(loaded_views, dirty_scene->get_views()));
}

TEST(SceneTest, WhenSaveViewsIsCalledOnASceneOnlyTheViewsAreUpdatedOnDisk)
{
    using ViewList = mve::Scene::ViewList;
    OnScopeExit clean_up;

    mve::Scene::Ptr dirty_scene = scene_with_dirty_bundle_and_view(&clean_up);
    dirty_scene->save_views();

    mve::Bundle::Ptr loaded_bundle = load_bundle_from(dirty_scene->get_path());
    ViewList loaded_views = load_views_from(dirty_scene->get_path());
    EXPECT_FALSE(bundle_cameras_match(loaded_bundle,
        dirty_scene->get_bundle()));
    EXPECT_TRUE(views_match(loaded_views, dirty_scene->get_views()));
}

//== Test resetting a scene's bundle ===========================================

TEST(SceneTest, ResetBundleRestoresTheBundleToItsStateOnDisk)
{
    OnScopeExit clean_up;

    std::string path = create_scene_on_disk(13, make_bundle(15), &clean_up);
    mve::Scene::Ptr scene_with_dirty_bundle  = mve::Scene::create(path);
    scene_with_dirty_bundle->set_bundle(make_bundle(0));

    scene_with_dirty_bundle->reset_bundle();
    EXPECT_TRUE(bundle_cameras_match(
        load_bundle_from(scene_with_dirty_bundle->get_path()),
        scene_with_dirty_bundle->get_bundle()));
}

//== Test the dirty state of a scene ===========================================

TEST(SceneTest, ACleanSceneBecomesDirtyIfAnyOfItsViewsBecomeDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr clean_scene = mve::Scene::create(
        create_scene_on_disk(10, make_bundle(8), &clean_up));
    make_a_clean_view_dirty(clean_scene);
    EXPECT_TRUE(clean_scene->is_dirty());
}

TEST(SceneTest, SetBundleMakesACleanSceneDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr clean_scene =  mve::Scene::create(
        create_scene_on_disk(5, mve::Bundle::create(), &clean_up));
    clean_scene->set_bundle(mve::Bundle::create());
    EXPECT_TRUE(clean_scene->is_dirty());
}

TEST(SceneTest, ADirtySceneRemainsDirtyWhenMoreOfItsElementsBecomeDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_dirty_view = mve::Scene::create(
        create_scene_on_disk(7, make_bundle(3), &clean_up));
    make_a_clean_view_dirty(scene_with_dirty_view);

    scene_with_dirty_view->set_bundle(make_bundle(0));
    EXPECT_TRUE(scene_with_dirty_view->is_dirty());

    make_a_clean_view_dirty(scene_with_dirty_view);
    EXPECT_TRUE(scene_with_dirty_view->is_dirty());
}

TEST(SceneTest, SavingADirtySceneCleansIt)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr dirty_scene = scene_with_dirty_bundle_and_view(&clean_up);
    dirty_scene->save_scene();
    EXPECT_FALSE(dirty_scene->is_dirty());
}

TEST(SceneTest, SaveViewsCleansASceneIfOnlyItsViewsAreDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_dirty_views =
        mve::Scene::create(create_scene_on_disk(4, make_bundle(4), &clean_up));
    for (mve::View::Ptr& view : scene_with_dirty_views->get_views())
        make_dirty(view);

    scene_with_dirty_views->save_views();
    EXPECT_FALSE(scene_with_dirty_views->is_dirty());
}

TEST(SceneTest, SaveViewsDoesNotCleanASceneIfItsBundleIsDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_dirty_bundle = mve::Scene::create(
        create_scene_on_disk(5, make_bundle(7), &clean_up));
    scene_with_dirty_bundle->set_bundle(make_bundle(6));

    scene_with_dirty_bundle->save_views();
    EXPECT_TRUE(scene_with_dirty_bundle->is_dirty());
}

TEST(SceneTest, SaveBundleCleansASceneIfOnlyItsBundleIsDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_dirty_bundle = mve::Scene::create(
        create_scene_on_disk(10, make_bundle(3), &clean_up));
    scene_with_dirty_bundle->set_bundle(mve::Bundle::create());

    scene_with_dirty_bundle->save_bundle();
    EXPECT_FALSE(scene_with_dirty_bundle->is_dirty());
}

TEST(SceneTest, SaveBundleDoesNotCleanASceneIfAnyOfItsViewsAreDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr dirty_scene = scene_with_dirty_bundle_and_view(&clean_up);
    dirty_scene->save_views();
    EXPECT_TRUE(dirty_scene->is_dirty());
}

TEST(SceneTest, ResetBundleCleansASceneIfOnlyItsBundleIsDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_dirty_bundle = mve::Scene::create(
        create_scene_on_disk(10, make_bundle(3), &clean_up));
    scene_with_dirty_bundle->set_bundle(mve::Bundle::create());

    scene_with_dirty_bundle->reset_bundle();
    EXPECT_FALSE(scene_with_dirty_bundle->is_dirty());
}

TEST(SceneTest, ResetBundleDoesNotCleanASceneIfAnyOfitsViewsIsDirty)
{
    OnScopeExit clean_up;

    mve::Scene::Ptr dirty_scene = scene_with_dirty_bundle_and_view(&clean_up);
    dirty_scene->reset_bundle();
    EXPECT_TRUE(dirty_scene->is_dirty());
}

TEST(SceneTest, SavingTheDirtyViewsOfASceneCleansTheSceneIfItsBundleIsClean)
{
    using View = mve::View::Ptr;
    OnScopeExit clean_up;

    mve::Scene::Ptr scene_with_dirty_views = mve::Scene::create(
        create_scene_on_disk(10, make_bundle(6), &clean_up));
    for (int i = 0; i < 5; ++i)
        make_a_clean_view_dirty(scene_with_dirty_views);

    for (View& view : scene_with_dirty_views->get_views())
        if (view->is_dirty())
            view->save_view();

    EXPECT_FALSE(scene_with_dirty_views->is_dirty());
}

//== End of tests ==============================================================

namespace {

std::string
create_scene_on_disk (std::size_t const view_count,
    mve::Bundle::Ptr const bundle, OnScopeExit* const on_scope_exit)
{
    namespace fs = util::fs;

    std::string scene_directory = std::string(std::tmpnam(nullptr))
        + std::string("_test_scene");
    std::string bundle_file = fs::join_path(scene_directory, "synth_0.out");
    std::string views_directory = fs::join_path(scene_directory, "views");

    fs::mkdir(scene_directory.c_str());
    on_scope_exit->unlink(scene_directory); // Schedules for cleanup.
    fs::mkdir(views_directory.c_str());

    for (std::size_t i = 0; i < view_count; ++i)
    {
        std::string const view_directory_path = [&i, &views_directory]()
        {
            std::stringstream stream;
            stream << fs::join_path(views_directory, "view_")
                << std::setw(4) << std::setfill('0') << std::to_string(i)
                << ".mve";
            return stream.str();
        }();
        fs::mkdir(view_directory_path.c_str());
        mve::View::Ptr view = mve::View::create();
        view->set_name("view" + std::to_string(i));
        view->set_id(i);
        view->save_view_as(view_directory_path);
    }

    if (bundle != nullptr)
        mve::save_mve_bundle(bundle, bundle_file);

    return scene_directory;
}

void
make_dirty (mve::View::Ptr view)
{
    view->set_name(view->get_name() + 'a');
    assert(view->is_dirty());
}

void
make_a_clean_view_dirty (mve::Scene::Ptr scene)
{
    using ViewList = mve::Scene::ViewList;
    ViewList& views = scene->get_views();
    ViewList::iterator clean_view = std::find_if(views.begin(), views.end(),
        [](mve::View::Ptr& view) { return !view->is_dirty(); });
    make_dirty(*clean_view);
}

mve::Scene::ViewList
load_views_from (std::string const& scene_directory)
{
    std::string view_directory = util::fs::join_path(scene_directory, "views");
    util::fs::Directory directory(view_directory);
    mve::Scene::ViewList list;
    list.reserve(directory.size());

    for (util::fs::File const& file : directory)
        list.push_back(mve::View::create(file.get_absolute_name()));

    return list;
}

mve::Bundle::Ptr
load_bundle_from (std::string const& scene_directory)
{
    std::string file_path = util::fs::join_path(scene_directory, "synth_0.out");
    return mve::load_mve_bundle(file_path);
}

mve::Bundle::Ptr
make_bundle (std::size_t const camera_count)
{
    mve::Bundle::Ptr bundle = mve::Bundle::create();
    bundle->get_cameras().reserve(camera_count);
    for (size_t i = 1; i <= camera_count; ++i)
    {
        mve::CameraInfo camera;
        camera.flen = (i % 3 == 1) ? 0 : 1.0f + 2.0f / i;
        camera.trans[0] = i - 10.0f;
        camera.trans[1] = 1.0f / i;
        camera.trans[2] = 10.0f - i;
        camera.paspect = 0.5f + 1.0f / i;
        bundle->get_cameras().push_back(std::move(camera));
    }

    return bundle;
}

bool
views_match (mve::Scene::ViewList const& lhs, mve::Scene::ViewList const& rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (mve::View::Ptr const& left_view : lhs)
    {
        mve::Scene::ViewList::const_iterator found = std::find_if(
            rhs.begin(), rhs.end(),
            [&left_view](mve::View::Ptr const& right_view)
            { return left_view->get_id() == right_view->get_id(); });
        if (found == rhs.end() || left_view->get_name() != (*found)->get_name())
            return false;
    }

    return true;
}

bool
bundle_cameras_match (mve::Bundle::Ptr lhs, mve::Bundle::ConstPtr rhs)
{
    auto constexpr is_invallid = [](mve::CameraInfo const& camera) -> bool
    {
        return camera.flen == 0.0f;
    };

    auto constexpr match = [](float l, float r) -> bool
    {
        constexpr float epsilon = 1e-3f;
        return (r == 0.0f) ? l < epsilon : (std::abs((l / r) - 1.0f) < epsilon);
    };

    auto const match_n =
        [&match](float const l[], float const r[], std::size_t count) -> bool
    {
        for (std::size_t i = 0; i < count; ++i)
            if (!match(l[i], r[i]))
                return false;
        return true;
    };

    if (lhs->get_cameras().size() != rhs->get_cameras().size()
        || lhs->get_features().size() != rhs->get_features().size())
        return false;

    for (std::size_t i = 0; i < lhs->get_cameras().size(); ++i)
    {
        mve::CameraInfo const& left_cam = lhs->get_cameras()[i];
        mve::CameraInfo const& right_cam = rhs->get_cameras()[i];

        if (is_invallid(left_cam))
            return is_invallid(right_cam);

        if (!match(left_cam.flen, right_cam.flen)
            || !match_n(left_cam.dist, right_cam.dist, 2)
            || !match_n(left_cam.trans, right_cam.trans, 3)
            || !match_n(left_cam.rot, right_cam.rot, 9))
            return false;
    }

    return true;
}

OnScopeExit::~OnScopeExit (void)
{
    bool exception_happened = false;
    for (std::string const& path : this->paths)
    {
        try
        {
            this->unlink_recursive(path);
        } catch (...)
        {
            exception_happened = true;
            std::cout <<  "Exception during cleanup!" << std::endl;
        }
    }
    if (exception_happened)
        throw std::runtime_error("Exception during file cleanup.");
}

void
OnScopeExit::unlink (std::string const& str)
{
    this->paths.push_back(str);
}

void
OnScopeExit::unlink_recursive (std::string const& path)
{
    namespace fs = util::fs;
    if (fs::file_exists(path.c_str()))
    {
       fs::unlink(path.c_str());
    }
    else if (fs::dir_exists(path.c_str()))
    {
        fs::Directory directory(path);
        for (fs::File const& node : directory)
            unlink_recursive(node.get_absolute_name());
        fs::rmdir(path.c_str());
    }
}

mve::Scene::Ptr
scene_with_dirty_bundle_and_view (OnScopeExit* const clean_up) {
    mve::Scene::Ptr scene = mve::Scene::create(
        create_scene_on_disk(10, make_bundle(1), clean_up));
    make_a_clean_view_dirty(scene);
    scene->set_bundle(make_bundle(0));
    return scene;
}

} // anonymous namespace