/*
 * Helper functionality to generate a new scene.
 * Written by Simon Fuhrmann.
 */

#ifndef MVE_MAKESCENE_HEADER
#define MVE_MAKESCENE_HEADER

#include <string>

#include "defines.h"

#define MVE_VIEWS_DIR "views/"
#define MVE_BUNDLE_PATH "bundle/"
#define MVE_PS_BUNDLE_LOG "coll.log"
#define MVE_PS_IMAGE_DIR "images/"
#define MVE_PS_UNDIST_DIR "undistorted/"
#define MVE_NOAH_BUNDLE_LIST "list.txt"
#define MVE_NOAH_IMAGE_DIR ""

MVE_NAMESPACE_BEGIN

class MakeScene
{
private:
    /* Config values. */
    std::string input_dir;
    std::string output_dir;
    unsigned int bundle_id;
    bool keep_invalid;
    bool import_original;
    bool images_only;

    /* Computed values. */
    std::string bundle_path;
    std::string views_path;

public:
    MakeScene (void);

    /** Sets the directory to import data from. */
    void set_input_dir (std::string const& input_dir);
    /** Sets the directory of the scene that is to be created. */
    void set_output_dir (std::string const& output_dir);
    /** Sets whether to also imports views with invalid cameras. */
    void set_keep_invalid (bool value);
    /** Sets whether original images (not only undistorted) are imported. */
    void set_import_original (bool value);
    /** Sets whether input_dir points to images only. */
    void set_images_only (bool value);
    /** Sets the ID of the bundle to be imported. */
    void set_bundle_id (int bundle_id);

    /** Starts the import operation. */
    void execute (void);
};

/* ---------------------------------------------------------------- */

inline
MakeScene::MakeScene (void)
    : bundle_id(0)
    , keep_invalid(false)
    , import_original(false)
    , images_only(false)
{
}

inline void
MakeScene::set_input_dir (std::string const& input_dir)
{
    this->input_dir = input_dir;
}

inline void
MakeScene::set_output_dir (std::string const& output_dir)
{
    this->output_dir = output_dir;
}

inline void
MakeScene::set_keep_invalid (bool value)
{
    this->keep_invalid = value;
}

inline void
MakeScene::set_import_original (bool value)
{
    this->import_original = value;
}

inline void
MakeScene::set_images_only (bool value)
{
    this->images_only = value;
}

inline void
MakeScene::set_bundle_id (int bundle_id)
{
    this->bundle_id = bundle_id;
}

MVE_NAMESPACE_END

#endif /* MVE_MAKESCENE_HEADER */
