#include <stdexcept>

#include "makescene.h"

MVE_NAMESPACE_BEGIN

void
MakeScene::execute (void)
{
    if (this->input_dir.empty())
        throw std::invalid_argument("No input dir given!");
    if (this->output_dir.empty())
        throw std::invalid_argument("No output dir given!");

    this->views_path = this->output_dir + "/" MVE_VIEWS_DIR;
    this->bundle_path = this->input_dir + "/" MVE_BUNDLE_PATH;
}

MVE_NAMESPACE_END
