#include "mve/image_tools.h"
#include "sfm/bundler_features.h"

SFM_NAMESPACE_BEGIN
SFM_BUNDLER_NAMESPACE_BEGIN

void
Features::compute (mve::Scene::Ptr scene, FeatureType type)
{
    /* Iterate the scene and check if recomputing is necessary. */
    mve::Scene::ViewList const& views = scene->get_views();
//#pragma omp parallel for schedule(dynamic)
    for (std::size_t i = 0; i < views.size(); ++i)
    {
        if (views[i] == NULL)
            continue;

        mve::View::Ptr view = views[i];
        if (!this->opts.force_recompute
            && view->has_data_embedding(this->opts.feature_embedding))
            continue;

        mve::ByteImage::Ptr image = view->get_byte_image
            (this->opts.image_embedding);
        if (image == NULL)
            continue;

        std::cout << "Computing features for view ID " << i
            << " (" << image->width() << "x" << image->height()
            << ")..." << std::endl;

        /* Rescale image until maximum image size is met. */
        bool was_scaled = false;
        while (image->width() * image->height() > this->opts.max_image_size)
        {
            image = mve::image::rescale_half_size<uint8_t>(image);
            was_scaled = true;
        }
        if (was_scaled)
        {
            std::cout << "  scaled to " << image->width() << "x"
                << image->height() << " pixels." << std::endl;
        }

        /* Compute features for the image. */
        mve::ByteImage::Ptr descr_data;
        switch (type)
        {
            case SIFT_FEATURES:
            {
                Sift sift(this->opts.sift_options);
                sift.set_image(image);
                sift.process();
                descr_data = descriptors_to_embedding(sift.get_descriptors(),
                    image->width(), image->height());
                break;
            }

            case SURF_FEATURES:
            {
                Surf surf(this->opts.surf_options);
                surf.set_image(image);
                surf.process();
                descr_data = descriptors_to_embedding(surf.get_descriptors(),
                    image->width(), image->height());
                break;
            }

            default:
                throw std::runtime_error("Invalid switch case");
        }

        /* Save descriptors in view. */
        view->set_data(this->opts.feature_embedding, descr_data);
        if (!this->opts.skip_saving_views)
            view->save_mve_file();
        view->cache_cleanup();
    }
}

/* ----------------- Serialization of Descriptors ----------------- */

#define DESCR_SIGNATURE "MVE_DESCRIPTORS\n"
#define DESCR_SIGNATURE_LEN 16

template <typename DESC, int LEN>
mve::ByteImage::Ptr
serialize_descriptors (std::vector<DESC> const& descriptors,
    int image_width, int image_height)
{
    /* Allocate storate for the descriptors. */
    std::size_t size_of_header = DESCR_SIGNATURE_LEN + 3 * sizeof(int32_t);
    std::size_t size_per_descriptor = 4 * sizeof(float) + LEN * sizeof(float);
    mve::ByteImage::Ptr data = mve::ByteImage::create
        (size_of_header + descriptors.size() * size_per_descriptor, 1, 1);

    /* Output stream to memory. */
    std::stringstream out;
    out.rdbuf()->pubsetbuf(data->get_byte_pointer(), data->get_byte_size());

    /* Write header. */
    int32_t num_descr = static_cast<int32_t>(descriptors.size());
    int32_t img_width = static_cast<int32_t>(image_width);
    int32_t img_height = static_cast<int32_t>(image_height);
    out.write(DESCR_SIGNATURE, DESCR_SIGNATURE_LEN);
    out.write(reinterpret_cast<char const*>(&num_descr), sizeof(int32_t));
    out.write(reinterpret_cast<char const*>(&img_width), sizeof(int32_t));
    out.write(reinterpret_cast<char const*>(&img_height), sizeof(int32_t));

    /* Write descriptors. */
    for (std::size_t i = 0; i < descriptors.size(); ++i)
    {
        DESC const& d = descriptors[i];
        out.write(reinterpret_cast<char const*>(&d.x), sizeof(float));
        out.write(reinterpret_cast<char const*>(&d.y), sizeof(float));
        out.write(reinterpret_cast<char const*>(&d.scale), sizeof(float));
        out.write(reinterpret_cast<char const*>(&d.orientation), sizeof(float));
        out.write(reinterpret_cast<char const*>(*d.data), sizeof(float) * LEN);
    }

    return data;
}

/* --------------- De-Serialization of Descriptors ---------------- */

template <typename DESC, int LEN>
void
deserialize_descriptors (mve::ByteImage::ConstPtr data,
    std::vector<DESC>* descriptors, int* width, int* height)
{
    if (descriptors == NULL)
        throw std::invalid_argument("NULL descriptors given");

    /* Input stream from memory. */
    std::istringstream in;
    in.rdbuf()->pubsetbuf(const_cast<char*>(data->get_byte_pointer()),
        data->get_byte_size());

    /* Check signature. */
    char signature[DESCR_SIGNATURE_LEN + 1];
    in.read(signature, DESCR_SIGNATURE_LEN);
    signature[DESCR_SIGNATURE_LEN] = '\0';
    if (std::string(DESCR_SIGNATURE) != signature)
        throw std::invalid_argument("Error matching signature");

    /* Read header. */
    int32_t num_descr, img_width, img_height;
    in.read(reinterpret_cast<char*>(&num_descr), sizeof(int32_t));
    in.read(reinterpret_cast<char*>(&img_width), sizeof(int32_t));
    in.read(reinterpret_cast<char*>(&img_height), sizeof(int32_t));

    if (!in || in.eof() || num_descr < 0 || num_descr > 1000000)
        throw std::invalid_argument("Error reading header");

    /* Read descriptors. */
    descriptors->resize(num_descr);
    for (std::size_t i = 0; i < descriptors->size(); ++i)
    {
        DESC& d = descriptors->at(i);
        in.read(reinterpret_cast<char*>(&d.x), sizeof(float));
        in.read(reinterpret_cast<char*>(&d.y), sizeof(float));
        in.read(reinterpret_cast<char*>(&d.scale), sizeof(float));
        in.read(reinterpret_cast<char*>(&d.orientation), sizeof(float));
        in.read(reinterpret_cast<char*>(*d.data), sizeof(float) * LEN);
    }

    if (width != NULL)
        *width = img_width;
    if (height != NULL)
        *height = img_height;
}

/* -------------------- Descriptor conversions -------------------- */

mve::ByteImage::Ptr
descriptors_to_embedding (Sift::Descriptors const& descriptors,
    int width, int height)
{
    return serialize_descriptors<Sift::Descriptor, 128>
        (descriptors, width, height);
}

void
embedding_to_descriptors (mve::ByteImage::ConstPtr data,
    Sift::Descriptors* descriptors, int* width, int* height)
{
    deserialize_descriptors<Sift::Descriptor, 128>
        (data, descriptors, width, height);
}

mve::ByteImage::Ptr
descriptors_to_embedding (Surf::Descriptors const& descriptors,
    int width, int height)
{
    return serialize_descriptors<Surf::Descriptor, 64>
        (descriptors, width, height);
}

void
embedding_to_descriptors (mve::ByteImage::ConstPtr data,
    Surf::Descriptors* descriptors, int* width, int* height)
{
    deserialize_descriptors<Surf::Descriptor, 64>
        (data, descriptors, width, height);
}

SFM_BUNDLER_NAMESPACE_END
SFM_NAMESPACE_END
